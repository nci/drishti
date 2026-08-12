#include <QtGui>
#include "common.h"
#include "metaimageplugin.h"
#include <iostream>
#include <itkImage.h>
#include <itkImageFileWriter.h>
#include <itkImageFileReader.h>
#include <itkMetaImageIO.h>
#include <itkImageRegionIterator.h>
#include "../rawfileutils.h"

#include <cmath>
#include <cstring>
#include <limits>
#include <memory>
#include <new>

namespace
{
bool readMetaImageHeader(const QString& fileName, MetaImage& reader,
                         QString& error)
{
  const QFileInfo info(fileName);
  if (!info.exists() || !info.isFile() || !info.isReadable())
    {
      error = QString("Cannot read MetaImage file %1.").arg(fileName);
      return false;
    }

  const QByteArray encodedName = QFile::encodeName(info.absoluteFilePath());
  if (!reader.Read(encodedName.constData(), false))
    {
      error = QString("Cannot parse MetaImage header %1.").arg(fileName);
      return false;
    }
  if (reader.NDims() != 3)
    {
      error = QString("MetaImage input must be 3D; %1 has %2 dimensions.")
                .arg(fileName).arg(reader.NDims());
      return false;
    }
  if (reader.ElementNumberOfChannels() != 1)
    {
      error = QString("MetaImage input must contain one scalar channel; %1 "
                      "contains %2 channels.")
                .arg(fileName).arg(reader.ElementNumberOfChannels());
      return false;
    }
  return true;
}

int metaImageVoxelType(MET_ValueEnumType elementType)
{
  if (elementType == MET_UCHAR) return _UChar;
  if (elementType == MET_CHAR) return _Char;
  if (elementType == MET_USHORT) return _UShort;
  if (elementType == MET_SHORT) return _Short;
  if (elementType == MET_INT) return _Int;
  if (elementType == MET_FLOAT) return _Float;
  return -1;
}

bool readMetaImageSlice(MetaImage& reader, int depth, int width, int height,
                        int sliceNumber, uchar *destination, QString& error)
{
  if (!destination || sliceNumber < 0 || sliceNumber >= depth)
    {
      error = QString("Invalid MetaImage slice %1.").arg(sliceNumber);
      return false;
    }

  int minimum[3] = { 0, 0, sliceNumber };
  int maximum[3] = { height-1, width-1, sliceNumber };
  if (!reader.ReadROI(minimum, maximum, NULL, true, destination, 1))
    {
      error = QString("Cannot decode MetaImage slice %1.").arg(sliceNumber);
      return false;
    }
  return true;
}

bool probeMetaImageData(const QString& headerFile, MetaImage& reader,
                        int depth, int width, int height, int voxelType,
                        QString& error)
{
  RawFileUtils::Layout layout;
  if (!RawFileUtils::makeLayout(depth, width, height, voxelType, 0,
                                layout, error))
    return false;

  const QString elementFile =
    QString::fromLocal8Bit(reader.ElementDataFileName()).trimmed();
  const bool localData = elementFile.compare("local", Qt::CaseInsensitive) == 0;
  const bool fileList = elementFile.startsWith("list", Qt::CaseInsensitive);
  if (!reader.CompressedData() && !localData && !fileList &&
      !elementFile.contains('%'))
    {
      qint64 requiredBytes = layout.volumeBytes;
      const int headerBytes = reader.HeaderSize();
      if (headerBytes > 0 &&
          !RawFileUtils::checkedAdd(requiredBytes, headerBytes,
                                    requiredBytes))
        {
          error = "MetaImage element-data size overflows the supported range.";
          return false;
        }
      const QString dataFile = QFileInfo(elementFile).isAbsolute() ?
        elementFile : QFileInfo(headerFile).absoluteDir().absoluteFilePath(
                        elementFile);
      if (!RawFileUtils::validateFileSize(dataFile, requiredBytes, error))
        return false;
    }

  std::unique_ptr<uchar[]> slice(new (std::nothrow)
    uchar[static_cast<std::size_t>(layout.sliceBytes)]);
  if (!slice)
    {
      error = QString("Cannot allocate %1 bytes to validate MetaImage data.")
                .arg(layout.sliceBytes);
      return false;
    }

  if (!readMetaImageSlice(reader, depth, width, height, 0,
                          slice.get(), error))
    return false;
  return depth == 1 ||
         readMetaImageSlice(reader, depth, width, height, depth-1,
                            slice.get(), error);
}

template<typename T>
void accumulateExact(const uchar *bytes, qint64 count, int histogramOffset,
                     float& minimum, float& maximum, QList<uint>& histogram)
{
  const T *values = reinterpret_cast<const T*>(bytes);
  for (qint64 i=0; i<count; ++i)
    {
      const int value = static_cast<int>(values[i]);
      minimum = qMin(minimum, static_cast<float>(value));
      maximum = qMax(maximum, static_cast<float>(value));
      histogram[value-histogramOffset]++;
    }
}

template<typename T>
bool accumulateRange(const uchar *bytes, qint64 count,
                     float& minimum, float& maximum)
{
  bool found = false;
  const T *values = reinterpret_cast<const T*>(bytes);
  for (qint64 i=0; i<count; ++i)
    {
      const double value = static_cast<double>(values[i]);
      if (!std::isfinite(value))
        continue;
      minimum = qMin(minimum, static_cast<float>(value));
      maximum = qMax(maximum, static_cast<float>(value));
      found = true;
    }
  return found;
}

template<typename T>
void accumulateScaled(const uchar *bytes, qint64 count,
                      float minimum, float maximum, QList<uint>& histogram)
{
  const T *values = reinterpret_cast<const T*>(bytes);
  const int lastIndex = histogram.size()-1;
  for (qint64 i=0; i<count; ++i)
    {
      const int index = RawFileUtils::scaledHistogramIndex(
        static_cast<float>(values[i]), minimum, maximum, lastIndex);
      if (index >= 0)
        histogram[index]++;
    }
}
}


