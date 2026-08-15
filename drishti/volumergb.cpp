#include "global.h"
#include "volumergb.h"
#include "staticfunctions.h"
#include "mainwindowui.h"
#include "xmlheaderfunctions.h"
#include "../common/src/pvlmanifest.h"

#include <QEventLoop>
#include <QFile>
#include <QFileDialog>

#include <limits>
#include <memory>
#include <new>

namespace
{
  bool
  checkedSizeFactor(qint64 factor, qint64 &size)
  {
    if (factor <= 0 || size > std::numeric_limits<qint64>::max()/factor)
      return false;
    size *= factor;
    return true;
  }

  bool
  validAllocationSize(qint64 size)
  {
    return (size > 0 &&
            static_cast<quint64>(size) <=
            static_cast<quint64>(std::numeric_limits<size_t>::max()));
  }
}

Vec VolumeRGB::getSubvolumeMin() { return m_dataMin; }
Vec VolumeRGB::getSubvolumeMax() { return m_dataMax; }

Vec VolumeRGB::getSubvolumeSize() { return m_subvolumeSize; }
Vec VolumeRGB::getSubvolumeTextureSize() { return m_subvolumeTextureSize; }
int VolumeRGB::getSubvolumeSubsamplingLevel() { return m_subvolumeSubsamplingLevel; }
unsigned char* VolumeRGB::getSubvolumeTexture() { return m_subvolumeTexture; }

Vec VolumeRGB::getDragSubvolumeTextureSize() { return m_dragSubvolumeTextureSize; }
int VolumeRGB::getDragSubvolumeSubsamplingLevel() { return m_dragSubvolumeSubsamplingLevel; }
uchar* VolumeRGB::getDragSubvolumeTexture() { return m_dragSubvolumeTexture; }

QList<QString> VolumeRGB::volumeFiles() { return m_volumeFiles; }

int*
VolumeRGB::getSubvolume1dHistogram(int vn)
{
  if (vn == 0)
    return m_subvolume1dHistogramR;
  if (vn == 1)
    return m_subvolume1dHistogramG;
  if (vn == 2)
    return m_subvolume1dHistogramB;
  if (vn == 3)
    return m_subvolume1dHistogramA;

  return 0;
}
int*
VolumeRGB::getDrag1dHistogram(int vn)
{
  if (vn == 0)
    return m_drag1dHistogramR;
  if (vn == 1)
    return m_drag1dHistogramG;
  if (vn == 2)
    return m_drag1dHistogramB;
  if (vn == 3)
    return m_drag1dHistogramA;

  return 0;
}

int*
VolumeRGB::getSubvolume2dHistogram(int vn)
{
  if (vn == 0)
    return m_subvolume2dHistogramR;
  if (vn == 1)
    return m_subvolume2dHistogramG;
  if (vn == 2)
    return m_subvolume2dHistogramB;
  if (vn == 3)
    return m_subvolume2dHistogramA;

  return 0;
}
int*
VolumeRGB::getDrag2dHistogram(int vn)
{
  if (vn == 0)
    return m_drag2dHistogramR;
  if (vn == 1)
    return m_drag2dHistogramG;
  if (vn == 2)
    return m_drag2dHistogramB;
  if (vn == 3)
    return m_drag2dHistogramA;

  return 0;
}

void
VolumeRGB::setVolumeFiles(QList<QString> volfiles)
{
  m_volumeFiles.clear();
  m_volumeFiles += volfiles;
}

VolumeRGB::VolumeRGB() :
  VolumeRGBBase()
{
  m_volnum = 0;
  m_dataMin = Vec(0,0,0);
  m_dataMax = Vec(0,0,0);
  
  m_volumeFiles.clear();

  m_dragSubvolumeTexture = 0;
  m_subvolumeTexture = 0;
  m_slabLayerCapacity = 0;

  m_subvolume1dHistogram = m_subvolume2dHistogram = 0;
  m_drag1dHistogram = m_drag2dHistogram = 0;

  m_subvolume1dHistogramR = m_subvolume2dHistogramR = 0;
  m_subvolume1dHistogramG = m_subvolume2dHistogramG = 0;
  m_subvolume1dHistogramB = m_subvolume2dHistogramB = 0;
  m_subvolume1dHistogramA = m_subvolume2dHistogramA = 0;

  m_drag1dHistogramR = m_drag2dHistogramR = 0;
  m_drag1dHistogramG = m_drag2dHistogramG = 0;
  m_drag1dHistogramB = m_drag2dHistogramB = 0;
  m_drag1dHistogramA = m_drag2dHistogramA = 0;

  m_flhist1DR = m_flhist2DR = 0;
  m_flhist1DG = m_flhist2DG = 0;
  m_flhist1DB = m_flhist2DB = 0;
  m_flhist1DA = m_flhist2DA = 0;

  m_sliceTemp = 0;

  m_texColumns = 0;
  m_texRows = 0;
  m_texWidth = m_texHeight = 0;
  m_dragTexWidth = m_dragTexHeight = 0;
  m_dragTextureInfo = Vec(1,1,1);
}

bool
VolumeRGB::ensureWorkingBuffers()
{
  int **oneDimensional[] = {
    &m_subvolume1dHistogramR, &m_subvolume1dHistogramG,
    &m_subvolume1dHistogramB, &m_subvolume1dHistogramA,
    &m_drag1dHistogramR, &m_drag1dHistogramG,
    &m_drag1dHistogramB, &m_drag1dHistogramA
  };
  int **twoDimensional[] = {
    &m_subvolume2dHistogramR, &m_subvolume2dHistogramG,
    &m_subvolume2dHistogramB, &m_subvolume2dHistogramA,
    &m_drag2dHistogramR, &m_drag2dHistogramG,
    &m_drag2dHistogramB, &m_drag2dHistogramA
  };
  float **floatOneDimensional[] = {
    &m_flhist1DR, &m_flhist1DG, &m_flhist1DB, &m_flhist1DA
  };
  float **floatTwoDimensional[] = {
    &m_flhist2DR, &m_flhist2DG, &m_flhist2DB, &m_flhist2DA
  };

  for (int i=0; i<8; i++)
    {
      if (!*oneDimensional[i])
        *oneDimensional[i] = new (std::nothrow) int[256];
      if (!*twoDimensional[i])
        *twoDimensional[i] = new (std::nothrow) int[256*256];
      if (!*oneDimensional[i] || !*twoDimensional[i])
        return false;
      memset(*oneDimensional[i], 0, 256*sizeof(int));
      memset(*twoDimensional[i], 0, 256*256*sizeof(int));
    }

  for (int i=0; i<4; i++)
    {
      if (!*floatOneDimensional[i])
        *floatOneDimensional[i] = new (std::nothrow) float[256];
      if (!*floatTwoDimensional[i])
        *floatTwoDimensional[i] =
          new (std::nothrow) float[256*256];
      if (!*floatOneDimensional[i] || !*floatTwoDimensional[i])
        return false;
      memset(*floatOneDimensional[i], 0, 256*sizeof(float));
      memset(*floatTwoDimensional[i], 0, 256*256*sizeof(float));
    }

  return true;
}

