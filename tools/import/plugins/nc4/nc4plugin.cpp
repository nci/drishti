#include <QtGui>
#include <ncFile.h>
#include <ncDim.h>
#include <ncException.h>
#include <netcdf>
#include "common.h"
#include <map>
#include "nc4plugin.h"
#include "../rawfileutils.h"

#include <cmath>
#include <cstring>
#include <limits>
#include <memory>
#include <new>

using namespace std;
using namespace netCDF;
using namespace netCDF::exceptions;

namespace
{
int nc4VoxelType(const NcType& type)
{
  switch (type.getTypeClass())
    {
    case NC_UBYTE : return _UChar;
    case NC_BYTE : return _Char;
    case NC_CHAR : return _Char;
    case NC_USHORT : return _UShort;
    case NC_SHORT : return _Short;
    case NC_INT : return _Int;
    case NC_FLOAT : return _Float;
    default : return -1;
    }
}

bool supportedNc4Layout(const NcVar& variable, int& depth, int& width,
                        int& height, int& voxelType, QString& error)
{
  if (variable.isNull() || variable.getDimCount() != 3)
    {
      error = "The selected NetCDF variable is missing or is not 3D.";
      return false;
    }
  voxelType = nc4VoxelType(variable.getType());
  if (voxelType < 0)
    {
      error = QString("Unsupported NetCDF variable type %1.")
                .arg(QString::fromStdString(variable.getType().getName()));
      return false;
    }

  const std::vector<NcDim> dimensions = variable.getDims();
  if (dimensions.size() != 3 ||
      dimensions[0].getSize() > static_cast<size_t>(std::numeric_limits<int>::max()) ||
      dimensions[1].getSize() > static_cast<size_t>(std::numeric_limits<int>::max()) ||
      dimensions[2].getSize() > static_cast<size_t>(std::numeric_limits<int>::max()))
    {
      error = "The NetCDF dimensions exceed the supported integer range.";
      return false;
    }
  depth = static_cast<int>(dimensions[0].getSize());
  width = static_cast<int>(dimensions[1].getSize());
  height = static_cast<int>(dimensions[2].getSize());

  RawFileUtils::Layout layout;
  return RawFileUtils::makeLayout(depth, width, height, voxelType, 0,
                                  layout, error);
}
}

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
  if (m_fileName.isEmpty())
    {
      m_lastError = "No NetCDF file is available for variable inspection.";
      return varNames;
    }
  try
    {
      NcFile dataFile(m_fileName[0].toStdString(), NcFile::read);
      const multimap<string, NcVar> groupMap = dataFile.getVars();
      for (const auto &entry : groupMap)
        varNames.append(QString::fromStdString(entry.first));
      dataFile.close();
    }
  catch(const std::exception& exception)
    {
      m_lastError = QString("Cannot inspect NetCDF variables in %1: %2")
                      .arg(m_fileName[0], exception.what());
    }
  return varNames;
}

