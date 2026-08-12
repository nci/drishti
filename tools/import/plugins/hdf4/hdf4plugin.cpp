#include <QtGui>
#include <netcdfcpp.h>
#include <mfhdf.h>
#include "common.h"
#include "hdf4plugin.h"
#include "../rawfileutils.h"

#include <cmath>
#include <cstring>
#include <limits>
#include <memory>
#include <new>

namespace
{
int hdf4VoxelType(int32 type)
{
  if (type == DFNT_CHAR8 || type == DFNT_INT8) return _Char;
  if (type == DFNT_UCHAR8 || type == DFNT_UINT8) return _UChar;
  if (type == DFNT_INT16) return _Short;
  if (type == DFNT_UINT16) return _UShort;
  if (type == DFNT_INT32) return _Int;
  if (type == DFNT_FLOAT32) return _Float;
  return -1;
}
}

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
  m_headerBytes = 0;
  m_bytesPerVoxel = 1;
  m_rawMin = m_rawMax = 0;
  m_histogram.clear();
  m_4dvol = false;
  m_Index = -1;
  m_varName.clear();
  m_lastError.clear();
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
  m_headerBytes = 0;
  m_bytesPerVoxel = 1;
  m_rawMin = m_rawMax = 0;
  m_histogram.clear();
  m_4dvol = false;
  m_Index = -1;
  m_varName.clear();
  m_lastError.clear();
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
QString HDF4Plugin::lastError() const { return m_lastError; }

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

  NcFile dataFile((char*)m_fileName[0].toUtf8().data(),
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
  m_lastError.clear();
  if (flnm.trimmed().isEmpty())
    {
      m_lastError = "The replacement HDF4 filename is empty.";
      return;
    }

  QStringList candidateImages;
  const QFileInfo input(flnm);
  if (input.isDir())
    {
      const QStringList names = QDir(input.absoluteFilePath()).entryList(
        QStringList() << "*.hdf" << "*.hdf4" << "*.h4",
        QDir::NoSymLinks | QDir::NoDotAndDotDot | QDir::Readable | QDir::Files);
      for (const QString& name : names)
        candidateImages << QDir(input.absoluteFilePath()).absoluteFilePath(name);
    }
  else
    candidateImages << input.absoluteFilePath();

  if (candidateImages.size() != m_depth)
    {
      m_lastError = QString("Replacement HDF4 volume has %1 slices; expected %2.")
                      .arg(candidateImages.size()).arg(m_depth);
      return;
    }

  for (const QString& imageFile : candidateImages)
    {
      const QByteArray encoded = QFile::encodeName(imageFile);
      int32 fileId = SDstart(encoded.constData(), DFACC_READ);
      int32 datasetId = fileId < 0 ? -1 : SDselect(fileId, m_Index);
      int32 rank = 0, type = 0, attributes = 0;
      int32 dimensions[MAX_VAR_DIMS] = { 0 };
      char datasetName[1024] = { 0 };
      const int status = datasetId < 0 ? -1 :
        SDgetinfo(datasetId, datasetName, &rank, dimensions, &type, &attributes);
      if (datasetId >= 0) SDendaccess(datasetId);
      if (fileId >= 0) SDend(fileId);
      if (status < 0 || rank != 2 || dimensions[0] != m_width ||
          dimensions[1] != m_height || hdf4VoxelType(type) != m_voxelType ||
          QString::fromLocal8Bit(datasetName) != m_varName)
        {
          m_lastError = QString("HDF4 file %1 does not contain a compatible "
                                "%2 dataset.").arg(imageFile, m_varName);
          return;
        }
    }

  m_fileName = QStringList() << input.absoluteFilePath();
  m_imageList = candidateImages;
}

