#include <QtGui>
#include "common.h"
#include "rawslicesplugin.h"
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
RawSlicesPlugin::registerPlugin()
{
  QStringList regString;
  regString << "directory";
  regString << "RAW Slices Directory";
  regString << "*.raw *";
  regString << "files";
  regString << "RAW Slice Files";
  
  return regString;
}

void
RawSlicesPlugin::init()
{
  m_fileName.clear();
  m_imageList.clear();

  m_description.clear();
  m_depth = m_width = m_height = 0;
  m_voxelType = _UChar;
  m_voxelUnit = _Micron;
  m_voxelSizeX = m_voxelSizeY = m_voxelSizeZ = 1;
  m_bytesPerVoxel = 1;
  m_headerBytes = 0;
  m_rawMin = m_rawMax = 0;
  m_histogram.clear();
  m_4dvol = false;
  m_lastError.clear();
  m_lastOperationCanceled = false;
}

void
RawSlicesPlugin::clear()
{
  m_fileName.clear();
  m_imageList.clear();

  m_description.clear();
  m_depth = m_width = m_height = 0;
  m_voxelType = _UChar;
  m_voxelUnit = _Micron;
  m_voxelSizeX = m_voxelSizeY = m_voxelSizeZ = 1;
  m_bytesPerVoxel = 1;
  m_headerBytes = 0;
  m_rawMin = m_rawMax = 0;
  m_histogram.clear();
  m_4dvol = false;
  m_lastError.clear();
  m_lastOperationCanceled = false;
}

void
RawSlicesPlugin::set4DVolume(bool flag)
{
  m_4dvol =  flag;
}

void
RawSlicesPlugin::voxelSize(float& vx, float& vy, float& vz)
  {
    vx = m_voxelSizeX;
    vy = m_voxelSizeY;
    vz = m_voxelSizeZ;
  }
QString RawSlicesPlugin::description() { return m_description; }
int RawSlicesPlugin::voxelType() { return m_voxelType; }
int RawSlicesPlugin::voxelUnit() { return m_voxelUnit; }
int RawSlicesPlugin::headerBytes() { return m_headerBytes; }

void
RawSlicesPlugin::setMinMax(float rmin, float rmax)
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
float RawSlicesPlugin::rawMin() { return m_rawMin; }
float RawSlicesPlugin::rawMax() { return m_rawMax; }
QList<uint> RawSlicesPlugin::histogram() { return m_histogram; }
QString RawSlicesPlugin::lastError() const { return m_lastError; }
bool RawSlicesPlugin::wasCanceled() const { return m_lastOperationCanceled; }

bool
RawSlicesPlugin::setError(const QString& error, bool showDialog)
{
  m_lastError = error;
  if (showDialog)
    QMessageBox::critical(0, "RAW Slices Import Error", error);
  return false;
}

void
RawSlicesPlugin::gridSize(int& d, int& w, int& h)
{
  d = m_depth;
  w = m_width;
  h = m_height;
}

void
RawSlicesPlugin::replaceFile(QString flnm)
{
  m_lastOperationCanceled = false;
  RawFileUtils::Layout layout;
  QString error;
  if (m_depth != 1)
    {
      setError("A multi-file RAW slice stack cannot be replaced by one file.");
      return;
    }
  if (!RawFileUtils::makeLayout(1, m_width, m_height, m_voxelType,
                                m_headerBytes, layout, error) ||
      !RawFileUtils::validateFileSize(flnm, layout.requiredFileBytes, error))
    {
      setError(error);
      return;
    }

  m_lastError.clear();
  m_fileName.clear();
  m_fileName << flnm;
  m_imageList = m_fileName;
}

