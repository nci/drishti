#include <QtGui>
#include <QDomDocument>
#include "common.h"
#include "importmemoryadmission.h"
#include "../../volumefilemanager.h"
#include "imagestackplugin.h"
#include "../../tiffinputrouting.h"
#include "imagestackpixelconversion.h"

#include <QtConcurrent>
#include <QApplication>
#include <QCollator>
#include <QEventLoop>
#include <QFutureWatcher>
#include <QImageReader>
#include <QSaveFile>
#include <QTimer>

#include <atomic>
#include <algorithm>
#include <cstring>
#include <limits>
#include <memory>
#include <new>

namespace
{
const quint64 kImageDecodeSafetyBytes = 64ULL*1024ULL*1024ULL;

bool
checkedImageSize(int width, int height, int bytesPerPixel,
		 qint64 &pixelCount, qint64 &byteCount)
{
  pixelCount = 0;
  byteCount = 0;

  if (width <= 0 || height <= 0 || bytesPerPixel <= 0)
    return false;

  pixelCount = static_cast<qint64>(width)*static_cast<qint64>(height);
  const qint64 maxInt = std::numeric_limits<int>::max();
  if (pixelCount > maxInt || pixelCount > maxInt/bytesPerPixel)
    return false;

  byteCount = pixelCount*static_cast<qint64>(bytesPerPixel);
  return true;
}

QString
imageMemoryAmount(quint64 bytes)
{
  const double mib = static_cast<double>(bytes)/(1024.0*1024.0);
  if (mib < 1024.0)
    return QStringLiteral("%1 MiB").arg(mib, 0, 'f', 1);
  return QStringLiteral("%1 GiB").arg(mib/1024.0, 0, 'f', 2);
}

QString
imageAdmissionError(const QString &fileName,
                    const ImportMemoryAdmission &admission)
{
  QString reason;
  if (admission.reason ==
      ImportMemoryAdmissionReason::InsufficientPhysicalMemory)
    reason = QStringLiteral("insufficient physical-memory headroom");
  else if (admission.reason == ImportMemoryAdmissionReason::InsufficientCommit)
    reason = QStringLiteral("insufficient Windows Commit headroom");
  else if (admission.reason ==
           ImportMemoryAdmissionReason::MemoryStatusUnavailable)
    reason = QStringLiteral("live physical-memory or Commit status unavailable");
  else
    reason = QStringLiteral("invalid or unsupported allocation size");

  return QStringLiteral(
    "Image decoding was stopped before entering the codec.\n%1\n\n"
    "Required peak increment: %2; usable physical budget: %3; "
    "usable Commit budget: %4; reason: %5.")
    .arg(fileName,
         imageMemoryAmount(admission.requiredBytes),
         admission.physicalMemoryChecked ?
           imageMemoryAmount(admission.availablePhysicalBudgetBytes) :
           QStringLiteral("unavailable"),
         admission.commitMemoryChecked ?
           imageMemoryAmount(admission.availableCommitBudgetBytes) :
           QStringLiteral("unavailable"),
         reason);
}

bool
allocateAdmittedImageBuffer(const QString &operation,
			    quint64 bufferBytes,
			    std::unique_ptr<uchar[]> &storage,
			    QString *error)
{
  quint64 requiredBytes = 0;
  if (bufferBytes == 0 ||
      bufferBytes > static_cast<quint64>(
	std::numeric_limits<size_t>::max()) ||
      !checkedImportAdd(bufferBytes, kImageDecodeSafetyBytes, requiredBytes))
    {
      *error = operation +
	QStringLiteral(" buffer size is invalid or exceeds the process address space.");
      return false;
    }

  const ImportMemoryAdmission admission =
    evaluateImportMemoryAdmission(requiredBytes);
  if (!admission.approved)
    {
      *error = QStringLiteral(
	"%1 was stopped before allocating memory. Required peak increment: %2; "
	"usable physical budget: %3; usable Commit budget: %4.")
	.arg(operation,
	     imageMemoryAmount(admission.requiredBytes),
	     admission.physicalMemoryChecked ?
	       imageMemoryAmount(admission.availablePhysicalBudgetBytes) :
	       QStringLiteral("unavailable"),
	     admission.commitMemoryChecked ?
	       imageMemoryAmount(admission.availableCommitBudgetBytes) :
	       QStringLiteral("unavailable"));
      return false;
    }

  storage.reset(new (std::nothrow) uchar[static_cast<size_t>(bufferBytes)]);
  if (!storage)
    {
      *error = operation + QStringLiteral(
	" could not allocate its admitted buffer. The system memory state changed.");
      return false;
    }
  return true;
}

bool
inspectAdmittedImage(QImageReader &reader,
		     const QString &fileName,
		     QSize *declaredSize,
		     QString *error)
{
  const QFileInfo fileInfo(fileName);
  if (!fileInfo.exists() || !fileInfo.isFile() || !fileInfo.isReadable())
    {
      *error = QString("Image file does not exist or is not readable:\n%1")
        .arg(fileInfo.absoluteFilePath());
      return false;
    }

  *declaredSize = reader.size();
  if (!declaredSize->isValid() || declaredSize->isEmpty())
    {
      *error = QString(
        "Cannot determine image dimensions without decoding, so the file was "
        "rejected before entering the codec:\n%1\n\n%2")
        .arg(fileName, reader.errorString());
      return false;
    }

  qint64 pixelCount = 0;
  qint64 argbByteCount = 0;
  if (!checkedImageSize(declaredSize->width(), declaredSize->height(), 4,
                        pixelCount, argbByteCount))
    {
      *error = QString("Image dimensions are too large for this importer: "
                       "%1 x %2\n%3")
        .arg(declaredSize->width()).arg(declaredSize->height()).arg(fileName);
      return false;
    }

  quint64 requiredBytes = 0;
  if (!checkedImportImageDecodeWorkingSet(
        static_cast<quint64>(pixelCount), kImageDecodeSafetyBytes,
        requiredBytes))
    {
      *error = QString("Image decode working-set calculation overflowed: %1")
        .arg(fileName);
      return false;
    }

  const ImportMemoryAdmission admission =
    evaluateImportMemoryAdmission(requiredBytes);
  if (!admission.approved)
    {
      *error = imageAdmissionError(fileName, admission);
      return false;
    }

  return true;
}

bool
readAdmittedImage(const QString &fileName, QImage *image, QString *error)
{
  QImageReader reader(fileName);
  reader.setAutoTransform(false);
  QSize declaredSize;
  if (!inspectAdmittedImage(reader, fileName, &declaredSize, error))
    return false;

  *image = reader.read();
  if (image->isNull())
    {
      *error = QString("Cannot load image:\n%1\n\n%2")
        .arg(fileName, reader.errorString());
      return false;
    }
  if (image->size() != declaredSize)
    {
      *error = QString("Decoded image dimensions differ from metadata. "
                       "Expected %1 x %2, found %3 x %4:\n%5")
        .arg(declaredSize.width()).arg(declaredSize.height())
        .arg(image->width()).arg(image->height()).arg(fileName);
      image->detach();
      *image = QImage();
      return false;
    }
  return true;
}

struct ImageValidationResult
{
  bool success;
  bool canceled;
  QString error;
  int width;
  int height;
  bool grayscale16;
  int rawMin;
  int rawMax;
  bool haveValue;
  QList<uint> histogram;

