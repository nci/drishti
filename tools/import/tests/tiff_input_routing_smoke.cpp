#include "../tiffinputrouting.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QTemporaryDir>
#include <QVector>

#include <tiffio.h>

#include <iostream>

namespace
{
int fail(const QString& message)
{
  std::cerr << message.toStdString() << std::endl;
  return 1;
}

bool touch(const QString& fileName)
{
  QFile file(fileName);
  return file.open(QIODevice::WriteOnly) && file.write("x", 1) == 1;
}

bool saveGrayscaleTiff(const QString& fileName, quint16 offset)
{
  QImage image(4, 3, QImage::Format_Grayscale16);
  if (image.isNull())
    return false;

  for (int row=0; row<image.height(); ++row)
    {
      quint16 *pixels = reinterpret_cast<quint16*>(image.scanLine(row));
      if (!pixels)
        return false;
      for (int column=0; column<image.width(); ++column)
        pixels[column] = static_cast<quint16>(offset+row*image.width()+column);
    }
  return image.save(fileName, "TIFF");
}

bool saveColorImage(const QString& fileName, const char *format)
{
  QImage image(4, 3, QImage::Format_RGB32);
  image.fill(qRgb(20, 80, 160));
  return image.save(fileName, format);
}

TIFF *openTiffForWrite(const QString& fileName)
{
#if defined(Q_OS_WIN)
  return TIFFOpenW(reinterpret_cast<const wchar_t*>(fileName.utf16()), "w");
#else
  const QByteArray encodedName = QFile::encodeName(fileName);
  return TIFFOpen(encodedName.constData(), "w");
#endif
}

bool saveControlledTiff(const QString& fileName,
                        uint16_t orientation,
                        uint16_t photometric = PHOTOMETRIC_MINISBLACK,
                        bool tiled = false,
                        uint32_t width = 4,
                        uint32_t height = 3)
{
  TIFF *image = openTiffForWrite(fileName);
  if (!image)
    return false;

  bool ok =
    TIFFSetField(image, TIFFTAG_IMAGEWIDTH, width) == 1 &&
    TIFFSetField(image, TIFFTAG_IMAGELENGTH, height) == 1 &&
    TIFFSetField(image, TIFFTAG_BITSPERSAMPLE, 16) == 1 &&
    TIFFSetField(image, TIFFTAG_SAMPLESPERPIXEL, 1) == 1 &&
    TIFFSetField(image, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT) == 1 &&
    TIFFSetField(image, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG) == 1 &&
    TIFFSetField(image, TIFFTAG_PHOTOMETRIC, photometric) == 1 &&
    TIFFSetField(image, TIFFTAG_ORIENTATION, orientation) == 1 &&
    TIFFSetField(image, TIFFTAG_COMPRESSION, COMPRESSION_NONE) == 1;

  if (ok && tiled)
    {
      const uint32_t tileWidth = 16;
      const uint32_t tileHeight = 16;
      ok = TIFFSetField(image, TIFFTAG_TILEWIDTH, tileWidth) == 1 &&
           TIFFSetField(image, TIFFTAG_TILELENGTH, tileHeight) == 1;
      QVector<quint16> tile(static_cast<int>(tileWidth*tileHeight), 42);
      if (ok)
        ok = TIFFWriteEncodedTile(image, 0, tile.data(),
                                  static_cast<tmsize_t>(tile.size()*2)) >= 0;
    }
  else if (ok)
    {
      ok = TIFFSetField(image, TIFFTAG_ROWSPERSTRIP, height) == 1;
      QVector<quint16> row(static_cast<int>(width), 0);
      for (uint32_t y=0; ok && y<height; ++y)
        {
          for (uint32_t x=0; x<width; ++x)
            row[static_cast<int>(x)] = static_cast<quint16>(y*width+x);
          ok = TIFFWriteScanline(image, row.data(), y, 0) >= 0;
        }
    }

  TIFFClose(image);
  return ok;
}
}

