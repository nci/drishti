#include <QtGui>
#include "common.h"
#include "rawplugin.h"
#include "loadrawdialog.h"
#include "../rawfileutils.h"
#include <math.h>
#include <cstring>
#include <memory>
#include <new>

#if defined(Q_OS_WIN32)
#define isnan _isnan
#endif

QStringList
RawPlugin::registerPlugin()
{
  QStringList regString;
  regString << "files";
  regString << "RAW Files";
  
  return regString;
}

void
RawPlugin::setValue(QString key, float val)
{
  if (key == "skiprawdialog")
    {
      if (val > 0)
	m_skipRawDialog = true;
      else
	m_skipRawDialog = false;
    }
}
    
void
RawPlugin::init()
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
  m_skipRawDialog = false;
  m_lastError.clear();
  m_lastOperationCanceled = false;
}

void
RawPlugin::clear()
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
  m_skipRawDialog = false;
  m_lastError.clear();
  m_lastOperationCanceled = false;
}

void
RawPlugin::set4DVolume(bool flag)
{
  m_4dvol = flag;
}

void
RawPlugin::voxelSize(float& vx, float& vy, float& vz)
  {
    vx = m_voxelSizeX;
    vy = m_voxelSizeY;
    vz = m_voxelSizeZ;
  }
QString RawPlugin::description() { return m_description; }
int RawPlugin::voxelType() { return m_voxelType; }
int RawPlugin::voxelUnit() { return m_voxelUnit; }
int RawPlugin::headerBytes() { return m_headerBytes; }

void
RawPlugin::setMinMax(float rmin, float rmax)
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
float RawPlugin::rawMin() { return m_rawMin; }
float RawPlugin::rawMax() { return m_rawMax; }
QList<uint> RawPlugin::histogram() { return m_histogram; }
QString RawPlugin::lastError() const { return m_lastError; }
bool RawPlugin::wasCanceled() const { return m_lastOperationCanceled; }

bool
RawPlugin::setError(const QString& error, bool showDialog)
{
  m_lastError = error;
  if (showDialog)
    QMessageBox::critical(0, "RAW Import Error", error);
  return false;
}

void
RawPlugin::gridSize(int& d, int& w, int& h)
{
  d = m_depth;
  w = m_width;
  h = m_height;
}

void
RawPlugin::replaceFile(QString flnm)
{
  m_lastOperationCanceled = false;
  RawFileUtils::Layout layout;
  QString error;
  if (!RawFileUtils::makeLayout(m_depth, m_width, m_height, m_voxelType,
                                m_skipBytes, layout, error) ||
      !RawFileUtils::validateFileSize(flnm, layout.requiredFileBytes, error))
    {
      setError(error);
      return;
    }

  m_lastError.clear();
  m_fileName.clear();
  m_fileName << flnm;
}