QStringList
MetaImagePlugin::registerPlugin()
{
  QStringList regString;
  regString << "files";
  regString << "MetaImage Files";
  
  return regString;
}

void
MetaImagePlugin::init()
{
  m_fileName.clear();
  m_description.clear();
  m_depth = m_width = m_height = 0;
  m_voxelType = _UChar;
  m_voxelUnit = _Micron;
  m_voxelSizeX = m_voxelSizeY = m_voxelSizeZ = 1;
  m_skipBytes = 0;
  m_headerBytes = 0;
  m_bytesPerVoxel = 1;
  m_rawMin = m_rawMax = 0;
  m_histogram.clear();
  m_4dvol = false;
  m_lastError.clear();
}

void
MetaImagePlugin::clear()
{
  m_fileName.clear();
  m_description.clear();
  m_depth = m_width = m_height = 0;
  m_voxelType = _UChar;
  m_voxelUnit = _Micron;
  m_voxelSizeX = m_voxelSizeY = m_voxelSizeZ = 1;
  m_skipBytes = 0;
  m_headerBytes = 0;
  m_bytesPerVoxel = 1;
  m_rawMin = m_rawMax = 0;
  m_histogram.clear();
  m_4dvol = false;
  m_lastError.clear();
}

void
MetaImagePlugin::set4DVolume(bool flag)
{
  m_4dvol = flag;
}

void
MetaImagePlugin::voxelSize(float& vx, float& vy, float& vz)
  {
    vx = m_voxelSizeX;
    vy = m_voxelSizeY;
    vz = m_voxelSizeZ;
  }
QString MetaImagePlugin::description() { return m_description; }
int MetaImagePlugin::voxelType() { return m_voxelType; }
int MetaImagePlugin::voxelUnit() { return m_voxelUnit; }
int MetaImagePlugin::headerBytes() { return m_headerBytes; }

void
MetaImagePlugin::setMinMax(float rmin, float rmax)
{
  m_rawMin = rmin;
  m_rawMax = rmax;
  
  if (m_voxelType == _UChar ||
      m_voxelType == _Char ||
      m_voxelType == _UShort ||
      m_voxelType == _Short)
    return;
  generateHistogram();
}
float MetaImagePlugin::rawMin() { return m_rawMin; }
float MetaImagePlugin::rawMax() { return m_rawMax; }
QList<uint> MetaImagePlugin::histogram() { return m_histogram; }
QString MetaImagePlugin::lastError() const { return m_lastError; }

void
MetaImagePlugin::gridSize(int& d, int& w, int& h)
{
  d = m_depth;
  w = m_width;
  h = m_height;
}

