#include "common.h"
#include "volumedata.h"

VolumeData::VolumeData()
{
  m_image = 0;
  clear();
}

VolumeData::~VolumeData()
{
  clear();
}

void
VolumeData::clear()
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

  m_pvlMapMax = 255;
  m_rawMap.clear();
  m_pvlMap.clear();

  if (m_image)
    delete [] m_image;
  m_image = 0;
}

void
VolumeData::setVoxelInfo(int vu,
			 float vx, float vy, float vz)
{
  m_voxelUnit = vu;  
  m_voxelSizeX = vx;
  m_voxelSizeY = vy;
  m_voxelSizeZ = vz;
}

void
VolumeData::voxelSize(float& vx, float& vy, float& vz)
  {
    //m_volInterface->voxelSize(vx, vy, vz);
    vx = m_voxelSizeX;
    vy = m_voxelSizeY;
    vz = m_voxelSizeZ;
  }
QString VolumeData::description() { return m_volInterface->description(); }
int VolumeData::voxelType() { return m_volInterface->voxelType(); }
int VolumeData::voxelUnit() { return m_volInterface->voxelUnit(); }
int VolumeData::headerBytes() { return m_headerBytes; }
int VolumeData::bytesPerVoxel() { return m_bytesPerVoxel; }

void
VolumeData::setMinMax(float rmin, float rmax)
{
  m_rawMin = rmin;
  m_rawMax = rmax;
  
  m_volInterface->setMinMax(m_rawMin, m_rawMax);
  m_histogram = m_volInterface->histogram();
}
float VolumeData::rawMin() { return m_rawMin; }
float VolumeData::rawMax() { return m_rawMax; }
QList<uint> VolumeData::histogram() { return m_histogram; }
QList<float> VolumeData::rawMap() { return m_rawMap; }
QList<int> VolumeData::pvlMap() { return m_pvlMap; }

void
VolumeData::setMap(QList<float> rm,
		   QList<int> pm)
{
  m_rawMap = rm;
  m_pvlMap = pm;

  m_pvlMapMax = m_pvlMap[m_pvlMap.count()-1];
}

void
VolumeData::gridSize(int& d, int& w, int& h)
{
  d = m_depth;
  w = m_width;
  h = m_height;
}

void
VolumeData::replaceFile(QString flnm)
{
//  m_fileName.clear();
//  m_fileName << flnm;
  m_volInterface->replaceFile(flnm);
}

bool
VolumeData::loadPlugin(QString pluginflnm)
{
//  QString plugindir = qApp->applicationDirPath() + QDir::separator() + "plugin";
//  QString pluginflnm = QFileDialog::getOpenFileName(0,
//						    "Load Plugin",
//						    plugindir,
//						    "dll files (*.dll)");
//  QString pluginflnm = QFileInfo(plugindir, plugindll).absoluteFilePath();
  QPluginLoader pluginLoader(pluginflnm);
  QObject *plugin = pluginLoader.instance();

  if (plugin)
    {
      m_volInterface = qobject_cast<VolInterface *>(plugin);
      if (m_volInterface)
	return true;
    }

  QMessageBox::information(0, "Error", "Cannot load plugin");

  return false;
}

bool
VolumeData::setFile(QStringList files,
		    QString voltype)
{
  clear();

  m_fileName = files;

  if (!loadPlugin(voltype))
    return false;

  m_volInterface->init();
  m_volInterface->set4DVolume(false);

  if (! m_volInterface->setFile(m_fileName))
    return false;

  m_volInterface->gridSize(m_depth, m_width, m_height);
  m_voxelType = m_volInterface->voxelType();

  m_bytesPerVoxel = 1;
  if (m_voxelType == _UChar) m_bytesPerVoxel = 1;
  else if (m_voxelType == _Char) m_bytesPerVoxel = 1;
  else if (m_voxelType == _UShort) m_bytesPerVoxel = 2;
  else if (m_voxelType == _Short) m_bytesPerVoxel = 2;
  else if (m_voxelType == _Int) m_bytesPerVoxel = 4;
  else if (m_voxelType == _Float) m_bytesPerVoxel = 4;

  float vx, vy, vz;
  m_volInterface->voxelSize(vx, vy, vz);
  m_voxelSizeX = vx;
  m_voxelSizeY = vy;
  m_voxelSizeZ = vz;
  
  m_rawMin = m_volInterface->rawMin();
  m_rawMax = m_volInterface->rawMax();
  m_histogram = m_volInterface->histogram();

  m_rawMap.append(m_rawMin);
  m_rawMap.append(m_rawMax);
  m_pvlMap.append(0);
  m_pvlMap.append(m_pvlMapMax);

  return true;
}

