#include "maskimportutils.h"

#include "blosc.h"
#include "getmemorysize.h"

#include <QFile>
#include <QIODevice>

#include <cstring>
#include <limits>
#include <new>
#include <utility>

namespace
{
const qint64 kMaximumCompressionBlockBytes = 256LL*1024LL*1024LL;
const std::uint64_t kMaskImportSafetyBytes = 16ULL*1024ULL*1024ULL;

bool checkedMultiply(std::uint64_t first,
                     std::uint64_t second,
                     std::uint64_t& result)
{
  if (first == 0 || second == 0)
    {
      result = 0;
      return true;
    }
  if (first > std::numeric_limits<std::uint64_t>::max()/second)
    return false;
  result = first*second;
  return true;
}

bool checkedAdd(std::uint64_t first,
                std::uint64_t second,
                std::uint64_t& result)
{
  if (first > std::numeric_limits<std::uint64_t>::max()-second)
    return false;
  result = first+second;
  return true;
}

bool readExact(QIODevice& input,
               void *destination,
               qint64 bytes,
               QString& error,
               const QString& field)
{
  if (!destination || bytes < 0)
    {
      error = QString("Invalid destination while reading %1").arg(field);
      return false;
    }

  qint64 completed = 0;
  while (completed < bytes)
    {
      const qint64 count = input.read(
        static_cast<char*>(destination)+completed, bytes-completed);
      if (count <= 0)
        {
          error = QString("Short read while reading %1 (%2 of %3 bytes): %4")
                    .arg(field).arg(completed).arg(bytes)
                    .arg(input.errorString());
          return false;
        }
      completed += count;
    }
  return true;
}

int bytesPerStoredLabel(quint8 voxelType)
{
  if (voxelType == 0 || voxelType == 1)
    return 1;
  if (voxelType == 2 || voxelType == 3)
    return 2;
  return 0;
}

bool validateDimensions(qint32 depth,
                        qint32 width,
                        qint32 height,
                        std::uint64_t& voxelCount,
                        QString& error)
{
  std::uint64_t planeVoxels = 0;
  if (depth <= 0 || width <= 0 || height <= 0 ||
      !checkedMultiply(static_cast<std::uint64_t>(width),
                       static_cast<std::uint64_t>(height), planeVoxels) ||
      !checkedMultiply(static_cast<std::uint64_t>(depth),
                       planeVoxels, voxelCount) ||
      voxelCount == 0 ||
      voxelCount > static_cast<std::uint64_t>(
                     std::numeric_limits<std::size_t>::max()/sizeof(quint16)))
    {
      error = QString("Invalid or unsupported mask dimensions %1 x %2 x %3")
                .arg(depth).arg(width).arg(height);
      return false;
    }
  return true;
}

QString memoryAmount(std::uint64_t bytes)
{
  return QString("%1 MiB")
    .arg(static_cast<double>(bytes)/(1024.0*1024.0), 0, 'f', 1);
}

bool admitMaskImport(qint32 depth,
                     qint32 width,
                     qint32 height,
                     std::uint64_t fixedBytes,
                     QString& error)
{
  const PaintAlgorithmMemoryAdmission admission =
    evaluatePaintAlgorithmMemoryAdmission(
      static_cast<std::uint64_t>(depth),
      static_cast<std::uint64_t>(width),
      static_cast<std::uint64_t>(height),
      sizeof(quint16), fixedBytes);
  if (admission.approved)
    return true;

  error = QString("Mask import was stopped before allocation. Required: %1; "
                  "physical budget: %2; Commit budget: %3.")
            .arg(memoryAmount(admission.requiredBytes))
            .arg(admission.physicalMemoryChecked ?
                   memoryAmount(admission.availablePhysicalBudgetBytes) :
                   QString("unavailable"))
            .arg(admission.commitMemoryChecked ?
                   memoryAmount(admission.availableCommitBudgetBytes) :
                   QString("unavailable"));
  return false;
}

bool allocateLabels(ImportedPaintMask& mask,
                    std::uint64_t fixedBytes,
                    QString& error)
{
  if (!admitMaskImport(mask.depth, mask.width, mask.height,
                       fixedBytes, error))
    return false;
  mask.labels.reset(new (std::nothrow) quint16[
    static_cast<std::size_t>(mask.voxelCount)]);
  if (!mask.labels)
    {
      error = QString("Cannot allocate %1 for imported mask labels")
                .arg(memoryAmount(mask.voxelCount*sizeof(quint16)));
      return false;
    }
  std::memset(mask.labels.get(), 0,
              static_cast<std::size_t>(mask.voxelCount*sizeof(quint16)));
  return true;
}

bool readCommonHeader(QIODevice& input,
                      quint8& voxelType,
                      ImportedPaintMask& mask,
                      QString& error)
{
  return readExact(input, &voxelType, 1, error, "voxel type") &&
         readExact(input, &mask.depth, 4, error, "depth") &&
         readExact(input, &mask.width, 4, error, "width") &&
         readExact(input, &mask.height, 4, error, "height") &&
         validateDimensions(mask.depth, mask.width, mask.height,
                            mask.voxelCount, error);
}

void widenByteLabels(ImportedPaintMask& mask)
{
  uchar *bytes = reinterpret_cast<uchar*>(mask.labels.get());
  for (std::uint64_t index=mask.voxelCount; index>0; --index)
    mask.labels[index-1] = bytes[index-1];
}

bool loadRawMask(QFile& input,
                 ImportedPaintMask& mask,
                 QString& error)
{
  quint8 voxelType = 0;
  if (!readCommonHeader(input, voxelType, mask, error))
    return false;
  const int sourceBytesPerLabel = bytesPerStoredLabel(voxelType);
  if (sourceBytesPerLabel == 0)
    {
      error = QString("Unsupported mask voxel type %1").arg(voxelType);
      return false;
    }

  std::uint64_t payloadBytes = 0;
  if (!checkedMultiply(mask.voxelCount,
                       static_cast<std::uint64_t>(sourceBytesPerLabel),
                       payloadBytes) ||
      payloadBytes > static_cast<std::uint64_t>(
                       std::numeric_limits<qint64>::max()) ||
      input.size()-input.pos() != static_cast<qint64>(payloadBytes))
    {
      error = "Raw mask payload size does not match its header";
      return false;
    }

  if (!allocateLabels(mask, kMaskImportSafetyBytes, error) ||
      !readExact(input, mask.labels.get(), static_cast<qint64>(payloadBytes),
                 error, "raw mask payload"))
    return false;
  if (sourceBytesPerLabel == 1)
    widenByteLabels(mask);
  return true;
}

bool loadCompressedMask(QFile& input,
                        ImportedPaintMask& mask,
                        QString& error)
{
  char magic[6] = {};
  if (!readExact(input, magic, 6, error, "compressed mask signature"))
    return false;
  if (std::memcmp(magic, "dpm100", 6) != 0)
    {
      error = "Compressed mask has an invalid signature";
      return false;
    }

  quint8 voxelType = 0;
  if (!readCommonHeader(input, voxelType, mask, error))
    return false;
  const int sourceBytesPerLabel = bytesPerStoredLabel(voxelType);
  if (sourceBytesPerLabel == 0)
    {
      error = QString("Unsupported compressed mask voxel type %1")
                .arg(voxelType);
      return false;
    }

  qint32 blockCount = 0;
  qint32 blockBytes = 0;
  if (!readExact(input, &blockCount, 4, error, "compression block count") ||
      !readExact(input, &blockBytes, 4, error, "compression block size"))
    return false;

  std::uint64_t sourceBytes = 0;
  if (!checkedMultiply(mask.voxelCount,
                       static_cast<std::uint64_t>(sourceBytesPerLabel),
                       sourceBytes) ||
      sourceBytes > static_cast<std::uint64_t>(
                      std::numeric_limits<qint64>::max()) ||
      blockCount <= 0 || blockBytes <= 0 ||
      blockBytes > kMaximumCompressionBlockBytes)
    {
      error = "Compressed mask block layout is invalid";
      return false;
    }
  const std::uint64_t expectedBlocks =
    1+(sourceBytes-1)/static_cast<std::uint64_t>(blockBytes);
  if (expectedBlocks != static_cast<std::uint64_t>(blockCount))
    {
      error = QString("Compressed mask has %1 blocks, expected %2")
                .arg(blockCount).arg(expectedBlocks);
      return false;
    }

  std::uint64_t compressedCapacity = 0;
  std::uint64_t fixedBytes = 0;
  if (!checkedAdd(static_cast<std::uint64_t>(blockBytes),
                  BLOSC_MAX_OVERHEAD, compressedCapacity) ||
      !checkedAdd(kMaskImportSafetyBytes, compressedCapacity, fixedBytes) ||
      compressedCapacity > static_cast<std::uint64_t>(
                             std::numeric_limits<std::size_t>::max()))
    {
      error = "Compressed mask buffer size overflows";
      return false;
    }
  if (!allocateLabels(mask, fixedBytes, error))
    return false;

  std::unique_ptr<uchar[]> compressed(new (std::nothrow) uchar[
    static_cast<std::size_t>(compressedCapacity)]);
  if (!compressed)
    {
      error = "Cannot allocate compressed mask input buffer";
      return false;
    }

  uchar *destination = reinterpret_cast<uchar*>(mask.labels.get());
  std::uint64_t outputOffset = 0;
  for (qint32 block=0; block<blockCount; ++block)
    {
      qint32 storedBytes = 0;
      if (!readExact(input, &storedBytes, 4, error,
                     QString("compressed size for block %1").arg(block)))
        return false;
      if (storedBytes <= 0 ||
          static_cast<std::uint64_t>(storedBytes) > compressedCapacity ||
          storedBytes > input.size()-input.pos())
        {
          error = QString("Compressed mask block %1 has an invalid size")
                    .arg(block);
          return false;
        }
      if (!readExact(input, compressed.get(), storedBytes, error,
                     QString("compressed block %1").arg(block)))
        return false;

      size_t validatedBytes = 0;
      if (blosc_cbuffer_validate(compressed.get(),
                                 static_cast<size_t>(storedBytes),
                                 &validatedBytes) != 0)
        {
          error = QString("Compressed mask block %1 is not valid Blosc data")
                    .arg(block);
          return false;
        }
      const std::uint64_t expectedBytes = qMin<std::uint64_t>(
        static_cast<std::uint64_t>(blockBytes), sourceBytes-outputOffset);
      if (validatedBytes != static_cast<size_t>(expectedBytes))
        {
          error = QString("Compressed mask block %1 expands to %2 bytes, expected %3")
                    .arg(block).arg(validatedBytes).arg(expectedBytes);
          return false;
        }
      const int decoded = blosc_decompress_ctx(
        compressed.get(), destination+outputOffset,
        static_cast<size_t>(expectedBytes), 4);
      if (decoded < 0 || static_cast<std::uint64_t>(decoded) != expectedBytes)
        {
          error = QString("Compressed mask block %1 failed to decompress")
                    .arg(block);
          return false;
        }
      outputOffset += expectedBytes;
    }

  if (outputOffset != sourceBytes || input.pos() != input.size())
    {
      error = "Compressed mask payload length does not match its header";
      return false;
    }
  if (sourceBytesPerLabel == 1)
    widenByteLabels(mask);
  return true;
}

std::uint64_t nearestSourceCoordinate(std::uint64_t targetCoordinate,
                                      std::uint64_t sourceSize,
                                      std::uint64_t targetSize)
{
  return ((2*targetCoordinate+1)*sourceSize)/(2*targetSize);
}
}

