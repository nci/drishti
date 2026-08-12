#include "filehandler.h"

#include "blosc.h"

#include <QFileInfo>
#include <QIODevice>
#include <QMutexLocker>
#include <QSaveFile>

#include <limits>
#include <memory>
#include <new>
#include <cstring>

namespace
{
const qint64 kCompressionBlockBytes = 8LL*1024LL*1024LL;
const qint64 kSnapshotCompressionBlockBytes = 8LL*1024LL*1024LL;
const qint64 kMaximumCompressionBlockBytes = 256LL*1024LL*1024LL;
const qint64 kCopyBlockBytes = 4LL*1024LL*1024LL;
}

FileHandler::FileHandler()
{
  reset();
}

FileHandler::~FileHandler()
{
  reset();
}

void
FileHandler::reset()
{
  if (m_qfile.isOpen())
    m_qfile.close();

  m_filename.clear();
  m_baseFilename.clear();
  m_filenames.clear();
  m_header = m_slabSize = 0;
  m_depth = m_width = m_height = 0;
  m_voxelType = 0;
  m_bytesPerVoxel = 1;
  m_volData = 0;
  m_volDataCapacity = 0;
  m_tempFile.clear();
  m_lastError.clear();
  m_savingFile.storeRelease(0);
  m_cancelRequested.storeRelease(0);
}

bool
FileHandler::canceled(const QString& operation)
{
  if (m_cancelRequested.loadAcquire() == 0)
    return false;
  setError(QString("%1: canceled after the save worker stopped making progress")
             .arg(operation));
  return true;
}

bool
FileHandler::checkedMultiply(qint64 a, qint64 b, qint64& result)
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
FileHandler::checkedAdd(qint64 a, qint64 b, qint64& result)
{
  if (a < 0 || b < 0 ||
      a > std::numeric_limits<qint64>::max()-b)
    return false;
  result = a+b;
  return true;
}

bool
FileHandler::setError(const QString& error)
{
  m_lastError = error;
  return false;
}

bool
FileHandler::readExact(QIODevice& device,
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
        device.read(reinterpret_cast<char*>(destination)+done, bytes-done);
      if (count <= 0)
        return setError(QString("%1: short read (%2 of %3 bytes): %4")
                        .arg(operation).arg(done).arg(bytes)
                        .arg(device.errorString()));
      done += count;
    }
  return true;
}

bool
FileHandler::writeExact(QIODevice& device,
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
        device.write(reinterpret_cast<const char*>(source)+done, bytes-done);
      if (count <= 0)
        return setError(QString("%1: short write (%2 of %3 bytes): %4")
                        .arg(operation).arg(done).arg(bytes)
                        .arg(device.errorString()));
      done += count;
    }
  return true;
}

bool
FileHandler::validateConfiguration(const QString& operation,
                                   qint64& voxelCount,
                                   qint64& volumeBytes)
{
  voxelCount = volumeBytes = 0;
  if (m_depth <= 0 || m_width <= 0 || m_height <= 0)
    return setError(QString("%1: invalid volume dimensions %2 x %3 x %4")
                    .arg(operation).arg(m_depth).arg(m_width).arg(m_height));
  if (m_bytesPerVoxel != 1 &&
      m_bytesPerVoxel != 2 &&
      m_bytesPerVoxel != 4)
    return setError(QString("%1: invalid voxel type %2")
                    .arg(operation).arg(m_voxelType));
  if (m_filenames.isEmpty() || m_filenames[0].isEmpty())
    return setError(QString("%1: no compressed mask filename is configured")
                    .arg(operation));
  if (!checkedMultiply(static_cast<qint64>(m_depth),
                       static_cast<qint64>(m_width), voxelCount) ||
      !checkedMultiply(voxelCount,
                       static_cast<qint64>(m_height), voxelCount) ||
      !checkedMultiply(voxelCount, m_bytesPerVoxel, volumeBytes) ||
      volumeBytes <= 0)
    return setError(QString("%1: volume byte count overflows").arg(operation));
  if (!m_volData || m_volDataCapacity < volumeBytes)
    return setError(QString("%1: volume buffer is unavailable or too small")
                    .arg(operation));
  return true;
}

void
FileHandler::setFilenameList(QStringList flist)
{
  m_filenames = flist;
  m_tempFile = m_filenames.isEmpty() ? QString() : m_filenames[0]+".tmp";
}

