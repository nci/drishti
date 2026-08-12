#ifndef IMAGESTACKPIXELCONVERSION_H
#define IMAGESTACKPIXELCONVERSION_H

#include "common.h"

#include <QRgb>
#include <QtGlobal>

namespace ImageStackPixelConversion
{
inline int bytesPerPixel(int voxelType)
{
  if (voxelType == _UChar)
    return 1;
  if (voxelType == _UShort)
    return 2;
  if (voxelType == _Rgb)
    return 3;
  if (voxelType == _Rgba)
    return 4;
  return 0;
}

inline bool packColorPixels(const QRgb *source,
                            qint64 pixelCount,
                            int voxelType,
                            uchar *destination)
{
  const int outputBytesPerPixel = bytesPerPixel(voxelType);
  if (!source || !destination || pixelCount < 0 ||
      (voxelType != _Rgb && voxelType != _Rgba))
    return false;

  for (qint64 i=0; i<pixelCount; ++i)
    {
      const QRgb pixel = source[i];
      destination[outputBytesPerPixel*i+0] = qRed(pixel);
      destination[outputBytesPerPixel*i+1] = qGreen(pixel);
      destination[outputBytesPerPixel*i+2] = qBlue(pixel);
      if (voxelType == _Rgba)
        destination[outputBytesPerPixel*i+3] = qAlpha(pixel);
    }
  return true;
}
}

#endif