bool
HDF4Plugin::setImageFiles(QStringList hdffiles)
{
  m_lastError.clear();
  if (hdffiles.isEmpty())
    {
      m_lastError = "No HDF4 image files were selected.";
      return false;
    }

  QStringList candidateImages;
  const QString baseDirectory = QFileInfo(m_fileName.first()).isDir() ?
    QFileInfo(m_fileName.first()).absoluteFilePath() : QString();
  for(const QString& hdfFile : hdffiles)
    {
      const QFileInfo fileInfo(hdfFile);
      candidateImages.append(fileInfo.isAbsolute() ? fileInfo.absoluteFilePath() :
                             QDir(baseDirectory).absoluteFilePath(hdfFile));
    }

  m_imageList = candidateImages;
  m_depth = candidateImages.size();


  /* Open the file and initiate the SD interface. */
  const QByteArray firstFileName = QFile::encodeName(m_imageList[0]);
  int32 sd_id = SDstart(firstFileName.constData(), DFACC_READ);
  if (sd_id < 0) {
    m_lastError = QString("Failed to open HDF4 file %1.").arg(m_imageList[0]);
    return false;
  }
    
  /* Determine the contents of the file. */
  int32 dim_sizes[MAX_VAR_DIMS];
  int32 rank, num_type, attributes, istat;
  char name[1024] = { 0 };
  int32 n_datasets, n_file_attrs;

  istat = SDfileinfo(sd_id, &n_datasets, &n_file_attrs);
  if (istat < 0)
    {
      SDend(sd_id);
      QMessageBox::warning(0, "HDF4 Import Error",
                           "Cannot inspect the selected HDF4 file.");
      return false;
    }

  /* Access the name of every data set in the file. */
  QStringList varNames;
  for (int32 index = 0; index < n_datasets; index++)
    {
      int32 sds_id = SDselect(sd_id, index);
      if (sds_id < 0)
        continue;
      istat = SDgetinfo(sds_id, name, &rank, dim_sizes,
			&num_type, &attributes);
      SDendaccess(sds_id);

      if (istat >= 0 && rank == 2)
	varNames.append(QString::fromLocal8Bit(name));
    }
  
  QString var;
  if (varNames.size() == 0) {
    SDend(sd_id);
    m_lastError = "No rank-2 datasets were found in the HDF4 file.";
    return false;
  }
  else if (varNames.size() == 1)
    {
      var = varNames[0];
    }
  else
    {
      bool ok = false;
      var = QInputDialog::getItem(0,
				  "Select Variable to Extract",
				  "Variable Names",
				  varNames,
				  0,
				  false,
				  &ok);
      if (!ok || var.isEmpty())
        {
          SDend(sd_id);
          m_lastError = "HDF4 dataset selection was canceled.";
          return false;
        }
    }
  
  m_Index = -1;
  for (int32 index = 0; index < n_datasets; index++)
    {
      int32 sds_id = SDselect(sd_id, index);
      if (sds_id < 0)
        continue;
      istat = SDgetinfo(sds_id, name, &rank, dim_sizes,
			&num_type, &attributes);
      SDendaccess(sds_id);
      if (istat >= 0 && var == QString::fromLocal8Bit(name))
	{
	  m_Index = index;
	  break;
	}
    }

  if (m_Index < 0)
    {
      SDend(sd_id);
      m_lastError = QString("Cannot locate HDF4 dataset %1.").arg(var);
      return false;
    }

  {    
    int32 sds_id = SDselect(sd_id, m_Index);
    if (sds_id < 0)
      {
        SDend(sd_id);
        m_lastError = QString("Cannot open HDF4 dataset %1.").arg(var);
        return false;
      }
    istat = SDgetinfo(sds_id, name, &rank, dim_sizes, &num_type, &attributes);
    SDendaccess(sds_id);
    if (istat < 0 || rank != 2)
      {
        SDend(sd_id);
        m_lastError = QString("Cannot inspect HDF4 dataset %1.").arg(var);
        return false;
      }
  }

  /* Terminate access to the SD interface and close the file. */
  istat = SDend(sd_id);


  m_voxelType = hdf4VoxelType(num_type);
  if (m_voxelType < 0)
    {
      m_lastError = QString("Unsupported HDF4 datatype %1.").arg(num_type);
      return false;
    }

  m_varName = var;

  m_width = dim_sizes[0];
  m_height = dim_sizes[1];

  m_bytesPerVoxel = 1;
  if (m_voxelType == _UChar) m_bytesPerVoxel = 1;
  else if (m_voxelType == _Char) m_bytesPerVoxel = 1;
  else if (m_voxelType == _UShort) m_bytesPerVoxel = 2;
  else if (m_voxelType == _Short) m_bytesPerVoxel = 2;
  else if (m_voxelType == _Int) m_bytesPerVoxel = 4;
  else if (m_voxelType == _Float) m_bytesPerVoxel = 4;

  RawFileUtils::Layout layout;
  QString layoutError;
  if (!RawFileUtils::makeLayout(m_depth, m_width, m_height, m_voxelType, 0,
                                layout, layoutError))
    {
      m_lastError = layoutError;
      return false;
    }

  for (const QString& imageFile : m_imageList)
    {
      const QByteArray encoded = QFile::encodeName(imageFile);
      int32 fileId = SDstart(encoded.constData(), DFACC_READ);
      if (fileId < 0)
        {
          m_lastError = QString("Cannot open HDF4 file %1.").arg(imageFile);
          return false;
        }
      int32 datasetId = SDselect(fileId, m_Index);
      int32 fileRank = 0, fileType = 0, fileAttributes = 0;
      int32 fileDimensions[MAX_VAR_DIMS] = { 0 };
      char fileName[1024] = { 0 };
      const int infoStatus = datasetId < 0 ? -1 :
        SDgetinfo(datasetId, fileName, &fileRank, fileDimensions, &fileType,
                  &fileAttributes);
      if (datasetId >= 0) SDendaccess(datasetId);
      SDend(fileId);
      if (infoStatus < 0 || fileRank != 2 || fileType != num_type ||
          fileDimensions[0] != m_width || fileDimensions[1] != m_height ||
          QString::fromLocal8Bit(fileName) != m_varName)
        {
          m_lastError = QString("HDF4 file %1 does not contain a compatible "
                                "%2 dataset.").arg(imageFile, m_varName);
          return false;
        }
    }

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

  return m_lastError.isEmpty() && !m_histogram.isEmpty();
}