  ImageValidationResult()
    : success(false), canceled(false), width(0), height(0),
      grayscale16(false), rawMin(0), rawMax(0), haveValue(false)
  {
  }
};
}

void ImageStackPlugin::generateHistogram() {} // to satisfy the interface

QStringList
ImageStackPlugin::registerPlugin()
{
  QStringList regString;
  regString << "directory";
  regString << "Standard Image Directory";
  regString << "files";
  regString << "Standard Image Files";
  
  return regString;
}

void
ImageStackPlugin::init()
{
  m_fileName.clear();
  m_imageList.clear();

  m_description.clear();
  m_depth = m_width = m_height = 0;
  m_voxelType = _UChar;
  m_voxelUnit = _Micron;
  m_voxelSizeX = m_voxelSizeY = m_voxelSizeZ = 1;
  m_headerBytes = 0;
  m_bytesPerVoxel = 1;
  m_rawMin = m_rawMax = 0;
  m_histogram.clear();
  m_lastError.clear();
  m_lastOperationCanceled = false;
  m_4dvol = false;
}

void
ImageStackPlugin::clear()
{
  m_fileName.clear();
  m_imageList.clear();

  m_description.clear();
  m_depth = m_width = m_height = 0;
  m_voxelType = _UChar;
  m_voxelUnit = _Micron;
  m_voxelSizeX = m_voxelSizeY = m_voxelSizeZ = 1;
  m_headerBytes = 0;
  m_bytesPerVoxel = 1;
  m_rawMin = m_rawMax = 0;
  m_histogram.clear();
  m_lastError.clear();
  m_lastOperationCanceled = false;
  m_4dvol = false;
}

void
ImageStackPlugin::set4DVolume(bool flag)
{
  m_4dvol = flag;
}

void
ImageStackPlugin::voxelSize(float& vx, float& vy, float& vz)
  {
    vx = m_voxelSizeX;
    vy = m_voxelSizeY;
    vz = m_voxelSizeZ;
  }
QString ImageStackPlugin::description() { return m_description; }
int ImageStackPlugin::voxelType() { return m_voxelType; }
int ImageStackPlugin::voxelUnit() { return m_voxelUnit; }
int ImageStackPlugin::headerBytes() { return m_headerBytes; }

void
ImageStackPlugin::setMinMax(float rmin, float rmax)
{
  m_rawMin = rmin;
  m_rawMax = rmax;
}
float ImageStackPlugin::rawMin() { return m_rawMin; }
float ImageStackPlugin::rawMax() { return m_rawMax; }
QList<uint> ImageStackPlugin::histogram() { return m_histogram; }

void
ImageStackPlugin::gridSize(int& d, int& w, int& h)
{
  d = m_depth;
  w = m_width;
  h = m_height;
}

void
ImageStackPlugin::replaceFile(QString flnm)
{
  m_lastError.clear();
  m_lastOperationCanceled = false;
  if (flnm.isEmpty())
    {
      m_lastError = "The replacement image filename is empty.";
      return;
    }

  QImage decodedImage;
  QString imageError;
  if (!readAdmittedImage(flnm, &decodedImage, &imageError))
    {
      m_lastError = imageError;
      return;
    }

  const QSize declaredSize = decodedImage.size();

  if (declaredSize.width() != m_height ||
      declaredSize.height() != m_width)
    {
      m_lastError = QString(
        "Replacement image dimensions %1 x %2 do not match %3 x %4: %5")
        .arg(declaredSize.width()).arg(declaredSize.height())
        .arg(m_height).arg(m_width).arg(flnm);
      return;
    }

  const bool replacementIsGrayscale16 =
    decodedImage.format() == QImage::Format_Grayscale16;
  if ((m_voxelType == _UShort) != replacementIsGrayscale16)
    {
      m_lastError = QString(
	"Replacement image bit depth does not match the active image stack: %1")
	.arg(flnm);
      return;
    }

  m_fileName = QStringList() << flnm;
  m_imageList = m_fileName;
  m_depth = 1;
}

