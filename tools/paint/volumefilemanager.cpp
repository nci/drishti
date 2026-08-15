#include "global.h"
#include "volumefilemanager.h"
#include "staticfunctions.h"
#include "checkpointhandler.h"
#include "slabsavetransaction.h"

#include <QtGui>
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QFileInfo>
#include <QDir>
#include <QEventLoop>
#include <QSaveFile>
#include <QTemporaryFile>
#include <QTimer>
#include <QDateTime>

#include "blosc.h"

#include <algorithm>
#include <limits>
#include <new>
#include <utility>

namespace
{
const int kBackgroundSaveNoProgressTimeoutMs = 30*1000;
const int kThreadShutdownTimeoutMs = 30*1000;
const qint64 kOrphanSnapshotMaxAgeMs = 24LL*60LL*60LL*1000LL;

bool
usesCompressedMaskFormat(const QStringList& filenames)
{
  return !filenames.isEmpty() &&
         (StaticFunctions::checkExtension(filenames[0], ".mask.sc") ||
          StaticFunctions::checkExtension(filenames[0], ".mask"));
}

void cleanupOrphanSnapshots(const QDir& directory)
{
  if (!directory.exists())
    return;

  const QDateTime cutoff = QDateTime::currentDateTimeUtc().addMSecs(
    -kOrphanSnapshotMaxAgeMs);
  const QFileInfoList snapshots = directory.entryInfoList(
    QStringList() << ".drishti-mask-snapshot-*.tmp"
                   << ".drishti-mask-base-*.tmp",
    QDir::Files | QDir::Hidden | QDir::Readable,
    QDir::Time);
  for (int i = 0; i < snapshots.count(); ++i)
    {
      const QFileInfo& info = snapshots.at(i);
      if (info.lastModified().toUTC() < cutoff)
        (void)QFile::remove(info.absoluteFilePath());
    }
}
}

VolumeFileManager::VolumeFileManager()
{
  m_thread = 0;
  m_handler = 0;
  m_slice = 0;
  m_sliceCapacity = 0;
  m_block = 0;
  m_blockCapacity = 0;
  m_blockSlices = 10;
  m_startBlock = m_endBlock = 0;
  m_filenames.clear();
  m_volData = 0;
  m_volDataCapacity = 0;
  m_memmapped = false;
  m_memChanged = false;
  m_mcTimes = 0;
  m_saveFreq = 50;
  m_changeGeneration = 0;
  m_saveGeneration = 0;
  m_fileHandlerBusy = false;
  m_waitingOnFileHandler = false;
  m_saveRequested = false;
  m_backgroundSaveFailed = false;
  m_snapshotPath.clear();
  m_snapshotBasePath.clear();
  m_dirtySnapshotChunks.clear();
  m_saveDebounceTimer = new QTimer(this);
  m_saveDebounceTimer->setSingleShot(true);
  m_saveDebounceTimer->setInterval(350);
  connect(m_saveDebounceTimer, SIGNAL(timeout()),
          this, SLOT(beginBackgroundSave()));
  (void)reset();
}

VolumeFileManager::~VolumeFileManager()
{
  // Normal window shutdown flushes through exiting().  If destruction is
  // reached after an I/O failure, stop the worker before QObject tears down
  // its queued-signal receiver; the failed dirty buffer cannot outlive this
  // object.
  if (!stopFileHandlerThread(true))
    {
      (void)stopFileHandlerThread(false);
      m_memChanged = false;
    }
  (void)reset();
}

QString
VolumeFileManager::lastError() const
{
  return m_lastError;
}

bool
VolumeFileManager::checkedMultiply(qint64 a, qint64 b, qint64& result)
{
  if (a < 0 || b < 0)
    return false;
  if (a == 0 || b == 0)
    {
      result = 0;
      return true;
    }
  if (a > std::numeric_limits<qint64>::max()/b)
    return false;
  result = a*b;
  return true;
}

bool
VolumeFileManager::checkedAdd(qint64 a, qint64 b, qint64& result)
{
  if (a < 0 || b < 0 ||
      a > std::numeric_limits<qint64>::max()-b)
    return false;
  result = a+b;
  return true;
}

bool
VolumeFileManager::setError(const QString& error)
{
  m_lastError = error;
  return false;
}

void
VolumeFileManager::clearError()
{
  m_lastError.clear();
}

bool
VolumeFileManager::validateGeometry(const QString& operation,
                                    bool requireFilename)
{
  if (m_depth <= 0 || m_width <= 0 || m_height <= 0)
    return setError(QString("%1: invalid volume dimensions %2 x %3 x %4")
                    .arg(operation).arg(m_depth).arg(m_width).arg(m_height));
  if (m_slabSize <= 0 || m_slabSize > std::numeric_limits<int>::max())
    return setError(QString("%1: invalid slab size %2")
                    .arg(operation).arg(m_slabSize));
  if (m_header < 0)
    return setError(QString("%1: invalid header size %2")
                    .arg(operation).arg(m_header));
  if (m_voxelType < _UChar || m_voxelType > _Float ||
      (m_bytesPerVoxel != 1 &&
       m_bytesPerVoxel != 2 &&
       m_bytesPerVoxel != 4))
    return setError(QString("%1: invalid voxel type %2")
                    .arg(operation).arg(m_voxelType));
  if (requireFilename && m_filenames.isEmpty() && m_baseFilename.isEmpty())
    return setError(QString("%1: no volume filename is configured")
                    .arg(operation));

  const int slabSize = static_cast<int>(m_slabSize);
  const int slabCount = 1+(m_depth-1)/slabSize;
  if (requireFilename && m_baseFilename.isEmpty() &&
      m_filenames.count() < slabCount &&
      !(m_filenames.count() == 1 &&
        (StaticFunctions::checkExtension(m_filenames[0], ".mask") ||
         StaticFunctions::checkExtension(m_filenames[0], ".mask.sc"))))
    return setError(QString("%1: only %2 of %3 slab filenames are configured")
                    .arg(operation).arg(m_filenames.count()).arg(slabCount));
  for(int slab=0; slab<qMin(slabCount, m_filenames.count()); ++slab)
    if (m_filenames[slab].isEmpty())
      return setError(QString("%1: slab %2 has an empty filename")
                      .arg(operation).arg(slab));
  return true;
}

bool
VolumeFileManager::sliceByteCount(qint64& bytes,
                                  const QString& operation)
{
  qint64 voxels = 0;
  if (!checkedMultiply(static_cast<qint64>(m_width),
                       static_cast<qint64>(m_height), voxels) ||
      !checkedMultiply(voxels, m_bytesPerVoxel, bytes) || bytes <= 0)
    return setError(QString("%1: slice byte count overflows").arg(operation));
  return true;
}

bool
VolumeFileManager::volumeByteCount(qint64& bytes,
                                   const QString& operation)
{
  qint64 sliceBytes = 0;
  if (!sliceByteCount(sliceBytes, operation) ||
      !checkedMultiply(sliceBytes, static_cast<qint64>(m_depth), bytes) ||
      bytes <= 0)
    return setError(QString("%1: volume byte count overflows").arg(operation));
  return true;
}

bool
VolumeFileManager::planeByteCount(int firstAxis,
                                  int secondAxis,
                                  qint64& bytes,
                                  const QString& operation)
{
  qint64 voxels = 0;
  if (firstAxis <= 0 || secondAxis <= 0 ||
      !checkedMultiply(static_cast<qint64>(firstAxis),
                       static_cast<qint64>(secondAxis), voxels) ||
      !checkedMultiply(voxels, m_bytesPerVoxel, bytes) || bytes <= 0)
    return setError(QString("%1: plane byte count overflows").arg(operation));
  return true;
}

bool
VolumeFileManager::ensureSliceCapacity(qint64 bytes,
                                       const QString& operation)
{
  if (bytes <= 0 ||
      static_cast<quint64>(bytes) >
      static_cast<quint64>(std::numeric_limits<size_t>::max()))
    return setError(QString("%1: requested slice buffer is too large")
                    .arg(operation));

  const size_t requested = static_cast<size_t>(bytes);
  if (m_slice && m_sliceCapacity >= requested)
    return true;

  uchar *replacement = new (std::nothrow) uchar[requested];
  if (!replacement)
    return setError(QString("%1: cannot allocate %2-byte slice buffer")
                    .arg(operation).arg(bytes));
  delete [] m_slice;
  m_slice = replacement;
  m_sliceCapacity = requested;
  return true;
}

bool
VolumeFileManager::ensureBlockCapacity(qint64 bytes,
                                       const QString& operation)
{
  if (bytes <= 0 ||
      static_cast<quint64>(bytes) >
      static_cast<quint64>(std::numeric_limits<size_t>::max()))
    return setError(QString("%1: requested interpolation buffer is too large")
                    .arg(operation));

  const size_t requested = static_cast<size_t>(bytes);
  if (m_block && m_blockCapacity >= requested)
    return true;

  uchar *replacement = new (std::nothrow) uchar[requested];
  if (!replacement)
    return setError(QString("%1: cannot allocate %2-byte interpolation buffer")
                    .arg(operation).arg(bytes));
  delete [] m_block;
  m_block = replacement;
  m_blockCapacity = requested;
  m_startBlock = m_endBlock = 0;
  return true;
}

QString
VolumeFileManager::slabFilename(int slab) const
{
  if (slab >= 0 && slab < m_filenames.count())
    return m_filenames[slab];
  return m_baseFilename +
         QString(".%1").arg(slab+1, 3, 10, QChar('0'));
}

bool
VolumeFileManager::openSlab(int slab,
                            QIODevice::OpenMode mode,
                            const QString& operation)
{
  if (slab < 0)
    return setError(QString("%1: invalid slab index %2")
                    .arg(operation).arg(slab));
  if (m_qfile.isOpen())
    m_qfile.close();
  m_filename = slabFilename(slab);
  m_qfile.setFileName(m_filename);
  if (!m_qfile.open(mode))
    return setError(QString("%1: cannot open '%2': %3")
                    .arg(operation).arg(m_filename).arg(m_qfile.errorString()));
  return true;
}

bool
VolumeFileManager::seekFile(QFile& file,
                            qint64 offset,
                            const QString& operation)
{
  if (offset < 0 || !file.seek(offset))
    return setError(QString("%1: cannot seek '%2' to byte %3: %4")
                    .arg(operation).arg(file.fileName()).arg(offset)
                    .arg(file.errorString()));
  return true;
}

bool
VolumeFileManager::readExact(QFile& file,
                             uchar *destination,
                             qint64 bytes,
                             const QString& operation)
{
  if (!destination || bytes < 0)
    return setError(QString("%1: invalid read buffer").arg(operation));
  qint64 done = 0;
  while (done < bytes)
    {
      const qint64 count =
        file.read(reinterpret_cast<char*>(destination)+done, bytes-done);
      if (count <= 0)
        return setError(QString("%1: short read from '%2' (%3 of %4 bytes): %5")
                        .arg(operation).arg(file.fileName()).arg(done).arg(bytes)
                        .arg(file.errorString()));
      done += count;
    }
  return true;
}

bool
VolumeFileManager::writeExact(QFile& file,
                              const uchar *source,
                              qint64 bytes,
                              const QString& operation)
{
  if (!source || bytes < 0)
    return setError(QString("%1: invalid write buffer").arg(operation));
  qint64 done = 0;
  while (done < bytes)
    {
      const qint64 count =
        file.write(reinterpret_cast<const char*>(source)+done, bytes-done);
      if (count <= 0)
        return setError(QString("%1: short write to '%2' (%3 of %4 bytes): %5")
                        .arg(operation).arg(file.fileName()).arg(done).arg(bytes)
                        .arg(file.errorString()));
      done += count;
    }
  return true;
}

bool
VolumeFileManager::flushFile(QFile& file, const QString& operation)
{
  if (!file.flush())
    return setError(QString("%1: cannot flush '%2': %3")
                    .arg(operation).arg(file.fileName()).arg(file.errorString()));
  return true;
}

bool
VolumeFileManager::expectedSlabSize(int slab,
                                    qint64 sliceBytes,
                                    qint64& bytes,
                                    const QString& operation)
{
  const int slabSize = static_cast<int>(m_slabSize);
  const int slabCount = 1+(m_depth-1)/slabSize;
  if (slab < 0 || slab >= slabCount)
    return setError(QString("%1: slab index %2 is outside [0, %3)")
                    .arg(operation).arg(slab).arg(slabCount));
  const qint64 firstSlice = static_cast<qint64>(slab)*slabSize;
  const int slices = qMin(slabSize,
                          m_depth-static_cast<int>(firstSlice));
  qint64 dataBytes = 0;
  if (!checkedMultiply(static_cast<qint64>(slices), sliceBytes, dataBytes) ||
      !checkedAdd(m_header, dataBytes, bytes))
    return setError(QString("%1: slab file size overflows").arg(operation));
  return true;
}

void
VolumeFileManager::cleanupPartialFiles(const QStringList& filenames)
{
  QStringList failures;
  for(int i=0; i<filenames.count(); ++i)
    if (QFileInfo::exists(filenames[i]) && !QFile::remove(filenames[i]))
      failures << filenames[i];
  if (!failures.isEmpty())
    {
      const QString cleanupError =
        QString("cleanup failed for: %1").arg(failures.join(", "));
      if (m_lastError.isEmpty())
        m_lastError = cleanupError;
      else
        m_lastError += QString("; %1").arg(cleanupError);
    }
}

void
VolumeFileManager::configureFileHandler(FileHandler& handler)
{
  handler.setFilenameList(m_filenames);
  handler.setBaseFilename(m_baseFilename);
  handler.setDepth(m_depth);
  handler.setWidth(m_width);
  handler.setHeight(m_height);
  handler.setHeaderSize(static_cast<int>(m_header));
  handler.setSlabSize(static_cast<int>(m_slabSize));
  handler.setVoxelType(m_voxelType);
  handler.setVolData(m_volData, static_cast<qint64>(m_volDataCapacity));
}

bool
VolumeFileManager::loadCompressedMask(uchar *destination, qint64 capacity)
{
  FileHandler handler;
  configureFileHandler(handler);
  handler.setVolData(destination, capacity);
  if (!handler.loadMemFile())
    return setError(handler.lastError());
  return true;
}

