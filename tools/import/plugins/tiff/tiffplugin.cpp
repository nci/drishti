#include <QtGui>
#include "common.h"
#include "importmemoryadmission.h"
#include "tiffpagevalidation.h"
#include "tiffplugin.h"

#include <tiffio.h>

#include <QtConcurrent>
#include <QApplication>
#include <QCollator>
#include <QEventLoop>
#include <QFutureWatcher>
#include <QTimer>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <memory>
#include <new>

using namespace std;
using namespace TiffPageValidation;

namespace
{
  const quint64 kTiffDecodeSafetyBytes = 64ULL*1024ULL*1024ULL;

  QString
  tiffMemoryAmount(quint64 bytes)
  {
    const double mib = static_cast<double>(bytes)/(1024.0*1024.0);
    if (mib < 1024.0)
      return QStringLiteral("%1 MiB").arg(mib, 0, 'f', 1);
    return QStringLiteral("%1 GiB").arg(mib/1024.0, 0, 'f', 2);
  }

  bool
  admitTiffDecode(const QString &label,
                  quint64 sliceBytes,
                  quint64 scanlineBytes,
                  QString *error)
  {
    quint64 requiredBytes = 0;
    quint64 decodedAndCodecBytes = 0;
    if (!checkedImportMultiply(sliceBytes, 2, decodedAndCodecBytes) ||
        !checkedImportAdd(decodedAndCodecBytes, scanlineBytes,
                          requiredBytes) ||
        !checkedImportAdd(requiredBytes, kTiffDecodeSafetyBytes,
                          requiredBytes))
      {
        *error = QString("TIFF decode working-set calculation overflowed: %1")
                   .arg(label);
        return false;
      }

    const ImportMemoryAdmission admission =
      evaluateImportMemoryAdmission(requiredBytes);
    if (admission.approved)
      return true;

    QString reason;
    if (admission.reason ==
        ImportMemoryAdmissionReason::InsufficientPhysicalMemory)
      reason = QStringLiteral("insufficient physical-memory headroom");
    else if (admission.reason ==
             ImportMemoryAdmissionReason::InsufficientCommit)
      reason = QStringLiteral("insufficient Windows Commit headroom");
    else if (admission.reason ==
             ImportMemoryAdmissionReason::MemoryStatusUnavailable)
      reason = QStringLiteral(
        "live physical-memory or Windows Commit status unavailable");
    else
      reason = QStringLiteral("invalid or unsupported allocation size");

    *error = QString(
      "TIFF decoding was stopped before reading pixels from %1. "
      "Required peak increment: %2; usable physical budget: %3; "
      "usable Commit budget: %4; reason: %5.")
      .arg(label,
           tiffMemoryAmount(admission.requiredBytes),
           admission.physicalMemoryChecked ?
             tiffMemoryAmount(admission.availablePhysicalBudgetBytes) :
             QStringLiteral("unavailable"),
           admission.commitMemoryChecked ?
             tiffMemoryAmount(admission.availableCommitBudgetBytes) :
             QStringLiteral("unavailable"),
           reason);
    return false;
  }

}

TiffPlugin::StatisticsResult::StatisticsResult()
  : success(false),
    canceled(false),
    minimum(0),
    maximum(0)
{
}

QStringList
TiffPlugin::registerPlugin()
{
  QStringList regString;
  regString << "directory";
  regString << "Grayscale TIFF Image Directory";
  regString << "files";
  regString << "Grayscale TIFF Image Files";
  
  return regString;
}

void
TiffPlugin::init()
{
  TIFFSetWarningHandler(NULL);
  
  m_fileName.clear();
  m_imageList.clear();
  m_directoryList.clear();

  m_description.clear();
  m_depth = m_width = m_height = 0;
  m_voxelType = _UChar;
  m_voxelUnit = _Micron;
  m_voxelSizeX = m_voxelSizeY = m_voxelSizeZ = 1;
  m_bytesPerVoxel = 1;
  m_headerBytes = 0;
  m_rawMin = m_rawMax = 0;
  m_histogram.clear();
  m_sliceBytes = 0;
  m_cancelRequested.store(false);
  m_progressValue.store(0);
  m_lastError.clear();
  m_lastOperationCanceled = false;
  m_4dvol = false;
}

void
TiffPlugin::clear()
{
  m_fileName.clear();
  m_imageList.clear();
  m_directoryList.clear();

  m_description.clear();
  m_depth = m_width = m_height = 0;
  m_voxelType = _UChar;
  m_voxelUnit = _Micron;
  m_voxelSizeX = m_voxelSizeY = m_voxelSizeZ = 1;
  m_bytesPerVoxel = 1;
  m_headerBytes = 0;
  m_rawMin = m_rawMax = 0;
  m_histogram.clear();
  m_sliceBytes = 0;
  m_cancelRequested.store(false);
  m_progressValue.store(0);
  m_lastError.clear();
  m_lastOperationCanceled = false;
  m_4dvol = false;
}

