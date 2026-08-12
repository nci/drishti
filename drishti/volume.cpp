#include "global.h"

#include "volume.h"
#include "staticfunctions.h"
#include "xmlheaderfunctions.h"

#include <limits>
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


bool Volume::valid()
{
  if (Global::volumeType() == Global::RGBVolume ||
      Global::volumeType() == Global::RGBAVolume)
    return m_volumeRGB != 0;

  return !m_volume.isEmpty();
}

float Volume::bbScale() { return m_bbScale; }

void
Volume::setBBScale(float vs)
{
  m_bbScale = vs;
  if (m_volume.count() > 0)
    QMessageBox::information(0, "Volume Bounding Box",
			     "Cannot change bounding box size once the data is loaded.\n Change will be reflected when you load datasets in the current session.\n Changes in bounding box are not persistent across the sessions.");
  else
    QMessageBox::information(0, "Volume Bounding Box",
			     "Scaled bounding box will be available in hires mode when you load project or dataset in this session.\nChanges in bounding box are not persistent across the sessions.");
}

QList<Vec>
Volume::offsets()
{
  QList<Vec> off;
  
  for (int i=0; i<m_volume.count(); i++)
    off << m_volume[i]->offset();

  return off;
}
  
void
Volume::setOffsets(int v, float od, float ow, float oh)
{
  if (v == -1)
    {
      for (int i=0; i<m_volume.count(); i++)
	m_volume[i]->setOffsets(od, ow, oh);
    }
  else
    {
      if (v>=0 && v<m_volume.count())
	m_volume[v]->setOffsets(od, ow, oh);
    }
}
		   
int
Volume::pvlVoxelType(int vol)
{
  if (Global::volumeType() == Global::DummyVolume ||
      Global::volumeType() == Global::RGBVolume ||
      Global::volumeType() == Global::RGBAVolume)
    return 0;

  return m_volume[vol]->pvlVoxelType();
}


void
Volume::closePvlFileManager()
{
  if (Global::volumeType() == Global::DummyVolume ||
      Global::volumeType() == Global::RGBVolume ||
      Global::volumeType() == Global::RGBAVolume)
    return;

  if (Global::volumeType() == Global::SingleVolume)
    m_volume[0]->closePvlFileManager();
  else if (Global::volumeType() == Global::DoubleVolume)
    {
      m_volume[0]->closePvlFileManager();
      m_volume[1]->closePvlFileManager();
    }
  else if (Global::volumeType() == Global::TripleVolume)
    {
      m_volume[0]->closePvlFileManager();
      m_volume[1]->closePvlFileManager();
      m_volume[2]->closePvlFileManager();
    }
  else if (Global::volumeType() == Global::QuadVolume)
    {
      m_volume[0]->closePvlFileManager();
      m_volume[1]->closePvlFileManager();
      m_volume[2]->closePvlFileManager();
      m_volume[3]->closePvlFileManager();
    }
}

VolumeFileManager*
Volume::pvlFileManager(int vol)
{
  if (Global::volumeType() == Global::DummyVolume ||
      Global::volumeType() == Global::RGBVolume ||
      Global::volumeType() == Global::RGBAVolume)
    return 0;

  return m_volume[vol]->pvlFileManager();
}
VolumeFileManager*
Volume::gradFileManager(int vol)
{
  if (Global::volumeType() == Global::DummyVolume ||
      Global::volumeType() == Global::RGBVolume ||
      Global::volumeType() == Global::RGBAVolume)
    return 0;

  return m_volume[vol]->gradFileManager();
}
VolumeFileManager*
Volume::lodFileManager(int vol)
{
  if (Global::volumeType() == Global::DummyVolume ||
      Global::volumeType() == Global::RGBVolume ||
      Global::volumeType() == Global::RGBAVolume)
    return 0;

  return m_volume[vol]->lodFileManager();
}

int
Volume::timestepNumber(int vol, int n)
{
  if (Global::volumeType() == Global::DummyVolume)
    return n;

  if (Global::volumeType() == Global::RGBVolume ||
      Global::volumeType() == Global::RGBAVolume)
    return n;

  return m_volume[vol]->timestepNumber(n);
}

QList<float>
Volume::getThicknessProfile(int searchType,
			    uchar *lut,
			    QList<Vec> voxel,
			    QList<Vec> normal)
{
  if (Global::volumeType() == Global::SingleVolume)
    return m_volume[0]->getThicknessProfile(searchType, lut, voxel, normal);

  // return dummy list
  QList<float> t;
  return t;
}

QList<Vec>
Volume::stickToSurface(uchar *lut,
		       int rad,
		       QList< QPair<Vec,Vec> > pn)
{
  if (Global::volumeType() == Global::SingleVolume)
    return m_volume[0]->stickToSurface(lut, rad, pn);

  // return dummy list
  QList<Vec> t;
  return t;
}

QMap<QString, QList<QVariant> >
Volume::rawValues(int radius,
		  QList<Vec> pos)
{
  if (Global::volumeType() == Global::SingleVolume)
    return m_volume[0]->rawValues(radius, pos);

  // return dummy list
  QMap<QString, QList<QVariant> > t;
  return t;
}

QList<QVariant>
Volume::rawValues(QList<Vec> pos)
{
  if (Global::volumeType() == Global::SingleVolume)
    return m_volume[0]->rawValues(pos);

  // return dummy list
  QList<QVariant> t;
  return t;
}

void
Volume::startHistogramCalculation()
{
  if (Global::volumeType() == Global::DummyVolume)
    return;

  if (Global::volumeType() == Global::SingleVolume)
    m_volume[0]->startHistogramCalculation();
  else if (Global::volumeType() == Global::DoubleVolume)
    {
      m_volume[0]->startHistogramCalculation();
      m_volume[1]->startHistogramCalculation();
    }
  else if (Global::volumeType() == Global::TripleVolume)
    {
      m_volume[0]->startHistogramCalculation();
      m_volume[1]->startHistogramCalculation();
      m_volume[2]->startHistogramCalculation();
    }
  else if (Global::volumeType() == Global::QuadVolume)
    {
      m_volume[0]->startHistogramCalculation();
      m_volume[1]->startHistogramCalculation();
      m_volume[2]->startHistogramCalculation();
      m_volume[3]->startHistogramCalculation();
    }
  else if (Global::volumeType() == Global::RGBVolume ||
	   Global::volumeType() == Global::RGBAVolume)
      m_volumeRGB->startHistogramCalculation();
}

void
Volume::endHistogramCalculation()
{
  if (Global::volumeType() == Global::DummyVolume)
    return;

  if (Global::volumeType() == Global::SingleVolume)
    m_volume[0]->endHistogramCalculation();
  else if (Global::volumeType() == Global::DoubleVolume)
    {
      m_volume[0]->endHistogramCalculation();
      m_volume[1]->endHistogramCalculation();
    }
  else if (Global::volumeType() == Global::TripleVolume)
    {
      m_volume[0]->endHistogramCalculation();
      m_volume[1]->endHistogramCalculation();
      m_volume[2]->endHistogramCalculation();
    }
  else if (Global::volumeType() == Global::QuadVolume)
    {
      m_volume[0]->endHistogramCalculation();
      m_volume[1]->endHistogramCalculation();
      m_volume[2]->endHistogramCalculation();
      m_volume[3]->endHistogramCalculation();
    }
  else if (Global::volumeType() == Global::RGBVolume ||
	   Global::volumeType() == Global::RGBAVolume)
      m_volumeRGB->endHistogramCalculation();
}

