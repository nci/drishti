#include <QtGui>
#include "common.h"
#include "niftiplugin.h"
#include <iostream>
#include <itkImage.h>
#include <itkImageFileWriter.h>
#include <itkImageFileReader.h>
#include <itkNiftiImageIO.h>
#include <itkImageRegionIterator.h>
#include <itkRegionOfInterestImageFilter.h>


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
  m_rawMin = m_rawMax = 0;
  m_histogram.clear();
  m_4dvol = false;
}

void
NiftiPlugin::clear()
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

void
NiftiPlugin::setMinMax(float rmin, float rmax)
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
  m_fileName.clear();
  m_fileName << flnm;
}


template <class T>
void
NiftiPlugin::readSlice(int idx[3], int sz[3],
		       int nbytes, uchar *slice)
{
  typedef itk::Image<T, 3> ImageType;

  typedef itk::ImageFileReader<ImageType> ReaderType;
  ReaderType::Pointer reader = ReaderType::New();
  reader->SetFileName(m_fileName[0].toLatin1().data());
  typedef itk::NiftiImageIO NiftiIOType;
  NiftiIOType::Pointer niftiIO = NiftiIOType::New();
  reader->SetImageIO(niftiIO);

  typedef itk::RegionOfInterestImageFilter<ImageType,ImageType> RegionExtractor;

  ImageType::RegionType region;
  ImageType::SizeType size;
  ImageType::IndexType index;
  index[2] = idx[2];
  index[1] = idx[1];
  index[0] = idx[0];
  size[2] = sz[2];
  size[1] = sz[1];
  size[0] = sz[0];
  region.SetIndex(index);
  region.SetSize(size);
  
  // Extract the relevant sub-region.
  RegionExtractor::Pointer extractor = RegionExtractor::New();
  extractor->SetInput(reader->GetOutput());
  extractor->SetRegionOfInterest(region);
  extractor->Update();
  ImageType *dimg = extractor->GetOutput();
  char *tdata = (char*)(dimg->GetBufferPointer());
  memcpy(slice, tdata, nbytes);
}

