#include "../plugins/rawfileutils.h"

#include <QCoreApplication>
#include <QTemporaryFile>

#include <iostream>
#include <limits>

namespace
{
int fail(const char *message)
{
  std::cerr << message << std::endl;
  return 1;
}
}

int main(int argc, char **argv)
{
  QCoreApplication application(argc, argv);

  int voxelType = -1;
  if (!RawFileUtils::decodeVoxelTypeCode(8, voxelType) || voxelType != 5 ||
      RawFileUtils::decodeVoxelTypeCode(7, voxelType))
    return fail("voxel type decoding failed");

  if (RawFileUtils::exactHistogramIndex(1, -128) != 0 ||
      RawFileUtils::exactHistogramIndex(1, 127) != 255 ||
      RawFileUtils::exactHistogramIndex(3, -32768) != 0 ||
      RawFileUtils::exactHistogramIndex(3, 32767) != 65535)
    return fail("signed histogram bounds are incorrect");

  RawFileUtils::Layout layout;
  QString error;
  if (!RawFileUtils::makeLayout(10, 1024, 1024, 2, 13,
                                layout, error) ||
      layout.sliceBytes != 2097152 ||
      layout.requiredFileBytes != 20971533)
    return fail("valid RAW layout was calculated incorrectly");

  if (RawFileUtils::makeLayout(0, 10, 10, 0, 0, layout, error) ||
      RawFileUtils::makeLayout(std::numeric_limits<int>::max(),
                               std::numeric_limits<int>::max(),
                               std::numeric_limits<int>::max(),
                               5, 0, layout, error))
    return fail("invalid or overflowing RAW layout was accepted");

  if (RawFileUtils::scaledHistogramIndex(42, 42, 42, 65535) != 0 ||
      RawFileUtils::scaledHistogramIndex(
        std::numeric_limits<float>::quiet_NaN(), 0, 1, 65535) != 0)
    return fail("constant/non-finite histogram mapping is unsafe");

  QTemporaryFile file;
  if (!file.open() || file.write("abc", 3) != 3 || !file.flush())
    return fail("cannot create temporary RAW fixture");
  if (RawFileUtils::validateFileSize(file.fileName(), 4, error))
    return fail("truncated RAW file was accepted");

  char buffer[4] = {};
  if (RawFileUtils::readAt(file.fileName(), 0, buffer, 4, error))
    return fail("short RAW read was accepted");

  std::cout << "RAW file safety smoke passed" << std::endl;
  return 0;
}
