#include "checkpointhandler.h"
#include "blosc.h"

#include <QApplication>
#include <QByteArray>
#include <QFile>
#include <QFileInfo>
#include <QInputDialog>
#include <QMessageBox>
#include <QMutex>
#include <QMutexLocker>
#include <QProgressDialog>
#include <QSaveFile>
#include <QVector>

#include <cstring>
#include <limits>
#include <memory>
#include <new>

namespace
{
const qint32 kMaximumRecords = 10000;
const qint64 kFatEntryBytes = 100;
const qint64 kDescriptionBytes = 84;
const qint64 kDirectoryBytes = 4LL+kMaximumRecords*kFatEntryBytes;
const qint64 kRecordHeaderBytes = 1LL+5LL*4LL;
const qint64 kWriteCompressionBlockBytes = 8LL*1024LL*1024LL;
const qint64 kMaximumCompressionBlockBytes = 100LL*1024LL*1024LL;
const int kCopyBlockBytes = 4*1024*1024;
const int kCompressionThreads = 4;

struct CheckpointRecord
{
  qint64 offset;
  qint64 size;
  QByteArray descriptionField;
  QString description;
  int voxelType;
  qint32 depth;
  qint32 width;
  qint32 height;
  qint32 blockCount;
  qint32 blockBytes;
  qint32 maximumStoredBytes;
  qint64 volumeBytes;