bool
RawPlugin::setFile(QStringList files)
{
  m_lastError.clear();
  m_lastOperationCanceled = false;
  if (files.count() != 1 || files[0].isEmpty())
    return setError("Select exactly one RAW file.");

  m_fileName = files;

  int nX = 0;
  int nY = 0;
  int nZ = 0;
  {
    if (m_skipRawDialog)
      {
	QString error;
	if (!RawFileUtils::readEmbeddedHeader(m_fileName[0], m_voxelType,
	                                      nX, nY, nZ, error))
	  return setError(error);
	m_skipBytes = 13;
      }
    else
      {
	// --- load various parameters from the raw file ---
	LoadRawDialog loadRawDialog(0,
				    (char *)m_fileName[0].toUtf8().data());
	
	if (!m_4dvol)
	  {
	    loadRawDialog.exec();    
	    if (loadRawDialog.result() == QDialog::Rejected)
	      {
	        m_lastOperationCanceled = true;
	        m_lastError = "RAW import canceled";
	        return false;
	      }
	  }

	m_voxelType = loadRawDialog.voxelType();
	m_skipBytes = loadRawDialog.skipHeaderBytes();
	loadRawDialog.gridSize(nX, nY, nZ);
      }
  }

  m_depth = nX;
  m_width = nY;
  m_height = nZ;

  int bytesPerVoxel = 0;
  if (!RawFileUtils::bytesPerVoxel(m_voxelType, bytesPerVoxel))
    return setError(QString("Unsupported RAW voxel type %1.")
                      .arg(m_voxelType));

  QFile fin(m_fileName[0]);
  if (!fin.open(QFile::ReadOnly))
    return setError(QString("Cannot open RAW file %1: %2")
                      .arg(m_fileName[0], fin.errorString()));

  //-- recheck the information (for backward compatibility) ----
  if (m_skipBytes == 0)
    {
      RawFileUtils::Layout payloadLayout;
      QString error;
      if (!RawFileUtils::makeLayout(nX, nY, nZ, m_voxelType, 0,
                                    payloadLayout, error))
	return setError(error);

      if (fin.size() == 13+payloadLayout.volumeBytes)
	m_skipBytes = 13;

      else if (fin.size() == 12+payloadLayout.volumeBytes)
	m_skipBytes = 12;
      else
	m_skipBytes = 0;
    }
  m_headerBytes = m_skipBytes;
  fin.close();

  RawFileUtils::Layout layout;
  QString error;
  if (!RawFileUtils::makeLayout(m_depth, m_width, m_height, m_voxelType,
                                m_skipBytes, layout, error))
    return setError(error);
  if (!RawFileUtils::validateFileSize(m_fileName[0],
                                      layout.requiredFileBytes, error))
    return setError(error);
  m_bytesPerVoxel = bytesPerVoxel;

  if (m_4dvol) // do not perform further calculations.
    return true;

  if (m_voxelType == _UChar ||
      m_voxelType == _Char ||
      m_voxelType == _UShort ||
      m_voxelType == _Short)
    {
      if (!findMinMaxandGenerateHistogram())
	return setError(m_lastError);
    }
  else
    {
      findMinMax();
      if (!m_lastError.isEmpty())
	return setError(m_lastError);
      generateHistogram();
      if (!m_lastError.isEmpty())
	return setError(m_lastError);
    }

  return true;
}


#define MINMAXANDHISTOGRAM()				\
  {							\
    for(qint64 j=0; j<voxelCount; j++)			\
      {							\
	qint64 val = ptr[j];				\
	m_rawMin = qMin(m_rawMin, (float)val);		\
	m_rawMax = qMax(m_rawMax, (float)val);		\
							\
	int idx = RawFileUtils::exactHistogramIndex(m_voxelType, val); \
	if (idx < 0 || idx >= m_histogram.size())		\
	  return setError("RAW histogram index is invalid.", false); \
	if (m_histogram[idx] < std::numeric_limits<uint>::max()) \
	  m_histogram[idx]++;				\
      }							\
  }