void
TiffPlugin::set4DVolume(bool flag)
{
  m_4dvol = flag;
}

void
TiffPlugin::voxelSize(float& vx, float& vy, float& vz)
  {
    vx = m_voxelSizeX;
    vy = m_voxelSizeY;
    vz = m_voxelSizeZ;
  }
QString TiffPlugin::description() { return m_description; }
int TiffPlugin::voxelType() { return m_voxelType; }
int TiffPlugin::voxelUnit() { return m_voxelUnit; }
int TiffPlugin::headerBytes() { return m_headerBytes; }

void
TiffPlugin::setMinMax(float rmin, float rmax)
{
  m_rawMin = rmin;
  m_rawMax = rmax;
}
float TiffPlugin::rawMin() { return m_rawMin; }
float TiffPlugin::rawMax() { return m_rawMax; }
QList<uint> TiffPlugin::histogram() { return m_histogram; }

void
TiffPlugin::gridSize(int& d, int& w, int& h)
{
  d = m_depth;
  w = m_width;
  h = m_height;
}

void
TiffPlugin::replaceFile(QString flnm)
{
  m_lastError.clear();
  m_lastOperationCanceled = false;
  const QStringList previousImageList = m_imageList;
  const QVector<quint32> previousDirectoryList = m_directoryList;
  const int previousDepth = m_depth;
  const int previousWidth = m_width;
  const int previousHeight = m_height;
  const int previousVoxelType = m_voxelType;
  const int previousBytesPerVoxel = m_bytesPerVoxel;
  const int previousHeaderBytes = m_headerBytes;
  const quint64 previousSliceBytes = m_sliceBytes;

  QString error;
  QStringList files;
  files << flnm;
  if (!setImageFiles(files, &error))
    {
      m_lastError = error;
      m_lastOperationCanceled = error == "TIFF import canceled";
      qWarning() << "Cannot replace TIFF input:" << error;
      return;
    }

  bool compatible = previousImageList.isEmpty() ||
                    (m_depth == previousDepth &&
                     m_width == previousWidth &&
                     m_height == previousHeight &&
                     m_voxelType == previousVoxelType &&
                     m_bytesPerVoxel == previousBytesPerVoxel &&
                     m_sliceBytes == previousSliceBytes);
  if (!compatible)
    {
      m_imageList = previousImageList;
      m_directoryList = previousDirectoryList;
      m_depth = previousDepth;
      m_width = previousWidth;
      m_height = previousHeight;
      m_voxelType = previousVoxelType;
      m_bytesPerVoxel = previousBytesPerVoxel;
      m_headerBytes = previousHeaderBytes;
      m_sliceBytes = previousSliceBytes;
      m_lastError =
        "Cannot replace TIFF input: stack layout differs from the original.";
      qWarning() << m_lastError;
      return;
    }

  // Time-series replacement intentionally keeps the original histogram and range.
  m_fileName = files;
}

