#include <QtGui>
#include "common.h"
#include "importmemoryadmission.h"
#include "txmplugin.h"

#include <QCollator>

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>

namespace
{
const std::uint64_t kTxmDecodeSafetyBytes = 32ULL*1024ULL*1024ULL;

bool admitTxmDecode(std::uint64_t sliceBytes, QString *error)
{
  std::uint64_t decodedBytes = 0;
  std::uint64_t requiredBytes = 0;
  if (!checkedImportMultiply(sliceBytes, 2, decodedBytes) ||
      !checkedImportAdd(decodedBytes, kTxmDecodeSafetyBytes, requiredBytes))
    {
      *error = "TXM decode working-set calculation overflowed.";
      return false;
    }
  const ImportMemoryAdmission admission =
    evaluateImportMemoryAdmission(requiredBytes);
  if (admission.approved)
    return true;
  *error = QString("TXM decoding was stopped before pixel allocation. "
                   "Required peak increment: %1 MiB; usable physical "
                   "budget: %2 MiB.")
             .arg(requiredBytes/(1024.0*1024.0), 0, 'f', 1)
             .arg(admission.availablePhysicalBudgetBytes/(1024.0*1024.0),
                  0, 'f', 1);
  return false;
}

template <class T>
bool readTxmValue(POLE::Storage *storage, const char *name, T *value)
{
  POLE::Stream stream(storage, name);
  return stream.size() >= sizeof(T) &&
         stream.read(reinterpret_cast<uchar*>(value), sizeof(T)) == sizeof(T);
}

void collectTxmImages(POLE::Storage *storage, const std::string &path,
                      QStringList *images, int nesting=0)
{
  if (nesting > 64)
    return;
  const std::list<std::string> entries = storage->entries(path);
  for (std::list<std::string>::const_iterator it=entries.begin();
       it != entries.end(); ++it)
    {
      const std::string fullName = path+*it;
      if (storage->isDirectory(fullName))
        {
          const QString name = QString::fromUtf8(it->c_str());
          if (name.startsWith("ImageData", Qt::CaseInsensitive))
            collectTxmImages(storage, fullName+"/", images, nesting+1);
        }
      else
        {
          const QString name = QString::fromUtf8(it->c_str());
          const QString suffix = name.mid(5);
          bool numericSuffix = name.startsWith("Image", Qt::CaseInsensitive) &&
                               !suffix.isEmpty();
          for (const QChar character : suffix)
            if (character < QLatin1Char('0') ||
                character > QLatin1Char('9'))
              {
                numericSuffix = false;
                break;
              }
          if (numericSuffix)
            images->append(QString::fromUtf8(fullName.c_str()));
        }
    }
}
}

TxmPlugin::~TxmPlugin()
{
  clear();
}

QStringList
TxmPlugin::registerPlugin()
{
  QStringList regString;
  regString << "files";
  regString << "TXM Files";
  
  return regString;
}

void
TxmPlugin::init()
{
  m_storage = 0;

  m_fileName.clear();

  m_description.clear();
  m_depth = m_width = m_height = 0;
  m_voxelType = _UChar;
  m_voxelUnit = _Micron;
  m_voxelSizeX = m_voxelSizeY = m_voxelSizeZ = 1;
  m_bytesPerVoxel = 1;
  m_rawMin = m_rawMax = 0;
  m_histogram.clear();
  m_4dvol = false;
  m_headerBytes = 0;
  m_lastError.clear();
  m_lastOperationCanceled = false;

  m_imageData.clear();
}

void
TxmPlugin::clear()
{
  if (m_storage)
    {
      m_storage->close();
      delete m_storage;
    }
  m_storage = 0;

  m_fileName.clear();

  m_description.clear();
  m_depth = m_width = m_height = 0;
  m_voxelType = _UChar;
  m_voxelUnit = _Micron;
  m_voxelSizeX = m_voxelSizeY = m_voxelSizeZ = 1;
  m_bytesPerVoxel = 1;
  m_rawMin = m_rawMax = 0;
  m_histogram.clear();
  m_4dvol = false;
  m_headerBytes = 0;
  m_lastError.clear();
  m_lastOperationCanceled = false;

  m_imageData.clear();
}

void
TxmPlugin::set4DVolume(bool flag)
{
  m_4dvol = flag;
}