void
MetaImagePlugin::replaceFile(QString flnm)
{
  m_lastError.clear();
  MetaImage reader;
  QString error;
  if (!readMetaImageHeader(flnm, reader, error))
    {
      m_lastError = error;
      return;
    }

  const int *dimensions = reader.DimSize();
  const int voxelType = metaImageVoxelType(reader.ElementType());
  if (!dimensions || dimensions[2] != m_depth || dimensions[1] != m_width ||
      dimensions[0] != m_height || voxelType != m_voxelType)
    {
      m_lastError = QString(
        "Replacement MetaImage volume is incompatible with the current "
        "%1 x %2 x %3, voxel-type %4 volume.")
          .arg(m_depth).arg(m_width).arg(m_height).arg(m_voxelType);
      return;
    }

  if (!probeMetaImageData(flnm, reader, m_depth, m_width, m_height,
                          m_voxelType, error))
    {
      m_lastError = error;
      return;
    }

  m_fileName = QStringList() << QFileInfo(flnm).absoluteFilePath();
}

bool
MetaImagePlugin::setFile(QStringList files)
{
  m_lastError.clear();
  if (files.isEmpty() || files.first().trimmed().isEmpty())
    {
      m_lastError = "No MetaImage file was selected.";
      return false;
    }

  const QString fileName = QFileInfo(files.first()).absoluteFilePath();
  MetaImage metaImageReader;
  if (!readMetaImageHeader(fileName, metaImageReader, m_lastError))
    return false;

  const int *dims = metaImageReader.DimSize();
  const double *spacing = metaImageReader.ElementSpacing();
  const int voxelType = metaImageVoxelType(metaImageReader.ElementType());
  if (!dims || !spacing || voxelType < 0)
    {
      m_lastError = QString("MetaImage file %1 uses an unsupported scalar "
                            "element type.").arg(fileName);
      return false;
    }

  RawFileUtils::Layout layout;
  QString layoutError;
  if (!RawFileUtils::makeLayout(dims[2], dims[1], dims[0], voxelType, 0,
                                layout, layoutError))
    {
      m_lastError = layoutError;
      return false;
    }

  m_fileName = QStringList() << fileName;
  m_height = dims[0];
  m_width = dims[1];
  m_depth = dims[2];
  m_voxelType = voxelType;
  m_voxelSizeX = std::isfinite(spacing[0]) && spacing[0] > 0 ? spacing[0] : 1;
  m_voxelSizeY = std::isfinite(spacing[1]) && spacing[1] > 0 ? spacing[1] : 1;
  m_voxelSizeZ = std::isfinite(spacing[2]) && spacing[2] > 0 ? spacing[2] : 1;

  m_skipBytes = m_headerBytes = 0;

  m_bytesPerVoxel = 1;
  if (m_voxelType == _UChar) m_bytesPerVoxel = 1;
  else if (m_voxelType == _Char) m_bytesPerVoxel = 1;
  else if (m_voxelType == _UShort) m_bytesPerVoxel = 2;
  else if (m_voxelType == _Short) m_bytesPerVoxel = 2;
  else if (m_voxelType == _Int) m_bytesPerVoxel = 4;
  else if (m_voxelType == _Float) m_bytesPerVoxel = 4;

  if (m_4dvol) // do not perform the full histogram scan.
    return probeMetaImageData(fileName, metaImageReader, m_depth, m_width,
                              m_height, m_voxelType, m_lastError);

  if (m_voxelType == _UChar ||
      m_voxelType == _Char ||
      m_voxelType == _UShort ||
      m_voxelType == _Short)
    {
      findMinMaxandGenerateHistogram();
    }
  else
    {
      findMinMax();
      generateHistogram();
    }

  return m_lastError.isEmpty() && !m_histogram.isEmpty();
}