bool
ImageStackPlugin::setImageFiles(QStringList files)
{
  m_lastOperationCanceled = false;
  if (files.isEmpty())
    {
      m_lastError = "No image files were selected.";
      return false;
    }

  std::atomic_bool cancelRequested(false);
  std::atomic_int progressValue(0);

  QProgressDialog progress("Validating image stack",
			   "Cancel",
			   0, files.size(),
			   QApplication::activeWindow());
  progress.setWindowModality(Qt::ApplicationModal);
  progress.setMinimumDuration(0);
  progress.setAutoClose(false);
  progress.setAutoReset(false);

  QEventLoop waitLoop;
  QTimer progressTimer;
  QFutureWatcher<ImageValidationResult> watcher;

  QObject::connect(&progress, &QProgressDialog::canceled,
		   [&cancelRequested]() { cancelRequested.store(true); });
  QObject::connect(&watcher, &QFutureWatcher<ImageValidationResult>::finished,
		   &waitLoop, &QEventLoop::quit);
  QObject::connect(&progressTimer, &QTimer::timeout,
		   [&]()
		   {
		     int value = qMin(progressValue.load(), files.size());
		     progress.setValue(value);
		     progress.setLabelText(QString("Validating image %1 of %2")
					     .arg(value).arg(files.size()));
		   });

  QFuture<ImageValidationResult> future = QtConcurrent::run(
    [files, &cancelRequested, &progressValue]()
    {
      ImageValidationResult result;
      for (int i=0; i<files.size(); ++i)
	{
	  if (cancelRequested.load())
	    {
	      result.canceled = true;
	      result.error = "Image import canceled";
	      return result;
	    }

	  QImage decodedImage;
	  QString imageError;
	  if (!readAdmittedImage(files[i], &decodedImage, &imageError))
	    {
	      result.error = imageError;
	      return result;
	    }
	  const QSize declaredSize = decodedImage.size();
	  const bool decodedGrayscale16 =
	    decodedImage.format() == QImage::Format_Grayscale16;

	  if (i == 0)
	    {
	      result.width = declaredSize.width();
	      result.height = declaredSize.height();
	      result.grayscale16 = decodedGrayscale16;
	      const int histogramSize = result.grayscale16 ? 65536 : 256;
	      result.histogram.reserve(histogramSize);
	      for (int bin=0; bin<histogramSize; ++bin)
		result.histogram.append(0);
	    }
	  else if (declaredSize.width() != result.width ||
		   declaredSize.height() != result.height)
	    {
	      result.error = QString("All images in a stack must have the same dimensions.\n"
				     "Expected %1 x %2, found %3 x %4:\n%5")
			       .arg(result.width).arg(result.height)
			       .arg(declaredSize.width()).arg(declaredSize.height())
			       .arg(files[i]);
	      return result;
	    }
	  else if (decodedGrayscale16 != result.grayscale16)
	    {
	      result.error = QString(
		"An image stack cannot mix 16-bit grayscale and 8-bit/color images:\n%1")
		.arg(files[i]);
	      return result;
	    }

	  const uint maximumCount = std::numeric_limits<uint>::max();
	  if (result.grayscale16)
	    {
	      for (int row=0; row<decodedImage.height(); ++row)
		{
		  if (cancelRequested.load())
		    {
		      result.canceled = true;
		      result.error = "Image import canceled";
		      return result;
		    }
		  const quint16 *pixels = reinterpret_cast<const quint16*>(
		    decodedImage.constScanLine(row));
		  if (!pixels)
		    {
		      result.error = QString(
			"Cannot access a 16-bit grayscale scanline:\n%1")
			.arg(files[i]);
		      return result;
		    }
		  for (int column=0; column<decodedImage.width(); ++column)
		    {
		      const int value = pixels[column];
		      uint &count = result.histogram[value];
		      if (count < maximumCount) ++count;
		      if (!result.haveValue)
			{
			  result.rawMin = result.rawMax = value;
			  result.haveValue = true;
			}
		      else
			{
			  result.rawMin = qMin(result.rawMin, value);
			  result.rawMax = qMax(result.rawMax, value);
			}
		    }
		}
	    }
	  else
	    {
	      if (decodedImage.format() != QImage::Format_ARGB32)
		decodedImage = decodedImage.convertToFormat(QImage::Format_ARGB32);
	      if (decodedImage.isNull())
		{
		  result.error = QString("Cannot convert image to ARGB32:\n%1")
		    .arg(files[i]);
		  return result;
		}
	      for (int row=0; row<decodedImage.height(); ++row)
		{
		  if (cancelRequested.load())
		    {
		      result.canceled = true;
		      result.error = "Image import canceled";
		      return result;
		    }
		  const QRgb *pixels = reinterpret_cast<const QRgb*>(
		    decodedImage.constScanLine(row));
		  if (!pixels)
		    {
		      result.error = QString("Cannot access an image scanline:\n%1")
			.arg(files[i]);
		      return result;
		    }
		  for (int column=0; column<decodedImage.width(); ++column)
		    {
		      const int value = qRed(pixels[column]);
		      uint &count = result.histogram[value];
		      if (count < maximumCount) ++count;
		      if (!result.haveValue)
			{
			  result.rawMin = result.rawMax = value;
			  result.haveValue = true;
			}
		      else
			{
			  result.rawMin = qMin(result.rawMin, value);
			  result.rawMax = qMax(result.rawMax, value);
			}
		    }
		}
	    }

	  progressValue.fetch_add(1);
	}

      if (cancelRequested.load())
	{
	  result.canceled = true;
	  result.error = "Image import canceled";
	  return result;
	}
      if (!result.haveValue || result.histogram.isEmpty())
	{
	  result.error = "The image stack contains no readable pixels.";
	  return result;
	}
      result.success = true;
      return result;
    });

  watcher.setFuture(future);
  progressTimer.start(50);
  waitLoop.exec();
  progressTimer.stop();
  // QProgressDialog::close() emits canceled(), so closing the dialog here can
  // misreport a successful validation as a user cancellation.
  progress.hide();

  ImageValidationResult validation = watcher.result();
  if (validation.success && cancelRequested.load())
    {
      m_lastError = "Image import canceled";
      m_lastOperationCanceled = true;
      return false;
    }
  if (!validation.success)
    {
      m_lastError = validation.error;
      m_lastOperationCanceled = validation.canceled;
      return false;
    }

  int imageWidth = validation.width;
  int imageHeight = validation.height;

  //--------------
  int voxelType = validation.grayscale16 ? _UShort : _UChar;
  int bytesPerVoxel = validation.grayscale16 ? 2 : 1;
  if (!validation.grayscale16)
    {
      QStringList dtypes;
      dtypes << "Grayscale Images"
	     << "RGB Images"
	     << "RGBA Images";
      bool ok = false;
      QString option = QInputDialog::getItem(0,
					     "Select Image Type",
					     "Image Type",
					     dtypes,
					     0,
					     false,
					     &ok);
      if (!ok)
	{
	  m_lastError = "Image import canceled";
	  m_lastOperationCanceled = true;
	  return false;
	}

      if (option == "RGB Images")
	{
	  voxelType = _Rgb;
	  bytesPerVoxel = 3;
	}
      else if (option == "RGBA Images")
	{
	  voxelType = _Rgba;
	  bytesPerVoxel = 4;
	}
    }
  //--------------


  qint64 pixelCount = 0;
  qint64 byteCount = 0;
  if (!checkedImageSize(imageWidth, imageHeight, 4,
			pixelCount, byteCount))
    {
      m_lastError = QString(
        "Image dimensions are too large for this importer: %1 x %2")
        .arg(imageWidth).arg(imageHeight);
      return false;
    }


  m_imageList = files;
  m_depth = m_imageList.size();
  m_height = imageWidth;
  m_width = imageHeight;
  m_voxelType = voxelType;
  m_bytesPerVoxel = bytesPerVoxel;
  m_headerBytes = 0;


  m_histogram = validation.histogram;
  m_rawMin = validation.rawMin;
  m_rawMax = validation.rawMax;

  return true;
}