void
Volume::getColumnsAndRows(int &ncols, int &nrows)
{
  if (Global::volumeType() == Global::DummyVolume)
    {
      ncols = nrows = 1;
    }
  else if (Global::volumeType() == Global::SingleVolume)
    m_volume[0]->getColumnsAndRows(ncols, nrows);
  else if (Global::volumeType() == Global::RGBVolume ||
	   Global::volumeType() == Global::RGBAVolume)
    m_volumeRGB->getColumnsAndRows(ncols, nrows);
  else
    {
      int nvol = 2;
      if (Global::volumeType() == Global::DoubleVolume) nvol = 2;
      if (Global::volumeType() == Global::TripleVolume) nvol = 3;
      if (Global::volumeType() == Global::QuadVolume) nvol = 4;

      int texWidth = 0, texHeight = 0;
      int maxlenx2 = 0, maxleny2 = 0;
      int lenx2[4], leny2[4];
      for (int v=0; v<nvol; v++)
	{
	  Vec subvolsize = m_volume[v]->getSubvolumeSize();
	  int svsl = m_volume[v]->getSubvolumeSubsamplingLevel();      
	  int lenx = subvolsize.x;
	  int leny = subvolsize.y;
	  lenx2[v] = lenx/svsl;
	  leny2[v] = leny/svsl;
	  maxlenx2 = qMax(maxlenx2, lenx2[v]);
	  maxleny2 = qMax(maxleny2, leny2[v]);

	  int texw, texh;
	  m_volume[v]->getSliceTextureSize(texw, texh);
	  texWidth = qMax(texWidth, texw);
	  texHeight = qMax(texHeight, texh);
	}     

      ncols = texWidth/maxlenx2;
      nrows = texHeight/maxleny2;
    }
}

void Volume::getSliceTextureSize(int& texX, int& texY)
{
  if (Global::volumeType() == Global::DummyVolume)
    {
      texX = texY = 1;
      return;
    }

  if (Global::volumeType() == Global::SingleVolume)
    m_volume[0]->getSliceTextureSize(texX, texY);
  else if (Global::volumeType() == Global::RGBVolume ||
	   Global::volumeType() == Global::RGBAVolume)
    m_volumeRGB->getSliceTextureSize(texX, texY);
  else
    {
      int nvol = 2;
      if (Global::volumeType() == Global::DoubleVolume) nvol = 2;
      if (Global::volumeType() == Global::TripleVolume) nvol = 3;
      if (Global::volumeType() == Global::QuadVolume) nvol = 4;

      int texWidth = 0, texHeight = 0;
      int maxlenx2 = 0, maxleny2 = 0;
      for (int v=0; v<nvol; v++)
	{
	  int texw, texh;
	  m_volume[v]->getSliceTextureSize(texw, texh);
	  texWidth = qMax(texWidth, texw);
	  texHeight = qMax(texHeight, texh);
	}     

      texX = texWidth;
      texY = texHeight;
    }

}

Vec
Volume::getDragTextureInfo()
{
  if (Global::volumeType() == Global::DummyVolume)
    return Vec(1,1,1);

  if (Global::volumeType() == Global::RGBVolume ||
      Global::volumeType() == Global::RGBAVolume)
  return m_volumeRGB->getDragTextureInfo();


  return m_volume[0]->getDragTextureInfo();
}

void
Volume::getDragTextureSize(int &dtexX, int &dtexY)
{
  if (Global::volumeType() == Global::DummyVolume)
    {
      dtexX = dtexY = 128;
      return;
    }

  if (Global::volumeType() == Global::RGBVolume ||
      Global::volumeType() == Global::RGBAVolume)
    {
      m_volumeRGB->getDragTextureSize(dtexX, dtexY);
      return;
    }

  m_volume[0]->getDragTextureSize(dtexX, dtexY);
}

QList<Vec>
Volume::getSliceTextureSizeSlabs()
{
  if (Global::volumeType() == Global::DummyVolume)
    {
      QList<Vec> stss;
      stss.clear();
      return stss;
    }

  if (Global::volumeType() == Global::SingleVolume)
    return m_volume[0]->getSliceTextureSizeSlabs();
  else if (Global::volumeType() == Global::RGBVolume ||
	   Global::volumeType() == Global::RGBAVolume)
    return m_volumeRGB->getSliceTextureSizeSlabs();
  else
    {
      int nvol = 2;
      if (Global::volumeType() == Global::DoubleVolume) nvol = 2;
      if (Global::volumeType() == Global::TripleVolume) nvol = 3;
      if (Global::volumeType() == Global::QuadVolume) nvol = 4;

      m_volume[0]->getSliceTextureSizeSlabs();

      Vec dataMin, dataMax;
      dataMin = m_volume[0]->getSubvolumeMin();
      dataMax = m_volume[0]->getSubvolumeMax();
      for (int v=1; v<nvol; v++)
	{
	  m_volume[v]->getSliceTextureSizeSlabs();

	  dataMin = StaticFunctions::minVec(dataMin,
		       m_volume[v]->getSubvolumeMin());
	  dataMax = StaticFunctions::maxVec(dataMax,
		       m_volume[v]->getSubvolumeMax());
	}

      int bpv = nvol;
      int tms = Global::textureMemorySize()-25*Global::actualDragVolSize(); // in Mb
      int svsl = StaticFunctions::getSubsamplingLevel(tms,
						      Global::maxArrayTextureLayers(),
						      bpv,
						      dataMin, dataMax);

      int nrows, ncols;
      QList<Vec> slabinfo = Global::getSlabs(svsl, bpv,
					     dataMin, dataMax,
					     nrows, ncols);

      
      int lenx = dataMax.x-dataMin.x+1;
      int leny = dataMax.y-dataMin.y+1;
      int lenx2 = lenx/svsl;
      int leny2 = leny/svsl;
      int texWidth = ncols*lenx2;
      int texHeight = nrows*leny2;

      Vec draginfo = Global::getDragInfo(bpv, dataMin, dataMax, 1);

      int dlenx2 = qMax(1, lenx/int(draginfo.z));
      int dleny2 = qMax(1, leny/int(draginfo.z));
      int dtexWidth = int(draginfo.x)*dlenx2;
      int dtexHeight = int(draginfo.y)*dleny2;
      
      for (int v=0; v<nvol; v++)
	m_volume[v]->forMultipleVolumes(svsl,
					draginfo, dtexWidth, dtexHeight,
					texWidth, texHeight,
					ncols, nrows);

      return slabinfo;
    }
}

void
Volume::deleteTextureSlab()
{
  if (m_subvolumeTexture) delete [] m_subvolumeTexture;
  m_subvolumeTexture = 0;
  m_slabLayerCapacity = 0;

  if (m_channelSlabTexture) delete [] m_channelSlabTexture;
  m_channelSlabTexture = 0;
  m_channelSlabBytes = 0;

  if (m_dragSubvolumeTexture) delete [] m_dragSubvolumeTexture;
  m_dragSubvolumeTexture = 0;

  if ((Global::volumeType() == Global::RGBVolume ||
       Global::volumeType() == Global::RGBAVolume) && m_volumeRGB)
    m_volumeRGB->deleteTextureSlab();
  else
    for (int v=0; v<m_volume.count(); v++)
      m_volume[v]->deleteTextureSlab();
}

bool Volume::forceCreateLowresVolume()
{
  if (!valid())
    return false;

  if (Global::volumeType() == Global::DummyVolume)
    return true;

  if (Global::volumeType() == Global::RGBVolume ||
      Global::volumeType() == Global::RGBAVolume)
    return m_volumeRGB && m_volumeRGB->createLowresTextureVolume();

  int volumeCount = 1;
  if (Global::volumeType() == Global::DoubleVolume)
    volumeCount = 2;
  else if (Global::volumeType() == Global::TripleVolume)
    volumeCount = 3;
  else if (Global::volumeType() == Global::QuadVolume)
    volumeCount = 4;

  if (m_volume.count() < volumeCount)
    return false;

  for (int volume=0; volume<volumeCount; ++volume)
    if (!m_volume[volume] ||
	!m_volume[volume]->createLowresTextureVolume())
      return false;

  return true;
}

