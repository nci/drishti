#include <QtGui>
#include <netcdfcpp.h>
#include <mfhdf.h>
#include "common.h"
#include "hdf4plugin.h"

QStringList
HDF4Plugin::registerPlugin()
{
  QStringList regString;
  regString << "directory";
  regString << "HDF4 Directory";
  regString << "files";
  regString << "HDF4 Files";
  
  return regString;
}

void
HDF4Plugin::init()
{
  m_fileName.clear();
  m_imageList.clear();

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
HDF4Plugin::clear()
{
  m_fileName.clear();
  m_imageList.clear();

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
HDF4Plugin::set4DVolume(bool flag)
{
  m_4dvol = flag;
}

void
HDF4Plugin::voxelSize(float& vx, float& vy, float& vz)
  {
    vx = m_voxelSizeX;
    vy = m_voxelSizeY;
    vz = m_voxelSizeZ;
  }
QString HDF4Plugin::description() { return m_description; }
int HDF4Plugin::voxelType() { return m_voxelType; }
int HDF4Plugin::voxelUnit() { return m_voxelUnit; }
int HDF4Plugin::headerBytes() { return m_headerBytes; }

void
HDF4Plugin::setMinMax(float rmin, float rmax)
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
float HDF4Plugin::rawMin() { return m_rawMin; }
float HDF4Plugin::rawMax() { return m_rawMax; }
QList<uint> HDF4Plugin::histogram() { return m_histogram; }

void
HDF4Plugin::gridSize(int& d, int& w, int& h)
{
  d = m_depth;
  w = m_width;
  h = m_height;
}

QList<QString>
HDF4Plugin::listAllVariables()
{
  QList<QString> varNames;

  NcError err(NcError::verbose_nonfatal);

  NcFile dataFile((char*)m_fileName[0].toLatin1().data(),
		  NcFile::ReadOnly);

  if (!dataFile.is_valid())
    {
      QMessageBox::information(0, "Error",
			       QString("%1 is not a valid NetCDF file").arg(m_fileName[0]));
      return varNames; // empty
    }

  int nvars = dataFile.num_vars();
  
  int i;
  for (i=0; i < nvars; i++)
    {
      NcVar *var;
      var = dataFile.get_var(i);

      varNames.append(var->name());
    }

  dataFile.close();

  if (varNames.size() == 0)
    QMessageBox::information(0, "Error", "No variables found in the file");

  return varNames;
}

void
HDF4Plugin::replaceFile(QString flnm)
{
  m_fileName.clear();
  m_fileName << flnm;
}

bool
HDF4Plugin::setImageFiles(QStringList hdffiles)
{
  m_imageList.clear();

  for(uint i=0; i<hdffiles.size(); i++)
    {
      QFileInfo fileInfo(m_fileName[0], hdffiles[i]);
      QString imgfl = fileInfo.absoluteFilePath();
      m_imageList.append(imgfl);
    }

  m_depth = m_imageList.size();


  /* Open the file and initiate the SD interface. */
  int32 sd_id = SDstart(strdup(m_imageList[0].toLatin1().data()),
			DFACC_READ);
  if (sd_id < 0) {
    QMessageBox::information(0, 
			     "Error",
			     QString("Failed to open %1").arg(m_imageList[0]));
    return false;
  }
    
  /* Determine the contents of the file. */
  int32 dim_sizes[MAX_VAR_DIMS];
  int32 rank, num_type, attributes, istat;
  char name[64];
  int32 n_datasets, n_file_attrs;

  istat = SDfileinfo(sd_id, &n_datasets, &n_file_attrs);

  /* Access the name of every data set in the file. */
  QStringList varNames;
  for (int32 index = 0; index < n_datasets; index++)
    {
      int32 sds_id = SDselect(sd_id, index);
      
      istat = SDgetinfo(sds_id, name, &rank, dim_sizes,	\
			&num_type, &attributes);
            
      istat = SDendaccess(sds_id);

      if (rank == 2)
	varNames.append(name);
    }
  
  QString var;
  if (varNames.size() == 0) {
    QMessageBox::information(0, 
			     "Error",
			     QString("No variables in file with rank of 2"));
    return false;
  }
  else if (varNames.size() == 1)
    {
      var = varNames[0];
    }
  else
    {
      var = QInputDialog::getItem(0,
				  "Select Variable to Extract",
				  "Variable Names",
				  varNames,
				  0,
				  false);
    }
  
  m_Index = 0;
  for (int32 index = 0; index < n_datasets; index++)
    {
      int32 sds_id = SDselect(sd_id, index);
      
      istat = SDgetinfo(sds_id, name, &rank, dim_sizes,	\
			&num_type, &attributes);
      
      istat = SDendaccess(sds_id);
      
      if (var == QString(name))
	{
	  m_Index = index;
	  break;
	}
    }

  {    
    int32 sds_id = SDselect(sd_id, m_Index);
    
    istat = SDgetinfo(sds_id,
		      name,
		      &rank,
		      dim_sizes,
		      &num_type,
		      &attributes);
    
    istat = SDendaccess(sds_id);
  }

  /* Terminate access to the SD interface and close the file. */
  istat = SDend(sd_id);


  if (num_type == DFNT_CHAR8)
    m_voxelType = _Char;
  else if (num_type == DFNT_UCHAR8)
    m_voxelType = _UChar;
  else if (num_type == DFNT_INT8)
    m_voxelType = _Char;
  else if (num_type == DFNT_UINT8)
    m_voxelType = _UChar;
  else if (num_type == DFNT_INT16)
    m_voxelType = _Short;
  else if (num_type == DFNT_UINT16)
    m_voxelType = _UShort;
  else if (num_type == DFNT_INT32)
    m_voxelType = _Int;
  else if (num_type == DFNT_FLOAT32)
    m_voxelType = _Float;
  else
    {
      QMessageBox::information(0, "Error",
			       QString("Cannot handle datatype %1").arg(num_type));
      return false;
    }

  m_width = dim_sizes[0];
  m_height = dim_sizes[1];

  m_bytesPerVoxel = 1;
  if (m_voxelType == _UChar) m_bytesPerVoxel = 1;
  else if (m_voxelType == _Char) m_bytesPerVoxel = 1;
  else if (m_voxelType == _UShort) m_bytesPerVoxel = 2;
  else if (m_voxelType == _Short) m_bytesPerVoxel = 2;
  else if (m_voxelType == _Int) m_bytesPerVoxel = 4;
  else if (m_voxelType == _Float) m_bytesPerVoxel = 4;

  m_headerBytes = 0;

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

bool
HDF4Plugin::setFile(QStringList files)
{
  m_fileName = files;

  QFileInfo f(m_fileName[0]);
  if (f.isDir())
    {
      // list all hdf4 image files in the directory
      QStringList imageNameFilter;
      imageNameFilter << "*.hdf";
      imageNameFilter << "*.hdf4";
      imageNameFilter << "*.h4";
      QStringList hdffiles= QDir(m_fileName[0]).entryList(imageNameFilter,
							  QDir::NoSymLinks|
							  QDir::NoDotAndDotDot|
							  QDir::Readable|
							  QDir::Files);
      return setImageFiles(hdffiles);
    }
  else
    return setImageFiles(files);

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
HDF4Plugin::findMinMaxandGenerateHistogram()
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

  int32 start[2], edges[2];
  start[0] = start[1] = 0;
  edges[0] = m_width;
  edges[1] = m_height;

  for(uint i=0; i<m_depth; i++)
    {
      progress.setValue((int)(100.0*(float)i/(float)m_depth));
      qApp->processEvents();

      int32 sd_id = SDstart(m_imageList[i].toLatin1().data(),
			    DFACC_READ);
      int32 sds_id = SDselect(sd_id, m_Index);
      int status = SDreaddata(sds_id,
			      start, NULL, edges,
			      (VOIDP)tmp);
      status = SDendaccess(sds_id);
      status = SDend(sd_id);

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
    for(uint j=0; j<nY*nZ; j++)				\
      {							\
	float val = ptr[j];				\
	m_rawMin = qMin(m_rawMin, val);			\
	m_rawMax = qMax(m_rawMax, val);			\
      }							\
  }


void
HDF4Plugin::findMinMax()
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

  int32 start[2], edges[2];
  start[0] = start[1] = 0;
  edges[0] = m_width;
  edges[1] = m_height;

  for(uint i=0; i<m_depth; i++)
    {
      progress.setValue((int)(100.0*(float)i/(float)nX));
      qApp->processEvents();

      int32 sd_id = SDstart(m_imageList[i].toLatin1().data(),
			    DFACC_READ);
      int32 sds_id = SDselect(sd_id, m_Index);
      int status = SDreaddata(sds_id,
			      start, NULL, edges,
			      (void*)tmp);
      status = SDendaccess(sds_id);
      status = SDend(sd_id);

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
    for(uint j=0; j<nY*nZ; j++)				\
      {							\
	float fidx = (ptr[j]-m_rawMin)/rSize;		\
	fidx = qBound(0.0f, fidx, 1.0f);		\
	int idx = fidx*(histogramSize - 1);		\
	m_histogram[idx]+=1;				\
      }							\
  }

void
HDF4Plugin::generateHistogram()
{
  float rSize = m_rawMax-m_rawMin;

  QProgressDialog progress("Generating Histogram",
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

  int32 start[2], edges[2];
  start[0] = start[1] = 0;
  edges[0] = m_width;
  edges[1] = m_height;

  for(uint i=0; i<nX; i++)
    {
      progress.setValue((int)(100.0*(float)i/(float)nX));
      qApp->processEvents();

      int32 sd_id = SDstart(m_imageList[i].toLatin1().data(),
			    DFACC_READ);
      int32 sds_id = SDselect(sd_id, m_Index);
      int status = SDreaddata(sds_id,
			      start, NULL, edges,
			      (VOIDP)tmp);
      status = SDendaccess(sds_id);
      status = SDend(sd_id);


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

  progress.setValue(100);
  qApp->processEvents();
}


void
HDF4Plugin::getDepthSlice(int slc,
			 uchar *slice)
{
  int nbytes = m_width*m_height*m_bytesPerVoxel;

  int32 start[2], edges[2];
  start[0] = start[1] = 0;
  edges[0] = m_width;
  edges[1] = m_height;

  int32 sd_id = SDstart(m_imageList[slc].toLatin1().data(),
			DFACC_READ);
  int32 sds_id = SDselect(sd_id, m_Index);
  int status = SDreaddata(sds_id,
			  start, NULL, edges,
			  (VOIDP)slice);
  status = SDendaccess(sds_id);
  status = SDend(sd_id);
}

//void
//HDF4Plugin::getWidthSlice(int slc,
//			  uchar *slice)
//{
//  int nbytes = m_depth*m_height*m_bytesPerVoxel;
//
//  uchar *hdftmp = new uchar[m_height*m_bytesPerVoxel];
//
//  int32 start[2], edges[2];
//  start[0] = 0;
//  start[1] = slc;
//  edges[0] = 1;
//  edges[1] = m_height;
//
//  for(uint i=0; i<m_depth; i++)
//    {
//      int32 sd_id = SDstart(m_imageList[i].toLatin1().data(),
//			    DFACC_READ);
//      int32 sds_id = SDselect(sd_id, m_Index);
//      int status = SDreaddata(sds_id,
//			      start, NULL, edges,
//			      (VOIDP)hdftmp);
//      status = SDendaccess(sds_id);
//      status = SDend(sd_id);
//
//      for(uint j=0; j<m_height; j++)
//	slice[i*m_height+j] = hdftmp[j];
//    }
//}
//
//void
//HDF4Plugin::getHeightSlice(int slc,
//			   uchar *slice)
//{
//  int nbytes = m_depth*m_width*m_bytesPerVoxel;
//
//  uchar *hdftmp = new uchar[m_width*m_bytesPerVoxel];
//
//  int32 start[2], edges[2];
//  start[0] = slc;
//  start[1] = 0;
//  edges[0] = m_width;
//  edges[1] = 1;
//
//  for(uint i=0; i<m_depth; i++)
//    {
//      int32 sd_id = SDstart(m_imageList[i].toLatin1().data(),
//			    DFACC_READ);
//      int32 sds_id = SDselect(sd_id, m_Index);
//      int status = SDreaddata(sds_id,
//			      start, NULL, edges,
//			      (VOIDP)hdftmp);
//      status = SDendaccess(sds_id);
//      status = SDend(sd_id);
//
//      for(uint j=0; j<m_width; j++)
//	slice[i*m_width+j] = hdftmp[j];
//    }
//}

QVariant
HDF4Plugin::rawValue(int d, int w, int h)
{
  QVariant v;

  if (d < 0 || d >= m_depth ||
      w < 0 || w >= m_width ||
      h < 0 || h >= m_height)
    {
      v = QVariant("OutOfBounds");
      return v;
    }

  uchar *hdftmp = new uchar[m_bytesPerVoxel];
  int32 start[2], edges[2];
  start[0] = w;
  start[1] = h;
  edges[0] = 1;
  edges[1] = 1;

  int32 sd_id = SDstart(m_imageList[d].toLatin1().data(),
			DFACC_READ);
  int32 sds_id = SDselect(sd_id, m_Index);
  int status = SDreaddata(sds_id,
			  start, NULL, edges,
			  (VOIDP)hdftmp);
  if (status == -1)
    QMessageBox::information(0, "error", "Cannot read");

  status = SDendaccess(sds_id);
  status = SDend(sd_id);

  if (m_voxelType == _UChar)
    {
      uchar *aptr = (uchar*) hdftmp;
      uchar a = *aptr;
      v = QVariant((uint)a);
    }
  else if (m_voxelType == _Char)
    {
      char *aptr = (char*) hdftmp;
      char a = *aptr;
      v = QVariant((int)a);
    }
  else if (m_voxelType == _UShort)
    {
      ushort *aptr = (ushort*) hdftmp;
      ushort a = *aptr;
      v = QVariant((uint)a);
    }
  else if (m_voxelType == _Short)
    {
      short *aptr = (short*) hdftmp;
      short a = *aptr;
      v = QVariant((int)a);
    }
  else if (m_voxelType == _Int)
    {
      int *aptr = (int*) hdftmp;
      int a = *aptr;
      v = QVariant((int)a);
    }
  else if (m_voxelType == _Float)
    {
      float *aptr = (float*) hdftmp;
      double a = *aptr;
      v = QVariant((double)a);
    }

  return v;
}

//void
//HDF4Plugin::saveTrimmed(QString trimFile,
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
//  int nbytes = m_height*m_width*m_bytesPerVoxel;
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
//  int32 start[2], edges[2];
//  start[0] = 0;
//  start[1] = 0;
//  edges[0] = m_width;
//  edges[1] = m_height;
//
//  for(uint i=dmin; i<=dmax; i++)
//    {
//      int32 sd_id = SDstart(m_imageList[i].toLatin1().data(),
//			    DFACC_READ);
//      int32 sds_id = SDselect(sd_id, m_Index);
//      int status = SDreaddata(sds_id,
//			      start, NULL, edges,
//			      (VOIDP)tmp);
//      status = SDendaccess(sds_id);
//      status = SDend(sd_id);
//      
//      for(uint j=wmin; j<=wmax; j++)
//	{
//	  memcpy(tmp+(j-wmin)*mZ*m_bytesPerVoxel,
//		 tmp+(j*nZ + hmin)*m_bytesPerVoxel,
//		 mZ*m_bytesPerVoxel);
//	}
//
//      fout.write((char*)tmp, mY*mZ*m_bytesPerVoxel);
//      
//      progress.setValue((int)(100*(float)(i-dmin)/(float)mX));
//      qApp->processEvents();
//    }
//
//  fout.close();
//
//  delete [] tmp;
//
//  m_headerBytes = 13; // to be used for applyMapping function
//}