void
MetaImagePlugin::findMinMaxandGenerateHistogram()
{
  QProgressDialog progress("Generating Histogram",
			   QString(),
			   0, 100,
			   0);
  progress.setMinimumDuration(0);

  int histogramOffset = 0;
  m_histogram.clear();
  if (m_voxelType == _UChar ||
      m_voxelType == _Char)
    {
      histogramOffset = m_voxelType == _Char ? -128 : 0;
      for(int i=0; i<256; i++)
	m_histogram.append(0);
    }
  else if (m_voxelType == _UShort ||
      m_voxelType == _Short)
    {
      histogramOffset = m_voxelType == _Short ? -32768 : 0;
      for(int i=0; i<65536; i++)
	m_histogram.append(0);
    }
  else
    {
      m_lastError = "MetaImage exact histogram requested for an unsupported type.";
      return;
    }

  RawFileUtils::Layout layout;
  QString error;
  if (!RawFileUtils::makeLayout(m_depth, m_width, m_height, m_voxelType, 0,
                                layout, error))
    {
      m_lastError = error;
      m_histogram.clear();
      return;
    }
  std::unique_ptr<uchar[]> tmp(new (std::nothrow)
                                uchar[static_cast<std::size_t>(layout.sliceBytes)]);
  if (!tmp)
    {
      m_lastError = QString("Cannot allocate %1 bytes for a MetaImage slice.")
                      .arg(layout.sliceBytes);
      m_histogram.clear();
      return;
    }
  MetaImage metaImageReader;
  if (!readMetaImageHeader(m_fileName.first(), metaImageReader, error))
    {
      m_lastError = error;
      m_histogram.clear();
      return;
    }

  m_rawMin = std::numeric_limits<float>::max();
  m_rawMax = std::numeric_limits<float>::lowest();
  for(int i=0; i<m_depth; i++)
    {
      progress.setValue((int)(100.0*(float)i/(float)m_depth));
      qApp->processEvents();

      if (!readMetaImageSlice(metaImageReader, m_depth, m_width, m_height,
                              i, tmp.get(), error))
        {
          m_lastError = error;
          m_histogram.clear();
          return;
        }
      if (m_voxelType == _UChar)
	accumulateExact<uchar>(tmp.get(), layout.sliceVoxels, histogramOffset,
                             m_rawMin, m_rawMax, m_histogram);
      else if (m_voxelType == _Char)
	accumulateExact<signed char>(tmp.get(), layout.sliceVoxels,
                                   histogramOffset, m_rawMin, m_rawMax,
                                   m_histogram);
      else if (m_voxelType == _UShort)
	accumulateExact<ushort>(tmp.get(), layout.sliceVoxels, histogramOffset,
                              m_rawMin, m_rawMax, m_histogram);
      else if (m_voxelType == _Short)
	accumulateExact<short>(tmp.get(), layout.sliceVoxels, histogramOffset,
                             m_rawMin, m_rawMax, m_histogram);
    }

  progress.setValue(100);
  qApp->processEvents();
}


void
MetaImagePlugin::findMinMax()
{
  QProgressDialog progress("Finding Min and Max",
			   QString(),
			   0, 100,
			   0);
  progress.setMinimumDuration(0);


  RawFileUtils::Layout layout;
  QString error;
  if (!RawFileUtils::makeLayout(m_depth, m_width, m_height, m_voxelType, 0,
                                layout, error))
    {
      m_lastError = error;
      return;
    }
  std::unique_ptr<uchar[]> tmp(new (std::nothrow)
                                uchar[static_cast<std::size_t>(layout.sliceBytes)]);
  if (!tmp)
    {
      m_lastError = QString("Cannot allocate %1 bytes for a MetaImage slice.")
                      .arg(layout.sliceBytes);
      return;
    }
  MetaImage metaImageReader;
  if (!readMetaImageHeader(m_fileName.first(), metaImageReader, error))
    {
      m_lastError = error;
      return;
    }

  m_rawMin = std::numeric_limits<float>::max();
  m_rawMax = std::numeric_limits<float>::lowest();
  bool foundFiniteValue = false;
  for(int i=0; i<m_depth; i++)
    {
      progress.setValue((int)(100.0*(float)i/(float)m_depth));
      qApp->processEvents();

      if (!readMetaImageSlice(metaImageReader, m_depth, m_width, m_height,
                              i, tmp.get(), error))
        {
          m_lastError = error;
          return;
        }
      if (m_voxelType == _UChar)
	foundFiniteValue |= accumulateRange<uchar>(tmp.get(), layout.sliceVoxels,
                                                  m_rawMin, m_rawMax);
      else if (m_voxelType == _Char)
	foundFiniteValue |= accumulateRange<signed char>(
          tmp.get(), layout.sliceVoxels, m_rawMin, m_rawMax);
      else if (m_voxelType == _UShort)
	foundFiniteValue |= accumulateRange<ushort>(tmp.get(), layout.sliceVoxels,
                                                   m_rawMin, m_rawMax);
      else if (m_voxelType == _Short)
	foundFiniteValue |= accumulateRange<short>(tmp.get(), layout.sliceVoxels,
                                                  m_rawMin, m_rawMax);
      else if (m_voxelType == _Int)
	foundFiniteValue |= accumulateRange<int>(tmp.get(), layout.sliceVoxels,
                                                m_rawMin, m_rawMax);
      else if (m_voxelType == _Float)
	foundFiniteValue |= accumulateRange<float>(tmp.get(), layout.sliceVoxels,
                                                  m_rawMin, m_rawMax);
    }
  if (!foundFiniteValue)
    m_rawMin = m_rawMax = 0;

  progress.setValue(100);
  qApp->processEvents();
}

