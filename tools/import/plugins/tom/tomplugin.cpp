#include <QtGui>
#include "common.h"
#include "tomplugin.h"

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
  m_bytesPerVoxel = 1;
  m_rawMin = m_rawMax = 0;
  m_histogram.clear();
  m_4dvol = false;
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
  m_bytesPerVoxel = 1;
  m_rawMin = m_rawMax = 0;
  m_histogram.clear();
  m_4dvol = false;
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
  m_fileName.clear();
  m_fileName << flnm;
}

bool
TomPlugin::setFile(QStringList files)
{
  m_fileName = files;

  QFile fin(m_fileName[0]);
  fin.open(QFile::ReadOnly);
  fin.read((char*)&m_tHead, 512);
  fin.close();
  

  m_description = QString("%1 %2 %3 %4 %5 %6 %7").	\
    arg(m_tHead.owner).					\
    arg(m_tHead.user).					\
    arg(m_tHead.specimen).				\
    arg(m_tHead.scan).					\
    arg(m_tHead.comment).				\
    arg(m_tHead.time).					\
    arg(m_tHead.duration);

  m_voxelSizeX = m_tHead.pixel_size;
  m_voxelSizeY = m_tHead.pixel_size;
  m_voxelSizeZ = m_tHead.pixel_size;
  m_voxelType = _UChar;
  m_bytesPerVoxel = 1;
  m_skipBytes = 512;
  m_headerBytes = m_skipBytes;
  m_depth = m_tHead.zsize;
  m_width = m_tHead.ysize;
  m_height = m_tHead.xsize;

  m_rawMin = 0;
  m_rawMax = 255;
  generateHistogram();

  return true;
}

#define GENHISTOGRAM()					\
  {							\
    for(uint j=0; j<nY*nZ; j++)				\
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
			   "Cancel",
			   0, 100,
			   0);
  progress.setMinimumDuration(0);


  int nX, nY, nZ;
  nX = m_depth;
  nY = m_width;
  nZ = m_height;

  int nbytes = nY*nZ*m_bytesPerVoxel;
  uchar *tmp = new uchar[nbytes];

  QFile fin(m_fileName[0]);
  fin.open(QFile::ReadOnly);
  fin.seek(m_skipBytes);

  m_histogram.clear();
  for(uint i=0; i<rSize+1; i++)
    m_histogram.append(0);

  int histogramSize = m_histogram.size()-1;
  for(uint i=0; i<nX; i++)
    {
      progress.setValue((int)(100.0*(float)i/(float)nX));
      qApp->processEvents();


      fin.read((char*)tmp, nbytes);

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

  delete [] tmp;

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
  int nbytes = m_width*m_height*m_bytesPerVoxel;
  QFile fin(m_fileName[0]);
  fin.open(QFile::ReadOnly);
  fin.seek((qint64)(m_skipBytes + nbytes*slc));
  fin.read((char*)slice, (qint64)nbytes);
  fin.close();
}

void
TomPlugin::getWidthSlice(int slc,
			 uchar *slice)
{
  QFile fin(m_fileName[0]);
  fin.open(QFile::ReadOnly);

  for(uint k=0; k<m_depth; k++)
    {
      fin.seek((qint64)(m_skipBytes +
			((qint64)slc*m_height +
			 (qint64)k*m_width*m_height)*m_bytesPerVoxel));

      fin.read((char*)(slice+(qint64)(k*m_height*m_bytesPerVoxel)),
	       (qint64)(m_height*m_bytesPerVoxel));

    }
  fin.close();
}

void
TomPlugin::getHeightSlice(int slc,
			  uchar *slice)
{
  QFile fin(m_fileName[0]);
  fin.open(QFile::ReadOnly);
  fin.seek(m_skipBytes);

  int ndum = m_width*m_height*m_bytesPerVoxel;
  uchar *dum = new uchar[ndum];
  
  uint it=0;
  for(uint k=0; k<m_depth; k++)
    {
      fin.read((char*)dum, (qint64)ndum);
      for(uint j=0; j<m_width; j++)
	{
	  memcpy(slice+it*m_bytesPerVoxel,
		 dum+(j*m_height+slc)*m_bytesPerVoxel,
		 m_bytesPerVoxel);
	  it++;
	}
    }
  delete [] dum;
  fin.close();
}

QVariant
TomPlugin::rawValue(int d, int w, int h)
{
  QVariant v;

  if (d < 0 || d >= m_depth ||
      w < 0 || w >= m_width ||
      h < 0 || h >= m_height)
    {
      v = QVariant("OutOfBounds");
      return v;
    }

  QFile fin(m_fileName[0]);
  fin.open(QFile::ReadOnly);
  fin.seek((qint64)(m_skipBytes +
	   m_bytesPerVoxel*(d*m_width*m_height +
			    w*m_height +
			    h)));

  unsigned char val;
  fin.read((char*)&val, (qint64)m_bytesPerVoxel);
  v = QVariant((uint)val);
  fin.close();

  return v;
}

void
TomPlugin::saveTrimmed(QString trimFile,
		       int dmin, int dmax,
		       int wmin, int wmax,
		       int hmin, int hmax)
{
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

  int nbytes = nY*nZ*m_bytesPerVoxel;
  uchar *tmp = new uchar[nbytes];

  uchar vt = 0; // unsigned byte
  
  QFile fout(trimFile);
  fout.open(QFile::WriteOnly);

  fout.write((char*)&vt, 1);
  fout.write((char*)&mX, 4);
  fout.write((char*)&mY, 4);
  fout.write((char*)&mZ, 4);

  QFile fin(m_fileName[0]);
  fin.open(QFile::ReadOnly);
  fin.seek((qint64)(m_skipBytes + nbytes*dmin));

  for(uint i=dmin; i<=dmax; i++)
    {
      fin.read((char*)tmp, (qint64)(nbytes));

      for(uint j=wmin; j<=wmax; j++)
	{
	  memcpy(tmp+(j-wmin)*mZ*m_bytesPerVoxel,
		 tmp+(j*nZ + hmin)*m_bytesPerVoxel,
		 mZ*m_bytesPerVoxel);
	}
	  
      fout.write((char*)tmp, (qint64)(mY*mZ*m_bytesPerVoxel));

      progress.setValue((int)(100*(float)(i-dmin)/(float)mX));
      qApp->processEvents();
    }

  fin.close();
  fout.close();

  delete [] tmp;

  m_headerBytes = 13; // to be used for applyMapping function
}