bool
ImageStackPlugin::setFile(QStringList files)
{
  m_lastError.clear();
  m_lastOperationCanceled = false;
  if (files.size() == 0)
    {
      m_lastError = "No image files were selected.";
      return false;
    }

  QFileInfo f(files[0]);
  if (f.isDir())
    {
      // list all image files in the directory
      const QStringList imageNameFilter =
	TiffInputRouting::standardImageNameFilters();
      QStringList imgfiles= QDir(files[0]).entryList(imageNameFilter,
							  QDir::NoSymLinks|
							  QDir::NoDotAndDotDot|
							  QDir::Readable|
							  QDir::Files,
							  QDir::NoSort);
      QCollator collator;
      collator.setCaseSensitivity(Qt::CaseInsensitive);
      collator.setNumericMode(true);
      std::sort(imgfiles.begin(), imgfiles.end(),
		[&collator](const QString &left, const QString &right)
		{
		  const int comparison = collator.compare(left, right);
		  return comparison == 0 ? left < right : comparison < 0;
		});
      QStringList imageList;
      for(int i=0; i<imgfiles.size(); i++)
	{
	  QFileInfo fileInfo(files[0], imgfiles[i]);
	  QString imgfl = fileInfo.absoluteFilePath();
	  imageList.append(imgfl);
	}      
      if (imageList.size() == 0)
	{
	  m_lastError = QString("No supported image files were found in %1")
	    .arg(QFileInfo(files[0]).absoluteFilePath());
	  return false;
	}

      if (!setImageFiles(imageList))
	return false;
      m_fileName = files;
      return true;
    }

  if (!setImageFiles(files))
    return false;
  m_fileName = files;
  return true;
}

void
ImageStackPlugin::getDepthSlice(int slc,
				uchar *slice)
{
  m_lastError.clear();
  if (!slice || slc < 0 || slc >= m_depth || slc >= m_imageList.size())
    {
      m_lastError = QString("Image-stack slice %1 is invalid.").arg(slc);
      return;
    }

  const int outputBytesPerPixel =
    ImageStackPixelConversion::bytesPerPixel(m_voxelType);

  if (outputBytesPerPixel == 0)
    {
      m_lastError = "The image-stack voxel type is unsupported.";
      return;
    }

  qint64 pixelCount = 0;
  qint64 argbByteCount = 0;
  if (!checkedImageSize(m_height, m_width, 4,
			pixelCount, argbByteCount))
    {
      m_lastError = "The image-stack slice size is unsupported.";
      return;
    }

  const qint64 outputByteCount = pixelCount*outputBytesPerPixel;
  std::memset(slice, 0, static_cast<size_t>(outputByteCount));

  QImage imgL;
  QString imageError;
  if (!readAdmittedImage(m_imageList[slc], &imgL, &imageError) ||
      imgL.width() != m_height || imgL.height() != m_width)
    {
      m_lastError = imageError.isEmpty() ?
	QString("Decoded image dimensions do not match the stack at slice %1.")
	.arg(slc) : imageError;
      qWarning() << "Cannot load image slice:" << imageError;
      return;
    }

  if (m_voxelType == _UShort)
    {
      if (imgL.format() != QImage::Format_Grayscale16)
	{
	  m_lastError = "A 16-bit grayscale stack contains a non-16-bit slice.";
	  return;
	}

      quint16 *output = reinterpret_cast<quint16*>(slice);
      for (int row=0; row<imgL.height(); ++row)
	{
	  const quint16 *source =
	    reinterpret_cast<const quint16*>(imgL.constScanLine(row));
	  if (!source)
	    {
	      std::memset(slice, 0, static_cast<size_t>(outputByteCount));
	      m_lastError = "Cannot access a 16-bit grayscale scanline.";
	      return;
	    }
	  std::memcpy(output+static_cast<qint64>(row)*imgL.width(), source,
		      static_cast<std::size_t>(imgL.width())*sizeof(quint16));
	}
      return;
    }

  if (imgL.format() != QImage::Format_ARGB32)
    imgL = imgL.convertToFormat(QImage::Format_ARGB32);

  if (imgL.isNull())
    {
      m_lastError = "Cannot convert the decoded image to ARGB32.";
      return;
    }

  const QRgb *pixels = reinterpret_cast<const QRgb*>(imgL.constBits());
  if (!pixels)
    {
      m_lastError = "Cannot access the decoded image pixels.";
      return;
    }

  if (m_voxelType == _UChar)
    {
      for(qint64 j=0; j<pixelCount; j++)
	slice[j] = static_cast<uchar>(qRed(pixels[j]));
    }
  else if (!ImageStackPixelConversion::packColorPixels(
             pixels, pixelCount, m_voxelType, slice))
    {
      std::memset(slice, 0, static_cast<size_t>(outputByteCount));
      m_lastError = "Cannot pack the decoded RGB/RGBA pixels.";
    }

}

QString ImageStackPlugin::lastError() const { return m_lastError; }
bool ImageStackPlugin::wasCanceled() const
{
  return m_lastOperationCanceled;
}

