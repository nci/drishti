#include <QtGui>
#include "common.h"
#include "metaimageplugin.h"
#include <iostream>
#include <itkImage.h>
#include <itkImageFileWriter.h>
#include <itkImageFileReader.h>
#include <itkMetaImageIO.h>
#include <itkImageRegionIterator.h>


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
  m_bytesPerVoxel = 1;
  m_rawMin = m_rawMax = 0;
  m_histogram.clear();
  m_4dvol = false;
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
  m_bytesPerVoxel = 1;
  m_rawMin = m_rawMax = 0;
  m_histogram.clear();
  m_4dvol = false;
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
  m_fileName.clear();
  m_fileName << flnm;
}

bool
MetaImagePlugin::setFile(QStringList files)
{
  m_fileName = files;

  MetaImage metaImageReader(m_fileName[0].toUtf8().data());
  int *dims;
  dims = (int*)metaImageReader.DimSize();
  m_height = dims[0];
  m_width = dims[1];
  m_depth = dims[2];

  float *esize = (float*)metaImageReader.ElementSpacing();
  m_voxelSizeX = esize[0];
  m_voxelSizeY = esize[1];
  m_voxelSizeZ = esize[2];

  int et = metaImageReader.ElementType();
  if (et == MET_UCHAR) m_voxelType = _UChar;
  if (et == MET_CHAR) m_voxelType = _Char;
  if (et == MET_USHORT) m_voxelType = _UShort;
  if (et == MET_SHORT) m_voxelType = _Short;
  if (et == MET_INT) m_voxelType = _Int;
  if (et == MET_FLOAT) m_voxelType = _Float;

  m_skipBytes = m_headerBytes = 0;

  m_bytesPerVoxel = 1;
  if (m_voxelType == _UChar) m_bytesPerVoxel = 1;
  else if (m_voxelType == _Char) m_bytesPerVoxel = 1;
  else if (m_voxelType == _UShort) m_bytesPerVoxel = 2;
  else if (m_voxelType == _Short) m_bytesPerVoxel = 2;
  else if (m_voxelType == _Int) m_bytesPerVoxel = 4;
  else if (m_voxelType == _Float) m_bytesPerVoxel = 4;

  if (m_4dvol) // do not perform further calculations.
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

  return true;
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
MetaImagePlugin::findMinMaxandGenerateHistogram()
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
      if (m_voxelType == _Char) rMin = -127;
      rSize = 255;
      for(int i=0; i<256; i++)
	m_histogram.append(0);
    }
  else if (m_voxelType == _UShort ||
      m_voxelType == _Short)
    {
      if (m_voxelType == _UShort) rMin = 0;
      if (m_voxelType == _Short) rMin = -32767;
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
  uchar *tmp = new uchar[nbytes];

  MetaImage metaImageReader(m_fileName[0].toUtf8().data());
  int mind[3];
  int maxd[3];
  mind[0] = mind[1] = mind[2] = 0;
  maxd[0] = m_height-1;
  maxd[1] = m_width-1;
  maxd[2] = 0;

  m_rawMin = 10000000;
  m_rawMax = -10000000;
  for(int i=0; i<m_depth; i++)
    {
      progress.setValue((int)(100.0*(float)i/(float)m_depth));
      qApp->processEvents();

      mind[2] = i;
      maxd[2] = i;
      metaImageReader.ReadROI(mind, maxd, NULL, true, tmp, 1);
 
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

  delete [] tmp;

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
	float val = ptr[j];				\
	m_rawMin = qMin(m_rawMin, val);			\
	m_rawMax = qMax(m_rawMax, val);			\
      }							\
  }

