#include <QtGui>
#include "common.h"
#include "txmplugin.h"

QStringList
TxmPlugin::registerPlugin()
{
  QStringList regString;
  regString << "files";
  regString << "TXM Files";
  
  return regString;
}

void
TxmPlugin::init()
{
  m_storage = 0;

  m_fileName.clear();

  m_description.clear();
  m_depth = m_width = m_height = 0;
  m_voxelType = _UChar;
  m_voxelUnit = _Micron;
  m_voxelSizeX = m_voxelSizeY = m_voxelSizeZ = 1;
  m_bytesPerVoxel = 1;
  m_rawMin = m_rawMax = 0;
  m_histogram.clear();
  m_4dvol = false;

  m_imageData.clear();
}

void
TxmPlugin::clear()
{
  if (m_storage)
    {
      m_storage->close();
      delete m_storage;
    }
  m_storage = 0;

  m_fileName.clear();

  m_description.clear();
  m_depth = m_width = m_height = 0;
  m_voxelType = _UChar;
  m_voxelUnit = _Micron;
  m_voxelSizeX = m_voxelSizeY = m_voxelSizeZ = 1;
  m_bytesPerVoxel = 1;
  m_rawMin = m_rawMax = 0;
  m_histogram.clear();
  m_4dvol = false;

  m_imageData.clear();
}

void
TxmPlugin::set4DVolume(bool flag)
{
  m_4dvol = flag;
}

void
TxmPlugin::voxelSize(float& vx, float& vy, float& vz)
  {
    vx = m_voxelSizeX;
    vy = m_voxelSizeY;
    vz = m_voxelSizeZ;
  }
QString TxmPlugin::description() { return m_description; }
int TxmPlugin::voxelType() { return m_voxelType; }
int TxmPlugin::voxelUnit() { return m_voxelUnit; }
int TxmPlugin::headerBytes() { return m_headerBytes; }

void
TxmPlugin::setMinMax(float rmin, float rmax)
{
  m_rawMin = rmin;
  m_rawMax = rmax;
}
float TxmPlugin::rawMin() { return m_rawMin; }
float TxmPlugin::rawMax() { return m_rawMax; }
QList<uint> TxmPlugin::histogram() { return m_histogram; }

void
TxmPlugin::gridSize(int& d, int& w, int& h)
{
  d = m_depth;
  w = m_width;
  h = m_height;
}

void
TxmPlugin::replaceFile(QString flnm)
{
  m_fileName.clear();
  m_fileName << flnm;
}