bool
VolumeFileManager::saveCompressedMask(uchar *source, qint64 capacity)
{
  FileHandler handler;
  configureFileHandler(handler);
  handler.setVolData(source, capacity);
  if (!handler.saveMemFile(m_changeGeneration))
    return setError(handler.lastError());
  return true;
}

bool
VolumeFileManager::stopFileHandlerThread(bool savePending)
{
  if (m_saveDebounceTimer)
    m_saveDebounceTimer->stop();

  // saveMemFile() selects the compressed-mask or ordinary slab transaction
  // path.  Calling flushPendingChanges() directly here would encode a dirty
  // non-mask volume as a compressed mask and overwrite its first slab.
  if (savePending && m_fileHandlerBusy && !waitForBackgroundSave())
    return false;
  if (savePending && m_memChanged && m_volData && !saveMemFile())
    return false;
  if (!savePending && m_fileHandlerBusy && !waitForBackgroundSave())
    {
      // A completed background failure does not make it unsafe to stop the
      // worker as long as the dirty in-memory buffer is being preserved.
      m_memChanged = true;
    }

  if (!m_thread)
    {
      delete m_handler;
      m_handler = 0;
      m_fileHandlerBusy = false;
      m_waitingOnFileHandler = false;
      m_saveRequested = false;
      if (!m_snapshotPath.isEmpty())
        QFile::remove(m_snapshotPath);
      m_snapshotPath.clear();
      return true;
    }

  m_thread->quit();
  if (!m_thread->wait(kThreadShutdownTimeoutMs))
    return setError(
      QString("mask save worker did not stop within %1 seconds")
        .arg(kThreadShutdownTimeoutMs/1000));
  m_handler = 0;
  delete m_thread;
  m_thread = 0;
  m_fileHandlerBusy = false;
  m_waitingOnFileHandler = false;
  m_saveRequested = false;
  if (!m_snapshotPath.isEmpty())
    QFile::remove(m_snapshotPath);
  m_snapshotPath.clear();
  return true;
}

bool
VolumeFileManager::startFileHandlerThread()
{
  clearError();
  if (m_thread)
    {
      if (m_thread->isRunning() && m_handler)
        return true;
      m_handler = 0;
      delete m_thread;
      m_thread = 0;
    }
  if (!usesCompressedMaskFormat(m_filenames))
    return setError("start mask save worker: target is not a mask file");
  if (!validateGeometry("start mask save worker") || !m_volData)
    {
      if (m_lastError.isEmpty())
        setError("start mask save worker: memory volume is unavailable");
      return false;
    }

  m_thread = new (std::nothrow) QThread();
  m_handler = new (std::nothrow) FileHandler();
  if (!m_thread || !m_handler)
    {
      delete m_handler;
      delete m_thread;
      m_handler = 0;
      m_thread = 0;
      return setError("start mask save worker: cannot allocate worker objects");
    }

  configureFileHandler(*m_handler);
  connect(m_thread, SIGNAL(finished()), m_handler, SLOT(deleteLater()));
  connect(m_thread, SIGNAL(finished()),
          this, SLOT(fileHandlerThreadFinished()),
          Qt::QueuedConnection);
  connect(this, SIGNAL(saveSnapshot(QString,quint64)),
          m_handler, SLOT(saveSnapshotFile(QString,quint64)));
  connect(m_handler, SIGNAL(doneFileSave(quint64,bool,QString)),
          this, SLOT(doneFileSave(quint64,bool,QString)),
          Qt::QueuedConnection);
  connect(m_handler, SIGNAL(fileSaveProgress(quint64)),
          this, SLOT(fileSaveProgress(quint64)),
          Qt::QueuedConnection);
  m_handler->moveToThread(m_thread);
  m_thread->start();
  return true;
}

bool
VolumeFileManager::createSaveSnapshot(QString& snapshotName,
                                      quint64& generation)
{
  const QString operation = "snapshot compressed mask";
  snapshotName.clear();
  generation = 0;

  qint64 volumeBytes = 0;
  if (!validateGeometry(operation) ||
      !volumeByteCount(volumeBytes, operation) ||
      !m_volData ||
      static_cast<quint64>(volumeBytes) >
        static_cast<quint64>(m_volDataCapacity))
    {
      if (m_lastError.isEmpty())
        setError(QString("%1: memory volume is unavailable or too small")
                 .arg(operation));
      return false;
    }

  const QFileInfo targetInfo(m_filenames[0]);
  // A process crash can leave the immutable raw snapshot or its reusable
  // baseline behind. Only remove files older than a day so an active save in
  // another process is never mistaken for an orphan.
  cleanupOrphanSnapshots(targetInfo.absoluteDir());

  const qint64 copyBlockBytes = 8LL*1024LL*1024LL;
  QProgressDialog progress(QStringLiteral("Preparing mask save"),
                           QStringLiteral("Cancel"), 0, 100, 0);
  progress.setMinimumDuration(500);
  progress.setAutoClose(true);
  progress.setAutoReset(true);
  const quint64 snapshotGeneration = m_changeGeneration;
  const bool fullRefresh = m_snapshotBasePath.isEmpty() ||
    !QFileInfo::exists(m_snapshotBasePath) ||
    QFileInfo(m_snapshotBasePath).size() != volumeBytes ||
    m_dirtySnapshotChunks.contains(-1) || m_dirtySnapshotChunks.isEmpty();
  if (fullRefresh)
    {
      QString basePath = m_snapshotBasePath;
      if (basePath.isEmpty())
        {
          QTemporaryFile baseName(
            QDir(targetInfo.absolutePath()).filePath(
              ".drishti-mask-base-XXXXXX.tmp"));
          baseName.setAutoRemove(false);
          if (!baseName.open())
            return setError(QString("%1: cannot create a baseline beside '%2': %3")
                            .arg(operation).arg(m_filenames[0])
                            .arg(baseName.errorString()));
          basePath = baseName.fileName();
          baseName.close();
          QFile::remove(basePath);
        }
      QSaveFile base(basePath);
      base.setDirectWriteFallback(false);
      if (!base.open(QIODevice::WriteOnly))
        return setError(QString("%1: cannot open baseline '%2': %3")
                        .arg(operation).arg(basePath).arg(base.errorString()));
      qint64 written = 0;
      while (written < volumeBytes)
        {
          QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
          if (progress.wasCanceled())
            {
              base.cancelWriting();
              return setError(QStringLiteral(
                "mask save snapshot canceled; dirty data was preserved"));
            }
          const qint64 bytes = qMin(copyBlockBytes, volumeBytes-written);
          const qint64 count = base.write(
            reinterpret_cast<const char*>(m_volData)+written, bytes);
          if (count != bytes)
            {
              base.cancelWriting();
              return setError(QString("%1: short baseline write after %2 of %3 bytes: %4")
                              .arg(operation).arg(written).arg(volumeBytes)
                              .arg(base.errorString()));
            }
          written += count;
          progress.setValue(static_cast<int>(
            qMin<qint64>(100, (written*100)/qMax<qint64>(1, volumeBytes))));
        }
      if (!base.commit())
        return setError(QString("%1: cannot commit baseline '%2': %3")
                        .arg(operation).arg(basePath).arg(base.errorString()));
      m_snapshotBasePath = basePath;
    }
  else if (!m_dirtySnapshotChunks.isEmpty())
    {
      QFile base(m_snapshotBasePath);
      if (!base.open(QIODevice::ReadWrite))
        return setError(QString("%1: cannot update baseline '%2': %3")
                        .arg(operation).arg(m_snapshotBasePath)
                        .arg(base.errorString()));
      QList<qint64> chunks = m_dirtySnapshotChunks.values();
      std::sort(chunks.begin(), chunks.end());
      for (const qint64 chunk : chunks)
        {
          if (chunk < 0)
            continue;
          QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
          if (progress.wasCanceled())
            {
              base.close();
              return setError(QStringLiteral(
                "mask save snapshot canceled; dirty data was preserved"));
            }
          const qint64 offset = chunk*copyBlockBytes;
          if (offset < 0 || offset >= volumeBytes ||
              !base.seek(offset))
            {
              const QString detail = base.errorString();
              base.close();
              return setError(QString("%1: cannot seek baseline chunk %2: %3")
                              .arg(operation).arg(chunk).arg(detail));
            }
          const qint64 bytes = qMin(copyBlockBytes, volumeBytes-offset);
          if (base.write(reinterpret_cast<const char*>(m_volData)+offset,
                         bytes) != bytes)
            {
              const QString detail = base.errorString();
              base.close();
              return setError(QString("%1: cannot update baseline chunk %2: %3")
                              .arg(operation).arg(chunk).arg(detail));
            }
          progress.setValue(static_cast<int>(
            qMin<qint64>(100, ((chunk+1)*100)/
                         qMax<qint64>(1, 1+(volumeBytes-1)/copyBlockBytes))));
        }
      if (!base.flush())
        {
          const QString detail = base.errorString();
          base.close();
          return setError(QString("%1: cannot flush baseline: %2")
                          .arg(operation).arg(detail));
        }
      base.close();
    }

  // The baseline is now complete. Make an immutable generation for the
  // worker, so later edits cannot race its compressor.
  QTemporaryFile snapshotNameTemplate(
    QDir(targetInfo.absolutePath()).filePath(
      ".drishti-mask-snapshot-XXXXXX.tmp"));
  snapshotNameTemplate.setAutoRemove(false);
  if (!snapshotNameTemplate.open())
    return setError(QString("%1: cannot create a snapshot beside '%2': %3")
                    .arg(operation).arg(m_filenames[0])
                    .arg(snapshotNameTemplate.errorString()));
  snapshotName = snapshotNameTemplate.fileName();
  snapshotNameTemplate.close();
  QFile::remove(snapshotName);
  QFile sourceSnapshot(m_snapshotBasePath);
  QFile materializedSnapshot(snapshotName);
  bool materialized = sourceSnapshot.open(QIODevice::ReadOnly) &&
                      materializedSnapshot.open(QIODevice::WriteOnly |
                                                QIODevice::Truncate);
  qint64 copied = 0;
  const qint64 copyBufferBytes = 8LL*1024LL*1024LL;
  QByteArray copyBuffer;
  if (materialized)
    copyBuffer.resize(static_cast<int>(qMin(copyBufferBytes,
                                            qMax<qint64>(1, volumeBytes))));
  while (materialized && copied < volumeBytes)
    {
      const qint64 requested = qMin(copyBufferBytes, volumeBytes-copied);
      const qint64 got = sourceSnapshot.read(copyBuffer.data(), requested);
      if (got != requested)
        {
          materialized = false;
          break;
        }
      if (materializedSnapshot.write(copyBuffer.constData(), got) != got)
        {
          materialized = false;
          break;
        }
      copied += got;
    }
  char trailingByte = 0;
  if (materialized &&
      (copied != volumeBytes || sourceSnapshot.read(&trailingByte, 1) != 0 ||
       !materializedSnapshot.flush()))
    materialized = false;
  sourceSnapshot.close();
  materializedSnapshot.close();
  if (!materialized || QFileInfo(snapshotName).size() != volumeBytes)
    {
      const QString detail = QString("source '%1' (%2 bytes), destination '%3' (%4 bytes), "
                                     "source error '%5', destination error '%6'")
                               .arg(m_snapshotBasePath)
                               .arg(QFileInfo(m_snapshotBasePath).size())
                               .arg(snapshotName)
                               .arg(QFileInfo(snapshotName).size())
                               .arg(sourceSnapshot.errorString())
                               .arg(materializedSnapshot.errorString());
      QFile::remove(snapshotName);
      snapshotName.clear();
      return setError(QString("%1: cannot materialize immutable snapshot: %2")
                      .arg(operation).arg(detail));
    }

  // If no edits arrived while the chunks were copied, the baseline is now
  // current and future saves can update only dirty chunks.  When edits did
  // arrive, retain the dirty set so the next generation refreshes them.
  if (snapshotGeneration == m_changeGeneration)
    m_dirtySnapshotChunks.clear();

  generation = snapshotGeneration;
  return true;
}

bool
VolumeFileManager::queueFileSave()
{
  if (!m_memChanged)
    return true;
  if (!usesCompressedMaskFormat(m_filenames))
    return setError("queue mask save: target is not a mask file");
  if (m_fileHandlerBusy)
    {
      m_saveRequested = true;
      return true;
    }
  if (m_backgroundSaveFailed)
    return setError(m_lastError.isEmpty() ?
                    QString("the previous background mask save failed") :
                    m_lastError);
  if (!m_handler || !m_thread || !m_thread->isRunning())
    if (!startFileHandlerThread())
      return false;

  QString snapshotName;
  quint64 generation = 0;
  if (!createSaveSnapshot(snapshotName, generation))
    return false;

  m_snapshotPath = snapshotName;
  m_saveGeneration = generation;
  m_fileHandlerBusy = true;
  m_saveRequested = false;
  emit saveSnapshot(snapshotName, generation);
  return true;
}

bool
VolumeFileManager::requestSave()
{
  if (!m_memChanged)
    return true;
  if (!usesCompressedMaskFormat(m_filenames))
    return saveMemFile();
  if (m_backgroundSaveFailed)
    return setError(m_lastError.isEmpty() ?
                    QString("the previous background mask save failed") :
                    m_lastError);

  m_saveRequested = true;
  if (!m_fileHandlerBusy && m_saveDebounceTimer)
    m_saveDebounceTimer->start();
  return true;
}

void
VolumeFileManager::beginBackgroundSave()
{
  if (!m_memChanged || m_fileHandlerBusy)
    return;
  if (!queueFileSave())
    {
      m_saveRequested = false;
      m_backgroundSaveFailed = true;
      emit saveCycleFinished();
    }
}