VolumeRGB::~VolumeRGB()
{
  if(m_flhist1DR) delete [] m_flhist1DR;
  if(m_flhist2DR) delete [] m_flhist2DR;
  if(m_flhist1DG) delete [] m_flhist1DG;
  if(m_flhist2DG) delete [] m_flhist2DG;
  if(m_flhist1DB) delete [] m_flhist1DB;
  if(m_flhist2DB) delete [] m_flhist2DB;
  if(m_flhist1DA) delete [] m_flhist1DA;
  if(m_flhist2DA) delete [] m_flhist2DA;
  m_flhist1DR = 0;
  m_flhist2DR = 0;
  m_flhist1DG = 0;
  m_flhist2DG = 0;
  m_flhist1DB = 0;
  m_flhist2DB = 0;
  m_flhist1DA = 0;
  m_flhist2DA = 0;

  if(m_dragSubvolumeTexture) delete [] m_dragSubvolumeTexture;
  if(m_subvolumeTexture) delete [] m_subvolumeTexture;

  if(m_subvolume1dHistogramR) delete [] m_subvolume1dHistogramR;
  if(m_subvolume2dHistogramR) delete [] m_subvolume2dHistogramR;
  if(m_subvolume1dHistogramG) delete [] m_subvolume1dHistogramG;
  if(m_subvolume2dHistogramG) delete [] m_subvolume2dHistogramG;
  if(m_subvolume1dHistogramB) delete [] m_subvolume1dHistogramB;
  if(m_subvolume2dHistogramB) delete [] m_subvolume2dHistogramB;
  if(m_subvolume1dHistogramA) delete [] m_subvolume1dHistogramA;
  if(m_subvolume2dHistogramA) delete [] m_subvolume2dHistogramA;

  m_subvolumeTexture = 0;
  m_subvolume1dHistogramR = 0;
  m_subvolume2dHistogramR = 0;
  m_subvolume1dHistogramG = 0;
  m_subvolume2dHistogramG = 0;
  m_subvolume1dHistogramB = 0;
  m_subvolume2dHistogramB = 0;
  m_subvolume1dHistogramA = 0;
  m_subvolume2dHistogramA = 0;


  if(m_drag1dHistogramR) delete [] m_drag1dHistogramR;
  if(m_drag2dHistogramR) delete [] m_drag2dHistogramR;
  if(m_drag1dHistogramG) delete [] m_drag1dHistogramG;
  if(m_drag2dHistogramG) delete [] m_drag2dHistogramG;
  if(m_drag1dHistogramB) delete [] m_drag1dHistogramB;
  if(m_drag2dHistogramB) delete [] m_drag2dHistogramB;
  if(m_drag1dHistogramA) delete [] m_drag1dHistogramA;
  if(m_drag2dHistogramA) delete [] m_drag2dHistogramA;

  m_drag1dHistogramR = 0;
  m_drag2dHistogramR = 0;
  m_drag1dHistogramG = 0;
  m_drag2dHistogramG = 0;
  m_drag1dHistogramB = 0;
  m_drag2dHistogramB = 0;
  m_drag1dHistogramA = 0;
  m_drag2dHistogramA = 0;


  m_volumeFiles.clear();

  if(m_sliceTemp) delete [] m_sliceTemp;
  m_sliceTemp = 0;
}

bool
VolumeRGB::loadVolume(const char *flnm, bool redo)
{
  if (!flnm || !*flnm || !ensureWorkingBuffers())
    return false;

  m_volnum = 0;
  m_dataMin = Vec(0,0,0);
  m_dataMax = Vec(0,0,0);

  m_volumeFiles.clear();
  m_volumeFiles.append(flnm);

  VolumeInformation vInfo;
  VolumeInformation::setVolumeInformation(vInfo);

  bool ok = VolumeRGBBase::loadVolume(flnm, redo);
  if (!ok)
    return false;

  if (m_sliceTemp)
    delete [] m_sliceTemp;
  m_sliceTemp = 0;

  return true;
}

bool
VolumeRGB::setSubvolume(Vec boxMin, Vec boxMax,
			int volnum,
			bool force)
{  
  if (volnum < 0 || volnum >= m_volumeFiles.count())
    {
      QMessageBox::information(0, "SubVolume Update",
			       QString("%1 greater than %2 volumes").\
			       arg(volnum).\
			       arg(m_volumeFiles.count()));
      return false;
    }

  PvlManifest manifest;
  if (!PvlManifestParser::parse(m_volumeFiles[volnum], manifest, true) ||
      !manifest.isColor)
    {
      QMessageBox::warning(0, "SubVolume Update",
                           manifest.error.isEmpty() ?
                           QString("Invalid colour volume manifest") :
                           manifest.error);
      return false;
    }
  const int n_depth = manifest.depth;
  const int n_width = manifest.width;
  const int n_height = manifest.height;

  //----------------
  int nRGB = 3;
  if (Global::volumeType() == Global::RGBAVolume)
    nRGB = 4;
  if (manifest.channelNames.count() != nRGB)
    {
      QMessageBox::warning(0, "SubVolume Update",
                           "Colour manifest channel count does not match the active RGB mode");
      return false;
    }
  //---------------

  int slabSize = manifest.slabSize;
  VolumeFileManager candidateManagers[4];
  for (int channel = 0; channel < manifest.channelNames.count(); ++channel)
    {
      candidateManagers[channel].setBaseFilename(manifest.channelNames.at(channel));
      candidateManagers[channel].setDepth(n_depth);
      candidateManagers[channel].setWidth(n_width);
      candidateManagers[channel].setHeight(n_height);
      candidateManagers[channel].setHeaderSize(13);
      candidateManagers[channel].setSlabSize(slabSize);
      if (!candidateManagers[channel].exists())
        {
          QMessageBox::warning(0, "SubVolume Update",
                               QString("Cannot read colour channel %1").
                               arg(candidateManagers[channel].fileName()));
          return false;
        }
    }
  for(int a=0; a<nRGB; a++)
    {
      m_rgbaFileManager[a].setBaseFilename(manifest.channelNames.at(a));
      m_rgbaFileManager[a].setDepth(n_depth);
      m_rgbaFileManager[a].setWidth(n_width);
      m_rgbaFileManager[a].setHeight(n_height);
      m_rgbaFileManager[a].setHeaderSize(13);
      m_rgbaFileManager[a].setSlabSize(slabSize);
    }
  //----------------


  boxMin = StaticFunctions::clampVec(Vec(0,0,0),
				     boxMin,
				     Vec(n_height-1, n_width-1, n_depth-1));
  boxMax = StaticFunctions::clampVec(Vec(0,0,0),
				     boxMax,
				     Vec(n_height-1, n_width-1, n_depth-1));

  if (force == false)
    {
      if (m_volnum == volnum &&
	  (m_dataMin-boxMin).squaredNorm() < 0.1 &&
	  (m_dataMax-boxMax).squaredNorm() < 0.1)
	// no change in dataMin and dataMax values
	  return false;
    }

  // All manifest and channel checks above are complete before changing the
  // active time point. A failed future-frame check therefore leaves the
  // currently rendered frame and bounds untouched.
  m_volnum = volnum;
  m_dataMin = boxMin;
  m_dataMax = boxMax;

  m_subvolumeSize = m_dataMax - m_dataMin + Vec(1,1,1); 

  int availMem = Global::textureMemorySize()-25*Global::actualDragVolSize();
  m_subvolumeSubsamplingLevel = StaticFunctions::getSubsamplingLevel(availMem,
								     Global::maxArrayTextureLayers(),
								     nRGB, boxMin, boxMax);
  m_subvolumeSubsamplingLevel = qMax(1, m_subvolumeSubsamplingLevel);

  //-------------
  int lenx = m_subvolumeSize.x;
  int leny = m_subvolumeSize.y;
  int lenz = m_subvolumeSize.z;
  int lenx2 = qMax(1, lenx/m_subvolumeSubsamplingLevel);
  int leny2 = qMax(1, leny/m_subvolumeSubsamplingLevel);
  int lenz2 = qMax(1, lenz/m_subvolumeSubsamplingLevel);
  m_subvolumeTextureSize = Vec(lenx2, leny2, lenz2);
  //-------------

  deleteTextureSlab();

  return true;
}

