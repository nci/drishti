#include <QtGui>
#include <netcdfcpp.h>
#include "common.h"
#include "ncplugin.h"
#include "../rawfileutils.h"

#include <cmath>
#include <cstring>
#include <limits>
#include <memory>
#include <new>

namespace
{
int nc3VoxelType(NcType type)
{
  if (type == ncByte || type == ncChar) return _Char;
  if (type == ncShort) return _Short;
  if (type == ncInt) return _Int;
  if (type == ncFloat) return _Float;
  return -1;
}

bool supportedNc3Layout(NcVar *variable, int& depth, int& width, int& height,
                        int& voxelType, QString& error)
{
  if (!variable || variable->num_dims() != 3)
    {
      error = "The selected NetCDF variable is missing or is not 3D.";
      return false;
    }
  voxelType = nc3VoxelType(variable->type());
  if (voxelType < 0)
    {
      error = QString("Unsupported NetCDF variable type %1.")
                .arg(static_cast<int>(variable->type()));
      return false;
    }
  const long d = variable->get_dim(0)->size();
  const long w = variable->get_dim(1)->size();
  const long h = variable->get_dim(2)->size();
  if (d <= 0 || w <= 0 || h <= 0 ||
      d > std::numeric_limits<int>::max() ||
      w > std::numeric_limits<int>::max() ||
      h > std::numeric_limits<int>::max())
    {
      error = "The NetCDF dimensions exceed the supported integer range.";
      return false;
    }
  depth = static_cast<int>(d);
  width = static_cast<int>(w);
  height = static_cast<int>(h);
  RawFileUtils::Layout layout;
  return RawFileUtils::makeLayout(depth, width, height, voxelType, 0,
                                  layout, error);
}

bool readNc3Slice(NcVar *variable, int slice, int width, int height,
                  int voxelType, uchar *destination)
{
  if (!variable || !destination || slice < 0 ||
      !variable->set_cur(slice, 0, 0))
    return false;
  if (voxelType == _Char)
    return variable->get(reinterpret_cast<ncbyte*>(destination),
                         1, width, height);
  if (voxelType == _Short)
    return variable->get(reinterpret_cast<short*>(destination),
                         1, width, height);
  if (voxelType == _Int)
    return variable->get(reinterpret_cast<int*>(destination),
                         1, width, height);
  if (voxelType == _Float)
    return variable->get(reinterpret_cast<float*>(destination),
                         1, width, height);
  return false;
}
}

QStringList
NcPlugin::registerPlugin()
{
  QStringList regString;
  regString << "directory";
  regString << "NetCDF-3 Directory";
  regString << "files";
  regString << "NetCDF-3 Files";
  
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
  m_headerBytes = 0;
  m_bytesPerVoxel = 1;
  m_rawMin = m_rawMax = 0;
  m_histogram.clear();
  m_depthList.clear();
  m_4dvol = false;
  m_lastError.clear();
}

