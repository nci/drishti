#include "tiffpagevalidation.h"

#include <QFile>

#include <limits>

namespace TiffPageValidation
{
namespace
{
bool checkedMultiply(quint64 lhs, quint64 rhs, quint64 *result)
{
  if (!result || (lhs != 0 && rhs > std::numeric_limits<quint64>::max()/lhs))
    return false;
  *result = lhs*rhs;
  return true;
}
}

void TiffCloser::operator()(TIFF *image) const
{
  if (image)
    TIFFClose(image);
}

TiffHandle openTiff(const QString& fileName)
{
#if defined(Q_OS_WIN)
  return TiffHandle(TIFFOpenW(
    reinterpret_cast<const wchar_t*>(fileName.utf16()), "r"));
#else
  const QByteArray encodedName = QFile::encodeName(fileName);
  return TiffHandle(TIFFOpen(encodedName.constData(), "r"));
#endif
}

QString pageName(const QString& fileName, quint64 directory)
{
  return QString("%1 (page %2)").arg(fileName).arg(directory);
}

bool readPageMetadata(TIFF *image,
                      const QString& label,
                      PageMetadata *metadata,
                      QString *error)
{
  if (!metadata || !error)
    return false;
  if (!image)
    {
      *error = QString("Cannot open TIFF %1").arg(label);
      return false;
    }

  PageMetadata md;
  md.width = 0;
  md.height = 0;
  md.bitsPerSample = 1;
  md.samplesPerPixel = 1;
  md.sampleFormat = SAMPLEFORMAT_UINT;
  md.planarConfig = PLANARCONFIG_CONTIG;
  md.photometric = PHOTOMETRIC_MINISBLACK;
  md.orientation = ORIENTATION_TOPLEFT;
  md.compression = COMPRESSION_NONE;

  if (!TIFFGetField(image, TIFFTAG_IMAGEWIDTH, &md.width) ||
      !TIFFGetField(image, TIFFTAG_IMAGELENGTH, &md.height))
    {
      *error = QString("Missing TIFF dimensions in %1").arg(label);
      return false;
    }

  TIFFGetFieldDefaulted(image, TIFFTAG_BITSPERSAMPLE, &md.bitsPerSample);
  TIFFGetFieldDefaulted(image, TIFFTAG_SAMPLESPERPIXEL, &md.samplesPerPixel);
  TIFFGetFieldDefaulted(image, TIFFTAG_SAMPLEFORMAT, &md.sampleFormat);
  TIFFGetFieldDefaulted(image, TIFFTAG_PLANARCONFIG, &md.planarConfig);
  TIFFGetFieldDefaulted(image, TIFFTAG_PHOTOMETRIC, &md.photometric);
  TIFFGetFieldDefaulted(image, TIFFTAG_ORIENTATION, &md.orientation);
  TIFFGetFieldDefaulted(image, TIFFTAG_COMPRESSION, &md.compression);

  if (md.width == 0 || md.height == 0)
    {
      *error = QString("Invalid zero-sized TIFF page in %1").arg(label);
      return false;
    }
  if (TIFFIsTiled(image))
    {
      *error = QString(
        "Tiled TIFF is not supported by the grayscale importer: %1")
        .arg(label);
      return false;
    }
  if (md.samplesPerPixel != 1)
    {
      *error = QString(
        "Expected one grayscale sample per pixel, found %1 in %2")
        .arg(md.samplesPerPixel).arg(label);
      return false;
    }
  if (md.planarConfig != PLANARCONFIG_CONTIG)
    {
      *error = QString("Unsupported TIFF planar configuration in %1")
        .arg(label);
      return false;
    }
  if (md.photometric != PHOTOMETRIC_MINISBLACK)
    {
      *error = QString("Unsupported TIFF photometric value %1 in %2; "
                       "only black-is-zero grayscale data is supported")
        .arg(md.photometric).arg(label);
      return false;
    }

  // Volume stacks preserve TIFF storage scanline order as their voxel axis.
  if (md.orientation != ORIENTATION_TOPLEFT &&
      md.orientation != ORIENTATION_BOTLEFT)
    {
      *error = QString("Unsupported TIFF orientation %1 in %2; "
                       "only top-left and bottom-left orientations are supported")
        .arg(md.orientation).arg(label);
      return false;
    }
  if (md.bitsPerSample != 8 && md.bitsPerSample != 16 &&
      md.bitsPerSample != 32)
    {
      *error = QString(
        "Unsupported TIFF bit depth %1 in %2; expected 8, 16, or 32")
        .arg(md.bitsPerSample).arg(label);
      return false;
    }
  if ((md.bitsPerSample == 8 || md.bitsPerSample == 16) &&
      md.sampleFormat != SAMPLEFORMAT_UINT &&
      md.sampleFormat != SAMPLEFORMAT_INT)
    {
      *error = QString(
        "Unsupported TIFF sample format %1 for %2-bit data in %3")
        .arg(md.sampleFormat).arg(md.bitsPerSample).arg(label);
      return false;
    }
  if (md.bitsPerSample == 32 &&
      md.sampleFormat != SAMPLEFORMAT_INT &&
      md.sampleFormat != SAMPLEFORMAT_IEEEFP)
    {
      *error = QString(
        "Unsupported TIFF sample format %1 for 32-bit data in %2")
        .arg(md.sampleFormat).arg(label);
      return false;
    }

  md.bytesPerVoxel = md.bitsPerSample/8;
  if (!checkedMultiply(md.width, md.bytesPerVoxel, &md.rowBytes) ||
      !checkedMultiply(md.rowBytes, md.height, &md.sliceBytes))
    {
      *error = QString("TIFF dimensions overflow a 64-bit byte count in %1")
        .arg(label);
      return false;
    }
  md.scanlineBytes = TIFFScanlineSize64(image);
  if (md.scanlineBytes < md.rowBytes)
    {
      *error = QString(
        "TIFF scanline is smaller than the expected pixel row in %1")
        .arg(label);
      return false;
    }
  if (md.sliceBytes > static_cast<quint64>(std::numeric_limits<int>::max()) ||
      md.scanlineBytes > static_cast<quint64>(std::numeric_limits<int>::max()))
    {
      *error = QString(
        "A TIFF page is too large for this Qt 5 importer (%1 bytes): %2")
        .arg(md.sliceBytes).arg(label);
      return false;
    }

  *metadata = md;
  return true;
}

bool samePageLayout(const PageMetadata& expected,
                    const PageMetadata& actual,
                    QString *difference)
{
  if (!difference)
    return false;
  if (expected.width != actual.width || expected.height != actual.height)
    *difference = "dimensions";
  else if (expected.bitsPerSample != actual.bitsPerSample)
    *difference = "bits per sample";
  else if (expected.samplesPerPixel != actual.samplesPerPixel)
    *difference = "samples per pixel";
  else if (expected.sampleFormat != actual.sampleFormat)
    *difference = "sample format";
  else if (expected.planarConfig != actual.planarConfig)
    *difference = "planar configuration";
  else if (expected.photometric != actual.photometric)
    *difference = "photometric interpretation";
  else if (expected.rowBytes != actual.rowBytes)
    *difference = "decoded row size";
  else
    return true;
  return false;
}

bool inspectFile(const QString& fileName,
                 QVector<PageMetadata> *pages,
                 QString *error)
{
  if (!pages || !error)
    return false;
  pages->clear();
  TiffHandle image = openTiff(fileName);
  if (!image)
    {
      *error = QString("Cannot open TIFF file: %1").arg(fileName);
      return false;
    }

  for (;;)
    {
      const tdir_t directory = TIFFCurrentDirectory(image.get());
      PageMetadata metadata;
      if (!readPageMetadata(image.get(), pageName(fileName, directory),
                            &metadata, error))
        return false;
      pages->append(metadata);
      if (TIFFLastDirectory(image.get()))
        break;
      if (!TIFFReadDirectory(image.get()))
        {
          *error = QString("Cannot read the TIFF directory after page %1 in %2")
            .arg(directory).arg(fileName);
          return false;
        }
    }
  return !pages->isEmpty();
}
}