void
TxmPlugin::voxelSize(float& vx, float& vy, float& vz)
  {
    vx = m_voxelSizeX;
    vy = m_voxelSizeY;
    vz = m_voxelSizeZ;
  }
QString TxmPlugin::description() { return m_description; }
int TxmPlugin::voxelType() { return m_voxelType; }
int TxmPlugin::voxelUnit() { return m_voxelUnit; }
int TxmPlugin::headerBytes() { return m_headerBytes; }
QString TxmPlugin::lastError() const { return m_lastError; }
bool TxmPlugin::wasCanceled() const { return m_lastOperationCanceled; }

void
TxmPlugin::setMinMax(float rmin, float rmax)
{
  m_lastError.clear();
  m_lastOperationCanceled = false;
  if (!std::isfinite(rmin) || !std::isfinite(rmax) || rmin > rmax)
    {
      m_lastError = "The TXM histogram range is invalid.";
      return;
    }

  const float previousRawMin = m_rawMin;
  const float previousRawMax = m_rawMax;
  const QList<uint> previousHistogram = m_histogram;
  m_rawMin = rmin;
  m_rawMax = rmax;
  if (m_voxelType == _Float)
    {
      generateHistogram();
      if (!m_lastError.isEmpty())
        {
          m_rawMin = previousRawMin;
          m_rawMax = previousRawMax;
          m_histogram = previousHistogram;
        }
    }
}
float TxmPlugin::rawMin() { return m_rawMin; }
float TxmPlugin::rawMax() { return m_rawMax; }
QList<uint> TxmPlugin::histogram() { return m_histogram; }

void
TxmPlugin::gridSize(int& d, int& w, int& h)
{
  d = m_depth;
  w = m_width;
  h = m_height;
}

void
TxmPlugin::replaceFile(QString flnm)
{
  loadFile(flnm, false, true);
}

bool
TxmPlugin::setFile(QStringList files)
{
  m_lastError.clear();
  m_lastOperationCanceled = false;
  if (files.isEmpty())
    {
      m_lastError = "No TXM file was selected.";
      return false;
    }

  return loadFile(files[0], true, false);
}

