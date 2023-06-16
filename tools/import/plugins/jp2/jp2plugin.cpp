#include <QtGui>
#include "common.h"
#include "jp2plugin.h"

#include "openjpeg.h"
using namespace std;

QStringList
Jp2Plugin::registerPlugin()
{
  QStringList regString;
  regString << "directory";
  regString << "Grayscale JPEG2000 Image Directory";
  regString << "files";
  regString << "Grayscale JPEG2000 Image Files";
  
  return regString;
}

void
Jp2Plugin::init()
{  
  m_fileName.clear();
  m_imageList.clear();

  m_description.clear();
  m_depth = m_width = m_height = 0;
  m_voxelType = _UChar;
  m_voxelUnit = _Micron;
  m_voxelSizeX = m_voxelSizeY = m_voxelSizeZ = 1;
  m_bytesPerVoxel = 1;
  m_rawMin = m_rawMax = 0;
  m_histogram.clear();
  m_4dvol = false;
}

void
Jp2Plugin::clear()
{
  m_fileName.clear();
  m_imageList.clear();

  m_description.clear();
  m_depth = m_width = m_height = 0;
  m_voxelType = _UChar;
  m_voxelUnit = _Micron;
  m_voxelSizeX = m_voxelSizeY = m_voxelSizeZ = 1;
  m_bytesPerVoxel = 1;
  m_rawMin = m_rawMax = 0;
  m_histogram.clear();
  m_4dvol = false;
}

void
Jp2Plugin::set4DVolume(bool flag)
{
  m_4dvol = flag;
}

void
Jp2Plugin::voxelSize(float& vx, float& vy, float& vz)
  {
    vx = m_voxelSizeX;
    vy = m_voxelSizeY;
    vz = m_voxelSizeZ;
  }
QString Jp2Plugin::description() { return m_description; }
int Jp2Plugin::voxelType() { return m_voxelType; }
int Jp2Plugin::voxelUnit() { return m_voxelUnit; }
int Jp2Plugin::headerBytes() { return m_headerBytes; }

void
Jp2Plugin::setMinMax(float rmin, float rmax)
{
  m_rawMin = rmin;
  m_rawMax = rmax;
}
float Jp2Plugin::rawMin() { return m_rawMin; }
float Jp2Plugin::rawMax() { return m_rawMax; }
QList<uint> Jp2Plugin::histogram() { return m_histogram; }

void
Jp2Plugin::gridSize(int& d, int& w, int& h)
{
  d = m_depth;
  w = m_width;
  h = m_height;
}

void
Jp2Plugin::replaceFile(QString flnm)
{
  m_fileName.clear();
  m_fileName << flnm;

  m_imageList = m_fileName;
}

bool
Jp2Plugin::setImageFiles(QStringList files)
{
  QProgressDialog progress("Enumerating files - may take some time...",
			   0,
			   0, 100,
			   0);
  progress.setMinimumDuration(0);

  QStringList flist;
  flist.clear();
  
  for(uint i=0; i<files.size(); i++)
    {
      progress.setValue(100*(float)i/(float)files.size());
      qApp->processEvents();
      
      QFileInfo fileInfo(m_fileName[0], files[i]);
      QString imgfl = fileInfo.absoluteFilePath();

      m_imageList.append(imgfl);
    }
  
  progress.setValue(100);
  qApp->processEvents();

  m_depth = m_imageList.size();


  if (!loadJp2ImageProperties(m_imageList[0]))
    return false;

  m_headerBytes = 0;
  
  if (m_voxelType == _UChar ||
      m_voxelType == _Char ||
      m_voxelType == _UShort ||
      m_voxelType == _Short)
    findMinMaxandGenerateHistogram();
  else
    {
      //QMessageBox::information(0, "Error",
      //		       "Currently accepting only 1- and 2-byte images");
      findMinMax();
      generateHistogram();
    }

  return true;
}