VolumeInformation
VolumeRGB::volInfo(int vnum)
{
  VolumeInformation pvlInfo;

  if (vnum < 0 || vnum >= m_volumeFiles.count())
    {
      QMessageBox::information(0, "Volume Information",
			       QString("%1 greater than %2 volumes").\
			       arg(vnum).\
			       arg(m_volumeFiles.count()));
      VolumeInformation::setVolumeInformation(pvlInfo);
      return pvlInfo;
    }


  if (VolumeInformation::volInfo(m_volumeFiles[vnum].toUtf8().data(),
				 pvlInfo) == false)
    {
      QMessageBox::information(0, "Volume Information",
			       QString("Invalid netCDF file %1").\
			       arg(m_volumeFiles[vnum]));
    }
  else
    {
      Global::setRelativeVoxelScaling(pvlInfo.relativeVoxelScaling);
      VolumeInformation::setVolumeInformation(pvlInfo);
    }

  return pvlInfo;
}

void
VolumeRGB::getColumnsAndRows(int &ncols, int &nrows)
{
  ncols = m_texColumns;
  nrows = m_texRows;
}

void
VolumeRGB::getSliceTextureSize(int& texX, int& texY)
{
  texX = m_texWidth;
  texY = m_texHeight;
}

Vec VolumeRGB::getDragTextureInfo() { return m_dragTextureInfo; }

void
VolumeRGB::getDragTextureSize(int& dtexX, int& dtexY)
{
  dtexX = m_dragTexWidth;
  dtexY = m_dragTexHeight;
}


QList<Vec>
VolumeRGB::getSliceTextureSizeSlabs()
{
  //---------------
  int nRGB = 3;
  if (Global::volumeType() == Global::RGBAVolume)
    nRGB = 4;
  //---------------

  MainWindowUI::mainWindowUI()->menubar->parentWidget()->\
    setWindowTitle(QString("Generating slabs limits"));
  Global::progressBar()->show();

  int nrows, ncols;
  QList<Vec> slabinfo = Global::getSlabs(m_subvolumeSubsamplingLevel,
					 nRGB,
					 m_dataMin, m_dataMax,
					 nrows, ncols);

  if (slabinfo.count() > 1)
    {
      m_dragTextureInfo = slabinfo[0];
      int dlenx2 = qMax(1, int(m_subvolumeSize.x)/int(m_dragTextureInfo.z));
      int dleny2 = qMax(1, int(m_subvolumeSize.y)/int(m_dragTextureInfo.z));
      m_dragTexWidth = int(m_dragTextureInfo.x)*dlenx2;
      m_dragTexHeight = int(m_dragTextureInfo.y)*dleny2;
    }
  else
    {
      m_dragTextureInfo = Vec(ncols, nrows, m_subvolumeSubsamplingLevel);
      int dlenx2 = qMax(1, int(m_subvolumeSize.x)/m_subvolumeSubsamplingLevel);
      int dleny2 = qMax(1, int(m_subvolumeSize.y)/m_subvolumeSubsamplingLevel);
      m_dragTexWidth = ncols*dlenx2;
      m_dragTexHeight = nrows*dleny2;
    }

  Global::progressBar()->setValue(10);
  
  m_texColumns = ncols;
  m_texRows = nrows;

  int lenx = m_subvolumeSize.x;
  int leny = m_subvolumeSize.y;
  int lenx2 = qMax(1, lenx/m_subvolumeSubsamplingLevel);
  int leny2 = qMax(1, leny/m_subvolumeSubsamplingLevel);

  m_texWidth = ncols*lenx2;
  m_texHeight = nrows*leny2;

  Global::progressBar()->setValue(50);

  Global::progressBar()->setValue(100);
  Global::hideProgressBar();

  return slabinfo;
}

//----------------------------
// for array texture
//----------------------------
void
VolumeRGB::deleteTextureSlab()
{
  if (m_dragSubvolumeTexture) delete [] m_dragSubvolumeTexture;
  m_dragSubvolumeTexture = 0;

  if (m_subvolumeTexture) delete [] m_subvolumeTexture;
  m_subvolumeTexture = 0;
  m_slabLayerCapacity = 0;
}

bool
VolumeRGB::allocSlabs(int layerCapacity)
{
  deleteTextureSlab();

  const int channels =
    Global::volumeType() == Global::RGBAVolume ? 4 : 3;
  const qint64 width = static_cast<qint64>(m_subvolumeTextureSize.x);
  const qint64 height = static_cast<qint64>(m_subvolumeTextureSize.y);
  const int lenx = static_cast<int>(m_subvolumeSize.x);
  const int leny = static_cast<int>(m_subvolumeSize.y);
  const int lenz = static_cast<int>(m_subvolumeSize.z);
  const int dragLod = qMax(1, static_cast<int>(m_dragTextureInfo.z));
  const int dragWidth = qMax(1, lenx/dragLod);
  const int dragHeight = qMax(1, leny/dragLod);
  const int dragDepth = qMax(1, lenz/dragLod);

  if (layerCapacity <= 0 || width <= 0 || height <= 0 ||
      lenx <= 0 || leny <= 0 || lenz <= 0)
    return false;

  qint64 slabBytes = 1;
  if (!checkedSizeFactor(channels, slabBytes) ||
      !checkedSizeFactor(width, slabBytes) ||
      !checkedSizeFactor(height, slabBytes) ||
      !checkedSizeFactor(layerCapacity, slabBytes) ||
      !validAllocationSize(slabBytes))
    return false;

  qint64 dragBytes = 1;
  if (!checkedSizeFactor(channels, dragBytes) ||
      !checkedSizeFactor(dragWidth, dragBytes) ||
      !checkedSizeFactor(dragHeight, dragBytes) ||
      !checkedSizeFactor(dragDepth, dragBytes) ||
      !validAllocationSize(dragBytes))
    return false;

  uchar *slab =
    new (std::nothrow) uchar[static_cast<size_t>(slabBytes)];
  uchar *drag =
    new (std::nothrow) uchar[static_cast<size_t>(dragBytes)];
  if (!slab || !drag)
    {
      delete [] slab;
      delete [] drag;
      return false;
    }

  memset(slab, 0, static_cast<size_t>(slabBytes));
  memset(drag, 0, static_cast<size_t>(dragBytes));
  m_subvolumeTexture = slab;
  m_dragSubvolumeTexture = drag;
  m_slabLayerCapacity = layerCapacity;
  m_dragSubvolumeSubsamplingLevel = dragLod;
  m_dragSubvolumeTextureSize = Vec(dragWidth, dragHeight, dragDepth);
  return true;
}

