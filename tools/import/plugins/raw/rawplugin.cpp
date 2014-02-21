#include <QtGui>
#include "common.h"
#include "rawplugin.h"
#include "loadrawdialog.h"

QStringList
RawPlugin::registerPlugin()
{
  QStringList regString;
  regString << "files";
  regString << "RAW Files";
  
  return regString;
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
  m_bytesPerVoxel = 1;
  m_rawMin = m_rawMax = 0;
  m_histogram.clear();
  m_4dvol = false;
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
  m_bytesPerVoxel = 1;
  m_rawMin = m_rawMax = 0;
  m_histogram.clear();
  m_4dvol = false;
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
  m_fileName.clear();
  m_fileName << flnm;
}

bool
RawPlugin::setFile(QStringList files)
{
  m_fileName = files;

  int nX, nY, nZ;
  {
    // --- load various parameters from the raw file ---
    LoadRawDialog loadRawDialog(0,
				(char *)m_fileName[0].toLatin1().data());

    if (!m_4dvol)
      {
	loadRawDialog.exec();    
	if (loadRawDialog.result() == QDialog::Rejected)
	  return false;
      }

    m_voxelType = loadRawDialog.voxelType();
    m_skipBytes = loadRawDialog.skipHeaderBytes();
    loadRawDialog.gridSize(nX, nY, nZ);
    m_depth = nX;
    m_width = nY;
    m_height = nZ;
  }

  //-----------------------------------
  QFile fin(m_fileName[0]);
  fin.open(QFile::ReadOnly);

  //-- recheck the information (for backward compatibility) ----
  if (m_skipBytes == 0)
    {
      int bpv = 1;
      if (m_voxelType == _UChar) bpv = 1;
      else if (m_voxelType == _Char) bpv = 1;
      else if (m_voxelType == _UShort) bpv = 2;
      else if (m_voxelType == _Short) bpv = 2;
      else if (m_voxelType == _Int) bpv = 4;
      else if (m_voxelType == _Float) bpv = 4;

      if (fin.size() == 13+nX*nY*nZ*bpv)
	m_skipBytes = 13;
      else if (fin.size() == 12+nX*nY*nZ*bpv)
	m_skipBytes = 12;
      else
	m_skipBytes = 0;
    }
  m_headerBytes = m_skipBytes;
  fin.close();
  //------------------------------

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
RawPlugin::findMinMaxandGenerateHistogram()
{
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
	  return;
	}
      else if (m_voxelType == _Char)
	{
	  m_rawMin = -127;
	  m_rawMax = 128;
	  return;
	}
      else if (m_voxelType == _UShort)
	{
	  m_rawMin = 0;
	  m_rawMax = 65535;
	  return;
	}
      else if (m_voxelType == _Short)
	{
	  m_rawMin = -32767;
	  m_rawMax = 32768;
	  return;
	}
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

  m_rawMin = 10000000;
  m_rawMax = -10000000;
  for(int i=0; i<nX; i++)
    {
      progress.setValue((int)(100.0*(float)i/(float)nX));
      qApp->processEvents();

      fin.read((char*)tmp, nbytes);

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
RawPlugin::findMinMax()
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

  QFile fin(m_fileName[0]);
  fin.open(QFile::ReadOnly);
  fin.seek(m_skipBytes);

  m_rawMin = 10000000;
  m_rawMax = -10000000;
  for(int i=0; i<nX; i++)
    {
      progress.setValue((int)(100.0*(float)i/(float)nX));
      qApp->processEvents();

      fin.read((char*)tmp, nbytes);

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
RawPlugin::generateHistogram()
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

  QFile fin(m_fileName[0]);
  fin.open(QFile::ReadOnly);
  fin.seek(m_skipBytes);

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

//  QMessageBox::information(0, "",  QString("%1 %2 : %3").\
//			   arg(m_rawMin).arg(m_rawMax).arg(rSize));

  progress.setValue(100);
  qApp->processEvents();
}

void
RawPlugin::getDepthSlice(int slc,
			 uchar *slice)
{
  qint64 nbytes = m_width*m_height*m_bytesPerVoxel;
  if (slc < 0 || slc >= m_depth)
    {
      memset(slice, 0, nbytes);
      return;
    }

  QFile fin(m_fileName[0]);
  fin.open(QFile::ReadOnly);
  fin.seek((qint64)(m_skipBytes + nbytes*slc));
  fin.read((char*)slice, (qint64)(nbytes));
  fin.close();
}

void
RawPlugin::getWidthSlice(int slc,
			 uchar *slice)
{
  int nbytes = m_depth*m_height*m_bytesPerVoxel;
  if (slc < 0 || slc >= m_width)
    {
      memset(slice, 0, nbytes);
      return;
    }

  QFile fin(m_fileName[0]);
  fin.open(QFile::ReadOnly);

  for(int k=0; k<m_depth; k++)
    {
      fin.seek((qint64)(m_skipBytes +
			((qint64)slc*m_height +
			 (qint64)k*m_width*m_height*m_bytesPerVoxel)));

      fin.read((char*)(slice+(qint64)(k*m_height*m_bytesPerVoxel)),
	       (qint64)(m_height*m_bytesPerVoxel));
    }
  fin.close();
}

void
RawPlugin::getHeightSlice(int slc,
			  uchar *slice)
{
  int nbytes = m_depth*m_width*m_bytesPerVoxel;
  if (slc < 0 || slc >= m_height)
    {
      memset(slice, 0, nbytes);
      return;
    }

  QFile fin(m_fileName[0]);
  fin.open(QFile::ReadOnly);
  fin.seek(m_skipBytes);

  int ndum = m_width*m_height*m_bytesPerVoxel;
  uchar *dum = new uchar[ndum];
  
  int it=0;
  for(int k=0; k<m_depth; k++)
    {
      fin.read((char*)dum, ndum);
      for(int j=0; j<m_width; j++)
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
RawPlugin::rawValue(int d, int w, int h)
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

  if (m_voxelType == _UChar)
    {
      unsigned char a;
      fin.read((char*)&a, m_bytesPerVoxel);
      v = QVariant((uint)a);
    }
  else if (m_voxelType == _Char)
    {
      char a;
      fin.read((char*)&a, m_bytesPerVoxel);
      v = QVariant((int)a);
    }
  else if (m_voxelType == _UShort)
    {
      unsigned short a;
      fin.read((char*)&a, m_bytesPerVoxel);
      v = QVariant((uint)a);
    }
  else if (m_voxelType == _Short)
    {
      short a;
      fin.read((char*)&a, m_bytesPerVoxel);
      v = QVariant((int)a);
    }
  else if (m_voxelType == _Int)
    {
      int a;
      fin.read((char*)&a, m_bytesPerVoxel);
      v = QVariant((int)a);
    }
  else if (m_voxelType == _Float)
    {
      float a;
      fin.read((char*)&a, m_bytesPerVoxel);
      v = QVariant((double)a);
    }
  fin.close();
 
  return v;
}

void
RawPlugin::saveTrimmed(QString trimFile,
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

  QFile fin(m_fileName[0]);
  fin.open(QFile::ReadOnly);
  fin.seek((qint64)(m_skipBytes + nbytes*dmin));

  for(int i=dmin; i<=dmax; i++)
    {
      fin.read((char*)tmp, (qint64)(nbytes));

      for(int j=wmin; j<=wmax; j++)
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