bool
VolumeFileManager::waitForBackgroundSave()
{
  while (m_fileHandlerBusy)
    {
      QEventLoop waitLoop;
      QTimer timeout;
      timeout.setSingleShot(true);
      connect(this, SIGNAL(saveCycleFinished()),
              &waitLoop, SLOT(quit()));
      connect(this, SIGNAL(saveCycleProgressed()),
              &waitLoop, SLOT(quit()));
      connect(&timeout, SIGNAL(timeout()),
              &waitLoop, SLOT(quit()));
      m_waitingOnFileHandler = true;
      if (m_fileHandlerBusy)
        {
          timeout.start(kBackgroundSaveNoProgressTimeoutMs);
          waitLoop.exec(QEventLoop::ExcludeUserInputEvents);
        }
      m_waitingOnFileHandler = false;
      if (m_fileHandlerBusy && !timeout.isActive())
        {
          if (m_handler)
            m_handler->requestCancel();
          m_memChanged = true;
          m_backgroundSaveFailed = true;
          return setError(
            QString("mask save made no progress for %1 seconds; "
                    "the dirty in-memory mask has been preserved")
              .arg(kBackgroundSaveNoProgressTimeoutMs/1000));
        }
    }
  return !m_backgroundSaveFailed;
}

bool
VolumeFileManager::flushPendingChanges()
{
  if (m_saveDebounceTimer)
    m_saveDebounceTimer->stop();
  m_saveRequested = false;

  for(;;)
    {
      if (m_fileHandlerBusy && !waitForBackgroundSave())
        return false;
      if (!m_memChanged)
        return true;

      // An explicit save is also the retry path after an automatic save
      // failed.  Keep the stable error until a new attempt is started.
      m_backgroundSaveFailed = false;
      clearError();
      if (!queueFileSave())
        {
          m_backgroundSaveFailed = true;
          return false;
        }
      if (!waitForBackgroundSave())
        return false;

      if (m_saveDebounceTimer)
        m_saveDebounceTimer->stop();
      m_saveRequested = false;
    }
}

void
VolumeFileManager::markChanged()
{
  m_memChanged = true;
  ++m_changeGeneration;
  if (m_changeGeneration == 0)
    ++m_changeGeneration;
  // -1 means that the complete baseline must be refreshed.  Callers that
  // know an exact byte range can use markChangedRange() instead.
  m_dirtySnapshotChunks.clear();
  m_dirtySnapshotChunks.insert(-1);
}

void
VolumeFileManager::markChangedRange(qint64 offset, qint64 bytes)
{
  m_memChanged = true;
  ++m_changeGeneration;
  if (m_changeGeneration == 0)
    ++m_changeGeneration;
  const qint64 chunkBytes = 8LL*1024LL*1024LL;
  if (offset < 0 || bytes <= 0 ||
      offset > std::numeric_limits<qint64>::max()-bytes)
    {
      m_dirtySnapshotChunks.clear();
      m_dirtySnapshotChunks.insert(-1);
      return;
    }
  if (m_dirtySnapshotChunks.contains(-1))
    return;
  const qint64 first = offset/chunkBytes;
  const qint64 last = (offset+bytes-1)/chunkBytes;
  for (qint64 chunk = first; chunk <= last; ++chunk)
    {
      m_dirtySnapshotChunks.insert(chunk);
      if (chunk == std::numeric_limits<qint64>::max())
        break;
    }
}

void
VolumeFileManager::discardSnapshotBaseline()
{
  if (!m_snapshotBasePath.isEmpty())
    QFile::remove(m_snapshotBasePath);
  m_snapshotBasePath.clear();
  m_dirtySnapshotChunks.clear();
}

bool
VolumeFileManager::loadCheckPoint()
{
  clearError();
  if (m_filenames.isEmpty() || !m_volData)
    return setError("load checkpoint: mask volume is unavailable");
  QString cflnm = m_filenames[0];
  if (StaticFunctions::checkExtension(cflnm, "mask.sc"))
    cflnm.chop(3);
  cflnm += ".checkpoint";

  return loadCheckPoint(cflnm);
}
bool
VolumeFileManager::loadCheckPoint(QString flnm)
{
  clearError();
  if (flnm.isEmpty() || !m_volData)
    return setError("load checkpoint: mask volume is unavailable");
  if (!stopFileHandlerThread(true))
    return false;
  if (!CheckpointHandler::loadCheckpoint(flnm,
					 m_voxelType,
					 m_depth, m_width, m_height,
					 m_volData))
    {
      const QString error = CheckpointHandler::lastError();
      if (!error.isEmpty())
        setError(error);
      return false;
    }
  markChanged();
  return true;
}
bool
VolumeFileManager::deleteCheckPoint()
{
  clearError();
  if (m_filenames.isEmpty() || !m_volData)
    return setError("delete checkpoint: mask volume is unavailable");
  QString cflnm = m_filenames[0];
  if (StaticFunctions::checkExtension(cflnm, "mask.sc"))
    cflnm.chop(3);
  cflnm += ".checkpoint";

  if (!CheckpointHandler::deleteCheckpoint(cflnm,
					    m_voxelType,
					    m_depth, m_width, m_height,
					    m_volData))
    {
      const QString error = CheckpointHandler::lastError();
      if (!error.isEmpty())
        setError(error);
      return false;
    }
  return true;
}

void
VolumeFileManager::checkPoint()
{
  clearError();
  if (m_filenames.isEmpty() || !m_volData)
    {
      setError("save checkpoint: mask volume is unavailable");
      return;
    }
  QString cflnm = m_filenames[0];
  if (StaticFunctions::checkExtension(cflnm, "mask.sc"))
    cflnm.chop(3);
  cflnm += ".checkpoint";
  bool ok;
  QString desc = QInputDialog::getText(0,
				       "Checkpoint",
				       "Description",
				       QLineEdit::Normal,
				       "",
				       &ok);
  desc = desc.trimmed();
  if (!ok)
    return;
  if (desc.isEmpty())
    {
      QMessageBox::information(0, "Checkpoint", "Empty description not allowed - checkpoint not saved\nPlease try again.");
      return;
    }
  
  if (!CheckpointHandler::saveCheckpoint(cflnm,
					 m_voxelType,
					 m_depth, m_width, m_height,
					 m_volData,
					 desc))
    {
      const QString error = CheckpointHandler::lastError();
      if (!error.isEmpty())
        setError(error);
    }
  return;
}

bool VolumeFileManager::setMemMapped(bool b)
{
  if (!stopFileHandlerThread(true))
    return false;
  if (m_saveDebounceTimer)
    m_saveDebounceTimer->stop();
  m_memmapped = b;

  if (m_volData)
    delete [] m_volData;
  m_volData = 0;
  m_volDataCapacity = 0;

  m_memChanged = false;
  m_mcTimes = 0;
  m_saveDSlices.clear();
  m_saveWSlices.clear();
  m_saveHSlices.clear();
  discardSnapshotBaseline();
  m_changeGeneration = 0;
  m_saveGeneration = 0;
  return true;
}

bool VolumeFileManager::isMemMapped() { return m_memmapped; }

void VolumeFileManager::setMemChanged(bool b)
{
  if (b)
    markChanged();
  else
    m_memChanged = false;
}

bool
VolumeFileManager::reset()
{
  if (m_saveDebounceTimer)
    m_saveDebounceTimer->stop();
  if (!stopFileHandlerThread(true))
    return false;

  m_baseFilename.clear();
  m_filenames.clear();
  m_header = m_slabSize = 0;
  m_depth = m_width = m_height = 0;
  m_voxelType = _UChar;
  m_bytesPerVoxel = 1;
  m_lastError.clear();

  m_filename.clear();
  m_slabno = m_prevslabno = -1;

  if (m_slice)
    delete [] m_slice;
  m_slice = 0;
  m_sliceCapacity = 0;

  if (m_block)
    delete [] m_block;
  m_block = 0;
  m_blockCapacity = 0;
  m_startBlock = m_endBlock = 0;

  if (m_volData)
    delete [] m_volData;
  m_volData = 0;
  m_volDataCapacity = 0;

  if (m_qfile.isOpen())
    m_qfile.close();

  m_memmapped = false;
  m_memChanged = false;
  m_mcTimes = 0;
  m_saveDSlices.clear();
  m_saveWSlices.clear();
  m_saveHSlices.clear();

  m_fileHandlerBusy = false;
  m_waitingOnFileHandler = false;
  m_saveRequested = false;
  m_backgroundSaveFailed = false;
  if (!m_snapshotPath.isEmpty())
    QFile::remove(m_snapshotPath);
  m_snapshotPath.clear();
  if (!m_snapshotBasePath.isEmpty())
    QFile::remove(m_snapshotBasePath);
  m_snapshotBasePath.clear();
  m_dirtySnapshotChunks.clear();
  m_changeGeneration = 0;
  m_saveGeneration = 0;
  return true;
}

bool
VolumeFileManager::prepareForStateSwap()
{
  return stopFileHandlerThread(true);
}

bool
VolumeFileManager::swapState(VolumeFileManager& other)
{
  if (this == &other)
    return true;
  if (m_thread || m_handler || other.m_thread || other.m_handler)
    {
      m_lastError = "Cannot swap volume state while a save worker is running";
      return false;
    }

  m_qfile.close();
  other.m_qfile.close();
  using std::swap;
  swap(m_fileHandlerBusy, other.m_fileHandlerBusy);
  swap(m_waitingOnFileHandler, other.m_waitingOnFileHandler);
  swap(m_saveRequested, other.m_saveRequested);
  swap(m_backgroundSaveFailed, other.m_backgroundSaveFailed);
  swap(m_memmapped, other.m_memmapped);
  swap(m_memChanged, other.m_memChanged);
  swap(m_saveFreq, other.m_saveFreq);
  swap(m_mcTimes, other.m_mcTimes);
  swap(m_baseFilename, other.m_baseFilename);
  swap(m_filenames, other.m_filenames);
  swap(m_header, other.m_header);
  swap(m_slabSize, other.m_slabSize);
  swap(m_depth, other.m_depth);
  swap(m_width, other.m_width);
  swap(m_height, other.m_height);
  swap(m_voxelType, other.m_voxelType);
  swap(m_bytesPerVoxel, other.m_bytesPerVoxel);
  swap(m_slice, other.m_slice);
  swap(m_sliceCapacity, other.m_sliceCapacity);
  swap(m_block, other.m_block);
  swap(m_blockCapacity, other.m_blockCapacity);
  swap(m_blockSlices, other.m_blockSlices);
  swap(m_startBlock, other.m_startBlock);
  swap(m_endBlock, other.m_endBlock);
  swap(m_filename, other.m_filename);
  swap(m_slabno, other.m_slabno);
  swap(m_prevslabno, other.m_prevslabno);
  swap(m_volData, other.m_volData);
  swap(m_volDataCapacity, other.m_volDataCapacity);
  swap(m_lastError, other.m_lastError);
  swap(m_changeGeneration, other.m_changeGeneration);
  swap(m_saveGeneration, other.m_saveGeneration);
  swap(m_snapshotPath, other.m_snapshotPath);
  swap(m_snapshotBasePath, other.m_snapshotBasePath);
  swap(m_dirtySnapshotChunks, other.m_dirtySnapshotChunks);
  swap(m_saveDSlices, other.m_saveDSlices);
  swap(m_saveWSlices, other.m_saveWSlices);
  swap(m_saveHSlices, other.m_saveHSlices);
  return true;
}

int VolumeFileManager::depth() { return m_depth; }
int VolumeFileManager::width() { return m_width; }
int VolumeFileManager::height() { return m_height; }

void VolumeFileManager::setFilenameList(QStringList flist) { m_filenames = flist; }
void VolumeFileManager::setBaseFilename(QString bfn) { m_baseFilename = bfn; }
void VolumeFileManager::setDepth(int d) { m_depth = d; }
void VolumeFileManager::setWidth(int w) { m_width = w; }
void VolumeFileManager::setHeight(int h) { m_height = h; }
void VolumeFileManager::setHeaderSize(int hs) { m_header = hs; }
void VolumeFileManager::setSlabSize(int ss) { m_slabSize = ss; }
void VolumeFileManager::setVoxelType(int vt)
{
  m_voxelType = vt;
  m_bytesPerVoxel = 0;
  if (m_voxelType == _UChar || m_voxelType == _Char)
    m_bytesPerVoxel = 1;
  else if (m_voxelType == _UShort || m_voxelType == _Short)
    m_bytesPerVoxel = 2;
  else if (m_voxelType == _Int || m_voxelType == _Float)
    m_bytesPerVoxel = 4;
}

int VolumeFileManager::bytesPerVoxel() { return m_bytesPerVoxel; }

QStringList VolumeFileManager::filenameList() { return m_filenames; }
QString VolumeFileManager::baseFilename() { return m_baseFilename; }
int VolumeFileManager::headerSize() { return m_header; }
int VolumeFileManager::slabSize() { return m_slabSize; }
QString VolumeFileManager::fileName() { return m_filename; }

void
VolumeFileManager::removeFile()
{
  clearError();
  if (!validateGeometry("remove volume files"))
    return;
  if (!stopFileHandlerThread(true))
    return;

  const int nslabs = 1+(m_depth-1)/static_cast<int>(m_slabSize);
  const QString firstFilename = slabFilename(0);
  if (!StaticFunctions::checkExtension(firstFilename, ".mask.sc") &&
      !StaticFunctions::checkExtension(firstFilename, ".mask"))
    {
      QStringList targets;
      for(int slab=0; slab<nslabs; ++slab)
        targets.append(slabFilename(slab));
      QString transactionError;
      if (!SlabSaveTransaction::recover(targets, &transactionError))
        {
          setError(QString("remove volume files: %1").arg(transactionError));
          return;
        }
    }
  for(int ns=0; ns<nslabs; ns++)
    {
      m_filename = slabFilename(ns);
      QFile::remove(m_filename);
    }

  m_memChanged = false;
  reset();
}

int VolumeFileManager::voxelType() { return m_voxelType; }