uchar*
VolumeRGB::getSubvolume()
{
  //---------------
  int nRGB = 3;
  if (Global::volumeType() == Global::RGBAVolume)
    nRGB = 4;
  //---------------

  int lod = qMax(1, m_subvolumeSubsamplingLevel);

  int minx = m_dataMin.x;
  int miny = m_dataMin.y;
  int minz = m_dataMin.z;
  
  int maxz = m_dataMax.z;

  int lenx = m_subvolumeSize.x;
  int leny = m_subvolumeSize.y;
  int lenz = m_subvolumeSize.z;

  int lenx2 = m_subvolumeTextureSize.x;
  int leny2 = m_subvolumeTextureSize.y;
  int lenz2 = m_subvolumeTextureSize.z;

  if (lenx2 <= 0 || leny2 <= 0 || lenz2 <= 0 ||
      m_width <= 0 || m_height <= 0)
    return 0;

  //-------- for dragTexure ---------------
  int dtlod = qMax(1, static_cast<int>(m_dragTextureInfo.z));
  int dtlenx2 = qMax(1, lenx/dtlod);
  int dtleny2 = qMax(1, leny/dtlod);
  int dtlenz2 = qMax(1, lenz/dtlod);
  float stp = (float)dtlod/(float)lod;
  //---------------------------------------


  m_dragSubvolumeSubsamplingLevel = dtlod;
  m_dragSubvolumeTextureSize = Vec(dtlenx2, dtleny2, dtlenz2);
  
  if (m_subvolumeTexture) delete [] m_subvolumeTexture;
  if (m_dragSubvolumeTexture) delete [] m_dragSubvolumeTexture;
  m_subvolumeTexture = 0;
  m_dragSubvolumeTexture = 0;
  m_slabLayerCapacity = 0;

  qint64 subvolumeBytes = 1;
  if (!checkedSizeFactor(nRGB, subvolumeBytes) ||
      !checkedSizeFactor(lenx2, subvolumeBytes) ||
      !checkedSizeFactor(leny2, subvolumeBytes) ||
      !checkedSizeFactor(lenz2, subvolumeBytes) ||
      !validAllocationSize(subvolumeBytes))
    return 0;

  qint64 dragBytes = 1;
  if (!checkedSizeFactor(nRGB, dragBytes) ||
      !checkedSizeFactor(dtlenx2, dragBytes) ||
      !checkedSizeFactor(dtleny2, dragBytes) ||
      !checkedSizeFactor(dtlenz2, dragBytes) ||
      !validAllocationSize(dragBytes))
    return 0;

  qint64 dragSliceBytes = 1;
  if (!checkedSizeFactor(nRGB, dragSliceBytes) ||
      !checkedSizeFactor(dtlenx2, dragSliceBytes) ||
      !checkedSizeFactor(dtleny2, dragSliceBytes) ||
      !validAllocationSize(dragSliceBytes))
    return 0;

  qint64 sourceVoxelCount = 1;
  if (!checkedSizeFactor(m_width, sourceVoxelCount) ||
      !checkedSizeFactor(m_height, sourceVoxelCount))
    return 0;

  qint64 sourceBytes = sourceVoxelCount;
  if (!checkedSizeFactor(nRGB, sourceBytes) ||
      !validAllocationSize(sourceBytes))
    return 0;

  if (!m_sliceTemp)
    {
      m_sliceTemp =
        new (std::nothrow) uchar[static_cast<size_t>(sourceBytes)];
      if (!m_sliceTemp)
        return 0;
    }

  qint64 filterSliceBytes = 1;
  if (!checkedSizeFactor(nRGB, filterSliceBytes) ||
      !checkedSizeFactor(lenx2, filterSliceBytes) ||
      !checkedSizeFactor(leny2, filterSliceBytes) ||
      !validAllocationSize(filterSliceBytes))
    return 0;

  const qint64 windowCount64 = 2*static_cast<qint64>(lod)-1;
  if (windowCount64 <= 0 ||
      windowCount64 > std::numeric_limits<int>::max())
    return 0;
  const int totcount = static_cast<int>(windowCount64);

  m_subvolumeTexture =
    new (std::nothrow) uchar[static_cast<size_t>(subvolumeBytes)];
  if (!m_subvolumeTexture)
    return 0;

  m_dragSubvolumeTexture =
    new (std::nothrow) uchar[static_cast<size_t>(dragBytes)];
  if (!m_dragSubvolumeTexture)
    {
      delete [] m_subvolumeTexture;
      m_subvolumeTexture = 0;
      return 0;
    }

  const qint64 scratchBytes =
    qMax(sourceBytes, qMax(filterSliceBytes, dragSliceBytes));
  uchar *tmp = new (std::nothrow) uchar[static_cast<size_t>(scratchBytes)];
  if (!tmp)
    {
      delete [] m_subvolumeTexture;
      delete [] m_dragSubvolumeTexture;
      m_subvolumeTexture = 0;
      m_dragSubvolumeTexture = 0;
      return 0;
    }

  unsigned char **volX = 0;
  if (lod > 1)
    {
      qint64 filterPointerBytes = sizeof(unsigned char*);
      if (!checkedSizeFactor(totcount, filterPointerBytes) ||
          !validAllocationSize(filterPointerBytes))
        {
          delete [] tmp;
          delete [] m_subvolumeTexture;
          delete [] m_dragSubvolumeTexture;
          m_subvolumeTexture = 0;
          m_dragSubvolumeTexture = 0;
          return 0;
        }

      volX = new (std::nothrow) unsigned char*[totcount];
      if (volX)
        {
          for(int i=0; i<totcount; i++) volX[i] = 0;
          for(int i=0; i<totcount; i++)
            {
              volX[i] = new (std::nothrow)
                unsigned char[static_cast<size_t>(filterSliceBytes)];
              if (!volX[i]) break;
            }
        }

      bool filterAllocationFailed = !volX;
      for(int i=0; volX && i<totcount; i++)
        filterAllocationFailed = filterAllocationFailed || !volX[i];
      if (filterAllocationFailed)
        {
          if (volX)
            {
              for(int i=0; i<totcount; i++) delete [] volX[i];
              delete [] volX;
            }
          delete [] tmp;
          delete [] m_subvolumeTexture;
          delete [] m_dragSubvolumeTexture;
          m_subvolumeTexture = 0;
          m_dragSubvolumeTexture = 0;
          return 0;
        }
    }

  memset(m_subvolumeTexture, 0, static_cast<size_t>(subvolumeBytes));
  memset(m_dragSubvolumeTexture, 0, static_cast<size_t>(dragBytes));

  Global::progressBar()->show();

  int count=0;
  
  int outputSlice = 0;
  bool filterWindowFilled = false;

  // additional slice at the top and bottom
  for(int k=minz; k<=maxz; k++)
    {
      Global::progressBar()->setValue((int)(100.0*(float)(k-minz)/(float)qMax(1, lenz)));
      if ((k-minz)%100==0) qApp->processEvents();

      if (k >= 0 && k < m_depth)
	{
	  for (int a=0; a<nRGB; a++)
	    {
	      uchar *vslice = m_rgbaFileManager[a].getSlice(k);
	      if (vslice)
		memcpy(tmp, vslice, static_cast<size_t>(sourceVoxelCount));

	      for (qint64 ij=0; ij<sourceVoxelCount; ij++)
		m_sliceTemp[nRGB*ij+a] = vslice ? tmp[ij] : 0;
	    }
	}
      else
	{
	  memset(m_sliceTemp, 0, static_cast<size_t>(sourceBytes));
	}

      //------------------------
      if (lod > 1)
	{
	  qint64 ji=0;
	  for(int j=0; j<leny2; j++)
	    { 
	      int y = miny + j*lod;
	      int loy = qMax(miny+0, y-lod+1);
	      int hiy = qMin(miny+leny-1, y+lod-1);
	      for(int i=0; i<lenx2; i++) 
		{ 
		  int x = minx + i*lod; 
		  int lox = qMax(minx+0, x-lod+1); 
		  int hix = qMin(minx+lenx-1, x+lod-1);
		  float sumv[4] = {0, 0, 0, 0}; 
		  for(int jy=loy; jy<=hiy; jy++) 
		    {
		      for(int ix=lox; ix<=hix; ix++) 
			{
		      const qint64 idx = static_cast<qint64>(jy)*m_height+ix;
			  for(int a=0; a<nRGB; a++)
			    sumv[a] += m_sliceTemp[nRGB*idx+a];
			}
		    }

		  for(int a=0; a<nRGB; a++)
		    tmp[nRGB*ji+a] = sumv[a]/((hiy-loy+1)*(hix-lox+1)); 

		  ji++;
		}
	    }
	  memcpy(m_sliceTemp, tmp, static_cast<size_t>(filterSliceBytes));

	  unsigned char *vptr;
	  vptr = volX[0];
	  for (int c=0; c<totcount-1; c++)
	    volX[c] = volX[c+1];
	  volX[totcount-1] = vptr;
	  
	  memcpy(volX[totcount-1], m_sliceTemp,
		 static_cast<size_t>(filterSliceBytes));
      
	  count ++;
	  const bool completeWindow = (count == totcount);
	  const bool finalPartialWindow =
	    (k == maxz && outputSlice < lenz2);
	  if ((completeWindow || finalPartialWindow) && outputSlice < lenz2)
	    {
	      const int validWindowSize = filterWindowFilled ? totcount : count;
	      const int firstValidWindow = totcount-validWindowSize;
	      for(qint64 j=0; j<filterSliceBytes; j++)
		{
		  float sum=0;
		  for(int x=firstValidWindow; x<totcount; x++)
		    sum += volX[x][j];
		  m_sliceTemp[j] = sum/qMax(1, validWindowSize);
		}
	      
	      if (completeWindow)
		{
		  filterWindowFilled = true;
		  count = totcount/2;
		}

	      
	      // copy into array texture
	      memcpy(m_subvolumeTexture + filterSliceBytes*outputSlice,
		     m_sliceTemp,
		     static_cast<size_t>(filterSliceBytes));
	      //---

	      //---
	      qint64 ji=0;
	      for(int j=0; j<dtleny2; j++)
		{ 
		  int y = j*stp;
		  for(int i=0; i<dtlenx2; i++) 
		    { 
		      int x = i*stp; 
		      for(int a=0; a<nRGB; a++)
			tmp[nRGB*ji+a] = m_sliceTemp[nRGB*(y*lenx2+x)+a];
		      ji++;
		    }
		}
	      // copy into drag array texture
	      int dtkslc = qBound(0, (int)(outputSlice/stp), dtlenz2-1);
	      memcpy(m_dragSubvolumeTexture + dragSliceBytes*dtkslc,
		     tmp,
		     static_cast<size_t>(dragSliceBytes));
	      outputSlice++;
	      //---

	      
	    }
	} // lod > 1
      //------------------------
      else
	{
	  for(int j=0; j<leny2; j++)
	    memmove(m_sliceTemp + static_cast<qint64>(nRGB)*j*lenx2,
		    m_sliceTemp + static_cast<qint64>(nRGB)*
		      (static_cast<qint64>(j+miny)*m_height + minx),
		    nRGB*lenx2);

	  // copy into array texture
	  if (outputSlice >= lenz2)
	    continue;

	  memcpy(m_subvolumeTexture + filterSliceBytes*outputSlice,
		 m_sliceTemp,
		 static_cast<size_t>(filterSliceBytes));
	  //---

	  //---
	  qint64 ji=0;
	  for(int j=0; j<dtleny2; j++)
	    { 
	      int y = j*stp;
	      for(int i=0; i<dtlenx2; i++) 
		{ 
		  int x = i*stp; 
		  for(int a=0; a<nRGB; a++)
		    tmp[nRGB*ji+a] = m_sliceTemp[nRGB*(y*lenx2+x)+a];
		  ji++;
		}
	    }
	  // copy into drag array texture
	  int dtkslc = qBound(0, (int)(outputSlice/stp), dtlenz2-1);
	  memcpy(m_dragSubvolumeTexture + dragSliceBytes*dtkslc,
		 tmp,
		 static_cast<size_t>(dragSliceBytes));
	  outputSlice++;
	  //---

	  
	}
      //---------------------
      for(qint64 ji=0; ji<static_cast<qint64>(leny2)*lenx2; ji++)
	{
	  uchar r = m_sliceTemp[nRGB*ji];
	  uchar g = m_sliceTemp[nRGB*ji+1];
	  uchar b = m_sliceTemp[nRGB*ji+2];
	  
	  m_flhist1DR[r]++;  m_flhist2DR[g*256 + r]++;
	  m_flhist1DG[g]++;  m_flhist2DG[b*256 + g]++;
	  m_flhist1DB[b]++;  m_flhist2DB[r*256 + b]++;

	  if (nRGB == 4)
	    {
	      uchar a = m_sliceTemp[nRGB*ji+3];
	      uchar rgb = qMax(r, qMax(g, b));
	      m_flhist1DA[a]++; m_flhist2DA[rgb*256 + a]++;
	    }
	}
      //---------------------

    } // look over k
  
  delete [] tmp;

  if (lod > 1)
    {
      for(int i=0; i<totcount; i++)
	delete [] volX[i];
      delete [] volX;
    }

  Global::progressBar()->setValue(100);
  MainWindowUI::mainWindowUI()->statusBar->showMessage("Ready");

  return m_subvolumeTexture;
}