void FileHandler::setBaseFilename(QString bfn) { m_baseFilename = bfn; }
void FileHandler::setDepth(int d) { m_depth = d; }
void FileHandler::setWidth(int w) { m_width = w; }
void FileHandler::setHeight(int h) { m_height = h; }
void FileHandler::setHeaderSize(int hs) { m_header = hs; }
void FileHandler::setSlabSize(int ss) { m_slabSize = ss; }

void
FileHandler::setVoxelType(int vt)
{
  m_voxelType = vt;
  m_bytesPerVoxel = 0;
  if (vt == 0 || vt == 1)
    m_bytesPerVoxel = 1;
  else if (vt == 2 || vt == 3)
    m_bytesPerVoxel = 2;
  else if (vt == 4 || vt == 5)
    m_bytesPerVoxel = 4;
}

void
FileHandler::setVolData(uchar *data, qint64 capacity)
{
  m_volData = data;
  m_volDataCapacity = capacity;
}

bool
FileHandler::loadMemFile()
{
  if (m_filenames.isEmpty())
    return setError("load compressed mask: no filename is configured");
  return loadMemFile(m_filenames[0]);
}

bool
FileHandler::loadMemFile(QString filename)
{
  QMutexLocker locker(&m_mutex);
  const QString operation = "load compressed mask";
  m_lastError.clear();

  qint64 voxelCount = 0;
  qint64 destinationBytes = 0;
  if (!validateConfiguration(operation, voxelCount, destinationBytes))
    return false;

  QFile input(filename);
  if (!input.open(QFile::ReadOnly))
    return setError(QString("%1: cannot open '%2': %3")
                    .arg(operation).arg(filename).arg(input.errorString()));

  char magic[6] = {0, 0, 0, 0, 0, 0};
  uchar sourceVoxelType = 0;
  qint32 fileDepth = 0;
  qint32 fileWidth = 0;
  qint32 fileHeight = 0;
  qint32 blockCount = 0;
  qint32 blockBytes = 0;

  bool ok = readExact(input, reinterpret_cast<uchar*>(magic), 6, operation) &&
            readExact(input, &sourceVoxelType, 1, operation) &&
            readExact(input, reinterpret_cast<uchar*>(&fileDepth), 4, operation) &&
            readExact(input, reinterpret_cast<uchar*>(&fileWidth), 4, operation) &&
            readExact(input, reinterpret_cast<uchar*>(&fileHeight), 4, operation) &&
            readExact(input, reinterpret_cast<uchar*>(&blockCount), 4, operation) &&
            readExact(input, reinterpret_cast<uchar*>(&blockBytes), 4, operation);
  if (!ok)
    return false;

  if (memcmp(magic, "dpm100", 6) != 0)
    return setError(QString("%1: '%2' has an invalid signature")
                    .arg(operation).arg(filename));
  if (fileDepth != m_depth || fileWidth != m_width || fileHeight != m_height)
    return setError(QString("%1: grid size %2 x %3 x %4 does not match %5 x %6 x %7")
                    .arg(operation).arg(fileDepth).arg(fileWidth).arg(fileHeight)
                    .arg(m_depth).arg(m_width).arg(m_height));

  qint64 sourceBytesPerVoxel = 0;
  if (sourceVoxelType == 0 || sourceVoxelType == 1)
    sourceBytesPerVoxel = 1;
  else if (sourceVoxelType == 2 || sourceVoxelType == 3)
    sourceBytesPerVoxel = 2;
  else if (sourceVoxelType == 4 || sourceVoxelType == 5)
    sourceBytesPerVoxel = 4;
  else
    return setError(QString("%1: unsupported voxel type %2")
                    .arg(operation).arg(sourceVoxelType));

  if (sourceBytesPerVoxel != m_bytesPerVoxel &&
      !(sourceBytesPerVoxel == 1 && m_bytesPerVoxel == 2))
    return setError(QString("%1: cannot convert %2-byte labels to %3-byte labels")
                    .arg(operation).arg(sourceBytesPerVoxel)
                    .arg(m_bytesPerVoxel));

  qint64 sourceBytes = 0;
  if (!checkedMultiply(voxelCount, sourceBytesPerVoxel, sourceBytes) ||
      sourceBytes <= 0 || sourceBytes > m_volDataCapacity)
    return setError(QString("%1: source byte count is invalid").arg(operation));
  if (blockCount <= 0 || blockBytes <= 0 ||
      blockBytes > kMaximumCompressionBlockBytes)
    return setError(QString("%1: invalid block layout (%2 blocks of %3 bytes)")
                    .arg(operation).arg(blockCount).arg(blockBytes));

  const qint64 expectedBlocks = 1+(sourceBytes-1)/blockBytes;
  if (expectedBlocks != blockCount)
    return setError(QString("%1: block count %2 does not match expected %3")
                    .arg(operation).arg(blockCount).arg(expectedBlocks));

  qint64 maximumCompressedBytes = 0;
  if (!checkedAdd(blockBytes, BLOSC_MAX_OVERHEAD, maximumCompressedBytes) ||
      static_cast<quint64>(maximumCompressedBytes) >
      static_cast<quint64>(std::numeric_limits<size_t>::max()))
    return setError(QString("%1: compressed buffer size overflows")
                    .arg(operation));

  uchar *compressed =
    new (std::nothrow) uchar[static_cast<size_t>(maximumCompressedBytes)];
  if (!compressed)
    return setError(QString("%1: cannot allocate %2-byte compressed buffer")
                    .arg(operation).arg(maximumCompressedBytes));

  qint64 outputOffset = 0;
  for(qint32 block=0; block<blockCount && ok; ++block)
    {
      qint32 compressedBytes = 0;
      ok = readExact(input,
                     reinterpret_cast<uchar*>(&compressedBytes), 4,
                     operation);
      if (!ok)
        break;
      if (compressedBytes <= 0 || compressedBytes > maximumCompressedBytes ||
          compressedBytes > input.size()-input.pos())
        {
          ok = setError(QString("%1: invalid compressed size %2 in block %3")
                        .arg(operation).arg(compressedBytes).arg(block));
          break;
        }
      if (!readExact(input, compressed, compressedBytes, operation))
        {
          ok = false;
          break;
        }

      size_t validatedBytes = 0;
      if (blosc_cbuffer_validate(compressed,
                                 static_cast<size_t>(compressedBytes),
                                 &validatedBytes) != 0)
        {
          ok = setError(QString("%1: block %2 is not valid Blosc data")
                        .arg(operation).arg(block));
          break;
        }

      const qint64 expectedBytes = qMin<qint64>(blockBytes,
                                                sourceBytes-outputOffset);
      if (validatedBytes != static_cast<size_t>(expectedBytes))
        {
          ok = setError(QString("%1: block %2 expands to %3 bytes, expected %4")
                        .arg(operation).arg(block).arg(validatedBytes)
                        .arg(expectedBytes));
          break;
        }

      const int decompressed =
        blosc_decompress_ctx(compressed,
                             m_volData+outputOffset,
                             static_cast<size_t>(expectedBytes), 4);
      if (decompressed != expectedBytes)
        {
          ok = setError(QString("%1: block %2 decompressed to %3 bytes, expected %4")
                        .arg(operation).arg(block).arg(decompressed)
                        .arg(expectedBytes));
          break;
        }
      outputOffset += expectedBytes;
    }

  delete [] compressed;
  if (!ok)
    {
      input.close();
      return false;
    }
  if (outputOffset != sourceBytes)
    return setError(QString("%1: decoded %2 of %3 bytes")
                    .arg(operation).arg(outputOffset).arg(sourceBytes));
  if (input.pos() != input.size())
    return setError(QString("%1: unexpected trailing data in '%2'")
                    .arg(operation).arg(filename));
  input.close();

  if (sourceBytesPerVoxel == 1 && m_bytesPerVoxel == 2)
    {
      ushort *destination = reinterpret_cast<ushort*>(m_volData);
      for(qint64 voxel=voxelCount; voxel>0; --voxel)
        destination[voxel-1] = m_volData[voxel-1];
    }

  return true;
}