int* Volume::getLowres1dHistogram(int vol)
{
  if (Global::volumeType() == Global::DummyVolume)
    return NULL;

  if (Global::volumeType() == Global::RGBVolume ||
      Global::volumeType() == Global::RGBAVolume)
    return m_volumeRGB->getLowres1dHistogram(vol);

  return m_volume[vol]->getLowres1dHistogram();
}
int* Volume::getLowres2dHistogram(int vol)
{
  if (Global::volumeType() == Global::DummyVolume)
    return NULL;

  if (Global::volumeType() == Global::RGBVolume ||
      Global::volumeType() == Global::RGBAVolume)
    return m_volumeRGB->getLowres2dHistogram(vol);

  return m_volume[vol]->getLowres2dHistogram();
}

int* Volume::getSubvolume1dHistogram(int vol)
{
  if (Global::volumeType() == Global::DummyVolume)
    return NULL;

  if (Global::volumeType() == Global::RGBVolume ||
      Global::volumeType() == Global::RGBAVolume)
    return m_volumeRGB->getSubvolume1dHistogram(vol);

  return m_volume[vol]->getSubvolume1dHistogram();
}
int* Volume::getSubvolume2dHistogram(int vol)
{
  if (Global::volumeType() == Global::DummyVolume)
    return NULL;

  if (Global::volumeType() == Global::RGBVolume ||
      Global::volumeType() == Global::RGBAVolume)
    return m_volumeRGB->getSubvolume2dHistogram(vol);

  return m_volume[vol]->getSubvolume2dHistogram();
}

int* Volume::getDrag1dHistogram(int vol)
{
  if (Global::volumeType() == Global::DummyVolume)
    return NULL;

  if (Global::volumeType() == Global::RGBVolume ||
      Global::volumeType() == Global::RGBAVolume)
    return m_volumeRGB->getDrag1dHistogram(vol);

  return m_volume[vol]->getDrag1dHistogram();
}
int* Volume::getDrag2dHistogram(int vol)
{
  if (Global::volumeType() == Global::DummyVolume)
    return NULL;

  if (Global::volumeType() == Global::RGBVolume ||
      Global::volumeType() == Global::RGBAVolume)
    return m_volumeRGB->getDrag2dHistogram(vol);

  return m_volume[vol]->getDrag2dHistogram();
}

QList<QString> Volume::volumeFiles(int vol)
{
  if (Global::volumeType() == Global::DummyVolume)
    {
      QList<QString> vf;
      vf.clear();
      return vf;
    }

  if (Global::volumeType() == Global::RGBVolume ||
      Global::volumeType() == Global::RGBAVolume)
    return m_volumeRGB->volumeFiles();

  return m_volume[vol]->volumeFiles();
}

Volume::Volume()
{
  Global::setVolumeType(Global::DummyVolume);
  m_volume.clear();
  m_volumeRGB = 0;
  m_subvolumeTexture = 0;
  m_slabLayerCapacity = 0;
  m_channelSlabTexture = 0;
  m_channelSlabBytes = 0;
  m_dragSubvolumeTexture = 0;
  m_dragTexture = 0;
  m_lowresTexture = 0;
  m_bbScale = 1.0;
}

Volume::~Volume()
{
  clearVolumes();
}

void
Volume::clearVolumes()
{
  if (m_subvolumeTexture)
    delete [] m_subvolumeTexture;
  m_subvolumeTexture = 0;
  m_slabLayerCapacity = 0;

  if (m_channelSlabTexture)
    delete [] m_channelSlabTexture;
  m_channelSlabTexture = 0;
  m_channelSlabBytes = 0;

  if (m_dragSubvolumeTexture)
    delete [] m_dragSubvolumeTexture;
  m_dragSubvolumeTexture = 0;

  if (m_dragTexture)
    delete [] m_dragTexture;
  m_dragTexture = 0;

  if (m_lowresTexture)
    delete [] m_lowresTexture;
  m_lowresTexture = 0;

  for(int i=0; i<m_volume.count(); i++)
    delete m_volume[i];
  m_volume.clear();

  if (m_volumeRGB)
    delete m_volumeRGB;
  m_volumeRGB = 0;

  Global::setVolumeType(Global::DummyVolume);
}

bool
Volume::loadVolumeRGB(const char *flnm, bool redo)
{
  clearVolumes();

  bool rgba = VolumeInformation::checkRGBA(flnm);

  if (rgba)
    Global::setVolumeType(Global::RGBAVolume);
  else
    Global::setVolumeType(Global::RGBVolume);

  m_volumeRGB = new (std::nothrow) VolumeRGB;
  if (!m_volumeRGB || !m_volumeRGB->loadVolume(flnm, redo))
    {
      clearVolumes();
      return false;
    }

  return true;
}

bool
Volume::loadDummyVolume(int nx, int ny, int nz)
{
  clearVolumes();

  Global::setVolumeType(Global::DummyVolume);

  VolumeSingle *vol = new (std::nothrow) VolumeSingle;
  if (!vol)
    {
      clearVolumes();
      return false;
    }
  if (vol->loadDummyVolume(nx, ny, nz))
    m_volume.append(vol);
  else
    {
      delete vol;
      clearVolumes();
      return false;
    }

  return true;
}

bool
Volume::loadVolume(QList<QString> vfiles, bool redo)
{
  clearVolumes();

  Global::setVolumeType(Global::SingleVolume);

  VolumeSingle *vol = new (std::nothrow) VolumeSingle;
  if (!vol)
    {
      clearVolumes();
      return false;
    }
  if (vol->loadVolume(vfiles, redo))
    {
      m_volume.append(vol);

      // used for centering smaller volume within larger ones
      Vec fvs = getFullVolumeSize();
      int h = fvs.x;
      int w = fvs.y;
      int d = fvs.z;
      if (!m_volume[0]->setMaxDimensions(h,w,d))
	{
	  clearVolumes();
	  return false;
	}
    }
  else
    {
      delete vol;
      clearVolumes();
      return false;
    }

  return true;
}

bool
Volume::loadVolume(QList<QString> vfiles0,
		   QList<QString> vfiles1,
		   bool redo)
{
  clearVolumes();

  Global::setVolumeType(Global::DoubleVolume);

  VolumeSingle *vol0 = new (std::nothrow) VolumeSingle;
  VolumeSingle *vol1 = new (std::nothrow) VolumeSingle;
  if (!vol0 || !vol1)
    {
      delete vol0;
      delete vol1;
      clearVolumes();
      return false;
    }
  if (vol0->loadVolume(vfiles0, redo) &&
      vol1->loadVolume(vfiles1, redo))
    {
      if (vol0->pvlVoxelType() != 0 ||
	  vol1->pvlVoxelType() != 0)
	{
	  QMessageBox::information(0, "Error",
				   "Cannot load multiple 16bit volumes");
	  delete vol0;
	  delete vol1;
	  clearVolumes();
	  return false;
	}
      m_volume.append(vol0);
      m_volume.append(vol1);

      // used for centering smaller volume within larger ones
      Vec fvs = getFullVolumeSize();
      int h = fvs.x;
      int w = fvs.y;
      int d = fvs.z;
      if (!m_volume[0]->setMaxDimensions(h,w,d) ||
	  !m_volume[1]->setMaxDimensions(h,w,d))
	{
	  clearVolumes();
	  return false;
	}
    }
  else
    {
      delete vol0;
      delete vol1;
      clearVolumes();
      return false;
    }

  return true;
}