bool
VolumeData::setFile(QStringList files,
		    QString voltype,
		    bool vol4d,
		    bool skipRawDialog)
{
  clear();

  m_fileName = files;

  if (!loadPlugin(voltype))
    return false;

  m_volInterface->init();
  m_volInterface->set4DVolume(vol4d);

  if (skipRawDialog)
    m_volInterface->setValue("skiprawdialog", skipRawDialog);

  if (! m_volInterface->setFile(m_fileName))
    return false;

  m_volInterface->gridSize(m_depth, m_width, m_height);
  m_voxelType = m_volInterface->voxelType();

  m_bytesPerVoxel = 1;
  if (m_voxelType == _UChar) m_bytesPerVoxel = 1;
  else if (m_voxelType == _Char) m_bytesPerVoxel = 1;
  else if (m_voxelType == _UShort) m_bytesPerVoxel = 2;
  else if (m_voxelType == _Short) m_bytesPerVoxel = 2;
  else if (m_voxelType == _Int) m_bytesPerVoxel = 4;
  else if (m_voxelType == _Float) m_bytesPerVoxel = 4;

  float vx, vy, vz;
  m_volInterface->voxelSize(vx, vy, vz);
  m_voxelSizeX = vx;
  m_voxelSizeY = vy;
  m_voxelSizeZ = vz;
  

  m_rawMin = m_volInterface->rawMin();
  m_rawMax = m_volInterface->rawMax();
  m_histogram = m_volInterface->histogram();

  m_rawMap.append(m_rawMin);
  m_rawMap.append(m_rawMax);
  m_pvlMap.append(0);
  m_pvlMap.append(m_pvlMapMax);

  return true;
}

void
VolumeData::getDepthSlice(int slc,
			  uchar *slice)
{
  m_volInterface->getDepthSlice(slc, slice);
}

QImage
VolumeData::getDepthSliceImage(int slc)
{
  int nX, nY, nZ;
  nX = m_depth;
  nY = m_width;
  nZ = m_height;

  if (m_image)
    delete [] m_image;

  int nbytes;

  if (m_voxelType != _Rgb && m_voxelType != _Rgba)
    {
      nbytes = nY*nZ*m_bytesPerVoxel;
      m_image = new uchar[nY*nZ];
    }
  else
    {
      nbytes = nY*nZ*4;
      m_image = new uchar[4*nY*nZ];
    }

  uchar *tmp = new uchar[nbytes];

  m_volInterface->getDepthSlice(slc, tmp);

  if (m_voxelType == _Rgb || m_voxelType == _Rgba)
    {  
      memcpy(m_image, tmp, 4*m_height*m_width);
      QImage img = QImage(m_image,
			  m_height, m_width,
			  QImage::Format_ARGB32);

      delete [] tmp;

      return img;
    }

  int rawSize = m_rawMap.size()-1;
  for(int i=0; i<nY*nZ; i++)
    {
      int idx = 0;
      float frc = 0;
      float v;

      if (m_voxelType == _UChar)
	v = ((uchar *)tmp)[i];
      else if (m_voxelType == _Char)
	v = ((char *)tmp)[i];
      else if (m_voxelType == _UShort)
	v = ((ushort *)tmp)[i];
      else if (m_voxelType == _Short)
	v = ((short *)tmp)[i];
      else if (m_voxelType == _Int)
	v = ((int *)tmp)[i];
      else if (m_voxelType == _Float)
	v = ((float *)tmp)[i];

      if (v < m_rawMap[0])
	{
	  idx = 0;
	  frc = 0;
	}
      else if (v > m_rawMap[rawSize])
	{
	  idx = rawSize-1;
	  frc = 1;
	}
      else
	{
	  for(int m=0; m<rawSize; m++)
	    {
	      if (v >= m_rawMap[m] &&
		  v <= m_rawMap[m+1])
		{
		  idx = m;
		  frc = ((float)v-(float)m_rawMap[m])/
		    ((float)m_rawMap[m+1]-(float)m_rawMap[m]);
		}
	    }
	}

      int pv = m_pvlMap[idx] + frc*(m_pvlMap[idx+1]-m_pvlMap[idx]);
      if (m_pvlMapMax > 255)
	pv/=256;
      m_image[i] = pv;
    }
  QImage img = QImage(m_image, nZ, nY, nZ, QImage::Format_Indexed8);

  delete [] tmp;

  return img;
}