bool
HDF4Plugin::setFile(QStringList files)
{
  m_lastError.clear();
  if (files.isEmpty() || files.first().trimmed().isEmpty())
    {
      m_lastError = "No HDF4 file or directory was selected.";
      return false;
    }

  m_fileName.clear();
  for (const QString& file : files)
    m_fileName << QFileInfo(file).absoluteFilePath();

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
  return setImageFiles(m_fileName);
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
  uchar *tmp = new (std::nothrow) uchar[nbytes];
  if (!tmp)
    {
      m_lastError = QString("Cannot allocate %1 bytes for an HDF4 slice.")
                      .arg(nbytes);
      m_histogram.clear();
      return;
    }

  int32 start[2], edges[2];
  start[0] = start[1] = 0;
  edges[0] = m_width;
  edges[1] = m_height;

  m_rawMin = std::numeric_limits<float>::max();
  m_rawMax = std::numeric_limits<float>::lowest();

  for(uint i=0; i<m_depth; i++)
    {
      progress.setValue((int)(100.0*(float)i/(float)m_depth));
      qApp->processEvents();

      if (!readSlice(i, tmp))
        {
          m_histogram.clear();
          delete [] tmp;
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
	if (std::isfinite(static_cast<double>(val)))		\
	  {						\
	    m_rawMin = qMin(m_rawMin, val);		\
	    m_rawMax = qMax(m_rawMax, val);		\
	  }						\
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
  uchar *tmp = new (std::nothrow) uchar[nbytes];
  if (!tmp)
    {
      m_lastError = QString("Cannot allocate %1 bytes for an HDF4 slice.")
                      .arg(nbytes);
      return;
    }

  m_rawMin = std::numeric_limits<float>::max();
  m_rawMax = std::numeric_limits<float>::lowest();

  int32 start[2], edges[2];
  start[0] = start[1] = 0;
  edges[0] = m_width;
  edges[1] = m_height;

  for(uint i=0; i<m_depth; i++)
    {
      progress.setValue((int)(100.0*(float)i/(float)nX));
      qApp->processEvents();

      if (!readSlice(i, tmp))
        {
          delete [] tmp;
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

  delete [] tmp;

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
HDF4Plugin::generateHistogram()
{
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
  uchar *tmp = new (std::nothrow) uchar[nbytes];
  if (!tmp)
    {
      m_lastError = QString("Cannot allocate %1 bytes for an HDF4 slice.")
                      .arg(nbytes);
      m_histogram.clear();
      return;
    }

  m_histogram.clear();
  for(uint i=0; i<65536; i++)
    m_histogram.append(0);

  int histogramSize = m_histogram.size()-1;

  int32 start[2], edges[2];
  start[0] = start[1] = 0;
  edges[0] = m_width;
  edges[1] = m_height;

  for(uint i=0; i<nX; i++)
    {
      progress.setValue((int)(100.0*(float)i/(float)nX));
      qApp->processEvents();

      if (!readSlice(i, tmp))
        {
          m_histogram.clear();
          delete [] tmp;
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

  delete [] tmp;

//  while(m_histogram.last() == 0)
//    m_histogram.removeLast();
//  while(m_histogram.first() == 0)
//    m_histogram.removeFirst();

  progress.setValue(100);
  qApp->processEvents();
}


bool
HDF4Plugin::readSlice(int slc, uchar *slice)
{
  RawFileUtils::Layout layout;
  QString layoutError;
  if (!RawFileUtils::makeLayout(1, m_width, m_height, m_voxelType, 0,
                                layout, layoutError) || !slice ||
      slc < 0 || slc >= m_imageList.size())
    {
      m_lastError = layoutError.isEmpty() ?
        QString("Invalid HDF4 slice %1.").arg(slc) : layoutError;
      return false;
    }

  int32 start[2], edges[2];
  start[0] = start[1] = 0;
  edges[0] = m_width;
  edges[1] = m_height;

  const QByteArray encoded = QFile::encodeName(m_imageList[slc]);
  int32 sd_id = SDstart(encoded.constData(), DFACC_READ);
  if (sd_id < 0)
    {
      m_lastError = QString("Cannot open HDF4 file %1.").arg(m_imageList[slc]);
      return false;
    }
  int32 sds_id = SDselect(sd_id, m_Index);
  if (sds_id < 0)
    {
      SDend(sd_id);
      m_lastError = QString("Cannot open dataset %1 in HDF4 file %2.")
                      .arg(m_varName, m_imageList[slc]);
      return false;
    }
  int status = SDreaddata(sds_id,
			  start, NULL, edges,
			  (VOIDP)slice);
  SDendaccess(sds_id);
  SDend(sd_id);
  if (status < 0)
    {
      m_lastError = QString("Cannot decode HDF4 slice %1 from %2.")
                      .arg(slc).arg(m_imageList[slc]);
      return false;
    }
  return true;
}

void
HDF4Plugin::getDepthSlice(int slc,
			 uchar *slice)
{
  RawFileUtils::Layout layout;
  QString layoutError;
  if (!RawFileUtils::makeLayout(1, m_width, m_height, m_voxelType, 0,
                                layout, layoutError) || !slice)
    return;
  if (!readSlice(slc, slice))
    std::memset(slice, 0, static_cast<std::size_t>(layout.sliceBytes));
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
//      int32 sd_id = SDstart(m_imageList[i].toUtf8().data(),
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
//      int32 sd_id = SDstart(m_imageList[i].toUtf8().data(),
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

  alignas(4) uchar hdftmp[4] = { 0, 0, 0, 0 };
  int32 start[2], edges[2];
  start[0] = w;
  start[1] = h;
  edges[0] = 1;
  edges[1] = 1;

  const QByteArray encoded = QFile::encodeName(m_imageList[d]);
  int32 sd_id = SDstart(encoded.constData(), DFACC_READ);
  if (sd_id < 0)
    return QVariant("ReadError");
  int32 sds_id = SDselect(sd_id, m_Index);
  if (sds_id < 0)
    {
      SDend(sd_id);
      return QVariant("ReadError");
    }
  int status = SDreaddata(sds_id,
			  start, NULL, edges,
			  (VOIDP)hdftmp);
  SDendaccess(sds_id);
  SDend(sd_id);
  if (status < 0)
    return QVariant("ReadError");

  if (m_voxelType == _UChar)
    {
      uchar *aptr = hdftmp;
      uchar a = *aptr;
      v = QVariant((uint)a);
    }
  else if (m_voxelType == _Char)
    {
      char *aptr = reinterpret_cast<char*>(hdftmp);
      char a = *aptr;
      v = QVariant((int)a);
    }
  else if (m_voxelType == _UShort)
    {
      ushort *aptr = reinterpret_cast<ushort*>(hdftmp);
      ushort a = *aptr;
      v = QVariant((uint)a);
    }
  else if (m_voxelType == _Short)
    {
      short *aptr = reinterpret_cast<short*>(hdftmp);
      short a = *aptr;
      v = QVariant((int)a);
    }
  else if (m_voxelType == _Int)
    {
      int *aptr = reinterpret_cast<int*>(hdftmp);
      int a = *aptr;
      v = QVariant((int)a);
    }
  else if (m_voxelType == _Float)
    {
      float *aptr = reinterpret_cast<float*>(hdftmp);
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
//      int32 sd_id = SDstart(m_imageList[i].toUtf8().data(),
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