bool
Volume::loadVolume(QList<QString> vfiles0,
		   QList<QString> vfiles1,
		   QList<QString> vfiles2,
		   bool redo)
{
  clearVolumes();

  Global::setVolumeType(Global::TripleVolume);

  VolumeSingle *vol0 = new (std::nothrow) VolumeSingle;
  VolumeSingle *vol1 = new (std::nothrow) VolumeSingle;
  VolumeSingle *vol2 = new (std::nothrow) VolumeSingle;
  if (!vol0 || !vol1 || !vol2)
    {
      delete vol0;
      delete vol1;
      delete vol2;
      clearVolumes();
      return false;
    }
  if (vol0->loadVolume(vfiles0, redo) &&
      vol1->loadVolume(vfiles1, redo) &&
      vol2->loadVolume(vfiles2, redo))
    {
      if (vol0->pvlVoxelType() != 0 ||
	  vol1->pvlVoxelType() != 0 ||
	  vol2->pvlVoxelType() != 0)
	{
	  QMessageBox::information(0, "Error",
				   "Cannot load multiple 16bit volumes");
	  delete vol0;
	  delete vol1;
	  delete vol2;
	  clearVolumes();
	  return false;
	}
      m_volume.append(vol0);
      m_volume.append(vol1);
      m_volume.append(vol2);

      // used for centering smaller volume within larger ones
      Vec fvs = getFullVolumeSize();
      int h = fvs.x;
      int w = fvs.y;
      int d = fvs.z;
      for(int v=0; v<3; v++)
	if (!m_volume[v]->setMaxDimensions(h,w,d))
	  {
	    clearVolumes();
	    return false;
	  }
    }
  else
    {
      delete vol0;
      delete vol1;
      delete vol2;
      clearVolumes();
      return false;
    }

  return true;
}

bool
Volume::loadVolume(QList<QString> vfiles0,
		   QList<QString> vfiles1,
		   QList<QString> vfiles2,
		   QList<QString> vfiles3,
		   bool redo)
{
  clearVolumes();

  Global::setVolumeType(Global::QuadVolume);

  VolumeSingle *vol0 = new (std::nothrow) VolumeSingle;
  VolumeSingle *vol1 = new (std::nothrow) VolumeSingle;
  VolumeSingle *vol2 = new (std::nothrow) VolumeSingle;
  VolumeSingle *vol3 = new (std::nothrow) VolumeSingle;
  if (!vol0 || !vol1 || !vol2 || !vol3)
    {
      delete vol0;
      delete vol1;
      delete vol2;
      delete vol3;
      clearVolumes();
      return false;
    }
  if (vol0->loadVolume(vfiles0, redo) &&
      vol1->loadVolume(vfiles1, redo) &&
      vol2->loadVolume(vfiles2, redo) &&
      vol3->loadVolume(vfiles3, redo))
    {
      if (vol0->pvlVoxelType() != 0 ||
	  vol1->pvlVoxelType() != 0 ||
	  vol2->pvlVoxelType() != 0 ||
	  vol3->pvlVoxelType() != 0)
	{
	  QMessageBox::information(0, "Error",
				   "Cannot load multiple 16bit volumes");
	  delete vol0;
	  delete vol1;
	  delete vol2;
	  delete vol3;
	  clearVolumes();
	  return false;
	}
      m_volume.append(vol0);
      m_volume.append(vol1);
      m_volume.append(vol2);
      m_volume.append(vol3);

      // used for centering smaller volume within larger ones
      Vec fvs = getFullVolumeSize();
      int h = fvs.x;
      int w = fvs.y;
      int d = fvs.z;
      for(int v=0; v<4; v++)
	if (!m_volume[v]->setMaxDimensions(h,w,d))
	  {
	    clearVolumes();
	    return false;
	  }
    }
  else
    {
      delete vol0;
      delete vol1;
      delete vol2;
      delete vol3;
      clearVolumes();
      return false;
    }

  return true;
}

void
Volume::setRepeatType(QList<bool> rt)
{
  if (m_volume.count() == 0)
    return;
  if (Global::volumeType() == Global::DummyVolume)
    return;
  if (Global::volumeType() == Global::RGBVolume ||
      Global::volumeType() == Global::RGBAVolume)
    return;

  for(int i=0; i<qMin(m_volume.count(), rt.count()); i++)
    m_volume[i]->setRepeatType(rt[i]);
    
}

void
Volume::setRepeatType(int vol, bool rt)
{
  if (vol > m_volume.count())
    return;

  if (Global::volumeType() == Global::DummyVolume)
    return;

  if (Global::volumeType() == Global::RGBVolume ||
      Global::volumeType() == Global::RGBAVolume)
    return;
      
  m_volume[vol]->setRepeatType(rt);
}

bool
Volume::setSubvolume(Vec boxMin, Vec boxMax,
		     int volnum,
		     bool force)
{  
  if (Global::volumeType() == Global::RGBVolume ||
      Global::volumeType() == Global::RGBAVolume)
    return m_volumeRGB->setSubvolume(boxMin, boxMax,
				     volnum,
				     force);
  else
    {
      if (m_volume.isEmpty())
	return false;

      const int bpv = m_volume[0]->pvlVoxelType() > 0 ? 2 : 1;

      int tms = Global::textureMemorySize()-10*Global::actualDragVolSize(); // in Mb
      int sslevel = StaticFunctions::getSubsamplingLevel(tms,
							 Global::maxArrayTextureLayers(),
							 bpv,
							 boxMin, boxMax);
      sslevel = qMax(sslevel, Global::lod());

      return m_volume[0]->setSubvolume(boxMin, boxMax,
				       sslevel,
				       volnum,
				       force);
    }
}

bool
Volume::setSubvolume(Vec boxMin, Vec boxMax,
		     int volnum0, int volnum1,
		     bool force)
{  
  int bpv = 2;
  int tms = Global::textureMemorySize()-25*Global::actualDragVolSize(); // in Mb
  int sslevel = StaticFunctions::getSubsamplingLevel(tms,
						     Global::maxArrayTextureLayers(),
						     bpv,
						     boxMin, boxMax);

  sslevel = qMax(sslevel, Global::lod());

  bool flag;
  flag = m_volume[0]->setSubvolume(boxMin, boxMax,
				   sslevel,
				   volnum0,
				   force);

  flag |= m_volume[1]->setSubvolume(boxMin, boxMax,
				    sslevel,
				    volnum1,
				    force);

  return flag;
}

bool
Volume::setSubvolume(Vec boxMin, Vec boxMax,
		     int volnum0, int volnum1, int volnum2,
		     bool force)
{  
  int bpv = 3;
  int tms = Global::textureMemorySize()-25*Global::actualDragVolSize(); // in Mb
  int sslevel = StaticFunctions::getSubsamplingLevel(tms,
						     Global::maxArrayTextureLayers(),
						     bpv,
						     boxMin, boxMax);

  sslevel = qMax(sslevel, Global::lod());

  bool flag;
  flag = m_volume[0]->setSubvolume(boxMin, boxMax,
				   sslevel,
				   volnum0,
				   force);

  flag |= m_volume[1]->setSubvolume(boxMin, boxMax,
				    sslevel,
				    volnum1,
				    force);

  flag |= m_volume[2]->setSubvolume(boxMin, boxMax,
				    sslevel,
				    volnum2,
				    force);

  return flag;
}