void
MetaImagePlugin::generateHistogram()
{
  QProgressDialog progress("Generating Histogram",
			   QString(),
			   0, 100,
			   0);
  progress.setMinimumDuration(0);


  m_histogram.clear();
  for(int i=0; i<65536; i++)
    m_histogram.append(0);

  RawFileUtils::Layout layout;
  QString error;
  if (!RawFileUtils::makeLayout(m_depth, m_width, m_height, m_voxelType, 0,
                                layout, error))
    {
      m_lastError = error;
      m_histogram.clear();
      return;
    }
  std::unique_ptr<uchar[]> tmp(new (std::nothrow)
                                uchar[static_cast<std::size_t>(layout.sliceBytes)]);
  if (!tmp)
    {
      m_lastError = QString("Cannot allocate %1 bytes for a MetaImage slice.")
                      .arg(layout.sliceBytes);
      m_histogram.clear();
      return;
    }
  MetaImage metaImageReader;
  if (!readMetaImageHeader(m_fileName.first(), metaImageReader, error))
    {
      m_lastError = error;
      m_histogram.clear();
      return;
    }

  for(int i=0; i<m_depth; i++)
    {
      progress.setValue((int)(100.0*(float)i/(float)m_depth));
      qApp->processEvents();

      if (!readMetaImageSlice(metaImageReader, m_depth, m_width, m_height,
                              i, tmp.get(), error))
        {
          m_lastError = error;
          m_histogram.clear();
          return;
        }
      if (m_voxelType == _UChar)
	accumulateScaled<uchar>(tmp.get(), layout.sliceVoxels, m_rawMin,
                              m_rawMax, m_histogram);
      else if (m_voxelType == _Char)
	accumulateScaled<signed char>(tmp.get(), layout.sliceVoxels, m_rawMin,
                                    m_rawMax, m_histogram);
      else if (m_voxelType == _UShort)
	accumulateScaled<ushort>(tmp.get(), layout.sliceVoxels, m_rawMin,
                               m_rawMax, m_histogram);
      else if (m_voxelType == _Short)
	accumulateScaled<short>(tmp.get(), layout.sliceVoxels, m_rawMin,
                              m_rawMax, m_histogram);
      else if (m_voxelType == _Int)
	accumulateScaled<int>(tmp.get(), layout.sliceVoxels, m_rawMin,
                            m_rawMax, m_histogram);
      else if (m_voxelType == _Float)
	accumulateScaled<float>(tmp.get(), layout.sliceVoxels, m_rawMin,
                              m_rawMax, m_histogram);
    }

  progress.setValue(100);
  qApp->processEvents();
}

void
MetaImagePlugin::getDepthSlice(int slc,
			 uchar *slice)
{
  m_lastError.clear();
  if (!slice)
    {
      m_lastError = "MetaImage depth-slice output buffer is null.";
      return;
    }

  RawFileUtils::Layout layout;
  QString error;
  if (!RawFileUtils::makeLayout(m_depth, m_width, m_height, m_voxelType, 0,
                                layout, error))
    {
      m_lastError = error;
      return;
    }
  if (slc < 0 || slc >= m_depth)
    {
      std::memset(slice, 0, static_cast<std::size_t>(layout.sliceBytes));
      m_lastError = QString("Invalid MetaImage slice %1.").arg(slc);
      return;
    }
  MetaImage metaImageReader;
  if (!readMetaImageHeader(m_fileName.first(), metaImageReader, error) ||
      !readMetaImageSlice(metaImageReader, m_depth, m_width, m_height, slc,
                          slice, error))
    {
      std::memset(slice, 0, static_cast<std::size_t>(layout.sliceBytes));
      m_lastError = error;
    }
}

