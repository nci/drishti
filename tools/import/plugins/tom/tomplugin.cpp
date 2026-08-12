#include <QtGui>
#include "common.h"
#include "tomplugin.h"
#include "../rawfileutils.h"

#include <cmath>
#include <cstring>
#include <memory>
#include <new>

namespace
{
template<std::size_t Size>
QString fixedTomString(const char (&text)[Size])
{
  int length = 0;
  while (length < static_cast<int>(Size) && text[length] != '\0')
    ++length;
  return QString::fromLocal8Bit(text, length).trimmed();
}
}

QStringList
TomPlugin::registerPlugin()
{
  QStringList regString;
  regString << "files";
  regString << "QMUL Tom Files";
  
  return regString;
}

void
TomPlugin::init()
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
TomPlugin::clear()
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
TomPlugin::set4DVolume(bool flag)
{
  m_4dvol = flag;
}

void
TomPlugin::voxelSize(float& vx, float& vy, float& vz)
  {
    vx = m_voxelSizeX;
    vy = m_voxelSizeY;
    vz = m_voxelSizeZ;
  }
QString TomPlugin::description() { return m_description; }
int TomPlugin::voxelType() { return m_voxelType; }
int TomPlugin::voxelUnit() { return m_voxelUnit; }
int TomPlugin::headerBytes() { return m_headerBytes; }

void
TomPlugin::setMinMax(float rmin, float rmax)
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
float TomPlugin::rawMin() { return m_rawMin; }
float TomPlugin::rawMax() { return m_rawMax; }
QList<uint> TomPlugin::histogram() { return m_histogram; }
QString TomPlugin::lastError() const { return m_lastError; }

void
TomPlugin::gridSize(int& d, int& w, int& h)
{
  d = m_depth;
  w = m_width;
  h = m_height;
}

void
TomPlugin::replaceFile(QString flnm)
{
  m_lastError.clear();
  const QString candidateFile = QFileInfo(flnm).absoluteFilePath();
  QFile input(candidateFile);
  if (!input.open(QFile::ReadOnly))
    {
      m_lastError = QString("Cannot open replacement TOM volume %1: %2")
                      .arg(candidateFile, input.errorString());
      return;
    }
  thead candidateHeader;
  std::memset(&candidateHeader, 0, sizeof(candidateHeader));
  QString error;
  if (!RawFileUtils::readExact(input, &candidateHeader, 512, error))
    {
      m_lastError = error;
      return;
    }
  input.close();
  if (candidateHeader.zsize != m_depth || candidateHeader.ysize != m_width ||
      candidateHeader.xsize != m_height)
    {
      m_lastError = QString("Replacement TOM volume is %1 x %2 x %3; "
                            "expected %4 x %5 x %6.")
                      .arg(candidateHeader.zsize).arg(candidateHeader.ysize)
                      .arg(candidateHeader.xsize).arg(m_depth).arg(m_width)
                      .arg(m_height);
      return;
    }
  RawFileUtils::Layout layout;
  if (!RawFileUtils::makeLayout(m_depth, m_width, m_height, m_voxelType,
                                m_skipBytes, layout, error) ||
      !RawFileUtils::validateFileSize(candidateFile, layout.requiredFileBytes,
                                      error))
    {
      m_lastError = error;
      return;
    }
  m_fileName = QStringList() << candidateFile;
  m_tHead = candidateHeader;
}

bool
TomPlugin::setFile(QStringList files)
{
  m_lastError.clear();
  if (files.isEmpty() || files.first().trimmed().isEmpty())
    {
      m_lastError = "No TOM volume file was selected.";
      return false;
    }

  const QString candidateFile = QFileInfo(files.first()).absoluteFilePath();

  QFile fin(candidateFile);
  if (!fin.open(QFile::ReadOnly))
    {
      m_lastError = QString("Cannot open TOM volume %1: %2")
                      .arg(candidateFile, fin.errorString());
      return false;
    }
  thead candidateHeader;
  std::memset(&candidateHeader, 0, sizeof(candidateHeader));
  QString readError;
  if (!RawFileUtils::readExact(fin, &candidateHeader, 512, readError))
    {
      m_lastError = readError;
      return false;
    }
  fin.close();
  

  m_description = QString("%1 %2 %3 %4 %5 %6 %7").	\
    arg(fixedTomString(candidateHeader.owner)).		\
    arg(fixedTomString(candidateHeader.user)).		\
    arg(fixedTomString(candidateHeader.specimen)).	\
    arg(fixedTomString(candidateHeader.scan)).		\
    arg(fixedTomString(candidateHeader.comment)).	\
    arg(fixedTomString(candidateHeader.time)).		\
    arg(fixedTomString(candidateHeader.duration));

  const float pixelSize = std::isfinite(candidateHeader.pixel_size) &&
                          candidateHeader.pixel_size > 0 ?
                            candidateHeader.pixel_size : 1.0f;
  m_voxelSizeX = m_voxelSizeY = m_voxelSizeZ = pixelSize;
  m_voxelType = _UChar;
  m_bytesPerVoxel = 1;
  m_skipBytes = 512;
  m_headerBytes = m_skipBytes;
  m_depth = candidateHeader.zsize;
  m_width = candidateHeader.ysize;
  m_height = candidateHeader.xsize;

  RawFileUtils::Layout layout;
  QString layoutError;
  if (!RawFileUtils::makeLayout(m_depth, m_width, m_height,
                                m_voxelType, m_skipBytes,
                                layout, layoutError) ||
      !RawFileUtils::validateFileSize(candidateFile,
                                      layout.requiredFileBytes,
                                      layoutError))
    {
      m_lastError = layoutError;
      return false;
    }

  m_fileName = QStringList() << candidateFile;
  m_tHead = candidateHeader;

  m_rawMin = 0;
  m_rawMax = 255;
  generateHistogram();

  return m_lastError.isEmpty() && !m_histogram.isEmpty();
}