int
VolumeFileManager::readVoxelType()
{
  clearError();
  uchar vt = 0;
  if (m_qfile.isOpen())
    {
      if (!m_qfile.isReadable() ||
          !seekFile(m_qfile, 0, "read voxel type") ||
          !readExact(m_qfile, &vt, 1, "read voxel type"))
        {
          m_qfile.close();
          return -1;
        }
      m_qfile.close();
    }
  else
    {
      if (m_filenames.count() > 0)
	m_qfile.setFileName(m_filenames[0]);
      else
	m_qfile.setFileName(m_baseFilename + ".001");

      if (!m_qfile.open(QFile::ReadOnly))
        {
          setError(QString("read voxel type: cannot open '%1': %2")
                   .arg(m_qfile.fileName()).arg(m_qfile.errorString()));
          return -1;
        }
      if (!readExact(m_qfile, &vt, 1, "read voxel type"))
        {
          m_qfile.close();
          return -1;
        }
      m_qfile.close();
    }
  return vt;
}

bool
VolumeFileManager::exists()
{
  clearError();
  if (m_qfile.isOpen())
    m_qfile.close();
  if (!validateGeometry("check volume files"))
    return false;

  qint64 bps = 0;
  if (!sliceByteCount(bps, "check volume files"))
    return false;
  const int slabSize = static_cast<int>(m_slabSize);
  const int nslabs = 1+(m_depth-1)/slabSize;

  const QString firstFilename = slabFilename(0);
  const bool compressedMask =
    StaticFunctions::checkExtension(firstFilename, ".mask.sc") ||
    StaticFunctions::checkExtension(firstFilename, ".mask");
  if (!compressedMask)
    {
      QStringList targets;
      for(int slab=0; slab<nslabs; ++slab)
        targets.append(slabFilename(slab));
      QString transactionError;
      if (!SlabSaveTransaction::recover(targets, &transactionError))
        return setError(QString("check volume files: %1")
                          .arg(transactionError));
    }

  for(int ns=0; ns<nslabs; ns++)
    {
      m_filename = slabFilename(ns);

      m_qfile.setFileName(m_filename);

      if (StaticFunctions::checkExtension(m_filename, ".mask.sc"))
	{
	  if (!m_qfile.exists() || m_qfile.size() < 27)
            return setError(QString("check compressed mask: '%1' is missing or too small")
                            .arg(m_filename));
	  return true;
	}
      
      if (StaticFunctions::checkExtension(m_filename, ".mask"))
	{ // check for .mask.sc instead
	  QString mflnm = m_filename;
	  mflnm += ".sc";

	  QStringList mflnms;
	  mflnms << mflnm;
	  setFilenameList(mflnms);

	  m_qfile.setFileName(mflnm);
	  if (!m_qfile.exists() || m_qfile.size() < 27)
            return setError(QString("check compressed mask: '%1' is missing or too small")
                            .arg(mflnm));
	  return true;
	}

      qint64 expectedSize = 0;
      if (!expectedSlabSize(ns, bps, expectedSize, "check volume files"))
        return false;
      if (!m_qfile.exists() || m_qfile.size() != expectedSize)
        return setError(QString("check volume files: '%1' has %2 bytes, expected %3")
                        .arg(m_filename).arg(m_qfile.size()).arg(expectedSize));
    }

  return true;
}

bool
VolumeFileManager::createFile(bool writeHeader, bool writeData)
{
  const QString operation = "create volume files";
  clearError();
  if (!validateGeometry(operation))
    return false;

  qint64 bps = 0;
  qint64 volumeBytes = 0;
  if (!sliceByteCount(bps, operation) ||
      !volumeByteCount(volumeBytes, operation))
    return false;

  //----------------------
  if (!m_filenames.isEmpty() &&
      StaticFunctions::checkExtension(m_filenames[0], ".mask.sc"))
    {
      QString mflnm = m_filenames[0];
      mflnm.chop(7);
      mflnm += "mask";

      m_qfile.setFileName(mflnm);

      // load .mask and save to .mask.sc file
      if (m_qfile.exists() &&
	  m_qfile.size() == m_header+volumeBytes)
	{
	  QProgressDialog progress(QString("Loading %1").	\
				   arg(mflnm),
				   "Cancel",
				   0, 100,
				   0,
				   Qt::WindowStaysOnTopHint);
	  progress.setMinimumDuration(0);
	  progress.setCancelButton(0);
	  
	  if (!createMemFile())
            return false;
	  if (!m_qfile.open(QFile::ReadOnly) ||
              !seekFile(m_qfile, m_header, operation) ||
              !readExact(m_qfile, m_volData, volumeBytes, operation))
            {
              if (m_qfile.isOpen())
                m_qfile.close();
              delete [] m_volData;
              m_volData = 0;
              m_volDataCapacity = 0;
              return false;
            }
	  m_qfile.close();
	  progress.setValue(100);

	  markChanged();
          if (!saveCompressedMask(m_volData,
                                  static_cast<qint64>(m_volDataCapacity)))
            return false;
	  m_memChanged = false;

	  // The caller removes the legacy mask only after its sidecar commits.
	  return true;
	}
    }
  //----------------------
  
  // .mask file does not exist, just proceed to create .mask.sc file

  if (writeData &&
      (!ensureSliceCapacity(bps, operation)))
    return false;
  if (writeData)
    memset(m_slice, 0, static_cast<size_t>(bps));

  m_slabno = m_prevslabno = -1;


  if (m_memmapped)
    {
      if (!createMemFile())
        return false;
      markChanged();
      if (!saveCompressedMask(m_volData,
                              static_cast<qint64>(m_volDataCapacity)))
        return false;
      m_memChanged = false;
      return true;
    }

  const int slabSize = static_cast<int>(m_slabSize);
  const int nslabs = 1+(m_depth-1)/slabSize;
  
  uchar vt;
  if (m_voxelType == _UChar) vt = 0; // unsigned byte
  if (m_voxelType == _Char) vt = 1; // signed byte
  if (m_voxelType == _UShort) vt = 2; // unsigned short
  if (m_voxelType == _Short) vt = 3; // signed short
  if (m_voxelType == _Int) vt = 4; // int
  if (m_voxelType == _Float) vt = 8; // float

  if (writeHeader)
    m_header = 13;

  QStringList touchedFiles;

  QProgressDialog progress(QString("Allocating space for\n%1\non disk").\
			   arg(m_baseFilename),
			   "Cancel",
			   0, 100,
			   0);
  progress.setMinimumDuration(0);
  progress.setCancelButton(0);

  for(int ns=0; ns<nslabs; ns++)
    {
      m_filename = slabFilename(ns);

      progress.setLabelText(m_filename);
      qApp->processEvents();

      if (m_qfile.isOpen())
	m_qfile.close();

      m_qfile.setFileName(m_filename);
      if (!m_qfile.open(QFile::WriteOnly | QFile::Truncate))
        {
          setError(QString("%1: cannot open '%2': %3")
                   .arg(operation).arg(m_filename).arg(m_qfile.errorString()));
          break;
        }
      touchedFiles << m_filename;

      const qint64 firstSlice = static_cast<qint64>(ns)*slabSize;
      const int nslices = qMin(slabSize,
                               m_depth-static_cast<int>(firstSlice));
      bool ok = true;
      if (writeHeader)
	{
	  const qint32 fileSlices = nslices;
          const qint32 fileWidth = m_width;
          const qint32 fileHeight = m_height;
	  ok = writeExact(m_qfile, &vt, 1, operation) &&
               writeExact(m_qfile,
                          reinterpret_cast<const uchar*>(&fileSlices), 4,
                          operation) &&
               writeExact(m_qfile,
                          reinterpret_cast<const uchar*>(&fileWidth), 4,
                          operation) &&
               writeExact(m_qfile,
                          reinterpret_cast<const uchar*>(&fileHeight), 4,
                          operation);
	}

      progress.setValue(10);

      if (ok && writeData)
	{
	  for(int t=0; t<nslices && ok; t++)
	    {
	      ok = writeExact(m_qfile, m_slice, bps, operation);
	      progress.setValue((int)(100*(float)t/(float)nslices));
	      if (qApp)
                qApp->processEvents();
	    }
	}
      if (ok)
        ok = flushFile(m_qfile, operation);
      m_qfile.close();
      if (!ok)
        break;
    }

  if (!m_lastError.isEmpty())
    {
      if (m_qfile.isOpen())
        m_qfile.close();
      cleanupPartialFiles(touchedFiles);
      return false;
    }

  progress.setValue(100);
//
//  if (m_memmapped)
//    createMemFile();
  return true;
}

uchar*
VolumeFileManager::getSlice(int d)
{
  const QString operation = "read depth slice";
  clearError();
  if (!validateGeometry(operation) || d < 0 || d >= m_depth)
    {
      if (m_lastError.isEmpty())
        setError(QString("%1: depth index %2 is outside [0, %3)")
                 .arg(operation).arg(d).arg(m_depth));
      return 0;
    }

  qint64 bps = 0;
  if (!sliceByteCount(bps, operation) ||
      !ensureSliceCapacity(bps, operation))
    return 0;
  memset(m_slice, 0, static_cast<size_t>(bps));

  const int slabSize = static_cast<int>(m_slabSize);
  m_slabno = d/slabSize;
  qint64 localOffset = 0;
  qint64 fileOffset = 0;
  qint64 expectedSize = 0;
  if (!checkedMultiply(static_cast<qint64>(d-m_slabno*slabSize),
                       bps, localOffset) ||
      !checkedAdd(m_header, localOffset, fileOffset) ||
      !expectedSlabSize(m_slabno, bps, expectedSize, operation))
    return 0;

  const bool ok = openSlab(m_slabno, QFile::ReadOnly, operation) &&
                  m_qfile.size() == expectedSize &&
                  seekFile(m_qfile, fileOffset, operation) &&
                  readExact(m_qfile, m_slice, bps, operation);
  if (!ok && m_lastError.isEmpty())
    setError(QString("%1: '%2' has an unexpected size")
             .arg(operation).arg(m_filename));
  m_qfile.close();
  if (!ok)
    {
      memset(m_slice, 0, static_cast<size_t>(bps));
      return 0;
    }
  return m_slice;
}

uchar*
VolumeFileManager::getWidthSlice(int w)
{
  const QString operation = "read width slice";
  clearError();
  if (!validateGeometry(operation) || w < 0 || w >= m_width)
    {
      if (m_lastError.isEmpty())
        setError(QString("%1: width index %2 is outside [0, %3)")
                 .arg(operation).arg(w).arg(m_width));
      return 0;
    }

  qint64 bps = 0;
  qint64 rowBytes = 0;
  qint64 planeBytes = 0;
  if (!sliceByteCount(bps, operation) ||
      !checkedMultiply(static_cast<qint64>(m_height),
                       m_bytesPerVoxel, rowBytes) ||
      !planeByteCount(m_depth, m_height, planeBytes, operation) ||
      !ensureSliceCapacity(planeBytes, operation))
    return 0;
  memset(m_slice, 0, static_cast<size_t>(planeBytes));

  const int slabSize = static_cast<int>(m_slabSize);
  int previousSlab = -1;
  for(int d=0; d<m_depth; d++)
    {
      const int slab = d/slabSize;
      if (previousSlab != slab)
	{
	  qint64 expectedSize = 0;
          if (!openSlab(slab, QFile::ReadOnly, operation) ||
              !expectedSlabSize(slab, bps, expectedSize, operation) ||
              m_qfile.size() != expectedSize)
            {
              if (m_lastError.isEmpty())
                setError(QString("%1: '%2' has an unexpected size")
                         .arg(operation).arg(m_filename));
              m_qfile.close();
              memset(m_slice, 0, static_cast<size_t>(planeBytes));
              return 0;
            }
	  previousSlab = slab;
	}

      qint64 sliceOffset = 0;
      qint64 widthOffset = 0;
      qint64 fileOffset = 0;
      qint64 outputOffset = 0;
      if (!checkedMultiply(static_cast<qint64>(d-slab*slabSize),
                           bps, sliceOffset) ||
          !checkedMultiply(static_cast<qint64>(w), rowBytes, widthOffset) ||
          !checkedAdd(sliceOffset, widthOffset, fileOffset) ||
          !checkedAdd(m_header, fileOffset, fileOffset) ||
          !checkedMultiply(static_cast<qint64>(d), rowBytes, outputOffset) ||
          !seekFile(m_qfile, fileOffset, operation) ||
          !readExact(m_qfile, m_slice+outputOffset, rowBytes, operation))
        {
          if (m_lastError.isEmpty())
            setError(QString("%1: byte offset overflows").arg(operation));
          m_qfile.close();
          memset(m_slice, 0, static_cast<size_t>(planeBytes));
          return 0;
        }
    }
  m_qfile.close();
  return m_slice;
}

uchar*
VolumeFileManager::getHeightSlice(int h)
{
  const QString operation = "read height slice";
  clearError();
  if (!validateGeometry(operation) || h < 0 || h >= m_height)
    {
      if (m_lastError.isEmpty())
        setError(QString("%1: height index %2 is outside [0, %3)")
                 .arg(operation).arg(h).arg(m_height));
      return 0;
    }

  qint64 bps = 0;
  qint64 planeBytes = 0;
  if (!sliceByteCount(bps, operation) ||
      !planeByteCount(m_depth, m_width, planeBytes, operation) ||
      !ensureSliceCapacity(planeBytes, operation))
    return 0;
  memset(m_slice, 0, static_cast<size_t>(planeBytes));

  if (static_cast<quint64>(bps) >
      static_cast<quint64>(std::numeric_limits<size_t>::max()))
    return setError(QString("%1: scratch buffer is too large").arg(operation)),
           static_cast<uchar*>(0);
  uchar *depthSlice = new (std::nothrow) uchar[static_cast<size_t>(bps)];
  if (!depthSlice)
    return setError(QString("%1: cannot allocate depth-slice scratch buffer")
                    .arg(operation)), static_cast<uchar*>(0);

  const int slabSize = static_cast<int>(m_slabSize);
  int previousSlab = -1;
  for(int d=0; d<m_depth; d++)
    {
      const int slab = d/slabSize;
      if (previousSlab != slab)
	{
	  qint64 expectedSize = 0;
          if (!openSlab(slab, QFile::ReadOnly, operation) ||
              !expectedSlabSize(slab, bps, expectedSize, operation) ||
              m_qfile.size() != expectedSize)
            {
              if (m_lastError.isEmpty())
                setError(QString("%1: '%2' has an unexpected size")
                         .arg(operation).arg(m_filename));
              delete [] depthSlice;
              m_qfile.close();
              memset(m_slice, 0, static_cast<size_t>(planeBytes));
              return 0;
            }
	  previousSlab = slab;
	}

      qint64 sliceOffset = 0;
      qint64 fileOffset = 0;
      if (!checkedMultiply(static_cast<qint64>(d-slab*slabSize),
                           bps, sliceOffset) ||
          !checkedAdd(m_header, sliceOffset, fileOffset) ||
          !seekFile(m_qfile, fileOffset, operation) ||
          !readExact(m_qfile, depthSlice, bps, operation))
        {
          if (m_lastError.isEmpty())
            setError(QString("%1: byte offset overflows").arg(operation));
          delete [] depthSlice;
          m_qfile.close();
          memset(m_slice, 0, static_cast<size_t>(planeBytes));
          return 0;
        }

      for(int w=0; w<m_width; ++w)
        {
          const qint64 sourceVoxel = static_cast<qint64>(w)*m_height+h;
          const qint64 outputVoxel = static_cast<qint64>(d)*m_width+w;
          memcpy(m_slice+outputVoxel*m_bytesPerVoxel,
                 depthSlice+sourceVoxel*m_bytesPerVoxel,
                 static_cast<size_t>(m_bytesPerVoxel));
        }
    }
  m_qfile.close();
  delete [] depthSlice;
  return m_slice;
}