QList<QString>
NcPlugin::listAllAttributes()
{
  QList<QString> attNames;
  if (m_fileName.isEmpty())
    {
      m_lastError = "No NetCDF file is available for attribute inspection.";
      return attNames;
    }
  try
    {
      NcFile dataFile(m_fileName[0].toStdString(), NcFile::read);
      const multimap<string, NcGroupAtt> groupMap = dataFile.getAtts();
      for (const auto &entry : groupMap)
        attNames.append(QString::fromStdString(entry.first));
      dataFile.close();
    }
  catch(const std::exception& exception)
    {
      m_lastError = QString("Cannot inspect NetCDF attributes in %1: %2")
                      .arg(m_fileName[0], exception.what());
    }
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

  try
    {
      const QString candidate = QFileInfo(flnm).absoluteFilePath();
      NcFile file(candidate.toStdString(), NcFile::read);
      NcVar variable = file.getVar(m_varName.toStdString());
      int depth = 0, width = 0, height = 0, voxelType = -1;
      QString error;
      if (!supportedNc4Layout(variable, depth, width, height, voxelType, error) ||
          depth != m_depth || width != m_width || height != m_height ||
          voxelType != m_voxelType)
        {
          m_lastError = error.isEmpty() ?
            QString("Replacement NetCDF volume is incompatible with the "
                    "current %1 x %2 x %3, voxel-type %4 volume.")
              .arg(m_depth).arg(m_width).arg(m_height).arg(m_voxelType) : error;
          return;
        }
      file.close();
      m_fileName = QStringList() << candidate;
      m_depthList = QList<int>() << depth;
    }
  catch (const NcException& exception)
    {
      m_lastError = QString("Cannot open replacement NetCDF file %1: %2")
                      .arg(flnm, exception.what());
    }
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
      for(int i=0; i<ncfiles.size(); i++)
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

  m_description.clear();
  m_voxelUnit = _Micron;
  m_voxelSizeX = m_voxelSizeY = m_voxelSizeZ = 1;
  m_histogram.clear();
  m_depthList.clear();

  try
    {
  QList<QString> varNames;
  const QList<QString> allVars = listAllVariables();
  
  if (allVars.size() == 0)
    {
      if (m_lastError.isEmpty())
        m_lastError = "No variables were found in the NetCDF file.";
      return false;
    }

  const QList<QString> allAtts = listAllAttributes();
  if (!m_lastError.isEmpty())
    return false;


  NcFile dataFile;
  try
    {
      dataFile.open(m_fileName[0].toStdString(),
		    NcFile::read);
    }
  catch(NcException &e)
    {
      m_lastError = QString("%1 is not a valid NetCDF file: %2")
                      .arg(m_fileName[0], e.what());
      return false;
    }

  //---------------------------------------------------------
  // -- Choose a variable for extraction --------------------
  for(int i=0; i<allVars.size(); i++)
    {
      NcVar ncvar;
      ncvar = dataFile.getVar(allVars[i].toStdString());
      if (ncvar.getDimCount() == 3 && nc4VoxelType(ncvar.getType()) >= 0)
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
	}
    }
  //---------------------------------------------------------
  
  NcVar ncvar;
  ncvar = dataFile.getVar(m_varName.toStdString());
  int firstDepth = 0;
  QString layoutError;
  if (!supportedNc4Layout(ncvar, firstDepth, m_width, m_height,
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
      NcGroupAtt att = dataFile.getAtt("voxel_size_xyz");
      const size_t length = att.isNull() ? 0 : att.getAttLength();
      if (length >= 3 && length <= 1024)
        {
          std::vector<double> values(length);
          att.getValues(values.data());
          m_voxelSizeX = values[0];
          m_voxelSizeY = values[1];
          m_voxelSizeZ = values[2];
        }
    }
  else // check with variable, it it has this attribute
    {
      const auto variableAttributes = ncvar.getAtts();
      const auto attribute = variableAttributes.find("voxel_size");
      if (attribute != variableAttributes.end())
        {
          const NcVarAtt& att = attribute->second;
          const size_t length = att.getAttLength();
          if (length >= 3 && length <= 1024)
	    {
	      std::vector<double> values(length);
	      att.getValues(values.data());
	      m_voxelSizeX = values[0];
	      m_voxelSizeY = values[1];
	      m_voxelSizeZ = values[2];
	    }
        }
    }
  
  ati = allAtts.indexOf("voxel_unit");
  if (ati > -1)
    {
      NcGroupAtt att = dataFile.getAtt("voxel_unit");
      if (!att.isNull() &&
          (att.getType().getTypeClass() == NC_CHAR ||
           att.getType().getTypeClass() == NC_STRING))
        {
          string values;
          att.getValues(values);
          if (values == "mm")
	    m_voxelUnit = _Millimeter;
        }
    }
  // ---------------------


  dataFile.close();

  if (!std::isfinite(m_voxelSizeX) || m_voxelSizeX <= 0) m_voxelSizeX = 1;
  if (!std::isfinite(m_voxelSizeY) || m_voxelSizeY <= 0) m_voxelSizeY = 1;
  if (!std::isfinite(m_voxelSizeZ) || m_voxelSizeZ <= 0) m_voxelSizeZ = 1;

  
  m_bytesPerVoxel = 1;
  if (m_voxelType == _UChar) m_bytesPerVoxel = 1;
  else if (m_voxelType == _Char) m_bytesPerVoxel = 1;
  else if (m_voxelType == _UShort) m_bytesPerVoxel = 2;
  else if (m_voxelType == _Short) m_bytesPerVoxel = 2;
  else if (m_voxelType == _Int) m_bytesPerVoxel = 4;
  else if (m_voxelType == _Float) m_bytesPerVoxel = 4;

  m_depth = 0;
  m_depthList.clear();
  for(int i=0; i<m_fileName.size(); i++)
    {
      try
        {
          NcFile ncfile(m_fileName[i].toStdString(), NcFile::read);
          NcVar fileVariable = ncfile.getVar(m_varName.toStdString());
          int fileDepth = 0, fileWidth = 0, fileHeight = 0, fileType = -1;
          QString fileError;
          if (!supportedNc4Layout(fileVariable, fileDepth, fileWidth,
                                  fileHeight, fileType, fileError) ||
              fileWidth != m_width || fileHeight != m_height ||
              fileType != m_voxelType ||
              fileDepth > std::numeric_limits<int>::max()-m_depth)
            {
              m_lastError = fileError.isEmpty() ?
                QString("NetCDF file %1 does not match the selected "
                        "variable layout.").arg(m_fileName[i]) : fileError;
              return false;
            }
          m_depth += fileDepth;
          m_depthList.append(m_depth);
          ncfile.close();
        }
      catch (const NcException& exception)
        {
          m_lastError = QString("Cannot inspect NetCDF file %1: %2")
                          .arg(m_fileName[i], exception.what());
          return false;
        }
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

  if (m_lastError.isEmpty() && m_histogram.isEmpty())
    m_lastError = "NetCDF histogram generation produced no data.";
  return m_lastError.isEmpty();
    }
  catch (const std::exception& exception)
    {
      m_lastError = QString("Cannot import NetCDF data: %1")
                      .arg(exception.what());
      m_histogram.clear();
      return false;
    }
}


#define MINMAXANDHISTOGRAM()				\
  {							\
    for(qint64 j=0; j<voxelCount; j++)			\
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
NcPlugin::getSlice(int sliceType, int a, int b, NcVar ncvar, int slc, uchar *tmp)
{
  if (!tmp || ncvar.isNull() || sliceType < 0 || sliceType > 2 ||
      a <= 0 || b <= 0 || slc < 0)
    return false;

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
  
  try
    {
      if (ncvar.getType() == ncUbyte)
        ncvar.getVar(start, count, reinterpret_cast<unsigned char*>(tmp));
      else if (ncvar.getType() == ncByte || ncvar.getType() == ncChar)
        ncvar.getVar(start, count, reinterpret_cast<signed char*>(tmp));
      else if (ncvar.getType() == ncUshort)
        ncvar.getVar(start, count, reinterpret_cast<unsigned short*>(tmp));
      else if (ncvar.getType() == ncShort)
        ncvar.getVar(start, count, reinterpret_cast<short*>(tmp));
      else if (ncvar.getType() == ncInt)
        ncvar.getVar(start, count, reinterpret_cast<int*>(tmp));
      else if (ncvar.getType() == ncFloat)
        ncvar.getVar(start, count, reinterpret_cast<float*>(tmp));
      else
        {
          m_lastError = QString("Unsupported NetCDF slice type %1.")
                          .arg(QString::fromStdString(
                            ncvar.getType().getName()));
          return false;
        }
    }
  catch (const NcException& exception)
    {
      m_lastError = QString("Cannot decode NetCDF slice %1: %2")
                      .arg(slc).arg(exception.what());
      return false;
    }
  return true;
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


  RawFileUtils::Layout layout;
  QString layoutError;
  if (!RawFileUtils::makeLayout(1, m_width, m_height, m_voxelType, 0,
                                layout, layoutError))
    {
      m_lastError = layoutError;
      m_histogram.clear();
      return;
    }
  const qint64 voxelCount = layout.sliceVoxels;
  const int nbytes = static_cast<int>(layout.sliceBytes);
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

  int nfls = m_fileName.size();
  if (m_4dvol) nfls = 1;
  for(int nf=0; nf<nfls; nf++)
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
	  m_lastError = QString("Cannot read NetCDF file %1: %2")
	                  .arg(m_fileName[nf], e.what());
	  m_histogram.clear();
	  return;
	}
      
      NcVar ncvar;
      ncvar = dataFile.getVar(m_varName.toStdString());
      
      const int iEnd = static_cast<int>(ncvar.getDim(0).getSize());
      for(int i=0; i<iEnd; i++)
	{
	  progress.setValue((int)(100.0*(float)i/(float)iEnd));
	  qApp->processEvents();

	  if (!getSlice(0, m_width, m_height, ncvar, i, tmp))
	    {
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

  progress.setValue(100);
  qApp->processEvents();
}


#define FINDMINMAX()					\
  {							\
    for(qint64 j=0; j<voxelCount; j++)			\
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

  RawFileUtils::Layout layout;
  QString layoutError;
  if (!RawFileUtils::makeLayout(1, m_width, m_height, m_voxelType, 0,
                                layout, layoutError))
    {
      m_lastError = layoutError;
      return;
    }
  const qint64 voxelCount = layout.sliceVoxels;
  const int nbytes = static_cast<int>(layout.sliceBytes);
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

  int nfls = m_fileName.size();
  if (m_4dvol) nfls = 1;
  for(int nf=0; nf<nfls; nf++)
    {
      NcFile dataFile;
      try
	{
	  dataFile.open(m_fileName[nf].toStdString(),
			NcFile::read);
	}
      catch(NcException &e)
	{
	  m_lastError = QString("Cannot read NetCDF file %1: %2")
	                  .arg(m_fileName[nf], e.what());
	  return;
	}
      
      NcVar ncvar;
      ncvar = dataFile.getVar(m_varName.toStdString());

      const int iEnd = static_cast<int>(ncvar.getDim(0).getSize());
      for(int i=0; i<iEnd; i++)
	{
	  progress.setValue((int)(100.0*(float)i/(float)iEnd));
	  qApp->processEvents();
	  
	  if (!getSlice(0, m_width, m_height, ncvar, i, tmp))
	    return;
	  
	  
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
    for(qint64 j=0; j<voxelCount; j++)			\
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

  RawFileUtils::Layout layout;
  QString layoutError;
  if (!RawFileUtils::makeLayout(1, m_width, m_height, m_voxelType, 0,
                                layout, layoutError))
    {
      m_lastError = layoutError;
      m_histogram.clear();
      return;
    }
  const qint64 voxelCount = layout.sliceVoxels;
  const int nbytes = static_cast<int>(layout.sliceBytes);
  std::unique_ptr<uchar[]> storage(new (std::nothrow) uchar[nbytes]);
  if (!storage)
    {
      m_lastError = QString("Cannot allocate %1 bytes for a NetCDF slice.")
                      .arg(nbytes);
      m_histogram.clear();
      return;
    }
  uchar *tmp = storage.get();

  int histogramSize = m_histogram.size()-1;

  int nfls = m_fileName.size();
  if (m_4dvol) nfls = 1;
  for(int nf=0; nf<nfls; nf++)
    {

      NcFile dataFile;
      try
	{
	  dataFile.open(m_fileName[nf].toStdString(),
			NcFile::read);
	}
      catch(NcException &e)
	{
	  m_lastError = QString("Cannot read NetCDF file %1: %2")
	                  .arg(m_fileName[nf], e.what());
	  m_histogram.clear();
	  return;
	}
      
      NcVar ncvar;
      ncvar = dataFile.getVar(m_varName.toStdString());
      
      const int iEnd = static_cast<int>(ncvar.getDim(0).getSize());
      for(int i=0; i<iEnd; i++)
	{
	  progress.setValue((int)(100.0*(float)i/(float)iEnd));
	  qApp->processEvents();
	  
	  if (!getSlice(0, m_width, m_height, ncvar, i, tmp))
	    {
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

  progress.setValue(100);
  qApp->processEvents();
}


void
NcPlugin::getDepthSlice(int slc,
			     uchar* slice)
{
  m_lastError.clear();
  if (!slice)
    {
      m_lastError = "NetCDF depth-slice output buffer is null.";
      return;
    }

  RawFileUtils::Layout layout;
  QString layoutError;
  if (!RawFileUtils::makeLayout(m_depth, m_width, m_height, m_voxelType, 0,
                                layout, layoutError))
    {
      m_lastError = layoutError;
      return;
    }
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

  try
    {
      NcFile dataFile(m_fileName[fileIndex].toStdString(), NcFile::read);
      NcVar ncvar = dataFile.getVar(m_varName.toStdString());
      if (!getSlice(0, m_width, m_height, ncvar, localSlice, slice))
        {
          std::memset(slice, 0, static_cast<std::size_t>(layout.sliceBytes));
          if (m_lastError.isEmpty())
            m_lastError = QString("Cannot decode NetCDF depth slice %1.")
                            .arg(slc);
        }
      dataFile.close();
    }
  catch (const NcException& exception)
    {
      std::memset(slice, 0, static_cast<std::size_t>(layout.sliceBytes));
      m_lastError = QString("Cannot decode NetCDF slice %1: %2")
                      .arg(slc).arg(exception.what());
    }
}

//void
//NcPlugin::getWidthSlice(int slc,
//			uchar *slice)
//{
//  for(uint nf=0; nf<m_fileName.size(); nf++)
//    {
//      NcFile dataFile((char *)m_fileName[nf].toUtf8().data(),
//		      NcFile::read);
//
//      int depth;
//      uchar *ptmp;
//      if (nf > 0)
//	{
//	  depth = m_depthList[nf]-m_depthList[nf-1];
//	  ptmp = slice + m_depthList[nf-1]*m_height*m_bytesPerVoxel;
//	}
//      else
//	{
//	  depth = m_depthList[0];
//	  ptmp = slice;
//	}
//
//      
//      NcVar ncvar;
//      ncvar = dataFile.getVar(m_varName.toStdString());
//      
//      getSlice(1, depth, m_height, ncvar, slc, ptmp);
//
//      dataFile.close();
//    }  
//}
//
//void
//NcPlugin::getHeightSlice(int slc,
//			 uchar *slice)
//{
//  for(uint nf=0; nf < m_fileName.size(); nf++)
//    {
//      NcFile dataFile((char *)m_fileName[nf].toUtf8().data(),
//		      NcFile::read);
//      
//      int depth;
//      uchar *ptmp;
//      if (nf > 0)
//	{
//	  depth = m_depthList[nf]-m_depthList[nf-1];
//	  ptmp = slice + m_depthList[nf-1]*m_width*m_bytesPerVoxel;
//	}
//      else
//	{
//	  depth = m_depthList[0];
//	  ptmp = slice;
//	}
//
//      
//      NcVar ncvar;
//      ncvar = dataFile.getVar(m_varName.toStdString());
//      
//      getSlice(2, depth, m_width, ncvar, slc, ptmp);
//
//      dataFile.close();
//    }
//}

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

  try
    {
      NcFile dataFile(m_fileName[fileIndex].toStdString(), NcFile::read);
      NcVar ncvar = dataFile.getVar(m_varName.toStdString());
      std::vector<size_t> index = {
        static_cast<size_t>(localSlice), static_cast<size_t>(w),
        static_cast<size_t>(h) };
      std::vector<size_t> count(3, 1);
      alignas(4) uchar bytes[4] = { 0, 0, 0, 0 };

      if (m_voxelType == _UChar)
        ncvar.getVar(index, count, reinterpret_cast<unsigned char*>(bytes));
      else if (m_voxelType == _Char)
        ncvar.getVar(index, count, reinterpret_cast<signed char*>(bytes));
      else if (m_voxelType == _UShort)
        ncvar.getVar(index, count, reinterpret_cast<unsigned short*>(bytes));
      else if (m_voxelType == _Short)
        ncvar.getVar(index, count, reinterpret_cast<short*>(bytes));
      else if (m_voxelType == _Int)
        ncvar.getVar(index, count, reinterpret_cast<int*>(bytes));
      else if (m_voxelType == _Float)
        ncvar.getVar(index, count, reinterpret_cast<float*>(bytes));
      else
        return QVariant("ReadError");
      dataFile.close();

      if (m_voxelType == _UChar)
        return QVariant(static_cast<uint>(bytes[0]));
      if (m_voxelType == _Char)
        {
          signed char value = 0;
          std::memcpy(&value, bytes, sizeof(value));
          return QVariant(static_cast<int>(value));
        }
      if (m_voxelType == _UShort)
        {
          unsigned short value = 0;
          std::memcpy(&value, bytes, sizeof(value));
          return QVariant(static_cast<uint>(value));
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
      float value = 0;
      std::memcpy(&value, bytes, sizeof(value));
      return QVariant(static_cast<double>(value));
    }
  catch (const NcException& exception)
    {
      m_lastError = QString("Cannot read NetCDF voxel (%1, %2, %3): %4")
                      .arg(d).arg(w).arg(h).arg(exception.what());
      return QVariant("ReadError");
    }
}

//void
//NcPlugin::saveTrimmed(QString trimFile,
//		      int dmin, int dmax,
//		      int wmin, int wmax,
//		      int hmin, int hmax)
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
//
//
//  int nfStart, nfEnd;
//  int slcStart, slcEnd;
//  //------ cater for multiple netCDF files ------
//  for(uint fl=0; fl<m_fileName.size(); fl++)
//    {
//      if (m_depthList[fl] > dmin)
//	{
//	  nfStart = fl;
//	  if (fl == 0)
//	    slcStart = dmin;
//	  else
//	    slcStart = dmin-m_depthList[fl-1];
//	  break;
//	}
//    }
//  for(uint fl=0; fl<m_fileName.size(); fl++)
//    {
//      if (m_depthList[fl] > dmax)
//	{
//	  nfEnd = fl;
//	  if (fl == 0)
//	    slcEnd = dmax;
//	  else
//	    slcEnd = dmax-m_depthList[fl-1];
//	  break;
//	}
//    }
//  //----------------------------------------
//
//  uint nslc = 0;
//  for(uint nf=nfStart; nf<=nfEnd; nf++)
//    {
//      NcFile dataFile;
//      dataFile.open(m_fileName[nf].toStdString(),
//		    NcFile::read);
//      NcVar ncvar;
//      ncvar = dataFile.getVar(m_varName.toStdString());
//
//      uint dStart, dEnd;
//      dStart = 0;
//      dEnd = ncvar.getDim(0).getSize()-1;
//
//      if (nf == nfStart) dStart = slcStart;
//      if (nf == nfEnd) dEnd = slcEnd;
//
//      for(uint i=dStart; i<=dEnd; i++)
//	{
//	  getSlice(0, m_width, m_height, ncvar, i, tmp);
//	  	  
//	  for(uint j=wmin; j<=wmax; j++)
//	    {
//	      memcpy(tmp+(j-wmin)*mZ*m_bytesPerVoxel,
//		     tmp+(j*nZ + hmin)*m_bytesPerVoxel,
//		     mZ*m_bytesPerVoxel);
//	    }
//	  fout.write((char*)tmp, mY*mZ*m_bytesPerVoxel);
//	  progress.setValue((int)(100*(float)nslc/(float)mX));
//	  qApp->processEvents();
//	  nslc++;
//	}
//      dataFile.close();
//    }
//
//  fout.close();  
//
//  delete [] tmp;
//
//  m_headerBytes = 13; // to be used in applyMapping
//}
