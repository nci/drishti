#include <QtGui>
#include "common.h"
#include "importmemoryadmission.h"
#include "niftiplugin.h"
#include <iostream>
#include <itkNiftiImageIO.h>

#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace
{
const std::uint64_t kNiftiDecodeSafetyBytes = 64ULL*1024ULL*1024ULL;

bool componentMatchesVoxelType(int componentType, int voxelType)
{
  if (voxelType == _UChar)
    return componentType == itk::ImageIOBase::UCHAR;
  if (voxelType == _Char)
    return componentType == itk::ImageIOBase::CHAR;
  if (voxelType == _UShort)
    return componentType == itk::ImageIOBase::USHORT;
  if (voxelType == _Short)
    return componentType == itk::ImageIOBase::SHORT;
  if (voxelType == _Int)
    return componentType == itk::ImageIOBase::INT;
  if (voxelType == _Float)
    return componentType == itk::ImageIOBase::FLOAT;
  return false;
}

QString niftiMemoryError(std::uint64_t pixelCount)
{
  std::uint64_t requiredBytes = 0;
  if (!checkedImportImageDecodeWorkingSet(pixelCount,
                                          kNiftiDecodeSafetyBytes,
                                          requiredBytes))
    return "NIfTI decode working-set calculation overflowed.";
  const ImportMemoryAdmission admission =
    evaluateImportMemoryAdmission(requiredBytes);
  if (admission.approved)
    return QString();
  return QString("NIfTI decoding was stopped before pixel allocation. "
                 "Required peak increment: %1 MiB; usable physical "
                 "budget: %2 MiB.")
    .arg(requiredBytes/(1024.0*1024.0), 0, 'f', 1)
    .arg(admission.availablePhysicalBudgetBytes/(1024.0*1024.0),
         0, 'f', 1);
}
}

QStringList
NiftiPlugin::registerPlugin()
{
  QStringList regString;
  regString << "files";
  regString << "NIFTI Files";
  
  return regString;
}

void
NiftiPlugin::init()
{
  m_fileName.clear();
  m_description.clear();
  m_depth = m_width = m_height = 0;
  m_voxelType = _UChar;
  m_voxelUnit = _Millimeter;
  m_voxelSizeX = m_voxelSizeY = m_voxelSizeZ = 1;
  m_skipBytes = 0;
  m_bytesPerVoxel = 1;
  m_headerBytes = 0;
  m_rawMin = m_rawMax = 0;
  m_histogram.clear();
  m_lastError.clear();
  m_lastOperationCanceled = false;
  m_4dvol = false;
}

void
NiftiPlugin::clear()
{
  m_fileName.clear();
  m_description.clear();
  m_depth = m_width = m_height = 0;
  m_voxelType = _UChar;
  m_voxelUnit = _Millimeter;
  m_voxelSizeX = m_voxelSizeY = m_voxelSizeZ = 1;
  m_skipBytes = 0;
  m_bytesPerVoxel = 1;
  m_headerBytes = 0;
  m_rawMin = m_rawMax = 0;
  m_histogram.clear();
  m_lastError.clear();
  m_lastOperationCanceled = false;
  m_4dvol = false;
}

void
NiftiPlugin::set4DVolume(bool flag)
{
  m_4dvol = flag;
}

void
NiftiPlugin::voxelSize(float& vx, float& vy, float& vz)
  {
    vx = m_voxelSizeX;
    vy = m_voxelSizeY;
    vz = m_voxelSizeZ;
  }
QString NiftiPlugin::description() { return m_description; }
int NiftiPlugin::voxelType() { return m_voxelType; }
int NiftiPlugin::voxelUnit() { return m_voxelUnit; }
int NiftiPlugin::headerBytes() { return m_headerBytes; }
QString NiftiPlugin::lastError() const { return m_lastError; }
bool NiftiPlugin::wasCanceled() const { return m_lastOperationCanceled; }