uchar*
VolumeRGB::getSlab(int startZSlice, int endZSlice, int layerCount)
{
  const int lod = qMax(1, m_subvolumeSubsamplingLevel);
  const int channels =
    Global::volumeType() == Global::RGBAVolume ? 4 : 3;
  const qint64 width = static_cast<qint64>(m_subvolumeTextureSize.x);
  const qint64 height = static_cast<qint64>(m_subvolumeTextureSize.y);
  const qint64 depth = static_cast<qint64>(m_subvolumeTextureSize.z);
  const int minx = static_cast<int>(m_dataMin.x);
  const int miny = static_cast<int>(m_dataMin.y);
  const int minz = static_cast<int>(m_dataMin.z);
  const int maxz = static_cast<int>(m_dataMax.z);
  const int lenx = static_cast<int>(m_subvolumeSize.x);
  const int leny = static_cast<int>(m_subvolumeSize.y);

  const qint64 firstSourceOffset =
    static_cast<qint64>(startZSlice)-minz;
  const qint64 firstLayer =
    firstSourceOffset/lod;
  const qint64 lastLayer = firstLayer+static_cast<qint64>(layerCount)-1;
  const qint64 lastSourceCenter =
    static_cast<qint64>(minz)+lastLayer*lod;

  if (!m_subvolumeTexture || !m_dragSubvolumeTexture ||
      layerCount <= 0 || layerCount > m_slabLayerCapacity ||
      width <= 0 || height <= 0 || depth <= 0 ||
      lenx <= 0 || leny <= 0 || m_width <= 0 || m_height <= 0 ||
      minx < 0 || miny < 0 ||
      static_cast<qint64>(minx)+lenx > m_height ||
      static_cast<qint64>(miny)+leny > m_width ||
      firstSourceOffset < 0 ||
      firstSourceOffset%lod != 0 || firstLayer < 0 ||
      lastLayer >= depth || endZSlice < lastSourceCenter ||
      endZSlice > maxz)
    return 0;

  qint64 voxelCount = 1;
  if (!checkedSizeFactor(width, voxelCount) ||
      !checkedSizeFactor(height, voxelCount) ||
      voxelCount > std::numeric_limits<int>::max())
    return 0;

  qint64 layerBytes = voxelCount;
  if (!checkedSizeFactor(channels, layerBytes) ||
      !validAllocationSize(layerBytes) ||
      layerBytes > std::numeric_limits<qint64>::max()/layerCount)
    return 0;

  quint32 *zSums = 0;
  quint64 *rowPrefix = 0;
  quint64 *rowAccum = 0;
  if (lod > 1)
    {
      qint64 zSumBytes = voxelCount;
      qint64 prefixBytes = static_cast<qint64>(lenx)+1;
      qint64 rowAccumBytes = width;
      if (!checkedSizeFactor(sizeof(quint32), zSumBytes) ||
          !checkedSizeFactor(sizeof(quint64), prefixBytes) ||
          !checkedSizeFactor(sizeof(quint64), rowAccumBytes) ||
          !validAllocationSize(zSumBytes) ||
          !validAllocationSize(prefixBytes) ||
          !validAllocationSize(rowAccumBytes))
        return 0;

      zSums = new (std::nothrow) quint32[static_cast<size_t>(voxelCount)];
      rowPrefix = new (std::nothrow) quint64[static_cast<size_t>(lenx)+1];
      rowAccum = new (std::nothrow) quint64[static_cast<size_t>(width)];
      if (!zSums || !rowPrefix || !rowAccum)
        {
          delete [] zSums;
          delete [] rowPrefix;
          delete [] rowAccum;
          return 0;
        }
    }

  MainWindowUI::mainWindowUI()->statusBar->showMessage(
    QString("Loading %1 to %2").arg(startZSlice).arg(endZSlice));
  Global::progressBar()->show();

  const int dragLod = qMax(1, m_dragSubvolumeSubsamplingLevel);
  const int dragWidth = static_cast<int>(m_dragSubvolumeTextureSize.x);
  const int dragHeight = static_cast<int>(m_dragSubvolumeTextureSize.y);
  const int dragDepth = static_cast<int>(m_dragSubvolumeTextureSize.z);
  const double dragStep = qMax(1.0, static_cast<double>(dragLod)/lod);

  for (int outputLayer=0; outputLayer<layerCount; outputLayer++)
    {
      Global::progressBar()->setValue(
        static_cast<int>(100.0*outputLayer/qMax(1, layerCount)));
      if (outputLayer%16 == 0)
        qApp->processEvents();

      uchar *target = m_subvolumeTexture+
                      static_cast<qint64>(outputLayer)*layerBytes;
      const qint64 globalLayer = firstLayer+outputLayer;
      const int sourceCenter =
        static_cast<int>(static_cast<qint64>(minz)+globalLayer*lod);

      if (lod == 1)
        {
          for (int channel=0; channel<channels; channel++)
            {
              uchar *source = m_rgbaFileManager[channel].getSlice(sourceCenter);
              if (!source)
                {
                  delete [] zSums;
                  delete [] rowPrefix;
                  delete [] rowAccum;
                  return 0;
                }

              for (qint64 y=0; y<height; y++)
                {
                  const qint64 sourceRow =
                    (static_cast<qint64>(miny)+y)*m_height+minx;
                  const qint64 targetRow = y*width;
                  for (qint64 x=0; x<width; x++)
                    target[channels*(targetRow+x)+channel] =
                      source[sourceRow+x];
                }
            }
        }
      else
        {
          const int zmin = qMax(minz, sourceCenter-lod+1);
          const int zmax = qMin(maxz, sourceCenter+lod-1);
          const qint64 zCount = static_cast<qint64>(zmax)-zmin+1;
          if (zCount <= 0 ||
              zCount > std::numeric_limits<quint32>::max()/255U)
            {
              delete [] zSums;
              delete [] rowPrefix;
              delete [] rowAccum;
              return 0;
            }

          for (int channel=0; channel<channels; channel++)
            {
              memset(zSums, 0,
                     static_cast<size_t>(voxelCount)*sizeof(quint32));

              for (int z=zmin; z<=zmax; z++)
                {
                  uchar *source = m_rgbaFileManager[channel].getSlice(z);
                  if (!source)
                    {
                      delete [] zSums;
                      delete [] rowPrefix;
                      delete [] rowAccum;
                      return 0;
                    }

                  for (qint64 outputY=0; outputY<height; outputY++)
                    {
                      memset(rowAccum, 0,
                             static_cast<size_t>(width)*sizeof(quint64));
                      const int centerY = miny+static_cast<int>(outputY)*lod;
                      const int sourceYMin = qMax(miny, centerY-lod+1);
                      const int sourceYMax =
                        qMin(miny+leny-1, centerY+lod-1);

                      for (int sourceY=sourceYMin;
                           sourceY<=sourceYMax; sourceY++)
                        {
                          const uchar *row = source+
                            static_cast<qint64>(sourceY)*m_height+minx;
                          rowPrefix[0] = 0;
                          for (int sourceX=0; sourceX<lenx; sourceX++)
                            rowPrefix[sourceX+1] =
                              rowPrefix[sourceX]+row[sourceX];

                          for (qint64 outputX=0; outputX<width; outputX++)
                            {
                              const int centerX =
                                static_cast<int>(outputX)*lod;
                              const int sourceXMin =
                                qMax(0, centerX-lod+1);
                              const int sourceXMax =
                                qMin(lenx-1, centerX+lod-1);
                              rowAccum[outputX] +=
                                rowPrefix[sourceXMax+1]-
                                rowPrefix[sourceXMin];
                            }
                        }

                      const qint64 sourceRows =
                        static_cast<qint64>(sourceYMax)-sourceYMin+1;
                      for (qint64 outputX=0; outputX<width; outputX++)
                        {
                          const int centerX =
                            static_cast<int>(outputX)*lod;
                          const int sourceXMin =
                            qMax(0, centerX-lod+1);
                          const int sourceXMax =
                            qMin(lenx-1, centerX+lod-1);
                          const qint64 area = sourceRows*
                            (static_cast<qint64>(sourceXMax)-
                             sourceXMin+1);
                          const qint64 index = outputY*width+outputX;
                          zSums[index] += static_cast<quint32>(
                            rowAccum[outputX]/qMax<qint64>(1, area));
                        }
                    }
                }

              for (qint64 index=0; index<voxelCount; index++)
                target[channels*index+channel] =
                  static_cast<uchar>(zSums[index]/zCount);
            }
        }

      if (!Global::histogramDisabled())
        for (qint64 index=0; index<voxelCount; index++)
          {
            const uchar r = target[channels*index];
            const uchar g = target[channels*index+1];
            const uchar b = target[channels*index+2];
            m_flhist1DR[r]++;  m_flhist2DR[g*256+r]++;
            m_flhist1DG[g]++;  m_flhist2DG[b*256+g]++;
            m_flhist1DB[b]++;  m_flhist2DB[r*256+b]++;
            if (channels == 4)
              {
                const uchar a = target[channels*index+3];
                const uchar rgb = qMax(r, qMax(g, b));
                m_flhist1DA[a]++;  m_flhist2DA[rgb*256+a]++;
              }
          }

      if (dragWidth > 0 && dragHeight > 0 && dragDepth > 0)
        {
          const qint64 dragLayer = qBound<qint64>(
            0, static_cast<qint64>(globalLayer/dragStep), dragDepth-1);
          uchar *dragTarget = m_dragSubvolumeTexture+
            static_cast<qint64>(channels)*dragLayer*dragWidth*dragHeight;
          for (int y=0; y<dragHeight; y++)
            {
              const qint64 sourceY = qBound<qint64>(
                0, static_cast<qint64>(y*dragStep), height-1);
              for (int x=0; x<dragWidth; x++)
                {
                  const qint64 sourceX = qBound<qint64>(
                    0, static_cast<qint64>(x*dragStep), width-1);
                  memcpy(dragTarget+
                           static_cast<qint64>(channels)*(y*dragWidth+x),
                         target+
                           static_cast<qint64>(channels)*
                           (sourceY*width+sourceX),
                         static_cast<size_t>(channels));
                }
            }
        }
    }

  delete [] zSums;
  delete [] rowPrefix;
  delete [] rowAccum;
  Global::progressBar()->setValue(100);
  MainWindowUI::mainWindowUI()->statusBar->showMessage("Ready");

  return m_subvolumeTexture;
}