bool
FileHandler::saveMemFile(quint64 generation)
{
  QMutexLocker locker(&m_mutex);
  const QString operation = "save compressed mask";
  m_lastError.clear();
  m_cancelRequested.storeRelease(0);
  m_savingFile.storeRelease(1);
  emit fileSaveProgress(generation);

  qint64 voxelCount = 0;
  qint64 volumeBytes = 0;
  bool ok = validateConfiguration(operation, voxelCount, volumeBytes);

  const qint64 blockCount64 = ok ? 1+(volumeBytes-1)/kCompressionBlockBytes : 0;
  if (ok && blockCount64 > std::numeric_limits<qint32>::max())
    ok = setError(QString("%1: too many compression blocks").arg(operation));

  qint64 compressedCapacity = 0;
  if (ok &&
      (!checkedAdd(kCompressionBlockBytes, BLOSC_MAX_OVERHEAD,
                   compressedCapacity) ||
       static_cast<quint64>(compressedCapacity) >
       static_cast<quint64>(std::numeric_limits<size_t>::max())))
    ok = setError(QString("%1: compression buffer size overflows")
                  .arg(operation));

  uchar *compressed = 0;
  if (ok)
    {
      compressed =
        new (std::nothrow) uchar[static_cast<size_t>(compressedCapacity)];
      if (!compressed)
        ok = setError(QString("%1: cannot allocate %2-byte compression buffer")
                      .arg(operation).arg(compressedCapacity));
    }

  QSaveFile output(ok ? m_filenames[0] : QString());
  output.setDirectWriteFallback(false);
  if (ok && !output.open(QFile::WriteOnly))
    ok = setError(QString("%1: cannot open '%2': %3")
                  .arg(operation).arg(m_filenames[0]).arg(output.errorString()));

  const char magic[6] = {'d', 'p', 'm', '1', '0', '0'};
  const uchar fileVoxelType = static_cast<uchar>(m_voxelType);
  const qint32 fileDepth = m_depth;
  const qint32 fileWidth = m_width;
  const qint32 fileHeight = m_height;
  const qint32 blockCount = static_cast<qint32>(blockCount64);
  const qint32 blockBytes = static_cast<qint32>(kCompressionBlockBytes);

  if (ok)
    ok = writeExact(output, reinterpret_cast<const uchar*>(magic), 6, operation) &&
         writeExact(output, &fileVoxelType, 1, operation) &&
         writeExact(output, reinterpret_cast<const uchar*>(&fileDepth), 4, operation) &&
         writeExact(output, reinterpret_cast<const uchar*>(&fileWidth), 4, operation) &&
         writeExact(output, reinterpret_cast<const uchar*>(&fileHeight), 4, operation) &&
         writeExact(output, reinterpret_cast<const uchar*>(&blockCount), 4, operation) &&
         writeExact(output, reinterpret_cast<const uchar*>(&blockBytes), 4, operation);

  for(qint32 block=0; block<blockCount && ok; ++block)
    {
      emit fileSaveProgress(generation);
      if (canceled(operation))
        {
          ok = false;
          break;
        }
      const qint64 sourceOffset = static_cast<qint64>(block)*blockBytes;
      const qint64 sourceBytes = qMin<qint64>(blockBytes,
                                              volumeBytes-sourceOffset);
      const int compressedBytes =
        blosc_compress_ctx(3, BLOSC_SHUFFLE,
                           static_cast<size_t>(m_bytesPerVoxel),
                           static_cast<size_t>(sourceBytes),
                           m_volData+sourceOffset,
                           compressed,
                           static_cast<size_t>(compressedCapacity),
                           "blosclz", 0, 4);
      if (compressedBytes <= 0 || compressedBytes > compressedCapacity)
        {
          ok = setError(QString("%1: compression failed for block %2")
                        .arg(operation).arg(block));
          break;
        }
      const qint32 storedBytes = compressedBytes;
      ok = writeExact(output,
                      reinterpret_cast<const uchar*>(&storedBytes), 4,
                      operation) &&
           writeExact(output, compressed, storedBytes, operation);
      if (ok)
        emit fileSaveProgress(generation);
    }

  delete [] compressed;
  if (ok && canceled(operation))
    ok = false;
  if (ok)
    {
      if (!output.commit())
        ok = setError(QString("%1: cannot commit '%2': %3")
                      .arg(operation).arg(m_filenames[0])
                      .arg(output.errorString()));
    }
  else
    output.cancelWriting();

  m_savingFile.storeRelease(0);
  emit doneFileSave(generation, ok, m_lastError);
  return ok;
}