bool
TxmPlugin::loadFile(const QString &fileName, bool scanStatistics,
                    bool requireCompatibleLayout)
{
  m_lastError.clear();
  m_lastOperationCanceled = false;

  const QFileInfo inputFile(fileName);
  if (!inputFile.exists() || !inputFile.isFile() || !inputFile.isReadable())
    {
      m_lastError = QString("The TXM file is missing or unreadable: %1")
                      .arg(inputFile.absoluteFilePath());
      return false;
    }

  std::unique_ptr<POLE::Storage> candidate(
    new POLE::Storage(inputFile.absoluteFilePath().toUtf8().constData()));
  candidate->open();
  if (candidate->result() != POLE::Storage::Ok)
    {
      m_lastError = QString("Unable to open TXM file %1")
                      .arg(inputFile.absoluteFilePath());
      return false;
    }

  int dataType = 0;
  int depth = 0;
  int width = 0;
  int height = 0;
  if (!readTxmValue(candidate.get(), "/ImageInfo/DataType", &dataType) ||
      !readTxmValue(candidate.get(), "/ImageInfo/NoOfImages", &depth) ||
      !readTxmValue(candidate.get(), "/ImageInfo/ImageWidth", &width) ||
      !readTxmValue(candidate.get(), "/ImageInfo/ImageHeight", &height))
    {
      m_lastError = "The TXM header is incomplete.";
      return false;
    }

  int voxelType = -1;
  int bytesPerVoxel = 0;
  if (dataType == 3)
    { voxelType = _UChar; bytesPerVoxel = 1; }
  else if (dataType == 5)
    { voxelType = _UShort; bytesPerVoxel = 2; }
  else if (dataType == 10)
    { voxelType = _Float; bytesPerVoxel = 4; }
  else
    {
      m_lastError = QString("Unsupported TXM data type %1.").arg(dataType);
      return false;
    }

  if (depth <= 0 || width <= 0 || height <= 0 ||
      static_cast<quint64>(width) >
        std::numeric_limits<quint64>::max()/height ||
      static_cast<quint64>(width)*height >
        std::numeric_limits<quint64>::max()/bytesPerVoxel)
    {
      m_lastError = "The TXM dimensions or slice byte count are invalid.";
      return false;
    }
  const quint64 sliceBytes =
    static_cast<quint64>(width)*height*bytesPerVoxel;
  if (sliceBytes > static_cast<quint64>(std::numeric_limits<int>::max()))
    {
      m_lastError = "A TXM slice is too large for this importer.";
      return false;
    }
  if (!admitTxmDecode(sliceBytes, &m_lastError))
    return false;

  QStringList imageData;
  collectTxmImages(candidate.get(), "/", &imageData);
  QCollator collator;
  collator.setCaseSensitivity(Qt::CaseInsensitive);
  collator.setNumericMode(true);
  std::sort(imageData.begin(), imageData.end(),
            [&collator](const QString &left, const QString &right)
            { return collator.compare(left, right) < 0; });
  if (imageData.size() != depth)
    {
      m_lastError = QString("TXM declares %1 images but contains %2 image streams.")
                      .arg(depth).arg(imageData.size());
      return false;
    }
  QProgressDialog validationProgress("Validating TXM images", "Cancel",
                                     0, imageData.size(), 0);
  validationProgress.setMinimumDuration(0);
  const bool validatePixels = !scanStatistics || m_4dvol;
  QByteArray validationPixels;
  if (validatePixels)
    validationPixels.resize(static_cast<int>(sliceBytes));
  for (int index=0; index<imageData.size(); ++index)
    {
      validationProgress.setValue(index);
      qApp->processEvents();
      if (validationProgress.wasCanceled())
        {
          m_lastOperationCanceled = true;
          m_lastError = "TXM import canceled.";
          return false;
        }
      POLE::Stream image(candidate.get(), imageData[index].toUtf8().constData());
      if (image.size() < sliceBytes)
        {
          m_lastError = QString("TXM image stream %1 is truncated.")
                          .arg(imageData[index]);
          return false;
        }
      if (validatePixels &&
          image.read(reinterpret_cast<uchar*>(validationPixels.data()),
                     sliceBytes) != sliceBytes)
        {
          m_lastError = QString("TXM image stream %1 is unreadable.")
                          .arg(imageData[index]);
          return false;
        }
    }
  validationProgress.setValue(imageData.size());
  qApp->processEvents();

  if (requireCompatibleLayout && m_storage &&
      (depth != m_depth || width != m_width || height != m_height ||
       voxelType != m_voxelType || bytesPerVoxel != m_bytesPerVoxel))
    {
      m_lastError = "Cannot replace TXM input: volume layout differs from the original.";
      return false;
    }

  POLE::Storage *previousStorage = m_storage;
  const QStringList previousFileName = m_fileName;
  const QStringList previousImageData = m_imageData;
  const int previousDepth = m_depth;
  const int previousWidth = m_width;
  const int previousHeight = m_height;
  const int previousVoxelType = m_voxelType;
  const int previousBytesPerVoxel = m_bytesPerVoxel;
  const int previousHeaderBytes = m_headerBytes;
  const float previousRawMin = m_rawMin;
  const float previousRawMax = m_rawMax;
  const QList<uint> previousHistogram = m_histogram;

  m_storage = candidate.release();
  m_fileName = QStringList() << inputFile.absoluteFilePath();
  m_imageData = imageData;
  m_depth = depth;
  m_width = width;
  m_height = height;
  m_voxelType = voxelType;
  m_bytesPerVoxel = bytesPerVoxel;
  m_headerBytes = 0;

  try
    {
      if (scanStatistics && !m_4dvol)
        {
          if (m_voxelType == _UChar || m_voxelType == _UShort)
            findMinMaxandGenerateHistogram();
          else
            {
              findMinMax();
              if (m_lastError.isEmpty())
                generateHistogram();
            }
        }
    }
  catch (const std::exception &error)
    {
      m_lastError = QString("Cannot scan TXM data: %1")
                      .arg(QString::fromLocal8Bit(error.what()));
    }
  catch (...)
    {
      m_lastError = "Cannot scan TXM data because an unknown error occurred.";
    }

  if (!m_lastError.isEmpty())
    {
      POLE::Storage *failedStorage = m_storage;
      m_storage = previousStorage;
      m_fileName = previousFileName;
      m_imageData = previousImageData;
      m_depth = previousDepth;
      m_width = previousWidth;
      m_height = previousHeight;
      m_voxelType = previousVoxelType;
      m_bytesPerVoxel = previousBytesPerVoxel;
      m_headerBytes = previousHeaderBytes;
      m_rawMin = previousRawMin;
      m_rawMax = previousRawMax;
      m_histogram = previousHistogram;
      failedStorage->close();
      delete failedStorage;
      return false;
    }

  if (previousStorage)
    {
      previousStorage->close();
      delete previousStorage;
    }
  return true;
}