void
VolumeRGB::startHistogramCalculation()
{
  memset(m_flhist1DR, 0, 256*4);
  memset(m_flhist2DR, 0, 256*256*4);
  memset(m_flhist1DG, 0, 256*4);
  memset(m_flhist2DG, 0, 256*256*4);
  memset(m_flhist1DB, 0, 256*4);
  memset(m_flhist2DB, 0, 256*256*4);
  memset(m_flhist1DA, 0, 256*4);
  memset(m_flhist2DA, 0, 256*256*4);
}

void
VolumeRGB::endHistogramCalculation()
{
  StaticFunctions::generateHistograms(m_flhist1DR, m_flhist2DR,
				      m_subvolume1dHistogramR,
				      m_subvolume2dHistogramR);
  StaticFunctions::generateHistograms(m_flhist1DG, m_flhist2DG,
				      m_subvolume1dHistogramG,
				      m_subvolume2dHistogramG);
  StaticFunctions::generateHistograms(m_flhist1DB, m_flhist2DB,
				      m_subvolume1dHistogramB,
				      m_subvolume2dHistogramB);
  if (Global::volumeType() == Global::RGBAVolume)
    StaticFunctions::generateHistograms(m_flhist1DA, m_flhist2DA,
					m_subvolume1dHistogramA,
					m_subvolume2dHistogramA);
}


