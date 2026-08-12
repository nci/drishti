#include <QtGui>
#include "common.h"
#include "rawslabsplugin.h"
#include "../rawfileutils.h"
#include <math.h>
#include <cstring>
#include <memory>
#include <new>

#if defined(Q_OS_WIN32)
#define isnan _isnan
#endif

QStringList
RawSlabsPlugin::registerPlugin()
{
  QStringList regString;
  regString << "files";
  regString << "RAW Slab Files";
  
  return regString;
}

void
RawSlabsPlugin::init()
{
  m_fileName.clear();
  m_slices.clear();

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
RawSlabsPlugin::clear()
{
  m_fileName.clear();
  m_slices.clear();

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
RawSlabsPlugin::set4DVolume(bool flag)
{
  m_4dvol =  flag;
}

void
RawSlabsPlugin::voxelSize(float& vx, float& vy, float& vz)
  {
    vx = m_voxelSizeX;
    vy = m_voxelSizeY;
    vz = m_voxelSizeZ;
  }
QString RawSlabsPlugin::description() { return m_description; }
int RawSlabsPlugin::voxelType() { return m_voxelType; }
int RawSlabsPlugin::voxelUnit() { return m_voxelUnit; }
int RawSlabsPlugin::headerBytes() { return m_headerBytes; }

void
RawSlabsPlugin::setMinMax(float rmin, float rmax)
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
float RawSlabsPlugin::rawMin() { return m_rawMin; }
float RawSlabsPlugin::rawMax() { return m_rawMax; }
QList<uint> RawSlabsPlugin::histogram() { return m_histogram; }
QString RawSlabsPlugin::lastError() const { return m_lastError; }

bool
RawSlabsPlugin::setError(const QString& error, bool showDialog)
{
  m_lastError = error;
  if (showDialog)
    QMessageBox::critical(0, "RAW Slabs Import Error", error);
  return false;
}

void
RawSlabsPlugin::gridSize(int& d, int& w, int& h)
{
  d = m_depth;
  w = m_width;
  h = m_height;
}

void
RawSlabsPlugin::replaceFile(QString flnm)
{
  if (m_fileName.count() != 1 || m_slices.count() != 1)
    {
      setError("A multi-file RAW slab volume cannot be replaced by one file.");
      return;
    }

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
RawSlabsPlugin::setFile(QStringList files)
{
  m_lastError.clear();
  if (files.isEmpty())
    return setError("No RAW slab files were selected.");
  for (int i=0; i<files.count(); ++i)
    if (files[i].isEmpty())
      return setError("The RAW slab file list contains an empty path.");

  m_fileName = files;
  m_slices.clear();

  QFileInfo fi(files[0]);
  const QString hdrflnm = QFileDialog::getOpenFileName(
    0, "Load text header file", fi.absolutePath(), "Files (*.*)");

  QList<int> slabDepths;
  int width = 0;
  int height = 0;
  if (hdrflnm.isEmpty())
    {
      m_headerBytes = m_skipBytes = 13;
      for (int nf=0; nf<m_fileName.count(); ++nf)
	{
	  int voxelType = -1;
	  int depth = 0;
	  int slabWidth = 0;
	  int slabHeight = 0;
	  QString error;
	  if (!RawFileUtils::readEmbeddedHeader(m_fileName[nf], voxelType,
	                                        depth, slabWidth, slabHeight,
	                                        error))
	    return setError(error);
	  if (nf == 0)
	    {
	      m_voxelType = voxelType;
	      width = slabWidth;
	      height = slabHeight;
	    }
	  else if (voxelType != m_voxelType || slabWidth != width ||
	           slabHeight != height)
	    return setError(QString("RAW slab %1 does not match voxel type and "
	                            "slice dimensions of the first slab.")
	                      .arg(m_fileName[nf]));
	  slabDepths.append(depth);
	}
    }
  else
    {
      QFile headerFile(hdrflnm);
      if (!headerFile.open(QFile::ReadOnly | QFile::Text))
	return setError(QString("Cannot open RAW slab text header %1: %2")
	                  .arg(hdrflnm, headerFile.errorString()));
      QTextStream in(&headerFile);
      const QString firstLine = in.readLine().simplified();
      const QStringList firstWords =
	firstLine.split(" ", QString::SkipEmptyParts);
      bool voxelOk = false;
      bool headerOk = false;
      const int voxelCode = firstWords.value(0).toInt(&voxelOk);
      const int headerBytes = firstWords.value(1).toInt(&headerOk);
      if (firstWords.count() < 2 || !voxelOk || !headerOk ||
	  !RawFileUtils::decodeVoxelTypeCode(voxelCode, m_voxelType) ||
	  headerBytes < 0)
	return setError(QString("Invalid RAW slab header first line: %1")
	                  .arg(firstLine));
      m_headerBytes = m_skipBytes = headerBytes;

      while (!in.atEnd())
	{
	  const QString line = in.readLine().simplified();
	  if (line.isEmpty())
	    continue;
	  const QStringList words = line.split(" ", QString::SkipEmptyParts);
	  bool depthOk = false;
	  bool widthOk = false;
	  bool heightOk = false;
	  const int depth = words.value(0).toInt(&depthOk);
	  const int slabWidth = words.value(1).toInt(&widthOk);
	  const int slabHeight = words.value(2).toInt(&heightOk);
	  if (words.count() < 3 || !depthOk || !widthOk || !heightOk)
	    return setError(QString("Invalid RAW slab dimensions line: %1")
	                      .arg(line));
	  if (slabDepths.isEmpty())
	    {
	      width = slabWidth;
	      height = slabHeight;
	    }
	  else if (slabWidth != width || slabHeight != height)
	    return setError(QString("RAW slab dimensions %1 x %2 do not match "
	                            "%3 x %4.")
	                      .arg(slabWidth).arg(slabHeight)
	                      .arg(width).arg(height));
	  slabDepths.append(depth);
	}
      if (slabDepths.count() != m_fileName.count())
	return setError(QString("RAW slab text header describes %1 files, but %2 "
	                        "data files were selected.")
	                  .arg(slabDepths.count()).arg(m_fileName.count()));
    }

  if (!RawFileUtils::bytesPerVoxel(m_voxelType, m_bytesPerVoxel))
    return setError(QString("Unsupported RAW voxel type %1.")
                      .arg(m_voxelType));

  qint64 totalDepth = 0;
  for (int nf=0; nf<m_fileName.count(); ++nf)
    {
      RawFileUtils::Layout layout;
      QString error;
      if (!RawFileUtils::makeLayout(slabDepths[nf], width, height,
	                            m_voxelType, m_skipBytes, layout, error) ||
	  !RawFileUtils::validateFileSize(m_fileName[nf],
	                                  layout.requiredFileBytes, error))
	return setError(error);
      if (!RawFileUtils::checkedAdd(totalDepth, slabDepths[nf], totalDepth) ||
	  totalDepth > std::numeric_limits<int>::max())
	return setError("The combined RAW slab depth exceeds the supported limit.");
      m_slices.append(static_cast<int>(totalDepth));
    }

  m_depth = static_cast<int>(totalDepth);
  m_width = width;
  m_height = height;

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
	  return setError("RAW slab histogram index is invalid.", false); \
	if (m_histogram[idx] < std::numeric_limits<uint>::max()) \
	  m_histogram[idx]++;				\
      }							\
  }


bool
RawSlabsPlugin::findMinMaxandGenerateHistogram()
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
      return setError("Unsupported voxel type for exact RAW slab histogram.",
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


  QProgressDialog progress("Generating Histogram",
			   QString(),
			   0, 100,
			   0);
  progress.setMinimumDuration(0);

  RawFileUtils::Layout layout;
  QString error;
  if (!RawFileUtils::makeLayout(1, m_width, m_height, m_voxelType,
                                m_skipBytes, layout, error))
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

  for (int nf=0; nf<m_fileName.count(); nf++)
    {
      QFile fin(m_fileName[nf]);
      if (!fin.open(QFile::ReadOnly) || !fin.seek(m_skipBytes))
	return setError(QString("Cannot open or seek RAW slab %1: %2")
	                  .arg(m_fileName[nf], fin.errorString()), false);

      int istart = ((nf > 0) ? m_slices[nf-1] : 0);
      int iend = m_slices[nf];
      for(int i=istart; i<iend; i++)
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
RawSlabsPlugin::findMinMax()
{
  QProgressDialog progress("Finding Min and Max",
			   QString(),
			   0, 100,
			   0);
  progress.setMinimumDuration(0);


  RawFileUtils::Layout layout;
  QString error;
  if (!RawFileUtils::makeLayout(1, m_width, m_height, m_voxelType,
                                m_skipBytes, layout, error))
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

  for (int nf=0; nf<m_fileName.count(); nf++)
    {
      QFile fin(m_fileName[nf]);
      if (!fin.open(QFile::ReadOnly) || !fin.seek(m_skipBytes))
	return setError(QString("Cannot open or seek RAW slab %1: %2")
	                  .arg(m_fileName[nf], fin.errorString()), false);

      int istart = ((nf > 0) ? m_slices[nf-1] : 0);
      int iend = m_slices[nf];

      for(int i=istart; i<iend; i++)
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
RawSlabsPlugin::generateHistogram()
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

  for (int nf=0; nf<m_fileName.count(); nf++)
    {
      QFile fin(m_fileName[nf]);
      if (!fin.open(QFile::ReadOnly) || !fin.seek(m_skipBytes))
	{
	  setError(QString("Cannot open or seek RAW slab %1: %2")
	             .arg(m_fileName[nf], fin.errorString()), false);
	  return;
	}

      int istart = ((nf > 0) ? m_slices[nf-1] : 0);
      int iend = m_slices[nf];
      for(int i=istart; i<iend; i++)
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
    }

//  while(m_histogram.last() == 0)
//    m_histogram.removeLast();
//  while(m_histogram.first() == 0)
//    m_histogram.removeFirst();

  progress.setValue(100);
  if (qApp) qApp->processEvents();
}

void
RawSlabsPlugin::getDepthSlice(int slc,
			     uchar *slice)
{
  m_lastError.clear();
  RawFileUtils::Layout layout;
  QString error;
  if (!RawFileUtils::makeLayout(1, m_width, m_height, m_voxelType,
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
      setError(QString("RAW slab slice %1 is outside [0, %2).")
                 .arg(slc).arg(m_depth), false);
      return;
    }
  int fno = -1;
  for(int nf=0; nf<m_slices.count(); nf++)
    if (slc<m_slices[nf])
      {
	fno = nf;
	break;
      }
  if (fno < 0 || fno >= m_fileName.count())
    {
      memset(slice, 0, static_cast<std::size_t>(nbytes));
      setError("RAW slab index table is inconsistent.", false);
      return;
    }
  int slcno = ((fno > 0) ? slc-m_slices[fno-1] : slc);
  qint64 payloadOffset = 0;
  qint64 fileOffset = 0;
  if (!RawFileUtils::checkedMultiply(nbytes, slcno, payloadOffset) ||
      !RawFileUtils::checkedAdd(m_skipBytes, payloadOffset, fileOffset) ||
      !RawFileUtils::readAt(m_fileName[fno], fileOffset, slice, nbytes,
                            error))
    {
      memset(slice, 0, static_cast<std::size_t>(nbytes));
      setError(error.isEmpty() ? "RAW slab slice offset overflowed." : error,
               false);
    }
}

//void
//RawSlabsPlugin::getWidthSlice(int slc,
//			      uchar *slice)
//{
//  int nbytes = m_depth*m_height*m_bytesPerVoxel;
//
//  if (slc < 0 || slc >= m_width)
//    {
//      memset(slice, 0, nbytes);
//      return;
//    }
//
//  QFile fin(m_fileName[0]);
//  fin.open(QFile::ReadOnly);
//  int prevfno = 0;
//  for(uint k=0; k<m_depth; k++)
//    {
//      int fno = 0;
//      for(int nf=0; nf<m_slices.count(); nf++)
//	if (k<m_slices[nf])
//	  {
//	    fno = nf;
//	    break;
//	  }
//      if (fno != prevfno)
//	{
//	  fin.close();
//	  fin.setFileName(m_fileName[fno]);
//	  fin.open(QFile::ReadOnly);
//	}
//      prevfno = fno;
//
//      int slcno = ((fno > 0) ? k-m_slices[fno-1] : k);
//
//      fin.seek((qint64)(m_skipBytes +
//	       ((qint64)slcno*m_width*m_height + 
//		(qint64)slc*m_height)*m_bytesPerVoxel));
//      fin.read((char*)(slice+(qint64)(k*m_height*m_bytesPerVoxel)),
//	       (qint64)(m_height*m_bytesPerVoxel));
//    }
//  fin.close();
//}
//
//void
//RawSlabsPlugin::getHeightSlice(int slc,
//			       uchar *slice)
//{
//  int nbytes = m_depth*m_width*m_bytesPerVoxel;
//  if (slc < 0 || slc >= m_height)
//    {
//      memset(slice, 0, nbytes);
//      return;
//    }
//
//  int ndum = m_width*m_height*m_bytesPerVoxel;
//  uchar *dum = new uchar[ndum];
//  
//  uint it=0;
//  QFile fin(m_fileName[0]);
//  fin.open(QFile::ReadOnly);
//  fin.seek(m_skipBytes);
//  int prevfno = 0;
//  for(uint k=0; k<m_depth; k++)
//    {
//      int fno = 0;
//      for(int nf=0; nf<m_slices.count(); nf++)
//	if (k<m_slices[nf])
//	  {
//	    fno = nf;
//	    break;
//	  }
//      if (fno != prevfno)
//	{
//	  fin.close();
//	  fin.setFileName(m_fileName[fno]);
//	  fin.open(QFile::ReadOnly);
//	  fin.seek(m_skipBytes);
//	}
//      prevfno = fno;
//
//      int slcno = ((fno > 0) ? k-m_slices[fno-1] : k);
//
//      fin.read((char*)dum, (qint64)ndum);
//      for(uint j=0; j<m_width; j++)
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
RawSlabsPlugin::rawValue(int d, int w, int h)
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

  //-----------------------------
  int fno = -1;
  for(int nf=0; nf<m_slices.count(); nf++)
    if (d<m_slices[nf])
      {
	fno = nf;
	break;
      }
  if (fno < 0 || fno >= m_fileName.count())
    {
      setError("RAW slab index table is inconsistent.", false);
      return QVariant("ReadError");
    }
  int slcno = ((fno > 0) ? d-m_slices[fno-1] : d);

  RawFileUtils::Layout layout;
  QString error;
  if (!RawFileUtils::makeLayout(1, m_width, m_height, m_voxelType,
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
  if (!RawFileUtils::checkedMultiply(slcno, layout.sliceVoxels,
                                     depthIndex) ||
      !RawFileUtils::checkedMultiply(w, m_height, rowIndex) ||
      !RawFileUtils::checkedAdd(depthIndex, rowIndex, voxelIndex) ||
      !RawFileUtils::checkedAdd(voxelIndex, h, voxelIndex) ||
      !RawFileUtils::checkedMultiply(voxelIndex, m_bytesPerVoxel, byteIndex) ||
      !RawFileUtils::checkedAdd(m_skipBytes, byteIndex, fileOffset))
    {
      setError("RAW slab voxel offset overflowed.", false);
      return QVariant("ReadError");
    }

  uchar bytes[sizeof(float)] = { 0, 0, 0, 0 };
  if (!RawFileUtils::readAt(m_fileName[fno], fileOffset, bytes,
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
//RawSlabsPlugin::saveTrimmed(QString trimFile,
//			   int dmin, int dmax,
//			   int wmin, int wmax,
//			   int hmin, int hmax)
//{
//  QMessageBox::information(0, "", "not implemented");
//  return;
//}
