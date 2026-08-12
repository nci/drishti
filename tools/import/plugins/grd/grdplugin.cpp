#include <QtGui>
#include "common.h"
#include "grdplugin.h"
#include "loadrawdialog.h"
#include "../rawfileutils.h"

#include <cmath>
#include <cstring>
#include <limits>
#include <memory>
#include <new>

QStringList
GrdPlugin::registerPlugin()
{
  QStringList regString;
  regString << "directory";
  regString << "GRD Directory";
  regString << "*.raw *";
  regString << "files";
  regString << "GRD Files";
  
  return regString;
}

void
GrdPlugin::init()
{
  m_fileName.clear();
  m_imageList.clear();

  m_description.clear();
  m_depth = m_width = m_height = 0;
  m_voxelType = _UChar;
  m_voxelUnit = _Micron;
  m_voxelSizeX = m_voxelSizeY = m_voxelSizeZ = 1;
  m_headerBytes = 0;
  m_bytesPerVoxel = 1;
  m_rawMin = m_rawMax = 0;
  m_histogram.clear();
  m_4dvol = false;
  m_lastError.clear();
}

void
GrdPlugin::clear()
{
  m_fileName.clear();
  m_imageList.clear();

  m_description.clear();
  m_depth = m_width = m_height = 0;
  m_voxelType = _UChar;
  m_voxelUnit = _Micron;
  m_voxelSizeX = m_voxelSizeY = m_voxelSizeZ = 1;
  m_headerBytes = 0;
  m_bytesPerVoxel = 1;
  m_rawMin = m_rawMax = 0;
  m_histogram.clear();
  m_4dvol = false;
  m_lastError.clear();
}

void
GrdPlugin::set4DVolume(bool flag)
{
  m_4dvol = flag;
}

void
GrdPlugin::voxelSize(float& vx, float& vy, float& vz)
  {
    vx = m_voxelSizeX;
    vy = m_voxelSizeY;
    vz = m_voxelSizeZ;
  }
QString GrdPlugin::description() { return m_description; }
int GrdPlugin::voxelType() { return m_voxelType; }
int GrdPlugin::voxelUnit() { return m_voxelUnit; }
int GrdPlugin::headerBytes() { return m_headerBytes; }

void
GrdPlugin::setMinMax(float rmin, float rmax)
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
float GrdPlugin::rawMin() { return m_rawMin; }
float GrdPlugin::rawMax() { return m_rawMax; }
QList<uint> GrdPlugin::histogram() { return m_histogram; }
QString GrdPlugin::lastError() const { return m_lastError; }

void
GrdPlugin::gridSize(int& d, int& w, int& h)
{
  d = m_depth;
  w = m_width;
  h = m_height;
}

void
GrdPlugin::replaceFile(QString flnm)
{
  m_lastError.clear();
  if (flnm.trimmed().isEmpty())
    {
      m_lastError = "The replacement GRD filename is empty.";
      return;
    }

  QStringList candidateImages;
  const QFileInfo input(flnm);
  if (input.isDir())
    {
      const QStringList names = QDir(input.absoluteFilePath()).entryList(
        QStringList() << "*", QDir::NoSymLinks | QDir::NoDotAndDotDot |
        QDir::Readable | QDir::Files);
      for (const QString& name : names)
        candidateImages << QDir(input.absoluteFilePath()).absoluteFilePath(name);
    }
  else
    candidateImages << input.absoluteFilePath();

  if (candidateImages.size() != m_depth)
    {
      m_lastError = QString("Replacement GRD volume has %1 slices; expected %2.")
                      .arg(candidateImages.size()).arg(m_depth);
      return;
    }

  RawFileUtils::Layout layout;
  QString error;
  if (!RawFileUtils::makeLayout(1, m_width, m_height, m_voxelType,
                                m_headerBytes, layout, error))
    {
      m_lastError = error;
      return;
    }
  for (const QString& image : candidateImages)
    if (!RawFileUtils::validateFileSize(image, layout.requiredFileBytes, error))
      {
        m_lastError = error;
        return;
      }

  m_fileName = QStringList() << input.absoluteFilePath();
  m_imageList = candidateImages;
}