bool
Volume::setSubvolume(Vec boxMin, Vec boxMax,
		     int volnum0, int volnum1,
		     int volnum2, int volnum3,
		     bool force)
{  
  int bpv = 4;

  int tms = Global::textureMemorySize()-25*Global::actualDragVolSize(); // in Mb
  int sslevel = StaticFunctions::getSubsamplingLevel(tms,
						     Global::maxArrayTextureLayers(),
						     bpv,
						     boxMin, boxMax);

  sslevel = qMax(sslevel, Global::lod());

  bool flag;
  flag = m_volume[0]->setSubvolume(boxMin, boxMax,
				   sslevel,
				   volnum0,
				   force);

  flag |= m_volume[1]->setSubvolume(boxMin, boxMax,
				    sslevel,
				    volnum1,
				    force);

  flag |= m_volume[2]->setSubvolume(boxMin, boxMax,
				    sslevel,
				    volnum2,
				    force);

  flag |= m_volume[3]->setSubvolume(boxMin, boxMax,
				    sslevel,
				    volnum3,
				    force);

  return flag;
}

VolumeInformation
Volume::volInfo(int vnum, int vol)
{
  if (Global::volumeType() == Global::RGBVolume ||
      Global::volumeType() == Global::RGBAVolume)
    {
      Global::setActualVolumeNumber(vnum, 0);
      return m_volumeRGB->volInfo(vnum);
    }

  if (vol > m_volume.count())
    {
      Global::setActualVolumeNumber(m_volume[0]->actualVolumeNumber(vnum), 0);
      return m_volume[0]->volInfo(vnum);
    }

  Global::setActualVolumeNumber(m_volume[vol]->actualVolumeNumber(vnum), vol);
  return m_volume[vol]->volInfo(vnum);
}

Vec Volume::getSubvolumeSize()
{
  if (Global::volumeType() == Global::RGBVolume ||
      Global::volumeType() == Global::RGBAVolume)
    return m_volumeRGB->getSubvolumeSize();
  
  if (Global::volumeType() == Global::SingleVolume ||
      Global::volumeType() == Global::DummyVolume)
    return m_volume[0]->getSubvolumeSize();


  int nvol = 2;
  if (Global::volumeType() == Global::DoubleVolume) nvol = 2;
  if (Global::volumeType() == Global::TripleVolume) nvol = 3;
  if (Global::volumeType() == Global::QuadVolume) nvol = 4;  
  Vec vsize = m_volume[0]->getSubvolumeSize();
  for (int v=1; v<nvol; v++)
    vsize = StaticFunctions::maxVec(vsize,
		 m_volume[v]->getSubvolumeSize());

  return vsize;
}

Vec Volume::getDragSubvolumeTextureSize()
{
  if (Global::volumeType() == Global::RGBVolume ||
      Global::volumeType() == Global::RGBAVolume)
    return m_volumeRGB->getDragSubvolumeTextureSize();

  if (Global::volumeType() == Global::SingleVolume ||
      Global::volumeType() == Global::DummyVolume)
    return m_volume[0]->getDragSubvolumeTextureSize();

  int nvol = 2;
  if (Global::volumeType() == Global::DoubleVolume) nvol = 2;
  if (Global::volumeType() == Global::TripleVolume) nvol = 3;
  if (Global::volumeType() == Global::QuadVolume) nvol = 4;
  Vec vsize = m_volume[0]->getDragSubvolumeTextureSize();
  for (int v=1; v<nvol; v++)
    vsize = StaticFunctions::maxVec(vsize,
		 m_volume[v]->getDragSubvolumeTextureSize());

  return vsize;
}

Vec Volume::getSubvolumeTextureSize()
{
  if (Global::volumeType() == Global::RGBVolume ||
      Global::volumeType() == Global::RGBAVolume)
    return m_volumeRGB->getSubvolumeTextureSize();

  if (Global::volumeType() == Global::SingleVolume ||
      Global::volumeType() == Global::DummyVolume)
    return m_volume[0]->getSubvolumeTextureSize();

  int nvol = 2;
  if (Global::volumeType() == Global::DoubleVolume) nvol = 2;
  if (Global::volumeType() == Global::TripleVolume) nvol = 3;
  if (Global::volumeType() == Global::QuadVolume) nvol = 4;
  Vec vsize = m_volume[0]->getSubvolumeTextureSize();
  for (int v=1; v<nvol; v++)
    vsize = StaticFunctions::maxVec(vsize,
		 m_volume[v]->getSubvolumeTextureSize());

  return vsize;
}

int Volume::getSubvolumeSubsamplingLevel()
{
  if (Global::volumeType() == Global::DummyVolume)
    return 1;

  if (Global::volumeType() == Global::RGBVolume ||
      Global::volumeType() == Global::RGBAVolume)
    return m_volumeRGB->getSubvolumeSubsamplingLevel();

  return m_volume[0]->getSubvolumeSubsamplingLevel();
}

int Volume::getDragSubvolumeSubsamplingLevel()
{
  if (Global::volumeType() == Global::DummyVolume)
    return 1;

  if (Global::volumeType() == Global::RGBVolume ||
      Global::volumeType() == Global::RGBAVolume)
    return m_volumeRGB ?
      m_volumeRGB->getDragSubvolumeSubsamplingLevel() : 1;

  return m_volume.isEmpty() ?
    1 : m_volume[0]->getDragSubvolumeSubsamplingLevel();
}

Vec Volume::getFullVolumeSize()
{
  if (Global::volumeType() == Global::RGBVolume ||
      Global::volumeType() == Global::RGBAVolume)
    return m_volumeRGB->getFullVolumeSize();

  if (Global::volumeType() == Global::SingleVolume ||
      Global::volumeType() == Global::DummyVolume)
   return m_volume[0]->getFullVolumeSize();



  int nvol = 2;
  if (Global::volumeType() == Global::DoubleVolume) nvol = 2;
  if (Global::volumeType() == Global::TripleVolume) nvol = 3;
  if (Global::volumeType() == Global::QuadVolume) nvol = 4;
  Vec vsize = m_volume[0]->getFullVolumeSize();
  for (int v=1; v<nvol; v++)
    vsize = StaticFunctions::maxVec(vsize,
		 m_volume[v]->getFullVolumeSize());

  vsize *= m_bbScale;
  
  return vsize;
}


