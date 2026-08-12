#include "volumefilemanager.h"

#include <QApplication>
#include <QFileInfo>
#include <QUuid>

#include <cstring>
#include <limits>
#include <new>

VolumeFileManager::VolumeFileManager()
{
  m_slice = 0;
  m_sliceCapacity = 0;
  reset();
}

VolumeFileManager::~VolumeFileManager()
{
  rollbackFileCreation();
  reset();
}

void
VolumeFileManager::reset()
{
  if (m_qfile.isOpen())
    m_qfile.close();

  m_baseFilename.clear();
  m_header = 0;
  m_slabSize = 0;
  m_depth = m_width = m_height = 0;
  m_voxelType = _UChar;
  m_bytesPerVoxel = 1;

  m_filename.clear();
  m_slabno = m_prevslabno = -1;

  delete [] m_slice;
  m_slice = 0;
  m_sliceCapacity = 0;

  m_slice0AtTop = false;
  m_lastError.clear();
  clearTransaction();
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

void VolumeFileManager::clearError() { m_lastError.clear(); }

QString VolumeFileManager::lastError() const { return m_lastError; }

bool
VolumeFileManager::validateGeometry(const QString& operation)
{
  if (m_depth <= 0 || m_width <= 0 || m_height <= 0)
    return setError(QString("%1: invalid volume dimensions %2 x %3 x %4")
                    .arg(operation).arg(m_depth).arg(m_width).arg(m_height));
  if (m_slabSize <= 0)
    return setError(QString("%1: invalid slab size %2")
                    .arg(operation).arg(m_slabSize));
  if (m_header < 0)
    return setError(QString("%1: invalid header size %2")
                    .arg(operation).arg(m_header));
  if (m_baseFilename.isEmpty())
    return setError(QString("%1: no volume filename is configured")
                    .arg(operation));
  if (m_voxelType < _UChar || m_voxelType > _Float ||
      (m_bytesPerVoxel != 1 && m_bytesPerVoxel != 2 &&
       m_bytesPerVoxel != 4))
    return setError(QString("%1: invalid voxel type %2")
                    .arg(operation).arg(m_voxelType));
  return true;
}

bool
VolumeFileManager::sliceByteCount(qint64& bytes, const QString& operation)
{
  qint64 pixels = 0;
  if (!checkedMultiply(static_cast<qint64>(m_width),
                       static_cast<qint64>(m_height), pixels) ||
      !checkedMultiply(pixels, m_bytesPerVoxel, bytes) || bytes <= 0 ||
      static_cast<quint64>(bytes) >
        static_cast<quint64>(std::numeric_limits<std::size_t>::max()))
    return setError(QString("%1: slice byte count overflows the supported address space")
                    .arg(operation));
  return true;
}

bool
VolumeFileManager::volumeByteCount(qint64& bytes, const QString& operation)
{
  qint64 sliceBytes = 0;
  if (!sliceByteCount(sliceBytes, operation) ||
      !checkedMultiply(sliceBytes, static_cast<qint64>(m_depth), bytes) ||
      bytes <= 0 ||
      static_cast<quint64>(bytes) >
        static_cast<quint64>(std::numeric_limits<std::size_t>::max()))
    return setError(QString("%1: volume byte count overflows the supported address space")
                    .arg(operation));
  return true;
}

bool
VolumeFileManager::slabFileSize(int slab,
                                qint64 sliceBytes,
                                qint64& bytes,
                                const QString& operation)
{
  const int nslabs = 1 + (m_depth-1)/m_slabSize;
  if (slab < 0 || slab >= nslabs)
    return setError(QString("%1: invalid slab index %2")
                    .arg(operation).arg(slab));

  const qint64 firstSlice = static_cast<qint64>(slab)*m_slabSize;
  const int slices = qMin(m_slabSize,
                          m_depth-static_cast<int>(firstSlice));
  qint64 dataBytes = 0;
  if (slices <= 0 ||
      !checkedMultiply(static_cast<qint64>(slices), sliceBytes, dataBytes) ||
      !checkedAdd(m_header, dataBytes, bytes) || bytes <= 0 ||
      static_cast<quint64>(bytes) >
        static_cast<quint64>(std::numeric_limits<std::size_t>::max()))
    return setError(QString("%1: slab %2 byte count overflows the supported address space")
                    .arg(operation).arg(slab));
  return true;
}

bool
VolumeFileManager::ensureSliceCapacity(qint64 bytes,
                                       const QString& operation)
{
  if (bytes <= 0 ||
      static_cast<quint64>(bytes) >
        static_cast<quint64>(std::numeric_limits<std::size_t>::max()))
    return setError(QString("%1: requested slice buffer is too large")
                    .arg(operation));

  const std::size_t requested = static_cast<std::size_t>(bytes);
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

QString
VolumeFileManager::slabFilename(int slab) const
{
  return m_baseFilename +
         QString(".%1").arg(slab+1, 3, 10, QChar('0'));
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
  if (!destination || bytes <= 0)
    return setError(QString("%1: invalid read buffer").arg(operation));

  qint64 done = 0;
  while (done < bytes)
    {
      const qint64 count = file.read(
        reinterpret_cast<char*>(destination) + done, bytes-done);
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
  if (!source || bytes <= 0)
    return setError(QString("%1: invalid write buffer").arg(operation));

  qint64 done = 0;
  while (done < bytes)
    {
      const qint64 count = file.write(
        reinterpret_cast<const char*>(source) + done, bytes-done);
      if (count <= 0)
        return setError(QString("%1: short write to '%2' (%3 of %4 bytes): %5")
                        .arg(operation).arg(file.fileName()).arg(done).arg(bytes)
                        .arg(file.errorString()));
      done += count;
    }
  return true;
}

bool
VolumeFileManager::flushAndCheckSize(QFile& file,
                                     qint64 expectedSize,
                                     const QString& operation)
{
  if (!file.flush())
    return setError(QString("%1: cannot flush '%2': %3")
                    .arg(operation).arg(file.fileName())
                    .arg(file.errorString()));
  if (file.size() != expectedSize)
    return setError(QString("%1: '%2' has %3 bytes, expected %4")
                    .arg(operation).arg(file.fileName())
                    .arg(file.size()).arg(expectedSize));
  return true;
}

void
VolumeFileManager::cleanupFiles(const QStringList& filenames)
{
  for(int i=0; i<filenames.count(); ++i)
    QFile::remove(filenames[i]);
}

void
VolumeFileManager::clearTransaction()
{
  m_transactionFinals.clear();
  m_transactionBackups.clear();
  m_transactionHadOriginal.clear();
}

void VolumeFileManager::setSliceZeroAtTop(bool fd) { m_slice0AtTop = fd; }
void VolumeFileManager::setBaseFilename(QString bfn) { m_baseFilename = bfn; }
void VolumeFileManager::setDepth(int d) { m_depth = d; }
void VolumeFileManager::setWidth(int w) { m_width = w; }
void VolumeFileManager::setHeight(int h) { m_height = h; }
void VolumeFileManager::setHeaderSize(int hs) { m_header = hs; }
void VolumeFileManager::setSlabSize(int ss) { m_slabSize = ss; }

void
VolumeFileManager::setVoxelType(int vt)
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

QString VolumeFileManager::fileName() { return m_filename; }

void
VolumeFileManager::removeFile()
{
  clearError();
  if (!validateGeometry("remove volume files"))
    return;

  rollbackFileCreation();
  const int nslabs = 1 + (m_depth-1)/m_slabSize;
  for(int ns=0; ns<nslabs; ++ns)
    QFile::remove(slabFilename(ns));
}

bool
VolumeFileManager::exists()
{
  clearError();
  if (!validateGeometry("check volume files"))
    return false;

  qint64 sliceBytes = 0;
  qint64 volumeBytes = 0;
  if (!sliceByteCount(sliceBytes, "check volume files") ||
      !volumeByteCount(volumeBytes, "check volume files"))
    return false;
  Q_UNUSED(volumeBytes);

  const int nslabs = 1 + (m_depth-1)/m_slabSize;
  for(int ns=0; ns<nslabs; ++ns)
    {
      m_filename = slabFilename(ns);
      qint64 expectedSize = 0;
      if (!slabFileSize(ns, sliceBytes, expectedSize,
                        "check volume files"))
        return false;

      QFile file(m_filename);
      if (!file.exists() || file.size() != expectedSize)
        return setError(QString("check volume files: '%1' is missing or has an unexpected size")
                        .arg(m_filename));
    }
  return true;
}

bool
VolumeFileManager::createFile(bool writeHeader)
{
  clearError();
  if (!m_transactionFinals.isEmpty())
    return setError("create volume files: the previous file transaction is still active");
  if (writeHeader)
    m_header = 13;
  if (!validateGeometry("create volume files"))
    return false;

  qint64 sliceBytes = 0;
  qint64 volumeBytes = 0;
  if (!sliceByteCount(sliceBytes, "create volume files") ||
      !volumeByteCount(volumeBytes, "create volume files") ||
      !ensureSliceCapacity(sliceBytes, "create volume files"))
    return false;
  std::memset(m_slice, 0, static_cast<std::size_t>(sliceBytes));

  const int nslabs = 1 + (m_depth-1)/m_slabSize;
  for(int ns=0; ns<nslabs; ++ns)
    {
      qint64 unused = 0;
      if (!slabFileSize(ns, sliceBytes, unused, "create volume files"))
        return false;
    }

  uchar voxelCode = 0;
  if (m_voxelType == _Char) voxelCode = 1;
  else if (m_voxelType == _UShort) voxelCode = 2;
  else if (m_voxelType == _Short) voxelCode = 3;
  else if (m_voxelType == _Int) voxelCode = 4;
  else if (m_voxelType == _Float) voxelCode = 8;

  const QString token = QUuid::createUuid().toString(QUuid::WithoutBraces);
  QStringList finalFiles;
  QStringList temporaryFiles;
  for(int ns=0; ns<nslabs; ++ns)
    {
      const QString finalName = slabFilename(ns);
      finalFiles << finalName;
      temporaryFiles << finalName + ".drishti-part-" + token;
    }

  QProgressDialog progress(QString("Allocating space for\n%1\non disk")
                           .arg(m_baseFilename),
                           "Cancel", 0, 100, 0);
  progress.setMinimumDuration(0);

  qint64 initializedBytes = 0;
  bool canceled = false;
  for(int ns=0; ns<nslabs && m_lastError.isEmpty(); ++ns)
    {
      m_filename = finalFiles[ns];
      progress.setLabelText(m_filename);
      if (qApp)
        qApp->processEvents();
      if (progress.wasCanceled())
        {
          canceled = true;
          break;
        }

      QFile temporary(temporaryFiles[ns]);
      if (!temporary.open(QFile::WriteOnly | QFile::Truncate))
        {
          setError(QString("create volume files: cannot open '%1': %2")
                   .arg(temporary.fileName()).arg(temporary.errorString()));
          break;
        }

      const qint64 firstSlice = static_cast<qint64>(ns)*m_slabSize;
      const int nslices = qMin(m_slabSize,
                               m_depth-static_cast<int>(firstSlice));
      bool ok = true;
      if (writeHeader)
        {
          const qint32 fileSlices = nslices;
          const qint32 fileWidth = m_width;
          const qint32 fileHeight = m_height;
          ok = writeExact(temporary, &voxelCode, 1,
                          "create volume header") &&
               writeExact(temporary,
                          reinterpret_cast<const uchar*>(&fileSlices), 4,
                          "create volume header") &&
               writeExact(temporary,
                          reinterpret_cast<const uchar*>(&fileWidth), 4,
                          "create volume header") &&
               writeExact(temporary,
                          reinterpret_cast<const uchar*>(&fileHeight), 4,
                          "create volume header");
        }

      for(int slice=0; slice<nslices && ok; ++slice)
        {
          ok = writeExact(temporary, m_slice, sliceBytes,
                          "initialize volume data");
          if (!ok)
            break;

          initializedBytes += sliceBytes;
          const int percent = static_cast<int>(
            100.0L*static_cast<long double>(initializedBytes)/
            static_cast<long double>(volumeBytes));
          progress.setValue(qBound(0, percent, 99));
          if (qApp)
            qApp->processEvents();
          if (progress.wasCanceled())
            {
              canceled = true;
              ok = false;
            }
        }

      qint64 expectedSize = 0;
      if (ok &&
          (!slabFileSize(ns, sliceBytes, expectedSize,
                         "create volume files") ||
           !flushAndCheckSize(temporary, expectedSize,
                              "create volume files")))
        ok = false;
      temporary.close();
      if (!ok)
        break;
    }

  if (canceled && m_lastError.isEmpty())
    setError("create volume files: allocation was canceled");
  if (!m_lastError.isEmpty())
    {
      cleanupFiles(temporaryFiles);
      return false;
    }

  QStringList backups;
  QList<bool> hadOriginal;
  QList<bool> originalMoved;
  QList<bool> newInstalled;
  for(int ns=0; ns<nslabs; ++ns)
    {
      const QString backup = finalFiles[ns] + ".drishti-backup-" + token;
      const bool existed = QFileInfo::exists(finalFiles[ns]);
      backups << backup;
      hadOriginal << existed;
      originalMoved << false;
      newInstalled << false;

      if (existed && !QFile::rename(finalFiles[ns], backup))
        {
          setError(QString("create volume files: cannot preserve existing '%1'")
                   .arg(finalFiles[ns]));
          break;
        }
      if (existed)
        originalMoved[ns] = true;
      if (!QFile::rename(temporaryFiles[ns], finalFiles[ns]))
        {
          setError(QString("create volume files: cannot install '%1'")
                   .arg(finalFiles[ns]));
          break;
        }
      newInstalled[ns] = true;
    }

  if (!m_lastError.isEmpty())
    {
      const QString installError = m_lastError;
      QStringList rollbackFailures;
      QStringList retainedBackups;
      for(int ns=backups.count()-1; ns>=0; --ns)
        {
          bool finalRemoved = !newInstalled[ns] ||
                              !QFileInfo::exists(finalFiles[ns]);
          if (!finalRemoved)
            finalRemoved = QFile::remove(finalFiles[ns]);
          if (!finalRemoved)
            rollbackFailures << QString("cannot remove replacement '%1'")
                                .arg(finalFiles[ns]);

          if (originalMoved[ns])
            {
              if (finalRemoved && QFileInfo::exists(backups[ns]) &&
                  QFile::rename(backups[ns], finalFiles[ns]))
                continue;
              rollbackFailures << QString("cannot restore '%1'")
                                  .arg(finalFiles[ns]);
              if (QFileInfo::exists(backups[ns]))
                retainedBackups << backups[ns];
            }
        }
      cleanupFiles(temporaryFiles);
      if (!rollbackFailures.isEmpty())
        m_lastError = installError + "; rollback incomplete: " +
                      rollbackFailures.join("; ");
      if (!retainedBackups.isEmpty())
        m_lastError += "; recoverable backups retained at: " +
                       retainedBackups.join(", ");
      return false;
    }

  m_transactionFinals = finalFiles;
  m_transactionBackups = backups;
  m_transactionHadOriginal = hadOriginal;
  m_slabno = m_prevslabno = -1;
  progress.setValue(100);
  return true;
}

bool
VolumeFileManager::commitFileCreation()
{
  clearError();
  QStringList failures;
  for(int i=0; i<m_transactionBackups.count(); ++i)
    {
      if (m_transactionHadOriginal.value(i) &&
          QFileInfo::exists(m_transactionBackups[i]) &&
          !QFile::remove(m_transactionBackups[i]))
        failures << m_transactionBackups[i];
    }
  clearTransaction();
  if (!failures.isEmpty())
    qWarning() << "Volume output committed; old backups could not be removed:"
               << failures;
  return true;
}

bool
VolumeFileManager::rollbackFileCreation()
{
  QStringList failures;
  QStringList remainingFinals;
  QStringList remainingBackups;
  QList<bool> remainingHadOriginal;
  for(int i=m_transactionFinals.count()-1; i>=0; --i)
    {
      bool restored = true;
      if (QFileInfo::exists(m_transactionFinals[i]) &&
          !QFile::remove(m_transactionFinals[i]))
        restored = false;
      if (restored && m_transactionHadOriginal.value(i))
        {
          if (!QFileInfo::exists(m_transactionBackups.value(i)) ||
              !QFile::rename(m_transactionBackups[i], m_transactionFinals[i]))
            restored = false;
        }
      if (!restored)
        {
          failures << QString("target '%1', backup '%2'")
                      .arg(m_transactionFinals[i],
                           m_transactionBackups.value(i));
          remainingFinals.prepend(m_transactionFinals[i]);
          remainingBackups.prepend(m_transactionBackups.value(i));
          remainingHadOriginal.prepend(m_transactionHadOriginal.value(i));
        }
    }
  m_transactionFinals = remainingFinals;
  m_transactionBackups = remainingBackups;
  m_transactionHadOriginal = remainingHadOriginal;
  if (!failures.isEmpty())
    return setError(QString("rollback volume files: cannot restore: %1")
                    .arg(failures.join(", ")));
  clearTransaction();
  return true;
}

uchar*
VolumeFileManager::getSlice(int ds)
{
  const QString operation = "read volume slice";
  clearError();
  if (!validateGeometry(operation) || ds < 0 || ds >= m_depth)
    {
      if (m_lastError.isEmpty())
        setError(QString("%1: slice index %2 is outside [0, %3)")
                 .arg(operation).arg(ds).arg(m_depth));
      return 0;
    }

  qint64 sliceBytes = 0;
  if (!sliceByteCount(sliceBytes, operation) ||
      !ensureSliceCapacity(sliceBytes, operation))
    return 0;

  const int d = m_slice0AtTop ? (m_depth-1)-ds : ds;
  m_slabno = d/m_slabSize;
  qint64 localBytes = 0;
  qint64 offset = 0;
  if (!checkedMultiply(static_cast<qint64>(d-m_slabno*m_slabSize),
                       sliceBytes, localBytes) ||
      !checkedAdd(m_header, localBytes, offset))
    {
      setError(QString("%1: file offset overflows").arg(operation));
      return 0;
    }

  m_filename = slabFilename(m_slabno);
  QFile file(m_filename);
  if (!file.open(QFile::ReadOnly))
    {
      setError(QString("%1: cannot open '%2': %3")
               .arg(operation).arg(m_filename).arg(file.errorString()));
      return 0;
    }
  const bool ok = seekFile(file, offset, operation) &&
                  readExact(file, m_slice, sliceBytes, operation);
  file.close();
  if (!ok)
    {
      std::memset(m_slice, 0, static_cast<std::size_t>(sliceBytes));
      return 0;
    }
  return m_slice;
}

bool
VolumeFileManager::setSlice(int ds, uchar *tmp)
{
  const QString operation = "write volume slice";
  clearError();
  if (!validateGeometry(operation) || ds < 0 || ds >= m_depth || !tmp)
    {
      if (m_lastError.isEmpty())
        setError(QString("%1: invalid slice index or buffer").arg(operation));
      return false;
    }

  qint64 sliceBytes = 0;
  if (!sliceByteCount(sliceBytes, operation))
    return false;

  const int d = m_slice0AtTop ? (m_depth-1)-ds : ds;
  m_slabno = d/m_slabSize;
  qint64 localBytes = 0;
  qint64 offset = 0;
  if (!checkedMultiply(static_cast<qint64>(d-m_slabno*m_slabSize),
                       sliceBytes, localBytes) ||
      !checkedAdd(m_header, localBytes, offset))
    return setError(QString("%1: file offset overflows").arg(operation));

  qint64 expectedSize = 0;
  if (!slabFileSize(m_slabno, sliceBytes, expectedSize, operation))
    return false;

  m_filename = slabFilename(m_slabno);
  QFile file(m_filename);
  if (!file.exists())
    return setError(QString("%1: '%2' does not exist")
                    .arg(operation).arg(m_filename));
  if (!file.open(QFile::ReadWrite))
    return setError(QString("%1: cannot open '%2': %3")
                    .arg(operation).arg(m_filename).arg(file.errorString()));
  if (file.size() != expectedSize)
    {
      const qint64 actualSize = file.size();
      file.close();
      return setError(QString("%1: '%2' has %3 bytes, expected %4")
                      .arg(operation).arg(m_filename)
                      .arg(actualSize).arg(expectedSize));
    }

  const bool ok = seekFile(file, offset, operation) &&
                  writeExact(file, tmp, sliceBytes, operation) &&
                  file.flush();
  if (!ok && m_lastError.isEmpty())
    setError(QString("%1: cannot flush '%2': %3")
             .arg(operation).arg(m_filename).arg(file.errorString()));
  file.close();
  return ok;
}
