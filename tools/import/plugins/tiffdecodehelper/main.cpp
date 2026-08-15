#include <QCoreApplication>
#include <QByteArray>
#include <QFile>
#include <QTextStream>
#include <QStringList>

#include "../../tiffpagevalidation.h"

#include <tiffio.h>

#include <cstring>
#include <cstdio>
#include <cstdint>
#include <memory>
#include <new>
#include <limits>
#if defined(Q_OS_WIN)
#include <fcntl.h>
#include <io.h>
#endif

namespace
{
QString argument(const QStringList& args, const QString& name)
{
  const int index = args.indexOf(name);
  return index >= 0 && index+1 < args.size() ? args.at(index+1) : QString();
}
}

int main(int argc, char **argv)
{
  QCoreApplication app(argc, argv);
#if defined(Q_OS_WIN)
  // Decode mode writes raw scanline bytes; text-mode stdout would expand
  // every 0x0a to CRLF and violate the helper's exact-length protocol.
  _setmode(_fileno(stdout), _O_BINARY);
#endif
  const QStringList args = app.arguments();
  const QString imagePath = argument(app.arguments(), QStringLiteral("--image"));
  if (args.contains(QStringLiteral("--inspect")))
    {
      if (imagePath.isEmpty())
        {
          std::fprintf(stderr, "invalid TIFF inspect helper arguments\n");
          return 2;
        }
      TiffPageValidation::TiffHandle image =
        TiffPageValidation::openTiff(imagePath);
      if (!image)
        {
          std::fprintf(stderr, "cannot open TIFF input\n");
          return 3;
        }

      QTextStream output(stdout);
      output << "DRISHTI_TIFF_METADATA\t1\n";
      for (;;)
        {
          const tdir_t directory = TIFFCurrentDirectory(image.get());
          TiffPageValidation::PageMetadata metadata;
          QString error;
          if (!TiffPageValidation::readPageMetadata(
                image.get(),
                TiffPageValidation::pageName(imagePath, directory),
                &metadata, &error))
            {
              std::fprintf(stderr, "%s\n", error.toLocal8Bit().constData());
              return 4;
            }
          output << static_cast<quint64>(directory) << '\t'
                 << metadata.width << '\t' << metadata.height << '\t'
                 << metadata.bitsPerSample << '\t'
                 << metadata.samplesPerPixel << '\t'
                 << metadata.sampleFormat << '\t'
                 << metadata.planarConfig << '\t'
                 << metadata.photometric << '\t'
                 << metadata.orientation << '\t'
                 << metadata.compression << '\t'
                 << metadata.bytesPerVoxel << '\t'
                 << metadata.rowBytes << '\t'
                 << metadata.scanlineBytes << '\t'
                 << metadata.sliceBytes << '\n';
          if (TIFFLastDirectory(image.get()))
            break;
          if (!TIFFReadDirectory(image.get()))
            {
              std::fprintf(stderr, "cannot read TIFF directory\n");
              return 5;
            }
        }
      output.flush();
      return output.status() == QTextStream::Ok ? 0 : 6;
    }

  bool okDirectory = false;
  const uint directory = argument(app.arguments(), QStringLiteral("--directory"))
                           .toUInt(&okDirectory);
  bool okFirst = false;
  const int firstRow = argument(app.arguments(), QStringLiteral("--first-row"))
                         .toInt(&okFirst);
  bool okRows = false;
  const int rows = argument(app.arguments(), QStringLiteral("--rows"))
                     .toInt(&okRows);
  bool okBytes = false;
  const int rowBytes = argument(app.arguments(), QStringLiteral("--row-bytes"))
                         .toInt(&okBytes);
  if (imagePath.isEmpty() || !okDirectory || !okFirst || !okRows ||
      !okBytes || firstRow < 0 || rows <= 0 || rowBytes <= 0 ||
      static_cast<uint64_t>(firstRow) >
        static_cast<uint64_t>(std::numeric_limits<int>::max()) -
        static_cast<uint64_t>(rows))
    {
      std::fprintf(stderr, "invalid TIFF decode helper arguments\n");
      return 2;
    }

  TIFF *raw = 0;
#if defined(Q_OS_WIN)
  raw = TIFFOpenW(reinterpret_cast<const wchar_t*>(imagePath.utf16()), "r");
#else
  const QByteArray encodedPath = QFile::encodeName(imagePath);
  raw = TIFFOpen(encodedPath.constData(), "r");
#endif
  if (!raw)
    {
      std::fprintf(stderr, "cannot open TIFF input\n");
      return 3;
    }
  std::unique_ptr<TIFF, decltype(&TIFFClose)> image(raw, &TIFFClose);
  if (!TIFFSetDirectory(image.get(), static_cast<tdir_t>(directory)))
    {
      std::fprintf(stderr, "cannot select TIFF directory\n");
      return 4;
    }
  const uint64_t scanlineBytes = TIFFScanlineSize64(image.get());
  if (scanlineBytes < static_cast<uint64_t>(rowBytes))
    {
      std::fprintf(stderr, "TIFF scanline is shorter than requested output\n");
      return 5;
    }
  if (scanlineBytes == 0 ||
      scanlineBytes > static_cast<uint64_t>(std::numeric_limits<size_t>::max()))
    {
      std::fprintf(stderr, "TIFF scanline size is not representable\n");
      return 6;
    }
  std::unique_ptr<unsigned char[]> scanline(new (std::nothrow) unsigned char[
    static_cast<size_t>(scanlineBytes)]);
  if (!scanline)
    {
      std::fprintf(stderr, "cannot allocate TIFF scanline\n");
      return 6;
    }
  for (int row=0; row<rows; ++row)
    {
      const uint64_t sourceRow = static_cast<uint64_t>(firstRow) +
                                  static_cast<uint64_t>(row);
      if (sourceRow > std::numeric_limits<uint32_t>::max() ||
          TIFFReadScanline(image.get(), scanline.get(),
                           static_cast<uint32_t>(sourceRow), 0) < 0)
        {
          std::fprintf(stderr, "cannot decode TIFF row\n");
          return 7;
        }
      if (fwrite(scanline.get(), 1, static_cast<size_t>(rowBytes), stdout) !=
          static_cast<size_t>(rowBytes))
        {
          std::fprintf(stderr, "short write from TIFF decode helper\n");
          return 8;
        }
    }
  fflush(stdout);
  return 0;
}