//--------------------
// for array textures
//--------------------
uchar* Volume::getDragSubvolumeTexture()
{
  if (Global::volumeType() == Global::DummyVolume)
    return NULL;

  // single volume
  if (Global::volumeType() == Global::SingleVolume)
    return m_volume[0]->getDragSubvolumeTexture();

  // rgb volume
  if (Global::volumeType() == Global::RGBVolume ||
      Global::volumeType() == Global::RGBAVolume)
    return m_volumeRGB->getDragSubvolumeTexture();

  // multiple volumes
  int nvol = 0;
  if (Global::volumeType() == Global::DoubleVolume) nvol = 2;
  if (Global::volumeType() == Global::TripleVolume) nvol = 3;
  if (Global::volumeType() == Global::QuadVolume) nvol = 4;

  delete [] m_dragSubvolumeTexture;
  m_dragSubvolumeTexture = 0;

  if (nvol < 1 || m_volume.count() < nvol || !m_volume[0])
    return 0;

  const Vec vsize = m_volume[0]->getDragSubvolumeTextureSize();
  const qint64 nx = static_cast<qint64>(vsize.x);
  const qint64 ny = static_cast<qint64>(vsize.y);
  const qint64 nz = static_cast<qint64>(vsize.z);
  const int bpv = m_volume[0]->pvlVoxelType() > 0 ? 2 : 1;

  QList<uchar*> channelTextures;
  for (int v=0; v<nvol; v++)
    {
      const Vec channelSize = m_volume[v]->getDragSubvolumeTextureSize();
      const int channelBpv = m_volume[v]->pvlVoxelType() > 0 ? 2 : 1;
      uchar *tex = m_volume[v]->getDragSubvolumeTexture();
      if (!tex || channelBpv != bpv ||
          static_cast<qint64>(channelSize.x) != nx ||
          static_cast<qint64>(channelSize.y) != ny ||
          static_cast<qint64>(channelSize.z) != nz)
        return 0;
      channelTextures.append(tex);
    }

  qint64 voxelCount = 1;
  if (!checkedSizeFactor(nx, voxelCount) ||
      !checkedSizeFactor(ny, voxelCount) ||
      !checkedSizeFactor(nz, voxelCount))
    return 0;

  qint64 textureBytes = voxelCount;
  if (!checkedSizeFactor(nvol, textureBytes) ||
      !checkedSizeFactor(bpv, textureBytes) ||
      !validAllocationSize(textureBytes))
    return 0;

  m_dragSubvolumeTexture =
    new (std::nothrow) uchar[static_cast<size_t>(textureBytes)];
  if (!m_dragSubvolumeTexture)
    return 0;
  memset(m_dragSubvolumeTexture, 0, static_cast<size_t>(textureBytes));

  for (int v=0; v<nvol; v++)
    {
      const uchar *tex = channelTextures[v];
      for (qint64 i=0; i<voxelCount; i++)
        memcpy(m_dragSubvolumeTexture + bpv*(i*nvol+v),
               tex + bpv*i,
               static_cast<size_t>(bpv));
    }
  
  return m_dragSubvolumeTexture;
}

uchar*
Volume::getSubvolumeTexture()
{
  if (Global::volumeType() == Global::DummyVolume)
    return 0;

  // single volume
  if (Global::volumeType() == Global::SingleVolume)
    return m_volume.isEmpty() || !m_volume[0] ?
      0 : m_volume[0]->getSubvolume();

  // rgb volume
  if (Global::volumeType() == Global::RGBVolume ||
      Global::volumeType() == Global::RGBAVolume)
    return m_volumeRGB ? m_volumeRGB->getSubvolume() : 0;

  // multiple volumes
  int nvol = 0;
  if (Global::volumeType() == Global::DoubleVolume) nvol = 2;
  if (Global::volumeType() == Global::TripleVolume) nvol = 3;
  if (Global::volumeType() == Global::QuadVolume) nvol = 4;

  delete [] m_subvolumeTexture;
  m_subvolumeTexture = 0;
  m_slabLayerCapacity = 0;

  if (nvol < 1 || m_volume.count() < nvol || !m_volume[0])
    return 0;

  const Vec vsize = m_volume[0]->getSubvolumeTextureSize();
  const qint64 nx = static_cast<qint64>(vsize.x);
  const qint64 ny = static_cast<qint64>(vsize.y);
  const qint64 nz = static_cast<qint64>(vsize.z);
  const int bpv = m_volume[0]->pvlVoxelType() > 0 ? 2 : 1;

  QList<uchar*> channelTextures;
  for (int v=0; v<nvol; ++v)
    {
      if (!m_volume[v])
        return 0;
      const Vec channelSize = m_volume[v]->getSubvolumeTextureSize();
      const int channelBpv = m_volume[v]->pvlVoxelType() > 0 ? 2 : 1;
      uchar *texture = m_volume[v]->getSubvolume();
      if (!texture || channelBpv != bpv ||
          static_cast<qint64>(channelSize.x) != nx ||
          static_cast<qint64>(channelSize.y) != ny ||
          static_cast<qint64>(channelSize.z) != nz)
        return 0;
      channelTextures.append(texture);
    }

  qint64 voxelCount = 1;
  if (!checkedSizeFactor(nx, voxelCount) ||
      !checkedSizeFactor(ny, voxelCount) ||
      !checkedSizeFactor(nz, voxelCount))
    return 0;

  qint64 textureBytes = voxelCount;
  if (!checkedSizeFactor(nvol, textureBytes) ||
      !checkedSizeFactor(bpv, textureBytes) ||
      !validAllocationSize(textureBytes))
    return 0;

  m_subvolumeTexture =
    new (std::nothrow) uchar[static_cast<size_t>(textureBytes)];
  if (!m_subvolumeTexture)
    return 0;
  memset(m_subvolumeTexture, 0, static_cast<size_t>(textureBytes));

  for (int v=0; v<nvol; v++)
    {
      const uchar *texture = channelTextures[v];
      for (qint64 i=0; i<voxelCount; ++i)
        memcpy(m_subvolumeTexture + bpv*(i*nvol+v),
               texture + bpv*i,
               static_cast<size_t>(bpv));
    }
  
  return m_subvolumeTexture;
}

bool
Volume::allocSlabs(int layerCapacity)
{
  const int volumeType = Global::volumeType();
  int nvol = 0;
  if (volumeType == Global::SingleVolume) nvol = 1;
  if (volumeType == Global::DoubleVolume) nvol = 2;
  if (volumeType == Global::TripleVolume) nvol = 3;
  if (volumeType == Global::QuadVolume) nvol = 4;

  if (m_subvolumeTexture) delete [] m_subvolumeTexture;
  m_subvolumeTexture = 0;
  m_slabLayerCapacity = 0;

  if (m_channelSlabTexture) delete [] m_channelSlabTexture;
  m_channelSlabTexture = 0;
  m_channelSlabBytes = 0;

  if (layerCapacity <= 0)
    {
      for (int v=0; v<m_volume.count(); v++)
        m_volume[v]->deleteTextureSlab();
      return false;
    }

  if (volumeType == Global::RGBVolume || volumeType == Global::RGBAVolume)
    return m_volumeRGB && m_volumeRGB->allocSlabs(layerCapacity);

  if (nvol == 0 || m_volume.count() < nvol)
    return false;

  for (int v=0; v<nvol; v++)
    {
      if (!m_volume[v]->allocSlabs(layerCapacity, nvol == 1))
        {
          for (int u=0; u<nvol; u++)
            m_volume[u]->deleteTextureSlab();
          return false;
        }
    }
  
  Vec vsize;
  vsize = m_volume[0]->getSubvolumeTextureSize();

  qint64 nx,ny;
  nx = vsize.x;
  ny = vsize.y;

  if (nx <= 0 || ny <= 0)
    {
      for (int u=0; u<nvol; u++)
        m_volume[u]->deleteTextureSlab();
      return false;
    }

  int bpv = 1;
  if (m_volume[0]->pvlVoxelType() > 0)
    bpv = 2;

  for (int v=1; v<nvol; v++)
    {
      const Vec channelSize = m_volume[v]->getSubvolumeTextureSize();
      const int channelBpv = m_volume[v]->pvlVoxelType() > 0 ? 2 : 1;
      if (static_cast<qint64>(channelSize.x) != nx ||
          static_cast<qint64>(channelSize.y) != ny ||
          channelBpv != bpv)
        {
          for (int u=0; u<nvol; u++)
            m_volume[u]->deleteTextureSlab();
          return false;
        }
    }

  if (nvol == 1)
    return true;

  qint64 channelBytes = 1;
  if (!checkedSizeFactor(bpv, channelBytes) ||
      !checkedSizeFactor(nx, channelBytes) ||
      !checkedSizeFactor(ny, channelBytes) ||
      !checkedSizeFactor(layerCapacity, channelBytes) ||
      !validAllocationSize(channelBytes))
    {
      for (int u=0; u<nvol; u++)
        m_volume[u]->deleteTextureSlab();
      return false;
    }

  qint64 slabBytes = channelBytes;
  if (!checkedSizeFactor(nvol, slabBytes) ||
      !validAllocationSize(slabBytes))
    {
      for (int u=0; u<nvol; u++)
        m_volume[u]->deleteTextureSlab();
      return false;
    }

  m_subvolumeTexture =
    new (std::nothrow) uchar[static_cast<size_t>(slabBytes)];
  m_channelSlabTexture =
    new (std::nothrow) uchar[static_cast<size_t>(channelBytes)];
  if (!m_subvolumeTexture || !m_channelSlabTexture)
    {
      delete [] m_subvolumeTexture;
      delete [] m_channelSlabTexture;
      m_subvolumeTexture = 0;
      m_channelSlabTexture = 0;
      for (int u=0; u<nvol; u++)
        m_volume[u]->deleteTextureSlab();
      return false;
    }

  memset(m_subvolumeTexture, 0, static_cast<size_t>(slabBytes));
  memset(m_channelSlabTexture, 0, static_cast<size_t>(channelBytes));
  m_channelSlabBytes = channelBytes;
  m_slabLayerCapacity = layerCapacity;

  return true;
}

