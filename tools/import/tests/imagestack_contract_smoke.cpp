#include "../plugins/imagestack/imagestackpixelconversion.h"

#include <array>
#include <iostream>

namespace
{
int fail(const char *message)
{
  std::cerr << message << std::endl;
  return 1;
}
}

int main()
{
  if (ImageStackPixelConversion::bytesPerPixel(_UChar) != 1 ||
      ImageStackPixelConversion::bytesPerPixel(_Rgb) != 3 ||
      ImageStackPixelConversion::bytesPerPixel(_Rgba) != 4)
    return fail("image-stack voxel byte contract is incorrect");

  const QRgb source[] = {
    qRgba(1, 2, 3, 4),
    qRgba(11, 12, 13, 14)
  };

  std::array<uchar, 8> rgb;
  rgb.fill(0xcd);
  if (!ImageStackPixelConversion::packColorPixels(
        source, 2, _Rgb, rgb.data()))
    return fail("RGB packing failed");
  const uchar expectedRgb[] = { 1, 2, 3, 11, 12, 13 };
  for (int i=0; i<6; ++i)
    if (rgb[static_cast<std::size_t>(i)] != expectedRgb[i])
      return fail("RGB packing produced the wrong channel layout");
  if (rgb[6] != 0xcd || rgb[7] != 0xcd)
    return fail("RGB packing wrote beyond three bytes per pixel");

  std::array<uchar, 10> rgba;
  rgba.fill(0xcd);
  if (!ImageStackPixelConversion::packColorPixels(
        source, 2, _Rgba, rgba.data()))
    return fail("RGBA packing failed");
  const uchar expectedRgba[] = { 1, 2, 3, 4, 11, 12, 13, 14 };
  for (int i=0; i<8; ++i)
    if (rgba[static_cast<std::size_t>(i)] != expectedRgba[i])
      return fail("RGBA packing produced the wrong channel layout");
  if (rgba[8] != 0xcd || rgba[9] != 0xcd)
    return fail("RGBA packing wrote beyond four bytes per pixel");

  if (ImageStackPixelConversion::packColorPixels(
        source, 2, _UChar, rgba.data()))
    return fail("scalar data was accepted by the color packer");

  std::cout << "Image stack pixel contract smoke passed" << std::endl;
  return 0;
}