void
FileHandler::saveSnapshotFile(QString snapshotName, quint64 generation)
{
  QMutexLocker locker(&m_mutex);
  const QString operation = "save compressed mask snapshot";
  m_lastError.clear();
  m_cancelRequested.storeRelease(0);
  m_savingFile.storeRelease(1);
  emit fileSaveProgress(generation);

  qint64 voxelCount = 0;
  qint64 volumeBytes = 0;
  bool ok = validateConfiguration(operation, voxelCount, volumeBytes);

  QFile snapshot(snapshotName);
  if (ok && !snapshot.open(QFile::ReadOnly))
    ok = setError(QString("%1: cannot open '%2': %3")
                  .arg(operation).arg(snapshotName).arg(snapshot.errorString()));
  if (ok && snapshot.size() != volumeBytes)
    ok = setError(QString("%1: snapshot has %2 bytes, expected %3")
                  .arg(operation).arg(snapshot.size()).arg(volumeBytes));

  const qint64 blockCount64 =
    ok ? 1+(volumeBytes-1)/kSnapshotCompressionBlockBytes : 0;
  if (ok && blockCount64 > std::numeric_limits<qint32>::max())
    ok = setError(QString("%1: too many compression blocks").arg(operation));

  qint64 compressedCapacity = 0;
  if (ok &&
      (!checkedAdd(kSnapshotCompressionBlockBytes, BLOSC_MAX_OVERHEAD,
                   compressedCapacity) ||
       static_cast<quint64>(compressedCapacity) >
         static_cast<quint64>(std::numeric_limits<size_t>::max())))
    ok = setError(QString("%1: compression buffer size overflows")
                  .arg(operation));

  std::unique_ptr<uchar[]> raw;
  std::unique_ptr<uchar[]> compressed;
  if (ok)
    {
      raw.reset(new (std::nothrow)
                uchar[static_cast<size_t>(kSnapshotCompressionBlockBytes)]);
      compressed.reset(new (std::nothrow)
                       uchar[static_cast<size_t>(compressedCapacity)]);
      if (!raw || !compressed)
        ok = setError(QString("%1: cannot allocate bounded compression buffers")
                      .arg(operation));
    }

  QSaveFile output(ok ? m_filenames[0] : QString());
  output.setDirectWriteFallback(false);
  if (ok && !output.open(QFile::WriteOnly))
    ok = setError(QString("%1: cannot open '%2': %3")
                  .arg(operation).arg(m_filenames[0]).arg(output.errorString()));

  const char magic[6] = {'d', 'p', 'm', '1', '0', '0'};
  const uchar fileVoxelType = static_cast<uchar>(m_voxelType);
  const qint32 fileDepth = m_depth;
  const qint32 fileWidth = m_width;
  const qint32 fileHeight = m_height;
  const qint32 blockCount = static_cast<qint32>(blockCount64);
  const qint32 blockBytes =
    static_cast<qint32>(kSnapshotCompressionBlockBytes);

  if (ok)
    ok = writeExact(output, reinterpret_cast<const uchar*>(magic), 6, operation) &&
         writeExact(output, &fileVoxelType, 1, operation) &&
         writeExact(output, reinterpret_cast<const uchar*>(&fileDepth), 4, operation) &&
         writeExact(output, reinterpret_cast<const uchar*>(&fileWidth), 4, operation) &&
         writeExact(output, reinterpret_cast<const uchar*>(&fileHeight), 4, operation) &&
         writeExact(output, reinterpret_cast<const uchar*>(&blockCount), 4, operation) &&
         writeExact(output, reinterpret_cast<const uchar*>(&blockBytes), 4, operation);

  qint64 sourceOffset = 0;
  for(qint32 block=0; block<blockCount && ok; ++block)
    {
      emit fileSaveProgress(generation);
      if (canceled(operation))
        {
          ok = false;
          break;
        }
      const qint64 sourceBytes =
        qMin<qint64>(blockBytes, volumeBytes-sourceOffset);
      if (!readExact(snapshot, raw.get(), sourceBytes, operation))
        {
          ok = false;
          break;
        }

      const int compressedBytes =
        blosc_compress_ctx(3, BLOSC_SHUFFLE,
                           static_cast<size_t>(m_bytesPerVoxel),
                           static_cast<size_t>(sourceBytes),
                           raw.get(), compressed.get(),
                           static_cast<size_t>(compressedCapacity),
                           "blosclz", 0, 4);
      if (compressedBytes <= 0 || compressedBytes > compressedCapacity)
        {
          ok = setError(QString("%1: compression failed for block %2")
                        .arg(operation).arg(block));
          break;
        }

      const qint32 storedBytes = compressedBytes;
      ok = writeExact(output,
                      reinterpret_cast<const uchar*>(&storedBytes), 4,
                      operation) &&
           writeExact(output, compressed.get(), storedBytes, operation);
      sourceOffset += sourceBytes;
      if (ok)
        emit fileSaveProgress(generation);
    }

  if (ok && (sourceOffset != volumeBytes || snapshot.pos() != snapshot.size()))
    ok = setError(QString("%1: consumed %2 of %3 snapshot bytes")
                  .arg(operation).arg(sourceOffset).arg(volumeBytes));

  snapshot.close();
  if (ok && canceled(operation))
    ok = false;
  if (ok)
    {
      if (!output.commit())
        ok = setError(QString("%1: cannot commit '%2': %3")
                      .arg(operation).arg(m_filenames[0])
                      .arg(output.errorString()));
    }
  else
    output.cancelWriting();

  if (!QFile::remove(snapshotName) && QFileInfo::exists(snapshotName) && ok)
    m_lastError = QString("saved mask but could not remove snapshot '%1'")
                    .arg(snapshotName);

  m_savingFile.storeRelease(0);
  emit doneFileSave(generation, ok, m_lastError);
}