void
NiftiPlugin::setMinMax(float rmin, float rmax)
{
  m_lastError.clear();
  m_lastOperationCanceled = false;
  if (!std::isfinite(rmin) || !std::isfinite(rmax) || rmin > rmax)
    {
      m_lastError = "The NIfTI histogram range is invalid.";
      return;
    }

  const float previousRawMin = m_rawMin;
  const float previousRawMax = m_rawMax;
  const QList<uint> previousHistogram = m_histogram;
  m_rawMin = rmin;
  m_rawMax = rmax;
  
  if (m_voxelType == _UChar ||
      m_voxelType == _Char ||
      m_voxelType == _UShort ||
      m_voxelType == _Short)
    return;
  try
    {
      generateHistogram();
    }
  catch (const itk::ExceptionObject &error)
    {
      m_lastError = QString("Cannot generate NIfTI histogram: %1")
                      .arg(QString::fromLocal8Bit(error.GetDescription()));
    }
  catch (const std::exception &error)
    {
      m_lastError = QString("Cannot generate NIfTI histogram: %1")
                      .arg(QString::fromLocal8Bit(error.what()));
    }
  catch (...)
    {
      m_lastError = "Cannot generate the NIfTI histogram.";
    }
  if (!m_lastError.isEmpty())
    {
      m_rawMin = previousRawMin;
      m_rawMax = previousRawMax;
      m_histogram = previousHistogram;
    }
}
float NiftiPlugin::rawMin() { return m_rawMin; }
float NiftiPlugin::rawMax() { return m_rawMax; }
QList<uint> NiftiPlugin::histogram() { return m_histogram; }

void
NiftiPlugin::gridSize(int& d, int& w, int& h)
{
  d = m_depth;
  w = m_width;
  h = m_height;
}

void
NiftiPlugin::replaceFile(QString flnm)
{
  const QStringList previousFileName = m_fileName;
  const int previousDepth = m_depth;
  const int previousWidth = m_width;
  const int previousHeight = m_height;
  const int previousVoxelType = m_voxelType;
  const int previousHeaderBytes = m_headerBytes;
  const int previousSkipBytes = m_skipBytes;
  const int previousBytesPerVoxel = m_bytesPerVoxel;
  const float previousVoxelSizeX = m_voxelSizeX;
  const float previousVoxelSizeY = m_voxelSizeY;
  const float previousVoxelSizeZ = m_voxelSizeZ;
  const float previousRawMin = m_rawMin;
  const float previousRawMax = m_rawMax;
  const QList<uint> previousHistogram = m_histogram;

  const bool previous4dVolume = m_4dvol;
  m_4dvol = true;
  const bool loaded = setFile(QStringList() << flnm);
  m_4dvol = previous4dVolume;
  if (!loaded)
    return;

  const bool compatible = previousFileName.isEmpty() ||
    (m_depth == previousDepth && m_width == previousWidth &&
     m_height == previousHeight && m_voxelType == previousVoxelType &&
     m_bytesPerVoxel == previousBytesPerVoxel &&
     qFuzzyCompare(m_voxelSizeX, previousVoxelSizeX) &&
     qFuzzyCompare(m_voxelSizeY, previousVoxelSizeY) &&
     qFuzzyCompare(m_voxelSizeZ, previousVoxelSizeZ));
  if (!compatible)
    {
      m_fileName = previousFileName;
      m_depth = previousDepth;
      m_width = previousWidth;
      m_height = previousHeight;
      m_voxelType = previousVoxelType;
      m_headerBytes = previousHeaderBytes;
      m_skipBytes = previousSkipBytes;
      m_bytesPerVoxel = previousBytesPerVoxel;
      m_voxelSizeX = previousVoxelSizeX;
      m_voxelSizeY = previousVoxelSizeY;
      m_voxelSizeZ = previousVoxelSizeZ;
      m_lastError =
        "Cannot replace NIfTI input: volume layout differs from the original.";
      return;
    }

  // Time-series replacement keeps the first volume's transfer statistics.
  m_rawMin = previousRawMin;
  m_rawMax = previousRawMax;
  m_histogram = previousHistogram;
}


