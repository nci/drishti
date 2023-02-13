#include <QtGui>
#include <ncFile.h>
#include <ncDim.h>
#include <ncException.h>
#include <netcdf>
#include "common.h"
#include <map>
#include "nc4plugin.h"

using namespace std;
using namespace netCDF;
using namespace netCDF::exceptions;

QStringList
NcPlugin::registerPlugin()
{
  QStringList regString;
  regString << "directory";
  regString << "NetCDF Directory";
  regString << "files";
  regString << "NetCDF Files";
  
  return regString;
}

void
NcPlugin::init()
{
  m_fileName.clear();
  m_varName.clear();

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
NcPlugin::clear()
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
NcPlugin::set4DVolume(bool flag)
{
  m_4dvol = flag;
}

void
NcPlugin::voxelSize(float& vx, float& vy, float& vz)
  {
    vx = m_voxelSizeX;
    vy = m_voxelSizeY;
    vz = m_voxelSizeZ;
  }
QString NcPlugin::description() { return m_description; }
int NcPlugin::voxelType() { return m_voxelType; }
int NcPlugin::voxelUnit() { return m_voxelUnit; }
int NcPlugin::headerBytes() { return m_headerBytes; }

void
NcPlugin::setMinMax(float rmin, float rmax)
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
float NcPlugin::rawMin() { return m_rawMin; }
float NcPlugin::rawMax() { return m_rawMax; }
QList<uint> NcPlugin::histogram() { return m_histogram; }

void
NcPlugin::gridSize(int& d, int& w, int& h)
{
  d = m_depth;
  w = m_width;
  h = m_height;
}

QList<QString>
NcPlugin::listAllVariables()
{
  QList<QString> varNames;

  NcFile dataFile;
  try
    {
      dataFile.open(m_fileName[0].toStdString(),
		    NcFile::read);
    }
  catch(NcException &e)
    {
      QMessageBox::information(0, "Error",
			       QString("%1 is not a valid NetCDF file").\
			       arg(m_fileName[0]));
      return varNames; // empty
    }


  multimap<string, NcVar> groupMap;
  groupMap = dataFile.getVars();
  for (const auto &p : groupMap)
    {
      varNames.append(QString::fromStdString(p.first));
    }

  dataFile.close();

  if (varNames.size() == 0)
    QMessageBox::information(0, "Error", "No variables found in the file");

  return varNames;
}

QList<QString>
NcPlugin::listAllAttributes()
{
  QList<QString> attNames;

  NcFile dataFile;
  try
    {
      dataFile.open(m_fileName[0].toStdString(),
		    NcFile::read);
    }
  catch(NcException &e)
    {
      QMessageBox::information(0, "Error",
			       QString("%1 is not a valid NetCDF file").\
			       arg(m_fileName[0]));
      return attNames; // empty
    }

  
  multimap<string, NcGroupAtt> groupMap;
  groupMap = dataFile.getAtts();
  for (const auto &p : groupMap)
    {
      attNames.append(QString::fromStdString(p.first));
    }
  

  dataFile.close();

  if (attNames.size() == 0)
    QMessageBox::information(0, "Error", "No attributes found in the file");

  return attNames;
}

void
NcPlugin::replaceFile(QString flnm)
{
  m_fileName.clear();
  m_fileName << flnm;
}

bool
NcPlugin::setFile(QStringList files)
{  
  if (files.size() == 0)
    return false;

  QFileInfo f(files[0]);
  if (f.isDir())
    {
      // list all image files in the directory
      QStringList imageNameFilter;
      imageNameFilter << "*.nc";
      QStringList ncfiles= QDir(files[0]).entryList(imageNameFilter,
						    QDir::NoSymLinks|
						    QDir::NoDotAndDotDot|
						    QDir::Readable|
						    QDir::Files);

      if (ncfiles.size() == 0)
	return false;
      
      m_fileName.clear();
      for(uint i=0; i<ncfiles.size(); i++)
	{
	  QFileInfo fileInfo(files[0], ncfiles[i]);
	  QString ncfl = fileInfo.absoluteFilePath();
	  m_fileName << ncfl;
	}
    }
  else
    m_fileName = files;


  
  QList<QString> varNames;
  QList<QString> allVars = listAllVariables();

  
  if (allVars.size() == 0)
    return false;

  QList<QString> allAtts = listAllAttributes();

  NcFile dataFile;
  try
    {
      dataFile.open(m_fileName[0].toStdString(),
		    NcFile::read);
    }
  catch(NcException &e)
    {
      QMessageBox::information(0, "Error",
			       QString("%1 is not a valid NetCDF file"). \
			       arg(m_fileName[0]));
      return false;
    }

  //---------------------------------------------------------
  // -- Choose a variable for extraction --------------------
  for(uint i=0; i<allVars.size(); i++)
    {
      NcVar ncvar;
      ncvar = dataFile.getVar(allVars[i].toStdString());
      if (ncvar.getDimCount() == 3)
	varNames.append(allVars[i]);
    }
  if (varNames.size() == 0)
    {
      QMessageBox::information(0, "Error", "No 3D variables found in the file");
      return false;
    }

  if (varNames.size() == 1)
    {
      m_varName = varNames[0];
    }
  else
    {
      bool ok;
      QString varName;  
      varName = QInputDialog::getItem(0,
				      "Choose a variable for extraction",
				      "Variables",
				      varNames,
				      0,
				      false,
				      &ok);
      if (ok)
	m_varName = varName;
      else
	{
	  m_varName = varNames[0];
	  QMessageBox::information(0, "Variable",
				   QString("extracting %1").arg(m_varName));
	}
    }
  //---------------------------------------------------------

  NcVar ncvar;
  ncvar = dataFile.getVar(m_varName.toStdString());

  NcType vtype = ncvar.getType();
  m_voxelType = _UChar; 
  switch (vtype.getTypeClass()) 
    {	  
    case NC_BYTE :
      m_voxelType = _UChar; break;
    case NC_CHAR :
      m_voxelType = _Char; break;
    case NC_SHORT :
      m_voxelType = _UShort; break;
    case NC_INT :
      m_voxelType = _Int; break;
    case NC_FLOAT :
      m_voxelType = _Float; break;
    }

  
  // ---------------------
  // get voxel size and unit if available
  int ati = allAtts.indexOf("voxel_size_xyz");
  if (ati > -1)
    {
      NcGroupAtt att = dataFile.getAtt("voxel_size_xyz");
      double values[10];
      att.getValues(&values[0]);
      
      m_voxelSizeX = values[0];
      m_voxelSizeY = values[1];
      m_voxelSizeZ = values[2];
    }
  else // check with variable, it it has this attribute
    {
      NcGroupAtt att = dataFile.getAtt("voxel_size");
      if (!att.isNull())
	{
	  double values[10];
	  att.getValues(&values[0]);
	  m_voxelSizeX = values[0];
	  m_voxelSizeY = values[1];
	  m_voxelSizeZ = values[2];
	}
    }
  
  ati = allAtts.indexOf("voxel_unit");
  if (ati > -1)
    {
      NcGroupAtt att = dataFile.getAtt("voxel_unit");
      string values;
      att.getValues(values);
      
      if (values == "mm")
	m_voxelUnit = _Millimeter;
    }
  // ---------------------


  vector<NcDim> sizes;
  sizes = ncvar.getDims();
  m_depth = sizes[0].getSize();
  m_width = sizes[1].getSize();
  m_height = sizes[2].getSize();

  dataFile.close();

  
  m_bytesPerVoxel = 1;
  if (m_voxelType == _UChar) m_bytesPerVoxel = 1;
  else if (m_voxelType == _Char) m_bytesPerVoxel = 1;
  else if (m_voxelType == _UShort) m_bytesPerVoxel = 2;
  else if (m_voxelType == _Short) m_bytesPerVoxel = 2;
  else if (m_voxelType == _Int) m_bytesPerVoxel = 4;
  else if (m_voxelType == _Float) m_bytesPerVoxel = 4;

  // ---------------------
  if (!m_4dvol)
    {
      m_depth = 0;
      m_depthList.clear();
      for(uint i=0; i<m_fileName.size(); i++)
	{
	  NcFile ncfile;
	  try
	    {
	      ncfile.open(m_fileName[i].toStdString(),
			    NcFile::read);
	    }
	  catch(NcException &e)
	    {
	      QMessageBox::information(0, "Error",
				       QString("%1 is not a valid NetCDF file"). \
				       arg(m_fileName[i]));
	      return false;
	    }
	  NcVar ncvar;
	  ncvar = ncfile.getVar(m_varName.toStdString());
	  m_depth += ncvar.getDim(0).getSize();
	  m_depthList.append(m_depth);
	  ncfile.close();
	}
    }
  // ---------------------


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
NcPlugin::getSlice(int sliceType, int a, int b, NcVar ncvar, int slc, uchar *tmp)
{
  std::vector<size_t> start(3);
  start[0] = 0;
  start[1] = 0;
  start[2] = 0;
  start[sliceType] = slc;
  
  std::vector<size_t> count(3);
  if (sliceType == 0)
    {
      count[0] = 1;
      count[1] = a;
      count[2] = b;
    }
  if (sliceType == 1)
    {
      count[0] = a;
      count[1] = 1;
      count[2] = b;
    }
  if (sliceType == 2)
    {
      count[0] = a;
      count[1] = b;
      count[2] = 1;
    }
  
	  
  if (ncvar.getType() == ncUbyte)
    ncvar.getVar(start, count, (unsigned char*)tmp);
  else if (ncvar.getType() == ncByte || ncvar.getType() == ncChar)
    ncvar.getVar(start, count, (uchar*)tmp);
  else if (ncvar.getType() == ncShort)
    ncvar.getVar(start, count, (short*)tmp);
  else if (ncvar.getType() == ncInt)
    ncvar.getVar(start, count, (int*)tmp);
  else if (ncvar.getType() == ncFloat)
    ncvar.getVar(start, count, (float*)tmp);
  else if (ncvar.getType() == ncDouble)
    ncvar.getVar(start, count, (double*)tmp);  
}


void
NcPlugin::findMinMaxandGenerateHistogram()
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

  m_rawMin = 10000000;
  m_rawMax = -10000000;

  int nfls = m_fileName.size();
  if (m_4dvol) nfls = 1;
  for(uint nf=0; nf<nfls; nf++)
    {
      QFileInfo finfo(m_fileName[nf]);
      progress.setLabelText(finfo.fileName());
      //progress.setLabelText(m_fileName[nf]);

      NcFile dataFile;
      try
	{
	  dataFile.open(m_fileName[nf].toStdString(),
			NcFile::read);
	}
      catch(NcException &e)
	{
	  QMessageBox::information(0, "Error",
				   QString("%1 is not a valid NetCDF file"). \
				   arg(m_fileName[nf]));
	  return;
	}
      
      NcVar ncvar;
      ncvar = dataFile.getVar(m_varName.toStdString());
      
      int iEnd = ncvar.getDim(0).getSize();
      for(uint i=0; i<iEnd; i++)
	{
	  progress.setValue((int)(100.0*(float)i/(float)iEnd));
	  qApp->processEvents();

	  getSlice(0, m_width, m_height, ncvar, i, tmp);	  
	  
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

      dataFile.close();
    }

  delete [] tmp;

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
NcPlugin::findMinMax()
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

  int nfls = m_fileName.size();
  if (m_4dvol) nfls = 1;
  for(uint nf=0; nf<nfls; nf++)
    {
      NcFile dataFile;
      try
	{
	  dataFile.open(m_fileName[nf].toStdString(),
			NcFile::read);
	}
      catch(NcException &e)
	{
	  QMessageBox::information(0, "Error",
				   QString("%1 is not a valid NetCDF file"). \
				   arg(m_fileName[nf]));
	  return;
	}
      
      NcVar ncvar;
      ncvar = dataFile.getVar(m_varName.toStdString());

      int iEnd = ncvar.getDim(0).getSize();
      for(uint i=0; i<iEnd; i++)
	{
	  progress.setValue((int)(100.0*(float)i/(float)iEnd));
	  qApp->processEvents();
	  
	  getSlice(0, m_width, m_height, ncvar, i, tmp);
	  
	  
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
      dataFile.close();
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
	int idx = fidx*histogramSize;			\
	m_histogram[idx]+=1;				\
      }							\
  }

void
NcPlugin::generateHistogram()
{
  QProgressDialog progress("Generating Histogram",
			   QString(),
			   0, 100,
			   0);
  progress.setMinimumDuration(0);

  float rSize = m_rawMax-m_rawMin;
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

  int nX, nY, nZ;
  nX = m_depth;
  nY = m_width;
  nZ = m_height;

  int nbytes = nY*nZ*m_bytesPerVoxel;
  uchar *tmp = new uchar[nbytes];

  int histogramSize = m_histogram.size()-1;

  int nfls = m_fileName.size();
  if (m_4dvol) nfls = 1;
  for(uint nf=0; nf<nfls; nf++)
    {

      NcFile dataFile;
      try
	{
	  dataFile.open(m_fileName[nf].toStdString(),
			NcFile::read);
	}
      catch(NcException &e)
	{
	  QMessageBox::information(0, "Error",
				   QString("%1 is not a valid NetCDF file"). \
				   arg(m_fileName[nf]));
	  return;
	}
      
      NcVar ncvar;
      ncvar = dataFile.getVar(m_varName.toStdString());
      
      int iEnd = ncvar.getDim(0).getSize();
      for(uint i=0; i<iEnd; i++)
	{
	  progress.setValue((int)(100.0*(float)i/(float)iEnd));
	  qApp->processEvents();
	  
	  getSlice(0, m_width, m_height, ncvar, i, tmp);
	  
	  
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
      dataFile.close();
    }

  delete [] tmp;

  progress.setValue(100);
  qApp->processEvents();
}


void
NcPlugin::getDepthSlice(int slc,
			     uchar* slice)
{
  int nf = 0;
  int slcno = slc;
  for(uint fl=0; fl<m_fileName.size(); fl++)
    {
      if (m_depthList[fl] > slc)
	{
	  nf = fl;
	  if (fl == 0)
	    slcno = slc;
	  else
	    slcno = slc-m_depthList[fl-1];
	  break;
	}
    }

  NcFile dataFile((char *)m_fileName[nf].toLatin1().data(),
		  NcFile::read);
  NcVar ncvar;
  ncvar = dataFile.getVar(m_varName.toStdString());
  
  getSlice(0, m_width, m_height, ncvar, slcno, slice);

  dataFile.close();
}

void
NcPlugin::getWidthSlice(int slc,
			uchar *slice)
{
  for(uint nf=0; nf<m_fileName.size(); nf++)
    {
      NcFile dataFile((char *)m_fileName[nf].toLatin1().data(),
		      NcFile::read);

      int depth;
      uchar *ptmp;
      if (nf > 0)
	{
	  depth = m_depthList[nf]-m_depthList[nf-1];
	  ptmp = slice + m_depthList[nf-1]*m_height*m_bytesPerVoxel;
	}
      else
	{
	  depth = m_depthList[0];
	  ptmp = slice;
	}

      
      NcVar ncvar;
      ncvar = dataFile.getVar(m_varName.toStdString());
      
      getSlice(1, depth, m_height, ncvar, slc, ptmp);

      dataFile.close();
    }  
}

void
NcPlugin::getHeightSlice(int slc,
			 uchar *slice)
{
  for(uint nf=0; nf < m_fileName.size(); nf++)
    {
      NcFile dataFile((char *)m_fileName[nf].toLatin1().data(),
		      NcFile::read);
      
      int depth;
      uchar *ptmp;
      if (nf > 0)
	{
	  depth = m_depthList[nf]-m_depthList[nf-1];
	  ptmp = slice + m_depthList[nf-1]*m_width*m_bytesPerVoxel;
	}
      else
	{
	  depth = m_depthList[0];
	  ptmp = slice;
	}

      
      NcVar ncvar;
      ncvar = dataFile.getVar(m_varName.toStdString());
      
      getSlice(2, depth, m_width, ncvar, slc, ptmp);

      dataFile.close();
    }
}

QVariant
NcPlugin::rawValue(int d, int w, int h)
{
  QVariant v;

  if (d < 0 || d >= m_depth ||
      w < 0 || w >= m_width ||
      h < 0 || h >= m_height)
    {
      v = QVariant("OutOfBounds");
      return v;
    }


  //------ cater for multiple netCDF files ------
  int nf = 0;
  int slcno = d;
  for(uint fl=0; fl<m_fileName.size(); fl++)
    {
      if (m_depthList[fl] > d)
	{
	  nf = fl;
	  if (fl == 0)
	    slcno = d;
	  else
	    slcno = d-m_depthList[fl-1];
	  break;
	}
    }
  //----------------------------------------


  NcFile dataFile((char *)m_fileName[nf].toLatin1().data(),
		  NcFile::read);
  std::vector<size_t> index(3);
  index[0] = slcno;
  index[1] = w;
  index[2] = h;

  NcVar ncvar;
  ncvar = dataFile.getVar(m_varName.toStdString());
  
  if (m_voxelType == _UChar)
    {
      unsigned char a;
      ncvar.getVar(index, (unsigned char*)&a);
      v = QVariant((uint)a);
    }
  else if (m_voxelType == _Char)
    {
      char a;
      ncvar.getVar(index, (char*)&a);
      v = QVariant((int)a);
    }
  else if (m_voxelType == _UShort)
    {
      unsigned short a;
      ncvar.getVar(index, (unsigned short*)&a);
      v = QVariant((uint)a);
    }
  else if (m_voxelType == _Short)
    {
      short a;
      ncvar.getVar(index, (short*)&a);
      v = QVariant((int)a);
    }
  else if (m_voxelType == _Int)
    {
      int a;
      ncvar.getVar(index, (int*)&a);
      v = QVariant((int)a);
    }
  else if (m_voxelType == _Float)
    {
      float a;
      ncvar.getVar(index, (float*)&a);
      v = QVariant((double)a);
    }
  dataFile.close();

  return v;
}

void
NcPlugin::saveTrimmed(QString trimFile,
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



  int nfStart, nfEnd;
  int slcStart, slcEnd;
  //------ cater for multiple netCDF files ------
  for(uint fl=0; fl<m_fileName.size(); fl++)
    {
      if (m_depthList[fl] > dmin)
	{
	  nfStart = fl;
	  if (fl == 0)
	    slcStart = dmin;
	  else
	    slcStart = dmin-m_depthList[fl-1];
	  break;
	}
    }
  for(uint fl=0; fl<m_fileName.size(); fl++)
    {
      if (m_depthList[fl] > dmax)
	{
	  nfEnd = fl;
	  if (fl == 0)
	    slcEnd = dmax;
	  else
	    slcEnd = dmax-m_depthList[fl-1];
	  break;
	}
    }
  //----------------------------------------

  uint nslc = 0;
  for(uint nf=nfStart; nf<=nfEnd; nf++)
    {
      NcFile dataFile;
      dataFile.open(m_fileName[nf].toStdString(),
		    NcFile::read);
      NcVar ncvar;
      ncvar = dataFile.getVar(m_varName.toStdString());

      uint dStart, dEnd;
      dStart = 0;
      dEnd = ncvar.getDim(0).getSize()-1;

      if (nf == nfStart) dStart = slcStart;
      if (nf == nfEnd) dEnd = slcEnd;

      for(uint i=dStart; i<=dEnd; i++)
	{
	  getSlice(0, m_width, m_height, ncvar, i, tmp);
	  	  
	  for(uint j=wmin; j<=wmax; j++)
	    {
	      memcpy(tmp+(j-wmin)*mZ*m_bytesPerVoxel,
		     tmp+(j*nZ + hmin)*m_bytesPerVoxel,
		     mZ*m_bytesPerVoxel);
	    }
	  fout.write((char*)tmp, mY*mZ*m_bytesPerVoxel);
	  progress.setValue((int)(100*(float)nslc/(float)mX));
	  qApp->processEvents();
	  nslc++;
	}
      dataFile.close();
    }

  fout.close();  

  delete [] tmp;

  m_headerBytes = 13; // to be used in applyMapping
}
