#include "common.h"
#include "analyzeplugin.h"

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
  m_bytesPerVoxel = 1;
  m_rawMin = m_rawMax = 0;
  m_histogram.clear();
  m_4dvol = false;
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
  m_bytesPerVoxel = 1;
  m_rawMin = m_rawMax = 0;
  m_histogram.clear();
  m_4dvol = false;
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
  m_fileName.clear();
  m_fileName << flnm;
}

bool
AnalyzePlugin::setFile(QStringList files)
{
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

  QFile fin(m_hdrFile);
  fin.open(QFile::ReadOnly);
  fin.read((char*)&(m_analyzeHeader.hk), 40);
  fin.read((char*)&(m_analyzeHeader.dime), 108);
  fin.read((char*)&(m_analyzeHeader.hist), 200);
  fin.close();

  m_byteSwap = false;
  if (m_analyzeHeader.hk.sizeof_hdr != 348)
    {
      uchar *sptr = (uchar*)(& m_analyzeHeader.hk.sizeof_hdr);
      swapbytes(sptr, 4);
      if (m_analyzeHeader.hk.sizeof_hdr != 348)
	{
	  QMessageBox::information(0, "Error",
	       QString("Header Size is not 348 : %1").\
				   arg(m_analyzeHeader.hk.sizeof_hdr));
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

	  m_byteSwap = true;
	}
      }
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
  
  m_skipBytes = 0;
  
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
			   "Cancel",
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

  int nX, nY, nZ;
  nX = m_depth;
  nY = m_width;
  nZ = m_height;

  int nbytes = nY*nZ*m_bytesPerVoxel;
  uchar *tmp = new uchar[nbytes];

  QFile fin(m_imgFile);
  fin.open(QFile::ReadOnly);
  fin.seek(m_skipBytes);

  m_rawMin = 10000000;
  m_rawMax = -10000000;
  for(uint i=0; i<nX; i++)
    {
      progress.setValue((int)(100.0*(float)i/(float)nX));
      qApp->processEvents();

      fin.read((char*)tmp, nbytes);

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
    for(uint j=0; j<nY*nZ; j++)				\
      {							\
	float val = ptr[j];				\
	m_rawMin = qMin(m_rawMin, val);			\
	m_rawMax = qMax(m_rawMax, val);			\
      }							\
  }

void
AnalyzePlugin::findMinMax()
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

  QFile fin(m_imgFile);
  fin.open(QFile::ReadOnly);
  fin.seek(m_skipBytes);

  m_rawMin = 10000000;
  m_rawMax = -10000000;
  for(uint i=0; i<nX; i++)
    {
      progress.setValue((int)(100.0*(float)i/(float)nX));
      qApp->processEvents();

      fin.read((char*)tmp, nbytes);

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

  delete [] tmp;

  progress.setValue(100);
  qApp->processEvents();
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
AnalyzePlugin::generateHistogram()
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

  QFile fin(m_imgFile);
  fin.open(QFile::ReadOnly);
  fin.seek(m_skipBytes);

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

      fin.read((char*)tmp, nbytes);

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
AnalyzePlugin::getDepthSlice(int slc,
			    uchar *slice)
{
  int nbytes = m_width*m_height*m_bytesPerVoxel;

  QFile fin(m_imgFile);
  fin.open(QFile::ReadOnly);
  fin.seek(m_skipBytes + nbytes*slc);
  fin.read((char*)slice, nbytes);
  fin.close();

  if (m_byteSwap && m_bytesPerVoxel > 1)
    swapbytes(slice, m_bytesPerVoxel, nbytes);      
}

void
AnalyzePlugin::getWidthSlice(int slc,
			     uchar *slice)
{
  int nbytes = m_depth*m_height*m_bytesPerVoxel;

  QFile fin(m_imgFile);
  fin.open(QFile::ReadOnly);

  for(uint k=0; k<m_depth; k++)
    {
      fin.seek(m_skipBytes +
	       (slc*m_height + k*m_width*m_height)*m_bytesPerVoxel);

      fin.read((char*)(slice+k*m_height*m_bytesPerVoxel),
	       m_height*m_bytesPerVoxel);
    }
  fin.close();

  if (m_byteSwap && m_bytesPerVoxel > 1)
    swapbytes(slice, m_bytesPerVoxel, nbytes);
}

void
AnalyzePlugin::getHeightSlice(int slc,
			      uchar *slice)
{
  int nbytes = m_depth*m_width*m_bytesPerVoxel;

  QFile fin(m_imgFile);
  fin.open(QFile::ReadOnly);

  int ndum = m_width*m_height*m_bytesPerVoxel;
  uchar *dum = new uchar[ndum];
  
  uint it=0;
  for(uint k=0; k<m_depth; k++)
    {
      fin.read((char*)dum, ndum);
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

  if (m_byteSwap && m_bytesPerVoxel > 1)
    swapbytes(slice, m_bytesPerVoxel, nbytes);
}

QVariant
AnalyzePlugin::rawValue(int d, int w, int h)
{
  QVariant v;

  if (d < 0 || d >= m_depth ||
      w < 0 || w >= m_width ||
      h < 0 || h >= m_height)
    {
      v = QVariant("OutOfBounds");
      return v;
    }

  QFile fin(m_imgFile);
  fin.open(QFile::ReadOnly);
  fin.seek(m_skipBytes +
	   m_bytesPerVoxel*(d*m_width*m_height +
			    w*m_height +
			    h));

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
      if (m_byteSwap)
	{
	  uchar *sptr = (uchar*)(&a);
	  swapbytes(sptr, 2);
	}
      v = QVariant((uint)a);
    }
  else if (m_voxelType == _Short)
    {
      short a;
      fin.read((char*)&a, m_bytesPerVoxel);
      if (m_byteSwap)
	{
	  uchar *sptr = (uchar*)(&a);
	  swapbytes(sptr, 2);
	}
      v = QVariant((int)a);
    }
  else if (m_voxelType == _Int)
    {
      int a;
      fin.read((char*)&a, m_bytesPerVoxel);
      if (m_byteSwap)
	{
	  uchar *sptr = (uchar*)(&a);
	  swapbytes(sptr, 4);
	}
      v = QVariant((int)a);
    }
  else if (m_voxelType == _Float)
    {
      float a;
      fin.read((char*)&a, m_bytesPerVoxel);
      if (m_byteSwap)
	{
	  uchar *sptr = (uchar*)(&a);
	  swapbytes(sptr, 4);
	}
      v = QVariant((double)a);
    }
  fin.close();

  return v;
}

void
AnalyzePlugin::saveTrimmed(QString trimFile,
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

  QFile fin(m_imgFile);
  fin.open(QFile::ReadOnly);
  fin.seek(m_skipBytes + nbytes*dmin);

  for(uint i=dmin; i<=dmax; i++)
    {
      fin.read((char*)tmp, nbytes);

      for(uint j=wmin; j<=wmax; j++)
	{
	  memcpy(tmp+(j-wmin)*mZ*m_bytesPerVoxel,
		 tmp+(j*nZ + hmin)*m_bytesPerVoxel,
		 mZ*m_bytesPerVoxel);
	}
	  
      if (m_byteSwap && m_bytesPerVoxel > 1)
	swapbytes(tmp,
		  m_bytesPerVoxel,
		  mY*mZ*m_bytesPerVoxel);      

      fout.write((char*)tmp, mY*mZ*m_bytesPerVoxel);

      progress.setValue((int)(100*(float)(i-dmin)/(float)mX));
      qApp->processEvents();
    }

  fin.close();
  fout.close();

  delete [] tmp;

  m_headerBytes = 13; // to be used for applyMapping function
}

bool
AnalyzePlugin::checkExtension(QString flnm, const char *ext)
{
  bool ok = true;
  int extlen = strlen(ext);

  QFileInfo info(flnm);
  if (info.exists() && info.isFile())
    {
      QByteArray exten = flnm.toLatin1().right(extlen);
      if (exten != ext)
	ok = false;
    }
  else
    ok = false;

  return ok;
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