bool
Jp2Plugin::setFile(QStringList files)
{
  if (files.size() == 0)
    return false;

  m_fileName = files;

  m_imageList.clear();

  QFileInfo f(m_fileName[0]);
  if (f.isDir())
    {
      // list all image files in the directory
      QStringList imageNameFilter;
      imageNameFilter << "*.jp2";
      imageNameFilter << "*.j2k";
      imageNameFilter << "*.jpt";
      QStringList imgfiles= QDir(m_fileName[0]).entryList(imageNameFilter,
							  QDir::NoSymLinks|
							  QDir::NoDotAndDotDot|
							  QDir::Readable|
							  QDir::Files);
      
      return setImageFiles(imgfiles);
    }
  else
    return setImageFiles(files);

  return true;
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


bool
Jp2Plugin::loadJp2ImageProperties(QString filename)
{
  // Open the input file
  opj_stream_t *stream = opj_stream_create_default_file_stream(filename.toLatin1().data(), true);
  if (!stream)
    {
      QMessageBox::information(0, "Error", "Failed to create input stream");
      opj_stream_destroy(stream);
      return false;
    }
  
  // Create an openjpeg codec
  opj_codec_t *codec;
  if (filename.right(3).toLower() == "jp2") codec = opj_create_decompress(OPJ_CODEC_JP2);
  if (filename.right(3).toLower() == "jpt") opj_codec_t *codec = opj_create_decompress(OPJ_CODEC_JPT);
  if (filename.right(3).toLower() == "j2k") opj_codec_t *codec = opj_create_decompress(OPJ_CODEC_J2K);

  if (!codec)
    {
      QMessageBox::information(0, "Error", "Failed to create codec");
      opj_stream_destroy(stream);
      opj_destroy_codec(codec);
      return false;
    }
  
  // Setup decompression parameters
  opj_dparameters_t parameters;
  opj_set_default_decoder_parameters(&parameters);
  if (!opj_setup_decoder(codec, &parameters))
    {
      QMessageBox::information(0, "Error", "Failed to setup JP2 decoder");
      opj_stream_destroy(stream);
      opj_destroy_codec(codec);
      return false;
    }
  
  
  // Read the header of the JP2 file
  opj_image_t *image = 0;
  if (!opj_read_header(stream, codec, &image))
    {
      QMessageBox::information(0, "Error", "Failed to read header");
      opj_stream_destroy(stream);
      opj_destroy_codec(codec);
      if (image) opj_image_destroy(image);
      return false;
    }
  
  // Read image properties
  m_width = image->comps[0].w;
  m_height = image->comps[0].h;
  int numComponents = image->numcomps;
  int bitDepth = image->comps[0].prec;
  
  m_voxelType = _UChar;
  m_bytesPerVoxel = 1;
  if (bitDepth == 16)
    {
      m_voxelType = _UShort;
      m_bytesPerVoxel = 2;
    }  
  
  // Cleanup
  opj_image_destroy(image);
  opj_stream_destroy(stream);
  opj_destroy_codec(codec);
  
  return true;
}

void
Jp2Plugin::loadJp2Image(int i, uchar* tmp)
{
  QString filename = m_imageList[i];

  // Open the input file
  opj_stream_t *stream = opj_stream_create_default_file_stream(filename.toLatin1().data(), true);
  if (!stream)
    {
      QMessageBox::information(0, "Error", "Failed to create input stream");
      opj_stream_destroy(stream);
      return;
    }
  
  // Create an openjpeg codec
  opj_codec_t *codec;
  if (filename.right(3).toLower() == "jp2") codec = opj_create_decompress(OPJ_CODEC_JP2);
  if (filename.right(3).toLower() == "jpt") opj_codec_t *codec = opj_create_decompress(OPJ_CODEC_JPT);
  if (filename.right(3).toLower() == "j2k") opj_codec_t *codec = opj_create_decompress(OPJ_CODEC_J2K);
  if (!codec)
    {
      QMessageBox::information(0, "Error", "Failed to create codec");
      opj_stream_destroy(stream);
      opj_destroy_codec(codec);
      return;
    }
  
  // Setup decompression parameters
  opj_dparameters_t parameters;
  opj_set_default_decoder_parameters(&parameters);
  if (!opj_setup_decoder(codec, &parameters))
    {
      QMessageBox::information(0, "Error", "Failed to setup JP2 decoder");
      opj_stream_destroy(stream);
      opj_destroy_codec(codec);
      return;
    }
  
  
  // Read the header of the JP2 file
  opj_image_t *image = 0;
  if (!opj_read_header(stream, codec, &image))
    {
      QMessageBox::information(0, "Error", "Failed to read header");
      opj_stream_destroy(stream);
      opj_destroy_codec(codec);
      if (image) opj_image_destroy(image);
      return;
    }

  if (!opj_decode(codec, stream, image))
    {
      QMessageBox::information(0, "Error", "Failed to decode : "+filename);
      opj_stream_destroy(stream);
      opj_destroy_codec(codec);
      if (image) opj_image_destroy(image);
      return;
    }
  
  // Retrieve pixel data    
  if (m_bytesPerVoxel == 1)
    {
      int idx = 0;
      for (int i=0; i<m_height; i++)
	for (int j=0; j<m_width; j++)
	  {
	    tmp[idx] = image->comps[0].data[i*m_width + j];
	    idx++;
	  }
    }
  else
    {
      int idx = 0;
      ushort *tmpUS = (ushort*)tmp;
      for (int i=0; i<m_height; i++)
	for (int j=0; j<m_width; j++)
	  {
	    tmpUS[idx] = image->comps[0].data[i*m_width + j];
	    idx++;
	  }
    }
  
  
  // Cleanup
  opj_image_destroy(image);
  opj_stream_destroy(stream);
  //opj_destroy_codec(codec);
}

void
Jp2Plugin::findMinMaxandGenerateHistogram()
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

      loadJp2Image(i, tmp);

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
Jp2Plugin::findMinMax()
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

  m_rawMin = 10000000;
  m_rawMax = -10000000;
  for(int i=0; i<nX; i++)
    {
      progress.setValue((int)(100.0*(float)i/(float)nX));
      qApp->processEvents();

      loadJp2Image(i, tmp);

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
Jp2Plugin::generateHistogram()
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
  for(int i=0; i<nX; i++)
    {
      progress.setValue((int)(100.0*(float)i/(float)nX));
      qApp->processEvents();

      loadJp2Image(i, tmp);

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
Jp2Plugin::getDepthSlice(int slc,
			  uchar *slice)
{
  int nbytes = m_width*m_height*m_bytesPerVoxel;

  uchar *tmp1 = new uchar[nbytes];

  loadJp2Image(slc, tmp1);

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
Jp2Plugin::getWidthSlice(int slc,
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

      loadJp2Image(i, imgSlice);

      if (m_voxelType == _UChar)
	{
	  for(uint j=0; j<m_height; j++)
	    slice[i*m_height+j] = imgSlice[j*m_width + slc];
	}
      else if (m_voxelType == _UShort)
	{
	  ushort *p0 = (ushort*)slice;
	  ushort *p1 = (ushort*)imgSlice;
	  for(uint j=0; j<m_height; j++)
	    p0[i*m_height+j] = p1[j*m_width + slc];
	}
      else if (m_voxelType == _Float)
	{
	  float *p0 = (float*)slice;
	  float *p1 = (float*)imgSlice;
	  for(uint j=0; j<m_height; j++)
	    p0[i*m_height+j] = p1[j*m_width + slc];
	}
    }
  delete [] imgSlice;
  progress.setValue(100);
  qApp->processEvents();
}

void
Jp2Plugin::getHeightSlice(int slc,
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

      loadJp2Image(i, imgSlice);

      if (m_voxelType == _UChar)
	{
	  for(uint j=0; j<m_width; j++)
	    slice[i*m_width+j] = imgSlice[slc*m_width + j];
	}
      else if (m_voxelType == _UShort)
	{
	  ushort *p0 = (ushort*)slice;
	  ushort *p1 = (ushort*)imgSlice;
	  for(uint j=0; j<m_width; j++)
	    p0[i*m_width+j] = p1[slc*m_width + j];
	}
      else if (m_voxelType == _Float)
	{
	  float *p0 = (float*)slice;
	  float *p1 = (float*)imgSlice;
	  for(uint j=0; j<m_width; j++)
	    p0[i*m_width+j] = p1[slc*m_width + j];
	}
    }
  delete [] imgSlice;
  progress.setValue(100);
  qApp->processEvents();
}

QVariant
Jp2Plugin::rawValue(int d, int w, int h)
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

  loadJp2Image(d, imgSlice);

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
Jp2Plugin::saveTrimmed(QString trimFile,
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
      loadJp2Image(i, tmp1);

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