bool
RawSlicesPlugin::setFile(QStringList files)
{
  m_lastError.clear();
  m_lastOperationCanceled = false;
  if (files.isEmpty() || files[0].isEmpty())
    return setError("No RAW slice files were selected.");

  m_fileName = files;

  m_imageList.clear();

  QFileInfo f(m_fileName[0]);
  if (f.isDir())
    {
      // list all image files in the directory
      QStringList imageNameFilter;
      imageNameFilter << "*";
      QStringList imgfiles= QDir(m_fileName[0]).entryList(imageNameFilter,
							  QDir::NoSymLinks|
							  QDir::NoDotAndDotDot|
							  QDir::Readable|
							  QDir::Files);


      m_imageList.clear();
      for(int i=0; i<imgfiles.size(); i++)
	{
	  QFileInfo fileInfo(m_fileName[0], imgfiles[i]);
	  QString imgfl = fileInfo.absoluteFilePath();
	  m_imageList.append(imgfl);
	}
    }
  else
    m_imageList = files;

  if (m_imageList.isEmpty())
    return setError("The selected RAW slice directory contains no files.");

  // --- load various parameters from the raw file ---
  LoadRawDialog loadRawDialog(0,
			      (char *)m_imageList[0].toUtf8().data());
  loadRawDialog.exec();
  if (loadRawDialog.result() == QDialog::Rejected)
    {
      m_lastOperationCanceled = true;
      m_lastError = "RAW slice import canceled";
      return false;
    }
  
  m_voxelType = loadRawDialog.voxelType();
  m_headerBytes = loadRawDialog.skipHeaderBytes();

  int nX, nY, nZ;
  loadRawDialog.gridSize(nX, nY, nZ);

  m_depth = m_imageList.size();
  m_width = nX;
  m_height = nY;

  if (!RawFileUtils::bytesPerVoxel(m_voxelType, m_bytesPerVoxel))
    return setError(QString("Unsupported RAW voxel type %1.")
                      .arg(m_voxelType));

  RawFileUtils::Layout sliceLayout;
  QString error;
  if (!RawFileUtils::makeLayout(1, m_width, m_height, m_voxelType,
                                m_headerBytes, sliceLayout, error))
    return setError(error);
  for (int i=0; i<m_imageList.count(); ++i)
    if (!RawFileUtils::validateFileSize(m_imageList[i],
                                        sliceLayout.requiredFileBytes, error))
      return setError(error);

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
	  return setError("RAW slice histogram index is invalid.", false); \
	if (m_histogram[idx] < std::numeric_limits<uint>::max()) \
	  m_histogram[idx]++;				\
      }							\
  }