uchar*
Volume::getSubvolumeTextureSlab(int startZSlice, int endZSlice,
			       int layerCount)
{
  if (Global::volumeType() == Global::DummyVolume || layerCount <= 0)
    return 0;
  
  // single volume
  if (Global::volumeType() == Global::SingleVolume)
    return m_volume.count() > 0 ?
      m_volume[0]->getSlab(startZSlice, endZSlice, layerCount) : 0;

  // rgb volume
  if (Global::volumeType() == Global::RGBVolume ||
      Global::volumeType() == Global::RGBAVolume)
    return m_volumeRGB ?
      m_volumeRGB->getSlab(startZSlice, endZSlice, layerCount) : 0;

  // multiple volumes
  int nvol = 0;
  if (Global::volumeType() == Global::DoubleVolume) nvol = 2;
  if (Global::volumeType() == Global::TripleVolume) nvol = 3;
  if (Global::volumeType() == Global::QuadVolume) nvol = 4;

  if (nvol < 1 || m_volume.count() < nvol ||
      !m_subvolumeTexture || !m_channelSlabTexture ||
      m_channelSlabBytes <= 0 || layerCount > m_slabLayerCapacity)
    return 0;

  Vec vsize;
  vsize = m_volume[0]->getSubvolumeTextureSize();
  qint64 nx,ny;
  nx = vsize.x;
  ny = vsize.y;

  int bpv = 1;
  if (m_volume[0]->pvlVoxelType() > 0)
    bpv = 2;
  
  qint64 voxelCount = 1;
  if (!checkedSizeFactor(nx, voxelCount) ||
      !checkedSizeFactor(ny, voxelCount) ||
      !checkedSizeFactor(layerCount, voxelCount))
    return 0;

  if (bpv == 1)
    {
      for (int v=0; v<nvol; v++)
	{
	  if (!m_volume[v]->fillSlab(startZSlice, endZSlice, layerCount,
				     m_channelSlabTexture,
				     m_channelSlabBytes))
            return 0;
	  for (qint64 i=0; i<voxelCount; i++)
	    m_subvolumeTexture[i*nvol+v] = m_channelSlabTexture[i];
	}
    }
  else // for 16-bit data
    {
      for (int v=0; v<nvol; v++)
	{
	  if (!m_volume[v]->fillSlab(startZSlice, endZSlice, layerCount,
				     m_channelSlabTexture,
				     m_channelSlabBytes))
            return 0;
	  for (qint64 i=0; i<voxelCount; i++)
	    ((ushort*)m_subvolumeTexture)[i*nvol+v] =
              ((ushort*)m_channelSlabTexture)[i];
	}
    }
  
  return m_subvolumeTexture;
}
//--------------------


Vec Volume::getLowresVolumeSize()
{
  if (Global::volumeType() == Global::RGBVolume ||
      Global::volumeType() == Global::RGBAVolume)
    return m_volumeRGB->getLowresVolumeSize();

  if (Global::volumeType() == Global::SingleVolume ||
      Global::volumeType() == Global::DummyVolume)
    return m_volume[0]->getLowresVolumeSize();


  int nvol = 1;
  if (Global::volumeType() == Global::DoubleVolume) nvol = 2;
  if (Global::volumeType() == Global::TripleVolume) nvol = 3;
  if (Global::volumeType() == Global::QuadVolume) nvol = 4;

  Vec vsize = m_volume[0]->getLowresVolumeSize();
  for (int v=1; v<nvol; v++)
    vsize = StaticFunctions::maxVec(vsize,
		 m_volume[v]->getLowresVolumeSize());

  return vsize;
}

Vec Volume::getLowresTextureVolumeSize()
{
  if (Global::volumeType() == Global::RGBVolume ||
      Global::volumeType() == Global::RGBAVolume)
    return m_volumeRGB->getLowresTextureVolumeSize();

  if (Global::volumeType() == Global::SingleVolume ||
      Global::volumeType() == Global::DummyVolume)
    return m_volume[0]->getLowresTextureVolumeSize();


  int nvol = 1;
  if (Global::volumeType() == Global::DoubleVolume) nvol = 2;
  if (Global::volumeType() == Global::TripleVolume) nvol = 3;
  if (Global::volumeType() == Global::QuadVolume) nvol = 4;

  Vec vsize = m_volume[0]->getLowresTextureVolumeSize();
  for (int v=1; v<nvol; v++)
    vsize = StaticFunctions::maxVec(vsize,
		 m_volume[v]->getLowresTextureVolumeSize());

  return vsize;
}

int* Volume::getLowres1dHistogram()
{
  return getLowres1dHistogram(0);
}

int* Volume::getLowres2dHistogram()
{
  return getLowres2dHistogram(0);
}