template <class T>
void
NiftiPlugin::readSlice(int idx[3], int sz[3],
		       int nbytes, uchar *slice)
{
  if (m_fileName.isEmpty() || !slice || nbytes < 0)
    throw std::runtime_error("NIfTI slice request is invalid");

  quint64 expectedBytes = sizeof(T);
  for (int axis=0; axis<3; ++axis)
    {
      if (idx[axis] < 0 || sz[axis] <= 0 ||
          expectedBytes > std::numeric_limits<quint64>::max()/sz[axis])
        throw std::runtime_error("NIfTI slice dimensions are invalid");
      expectedBytes *= sz[axis];
    }
  if (expectedBytes != static_cast<quint64>(nbytes))
    throw std::runtime_error("NIfTI slice byte count does not match its dimensions");

  typedef itk::NiftiImageIO NiftiIOType;
  NiftiIOType::Pointer niftiIO = NiftiIOType::New();
  niftiIO->SetFileName(m_fileName[0].toUtf8().constData());
  niftiIO->ReadImageInformation();
  if (niftiIO->GetNumberOfDimensions() != 3 ||
      niftiIO->GetNumberOfComponents() != 1 ||
      niftiIO->GetComponentSize() != sizeof(T) ||
      !componentMatchesVoxelType(niftiIO->GetComponentType(), m_voxelType) ||
      niftiIO->GetDimensions(0) != static_cast<quint64>(m_height) ||
      niftiIO->GetDimensions(1) != static_cast<quint64>(m_width) ||
      niftiIO->GetDimensions(2) != static_cast<quint64>(m_depth) ||
      !qFuzzyCompare(static_cast<float>(niftiIO->GetSpacing(0)),
                     m_voxelSizeX) ||
      !qFuzzyCompare(static_cast<float>(niftiIO->GetSpacing(1)),
                     m_voxelSizeY) ||
      !qFuzzyCompare(static_cast<float>(niftiIO->GetSpacing(2)),
                     m_voxelSizeZ))
    throw std::runtime_error("NIfTI slice type no longer matches the volume");

  itk::ImageIORegion region(3);
  for (int axis=0; axis<3; ++axis)
    {
      const quint64 dimension = niftiIO->GetDimensions(axis);
      if (static_cast<quint64>(idx[axis]) > dimension ||
          static_cast<quint64>(sz[axis]) >
            dimension-static_cast<quint64>(idx[axis]))
        throw std::runtime_error("NIfTI slice region is outside the volume");
      region.SetIndex(axis, idx[axis]);
      region.SetSize(axis, sz[axis]);
    }
  niftiIO->SetIORegion(region);
  niftiIO->Read(slice);
}