bool
FileHandler::copyFileTransactional(const QString& sourceName,
                                   const QString& destinationName)
{
  QFile source(sourceName);
  if (!source.open(QFile::ReadOnly))
    return setError(QString("copy file: cannot open '%1': %2")
                    .arg(sourceName).arg(source.errorString()));

  QSaveFile destination(destinationName);
  destination.setDirectWriteFallback(false);
  if (!destination.open(QFile::WriteOnly))
    return setError(QString("copy file: cannot open '%1': %2")
                    .arg(destinationName).arg(destination.errorString()));

  uchar *buffer = new (std::nothrow) uchar[static_cast<size_t>(kCopyBlockBytes)];
  if (!buffer)
    return setError("copy file: cannot allocate copy buffer");

  bool ok = true;
  while (!source.atEnd())
    {
      const qint64 count = source.read(reinterpret_cast<char*>(buffer),
                                       kCopyBlockBytes);
      if (count <= 0)
        {
          ok = setError(QString("copy file: cannot read '%1': %2")
                        .arg(sourceName).arg(source.errorString()));
          break;
        }
      if (!writeExact(destination, buffer, count, "copy file"))
        {
          ok = false;
          break;
        }
    }

  delete [] buffer;
  source.close();
  if (ok && !destination.commit())
    ok = setError(QString("copy file: cannot commit '%1': %2")
                  .arg(destinationName).arg(destination.errorString()));
  if (!ok)
    destination.cancelWriting();
  return ok;
}