bool
RawPlugin::findMinMaxandGenerateHistogram()
{
  m_histogram.clear();
  if (m_voxelType == _UChar ||
      m_voxelType == _Char)
    {
      for(int i=0; i<256; i++)
	m_histogram.append(0);
    }
  else if (m_voxelType == _UShort ||
      m_voxelType == _Short)
    {
      for(int i=0; i<65536; i++)
	m_histogram.append(0);
    }
  else
    {
      return setError("Unsupported voxel type for exact RAW histogram.",
                      false);
    }

  //==================
  // do not calculate histogram
  QString yn = "no";
  if (!m_skipRawDialog)
    {
      QStringList items;
      items << "yes" << "no";
      yn = QInputDialog::getItem(0, "Histogram",
				 "Want to generate histogram ?",
				 items,
				 0,
				 false);
    }

  if (yn != "yes")
    {
      if (m_voxelType == _UChar)
	{
	  m_rawMin = 0;
	  m_rawMax = 255;
	  return true;
	}
      else if (m_voxelType == _Char)
	{
	  m_rawMin = -128;
	  m_rawMax = 127;
	  return true;
	}
      else if (m_voxelType == _UShort)
	{
	  m_rawMin = 0;
	  m_rawMax = 65535;
	  return true;
	}
      else if (m_voxelType == _Short)
	{
	  m_rawMin = -32768;
	  m_rawMax = 32767;
	  return true;
	}
    }
  //==================

  QProgressDialog progress("Generating Histogram",
			   QString(),
			   0, 100,
			   0);
  progress.setMinimumDuration(0);

  RawFileUtils::Layout layout;
  QString error;
  if (!RawFileUtils::makeLayout(m_depth, m_width, m_height, m_voxelType,
                                m_skipBytes, layout, error))
    return setError(error, false);
  const qint64 voxelCount = layout.sliceVoxels;
  const qint64 nbytes = layout.sliceBytes;
  std::unique_ptr<uchar[]> tmp(new (std::nothrow)
                               uchar[static_cast<std::size_t>(nbytes)]);
  if (!tmp)
    return setError(QString("Cannot allocate %1 bytes for a RAW slice.")
                      .arg(nbytes), false);

  QFile fin(m_fileName[0]);
  if (!fin.open(QFile::ReadOnly) || !fin.seek(m_skipBytes))
    return setError(QString("Cannot open or seek RAW file %1: %2")
                      .arg(m_fileName[0], fin.errorString()), false);

  m_rawMin = std::numeric_limits<float>::max();
  m_rawMax = std::numeric_limits<float>::lowest();
  for(int i=0; i<m_depth; i++)
    {
      progress.setValue((int)(100.0*(float)i/(float)m_depth));
      if (qApp) qApp->processEvents();

      if (!RawFileUtils::readExact(fin, tmp.get(), nbytes, error))
	return setError(error, false);

      if (m_voxelType == _UChar)
	{
	  uchar *ptr = tmp.get();
	  MINMAXANDHISTOGRAM();
	}
      else if (m_voxelType == _Char)
	{
	  qint8 *ptr = reinterpret_cast<qint8*>(tmp.get());
	  MINMAXANDHISTOGRAM();
	}
      if (m_voxelType == _UShort)
	{
	  quint16 *ptr = reinterpret_cast<quint16*>(tmp.get());
	  MINMAXANDHISTOGRAM();
	}
      else if (m_voxelType == _Short)
	{
	  qint16 *ptr = reinterpret_cast<qint16*>(tmp.get());
	  MINMAXANDHISTOGRAM();
	}
      else if (m_voxelType == _Int)
	{
	  qint32 *ptr = reinterpret_cast<qint32*>(tmp.get());
	  MINMAXANDHISTOGRAM();
	}
      else if (m_voxelType == _Float)
	{
	  float *ptr = reinterpret_cast<float*>(tmp.get());
	  MINMAXANDHISTOGRAM();
	}
    }
  fin.close();

//  while(m_histogram.last() == 0)
//    m_histogram.removeLast();
//  while(m_histogram.first() == 0)
//    m_histogram.removeFirst();

  progress.setValue(100);
  if (qApp) qApp->processEvents();
  return true;
}


#define FINDMINMAX()					\
  {							\
    for(qint64 j=0; j<voxelCount; j++)			\
      {							\
	float val = RawFileUtils::finiteValue(ptr[j]);		\
	m_rawMin = qMin(m_rawMin, val);			\
	m_rawMax = qMax(m_rawMax, val);			\
      }							\
  }