bool
TiffPlugin::setImageFiles(const QStringList &files, QString *error)
{
  if (files.isEmpty())
    {
      *error = "No TIFF files were selected";
      return false;
    }

  QStringList sliceFiles;
  QVector<quint32> sliceDirectories;
  PageMetadata baseline;
  bool haveBaseline = false;

  QProgressDialog progress("Validating TIFF metadata",
                           "Cancel",
                           0, files.size(),
                           0);
  progress.setWindowModality(Qt::ApplicationModal);
  progress.setMinimumDuration(0);

  for (int fileIndex=0; fileIndex<files.size(); ++fileIndex)
    {
      if (progress.wasCanceled())
        {
          *error = "TIFF import canceled";
          return false;
        }

      QFileInfo fileInfo(files[fileIndex]);
      QString absolutePath = fileInfo.absoluteFilePath();
      if (!fileInfo.exists() || !fileInfo.isFile() || !fileInfo.isReadable())
        {
          *error = QString("TIFF file is missing or unreadable: %1")
                     .arg(absolutePath);
          return false;
        }

      progress.setLabelText(QString("Validating %1 of %2\n%3")
                              .arg(fileIndex+1).arg(files.size()).arg(absolutePath));
      progress.setValue(fileIndex);
      qApp->processEvents();
      if (progress.wasCanceled())
        {
          *error = "TIFF import canceled";
          return false;
        }

      TiffHandle image = openTiff(absolutePath);
      if (!image)
        {
          *error = QString("Cannot open TIFF file: %1").arg(absolutePath);
          return false;
        }

      while (true)
        {
          tdir_t directory = TIFFCurrentDirectory(image.get());
          PageMetadata metadata;
          QString label = pageName(absolutePath, directory);
          if (!readPageMetadata(image.get(), label, &metadata, error))
            return false;

          if (!haveBaseline)
            {
              baseline = metadata;
              haveBaseline = true;
            }
          else
            {
              QString difference;
              if (!samePageLayout(baseline, metadata, &difference))
                {
                  *error = QString("TIFF stack has inconsistent %1 at %2")
                             .arg(difference).arg(label);
                  return false;
                }
            }

          if (sliceFiles.size() == std::numeric_limits<int>::max())
            {
              *error = "TIFF stack has too many pages for the Qt 5 importer";
              return false;
            }

          sliceFiles.append(absolutePath);
          sliceDirectories.append(static_cast<quint32>(directory));

          if ((sliceFiles.size() & 63) == 0)
            {
              qApp->processEvents();
              if (progress.wasCanceled())
                {
                  *error = "TIFF import canceled";
                  return false;
                }
            }

          if (TIFFLastDirectory(image.get()))
            break;

          if (!TIFFReadDirectory(image.get()))
            {
              *error = QString("Cannot read the TIFF directory after page %1 in %2")
                         .arg(directory).arg(absolutePath);
              return false;
            }
        }
    }

  progress.setValue(files.size());

  if (!haveBaseline || sliceFiles.isEmpty())
    {
      *error = "No readable TIFF pages were found";
      return false;
    }

  m_imageList = sliceFiles;
  m_directoryList = sliceDirectories;
  m_depth = sliceFiles.size();
  // VolInterface names the in-plane row axis "width" and the column axis
  // "height".  Keep decoded TIFF scanlines in their native row-major order.
  m_width = static_cast<int>(baseline.height);
  m_height = static_cast<int>(baseline.width);
  m_sliceBytes = baseline.sliceBytes;
  m_bytesPerVoxel = static_cast<int>(baseline.bytesPerVoxel);
  m_headerBytes = 0;

  if (baseline.bitsPerSample == 8)
    m_voxelType = baseline.sampleFormat == SAMPLEFORMAT_INT ? _Char : _UChar;
  else if (baseline.bitsPerSample == 16)
    m_voxelType = baseline.sampleFormat == SAMPLEFORMAT_INT ? _Short : _UShort;
  else
    m_voxelType = baseline.sampleFormat == SAMPLEFORMAT_INT ? _Int : _Float;

  return true;
}

bool
TiffPlugin::setFile(QStringList files)
{
  m_lastError.clear();
  m_lastOperationCanceled = false;
  if (files.isEmpty())
    {
      m_lastError = "No TIFF files were selected";
      return false;
    }

  const QStringList previousFileName = m_fileName;
  const QStringList previousImageList = m_imageList;
  const QVector<quint32> previousDirectoryList = m_directoryList;
  const int previousDepth = m_depth;
  const int previousWidth = m_width;
  const int previousHeight = m_height;
  const int previousVoxelType = m_voxelType;
  const int previousBytesPerVoxel = m_bytesPerVoxel;
  const int previousHeaderBytes = m_headerBytes;
  const quint64 previousSliceBytes = m_sliceBytes;
  const float previousRawMin = m_rawMin;
  const float previousRawMax = m_rawMax;
  const QList<uint> previousHistogram = m_histogram;

  auto restorePreviousState = [&]()
    {
      m_fileName = previousFileName;
      m_imageList = previousImageList;
      m_directoryList = previousDirectoryList;
      m_depth = previousDepth;
      m_width = previousWidth;
      m_height = previousHeight;
      m_voxelType = previousVoxelType;
      m_bytesPerVoxel = previousBytesPerVoxel;
      m_headerBytes = previousHeaderBytes;
      m_sliceBytes = previousSliceBytes;
      m_rawMin = previousRawMin;
      m_rawMax = previousRawMax;
      m_histogram = previousHistogram;
    };

  QFileInfo f(files[0]);
  QStringList imageFiles;
  if (f.isDir())
    {
      QStringList imageNameFilter;
      imageNameFilter << "*.tif" << "*.tiff" << "*.TIF" << "*.TIFF";
      QDir directory(files[0]);
      QStringList names = directory.entryList(imageNameFilter,
                                               QDir::NoSymLinks|
                                               QDir::NoDotAndDotDot|
                                               QDir::Readable|
                                               QDir::Files,
                                               QDir::NoSort);
      QCollator collator;
      collator.setCaseSensitivity(Qt::CaseInsensitive);
      collator.setNumericMode(true);
      std::sort(names.begin(), names.end(),
                [&collator](const QString &left, const QString &right)
                {
                  const int comparison = collator.compare(left, right);
                  return comparison == 0 ? left < right : comparison < 0;
                });
      for (int i=0; i<names.size(); ++i)
        imageFiles.append(directory.absoluteFilePath(names[i]));
    }
  else
    {
      for (int i=0; i<files.size(); ++i)
        imageFiles.append(QFileInfo(files[i]).absoluteFilePath());
    }

  QString error;
  if (!setImageFiles(imageFiles, &error))
    {
      m_lastError = error;
      m_lastOperationCanceled = error == "TIFF import canceled";
      return false;
    }

  if (!generateStatistics(&error))
    {
      restorePreviousState();
      m_lastError = error;
      m_lastOperationCanceled = error == "TIFF import canceled";
      return false;
    }

  m_fileName = files;
  return true;
}