  CheckpointRecord()
    : offset(0), size(0), voxelType(-1), depth(0), width(0), height(0),
      blockCount(0), blockBytes(0), maximumStoredBytes(0), volumeBytes(0)
  {
  }
};

QMutex g_errorMutex;
QString g_lastError;

void clearError()
{
  QMutexLocker locker(&g_errorMutex);
  g_lastError.clear();
}

bool setError(const QString& error)
{
  QMutexLocker locker(&g_errorMutex);
  g_lastError = error;
  return false;
}

QString currentError()
{
  QMutexLocker locker(&g_errorMutex);
  return g_lastError;
}

void reportError(const QString& title)
{
  if (qApp)
    QMessageBox::critical(0, title, currentError());
}

void reportInformation(const QString& title, const QString& message)
{
  if (qApp)
    QMessageBox::information(0, title, message);
}

void updateProgress(QProgressDialog *progress, int value)
{
  if (!progress)
    return;
  progress->setValue(qBound(0, value, 100));
  if (qApp)
    qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
}

bool checkedAdd(qint64 first, qint64 second, qint64& result)
{
  if (first < 0 || second < 0 ||
      first > std::numeric_limits<qint64>::max()-second)
    return false;
  result = first+second;
  return true;
}

bool checkedMultiply(qint64 first, qint64 second, qint64& result)
{
  if (first < 0 || second < 0)
    return false;
  if (first == 0 || second == 0)
    {
      result = 0;
      return true;
    }
  if (first > std::numeric_limits<qint64>::max()/second)
    return false;
  result = first*second;
  return true;
}

int bytesPerVoxel(int voxelType)
{
  if (voxelType == 0 || voxelType == 1)
    return 1;
  if (voxelType == 2 || voxelType == 3)
    return 2;
  return 0;
}

bool volumeByteCount(int voxelType,
                     qint64 depth, qint64 width, qint64 height,
                     qint64& bytes,
                     const QString& operation)
{
  const int voxelBytes = bytesPerVoxel(voxelType);
  qint64 voxels = 0;
  if (voxelBytes == 0)
    return setError(QString("%1: unsupported checkpoint voxel type %2")
                    .arg(operation).arg(voxelType));
  if (depth <= 0 || width <= 0 || height <= 0)
    return setError(QString("%1: invalid grid size %2 x %3 x %4")
                    .arg(operation).arg(depth).arg(width).arg(height));
  if (!checkedMultiply(depth, width, voxels) ||
      !checkedMultiply(voxels, height, voxels) ||
      !checkedMultiply(voxels, voxelBytes, bytes) || bytes <= 0)
    return setError(QString("%1: volume byte count overflows").arg(operation));
  if (static_cast<quint64>(bytes) >
      static_cast<quint64>(std::numeric_limits<size_t>::max()))
    return setError(QString("%1: volume is too large for this process")
                    .arg(operation));
  return true;
}

bool seekExact(QIODevice& device, qint64 offset, const QString& operation)
{
  if (offset < 0 || !device.seek(offset))
    return setError(QString("%1: cannot seek to byte %2: %3")
                    .arg(operation).arg(offset).arg(device.errorString()));
  return true;
}

bool readExact(QIODevice& device, char *destination, qint64 bytes,
               const QString& operation)
{
  if (bytes < 0 || (!destination && bytes > 0))
    return setError(QString("%1: invalid read buffer").arg(operation));

  qint64 done = 0;
  while (done < bytes)
    {
      const qint64 count = device.read(destination+done, bytes-done);
      if (count <= 0)
        return setError(QString("%1: short read (%2 of %3 bytes): %4")
                        .arg(operation).arg(done).arg(bytes)
                        .arg(device.errorString()));
      done += count;
    }
  return true;
}

bool writeExact(QIODevice& device, const char *source, qint64 bytes,
                const QString& operation)
{
  if (bytes < 0 || (!source && bytes > 0))
    return setError(QString("%1: invalid write buffer").arg(operation));

  qint64 done = 0;
  while (done < bytes)
    {
      const qint64 count = device.write(source+done, bytes-done);
      if (count <= 0)
        return setError(QString("%1: short write (%2 of %3 bytes): %4")
                        .arg(operation).arg(done).arg(bytes)
                        .arg(device.errorString()));
      done += count;
    }
  return true;
}

QByteArray encodeDescription(QString description)
{
  description.replace(QChar('\0'), QChar(' '));
  QByteArray encoded = description.toUtf8();
  while (encoded.size() >= kDescriptionBytes && !description.isEmpty())
    {
      description.chop(1);
      encoded = description.toUtf8();
    }

  QByteArray field(static_cast<int>(kDescriptionBytes), '\0');
  if (!encoded.isEmpty())
    memcpy(field.data(), encoded.constData(), static_cast<size_t>(encoded.size()));
  return field;
}

QString decodeDescription(const QByteArray& field)
{
  int length = 0;
  while (length < field.size() && field[length] != '\0')
    ++length;
  return QString::fromUtf8(field.constData(), length);
}

bool validateRecordLayout(QFile& input,
                          CheckpointRecord& record,
                          int recordIndex)
{
  const QString operation =
    QString("validate checkpoint record %1").arg(recordIndex+1);
  qint64 recordEnd = 0;
  if (record.offset < kDirectoryBytes || record.size < kRecordHeaderBytes ||
      !checkedAdd(record.offset, record.size, recordEnd) ||
      recordEnd > input.size())
    return setError(QString("%1: invalid record range [%2, %3)")
                    .arg(operation).arg(record.offset).arg(recordEnd));
  if (!seekExact(input, record.offset, operation))
    return false;

  uchar storedVoxelType = 0;
  qint32 depth = 0;
  qint32 width = 0;
  qint32 height = 0;
  qint32 blockCount = 0;
  qint32 blockBytes = 0;
  if (!readExact(input, reinterpret_cast<char*>(&storedVoxelType), 1,
                 operation) ||
      !readExact(input, reinterpret_cast<char*>(&depth), 4, operation) ||
      !readExact(input, reinterpret_cast<char*>(&width), 4, operation) ||
      !readExact(input, reinterpret_cast<char*>(&height), 4, operation) ||
      !readExact(input, reinterpret_cast<char*>(&blockCount), 4, operation) ||
      !readExact(input, reinterpret_cast<char*>(&blockBytes), 4, operation))
    return false;

  qint64 volumeBytes = 0;
  if (!volumeByteCount(storedVoxelType, depth, width, height,
                       volumeBytes, operation))
    return false;
  if (blockCount <= 0 || blockBytes <= 0 ||
      blockBytes > kMaximumCompressionBlockBytes)
    return setError(QString("%1: invalid block layout (%2 blocks of %3 bytes)")
                    .arg(operation).arg(blockCount).arg(blockBytes));

  const qint64 expectedBlocks = 1+(volumeBytes-1)/blockBytes;
  if (expectedBlocks != blockCount)
    return setError(QString("%1: block count %2 does not match expected %3")
                    .arg(operation).arg(blockCount).arg(expectedBlocks));

  qint64 outputOffset = 0;
  for(qint32 block=0; block<blockCount; ++block)
    {
      qint32 compressedBytes = 0;
      if (!readExact(input, reinterpret_cast<char*>(&compressedBytes), 4,
                     operation))
        return false;

      const qint64 expectedBytes =
        qMin<qint64>(blockBytes, volumeBytes-outputOffset);
      qint64 maximumCompressedBytes = 0;
      qint64 payloadEnd = 0;
      if (compressedBytes <= 0 || expectedBytes <= 0 ||
          !checkedAdd(blockBytes, BLOSC_MAX_OVERHEAD,
                      maximumCompressedBytes) ||
          compressedBytes > maximumCompressedBytes ||
          !checkedAdd(input.pos(), compressedBytes, payloadEnd) ||
          payloadEnd > recordEnd)
        return setError(QString("%1: invalid compressed length %2 in block %3")
                        .arg(operation).arg(compressedBytes).arg(block));
      if (!seekExact(input, payloadEnd, operation) ||
          !checkedAdd(outputOffset, expectedBytes, outputOffset))
        return false;
      record.maximumStoredBytes =
        qMax(record.maximumStoredBytes, compressedBytes);
    }

  if (outputOffset != volumeBytes || input.pos() != recordEnd)
    return setError(QString("%1: record size does not match its block data")
                    .arg(operation));

  record.voxelType = storedVoxelType;
  record.depth = depth;
  record.width = width;
  record.height = height;
  record.blockCount = blockCount;
  record.blockBytes = blockBytes;
  record.volumeBytes = volumeBytes;
  return true;
}

bool readArchive(QFile& input, QVector<CheckpointRecord>& records)
{
  const QString operation = "read checkpoint archive";
  records.clear();
  if (!input.open(QFile::ReadOnly))
    return setError(QString("%1: cannot open '%2': %3")
                    .arg(operation, input.fileName(), input.errorString()));
  if (input.size() < kDirectoryBytes)
    return setError(QString("%1: '%2' is %3 bytes, expected at least %4")
                    .arg(operation, input.fileName())
                    .arg(input.size()).arg(kDirectoryBytes));

  qint32 recordCount = 0;
  if (!readExact(input, reinterpret_cast<char*>(&recordCount), 4, operation))
    return false;
  if (recordCount < 0 || recordCount > kMaximumRecords)
    return setError(QString("%1: invalid record count %2")
                    .arg(operation).arg(recordCount));

  qint64 expectedOffset = kDirectoryBytes;
  records.reserve(recordCount);
  for(qint32 index=0; index<recordCount; ++index)
    {
      CheckpointRecord record;
      record.descriptionField.resize(static_cast<int>(kDescriptionBytes));
      if (!readExact(input, reinterpret_cast<char*>(&record.offset), 8,
                     operation) ||
          !readExact(input, reinterpret_cast<char*>(&record.size), 8,
                     operation) ||
          !readExact(input, record.descriptionField.data(),
                     kDescriptionBytes, operation))
        return false;

      qint64 end = 0;
      if (record.offset != expectedOffset ||
          record.size < kRecordHeaderBytes ||
          !checkedAdd(record.offset, record.size, end) || end > input.size())
        return setError(QString("%1: FAT entry %2 has invalid range [%3, %4)")
                        .arg(operation).arg(index+1)
                        .arg(record.offset).arg(end));
      expectedOffset = end;
      record.description = decodeDescription(record.descriptionField);
      records.append(record);
    }

  if (expectedOffset != input.size())
    return setError(QString("%1: archive has %2 unreferenced trailing bytes")
                    .arg(operation).arg(input.size()-expectedOffset));

  for(int index=0; index<records.size(); ++index)
    if (!validateRecordLayout(input, records[index], index))
      return false;
  return true;
}

bool writeZeroBytes(QIODevice& output, qint64 bytes,
                    const QString& operation)
{
  if (bytes < 0)
    return setError(QString("%1: zero-fill byte count is invalid")
                    .arg(operation));
  QByteArray zeros(kCopyBlockBytes, '\0');
  qint64 written = 0;
  while (written < bytes)
    {
      const qint64 count = qMin<qint64>(zeros.size(), bytes-written);
      if (!writeExact(output, zeros.constData(), count, operation) ||
          !checkedAdd(written, count, written))
        return false;
    }
  return true;
}

bool writeDirectory(QIODevice& output,
                    const QVector<CheckpointRecord>& records)
{
  const QString operation = "write checkpoint directory";
  if (records.size() > kMaximumRecords)
    return setError(QString("%1: too many records (%2)")
                    .arg(operation).arg(records.size()));
  if (!seekExact(output, 0, operation))
    return false;

  const qint32 recordCount = records.size();
  if (!writeExact(output, reinterpret_cast<const char*>(&recordCount), 4,
                  operation))
    return false;
  for(int index=0; index<records.size(); ++index)
    {
      const CheckpointRecord& record = records[index];
      qint64 end = 0;
      if (record.offset < kDirectoryBytes || record.size < kRecordHeaderBytes ||
          !checkedAdd(record.offset, record.size, end))
        return setError(QString("%1: record %2 has an invalid range")
                        .arg(operation).arg(index+1));

      QByteArray entry(static_cast<int>(kFatEntryBytes), '\0');
      memcpy(entry.data(), &record.offset, 8);
      memcpy(entry.data()+8, &record.size, 8);
      const QByteArray description =
        record.descriptionField.size() == kDescriptionBytes ?
          record.descriptionField : encodeDescription(record.description);
      memcpy(entry.data()+16, description.constData(),
             static_cast<size_t>(kDescriptionBytes));
      if (!writeExact(output, entry.constData(), kFatEntryBytes, operation))
        return false;
    }

  qint64 unusedEntries = kMaximumRecords-records.size();
  qint64 unusedBytes = 0;
  if (!checkedMultiply(unusedEntries, kFatEntryBytes, unusedBytes) ||
      !writeZeroBytes(output, unusedBytes, operation) ||
      output.pos() != kDirectoryBytes)
    return setError(QString("%1: directory size is invalid").arg(operation));
  return true;
}

bool copyRange(QFile& input, QIODevice& output,
               qint64 sourceOffset, qint64 bytes,
               QByteArray& buffer,
               const QString& operation)
{
  qint64 sourceEnd = 0;
  qint64 outputEnd = 0;
  if (sourceOffset < 0 || bytes < 0 ||
      !checkedAdd(sourceOffset, bytes, sourceEnd) || sourceEnd > input.size() ||
      output.pos() < 0 || !checkedAdd(output.pos(), bytes, outputEnd))
    return setError(QString("%1: copy range is invalid").arg(operation));
  if (!seekExact(input, sourceOffset, operation))
    return false;

  qint64 copied = 0;
  while (copied < bytes)
    {
      const qint64 count = qMin<qint64>(buffer.size(), bytes-copied);
      if (!readExact(input, buffer.data(), count, operation) ||
          !writeExact(output, buffer.constData(), count, operation) ||
          !checkedAdd(copied, count, copied))
        return false;
    }
  if (output.pos() != outputEnd)
    return setError(QString("%1: copied byte count is invalid").arg(operation));
  return true;
}

bool writeCompressedRecord(QIODevice& output,
                           int voxelType,
                           int depth, int width, int height,
                           const uchar *volumeData,
                           const QString& description,
                           CheckpointRecord& record,
                           QProgressDialog *progress)
{
  const QString operation = "write checkpoint record";
  qint64 volumeBytes = 0;
  if (!volumeData ||
      !volumeByteCount(voxelType, depth, width, height,
                       volumeBytes, operation))
    {
      if (volumeData == 0 && currentError().isEmpty())
        setError(QString("%1: volume buffer is null").arg(operation));
      return false;
    }

  const qint64 blockCount64 =
    1+(volumeBytes-1)/kWriteCompressionBlockBytes;
  if (blockCount64 <= 0 ||
      blockCount64 > std::numeric_limits<qint32>::max())
    return setError(QString("%1: block count overflows").arg(operation));
  qint64 compressedCapacity = 0;
  if (!checkedAdd(kWriteCompressionBlockBytes, BLOSC_MAX_OVERHEAD,
                  compressedCapacity) ||
      static_cast<quint64>(compressedCapacity) >
        static_cast<quint64>(std::numeric_limits<size_t>::max()))
    return setError(QString("%1: compression buffer size overflows")
                    .arg(operation));

  std::unique_ptr<uchar[]> compressed(
    new (std::nothrow) uchar[static_cast<size_t>(compressedCapacity)]);
  if (!compressed)
    return setError(QString("%1: cannot allocate %2-byte compression buffer")
                    .arg(operation).arg(compressedCapacity));

  record = CheckpointRecord();
  record.offset = output.pos();
  if (record.offset < kDirectoryBytes)
    return setError(QString("%1: invalid output offset %2")
                    .arg(operation).arg(record.offset));
  record.description = description;
  record.descriptionField = encodeDescription(description);
  record.voxelType = voxelType;
  record.depth = depth;
  record.width = width;
  record.height = height;
  record.blockCount = static_cast<qint32>(blockCount64);
  record.blockBytes = static_cast<qint32>(kWriteCompressionBlockBytes);
  record.volumeBytes = volumeBytes;

  const uchar storedVoxelType = static_cast<uchar>(voxelType);
  if (!writeExact(output,
                  reinterpret_cast<const char*>(&storedVoxelType), 1,
                  operation) ||
      !writeExact(output, reinterpret_cast<const char*>(&record.depth), 4,
                  operation) ||
      !writeExact(output, reinterpret_cast<const char*>(&record.width), 4,
                  operation) ||
      !writeExact(output, reinterpret_cast<const char*>(&record.height), 4,
                  operation) ||
      !writeExact(output, reinterpret_cast<const char*>(&record.blockCount), 4,
                  operation) ||
      !writeExact(output, reinterpret_cast<const char*>(&record.blockBytes), 4,
                  operation))
    return false;

  qint64 sourceOffset = 0;
  for(qint32 block=0; block<record.blockCount; ++block)
    {
      const qint64 sourceBytes =
        qMin<qint64>(record.blockBytes, volumeBytes-sourceOffset);
      const int compressedBytes =
        blosc_compress_ctx(3, BLOSC_SHUFFLE,
                           static_cast<size_t>(bytesPerVoxel(voxelType)),
                           static_cast<size_t>(sourceBytes),
                           volumeData+sourceOffset,
                           compressed.get(),
                           static_cast<size_t>(compressedCapacity),
                           "blosclz", 0, kCompressionThreads);
      if (compressedBytes <= 0 || compressedBytes > compressedCapacity)
        return setError(QString("%1: compression failed for block %2")
                        .arg(operation).arg(block));
      record.maximumStoredBytes =
        qMax(record.maximumStoredBytes, static_cast<qint32>(compressedBytes));

      const qint32 storedBytes = compressedBytes;
      if (!writeExact(output,
                      reinterpret_cast<const char*>(&storedBytes), 4,
                      operation) ||
          !writeExact(output,
                      reinterpret_cast<const char*>(compressed.get()),
                      storedBytes, operation) ||
          !checkedAdd(sourceOffset, sourceBytes, sourceOffset))
        return false;
      updateProgress(progress,
                     10+static_cast<int>(80LL*(block+1)/record.blockCount));
    }

  const qint64 recordEnd = output.pos();
  if (sourceOffset != volumeBytes || recordEnd < record.offset)
    return setError(QString("%1: compressed byte count is invalid")
                    .arg(operation));
  record.size = recordEnd-record.offset;
  if (record.size < kRecordHeaderBytes)
    return setError(QString("%1: record size is invalid").arg(operation));
  return true;
}

bool recordMatches(const CheckpointRecord& record,
                   int voxelType, int depth, int width, int height)
{
  return record.voxelType == voxelType &&
         record.depth == depth && record.width == width &&
         record.height == height;
}

bool loadRecord(QFile& input,
                const CheckpointRecord& record,
                int voxelType, int depth, int width, int height,
                uchar *volumeData,
                QProgressDialog *progress)
{
  const QString operation = "restore checkpoint record";
  qint64 volumeBytes = 0;
  if (!volumeData)
    return setError(QString("%1: destination buffer is null").arg(operation));
  if (!volumeByteCount(voxelType, depth, width, height,
                       volumeBytes, operation))
    return false;
  if (!recordMatches(record, voxelType, depth, width, height) ||
      record.volumeBytes != volumeBytes)
    return setError(QString("%1: checkpoint grid or voxel type does not match")
                    .arg(operation));

  std::unique_ptr<uchar[]> staging(
    new (std::nothrow) uchar[static_cast<size_t>(volumeBytes)]);
  if (!staging)
    return setError(QString("%1: cannot allocate %2-byte staging buffer")
                    .arg(operation).arg(volumeBytes));

  const qint64 compressedCapacity = record.maximumStoredBytes;
  if (compressedCapacity <= 0 ||
      static_cast<quint64>(compressedCapacity) >
        static_cast<quint64>(std::numeric_limits<size_t>::max()))
    return setError(QString("%1: compressed buffer size is invalid")
                    .arg(operation));
  std::unique_ptr<uchar[]> compressed(
    new (std::nothrow) uchar[static_cast<size_t>(compressedCapacity)]);
  if (!compressed)
    return setError(QString("%1: cannot allocate %2-byte compressed buffer")
                    .arg(operation).arg(compressedCapacity));

  qint64 payloadStart = 0;
  qint64 recordEnd = 0;
  if (!checkedAdd(record.offset, kRecordHeaderBytes, payloadStart) ||
      !checkedAdd(record.offset, record.size, recordEnd) ||
      !seekExact(input, payloadStart, operation))
    return false;

  qint64 outputOffset = 0;
  for(qint32 block=0; block<record.blockCount; ++block)
    {
      qint32 compressedBytes = 0;
      if (!readExact(input, reinterpret_cast<char*>(&compressedBytes), 4,
                     operation))
        return false;
      const qint64 expectedBytes =
        qMin<qint64>(record.blockBytes, volumeBytes-outputOffset);
      qint64 maximumCompressedBytes = 0;
      qint64 payloadEnd = 0;
      if (compressedBytes <= 0 || expectedBytes <= 0 ||
          !checkedAdd(record.blockBytes, BLOSC_MAX_OVERHEAD,
                      maximumCompressedBytes) ||
          compressedBytes > maximumCompressedBytes ||
          compressedBytes > compressedCapacity ||
          !checkedAdd(input.pos(), compressedBytes, payloadEnd) ||
          payloadEnd > recordEnd)
        return setError(QString("%1: invalid compressed length %2 in block %3")
                        .arg(operation).arg(compressedBytes).arg(block));
      if (!readExact(input, reinterpret_cast<char*>(compressed.get()),
                     compressedBytes, operation))
        return false;

      size_t validatedBytes = 0;
      if (blosc_cbuffer_validate(compressed.get(),
                                 static_cast<size_t>(compressedBytes),
                                 &validatedBytes) != 0 ||
          validatedBytes != static_cast<size_t>(expectedBytes))
        return setError(QString("%1: invalid Blosc data in block %2")
                        .arg(operation).arg(block));
      const int decompressed =
        blosc_decompress_ctx(compressed.get(),
                             staging.get()+outputOffset,
                             static_cast<size_t>(expectedBytes),
                             kCompressionThreads);
      if (decompressed != expectedBytes)
        return setError(QString("%1: block %2 decompressed to %3 bytes, expected %4")
                        .arg(operation).arg(block).arg(decompressed)
                        .arg(expectedBytes));
      if (!checkedAdd(outputOffset, expectedBytes, outputOffset))
        return setError(QString("%1: output offset overflows").arg(operation));
      updateProgress(progress,
                     10+static_cast<int>(80LL*(block+1)/record.blockCount));
    }

  if (outputOffset != volumeBytes || input.pos() != recordEnd)
    return setError(QString("%1: decoded record size is invalid")
                    .arg(operation));

  memcpy(volumeData, staging.get(), static_cast<size_t>(volumeBytes));
  return true;
}

QStringList recordChoices(const QVector<CheckpointRecord>& records)
{
  QStringList choices;
  for(int index=0; index<records.size(); ++index)
    {
      const QString description = records[index].description.isEmpty() ?
        QString("(unnamed)") : records[index].description;
      choices << QString("%1. %2").arg(index+1).arg(description);
    }
  return choices;
}

bool selectRecord(const QVector<CheckpointRecord>& records,
                  const QString& title,
                  const QString& prompt,
                  int& selectedIndex)
{
  selectedIndex = -1;
  if (!qApp)
    return setError("select checkpoint record: a GUI application is required");
  const QStringList choices = recordChoices(records);
  bool accepted = false;
  const QString selected =
    QInputDialog::getItem(0, title, prompt, choices, 0, false, &accepted);
  if (!accepted)
    {
      clearError();
      return false;
    }
  selectedIndex = choices.indexOf(selected);
  if (selectedIndex < 0 || selectedIndex >= records.size())
    return setError("select checkpoint record: invalid selection");
  return true;
}

QProgressDialog* createProgress(const QString& label)
{
  if (!qApp)
    return 0;
  QProgressDialog *progress =
    new (std::nothrow) QProgressDialog(label, QString(), 0, 100, 0,
                                      Qt::WindowStaysOnTopHint);
  if (progress)
    {
      progress->setMinimumDuration(0);
      progress->setCancelButton(0);
      progress->setWindowModality(Qt::ApplicationModal);
    }
  return progress;
}

bool validateRequest(const QString& filename,
                     int voxelType,
                     int depth, int width, int height,
                     const uchar *volumeData,
                     const QString& operation,
                     const QString& bufferDescription)
{
  qint64 ignoredBytes = 0;
  if (filename.isEmpty())
    return setError(QString("%1: filename is empty").arg(operation));
  if (!volumeData)
    return setError(QString("%1: %2 buffer is null")
                    .arg(operation, bufferDescription));
  return volumeByteCount(voxelType, depth, width, height,
                         ignoredBytes, operation);
}

bool restoreSelectedRecord(QFile& input,
                           const QVector<CheckpointRecord>& records,
                           int selectedIndex,
                           int voxelType,
                           int depth, int width, int height,
                           uchar *volumeData)
{
  const QString operation = "load checkpoint";
  if (selectedIndex < 0 || selectedIndex >= records.size())
    return setError(QString("%1: record index %2 is outside [0, %3)")
                    .arg(operation).arg(selectedIndex).arg(records.size()));
  if (!recordMatches(records[selectedIndex],
                     voxelType, depth, width, height))
    return setError(QString("%1: selected record grid or voxel type does not match")
                    .arg(operation));

  std::unique_ptr<QProgressDialog> progress(
    createProgress(QString("Restoring checkpoint %1")
                     .arg(records[selectedIndex].description)));
  const bool loaded = loadRecord(input, records[selectedIndex],
                                 voxelType, depth, width, height,
                                 volumeData, progress.get());
  input.close();
  if (!loaded)
    return false;
  updateProgress(progress.get(), 100);
  return true;
}

bool deleteSelectedRecord(const QString& filename,
                          QFile& input,
                          const QVector<CheckpointRecord>& records,
                          int selectedIndex)
{
  const QString operation = "delete checkpoint";
  if (selectedIndex < 0 || selectedIndex >= records.size())
    return setError(QString("%1: record index %2 is outside [0, %3)")
                    .arg(operation).arg(selectedIndex).arg(records.size()));

  QSaveFile output(filename);
  output.setDirectWriteFallback(false);
  if (!output.open(QFile::WriteOnly))
    return setError(QString("%1: cannot create temporary file for '%2': %3")
                    .arg(operation, filename, output.errorString()));

  std::unique_ptr<QProgressDialog> progress(
    createProgress(QString("Deleting checkpoint %1")
                     .arg(records[selectedIndex].description)));
  QVector<CheckpointRecord> keptRecords;
  QByteArray copyBuffer(kCopyBlockBytes, '\0');
  bool ok = writeDirectory(output, keptRecords);
  int copiedRecords = 0;
  const int recordsToCopy = records.size()-1;
  for(int index=0; index<records.size() && ok; ++index)
    {
      if (index == selectedIndex)
        continue;
      CheckpointRecord copiedRecord = records[index];
      copiedRecord.offset = output.pos();
      ok = copyRange(input, output,
                     records[index].offset, records[index].size,
                     copyBuffer, operation);
      if (ok)
        {
          keptRecords.append(copiedRecord);
          ++copiedRecords;
          if (recordsToCopy > 0)
            updateProgress(progress.get(),
                           10+80*copiedRecords/recordsToCopy);
        }
    }
  input.close();
  if (ok)
    ok = writeDirectory(output, keptRecords);
  updateProgress(progress.get(), 95);
  if (ok && !output.commit())
    ok = setError(QString("%1: cannot atomically commit '%2': %3")
                  .arg(operation, filename, output.errorString()));
  if (!ok)
    {
      output.cancelWriting();
      return false;
    }
  updateProgress(progress.get(), 100);
  return true;
}
}