//void
//ImageStackPlugin::getWidthSlice(int slc,
//				uchar *slice)
//{
//  for(uint i=0; i<m_depth; i++)
//    {
//      QImage imgL = QImage(m_imageList[i]);
//      if (imgL.format() != QImage::Format_ARGB32)
//	imgL = imgL.convertToFormat(QImage::Format_ARGB32);
//
//      uchar *imgbits = imgL.bits();
//      
//      if (m_voxelType == _UChar)
//	{
//	  for(uint j=0; j<m_height; j++)
//	    slice[i*m_height+j] = imgbits[4*(slc*m_height+j)];
//	}      
//      else
//	{
//	  for(uint j=0; j<m_height; j++)
//	    {
//	      slice[4*(i*m_height+j)+0] = imgbits[4*(slc*m_height+j)+0];
//	      slice[4*(i*m_height+j)+1] = imgbits[4*(slc*m_height+j)+1];
//	      slice[4*(i*m_height+j)+2] = imgbits[4*(slc*m_height+j)+2];
//	      slice[4*(i*m_height+j)+3] = imgbits[4*(slc*m_height+j)+3];
//	    }
//	}
//    }
//}
//
//void
//ImageStackPlugin::getHeightSlice(int slc,
//				 uchar *slice)
//{
//  for(uint i=0; i<m_depth; i++)
//    {
//      QImage imgL = QImage(m_imageList[i]);
//      if (imgL.format() != QImage::Format_ARGB32)
//	imgL = imgL.convertToFormat(QImage::Format_ARGB32);
//
//      uchar *imgbits = imgL.bits();
//      if (m_voxelType == _UChar)
//	{
//	  for(uint j=0; j<m_width; j++)
//	    slice[i*m_width+j] = imgbits[4*(j*m_height+slc)];
//	}
//      else
//	{
//	  for(uint j=0; j<m_width; j++)
//	    {
//	      slice[4*(i*m_width+j)+0] = imgbits[4*(j*m_height+slc)+0];
//	      slice[4*(i*m_width+j)+1] = imgbits[4*(j*m_height+slc)+1];
//	      slice[4*(i*m_width+j)+2] = imgbits[4*(j*m_height+slc)+2];
//	      slice[4*(i*m_width+j)+3] = imgbits[4*(j*m_height+slc)+3];
//	    }
//	}
//    }
//}

QVariant
ImageStackPlugin::rawValue(int d, int w, int h)
{
  if (d < 0 || d >= m_depth || d >= m_imageList.size() ||
      w < 0 || w >= m_width ||
      h < 0 || h >= m_height)
    return QVariant("OutOfBounds");

  if (m_voxelType != _UChar && m_voxelType != _UShort &&
      m_voxelType != _Rgb && m_voxelType != _Rgba)
    return QVariant("OutOfBounds");

  qint64 pixelCount = 0;
  qint64 byteCount = 0;
  if (!checkedImageSize(m_height, m_width, 4,
			pixelCount, byteCount))
    return QVariant("OutOfBounds");

  QImage imgL;
  QString imageError;
  if (!readAdmittedImage(m_imageList[d], &imgL, &imageError) ||
      imgL.width() != m_height || imgL.height() != m_width)
    {
      qWarning() << "Cannot load image value:" << imageError;
      return QVariant("OutOfBounds");
    }

  if (m_voxelType == _UShort)
    {
      if (imgL.format() != QImage::Format_Grayscale16)
	return QVariant("OutOfBounds");
      const quint16 *scanline =
	reinterpret_cast<const quint16*>(imgL.constScanLine(w));
      return scanline ? QVariant(static_cast<uint>(scanline[h])) :
	QVariant("OutOfBounds");
    }

  if (imgL.format() != QImage::Format_ARGB32)
	imgL = imgL.convertToFormat(QImage::Format_ARGB32);

  if (imgL.isNull())
    return QVariant("OutOfBounds");

  const QRgb *scanline =
    reinterpret_cast<const QRgb*>(imgL.constScanLine(w));
  if (!scanline)
    return QVariant("OutOfBounds");

  const QRgb pixel = scanline[h];

  if (m_voxelType == _Rgb || m_voxelType == _Rgba)
    {
      const int r = qRed(pixel);
      const int g = qGreen(pixel);
      const int b = qBlue(pixel);
      const int a = qAlpha(pixel);
      
      return QVariant(QString(" (%1 %2 %3 %4)").\
			      arg(r).arg(g).arg(b).arg(a));
    }

  return QVariant(static_cast<uint>(qRed(pixel)));
}

