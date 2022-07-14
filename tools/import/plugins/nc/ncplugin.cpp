#include <QtGui>
#include <netcdfcpp.h>
#include "common.h"
#include "ncplugin.h"

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

  NcError err(NcError::verbose_nonfatal);

  NcFile dataFile((char*)m_fileName[0].toLatin1().data(),
		  NcFile::ReadOnly);

  if (!dataFile.is_valid())
    {
      QMessageBox::information(0, "Error",
			       QString("%1 is not a valid NetCDF file").\
			       arg(m_fileName[0]));
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

QList<QString>
NcPlugin::listAllAttributes()
{
  QList<QString> attNames;

  NcError err(NcError::verbose_nonfatal);

  NcFile dataFile((char*)m_fileName[0].toLatin1().data(),
		  NcFile::ReadOnly);

  if (!dataFile.is_valid())
    {
      QMessageBox::information(0, "Error",
			       QString("%1 is not a valid NetCDF file").\
			       arg(m_fileName[0]));
      return attNames; // empty
    }

  int natts = dataFile.num_atts();
  
  int i;
  for (i=0; i < natts; i++)
    {
      NcAtt *att;
      att = dataFile.get_att(i);

      attNames.append(att->name());
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

  NcError err(NcError::verbose_nonfatal);

  NcFile dataFile((char*)m_fileName[0].toLatin1().data(),
		  NcFile::ReadOnly);

  if (!dataFile.is_valid())
    {
      QMessageBox::information(0, "Error",
			       QString("%1 is not a valid NetCDF file").\
			       arg(m_fileName[0]));
      return false;
    }

  //---------------------------------------------------------
  // -- Choose a variable for extraction --------------------
  for(uint i=0; i<allVars.size(); i++)
    {
      NcVar *ncvar;
      ncvar = dataFile.get_var((char *)allVars[i].toLatin1().data());
      if (ncvar->num_dims() == 3)
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

  NcVar *ncvar;
  ncvar = dataFile.get_var((char *)m_varName.toLatin1().data());

  m_voxelType = _UChar;
  switch (ncvar->type())
    {	  
    case ncByte :
      m_voxelType = _UChar; break;
    case ncChar :
      m_voxelType = _Char; break;
    case ncShort :
      m_voxelType = _UShort; break;
    case ncInt :
      m_voxelType = _Int; break;
    case ncFloat :
      m_voxelType = _Float; break;
    }

  long sizes[100];
  memset(sizes, 0, 400);
  for(uint i=0; i<ncvar->num_dims(); i++)
    sizes[i] = ncvar->get_dim(i)->size();

  // ---------------------
  // get voxel size and unit if available
  int ati = allAtts.indexOf("voxel_size_xyz");
  if (ati > -1)
    {
      NcAtt* att = dataFile.get_att("voxel_size_xyz");
      m_voxelSizeX = att->as_float(0);
      m_voxelSizeY = att->as_float(1);
      m_voxelSizeZ = att->as_float(2);
    }
  else // check with variable, it it has this attribute
    {
      int nats = ncvar->num_atts();  
      for (int ni=0; ni<nats; ni++)
	{
	  NcAtt *att = ncvar->get_att(ni);
	  if (QString(att->name()) == "voxel_size")
	    {
	      m_voxelSizeX = att->as_float(0);
	      m_voxelSizeY = att->as_float(1);
	      m_voxelSizeZ = att->as_float(2);
	      break;
	    }
	}
    }
  ati = allAtts.indexOf("voxel_unit");
  if (ati > -1)
    {
      NcAtt* att = dataFile.get_att("voxel_unit");
      QString str(att->as_string(0));
      if (str == "mm")
	m_voxelUnit = _Millimeter;
    }
  else // check with variable, it it has this attribute
    {
      int nats = ncvar->num_atts();  
      for (int ni=0; ni<nats; ni++)
	{
	  NcAtt *att = ncvar->get_att(ni);
	  if (QString(att->name()) == "voxel_unit")
	    {
	      QString str(att->as_string(0));
	      if (str == "mm")
		m_voxelUnit = _Millimeter;
	      break;
	    }
	}
    }
  // ---------------------

  dataFile.close();

  //m_depth = sizes[0];
  m_width = sizes[1];
  m_height = sizes[2];


  m_bytesPerVoxel = 1;
  if (m_voxelType == _UChar) m_bytesPerVoxel = 1;
  else if (m_voxelType == _Char) m_bytesPerVoxel = 1;
  else if (m_voxelType == _UShort) m_bytesPerVoxel = 2;
  else if (m_voxelType == _Short) m_bytesPerVoxel = 2;
  else if (m_voxelType == _Int) m_bytesPerVoxel = 4;
  else if (m_voxelType == _Float) m_bytesPerVoxel = 4;

  // ---------------------
  if (m_4dvol)
    m_depth = sizes[0];
  else
    {
      m_depth = 0;
      m_depthList.clear();
      for(uint i=0; i<m_fileName.size(); i++)
	{
	  NcFile ncfile((char*)m_fileName[i].toLatin1().data(),
			NcFile::ReadOnly);
	  
	  if (!ncfile.is_valid())
	    {
	      QMessageBox::information(0, "Error",
				       QString("%1 is not a valid NetCDF file"). \
				       arg(m_fileName[i]));
	      return false;
	    }
	  NcVar *ncvar;
	  ncvar = ncfile.get_var((char *)m_varName.toLatin1().data());
	  m_depth += ncvar->get_dim(0)->size();
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
NcPlugin::findMinMaxandGenerateHistogram()
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

  m_rawMin = 10000000;
  m_rawMax = -10000000;

  NcError err(NcError::verbose_nonfatal);
  int nfls = m_fileName.size();
  if (m_4dvol) nfls = 1;
  //for(uint nf=0; nf<m_fileName.size(); nf++)
  for(uint nf=0; nf<nfls; nf++)
    {
      progress.setLabelText(m_fileName[nf]);

      NcFile dataFile((char *)m_fileName[nf].toLatin1().data(),
		      NcFile::ReadOnly);

      NcVar *ncvar;
      ncvar = dataFile.get_var((char *)m_varName.toLatin1().data());
      
      int iEnd = ncvar->get_dim(0)->size();
      for(uint i=0; i<iEnd; i++)
	{
	  progress.setValue((int)(100.0*(float)i/(float)iEnd));
	  qApp->processEvents();

	  ncvar->set_cur(i, 0, 0);
	  if (ncvar->type() == ncByte || ncvar->type() == ncChar)
	    ncvar->get((ncbyte*)tmp, 1, m_width, m_height);
	  else if (ncvar->type() == ncShort)
	    ncvar->get((short*)tmp, 1, m_width, m_height);
	  else if (ncvar->type() == ncInt)
	    ncvar->get((int*)tmp, 1, m_width, m_height);
	  else if (ncvar->type() == ncFloat)
	    ncvar->get((float*)tmp, 1, m_width, m_height);
	  else if (ncvar->type() == ncDouble)
	    ncvar->get((double*)tmp, 1, m_width, m_height);
	  
	  
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
NcPlugin::findMinMax()
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

  NcError err(NcError::verbose_nonfatal);
  int nfls = m_fileName.size();
  if (m_4dvol) nfls = 1;
  for(uint nf=0; nf<nfls; nf++)
  //for(uint nf=0; nf<m_fileName.size(); nf++)
    {
      NcFile dataFile((char *)m_fileName[nf].toLatin1().data(),
		      NcFile::ReadOnly);

      NcVar *ncvar;
      ncvar = dataFile.get_var((char *)m_varName.toLatin1().data());

      int iEnd = ncvar->get_dim(0)->size();
      for(uint i=0; i<iEnd; i++)
	{
	  progress.setValue((int)(100.0*(float)i/(float)iEnd));
	  qApp->processEvents();
	  
	  ncvar->set_cur(i, 0, 0);
	  if (ncvar->type() == ncByte || ncvar->type() == ncChar)
	    ncvar->get((ncbyte*)tmp, 1, m_width, m_height);
	  else if (ncvar->type() == ncShort)
	    ncvar->get((short*)tmp, 1, m_width, m_height);
	  else if (ncvar->type() == ncInt)
	    ncvar->get((int*)tmp, 1, m_width, m_height);
	  else if (ncvar->type() == ncFloat)
	    ncvar->get((float*)tmp, 1, m_width, m_height);
	  else if (ncvar->type() == ncDouble)
	    ncvar->get((double*)tmp, 1, m_width, m_height);

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
			   "Cancel",
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

  NcError err(NcError::verbose_nonfatal);
  int histogramSize = m_histogram.size()-1;

  int nfls = m_fileName.size();
  if (m_4dvol) nfls = 1;
  for(uint nf=0; nf<nfls; nf++)
  //for(uint nf=0; nf<m_fileName.size(); nf++)
    {
      NcFile dataFile((char *)m_fileName[nf].toLatin1().data(),
		      NcFile::ReadOnly);

      NcVar *ncvar;
      ncvar = dataFile.get_var((char *)m_varName.toLatin1().data());
      
      int iEnd = ncvar->get_dim(0)->size();
      for(uint i=0; i<iEnd; i++)
	{
	  progress.setValue((int)(100.0*(float)i/(float)iEnd));
	  qApp->processEvents();

	  ncvar->set_cur(i, 0, 0);
	  if (ncvar->type() == ncByte || ncvar->type() == ncChar)
	    ncvar->get((ncbyte*)tmp, 1, m_width, m_height);
	  else if (ncvar->type() == ncShort)
	    ncvar->get((short*)tmp, 1, m_width, m_height);
	  else if (ncvar->type() == ncInt)
	    ncvar->get((int*)tmp, 1, m_width, m_height);
	  else if (ncvar->type() == ncFloat)
	    ncvar->get((float*)tmp, 1, m_width, m_height);
	  else if (ncvar->type() == ncDouble)
	    ncvar->get((double*)tmp, 1, m_width, m_height);
	  
	  
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
NcPlugin::getDepthSlice(int slc,
			     uchar* slice)
{
  NcError err(NcError::verbose_nonfatal);

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
		  NcFile::ReadOnly);
  NcVar *ncvar;
  ncvar = dataFile.get_var((char *)m_varName.toLatin1().data());
  ncvar->set_cur(slcno, 0, 0);
  if (ncvar->type() == ncByte || ncvar->type() == ncChar)
    ncvar->get((ncbyte*)slice, 1, m_width, m_height);
  else if (ncvar->type() == ncShort)
    ncvar->get((short*)slice, 1, m_width, m_height);
  else if (ncvar->type() == ncInt)
    ncvar->get((int*)slice, 1, m_width, m_height);
  else if (ncvar->type() == ncFloat)
    ncvar->get((float*)slice, 1, m_width, m_height);
  else if (ncvar->type() == ncDouble)
    ncvar->get((double*)slice, 1, m_width, m_height);
  dataFile.close();
}

void
NcPlugin::getWidthSlice(int slc,
			uchar *slice)
{
  NcError err(NcError::verbose_nonfatal);

  for(uint nf=0; nf<m_fileName.size(); nf++)
    {
      NcFile dataFile((char *)m_fileName[nf].toLatin1().data(),
		      NcFile::ReadOnly);
      NcVar *ncvar;
      ncvar = dataFile.get_var((char *)m_varName.toLatin1().data());
      ncvar->set_cur(0, slc, 0);

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

      if (ncvar->type() == ncByte || ncvar->type() == ncChar)
	ncvar->get((ncbyte*)ptmp, depth, 1, m_height);
      else if (ncvar->type() == ncShort)
	ncvar->get((short*)ptmp, depth, 1, m_height);
      else if (ncvar->type() == ncInt)
	ncvar->get((int*)ptmp, depth, 1, m_height);
      else if (ncvar->type() == ncFloat)
	ncvar->get((float*)ptmp, depth, 1, m_height);
      else if (ncvar->type() == ncDouble)
	ncvar->get((double*)ptmp, depth, 1, m_height);
      dataFile.close();
    }  
}

void
NcPlugin::getHeightSlice(int slc,
			 uchar *slice)
{
  NcError err(NcError::verbose_nonfatal);

  for(uint nf=0; nf < m_fileName.size(); nf++)
    {
      NcFile dataFile((char *)m_fileName[nf].toLatin1().data(),
		      NcFile::ReadOnly);
      NcVar *ncvar;
      ncvar = dataFile.get_var((char *)m_varName.toLatin1().data());
      ncvar->set_cur(0, 0, slc);
      
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

      if (ncvar->type() == ncByte || ncvar->type() == ncChar)
	ncvar->get((ncbyte*)ptmp, depth, m_width, 1);
      else if (ncvar->type() == ncShort)
	ncvar->get((short*)ptmp, depth, m_width, 1);
      else if (ncvar->type() == ncInt)
	ncvar->get((int*)ptmp, depth, m_width, 1);
      else if (ncvar->type() == ncFloat)
	ncvar->get((float*)ptmp, depth, m_width, 1);
      else if (ncvar->type() == ncDouble)
	ncvar->get((double*)ptmp, depth, m_width, 1);
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


  NcError err(NcError::verbose_nonfatal);
  NcFile dataFile((char *)m_fileName[nf].toLatin1().data(),
		  NcFile::ReadOnly);
  NcVar *ncvar;
  ncvar = dataFile.get_var((char *)m_varName.toLatin1().data());
  ncvar->set_cur(slcno, w, h);

  if (m_voxelType == _UChar)
    {
      unsigned char a;
      ncvar->get((ncbyte*)&a, 1, 1, 1);
      v = QVariant((uint)a);
    }
  else if (m_voxelType == _Char)
    {
      char a;
      ncvar->get((ncbyte*)&a, 1, 1, 1);
      v = QVariant((int)a);
    }
  else if (m_voxelType == _UShort)
    {
      unsigned short a;
      ncvar->get((short*)&a, 1, 1, 1);
      v = QVariant((uint)a);
    }
  else if (m_voxelType == _Short)
    {
      short a;
      ncvar->get((short*)&a, 1, 1, 1);
      v = QVariant((int)a);
    }
  else if (m_voxelType == _Int)
    {
      int a;
      ncvar->get((int*)&a, 1, 1, 1);
      v = QVariant((int)a);
    }
  else if (m_voxelType == _Float)
    {
      float a;
      ncvar->get((float*)&a, 1, 1, 1);
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


  NcError err(NcError::verbose_nonfatal);

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
      NcFile dataFile((char *)m_fileName[nf].toLatin1().data(),
		      NcFile::ReadOnly);
      NcVar *ncvar;
      ncvar = dataFile.get_var((char *)m_varName.toLatin1().data());

      uint dStart, dEnd;
      dStart = 0;
      dEnd = ncvar->get_dim(0)->size()-1;

      if (nf == nfStart) dStart = slcStart;
      if (nf == nfEnd) dEnd = slcEnd;

      for(uint i=dStart; i<=dEnd; i++)
	{
	  ncvar->set_cur(i, 0, 0);
	  if (ncvar->type() == ncByte || ncvar->type() == ncChar)
	    ncvar->get((ncbyte*)tmp, 1, m_width, m_height);
	  else if (ncvar->type() == ncShort)
	    ncvar->get((short*)tmp, 1, m_width, m_height);
	  else if (ncvar->type() == ncInt)
	    ncvar->get((int*)tmp, 1, m_width, m_height);
	  else if (ncvar->type() == ncFloat)
	    ncvar->get((float*)tmp, 1, m_width, m_height);
	  else if (ncvar->type() == ncDouble)
	    ncvar->get((double*)tmp, 1, m_width, m_height);
	  
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