bool
FileHandler::genUndo()
{
  QMutexLocker locker(&m_mutex);
  m_lastError.clear();
  if (m_filenames.isEmpty() || m_tempFile.isEmpty())
    return setError("create undo file: no mask filename is configured");
  if (!QFileInfo::exists(m_filenames[0]))
    return setError(QString("create undo file: '%1' does not exist")
                    .arg(m_filenames[0]));
  return copyFileTransactional(m_filenames[0], m_tempFile);
}

bool
FileHandler::undo()
{
  QMutexLocker locker(&m_mutex);
  const QString operation = "restore undo file";
  m_lastError.clear();
  if (m_filenames.isEmpty() || m_tempFile.isEmpty() ||
      !QFileInfo::exists(m_tempFile))
    return setError(QString("%1: no undo file is available").arg(operation));

  qint64 voxelCount = 0;
  qint64 volumeBytes = 0;
  if (!validateConfiguration(operation, voxelCount, volumeBytes) ||
      static_cast<quint64>(volumeBytes) >
        static_cast<quint64>(std::numeric_limits<size_t>::max()))
    return false;

  std::unique_ptr<uchar[]> restored(
    new (std::nothrow) uchar[static_cast<size_t>(volumeBytes)]);
  if (!restored)
    return setError(QString("%1: cannot allocate %2-byte staging buffer")
                    .arg(operation).arg(volumeBytes));

  FileHandler validator;
  QStringList undoFiles;
  undoFiles << m_tempFile;
  validator.setFilenameList(undoFiles);
  validator.setBaseFilename(m_baseFilename);
  validator.setDepth(m_depth);
  validator.setWidth(m_width);
  validator.setHeight(m_height);
  validator.setHeaderSize(static_cast<int>(m_header));
  validator.setSlabSize(static_cast<int>(m_slabSize));
  validator.setVoxelType(m_voxelType);
  validator.setVolData(restored.get(), volumeBytes);
  if (!validator.loadMemFile())
    return setError(QString("%1: %2")
                    .arg(operation).arg(validator.lastError()));

  if (!copyFileTransactional(m_tempFile, m_filenames[0]))
    return false;

  memcpy(m_volData, restored.get(), static_cast<size_t>(volumeBytes));
  return true;
}

void
FileHandler::saveDataBlock()
{
  if (!savingFile())
    saveMemFile(0);
}
