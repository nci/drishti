#include "sliceorderutils.h"

#include <cstring>
#include <limits>
#include <memory>
#include <new>

namespace
{
bool checkedMultiply(size_t left, size_t right, size_t &result)
{
  if (left != 0 && right > std::numeric_limits<size_t>::max()/left)
    return false;

  result = left*right;
  return true;
}

bool sliceLayout(int depth,
                 int width,
                 int height,
                 int bytesPerVoxel,
                 size_t &sliceBytes)
{
  if (depth <= 0 || width <= 0 || height <= 0 || bytesPerVoxel <= 0)
    return false;

  size_t pixels = 0;
  size_t volumeBytes = 0;
  return checkedMultiply(static_cast<size_t>(width),
                         static_cast<size_t>(height), pixels) &&
         checkedMultiply(pixels, static_cast<size_t>(bytesPerVoxel),
                         sliceBytes) &&
         checkedMultiply(sliceBytes, static_cast<size_t>(depth), volumeBytes);
}

void reverseSlices(uchar *data,
                   int depth,
                   size_t sliceBytes,
                   uchar *temporary)
{
  for (int slice=0; slice<depth/2; ++slice)
    {
      uchar *front = data+static_cast<size_t>(slice)*sliceBytes;
      uchar *back = data+static_cast<size_t>(depth-1-slice)*sliceBytes;
      std::memcpy(temporary, front, sliceBytes);
      std::memcpy(front, back, sliceBytes);
      std::memcpy(back, temporary, sliceBytes);
    }
}
}

bool
reversePaintSliceOrder(uchar *volume,
                       int volumeBytesPerVoxel,
                       uchar *mask,
                       int depth,
                       int width,
                       int height,
                       QString &error)
{
  error.clear();
  if (!volume || !mask)
    {
      error = QStringLiteral("Volume or mask storage is unavailable.");
      return false;
    }

  size_t volumeSliceBytes = 0;
  size_t maskSliceBytes = 0;
  if (!sliceLayout(depth, width, height, volumeBytesPerVoxel,
                   volumeSliceBytes) ||
      !sliceLayout(depth, width, height, 2, maskSliceBytes))
    {
      error = QStringLiteral("Volume dimensions overflow the slice-ordering buffer.");
      return false;
    }

  const size_t temporaryBytes =
    volumeSliceBytes > maskSliceBytes ? volumeSliceBytes : maskSliceBytes;
  std::unique_ptr<uchar[]> temporary(
    new (std::nothrow) uchar[temporaryBytes]);
  if (!temporary)
    {
      error = QStringLiteral("Not enough memory to reverse the slice order.");
      return false;
    }

  reverseSlices(volume, depth, volumeSliceBytes, temporary.get());
  reverseSlices(mask, depth, maskSliceBytes, temporary.get());
  return true;
}