bool
GrdPlugin::setFile(QStringList files)
{
  m_lastError.clear();
  if (files.isEmpty() || files.first().trimmed().isEmpty())
    {
      m_lastError = "No GRD file or directory was selected.";
      return false;
    }

  m_fileName = files;

  m_imageList.clear();

  QFileInfo f(m_fileName[0]);
  if (!f.exists() || !f.isReadable())
    {
      m_lastError = QString("Cannot read GRD input %1.").arg(m_fileName[0]);
      return false;
    }
  if (f.isDir())
    {
      // list all image files in the directory
      QStringList imageNameFilter;
      imageNameFilter << "*";
      QStringList imgfiles= QDir(m_fileName[0]).entryList(imageNameFilter,
							  QDir::NoSymLinks|
							  QDir::NoDotAndDotDot|
							  QDir::Readable|
							  QDir::Files);


      m_imageList.clear();
      for(uint i=0; i<imgfiles.size(); i++)
	{
	  QFileInfo fileInfo(m_fileName[0], imgfiles[i]);
	  QString imgfl = fileInfo.absoluteFilePath();
	  m_imageList.append(imgfl);
	}
    }
  else
    {
      for (const QString& file : files)
        {
          const QFileInfo image(file);
          if (file.trimmed().isEmpty() || !image.exists() ||
              !image.isFile() || !image.isReadable())
            {
              m_lastError = QString("Cannot read GRD slice %1.").arg(file);
              return false;
            }
          m_imageList << image.absoluteFilePath();
        }
    }

  if (m_imageList.isEmpty())
    {
      m_lastError = "The selected GRD directory contains no readable files.";
      return false;
    }

  // --- load various parameters from the raw file ---
  LoadRawDialog loadRawDialog(0,
			      (char *)m_imageList[0].toUtf8().data());

  loadRawDialog.exec();
  if (loadRawDialog.result() == QDialog::Rejected)
    return false;
  
  m_voxelType = loadRawDialog.voxelType();
  m_headerBytes = loadRawDialog.skipHeaderBytes();

  int nX, nY, nZ;
  loadRawDialog.gridSize(nX, nY, nZ);

  m_depth = m_imageList.size();
  m_width = nX;
  m_height = nY;


  m_bytesPerVoxel = 1;
  if (m_voxelType == _UChar) m_bytesPerVoxel = 1;
  else if (m_voxelType == _Char) m_bytesPerVoxel = 1;
  else if (m_voxelType == _UShort) m_bytesPerVoxel = 2;
  else if (m_voxelType == _Short) m_bytesPerVoxel = 2;
  else if (m_voxelType == _Int) m_bytesPerVoxel = 4;
  else if (m_voxelType == _Float) m_bytesPerVoxel = 4;

  RawFileUtils::Layout layout;
  QString layoutError;
  if (!RawFileUtils::makeLayout(1, m_width, m_height, m_voxelType,
                                m_headerBytes, layout, layoutError))
    {
      m_lastError = layoutError;
      return false;
    }
  for (const QString& image : m_imageList)
    if (!RawFileUtils::validateFileSize(image, layout.requiredFileBytes,
                                        layoutError))
      {
        m_lastError = layoutError;
        return false;
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

void
GrdPlugin::findMinMaxandGenerateHistogram()
{
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

//  //==================
//  // do not calculate histogram
//  if (m_voxelType == _UChar)
//    {
//      m_rawMin = 0;
//      m_rawMax = 255;
//      return;
//    }
//  //==================

  RawFileUtils::Layout layout;
  QString error;
  if (!RawFileUtils::makeLayout(1, m_width, m_height, m_voxelType,
                                m_headerBytes, layout, error))
    {
      m_lastError = error;
      m_histogram.clear();
      return;
    }
  const qint64 voxelCount = layout.sliceVoxels;
  std::unique_ptr<uchar[]> storage(new (std::nothrow)
                                uchar[static_cast<std::size_t>(layout.sliceBytes)]);
  if (!storage)
    {
      m_lastError = QString("Cannot allocate %1 bytes for a GRD slice.")
                      .arg(layout.sliceBytes);
      m_histogram.clear();
      return;
    }
  uchar *tmp = storage.get();

  m_rawMin = std::numeric_limits<float>::max();
  m_rawMax = std::numeric_limits<float>::lowest();

  QProgressDialog progress("Generating Histogram",
			   0,
			   0, 100,
			   0);
  progress.setMinimumDuration(0);

  for(uint i=0; i<m_depth; i++)
    {
      progress.setValue((int)(100.0*(float)i/(float)m_depth));
      qApp->processEvents();

      if (!RawFileUtils::readAt(m_imageList[i], m_headerBytes, tmp,
                                layout.sliceBytes, error))
        {
          m_lastError = error;
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

//  while(m_histogram.last() == 0)
//    m_histogram.removeLast();
//  while(m_histogram.first() == 0)
//    m_histogram.removeFirst();

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
GrdPlugin::findMinMax()
{
  QProgressDialog progress("Finding Min and Max",
			   QString(),
			   0, 100,
			   0);
  progress.setMinimumDuration(0);


  RawFileUtils::Layout layout;
  QString error;
  if (!RawFileUtils::makeLayout(1, m_width, m_height, m_voxelType,
                                m_headerBytes, layout, error))
    {
      m_lastError = error;
      return;
    }
  const qint64 voxelCount = layout.sliceVoxels;
  std::unique_ptr<uchar[]> storage(new (std::nothrow)
                                uchar[static_cast<std::size_t>(layout.sliceBytes)]);
  if (!storage)
    {
      m_lastError = QString("Cannot allocate %1 bytes for a GRD slice.")
                      .arg(layout.sliceBytes);
      return;
    }
  uchar *tmp = storage.get();

  m_rawMin = std::numeric_limits<float>::max();
  m_rawMax = std::numeric_limits<float>::lowest();
  for(uint i=0; i<m_depth; i++)
    {
      progress.setValue((int)(100.0*(float)i/(float)m_depth));
      qApp->processEvents();

      if (!RawFileUtils::readAt(m_imageList[i], m_headerBytes, tmp,
                                layout.sliceBytes, error))
        {
          m_lastError = error;
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
GrdPlugin::generateHistogram()
{
  QProgressDialog progress("Generating Histogram",
			   QString(),
			   0, 100,
			   0);
  progress.setMinimumDuration(0);


  RawFileUtils::Layout layout;
  QString error;
  if (!RawFileUtils::makeLayout(1, m_width, m_height, m_voxelType,
                                m_headerBytes, layout, error))
    {
      m_lastError = error;
      m_histogram.clear();
      return;
    }
  const qint64 voxelCount = layout.sliceVoxels;
  std::unique_ptr<uchar[]> storage(new (std::nothrow)
                                uchar[static_cast<std::size_t>(layout.sliceBytes)]);
  if (!storage)
    {
      m_lastError = QString("Cannot allocate %1 bytes for a GRD slice.")
                      .arg(layout.sliceBytes);
      m_histogram.clear();
      return;
    }
  uchar *tmp = storage.get();

  m_histogram.clear();
  if (m_voxelType == _UChar ||
      m_voxelType == _Char ||
      m_voxelType == _UShort ||
      m_voxelType == _Short)
    {
      for(uint i=0; i<65536; i++)
	m_histogram.append(0);
    }
  else
    {      
      for(uint i=0; i<65536; i++)
	m_histogram.append(0);
    }

  int histogramSize = m_histogram.size()-1;
  for(uint i=0; i<m_depth; i++)
    {
      progress.setValue((int)(100.0*(float)i/(float)m_depth));
      qApp->processEvents();

      if (!RawFileUtils::readAt(m_imageList[i], m_headerBytes, tmp,
                                layout.sliceBytes, error))
        {
          m_lastError = error;
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
GrdPlugin::getDepthSlice(int slc,
			      uchar *slice)
{
  m_lastError.clear();
  if (!slice)
    {
      m_lastError = "GRD depth-slice output buffer is null.";
      return;
    }

  RawFileUtils::Layout layout;
  QString error;
  if (!RawFileUtils::makeLayout(1, m_width, m_height, m_voxelType,
                                m_headerBytes, layout, error))
    {
      m_lastError = error;
      return;
    }
  if (slc < 0 || slc >= m_imageList.size() ||
      !RawFileUtils::readAt(m_imageList[slc], m_headerBytes, slice,
                            layout.sliceBytes, error))
    {
      std::memset(slice, 0, static_cast<std::size_t>(layout.sliceBytes));
      m_lastError = error.isEmpty() ?
        QString("Invalid GRD slice %1.").arg(slc) : error;
    }
}

//void
//GrdPlugin::getWidthSlice(int slc,
//			       uchar *slice)
//{
//  for(uint i=0; i<m_depth; i++)
//    {
//      QFile fin(m_imageList[i]);
//      fin.open(QFile::ReadOnly);
//      fin.seek(m_headerBytes +
//	       slc*m_height*m_bytesPerVoxel);
//      fin.read((char*)(slice+i*m_height*m_bytesPerVoxel),
//	       m_height*m_bytesPerVoxel);
//      fin.close();
//    }
//}
//
//void
//GrdPlugin::getHeightSlice(int slc,
//				uchar *slice)
//{
//  int ndum = m_width*m_height*m_bytesPerVoxel;
//  uchar *dum = new uchar[ndum];  
//  uint it=0;
//  for(uint i=0; i<m_depth; i++)
//    {
//      QFile fin(m_imageList[i]);
//      fin.open(QFile::ReadOnly);
//      fin.seek(m_headerBytes);
//      fin.read((char*)dum, ndum);
//      fin.close();
//
//      for(uint j=0; j<m_width; j++)
//	{
//	  memcpy(slice+it*m_bytesPerVoxel,
//		 dum+(j*m_height+slc)*m_bytesPerVoxel,
//		 m_bytesPerVoxel);
//	  it++;
//	}
//    }
//  delete [] dum;
//}

QVariant
GrdPlugin::rawValue(int d, int w, int h)
{
  if (d < 0 || d >= m_depth ||
      w < 0 || w >= m_width ||
      h < 0 || h >= m_height)
    return QVariant("OutOfBounds");

  qint64 rowOffset = 0;
  qint64 voxelIndex = 0;
  qint64 byteOffset = 0;
  if (!RawFileUtils::checkedMultiply(w, m_height, rowOffset) ||
      !RawFileUtils::checkedAdd(rowOffset, h, voxelIndex) ||
      !RawFileUtils::checkedMultiply(voxelIndex, m_bytesPerVoxel, byteOffset) ||
      !RawFileUtils::checkedAdd(byteOffset, m_headerBytes, byteOffset))
    return QVariant("ReadError");

  alignas(4) uchar bytes[4] = { 0, 0, 0, 0 };
  QString error;
  if (!RawFileUtils::readAt(m_imageList[d], byteOffset, bytes,
                            m_bytesPerVoxel, error))
    {
      m_lastError = error;
      return QVariant("ReadError");
    }

  if (m_voxelType == _UChar)
    return QVariant(static_cast<uint>(bytes[0]));
  else if (m_voxelType == _Char)
    {
      signed char value = 0;
      std::memcpy(&value, bytes, sizeof(value));
      return QVariant(static_cast<int>(value));
    }
  else if (m_voxelType == _UShort)
    {
      unsigned short value = 0;
      std::memcpy(&value, bytes, sizeof(value));
      return QVariant(static_cast<uint>(value));
    }
  else if (m_voxelType == _Short)
    {
      short value = 0;
      std::memcpy(&value, bytes, sizeof(value));
      return QVariant(static_cast<int>(value));
    }
  else if (m_voxelType == _Int)
    {
      int value = 0;
      std::memcpy(&value, bytes, sizeof(value));
      return QVariant(value);
    }
  else if (m_voxelType == _Float)
    {
      float value = 0;
      std::memcpy(&value, bytes, sizeof(value));
      return QVariant(static_cast<double>(value));
    }
  return QVariant("ReadError");
}

//void
//GrdPlugin::saveTrimmed(QString trimFile,
//			    int dmin, int dmax,
//			    int wmin, int wmax,
//			    int hmin, int hmax)
//{
//  QProgressDialog progress("Saving trimmed volume",
//			   0,
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
//  for(uint i=dmin; i<=dmax; i++)
//    {
//      //----------------------------
//      QFile fin(m_imageList[i]);
//      fin.open(QFile::ReadOnly);
//      fin.seek(m_headerBytes);
//      fin.read((char*)tmp, nbytes);
//      fin.close();
//      //----------------------------      
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