void
ImageStackPlugin::saveTrimmed(QString trimFile,
			      int dmin, int dmax,
			      int wmin, int wmax,
			      int hmin, int hmax)
{
  m_lastError.clear();
  m_lastOperationCanceled = false;
  if (dmin < 0 || dmax < dmin || dmax >= m_depth ||
      wmin < 0 || wmax < wmin || wmax >= m_width ||
      hmin < 0 || hmax < hmin || hmax >= m_height)
    {
      m_lastError = "The selected image-stack range is invalid.";
      QMessageBox::critical(0, "Save Trimmed Volume",
			    m_lastError);
      return;
    }

  if (m_voxelType == _Rgb || m_voxelType == _Rgba)
    {
      saveTrimmedRGB(trimFile,
		     dmin, dmax,
		     wmin, wmax,
		     hmin, hmax);
      return;
    }

  QProgressDialog progress("Saving trimmed volume",
			   "Cancel",
			   0, 100,
			   0);
  progress.setMinimumDuration(0);

  int nX, nY, nZ;
  nX = m_depth;
  nY = m_width;
  nZ = m_height;

  int mX, mY, mZ;
  mX = dmax-dmin+1;
  mY = wmax-wmin+1;
  mZ = hmax-hmin+1;

  int bpv = m_bytesPerVoxel;
  if ((m_voxelType != _UChar && m_voxelType != _UShort) ||
      (m_voxelType == _UChar && bpv != 1) ||
      (m_voxelType == _UShort && bpv != 2))
    {
      m_lastError = "The scalar image-stack voxel type is unsupported.";
      QMessageBox::critical(0, "Save Trimmed Volume",
			    m_lastError);
      return;
    }
  qint64 pixelCount = 0;
  qint64 byteCount = 0;
  qint64 outputPixelCount = 0;
  qint64 outputByteCount = 0;
  if (!checkedImageSize(nY, nZ, bpv, pixelCount, byteCount))
    {
      m_lastError = "The source slice size is unsupported.";
      QMessageBox::critical(0, "Save Trimmed Volume",
			    m_lastError);
      return;
    }
  if (!checkedImageSize(mY, mZ, bpv,
			outputPixelCount, outputByteCount))
    {
      m_lastError = "The output slice size is unsupported.";
      QMessageBox::critical(0, "Save Trimmed Volume",
			    m_lastError);
      return;
    }

  std::unique_ptr<uchar[]> tmpStorage;
  QString bufferError;
  if (!allocateAdmittedImageBuffer(
	QStringLiteral("Trimmed image-stack export"),
	static_cast<quint64>(byteCount), tmpStorage, &bufferError))
    {
      m_lastError = bufferError;
      QMessageBox::critical(0, "Save Trimmed Volume", m_lastError);
      return;
    }
  uchar *tmp = tmpStorage.get();

  const uchar vt = m_voxelType == _UShort ? 2 : 0;

  QSaveFile fout(trimFile);
  if (!fout.open(QFile::WriteOnly))
    {
      m_lastError = QString("Cannot open the output file for writing: %1")
	.arg(fout.errorString());
      QMessageBox::critical(0, "Save Trimmed Volume",
			    m_lastError);
      return;
    }

  if (fout.write((char*)&vt, 1) != 1 ||
      fout.write((char*)&mX, 4) != 4 ||
      fout.write((char*)&mY, 4) != 4 ||
      fout.write((char*)&mZ, 4) != 4)
    {
      fout.cancelWriting();
      m_lastError = QString("Cannot write the output header: %1")
	.arg(fout.errorString());
      QMessageBox::critical(0, "Save Trimmed Volume", m_lastError);
      return;
    }

  //for(uint i=dmin; i<=dmax; i++)
  for(int i=dmax; i>=dmin; i--)
    {
      if (progress.wasCanceled())
	{
	  fout.cancelWriting();
          m_lastError = "Image export canceled";
          m_lastOperationCanceled = true;
	  QMessageBox::information(0, "Save Trimmed Volume",
				   "-----Aborted-----");
	  return;
	}

      QImage imgL;
      QString imageError;
      if (!readAdmittedImage(m_imageList[i], &imgL, &imageError))
	{
	  fout.cancelWriting();
	  m_lastError = imageError;
	  QMessageBox::critical(0, "Image Load Error", m_lastError);
	  return;
	}
      if (imgL.width() != nZ || imgL.height() != nY)
	{
	  fout.cancelWriting();
          m_lastError = "Decoded image dimensions no longer match the stack.";
	  QMessageBox::critical(0, "Image Load Error",
				m_lastError);
	  return;
	}

      if (m_voxelType == _UShort)
	{
	  if (imgL.format() != QImage::Format_Grayscale16)
	    {
	      fout.cancelWriting();
              m_lastError =
                "A 16-bit image stack contains a non-16-bit slice.";
	      QMessageBox::critical(0, "Image Load Error",
				    m_lastError);
	      return;
	    }

	  for(int row=0; row<nY; ++row)
	    {
	      const uchar *source = imgL.constScanLine(row);
	      if (!source)
		{
		  fout.cancelWriting();
                  m_lastError = "Cannot access a 16-bit image scanline.";
		  QMessageBox::critical(0, "Image Load Error",
					m_lastError);
		  return;
		}
	      std::memcpy(tmp+static_cast<qint64>(row)*nZ*bpv,
			  source, static_cast<std::size_t>(nZ)*bpv);
	    }
	}
      else
	{
	  if (imgL.format() != QImage::Format_ARGB32)
	    imgL = imgL.convertToFormat(QImage::Format_ARGB32);

	  if (imgL.isNull())
	    {
	      fout.cancelWriting();
              m_lastError = "Cannot convert the image to ARGB32.";
	      QMessageBox::critical(0, "Image Load Error",
				    m_lastError);
	      return;
	    }
	  for(int row=0; row<nY; ++row)
	    {
	      const QRgb *pixels =
		reinterpret_cast<const QRgb*>(imgL.constScanLine(row));
	      if (!pixels)
		{
		  fout.cancelWriting();
                  m_lastError = "Cannot access a decoded image scanline.";
		  QMessageBox::critical(0, "Image Load Error",
					m_lastError);
		  return;
		}
	      for(int column=0; column<nZ; ++column)
		tmp[static_cast<qint64>(row)*nZ+column] =
		  static_cast<uchar>(qRed(pixels[column]));
	    }
	}

      for(int j=wmin; j<=wmax; j++)
	{
	  const qint64 destinationOffset =
	    static_cast<qint64>(j-wmin)*mZ*bpv;
	  const qint64 sourceOffset =
	    (static_cast<qint64>(j)*nZ+hmin)*bpv;
	  std::memmove(tmp+destinationOffset, tmp+sourceOffset,
		       static_cast<std::size_t>(mZ)*bpv);
	}
      
      if (fout.write((char*)tmp, outputByteCount) != outputByteCount)
	{
	  fout.cancelWriting();
	  m_lastError = QString("Cannot write an output slice: %1")
	    .arg(fout.errorString());
	  QMessageBox::critical(0, "Save Trimmed Volume", m_lastError);
	  return;
	}
      
      progress.setValue(static_cast<int>(
	100LL*static_cast<qint64>(dmax-i)/mX));
      qApp->processEvents();
    }

  if (progress.wasCanceled())
    {
      fout.cancelWriting();
      m_lastError = "Image export canceled";
      m_lastOperationCanceled = true;
      QMessageBox::information(0, "Save Trimmed Volume", "-----Aborted-----");
      return;
    }

  const qint64 expectedSize = 13 + static_cast<qint64>(mX)*outputByteCount;
  if (fout.size() != expectedSize || !fout.commit())
    {
      m_lastError = QString("Cannot commit the output file: %1")
	.arg(fout.errorString());
      QMessageBox::critical(0, "Save Trimmed Volume", m_lastError);
      return;
    }

  m_headerBytes = 13; // to be used for applyMapping function
  progress.setValue(100);
}

