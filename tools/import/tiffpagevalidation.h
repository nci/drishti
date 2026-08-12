#ifndef TIFFPAGEVALIDATION_H
#define TIFFPAGEVALIDATION_H

#include <QString>
#include <QVector>

#include <memory>

#include <tiffio.h>

namespace TiffPageValidation
{
struct TiffCloser
{
  void operator()(TIFF *image) const;
};

typedef std::unique_ptr<TIFF, TiffCloser> TiffHandle;

struct PageMetadata
{
  uint32_t width;
  uint32_t height;
  uint16_t bitsPerSample;
  uint16_t samplesPerPixel;
  uint16_t sampleFormat;
  uint16_t planarConfig;
  uint16_t photometric;
  uint16_t orientation;
  uint16_t compression;
  quint64 bytesPerVoxel;
  quint64 rowBytes;
  quint64 scanlineBytes;
  quint64 sliceBytes;
};

TiffHandle openTiff(const QString& fileName);
QString pageName(const QString& fileName, quint64 directory);

bool readPageMetadata(TIFF *image,
                      const QString& label,
                      PageMetadata *metadata,
                      QString *error);
bool samePageLayout(const PageMetadata& expected,
                    const PageMetadata& actual,
                    QString *difference);
bool inspectFile(const QString& fileName,
                 QVector<PageMetadata> *pages,
                 QString *error);
}

#endif