bool
NiftiPlugin::setFile(QStringList files)
{
  m_lastError.clear();
  m_lastOperationCanceled = false;

  if (files.isEmpty())
    {
      m_lastError = "No NIfTI file was selected.";
      return false;
    }

  const QFileInfo inputFile(files[0]);
  if (!inputFile.exists() || !inputFile.isFile() || !inputFile.isReadable())
    {
      m_lastError = QString("The NIfTI file is missing or unreadable: %1")
                      .arg(inputFile.absoluteFilePath());
      return false;
    }

  const QStringList previousFileName = m_fileName;
  const int previousDepth = m_depth;
  const int previousWidth = m_width;
  const int previousHeight = m_height;
  const int previousVoxelType = m_voxelType;
  const int previousHeaderBytes = m_headerBytes;
  const int previousSkipBytes = m_skipBytes;
  const int previousBytesPerVoxel = m_bytesPerVoxel;
  const float previousVoxelSizeX = m_voxelSizeX;
  const float previousVoxelSizeY = m_voxelSizeY;
  const float previousVoxelSizeZ = m_voxelSizeZ;
  const float previousRawMin = m_rawMin;
  const float previousRawMax = m_rawMax;
  const QList<uint> previousHistogram = m_histogram;

  m_fileName = QStringList() << inputFile.absoluteFilePath();

  try
    {
      typedef itk::NiftiImageIO NiftiIOType;
      NiftiIOType::Pointer imageIO = NiftiIOType::New();
      imageIO->SetFileName(m_fileName[0].toUtf8().constData());
      imageIO->ReadImageInformation();

      if (imageIO->GetNumberOfDimensions() != 3)
        throw std::runtime_error("Only three-dimensional NIfTI images are supported");
      if (imageIO->GetNumberOfComponents() != 1)
        throw std::runtime_error("Only scalar NIfTI images are supported");

      const quint64 height = imageIO->GetDimensions(0);
      const quint64 width = imageIO->GetDimensions(1);
      const quint64 depth = imageIO->GetDimensions(2);
      const quint64 maximumDimension =
        static_cast<quint64>(std::numeric_limits<int>::max());
      if (height == 0 || width == 0 || depth == 0 ||
          height > maximumDimension || width > maximumDimension ||
          depth > maximumDimension)
        throw std::runtime_error("NIfTI dimensions are empty or too large");

      int voxelType = -1;
      int bytesPerVoxel = 0;
      const int componentType = imageIO->GetComponentType();
      if (componentType == itk::ImageIOBase::UCHAR)
        { voxelType = _UChar; bytesPerVoxel = 1; }
      else if (componentType == itk::ImageIOBase::CHAR)
        { voxelType = _Char; bytesPerVoxel = 1; }
      else if (componentType == itk::ImageIOBase::USHORT)
        { voxelType = _UShort; bytesPerVoxel = 2; }
      else if (componentType == itk::ImageIOBase::SHORT)
        { voxelType = _Short; bytesPerVoxel = 2; }
      else if (componentType == itk::ImageIOBase::INT)
        { voxelType = _Int; bytesPerVoxel = 4; }
      else if (componentType == itk::ImageIOBase::FLOAT)
        { voxelType = _Float; bytesPerVoxel = 4; }
      else
        throw std::runtime_error("The NIfTI voxel type is unsupported");

      if (width > std::numeric_limits<quint64>::max()/height ||
          width*height > std::numeric_limits<quint64>::max()/bytesPerVoxel)
        throw std::runtime_error("NIfTI slice byte count overflowed");
      const quint64 sliceBytes = height*width*bytesPerVoxel;
      if (sliceBytes > static_cast<quint64>(std::numeric_limits<int>::max()))
        throw std::runtime_error("A NIfTI slice is too large for this importer");
      const QString memoryError = niftiMemoryError(width*height);
      if (!memoryError.isEmpty())
        throw std::runtime_error(memoryError.toStdString());

      m_height = static_cast<int>(height);
      m_width = static_cast<int>(width);
      m_depth = static_cast<int>(depth);
      m_voxelType = voxelType;
      m_bytesPerVoxel = bytesPerVoxel;
      m_voxelSizeX = imageIO->GetSpacing(0);
      m_voxelSizeY = imageIO->GetSpacing(1);
      m_voxelSizeZ = imageIO->GetSpacing(2);
      if (!std::isfinite(m_voxelSizeX) || !std::isfinite(m_voxelSizeY) ||
          !std::isfinite(m_voxelSizeZ) || m_voxelSizeX <= 0 ||
          m_voxelSizeY <= 0 || m_voxelSizeZ <= 0)
        throw std::runtime_error("NIfTI voxel spacing is invalid");
      m_skipBytes = m_headerBytes = 0;

      if (m_4dvol)
        {
          QByteArray probe(static_cast<int>(sliceBytes), 0);
          int idx[3] = {0, 0, m_depth-1};
          int size[3] = {m_height, m_width, 1};
          if (m_voxelType == _UChar)
            readSlice<unsigned char>(idx, size, probe.size(),
                                     reinterpret_cast<uchar*>(probe.data()));
          else if (m_voxelType == _Char)
            readSlice<char>(idx, size, probe.size(),
                            reinterpret_cast<uchar*>(probe.data()));
          else if (m_voxelType == _UShort)
            readSlice<unsigned short>(idx, size, probe.size(),
                                      reinterpret_cast<uchar*>(probe.data()));
          else if (m_voxelType == _Short)
            readSlice<short>(idx, size, probe.size(),
                             reinterpret_cast<uchar*>(probe.data()));
          else if (m_voxelType == _Int)
            readSlice<int>(idx, size, probe.size(),
                           reinterpret_cast<uchar*>(probe.data()));
          else if (m_voxelType == _Float)
            readSlice<float>(idx, size, probe.size(),
                             reinterpret_cast<uchar*>(probe.data()));
        }
      else
        {
          if (m_voxelType == _UChar ||
              m_voxelType == _Char ||
              m_voxelType == _UShort ||
              m_voxelType == _Short)
            findMinMaxandGenerateHistogram();
          else
            {
              findMinMax();
              generateHistogram();
            }
        }

      return true;
    }
  catch (const itk::ExceptionObject &error)
    {
      m_lastError = QString("Cannot read NIfTI data: %1")
                      .arg(QString::fromLocal8Bit(error.GetDescription()));
    }
  catch (const std::exception &error)
    {
      m_lastError = QString("Cannot read NIfTI data: %1")
                      .arg(QString::fromLocal8Bit(error.what()));
    }
  catch (...)
    {
      m_lastError = "Cannot read NIfTI data because an unknown error occurred.";
    }

  m_fileName = previousFileName;
  m_depth = previousDepth;
  m_width = previousWidth;
  m_height = previousHeight;
  m_voxelType = previousVoxelType;
  m_headerBytes = previousHeaderBytes;
  m_skipBytes = previousSkipBytes;
  m_bytesPerVoxel = previousBytesPerVoxel;
  m_voxelSizeX = previousVoxelSizeX;
  m_voxelSizeY = previousVoxelSizeY;
  m_voxelSizeZ = previousVoxelSizeZ;
  m_rawMin = previousRawMin;
  m_rawMax = previousRawMax;
  m_histogram = previousHistogram;
  qWarning() << m_lastError;
  return false;
}


