#include "common.h"
#include "analyzeplugin.h"
#include "../rawfileutils.h"

#include <cmath>
#include <cstring>
#include <limits>
#include <memory>
#include <new>

QStringList
AnalyzePlugin::registerPlugin()
{
  QStringList regString;
  regString << "files";
  regString << "Analyze 7.6";
  
  return regString;
}

void
AnalyzePlugin::init()
{
  m_fileName.clear();
  m_hdrFile.clear();
  m_imgFile.clear();

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
AnalyzePlugin::clear()
{
  m_fileName.clear();
  m_hdrFile.clear();
  m_imgFile.clear();

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
AnalyzePlugin::set4DVolume(bool flag)
{
  m_4dvol = flag;
}

void
AnalyzePlugin::voxelSize(float& vx, float& vy, float& vz)
  {
    vx = m_voxelSizeX;
    vy = m_voxelSizeY;
    vz = m_voxelSizeZ;
  }
QString AnalyzePlugin::description() { return m_description; }
int AnalyzePlugin::voxelType() { return m_voxelType; }
int AnalyzePlugin::voxelUnit() { return m_voxelUnit; }
int AnalyzePlugin::headerBytes() { return m_headerBytes; }

void
AnalyzePlugin::setMinMax(float rmin, float rmax)
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
float AnalyzePlugin::rawMin() { return m_rawMin; }
float AnalyzePlugin::rawMax() { return m_rawMax; }
QList<uint> AnalyzePlugin::histogram() { return m_histogram; }
QString AnalyzePlugin::lastError() const { return m_lastError; }

void
AnalyzePlugin::gridSize(int& d, int& w, int& h)
{
  d = m_depth;
  w = m_width;
  h = m_height;
}

void
AnalyzePlugin::replaceFile(QString flnm)
{
  m_lastError.clear();
  AnalyzePlugin candidate;
  candidate.init();
  candidate.set4DVolume(true);
  if (!candidate.setFile(QStringList() << flnm))
    {
      m_lastError = candidate.lastError().isEmpty() ?
        "The replacement Analyze volume is invalid." : candidate.lastError();
      return;
    }
  if (candidate.m_depth != m_depth || candidate.m_width != m_width ||
      candidate.m_height != m_height || candidate.m_voxelType != m_voxelType)
    {
      m_lastError = QString("Replacement Analyze volume is %1 x %2 x %3, "
                            "voxel-type %4; expected %5 x %6 x %7, type %8.")
                      .arg(candidate.m_depth).arg(candidate.m_width)
                      .arg(candidate.m_height).arg(candidate.m_voxelType)
                      .arg(m_depth).arg(m_width).arg(m_height).arg(m_voxelType);
      return;
    }

  m_fileName = candidate.m_fileName;
  m_hdrFile = candidate.m_hdrFile;
  m_imgFile = candidate.m_imgFile;
  m_analyzeHeader = candidate.m_analyzeHeader;
  m_byteSwap = candidate.m_byteSwap;
  m_skipBytes = candidate.m_skipBytes;
  m_headerBytes = candidate.m_headerBytes;
}

bool
AnalyzePlugin::setFile(QStringList files)
{
  m_lastError.clear();
  if (files.isEmpty() || files.first().trimmed().isEmpty())
    {
      m_lastError = "No Analyze 7.6 file was selected.";
      return false;
    }

  m_fileName = files;

  if (checkExtension(files[0], "hdr"))
    {
      m_hdrFile = files[0];
      m_imgFile = m_hdrFile;
      m_imgFile.chop(3);
      m_imgFile += "img";
    }
  else if (checkExtension(files[0], "img"))
    {
      m_imgFile = files[0];
      m_hdrFile = m_imgFile;
      m_hdrFile.chop(3);
      m_hdrFile += "hdr";
    }
  else
    {
      m_lastError = "Select an Analyze .hdr or .img file.";
      return false;
    }

  QFile fin(m_hdrFile);
  if (!fin.open(QFile::ReadOnly))
    {
      m_lastError = QString("Cannot open Analyze header %1: %2")
                      .arg(m_hdrFile, fin.errorString());
      return false;
    }
  std::memset(&m_analyzeHeader, 0, sizeof(m_analyzeHeader));
  QString readError;
  if (!RawFileUtils::readExact(fin, &(m_analyzeHeader.hk), 40, readError) ||
      !RawFileUtils::readExact(fin, &(m_analyzeHeader.dime), 108, readError) ||
      !RawFileUtils::readExact(fin, &(m_analyzeHeader.hist), 200, readError))
    {
      m_lastError = readError;
      return false;
    }
  fin.close();

  m_byteSwap = false;
  if (m_analyzeHeader.hk.sizeof_hdr != 348)
    {
      uchar *sptr = (uchar*)(& m_analyzeHeader.hk.sizeof_hdr);
      swapbytes(sptr, 4);
      if (m_analyzeHeader.hk.sizeof_hdr != 348)
	{
	  m_lastError = QString("Analyze header size is %1, expected 348.")
	                  .arg(m_analyzeHeader.hk.sizeof_hdr);
	  return false;
	}
      else
	{
	  uchar *sptr;
	  
	  for(uint i=0; i<8; i++)
	    {
	      sptr = (uchar*) &(m_analyzeHeader.dime.dim[i]);
	      swapbytes(sptr, 2);
	    }
	  
	  for(uint i=0; i<8; i++)
	    {
	      sptr = (uchar*) &(m_analyzeHeader.dime.pixdim[i]);
	      swapbytes(sptr, 4);
	    }
	  
	  sptr = (uchar*) &(m_analyzeHeader.dime.datatype);
	  swapbytes(sptr, 2);

	  sptr = (uchar*) &(m_analyzeHeader.dime.bitpix);
	  swapbytes(sptr, 2);

	  sptr = (uchar*) &(m_analyzeHeader.dime.vox_offset);
	  swapbytes(sptr, 4);

	  m_byteSwap = true;
	}
      }
  m_voxelType = -1;
  if (m_analyzeHeader.dime.datatype == 0)
    {
      if (m_analyzeHeader.dime.bitpix == 8)
	m_voxelType = _UChar;
      else if (m_analyzeHeader.dime.bitpix == 16)
	m_voxelType = _Short;
      else
	{
	  QStringList dtypes;
	  dtypes << "Int"
		 << "Float";
      
	  QString option = QInputDialog::getItem(0,
						 "Data Type",
	  "Img file does not specify data type. Please choose one. ",
						 dtypes,
						 0,
						 false);
	  
	  if (option == "Int")
	    m_voxelType = _Int;
	  else if (option == "Float")
	    m_voxelType = _Float;
	}
    }
  else
    {
      if (m_analyzeHeader.dime.datatype == 2)
	m_voxelType = _UChar;
      else if (m_analyzeHeader.dime.datatype == 4)
	m_voxelType = _Short;
      else if (m_analyzeHeader.dime.datatype == 8)
	m_voxelType = _Int;
      else if (m_analyzeHeader.dime.datatype == 16)
	m_voxelType = _Float;
    }

  if (m_voxelType == _UChar)
    m_byteSwap = false;
  if (m_voxelType < _UChar || m_voxelType > _Float)
    {
      m_lastError = QString("Unsupported Analyze datatype %1 (%2 bits).")
                      .arg(m_analyzeHeader.dime.datatype)
                      .arg(m_analyzeHeader.dime.bitpix);
      return false;
    }

  const int expectedBits = (m_voxelType == _UChar || m_voxelType == _Char) ? 8 :
                           (m_voxelType == _UShort || m_voxelType == _Short) ? 16 : 32;
  if (m_analyzeHeader.dime.bitpix != expectedBits)
    {
      m_lastError = QString("Analyze datatype %1 requires %2 bits, but the "
                            "header reports %3.")
                      .arg(m_analyzeHeader.dime.datatype).arg(expectedBits)
                      .arg(m_analyzeHeader.dime.bitpix);
      return false;
    }

  const double voxelOffset = m_analyzeHeader.dime.vox_offset;
  if (!std::isfinite(voxelOffset) || voxelOffset < 0 ||
      voxelOffset > std::numeric_limits<int>::max() ||
      std::fabs(voxelOffset-std::round(voxelOffset)) > 0.001)
    {
      m_lastError = QString("Analyze voxel offset %1 is invalid.")
                      .arg(voxelOffset);
      return false;
    }
  m_skipBytes = static_cast<int>(std::llround(voxelOffset));
  
  m_depth = m_analyzeHeader.dime.dim[3];
  m_width = m_analyzeHeader.dime.dim[2];
  m_height = m_analyzeHeader.dime.dim[1];  

  m_voxelSizeX = m_analyzeHeader.dime.pixdim[1];
  m_voxelSizeY = m_analyzeHeader.dime.pixdim[2];
  m_voxelSizeZ = m_analyzeHeader.dime.pixdim[3];

  m_headerBytes = m_skipBytes;
  //------------------------------

  m_bytesPerVoxel = 1;
  if (m_voxelType == _UChar) m_bytesPerVoxel = 1;
  else if (m_voxelType == _Char) m_bytesPerVoxel = 1;
  else if (m_voxelType == _UShort) m_bytesPerVoxel = 2;
  else if (m_voxelType == _Short) m_bytesPerVoxel = 2;
  else if (m_voxelType == _Int) m_bytesPerVoxel = 4;
  else if (m_voxelType == _Float) m_bytesPerVoxel = 4;

  RawFileUtils::Layout layout;
  QString layoutError;
  if (!RawFileUtils::makeLayout(m_depth, m_width, m_height,
                                m_voxelType, m_skipBytes,
                                layout, layoutError) ||
      !RawFileUtils::validateFileSize(m_imgFile,
                                      layout.requiredFileBytes,
                                      layoutError))
    {
      m_lastError = layoutError;
      return false;
    }

  if (!std::isfinite(m_voxelSizeX) || m_voxelSizeX <= 0) m_voxelSizeX = 1;
  if (!std::isfinite(m_voxelSizeY) || m_voxelSizeY <= 0) m_voxelSizeY = 1;
  if (!std::isfinite(m_voxelSizeZ) || m_voxelSizeZ <= 0) m_voxelSizeZ = 1;

  if (m_4dvol)
    return true;

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

#define MINMAXANDHISTOGRAM()				\
  {							\
    for(uint j=0; j<nY*nZ; j++)				\
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
AnalyzePlugin::findMinMaxandGenerateHistogram()
{
  QProgressDialog progress("Generating Histogram",
			   QString(),
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
      for(uint i=0; i<256; i++)
	m_histogram.append(0);
    }
  else if (m_voxelType == _UShort ||
      m_voxelType == _Short)
    {
      if (m_voxelType == _UShort) rMin = 0;
      if (m_voxelType == _Short) rMin = -32768;
      rSize = 65535;
      for(uint i=0; i<65536; i++)
	m_histogram.append(0);
    }
  else
    {
      QMessageBox::information(0, "Error", "Why am i here ???");
      return;
    }

  int nX, nY, nZ;
  nX = m_depth;
  nY = m_width;
  nZ = m_height;

  RawFileUtils::Layout layout;
  QString error;
  if (!RawFileUtils::makeLayout(m_depth, m_width, m_height, m_voxelType,
                                m_skipBytes, layout, error))
    {
      m_lastError = error;
      m_histogram.clear();
      return;
    }
  const int nbytes = static_cast<int>(layout.sliceBytes);
  std::unique_ptr<uchar[]> storage(new (std::nothrow) uchar[nbytes]);
  if (!storage)
    {
      m_lastError = QString("Cannot allocate %1 bytes for an Analyze slice.")
                      .arg(nbytes);
      m_histogram.clear();
      return;
    }
  uchar *tmp = storage.get();

  QFile fin(m_imgFile);
  if (!fin.open(QFile::ReadOnly) || !fin.seek(m_skipBytes))
    {
      m_lastError = QString("Cannot read Analyze image file %1: %2")
                      .arg(m_imgFile, fin.errorString());
      m_histogram.clear();
      return;
    }

  m_rawMin = std::numeric_limits<float>::max();
  m_rawMax = std::numeric_limits<float>::lowest();
  for(uint i=0; i<nX; i++)
    {
      progress.setValue((int)(100.0*(float)i/(float)nX));
      qApp->processEvents();

      if (!RawFileUtils::readExact(fin, tmp, nbytes, error))
        {
          m_lastError = error;
          m_histogram.clear();
          return;
        }

      if (m_byteSwap && m_bytesPerVoxel > 1)
	swapbytes(tmp, m_bytesPerVoxel, nbytes);      

      if (m_voxelType == _UChar)
	{
	  uchar *ptr = tmp;
	  MINMAXANDHISTOGRAM();
	}
      else if (m_voxelType == _Char)
	{
	  char *ptr = (char*) tmp;
	  MINMAXANDHISTOGRAM();
	}
      if (m_voxelType == _UShort)
	{
	  ushort *ptr = (ushort*) tmp;
	  MINMAXANDHISTOGRAM();
	}
      else if (m_voxelType == _Short)
	{
	  short *ptr = (short*) tmp;
	  MINMAXANDHISTOGRAM();
	}
      else if (m_voxelType == _Int)
	{
	  int *ptr = (int*) tmp;
	  MINMAXANDHISTOGRAM();
	}
      else if (m_voxelType == _Float)
	{
	  float *ptr = (float*) tmp;
	  MINMAXANDHISTOGRAM();
	}
    }
  fin.close();

//  while(m_histogram.last() == 0)
//    m_histogram.removeLast();
//  while(m_histogram.first() == 0)
//    m_histogram.removeFirst();

  progress.setValue(100);
  qApp->processEvents();
}


#define FINDMINMAX()					\
  {							\
    for(uint j=0; j<nY*nZ; j++)				\
      {							\
	float val = ptr[j];				\
	if (std::isfinite(static_cast<double>(val)))		\
	  {						\
	    m_rawMin = qMin(m_rawMin, val);		\
	    m_rawMax = qMax(m_rawMax, val);		\
	  }						\
      }							\
  }

void
AnalyzePlugin::findMinMax()
{
  QProgressDialog progress("Finding Min and Max",
			   QString(),
			   0, 100,
			   0);
  progress.setMinimumDuration(0);


  int nX, nY, nZ;
  nX = m_depth;
  nY = m_width;
  nZ = m_height;

  RawFileUtils::Layout layout;
  QString error;
  if (!RawFileUtils::makeLayout(m_depth, m_width, m_height, m_voxelType,
                                m_skipBytes, layout, error))
    {
      m_lastError = error;
      return;
    }
  const int nbytes = static_cast<int>(layout.sliceBytes);
  std::unique_ptr<uchar[]> storage(new (std::nothrow) uchar[nbytes]);
  if (!storage)
    {
      m_lastError = QString("Cannot allocate %1 bytes for an Analyze slice.")
                      .arg(nbytes);
      return;
    }
  uchar *tmp = storage.get();

  QFile fin(m_imgFile);
  if (!fin.open(QFile::ReadOnly) || !fin.seek(m_skipBytes))
    {
      m_lastError = QString("Cannot read Analyze image file %1: %2")
                      .arg(m_imgFile, fin.errorString());
      return;
    }

  m_rawMin = std::numeric_limits<float>::max();
  m_rawMax = std::numeric_limits<float>::lowest();
  for(uint i=0; i<nX; i++)
    {
      progress.setValue((int)(100.0*(float)i/(float)nX));
      qApp->processEvents();

      if (!RawFileUtils::readExact(fin, tmp, nbytes, error))
        {
          m_lastError = error;
          return;
        }

      if (m_byteSwap && m_bytesPerVoxel > 1)
	swapbytes(tmp, m_bytesPerVoxel, nbytes);      

      if (m_voxelType == _UChar)
	{
	  uchar *ptr = tmp;
	  FINDMINMAX();
	}
      else if (m_voxelType == _Char)
	{
	  char *ptr = (char*) tmp;
	  FINDMINMAX();
	}
      if (m_voxelType == _UShort)
	{
	  ushort *ptr = (ushort*) tmp;
	  FINDMINMAX();
	}
      else if (m_voxelType == _Short)
	{
	  short *ptr = (short*) tmp;
	  FINDMINMAX();
	}
      else if (m_voxelType == _Int)
	{
	  int *ptr = (int*) tmp;
	  FINDMINMAX();
	}
      else if (m_voxelType == _Float)
	{
	  float *ptr = (float*) tmp;
	  FINDMINMAX();
	}
    }
  fin.close();

  if (m_rawMin > m_rawMax)
    m_rawMin = m_rawMax = 0;
  progress.setValue(100);
  qApp->processEvents();
}

#define GENHISTOGRAM()					\
  {							\
    for(uint j=0; j<nY*nZ; j++)				\
      {							\
	int idx = RawFileUtils::scaledHistogramIndex(		\
	  static_cast<float>(ptr[j]), m_rawMin, m_rawMax, \
	  histogramSize);					\
	if (idx >= 0) m_histogram[idx]+=1;			\
      }							\
  }

void
AnalyzePlugin::generateHistogram()
{
  QProgressDialog progress("Generating Histogram",
			   QString(),
			   0, 100,
			   0);
  progress.setMinimumDuration(0);


  float rSize = m_rawMax-m_rawMin;

  int nX, nY, nZ;
  nX = m_depth;
  nY = m_width;
  nZ = m_height;

  RawFileUtils::Layout layout;
  QString error;
  if (!RawFileUtils::makeLayout(m_depth, m_width, m_height, m_voxelType,
                                m_skipBytes, layout, error))
    {
      m_lastError = error;
      m_histogram.clear();
      return;
    }
  const int nbytes = static_cast<int>(layout.sliceBytes);
  std::unique_ptr<uchar[]> storage(new (std::nothrow) uchar[nbytes]);
  if (!storage)
    {
      m_lastError = QString("Cannot allocate %1 bytes for an Analyze slice.")
                      .arg(nbytes);
      m_histogram.clear();
      return;
    }
  uchar *tmp = storage.get();

  QFile fin(m_imgFile);
  if (!fin.open(QFile::ReadOnly) || !fin.seek(m_skipBytes))
    {
      m_lastError = QString("Cannot read Analyze image file %1: %2")
                      .arg(m_imgFile, fin.errorString());
      m_histogram.clear();
      return;
    }

  m_histogram.clear();
  if (m_voxelType == _UChar ||
      m_voxelType == _Char ||
      m_voxelType == _UShort ||
      m_voxelType == _Short)
    {
      for(uint i=0; i<rSize+1; i++)
	m_histogram.append(0);
    }
  else
    {      
      for(uint i=0; i<65536; i++)
	m_histogram.append(0);
    }

  int histogramSize = m_histogram.size()-1;
  for(uint i=0; i<nX; i++)
    {
      progress.setValue((int)(100.0*(float)i/(float)nX));
      qApp->processEvents();

      if (!RawFileUtils::readExact(fin, tmp, nbytes, error))
        {
          m_lastError = error;
          m_histogram.clear();
          return;
        }

      if (m_byteSwap && m_bytesPerVoxel > 1)
	swapbytes(tmp, m_bytesPerVoxel, nbytes);      

      if (m_voxelType == _UChar)
	{
	  uchar *ptr = tmp;
	  GENHISTOGRAM();
	}
      else if (m_voxelType == _Char)
	{
	  char *ptr = (char*) tmp;
	  GENHISTOGRAM();
	}
      if (m_voxelType == _UShort)
	{
	  ushort *ptr = (ushort*) tmp;
	  GENHISTOGRAM();
	}
      else if (m_voxelType == _Short)
	{
	  short *ptr = (short*) tmp;
	  GENHISTOGRAM();
	}
      else if (m_voxelType == _Int)
	{
	  int *ptr = (int*) tmp;
	  GENHISTOGRAM();
	}
      else if (m_voxelType == _Float)
	{
	  float *ptr = (float*) tmp;
	  GENHISTOGRAM();
	}
    }
  fin.close();

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
AnalyzePlugin::getDepthSlice(int slc,
			    uchar *slice)
{
  m_lastError.clear();
  if (!slice)
    {
      m_lastError = "Analyze depth-slice output buffer is null.";
      return;
    }

  RawFileUtils::Layout layout;
  QString error;
  if (!RawFileUtils::makeLayout(m_depth, m_width, m_height,
                                m_voxelType, m_skipBytes, layout, error))
    {
      m_lastError = error;
      return;
    }
  if (slc < 0 || slc >= m_depth)
    {
      std::memset(slice, 0, static_cast<std::size_t>(layout.sliceBytes));
      m_lastError = QString("Invalid Analyze depth slice %1.").arg(slc);
      return;
    }

  const qint64 offset = m_skipBytes+layout.sliceBytes*slc;
  if (!RawFileUtils::readAt(m_imgFile, offset, slice,
                            layout.sliceBytes, error))
    {
      std::memset(slice, 0, static_cast<std::size_t>(layout.sliceBytes));
      m_lastError = error;
    }

  if (m_byteSwap && m_bytesPerVoxel > 1)
    swapbytes(slice, m_bytesPerVoxel, layout.sliceBytes);
}

//void
//AnalyzePlugin::getWidthSlice(int slc,
//			     uchar *slice)
//{
//  int nbytes = m_depth*m_height*m_bytesPerVoxel;
//  if (slc < 0 || slc >= m_width)
//    {
//      memset(slice, 0, nbytes);
//      return;
//    }
//
//  QFile fin(m_imgFile);
//  fin.open(QFile::ReadOnly);
//
//  for(int k=0; k<m_depth; k++)
//    {
//      fin.seek((qint64)(m_skipBytes +
//			((qint64)slc*m_height +
//			 (qint64)k*m_width*m_height)*m_bytesPerVoxel));
//
//      fin.read((char*)(slice+(qint64)(k*m_height*m_bytesPerVoxel)),
//	       (qint64)(m_height*m_bytesPerVoxel));
//    }
//  fin.close();
//
//  if (m_byteSwap && m_bytesPerVoxel > 1)
//    swapbytes(slice, m_bytesPerVoxel, nbytes);
//}
//
//void
//AnalyzePlugin::getHeightSlice(int slc,
//			      uchar *slice)
//{
//  int nbytes = m_depth*m_width*m_bytesPerVoxel;
//  if (slc < 0 || slc >= m_height)
//    {
//      memset(slice, 0, nbytes);
//      return;
//    }
//
//  QFile fin(m_imgFile);
//  fin.open(QFile::ReadOnly);
//
//  int ndum = m_width*m_height*m_bytesPerVoxel;
//  uchar *dum = new uchar[ndum];
//  
//  uint it=0;
//  for(uint k=0; k<m_depth; k++)
//    {
//      fin.read((char*)dum, ndum);
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
//
//  if (m_byteSwap && m_bytesPerVoxel > 1)
//    swapbytes(slice, m_bytesPerVoxel, nbytes);
//}

QVariant
AnalyzePlugin::rawValue(int d, int w, int h)
{
  if (d < 0 || d >= m_depth ||
      w < 0 || w >= m_width ||
      h < 0 || h >= m_height)
    return QVariant("OutOfBounds");

  qint64 planeVoxels = 0;
  qint64 voxelIndex = 0;
  qint64 byteOffset = 0;
  qint64 rowOffset = 0;
  if (!RawFileUtils::checkedMultiply(m_width, m_height, planeVoxels) ||
      !RawFileUtils::checkedMultiply(d, planeVoxels, voxelIndex) ||
      !RawFileUtils::checkedMultiply(w, m_height, rowOffset) ||
      !RawFileUtils::checkedAdd(voxelIndex, rowOffset, voxelIndex) ||
      !RawFileUtils::checkedAdd(voxelIndex, h, voxelIndex) ||
      !RawFileUtils::checkedMultiply(voxelIndex, m_bytesPerVoxel,
                                     byteOffset) ||
      !RawFileUtils::checkedAdd(byteOffset, m_skipBytes, byteOffset))
    return QVariant("ReadError");

  alignas(4) uchar bytes[4] = { 0, 0, 0, 0 };
  QString error;
  if (!RawFileUtils::readAt(m_imgFile, byteOffset, bytes,
                            m_bytesPerVoxel, error))
    return QVariant("ReadError");
  if (m_byteSwap && m_bytesPerVoxel > 1)
    swapbytes(bytes, m_bytesPerVoxel, m_bytesPerVoxel);

  if (m_voxelType == _UChar)
    {
      return QVariant(static_cast<uint>(bytes[0]));
    }
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
//AnalyzePlugin::saveTrimmed(QString trimFile,
//			   int dmin, int dmax,
//			   int wmin, int wmax,
//			   int hmin, int hmax)
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
//  qint64 nbytes = nY*nZ*m_bytesPerVoxel;
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
//  QFile fin(m_imgFile);
//  fin.open(QFile::ReadOnly);
//  fin.seek((qint64)(m_skipBytes + nbytes*dmin));
//
//  for(uint i=dmin; i<=dmax; i++)
//    {
//      fin.read((char*)tmp, nbytes);
//
//      for(uint j=wmin; j<=wmax; j++)
//	{
//	  memcpy(tmp+(j-wmin)*mZ*m_bytesPerVoxel,
//		 tmp+(j*nZ + hmin)*m_bytesPerVoxel,
//		 mZ*m_bytesPerVoxel);
//	}
//	  
//      if (m_byteSwap && m_bytesPerVoxel > 1)
//	swapbytes(tmp,
//		  m_bytesPerVoxel,
//		  mY*mZ*m_bytesPerVoxel);      
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

bool
AnalyzePlugin::checkExtension(QString flnm, const char *ext)
{
  const QFileInfo info(flnm);
  return info.exists() && info.isFile() &&
         info.suffix().compare(QString::fromLatin1(ext),
                               Qt::CaseInsensitive) == 0;
}

void
AnalyzePlugin::swapbytes(uchar *ptr, int nbytes)
{
  for(uint i=0; i<nbytes/2; i++)
    {
      uchar t;
      t = ptr[i];
      ptr[i] = ptr[nbytes-1-i];
      ptr[nbytes-1-i] = t;
    }
}

void
AnalyzePlugin::swapbytes(uchar *ptr, int bpv, int nbytes)
{
  int nb = nbytes/bpv;
  for(uint j=0; j<nb; j++)
    {
      uchar *p = ptr + bpv*j;
      for(uint i=0; i<bpv/2; i++)
	{
	  uchar t;
	  t = p[i];
	  p[i] = p[bpv-1-i];
	  p[bpv-1-i] = t;
	}
    }

}
