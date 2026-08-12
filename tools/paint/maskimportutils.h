#ifndef MASKIMPORTUTILS_H
#define MASKIMPORTUTILS_H

#include <QtGlobal>
#include <QString>

#include <cstdint>
#include <memory>

struct ImportedPaintMask
{
  qint32 depth = 0;
  qint32 width = 0;
  qint32 height = 0;
  std::uint64_t voxelCount = 0;
  std::unique_ptr<quint16[]> labels;
};

using ImportedPaintMaskProgress =
  void (*)(std::uint64_t completed, std::uint64_t total, void *context);

bool loadImportedPaintMask(const QString& fileName,
                           ImportedPaintMask& mask,
                           QString& error);

bool overlayImportedPaintMask(const ImportedPaintMask& source,
                              bool sourceSliceZeroAtTop,
                              qint32 targetDepth,
                              qint32 targetWidth,
                              qint32 targetHeight,
                              int targetBytesPerLabel,
                              uchar *target,
                              std::uint64_t targetCapacityBytes,
                              QString& error,
                              ImportedPaintMaskProgress progress = nullptr,
                              void *progressContext = nullptr);

#endif
