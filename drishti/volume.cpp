#include "global.h"

#include "volume.h"
#include "staticfunctions.h"
#include "xmlheaderfunctions.h"


bool Volume::valid() { return (m_volume.count() > 0); }

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

uchar* Volume::getDragTexture()
{
  if (Global::volumeType() == Global::DummyVolume)
    return NULL;

  if (Global::volumeType() == Global::SingleVolume)
    return m_volume[0]->getDragTexture();

  if (Global::volumeType() == Global::RGBVolume ||
      Global::volumeType() == Global::RGBAVolume)
    return m_volumeRGB->getDragTexture();

  int nvol = 2;
  if (Global::volumeType() == Global::DoubleVolume) nvol = 2;
  if (Global::volumeType() == Global::TripleVolume) nvol = 3;
  if (Global::volumeType() == Global::QuadVolume) nvol = 4;

  int texWidth, texHeight;
  m_volume[0]->getDragTextureSize(texWidth, texHeight);
	  
  if (m_dragTexture) delete [] m_dragTexture;
  m_dragTexture = new uchar[nvol*texWidth*texHeight];
  memset(m_dragTexture, 0, nvol*texWidth*texHeight);
  
  for (int v=0; v<nvol; v++)
    {
      uchar *tex = m_volume[v]->getDragTexture();
      for (int i=0; i<texWidth*texHeight; i++)
	m_dragTexture[i*nvol+v] = tex[i];
    }
  
  return m_dragTexture;
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
      int tms = Global::textureMemorySize(); // in Mb
      int svsl = StaticFunctions::getSubsamplingLevel(tms,
						      Global::textureSizeLimit(),
						      bpv,
						      dataMin, dataMax);

      int nrows, ncols;
      QList<Vec> slabinfo = Global::getSlabs(svsl,
					     dataMin, dataMax,
					     nrows, ncols);

      
      int lenx = dataMax.x-dataMin.x+1;
      int leny = dataMax.y-dataMin.y+1;
      int lenx2 = lenx/svsl;
      int leny2 = leny/svsl;
      int texWidth = ncols*lenx2;
      int texHeight = nrows*leny2;

      Vec draginfo = Global::getDragInfo(dataMin, dataMax, 1);

      int dlenx2 = lenx/int(draginfo.z);
      int dleny2 = leny/int(draginfo.z);
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
  if (Global::volumeType() == Global::DummyVolume)
    return;

  if (m_subvolumeTexture) delete [] m_subvolumeTexture;
  m_subvolumeTexture = 0;

  if (m_dragSubvolumeTexture) delete [] m_dragSubvolumeTexture;
  m_dragSubvolumeTexture = 0;

  if (Global::volumeType() == Global::RGBVolume ||
      Global::volumeType() == Global::RGBAVolume)
    {
      m_volumeRGB->deleteTextureSlab();
      return;
    }
}

uchar*
Volume::getSliceTextureSlab(int minz, int maxz)
{
  if (Global::volumeType() == Global::DummyVolume)
    return NULL;

  if (Global::volumeType() == Global::RGBVolume ||
	   Global::volumeType() == Global::RGBAVolume)
    return m_volumeRGB->getSliceTextureSlab(minz, maxz);
}


void Volume::forceCreateLowresVolume()
{
  if (!valid())
    return;

  if (Global::volumeType() == Global::DummyVolume)
    return;

  if (Global::volumeType() == Global::RGBVolume ||
      Global::volumeType() == Global::RGBAVolume)
    {
      m_volumeRGB->createLowresTextureVolume();
      return;
    }

  m_volume[0]->createLowresTextureVolume();

  if (Global::volumeType() == Global::DoubleVolume)
    {
      m_volume[1]->createLowresTextureVolume();
    }
  else if (Global::volumeType() == Global::TripleVolume)
    {
      m_volume[1]->createLowresTextureVolume();
      m_volume[2]->createLowresTextureVolume();
    }
  else if (Global::volumeType() == Global::QuadVolume)
    {
      m_volume[1]->createLowresTextureVolume();
      m_volume[2]->createLowresTextureVolume();
      m_volume[3]->createLowresTextureVolume();
    }
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

  m_volumeRGB = new VolumeRGB;
  if (m_volumeRGB->loadVolume(flnm, redo) == false)
    {
      delete m_volumeRGB;
      m_volumeRGB = 0;
      return false;
    }

  return true;
}