bool
VolumeFileManager::setSlice(int d, uchar *tmp)
{
  const QString operation = "write depth slice";
  clearError();
  if (!tmp)
    return setError(QString("%1: source buffer is null").arg(operation));
  if (!validateGeometry(operation) || d < 0 || d >= m_depth)
    {
      if (m_lastError.isEmpty())
        setError(QString("%1: depth index %2 is outside [0, %3)")
                 .arg(operation).arg(d).arg(m_depth));
      return false;
    }

  qint64 bps = 0;
  if (!sliceByteCount(bps, operation))
    return false;
  const int slabSize = static_cast<int>(m_slabSize);
  m_slabno = d/slabSize;
  qint64 localOffset = 0;
  qint64 fileOffset = 0;
  qint64 endOffset = 0;
  qint64 maximumSize = 0;
  if (!checkedMultiply(static_cast<qint64>(d-m_slabno*slabSize),
                       bps, localOffset) ||
      !checkedAdd(m_header, localOffset, fileOffset) ||
      !checkedAdd(fileOffset, bps, endOffset) ||
      !expectedSlabSize(m_slabno, bps, maximumSize, operation))
    return false;

  if (!openSlab(m_slabno, QFile::ReadWrite, operation))
    return false;
  const qint64 initialSize = m_qfile.size();
  if (initialSize < m_header || initialSize > maximumSize ||
      endOffset > maximumSize)
    {
      m_qfile.close();
      return setError(QString("%1: '%2' has invalid size %3")
                      .arg(operation).arg(m_filename).arg(initialSize));
    }
  const bool ok = seekFile(m_qfile, fileOffset, operation) &&
                  writeExact(m_qfile, tmp, bps, operation) &&
                  flushFile(m_qfile, operation) &&
                  m_qfile.size() == qMax(initialSize, endOffset);
  if (!ok && m_lastError.isEmpty())
    setError(QString("%1: '%2' has an unexpected final size")
             .arg(operation).arg(m_filename));
  m_qfile.close();
  return ok;
}

bool
VolumeFileManager::setWidthSlice(int w, uchar *tmp)
{
  const QString operation = "write width slice";
  clearError();
  if (!tmp)
    return setError(QString("%1: source buffer is null").arg(operation));
  if (!validateGeometry(operation) || w < 0 || w >= m_width)
    {
      if (m_lastError.isEmpty())
        setError(QString("%1: width index %2 is outside [0, %3)")
                 .arg(operation).arg(w).arg(m_width));
      return false;
    }

  qint64 bps = 0;
  qint64 rowBytes = 0;
  if (!sliceByteCount(bps, operation) ||
      !checkedMultiply(static_cast<qint64>(m_height),
                       m_bytesPerVoxel, rowBytes))
    return setError(QString("%1: row byte count overflows").arg(operation));

  const int slabSize = static_cast<int>(m_slabSize);
  int previousSlab = -1;
  qint64 initialSize = 0;
  qint64 expectedSize = 0;
  qint64 maximumSize = 0;
  for(int d=0; d<m_depth; ++d)
    {
      const int slab = d/slabSize;
      if (slab != previousSlab)
        {
          if (m_qfile.isOpen())
            {
              if (!flushFile(m_qfile, operation) ||
                  m_qfile.size() != expectedSize)
                {
                  if (m_lastError.isEmpty())
                    setError(QString("%1: '%2' has an unexpected final size")
                             .arg(operation).arg(m_filename));
                  m_qfile.close();
                  return false;
                }
              m_qfile.close();
            }
          if (!openSlab(slab, QFile::ReadWrite, operation) ||
              !expectedSlabSize(slab, bps, maximumSize, operation))
            return false;
          initialSize = m_qfile.size();
          if (initialSize < m_header || initialSize > maximumSize)
            {
              m_qfile.close();
              return setError(QString("%1: '%2' has invalid size %3")
                              .arg(operation).arg(m_filename).arg(initialSize));
            }
          expectedSize = initialSize;
          previousSlab = slab;
        }

      qint64 sliceOffset = 0;
      qint64 widthOffset = 0;
      qint64 fileOffset = 0;
      qint64 endOffset = 0;
      qint64 inputOffset = 0;
      if (!checkedMultiply(static_cast<qint64>(d-slab*slabSize),
                           bps, sliceOffset) ||
          !checkedMultiply(static_cast<qint64>(w), rowBytes, widthOffset) ||
          !checkedAdd(sliceOffset, widthOffset, fileOffset) ||
          !checkedAdd(m_header, fileOffset, fileOffset) ||
          !checkedAdd(fileOffset, rowBytes, endOffset) ||
          !checkedMultiply(static_cast<qint64>(d), rowBytes, inputOffset) ||
          endOffset > maximumSize ||
          !seekFile(m_qfile, fileOffset, operation) ||
          !writeExact(m_qfile, tmp+inputOffset, rowBytes, operation))
        {
          if (m_lastError.isEmpty())
            setError(QString("%1: byte offset overflows").arg(operation));
          m_qfile.close();
          return false;
        }
      expectedSize = qMax(expectedSize, endOffset);
    }

  const bool ok = !m_qfile.isOpen() ||
                  (flushFile(m_qfile, operation) &&
                   m_qfile.size() == expectedSize);
  if (!ok && m_lastError.isEmpty())
    setError(QString("%1: '%2' has an unexpected final size")
             .arg(operation).arg(m_filename));
  m_qfile.close();
  return ok;
}

bool
VolumeFileManager::setHeightSlice(int h, uchar *tmp)
{
  const QString operation = "write height slice";
  clearError();
  if (!tmp)
    return setError(QString("%1: source buffer is null").arg(operation));
  if (!validateGeometry(operation) || h < 0 || h >= m_height)
    {
      if (m_lastError.isEmpty())
        setError(QString("%1: height index %2 is outside [0, %3)")
                 .arg(operation).arg(h).arg(m_height));
      return false;
    }

  qint64 bps = 0;
  if (!sliceByteCount(bps, operation))
    return false;
  const int slabSize = static_cast<int>(m_slabSize);
  int previousSlab = -1;
  qint64 expectedSize = 0;
  qint64 maximumSize = 0;
  for(int d=0; d<m_depth; ++d)
    {
      const int slab = d/slabSize;
      if (slab != previousSlab)
        {
          if (m_qfile.isOpen())
            {
              if (!flushFile(m_qfile, operation) ||
                  m_qfile.size() != expectedSize)
                {
                  if (m_lastError.isEmpty())
                    setError(QString("%1: '%2' has an unexpected final size")
                             .arg(operation).arg(m_filename));
                  m_qfile.close();
                  return false;
                }
              m_qfile.close();
            }
          if (!openSlab(slab, QFile::ReadWrite, operation) ||
              !expectedSlabSize(slab, bps, maximumSize, operation))
            return false;
          expectedSize = m_qfile.size();
          if (expectedSize < m_header || expectedSize > maximumSize)
            {
              m_qfile.close();
              return setError(QString("%1: '%2' has invalid size %3")
                              .arg(operation).arg(m_filename).arg(expectedSize));
            }
          previousSlab = slab;
        }

      for(int w=0; w<m_width; ++w)
        {
          qint64 sliceOffset = 0;
          qint64 rowVoxel = 0;
          qint64 voxelOffset = 0;
          qint64 dataOffset = 0;
          qint64 fileOffset = 0;
          qint64 endOffset = 0;
          qint64 inputVoxel = 0;
          qint64 inputOffset = 0;
          if (!checkedMultiply(static_cast<qint64>(d-slab*slabSize),
                               bps, sliceOffset) ||
              !checkedMultiply(static_cast<qint64>(w),
                               static_cast<qint64>(m_height), rowVoxel) ||
              !checkedAdd(rowVoxel, static_cast<qint64>(h), voxelOffset) ||
              !checkedMultiply(voxelOffset, m_bytesPerVoxel, dataOffset) ||
              !checkedAdd(sliceOffset, dataOffset, fileOffset) ||
              !checkedAdd(m_header, fileOffset, fileOffset) ||
              !checkedAdd(fileOffset, m_bytesPerVoxel, endOffset) ||
              !checkedMultiply(static_cast<qint64>(d),
                               static_cast<qint64>(m_width), inputVoxel) ||
              !checkedAdd(inputVoxel, static_cast<qint64>(w), inputVoxel) ||
              !checkedMultiply(inputVoxel, m_bytesPerVoxel, inputOffset) ||
              endOffset > maximumSize ||
              !seekFile(m_qfile, fileOffset, operation) ||
              !writeExact(m_qfile, tmp+inputOffset,
                          m_bytesPerVoxel, operation))
            {
              if (m_lastError.isEmpty())
                setError(QString("%1: byte offset overflows").arg(operation));
              m_qfile.close();
              return false;
            }
          expectedSize = qMax(expectedSize, endOffset);
        }
    }

  const bool ok = !m_qfile.isOpen() ||
                  (flushFile(m_qfile, operation) &&
                   m_qfile.size() == expectedSize);
  if (!ok && m_lastError.isEmpty())
    setError(QString("%1: '%2' has an unexpected final size")
             .arg(operation).arg(m_filename));
  m_qfile.close();
  return ok;
}

uchar*
VolumeFileManager::rawValue(int d, int w, int h)
{
  const QString operation = "read voxel";
  clearError();
  if (!validateGeometry(operation) ||
      !ensureSliceCapacity(8, operation))
    return 0;

  // at most we will be reading an 8 byte value
  // initialize first 8 bytes to 0
  memset(m_slice, 0, 8);

  if (d < 0 || d >= m_depth ||
      w < 0 || w >= m_width ||
      h < 0 || h >= m_height)
    return m_slice;

  qint64 bps = 0;
  if (!sliceByteCount(bps, operation))
    return 0;
  const int slabSize = static_cast<int>(m_slabSize);
  m_slabno = d/slabSize;
  qint64 sliceOffset = 0;
  qint64 rowVoxel = 0;
  qint64 voxelOffset = 0;
  qint64 dataOffset = 0;
  qint64 fileOffset = 0;
  if (!checkedMultiply(static_cast<qint64>(d-m_slabno*slabSize),
                       bps, sliceOffset) ||
      !checkedMultiply(static_cast<qint64>(w),
                       static_cast<qint64>(m_height), rowVoxel) ||
      !checkedAdd(rowVoxel, static_cast<qint64>(h), voxelOffset) ||
      !checkedMultiply(voxelOffset, m_bytesPerVoxel, dataOffset) ||
      !checkedAdd(sliceOffset, dataOffset, dataOffset) ||
      !checkedAdd(m_header, dataOffset, fileOffset))
    {
      setError(QString("%1: file offset overflows").arg(operation));
      return 0;
    }
  const bool ok = openSlab(m_slabno, QFile::ReadOnly, operation) &&
                  seekFile(m_qfile, fileOffset, operation) &&
                  readExact(m_qfile, m_slice, m_bytesPerVoxel, operation);
  m_qfile.close();
  if (!ok)
    {
      memset(m_slice, 0, 8);
      return 0;
    }
  return m_slice;
}

#define interpVal(T)					\
  T *v[8];						\
  for(int i=0; i<8; i++)				\
    v[i] = (T*)(rv + i*m_bytesPerVoxel);		\
							\
  T vb = ((1-dd)*(1-ww)*(1-hh)*(*v[0]) +		\
	  (1-dd)*(1-ww)*(  hh)*(*v[1]) +		\
	  (1-dd)*(  ww)*(1-hh)*(*v[2]) +		\
	  (1-dd)*(  ww)*(  hh)*(*v[3]) +		\
	  (  dd)*(1-ww)*(1-hh)*(*v[4]) +		\
	  (  dd)*(1-ww)*(  hh)*(*v[5]) +		\
	  (  dd)*(  ww)*(1-hh)*(*v[6]) +		\
	  (  dd)*(  ww)*(  hh)*(*v[7]));		\
  memcpy(m_slice, &vb, sizeof(T));