#define GENHISTOGRAM()					\
  {							\
    for(qint64 j=0; j<layout.sliceVoxels; j++)		\
      {							\
	float fidx = (ptr[j]-m_rawMin)/rSize;		\
	fidx = qBound(0.0f, fidx, 1.0f);		\
	int idx = fidx*histogramSize;			\
	m_histogram[idx]+=1;				\
      }							\
  }

void
TomPlugin::generateHistogram()
{
  float rSize = m_rawMax-m_rawMin;

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
      m_histogram.clear();
      for(uint i=0; i<rSize+1; i++)
	m_histogram.append(0);
      return;
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
    {
      m_lastError = error;
      m_histogram.clear();
      return;
    }
  std::unique_ptr<uchar[]> tmp(new (std::nothrow)
                                uchar[static_cast<std::size_t>(layout.sliceBytes)]);
  if (!tmp)
    {
      m_lastError = QString("Cannot allocate %1 bytes for a TOM slice.")
                      .arg(layout.sliceBytes);
      m_histogram.clear();
      return;
    }

  m_histogram.clear();
  for(uint i=0; i<rSize+1; i++)
    m_histogram.append(0);

  int histogramSize = m_histogram.size()-1;
  for(int i=0; i<m_depth; i++)
    {
      progress.setValue((int)(100.0*(float)i/(float)m_depth));
      qApp->processEvents();

      if (!RawFileUtils::readAt(m_fileName.first(),
                                m_skipBytes+layout.sliceBytes*i, tmp.get(),
                                layout.sliceBytes, error))
        {
          m_lastError = error;
          m_histogram.clear();
          return;
        }

      if (m_voxelType == _UChar)
	{
	  uchar *ptr = tmp.get();
	  GENHISTOGRAM();
	}
      else if (m_voxelType == _Char)
	{
	  char *ptr = reinterpret_cast<char*>(tmp.get());
	  GENHISTOGRAM();
	}
      if (m_voxelType == _UShort)
	{
	  ushort *ptr = reinterpret_cast<ushort*>(tmp.get());
	  GENHISTOGRAM();
	}
      else if (m_voxelType == _Short)
	{
	  short *ptr = reinterpret_cast<short*>(tmp.get());
	  GENHISTOGRAM();
	}
      else if (m_voxelType == _Int)
	{
	  int *ptr = reinterpret_cast<int*>(tmp.get());
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
  qApp->processEvents();
}

void
TomPlugin::getDepthSlice(int slc,
			      uchar *slice)
{
  m_lastError.clear();
  if (!slice)
    {
      m_lastError = "TOM depth-slice output buffer is null.";
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
      m_lastError = QString("Invalid TOM depth slice %1.").arg(slc);
      return;
    }
  if (!RawFileUtils::readAt(m_fileName[0],
                            m_skipBytes+layout.sliceBytes*slc,
                            slice, layout.sliceBytes, error))
    {
      std::memset(slice, 0, static_cast<std::size_t>(layout.sliceBytes));
      m_lastError = error;
    }
}

//void
//TomPlugin::getWidthSlice(int slc,
//			 uchar *slice)
//{
//  QFile fin(m_fileName[0]);
//  fin.open(QFile::ReadOnly);
//
//  for(uint k=0; k<m_depth; k++)
//    {
//      fin.seek((qint64)(m_skipBytes +
//			((qint64)slc*m_height +
//			 (qint64)k*m_width*m_height)*m_bytesPerVoxel));
//
//      fin.read((char*)(slice+(qint64)(k*m_height*m_bytesPerVoxel)),
//	       (qint64)(m_height*m_bytesPerVoxel));
//
//    }
//  fin.close();
//}
//
//void
//TomPlugin::getHeightSlice(int slc,
//			  uchar *slice)
//{
//  QFile fin(m_fileName[0]);
//  fin.open(QFile::ReadOnly);
//  fin.seek(m_skipBytes);
//
//  int ndum = m_width*m_height*m_bytesPerVoxel;
//  uchar *dum = new uchar[ndum];
//  
//  uint it=0;
//  for(uint k=0; k<m_depth; k++)
//    {
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
TomPlugin::rawValue(int d, int w, int h)
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

  uchar value = 0;
  QString error;
  if (!RawFileUtils::readAt(m_fileName[0], byteOffset, &value, 1, error))
    {
      m_lastError = error;
      return QVariant("ReadError");
    }
  return QVariant(static_cast<uint>(value));
}

//void
//TomPlugin::saveTrimmed(QString trimFile,
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
//  uchar vt = 0; // unsigned byte
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