bool
ImageStackPlugin::savePvlHeader(QString pvlFilename,
				int d, int w, int h,
				QString voxelType,
				int slabSize,
				QString &error)
{
  QString xmlfile = pvlFilename;
  error.clear();

  QDomDocument doc("Drishti_Header");

  QDomElement topElement = doc.createElement("PvlDotNcFileHeader");
  doc.appendChild(topElement);

  {      
    QDomElement de0 = doc.createElement("gridsize");
    QDomText tn0;
    tn0 = doc.createTextNode(QString("%1 %2 %3").arg(d).arg(w).arg(h));
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }

  {      
    QString vstr = "no units";    
    QDomElement de0 = doc.createElement("voxelunit");
    QDomText tn0;
    tn0 = doc.createTextNode(QString("%1").arg(vstr));
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }

  {      
    QString vstr = voxelType;    
    QDomElement de0 = doc.createElement("voxeltype");
    QDomText tn0;
    tn0 = doc.createTextNode(voxelType);
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }

  {      
    QDomElement de0 = doc.createElement("voxelsize");
    QDomText tn0;
    tn0 = doc.createTextNode("1 1 1");
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }
  
  {
    QDomElement de0 = doc.createElement("description");
    QDomText tn0;
    tn0 = doc.createTextNode("Colour volume");
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }

  {      
    QDomElement de0 = doc.createElement("slabsize");
    QDomText tn0;
    tn0 = doc.createTextNode(QString("%1").arg(slabSize));
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }  
  
  QSaveFile f(xmlfile);
  if (!f.open(QIODevice::WriteOnly))
    {
      error = QString("Cannot open PVL header '%1': %2")
	.arg(xmlfile).arg(f.errorString());
      return false;
    }
  QTextStream out(&f);
  doc.save(out, 2);
  out.flush();
  if (out.status() != QTextStream::Ok)
    {
      error = QString("Cannot write PVL header '%1': %2")
	.arg(xmlfile).arg(f.errorString());
      f.cancelWriting();
      return false;
    }
  if (!f.commit())
    {
      error = QString("Cannot commit PVL header '%1': %2")
	.arg(xmlfile).arg(f.errorString());
      return false;
    }
  return true;
}