QImage
VolumeData::getWidthSliceImage(int slc)
{
  int nX, nY, nZ;
  nX = m_depth;
  nY = m_width;
  nZ = m_height;

  if (slc < 0 || slc >= nY)
    {
      QImage img = QImage(100, 100, QImage::Format_Indexed8);
      return img;
    }

  if (m_image)
    delete [] m_image;

  int nbytes;
  if (m_voxelType != _Rgb && m_voxelType != _Rgba)
    {
      nbytes = nX*nZ*m_bytesPerVoxel;
      m_image = new uchar[nX*nZ];
    }
  else
    {
      nbytes = nX*nZ*4;
      m_image = new uchar[nX*nZ*4];
    }

  uchar *tmp = new uchar[nbytes];

  m_volInterface->getWidthSlice(slc, tmp);

  if (m_voxelType == _Rgb || m_voxelType == _Rgba)
    {  
      memcpy(m_image, tmp, 4*m_depth*m_height);
      QImage img = QImage(m_image,
			  m_height, m_depth,
			  QImage::Format_ARGB32);

      delete [] tmp;

      return img;
    }

  int rawSize = m_rawMap.size()-1;
  for(int i=0; i<nX*nZ; i++)
    {
      int idx = m_rawMap.size()-1;
      float frc = 0;
      float v;

      if (m_voxelType == _UChar)
	v = ((uchar *)tmp)[i];
      else if (m_voxelType == _Char)
	v = ((char *)tmp)[i];
      else if (m_voxelType == _UShort)
	v = ((ushort *)tmp)[i];
      else if (m_voxelType == _Short)
	v = ((short *)tmp)[i];
      else if (m_voxelType == _Int)
	v = ((int *)tmp)[i];
      else if (m_voxelType == _Float)
	v = ((float *)tmp)[i];

      if (v < m_rawMap[0])
	{
	  idx = 0;
	  frc = 0;
	}
      else if (v > m_rawMap[rawSize])
	{
	  idx = rawSize-1;
	  frc = 1;
	}
      else
	{
	  for(int m=0; m<rawSize; m++)
	    {
	      if (v >= m_rawMap[m] &&
		  v <= m_rawMap[m+1])
		{
		  idx = m;
		  frc = ((float)v-(float)m_rawMap[m])/
		    ((float)m_rawMap[m+1]-(float)m_rawMap[m]);
		}
	    }
	}

      int pv = m_pvlMap[idx] + frc*(m_pvlMap[idx+1]-m_pvlMap[idx]);
      if (m_pvlMapMax > 255)
	pv/=256;
      m_image[i] = pv;
    }
  QImage img = QImage(m_image, nZ, nX, nZ, QImage::Format_Indexed8);

  delete [] tmp;

  return img;
}