bool
TiffPlugin::loadTiffImage(int i,
                          uchar *destination,
                          quint64 destinationBytes,
                          QString *error,
                          const std::atomic_bool *cancelRequested) const
{
  if (i < 0 || i >= m_imageList.size() || i >= m_directoryList.size())
    {
      *error = QString("TIFF slice index %1 is out of range").arg(i);
      return false;
    }

  if (!destination || destinationBytes < m_sliceBytes)
    {
      *error = QString("TIFF destination buffer is too small for slice %1").arg(i);
      return false;
    }

  TiffHandle image = openTiff(m_imageList[i]);
  if (!image)
    {
      *error = QString("Cannot open TIFF file: %1").arg(m_imageList[i]);
      return false;
    }

  quint32 directory = m_directoryList[i];
  if (static_cast<quint64>(directory) >
        static_cast<quint64>(std::numeric_limits<tdir_t>::max()) ||
      !TIFFSetDirectory(image.get(), static_cast<tdir_t>(directory)))
    {
      *error = QString("Cannot select TIFF page %1 in %2")
                 .arg(directory).arg(m_imageList[i]);
      return false;
    }

  quint64 scanlineBytes64 = TIFFScanlineSize64(image.get());
  quint64 rowBytes = static_cast<quint64>(m_height)*m_bytesPerVoxel;
  if (scanlineBytes64 < rowBytes ||
      scanlineBytes64 > static_cast<quint64>(std::numeric_limits<int>::max()))
    {
      *error = QString("Invalid TIFF scanline size in %1")
                 .arg(pageName(m_imageList[i], directory));
      return false;
    }

  const QString label = pageName(m_imageList[i], directory);
  if (!admitTiffDecode(label, m_sliceBytes, scanlineBytes64, error))
    return false;

  std::unique_ptr<uchar[]> scanline(new (std::nothrow) uchar[
    static_cast<size_t>(scanlineBytes64)]);
  if (!scanline)
    {
      *error = QString("Cannot allocate the TIFF scanline buffer for %1")
                 .arg(label);
      return false;
    }
  for (int row=0; row<m_width; ++row)
    {
      if (cancelRequested && cancelRequested->load())
        {
          *error = "TIFF import canceled";
          return false;
        }

      if (TIFFReadScanline(image.get(), scanline.get(),
                           static_cast<uint32_t>(row), 0) < 0)
        {
          *error = QString("Cannot decode row %1 from %2")
                     .arg(row).arg(pageName(m_imageList[i], directory));
          return false;
        }

      quint64 destinationOffset = static_cast<quint64>(row)*rowBytes;
      if (destinationOffset > destinationBytes ||
          rowBytes > destinationBytes-destinationOffset)
        {
          *error = QString("Decoded TIFF row exceeds the destination buffer in %1")
                     .arg(pageName(m_imageList[i], directory));
          return false;
        }

      std::memcpy(destination+destinationOffset,
                  scanline.get(),
                  static_cast<size_t>(rowBytes));
    }

  return true;
}

bool
TiffPlugin::loadTiffRow(int slice,
                        int row,
                        QByteArray *scanline,
                        QString *error) const
{
  if (slice < 0 || slice >= m_imageList.size() ||
      slice >= m_directoryList.size() ||
      row < 0 || row >= m_width)
    {
      *error = "TIFF pixel location is out of range";
      return false;
    }

  TiffHandle image = openTiff(m_imageList[slice]);
  if (!image)
    {
      *error = QString("Cannot open TIFF file: %1").arg(m_imageList[slice]);
      return false;
    }

  quint32 directory = m_directoryList[slice];
  if (static_cast<quint64>(directory) >
        static_cast<quint64>(std::numeric_limits<tdir_t>::max()) ||
      !TIFFSetDirectory(image.get(), static_cast<tdir_t>(directory)))
    {
      *error = QString("Cannot select TIFF page %1 in %2")
                 .arg(directory).arg(m_imageList[slice]);
      return false;
    }

  quint64 scanlineBytes64 = TIFFScanlineSize64(image.get());
  quint64 requiredBytes = static_cast<quint64>(m_height)*m_bytesPerVoxel;
  if (scanlineBytes64 < requiredBytes ||
      scanlineBytes64 > static_cast<quint64>(std::numeric_limits<int>::max()))
    {
      *error = QString("Invalid TIFF scanline size in %1")
                 .arg(pageName(m_imageList[slice], directory));
      return false;
    }

  if (!admitTiffDecode(pageName(m_imageList[slice], directory),
                       m_sliceBytes, scanlineBytes64, error))
    return false;

  scanline->resize(static_cast<int>(scanlineBytes64));
  if (TIFFReadScanline(image.get(), scanline->data(),
                       static_cast<uint32_t>(row), 0) < 0)
    {
      *error = QString("Cannot decode row %1 from %2")
                 .arg(row).arg(pageName(m_imageList[slice], directory));
      return false;
    }

  return true;
}