QString
CheckpointHandler::lastError()
{
  return currentError();
}

bool
CheckpointHandler::saveCheckpoint(QString filename,
                                  int voxelType,
                                  int depth, int width, int height,
                                  uchar *volumeData,
                                  QString descriptor)
{
  clearError();
  const QString operation = "save checkpoint";
  qint64 volumeBytes = 0;
  descriptor = descriptor.trimmed();
  if (filename.isEmpty())
    setError(QString("%1: filename is empty").arg(operation));
  else if (!volumeData)
    setError(QString("%1: volume buffer is null").arg(operation));
  else if (descriptor.isEmpty())
    setError(QString("%1: description is empty").arg(operation));
  else
    volumeByteCount(voxelType, depth, width, height,
                    volumeBytes, operation);
  if (!currentError().isEmpty())
    {
      reportError("Checkpoint Error");
      return false;
    }

  QVector<CheckpointRecord> existingRecords;
  QFile input(filename);
  const QFileInfo inputInfo(filename);
  if (inputInfo.exists() && inputInfo.size() > 0)
    {
      if (!readArchive(input, existingRecords))
        {
          reportError("Checkpoint Error");
          return false;
        }
      if (existingRecords.size() >= kMaximumRecords)
        {
          setError(QString("%1: archive already contains the maximum %2 records")
                   .arg(operation).arg(kMaximumRecords));
          reportError("Checkpoint Error");
          return false;
        }
      for(int index=0; index<existingRecords.size(); ++index)
        if (!recordMatches(existingRecords[index],
                           voxelType, depth, width, height))
          {
            setError(QString("%1: existing record %2 belongs to a different volume")
                     .arg(operation).arg(index+1));
            reportError("Checkpoint Error");
            return false;
          }
    }

  QSaveFile output(filename);
  output.setDirectWriteFallback(false);
  if (!output.open(QFile::WriteOnly))
    {
      setError(QString("%1: cannot create temporary file for '%2': %3")
               .arg(operation, filename, output.errorString()));
      reportError("Checkpoint Error");
      return false;
    }

  std::unique_ptr<QProgressDialog> progress(
    createProgress(QString("Saving checkpoint %1").arg(descriptor)));
  QVector<CheckpointRecord> outputRecords;
  QByteArray copyBuffer(kCopyBlockBytes, '\0');
  bool ok = writeDirectory(output, outputRecords);
  for(int index=0; index<existingRecords.size() && ok; ++index)
    {
      CheckpointRecord copiedRecord = existingRecords[index];
      copiedRecord.offset = output.pos();
      ok = copyRange(input, output,
                     existingRecords[index].offset,
                     existingRecords[index].size,
                     copyBuffer, operation);
      if (ok)
        outputRecords.append(copiedRecord);
    }
  if (input.isOpen())
    input.close();

  CheckpointRecord newRecord;
  if (ok)
    ok = writeCompressedRecord(output,
                               voxelType, depth, width, height,
                               volumeData, descriptor,
                               newRecord, progress.get());
  if (ok)
    {
      outputRecords.append(newRecord);
      ok = writeDirectory(output, outputRecords);
    }
  updateProgress(progress.get(), 95);
  if (ok && !output.commit())
    ok = setError(QString("%1: cannot atomically commit '%2': %3")
                  .arg(operation, filename, output.errorString()));
  if (!ok)
    {
      output.cancelWriting();
      progress.reset();
      reportError("Checkpoint Error");
      return false;
    }

  updateProgress(progress.get(), 100);
  progress.reset();
  reportInformation("Checkpoint",
                    QString("Saved checkpoint information to\n%1")
                      .arg(filename));
  return true;
}