//void
//MetaImagePlugin::getWidthSlice(int slc,
//			 uchar *slice)
//{
//  int nbytes = m_depth*m_height*m_bytesPerVoxel;
//  if (slc < 0 || slc >= m_width)
//    {
//      memset(slice, 0, nbytes);
//      return;
//    }
//
//  MetaImage metaImageReader(m_fileName[0].toUtf8().data());
//  int mind[3];
//  int maxd[3];
//  mind[0] = 0;
//  mind[1] = slc;
//  mind[2] = 0;
//  maxd[0] = m_height-1;
//  maxd[1] = slc;
//  maxd[2] = m_depth-1;
//  metaImageReader.ReadROI(mind, maxd, NULL, true, slice, 1);
//}
//
//void
//MetaImagePlugin::getHeightSlice(int slc,
//			  uchar *slice)
//{
//  int nbytes = m_depth*m_width*m_bytesPerVoxel;
//  if (slc < 0 || slc >= m_height)
//    {
//      memset(slice, 0, nbytes);
//      return;
//    }
//
//  MetaImage metaImageReader(m_fileName[0].toUtf8().data());
//  int mind[3];
//  int maxd[3];
//  mind[0] = slc;
//  mind[1] = 0;
//  mind[2] = 0;
//  maxd[0] = slc;
//  maxd[1] = m_width-1;
//  maxd[2] = m_depth-1;
//  metaImageReader.ReadROI(mind, maxd, NULL, true, slice, 1);
//}

QVariant
MetaImagePlugin::rawValue(int d, int w, int h)
{
  if (d < 0 || d >= m_depth ||
      w < 0 || w >= m_width ||
      h < 0 || h >= m_height)
    return QVariant("OutOfBounds");

  MetaImage metaImageReader;
  QString error;
  if (!readMetaImageHeader(m_fileName.first(), metaImageReader, error))
    {
      m_lastError = error;
      return QVariant("ReadError");
    }
  int minimum[3] = { h, w, d };
  int maximum[3] = { h, w, d };
  alignas(4) uchar bytes[4] = { 0, 0, 0, 0 };
  if (!metaImageReader.ReadROI(minimum, maximum, NULL, true, bytes, 1))
    {
      m_lastError = QString("Cannot decode MetaImage voxel (%1, %2, %3).")
                      .arg(d).arg(w).arg(h);
      return QVariant("ReadError");
    }

  if (m_voxelType == _UChar)
    return QVariant(static_cast<uint>(bytes[0]));
  else if (m_voxelType == _Char)
    {
      signed char value = 0;
      std::memcpy(&value, bytes, sizeof(value));
      return QVariant(static_cast<int>(value));
    }
  else if (m_voxelType == _UShort)
    {
      ushort value = 0;
      std::memcpy(&value, bytes, sizeof(value));
      return QVariant(static_cast<uint>(value));
    }
  else if (m_voxelType == _Short)
    {
      short value = 0;
      std::memcpy(&value, bytes, sizeof(value));
      return QVariant(static_cast<int>(value));
    }
  else if (m_voxelType == _Int)
    {
      int value = 0;
      std::memcpy(&value, bytes, sizeof(value));
      return QVariant(value);
    }
  else if (m_voxelType == _Float)
    {
      float value = 0;
      std::memcpy(&value, bytes, sizeof(value));
      return QVariant(static_cast<double>(value));
    }
  return QVariant("ReadError");
}

//void
//MetaImagePlugin::saveTrimmed(QString trimFile,
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
//  MetaImage metaImageReader(m_fileName[0].toUtf8().data());
//  int mind[3];
//  int maxd[3];
//  mind[0] = hmin;
//  mind[1] = wmin;
//  maxd[0] = hmax;
//  maxd[1] = wmax;
//
//  for(int i=dmin; i<=dmax; i++)
//    {
//      mind[2] = i;
//      maxd[2] = i;
//      metaImageReader.ReadROI(mind, maxd, NULL, true, tmp, 1);
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