uchar*
VolumeFileManager::interpolatedRawValue(float dv, float wv, float hv)
{
  const QString operation = "interpolate voxel";
  clearError();
  if (!validateGeometry(operation) ||
      !ensureSliceCapacity(8, operation))
    return 0;

  int d = dv;
  int w = wv;
  int h = hv;
  int d1 = d+1;
  int w1 = w+1;
  int h1 = h+1;
  float dd = dv-d;
  float ww = wv-w;
  float hh = hv-h;
  
  // at most we will be reading an 8 byte value
  // initialize first 8 bytes to 0
  memset(m_slice, 0, 8);

  if (d < 0 || d1 >= m_depth ||
      w < 0 || w1 >= m_width ||
      h < 0 || h1 >= m_height)
    return m_slice;

  int da[8], wa[8], ha[8];
  da[0]=d;  wa[0]=w;  ha[0]=h;
  da[1]=d;  wa[1]=w;  ha[1]=h1;
  da[2]=d;  wa[2]=w1; ha[2]=h;
  da[3]=d;  wa[3]=w1; ha[3]=h1;
  da[4]=d1; wa[4]=w;  ha[4]=h;
  da[5]=d1; wa[5]=w;  ha[5]=h1;
  da[6]=d1; wa[6]=w1; ha[6]=h;
  da[7]=d1; wa[7]=w1; ha[7]=h1;

  uchar rv[8*4];
  memset(rv, 0, sizeof(rv));
  for(int i=0; i<8; i++)
    {
      uchar *sample = rawValue(da[i], wa[i], ha[i]);
      if (!sample)
        {
          memset(m_slice, 0, 8);
          return 0;
        }
      memcpy(rv+i*m_bytesPerVoxel, sample,
             static_cast<size_t>(m_bytesPerVoxel));
    }
  
  if (m_voxelType == _UChar)
    {
      interpVal(uchar);
    }
  else if (m_voxelType == _Char)
    {
      interpVal(char);
    }
  else if (m_voxelType == _UShort)
    {
      interpVal(ushort);
    }
  else if (m_voxelType == _Short)
    {
      interpVal(short);
    }
  else if (m_voxelType == _Int)
    {
      interpVal(int);
    }
  else if (m_voxelType == _Float)
    {
      interpVal(float);
    }
  
  return m_slice;
}

void
VolumeFileManager::startBlockInterpolation()
{
  const QString operation = "start block interpolation";
  clearError();
  qint64 bps = 0;
  qint64 blockBytes = 0;
  if (!validateGeometry(operation) ||
      !sliceByteCount(bps, operation) ||
      !checkedMultiply(static_cast<qint64>(m_blockSlices),
                       bps, blockBytes) ||
      !ensureBlockCapacity(blockBytes, operation))
    {
      if (m_lastError.isEmpty())
        setError(QString("%1: buffer byte count overflows").arg(operation));
      return;
    }
  memset(m_block, 0, static_cast<size_t>(blockBytes));
  readBlocks(0);
}

void
VolumeFileManager::endBlockInterpolation()
{
  if (m_block)
    delete [] m_block;

  m_block = 0;
  m_blockCapacity = 0;
  m_startBlock = m_endBlock = 0;
}

bool
VolumeFileManager::readBlocks(int d)
{
  const QString operation = "read interpolation block";
  if (!validateGeometry(operation) ||
      d > std::numeric_limits<int>::max()-m_blockSlices)
    return setError(QString("%1: invalid block range").arg(operation));
  qint64 bps = 0;
  qint64 blockBytes = 0;
  if (!sliceByteCount(bps, operation) ||
      !checkedMultiply(static_cast<qint64>(m_blockSlices),
                       bps, blockBytes) ||
      !ensureBlockCapacity(blockBytes, operation))
    return false;
  memset(m_block, 0, static_cast<size_t>(blockBytes));
  m_startBlock = d;
  m_endBlock = d+m_blockSlices;

  const int slabSize = static_cast<int>(m_slabSize);
  int previousSlab = -1;
  for(int blockIndex=0; blockIndex<m_blockSlices; ++blockIndex)
    {
      const qint64 volumeDepth = static_cast<qint64>(d)+blockIndex;
      if (volumeDepth < 0 || volumeDepth >= m_depth)
        continue;
      const int slab = static_cast<int>(volumeDepth)/slabSize;
      if (slab != previousSlab)
        {
          if (!openSlab(slab, QFile::ReadOnly, operation))
            {
              memset(m_block, 0, static_cast<size_t>(blockBytes));
              return false;
            }
          previousSlab = slab;
        }
      qint64 sliceOffset = 0;
      qint64 fileOffset = 0;
      qint64 outputOffset = 0;
      if (!checkedMultiply(volumeDepth-static_cast<qint64>(slab)*slabSize,
                           bps, sliceOffset) ||
          !checkedAdd(m_header, sliceOffset, fileOffset) ||
          !checkedMultiply(static_cast<qint64>(blockIndex),
                           bps, outputOffset) ||
          !seekFile(m_qfile, fileOffset, operation) ||
          !readExact(m_qfile, m_block+outputOffset, bps, operation))
        {
          if (m_lastError.isEmpty())
            setError(QString("%1: byte offset overflows").arg(operation));
          m_qfile.close();
          memset(m_block, 0, static_cast<size_t>(blockBytes));
          return false;
        }
    }
  m_qfile.close();
  return true;
}

uchar*
VolumeFileManager::blockInterpolatedRawValue(float dv, float wv, float hv)
{
  const QString operation = "interpolate voxel block";
  clearError();
  if (!validateGeometry(operation) ||
      !ensureSliceCapacity(8, operation))
    return 0;

  int d = dv;
  int w = wv;
  int h = hv;
  int d1 = d+1;
  int w1 = w+1;
  int h1 = h+1;
  float dd = dv-d;
  float ww = wv-w;
  float hh = hv-h;
  
  qint64 bps = 0;
  if (!sliceByteCount(bps, operation))
    return 0;

  // at most we will be reading an 8 byte value
  // initialize first 8 bytes to 0
  memset(m_slice, 0, 8);

  if (d < 0 || d1 >= m_depth ||
      w < 0 || w1 >= m_width ||
      h < 0 || h1 >= m_height)
    return m_slice;

  int da[8], wa[8], ha[8];
  da[0]=d;  wa[0]=w;  ha[0]=h;
  da[1]=d;  wa[1]=w;  ha[1]=h1;
  da[2]=d;  wa[2]=w1; ha[2]=h;
  da[3]=d;  wa[3]=w1; ha[3]=h1;
  da[4]=d1; wa[4]=w;  ha[4]=h;
  da[5]=d1; wa[5]=w;  ha[5]=h1;
  da[6]=d1; wa[6]=w1; ha[6]=h;
  da[7]=d1; wa[7]=w1; ha[7]=h1;

  uchar rv[8*4];
  memset(rv, 0, sizeof(rv));

  if (!m_block)
    {
      if (!readBlocks(da[0]))
        return 0;
    }

  for(int i=0; i<8; i++)
    {
      if (da[i] < m_startBlock ||
	  da[i] >= m_endBlock)
	{
          if (!readBlocks(da[i]))
            return 0;
        }

      memcpy((char*)rv+i*m_bytesPerVoxel,
	     m_block + static_cast<qint64>(da[i]-m_startBlock)*bps +
	               (static_cast<qint64>(wa[i])*m_height + ha[i])*m_bytesPerVoxel,
	     static_cast<size_t>(m_bytesPerVoxel));
    }
  
  if (m_voxelType == _UChar)
    {
      interpVal(uchar);
    }
  else if (m_voxelType == _Char)
    {
      interpVal(char);
    }
  else if (m_voxelType == _UShort)
    {
      interpVal(ushort);
    }
  else if (m_voxelType == _Short)
    {
      interpVal(short);
    }
  else if (m_voxelType == _Int)
    {
      interpVal(int);
    }
  else if (m_voxelType == _Float)
    {
      interpVal(float);
    }
  
  return m_slice;
}

bool
VolumeFileManager::checkFileSave()
{
  if (m_memChanged)
    return requestSave();
  return true;
}

void
VolumeFileManager::fileSaveProgress(quint64 generation)
{
  if (m_fileHandlerBusy && generation == m_saveGeneration)
    emit saveCycleProgressed();
}

void
VolumeFileManager::doneFileSave(quint64 generation,
                                bool saved,
                                QString error)
{
  const quint64 expectedGeneration = m_saveGeneration;
  const bool saveAgain = m_saveRequested;
  const bool explicitWait = m_waitingOnFileHandler;

  m_fileHandlerBusy = false;
  m_saveGeneration = 0;
  m_saveRequested = false;
  if (!m_snapshotPath.isEmpty())
    QFile::remove(m_snapshotPath);
  m_snapshotPath.clear();

  if (generation != expectedGeneration)
    {
      saved = false;
      error = QString("mask save generation %1 does not match expected %2")
                .arg(generation).arg(expectedGeneration);
    }

  if (!saved)
    {
      m_lastError = error.isEmpty() ?
        QString("background mask save failed") : error;
      m_backgroundSaveFailed = true;
      m_memChanged = true;
      emit saveCycleFinished();
      return;
    }

  m_backgroundSaveFailed = false;
  m_lastError = error;
  if (generation == m_changeGeneration)
    {
      m_memChanged = false;
      m_mcTimes = 0;
      m_saveDSlices.clear();
      m_saveWSlices.clear();
      m_saveHSlices.clear();
    }

  if (m_memChanged && saveAgain && !explicitWait)
    {
      m_saveRequested = true;
      if (m_saveDebounceTimer)
        m_saveDebounceTimer->start();
    }

  emit saveCycleFinished();
}

void
VolumeFileManager::fileHandlerThreadFinished()
{
  if (!m_thread || m_thread->isRunning())
    return;

  const bool interruptedSave = m_fileHandlerBusy;
  m_handler = 0;
  m_thread->deleteLater();
  m_thread = 0;

  if (!interruptedSave)
    return;

  m_fileHandlerBusy = false;
  m_saveGeneration = 0;
  m_saveRequested = false;
  m_memChanged = true;
  m_backgroundSaveFailed = true;

  QString cleanupError;
  if (!m_snapshotPath.isEmpty() &&
      QFileInfo::exists(m_snapshotPath) &&
      !QFile::remove(m_snapshotPath))
    cleanupError = QString("; snapshot cleanup failed for '%1'")
                     .arg(m_snapshotPath);
  m_snapshotPath.clear();
  m_lastError = QString("mask save worker exited before reporting completion%1")
                  .arg(cleanupError);
  emit saveCycleFinished();
}

bool
VolumeFileManager::saveSlicesToFile()
{
  return saveMemFile();
}

bool
VolumeFileManager::exiting()
{
  if (!saveMemFile())
    return false;
  return stopFileHandlerThread(false);
}

bool
VolumeFileManager::saveMemFile()
{
  if (!m_memChanged)
    return true;

  const QString operation = "save memory volume";
  clearError();
  if (!validateGeometry(operation))
    return false;
  qint64 bps = 0;
  qint64 volumeBytes = 0;
  if (!sliceByteCount(bps, operation) ||
      !volumeByteCount(volumeBytes, operation) ||
      !m_volData ||
      static_cast<quint64>(volumeBytes) >
      static_cast<quint64>(m_volDataCapacity))
    {
      if (m_lastError.isEmpty())
        setError(QString("%1: memory volume is unavailable or too small")
                 .arg(operation));
      return false;
    }

  if (usesCompressedMaskFormat(m_filenames))
    return flushPendingChanges();

  uchar vt = 0;
  if (m_voxelType == _Char) vt = 1;
  else if (m_voxelType == _UShort) vt = 2;
  else if (m_voxelType == _Short) vt = 3;
  else if (m_voxelType == _Int) vt = 4;
  else if (m_voxelType == _Float) vt = 8;

  const int slabSize = static_cast<int>(m_slabSize);
  const int slabCount = 1+(m_depth-1)/slabSize;
  QStringList targetFiles;
  for(int slab=0; slab<slabCount; ++slab)
    targetFiles.append(slabFilename(slab));

  if (m_qfile.isOpen())
    m_qfile.close();

  SlabSaveTransactionState transaction;
  QString transactionError;
  if (!SlabSaveTransaction::begin(targetFiles, transaction,
                                  &transactionError))
    return setError(QString("%1: %2").arg(operation, transactionError));

  const auto discardStages = [this, &operation, &transaction]
                             (const QString& error)
    {
      QString cleanupError;
      (void)SlabSaveTransaction::discardStages(transaction, &cleanupError);
      return setError(cleanupError.isEmpty() ? error :
                      QString("%1; cleanup failed: %2")
                        .arg(error, cleanupError));
    };

  for(int slab=0; slab<slabCount; ++slab)
    {
      const QString filename = targetFiles[slab];
      const QString stageFilename = transaction.entries[slab].stagePath;
      QSaveFile output(stageFilename);
      output.setDirectWriteFallback(false);
      if (!output.open(QFile::WriteOnly))
        return discardStages(
          QString("%1: cannot stage '%2': %3")
            .arg(operation, filename, output.errorString()));

      const qint64 firstSlice = static_cast<qint64>(slab)*slabSize;
      const qint32 slices = qMin(slabSize,
                                 m_depth-static_cast<int>(firstSlice));
      const qint32 width = m_width;
      const qint32 height = m_height;
      qint64 slabBytes = 0;
      qint64 sourceOffset = 0;
      bool ok = checkedMultiply(static_cast<qint64>(slices),
                                bps, slabBytes) &&
                checkedMultiply(firstSlice, bps, sourceOffset);
      if (!ok)
        {
          output.cancelWriting();
          return discardStages(
            QString("%1: slab byte count overflows").arg(operation));
        }

      const uchar *parts[] =
        {
          &vt,
          reinterpret_cast<const uchar*>(&slices),
          reinterpret_cast<const uchar*>(&width),
          reinterpret_cast<const uchar*>(&height),
          m_volData+sourceOffset
        };
      const qint64 partSizes[] = {1, 4, 4, 4, slabBytes};
      for(int part=0; part<5 && ok; ++part)
        {
          qint64 written = 0;
          while (written < partSizes[part])
            {
              const qint64 count = output.write(
                reinterpret_cast<const char*>(parts[part])+written,
                partSizes[part]-written);
              if (count <= 0)
                {
                  ok = false;
                  m_lastError = QString("%1: short write while staging '%2': %3")
                                  .arg(operation, filename,
                                       output.errorString());
                  break;
                }
              written += count;
            }
        }
      if (!ok || !output.commit())
        {
          if (ok)
            m_lastError = QString("%1: cannot commit stage for '%2': %3")
                            .arg(operation, filename, output.errorString());
          else
            output.cancelWriting();
          return discardStages(m_lastError);
        }

      qint64 expectedStageBytes = 0;
      const qint64 actualStageBytes = QFileInfo(stageFilename).size();
      if (!checkedAdd(13, slabBytes, expectedStageBytes) ||
          actualStageBytes != expectedStageBytes)
        return discardStages(
          QString("%1: stage for '%2' has %3 bytes, expected %4")
            .arg(operation, filename)
            .arg(actualStageBytes).arg(expectedStageBytes));
    }

  if (!SlabSaveTransaction::commit(transaction, &transactionError))
    {
      if (!QFileInfo::exists(transaction.journalPath))
        {
          QString cleanupError;
          (void)SlabSaveTransaction::discardStages(transaction,
                                                    &cleanupError);
          if (!cleanupError.isEmpty())
            transactionError += QString("; cleanup failed: %1")
                                  .arg(cleanupError);
        }
      return setError(QString("%1: %2").arg(operation, transactionError));
    }

  m_memChanged = false;
  m_mcTimes = 0;
  m_saveDSlices.clear();
  m_saveWSlices.clear();
  m_saveHSlices.clear();
  return true;
}