#define MINMAXANDHISTOGRAM()				\
  {							\
    for(int j=0; j<m_width*m_height; j++)		\
      {							\
	int val = ptr[j];				\
	m_rawMin = qMin(m_rawMin, (float)val);		\
	m_rawMax = qMax(m_rawMax, (float)val);		\
							\
	int idx = val-rMin;				\
	m_histogram[idx]++;				\
      }							\
  }

bool
TxmPlugin::loadTxmImage(int i, uchar* tmp)
{
  if (!m_storage || !tmp || i < 0 || i >= m_depth ||
      i >= m_imageData.size())
    {
      m_lastError = QString("TXM slice %1 is invalid.").arg(i);
      return false;
    }

  int nbytes = m_width*m_height*m_bytesPerVoxel;
  POLE::Stream image(m_storage, m_imageData[i].toUtf8().constData());
  if (image.size() < static_cast<POLE::uint64>(nbytes) ||
      image.read(tmp, nbytes) != static_cast<POLE::uint64>(nbytes))
    {
      m_lastError = QString("TXM image stream %1 is truncated or unreadable.")
                      .arg(m_imageData[i]);
      return false;
    }
  return true;
}

void
TxmPlugin::findMinMaxandGenerateHistogram()
{
  QProgressDialog progress("Generating Histogram",
			   "Cancel",
			   0, 100,
			   0);
  progress.setMinimumDuration(0);

  float rSize;
  float rMin;
  m_histogram.clear();
  if (m_voxelType == _UChar ||
      m_voxelType == _Char)
    {
      if (m_voxelType == _UChar) rMin = 0;
      if (m_voxelType == _Char) rMin = -127;
      rSize = 255;
      for(uint i=0; i<256; i++)
	m_histogram.append(0);
    }
  else if (m_voxelType == _UShort ||
      m_voxelType == _Short)
    {
      if (m_voxelType == _UShort) rMin = 0;
      if (m_voxelType == _Short) rMin = -32767;
      rSize = 65535;
      for(uint i=0; i<65536; i++)
	m_histogram.append(0);
    }
  else
    {
      m_lastError = "Cannot generate a TXM histogram for this voxel type.";
      return;
    }

  int nbytes = m_width*m_height*m_bytesPerVoxel;
  QByteArray tmp(nbytes, 0);

  m_rawMin = std::numeric_limits<float>::max();
  m_rawMax = std::numeric_limits<float>::lowest();

  for(int i=0; i<m_depth; i++)
    {
      progress.setValue((int)(100.0*(float)i/(float)m_depth));
      progress.setLabelText(QString("%1 of %2").arg(i).arg(m_depth-1));
      qApp->processEvents();
      if (progress.wasCanceled())
        {
          m_lastOperationCanceled = true;
          m_lastError = "TXM import canceled.";
          return;
        }

      if (!loadTxmImage(i, reinterpret_cast<uchar*>(tmp.data())))
        return;

      if (m_voxelType == _UChar)
	{
	  uchar *ptr = reinterpret_cast<uchar*>(tmp.data());
	  MINMAXANDHISTOGRAM();
	}
      else if (m_voxelType == _Char)
	{
	  char *ptr = reinterpret_cast<char*>(tmp.data());
	  MINMAXANDHISTOGRAM();
	}
      if (m_voxelType == _UShort)
	{
	  ushort *ptr = reinterpret_cast<ushort*>(tmp.data());
	  MINMAXANDHISTOGRAM();
	}
      else if (m_voxelType == _Short)
	{
	  short *ptr = reinterpret_cast<short*>(tmp.data());
	  MINMAXANDHISTOGRAM();
	}
      else if (m_voxelType == _Int)
	{
	  int *ptr = reinterpret_cast<int*>(tmp.data());
	  MINMAXANDHISTOGRAM();
	}
      else if (m_voxelType == _Float)
	{
	  float *ptr = reinterpret_cast<float*>(tmp.data());
	  MINMAXANDHISTOGRAM();
	}
    }

  if (m_voxelType != _Float)
    {
      if (qAbs(m_rawMax-m_rawMin) < 1)
	{
	  m_rawMin = m_rawMin-1;
	  m_rawMax = m_rawMax+1;
	}
    }
  else
    {
      if (qAbs(m_rawMax-m_rawMin) < 0.001)
	{
	  m_rawMin = m_rawMin-1;
	  m_rawMax = m_rawMax+1;
	}
    }

  
//  while(m_histogram.last() == 0)
//    m_histogram.removeLast();
//  while(m_histogram.first() == 0)
//    m_histogram.removeFirst();

  progress.setValue(100);
  qApp->processEvents();
}