void
ImageStackPlugin::saveTrimmedRGB(QString trimFile,
				 int dmin, int dmax,
				 int wmin, int wmax,
				 int hmin, int hmax)
{
  QStringList dtypes;
  dtypes << "No"
	 << "Yes";

  bool accepted = false;
  QString option = QInputDialog::getItem(0,
					 "Save Alpha Channel",
					 "Alpha Channel",
					 dtypes,
					 0,
					 false,
					 &accepted);
  if (!accepted)
    {
      m_lastError = "Image export canceled";
      m_lastOperationCanceled = true;
      return;
    }
  
  bool saveAlpha = false;
  if (option == "Yes")
    saveAlpha = true;


  QProgressDialog progress("Saving trimmed RGB volume",
			   "Cancel",
			   0, 100,
			   0);
  progress.setMinimumDuration(0);

  int nX, nY, nZ;
  nX = m_depth;
  nY = m_width;
  nZ = m_height;

  int d, w, h;
  d = dmax-dmin+1;
  w = wmax-wmin+1;
  h = hmax-hmin+1;

  qint64 pixelCount = 0;
  qint64 channelBytes = 0;
  const int channelCount = saveAlpha ? 4 : 3;
  if (!checkedImageSize(nY, nZ, channelCount,
			pixelCount, channelBytes))
    {
      m_lastError = "The source slice size is unsupported.";
      QMessageBox::critical(0, "Save Trimmed RGB Volume",
			    m_lastError);
      return;
    }

  std::unique_ptr<uchar[]> channelStorage;
  QString bufferError;
  if (!allocateAdmittedImageBuffer(
	QStringLiteral("Trimmed RGB image-stack export"),
	static_cast<quint64>(channelBytes), channelStorage, &bufferError))
    {
      m_lastError = bufferError;
      QMessageBox::critical(0, "Save Trimmed RGB Volume", m_lastError);
      return;
    }

  uchar *tmpR = channelStorage.get();
  uchar *tmpG = tmpR + pixelCount;
  uchar *tmpB = tmpG + pixelCount;
  uchar *tmpA = saveAlpha ? tmpB + pixelCount : 0;


  QString voxelType = "RGB";
  if (saveAlpha) voxelType = "RGBA";
  
  //*** max 1Gb per slab
  int slabSize;
  const qint64 outputPixels = static_cast<qint64>(w)*h;
  slabSize = qMax(1, static_cast<int>((1024LL*1024LL*1024LL)/outputPixels));

  VolumeFileManager rFileManager;
  VolumeFileManager gFileManager;
  VolumeFileManager bFileManager;
  VolumeFileManager aFileManager;

  QString pvlfile = trimFile;
  pvlfile.chop(6);

  QString rFilename = pvlfile + QString("red");
  QString gFilename = pvlfile + QString("green");
  QString bFilename = pvlfile + QString("blue");
  QString aFilename = pvlfile + QString("alpha");

  rFileManager.setBaseFilename(rFilename);
  rFileManager.setDepth(d);
  rFileManager.setWidth(w);
  rFileManager.setHeight(h);
  rFileManager.setVoxelType(0);
  rFileManager.setHeaderSize(13);
  rFileManager.setSlabSize(slabSize);
  if (!rFileManager.createFile(true))
    {
      m_lastError = rFileManager.lastError();
      m_lastOperationCanceled =
	m_lastError.contains("canceled", Qt::CaseInsensitive);
      QMessageBox::critical(0, "Save Trimmed RGB Volume", m_lastError);
      return;
    }

  gFileManager.setBaseFilename(gFilename);
  gFileManager.setDepth(d);
  gFileManager.setWidth(w);
  gFileManager.setHeight(h);
  gFileManager.setVoxelType(0);
  gFileManager.setHeaderSize(13);
  gFileManager.setSlabSize(slabSize);
  if (!gFileManager.createFile(true))
    {
      m_lastError = gFileManager.lastError();
      m_lastOperationCanceled =
	m_lastError.contains("canceled", Qt::CaseInsensitive);
      QMessageBox::critical(0, "Save Trimmed RGB Volume", m_lastError);
      return;
    }

  bFileManager.setBaseFilename(bFilename);
  bFileManager.setDepth(d);
  bFileManager.setWidth(w);
  bFileManager.setHeight(h);
  bFileManager.setVoxelType(0);
  bFileManager.setHeaderSize(13);
  bFileManager.setSlabSize(slabSize);
  if (!bFileManager.createFile(true))
    {
      m_lastError = bFileManager.lastError();
      m_lastOperationCanceled =
	m_lastError.contains("canceled", Qt::CaseInsensitive);
      QMessageBox::critical(0, "Save Trimmed RGB Volume", m_lastError);
      return;
    }

  if (saveAlpha)
    {
      aFileManager.setBaseFilename(aFilename);
      aFileManager.setDepth(d);
      aFileManager.setWidth(w);
      aFileManager.setHeight(h);
      aFileManager.setVoxelType(0);
      aFileManager.setHeaderSize(13);
      aFileManager.setSlabSize(slabSize);
      if (!aFileManager.createFile(true))
	{
	  m_lastError = aFileManager.lastError();
	  m_lastOperationCanceled =
	    m_lastError.contains("canceled", Qt::CaseInsensitive);
	  QMessageBox::critical(0, "Save Trimmed RGB Volume", m_lastError);
	  return;
	}
    }


  for(int i=dmax; i>=dmin; i--)
    {
      if (progress.wasCanceled())
	{
          m_lastError = "Image export canceled";
          m_lastOperationCanceled = true;
	  QMessageBox::information(0, "Save Trimmed RGB Volume",
				   "-----Aborted-----");
	  return;
	}

      QImage imgL;
      QString imageError;
      if (!readAdmittedImage(m_imageList[i], &imgL, &imageError))
	{
	  m_lastError = imageError;
	  QMessageBox::critical(0, "Image Load Error", m_lastError);
	  return;
	}
      if (imgL.width() != nZ || imgL.height() != nY)
	{
          m_lastError = "Decoded image dimensions no longer match the stack.";
	  QMessageBox::critical(0, "Image Load Error",
				m_lastError);
	  return;
	}
      if (imgL.format() != QImage::Format_ARGB32)
	imgL = imgL.convertToFormat(QImage::Format_ARGB32);

      if (imgL.isNull())
	{
          m_lastError = "Cannot convert the image to ARGB32.";
	  QMessageBox::critical(0, "Image Load Error",
				m_lastError);
	  return;
	}

      const QRgb *pixels = reinterpret_cast<const QRgb*>(imgL.constBits());
      if (!pixels)
	{
          m_lastError = "Cannot access the decoded image pixels.";
	  QMessageBox::critical(0, "Image Load Error",
				m_lastError);
	  return;
	}
      for(qint64 j=0; j<pixelCount; j++)
	{
	  tmpR[j] = static_cast<uchar>(qRed(pixels[j]));
	  tmpG[j] = static_cast<uchar>(qGreen(pixels[j]));
	  tmpB[j] = static_cast<uchar>(qBlue(pixels[j]));
	}

      if (saveAlpha)
	{
	  for(qint64 j=0; j<pixelCount; j++)
	    tmpA[j] = static_cast<uchar>(qAlpha(pixels[j]));
	}

      for(int j=wmin; j<=wmax; j++)
	std::memmove(tmpR+static_cast<qint64>(j-wmin)*h,
		     tmpR+static_cast<qint64>(j)*nZ+hmin,
		     static_cast<std::size_t>(h));

      for(int j=wmin; j<=wmax; j++)
	std::memmove(tmpG+static_cast<qint64>(j-wmin)*h,
		     tmpG+static_cast<qint64>(j)*nZ+hmin,
		     static_cast<std::size_t>(h));

      for(int j=wmin; j<=wmax; j++)
	std::memmove(tmpB+static_cast<qint64>(j-wmin)*h,
		     tmpB+static_cast<qint64>(j)*nZ+hmin,
		     static_cast<std::size_t>(h));

      if (saveAlpha)
	{
	  for(int j=wmin; j<=wmax; j++)
	    std::memmove(tmpA+static_cast<qint64>(j-wmin)*h,
			 tmpA+static_cast<qint64>(j)*nZ+hmin,
			 static_cast<std::size_t>(h));
	}

      if (!rFileManager.setSlice(dmax-i, tmpR) ||
	  !gFileManager.setSlice(dmax-i, tmpG) ||
	  !bFileManager.setSlice(dmax-i, tmpB) ||
	  (saveAlpha && !aFileManager.setSlice(dmax-i, tmpA)))
	{
	  QString error = rFileManager.lastError();
	  if (error.isEmpty()) error = gFileManager.lastError();
	  if (error.isEmpty()) error = bFileManager.lastError();
	  if (error.isEmpty()) error = aFileManager.lastError();
	  m_lastError = error;
	  QMessageBox::critical(0, "Save Trimmed RGB Volume", m_lastError);
	  return;
	}

      progress.setValue((int)(100*(float)(dmax-i)/(float)d));
      qApp->processEvents();
    }

  if (progress.wasCanceled())
    {
      m_lastError = "Image export canceled";
      m_lastOperationCanceled = true;
      QMessageBox::information(0, "Save Trimmed RGB Volume",
			       "-----Aborted-----");
      return;
    }

  QString headerError;
  if (!savePvlHeader(trimFile,
		     d, w, h,
		     voxelType,
		     slabSize,
		     headerError))
    {
      m_lastError = headerError;
      QMessageBox::critical(0, "Save Trimmed RGB Volume", m_lastError);
      return;
    }
  const bool rCommitted = rFileManager.commitFileCreation();
  const bool gCommitted = gFileManager.commitFileCreation();
  const bool bCommitted = bFileManager.commitFileCreation();
  const bool aCommitted = !saveAlpha || aFileManager.commitFileCreation();
  if (!rCommitted || !gCommitted || !bCommitted || !aCommitted)
    {
      QString error = rFileManager.lastError();
      if (error.isEmpty()) error = gFileManager.lastError();
      if (error.isEmpty()) error = bFileManager.lastError();
      if (error.isEmpty()) error = aFileManager.lastError();
      m_lastError = error;
      QMessageBox::critical(0, "Save Trimmed RGB Volume", m_lastError);
      return;
    }

  progress.setValue(100);
}