bool
TxmPlugin::setFile(QStringList files)
{
  if (files.size() == 0)
    return false;

  m_fileName = files;

  if (m_storage)
    {
      m_storage->close();
      delete m_storage;
    }

  m_fileName = files;


  m_storage = new POLE::Storage(m_fileName[0].toLatin1().data() );
  m_storage->open();

  if(m_storage->result() != POLE::Storage::Ok )
  {
    QString msg = QString( tr("Unable to open file %1\n") ).arg(m_fileName[0]);
    QMessageBox::critical( 0, tr("Error"), msg );
    m_storage->close();
    delete m_storage;
    return false;
  }
  
  int vt;
  POLE::Stream(m_storage, "/ImageInfo/DataType").read((uchar*)&vt, 4);
//  QMessageBox::information(0, "", QString("data type : %1").arg(vt));

  m_bytesPerVoxel = 1;
  if (vt == 5)
    {
      m_voxelType = _UShort;
      m_bytesPerVoxel = 2;
    }
  else if (vt == 10)
    {
      m_voxelType = _Float;
      m_bytesPerVoxel = 4;
    }

  m_headerBytes = 0;

//  float ps;
//  POLE::Stream(m_storage, "/ImageInfo/PixelSize").read((uchar*)&ps, 4);
//  QMessageBox::information(0, "", QString("pixel size : %1").arg(ps));
  POLE::Stream(m_storage, "/ImageInfo/NoOfImages").read((uchar*)&m_depth, 4);
  POLE::Stream(m_storage, "/ImageInfo/ImageWidth").read((uchar*)&m_width, 4);
  POLE::Stream(m_storage, "/ImageInfo/ImageHeight").read((uchar*)&m_height, 4);

//  QMessageBox::information(0, "", QString("%1 %2 %3").\
//			   arg(m_depth).arg(m_width).arg(m_height));


  m_imageData.clear();
  enumerateImages(m_storage, "/");

//  QMessageBox::information(0, "", QString("Number of Images %1\n%2").\
//			   arg(m_imageData.count()).arg(m_imageData[0]));

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

void
TxmPlugin::enumerateImages(POLE::Storage *storage, std::string path)
{
  std::list<std::string> entries;
  entries = storage->entries(path);

  std::list<std::string>::iterator it;
  for(it = entries.begin(); it != entries.end(); ++it)
  {
    std::string name = *it;
    std::string fullname = path + name;
    
    if(storage->isDirectory(fullname))
      {
	QString str = name.c_str();
	str.truncate(9);
	if (str == "ImageData")
	  enumerateImages(storage, fullname + "/" );
      }
    else
      {
	QString str = name.c_str();
	str.truncate(5);
	if (str == "Image")
	  m_imageData << fullname.c_str();
      }
  }
}

#define MINMAXANDHISTOGRAM()				\
  {							\
    for(uint j=0; j<m_width*m_height; j++)		\
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
TxmPlugin::loadTxmImage(int i, uchar* tmp)
{
  int nbytes = m_width*m_height*m_bytesPerVoxel;
  POLE::Stream(m_storage, m_imageData[i].toLatin1().data()).read(tmp, nbytes);
}

void
TxmPlugin::findMinMaxandGenerateHistogram()
{
  QProgressDialog progress("Generating Histogram",
			   0,
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
      for(uint i=0; i<256; i++)
	m_histogram.append(0);
    }
  else if (m_voxelType == _UShort ||
      m_voxelType == _Short)
    {
      if (m_voxelType == _UShort) rMin = 0;
      if (m_voxelType == _Short) rMin = -32767;
      rSize = 65535;
      for(uint i=0; i<65536; i++)
	m_histogram.append(0);
    }
  else
    {
      QMessageBox::information(0, "Error", "Why am i here ???");
      return;
    }

  int nbytes = m_width*m_height*m_bytesPerVoxel;
  uchar *tmp = new uchar[nbytes];

  m_rawMin = 10000000;
  m_rawMax = -10000000;

  for(uint i=0; i<m_depth; i++)
    {
      progress.setValue((int)(100.0*(float)i/(float)m_depth));
      progress.setLabelText(QString("%1 of %2").arg(i).arg(m_depth-1));
      qApp->processEvents();

      loadTxmImage(i, tmp);

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
TxmPlugin::findMinMax()
{
  QProgressDialog progress("Finding Min and Max",
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

  m_rawMin = 10000000;
  m_rawMax = -10000000;
  for(int i=0; i<nX; i++)
    {
      progress.setValue((int)(100.0*(float)i/(float)nX));
      qApp->processEvents();

      loadTxmImage(i, tmp);

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
TxmPlugin::generateHistogram()
{
  QProgressDialog progress("Generating Histogram",
			   "Cancel",
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
  for(int i=0; i<nX; i++)
    {
      progress.setValue((int)(100.0*(float)i/(float)nX));
      qApp->processEvents();

      loadTxmImage(i, tmp);

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
TxmPlugin::getDepthSlice(int slc,
			  uchar *slice)
{
  int nbytes = m_width*m_height*m_bytesPerVoxel;

  uchar *tmp1 = new uchar[nbytes];

  loadTxmImage(slc, tmp1);

  if (m_voxelType == _UChar)
    {
      for(uint j=0; j<m_width; j++)
	for(uint k=0; k<m_height; k++)
	  slice[j*m_height+k] = tmp1[k*m_width+j];
    }
  else if (m_voxelType == _UShort)
    {
      ushort *p0 = (ushort*)slice;
      ushort *p1 = (ushort*)tmp1;
      for(uint j=0; j<m_width; j++)
	for(uint k=0; k<m_height; k++)
	  p0[j*m_height+k] = p1[k*m_width+j];
    }
  else if (m_voxelType == _Float)
    {
      float *p0 = (float*)slice;
      float *p1 = (float*)tmp1;
      for(uint j=0; j<m_width; j++)
	for(uint k=0; k<m_height; k++)
	  p0[j*m_height+k] = p1[k*m_width+j];
    }
  delete [] tmp1;
}

void
TxmPlugin::getWidthSlice(int slc,
			  uchar *slice)
{
  QProgressDialog progress("Extracting Slice",
			   0,
			   0, 100,
			   0);
  progress.setMinimumDuration(0);
  uchar *imgSlice = new uchar[m_width*m_height*m_bytesPerVoxel];
  for(uint i=0; i<m_depth; i++)
    {
      progress.setValue((int)(100.0*(float)i/(float)m_depth));
      qApp->processEvents();

      loadTxmImage(i, imgSlice);

      if (m_voxelType == _UChar)
	{
	  for(uint j=0; j<m_height; j++)
	    slice[i*m_height+j] = imgSlice[slc*m_height+j];
	}
      else if (m_voxelType == _UShort)
	{
	  ushort *p0 = (ushort*)slice;
	  ushort *p1 = (ushort*)imgSlice;
	  for(uint j=0; j<m_height; j++)
	    p0[i*m_height+j] = p1[slc*m_height+j];
	}
      else if (m_voxelType == _Float)
	{
	  float *p0 = (float*)slice;
	  float *p1 = (float*)imgSlice;
	  for(uint j=0; j<m_height; j++)
	    p0[i*m_height+j] = p1[slc*m_height+j];
	}
    }
  delete [] imgSlice;
  progress.setValue(100);
  qApp->processEvents();
}

void
TxmPlugin::getHeightSlice(int slc,
			   uchar *slice)
{
  uchar *imgSlice = new uchar[m_width*m_height*m_bytesPerVoxel];
  QProgressDialog progress("Extracting Slice",
			   0,
			   0, 100,
			   0);
  progress.setMinimumDuration(0);
  for(uint i=0; i<m_depth; i++)
    {
      progress.setValue((int)(100.0*(float)i/(float)m_depth));
      qApp->processEvents();

      loadTxmImage(i, imgSlice);

      if (m_voxelType == _UChar)
	{
	  for(uint j=0; j<m_width; j++)
	    slice[i*m_width+j] = imgSlice[j*m_height+slc];
	}
      else if (m_voxelType == _UShort)
	{
	  ushort *p0 = (ushort*)slice;
	  ushort *p1 = (ushort*)imgSlice;
	  for(uint j=0; j<m_width; j++)
	    p0[i*m_width+j] = p1[j*m_height+slc];
	}
      else if (m_voxelType == _Float)
	{
	  float *p0 = (float*)slice;
	  float *p1 = (float*)imgSlice;
	  for(uint j=0; j<m_width; j++)
	    p0[i*m_width+j] = p1[j*m_height+slc];
	}
    }
  delete [] imgSlice;
  progress.setValue(100);
  qApp->processEvents();
}

QVariant
TxmPlugin::rawValue(int d, int w, int h)
{
  QVariant v;

  if (d < 0 || d >= m_depth ||
      w < 0 || w >= m_width ||
      h < 0 || h >= m_height)
    {
      v = QVariant("OutOfBounds");
      return v;
    }

  uchar *tmp = new uchar[10];
  uchar *imgSlice = new uchar[m_width*m_height*m_bytesPerVoxel];

  loadTxmImage(d, imgSlice);

  if (m_voxelType == _UChar)
    tmp[0] = imgSlice[w*m_height+h];
  else if (m_voxelType == _UShort)
    {
      ushort *p0 = (ushort*)tmp;
      ushort *p1 = (ushort*)imgSlice;
      p0[0] = p1[w*m_height+h];
    }
  else if (m_voxelType == _Float)
    {
      float *p0 = (float*)tmp;
      float *p1 = (float*)imgSlice;
      p0[0] = p1[w*m_height+h];
    }
  
  if (m_voxelType == _UChar)
    {
      uchar a = *tmp;
      v = QVariant((uint)a);
    }
  else if (m_voxelType == _UShort)
    {
      ushort a = *(ushort*)tmp;
      v = QVariant((uint)a);
    }
  else if (m_voxelType == _Float)
    {
      float a = *(float*)tmp;
      v = QVariant((double)a);
    }

  return v;
}

void
TxmPlugin::saveTrimmed(QString trimFile,
			    int dmin, int dmax,
			    int wmin, int wmax,
			    int hmin, int hmax)
{
  QProgressDialog progress("Saving trimmed volume",
			   0,
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
  uchar *tmp1 = new uchar[nbytes];

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

  //for(uint i=dmin; i<=dmax; i++)
  for(int i=dmax; i>=dmin; i--)
    {
      loadTxmImage(i, tmp1);

      if (m_voxelType == _UChar)
	{
	  for(uint j=0; j<m_width; j++)
	    for(uint k=0; k<m_height; k++)
	      tmp[j*m_height+k] = tmp1[k*m_width+j];
	}
      else if (m_voxelType == _UShort)
	{
	  ushort *p0 = (ushort*)tmp;
	  ushort *p1 = (ushort*)tmp1;
	  for(uint j=0; j<m_width; j++)
	    for(uint k=0; k<m_height; k++)
	      p0[j*m_height+k] = p1[k*m_width+j];
	}
      else if (m_voxelType == _Float)
	{
	  float *p0 = (float*)tmp;
	  float *p1 = (float*)tmp1;
	  for(uint j=0; j<m_width; j++)
	    for(uint k=0; k<m_height; k++)
	      p0[j*m_height+k] = p1[k*m_width+j];
	}
      

      for(uint j=wmin; j<=wmax; j++)
	{
	  memcpy(tmp+(j-wmin)*mZ*m_bytesPerVoxel,
		 tmp+(j*nZ + hmin)*m_bytesPerVoxel,
		 mZ*m_bytesPerVoxel);
	}
	  
      fout.write((char*)tmp, mY*mZ*m_bytesPerVoxel);

      progress.setValue((int)(100*(float)(dmax-i)/(float)mX));
      qApp->processEvents();
    }

  fout.close();

  delete [] tmp;
  delete [] tmp1;

  progress.setValue(100);

  m_headerBytes = 13; // to be used for applyMapping function
}