bool
RawPlugin::findMinMax()
{
  QProgressDialog progress("Finding Min and Max",
			   QString(),
			   0, 100,
			   0);
  progress.setMinimumDuration(0);


  RawFileUtils::Layout layout;
  QString error;
  if (!RawFileUtils::makeLayout(m_depth, m_width, m_height, m_voxelType,
                                m_skipBytes, layout, error))
    return setError(error, false);
  const qint64 voxelCount = layout.sliceVoxels;
  const qint64 nbytes = layout.sliceBytes;
  std::unique_ptr<uchar[]> tmp(new (std::nothrow)
                               uchar[static_cast<std::size_t>(nbytes)]);
  if (!tmp)
    return setError(QString("Cannot allocate %1 bytes for a RAW slice.")
                      .arg(nbytes), false);

  QFile fin(m_fileName[0]);
  if (!fin.open(QFile::ReadOnly) || !fin.seek(m_skipBytes))
    return setError(QString("Cannot open or seek RAW file %1: %2")
                      .arg(m_fileName[0], fin.errorString()), false);

  m_rawMin = std::numeric_limits<float>::max();
  m_rawMax = std::numeric_limits<float>::lowest();
  for(int i=0; i<m_depth; i++)
    {
      progress.setValue((int)(100.0*(float)i/(float)m_depth));
      if (qApp) qApp->processEvents();

      if (!RawFileUtils::readExact(fin, tmp.get(), nbytes, error))
	return setError(error, false);

      if (m_voxelType == _UChar)
	{
	  uchar *ptr = tmp.get();
	  FINDMINMAX();
	}
      else if (m_voxelType == _Char)
	{
	  qint8 *ptr = reinterpret_cast<qint8*>(tmp.get());
	  FINDMINMAX();
	}
      if (m_voxelType == _UShort)
	{
	  quint16 *ptr = reinterpret_cast<quint16*>(tmp.get());
	  FINDMINMAX();
	}
      else if (m_voxelType == _Short)
	{
	  qint16 *ptr = reinterpret_cast<qint16*>(tmp.get());
	  FINDMINMAX();
	}
      else if (m_voxelType == _Int)
	{
	  qint32 *ptr = reinterpret_cast<qint32*>(tmp.get());
	  FINDMINMAX();
	}
      else if (m_voxelType == _Float)
	{
	  float *ptr = reinterpret_cast<float*>(tmp.get());
	  FINDMINMAX();
	}
    }
  fin.close();

  progress.setValue(100);
  if (qApp) qApp->processEvents();
  return true;
}

#define GENHISTOGRAM()					\
  {							\
    for(qint64 j=0; j<voxelCount; j++)			\
      {							\
	float val = RawFileUtils::finiteValue(ptr[j]);		\
	int idx = RawFileUtils::scaledHistogramIndex(		\
	            val, m_rawMin, m_rawMax, histogramSize); \
	if (idx >= 0 && idx < m_histogram.size() &&		\
	    m_histogram[idx] < std::numeric_limits<uint>::max()) \
	  m_histogram[idx]++;				\
      }							\
  }

void
RawPlugin::generateHistogram()
{
  m_lastError.clear();
  QProgressDialog progress("Generating Histogram",
			   QString(),
			   0, 100,
			   0);
  progress.setMinimumDuration(0);


  RawFileUtils::Layout layout;
  QString error;
  if (!RawFileUtils::makeLayout(m_depth, m_width, m_height, m_voxelType,
                                m_skipBytes, layout, error))
    {
      setError(error, false);
      return;
    }
  const qint64 voxelCount = layout.sliceVoxels;
  const qint64 nbytes = layout.sliceBytes;
  std::unique_ptr<uchar[]> tmp(new (std::nothrow)
                               uchar[static_cast<std::size_t>(nbytes)]);
  if (!tmp)
    {
      setError(QString("Cannot allocate %1 bytes for a RAW slice.")
                 .arg(nbytes), false);
      return;
    }

  QFile fin(m_fileName[0]);
  if (!fin.open(QFile::ReadOnly) || !fin.seek(m_skipBytes))
    {
      setError(QString("Cannot open or seek RAW file %1: %2")
                 .arg(m_fileName[0], fin.errorString()), false);
      return;
    }

  m_histogram.clear();
  if (m_voxelType == _UChar ||
      m_voxelType == _Char ||
      m_voxelType == _UShort ||
      m_voxelType == _Short)
    {
	const int bins = (m_voxelType == _UChar || m_voxelType == _Char) ?
	                   256 : 65536;
      for(int i=0; i<bins; i++)
	m_histogram.append(0);
    }
  else
    {      
      for(int i=0; i<65536; i++)
	m_histogram.append(0);
    }

  int histogramSize = m_histogram.size()-1;
  for(int i=0; i<m_depth; i++)
    {
      progress.setValue((int)(100.0*(float)i/(float)m_depth));
      if (qApp) qApp->processEvents();

      if (!RawFileUtils::readExact(fin, tmp.get(), nbytes, error))
	{
	  setError(error, false);
	  return;
	}

      if (m_voxelType == _UChar)
	{
	  uchar *ptr = tmp.get();
	  GENHISTOGRAM();
	}
      else if (m_voxelType == _Char)
	{
	  qint8 *ptr = reinterpret_cast<qint8*>(tmp.get());
	  GENHISTOGRAM();
	}
      if (m_voxelType == _UShort)
	{
	  quint16 *ptr = reinterpret_cast<quint16*>(tmp.get());
	  GENHISTOGRAM();
	}
      else if (m_voxelType == _Short)
	{
	  qint16 *ptr = reinterpret_cast<qint16*>(tmp.get());
	  GENHISTOGRAM();
	}
      else if (m_voxelType == _Int)
	{
	  qint32 *ptr = reinterpret_cast<qint32*>(tmp.get());
	  GENHISTOGRAM();
	}
      else if (m_voxelType == _Float)
	{
	  float *ptr = reinterpret_cast<float*>(tmp.get());
	  GENHISTOGRAM();
	}
    }
  fin.close();

//  while(m_histogram.last() == 0)
//    m_histogram.removeLast();
//  while(m_histogram.first() == 0)
//    m_histogram.removeFirst();

  progress.setValue(100);
  if (qApp) qApp->processEvents();
}

