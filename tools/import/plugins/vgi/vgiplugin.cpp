#include <QtGui>
#include "common.h"
#include "vgiplugin.h"
#include "../rawfileutils.h"

#include <cmath>
#include <cstring>
#include <limits>
#include <memory>
#include <new>

QStringList
VgiPlugin::registerPlugin()
{
  QStringList regString;
  regString << "files";
  regString << "VGI";
  
  return regString;
}

void
VgiPlugin::init()
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
VgiPlugin::clear()
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
VgiPlugin::set4DVolume(bool flag)
{
  m_4dvol = flag;
}

void
VgiPlugin::voxelSize(float& vx, float& vy, float& vz)
  {
    vx = m_voxelSizeX;
    vy = m_voxelSizeY;
    vz = m_voxelSizeZ;
  }
QString VgiPlugin::description() { return m_description; }
int VgiPlugin::voxelType() { return m_voxelType; }
int VgiPlugin::voxelUnit() { return m_voxelUnit; }
int VgiPlugin::headerBytes() { return m_headerBytes; }

void
VgiPlugin::setMinMax(float rmin, float rmax)
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
float VgiPlugin::rawMin() { return m_rawMin; }
float VgiPlugin::rawMax() { return m_rawMax; }
QList<uint> VgiPlugin::histogram() { return m_histogram; }
QString VgiPlugin::lastError() const { return m_lastError; }

void
VgiPlugin::gridSize(int& d, int& w, int& h)
{
  d = m_depth;
  w = m_width;
  h = m_height;
}

void
VgiPlugin::replaceFile(QString flnm)
{
  m_lastError.clear();
  VgiPlugin candidate;
  candidate.init();
  candidate.set4DVolume(true);
  if (!candidate.setFile(QStringList() << flnm))
    {
      m_lastError = candidate.lastError().isEmpty() ?
        "The replacement VGI volume is invalid." : candidate.lastError();
      return;
    }
  if (candidate.m_depth != m_depth || candidate.m_width != m_width ||
      candidate.m_height != m_height || candidate.m_voxelType != m_voxelType)
    {
      m_lastError = QString("Replacement VGI volume is %1 x %2 x %3, "
                            "voxel-type %4; expected %5 x %6 x %7, type %8.")
                      .arg(candidate.m_depth).arg(candidate.m_width)
                      .arg(candidate.m_height).arg(candidate.m_voxelType)
                      .arg(m_depth).arg(m_width).arg(m_height).arg(m_voxelType);
      return;
    }

  m_fileName = candidate.m_fileName;
  m_hdrFile = candidate.m_hdrFile;
  m_imgFile = candidate.m_imgFile;
  m_byteSwap = candidate.m_byteSwap;
  m_skipBytes = candidate.m_skipBytes;
  m_headerBytes = candidate.m_headerBytes;
}

QString
VgiPlugin::getImgFilename(QString hdrFile)
{
  QFile file(hdrFile);
  if (!file.open(QFile::ReadOnly | QIODevice::Text))
    return QString();
  QTextStream in(&file);
  QString fallbackName;
  bool inFileSection = false;
  while (!in.atEnd())
    {
      const QString line = in.readLine().trimmed();
      if (line.startsWith('[') && line.endsWith(']'))
        {
          inFileSection = line.mid(1, line.size()-2).trimmed()
                            .compare("file1", Qt::CaseInsensitive) == 0;
          continue;
        }
      const int separator = line.indexOf('=');
      if (separator > 0 &&
          line.left(separator).trimmed().compare("name", Qt::CaseInsensitive) == 0)
	{
	  QString imageName = line.mid(separator+1).trimmed();
	  if (imageName.size() >= 2 &&
	      ((imageName.startsWith('"') && imageName.endsWith('"')) ||
	       (imageName.startsWith('\'') && imageName.endsWith('\''))))
	    imageName = imageName.mid(1, imageName.size()-2);
	  if (inFileSection)
	    return imageName;
	  if (fallbackName.isEmpty())
	    fallbackName = imageName;
	}
    }      

  return fallbackName;
}