void
MetaImagePlugin::findMinMax()
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

  int nbytes = nY*nZ*m_bytesPerVoxel;
  uchar *tmp = new uchar[nbytes];

  MetaImage metaImageReader(m_fileName[0].toUtf8().data());
  int mind[3];
  int maxd[3];
  mind[0] = mind[1] = mind[2] = 0;
  maxd[0] = m_height-1;
  maxd[1] = m_width-1;
  maxd[2] = 0;

  m_rawMin = 10000000;
  m_rawMax = -10000000;
  for(int i=0; i<nX; i++)
    {
      progress.setValue((int)(100.0*(float)i/(float)nX));
      qApp->processEvents();

      mind[2] = i;
      maxd[2] = i;
      metaImageReader.ReadROI(mind, maxd, NULL, true, tmp, 1);

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

  delete [] tmp;

  progress.setValue(100);
  qApp->processEvents();
}

#define GENHISTOGRAM()					\
  {							\
    for(int j=0; j<nY*nZ; j++)				\
      {							\
	float fidx = (ptr[j]-m_rawMin)/rSize;		\
	fidx = qBound(0.0f, fidx, 1.0f);		\
	int idx = fidx*histogramSize;			\
	m_histogram[idx]+=1;				\
      }							\
  }

void
MetaImagePlugin::generateHistogram()
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

  int nbytes = nY*nZ*m_bytesPerVoxel;
  uchar *tmp = new uchar[nbytes];

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

  MetaImage metaImageReader(m_fileName[0].toUtf8().data());
  int mind[3];
  int maxd[3];
  mind[0] = mind[1] = mind[2] = 0;
  maxd[0] = m_height-1;
  maxd[1] = m_width-1;
  maxd[2] = 0;

  for(int i=0; i<nX; i++)
    {
      progress.setValue((int)(100.0*(float)i/(float)nX));
      qApp->processEvents();

      mind[2] = i;
      maxd[2] = i;
      metaImageReader.ReadROI(mind, maxd, NULL, true, tmp, 1);

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

  delete [] tmp;

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
MetaImagePlugin::getDepthSlice(int slc,
			 uchar *slice)
{
  int nbytes = m_width*m_height*m_bytesPerVoxel;
  if (slc < 0 || slc >= m_depth)
    {
      memset(slice, 0, nbytes);
      return;
    }

  MetaImage metaImageReader(m_fileName[0].toUtf8().data());
  int mind[3];
  int maxd[3];
  mind[0] = 0;
  mind[1] = 0;
  mind[2] = slc;
  maxd[0] = m_height-1;
  maxd[1] = m_width-1;
  maxd[2] = slc;
  metaImageReader.ReadROI(mind, maxd, NULL, true, slice, 1);
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
  QVariant v;

  if (d < 0 || d >= m_depth ||
      w < 0 || w >= m_width ||
      h < 0 || h >= m_height)
    {
      v = QVariant("OutOfBounds");
      return v;
    }

  MetaImage metaImageReader(m_fileName[0].toUtf8().data());
  int mind[3];
  int maxd[3];
  mind[0] = h;
  mind[1] = w;
  mind[2] = d;
  maxd[0] = h;
  maxd[1] = w;
  maxd[2] = d;

  if (m_voxelType == _UChar)
    {
      unsigned char a;
      metaImageReader.ReadROI(mind, maxd, NULL, true, &a, 1);
      v = QVariant((uint)a);
    }
  else if (m_voxelType == _Char)
    {
      char a;
      metaImageReader.ReadROI(mind, maxd, NULL, true, &a, 1);
      v = QVariant((int)a);
    }
  else if (m_voxelType == _UShort)
    {
      unsigned short a;
      metaImageReader.ReadROI(mind, maxd, NULL, true, &a, 1);
      v = QVariant((uint)a);
    }
  else if (m_voxelType == _Short)
    {
      short a;
      metaImageReader.ReadROI(mind, maxd, NULL, true, &a, 1);
      v = QVariant((int)a);
    }
  else if (m_voxelType == _Int)
    {
      int a;
      metaImageReader.ReadROI(mind, maxd, NULL, true, &a, 1);
      v = QVariant((int)a);
    }
  else if (m_voxelType == _Float)
    {
      float a;
      metaImageReader.ReadROI(mind, maxd, NULL, true, &a, 1);
      v = QVariant((double)a);
    }

  return v;
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