void
RawPlugin::getDepthSlice(int slc,
			 uchar *slice)
{
  m_lastError.clear();
  RawFileUtils::Layout layout;
  QString error;
  if (!RawFileUtils::makeLayout(m_depth, m_width, m_height, m_voxelType,
                                m_skipBytes, layout, error))
    {
      setError(error, false);
      return;
    }
  const qint64 nbytes = layout.sliceBytes;
  if (!slice)
    {
      setError("RAW slice destination is null.", false);
      return;
    }
  if (slc < 0 || slc >= m_depth)
    {
      memset(slice, 0, static_cast<std::size_t>(nbytes));
      setError(QString("RAW slice %1 is outside [0, %2).")
                 .arg(slc).arg(m_depth), false);
      return;
    }

  qint64 payloadOffset = 0;
  qint64 fileOffset = 0;
  if (!RawFileUtils::checkedMultiply(nbytes, slc, payloadOffset) ||
      !RawFileUtils::checkedAdd(m_skipBytes, payloadOffset, fileOffset) ||
      !RawFileUtils::readAt(m_fileName[0], fileOffset, slice, nbytes, error))
    {
      memset(slice, 0, static_cast<std::size_t>(nbytes));
      setError(error.isEmpty() ? "RAW slice offset overflowed." : error,
               false);
    }
}

//void
//RawPlugin::getWidthSlice(int slc,
//			 uchar *slice)
//{
//  int nbytes = m_depth*m_height*m_bytesPerVoxel;
//  if (slc < 0 || slc >= m_width)
//    {
//      memset(slice, 0, nbytes);
//      return;
//    }
//
//  QFile fin(m_fileName[0]);
//  fin.open(QFile::ReadOnly);
//
//  for(int k=0; k<m_depth; k++)
//    {
//      fin.seek((qint64)(m_skipBytes +
//			((qint64)slc*m_height +
//			 (qint64)k*m_width*m_height*m_bytesPerVoxel)));
//
//      fin.read((char*)(slice+(qint64)(k*m_height*m_bytesPerVoxel)),
//	       (qint64)(m_height*m_bytesPerVoxel));
//    }
//  fin.close();
//}
//
//void
//RawPlugin::getHeightSlice(int slc,
//			  uchar *slice)
//{
//  int nbytes = m_depth*m_width*m_bytesPerVoxel;
//  if (slc < 0 || slc >= m_height)
//    {
//      memset(slice, 0, nbytes);
//      return;
//    }
//
//  QFile fin(m_fileName[0]);
//  fin.open(QFile::ReadOnly);
//  fin.seek(m_skipBytes);
//
//  int ndum = m_width*m_height*m_bytesPerVoxel;
//  uchar *dum = new uchar[ndum];
//  
//  int it=0;
//  for(int k=0; k<m_depth; k++)
//    {
//      fin.read((char*)dum, ndum);
//      for(int j=0; j<m_width; j++)
//	{
//	  memcpy(slice+it*m_bytesPerVoxel,
//		 dum+(j*m_height+slc)*m_bytesPerVoxel,
//		 m_bytesPerVoxel);
//	  it++;
//	}
//    }
//  delete [] dum;
//  fin.close();
//}