QString
VolumeFileManager::exportMask()
{
  const QString operation = "export mask";
  clearError();
  qint64 bps = 0;
  qint64 volumeBytes = 0;
  if (!validateGeometry(operation) ||
      !sliceByteCount(bps, operation) ||
      !volumeByteCount(volumeBytes, operation) ||
      !m_volData ||
      static_cast<quint64>(volumeBytes) >
      static_cast<quint64>(m_volDataCapacity))
    {
      if (m_lastError.isEmpty())
        setError(QString("%1: memory mask is unavailable or too small")
                 .arg(operation));
      return QString();
    }

  QString flnm;
  flnm = QFileDialog::getSaveFileName(0,
				      "Export to .raw file",
				      Global::previousDirectory(),
				      "Mask Files (*.raw)",
				      0,
				      QFileDialog::DontUseNativeDialog);

  
  if (flnm.isEmpty())
    return QString();

  if (!StaticFunctions::checkExtension(flnm, ".raw"))
    flnm += ".raw";

  uchar vt;
  if (m_voxelType == _UChar) vt = 0; // unsigned byte
  if (m_voxelType == _Char) vt = 1; // signed byte
  if (m_voxelType == _UShort) vt = 2; // unsigned short
  if (m_voxelType == _Short) vt = 3; // signed short
  if (m_voxelType == _Int) vt = 4; // int
  if (m_voxelType == _Float) vt = 8; // float

  QProgressDialog progress(QString("Saving %1").arg(flnm),
			   "Cancel",
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);
  progress.setCancelButton(0);

  QSaveFile output(flnm);
  if (!output.open(QFile::WriteOnly))
    {
      setError(QString("%1: cannot open '%2': %3")
               .arg(operation).arg(flnm).arg(output.errorString()));
      return QString();
    }

  const qint32 depth = m_depth;
  const qint32 width = m_width;
  const qint32 height = m_height;
  const uchar *parts[] =
    {
      &vt,
      reinterpret_cast<const uchar*>(&depth),
      reinterpret_cast<const uchar*>(&width),
      reinterpret_cast<const uchar*>(&height),
      m_volData
    };
  const qint64 partSizes[] = {1, 4, 4, 4, volumeBytes};
  bool ok = true;
  for(int part=0; part<5 && ok; ++part)
    {
      qint64 written = 0;
      while (written < partSizes[part])
        {
          const qint64 count = output.write(
            reinterpret_cast<const char*>(parts[part])+written,
            partSizes[part]-written);
          if (count <= 0)
            {
              ok = setError(QString("%1: short write to '%2': %3")
                            .arg(operation).arg(flnm)
                            .arg(output.errorString()));
              break;
            }
          written += count;
        }
    }
  if (!ok || !output.commit())
    {
      if (ok)
        setError(QString("%1: cannot commit '%2': %3")
                 .arg(operation).arg(flnm).arg(output.errorString()));
      else
        output.cancelWriting();
      return QString();
    }

  progress.setValue(100);

  return flnm;
}

bool
VolumeFileManager::createUndo()
{
  const QString operation = "create mask undo";
  clearError();
  if (!usesCompressedMaskFormat(m_filenames))
    return setError(QString("%1: target is not a compressed mask")
                    .arg(operation));
  if (!validateGeometry(operation) || !m_volData)
    {
      if (m_lastError.isEmpty())
        setError(QString("%1: mask volume is unavailable").arg(operation));
      return false;
    }
  if (!flushPendingChanges())
    return false;

  FileHandler handler;
  configureFileHandler(handler);
  if (!handler.genUndo())
    return setError(handler.lastError());
  return true;
}

bool
VolumeFileManager::undo()
{
  const QString operation = "restore mask undo";
  clearError();
  if (!validateGeometry(operation) || !m_volData)
    return false;
  if (!stopFileHandlerThread(true))
    return false;
  FileHandler handler;
  configureFileHandler(handler);
  if (!handler.undo())
    return setError(handler.lastError());
  m_memChanged = false;
  m_mcTimes = 0;
  return startFileHandlerThread();
}

bool
VolumeFileManager::loadRawFile(QString flnm)
{
  const QString operation = "import raw mask";
  clearError();
  if (!validateGeometry(operation) ||
      m_bytesPerVoxel != 2 || !m_volData)
    {
      if (m_lastError.isEmpty())
        setError(QString("%1: a 16-bit in-memory mask is required")
                 .arg(operation));
      return false;
    }
  
  QProgressDialog progress(QString("Loading %1").\
			   arg(flnm),
			   "Cancel",
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);
  progress.setCancelButton(0);

  int bpl = 0;
  qint32 ldepth = 0;
  qint32 lwidth = 0;
  qint32 lheight = 0;
  
  m_qfile.setFileName(flnm);
  if (!m_qfile.open(QFile::ReadOnly))
    return setError(QString("%1: cannot open '%2': %3")
                    .arg(operation).arg(flnm).arg(m_qfile.errorString()));
  uchar vt = 0;
  bool ok = readExact(m_qfile, &vt, 1, operation) &&
            readExact(m_qfile, reinterpret_cast<uchar*>(&ldepth), 4,
                      operation) &&
            readExact(m_qfile, reinterpret_cast<uchar*>(&lwidth), 4,
                      operation) &&
            readExact(m_qfile, reinterpret_cast<uchar*>(&lheight), 4,
                      operation);
  if (!ok)
    {
      m_qfile.close();
      return false;
    }
  if (vt == 0) bpl = 1; // 1-byte per label
  else if (vt == 2) bpl = 2; // 2-bytes per label
  else
    {
      m_qfile.close();
      return setError(QString("%1: unsupported label voxel type %2")
                      .arg(operation).arg(vt));
    }
  if (ldepth <= 0 || lwidth <= 0 || lheight <= 0)
    {
      m_qfile.close();
      return setError(QString("%1: invalid source dimensions")
                      .arg(operation));
    }

  qint64 sourceVoxels = 0;
  qint64 lvsz = 0;
  qint64 expectedSize = 0;
  if (!checkedMultiply(static_cast<qint64>(ldepth),
                       static_cast<qint64>(lwidth), sourceVoxels) ||
      !checkedMultiply(sourceVoxels,
                       static_cast<qint64>(lheight), sourceVoxels) ||
      !checkedMultiply(sourceVoxels, static_cast<qint64>(bpl), lvsz) ||
      !checkedAdd(13, lvsz, expectedSize) ||
      m_qfile.size() != expectedSize ||
      static_cast<quint64>(lvsz) >
      static_cast<quint64>(std::numeric_limits<size_t>::max()))
    {
      m_qfile.close();
      return setError(QString("%1: source size is invalid or overflows")
                      .arg(operation));
    }

  uchar *lvolData = new (std::nothrow) uchar[static_cast<size_t>(lvsz)];
  if (!lvolData)
    {
      m_qfile.close();
      return setError(QString("%1: cannot allocate %2-byte source buffer")
                      .arg(operation).arg(lvsz));
    }
  ok = readExact(m_qfile, lvolData, lvsz, operation);
  m_qfile.close();
  if (!ok)
    {
      delete [] lvolData;
      return false;
    }

  if (!stopFileHandlerThread(true))
    {
      delete [] lvolData;
      return false;
    }

  ushort* volData = (ushort*)m_volData;
  qint64 targetBytes = 0;
  if (!volumeByteCount(targetBytes, operation) ||
      static_cast<quint64>(targetBytes) >
      static_cast<quint64>(m_volDataCapacity))
    {
      delete [] lvolData;
      return setError(QString("%1: destination buffer is too small")
                      .arg(operation));
    }

  const uchar *source8 = lvolData;
  const ushort *source16 = reinterpret_cast<const ushort*>(lvolData);
  qint64 targetIndex = 0;
  for(int d=0; d<m_depth; ++d)
    {
      if (d%20 == 0)
        {
          progress.setValue((int)(100.0*d/m_depth));
          if (qApp)
            qApp->processEvents();
        }
      const qint64 sourceD = static_cast<qint64>(d)*ldepth/m_depth;
      for(int w=0; w<m_width; ++w)
        {
          const qint64 sourceW = static_cast<qint64>(w)*lwidth/m_width;
          for(int h=0; h<m_height; ++h, ++targetIndex)
            {
              const qint64 sourceH = static_cast<qint64>(h)*lheight/m_height;
              const qint64 sourceIndex =
                (sourceD*static_cast<qint64>(lwidth)+sourceW)*lheight+sourceH;
              volData[targetIndex] = bpl == 1 ?
                source8[sourceIndex] : source16[sourceIndex];
            }
        }
    }

  delete [] lvolData;
  progress.setValue(100);
  markChanged();
  return true;
}

bool
VolumeFileManager::loadMemFile()
{
  if (!m_memmapped)
    return true;

  if (!stopFileHandlerThread(true))
    return false;

  const QString operation = "load memory volume";
  clearError();
  if (!validateGeometry(operation))
    return false;
  qint64 bps = 0;
  qint64 volumeBytes = 0;
  if (!sliceByteCount(bps, operation) ||
      !volumeByteCount(volumeBytes, operation) ||
      static_cast<quint64>(volumeBytes) >
      static_cast<quint64>(std::numeric_limits<size_t>::max()))
    return false;

  const bool compressedMask =
    !m_filenames.isEmpty() &&
    StaticFunctions::checkExtension(m_filenames[0], ".mask.sc");
  if (!compressedMask)
    {
      const int slabSize = static_cast<int>(m_slabSize);
      const int slabCount = 1+(m_depth-1)/slabSize;
      QStringList targets;
      for(int slab=0; slab<slabCount; ++slab)
        targets.append(slabFilename(slab));
      QString transactionError;
      if (!SlabSaveTransaction::recover(targets, &transactionError))
        return setError(QString("%1: %2").arg(operation, transactionError));
    }

  uchar *replacement =
    new (std::nothrow) uchar[static_cast<size_t>(volumeBytes)];
  if (!replacement)
    return setError(QString("%1: cannot allocate %2-byte volume buffer")
                    .arg(operation).arg(volumeBytes));
  memset(replacement, 0, static_cast<size_t>(volumeBytes));

  // --------------------
  // .mask.sc file loading here
  if (compressedMask)
    {
      m_qfile.setFileName(m_filenames[0]);
      if (!m_qfile.exists())
        {
          delete [] replacement;
          return setError(QString("%1: '%2' does not exist")
                          .arg(operation).arg(m_filenames[0]));
        }
      if (!loadCompressedMask(replacement, volumeBytes))
        {
          delete [] replacement;
          return false;
        }
      delete [] m_volData;
      m_volData = replacement;
      m_volDataCapacity = static_cast<size_t>(volumeBytes);
      m_memChanged = false;
      m_mcTimes = 0;
      m_saveDSlices.clear();
      m_saveWSlices.clear();
      m_saveHSlices.clear();
      discardSnapshotBaseline();
      m_changeGeneration = 0;
      m_saveGeneration = 0;
      return true;
    }
  // --------------------

  QProgressDialog progress(QString("Loading %1").\
			   arg(m_baseFilename),
			   "Cancel",
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);
  progress.setCancelButton(0);

  const int slabSize = static_cast<int>(m_slabSize);
  const int nslabs = 1+(m_depth-1)/slabSize;
  for(int ns=0; ns<nslabs; ns++)
    {
      m_filename = slabFilename(ns);

      m_qfile.setFileName(m_filename);
      if (!m_qfile.open(QFile::ReadOnly))
        {
          setError(QString("%1: cannot open '%2': %3")
                   .arg(operation).arg(m_filename).arg(m_qfile.errorString()));
          break;
        }

      const qint64 firstSlice = static_cast<qint64>(ns)*slabSize;
      const int slices = qMin(slabSize,
                              m_depth-static_cast<int>(firstSlice));
      qint64 slabBytes = 0;
      qint64 destinationOffset = 0;
      qint64 expectedSize = 0;
      bool ok = checkedMultiply(static_cast<qint64>(slices), bps,
                                slabBytes) &&
                checkedMultiply(firstSlice, bps, destinationOffset) &&
                checkedAdd(m_header, slabBytes, expectedSize);
      if (!ok)
        setError(QString("%1: slab byte count overflows").arg(operation));
      if (ok && m_qfile.size() != expectedSize)
        ok = setError(QString("%1: '%2' has %3 bytes, expected %4")
                      .arg(operation).arg(m_filename)
                      .arg(m_qfile.size()).arg(expectedSize));
      if (ok)
        ok = seekFile(m_qfile, m_header, operation) &&
             readExact(m_qfile, replacement+destinationOffset,
                       slabBytes, operation);

      progress.setLabelText(QString("%1 : %2 %3").arg(m_filename).\
			    arg(firstSlice).arg(firstSlice+slices-1));
      m_qfile.close();
      if (!ok)
        break;
      progress.setValue((int)(100.0*(firstSlice+slices)/m_depth));
      if (qApp)
        qApp->processEvents();
    }

  if (!m_lastError.isEmpty())
    {
      if (m_qfile.isOpen())
        m_qfile.close();
      delete [] replacement;
      return false;
    }

  delete [] m_volData;
  m_volData = replacement;
  m_volDataCapacity = static_cast<size_t>(volumeBytes);
  m_memChanged = false;
  m_mcTimes = 0;
  m_saveDSlices.clear();
  m_saveWSlices.clear();
  m_saveHSlices.clear();
  discardSnapshotBaseline();
  m_changeGeneration = 0;
  m_saveGeneration = 0;
  progress.setValue(100);
  return true;
}