int main(int argc, char **argv)
{
  QCoreApplication application(argc, argv);

  const QStringList fileDescriptions = QStringList()
    << "Standard Image Files"
    << "Grayscale TIFF Image Files"
    << "Other Files";
  const QStringList fileLibraries = QStringList()
    << "C:/portable/importplugins/imagestackplugin.dll"
    << "C:/portable/importplugins/tiffplugin.dll"
    << "C:/portable/importplugins/other.dll";

  QTemporaryDir temporary;
  if (!temporary.isValid())
    return fail("Cannot create the routing fixture directory");

  const QString allTiffPath = QDir(temporary.path()).filePath("all tiff");
  const QString mixedPath = QDir(temporary.path()).filePath("mixed");
  const QString emptyPath = QDir(temporary.path()).filePath("empty");
  const QString firstGray = QDir(allTiffPath).filePath("slice1.tif");
  const QString secondGray = QDir(allTiffPath).filePath("slice2.TIFF");
  const QString mixedGray = QDir(mixedPath).filePath("slice1.tif");
  const QString mixedPng = QDir(mixedPath).filePath("slice2.png");
  const QString colorTiff = QDir(temporary.path()).filePath("color.tif");
  const QString bottomLeftTiff =
    QDir(temporary.path()).filePath("bottom-left.tif");
  const QString whiteIsZeroTiff =
    QDir(temporary.path()).filePath("white-is-zero.tif");
  const QString tiledTiff = QDir(temporary.path()).filePath("tiled.tif");
  const QString differentSizeTiff =
    QDir(temporary.path()).filePath("different-size.tif");
  if (!QDir().mkpath(allTiffPath) || !QDir().mkpath(mixedPath) ||
      !QDir().mkpath(emptyPath) ||
      !saveGrayscaleTiff(firstGray, 0) ||
      !saveGrayscaleTiff(secondGray, 20) ||
      !touch(QDir(allTiffPath).filePath("notes.txt")) ||
      !saveGrayscaleTiff(mixedGray, 40) ||
      !saveColorImage(mixedPng, "PNG") ||
      !saveColorImage(colorTiff, "TIFF") ||
      !saveControlledTiff(bottomLeftTiff, ORIENTATION_BOTLEFT) ||
      !saveControlledTiff(whiteIsZeroTiff, ORIENTATION_TOPLEFT,
                          PHOTOMETRIC_MINISWHITE) ||
      !saveControlledTiff(tiledTiff, ORIENTATION_TOPLEFT,
                          PHOTOMETRIC_MINISBLACK, true, 16, 16) ||
      !saveControlledTiff(differentSizeTiff, ORIENTATION_TOPLEFT,
                          PHOTOMETRIC_MINISBLACK, false, 5, 3))
    return fail("Cannot create the routing fixtures");

  QStringList unsupportedOrientationTiffs;
  const uint16_t unsupportedOrientations[] = {
    ORIENTATION_TOPRIGHT, ORIENTATION_BOTRIGHT, ORIENTATION_LEFTTOP,
    ORIENTATION_RIGHTTOP, ORIENTATION_RIGHTBOT, ORIENTATION_LEFTBOT
  };
  for (uint16_t orientation : unsupportedOrientations)
    {
      const QString fileName = QDir(temporary.path()).filePath(
        QString("orientation-%1.tif").arg(orientation));
      if (!saveControlledTiff(fileName, orientation))
        return fail("Cannot create an unsupported-orientation TIFF fixture");
      unsupportedOrientationTiffs.append(fileName);
    }

  if (TiffInputRouting::routedPluginIndex(
        0, fileDescriptions, fileLibraries,
        QStringList() << firstGray << secondGray, false) != 1)
    return fail("An all-TIFF file selection was not routed");

  if (TiffInputRouting::routedPluginIndex(
        0, fileDescriptions, fileLibraries,
        QStringList() << firstGray << mixedPng, false) != 0)
    return fail("A mixed-format file selection was incorrectly routed");

  if (TiffInputRouting::routedPluginIndex(
        0, fileDescriptions, fileLibraries,
        QStringList() << colorTiff, false) != 0)
    return fail("A color TIFF was incorrectly routed to the grayscale importer");

  if (TiffInputRouting::routedPluginIndex(
        0, fileDescriptions, fileLibraries,
        QStringList() << firstGray << bottomLeftTiff, false) != 1)
    return fail("A supported top-left/bottom-left TIFF stack was not routed");

  if (TiffInputRouting::routedPluginIndex(
        0, fileDescriptions, fileLibraries,
        QStringList() << whiteIsZeroTiff, false) != 0)
    return fail("A white-is-zero TIFF was incorrectly routed");

  if (TiffInputRouting::routedPluginIndex(
        0, fileDescriptions, fileLibraries,
        QStringList() << tiledTiff, false) != 0)
    return fail("A tiled TIFF was incorrectly routed");

  if (TiffInputRouting::routedPluginIndex(
        0, fileDescriptions, fileLibraries,
        QStringList() << firstGray << differentSizeTiff, false) != 0)
    return fail("An inconsistent TIFF stack was incorrectly routed");

  for (const QString& fileName : unsupportedOrientationTiffs)
    if (TiffInputRouting::routedPluginIndex(
          0, fileDescriptions, fileLibraries,
          QStringList() << fileName, false) != 0)
      return fail("An unsupported TIFF orientation was incorrectly routed");

  if (TiffInputRouting::routedPluginIndex(
        0, fileDescriptions, fileLibraries,
        QStringList() << QDir(temporary.path()).filePath("missing.tif"),
        false) != 0)
    return fail("An unreadable TIFF was incorrectly routed");

  if (TiffInputRouting::routedPluginIndex(
        2, fileDescriptions, fileLibraries,
        QStringList() << firstGray, false) != 2)
    return fail("A non-ImageStack plugin selection was changed");

  QStringList noTiffDescriptions = fileDescriptions;
  QStringList noTiffLibraries = fileLibraries;
  noTiffDescriptions.removeAt(1);
  noTiffLibraries.removeAt(1);
  if (TiffInputRouting::routedPluginIndex(
        0, noTiffDescriptions, noTiffLibraries,
        QStringList() << firstGray, false) != 0)
    return fail("Routing changed when the native TIFF plugin was unavailable");

  const QStringList unixLibraries = QStringList()
    << "/portable/importplugins/libimagestackplugind.so"
    << "/portable/importplugins/libtiffplugind.so";
  if (TiffInputRouting::routedPluginIndex(
        0, fileDescriptions.mid(0, 2), unixLibraries,
        QStringList() << firstGray, false) != 1)
    return fail("Debug or Unix plugin names were not recognized");

  const QStringList directoryDescriptions = QStringList()
    << "Standard Image Directory"
    << "Grayscale TIFF Image Directory";
  const QStringList directoryLibraries = QStringList()
    << "C:/portable/importplugins/imagestackplugin.dll"
    << "C:/portable/importplugins/tiffplugin.dll";

  if (TiffInputRouting::routedPluginIndex(
        0, directoryDescriptions, directoryLibraries,
        QStringList() << allTiffPath, true) != 1)
    return fail("An all-TIFF directory was not routed");

  if (TiffInputRouting::routedPluginIndex(
        0, directoryDescriptions, directoryLibraries,
        QStringList() << mixedPath, true) != 0)
    return fail("A mixed image directory was incorrectly routed");

  if (TiffInputRouting::routedPluginIndex(
        0, directoryDescriptions, directoryLibraries,
        QStringList() << emptyPath, true) != 0)
    return fail("An empty directory was incorrectly routed");

  if (TiffInputRouting::routedPluginIndex(
        0, directoryDescriptions, directoryLibraries,
        QStringList() << allTiffPath << mixedPath, true) != 0)
    return fail("Multiple directory inputs were incorrectly routed");

  if (TiffInputRouting::routedPluginIndex(
        -1, directoryDescriptions, directoryLibraries,
        QStringList() << allTiffPath, true) != -1)
    return fail("An invalid plugin index was changed");

  if (argc == 2)
    {
      const QString realTiff = QString::fromLocal8Bit(argv[1]);
      if (TiffInputRouting::routedPluginIndex(
            0, fileDescriptions, fileLibraries,
            QStringList() << realTiff, false) != 1)
        return fail("The supplied real grayscale TIFF was not routed");
    }

  std::cout << "TIFF input routing smoke passed" << std::endl;
  return 0;
}