void
VolumeRGB::maskRawVolume(unsigned char *lut,
			 QList<Vec> clipPos,
			 QList<Vec> clipNormal,
			 QList<CropObject> crops)
{
  if (!lut)
    {
      QMessageBox::warning(0, "Save Image Stack",
			   "The colour lookup table is unavailable");
      return;
    }

  QString imgflnm;
  imgflnm = QFileDialog::getSaveFileName(0,
			 "Save images with basename as",
		         Global::previousDirectory(),
			 "Image Files (*.png *.tif *.bmp *.jpg)");

  if (imgflnm.isEmpty())
    return;

  QFileInfo f(imgflnm);	
  QChar fillChar = '0';
  QImage timage;

  MainWindowUI::mainWindowUI()->menubar->parentWidget()->setWindowTitle("Saving image stack");
  Global::progressBar()->show();

  //---------------
  int nRGB = 3;
  if (Global::volumeType() == Global::RGBAVolume)
    nRGB = 4;
  //---------------


  int minx = m_dataMin.x;
  int maxx = m_dataMax.x;
  int miny = m_dataMin.y;
  int maxy = m_dataMax.y;
  int minz = m_dataMin.z;
  int maxz = m_dataMax.z;
  int lenx = m_subvolumeSize.x;
  int leny = m_subvolumeSize.y;
  int lenz = m_subvolumeSize.z;

  Vec voxelScaling = Global::voxelScaling();

  qint64 pixelCount = 1;
  if (lenz <= 0 ||
      !checkedSizeFactor(lenx, pixelCount) ||
      !checkedSizeFactor(leny, pixelCount))
    {
      MainWindowUI::mainWindowUI()->menubar->parentWidget()->
	setWindowTitle(QString("Drishti"));
      Global::hideProgressBar();
      QMessageBox::warning(0, "Save Image Stack",
			   "The image dimensions exceed the supported range");
      return;
    }

  qint64 imageBytes = pixelCount;
  if (!checkedSizeFactor(4, imageBytes) ||
      !validAllocationSize(imageBytes) ||
      lenx > std::numeric_limits<int>::max()/4)
    {
      MainWindowUI::mainWindowUI()->menubar->parentWidget()->
	setWindowTitle(QString("Drishti"));
      Global::hideProgressBar();
      QMessageBox::warning(0, "Save Image Stack",
			   "The image buffer size exceeds the supported range");
      return;
    }

  std::unique_ptr<uchar[]> vol(
    new (std::nothrow) uchar[static_cast<size_t>(imageBytes)]);
  std::unique_ptr<uchar[]> rgb(
    new (std::nothrow) uchar[static_cast<size_t>(imageBytes)]);
  if (!vol || !rgb)
    {
      MainWindowUI::mainWindowUI()->menubar->parentWidget()->
	setWindowTitle(QString("Drishti"));
      Global::hideProgressBar();
      QMessageBox::warning(0, "Save Image Stack",
			   "Not enough memory for the image export buffers");
      return;
    }


  QImage img = QImage(rgb.get(),
		      lenx, leny,
		      QImage::Format_ARGB32);      

  int lsize = 4*256*256;

  QStringList writtenFiles;
  for(int z=minz; z<=maxz; z++)
    {
      memset(vol.get(), 255, static_cast<size_t>(imageBytes));
      memset(rgb.get(), 0, static_cast<size_t>(imageBytes));

      for (int q=0; q<nRGB; q++)
	{
	  uchar *vslice = m_rgbaFileManager[q].getSlice(z);
	  if (!vslice)
	    {
	      const QString error = m_rgbaFileManager[q].lastError();
	      for (int i=0; i<writtenFiles.count(); ++i)
		QFile::remove(writtenFiles[i]);
	      MainWindowUI::mainWindowUI()->menubar->parentWidget()->
		setWindowTitle(QString("Drishti"));
	      Global::hideProgressBar();
	      QMessageBox::warning(0, "Save Image Stack",
		QString("Cannot read colour slice %1: %2")
		.arg(z).arg(error));
	      return;
	    }
	  
	  for(int y=miny; y<=maxy; y++)
	    for(int x=minx; x<=maxx; x++)
	      {
		const qint64 ij = static_cast<qint64>(y)*m_height+x;
		const qint64 idx =
		  static_cast<qint64>(y-miny)*lenx+(x-minx);
		vol[4*idx+q] = vslice[ij];
	      }
	}


      for(int y=miny; y<=maxy; y++)
	for(int x=minx; x<=maxx; x++)
	  {
	    const qint64 idx =
	      static_cast<qint64>(y-miny)*lenx+(x-minx);

	    Vec po = Vec(x, y, z);
	    po = VECPRODUCT(po, voxelScaling);

	    bool ok = StaticFunctions::getClip(po, clipPos, clipNormal);

	    for(int ci=0; ci<crops.count(); ci++)
	      ok &= crops[ci].checkCropped(po);

	    // apply clipping
	    if (ok)
	      {
		uchar r = vol[4*idx+0];
		uchar g = vol[4*idx+1];
		uchar b = vol[4*idx+2];
		uchar a = vol[4*idx+3];

		float opac = (lut[4*(256*g + r)+3]/255.0f);
		opac *= (lut[lsize + 4*(256*b + g)+3]/255.0f);
		opac *= (lut[2*lsize + 4*(256*r + b)+3]/255.0f);
		if (nRGB == 4)
		  {
		    a = vol[4*idx+3];
		    uchar rgb = qMax(r, qMax(g, b));
		    opac *= (lut[3*lsize + 4*(256*rgb + a)+3]/255.0f);
		  }

		if (opac > 0)
		  {
		    rgb[4*idx+0] = b * opac;
		    rgb[4*idx+1] = g * opac;
		    rgb[4*idx+2] = r * opac;
		    rgb[4*idx+3] = a * opac;
		  }
	      }
	  }

      QString flname = f.absolutePath() + QDir::separator() +
	               f.baseName();
      flname += QString("%1").arg((int)z, 5, 10, fillChar);
      flname += ".";
      flname += f.completeSuffix();

      if (!img.save(flname))
	{
	  for (int i=0; i<writtenFiles.count(); ++i)
	    QFile::remove(writtenFiles[i]);
	  QFile::remove(flname);
	  MainWindowUI::mainWindowUI()->menubar->parentWidget()->
	    setWindowTitle(QString("Drishti"));
	  Global::hideProgressBar();
	  QMessageBox::warning(0, "Save Image Stack",
			       QString("Cannot write image %1").arg(flname));
	  return;
	}
      writtenFiles.append(flname);
      Global::progressBar()->setValue((int)(100*(float)(z-minz)/(float)lenz));
      qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
    }

  Global::progressBar()->setValue(100);
  Global::hideProgressBar();
  MainWindowUI::mainWindowUI()->menubar->parentWidget()->
    setWindowTitle(QString("Drishti"));
}