bool
RawSlicesPlugin::findMinMaxandGenerateHistogram()
{
  m_histogram.clear();
  if (m_voxelType == _UChar ||
      m_voxelType == _Char)
    {
      for(uint i=0; i<256; i++)
	m_histogram.append(0);
    }
  else if (m_voxelType == _UShort ||
      m_voxelType == _Short)
    {
      for(uint i=0; i<65536; i++)
	m_histogram.append(0);
    }
  else
    {
      return setError("Unsupported voxel type for exact RAW slice histogram.",
                      false);
    }

  //==================
  // do not calculate histogram
  QStringList items;
  items << "yes" << "no";
  QString yn = QInputDialog::getItem(0, "Histogram",
				     "Want to generate histogram ?",
				     items,
				     0,
				     false);
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

  RawFileUtils::Layout layout;
  QString error;
  if (!RawFileUtils::makeLayout(1, m_width, m_height, m_voxelType,
                                m_headerBytes, layout, error))
    return setError(error, false);
  const qint64 voxelCount = layout.sliceVoxels;
  const qint64 nbytes = layout.sliceBytes;
  std::unique_ptr<uchar[]> tmp(new (std::nothrow)
                               uchar[static_cast<std::size_t>(nbytes)]);
  if (!tmp)
    return setError(QString("Cannot allocate %1 bytes for a RAW slice.")
                      .arg(nbytes), false);

  m_rawMin = std::numeric_limits<float>::max();
  m_rawMax = std::numeric_limits<float>::lowest();

  QProgressDialog progress("Generating Histogram",
			   0,
			   0, 100,
			   0);
  progress.setMinimumDuration(0);

  for(int i=0; i<m_depth; i++)
    {
      progress.setValue((int)(100.0*(float)i/(float)m_depth));
      if (qApp) qApp->processEvents();

      if (!RawFileUtils::readAt(m_imageList[i], m_headerBytes,
                                tmp.get(), nbytes, error))
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
RawSlicesPlugin::findMinMax()
{
  QProgressDialog progress("Finding Min and Max",
			   QString(),
			   0, 100,
			   0);
  progress.setMinimumDuration(0);


  RawFileUtils::Layout layout;
  QString error;
  if (!RawFileUtils::makeLayout(1, m_width, m_height, m_voxelType,
                                m_headerBytes, layout, error))
    return setError(error, false);
  const qint64 voxelCount = layout.sliceVoxels;
  const qint64 nbytes = layout.sliceBytes;
  std::unique_ptr<uchar[]> tmp(new (std::nothrow)
                               uchar[static_cast<std::size_t>(nbytes)]);
  if (!tmp)
    return setError(QString("Cannot allocate %1 bytes for a RAW slice.")
                      .arg(nbytes), false);

  m_rawMin = std::numeric_limits<float>::max();
  m_rawMax = std::numeric_limits<float>::lowest();
  for(int i=0; i<m_depth; i++)
    {
      progress.setValue((int)(100.0*(float)i/(float)m_depth));
      if (qApp) qApp->processEvents();

      if (!RawFileUtils::readAt(m_imageList[i], m_headerBytes,
                                tmp.get(), nbytes, error))
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
RawSlicesPlugin::generateHistogram()
{
  m_lastError.clear();
  QProgressDialog progress("Generating Histogram",
			   QString(),
			   0, 100,
			   0);
  progress.setMinimumDuration(0);


  RawFileUtils::Layout layout;
  QString error;
  if (!RawFileUtils::makeLayout(1, m_width, m_height, m_voxelType,
                                m_headerBytes, layout, error))
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
      for(uint i=0; i<65536; i++)
	m_histogram.append(0);
    }

  int histogramSize = m_histogram.size()-1;
  for(int i=0; i<m_depth; i++)
    {
      progress.setValue((int)(100.0*(float)i/(float)m_depth));
      if (qApp) qApp->processEvents();

      if (!RawFileUtils::readAt(m_imageList[i], m_headerBytes,
                                tmp.get(), nbytes, error))
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
//  while(m_histogram.last() == 0)
//    m_histogram.removeLast();
//  while(m_histogram.first() == 0)
//    m_histogram.removeFirst();

  progress.setValue(100);
  if (qApp) qApp->processEvents();
}

void
RawSlicesPlugin::getDepthSlice(int slc,
			      uchar *slice)
{
  m_lastError.clear();
  RawFileUtils::Layout layout;
  QString error;
  if (!RawFileUtils::makeLayout(1, m_width, m_height, m_voxelType,
                                m_headerBytes, layout, error))
    {
      setError(error, false);
      return;
    }
  if (!slice)
    {
      setError("RAW slice destination is null.", false);
      return;
    }
  if (slc < 0 || slc >= m_depth || slc >= m_imageList.count())
    {
      memset(slice, 0, static_cast<std::size_t>(layout.sliceBytes));
      setError(QString("RAW slice %1 is outside [0, %2).")
                 .arg(slc).arg(m_depth), false);
      return;
    }
  if (!RawFileUtils::readAt(m_imageList[slc], m_headerBytes, slice,
                            layout.sliceBytes, error))
    {
      memset(slice, 0, static_cast<std::size_t>(layout.sliceBytes));
      setError(error, false);
    }
}

//void
//RawSlicesPlugin::getWidthSlice(int slc,
//			       uchar *slice)
//{
//  for(uint i=0; i<m_depth; i++)
//    {
//      QFile fin(m_imageList[i]);
//      fin.open(QFile::ReadOnly);
//      fin.seek(m_headerBytes +
//	       slc*m_height*m_bytesPerVoxel);
//      fin.read((char*)(slice+i*m_height*m_bytesPerVoxel),
//	       m_height*m_bytesPerVoxel);
//      fin.close();
//    }
//}
//
//void
//RawSlicesPlugin::getHeightSlice(int slc,
//				uchar *slice)
//{
//  int ndum = m_width*m_height*m_bytesPerVoxel;
//  uchar *dum = new uchar[ndum];  
//  uint it=0;
//  for(uint i=0; i<m_depth; i++)
//    {
//      QFile fin(m_imageList[i]);
//      fin.open(QFile::ReadOnly);
//      fin.seek(m_headerBytes);
//      fin.read((char*)dum, ndum);
//      fin.close();
//
//      for(uint j=0; j<m_width; j++)
//	{
//	  memcpy(slice+it*m_bytesPerVoxel,
//		 dum+(j*m_height+slc)*m_bytesPerVoxel,
//		 m_bytesPerVoxel);
//	  it++;
//	}
//    }
//  delete [] dum;
//}

QVariant
RawSlicesPlugin::rawValue(int d, int w, int h)
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
  if (!RawFileUtils::makeLayout(1, m_width, m_height, m_voxelType,
                                m_headerBytes, layout, error))
    {
      setError(error, false);
      return QVariant("ReadError");
    }

  qint64 rowIndex = 0;
  qint64 voxelIndex = 0;
  qint64 byteIndex = 0;
  qint64 fileOffset = 0;
  if (!RawFileUtils::checkedMultiply(w, m_height, rowIndex) ||
      !RawFileUtils::checkedAdd(rowIndex, h, voxelIndex) ||
      !RawFileUtils::checkedMultiply(voxelIndex, m_bytesPerVoxel, byteIndex) ||
      !RawFileUtils::checkedAdd(m_headerBytes, byteIndex, fileOffset))
    {
      setError("RAW voxel offset overflowed.", false);
      return QVariant("ReadError");
    }

  uchar bytes[sizeof(float)] = { 0, 0, 0, 0 };
  if (!RawFileUtils::readAt(m_imageList[d], fileOffset, bytes,
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
//RawSlicesPlugin::saveTrimmed(QString trimFile,
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
//  for(uint i=dmin; i<=dmax; i++)
//    {
//      //----------------------------
//      QFile fin(m_imageList[i]);
//      fin.open(QFile::ReadOnly);
//      fin.seek(m_headerBytes);
//      fin.read((char*)tmp, nbytes);
//      fin.close();
//      //----------------------------      
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
//      progress.setValue((int)(100*(float)(i-dmin)/(float)mX));
//      qApp->processEvents();
//    }
//
//  fout.close();
//
//  delete [] tmp;
//
//  m_headerBytes = 13; // to be used for applyMapping function
//}