QVariant
RawPlugin::rawValue(int d, int w, int h)
{
  m_lastError.clear();
  QVariant v;

  if (d < 0 || d >= m_depth ||
      w < 0 || w >= m_width ||
      h < 0 || h >= m_height)
    {
      v = QVariant("OutOfBounds");
      setError("RAW voxel coordinates are out of bounds.", false);
      return v;
    }

  RawFileUtils::Layout layout;
  QString error;
  if (!RawFileUtils::makeLayout(m_depth, m_width, m_height, m_voxelType,
                                m_skipBytes, layout, error))
    {
      setError(error, false);
      return QVariant("ReadError");
    }

  qint64 depthIndex = 0;
  qint64 rowIndex = 0;
  qint64 voxelIndex = 0;
  qint64 byteIndex = 0;
  qint64 fileOffset = 0;
  if (!RawFileUtils::checkedMultiply(d, layout.sliceVoxels, depthIndex) ||
      !RawFileUtils::checkedMultiply(w, m_height, rowIndex) ||
      !RawFileUtils::checkedAdd(depthIndex, rowIndex, voxelIndex) ||
      !RawFileUtils::checkedAdd(voxelIndex, h, voxelIndex) ||
      !RawFileUtils::checkedMultiply(voxelIndex, m_bytesPerVoxel, byteIndex) ||
      !RawFileUtils::checkedAdd(m_skipBytes, byteIndex, fileOffset))
    {
      setError("RAW voxel offset overflowed.", false);
      return QVariant("ReadError");
    }

  uchar bytes[sizeof(float)] = { 0, 0, 0, 0 };
  if (!RawFileUtils::readAt(m_fileName[0], fileOffset, bytes,
                            m_bytesPerVoxel, error))
    {
      setError(error, false);
      return QVariant("ReadError");
    }

  if (m_voxelType == _UChar)
    {
      unsigned char a = bytes[0];
      v = QVariant((uint)a);
    }
  else if (m_voxelType == _Char)
    {
      qint8 a = 0;
      memcpy(&a, bytes, sizeof(a));
      v = QVariant((int)a);
    }
  else if (m_voxelType == _UShort)
    {
      quint16 a = 0;
      memcpy(&a, bytes, sizeof(a));
      v = QVariant((uint)a);
    }
  else if (m_voxelType == _Short)
    {
      qint16 a = 0;
      memcpy(&a, bytes, sizeof(a));
      v = QVariant((int)a);
    }
  else if (m_voxelType == _Int)
    {
      qint32 a = 0;
      memcpy(&a, bytes, sizeof(a));
      v = QVariant((int)a);
    }
  else if (m_voxelType == _Float)
    {
      float a = 0;
      memcpy(&a, bytes, sizeof(a));
      v = QVariant((double)a);
    }
  return v;
}

//void
//RawPlugin::saveTrimmed(QString trimFile,
//		       int dmin, int dmax,
//		       int wmin, int wmax,
//		       int hmin, int hmax)
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
//  QFile fin(m_fileName[0]);
//  fin.open(QFile::ReadOnly);
//  fin.seek((qint64)(m_skipBytes + nbytes*dmin));
//
//  for(int i=dmin; i<=dmax; i++)
//    {
//      fin.read((char*)tmp, (qint64)(nbytes));
//
//      for(int j=wmin; j<=wmax; j++)
//	{
//	  memcpy(tmp+(j-wmin)*mZ*m_bytesPerVoxel,
//		 tmp+(j*nZ + hmin)*m_bytesPerVoxel,
//		 mZ*m_bytesPerVoxel);
//	}
//	  
//      fout.write((char*)tmp, (qint64)(mY*mZ*m_bytesPerVoxel));
//
//      progress.setValue((int)(100*(float)(i-dmin)/(float)mX));
//      qApp->processEvents();
//    }
//
//  fin.close();
//  fout.close();
//
//  delete [] tmp;
//
//  m_headerBytes = 13; // to be used for applyMapping function
//}