QImage
VolumeData::getHeightSliceImage(int slc)
{
  int nX, nY, nZ;
  nX = m_depth;
  nY = m_width;
  nZ = m_height;

  if (slc < 0 || slc >= nZ)
    {
      QImage img = QImage(100, 100, QImage::Format_Indexed8);
      return img;
    }

  if (m_image)
    delete [] m_image;

  int nbytes;
  if (m_voxelType != _Rgb && m_voxelType != _Rgba)
    {
      nbytes = nX*nY*m_bytesPerVoxel;
      m_image = new uchar[nX*nY];
    }
  else
    {
      nbytes = nX*nY*4;
      m_image = new uchar[4*nX*nY];
    }

  uchar *tmp = new uchar[nbytes];

  m_volInterface->getHeightSlice(slc, tmp);

  if (m_voxelType == _Rgb || m_voxelType == _Rgba)
    {  
      memcpy(m_image, tmp, 4*nX*nY);
      QImage img = QImage(m_image,
			  nY, nX,
			  QImage::Format_ARGB32);

      delete [] tmp;

      return img;
    }

  int rawSize = m_rawMap.size()-1;
  for(int i=0; i<nX*nY; i++)
    {
      int idx = m_rawMap.size()-1;
      float frc = 0;
      float v;

      if (m_voxelType == _UChar)
	v = ((uchar *)tmp)[i];
      else if (m_voxelType == _Char)
	v = ((char *)tmp)[i];
      else if (m_voxelType == _UShort)
	v = ((ushort *)tmp)[i];
      else if (m_voxelType == _Short)
	v = ((short *)tmp)[i];
      else if (m_voxelType == _Int)
	v = ((int *)tmp)[i];
      else if (m_voxelType == _Float)
	v = ((float *)tmp)[i];

      if (v < m_rawMap[0])
	{
	  idx = 0;
	  frc = 0;
	}
      else if (v > m_rawMap[rawSize])
	{
	  idx = rawSize-1;
	  frc = 1;
	}
      else
	{
	  for(int m=0; m<rawSize; m++)
	    {
	      if (v >= m_rawMap[m] &&
		  v <= m_rawMap[m+1])
		{
		  idx = m;
		  frc = ((float)v-(float)m_rawMap[m])/
		    ((float)m_rawMap[m+1]-(float)m_rawMap[m]);
		}
	    }
	}

      int pv = m_pvlMap[idx] + frc*(m_pvlMap[idx+1]-m_pvlMap[idx]);
      if (m_pvlMapMax > 255)
	pv/=256;
      m_image[i] = pv;
    }
  QImage img = QImage(m_image, nY, nX, nY, QImage::Format_Indexed8);

  delete [] tmp;

  return img;
}

QPair<QVariant, QVariant>
VolumeData::rawValue(int d, int w, int h)
{
  QPair<QVariant, QVariant> pair;

  if (d < 0 || d >= m_depth ||
      w < 0 || w >= m_width ||
      h < 0 || h >= m_height)
    {
      pair.first = QVariant("OutOfBounds");
      pair.second = QVariant("OutOfBounds");
      return pair;
    }

  QVariant v = m_volInterface->rawValue(d, w, h);

  if (v.type() == QVariant::String)
    {
      pair.first = v;

      QString str = v.toString();
      if (str == "OutOfBounds")
	pair.second = QVariant("OutOfBounds");
      else
	pair.second = QVariant("rgba");
      return pair;
    }

  int rawSize = m_rawMap.size()-1;
  int idx = rawSize;
  float frc = 0;
  float val;

  if (v.type() == QVariant::UInt)
    val = v.toUInt();
  else if (v.type() == QVariant::Int)
    val = v.toInt();
  else if (v.type() == QVariant::Double)
    val = v.toDouble();

  if (val <= m_rawMap[0])
    {
      idx = 0;
      frc = 0;
    }
  else if (val >= m_rawMap[rawSize])
    {
      idx = rawSize-1;
      frc = 1;
    }
  else
    {
      for(int m=0; m<rawSize; m++)
	{
	  if (val >= m_rawMap[m] &&
	      val <= m_rawMap[m+1])
	    {
	      idx = m;
	      frc = ((float)val-(float)m_rawMap[m])/
		((float)m_rawMap[m+1]-(float)m_rawMap[m]);
	    }
	}
    }
  
  int pv = m_pvlMap[idx] + frc*(m_pvlMap[idx+1]-m_pvlMap[idx]);

  pair.first = v;
  pair.second = QVariant((uint)pv);
  return pair;
}

void
VolumeData::saveTrimmed(QString trimFile,
		       int dmin, int dmax,
		       int wmin, int wmax,
		       int hmin, int hmax)
{
  m_volInterface->saveTrimmed(trimFile,
			      dmin, dmax,
			      wmin, wmax,
			      hmin, hmax);
}