#define MINMAXANDHISTOGRAM()				\
  {							\
    for(int j=0; j<nY*nZ; j++)				\
      {							\
	int val = ptr[j];				\
	m_rawMin = qMin(m_rawMin, (float)val);		\
	m_rawMax = qMax(m_rawMax, (float)val);		\
							\
	int idx = val-rMin;				\
	m_histogram[idx]++;				\
      }							\
  }


void
NiftiPlugin::findMinMaxandGenerateHistogram()
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
      if (m_voxelType == _Char) rMin = -128;
      rSize = 255;
      for(int i=0; i<256; i++)
	m_histogram.append(0);
    }
  else if (m_voxelType == _UShort ||
      m_voxelType == _Short)
    {
      if (m_voxelType == _UShort) rMin = 0;
      if (m_voxelType == _Short) rMin = -32768;
      rSize = 65535;
      for(int i=0; i<65536; i++)
	m_histogram.append(0);
    }
  else
    {
      QMessageBox::information(0, "Error", "Why am i here ???");
      return;
    }

//  //==================
//  // do not calculate histogram
//  if (m_voxelType == _UChar)
//    {
//      m_rawMin = 0;
//      m_rawMax = 255;
//      progress.setValue(100);
//      return;
//    }
//  //==================

  int nX, nY, nZ;
  nX = m_depth;
  nY = m_width;
  nZ = m_height;

  int nbytes = nY*nZ*m_bytesPerVoxel;
  QByteArray tmp(nbytes, 0);

  int idx[3];
  int sz[3];
  idx[0] = idx[1] = idx[2] = 0;
  sz[0] = m_height;
  sz[1] = m_width;
  sz[2] = 1;

  m_rawMin = std::numeric_limits<float>::max();
  m_rawMax = std::numeric_limits<float>::lowest();
  for(int i=0; i<m_depth; i++)
    {
      progress.setValue((int)(100.0*(float)i/(float)m_depth));
      qApp->processEvents();
      if (progress.wasCanceled())
        {
          m_lastOperationCanceled = true;
          throw std::runtime_error("NIfTI import canceled");
        }

      idx[2] = i;

      if (m_voxelType == _UChar)
	readSlice<unsigned char>(idx, sz, nbytes,
				 reinterpret_cast<uchar*>(tmp.data()));
      else if (m_voxelType == _Char)
	readSlice<char>(idx, sz, nbytes, reinterpret_cast<uchar*>(tmp.data()));
      else if (m_voxelType == _UShort)
	readSlice<unsigned short>(idx, sz, nbytes,
				  reinterpret_cast<uchar*>(tmp.data()));
      else if (m_voxelType == _Short)
	readSlice<short>(idx, sz, nbytes, reinterpret_cast<uchar*>(tmp.data()));
      else if (m_voxelType == _Int)
	readSlice<int>(idx, sz, nbytes, reinterpret_cast<uchar*>(tmp.data()));
      else if (m_voxelType == _Float)
	readSlice<float>(idx, sz, nbytes, reinterpret_cast<uchar*>(tmp.data()));
 
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
NiftiPlugin::findMinMax()
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

  int idx[3];
  int sz[3];
  idx[0] = idx[1] = idx[2] = 0;
  sz[0] = m_height;
  sz[1] = m_width;
  sz[2] = 1;

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
          throw std::runtime_error("NIfTI import canceled");
        }

      idx[2] = i;

      if (m_voxelType == _UChar)
	readSlice<unsigned char>(idx, sz, nbytes,
				 reinterpret_cast<uchar*>(tmp.data()));
      else if (m_voxelType == _Char)
	readSlice<char>(idx, sz, nbytes, reinterpret_cast<uchar*>(tmp.data()));
      else if (m_voxelType == _UShort)
	readSlice<unsigned short>(idx, sz, nbytes,
				  reinterpret_cast<uchar*>(tmp.data()));
      else if (m_voxelType == _Short)
	readSlice<short>(idx, sz, nbytes, reinterpret_cast<uchar*>(tmp.data()));
      else if (m_voxelType == _Int)
	readSlice<int>(idx, sz, nbytes, reinterpret_cast<uchar*>(tmp.data()));
      else if (m_voxelType == _Float)
	readSlice<float>(idx, sz, nbytes, reinterpret_cast<uchar*>(tmp.data()));

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
NiftiPlugin::generateHistogram()
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

  int idx[3];
  int sz[3];
  idx[0] = idx[1] = idx[2] = 0;
  sz[0] = m_height;
  sz[1] = m_width;
  sz[2] = 1;

  for(int i=0; i<nX; i++)
    {
      progress.setValue((int)(100.0*(float)i/(float)nX));
      qApp->processEvents();
      if (progress.wasCanceled())
        {
          m_lastOperationCanceled = true;
          throw std::runtime_error("NIfTI import canceled");
        }

      idx[2] = i;

      if (m_voxelType == _UChar)
	readSlice<unsigned char>(idx, sz, nbytes,
				 reinterpret_cast<uchar*>(tmp.data()));
      else if (m_voxelType == _Char)
	readSlice<char>(idx, sz, nbytes, reinterpret_cast<uchar*>(tmp.data()));
      else if (m_voxelType == _UShort)
	readSlice<unsigned short>(idx, sz, nbytes,
				  reinterpret_cast<uchar*>(tmp.data()));
      else if (m_voxelType == _Short)
	readSlice<short>(idx, sz, nbytes, reinterpret_cast<uchar*>(tmp.data()));
      else if (m_voxelType == _Int)
	readSlice<int>(idx, sz, nbytes, reinterpret_cast<uchar*>(tmp.data()));
      else if (m_voxelType == _Float)
	readSlice<float>(idx, sz, nbytes, reinterpret_cast<uchar*>(tmp.data()));

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
NiftiPlugin::getDepthSlice(int slc,
			 uchar *slice)
{
  int nbytes = m_width*m_height*m_bytesPerVoxel;
  m_lastError.clear();
  m_lastOperationCanceled = false;
  if (!slice || slc < 0 || slc >= m_depth)
    {
      if (slice && nbytes > 0)
        memset(slice, 0, nbytes);
      m_lastError = QString("NIfTI slice %1 is invalid.").arg(slc);
      return;
    }

  int idx[3];
  int sz[3];
  idx[0] = idx[1] = 0;
  idx[2] = slc;
  sz[0] = m_height;
  sz[1] = m_width;
  sz[2] = 1;

  try
    {
      if (m_voxelType == _UChar)
        readSlice<unsigned char>(idx, sz, nbytes, slice);
      else if (m_voxelType == _Char)
        readSlice<char>(idx, sz, nbytes, slice);
      else if (m_voxelType == _UShort)
        readSlice<unsigned short>(idx, sz, nbytes, slice);
      else if (m_voxelType == _Short)
        readSlice<short>(idx, sz, nbytes, slice);
      else if (m_voxelType == _Int)
        readSlice<int>(idx, sz, nbytes, slice);
      else if (m_voxelType == _Float)
        readSlice<float>(idx, sz, nbytes, slice);
    }
  catch (const itk::ExceptionObject &error)
    {
      memset(slice, 0, nbytes);
      m_lastError = QString("Cannot read NIfTI slice: %1")
                      .arg(QString::fromLocal8Bit(error.GetDescription()));
    }
  catch (const std::exception &error)
    {
      memset(slice, 0, nbytes);
      m_lastError = QString("Cannot read NIfTI slice: %1")
                      .arg(QString::fromLocal8Bit(error.what()));
    }
}

//void
//NiftiPlugin::getWidthSlice(int slc,
//			 uchar *slice)
//{
//  int nbytes = m_depth*m_height*m_bytesPerVoxel;
//  if (slc < 0 || slc >= m_width)
//    {
//      memset(slice, 0, nbytes);
//      return;
//    }
//
//  int idx[3];
//  int sz[3];
//  idx[0] = 0;
//  idx[1] = slc;
//  idx[2] = 0;
//  sz[0] = m_height;
//  sz[1] = 1;
//  sz[2] = m_depth;
//
//  if (m_voxelType == _UChar)
//    readSlice<unsigned char>(idx, sz, nbytes, slice);
//  else if (m_voxelType == _Char)
//    readSlice<char>(idx, sz, nbytes, slice);
//  else if (m_voxelType == _UShort)
//    readSlice<unsigned short>(idx, sz, nbytes, slice);
//  else if (m_voxelType == _Short)
//    readSlice<short>(idx, sz, nbytes, slice);
//  else if (m_voxelType == _Int)
//    readSlice<int>(idx, sz, nbytes, slice);
//  else if (m_voxelType == _Float)
//    readSlice<float>(idx, sz, nbytes, slice);
//}
//
//void
//NiftiPlugin::getHeightSlice(int slc,
//			  uchar *slice)
//{
//  int nbytes = m_depth*m_width*m_bytesPerVoxel;
//  if (slc < 0 || slc >= m_height)
//    {
//      memset(slice, 0, nbytes);
//      return;
//    }
//
//  int idx[3];
//  int sz[3];
//  idx[0] = slc;
//  idx[1] = 0;
//  idx[2] = 0;
//  sz[0] = 1;
//  sz[1] = m_width;
//  sz[2] = m_depth;
//
//  if (m_voxelType == _UChar)
//    readSlice<unsigned char>(idx, sz, nbytes, slice);
//  else if (m_voxelType == _Char)
//    readSlice<char>(idx, sz, nbytes, slice);
//  else if (m_voxelType == _UShort)
//    readSlice<unsigned short>(idx, sz, nbytes, slice);
//  else if (m_voxelType == _Short)
//    readSlice<short>(idx, sz, nbytes, slice);
//  else if (m_voxelType == _Int)
//    readSlice<int>(idx, sz, nbytes, slice);
//  else if (m_voxelType == _Float)
//    readSlice<float>(idx, sz, nbytes, slice);
//}

QVariant
NiftiPlugin::rawValue(int d, int w, int h)
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

  int idx[3];
  int sz[3];
  idx[0] = h;
  idx[1] = w;
  idx[2] = d;
  sz[0] = 1;
  sz[1] = 1;
  sz[2] = 1;

  try
    {
      if (m_voxelType == _UChar)
        {
          unsigned char a = 0;
          readSlice<unsigned char>(idx, sz, 1, &a);
          v = QVariant((uint)a);
        }
      else if (m_voxelType == _Char)
        {
          char a = 0;
          readSlice<char>(idx, sz, 1, (uchar*)&a);
          v = QVariant((int)a);
        }
      else if (m_voxelType == _UShort)
        {
          unsigned short a = 0;
          readSlice<unsigned short>(idx, sz, 2, (uchar*)&a);
          v = QVariant((uint)a);
        }
      else if (m_voxelType == _Short)
        {
          short a = 0;
          readSlice<short>(idx, sz, 2, (uchar*)&a);
          v = QVariant((int)a);
        }
      else if (m_voxelType == _Int)
        {
          int a = 0;
          readSlice<int>(idx, sz, 4, (uchar*)&a);
          v = QVariant((int)a);
        }
      else if (m_voxelType == _Float)
        {
          float a = 0;
          readSlice<float>(idx, sz, 4, (uchar*)&a);
          v = QVariant((double)a);
        }
    }
  catch (const itk::ExceptionObject &error)
    {
      m_lastError = QString("Cannot read NIfTI value: %1")
                      .arg(QString::fromLocal8Bit(error.GetDescription()));
    }
  catch (const std::exception &error)
    {
      m_lastError = QString("Cannot read NIfTI value: %1")
                      .arg(QString::fromLocal8Bit(error.what()));
    }

  return v;
}