bool
CheckpointHandler::loadCheckpoint(QString filename,
                                  int voxelType,
                                  int depth, int width, int height,
                                  uchar *volumeData)
{
  clearError();
  const QString operation = "load checkpoint";
  if (!validateRequest(filename, voxelType, depth, width, height,
                       volumeData, operation, "destination"))
    {
      reportError("Checkpoint Error");
      return false;
    }

  QFile input(filename);
  QVector<CheckpointRecord> records;
  if (!readArchive(input, records))
    {
      reportError("Checkpoint Error");
      return false;
    }
  if (records.isEmpty())
    {
      setError(QString("%1: archive contains no records").arg(operation));
      reportInformation("Load checkpoint", currentError());
      return false;
    }

  int selectedIndex = -1;
  if (!selectRecord(records, "Checkpoint Records", "Select record",
                    selectedIndex))
    {
      if (!currentError().isEmpty())
        reportError("Checkpoint Error");
      return false;
    }
  if (!restoreSelectedRecord(input, records, selectedIndex,
                             voxelType, depth, width, height,
                             volumeData))
    {
      reportError("Checkpoint Error");
      return false;
    }
  return true;
}

bool
CheckpointHandler::loadCheckpointRecord(QString filename,
                                        int voxelType,
                                        int depth, int width, int height,
                                        uchar *volumeData,
                                        int recordIndex)
{
  clearError();
  const QString operation = "load checkpoint";
  if (!validateRequest(filename, voxelType, depth, width, height,
                       volumeData, operation, "destination"))
    return false;

  QFile input(filename);
  QVector<CheckpointRecord> records;
  if (!readArchive(input, records) ||
      !restoreSelectedRecord(input, records, recordIndex,
                             voxelType, depth, width, height,
                             volumeData))
    return false;
  return true;
}