bool
NiftiPlugin::setFile(QStringList files)
{
  m_fileName = files;
 
  typedef itk::Image<unsigned char, 3> ImageType;
  typedef itk::ImageFileReader<ImageType> ReaderType;
  ReaderType::Pointer reader = ReaderType::New();
  reader->SetFileName(m_fileName[0].toLatin1().data());

  typedef itk::NiftiImageIO NiftiIOType;
  NiftiIOType::Pointer niftiIO = NiftiIOType::New();
  reader->SetImageIO(niftiIO);
  reader->Update();

  itk::ImageIOBase::Pointer imageIO = reader->GetImageIO();

  m_height = imageIO->GetDimensions(0);
  m_width = imageIO->GetDimensions(1);
  m_depth = imageIO->GetDimensions(2);

  m_voxelSizeX = imageIO->GetSpacing(0);
  m_voxelSizeY = imageIO->GetSpacing(1);
  m_voxelSizeZ = imageIO->GetSpacing(2);

  int et = imageIO->GetComponentType();
  if (et == itk::ImageIOBase::UCHAR) m_voxelType = _UChar;
  if (et == itk::ImageIOBase::CHAR) m_voxelType = _Char;
  if (et == itk::ImageIOBase::USHORT) m_voxelType = _UShort;
  if (et == itk::ImageIOBase::SHORT) m_voxelType = _Short;
  if (et == itk::ImageIOBase::INT) m_voxelType = _Int;
  if (et == itk::ImageIOBase::FLOAT) m_voxelType = _Float;

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
NiftiPlugin::findMinMaxandGenerateHistogram()
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

  int idx[3];
  int sz[3];
  idx[0] = idx[1] = idx[2] = 0;
  sz[0] = m_height;
  sz[1] = m_width;
  sz[2] = 1;

  m_rawMin = 10000000;
  m_rawMax = -10000000;
  for(int i=0; i<m_depth; i++)
    {
      progress.setValue((int)(100.0*(float)i/(float)m_depth));
      qApp->processEvents();

      idx[2] = i;

      if (m_voxelType == _UChar)
	readSlice<unsigned char>(idx, sz, nbytes, tmp);
      else if (m_voxelType == _Char)
	readSlice<char>(idx, sz, nbytes, tmp);
      else if (m_voxelType == _UShort)
	readSlice<unsigned short>(idx, sz, nbytes, tmp);
      else if (m_voxelType == _Short)
	readSlice<short>(idx, sz, nbytes, tmp);
      else if (m_voxelType == _Int)
	readSlice<int>(idx, sz, nbytes, tmp);
      else if (m_voxelType == _Float)
	readSlice<float>(idx, sz, nbytes, tmp);
 
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
NiftiPlugin::findMinMax()
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

  int idx[3];
  int sz[3];
  idx[0] = idx[1] = idx[2] = 0;
  sz[0] = m_height;
  sz[1] = m_width;
  sz[2] = 1;

  m_rawMin = 10000000;
  m_rawMax = -10000000;
  for(int i=0; i<nX; i++)
    {
      progress.setValue((int)(100.0*(float)i/(float)nX));
      qApp->processEvents();

      idx[2] = i;

      if (m_voxelType == _UChar)
	readSlice<unsigned char>(idx, sz, nbytes, tmp);
      else if (m_voxelType == _Char)
	readSlice<char>(idx, sz, nbytes, tmp);
      else if (m_voxelType == _UShort)
	readSlice<unsigned short>(idx, sz, nbytes, tmp);
      else if (m_voxelType == _Short)
	readSlice<short>(idx, sz, nbytes, tmp);
      else if (m_voxelType == _Int)
	readSlice<int>(idx, sz, nbytes, tmp);
      else if (m_voxelType == _Float)
	readSlice<float>(idx, sz, nbytes, tmp);

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
NiftiPlugin::generateHistogram()
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

      idx[2] = i;

      if (m_voxelType == _UChar)
	readSlice<unsigned char>(idx, sz, nbytes, tmp);
      else if (m_voxelType == _Char)
	readSlice<char>(idx, sz, nbytes, tmp);
      else if (m_voxelType == _UShort)
	readSlice<unsigned short>(idx, sz, nbytes, tmp);
      else if (m_voxelType == _Short)
	readSlice<short>(idx, sz, nbytes, tmp);
      else if (m_voxelType == _Int)
	readSlice<int>(idx, sz, nbytes, tmp);
      else if (m_voxelType == _Float)
	readSlice<float>(idx, sz, nbytes, tmp);

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
NiftiPlugin::getDepthSlice(int slc,
			 uchar *slice)
{
  int nbytes = m_width*m_height*m_bytesPerVoxel;
  if (slc < 0 || slc >= m_depth)
    {
      memset(slice, 0, nbytes);
      return;
    }

  int idx[3];
  int sz[3];
  idx[0] = idx[1] = 0;
  idx[2] = slc;
  sz[0] = m_height;
  sz[1] = m_width;
  sz[2] = 1;

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

void
NiftiPlugin::getWidthSlice(int slc,
			 uchar *slice)
{
  int nbytes = m_depth*m_height*m_bytesPerVoxel;
  if (slc < 0 || slc >= m_width)
    {
      memset(slice, 0, nbytes);
      return;
    }

  int idx[3];
  int sz[3];
  idx[0] = 0;
  idx[1] = slc;
  idx[2] = 0;
  sz[0] = m_height;
  sz[1] = 1;
  sz[2] = m_depth;

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

void
NiftiPlugin::getHeightSlice(int slc,
			  uchar *slice)
{
  int nbytes = m_depth*m_width*m_bytesPerVoxel;
  if (slc < 0 || slc >= m_height)
    {
      memset(slice, 0, nbytes);
      return;
    }

  int idx[3];
  int sz[3];
  idx[0] = slc;
  idx[1] = 0;
  idx[2] = 0;
  sz[0] = 1;
  sz[1] = m_width;
  sz[2] = m_depth;

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

QVariant
NiftiPlugin::rawValue(int d, int w, int h)
{
  QVariant v;

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

  if (m_voxelType == _UChar)
    {
      unsigned char a;
      readSlice<unsigned char>(idx, sz, 1, &a);
      v = QVariant((uint)a);
    }
  else if (m_voxelType == _Char)
    {
      char a;
      readSlice<char>(idx, sz, 1, (uchar*)&a);
      v = QVariant((int)a);
    }
  else if (m_voxelType == _UShort)
    {
      unsigned short a;
      readSlice<unsigned short>(idx, sz, 2, (uchar*)&a);
      v = QVariant((uint)a);
    }
  else if (m_voxelType == _Short)
    {
      short a;
      readSlice<short>(idx, sz, 2, (uchar*)&a);
      v = QVariant((int)a);
    }
  else if (m_voxelType == _Int)
    {
      int a;
      readSlice<short>(idx, sz, 4, (uchar*)&a);
      v = QVariant((int)a);
    }
  else if (m_voxelType == _Float)
    {
      float a;
      readSlice<short>(idx, sz, 4, (uchar*)&a);
      v = QVariant((double)a);
    }

  return v;
}

void
NiftiPlugin::saveTrimmed(QString trimFile,
			     int dmin, int dmax,
			     int wmin, int wmax,
			     int hmin, int hmax)
{
  QProgressDialog progress("Saving trimmed volume",
			   QString(),
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

  uchar vt;
  if (m_voxelType == _UChar) vt = 0; // unsigned byte
  if (m_voxelType == _Char) vt = 1; // signed byte
  if (m_voxelType == _UShort) vt = 2; // unsigned short
  if (m_voxelType == _Short) vt = 3; // signed short
  if (m_voxelType == _Int) vt = 4; // int
  if (m_voxelType == _Float) vt = 8; // float
  
  QFile fout(trimFile);
  fout.open(QFile::WriteOnly);

  fout.write((char*)&vt, 1);
  fout.write((char*)&mX, 4);
  fout.write((char*)&mY, 4);
  fout.write((char*)&mZ, 4);


  int idx[3];
  int sz[3];
  idx[0] = hmin;
  idx[1] = wmin;
  idx[2] = dmin;
  sz[0] = mZ;
  sz[1] = mY;
  sz[2] = 1;

  for(int i=dmin; i<=dmax; i++)
    {
      idx[2] = i;

      if (m_voxelType == _UChar)
	readSlice<unsigned char>(idx, sz, nbytes, tmp);
      else if (m_voxelType == _Char)
	readSlice<char>(idx, sz, nbytes, tmp);
      else if (m_voxelType == _UShort)
	readSlice<unsigned short>(idx, sz, nbytes, tmp);
      else if (m_voxelType == _Short)
	readSlice<short>(idx, sz, nbytes, tmp);
      else if (m_voxelType == _Int)
	readSlice<int>(idx, sz, nbytes, tmp);
      else if (m_voxelType == _Float)
	readSlice<float>(idx, sz, nbytes, tmp);

      fout.write((char*)tmp, mY*mZ*m_bytesPerVoxel);

      progress.setValue((int)(100*(float)(i-dmin)/(float)mX));
      qApp->processEvents();
    }
  fout.close();

  delete [] tmp;

  m_headerBytes = 13; // to be used for applyMapping function
}