bool
loadImportedPaintMask(const QString& fileName,
                      ImportedPaintMask& mask,
                      QString& error)
{
  error.clear();
  ImportedPaintMask loaded;
  QFile input(fileName);
  if (!input.open(QFile::ReadOnly))
    {
      error = QString("Cannot open mask '%1': %2")
                .arg(fileName, input.errorString());
      return false;
    }

  const bool compressed = fileName.endsWith(".mask.sc", Qt::CaseInsensitive);
  const bool raw = fileName.endsWith(".mask", Qt::CaseInsensitive);
  if (!compressed && !raw)
    {
      error = "Mask filename must end in .mask or .mask.sc";
      return false;
    }

  const bool ok = compressed ? loadCompressedMask(input, loaded, error) :
                               loadRawMask(input, loaded, error);
  input.close();
  if (!ok)
    return false;
  mask = std::move(loaded);
  return true;
}

bool
overlayImportedPaintMask(const ImportedPaintMask& source,
                         bool sourceSliceZeroAtTop,
                         qint32 targetDepth,
                         qint32 targetWidth,
                         qint32 targetHeight,
                         int targetBytesPerLabel,
                         uchar *target,
                         std::uint64_t targetCapacityBytes,
                         QString& error,
                         ImportedPaintMaskProgress progress,
                         void *progressContext)
{
  error.clear();
  std::uint64_t sourceVoxels = 0;
  std::uint64_t targetVoxels = 0;
  std::uint64_t requiredBytes = 0;
  if (!source.labels || source.voxelCount == 0 ||
      !validateDimensions(source.depth, source.width, source.height,
                          sourceVoxels, error) ||
      sourceVoxels != source.voxelCount ||
      !validateDimensions(targetDepth, targetWidth, targetHeight,
                          targetVoxels, error) ||
      (targetBytesPerLabel != 1 && targetBytesPerLabel != 2) ||
      !checkedMultiply(targetVoxels,
                       static_cast<std::uint64_t>(targetBytesPerLabel),
                       requiredBytes) ||
      !target || requiredBytes > targetCapacityBytes)
    {
      if (error.isEmpty())
        error = "Mask overlay dimensions or destination buffer are invalid";
      return false;
    }

  if (targetBytesPerLabel == 1)
    for (std::uint64_t index=0; index<source.voxelCount; ++index)
      if (source.labels[index] > std::numeric_limits<quint8>::max())
        {
          error = QString("Mask label %1 cannot be stored in an 8-bit mask")
                    .arg(source.labels[index]);
          return false;
        }

  for (std::uint64_t d=0; d<static_cast<std::uint64_t>(targetDepth); ++d)
    {
      if (progress)
        progress(d, static_cast<std::uint64_t>(targetDepth), progressContext);
      std::uint64_t sourceDepth = nearestSourceCoordinate(
        d, static_cast<std::uint64_t>(source.depth),
        static_cast<std::uint64_t>(targetDepth));
      if (!sourceSliceZeroAtTop)
        sourceDepth = static_cast<std::uint64_t>(source.depth)-1-sourceDepth;
      for (std::uint64_t w=0; w<static_cast<std::uint64_t>(targetWidth); ++w)
        {
          const std::uint64_t sourceWidth = nearestSourceCoordinate(
            w, static_cast<std::uint64_t>(source.width),
            static_cast<std::uint64_t>(targetWidth));
          for (std::uint64_t h=0; h<static_cast<std::uint64_t>(targetHeight); ++h)
            {
              const std::uint64_t sourceHeight = nearestSourceCoordinate(
                h, static_cast<std::uint64_t>(source.height),
                static_cast<std::uint64_t>(targetHeight));
              const std::uint64_t sourceIndex =
                (sourceDepth*static_cast<std::uint64_t>(source.width)+sourceWidth)*
                  static_cast<std::uint64_t>(source.height)+sourceHeight;
              const quint16 label = source.labels[sourceIndex];
              if (label == 0)
                continue;
              const std::uint64_t targetIndex =
                (d*static_cast<std::uint64_t>(targetWidth)+w)*
                  static_cast<std::uint64_t>(targetHeight)+h;
              if (targetBytesPerLabel == 1)
                target[targetIndex] = static_cast<quint8>(label);
              else
                reinterpret_cast<quint16*>(target)[targetIndex] = label;
            }
        }
    }
  if (progress)
    progress(static_cast<std::uint64_t>(targetDepth),
             static_cast<std::uint64_t>(targetDepth), progressContext);
  return true;
}