bool
VgiPlugin::setFile(QStringList files)
{
  m_lastError.clear();
  if (files.isEmpty() || files.first().trimmed().isEmpty())
    {
      m_lastError = "No VGI/VOL volume file was selected.";
      return false;
    }

  m_fileName = QStringList() << QFileInfo(files.first()).absoluteFilePath();

  if (checkExtension(files[0], "vgi"))
    {
      m_hdrFile = m_fileName[0];
      QString imgflnm = getImgFilename(m_hdrFile);
      if (imgflnm.isEmpty())
        {
          m_lastError = "The VGI header does not identify a volume data file.";
          return false;
        }
      QFileInfo info(m_hdrFile);
      QDir direc = info.absoluteDir();
      m_imgFile = direc.absoluteFilePath(imgflnm);
      
      //m_imgFile = m_hdrFile;
      //m_imgFile.chop(3);
      //m_imgFile += "vol";
    }
  else if (checkExtension(files[0], "vol"))
    {
      m_imgFile = m_fileName[0];
      m_hdrFile = m_imgFile;
      m_hdrFile.chop(3);
      m_hdrFile += "vgi";
    }
  else
    {
      m_lastError = "Select a VGI .vgi header or .vol data file.";
      return false;
    }

  if (m_imgFile.isEmpty())
    {
      m_lastError = "The VGI header does not identify a volume data file.";
      return false;
    }

  m_byteSwap = false;
  m_voxelType = -1;

  QFile file(m_hdrFile);
  if (!file.open(QFile::ReadOnly | QIODevice::Text))
    {
      m_lastError = QString("Cannot open VGI header %1: %2")
                      .arg(m_hdrFile, file.errorString());
      return false;
    }
  QTextStream in(&file);
  bool gotDimensions = false;
  bool gotResolution = false;
  bool gotVoxelType = false;
  QString vtp;
  int bpe = 0;
  while (!in.atEnd())
    {
      const QString line = in.readLine();
      const int separator = line.indexOf('=');
      if (separator > 0)
	{
	  const QString key = line.left(separator).simplified().toLower();
	  const QString value = line.mid(separator+1).simplified();
	  if (key == "size")
	    {
	      const QStringList size = value.split(' ', Qt::SkipEmptyParts);
	      if (size.size() == 3)
	        {
	          m_depth = size[2].toInt();
	          m_width = size[1].toInt();
	          m_height = size[0].toInt();
	          gotDimensions = true;
	        }
	    }
	  else if (key == "datatype")
	    {
	      vtp = value.toLower();
	    }
	  else if (key == "bitsperelement")
	    {
	      bpe = value.toInt();
	    }
	  else if (key == "resolution")
	    {
	      const QStringList size = value.split(' ', Qt::SkipEmptyParts);
	      if (size.size() == 3)
	        {
	          m_voxelSizeZ = size[2].toFloat();
	          m_voxelSizeY = size[1].toFloat();
	          m_voxelSizeX = size[0].toFloat();
	          gotResolution = true;
	        }
	    }
	  else if (key == "unit")
	    {
	      if (value.compare("mm", Qt::CaseInsensitive) == 0)
		m_voxelUnit = _Millimeter;
	    }

	  if (!vtp.isEmpty() && bpe > 0)
	    {
	      if (bpe == 8)
		{
		  if (vtp == "unsigned integer")
		    m_voxelType = _UChar;
		  else if (vtp == "integer")
		    m_voxelType = _Char;
		}
	      else if (bpe == 16)
		{
		  if (vtp == "unsigned integer")
		    m_voxelType = _UShort;
		  else if (vtp == "integer")
		    m_voxelType = _Short;

		}
	      else if (bpe == 32)
		{
		  if (vtp == "integer")
		    m_voxelType = _Int;
		  else if (vtp == "float")
		    m_voxelType = _Float;
		}

	      vtp = "";
	      bpe = 0;
	      gotVoxelType = m_voxelType >= _UChar && m_voxelType <= _Float;
	    }
	}

      if (gotDimensions && gotVoxelType && gotResolution)
	break;
    }
  file.close();
      
  m_skipBytes = 0;
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
  if (!gotDimensions || !gotVoxelType ||
      !RawFileUtils::makeLayout(m_depth, m_width, m_height,
                                m_voxelType, m_skipBytes,
                                layout, layoutError) ||
      !RawFileUtils::validateFileSize(m_imgFile,
                                      layout.requiredFileBytes,
                                      layoutError))
    {
      if (layoutError.isEmpty())
        layoutError = "The VGI header is missing size or datatype information.";
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
VgiPlugin::findMinMaxandGenerateHistogram()
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
      m_lastError = QString("Cannot allocate %1 bytes for a VGI slice.")
                      .arg(nbytes);
      m_histogram.clear();
      return;
    }
  uchar *tmp = storage.get();

  QFile fin(m_imgFile);
  if (!fin.open(QFile::ReadOnly) || !fin.seek(m_skipBytes))
    {
      m_lastError = QString("Cannot read VGI volume %1: %2")
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
VgiPlugin::findMinMax()
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
      m_lastError = QString("Cannot allocate %1 bytes for a VGI slice.")
                      .arg(nbytes);
      return;
    }
  uchar *tmp = storage.get();

  QFile fin(m_imgFile);
  if (!fin.open(QFile::ReadOnly) || !fin.seek(m_skipBytes))
    {
      m_lastError = QString("Cannot read VGI volume %1: %2")
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
VgiPlugin::generateHistogram()
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
      m_lastError = QString("Cannot allocate %1 bytes for a VGI slice.")
                      .arg(nbytes);
      m_histogram.clear();
      return;
    }
  uchar *tmp = storage.get();

  QFile fin(m_imgFile);
  if (!fin.open(QFile::ReadOnly) || !fin.seek(m_skipBytes))
    {
      m_lastError = QString("Cannot read VGI volume %1: %2")
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
VgiPlugin::getDepthSlice(int slc,
			    uchar *slice)
{
  m_lastError.clear();
  if (!slice)
    {
      m_lastError = "VGI depth-slice output buffer is null.";
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
      m_lastError = QString("Invalid VGI depth slice %1.").arg(slc);
      return;
    }
  if (!RawFileUtils::readAt(m_imgFile,
                            m_skipBytes+layout.sliceBytes*slc,
                            slice, layout.sliceBytes, error))
    {
      std::memset(slice, 0, static_cast<std::size_t>(layout.sliceBytes));
      m_lastError = error;
    }

  if (m_byteSwap && m_bytesPerVoxel > 1)
    swapbytes(slice, m_bytesPerVoxel, layout.sliceBytes);
}

//void
//VgiPlugin::getWidthSlice(int slc,
//			     uchar *slice)
//{
//  int nbytes = m_depth*m_height*m_bytesPerVoxel;
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
//VgiPlugin::getHeightSlice(int slc,
//			      uchar *slice)
//{
//  int nbytes = m_depth*m_width*m_bytesPerVoxel;
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
VgiPlugin::rawValue(int d, int w, int h)
{
  if (d < 0 || d >= m_depth ||
      w < 0 || w >= m_width ||
      h < 0 || h >= m_height)
    return QVariant("OutOfBounds");

  const qint64 voxelIndex = static_cast<qint64>(d)*m_width*m_height+
                            static_cast<qint64>(w)*m_height+h;
  qint64 byteOffset = 0;
  if (!RawFileUtils::checkedMultiply(voxelIndex, m_bytesPerVoxel,
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
//VgiPlugin::saveTrimmed(QString trimFile,
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
//  QFile fin(m_imgFile);
//  fin.open(QFile::ReadOnly);
//  fin.seek((qint64)(m_skipBytes + nbytes*dmin));
//
//  for(uint i=dmin; i<=dmax; i++)
//    {
//      fin.read((char*)tmp, (qint64)(nbytes));
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
VgiPlugin::checkExtension(QString flnm, const char *ext)
{
  const QFileInfo info(flnm);
  return info.exists() && info.isFile() &&
         info.suffix().compare(QString::fromLatin1(ext),
                               Qt::CaseInsensitive) == 0;
}

void
VgiPlugin::swapbytes(uchar *ptr, int nbytes)
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
VgiPlugin::swapbytes(uchar *ptr, int bpv, int nbytes)
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