#define FINDMINMAX()					\
  {							\
    for(int j=0; j<nY*nZ; j++)				\
      {							\
	const double val = static_cast<double>(ptr[j]);	\
	if (std::isfinite(val))				\
	  {						\
	    m_rawMin = qMin(m_rawMin, static_cast<float>(val)); \
	    m_rawMax = qMax(m_rawMax, static_cast<float>(val)); \
	    ++finiteValueCount;				\
	  }						\
      }							\
  }

void
TxmPlugin::findMinMax()
{
  QProgressDialog progress("Finding Min and Max",
			   "Cancel",
			   0, 100,
			   0);
  progress.setMinimumDuration(0);


  int nX, nY, nZ;
  nX = m_depth;
  nY = m_width;
  nZ = m_height;

  int nbytes = nY*nZ*m_bytesPerVoxel;
  QByteArray tmp(nbytes, 0);

  m_rawMin = std::numeric_limits<float>::max();
  m_rawMax = std::numeric_limits<float>::lowest();
  quint64 finiteValueCount = 0;
  for(int i=0; i<nX; i++)
    {
      progress.setValue((int)(100.0*(float)i/(float)nX));
      qApp->processEvents();
      if (progress.wasCanceled())
        {
          m_lastOperationCanceled = true;
          m_lastError = "TXM import canceled.";
          return;
        }

      if (!loadTxmImage(i, reinterpret_cast<uchar*>(tmp.data())))
        return;

      if (m_voxelType == _UChar)
	{
	  uchar *ptr = reinterpret_cast<uchar*>(tmp.data());
	  FINDMINMAX();
	}
      else if (m_voxelType == _Char)
	{
	  char *ptr = reinterpret_cast<char*>(tmp.data());
	  FINDMINMAX();
	}
      if (m_voxelType == _UShort)
	{
	  ushort *ptr = reinterpret_cast<ushort*>(tmp.data());
	  FINDMINMAX();
	}
      else if (m_voxelType == _Short)
	{
	  short *ptr = reinterpret_cast<short*>(tmp.data());
	  FINDMINMAX();
	}
      else if (m_voxelType == _Int)
	{
	  int *ptr = reinterpret_cast<int*>(tmp.data());
	  FINDMINMAX();
	}
      else if (m_voxelType == _Float)
	{
	  float *ptr = reinterpret_cast<float*>(tmp.data());
	  FINDMINMAX();
	}
    }

  if (finiteValueCount == 0)
    m_rawMin = m_rawMax = 0;

  progress.setValue(100);
  qApp->processEvents();
}

#define GENHISTOGRAM()					\
  {							\
    for(int j=0; j<nY*nZ; j++)				\
      {							\
	const double value = static_cast<double>(ptr[j]);	\
	int idx = 0;					\
	if (std::isfinite(value))			\
	  {						\
	    const double fraction = rSize > 0 ?		\
	      (value-static_cast<double>(m_rawMin))/rSize : 0; \
	    const double bounded = qBound(0.0, fraction, 1.0); \
	    idx = static_cast<int>(bounded*histogramSize); \
	  }						\
	else if (value > 0)				\
	  idx = histogramSize;				\
	m_histogram[idx]+=1;				\
      }							\
  }