uchar* Volume::getLowresTextureVolume()
{
  if (Global::volumeType() == Global::DummyVolume)
    return 0;
  else if (Global::volumeType() == Global::RGBVolume ||
	   Global::volumeType() == Global::RGBAVolume)
    return m_volumeRGB ? m_volumeRGB->getLowresTextureVolume() : 0;
  else if (Global::volumeType() == Global::SingleVolume)
    return m_volume.isEmpty() || !m_volume[0] ?
      0 : m_volume[0]->getLowresTextureVolume();


  int nvol = 1;
  if (Global::volumeType() == Global::DoubleVolume) nvol = 2;
  if (Global::volumeType() == Global::TripleVolume) nvol = 3;
  if (Global::volumeType() == Global::QuadVolume) nvol = 4;

  if (nvol < 1 || m_volume.count() < nvol)
    return 0;
  for (int v=0; v<nvol; ++v)
    if (!m_volume[v])
      return 0;

  Vec texSize = getLowresTextureVolumeSize();
  qint64 nsubX = texSize.x;
  qint64 nsubY = texSize.y;
  qint64 nsubZ = texSize.z;

  Vec glowvol = getLowresVolumeSize();

  int bpv = 1;
  if (m_volume[0]->pvlVoxelType() > 0) bpv = 2;

  delete [] m_lowresTexture;
  m_lowresTexture = 0;

  const qint64 lowresDepth = static_cast<qint64>(glowvol.z);
  if (lowresDepth <= 0 || lowresDepth > nsubZ)
    return 0;

  QList<uchar*> channelTextures;
  QList<Vec> channelTextureSizes;
  QList<Vec> channelVolumeSizes;
  for (int v=0; v<nvol; ++v)
    {
      if (!m_volume[v])
        return 0;
      const int channelBpv = m_volume[v]->pvlVoxelType() > 0 ? 2 : 1;
      uchar *texture = m_volume[v]->getLowresTextureVolume();
      const Vec textureSize = m_volume[v]->getLowresTextureVolumeSize();
      const Vec volumeSize = m_volume[v]->getLowresVolumeSize();
      const qint64 textureWidth = static_cast<qint64>(textureSize.x);
      const qint64 textureHeight = static_cast<qint64>(textureSize.y);
      const qint64 textureDepth = static_cast<qint64>(textureSize.z);
      const qint64 volumeDepth = static_cast<qint64>(volumeSize.z);
      if (!texture || channelBpv != bpv ||
          textureWidth <= 0 || textureWidth > nsubX ||
          textureHeight <= 0 || textureHeight > nsubY ||
          textureDepth <= 0 || textureDepth > nsubZ ||
          volumeDepth <= 0 || volumeDepth > textureDepth ||
          volumeDepth > lowresDepth)
        return 0;
      channelTextures.append(texture);
      channelTextureSizes.append(textureSize);
      channelVolumeSizes.append(volumeSize);
    }

  qint64 voxelCount = 1;
  if (!checkedSizeFactor(nsubX, voxelCount) ||
      !checkedSizeFactor(nsubY, voxelCount) ||
      !checkedSizeFactor(nsubZ, voxelCount))
    return 0;

  qint64 textureBytes = voxelCount;
  if (!checkedSizeFactor(bpv, textureBytes) ||
      !checkedSizeFactor(nvol, textureBytes) ||
      !validAllocationSize(textureBytes))
    return 0;

  m_lowresTexture =
    new (std::nothrow) uchar[static_cast<size_t>(textureBytes)];
  if (!m_lowresTexture)
    return 0;
  memset(m_lowresTexture, 0, static_cast<size_t>(textureBytes));

  if (bpv == 1)
    {
      for(int v=0; v<nvol; v++)
	{
	  const uchar *tex = channelTextures[v];
	  const qint64 textureWidth =
	    static_cast<qint64>(channelTextureSizes[v].x);
	  const qint64 textureHeight =
	    static_cast<qint64>(channelTextureSizes[v].y);
	  const qint64 volumeDepth =
	    static_cast<qint64>(channelVolumeSizes[v].z);
	  const qint64 offX = (nsubX-textureWidth)/2;
	  const qint64 offY = (nsubY-textureHeight)/2;
	  const qint64 offZ = (lowresDepth-volumeDepth)/2;
	  for(qint64 z=0; z<volumeDepth; z++)
	    for(qint64 y=0; y<textureHeight; y++)
	      for(qint64 x=0; x<textureWidth; x++)
		{
		  qint64 idx = (z+offZ)*nsubY*nsubX + (y+offY)*nsubX + (x+offX);
		  qint64 tdx = z*textureHeight*textureWidth +
		                y*textureWidth + x;
		  
		  m_lowresTexture[nvol*idx+v] = tex[tdx];
		}
	}
    }
  else // for 16-bit data
    {
      for(int v=0; v<nvol; v++)
	{
	  const ushort *tex = reinterpret_cast<const ushort*>(
	    channelTextures[v]);
	  const qint64 textureWidth =
	    static_cast<qint64>(channelTextureSizes[v].x);
	  const qint64 textureHeight =
	    static_cast<qint64>(channelTextureSizes[v].y);
	  const qint64 volumeDepth =
	    static_cast<qint64>(channelVolumeSizes[v].z);
	  const qint64 offX = (nsubX-textureWidth)/2;
	  const qint64 offY = (nsubY-textureHeight)/2;
	  const qint64 offZ = (lowresDepth-volumeDepth)/2;
	  for(qint64 z=0; z<volumeDepth; z++)
	    for(qint64 y=0; y<textureHeight; y++)
	      for(qint64 x=0; x<textureWidth; x++)
		{
		  qint64 idx = (z+offZ)*nsubY*nsubX + (y+offY)*nsubX + (x+offX);
		  qint64 tdx = z*textureHeight*textureWidth +
		                y*textureWidth + x;
		  
		  reinterpret_cast<ushort*>(m_lowresTexture)[nvol*idx+v] =
		    tex[tdx];
		}
	}
    }


  return m_lowresTexture;
}

void
Volume::getSurfaceArea(uchar *lut,
		       QList<Vec> clipPos,
		       QList<Vec> clipNormal,
		       QList<CropObject> crops,
		       QList<PathObject> paths)
{
  if (Global::volumeType() == Global::SingleVolume)
    m_volume[0]->getSurfaceArea(lut,
				clipPos, clipNormal,
				crops,
				paths);
  else
    QMessageBox::critical(0, "Error",
			  "Surface Area calculations possible only for single volumes");
}

void
Volume::saveVolume(uchar *lut,
		   QList<Vec> clipPos,
		   QList<Vec> clipNormal,
		   QList<CropObject> crops,
		   QList<PathObject> paths)
{
  if (Global::volumeType() == Global::SingleVolume)
    m_volume[0]->saveVolume(lut,
			    clipPos, clipNormal,
			    crops,
			    paths);
  else if (Global::volumeType() == Global::RGBVolume ||
	   Global::volumeType() == Global::RGBAVolume)
    m_volumeRGB->saveOpacityVolume(lut,
				   clipPos, clipNormal,
				   crops);
  else
    QMessageBox::critical(0, "Error",
			  "Save Opacity Volume possible only for single volumes");
}

void
Volume::maskRawVolume(uchar *lut,
		      QList<Vec> clipPos,
		      QList<Vec> clipNormal,
		      QList<CropObject> crops,
		      QList<PathObject> paths)
{
  if (Global::volumeType() == Global::SingleVolume)
      m_volume[0]->maskRawVolume(lut,
				 clipPos, clipNormal,
				 crops,
				 paths);
  else if (Global::volumeType() == Global::RGBVolume ||
	   Global::volumeType() == Global::RGBAVolume)
    m_volumeRGB->maskRawVolume(lut,
			       clipPos, clipNormal,
			       crops);
  else
    QMessageBox::critical(0, "Error",
			  "Save masked raw volume possible only for single volumes");
}

void
Volume::countIsolatedRegions(uchar *lut,
			     QList<Vec> clipPos,
			     QList<Vec> clipNormal,
			     QList<CropObject> crops,
			     QList<PathObject> paths)
{
  if (Global::volumeType() == Global::SingleVolume)
    m_volume[0]->countIsolatedRegions(lut,
				      clipPos, clipNormal,
				      crops,
				      paths);
  else
    QMessageBox::critical(0, "Error",
			  "Save masked raw volume possible only for single volumes");
}

QBitArray
Volume::getBitmask(uchar *lut,
		   QList<Vec> clipPos,
		   QList<Vec> clipNormal,
		   QList<CropObject> crops,
		   QList<PathObject> paths)
{
  return m_volume[0]->getBitmask(lut,
				 clipPos, clipNormal,
				 crops,
				 paths);
}

void
Volume::saveSliceImage(Vec pos,
		       Vec normal, Vec xaxis, Vec yaxis,
		       float scalex, float scaley,
		       int step)
{
  if (Global::volumeType() == Global::SingleVolume)
    m_volume[0]->saveSliceImage(pos, normal, xaxis, yaxis, scalex, scaley, step);
  else
    QMessageBox::critical(0, "Error",
			  "Save slice image possible only for single volumes");
}

void
Volume::resliceVolume(Vec pos,
		      Vec normal, Vec xaxis, Vec yaxis,
		      float scalex, float scaley,
		      int step1, int step2)
{
  if (Global::volumeType() == Global::SingleVolume)
    m_volume[0]->resliceVolume(pos, normal, xaxis, yaxis, scalex, scaley, step1, step2);
  else
    QMessageBox::critical(0, "Error",
			  "Reslicing possible only for single volumes");
}