TiffPlugin::StatisticsResult
TiffPlugin::calculateStatistics()
{
  StatisticsResult result;
  if (m_depth <= 0 || m_width <= 0 || m_height <= 0 || m_sliceBytes == 0)
    {
      result.error = "TIFF stack dimensions are invalid";
      return result;
    }

  quint64 pixelCount = static_cast<quint64>(m_width)*m_height;
  QString admissionError;
  if (!admitTiffDecode(QStringLiteral("TIFF statistics"),
                       m_sliceBytes, 0, &admissionError))
    {
      result.error = admissionError;
      return result;
    }
  std::unique_ptr<uchar[]> slice(new (std::nothrow) uchar[
    static_cast<size_t>(m_sliceBytes)]);
  if (!slice)
    {
      result.error = QString("Cannot allocate a %1-byte TIFF slice buffer")
                       .arg(m_sliceBytes);
      return result;
    }

  bool exactHistogram = (m_voxelType == _UChar ||
                         m_voxelType == _Char ||
                         m_voxelType == _UShort ||
                         m_voxelType == _Short);
  int histogramSize = 65536;
  if (m_voxelType == _UChar || m_voxelType == _Char)
    histogramSize = 256;
  result.histogram.fill(0, histogramSize);

  double minimum = std::numeric_limits<double>::infinity();
  double maximum = -std::numeric_limits<double>::infinity();
  bool haveFiniteValue = false;

  for (int sliceIndex=0; sliceIndex<m_depth; ++sliceIndex)
    {
      if (m_cancelRequested.load())
        {
          result.canceled = true;
          result.error = "TIFF import canceled";
          return result;
        }

      QString error;
      if (!loadTiffImage(sliceIndex, slice.get(), m_sliceBytes,
                         &error, &m_cancelRequested))
        {
          result.canceled = m_cancelRequested.load();
          result.error = result.canceled ? "TIFF import canceled" : error;
          return result;
        }

      if (m_voxelType == _UChar)
        {
          const quint8 *values = reinterpret_cast<const quint8*>(slice.get());
          for (quint64 i=0; i<pixelCount; ++i)
            {
              quint8 value = values[i];
              result.histogram[value]++;
              minimum = qMin(minimum, static_cast<double>(value));
              maximum = qMax(maximum, static_cast<double>(value));
            }
          haveFiniteValue = true;
        }
      else if (m_voxelType == _Char)
        {
          const qint8 *values = reinterpret_cast<const qint8*>(slice.get());
          for (quint64 i=0; i<pixelCount; ++i)
            {
              qint8 value = values[i];
              result.histogram[static_cast<int>(value)+128]++;
              minimum = qMin(minimum, static_cast<double>(value));
              maximum = qMax(maximum, static_cast<double>(value));
            }
          haveFiniteValue = true;
        }
      else if (m_voxelType == _UShort)
        {
          const quint16 *values = reinterpret_cast<const quint16*>(slice.get());
          for (quint64 i=0; i<pixelCount; ++i)
            {
              quint16 value = values[i];
              result.histogram[value]++;
              minimum = qMin(minimum, static_cast<double>(value));
              maximum = qMax(maximum, static_cast<double>(value));
            }
          haveFiniteValue = true;
        }
      else if (m_voxelType == _Short)
        {
          const qint16 *values = reinterpret_cast<const qint16*>(slice.get());
          for (quint64 i=0; i<pixelCount; ++i)
            {
              qint16 value = values[i];
              result.histogram[static_cast<int>(value)+32768]++;
              minimum = qMin(minimum, static_cast<double>(value));
              maximum = qMax(maximum, static_cast<double>(value));
            }
          haveFiniteValue = true;
        }
      else if (m_voxelType == _Int)
        {
          const qint32 *values = reinterpret_cast<const qint32*>(slice.get());
          for (quint64 i=0; i<pixelCount; ++i)
            {
              double value = values[i];
              minimum = qMin(minimum, value);
              maximum = qMax(maximum, value);
            }
          haveFiniteValue = true;
        }
      else if (m_voxelType == _Float)
        {
          const float *values = reinterpret_cast<const float*>(slice.get());
          for (quint64 i=0; i<pixelCount; ++i)
            {
              double value = values[i];
              if (!std::isfinite(value))
                continue;
              minimum = qMin(minimum, value);
              maximum = qMax(maximum, value);
              haveFiniteValue = true;
            }
        }

      m_progressValue.fetch_add(1);
    }

  if (!haveFiniteValue)
    {
      result.error = "TIFF stack contains no finite pixel values";
      return result;
    }

  if (!exactHistogram)
    {
      double valueRange = maximum-minimum;
      for (int sliceIndex=0; sliceIndex<m_depth; ++sliceIndex)
        {
          if (m_cancelRequested.load())
            {
              result.canceled = true;
              result.error = "TIFF import canceled";
              return result;
            }

          QString error;
          if (!loadTiffImage(sliceIndex, slice.get(), m_sliceBytes,
                             &error, &m_cancelRequested))
            {
              result.canceled = m_cancelRequested.load();
              result.error = result.canceled ? "TIFF import canceled" : error;
              return result;
            }

          if (m_voxelType == _Int)
            {
              const qint32 *values = reinterpret_cast<const qint32*>(slice.get());
              for (quint64 i=0; i<pixelCount; ++i)
                {
                  int bin = 0;
                  if (valueRange > 0)
                    bin = static_cast<int>(65535.0*(values[i]-minimum)/valueRange);
                  bin = qBound(0, bin, 65535);
                  result.histogram[bin]++;
                }
            }
          else
            {
              const float *values = reinterpret_cast<const float*>(slice.get());
              for (quint64 i=0; i<pixelCount; ++i)
                {
                  double value = values[i];
                  if (!std::isfinite(value))
                    continue;
                  int bin = 0;
                  if (valueRange > 0)
                    bin = static_cast<int>(65535.0*(value-minimum)/valueRange);
                  bin = qBound(0, bin, 65535);
                  result.histogram[bin]++;
                }
            }

          m_progressValue.fetch_add(1);
        }
    }

  result.minimum = static_cast<float>(minimum);
  result.maximum = static_cast<float>(maximum);
  result.success = true;
  return result;
}