void
TxmPlugin::generateHistogram()
{
  QProgressDialog progress("Generating Histogram",
			   "Cancel",
			   0, 100,
			   0);
  progress.setMinimumDuration(0);


  const double rSize = static_cast<double>(m_rawMax)-m_rawMin;
  int nX, nY, nZ;
  nX = m_depth;
  nY = m_width;
  nZ = m_height;

  int nbytes = nY*nZ*m_bytesPerVoxel;
  QByteArray tmp(nbytes, 0);

  m_histogram.clear();
  if (m_voxelType == _UChar ||
      m_voxelType == _Char ||
      m_voxelType == _UShort ||
      m_voxelType == _Short)
    {
      for(int i=0; i<rSize+1; i++)
	m_histogram.append(0);
    }
  else
    {      
      for(int i=0; i<65536; i++)
	m_histogram.append(0);
    }

  int histogramSize = m_histogram.size()-1;
  for(int i=0; i<nX; i++)
    {
      progress.setValue((int)(100.0*(float)i/(float)nX));
      qApp->processEvents();
      if (progress.wasCanceled())
        {
          m_lastOperationCanceled = true;
          m_lastError = "TXM import canceled.";
          return;
        }

      if (!loadTxmImage(i, reinterpret_cast<uchar*>(tmp.data())))
        return;

      if (m_voxelType == _UChar)
	{
	  uchar *ptr = reinterpret_cast<uchar*>(tmp.data());
	  GENHISTOGRAM();
	}
      else if (m_voxelType == _Char)
	{
	  char *ptr = reinterpret_cast<char*>(tmp.data());
	  GENHISTOGRAM();
	}
      if (m_voxelType == _UShort)
	{
	  ushort *ptr = reinterpret_cast<ushort*>(tmp.data());
	  GENHISTOGRAM();
	}
      else if (m_voxelType == _Short)
	{
	  short *ptr = reinterpret_cast<short*>(tmp.data());
	  GENHISTOGRAM();
	}
      else if (m_voxelType == _Int)
	{
	  int *ptr = reinterpret_cast<int*>(tmp.data());
	  GENHISTOGRAM();
	}
      else if (m_voxelType == _Float)
	{
	  float *ptr = reinterpret_cast<float*>(tmp.data());
	  GENHISTOGRAM();
	}
    }

//  while(m_histogram.last() == 0)
//    m_histogram.removeLast();
//  while(m_histogram.first() == 0)
//    m_histogram.removeFirst();

//  QMessageBox::information(0, "",  QString("%1 %2 : %3").\
//			   arg(m_rawMin).arg(m_rawMax).arg(rSize));

  progress.setValue(100);
  qApp->processEvents();
}

void
TxmPlugin::getDepthSlice(int slc,
			  uchar *slice)
{
  int nbytes = m_width*m_height*m_bytesPerVoxel;
  m_lastError.clear();
  m_lastOperationCanceled = false;
  if (!slice || slc < 0 || slc >= m_depth)
    {
      if (slice && nbytes > 0)
        memset(slice, 0, nbytes);
      m_lastError = QString("TXM slice %1 is invalid.").arg(slc);
      return;
    }

  QByteArray tmp1(nbytes, 0);

  if (!loadTxmImage(slc, reinterpret_cast<uchar*>(tmp1.data())))
    {
      memset(slice, 0, nbytes);
      return;
    }

  if (m_voxelType == _UChar)
    {
      for(int j=0; j<m_width; j++)
	for(int k=0; k<m_height; k++)
	  slice[j*m_height+k] =
	    reinterpret_cast<const uchar*>(tmp1.constData())[k*m_width+j];
    }
  else if (m_voxelType == _UShort)
    {
      ushort *p0 = (ushort*)slice;
      const ushort *p1 = reinterpret_cast<const ushort*>(tmp1.constData());
      for(int j=0; j<m_width; j++)
	for(int k=0; k<m_height; k++)
	  p0[j*m_height+k] = p1[k*m_width+j];
    }
  else if (m_voxelType == _Float)
    {
      float *p0 = (float*)slice;
      const float *p1 = reinterpret_cast<const float*>(tmp1.constData());
      for(int j=0; j<m_width; j++)
	for(int k=0; k<m_height; k++)
	  p0[j*m_height+k] = p1[k*m_width+j];
    }
}