void
VolumeRGB::saveOpacityVolume(unsigned char *lut,
			     QList<Vec> clipPos,
			     QList<Vec> clipNormal,
			     QList<CropObject> crops)
{
  if (!lut)
    {
      QMessageBox::warning(0, "Save Opacity Volume",
			   "The colour lookup table is unavailable");
      return;
    }

  QString opFile;

  opFile= QFileDialog::getSaveFileName(0,
				       "Save Opacity Volume",
				       Global::previousDirectory(),
				       "RAW Files (*.raw)");
  if (opFile.isEmpty())
    return;


  QFileInfo f(opFile);	
  QChar fillChar = '0';
  QImage timage;

  MainWindowUI::mainWindowUI()->menubar->parentWidget()->setWindowTitle("Saving image stack");
  Global::progressBar()->show();

  //---------------
  int nRGB = 3;
  if (Global::volumeType() == Global::RGBAVolume)
    nRGB = 4;
  //---------------


  int minx = m_dataMin.x;
  int maxx = m_dataMax.x;
  int miny = m_dataMin.y;
  int maxy = m_dataMax.y;
  int minz = m_dataMin.z;
  int maxz = m_dataMax.z;
  int lenx = m_subvolumeSize.x;
  int leny = m_subvolumeSize.y;
  int lenz = m_subvolumeSize.z;

  Vec voxelScaling = Global::voxelScaling();


  qint64 pixelCount = 1;
  if (lenz <= 0 ||
      !checkedSizeFactor(lenx, pixelCount) ||
      !checkedSizeFactor(leny, pixelCount) ||
      !validAllocationSize(pixelCount))
    {
      MainWindowUI::mainWindowUI()->menubar->parentWidget()->
	setWindowTitle(QString("Drishti"));
      Global::hideProgressBar();
      QMessageBox::warning(0, "Save Opacity Volume",
			   "The opacity volume dimensions exceed the supported range");
      return;
    }

  qint64 colourBytes = pixelCount;
  if (!checkedSizeFactor(4, colourBytes) ||
      !validAllocationSize(colourBytes))
    {
      MainWindowUI::mainWindowUI()->menubar->parentWidget()->
	setWindowTitle(QString("Drishti"));
      Global::hideProgressBar();
      QMessageBox::warning(0, "Save Opacity Volume",
			   "The colour slice buffer exceeds the supported range");
      return;
    }

  // Keep each output slab below one GiB without overflowing int arithmetic.
  const qint64 maxSlabBytes = 1024LL*1024LL*1024LL;
  const int opslabSize = static_cast<int>(qMax<qint64>(
    1, qMin<qint64>(std::numeric_limits<int>::max(),
		    maxSlabBytes/pixelCount)));
  VolumeFileManager opFileManager;
  opFileManager.setBaseFilename(opFile);
  opFileManager.setDepth(lenz);
  opFileManager.setWidth(leny);
  opFileManager.setHeight(lenx);
  opFileManager.setHeaderSize(13);
  opFileManager.setSlabSize(opslabSize);
  if (!opFileManager.createFile(true))
    {
      const QString error = opFileManager.lastError();
      MainWindowUI::mainWindowUI()->menubar->parentWidget()->
	setWindowTitle(QString("Drishti"));
      Global::hideProgressBar();
      QMessageBox::warning(0, "Save Opacity Volume",
			   QString("Cannot create output volume: %1").arg(error));
      return;
    }

  std::unique_ptr<uchar[]> opacity(
    new (std::nothrow) uchar[static_cast<size_t>(pixelCount)]);
  std::unique_ptr<uchar[]> vol(
    new (std::nothrow) uchar[static_cast<size_t>(colourBytes)]);
  if (!opacity || !vol)
    {
      opFileManager.removeFile();
      MainWindowUI::mainWindowUI()->menubar->parentWidget()->
	setWindowTitle(QString("Drishti"));
      Global::hideProgressBar();
      QMessageBox::warning(0, "Save Opacity Volume",
			   "Not enough memory for the opacity export buffers");
      return;
    }


  int lsize = 4*256*256;

  for(int z=minz; z<=maxz; z++)
    {
      memset(vol.get(), 255, static_cast<size_t>(colourBytes));
      memset(opacity.get(), 0, static_cast<size_t>(pixelCount));

      for (int q=0; q<nRGB; q++)
	{
	  uchar *vslice = m_rgbaFileManager[q].getSlice(z);
	  if (!vslice)
	    {
	      const QString error = m_rgbaFileManager[q].lastError();
	      opFileManager.removeFile();
	      MainWindowUI::mainWindowUI()->menubar->parentWidget()->
		setWindowTitle(QString("Drishti"));
	      Global::hideProgressBar();
	      QMessageBox::warning(0, "Save Opacity Volume",
		QString("Cannot read colour slice %1: %2")
		.arg(z).arg(error));
	      return;
	    }
	  
	  for(int y=miny; y<=maxy; y++)
	    for(int x=minx; x<=maxx; x++)
	      {
		const qint64 ij = static_cast<qint64>(y)*m_height+x;
		const qint64 idx =
		  static_cast<qint64>(y-miny)*lenx+(x-minx);
		vol[4*idx+q] = vslice[ij];
	      }
	}


      for(int y=miny; y<=maxy; y++)
	for(int x=minx; x<=maxx; x++)
	  {
	    const qint64 idx =
	      static_cast<qint64>(y-miny)*lenx+(x-minx);

	    Vec po = Vec(x, y, z);
	    po = VECPRODUCT(po, voxelScaling);

	    bool ok = StaticFunctions::getClip(po, clipPos, clipNormal);

	    for(int ci=0; ci<crops.count(); ci++)
	      ok &= crops[ci].checkCropped(po);

	    // apply clipping
	    if (ok)
	      {
		uchar r = vol[4*idx+0];
		uchar g = vol[4*idx+1];
		uchar b = vol[4*idx+2];
		uchar a = vol[4*idx+3];

		float opac = (lut[4*(256*g + r)+3]/255.0f);
		opac *= (lut[lsize + 4*(256*b + g)+3]/255.0f);
		opac *= (lut[2*lsize + 4*(256*r + b)+3]/255.0f);
		if (nRGB == 4)
		  {
		    a = vol[4*idx+3];
		    uchar rgb = qMax(r, qMax(g, b));
		    opac *= (lut[3*lsize + 4*(256*rgb + a)+3]/255.0f);
		  }

		opacity[idx] = opac*255;
	      }
	  }

      if (!opFileManager.setSlice(z-minz, opacity.get()))
	{
	  const QString error = opFileManager.lastError();
	  opFileManager.removeFile();
	  MainWindowUI::mainWindowUI()->menubar->parentWidget()->
	    setWindowTitle(QString("Drishti"));
	  Global::hideProgressBar();
	  QMessageBox::warning(0, "Save Opacity Volume",
		QString("Cannot write opacity slice %1: %2")
		.arg(z-minz).arg(error));
	  return;
	}
      Global::progressBar()->setValue((int)(100*(float)(z-minz)/(float)lenz));
      qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
    }


  MainWindowUI::mainWindowUI()->menubar->parentWidget()->\
    setWindowTitle(QString("Drishti"));
  Global::progressBar()->setValue(100);
  Global::hideProgressBar();

  QMessageBox::information(0, "Save Opacity Volume",
			   QString("Saved opacity volume to ")+opFile);
}