bool
CheckpointHandler::deleteCheckpoint(QString filename,
                                    int voxelType,
                                    int depth, int width, int height,
                                    uchar *volumeData)
{
  clearError();
  const QString operation = "delete checkpoint";
  if (!validateRequest(filename, voxelType, depth, width, height,
                       volumeData, operation, "volume"))
    {
      reportError("Checkpoint Error");
      return false;
    }

  QFile input(filename);
  QVector<CheckpointRecord> records;
  if (!readArchive(input, records))
    {
      reportError("Checkpoint Error");
      return false;
    }
  if (records.isEmpty())
    {
      setError(QString("%1: archive contains no records").arg(operation));
      reportInformation("Delete checkpoint", currentError());
      return false;
    }

  int selectedIndex = -1;
  if (!selectRecord(records, "Checkpoint Records",
                    "Select record to delete", selectedIndex))
    {
      if (!currentError().isEmpty())
        reportError("Checkpoint Error");
      return false;
    }

  if (!deleteSelectedRecord(filename, input, records, selectedIndex))
    {
      reportError("Checkpoint Error");
      return false;
    }
  reportInformation("Delete checkpoint",
                    QString("Removed checkpoint %1")
                      .arg(records[selectedIndex].description));
  return true;
}

bool
CheckpointHandler::deleteCheckpointRecord(QString filename,
                                          int voxelType,
                                          int depth, int width, int height,
                                          uchar *volumeData,
                                          int recordIndex)
{
  clearError();
  const QString operation = "delete checkpoint";
  if (!validateRequest(filename, voxelType, depth, width, height,
                       volumeData, operation, "volume"))
    return false;

  QFile input(filename);
  QVector<CheckpointRecord> records;
  if (!readArchive(input, records))
    return false;
  if (!deleteSelectedRecord(filename, input, records, recordIndex))
    return false;
  return true;
}