//void
//TxmPlugin::getWidthSlice(int slc,
//			  uchar *slice)
//{
//  QProgressDialog progress("Extracting Slice",
//			   0,
//			   0, 100,
//			   0);
//  progress.setMinimumDuration(0);
//  uchar *imgSlice = new uchar[m_width*m_height*m_bytesPerVoxel];
//  for(uint i=0; i<m_depth; i++)
//    {
//      progress.setValue((int)(100.0*(float)i/(float)m_depth));
//      qApp->processEvents();
//
//      loadTxmImage(i, imgSlice);
//
//      if (m_voxelType == _UChar)
//	{
//	  for(uint j=0; j<m_height; j++)
//	    slice[i*m_height+j] = imgSlice[slc*m_height+j];
//	}
//      else if (m_voxelType == _UShort)
//	{
//	  ushort *p0 = (ushort*)slice;
//	  ushort *p1 = (ushort*)imgSlice;
//	  for(uint j=0; j<m_height; j++)
//	    p0[i*m_height+j] = p1[slc*m_height+j];
//	}
//      else if (m_voxelType == _Float)
//	{
//	  float *p0 = (float*)slice;
//	  float *p1 = (float*)imgSlice;
//	  for(uint j=0; j<m_height; j++)
//	    p0[i*m_height+j] = p1[slc*m_height+j];
//	}
//    }
//  delete [] imgSlice;
//  progress.setValue(100);
//  qApp->processEvents();
//}
//
//void
//TxmPlugin::getHeightSlice(int slc,
//			   uchar *slice)
//{
//  uchar *imgSlice = new uchar[m_width*m_height*m_bytesPerVoxel];
//  QProgressDialog progress("Extracting Slice",
//			   0,
//			   0, 100,
//			   0);
//  progress.setMinimumDuration(0);
//  for(uint i=0; i<m_depth; i++)
//    {
//      progress.setValue((int)(100.0*(float)i/(float)m_depth));
//      qApp->processEvents();
//
//      loadTxmImage(i, imgSlice);
//
//      if (m_voxelType == _UChar)
//	{
//	  for(uint j=0; j<m_width; j++)
//	    slice[i*m_width+j] = imgSlice[j*m_height+slc];
//	}
//      else if (m_voxelType == _UShort)
//	{
//	  ushort *p0 = (ushort*)slice;
//	  ushort *p1 = (ushort*)imgSlice;
//	  for(uint j=0; j<m_width; j++)
//	    p0[i*m_width+j] = p1[j*m_height+slc];
//	}
//      else if (m_voxelType == _Float)
//	{
//	  float *p0 = (float*)slice;
//	  float *p1 = (float*)imgSlice;
//	  for(uint j=0; j<m_width; j++)
//	    p0[i*m_width+j] = p1[j*m_height+slc];
//	}
//    }
//  delete [] imgSlice;
//  progress.setValue(100);
//  qApp->processEvents();
//}

QVariant
TxmPlugin::rawValue(int d, int w, int h)
{
  QVariant v;
  m_lastError.clear();
  m_lastOperationCanceled = false;

  if (d < 0 || d >= m_depth ||
      w < 0 || w >= m_width ||
      h < 0 || h >= m_height)
    {
      v = QVariant("OutOfBounds");
      return v;
    }

  QByteArray imgSlice(m_width*m_height*m_bytesPerVoxel, 0);

  if (!loadTxmImage(d, reinterpret_cast<uchar*>(imgSlice.data())))
    return QVariant("ReadError");

  if (m_voxelType == _UChar)
    v = QVariant((uint)reinterpret_cast<const uchar*>(imgSlice.constData())
                         [h*m_width+w]);
  else if (m_voxelType == _UShort)
    {
      const ushort *values =
	reinterpret_cast<const ushort*>(imgSlice.constData());
      v = QVariant((uint)values[h*m_width+w]);
    }
  else if (m_voxelType == _Float)
    {
      const float *values = reinterpret_cast<const float*>(imgSlice.constData());
      v = QVariant((double)values[h*m_width+w]);
    }

  return v;
}

//void
//TxmPlugin::saveTrimmed(QString trimFile,
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
//      loadTxmImage(i, tmp1);
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