void
NcPlugin::clear()
{
  m_fileName.clear();
  m_varName.clear();
  m_description.clear();
  m_depth = m_width = m_height = 0;
  m_voxelType = _UChar;
  m_voxelUnit = _Micron;
  m_voxelSizeX = m_voxelSizeY = m_voxelSizeZ = 1;
  m_skipBytes = 0;
  m_headerBytes = 0;
  m_bytesPerVoxel = 1;
  m_rawMin = m_rawMax = 0;
  m_histogram.clear();
  m_depthList.clear();
  m_4dvol = false;
  m_lastError.clear();
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
QString NcPlugin::lastError() const { return m_lastError; }

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

  NcFile dataFile((char*)m_fileName[0].toUtf8().data(),
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

  NcFile dataFile((char*)m_fileName[0].toUtf8().data(),
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
  m_lastError.clear();
  if (flnm.trimmed().isEmpty())
    {
      m_lastError = "The replacement NetCDF filename is empty.";
      return;
    }

  const QString candidate = QFileInfo(flnm).absoluteFilePath();
  NcError errors(NcError::silent_nonfatal);
  NcFile file(candidate.toLocal8Bit().constData(), NcFile::ReadOnly);
  NcVar *variable = file.is_valid() ?
    file.get_var(m_varName.toLocal8Bit().constData()) : NULL;
  int depth = 0, width = 0, height = 0, voxelType = -1;
  QString error;
  if (!file.is_valid() ||
      !supportedNc3Layout(variable, depth, width, height, voxelType, error) ||
      depth != m_depth || width != m_width || height != m_height ||
      voxelType != m_voxelType)
    {
      m_lastError = error.isEmpty() ?
        QString("Replacement NetCDF volume is incompatible with the current "
                "%1 x %2 x %3, voxel-type %4 volume.")
          .arg(m_depth).arg(m_width).arg(m_height).arg(m_voxelType) : error;
      return;
    }
  file.close();
  m_fileName = QStringList() << candidate;
  m_depthList = QList<int>() << depth;
}

bool
NcPlugin::setFile(QStringList files)
{  
  m_lastError.clear();
  if (files.isEmpty() || files.first().trimmed().isEmpty())
    {
      m_lastError = "No NetCDF file or directory was selected.";
      return false;
    }

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
	{
	  m_lastError = QString("No readable .nc files were found in %1.")
	                  .arg(files.first());
	  return false;
	}
      
      m_fileName.clear();
      for(uint i=0; i<ncfiles.size(); i++)
	{
	  QFileInfo fileInfo(files[0], ncfiles[i]);
	  QString ncfl = fileInfo.absoluteFilePath();
	  m_fileName << ncfl;
	}
    }
  else
    {
      m_fileName.clear();
      for (const QString& file : files)
        m_fileName << QFileInfo(file).absoluteFilePath();
    }

  if (m_4dvol && m_fileName.size() > 1)
    m_fileName = QStringList() << m_fileName.first();


  QList<QString> varNames;
  QList<QString> allVars = listAllVariables();

  
  if (allVars.size() == 0)
    return false;

  QList<QString> allAtts = listAllAttributes();

  NcError err(NcError::verbose_nonfatal);

  NcFile dataFile((char*)m_fileName[0].toUtf8().data(),
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
      ncvar = dataFile.get_var((char *)allVars[i].toUtf8().data());
      if (ncvar && ncvar->num_dims() == 3 && nc3VoxelType(ncvar->type()) >= 0)
	varNames.append(allVars[i]);
    }
  if (varNames.size() == 0)
    {
      m_lastError = "No supported scalar 3D variables were found in the NetCDF file.";
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
  ncvar = dataFile.get_var((char *)m_varName.toUtf8().data());
  int firstDepth = 0;
  QString layoutError;
  if (!supportedNc3Layout(ncvar, firstDepth, m_width, m_height,
                          m_voxelType, layoutError))
    {
      m_lastError = layoutError;
      return false;
    }

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

  m_bytesPerVoxel = 1;
  if (m_voxelType == _UChar) m_bytesPerVoxel = 1;
  else if (m_voxelType == _Char) m_bytesPerVoxel = 1;
  else if (m_voxelType == _UShort) m_bytesPerVoxel = 2;
  else if (m_voxelType == _Short) m_bytesPerVoxel = 2;
  else if (m_voxelType == _Int) m_bytesPerVoxel = 4;
  else if (m_voxelType == _Float) m_bytesPerVoxel = 4;

  if (!std::isfinite(m_voxelSizeX) || m_voxelSizeX <= 0) m_voxelSizeX = 1;
  if (!std::isfinite(m_voxelSizeY) || m_voxelSizeY <= 0) m_voxelSizeY = 1;
  if (!std::isfinite(m_voxelSizeZ) || m_voxelSizeZ <= 0) m_voxelSizeZ = 1;

  m_depth = 0;
  m_depthList.clear();
  for(int i=0; i<m_fileName.size(); i++)
    {
      NcFile ncfile(m_fileName[i].toLocal8Bit().constData(), NcFile::ReadOnly);
      NcVar *fileVariable = ncfile.is_valid() ?
        ncfile.get_var(m_varName.toLocal8Bit().constData()) : NULL;
      int fileDepth = 0, fileWidth = 0, fileHeight = 0, fileType = -1;
      QString fileError;
      if (!ncfile.is_valid() ||
          !supportedNc3Layout(fileVariable, fileDepth, fileWidth, fileHeight,
                              fileType, fileError) ||
          fileWidth != m_width || fileHeight != m_height ||
          fileType != m_voxelType ||
          fileDepth > std::numeric_limits<int>::max()-m_depth)
        {
          m_lastError = fileError.isEmpty() ?
            QString("NetCDF file %1 does not match the selected variable layout.")
              .arg(m_fileName[i]) : fileError;
          return false;
        }
      m_depth += fileDepth;
      m_depthList.append(m_depth);
      ncfile.close();
    }

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

  return m_lastError.isEmpty() && !m_histogram.isEmpty();
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
      if (m_voxelType == _Char) rMin = -128;
      rSize = 255;
      for(uint i=0; i<256; i++)
	m_histogram.append(0);
    }
  else if (m_voxelType == _UShort ||
      m_voxelType == _Short)
    {
      if (m_voxelType == _UShort) rMin = 0;
      if (m_voxelType == _Short) rMin = -32768;
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
  std::unique_ptr<uchar[]> storage(new (std::nothrow) uchar[nbytes]);
  if (!storage)
    {
      m_lastError = QString("Cannot allocate %1 bytes for a NetCDF slice.")
                      .arg(nbytes);
      m_histogram.clear();
      return;
    }
  uchar *tmp = storage.get();

  m_rawMin = std::numeric_limits<float>::max();
  m_rawMax = std::numeric_limits<float>::lowest();

  NcError err(NcError::verbose_nonfatal);
  int nfls = m_fileName.size();
  if (m_4dvol) nfls = 1;
  //for(uint nf=0; nf<m_fileName.size(); nf++)
  for(uint nf=0; nf<nfls; nf++)
    {
      progress.setLabelText(m_fileName[nf]);

      NcFile dataFile((char *)m_fileName[nf].toUtf8().data(),
		      NcFile::ReadOnly);

      if (!dataFile.is_valid())
        {
          m_lastError = QString("Cannot read NetCDF file %1.").arg(m_fileName[nf]);
          m_histogram.clear();
          return;
        }
      NcVar *ncvar = dataFile.get_var((char *)m_varName.toUtf8().data());
      
      int iEnd = ncvar->get_dim(0)->size();
      for(uint i=0; i<iEnd; i++)
	{
	  progress.setValue((int)(100.0*(float)i/(float)iEnd));
	  qApp->processEvents();

	  if (!readNc3Slice(ncvar, i, m_width, m_height, m_voxelType, tmp))
	    {
	      m_lastError = QString("Cannot decode slice %1 from %2.")
	                      .arg(i).arg(m_fileName[nf]);
	      m_histogram.clear();
	      return;
	    }
	  
	  
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
	if (std::isfinite(static_cast<double>(val)))		\
	  {						\
	    m_rawMin = qMin(m_rawMin, val);		\
	    m_rawMax = qMax(m_rawMax, val);		\
	  }						\
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
  std::unique_ptr<uchar[]> storage(new (std::nothrow) uchar[nbytes]);
  if (!storage)
    {
      m_lastError = QString("Cannot allocate %1 bytes for a NetCDF slice.")
                      .arg(nbytes);
      return;
    }
  uchar *tmp = storage.get();

  m_rawMin = std::numeric_limits<float>::max();
  m_rawMax = std::numeric_limits<float>::lowest();

  NcError err(NcError::verbose_nonfatal);
  int nfls = m_fileName.size();
  if (m_4dvol) nfls = 1;
  for(uint nf=0; nf<nfls; nf++)
  //for(uint nf=0; nf<m_fileName.size(); nf++)
    {
      NcFile dataFile((char *)m_fileName[nf].toUtf8().data(),
		      NcFile::ReadOnly);

      if (!dataFile.is_valid())
        {
          m_lastError = QString("Cannot read NetCDF file %1.").arg(m_fileName[nf]);
          return;
        }
      NcVar *ncvar = dataFile.get_var((char *)m_varName.toUtf8().data());

      int iEnd = ncvar->get_dim(0)->size();
      for(uint i=0; i<iEnd; i++)
	{
	  progress.setValue((int)(100.0*(float)i/(float)iEnd));
	  qApp->processEvents();
	  
	  if (!readNc3Slice(ncvar, i, m_width, m_height, m_voxelType, tmp))
	    {
	      m_lastError = QString("Cannot decode slice %1 from %2.")
	                      .arg(i).arg(m_fileName[nf]);
	      return;
	    }

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

  if (m_rawMin > m_rawMax)
    m_rawMin = m_rawMax = 0;
  progress.setValue(100);
  qApp->processEvents();
}

#define GENHISTOGRAM()					\
  {							\
    for(uint j=0; j<nY*nZ; j++)				\
      {							\
	int idx = RawFileUtils::scaledHistogramIndex(		\
	  static_cast<float>(ptr[j]), m_rawMin, m_rawMax, \
	  histogramSize);					\
	if (idx >= 0) m_histogram[idx]+=1;			\
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

  m_histogram.clear();
  for(uint i=0; i<65536; i++)
    m_histogram.append(0);

  int nX, nY, nZ;
  nX = m_depth;
  nY = m_width;
  nZ = m_height;

  int nbytes = nY*nZ*m_bytesPerVoxel;
  std::unique_ptr<uchar[]> storage(new (std::nothrow) uchar[nbytes]);
  if (!storage)
    {
      m_lastError = QString("Cannot allocate %1 bytes for a NetCDF slice.")
                      .arg(nbytes);
      m_histogram.clear();
      return;
    }
  uchar *tmp = storage.get();

  NcError err(NcError::verbose_nonfatal);
  int histogramSize = m_histogram.size()-1;

  int nfls = m_fileName.size();
  if (m_4dvol) nfls = 1;
  for(uint nf=0; nf<nfls; nf++)
  //for(uint nf=0; nf<m_fileName.size(); nf++)
    {
      NcFile dataFile((char *)m_fileName[nf].toUtf8().data(),
		      NcFile::ReadOnly);

      if (!dataFile.is_valid())
        {
          m_lastError = QString("Cannot read NetCDF file %1.").arg(m_fileName[nf]);
          m_histogram.clear();
          return;
        }
      NcVar *ncvar = dataFile.get_var((char *)m_varName.toUtf8().data());
      
      int iEnd = ncvar->get_dim(0)->size();
      for(uint i=0; i<iEnd; i++)
	{
	  progress.setValue((int)(100.0*(float)i/(float)iEnd));
	  qApp->processEvents();

	  if (!readNc3Slice(ncvar, i, m_width, m_height, m_voxelType, tmp))
	    {
	      m_lastError = QString("Cannot decode slice %1 from %2.")
	                      .arg(i).arg(m_fileName[nf]);
	      m_histogram.clear();
	      return;
	    }
	  
	  
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
  RawFileUtils::Layout layout;
  QString layoutError;
  if (!RawFileUtils::makeLayout(m_depth, m_width, m_height, m_voxelType, 0,
                                layout, layoutError) || !slice)
    return;
  if (slc < 0 || slc >= m_depth || m_fileName.isEmpty() ||
      m_depthList.size() != m_fileName.size())
    {
      std::memset(slice, 0, static_cast<std::size_t>(layout.sliceBytes));
      m_lastError = QString("Invalid NetCDF depth slice %1.").arg(slc);
      return;
    }

  int fileIndex = 0;
  while (fileIndex < m_depthList.size() && m_depthList[fileIndex] <= slc)
    ++fileIndex;
  if (fileIndex >= m_fileName.size())
    {
      std::memset(slice, 0, static_cast<std::size_t>(layout.sliceBytes));
      m_lastError = QString("Cannot map NetCDF depth slice %1 to a file.").arg(slc);
      return;
    }
  const int localSlice = fileIndex == 0 ? slc : slc-m_depthList[fileIndex-1];

  NcError err(NcError::silent_nonfatal);
  NcFile dataFile(m_fileName[fileIndex].toLocal8Bit().constData(),
                  NcFile::ReadOnly);
  NcVar *ncvar = dataFile.is_valid() ?
    dataFile.get_var(m_varName.toLocal8Bit().constData()) : NULL;
  if (!readNc3Slice(ncvar, localSlice, m_width, m_height, m_voxelType, slice))
    {
      std::memset(slice, 0, static_cast<std::size_t>(layout.sliceBytes));
      m_lastError = QString("Cannot decode NetCDF depth slice %1.").arg(slc);
    }
  dataFile.close();
}

void
NcPlugin::getWidthSlice(int slc,
			uchar *slice)
{
  NcError err(NcError::verbose_nonfatal);

  for(uint nf=0; nf<m_fileName.size(); nf++)
    {
      NcFile dataFile((char *)m_fileName[nf].toUtf8().data(),
		      NcFile::ReadOnly);
      NcVar *ncvar;
      ncvar = dataFile.get_var((char *)m_varName.toUtf8().data());
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
      NcFile dataFile((char *)m_fileName[nf].toUtf8().data(),
		      NcFile::ReadOnly);
      NcVar *ncvar;
      ncvar = dataFile.get_var((char *)m_varName.toUtf8().data());
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
  if (d < 0 || d >= m_depth ||
      w < 0 || w >= m_width ||
      h < 0 || h >= m_height)
    return QVariant("OutOfBounds");
  if (m_fileName.isEmpty() || m_depthList.size() != m_fileName.size())
    return QVariant("ReadError");

  int fileIndex = 0;
  while (fileIndex < m_depthList.size() && m_depthList[fileIndex] <= d)
    ++fileIndex;
  if (fileIndex >= m_fileName.size())
    return QVariant("ReadError");
  const int localSlice = fileIndex == 0 ? d : d-m_depthList[fileIndex-1];

  NcError err(NcError::silent_nonfatal);
  NcFile dataFile(m_fileName[fileIndex].toLocal8Bit().constData(),
                  NcFile::ReadOnly);
  NcVar *ncvar = dataFile.is_valid() ?
    dataFile.get_var(m_varName.toLocal8Bit().constData()) : NULL;
  if (!ncvar || !ncvar->set_cur(localSlice, w, h))
    return QVariant("ReadError");

  alignas(4) uchar bytes[4] = { 0, 0, 0, 0 };
  bool ok = false;
  if (m_voxelType == _Char)
    ok = ncvar->get(reinterpret_cast<ncbyte*>(bytes), 1, 1, 1);
  else if (m_voxelType == _Short)
    ok = ncvar->get(reinterpret_cast<short*>(bytes), 1, 1, 1);
  else if (m_voxelType == _Int)
    ok = ncvar->get(reinterpret_cast<int*>(bytes), 1, 1, 1);
  else if (m_voxelType == _Float)
    ok = ncvar->get(reinterpret_cast<float*>(bytes), 1, 1, 1);
  dataFile.close();
  if (!ok)
    return QVariant("ReadError");

  if (m_voxelType == _Char)
    {
      signed char value = 0;
      std::memcpy(&value, bytes, sizeof(value));
      return QVariant(static_cast<int>(value));
    }
  if (m_voxelType == _Short)
    {
      short value = 0;
      std::memcpy(&value, bytes, sizeof(value));
      return QVariant(static_cast<int>(value));
    }
  if (m_voxelType == _Int)
    {
      int value = 0;
      std::memcpy(&value, bytes, sizeof(value));
      return QVariant(value);
    }
  if (m_voxelType == _Float)
    {
      float value = 0;
      std::memcpy(&value, bytes, sizeof(value));
      return QVariant(static_cast<double>(value));
    }
  return QVariant("ReadError");
}