//void
//NiftiPlugin::saveTrimmed(QString trimFile,
//			     int dmin, int dmax,
//			     int wmin, int wmax,
//			     int hmin, int hmax)
//{
//  QProgressDialog progress("Saving trimmed volume",
//			   QString(),
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
//
//  int idx[3];
//  int sz[3];
//  idx[0] = hmin;
//  idx[1] = wmin;
//  idx[2] = dmin;
//  sz[0] = mZ;
//  sz[1] = mY;
//  sz[2] = 1;
//
//  for(int i=dmin; i<=dmax; i++)
//    {
//      idx[2] = i;
//
//      if (m_voxelType == _UChar)
//	readSlice<unsigned char>(idx, sz, nbytes, tmp);
//      else if (m_voxelType == _Char)
//	readSlice<char>(idx, sz, nbytes, tmp);
//      else if (m_voxelType == _UShort)
//	readSlice<unsigned short>(idx, sz, nbytes, tmp);
//      else if (m_voxelType == _Short)
//	readSlice<short>(idx, sz, nbytes, tmp);
//      else if (m_voxelType == _Int)
//	readSlice<int>(idx, sz, nbytes, tmp);
//      else if (m_voxelType == _Float)
//	readSlice<float>(idx, sz, nbytes, tmp);
//
//      fout.write((char*)tmp, mY*mZ*m_bytesPerVoxel);
//
//      progress.setValue((int)(100*(float)(i-dmin)/(float)mX));
//      qApp->processEvents();
//    }
//  fout.close();
//
//  delete [] tmp;
//
//  m_headerBytes = 13; // to be used for applyMapping function
//}