bool
Volume::loadDummyVolume(int nx, int ny, int nz)
{
  clearVolumes();

  Global::setVolumeType(Global::DummyVolume);

  VolumeSingle *vol = new VolumeSingle;
  if (vol->loadDummyVolume(nx, ny, nz))
    m_volume.append(vol);
  else
    {
      delete vol;
      return false;
    }

  return true;
}

bool
Volume::loadVolume(QList<QString> vfiles, bool redo)
{
  clearVolumes();

  Global::setVolumeType(Global::SingleVolume);

  VolumeSingle *vol = new VolumeSingle;
  if (vol->loadVolume(vfiles, redo))
    {
      m_volume.append(vol);

      // used for centering smaller volume within larger ones
      Vec fvs = getFullVolumeSize();
      int h = fvs.x;
      int w = fvs.y;
      int d = fvs.z;
      m_volume[0]->setMaxDimensions(h,w,d);
    }
  else
    {
      delete vol;
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

  VolumeSingle *vol0 = new VolumeSingle;
  VolumeSingle *vol1 = new VolumeSingle;
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
	  return false;
	}
      m_volume.append(vol0);
      m_volume.append(vol1);

      // used for centering smaller volume within larger ones
      Vec fvs = getFullVolumeSize();
      int h = fvs.x;
      int w = fvs.y;
      int d = fvs.z;
      m_volume[0]->setMaxDimensions(h,w,d);
      m_volume[1]->setMaxDimensions(h,w,d);
    }
  else
    {
      delete vol0;
      delete vol1;
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

  VolumeSingle *vol0 = new VolumeSingle;
  VolumeSingle *vol1 = new VolumeSingle;
  VolumeSingle *vol2 = new VolumeSingle;
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
	m_volume[v]->setMaxDimensions(h,w,d);
    }
  else
    {
      delete vol0;
      delete vol1;
      delete vol2;
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

  VolumeSingle *vol0 = new VolumeSingle;
  VolumeSingle *vol1 = new VolumeSingle;
  VolumeSingle *vol2 = new VolumeSingle;
  VolumeSingle *vol3 = new VolumeSingle;
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
	m_volume[v]->setMaxDimensions(h,w,d);
    }
  else
    {
      delete vol0;
      delete vol1;
      delete vol2;
      delete vol3;
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
      int bpv = 1;

      int tms = Global::textureMemorySize(); // in Mb
      int sslevel = StaticFunctions::getSubsamplingLevel(tms,
							 Global::textureSizeLimit(),
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
  int tms = Global::textureMemorySize(); // in Mb
  int sslevel = StaticFunctions::getSubsamplingLevel(tms,
						     Global::textureSizeLimit(),
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
  int tms = Global::textureMemorySize(); // in Mb
  int sslevel = StaticFunctions::getSubsamplingLevel(tms,
						     Global::textureSizeLimit(),
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

  int tms = Global::textureMemorySize(); // in Mb
  int sslevel = StaticFunctions::getSubsamplingLevel(tms,
						     Global::textureSizeLimit(),
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
//  if (Global::volumeType() == Global::RGBVolume ||
//      Global::volumeType() == Global::RGBAVolume)
//    return m_volumeRGB->getDragSubvolumeTextureSize();

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
//  if (Global::volumeType() == Global::RGBVolume ||
//      Global::volumeType() == Global::RGBAVolume)
//    return m_volumeRGB->getSubvolumeTextureSize();

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

//  if (Global::volumeType() == Global::RGBVolume ||
//      Global::volumeType() == Global::RGBAVolume)
//    return m_volumeRGB->getDragSubvolumeSubsamplingLevel();

  return m_volume[0]->getDragSubvolumeSubsamplingLevel();
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

//  // rgb volume
//  if (Global::volumeType() == Global::RGBVolume ||
//      Global::volumeType() == Global::RGBAVolume)
//    return m_volumeRGB->getDragSubvolumeTexture();

  // multiple volumes
  int nvol = 0;
  if (Global::volumeType() == Global::DoubleVolume) nvol = 2;
  if (Global::volumeType() == Global::TripleVolume) nvol = 3;
  if (Global::volumeType() == Global::QuadVolume) nvol = 4;

  if (nvol < 1) return 0;

  
  Vec vsize;
  vsize = m_volume[0]->getDragSubvolumeTextureSize();

  int nx,ny,nz;
  nx = vsize.x;
  ny = vsize.y;
  nz = vsize.z;
  if (m_dragSubvolumeTexture) delete [] m_dragSubvolumeTexture;
  m_dragSubvolumeTexture = new uchar[nvol*nx*ny*nz];
  memset(m_dragSubvolumeTexture, 0, nvol*nx*ny*nz);

  for (int v=0; v<nvol; v++)
    {
      uchar *tex = m_volume[v]->getDragSubvolumeTexture();
      for (int i=0; i<nx*ny*nz; i++)
	m_dragSubvolumeTexture[i*nvol+v] = tex[i];
    }
  
  return m_dragSubvolumeTexture;
}

uchar* Volume::getSubvolumeTexture()
{
  if (Global::volumeType() == Global::DummyVolume)
    return 0;

  // single volume
  if (Global::volumeType() == Global::SingleVolume)
    //return m_volume[0]->getSubvolumeTexture();
    return m_volume[0]->getSubvolume();

//  // rgb volume
//  if (Global::volumeType() == Global::RGBVolume ||
//      Global::volumeType() == Global::RGBAVolume)
//    return m_volumeRGB->getSubvolumeTexture();

  // multiple volumes
  int nvol = 0;
  if (Global::volumeType() == Global::DoubleVolume) nvol = 2;
  if (Global::volumeType() == Global::TripleVolume) nvol = 3;
  if (Global::volumeType() == Global::QuadVolume) nvol = 4;

  if (nvol < 1) return 0;

  
  Vec vsize;
  vsize = m_volume[0]->getSubvolumeTextureSize();

  int nx,ny,nz;
  nx = vsize.x;
  ny = vsize.y;
  nz = vsize.z;
  if (m_subvolumeTexture) delete [] m_subvolumeTexture;
  m_subvolumeTexture = new uchar[nvol*nx*ny*nz];
  memset(m_subvolumeTexture, 0, nvol*nx*ny*nz);

  for (int v=0; v<nvol; v++)
    {
      //uchar *tex = m_volume[v]->getSubvolumeTexture();
      uchar *tex = m_volume[v]->getSubvolume();
      for (int i=0; i<nx*ny*nz; i++)
	m_subvolumeTexture[i*nvol+v] = tex[i];
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
    return m_volumeRGB->getLowresTextureVolume();
  else if (Global::volumeType() == Global::SingleVolume)
    return m_volume[0]->getLowresTextureVolume();


  int nvol = 1;
  if (Global::volumeType() == Global::DoubleVolume) nvol = 2;
  if (Global::volumeType() == Global::TripleVolume) nvol = 3;
  if (Global::volumeType() == Global::QuadVolume) nvol = 4;

  Vec texSize = getLowresTextureVolumeSize();
  int nsubX = texSize.x;
  int nsubY = texSize.y;
  int nsubZ = texSize.z;

  Vec glowvol = getLowresVolumeSize();

  if (m_lowresTexture) delete [] m_lowresTexture;
  m_lowresTexture = new uchar[nvol*nsubX*nsubY*nsubZ];
  memset(m_lowresTexture, 0, nvol*nsubX*nsubY*nsubZ);

  int glss = 1;
  for(int v=0; v<nvol; v++)
    glss = qMax(glss, m_volume[v]->getLowresSubsamplingLevel());

  for(int v=0; v<nvol; v++)
    {
      uchar *tex = m_volume[v]->getLowresTextureVolume();
      Vec tSize = m_volume[v]->getLowresTextureVolumeSize();

      int lss = 1;
//      int lss = glss/m_volume[v]->getLowresSubsamplingLevel();

      Vec vlowvol = m_volume[v]->getLowresVolumeSize();
      int offX = (nsubX-tSize.x/lss)/2;
      int offY = (nsubY-tSize.y/lss)/2;
      int offZ = (glowvol.z-vlowvol.z/lss)/2;
      int i=0;
      for(int z=0; z<(int)vlowvol.z/lss; z++)
	for(int y=0; y<(int)tSize.y/lss; y++)
	  for(int x=0; x<(int)tSize.x/lss; x++)
	    {
	      int idx = (z+offZ)*nsubY*nsubX + (y+offY)*nsubX + (x+offX);
	      int tdx = (z*tSize.y*tSize.x + y*tSize.x + x)*lss;

	      m_lowresTexture[nvol*idx+v] = tex[tdx];
	      
	      i++;
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