bool
TiffPlugin::generateStatistics(QString *error)
{
  m_cancelRequested.store(false);
  m_progressValue.store(0);

  bool twoPass = (m_voxelType == _Int || m_voxelType == _Float);
  qint64 totalSteps64 = static_cast<qint64>(m_depth)*(twoPass ? 2 : 1);
  if (totalSteps64 > std::numeric_limits<int>::max())
    {
      *error = "TIFF stack has too many pages for progress tracking";
      return false;
    }
  int totalSteps = static_cast<int>(totalSteps64);

  QProgressDialog progress("Scanning TIFF data",
                           "Cancel",
                           0, totalSteps,
                           QApplication::activeWindow());
  progress.setWindowModality(Qt::ApplicationModal);
  progress.setMinimumDuration(0);
  progress.setAutoClose(false);
  progress.setAutoReset(false);

  QEventLoop waitLoop;
  QTimer progressTimer;
  QFutureWatcher<StatisticsResult> watcher;

  QObject::connect(&progress, &QProgressDialog::canceled,
                   [this]() { m_cancelRequested.store(true); });
  QObject::connect(&watcher, &QFutureWatcher<StatisticsResult>::finished,
                   &waitLoop, &QEventLoop::quit);
  QObject::connect(&progressTimer, &QTimer::timeout,
                   [&]()
                   {
                     int value = qMin(m_progressValue.load(), totalSteps);
                     progress.setValue(value);
                     progress.setLabelText(QString("Scanning TIFF slice %1 of %2")
                                             .arg(value).arg(totalSteps));
                   });

  QFuture<StatisticsResult> future = QtConcurrent::run(
    [this]() { return calculateStatistics(); });
  watcher.setFuture(future);
  progressTimer.start(50);
  waitLoop.exec();
  progressTimer.stop();
  // QProgressDialog::close() emits canceled() and would leave the shared
  // cancellation flag set even though the statistics worker completed.
  progress.hide();

  StatisticsResult result = watcher.result();
  if (!result.success)
    {
      *error = result.error;
      return false;
    }

  m_rawMin = result.minimum;
  m_rawMax = result.maximum;
  m_histogram.clear();
  m_histogram.reserve(result.histogram.size());
  const quint64 maxUiCount = std::numeric_limits<uint>::max();
  for (int i=0; i<result.histogram.size(); ++i)
    m_histogram.append(static_cast<uint>(qMin(result.histogram[i], maxUiCount)));

  return true;
}

void
TiffPlugin::generateHistogram()
{
  QString error;
  if (!generateStatistics(&error))
    {
      m_lastError = error;
      m_lastOperationCanceled = error == "TIFF import canceled";
    }
}

