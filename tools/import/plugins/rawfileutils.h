#ifndef RAWFILEUTILS_H
#define RAWFILEUTILS_H

#include <QFile>
#include <QString>
#include <QtGlobal>

#include <cmath>
#include <cstddef>
#include <limits>

namespace RawFileUtils
{
struct Layout
{
  qint64 sliceVoxels;
  qint64 sliceBytes;
  qint64 volumeBytes;
  qint64 requiredFileBytes;
};

inline bool checkedMultiply(qint64 lhs, qint64 rhs, qint64& result)
{
  if (lhs < 0 || rhs < 0 ||
      (lhs != 0 && rhs > std::numeric_limits<qint64>::max()/lhs))
    return false;

  result = lhs*rhs;
  return true;
}

inline bool checkedAdd(qint64 lhs, qint64 rhs, qint64& result)
{
  if (lhs < 0 || rhs < 0 ||
      rhs > std::numeric_limits<qint64>::max()-lhs)
    return false;

  result = lhs+rhs;
  return true;
}

inline bool bytesPerVoxel(int voxelType, int& bytes)
{
  if (voxelType == 0 || voxelType == 1)
    bytes = 1;
  else if (voxelType == 2 || voxelType == 3)
    bytes = 2;
  else if (voxelType == 4 || voxelType == 5)
    bytes = 4;
  else
    return false;

  return true;
}

inline bool decodeVoxelTypeCode(int code, int& voxelType)
{
  if (code >= '0' && code <= '4')
    code -= '0';
  else if (code == '8')
    code = 8;

  if (code >= 0 && code <= 4)
    voxelType = code;
  else if (code == 8)
    voxelType = 5;
  else
    return false;

  return true;
}

inline bool makeLayout(int depth, int width, int height,
                       int voxelType, qint64 headerBytes,
                       Layout& layout, QString& error)
{
  error.clear();
  if (depth <= 0 || width <= 0 || height <= 0)
    {
      error = QString("RAW dimensions must be positive; got %1 x %2 x %3.")
                .arg(depth).arg(width).arg(height);
      return false;
    }
  if (headerBytes < 0)
    {
      error = QString("RAW header size cannot be negative (%1).")
                .arg(headerBytes);
      return false;
    }

  int voxelBytes = 0;
  if (!bytesPerVoxel(voxelType, voxelBytes))
    {
      error = QString("Unsupported RAW voxel type %1.").arg(voxelType);
      return false;
    }

  if (!checkedMultiply(width, height, layout.sliceVoxels) ||
      !checkedMultiply(layout.sliceVoxels, voxelBytes, layout.sliceBytes) ||
      !checkedMultiply(layout.sliceBytes, depth, layout.volumeBytes) ||
      !checkedAdd(headerBytes, layout.volumeBytes,
                  layout.requiredFileBytes))
    {
      error = QString("RAW dimensions %1 x %2 x %3 overflow the supported "
                      "file layout.").arg(depth).arg(width).arg(height);
      return false;
    }

  // Several import consumers still use int-sized slice loops and buffers.
  if (layout.sliceVoxels > std::numeric_limits<int>::max() ||
      layout.sliceBytes > std::numeric_limits<int>::max())
    {
      error = QString("A RAW slice requires %1 voxels and %2 bytes, which "
                      "exceeds the supported per-slice limit.")
                .arg(layout.sliceVoxels).arg(layout.sliceBytes);
      return false;
    }

  return true;
}

inline bool readExact(QFile& file, void *destination, qint64 bytes,
                      QString& error)
{
  if (!destination || bytes < 0)
    {
      error = "Invalid RAW read buffer or byte count.";
      return false;
    }

  char *output = static_cast<char*>(destination);
  qint64 total = 0;
  while (total < bytes)
    {
      const qint64 count = file.read(output+static_cast<std::size_t>(total),
                                     bytes-total);
      if (count <= 0)
        {
          error = QString("Short RAW read from %1: expected %2 bytes, got %3.")
                    .arg(file.fileName()).arg(bytes).arg(total);
          return false;
        }
      total += count;
    }
  return true;
}

inline bool readAt(const QString& fileName, qint64 offset,
                   void *destination, qint64 bytes, QString& error)
{
  error.clear();
  if (offset < 0)
    {
      error = QString("Invalid negative RAW file offset %1.").arg(offset);
      return false;
    }

  QFile file(fileName);
  if (!file.open(QFile::ReadOnly))
    {
      error = QString("Cannot open RAW file %1: %2")
                .arg(fileName, file.errorString());
      return false;
    }
  if (!file.seek(offset))
    {
      error = QString("Cannot seek to byte %1 in RAW file %2: %3")
                .arg(offset).arg(fileName, file.errorString());
      return false;
    }
  return readExact(file, destination, bytes, error);
}

inline bool validateFileSize(const QString& fileName, qint64 requiredBytes,
                             QString& error)
{
  error.clear();
  QFile file(fileName);
  if (!file.open(QFile::ReadOnly))
    {
      error = QString("Cannot open RAW file %1: %2")
                .arg(fileName, file.errorString());
      return false;
    }
  if (file.size() < requiredBytes)
    {
      error = QString("RAW file %1 is truncated: expected at least %2 bytes, "
                      "found %3.")
                .arg(fileName).arg(requiredBytes).arg(file.size());
      return false;
    }
  return true;
}

inline bool readEmbeddedHeader(const QString& fileName, int& voxelType,
                               int& depth, int& width, int& height,
                               QString& error)
{
  uchar code = 0;
  qint32 dimensions[3] = { 0, 0, 0 };
  QFile file(fileName);
  if (!file.open(QFile::ReadOnly))
    {
      error = QString("Cannot open RAW file %1: %2")
                .arg(fileName, file.errorString());
      return false;
    }
  if (!readExact(file, &code, 1, error) ||
      !readExact(file, dimensions, 3*static_cast<qint64>(sizeof(qint32)),
                 error))
    return false;
  if (!decodeVoxelTypeCode(code, voxelType))
    {
      error = QString("RAW file %1 has unsupported voxel type code %2.")
                .arg(fileName).arg(code);
      return false;
    }

  depth = dimensions[0];
  width = dimensions[1];
  height = dimensions[2];
  return true;
}

inline int exactHistogramIndex(int voxelType, qint64 value)
{
  if (voxelType == 0)
    return value >= 0 && value <= 255 ? static_cast<int>(value) : -1;
  if (voxelType == 1)
    return value >= -128 && value <= 127 ? static_cast<int>(value+128) : -1;
  if (voxelType == 2)
    return value >= 0 && value <= 65535 ? static_cast<int>(value) : -1;
  if (voxelType == 3)
    return value >= -32768 && value <= 32767 ?
             static_cast<int>(value+32768) : -1;
  return -1;
}

inline float finiteValue(float value)
{
  return std::isfinite(static_cast<double>(value)) ? value : 0.0f;
}

inline int scaledHistogramIndex(float value, float minimum, float maximum,
                                int histogramSize)
{
  if (histogramSize <= 0)
    return -1;

  value = finiteValue(value);
  if (!std::isfinite(static_cast<double>(minimum)) ||
      !std::isfinite(static_cast<double>(maximum)) || maximum <= minimum)
    return 0;

  double fraction = (static_cast<double>(value)-minimum)/
                    (static_cast<double>(maximum)-minimum);
  if (!std::isfinite(fraction) || fraction <= 0.0)
    return 0;
  if (fraction >= 1.0)
    return histogramSize;
  return static_cast<int>(fraction*histogramSize);
}
}

#endif