bool
VolumeFileManager::createMemFile()
{
  const QString operation = "allocate memory volume";
  qint64 volumeBytes = 0;
  if (!validateGeometry(operation) ||
      !volumeByteCount(volumeBytes, operation) ||
      static_cast<quint64>(volumeBytes) >
      static_cast<quint64>(std::numeric_limits<size_t>::max()))
    return false;

  if (!stopFileHandlerThread(true))
    return false;

  if (m_volData && m_volDataCapacity >= static_cast<size_t>(volumeBytes))
    {
      memset(m_volData, 0, static_cast<size_t>(volumeBytes));
      discardSnapshotBaseline();
      return true;
    }

  uchar *replacement =
    new (std::nothrow) uchar[static_cast<size_t>(volumeBytes)];
  if (!replacement)
    return setError(QString("%1: cannot allocate %2-byte volume buffer")
                    .arg(operation).arg(volumeBytes));
  memset(replacement, 0, static_cast<size_t>(volumeBytes));

  delete [] m_volData;
  m_volData = replacement;
  m_volDataCapacity = static_cast<size_t>(volumeBytes);
  discardSnapshotBaseline();
  return true;
}

bool
VolumeFileManager::setDepthSliceMem(int d, uchar *tmp)
{
  if (!m_memmapped)
    return setSlice(d, tmp);

  const QString operation = "write memory depth slice";
  clearError();
  if (!tmp)
    return setError(QString("%1: source buffer is null").arg(operation));
  if (!validateGeometry(operation) || d < 0 || d >= m_depth)
    {
      if (m_lastError.isEmpty())
        setError(QString("%1: depth index %2 is outside [0, %3)")
                 .arg(operation).arg(d).arg(m_depth));
      return false;
    }
  qint64 bps = 0;
  qint64 volumeBytes = 0;
  qint64 destinationOffset = 0;
  if (!sliceByteCount(bps, operation) ||
      !volumeByteCount(volumeBytes, operation) ||
      !checkedMultiply(static_cast<qint64>(d), bps, destinationOffset) ||
      !m_volData ||
      static_cast<quint64>(volumeBytes) >
      static_cast<quint64>(m_volDataCapacity))
    return setError(QString("%1: memory volume is unavailable or too small")
                    .arg(operation));

  memcpy(m_volData+destinationOffset, tmp, static_cast<size_t>(bps));
  if (!m_saveDSlices.contains(d))
    m_saveDSlices << d;
  markChangedRange(destinationOffset, bps);
  m_mcTimes++;
  if (m_mcTimes > m_saveFreq)
    return requestSave();
  return true;
}
bool
VolumeFileManager::setWidthSliceMem(int w, uchar *tmp)
{
  if (!m_memmapped)
    return setWidthSlice(w, tmp);

  const QString operation = "write memory width slice";
  clearError();
  if (!tmp)
    return setError(QString("%1: source buffer is null").arg(operation));
  if (!validateGeometry(operation) || w < 0 || w >= m_width)
    {
      if (m_lastError.isEmpty())
        setError(QString("%1: width index %2 is outside [0, %3)")
                 .arg(operation).arg(w).arg(m_width));
      return false;
    }
  qint64 bps = 0;
  qint64 volumeBytes = 0;
  qint64 rowBytes = 0;
  qint64 widthOffset = 0;
  if (!sliceByteCount(bps, operation) ||
      !volumeByteCount(volumeBytes, operation) ||
      !checkedMultiply(static_cast<qint64>(m_height),
                       m_bytesPerVoxel, rowBytes) ||
      !checkedMultiply(static_cast<qint64>(w), rowBytes, widthOffset) ||
      !m_volData ||
      static_cast<quint64>(volumeBytes) >
      static_cast<quint64>(m_volDataCapacity))
    return setError(QString("%1: memory volume is unavailable or too small")
                    .arg(operation));
  for(int d=0; d<m_depth; ++d)
    {
      const qint64 volumeOffset = static_cast<qint64>(d)*bps+widthOffset;
      const qint64 inputOffset = static_cast<qint64>(d)*rowBytes;
      memcpy(m_volData+volumeOffset, tmp+inputOffset,
             static_cast<size_t>(rowBytes));
    }
  if (!m_saveWSlices.contains(w))
    m_saveWSlices << w;
  markChanged();
  m_mcTimes++;
  if (m_mcTimes > m_saveFreq)
    return requestSave();
  return true;
}
bool
VolumeFileManager::setHeightSliceMem(int h, uchar *tmp)
{
  if (!m_memmapped)
    return setHeightSlice(h, tmp);

  const QString operation = "write memory height slice";
  clearError();
  if (!tmp)
    return setError(QString("%1: source buffer is null").arg(operation));
  if (!validateGeometry(operation) || h < 0 || h >= m_height)
    {
      if (m_lastError.isEmpty())
        setError(QString("%1: height index %2 is outside [0, %3)")
                 .arg(operation).arg(h).arg(m_height));
      return false;
    }
  qint64 bps = 0;
  qint64 volumeBytes = 0;
  if (!sliceByteCount(bps, operation) ||
      !volumeByteCount(volumeBytes, operation) ||
      !m_volData ||
      static_cast<quint64>(volumeBytes) >
      static_cast<quint64>(m_volDataCapacity))
    return setError(QString("%1: memory volume is unavailable or too small")
                    .arg(operation));

  for(int d=0; d<m_depth; ++d)
    for(int w=0; w<m_width; ++w)
      {
        const qint64 volumeVoxel =
          (static_cast<qint64>(d)*m_width+w)*m_height+h;
        const qint64 inputVoxel = static_cast<qint64>(d)*m_width+w;
        memcpy(m_volData+volumeVoxel*m_bytesPerVoxel,
               tmp+inputVoxel*m_bytesPerVoxel,
               static_cast<size_t>(m_bytesPerVoxel));
      }
  if (!m_saveHSlices.contains(h))
    m_saveHSlices << h;
  markChanged();
  m_mcTimes++;
  if (m_mcTimes > m_saveFreq)
    return requestSave();
  return true;
}


uchar*
VolumeFileManager::getDepthSliceMem(int d)
{
  if (!m_memmapped)
    return getSlice(d);

  const QString operation = "read memory depth slice";
  clearError();
  if (!validateGeometry(operation) || d < 0 || d >= m_depth)
    {
      if (m_lastError.isEmpty())
        setError(QString("%1: depth index %2 is outside [0, %3)")
                 .arg(operation).arg(d).arg(m_depth));
      return 0;
    }
  qint64 bps = 0;
  qint64 volumeBytes = 0;
  qint64 sourceOffset = 0;
  if (!sliceByteCount(bps, operation) ||
      !volumeByteCount(volumeBytes, operation) ||
      !checkedMultiply(static_cast<qint64>(d), bps, sourceOffset) ||
      !m_volData ||
      static_cast<quint64>(volumeBytes) >
      static_cast<quint64>(m_volDataCapacity) ||
      !ensureSliceCapacity(bps, operation))
    return setError(QString("%1: memory volume is unavailable or too small")
                    .arg(operation)), static_cast<uchar*>(0);
  memset(m_slice, 0, static_cast<size_t>(bps));
  memcpy(m_slice, m_volData+sourceOffset, static_cast<size_t>(bps));

  return m_slice;
}

uchar*
VolumeFileManager::getWidthSliceMem(int w)
{
  if (!m_memmapped)
    return getWidthSlice(w);

  const QString operation = "read memory width slice";
  clearError();
  if (!validateGeometry(operation) || w < 0 || w >= m_width)
    {
      if (m_lastError.isEmpty())
        setError(QString("%1: width index %2 is outside [0, %3)")
                 .arg(operation).arg(w).arg(m_width));
      return 0;
    }
  qint64 bps = 0;
  qint64 volumeBytes = 0;
  qint64 rowBytes = 0;
  qint64 planeBytes = 0;
  qint64 widthOffset = 0;
  if (!sliceByteCount(bps, operation) ||
      !volumeByteCount(volumeBytes, operation) ||
      !checkedMultiply(static_cast<qint64>(m_height),
                       m_bytesPerVoxel, rowBytes) ||
      !planeByteCount(m_depth, m_height, planeBytes, operation) ||
      !checkedMultiply(static_cast<qint64>(w), rowBytes, widthOffset) ||
      !m_volData ||
      static_cast<quint64>(volumeBytes) >
      static_cast<quint64>(m_volDataCapacity) ||
      !ensureSliceCapacity(planeBytes, operation))
    return setError(QString("%1: memory volume is unavailable or too small")
                    .arg(operation)), static_cast<uchar*>(0);
  memset(m_slice, 0, static_cast<size_t>(planeBytes));
  for(int d=0; d<m_depth; ++d)
    memcpy(m_slice+static_cast<qint64>(d)*rowBytes,
           m_volData+static_cast<qint64>(d)*bps+widthOffset,
           static_cast<size_t>(rowBytes));

  return m_slice;
}

uchar*
VolumeFileManager::getHeightSliceMem(int h)
{
  if (!m_memmapped)
    return getHeightSlice(h);

  const QString operation = "read memory height slice";
  clearError();
  if (!validateGeometry(operation) || h < 0 || h >= m_height)
    {
      if (m_lastError.isEmpty())
        setError(QString("%1: height index %2 is outside [0, %3)")
                 .arg(operation).arg(h).arg(m_height));
      return 0;
    }
  qint64 volumeBytes = 0;
  qint64 planeBytes = 0;
  if (!volumeByteCount(volumeBytes, operation) ||
      !planeByteCount(m_depth, m_width, planeBytes, operation) ||
      !m_volData ||
      static_cast<quint64>(volumeBytes) >
      static_cast<quint64>(m_volDataCapacity) ||
      !ensureSliceCapacity(planeBytes, operation))
    return setError(QString("%1: memory volume is unavailable or too small")
                    .arg(operation)), static_cast<uchar*>(0);
  memset(m_slice, 0, static_cast<size_t>(planeBytes));
  for(int d=0; d<m_depth; ++d)
    for(int w=0; w<m_width; ++w)
      {
        const qint64 volumeVoxel =
          (static_cast<qint64>(d)*m_width+w)*m_height+h;
        const qint64 outputVoxel = static_cast<qint64>(d)*m_width+w;
        memcpy(m_slice+outputVoxel*m_bytesPerVoxel,
               m_volData+volumeVoxel*m_bytesPerVoxel,
               static_cast<size_t>(m_bytesPerVoxel));
      }
  return m_slice;
}
uchar*
VolumeFileManager::rawValueMem(int d, int w, int h)
{
  if (!m_memmapped)
    return rawValue(d,w,h);

  const QString operation = "read memory voxel";
  clearError();
  if (!validateGeometry(operation) ||
      !ensureSliceCapacity(8, operation))
    return 0;

  // at most we will be reading an 8 byte value
  // initialize first 8 bytes to 0
  memset(m_slice, 0, 8);

  if (d < 0 || d >= m_depth ||
      w < 0 || w >= m_width ||
      h < 0 || h >= m_height)
    return m_slice;

  qint64 volumeBytes = 0;
  qint64 sliceVoxels = 0;
  qint64 voxelIndex = 0;
  if (!volumeByteCount(volumeBytes, operation) ||
      !checkedMultiply(static_cast<qint64>(m_width),
                       static_cast<qint64>(m_height), sliceVoxels) ||
      !checkedMultiply(static_cast<qint64>(d), sliceVoxels, voxelIndex) ||
      !checkedAdd(voxelIndex,
                  static_cast<qint64>(w)*m_height+h, voxelIndex) ||
      !m_volData ||
      static_cast<quint64>(volumeBytes) >
      static_cast<quint64>(m_volDataCapacity))
    return setError(QString("%1: memory volume is unavailable or too small")
                    .arg(operation)), static_cast<uchar*>(0);
  memcpy(m_slice, m_volData+voxelIndex*m_bytesPerVoxel,
         static_cast<size_t>(m_bytesPerVoxel));

  return m_slice;
}

bool
VolumeFileManager::setValueMem(int d, int w, int h, int val)
{
  const QString operation = "write memory voxel";
  clearError();
  if (!m_memmapped)
    return setError(QString("%1: memory mapping is disabled").arg(operation));

  if (!validateGeometry(operation) ||
      d < 0 || d >= m_depth ||
      w < 0 || w >= m_width ||
      h < 0 || h >= m_height)
    return setError(QString("%1: voxel index is outside the volume")
                    .arg(operation));

  qint64 volumeBytes = 0;
  qint64 sliceVoxels = 0;
  qint64 voxelIndex = 0;
  if (!volumeByteCount(volumeBytes, operation) ||
      !checkedMultiply(static_cast<qint64>(m_width),
                       static_cast<qint64>(m_height), sliceVoxels) ||
      !checkedMultiply(static_cast<qint64>(d), sliceVoxels, voxelIndex) ||
      !checkedAdd(voxelIndex,
                  static_cast<qint64>(w)*m_height+h, voxelIndex) ||
      !m_volData ||
      static_cast<quint64>(volumeBytes) >
      static_cast<quint64>(m_volDataCapacity))
    return setError(QString("%1: memory volume is unavailable or too small")
                    .arg(operation));

  if (m_bytesPerVoxel == 1)
    m_volData[voxelIndex] = val;
  else if (m_bytesPerVoxel == 2)
    reinterpret_cast<ushort*>(m_volData)[voxelIndex] = val;
  else
    return setError(QString("%1: only 8-bit and 16-bit values are supported")
                    .arg(operation));
  
//  QMessageBox::information(0, "", QString("%1 %2 %3 : %4").\
//			   arg(d).arg(w).arg(h).arg(m_volData[d*m_width*m_height + w*m_height + h]));

  markChangedRange(voxelIndex*m_bytesPerVoxel, m_bytesPerVoxel);
  m_mcTimes++;
  if (m_mcTimes > m_saveFreq)
    return requestSave();

  return true;
}

bool
VolumeFileManager::saveBlock()
{
  if (!m_memmapped)
    return true;

  markChanged();
  return requestSave();
}