void
TiffPlugin::getDepthSlice(int slc,
			  uchar *slice)
{
  m_lastError.clear();
  if (!slice || slc < 0 || slc >= m_depth || m_sliceBytes == 0)
    {
      m_lastError = QString("TIFF slice %1 is invalid.").arg(slc);
      return;
    }

  QString error;
  if (!admitTiffDecode(QStringLiteral("TIFF preview"),
                       m_sliceBytes, 0, &error))
    {
      m_lastError = error;
      qWarning() << error;
      std::memset(slice, 0, static_cast<size_t>(m_sliceBytes));
      return;
    }

  if (!loadTiffImage(slc, slice, m_sliceBytes, &error))
    {
      m_lastError = error;
      qWarning() << "Cannot load TIFF preview:" << error;
      std::memset(slice, 0, static_cast<size_t>(m_sliceBytes));
      return;
    }
}

QString TiffPlugin::lastError() const { return m_lastError; }
bool TiffPlugin::wasCanceled() const
{
  return m_lastOperationCanceled;
}

void
TiffPlugin::getWidthSlice(int slc, uchar *slice)
{
  m_lastError.clear();
  m_lastOperationCanceled = false;
  quint64 outputBytes = static_cast<quint64>(m_depth)*m_height*m_bytesPerVoxel;
  if (!slice || slc < 0 || slc >= m_width || outputBytes == 0)
    {
      m_lastError = QString("TIFF width slice %1 is invalid.").arg(slc);
      return;
    }
  std::memset(slice, 0, static_cast<size_t>(outputBytes));

  QProgressDialog progress("Extracting TIFF width slice", "Cancel",
                           0, m_depth, 0);
  progress.setMinimumDuration(500);
  for (int depth=0; depth<m_depth; ++depth)
    {
      if ((depth & 15) == 0)
        {
          progress.setValue(depth);
          qApp->processEvents();
          if (progress.wasCanceled())
            {
              m_lastError = "TIFF preview canceled";
              m_lastOperationCanceled = true;
              std::memset(slice, 0, static_cast<size_t>(outputBytes));
              return;
            }
        }

      QByteArray row;
      QString error;
      if (!loadTiffRow(depth, slc, &row, &error))
        {
          m_lastError = error;
          std::memset(slice, 0, static_cast<size_t>(outputBytes));
          return;
        }
      const size_t rowBytes = static_cast<size_t>(m_height)*m_bytesPerVoxel;
      std::memcpy(slice+static_cast<quint64>(depth)*rowBytes,
                  row.constData(), rowBytes);
    }
  progress.setValue(m_depth);
}

void
TiffPlugin::getHeightSlice(int slc, uchar *slice)
{
  m_lastError.clear();
  m_lastOperationCanceled = false;
  quint64 outputBytes = static_cast<quint64>(m_depth)*m_width*m_bytesPerVoxel;
  if (!slice || slc < 0 || slc >= m_height || outputBytes == 0)
    {
      m_lastError = QString("TIFF height slice %1 is invalid.").arg(slc);
      return;
    }
  std::memset(slice, 0, static_cast<size_t>(outputBytes));

  std::unique_ptr<uchar[]> decoded(new (std::nothrow) uchar[
    static_cast<size_t>(m_sliceBytes)]);
  if (!decoded)
    {
      m_lastError = QString("Cannot allocate TIFF slice buffer (%1 bytes).")
                      .arg(m_sliceBytes);
      return;
    }

  QProgressDialog progress("Extracting TIFF height slice", "Cancel",
                           0, m_depth, 0);
  progress.setMinimumDuration(500);
  for (int depth=0; depth<m_depth; ++depth)
    {
      if ((depth & 15) == 0)
        {
          progress.setValue(depth);
          qApp->processEvents();
          if (progress.wasCanceled())
            {
              m_lastError = "TIFF preview canceled";
              m_lastOperationCanceled = true;
              std::memset(slice, 0, static_cast<size_t>(outputBytes));
              return;
            }
        }

      QString error;
      if (!loadTiffImage(depth, decoded.get(), m_sliceBytes, &error))
        {
          m_lastError = error;
          std::memset(slice, 0, static_cast<size_t>(outputBytes));
          return;
        }
      for (int row=0; row<m_width; ++row)
        {
          const quint64 sourceOffset =
            (static_cast<quint64>(row)*m_height+slc)*m_bytesPerVoxel;
          const quint64 destinationOffset =
            (static_cast<quint64>(depth)*m_width+row)*m_bytesPerVoxel;
          std::memcpy(slice+destinationOffset, decoded.get()+sourceOffset,
                      static_cast<size_t>(m_bytesPerVoxel));
        }
    }
  progress.setValue(m_depth);
}

QVariant
TiffPlugin::rawValue(int d, int w, int h)
{
  if (d < 0 || d >= m_depth ||
      w < 0 || w >= m_width ||
      h < 0 || h >= m_height)
    return QVariant("OutOfBounds");

  QByteArray scanline;
  QString error;
  if (!loadTiffRow(d, w, &scanline, &error))
    return QVariant(error);

  quint64 byteOffset = static_cast<quint64>(h)*m_bytesPerVoxel;
  if (byteOffset+m_bytesPerVoxel > static_cast<quint64>(scanline.size()))
    return QVariant("OutOfBounds");

  const uchar *pixel =
    reinterpret_cast<const uchar*>(scanline.constData())+byteOffset;
  if (m_voxelType == _UChar)
    return QVariant(static_cast<uint>(*reinterpret_cast<const quint8*>(pixel)));
  if (m_voxelType == _Char)
    return QVariant(static_cast<int>(*reinterpret_cast<const qint8*>(pixel)));
  if (m_voxelType == _UShort)
    {
      quint16 value;
      std::memcpy(&value, pixel, sizeof(value));
      return QVariant(static_cast<uint>(value));
    }
  if (m_voxelType == _Short)
    {
      qint16 value;
      std::memcpy(&value, pixel, sizeof(value));
      return QVariant(static_cast<int>(value));
    }
  if (m_voxelType == _Int)
    {
      qint32 value;
      std::memcpy(&value, pixel, sizeof(value));
      return QVariant(value);
    }
  if (m_voxelType == _Float)
    {
      float value;
      std::memcpy(&value, pixel, sizeof(value));
      return QVariant(static_cast<double>(value));
    }

  return QVariant("UnsupportedType");
}

//void
//TiffPlugin::saveTrimmed(QString trimFile,
//			    int dmin, int dmax,
//			    int wmin, int wmax,
//			    int hmin, int hmax)
//{
//  QProgressDialog progress("Saving trimmed volume",
//			   0,
//			   0, 100,
//			   0);
//  progress.setMinimumDuration(0);
//
//  int nX, nY, nZ;
//  nX = m_depth;
//  nY = m_width;
//  nZ = m_height;
//
//  int mX, mY, mZ;
//  mX = dmax-dmin+1;
//  mY = wmax-wmin+1;
//  mZ = hmax-hmin+1;
//
//  int nbytes = nY*nZ*m_bytesPerVoxel;
//  uchar *tmp = new uchar[nbytes];
//  uchar *tmp1 = new uchar[nbytes];
//
//  uchar vt;
//  if (m_voxelType == _UChar) vt = 0; // unsigned byte
//  if (m_voxelType == _Char) vt = 1; // signed byte
//  if (m_voxelType == _UShort) vt = 2; // unsigned short
//  if (m_voxelType == _Short) vt = 3; // signed short
//  if (m_voxelType == _Int) vt = 4; // int
//  if (m_voxelType == _Float) vt = 8; // float
//  
//  QFile fout(trimFile);
//  fout.open(QFile::WriteOnly);
//
//  fout.write((char*)&vt, 1);
//  fout.write((char*)&mX, 4);
//  fout.write((char*)&mY, 4);
//  fout.write((char*)&mZ, 4);
//
//  //for(uint i=dmin; i<=dmax; i++)
//  for(int i=dmax; i>=dmin; i--)
//    {
//      loadTiffImage(i, tmp1);
//
//      if (m_voxelType == _UChar)
//	{
//	  for(uint j=0; j<m_width; j++)
//	    for(uint k=0; k<m_height; k++)
//	      tmp[j*m_height+k] = tmp1[k*m_width+j];
//	}
//      else if (m_voxelType == _UShort)
//	{
//	  ushort *p0 = (ushort*)tmp;
//	  ushort *p1 = (ushort*)tmp1;
//	  for(uint j=0; j<m_width; j++)
//	    for(uint k=0; k<m_height; k++)
//	      p0[j*m_height+k] = p1[k*m_width+j];
//	}
//      else if (m_voxelType == _Float)
//	{
//	  float *p0 = (float*)tmp;
//	  float *p1 = (float*)tmp1;
//	  for(uint j=0; j<m_width; j++)
//	    for(uint k=0; k<m_height; k++)
//	      p0[j*m_height+k] = p1[k*m_width+j];
//	}
//      
//
//      for(uint j=wmin; j<=wmax; j++)
//	{
//	  memcpy(tmp+(j-wmin)*mZ*m_bytesPerVoxel,
//		 tmp+(j*nZ + hmin)*m_bytesPerVoxel,
//		 mZ*m_bytesPerVoxel);
//	}
//	  
//      fout.write((char*)tmp, mY*mZ*m_bytesPerVoxel);
//
//      progress.setValue((int)(100*(float)(dmax-i)/(float)mX));
//      qApp->processEvents();
//    }
//
//  fout.close();
//
//  delete [] tmp;
//  delete [] tmp1;
//
//  progress.setValue(100);
//
//  m_headerBytes = 13; // to be used for applyMapping function
//}
