#include <stdlib.h>
#include "drawhiresvolume.h"
#include "viewer.h"
#include "volume.h"
#include "staticfunctions.h"
#include "matrix.h"
#include "shaderfactory.h"
#include "shaderfactory2.h"
#include "shaderfactoryrgb.h"
#include "global.h"
#include "geometryobjects.h"
#include "lighthandler.h"
#include "tick.h"
#include "enums.h"
#include "prunehandler.h"
#include "mainwindowui.h"

#include <QInputDialog>
#include <QFileDialog>
#include <QDataStream>

double* DrawHiresVolume::brick0Xform() { return m_bricks->getMatrix(); }

void
DrawHiresVolume::getOpMod(float& front, float& back)
{
  front = m_frontOpMod;
  back = m_backOpMod;
}

void
DrawHiresVolume::setOpMod(float fo, float bo)
{
  m_frontOpMod = fo;
  m_backOpMod = bo;
}

void
DrawHiresVolume::check_MIP()
{
  if (MainWindowUI::mainWindowUI()->actionMIP->isChecked())
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
}

void
DrawHiresVolume::saveImage2Volume(QString pfile)
{
  m_image2VolumeFile = pfile;
  m_saveImage2Volume = true;
}
void DrawHiresVolume::disableSubvolumeUpdates() { m_updateSubvolume = false; }
void DrawHiresVolume::enableSubvolumeUpdates() { m_updateSubvolume = true; }
bool DrawHiresVolume::subvolumeUpdates() { return m_updateSubvolume; }
void DrawHiresVolume::setSubvolumeUpdates(bool s)  { m_updateSubvolume = s; }

int DrawHiresVolume::getSubvolumeSubsamplingLevel()
{ return m_Volume->getSubvolumeSubsamplingLevel(); }

int DrawHiresVolume::getDragSubsamplingLevel()
{
  Vec dragInfo = m_Volume->getDragTextureInfo();
  return dragInfo.z;
}

void DrawHiresVolume::getSliceTextureSize(int& texX, int& texY)
{ m_Volume->getSliceTextureSize(texX, texY); }

void DrawHiresVolume::getDragTextureSize(int& texX, int& texY)
{ m_Volume->getDragTextureSize(texX, texY); }

bool DrawHiresVolume::raised() { return m_showing; }
void DrawHiresVolume::lower() { m_showing = false; }
void DrawHiresVolume::raise()
{
  for(int bno=0; bno<m_polygon.size(); bno++)
    delete m_polygon[bno];
  m_polygon.clear();  

  initShadowBuffers();
  m_showing = true;
}

void DrawHiresVolume::setCurrentVolume(int vnum) { m_currentVolume = vnum; }

void DrawHiresVolume::setBricks(Bricks *bricks) { m_bricks = bricks; }

QImage DrawHiresVolume::histogramImage1D()
{
  if (!Global::useDragVolume())
    return m_histogram1D;
  else
    return m_histogramDrag1D;
}
QImage DrawHiresVolume::histogramImage2D()
{
  if (!Global::useDragVolume())
    return m_histogram2D;
  else
    return m_histogramDrag2D;
}

int* DrawHiresVolume::histogram1D()
{
  if (!Global::useDragVolume())
    return m_Volume->getSubvolume1dHistogram(m_currentVolume);
  else
    return m_Volume->getDrag1dHistogram(m_currentVolume);
}
int* DrawHiresVolume::histogram2D()
{
  if (!Global::useDragVolume())
    return m_Volume->getSubvolume2dHistogram(m_currentVolume);
  else
    return m_Volume->getDrag2dHistogram(m_currentVolume);
}


Vec DrawHiresVolume::volumeSize() { return m_virtualTextureSize; }
Vec DrawHiresVolume::volumeMin() { return m_dataMin; }
Vec DrawHiresVolume::volumeMax() { return m_dataMax; }

LightingInformation DrawHiresVolume::lightInfo() { return m_lightInfo; }
QList<BrickInformation> DrawHiresVolume::bricks() { return m_bricks->bricks(); }

int DrawHiresVolume::renderQuality() { return m_renderQuality; }
void
DrawHiresVolume::setRenderQuality(int rq)
{
  m_renderQuality = rq;
  if (m_renderQuality == Enums::RenderDefault)
    m_bricks->activateBounds();
  else
    m_bricks->deactivateBounds();


  if (m_renderQuality == Enums::RenderDefault)
    MainWindowUI::mainWindowUI()->actionShadowRender->setChecked(false);
  else
    MainWindowUI::mainWindowUI()->actionShadowRender->setChecked(true);
}

void
DrawHiresVolume::loadVolume()
{
  PruneHandler::clean();
  PruneHandler::createPruneShader((m_Volume->pvlVoxelType(0) > 0));
  PruneHandler::createMopShaders();
  
  LightHandler::createOpacityShader((m_Volume->pvlVoxelType(0) > 0));
  LightHandler::createLightShaders();

  createShaders();

  m_bricks->reset();
  GeometryObjects::clipplanes()->reset();
}

DrawHiresVolume::DrawHiresVolume(Viewer *viewer,
				 Volume *volume) :
  QObject()
{
  m_Viewer = viewer;
  m_Volume = volume;

  m_histData1D = 0;
  m_histData2D = 0;
  m_histDragData1D = 0;
  m_histDragData2D = 0;

  m_dataTexSize = 0;
  m_dataTex = 0;
  m_lutTex = 0;
  m_textureSlab.clear();

  m_shadowWidth = 0;
  m_shadowHeight = 0;
  m_shadowBuffer = 0;
  m_blurredBuffer = 0;

  m_lutShader=0;
  m_passthruShader=0;
  m_defaultShader=0;
  m_blurShader=0;
  m_backplaneShader1=0;
  m_backplaneShader2=0;

  m_useScreenShadows = false;
  m_shadowLod = 1;
  m_shdBuffer = 0;
  m_shdTex[0] = 0;
  m_shdTex[1] = 0;
  m_shdNum = 0;

  m_dofBuffer = 0;
  m_dofTex[0] = 0;
  m_dofTex[1] = 0;
  m_dofTex[2] = 0;

  renew();
}

DrawHiresVolume::~DrawHiresVolume()
{
  cleanup();
}

void
DrawHiresVolume::renew()
{
  m_focalPoint = 0.0;
  m_dofBlur = 0;

  m_frontOpMod = 1.0;
  m_backOpMod = 1.0;
  m_saveImage2Volume = false;

  m_showing = true;
  m_forceBackToFront = false;

  m_loadingData = false;

  m_currentVolume = 0;

  m_virtualTextureSize = m_virtualTextureMin = m_virtualTextureMax = Vec(0,0,0);
  m_dataMin = m_dataMax = m_dataSize = Vec(0,0,0);

  if (m_histData1D) delete [] m_histData1D;
  if (m_histData2D) delete [] m_histData2D;
  if (m_histDragData1D) delete [] m_histDragData1D;
  if (m_histDragData2D) delete [] m_histDragData2D;
  m_histData1D = new unsigned char[256*256*4];
  m_histData2D = new unsigned char[256*256*4];
  m_histDragData1D = new unsigned char[256*256*4];
  m_histDragData2D = new unsigned char[256*256*4];
  m_histogram1D = QImage(256, 256, QImage::Format_RGB32);
  m_histogram2D = QImage(256, 256, QImage::Format_RGB32);
  m_histogramDrag1D = QImage(256, 256, QImage::Format_RGB32);
  m_histogramDrag2D = QImage(256, 256, QImage::Format_RGB32);

  for(int bno=0; bno<m_polygon.size(); bno++)
    delete m_polygon[bno];
  m_polygon.clear();
  
  m_dataTexSize = 0;
  m_dataTex = 0;
  m_lutTex = 0;
  m_textureSlab.clear();

  m_backlit = false;  

  m_shadowWidth = 0;
  m_shadowHeight = 0;
  m_shadowBuffer = 0;
  m_blurredBuffer = 0;

  m_imgSizeRatio = 1;

  m_renderQuality = Enums::RenderDefault;

  m_crops.clear();
  m_paths.clear();

  m_updateSubvolume = true;

  m_mixvol = 0;
  m_mixColor = false;
  m_mixOpacity = false;

  m_interpolateVolumes = 0;
  m_interpVol = 0.0f;
  m_mixTag = false;

  Global::setUpdatePruneTexture(true);

  if (m_shdBuffer) glDeleteFramebuffers(1, &m_shdBuffer);
  if (m_shdTex[0]) glDeleteTextures(2, m_shdTex);  
  m_shdBuffer = 0;
  m_shdTex[0] = m_shdTex[1] = 0;
  m_shdNum = 0;

  if (m_dofBuffer) glDeleteFramebuffers(1, &m_dofBuffer);
  if (m_dofTex[0]) glDeleteTextures(3, m_dofTex);
  m_dofBuffer = 0;
  m_dofTex[0] = m_dofTex[1] = m_dofTex[2] = 0;

}

void
DrawHiresVolume::cleanup()
{
  if (m_dofBuffer) glDeleteFramebuffers(1, &m_dofBuffer);
  if (m_dofTex[0]) glDeleteTextures(3, m_dofTex);
  m_dofBuffer = 0;
  m_dofTex[0] = m_dofTex[1] = m_dofTex[2] = 0;

  if (m_shdBuffer) glDeleteFramebuffers(1, &m_shdBuffer);
  if (m_shdTex[0]) glDeleteTextures(2, m_shdTex);  
  m_shdBuffer = 0;
  m_shdTex[0] = m_shdTex[1] = 0;
  m_shdNum = 0;

  if (m_histData1D) delete [] m_histData1D;
  if (m_histData2D) delete [] m_histData2D;
  if (m_histDragData1D) delete [] m_histDragData1D;
  if (m_histDragData2D) delete [] m_histDragData2D;
  m_histData1D = 0;
  m_histData2D = 0;
  m_histDragData1D = 0;
  m_histDragData2D = 0;

  if (m_shadowBuffer)  delete m_shadowBuffer;
  if (m_blurredBuffer) delete m_blurredBuffer;
  m_shadowBuffer = 0;
  m_blurredBuffer = 0;
  
  m_virtualTextureSize = m_virtualTextureMin = m_virtualTextureMax = Vec(0,0,0);
  m_dataMin = m_dataMax = m_dataSize = Vec(0,0,0);

  if (m_dataTexSize > 0)
    {
      glDeleteTextures(m_dataTexSize, m_dataTex);
      delete [] m_dataTex;
    }
  if (m_lutTex) glDeleteTextures(1, &m_lutTex);

  m_lutTex = 0;
  m_dataTex = 0;
  m_dataTexSize = 0;

  m_crops.clear();
  m_paths.clear();
}

void
DrawHiresVolume::generateHistogramImage()
{
  if (m_Volume->pvlVoxelType(0) > 0)
    return;

  if (Global::volumeType() == Global::DummyVolume)
    return;

  int *hist2D = m_Volume->getSubvolume2dHistogram(m_currentVolume);
  if (m_Volume->pvlVoxelType(0) == 0)
    {
      for (int i=0; i<256*256; i++)
	{
	  m_histData2D[4*i + 3] = 255;
	  m_histData2D[4*i + 0] = hist2D[i];
	  m_histData2D[4*i + 1] = hist2D[i];
	  m_histData2D[4*i + 2] = hist2D[i];
	}
      m_histogram2D = QImage(m_histData2D, 256, 256, QImage::Format_ARGB32);
      m_histogram2D = m_histogram2D.mirrored();  
    }
  else
    {
      for (int i=0; i<256*256; i++)
	{
	  
	  m_histData2D[4*i + 3] = 255;
	  m_histData2D[4*i + 0] = hist2D[i];
	  m_histData2D[4*i + 1] = hist2D[i];
	  m_histData2D[4*i + 2] = hist2D[i];
	}
      m_histogram2D = QImage(m_histData2D, 256, 256, QImage::Format_RGB32);
    }

  int *hist1D = m_Volume->getSubvolume1dHistogram(m_currentVolume);
  memset(m_histData1D, 0, 4*256*256);
  for (int i=0; i<256; i++)
    {
      for (int j=0; j<256; j++)
	{
	  int idx = 256*j + i;
	  m_histData1D[4*idx + 3] = 255;
	}

      int h = hist1D[i];
      for (int j=0; j<h; j++)
	{
	  int idx = 256*j + i;
	  m_histData1D[4*idx + 0] = 255*j/h;
	  m_histData1D[4*idx + 1] = 255*j/h;
	  m_histData1D[4*idx + 2] = 255*j/h;
	}
    }
  m_histogram1D = QImage(m_histData1D, 256, 256, QImage::Format_ARGB32);
  m_histogram1D = m_histogram1D.mirrored();  

  generateDragHistogramImage();
}

void
DrawHiresVolume::generateDragHistogramImage()
{
  if (Global::volumeType() == Global::DummyVolume)
    return;

  int *hist2D = m_Volume->getDrag2dHistogram(m_currentVolume);
  for (int i=0; i<256*256; i++)
    {
      m_histDragData2D[4*i + 3] = 255;
      m_histDragData2D[4*i + 0] = hist2D[i];
      m_histDragData2D[4*i + 1] = hist2D[i];
      m_histDragData2D[4*i + 2] = hist2D[i];
    }
  m_histogramDrag2D = QImage(m_histDragData2D, 256, 256, QImage::Format_ARGB32);
  m_histogramDrag2D = m_histogramDrag2D.mirrored();  


  int *hist1D = m_Volume->getDrag1dHistogram(m_currentVolume);
  memset(m_histDragData1D, 0, 4*256*256);
  for (int i=0; i<256; i++)
    {
      for (int j=0; j<256; j++)
	{
	  int idx = 256*j + i;
	  m_histDragData1D[4*idx + 3] = 255;
	}

      int h = hist1D[i];
      for (int j=0; j<h; j++)
	{
	  int idx = 256*j + i;
	  m_histDragData1D[4*idx + 0] = 255*j/h;
	  m_histDragData1D[4*idx + 1] = 255*j/h;
	  m_histDragData1D[4*idx + 2] = 255*j/h;
	}
    }
  m_histogramDrag1D = QImage(m_histDragData1D, 256, 256, QImage::Format_ARGB32);
  m_histogramDrag1D = m_histogramDrag1D.mirrored();  
}

void
DrawHiresVolume::initShadowBuffers(bool force)
{
//  int shdSizeW = m_Viewer->camera()->screenWidth()*m_lightInfo.shadowScale;
//  int shdSizeH = m_Viewer->camera()->screenHeight()*m_lightInfo.shadowScale;
//  shdSizeW *= 2;
//  shdSizeH *= 2;
//  if (!force &&
//      m_shadowWidth == shdSizeW &&
//      m_shadowHeight == shdSizeH)
//    return; // no need to resize shadow buffers

  int shdSizeW = m_Viewer->camera()->screenWidth();
  int shdSizeH = m_Viewer->camera()->screenHeight();
  m_shadowWidth = shdSizeW;
  m_shadowHeight = shdSizeH;

  GLuint target = GL_TEXTURE_RECTANGLE_EXT;

  if (m_dofBuffer) glDeleteFramebuffers(1, &m_dofBuffer);
  if (m_dofTex[0]) glDeleteTextures(3, m_dofTex);  
  glGenFramebuffers(1, &m_dofBuffer);
  glGenTextures(3, m_dofTex);
  for(int i=0; i<3; i++)
    {
      glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_dofTex[i]);
      glTexImage2D(GL_TEXTURE_RECTANGLE_ARB,
		   0,
		   GL_RGBA,
		   m_shadowWidth, m_shadowHeight,
		   0,
		   GL_RGBA,
		   GL_UNSIGNED_BYTE,
		   0);
    }

  //m_shadowLod = 2;
  if (m_shdBuffer) glDeleteFramebuffers(1, &m_shdBuffer);
  if (m_shdTex[0]) glDeleteTextures(2, m_shdTex);  
  glGenFramebuffers(1, &m_shdBuffer);
  glGenTextures(2, m_shdTex);
  for(int i=0; i<2; i++)
    {
      glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_shdTex[i]);
      glTexImage2D(GL_TEXTURE_RECTANGLE_ARB,
		   0,
		   GL_RGBA,
		   m_shadowWidth/m_shadowLod, m_shadowHeight/m_shadowLod,
		   0,
		   GL_RGBA,
		   GL_UNSIGNED_BYTE,
		   0);
    }
}

void
DrawHiresVolume::updateScaling()
{
  m_bricks->updateScaling();
}

void
DrawHiresVolume::updateSubvolume()
{
  if (!m_updateSubvolume)
    return;

  if (Global::volumeType() == Global::SingleVolume ||
      Global::volumeType() == Global::DummyVolume)
    updateSubvolume(Global::volumeNumber(),
		    m_dataMin, m_dataMax);
  else if (Global::volumeType() == Global::DoubleVolume)
    updateSubvolume(Global::volumeNumber(0),
		    Global::volumeNumber(1),
		    m_dataMin, m_dataMax);
  else if (Global::volumeType() == Global::TripleVolume)
    updateSubvolume(Global::volumeNumber(0),
		    Global::volumeNumber(1),
		    Global::volumeNumber(2),
		    m_dataMin, m_dataMax);
  else if (Global::volumeType() == Global::QuadVolume)
    updateSubvolume(Global::volumeNumber(0),
		    Global::volumeNumber(1),
		    Global::volumeNumber(2),
		    Global::volumeNumber(3),
		    m_dataMin, m_dataMax);
  else if (Global::volumeType() == Global::RGBVolume)
    updateSubvolume(Global::volumeNumber(),
		    m_dataMin, m_dataMax);
  else if (Global::volumeType() == Global::RGBAVolume)
    updateSubvolume(Global::volumeNumber(),
		    m_dataMin, m_dataMax);
}

void
DrawHiresVolume::updateSubvolume(int volnum,
				 Vec boxMin, Vec boxMax,
				 bool force)
{
  if (!m_updateSubvolume)
    return;

  if (m_Volume->setSubvolume(boxMin, boxMax,
			     volnum,
			     force) == false)
    return;

  postUpdateSubvolume(boxMin, boxMax);

  m_Volume->closePvlFileManager();
}

void
DrawHiresVolume::updateSubvolume(int volnum1, int volnum2,
				 Vec boxMin, Vec boxMax,
				 bool force)
{
  if (!m_updateSubvolume)
    return;

  if (m_Volume->setSubvolume(boxMin, boxMax,
			     volnum1, volnum2,
			     force) == false)
    return;

  postUpdateSubvolume(boxMin, boxMax);
}

void
DrawHiresVolume::updateSubvolume(int volnum1, int volnum2, int volnum3,
				 Vec boxMin, Vec boxMax,
				 bool force)
{
  if (!m_updateSubvolume)
    return;

  if (m_Volume->setSubvolume(boxMin, boxMax,
			     volnum1, volnum2, volnum3,
			     force) == false)
    return;

  postUpdateSubvolume(boxMin, boxMax);
}

void
DrawHiresVolume::updateSubvolume(int volnum1, int volnum2,
				 int volnum3, int volnum4,
				 Vec boxMin, Vec boxMax,
				 bool force)
{
  if (!m_updateSubvolume)
    return;

  if (m_Volume->setSubvolume(boxMin, boxMax,
			     volnum1, volnum2, volnum3, volnum4,
			     force) == false)
    return;

  postUpdateSubvolume(boxMin, boxMax);
}

void
DrawHiresVolume::postUpdateSubvolume(Vec boxMin, Vec boxMax)
{
  Vec textureSize = m_Volume->getSubvolumeTextureSize();
  Vec subVolSize = m_Volume->getSubvolumeSize();
  int subsamplingLevel = m_Volume->getSubvolumeSubsamplingLevel();

  m_dataMin = boxMin;
  m_dataMax = boxMax;
  m_dataSize = m_dataMax-m_dataMin;

  Global::setBounds(m_dataMin, m_dataMax);
  m_bricks->setBounds(m_dataMin, m_dataMax);
  GeometryObjects::clipplanes()->setBounds(m_dataMin, m_dataMax);
  

  m_virtualTextureSize = textureSize * subsamplingLevel;
  m_virtualTextureMin = Vec(0,0,0);
  m_virtualTextureMax = m_virtualTextureMin + subVolSize;

//  // don't proceed with data loading if in raycast mode
//  if (m_rcMode)
//    return;

  m_Volume->startHistogramCalculation();
  loadTextureMemory();
  m_Volume->endHistogramCalculation();


  // update saved buffer after every subvolume change
  PruneHandler::setUseSavedBuffer(false);
  // update prunetexture after every subvolume change
  bool upt = Global::updatePruneTexture();
  Global::setUpdatePruneTexture(true);

  if (Global::volumeType() != Global::DummyVolume &&
      Global::volumeType() != Global::RGBVolume &&
      Global::volumeType() != Global::RGBAVolume)
    {
      if (Global::updatePruneTexture())
	updateAndLoadPruneTexture();

      m_Viewer->updateTagColors();
    }
  Global::setUpdatePruneTexture(upt);
  // restore updateprunetexture flag

  if (Global::volumeType() != Global::DummyVolume)
    {
      updateAndLoadLightTexture();

      generateHistogramImage();

      if (!Global::useDragVolume())
	emit histogramUpdated(m_histogram1D, m_histogram2D);
      else
	emit histogramUpdated(m_histogramDrag1D, m_histogramDrag2D);
    }
}

void
DrawHiresVolume::updateAndLoadPruneTexture()
{
  if (Global::volumeType() == Global::DummyVolume)
    return;

  if (Global::volumeType() == Global::RGBVolume ||
      Global::volumeType() == Global::RGBAVolume)
    return;

  if (!Global::emptySpaceSkip())
    return;

  MainWindowUI::mainWindowUI()->menubar->parentWidget()->\
    setWindowTitle("Updating prune texture");

  int dtextureX, dtextureY;
  m_Volume->getDragTextureSize(dtextureX, dtextureY);

  Vec dragInfo = m_Volume->getDragTextureInfo();
  Vec subVolSize = m_Volume->getSubvolumeSize();

  Vec dragvsz = m_Volume->getDragSubvolumeTextureSize();

  PruneHandler::updateAndLoadPruneTexture(m_dataTex[0],
					  dtextureX, dtextureY,
					  dragInfo, subVolSize,
					  dragvsz,
					  m_Viewer->lookupTable());

  MainWindowUI::mainWindowUI()->menubar->parentWidget()->\
    setWindowTitle(Global::DrishtiVersion());
}

void
DrawHiresVolume::updateAndLoadLightTexture()
{
  if (LightHandler::basicLight())
    return;

  if (Global::volumeType() == Global::DummyVolume)
    return;

  int dtextureX, dtextureY;
  m_Volume->getDragTextureSize(dtextureX, dtextureY);

  Vec dragInfo = m_Volume->getDragTextureInfo();
  Vec subVolSize = m_Volume->getSubvolumeSize();

  QList<Vec> cpos, cnorm;
  getClipForMask(cpos, cnorm);
  LightHandler::setClips(cpos, cnorm);
 
  Vec dragvsz = m_Volume->getDragSubvolumeTextureSize();

  LightHandler::updateAndLoadLightTexture(m_dataTex[0],
					  dtextureX, dtextureY,
					  dragInfo, dragvsz,
					  m_dataMin, m_dataMax,
					  subVolSize,
					  m_Viewer->lookupTable());
}


void
DrawHiresVolume::loadTextureMemory()
{
  if (Global::volumeType() == Global::DummyVolume)
    {
      if (m_dataTexSize > 0)
	{
	  glDeleteTextures(m_dataTexSize, m_dataTex);
	  delete [] m_dataTex;
	}
      m_textureSlab.append(Vec(1,1,1));
      m_dataTex = 0;
      m_dataTexSize = 0;
      return;
    }

  m_loadingData = true;

  int nvol = 1;
  if (Global::volumeType() == Global::DoubleVolume) nvol = 2;
  if (Global::volumeType() == Global::TripleVolume) nvol = 3;
  if (Global::volumeType() == Global::QuadVolume) nvol = 4;
  if (Global::volumeType() == Global::RGBVolume) nvol = 3;
  if (Global::volumeType() == Global::RGBAVolume) nvol = 4;

  //int format = GL_LUMINANCE;
  int format = GL_RED;
  if (nvol == 2) format = GL_LUMINANCE_ALPHA;
  else if (nvol == 3) format = GL_RGB;
  else if (nvol == 4) format = GL_RGBA;

  int internalFormat = nvol;
  int vtype = GL_UNSIGNED_BYTE;

  if (m_Volume->pvlVoxelType(0) > 0)
    {
      if (nvol == 1) internalFormat = GL_LUMINANCE16;
      if (nvol == 2) internalFormat = GL_LUMINANCE16_ALPHA16;
      if (nvol == 3) internalFormat = GL_RGB16;
      if (nvol == 4) internalFormat = GL_RGBA16;
      
      if (nvol == 1) format = GL_LUMINANCE;
      if (nvol == 2) format = GL_LUMINANCE_ALPHA;
      if (nvol == 3) format = GL_RGB;
      if (nvol == 4) format = GL_RGBA;
      
      vtype = GL_UNSIGNED_SHORT;
    }
    
  m_textureSlab = m_Volume->getSliceTextureSizeSlabs();

  int textureX, textureY;
  m_Volume->getSliceTextureSize(textureX, textureY);

  int dtextureX, dtextureY;
  m_Volume->getDragTextureSize(dtextureX, dtextureY);

  if (m_dataTexSize > 0)
    {
      glDeleteTextures(m_dataTexSize, m_dataTex);
      delete [] m_dataTex;
    }
      
  m_dataTexSize = m_textureSlab.count();
  if (m_dataTexSize <= 0)
    return;
			   
  // -- disable screen updates 
  bool uv = Global::updateViewer();
  if (uv)
    {
      Global::disableViewerUpdate();
      MainWindowUI::changeDrishtiIcon(false);
    }

  m_dataTex = new GLuint[m_dataTexSize];
  glGenTextures(m_dataTexSize, m_dataTex);

  MainWindowUI::mainWindowUI()->menubar->parentWidget()->\
    setWindowTitle(QString("Uploading slices"));
  Global::progressBar()->show();

//  {      
//    uchar *voxelVol = NULL;
//    if (!Global::loadDragOnly())
//      voxelVol = m_Volume->getSubvolumeTexture();
//    
//    Vec vsz = m_Volume->getSubvolumeTextureSize();
//    int hsz = vsz.x;
//    int wsz = vsz.y;
//    int dsz = vsz.z;
//    
//    glActiveTexture(GL_TEXTURE1);
//    glEnable(GL_TEXTURE_2D_ARRAY);
//    glBindTexture(GL_TEXTURE_2D_ARRAY, m_dataTex[1]);
//    glTexImage3D(GL_TEXTURE_2D_ARRAY,
//		 0, // single resolution
//		 internalFormat,
//		 hsz, wsz, dsz,
//		 0, // no border
//		 format,
//		 vtype,
//		 voxelVol);
//    glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
//    glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
//    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//    glDisable(GL_TEXTURE_2D_ARRAY);
//  }
  
  {
    uchar *voxelVol = m_Volume->getSubvolumeTexture();
    
    Vec vsz = m_Volume->getSubvolumeTextureSize();
    int hsz = vsz.x;
    int wsz = vsz.y;
    int dsz = vsz.z;
    
    for(int i=1; i<m_dataTexSize; i++)
      {
	int zslc = (i-1)*(Global::textureSizeLimit()-1);
	qint64 zoffset = zslc*hsz*wsz;
	int zslices = qMin(dsz-zslc, Global::textureSizeLimit());
	
	glActiveTexture(GL_TEXTURE1);
	glEnable(GL_TEXTURE_2D_ARRAY);
	glBindTexture(GL_TEXTURE_2D_ARRAY, m_dataTex[i]);
	glTexImage3D(GL_TEXTURE_2D_ARRAY,
		     0, // single resolution
		     internalFormat,
		     hsz, wsz, zslices,
		     0, // no border
		     format,
		     vtype,
		     (voxelVol+zoffset));
	glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
	glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glDisable(GL_TEXTURE_2D_ARRAY);
      }
  }
  
  
  //---------------------------------------
  // now load drag texture
  if (m_dataTexSize > 1)
    {
      uchar *voxelVol = NULL;
      voxelVol = m_Volume->getDragSubvolumeTexture();

      Vec vsz = m_Volume->getDragSubvolumeTextureSize();
      int hsz = vsz.x;
      int wsz = vsz.y;
      int dsz = vsz.z;
      
      glActiveTexture(GL_TEXTURE1);
      glEnable(GL_TEXTURE_2D_ARRAY);
      glBindTexture(GL_TEXTURE_2D_ARRAY, m_dataTex[0]);
      glTexImage3D(GL_TEXTURE_2D_ARRAY,
		   0, // single resolution
		   internalFormat,
		   hsz, wsz, dsz,
		   0, // no border
		   format,
		   vtype,
		   voxelVol);
      glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
      glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
      glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glDisable(GL_TEXTURE_2D_ARRAY);
    }

  Global::hideProgressBar();
  MainWindowUI::mainWindowUI()->statusBar->showMessage("Ready");

  m_loadingData = false;

  m_Volume->deleteTextureSlab();

  if (uv)
    {
      Global::enableViewerUpdate();
      MainWindowUI::changeDrishtiIcon(true);
    }
}

void DrawHiresVolume::setImageSizeRatio(float ratio) { m_imgSizeRatio = ratio; }

void
DrawHiresVolume::createBlurShader()
{
  QString shaderString;
  shaderString = ShaderFactory::genBoxShaderString();


  if (m_blurShader)
    glDeleteObjectARB(m_blurShader);

  m_blurShader = glCreateProgramObjectARB();
  if (! ShaderFactory::loadShader(m_blurShader,
				  shaderString))
    exit(0);
  m_blurParm[0] = glGetUniformLocationARB(m_blurShader, "blurTex");
  m_blurParm[1] = glGetUniformLocationARB(m_blurShader, "direc");
  m_blurParm[2] = glGetUniformLocationARB(m_blurShader, "type");
}

void
DrawHiresVolume::createBackplaneShader(float scale)
{
  QString shaderString;

  if (m_backplaneShader1)
    glDeleteObjectARB(m_backplaneShader1);
  if (m_backplaneShader2)
    glDeleteObjectARB(m_backplaneShader2);

  // --- backplane without texture ---
  shaderString = ShaderFactory::genBackplaneShaderString1(scale);
  m_backplaneShader1 = glCreateProgramObjectARB();
  if (! ShaderFactory::loadShader(m_backplaneShader1,
				  shaderString))
    exit(0);
  m_backplaneParm1[0] = glGetUniformLocationARB(m_backplaneShader1, "shadowTex");

  // --- backplane with a texture ---
  shaderString = ShaderFactory::genBackplaneShaderString2(scale);
  m_backplaneShader2 = glCreateProgramObjectARB();
  if (! ShaderFactory::loadShader(m_backplaneShader2,
				  shaderString))
    exit(0);
  m_backplaneParm2[0] = glGetUniformLocationARB(m_backplaneShader2, "shadowTex");
  m_backplaneParm2[1] = glGetUniformLocationARB(m_backplaneShader2, "bgTex");
}

void
DrawHiresVolume::createDefaultShader()
{ 
  QString shaderString;

  GeometryObjects::trisets()->createDefaultShader(m_crops);

  if (Global::volumeType() == Global::SingleVolume ||
      Global::volumeType() == Global::DummyVolume)
    {
      shaderString = ShaderFactory::genDefaultSliceShaderString((m_Volume->pvlVoxelType(0) > 0),
								m_lightInfo.applyLighting,
								m_lightInfo.applyEmissive,
								m_crops,
								m_lightInfo.peel,
								m_lightInfo.peelType,
								m_lightInfo.peelMin,
								m_lightInfo.peelMax,
								m_lightInfo.peelMix);
    }
  else if (Global::volumeType() == Global::RGBVolume ||
	   Global::volumeType() == Global::RGBAVolume)
    shaderString = ShaderFactoryRGB::genDefaultSliceShaderString(m_lightInfo.applyLighting,
								 m_lightInfo.applyEmissive,
								 m_crops,
								 m_lightInfo.peel,
								 m_lightInfo.peelType,
								 m_lightInfo.peelMin,
								 m_lightInfo.peelMax,
								 m_lightInfo.peelMix);
  else
    {
      int nvol = 1;
      if (Global::volumeType() == Global::DoubleVolume) nvol = 2;
      if (Global::volumeType() == Global::TripleVolume) nvol = 3;
      if (Global::volumeType() == Global::QuadVolume) nvol = 4;
    
      shaderString = ShaderFactory2::genDefaultSliceShaderString(m_lightInfo.applyLighting,
								 m_lightInfo.applyEmissive,
								 m_crops,
								 nvol,
								 m_lightInfo.peel,
								 m_lightInfo.peelType,
								 m_lightInfo.peelMin,
								 m_lightInfo.peelMax,
								 m_lightInfo.peelMix,
								 m_mixvol+1, m_mixColor, m_mixOpacity,
								 m_interpolateVolumes);
    }

  if (m_defaultShader)
    glDeleteObjectARB(m_defaultShader);

  m_defaultShader = glCreateProgramObjectARB();
  if (! ShaderFactory::loadShader(m_defaultShader,
				    shaderString))
    exit(0);

  m_vertParm[0] = glGetUniformLocationARB(m_defaultShader, "ClipPlane0");
  m_vertParm[1] = glGetUniformLocationARB(m_defaultShader, "ClipPlane1");

  m_defaultParm[0] = glGetUniformLocationARB(m_defaultShader, "lutTex");
  m_defaultParm[1] = glGetUniformLocationARB(m_defaultShader, "dataTex");

  if (Global::volumeType() != Global::RGBVolume &&
      Global::volumeType() != Global::RGBAVolume)
    m_defaultParm[3] = glGetUniformLocationARB(m_defaultShader, "tfSet");
  else
    m_defaultParm[3] = glGetUniformLocationARB(m_defaultShader, "layerSpacing");

  m_defaultParm[4] = glGetUniformLocationARB(m_defaultShader, "delta");
  m_defaultParm[5] = glGetUniformLocationARB(m_defaultShader, "eyepos");
  m_defaultParm[6] = glGetUniformLocationARB(m_defaultShader, "lightpos");
  m_defaultParm[7] = glGetUniformLocationARB(m_defaultShader, "ambient");
  m_defaultParm[8] = glGetUniformLocationARB(m_defaultShader, "diffuse");
  m_defaultParm[9] = glGetUniformLocationARB(m_defaultShader, "specular");
  m_defaultParm[10]= glGetUniformLocationARB(m_defaultShader, "speccoeff");

  m_defaultParm[11] = glGetUniformLocationARB(m_defaultShader, "paintCenter");
  m_defaultParm[12] = glGetUniformLocationARB(m_defaultShader, "paintSize");

  m_defaultParm[14] = glGetUniformLocationARB(m_defaultShader, "paintTex");

  m_defaultParm[15] = glGetUniformLocationARB(m_defaultShader, "gridx");
  m_defaultParm[16] = glGetUniformLocationARB(m_defaultShader, "tsizex");
  m_defaultParm[17] = glGetUniformLocationARB(m_defaultShader, "tsizey");

  m_defaultParm[18] = glGetUniformLocationARB(m_defaultShader, "depthcue");

  m_defaultParm[19] = glGetUniformLocationARB(m_defaultShader, "pruneTex");
  m_defaultParm[20] = glGetUniformLocationARB(m_defaultShader, "prunegridx");
  m_defaultParm[21] = glGetUniformLocationARB(m_defaultShader, "prunetsizex");
  m_defaultParm[22] = glGetUniformLocationARB(m_defaultShader, "prunetsizey");
  m_defaultParm[23] = glGetUniformLocationARB(m_defaultShader, "prunelod");
  m_defaultParm[24] = glGetUniformLocationARB(m_defaultShader, "zoffset");

  m_defaultParm[25] = glGetUniformLocationARB(m_defaultShader, "dataMin");
  m_defaultParm[26] = glGetUniformLocationARB(m_defaultShader, "dataSize");
  m_defaultParm[27] = glGetUniformLocationARB(m_defaultShader, "tminz");

  m_defaultParm[28] = glGetUniformLocationARB(m_defaultShader, "lod");
  m_defaultParm[29] = glGetUniformLocationARB(m_defaultShader, "dirFront");
  m_defaultParm[30] = glGetUniformLocationARB(m_defaultShader, "dirUp");
  m_defaultParm[31] = glGetUniformLocationARB(m_defaultShader, "dirRight");

  m_defaultParm[32] = glGetUniformLocationARB(m_defaultShader, "interpVol");
  m_defaultParm[33] = glGetUniformLocationARB(m_defaultShader, "mixTag");

  m_defaultParm[34] = glGetUniformLocationARB(m_defaultShader, "lightTex");
  m_defaultParm[35] = glGetUniformLocationARB(m_defaultShader, "lightgridx");
  m_defaultParm[36] = glGetUniformLocationARB(m_defaultShader, "lightgridy");
  m_defaultParm[37] = glGetUniformLocationARB(m_defaultShader, "lightgridz");
  m_defaultParm[38] = glGetUniformLocationARB(m_defaultShader, "lightnrows");
  m_defaultParm[39] = glGetUniformLocationARB(m_defaultShader, "lightncols");
  m_defaultParm[40] = glGetUniformLocationARB(m_defaultShader, "lightlod");

  m_defaultParm[42] = glGetUniformLocationARB(m_defaultShader, "brickMin");
  m_defaultParm[43] = glGetUniformLocationARB(m_defaultShader, "brickMax");

  m_defaultParm[46] = glGetUniformLocationARB(m_defaultShader, "shdlod");
  m_defaultParm[47] = glGetUniformLocationARB(m_defaultShader, "shdTex");
  m_defaultParm[48] = glGetUniformLocationARB(m_defaultShader, "shdIntensity");

  m_defaultParm[49] = glGetUniformLocationARB(m_defaultShader, "opmod");
  m_defaultParm[50] = glGetUniformLocationARB(m_defaultShader, "linearInterpolation");

  m_defaultParm[51] = glGetUniformLocationARB(m_defaultShader, "dofscale");

  m_defaultParm[52] = glGetUniformLocationARB(m_defaultShader, "vsize");
  m_defaultParm[53] = glGetUniformLocationARB(m_defaultShader, "vmin");
  m_defaultParm[54] = glGetUniformLocationARB(m_defaultShader, "dataTexAT");

  m_defaultParm[55] = glGetUniformLocationARB(m_defaultShader, "nclip");
  m_defaultParm[56] = glGetUniformLocationARB(m_defaultShader, "clipPos");
  m_defaultParm[57] = glGetUniformLocationARB(m_defaultShader, "clipNormal");
}

void
DrawHiresVolume::createCopyShader()
{
  QString shaderString;

  if (Global::copyShader())
    return;

  shaderString = ShaderFactory::genCopyShaderString();
  Global::setCopyShader(glCreateProgramObjectARB());
  GLhandleARB cs = Global::copyShader();
  if (! ShaderFactory::loadShader(cs,
				  shaderString))
    exit(0);

  GLint copyParm[5];
  copyParm[0] = glGetUniformLocationARB(Global::copyShader(), "shadowTex");
  Global::setCopyParm(copyParm, 1);
}

void
DrawHiresVolume::createReduceShader()
{
  QString shaderString;

  if (Global::reduceShader())
    return;

  shaderString = ShaderFactory::genReduceShaderString();
  Global::setReduceShader(glCreateProgramObjectARB());
  GLhandleARB cs = Global::reduceShader();
  if (! ShaderFactory::loadShader(cs, shaderString))
    exit(0);

  GLint reduceParm[5];
  reduceParm[0] = glGetUniformLocationARB(Global::reduceShader(), "rtex");
  reduceParm[1] = glGetUniformLocationARB(Global::reduceShader(), "lod");
  Global::setReduceParm(reduceParm, 2);
}

void
DrawHiresVolume::createExtractSliceShader()
{
  QString shaderString;

  if (Global::extractSliceShader())
    return;

  shaderString = ShaderFactory::genExtractSliceShaderString();
  Global::setExtractSliceShader(glCreateProgramObjectARB());
  GLhandleARB cs = Global::extractSliceShader();
  if (! ShaderFactory::loadShader(cs, shaderString))
    exit(0);

  GLint extractSliceParm[5];
  extractSliceParm[0] = glGetUniformLocationARB(Global::extractSliceShader(), "atex");
  extractSliceParm[1] = glGetUniformLocationARB(Global::extractSliceShader(), "btex");
  Global::setExtractSliceParm(extractSliceParm, 2);
}

void
DrawHiresVolume::createPassThruShader()
{
  if (m_passthruShader)
    glDeleteObjectARB(m_passthruShader);

  if (m_lutShader)
    glDeleteObjectARB(m_lutShader);

  QString shaderString;

  shaderString = ShaderFactory::genPassThruShaderString();
  m_passthruShader = glCreateProgramObjectARB();
  if (! ShaderFactory::loadShader(m_passthruShader,
				    shaderString))
    exit(0);

  shaderString = ShaderFactory::genLutShaderString((m_Volume->pvlVoxelType(0) > 0));
  m_lutShader = glCreateProgramObjectARB();
  if (! ShaderFactory::loadShader(m_lutShader,
				  shaderString))
    exit(0);
  m_lutParm[0] = glGetUniformLocationARB(m_lutShader, "lutTex");
  glUniform1iARB(m_lutParm[0], 0); // lutTex
}

void
DrawHiresVolume::runLutShader(bool flag)
{
  if (flag)
    glUseProgramObjectARB(m_lutShader);
  else
    glUseProgramObjectARB(0);
}


void
DrawHiresVolume::createShaders()
{ 
  createPassThruShader();
  createCopyShader();
  createReduceShader();
  createExtractSliceShader();

  createDefaultShader();
  createBlurShader();
  createBackplaneShader(m_lightInfo.backplaneIntensity);
}
//--------------------------------------------------------

void
DrawHiresVolume::enableTextureUnits()
{
  m_Viewer->enableTextureUnits();

//  glActiveTexture(GL_TEXTURE1);
//  glEnable(GL_TEXTURE_RECTANGLE_ARB);
//  glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//  glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glActiveTexture(GL_TEXTURE2);
  glEnable(GL_TEXTURE_RECTANGLE_ARB);
  glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glActiveTexture(GL_TEXTURE3);
  glEnable(GL_TEXTURE_RECTANGLE_ARB);
  glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glActiveTexture(GL_TEXTURE4);
  glEnable(GL_TEXTURE_RECTANGLE_ARB);
  glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glActiveTexture(GL_TEXTURE6);
  glEnable(GL_TEXTURE_RECTANGLE_ARB);
  glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void
DrawHiresVolume::disableTextureUnits()
{
  m_Viewer->disableTextureUnits();

//  glActiveTexture(GL_TEXTURE1);
//  glDisable(GL_TEXTURE_RECTANGLE_ARB);

  glActiveTexture(GL_TEXTURE2);
  glDisable(GL_TEXTURE_RECTANGLE_ARB);

  glActiveTexture(GL_TEXTURE3);
  glDisable(GL_TEXTURE_RECTANGLE_ARB);

  glActiveTexture(GL_TEXTURE4);
  glDisable(GL_TEXTURE_RECTANGLE_ARB);

  glActiveTexture(GL_TEXTURE6);
  glDisable(GL_TEXTURE_RECTANGLE_ARB);
}

void
DrawHiresVolume::drawDragImage(float stepsize)
{
  m_drawImageType = Enums::DragImage;

  if (Global::volumeType() != Global::DummyVolume)    
    draw(stepsize, false);
  else // increase the stepsize
    draw(5*stepsize, false);
}

void
DrawHiresVolume::drawStillImage(float stepsize)
{
  if (Global::useDragVolume() || Global::loadDragOnly())
    m_drawImageType = Enums::DragImage;
  else
    m_drawImageType = Enums::StillImage;

  if (Global::volumeType() != Global::DummyVolume)    
    draw(stepsize, true);
  else // increase the stepsize
    draw(5*stepsize, true);
}

void
DrawHiresVolume::getClipForMask(QList<Vec> &clipPos,
				QList<Vec> &clipNormal)
{
  QList<Vec> cpos = GeometryObjects::clipplanes()->positions();
  QList<Vec> cnormal = GeometryObjects::clipplanes()->normals();
  QList<bool> applyclip = GeometryObjects::clipplanes()->applyClip();

  QList<bool> clippers;
  clippers = m_bricks->brickInformation(0).clippers;
  if (clippers.count() < cpos.size())
    {
      for(int ci=clippers.count(); ci<cpos.size(); ci++)
	clippers << false;
    }

  for(int ci=0; ci<cpos.size(); ci++)
    {
      if (clippers[ci] && applyclip[ci])
	{
	  Vec p = cpos[ci];
	  Vec n = cnormal[ci];
	  clipPos.append(p);
	  clipNormal.append(n);
	}
    }
}


void
DrawHiresVolume::collectBrickInformation(bool force)
{
  double bxform[16];
  memcpy(bxform, m_bricks->getMatrix(), 16*sizeof(double));
  GeometryObjects::clipplanes()->setBrick0Xform(bxform);

  if (force == false &&
      m_bricks->updateFlag() == false)
    return;

  double Xform[16];

  m_bricks->resetUpdateFlag();

  m_drawBrickInformation.reset();

  Vec voxelScaling = Global::voxelScaling();

  QList<BrickInformation> userBricks = m_bricks->bricks();
  QList<BrickBounds> ghostBricks = m_bricks->ghostBricks();

  int numGhostBricks = ghostBricks.size();
  m_numBricks = ghostBricks.size() + userBricks.size() - 1;


  ClipInformation clipInfo = GeometryObjects::clipplanes()->clipInfo();

  QList<Vec> cpos;
  QList<Vec> cnormal;
  for(int ci=0; ci<clipInfo.pos.size(); ci++)
    {
      Vec p = clipInfo.pos[ci];
      Vec n = clipInfo.rot[ci].rotate(Vec(0,0,1));
      cpos.append(p);
      cnormal.append(n);
    }

  for (int bnum=0; bnum<m_numBricks; bnum++)
    {
      int bno;
      float xmin, xmax, ymin, ymax, zmin, zmax;
      Vec subdim, subcorner, subvol[8], texture[8];
      Vec bmin, bmax;
      QList<bool> clippers;
      int tfSet;
      Vec brickPivot;
      Vec brickScale;
      
      if (bnum < numGhostBricks)
	{
	  bno = bnum;
	  bmin = m_dataMin + VECPRODUCT(m_dataSize,ghostBricks[bno].brickMin);
	  bmax = m_dataMin + VECPRODUCT(m_dataSize,ghostBricks[bno].brickMax);	  

	  clippers = m_bricks->brickInformation(0).clippers;
	  tfSet = m_bricks->brickInformation(0).tfSet;
	  
	  brickPivot = m_bricks->getScalePivot(0);
	  brickPivot = VECPRODUCT(brickPivot, voxelScaling);
	  brickScale = m_bricks->getScale(0);
	}
      else
	{
	  // userBrick 0 is whole subvolume
	  // user defined bricks actually start from 1 onwards
	  bno = bnum - numGhostBricks + 1;
	  bmin = m_dataMin + VECPRODUCT(m_dataSize,userBricks[bno].brickMin);
	  bmax = m_dataMin + VECPRODUCT(m_dataSize,userBricks[bno].brickMax);

	  clippers = m_bricks->brickInformation(bno).clippers;
	  tfSet = m_bricks->brickInformation(bno).tfSet;

	  brickPivot = m_bricks->getScalePivot(bno);
	  brickPivot = VECPRODUCT(brickPivot, voxelScaling);
	  brickScale = m_bricks->getScale(bno);
	}

      bmin = StaticFunctions::maxVec(m_dataMin+Vec(1,1,1), bmin);
      bmax = StaticFunctions::minVec(m_dataMax-Vec(1,1,1), bmax);

      texture[0] = Vec(bmin.x, bmin.y, bmin.z);
      texture[1] = Vec(bmax.x, bmin.y, bmin.z);
      texture[2] = Vec(bmax.x, bmax.y, bmin.z);
      texture[3] = Vec(bmin.x, bmax.y, bmin.z);
      texture[4] = Vec(bmin.x, bmin.y, bmax.z);
      texture[5] = Vec(bmax.x, bmin.y, bmax.z);
      texture[6] = Vec(bmax.x, bmax.y, bmax.z);
      texture[7] = Vec(bmin.x, bmax.y, bmax.z);

      xmin = (bmin.x * voxelScaling.x);
      ymin = (bmin.y * voxelScaling.y);
      zmin = (bmin.z * voxelScaling.z);
      xmax = (bmax.x * voxelScaling.x);
      ymax = (bmax.y * voxelScaling.y);
      zmax = (bmax.z * voxelScaling.z);

      subvol[0] = Vec(xmin, ymin, zmin);
      subvol[1] = Vec(xmax, ymin, zmin);
      subvol[2] = Vec(xmax, ymax, zmin);
      subvol[3] = Vec(xmin, ymax, zmin);
      subvol[4] = Vec(xmin, ymin, zmax);
      subvol[5] = Vec(xmax, ymin, zmax);
      subvol[6] = Vec(xmax, ymax, zmax);
      subvol[7] = Vec(xmin, ymax, zmax);


      subcorner = VECPRODUCT(m_virtualTextureMin,voxelScaling);
      subcorner += VECPRODUCT(m_dataMin, voxelScaling);

      subdim = VECPRODUCT(m_virtualTextureSize,voxelScaling) - Vec(1,1,1);

      if (Global::flipImage())
	{
	  QVector<int> idx;

	  if (Global::flipImageZ())
	    {
	      idx << 0 << 4;
	      idx << 1 << 5;
	      idx << 2 << 6;
	      idx << 3 << 7;
	    }
	  else if (Global::flipImageY())
	    {
	      idx << 0 << 3;
	      idx << 1 << 2;
	      idx << 4 << 7;
	      idx << 5 << 6;
	    }
	  else if (Global::flipImageX())
	    {
	      idx << 0 << 1;
	      idx << 2 << 3;
	      idx << 4 << 5;
	      idx << 6 << 7;
	    }

	  for (int i=0; i<4; i++)
	    {
	      Vec vt;
	      
	      vt = texture[idx[2*i]];
	      texture[idx[2*i]] = texture[idx[2*i+1]];
	      texture[idx[2*i+1]] = vt;
	    }
	}


      //---------------------------------------------------
      // apply transformation
      if (bnum >= numGhostBricks)
	memcpy(Xform, m_bricks->getMatrix(bno), 16*sizeof(double));
      else
	memcpy(Xform, m_bricks->getMatrix(0), 16*sizeof(double));


      for (int i=0; i<8; i++)
	subvol[i] = Matrix::xformVec(Xform, subvol[i]);
      
      //---------------------
      // now apply scaling
      for (int i=0; i<8; i++)
	{
	  Vec sv = subvol[i];
	  sv = subvol[i]-brickPivot;
	  subvol[i] = brickPivot + VECPRODUCT(sv, brickScale);
	}
      //---------------------


      subcorner = Matrix::xformVec(Xform, subcorner);

      // save information in the list
      m_drawBrickInformation.append(tfSet,
				    subvol, texture,
				    subcorner, subdim,
				    clippers,
				    brickPivot, brickScale);
    }
}

void
DrawHiresVolume::getMinMaxVertices(float &zdepth,
				   Vec &minvert, Vec &maxvert)
{
 float mindepth, maxdepth;

 for (int i=0; i<m_drawBrickInformation.subvolSize(); i++)
   {
     Vec subvol = m_drawBrickInformation.subvol(i);
     Vec camCoord = m_Viewer->camera()->cameraCoordinatesOf(subvol);
     float zval = camCoord.z;
     
      if (i == 0)
	{
	  mindepth = maxdepth = zval;
	  minvert = maxvert = subvol;
	}
      else
	{
	  if (zval < mindepth)
	    {
	      mindepth = zval;
	      minvert = subvol;
	    }

	  if (zval > maxdepth)
	    {
	      maxdepth = zval;
	      maxvert = subvol;
	    }
	}
    }

 if (maxdepth > 0) // camera is inside the volume
   {
     maxdepth = 0;
     maxvert = m_Viewer->camera()->position();
   }
 if (mindepth > 0)
   {
     mindepth = 0;
     minvert = m_Viewer->camera()->position();
   }
   

 zdepth = maxdepth - mindepth;
}

void
DrawHiresVolume::loadCameraMatrices()
{
  if (Global::saveImageType() == Global::NoImage)
    {
      if (m_Viewer->displaysInStereo() == false)
	{
	  m_Viewer->camera()->loadProjectionMatrix(true);
	  m_Viewer->camera()->loadModelViewMatrix(true);
	}
      else
	{
	  m_Viewer->camera()->setFocusDistance(m_focusDistance);
	  m_Viewer->camera()->setPhysicalScreenWidth(m_screenWidth);

	  if (m_currentDrawbuf == GL_BACK_LEFT)
	    {
	      m_Viewer->camera()->loadProjectionMatrixStereo(false);
	      m_Viewer->camera()->loadModelViewMatrixStereo(false);
	    }
	  else
	    {
	      m_Viewer->camera()->loadProjectionMatrixStereo(true);
	      m_Viewer->camera()->loadModelViewMatrixStereo(true);
	    }
	}
    }
  else
    {
      if (Global::saveImageType() == Global::MonoImage)
	{
	  m_Viewer->camera()->loadProjectionMatrix(true);
	  m_Viewer->camera()->loadModelViewMatrix(true);
	}
      else if (Global::saveImageType() == Global::RightImage ||
	       Global::saveImageType() == Global::LeftImage ||
	       Global::saveImageType() == Global::RightImageAnaglyph ||
	       Global::saveImageType() == Global::LeftImageAnaglyph)
	{
	  // Stereo Images

	  m_Viewer->camera()->setFocusDistance(m_focusDistance);
	  m_Viewer->camera()->setPhysicalScreenWidth(m_screenWidth);

	  if (Global::saveImageType() == Global::LeftImage ||
	      Global::saveImageType() == Global::LeftImageAnaglyph)
	    {
	      m_Viewer->camera()->loadProjectionMatrixStereo(false);
	      m_Viewer->camera()->loadModelViewMatrixStereo(false);
	    }
	  else if (Global::saveImageType() == Global::RightImage ||
		   Global::saveImageType() == Global::RightImageAnaglyph)
	    {
	      m_Viewer->camera()->loadProjectionMatrixStereo(true);
	      m_Viewer->camera()->loadModelViewMatrixStereo(true);
	    }
	}
      else
	{
	  // Cubic Images (or Pano)
	  m_Viewer->camera()->loadProjectionMatrix(true);
	  m_Viewer->camera()->loadModelViewMatrix(true);
	}
    }
}

void
DrawHiresVolume::draw(float stepsize,
		      bool stillimage)
{
  if (m_loadingData)
    return;

  if (MainWindowUI::mainWindowUI()->actionRedBlue->isChecked() ||
      MainWindowUI::mainWindowUI()->actionRedCyan->isChecked())
    {
      if (Global::saveImageType() == Global::LeftImageAnaglyph)
	{
	  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	  glClearColor(0, 0, 0, 0);
	  glClear(GL_COLOR_BUFFER_BIT);

	  glColorMask(GL_TRUE, GL_FALSE, GL_FALSE, GL_TRUE);
	}
      else // right image
	{
	  glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);
	  glClearColor(0, 0, 0, 0);
	  glClear(GL_COLOR_BUFFER_BIT);

	  if (MainWindowUI::mainWindowUI()->actionRedBlue->isChecked())
	    glColorMask(GL_FALSE, GL_FALSE, GL_TRUE, GL_TRUE);
	  else
	    glColorMask(GL_FALSE, GL_TRUE, GL_TRUE, GL_TRUE);
	}
    }

  m_focusDistance = m_Viewer->camera()->focusDistance();
  m_screenWidth = m_Viewer->camera()->physicalScreenWidth();
  m_cameraWidth = m_Viewer->camera()->screenWidth();
  m_cameraHeight = m_Viewer->camera()->screenHeight();

  checkCrops();
  checkPaths();

  glGetIntegerv(GL_DRAW_BUFFER, &m_currentDrawbuf);

  glDepthMask(GL_TRUE);
  glEnable(GL_ALPHA_TEST);
  glAlphaFunc(GL_GREATER, 0);

  collectBrickInformation();
  collectEnclosingBoxInfo();

  Vec pn;
  Vec minvert, maxvert;
  int layers;
  float zdepth;

  // light direction and position is different for cubic images
  // we want to keep same direction and position for all cube face images
  if ( Global::saveImageType() >= Global::CubicFrontImage)
    m_lightVector = m_Viewer->camera()->viewDirection();
  else
    m_lightVector = (m_lightInfo.userLightVector.x * m_Viewer->camera()->rightVector() +
		     m_lightInfo.userLightVector.y * m_Viewer->camera()->upVector() +
		     m_lightInfo.userLightVector.z * m_Viewer->camera()->viewDirection());
  float camdist = m_Viewer->camera()->sceneRadius();

  if ( Global::saveImageType() >= Global::CubicFrontImage)
    m_lightPosition = m_Viewer->camera()->position();
  else
    m_lightPosition = (m_Viewer->camera()->sceneCenter() -
		       (3.0f + m_lightInfo.lightDistanceOffset)*camdist*m_lightVector);  


  Quaternion defaultCamRot = m_Viewer->camera()->orientation();
  Vec defaultCamPos = m_Viewer->camera()->position();
  Vec defaultViewDirection = m_Viewer->camera()->viewDirection();
  Vec midCamPos = defaultCamPos;
  if ( stillimage &&
       Global::saveImageType() < Global::CubicFrontImage &&
       m_renderQuality == Enums::RenderHighQuality &&
      (m_lightInfo.applyShadows || m_lightInfo.applyLighting))
    {
      if (m_lightVector*defaultViewDirection >= 0)
	{
	  Vec vdir = (m_lightVector+defaultViewDirection).unit();
	  midCamPos = m_Viewer->camera()->position() - camdist*vdir;
	}
      else
	{
	  Vec vdir = (m_lightVector-defaultViewDirection).unit();
	  midCamPos = m_Viewer->camera()->position() + camdist*vdir;
	}
      m_Viewer->camera()->setPosition(midCamPos);
      m_Viewer->camera()->lookAt(m_Viewer->sceneCenter());
      m_Viewer->camera()->loadProjectionMatrix(true);
      m_Viewer->camera()->loadModelViewMatrix(true);
    }
  pn = m_Viewer->camera()->viewDirection(); // slicing direction

  getMinMaxVertices(zdepth, minvert, maxvert);
  
  m_Viewer->camera()->setPosition(defaultCamPos);
  m_Viewer->camera()->setOrientation(defaultCamRot);
  m_Viewer->camera()->setViewDirection(defaultViewDirection);
  loadCameraMatrices();

//------------------------------------
//  change stepsize based on lod
//------------------------------------
  if (m_drawImageType == Enums::DragImage)
    {
      Vec draginfo = m_Volume->getDragTextureInfo();
      float lod = draginfo.z;
      stepsize *= lod;
    }
  else
    {
      float lod = m_Volume->getSubvolumeSubsamplingLevel();
      stepsize *= lod;
    }
//------------------------------------

  layers = zdepth/stepsize;

  Global::setGeoRenderSteps(qMax(1, (int)(1.0/stepsize)));

  glLineWidth(1);

  m_drawGeometryPresent = false;
  m_drawGeometryPresent |= (GeometryObjects::clipplanes()->count() > 0);
  m_drawGeometryPresent |= Global::drawBox();
  m_drawGeometryPresent |= Global::drawAxis();
  m_drawGeometryPresent |= (GeometryObjects::trisets()->count() > 0);
  m_drawGeometryPresent |= (GeometryObjects::networks()->count() > 0);
  m_drawGeometryPresent |= (GeometryObjects::paths()->count() > 0);
  m_drawGeometryPresent |= (GeometryObjects::grids()->count() > 0);
  m_drawGeometryPresent |= (GeometryObjects::crops()->count() > 0);
  m_drawGeometryPresent |= (GeometryObjects::pathgroups()->count() > 0);
  m_drawGeometryPresent |= (GeometryObjects::hitpoints()->count() > 0);
  m_drawGeometryPresent |= (GeometryObjects::landmarks()->count() > 0);
  m_drawGeometryPresent |= (GeometryObjects::imageCaptions()->count() > 0);
  m_drawGeometryPresent |= (LightHandler::giLights()->count() > 0);

  m_useScreenShadows = false;
  if (!stillimage || m_renderQuality == Enums::RenderDefault)
     drawDefault(pn, minvert, maxvert, layers, stepsize);
  else if (m_renderQuality == Enums::RenderHighQuality)
    {
      m_useScreenShadows = true;
      drawDefault(pn, minvert, maxvert, layers, stepsize);
    }

  drawBackground();
  
  disableTextureUnits();
  glUseProgramObjectARB(0);

  if (Global::drawBox())
    {
      double Xform[16];
      memcpy(Xform, m_bricks->getMatrix(0), 16*sizeof(double));
      Tick::draw(m_Viewer->camera(), Xform);
    }

  if (Global::drawAxis())
    drawAxisText();


  GeometryObjects::clipplanes()->postdraw((QGLViewer*)m_Viewer);

  glEnable(GL_DEPTH_TEST);
  //glActiveTexture(GL_TEXTURE0);
}

void
DrawHiresVolume::drawAxisText()
{
  glEnable(GL_DEPTH_TEST);
  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_TEXTURE_2D);

  glColor3f(1,1,1);

  //-------------
  // calculate font scale based on dpi
  float fscl = 120.0/Global::dpi();
  //-------------

  QFont tfont = QFont("Helvetica", 18*fscl);
  tfont.setStyleStrategy(QFont::PreferAntialias);
  tfont.setBold(true);

  Vec posX, posY, posZ;
  posX = m_enclosingBox[0] + m_axisArrow[0];
  m_Viewer->renderText(posX.x, posX.y, posX.z,
		       QString("X"),
		       tfont);
  posY = m_enclosingBox[0] + m_axisArrow[1];
  m_Viewer->renderText(posY.x, posY.y, posY.z,
		       QString("Y"),
		       tfont);
  posZ = m_enclosingBox[0] + m_axisArrow[2];
  m_Viewer->renderText(posZ.x, posZ.y, posZ.z,
		       QString("Z"),
		       tfont);
  
  
  tfont.setBold(false);
  glColor3f(0,0,0);
  m_Viewer->renderText(posX.x, posX.y, posX.z,
		       QString("X"),
		       tfont);
  m_Viewer->renderText(posY.x, posY.y, posY.z,
		       QString("Y"),
		       tfont);
  m_Viewer->renderText(posZ.x, posZ.y, posZ.z,
		       QString("Z"),
		       tfont);

  glDisable(GL_DEPTH_TEST);
  glDisable(GL_TEXTURE_2D);
}

int
DrawHiresVolume::drawpoly(Vec po, Vec pn,
			  Vec *subvol, Vec *texture,
			  ViewAlignedPolygon *vap)
{
  int sidx[] = { 0, 1,
		 0, 3,
		 2, 1,
		 2, 3,
		 4, 5,
		 4, 7,
		 6, 5,
		 6, 7,
		 0, 4,
		 1, 5,
		 2, 6,
		 3, 7 };

  vap->edges = 0;

  Vec poly[100];
  Vec tex[100];
  int edges = 0;

  for(int si=0; si<12; si++)
    {
      int k = sidx[2*si];
      int l = sidx[2*si+1];
      edges += StaticFunctions::intersectType1WithTexture(po, pn,
							  subvol[k], subvol[l],
							  texture[k], texture[l],
							  poly[edges], tex[edges]);
    }

  if (edges < 3) return 0;

  Vec cen;
  int i;
  for(i=0; i<edges; i++)
    cen += poly[i];
  cen/=edges;

  float angle[6];
  Vec vaxis, vperp;
  vaxis = poly[0]-cen;
  vaxis.normalize();

  vperp = vaxis^(poly[1]-cen);
  vperp = vperp^vaxis;
  vperp.normalize();

  angle[0] = 1;
  for(i=1; i<edges; i++)
    {
      Vec v;
      v = poly[i]-cen;
      v.normalize();
      angle[i] = vaxis*v;
      if (vperp*v < 0)
	angle[i] = -2 - angle[i];
    }

  // sort angle
  int order[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
  for(i=edges-1; i>=0; i--)
    for(int j=1; j<=i; j++)
      {
	if (angle[order[i]] < angle[order[j]])
	  {
	    int tmp = order[i];
	    order[i] = order[j];
	    order[j] = tmp;
	  }
      }

  //---- apply clipping
  int tedges;
  Vec tpoly[100];
  Vec ttex[100];

  for(i=0; i<edges; i++)
    tpoly[i] = poly[order[i]];
  for(i=0; i<edges; i++)
    poly[i] = tpoly[i];

  for(i=0; i<edges; i++)
    ttex[i] = tex[order[i]];
  for(i=0; i<edges; i++)
    tex[i] = ttex[i];

  // copy data into ViewAlignedPolygon
  vap->edges = edges;
  for(i=0; i<edges; i++)
    {  
      vap->vertex[i] = poly[i];
      vap->texcoord[i] = tex[i];
    }

  return 1;
}

void
DrawHiresVolume::collectEnclosingBoxInfo()
{
  Vec voxelScaling = Global::voxelScaling();
  Vec bmin = VECPRODUCT(m_dataMin,voxelScaling);
  Vec bmax = VECPRODUCT(m_dataMax,voxelScaling);

  double *Xform = m_bricks->getMatrix(0);

  m_enclosingBox[0] = Vec(bmin.x, bmin.y, bmin.z);
  m_enclosingBox[1] = Vec(bmax.x, bmin.y, bmin.z);
  m_enclosingBox[2] = Vec(bmax.x, bmax.y, bmin.z);
  m_enclosingBox[3] = Vec(bmin.x, bmax.y, bmin.z);
  m_enclosingBox[4] = Vec(bmin.x, bmin.y, bmax.z);
  m_enclosingBox[5] = Vec(bmax.x, bmin.y, bmax.z);
  m_enclosingBox[6] = Vec(bmax.x, bmax.y, bmax.z);
  m_enclosingBox[7] = Vec(bmin.x, bmax.y, bmax.z);

  for (int i=0; i<8; i++)
    m_enclosingBox[i] = Matrix::xformVec(Xform, m_enclosingBox[i]);

  m_axisArrow[0] =  m_enclosingBox[1] - m_enclosingBox[0];
  m_axisArrow[1] =  m_enclosingBox[3] - m_enclosingBox[0];
  m_axisArrow[2] =  m_enclosingBox[4] - m_enclosingBox[0];

  m_axisArrow[0] *= 0.5;
  m_axisArrow[1] *= 0.5;
  m_axisArrow[2] *= 0.5;
}


void
DrawHiresVolume::postDrawGeometry()
{
  // for fixed-function pipeline
  glDisable(GL_CLIP_PLANE0);
  glDisable(GL_CLIP_PLANE1);

  // for progammable pipeline
  glDisable(GL_CLIP_DISTANCE0);
  glDisable(GL_CLIP_DISTANCE1);
  glUniform4fARB(m_vertParm[0], 0, 1, 0, 10000);
  glUniform4fARB(m_vertParm[1], 1, 0, 0, 10000);
  GeometryObjects::trisets()->setClipDistance0(0, 1, 0, 10000);
  GeometryObjects::trisets()->setClipDistance1(1, 0, 0, 10000);
}

void
DrawHiresVolume::preDrawGeometry(int s, int layers,
				 Vec po, Vec pn, Vec step,
				 bool fromclip)
{
  //----- restrict geometry between two slices ----
  GLdouble eqn[4];

  if (m_backlit)
    {
      if (s > 0)
	{
	  eqn[0] = -pn.x;
	  eqn[1] = -pn.y;
	  eqn[2] = -pn.z;
	  eqn[3] = pn*po;
	  glEnable(GL_CLIP_PLANE0);
	  glClipPlane(GL_CLIP_PLANE0, eqn);
	  if (fromclip)
	    {
	      glEnable(GL_CLIP_DISTANCE0);
	      glUniform4fARB(m_vertParm[0], eqn[0], eqn[1], eqn[2], eqn[3]);
	    }
	  else
	    GeometryObjects::trisets()->setClipDistance0(eqn[0], eqn[1], eqn[2], eqn[3]);
	}
      if (s < layers-1)
	{
	  eqn[0] = pn.x;
	  eqn[1] = pn.y;
	  eqn[2] = pn.z;
	  eqn[3] = -pn*(po-step);
	  glEnable(GL_CLIP_PLANE1);
	  glClipPlane(GL_CLIP_PLANE1, eqn);      
	  if (fromclip)
	    {
	      glEnable(GL_CLIP_DISTANCE1);
	      glUniform4fARB(m_vertParm[1], eqn[0], eqn[1], eqn[2], eqn[3]);
	    }
	  else
	    GeometryObjects::trisets()->setClipDistance1(eqn[0], eqn[1], eqn[2], eqn[3]);
	}
    }
  else
    {
      if (s > 0)
	{
	  eqn[0] = pn.x;
	  eqn[1] = pn.y;
	  eqn[2] = pn.z;
	  eqn[3] = -pn*po;
	  glEnable(GL_CLIP_PLANE0);
	  glClipPlane(GL_CLIP_PLANE0, eqn);
	  if (fromclip)
	    {
	      glEnable(GL_CLIP_DISTANCE0);
	      glUniform4fARB(m_vertParm[0], eqn[0], eqn[1], eqn[2], eqn[3]);
	    }
	  else
	    GeometryObjects::trisets()->setClipDistance0(eqn[0], eqn[1], eqn[2], eqn[3]);
	}
      if (s < layers-1)
	{
	  eqn[0] = -pn.x;
	  eqn[1] = -pn.y;
	  eqn[2] = -pn.z;
	  eqn[3] = pn*(po+step);
	  glEnable(GL_CLIP_PLANE1);
	  glClipPlane(GL_CLIP_PLANE1, eqn);
	  if (fromclip)
	    {
	      glEnable(GL_CLIP_DISTANCE1);
	      glUniform4fARB(m_vertParm[1], eqn[0], eqn[1], eqn[2], eqn[3]);
	    }
	  else
	    GeometryObjects::trisets()->setClipDistance1(eqn[0], eqn[1], eqn[2], eqn[3]);
	}
    }
}

void
DrawHiresVolume::getGeoSliceBound(int s, int layers,
				  Vec po, Vec pn, Vec step,
				  float &pstart, float &pend)
{
  if (m_backlit)
    {
      if (s > 0 && s < layers-1)
	{
	  pstart = pn*(po-step);
	  pend = pn*po;
	}
      else if (s == 0)
	{
	  pstart = pn*(po-step);
	  pend = pn*(po+1000*step);
	}
      else
	{
	  pstart = pn*(po-1000*step);
	  pend = pn*po;
	}
    }
  else
    {
      if (s > 0 && s < layers-1)
	{
	  pstart = pn*po;
	  pend = pn*(po+step);
	}
      else if (s == 0)
	{
	  pstart = pn*(po-1000*step);
	  pend = pn*(po+step);
	}
      else
	{
	  pstart = pn*po;
	  pend = pn*(po+1000*step);
	}
    }
}

void
DrawHiresVolume::renderGeometry(int s, int layers,
				Vec po, Vec pn, Vec step,
				bool shadow, bool shadowshader, Vec eyepos,
				bool offset)
{
  if (m_drawGeometryPresent)
    {
      glUseProgramObjectARB(0);
      disableTextureUnits();
	  
      preDrawGeometry(s, layers,
		      po, pn, step);
      
      float pstart, pend;
      getGeoSliceBound(s, layers,
		       po, pn, step,
		       pstart, pend);

      Vec pstep = Vec(0,0,0);
      if (offset)
	{
	  if (m_backlit) pstep = -step;
	  else pstep = step;
	  pstep *= 1.0f/Global::geoRenderSteps();
	}
      
      drawGeometry(pn, pstart, pend, pstep,
		   shadow, shadowshader, eyepos);
      
      postDrawGeometry();
      
      glEnable(GL_DEPTH_TEST);
    }
}

void
DrawHiresVolume::drawGeometry(Vec pn, float pnear, float pfar, Vec step,
			      bool applyShadows, bool applyshadowShader,
			      Vec eyepos)
{
  GeometryObjects::trisets()->draw(m_Viewer,
				   m_lightPosition,
				   pnear, pfar,
				   step,
				   applyShadows, applyshadowShader,
				   eyepos,
				   m_clipPos, m_clipNormal,
				   true);

  GeometryObjects::networks()->draw(m_Viewer,
				    pnear, pfar,
				    m_backlit);

  glLineWidth(1.5);

  Vec lineColor = Vec(0.9, 0.9, 0.9);

  Vec bgcolor = Global::backgroundColor();
  float bgintensity = (0.3*bgcolor.x +
		       0.5*bgcolor.y +
		       0.2*bgcolor.z);
  if (bgintensity > 0.5)
    lineColor = Vec(0, 0, 0);

  if (Global::drawBox())
    StaticFunctions::drawEnclosingCube(m_enclosingBox,
				       lineColor);
  

  if (Global::drawAxis())
    StaticFunctions::drawAxis(m_enclosingBox[0],
			      m_axisArrow[0],
			      m_axisArrow[1],
			      m_axisArrow[2]);

  //-----------------------------------------

  // draw clipplanes before applying brick0 transformations
  GeometryObjects::clipplanes()->draw(m_Viewer, m_backlit);

  //----- apply brick0 transformation -------
  Vec voxelScaling = Global::voxelScaling();
  Vec pos = VECPRODUCT(m_bricks->getTranslation(0), voxelScaling);
  Vec pivot = VECPRODUCT(m_bricks->getPivot(0),voxelScaling);
  Vec axis = m_bricks->getAxis(0);
  float angle = m_bricks->getAngle(0);

  glPushMatrix();

  glTranslatef(pos.x, pos.y, pos.z);
      
  glTranslatef(pivot.x, pivot.y, pivot.z);
  glRotatef(angle,
	    axis.x,
	    axis.y,
	    axis.z);
  glTranslatef(-pivot.x, -pivot.y, -pivot.z);
  //-----------------------------------------

  GeometryObjects::crops()->draw(m_Viewer, m_backlit);
  GeometryObjects::paths()->draw(m_Viewer, pn, pnear, pfar, m_backlit, m_lightPosition);
  GeometryObjects::grids()->draw(m_Viewer, m_backlit, m_lightPosition);
  GeometryObjects::pathgroups()->draw(m_Viewer, m_backlit, m_lightPosition, true);
  GeometryObjects::hitpoints()->draw(m_Viewer, m_backlit);
  GeometryObjects::landmarks()->draw(m_Viewer, m_backlit);
  
  //if (m_drawImageType == Enums::StillImage)
  GeometryObjects::imageCaptions()->draw(m_Viewer, m_backlit);

  LightHandler::giLights()->draw(m_Viewer, m_backlit);


  //-----------------------------------------
  //----- apply brick0 transformation -------
  glPopMatrix();
  //-----------------------------------------

  glLineWidth(1);
}

void
DrawHiresVolume::setShader2DTextureParameter(bool useAllTex,
					     bool defaultShader)
{
  Vec draginfo = m_Volume->getDragTextureInfo();
  int lod = m_Volume->getSubvolumeSubsamplingLevel();
  float plod = draginfo.z/float(lod);

  int gridx, gridy, lenx2, leny2;
  if (useAllTex)
    {
      m_Volume->getColumnsAndRows(gridx, gridy);
      int lenx = m_dataSize.x+1;
      int leny = m_dataSize.y+1;
      lenx2 = lenx/lod;
      leny2 = leny/lod;
    }
  else
    {
      lod = draginfo.z;
      gridx = draginfo.x;
      int lenx = (m_dataSize.x+1);
      int leny = (m_dataSize.y+1);
      lenx2 = lenx/lod;
      leny2 = leny/lod;
      plod = 1;
    }


  glUniform1iARB(m_defaultParm[15], gridx);
  glUniform1iARB(m_defaultParm[16], lenx2);
  glUniform1iARB(m_defaultParm[17], leny2);
  glUniform1fARB(m_defaultParm[23], plod); // prunelod
  glUniform1iARB(m_defaultParm[28], lod);
}

void
DrawHiresVolume::setRenderDefault()
{
  Vec voxelScaling = Global::voxelScaling();
  Vec bpos = VECPRODUCT(m_bricks->getTranslation(0), voxelScaling);
  Vec bpivot = VECPRODUCT(m_bricks->getPivot(0),voxelScaling);
  Vec baxis = m_bricks->getAxis(0);
  float bangle = m_bricks->getAngle(0);
  Quaternion q(baxis, -DEG2RAD(bangle)); 

  glUseProgramObjectARB(m_defaultShader);

  glUniform1iARB(m_defaultParm[0], 0); // lutTex
  glUniform1iARB(m_defaultParm[1], 1); // dataTex
  glUniform1iARB(m_defaultParm[54], 1); // dataTexAT

  {
    QList<Vec> cPos;
    QList<Vec> cNorm;
    cPos += GeometryObjects::clipplanes()->positions();
    cNorm += GeometryObjects::clipplanes()->normals();
    int nclip = qMin(10,cPos.count()); // max 10 clip planes allowed
    float cpos[100];
    float cnormal[100];
    for(int c=0; c<nclip; c++)
      {
	cpos[3*c+0] = cPos[c].x*voxelScaling.x;
	cpos[3*c+1] = cPos[c].y*voxelScaling.y;
	cpos[3*c+2] = cPos[c].z*voxelScaling.z;
      }
    for(int c=0; c<nclip; c++)
      {
	cnormal[3*c+0] = -cNorm[c].x;
	cnormal[3*c+1] = -cNorm[c].y;
	cnormal[3*c+2] = -cNorm[c].z;
      }
    glUniform1i(m_defaultParm[55], nclip); // clipplanes
    glUniform3fv(m_defaultParm[56], nclip, cpos); // clipplanes
    glUniform3fv(m_defaultParm[57], nclip, cnormal); // clipplanes
  }
  
  if (Global::volumeType() != Global::DummyVolume)
    {
      glDisable(GL_CLIP_DISTANCE0);
      glDisable(GL_CLIP_DISTANCE1);
      glUniform4fARB(m_vertParm[0], 0, 1, 0, 10000);
      glUniform4fARB(m_vertParm[1], 1, 0, 0, 10000);

      //------ prune information ---------
      int dtexX, dtexY;
      m_Volume->getDragTextureSize(dtexX, dtexY);
      int svsl = m_Volume->getSubvolumeSubsamplingLevel();
      Vec dragInfo = m_Volume->getDragTextureInfo();
      int pgridx = dragInfo.x;
      int pgridy = dragInfo.y;
      float plod = dragInfo.z/float(svsl);
      int ptsizex = dtexX/pgridx;
      int ptsizey = dtexY/pgridy;

      if (m_drawImageType == Enums::DragImage)
	plod = 1;
      
      glUniform1iARB(m_defaultParm[19], 6); // pruneTex
      glUniform1iARB(m_defaultParm[20], pgridx); // prunegridx
      glUniform1iARB(m_defaultParm[21], ptsizex); // prunetsizex
      glUniform1iARB(m_defaultParm[22], ptsizey); // prunetsizey
      glUniform1fARB(m_defaultParm[23], plod); // prunelod
      glUniform1iARB(m_defaultParm[24], 0); // zoffset
      //----------------------------------
    }

  if (Global::volumeType() != Global::DummyVolume)
    {
      Vec textureSize = m_Volume->getSubvolumeTextureSize();
      Vec delta = Vec(1.0/textureSize.x,
		      1.0/textureSize.y,
		      1.0/textureSize.z);
      Vec eyepos = m_Viewer->camera()->position();

      eyepos -= bpos;
      eyepos -= bpivot;
      eyepos = q.rotate(eyepos);
      eyepos += bpivot;
      
      glUniform3fARB(m_defaultParm[4], delta.x, delta.y, delta.z);
      glUniform3fARB(m_defaultParm[5], eyepos.x, eyepos.y, eyepos.z);
      glUniform3fARB(m_defaultParm[6], m_lightPosition.x,
		                       m_lightPosition.y,
		                       m_lightPosition.z);

      glUniform1fARB(m_defaultParm[7], m_lightInfo.highlights.ambient);
      glUniform1fARB(m_defaultParm[8], m_lightInfo.highlights.diffuse);
      glUniform1fARB(m_defaultParm[9], m_lightInfo.highlights.specular);
      glUniform1fARB(m_defaultParm[10], (int)pow((float)2, (float)(m_lightInfo.highlights.specularCoefficient)));

      bool useAllTex = (m_drawImageType != Enums::DragImage ||
			m_dataTexSize == 1);
      setShader2DTextureParameter(useAllTex, true);

      glUniform1iARB(m_defaultParm[14], 5);

      glUniform2fARB(m_defaultParm[25], m_dataMin.x, m_dataMin.y);
      glUniform2fARB(m_defaultParm[26], m_dataSize.x, m_dataSize.y);
      if (m_drawImageType == Enums::DragImage &&
	  m_dataTexSize != 1)
	glUniform1iARB(m_defaultParm[27], m_dataMin.z);
    }

  {
    Vec front = m_Viewer->camera()->viewDirection();
    Vec up = m_Viewer->camera()->upVector();
    Vec right = m_Viewer->camera()->rightVector();
    front = q.rotate(front);
    up = q.rotate(up);
    right = q.rotate(right);
    glUniform3fARB(m_defaultParm[29], front.x, front.y, front.z);
    glUniform3fARB(m_defaultParm[30], up.x, up.y, up.z);
    glUniform3fARB(m_defaultParm[31], right.x, right.y, right.z);
  }

  glUniform1fARB(m_defaultParm[32], m_interpVol);
  glUniform1iARB(m_defaultParm[33], m_mixTag);

  glUniform1iARB(m_defaultParm[34], 4); // lightTex
  //if (!LightHandler::basicLight() && !m_useScreenShadows)
  if (!LightHandler::basicLight())
    {
      int lightgridx, lightgridy, lightgridz, lightncols, lightnrows, lightlod;
      LightHandler::lightBufferInfo(lightgridx, lightgridy, lightgridz,
				    lightnrows, lightncols, lightlod);
      glUniform1iARB(m_defaultParm[35], lightgridx); // lightgridx
      glUniform1iARB(m_defaultParm[36], lightgridy); // lightgridy
      glUniform1iARB(m_defaultParm[37], lightgridz); // lightgridz
      glUniform1iARB(m_defaultParm[38], lightnrows); // lightnrows
      glUniform1iARB(m_defaultParm[39], lightncols); // lightncols
      glUniform1iARB(m_defaultParm[40], lightlod); // lightlod
    }
  else
    {
      // lightlod 0 means use basic lighting model
      int lightlod = 0;
      glUniform1iARB(m_defaultParm[40], lightlod); // lightlod
    }


  if (Global::volumeType() == Global::RGBVolume ||
      Global::volumeType() == Global::RGBAVolume)
    {
      float frc = Global::stepsizeStill();
      glUniform1fARB(m_defaultParm[3], frc);
    }  

  if (m_drawImageType != Enums::DragImage)
    {
      Vec vsize = m_Volume->getSubvolumeTextureSize();
      glUniform3fARB(m_defaultParm[52], vsize.x, vsize.y, vsize.z);
    }
  else
    {
      Vec vsize = m_Volume->getDragSubvolumeTextureSize();
      glUniform3fARB(m_defaultParm[52], vsize.x, vsize.y, vsize.z);
    }

  glUniform3fARB(m_defaultParm[53], m_dataMin.x, m_dataMin.y, m_dataMin.z);

}

void
DrawHiresVolume::drawDefault(Vec pn,
			     Vec minvert, Vec maxvert,
			     int layers, float stepsize)
{
  glEnable(GL_DEPTH_TEST);

  if (!m_forceBackToFront && !m_saveImage2Volume)
    {
      m_backlit = false; // going to render front to back
      glDepthFunc(GL_GEQUAL); // front to back rendering
      glClearDepth(0);
      glClear(GL_DEPTH_BUFFER_BIT);
    }
  else
    {
      m_backlit = true; // going to render back to front
      glDepthFunc(GL_LEQUAL); // back to front rendering
      glClearDepth(1);
      glClear(GL_DEPTH_BUFFER_BIT);
    }  

  if (m_Viewer->imageBuffer()->isBound() ||
      m_saveImage2Volume ||
      m_useScreenShadows && !m_forceBackToFront)
    {
      if (! m_Viewer->imageBuffer()->isBound())
	m_Viewer->imageBuffer()->bind();

      if (MainWindowUI::mainWindowUI()->actionFor3DTV->isChecked())
	{
	  if (Global::saveImageType() == Global::LeftImage)
	    {
	      glClear(GL_DEPTH_BUFFER_BIT);
	      glClearColor(0, 0, 0, 0);
	      glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
	      glClear(GL_COLOR_BUFFER_BIT);
	    }
	  else
	    {
	    }
	}
      else
	{
	  if (MainWindowUI::mainWindowUI()->actionCrosseye->isChecked() &&
	      Global::saveImageType() == Global::LeftImage)
	    {
	    }
	  else
	    {
	      glClear(GL_DEPTH_BUFFER_BIT);
	      glClearColor(0, 0, 0, 0);
	      glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
	      glClear(GL_COLOR_BUFFER_BIT);
	    }
	}


      // clear depth of field buffer
      glBindFramebuffer(GL_FRAMEBUFFER_EXT, m_dofBuffer);
      for(int fbn=0; fbn<3; fbn++)
	{
	  glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
				 GL_COLOR_ATTACHMENT0_EXT,
				 GL_TEXTURE_RECTANGLE_ARB,
				 m_dofTex[fbn],
				 0);
	  glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
	  glClearColor(0, 0, 0, 0);
	  glClear(GL_COLOR_BUFFER_BIT);
	}


      // clear shadow buffer
      glBindFramebuffer(GL_FRAMEBUFFER_EXT, m_shdBuffer);
      for(int fbn=0; fbn<2; fbn++)
	{
	  glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
				 GL_COLOR_ATTACHMENT0_EXT,
				 GL_TEXTURE_RECTANGLE_ARB,
				 m_shdTex[fbn],
				 0);
	  glClearColor(0, 0, 0, 0);
	  glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
	  glClear(GL_COLOR_BUFFER_BIT);
	}

      m_shdNum = 0;
      glActiveTexture(GL_TEXTURE3);
      glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_shdTex[m_shdNum]);
      glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
      glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
      glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

      glActiveTexture(GL_TEXTURE2);
      glEnable(GL_TEXTURE_RECTANGLE_ARB);
      glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_Viewer->imageBuffer()->texture());

      m_Viewer->imageBuffer()->bind();
      glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
    }
  

  enableTextureUnits();

  if (Global::volumeType() != Global::DummyVolume)
    setRenderDefault();

  glEnable(GL_BLEND);
  if (!m_backlit)
    glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE); // for frontlit volume
  else
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // back to front


  //-----------------------------
  check_MIP();
  //-----------------------------


  drawSlicesDefault(pn, minvert, maxvert,
		    layers, stepsize);

  glDisable(GL_BLEND);
  disableTextureUnits();  
  glUseProgramObjectARB(0);

  m_bricks->draw();
}

void
DrawHiresVolume::drawSlicesDefault(Vec pn, Vec minvert, Vec maxvert,
				   int layers, float stepsize)
{
  Vec voxelScaling = Global::voxelScaling();

  //----------------------------------------------------------------
  if (m_useScreenShadows)
    {	  
      glUniform1iARB(m_defaultParm[46], m_shadowLod);
      glUniform1iARB(m_defaultParm[47], 3);
    }
  else
    {	  
      glUniform1iARB(m_defaultParm[46], 0);
      glUniform1iARB(m_defaultParm[47], 3);
    }
  glUniform1fARB(m_defaultParm[48], 2.0-0.8*m_lightInfo.shadowIntensity);
  //----------------------------------------------------------------
  if (Global::interpolationType(Global::TextureInterpolation)) // linear
    glUniform1iARB(m_defaultParm[50], 1);
  else
    glUniform1iARB(m_defaultParm[50], 0);


  //----------------------------------------------------------------
  // for polygons generated from bricks
  for(int bno=0; bno<m_polygon.size(); bno++)
    delete m_polygon[bno];
  m_polygon.clear();  
  //----------------------------------------------------------------

  Vec step = stepsize*pn;
  Vec poStart;
  if (!m_backlit)
    poStart = maxvert;
  else
    poStart = maxvert+layers*step;

  GeometryObjects::trisets()->predraw(m_Viewer,
				      m_bricks->getMatrix(0),
				      pn,
				      false,
				      m_shadowWidth,
				      m_shadowHeight);

  //----------------------------------------------------------------
  QList<Vec> clipGeoPos;
  QList<Vec> clipGeoNormal;  
  getClipForMask(clipGeoPos, clipGeoNormal);

  GeometryObjects::pathgroups()->predraw(m_Viewer, m_backlit,
					 clipGeoPos,
					 clipGeoNormal,
					 m_crops);

  GeometryObjects::networks()->predraw(m_Viewer,
				       m_bricks->getMatrix(0),
				       pn,
				       clipGeoPos,
				       clipGeoNormal,
				       m_crops,
				       m_lightInfo.userLightVector);
  //----------------------------------------------------------------

  enableTextureUnits();
  glUseProgramObjectARB(m_defaultShader);

  //----------------------------------------------------------------

  //----------------------------------
  VolumeFileManager pFileManager;
  int img2vol_nslices, img2vol_wd, img2vol_ht;  
  uchar *slice = 0;
  if (m_saveImage2Volume)
    {
      Vec dmin = VECPRODUCT(m_dataMin, voxelScaling);
      Vec p0 = m_Viewer->camera()->projectedCoordinatesOf(dmin);
      Vec p1 = p0 + Vec(10,0,0);
      Vec drt = m_Viewer->camera()->unprojectedCoordinatesOf(p1);
      float dlen = (dmin-drt).norm();
      float depthres = 10*step.norm()/dlen;

      // modify layers and step
      layers *= depthres;
      step /= depthres;

      img2vol_nslices = layers;
      img2vol_wd = m_Viewer->camera()->screenWidth();
      img2vol_ht = m_Viewer->camera()->screenHeight();

      slice = new uchar[img2vol_wd*img2vol_ht];
      saveReslicedVolume(m_image2VolumeFile,
			 img2vol_nslices, img2vol_wd, img2vol_ht, pFileManager,
			 false, Vec(1,1,1), 0);
    }
  //----------------------------------


  QList<int> tfSet;
  QList<float> tfSetF;

  tfSet = getSlices(poStart, step, pn, layers);

  for (int s=0; s<tfSet.count(); s++)
    tfSetF.append((float)tfSet[s]/(float)Global::lutSize());


  int slabstart, slabend;
  if (Global::volumeType() != Global::DummyVolume)
    {
      if (m_drawImageType != Enums::DragImage &&
	  m_dataTexSize > 1)
	{
	  slabstart = 1;
	  slabend = m_dataTexSize;
	}
      else
	{
	  slabstart = 0;
	  slabend = 1;
	}
    }

  Vec dragTexsize;
  int lenx2, leny2, lod;
  if (Global::volumeType() != Global::DummyVolume)
    getDragRenderInfo(dragTexsize, lenx2, leny2, lod);


  //-- adjust light position based on brick transforms
  int numGhostBricks = m_bricks->ghostBricks().size();  
  double XformInv[16];
  QList<Vec> lpos;
  for (int bn=0; bn<m_numBricks; bn++)
    {
      Vec lp = m_lightPosition;
      if (bn >= numGhostBricks)
	{
	  int bo = bn - numGhostBricks + 1;
	  memcpy(XformInv, m_bricks->getMatrixInv(bo), 16*sizeof(double));
	}
      else
	memcpy(XformInv, m_bricks->getMatrixInv(0), 16*sizeof(double));
      lp = Matrix::xformVec(XformInv, lp);
      lpos.append(lp);
    }
  //--

  emptySpaceSkip();

  //-------------------------------
  //-- for depthcue calculation --
  Vec cpos = m_Viewer->camera()->position();
  float deplen = (m_Viewer->camera()->sceneCenter() - maxvert).norm();
  if (deplen < m_Viewer->camera()->sceneRadius())
    deplen += m_Viewer->camera()->sceneRadius();
  else
    deplen = 2*m_Viewer->camera()->sceneRadius();
  //-------------------------------

  //------------
  glUniform1i(m_defaultParm[55], 0); // no clip for viewport
  //------------


  //-------------------------------
  if (Global::volumeType() != Global::DummyVolume)
    {
      drawClipPlaneInViewport(m_numBricks*layers,
			      lpos[0],
			      1.0,
			      lenx2, leny2, lod,
			      dragTexsize,
			      true); // defaultShader

      drawPathInViewport(m_numBricks*layers,
			 lpos[0],
			 1.0,
			 lenx2, leny2, lod,
			 dragTexsize,
			 true); // defaultShader
      
      
      // if viewport occupies full screen do not render any further
      if (GeometryObjects::clipplanes()->viewportsVisible())
	{
	  ClipInformation clipInfo = GeometryObjects::clipplanes()->clipInfo();
	  for (int ic=0; ic<clipInfo.viewport.count(); ic++)
	    {
	      QVector4D vp = clipInfo.viewport[ic];
	      // render only when textured plane and viewport active
	      if (clipInfo.tfSet[ic] >= 0 &&
		  clipInfo.tfSet[ic] < Global::lutSize() &&
		  vp.x() >= 0.0)
		{
		  if (vp.z() > 0.97 && vp.w() > 0.97)
		    return;
		}
	    }
	  
	  // some shader parameters might have changed so reset them
	  setRenderDefault();
	}
      
      if (GeometryObjects::paths()->viewportsVisible())
	setRenderDefault();
    }
  //-------------------------------


  bool validTF = false;
  for (int bno=0; bno<m_numBricks; bno++)
    validTF |= (tfSet[bno] < Global::lutSize());
  if (!validTF)
    {
      renderGeometry(0, layers,
		     poStart, pn, layers*step,
		     false, false, Vec(0,0,0),
		     false);

      enableTextureUnits();
      glUseProgramObjectARB(m_defaultShader);
      drawClipPlaneDefault(0, layers,
			   poStart, pn, layers*step,
			   m_numBricks*layers,
			   lpos[0],
			   1.0,
			   slabstart, slabend,
			   lenx2, leny2, lod,
			   dragTexsize);
      layers = 0;
    }


  int ScreenXMin, ScreenXMax, ScreenYMin, ScreenYMax;
  ScreenXMin = ScreenYMin = 100000;
  ScreenXMax = ScreenYMax = 0;

  int shadowRenderSteps = qMax(1, (int)(1.0/stepsize));

  Vec pnDir = step;
  if (m_backlit)
    pnDir = -step;
  int pno = 0;
  Vec po = poStart;

  //--------------depth of field--------
  int dofSlice, maxDof;
  if (m_dofBlur > 0)
    {
      dofSlice = (m_focalPoint-0.1)*layers;
      maxDof = qMax(dofSlice,layers-dofSlice);
    }
  //------------------------------------

  //------------------------------------
  // restore clip
  int nclip = qMin(10,GeometryObjects::clipplanes()->positions().count()); // max 10 clip planes allowed
  glUniform1i(m_defaultParm[55], nclip); // restore clip
  //------------------------------------

  
  for(int s=0; s<layers; s++)
    {

      po += pnDir;

      int SlcXMin, SlcXMax, SlcYMin, SlcYMax;
      SlcXMin = SlcYMin = 100000;
      SlcXMax = SlcYMax = 0;

      // set depth of field
      float tap = 0;
      if (m_dofBlur > 0)
	{
	  tap = (float)qAbs(dofSlice-s)/(float)maxDof;
	  tap *= tap;
	  tap *= m_dofBlur;
	  if (tap < 1.0) tap = 0;
	}
      glUniform1fARB(m_defaultParm[51], qMax(1.0f, tap));

      
      // generate opacity modulation
      float sdist = qAbs((maxvert - po)*pn);
      float modop = StaticFunctions::smoothstep(0, 1, sdist/deplen);
      modop = m_frontOpMod*(1-modop) + modop*m_backOpMod;
      modop = qBound(0.0f, modop, 1.0f);


      float depthcue = 1;     
      if (Global::depthcue())
	{
	  float sdist = qAbs((maxvert - po)*pn);
	  depthcue = 1.0 - qBound(0.0f, sdist/deplen, 0.95f);
	}

      if (m_drawImageType != Enums::DragImage)
	{
	  if (tap > 0)
	    glViewport(0,0, m_shadowWidth/tap, m_shadowHeight/tap);
	  else
	    {
	      //-------------------------------------------
	      // restore viewport
	      if (MainWindowUI::mainWindowUI()->actionFor3DTV->isChecked() ||
		  MainWindowUI::mainWindowUI()->actionCrosseye->isChecked())
		{
		  if (MainWindowUI::mainWindowUI()->actionFor3DTV->isChecked())
		    {
		      if (Global::saveImageType() == Global::LeftImage)
			glViewport(0,0, m_shadowWidth/2, m_shadowHeight);
		      else
			glViewport(m_shadowWidth/2,0, m_shadowWidth/2, m_shadowHeight);
		}
		  else
		    {
		      if (Global::saveImageType() == Global::LeftImage)
			glViewport(m_shadowWidth/2,0, m_shadowWidth/2, m_shadowHeight);
		      else
			glViewport(0,0, m_shadowWidth/2, m_shadowHeight);
		    }
		}
	    }
	  //-------------------------------------------
	  
	  glBindFramebuffer(GL_FRAMEBUFFER_EXT, m_dofBuffer);
	  glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
				 GL_COLOR_ATTACHMENT0_EXT,
				 GL_TEXTURE_RECTANGLE_ARB,
				 m_dofTex[0],
				 0);
	  glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);  
	  glClear(GL_COLOR_BUFFER_BIT);
	}
      //---------------------

      if (m_drawGeometryPresent &&
	  (s%Global::geoRenderSteps() == 0 || s == layers-1))
	{
	  renderGeometry(s, layers,
			 po, pn, Global::geoRenderSteps()*step,
			 false, false, Vec(0,0,0),
			 false);
	  glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE);
	  check_MIP();
	}		     
      //------------------------------------------------------

      if (Global::volumeType() != Global::DummyVolume)
	{
	  enableTextureUnits();
	  glUseProgramObjectARB(m_defaultShader);
      
	  for (int bno=0; bno<m_numBricks; bno++)
	    {
	      Vec lp = lpos[bno];
	      glUniform3fARB(m_defaultParm[6], lp.x, lp.y, lp.z);

	      glUniform1fARB(m_defaultParm[49], modop);

	      QList<bool> clips;
	      int tfset;
	      Vec subvol[8],texture[8], subcorner, subdim;
	      Vec brickPivot, brickScale;
	      
	      m_drawBrickInformation.get(bno, 
					 tfset,
					 subvol, texture,
					 subcorner, subdim,
					 clips,
					 brickPivot, brickScale);
	  
	      {
		QList<Vec> cPos1;
		QList<Vec> cNorm1;
		cPos1 += GeometryObjects::clipplanes()->positions();
		cNorm1 += GeometryObjects::clipplanes()->normals();
		QList<Vec> cPos;
		QList<Vec> cNorm;
		for(int c=0; c<clips.count(); c++)
		  {
		    if (clips[c])
		      {
			cPos << cPos1[c];
			cNorm << cNorm1[c];
		      }
		  }
		int nclip = qMin(10,cPos.count()); // max 10 clip planes allowed
		float cpos[100];
		float cnormal[100];
		for(int c=0; c<nclip; c++)
		  {
		    cpos[3*c+0] = cPos[c].x*voxelScaling.x;
		    cpos[3*c+1] = cPos[c].y*voxelScaling.y;
		    cpos[3*c+2] = cPos[c].z*voxelScaling.z;
		  }
		for(int c=0; c<nclip; c++)
		  {
		    cnormal[3*c+0] = -cNorm[c].x;
		    cnormal[3*c+1] = -cNorm[c].y;
		    cnormal[3*c+2] = -cNorm[c].z;
		  }
		glUniform1i(m_defaultParm[55], nclip); // clipplanes
		glUniform3fv(m_defaultParm[56], nclip, cpos); // clipplanes
		glUniform3fv(m_defaultParm[57], nclip, cnormal); // clipplanes
	      }

	      ViewAlignedPolygon *vap = m_polygon[pno];
	      pno++;
	  
	      //---------------------------------
	      for(int pi=0; pi<vap->edges; pi++)
		{  
		  Vec scr = m_Viewer->camera()->projectedCoordinatesOf(vap->vertex[pi]);
		  scr.y = m_shadowHeight - scr.y;
		  SlcXMin = qMin(SlcXMin, (int)scr.x);
		  SlcXMax = qMax(SlcXMax, (int)scr.x);
		  SlcYMin = qMin(SlcYMin, (int)scr.y);
		  SlcYMax = qMax(SlcYMax, (int)scr.y);
		}
	      for(int pi=0; pi<vap->edges; pi++)
		{  
		  Vec scr = m_Viewer->camera()->projectedCoordinatesOf(vap->vertex[pi]);
		  scr.y = m_shadowHeight - scr.y;
		  ScreenXMin = qMin(ScreenXMin, (int)scr.x);
		  ScreenXMax = qMax(ScreenXMax, (int)scr.x);
		  ScreenYMin = qMin(ScreenYMin, (int)scr.y);
		  ScreenYMax = qMax(ScreenYMax, (int)scr.y);
		}
	      //--------------------------------

	      if (tfSet[bno] < Global::lutSize())
		{
		  if (Global::volumeType() != Global::RGBVolume &&
		      Global::volumeType() != Global::RGBAVolume)
		    glUniform1fARB(m_defaultParm[3], tfSetF[bno]);
		  
		  glUniform1fARB(m_defaultParm[18], depthcue);

		  for(int b=slabstart; b<slabend; b++)
		  {
		      float tminz = m_dataMin.z;
		      float tmaxz = m_dataMax.z;
		      if (slabend > 1)
			{
			  tminz = m_textureSlab[b].y;
			  tmaxz = m_textureSlab[b].z;
			}
		      glUniform3fARB(m_defaultParm[42], m_dataMin.x, m_dataMin.y, tminz);
		      glUniform3fARB(m_defaultParm[43], m_dataMax.x, m_dataMax.y, tmaxz);

		      glUniform3fARB(m_defaultParm[53], m_dataMin.x, m_dataMin.y, tminz);

		      bindDataTextures(b);
		      
		      //-------------------------
		      if (b == 0)
			renderDragSlice(vap, false, dragTexsize);
		      else
			renderSlicedSlice(0,
					  vap,
					  false,
					  lenx2, leny2, b, lod);
			
		      
		      releaseDataTextures(b);
		      //-------------------------
		      
		    } // loop over b
		}
	    } // loop over bricks


	  //--- for drawing clip planes if any
	  drawClipPlaneDefault(s, layers,
			       po, pn, step,
			       m_numBricks*layers,
			       lpos[0],
			       depthcue,
			       slabstart, slabend,
			       lenx2, leny2, lod,
			       dragTexsize);

  
	} // not DummyVolume
      //------------------------------------------------------

      if (m_saveImage2Volume)
	{
	  glReadPixels(0, 0, img2vol_wd, img2vol_ht, GL_ALPHA, GL_UNSIGNED_BYTE, slice);
	  pFileManager.setSlice(s, slice);
	  glClear(GL_COLOR_BUFFER_BIT);
	  //glClear(GL_DEPTH_BUFFER_BIT);
	}

      //-------------------------------------------
      // copy to imageBuffer
      if (m_drawImageType != Enums::DragImage)
	{
	  int xmin = 0;
	  int ymin = 0;
	  int xmax = m_shadowWidth/qMax(1.0f,tap);
	  int ymax = m_shadowHeight/qMax(1.0f,tap);

	  if (tap > 0)
	    depthOfFieldBlur(xmin, xmax, ymin, ymax, (int)tap);

	  StaticFunctions::pushOrthoView(0, 0, m_shadowWidth, m_shadowHeight);
	  m_Viewer->imageBuffer()->bind();
	  glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);

	  glActiveTexture(GL_TEXTURE2);
	  glEnable(GL_TEXTURE_RECTANGLE_ARB);
	  if (tap < 1.0)
	    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_dofTex[0]);
	  else
	    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_dofTex[1]);

	  glUseProgramObjectARB(Global::copyShader());
	  glUniform1iARB(Global::copyParm(0), 2);
	  if (tap > 0) // copy from dofTex[1] into viewerbuffer
	    StaticFunctions::drawQuad(0, 0,
				      m_shadowWidth, m_shadowHeight, 1.0/tap);
	  else // copy from dofTex[0] into viewerbuffer
	    StaticFunctions::drawQuad(0, 0,
				      m_shadowWidth, m_shadowHeight, 1);

	  StaticFunctions::popOrthoView();
	}
      //-------------------------------------------

      //-------------------------------------------
      // update shadow buffer
      if (m_useScreenShadows &&
	  !m_forceBackToFront &&
	  s > 0 &&
	  s%shadowRenderSteps == 0)
	{
	  glDisable(GL_BLEND);
	  screenShadow(ScreenXMin, ScreenXMax, ScreenYMin, ScreenYMax, qMax(1.0f,tap));
	  glEnable(GL_BLEND);
	}
      //-------------------------------------------

    } // loop over s


  if (m_drawImageType != Enums::DragImage)
    {
      m_Viewer->imageBuffer()->bind();
      glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
    }

  if (m_saveImage2Volume)
    {
      m_saveImage2Volume = false;
      pFileManager.closeQFile();
      delete [] slice;
      QMessageBox::information(0, "Saved Image to Volume",
			       QString("image converted to 3D volume and saved to %1 and %1.001"). \
			       arg(m_image2VolumeFile));
    }

  //------------------------------------------------------
  glUseProgramObjectARB(0);
  disableTextureUnits();	  
  Vec bpos = VECPRODUCT(m_bricks->getTranslation(0), voxelScaling);
  Vec bpivot = VECPRODUCT(m_bricks->getPivot(0),voxelScaling);
  Vec baxis = m_bricks->getAxis(0);
  float bangle = m_bricks->getAngle(0);
  Quaternion q(baxis, -DEG2RAD(bangle));    
  Vec eyepos = m_Viewer->camera()->position();
  eyepos -= bpos;
  eyepos -= bpivot;
  eyepos = q.rotate(eyepos);
  eyepos += bpivot; 
  glEnable(GL_DEPTH_TEST);
  GeometryObjects::trisets()->draw(m_Viewer,
				   m_lightPosition,
				   0, 0, Vec(0,0,0),
				   false, false,
				   eyepos,
				   m_clipPos, m_clipNormal,
				   false);

  GeometryObjects::pathgroups()->draw(m_Viewer,
				      m_backlit,
				      m_lightPosition,
				      false);
  //------------------------------------------------------ 

  glUseProgramObjectARB(0);
  disableTextureUnits();	  
}

void
DrawHiresVolume::depthOfFieldBlur(int xmin, int xmax, int ymin, int ymax,
				  int ntimes)
{
  if (ntimes < 1) return;

  glDisable(GL_DEPTH_TEST);

  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ZERO); // replace texture

  StaticFunctions::pushOrthoView(0, 0, m_shadowWidth/ntimes, m_shadowHeight/ntimes);
  glBindFramebuffer(GL_FRAMEBUFFER_EXT, m_dofBuffer);
  glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
			 GL_COLOR_ATTACHMENT0_EXT,
			 GL_TEXTURE_RECTANGLE_ARB,
			 m_dofTex[1],
			 0);
  glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);  
  glClear(GL_COLOR_BUFFER_BIT);
	      
  glActiveTexture(GL_TEXTURE2);
  glEnable(GL_TEXTURE_RECTANGLE_ARB);
  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_dofTex[0]);
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,
		  GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,
		  GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glUseProgramObjectARB(Global::copyShader());
  glUniform1iARB(Global::copyParm(0), 2); // copy from dofTex[0] into dofTex[1]
  glBegin(GL_QUADS);
  glTexCoord2f(xmin, ymin); glVertex2f(xmin, ymin);
  glTexCoord2f(xmax, ymin); glVertex2f(xmax, ymin);
  glTexCoord2f(xmax, ymax); glVertex2f(xmax, ymax);
  glTexCoord2f(xmin, ymax); glVertex2f(xmin, ymax);
  glEnd();

  glUseProgramObjectARB(m_blurShader);
  glUniform1iARB(m_blurParm[0], 2); // copy from imageBuffer into sliceTex[0]

  int btype0[] = {1, 1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 4};
  int btype1[] = {0, 1, 0, 1, 0, 1, 2, 0, 1, 2, 3, 4};
 
  for(int i=0; i<2; i++)
    {
      if (i == 0)
	glUniform1iARB(m_blurParm[2], btype0[ntimes-1]); // type
      else
	{
	  glUniform1iARB(m_blurParm[2], btype1[ntimes-1]); // type
	  if (btype1[ntimes-1] == 0)
	    break;
	}

      for(int j=0; j<2; j++)
	{
	  glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
				 GL_COLOR_ATTACHMENT0_EXT,
				 GL_TEXTURE_RECTANGLE_ARB,
				 m_dofTex[1+(j+1)%2],
				 0);
	  glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);  
	  glClear(GL_COLOR_BUFFER_BIT);
	  
	  glActiveTexture(GL_TEXTURE2);
	  glEnable(GL_TEXTURE_RECTANGLE_ARB);
	  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_dofTex[j+1]);
	  
	  if (j == 0)
	    glUniform2fARB(m_blurParm[1], 1.0, 0.0); // direc
	  else
	    glUniform2fARB(m_blurParm[1], 0.0, 1.0); // direc
	  
	  glBegin(GL_QUADS);
	  glTexCoord2f(xmin, ymin); glVertex2f(xmin, ymin);
	  glTexCoord2f(xmax, ymin); glVertex2f(xmax, ymin);
	  glTexCoord2f(xmax, ymax); glVertex2f(xmax, ymax);
	  glTexCoord2f(xmin, ymax); glVertex2f(xmin, ymax);
	  glEnd();
	  glFinish();
	}
    }
  glUseProgramObjectARB(0); // disable shaders 

  StaticFunctions::popOrthoView();

  glEnable(GL_BLEND);
  if (!m_backlit)
    glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE); // for frontlit volume
  else
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // back to front

  glEnable(GL_DEPTH_TEST);
}

void
DrawHiresVolume::screenShadow(int ScreenXMin, int ScreenXMax,
			      int ScreenYMin, int ScreenYMax,
			      float tap)
{
  StaticFunctions::pushOrthoView(0, 0, m_shadowWidth, m_shadowHeight);

  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ZERO); // replace texture in sliceTex[1]


  int xmin = qMax(0, ScreenXMin/m_shadowLod);
  int ymin = qMax(0, ScreenYMin/m_shadowLod);
  int xmax = qMin(m_shadowWidth/m_shadowLod, ScreenXMax/m_shadowLod);
  int ymax = qMin(m_shadowHeight/m_shadowLod, ScreenYMax/m_shadowLod);

  int txmin = qMax(0, ScreenXMin/m_shadowLod);
  int tymin = qMax(0, ScreenYMin/m_shadowLod);
  int txmax = qMin(m_shadowWidth/m_shadowLod, ScreenXMax/m_shadowLod);
  int tymax = qMin(m_shadowHeight/m_shadowLod, ScreenYMax/m_shadowLod);


  if (MainWindowUI::mainWindowUI()->actionFor3DTV->isChecked() ||
      MainWindowUI::mainWindowUI()->actionCrosseye->isChecked())
    {
      xmin = 0;
      ymin = 0;
      xmax = m_shadowWidth/m_shadowLod;
      ymax = m_shadowHeight/m_shadowLod;
      
      txmin = 0;
      tymin = 0;
      txmax = m_shadowWidth/m_shadowLod;
      tymax = m_shadowHeight/m_shadowLod;
    }

  // update shadow texture
  glBindFramebuffer(GL_FRAMEBUFFER_EXT, m_shdBuffer);
  glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
			 GL_COLOR_ATTACHMENT0_EXT,
			 GL_TEXTURE_RECTANGLE_ARB,
			 m_shdTex[m_shdNum],
			 0);
  glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);  

  glActiveTexture(GL_TEXTURE2);
  glEnable(GL_TEXTURE_RECTANGLE_ARB);
  if (m_drawImageType != Enums::DragImage)
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_dofTex[0]);
  else
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_Viewer->imageBuffer()->texture());

  //------------------------------------------------------------
  // blend slice using front to back blending into shadowbuffer
  glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE);
  glUseProgramObjectARB(Global::copyShader());
  glUniform1iARB(Global::copyParm(0), 2); // copy from sliceTex[1] to shadowbuffer
  glBegin(GL_QUADS);
  glTexCoord2f(txmin/tap, tymin/tap); glVertex2f(xmin, ymin);
  glTexCoord2f(txmax/tap, tymin/tap); glVertex2f(xmax, ymin);
  glTexCoord2f(txmax/tap, tymax/tap); glVertex2f(xmax, ymax);
  glTexCoord2f(txmin/tap, tymax/tap); glVertex2f(xmin, ymax);
  glEnd();


  //-------------------------------
  // smooth shadow into other shadowbuffer
  glBlendFunc(GL_ONE, GL_ZERO); // replace texture

  glUseProgramObjectARB(m_blurShader);
  glUniform1iARB(m_blurParm[0], 3); // copy from shadowBuffer[0] to shadowbuffer[1]
  
  int nblur = m_lightInfo.shadowBlur;
  int nit = 2;
  if (nblur > 4) nit = 3;
  nit *= m_imgSizeRatio;
  for(int i=0; i<nit; i++)
    {
      glUniform1iARB(m_blurParm[2], qBound(1, nblur, 4)); // type
      for(int j=0; j<2; j++)
	{
	  glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
				 GL_COLOR_ATTACHMENT0_EXT,
				 GL_TEXTURE_RECTANGLE_ARB,
				 m_shdTex[(m_shdNum+1)%2],
				 0);
	  glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);  
	  
	  if (j == 0)
	    glUniform2fARB(m_blurParm[1], 1.0, 0.0); // direc
	  else
	    glUniform2fARB(m_blurParm[1], 0.0, 1.0); // direc
	  
	  glActiveTexture(GL_TEXTURE3);
	  glEnable(GL_TEXTURE_RECTANGLE_ARB);
	  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_shdTex[m_shdNum]);
	  
	  glBegin(GL_QUADS);
	  glTexCoord2f(txmin, tymin); glVertex2f(xmin, ymin);
	  glTexCoord2f(txmax, tymin); glVertex2f(xmax, ymin);
	  glTexCoord2f(txmax, tymax); glVertex2f(xmax, ymax);
	  glTexCoord2f(txmin, tymax); glVertex2f(xmin, ymax);
	  glEnd();
	  
	  m_shdNum = (m_shdNum+1)%2;
	}
    }

  //-------------------------------



  glDisable(GL_BLEND);
  if (!m_backlit)
    glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE); // for frontlit volume
  else
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // back to front

  check_MIP();

  glUseProgramObjectARB(0); // disable shaders 
  StaticFunctions::popOrthoView();
  
  //-------------------------------------------
  // render to imageBuffer
  m_Viewer->imageBuffer()->bind();
  glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);

  // rebind imageBuffer->texture
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_Viewer->imageBuffer()->texture());
  //-------------------------------------------

  //-------------------------------------------
  // choose other shadow texture
  glActiveTexture(GL_TEXTURE3);
  glEnable(GL_TEXTURE_RECTANGLE_ARB);
  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_shdTex[(m_shdNum+1)%2]);
  // next time choose the other shadow texture
  m_shdNum = (m_shdNum+1)%2;
  //-------------------------------------------
  

  //-------------------------------------------
  // restore viewport
  int camWidth = m_Viewer->camera()->screenWidth();
  int camHeight = m_Viewer->camera()->screenHeight();
  if (MainWindowUI::mainWindowUI()->actionFor3DTV->isChecked())
    {
      if (Global::saveImageType() == Global::LeftImage)
	glViewport(0,0, camWidth/2, camHeight);
      else
	glViewport(camWidth/2,0, camWidth/2, camHeight);
    }
  else
    {
      if (MainWindowUI::mainWindowUI()->actionCrosseye->isChecked() &&
	  Global::saveImageType() == Global::LeftImage)
	glViewport(camWidth,0, camWidth, camHeight);
      else
	glViewport(0,0, camWidth, camHeight);
    }
  //-------------------------------------------
  

  //-------------------------------------------
  // restore shader
  glUseProgramObjectARB(m_defaultShader);
  //-------------------------------------------
}


void
DrawHiresVolume::clipSlab(Vec vp0, Vec vp1,
			  int &edges,
			  Vec *poly,
			  Vec *tex,
			  Vec *sha)
{
  //---- apply clipping
  for(int ci=0; ci<2; ci++)
    {
      Vec cpo = vp0;
      Vec cpn = Vec(0,0,-1);
      if (ci == 1)
	{
	  cpo = vp1;
	  cpn = Vec(0,0,1);
	}
      
      int tedges = edges;
      Vec tpoly[100];
      Vec ttex[100];
      Vec tsha[100];
      for(int pi=0; pi<tedges; pi++) tpoly[pi] = poly[pi];
      for(int pi=0; pi<tedges; pi++) ttex[pi] = tex[pi];
      if (sha)
	for(int pi=0; pi<tedges; pi++) tsha[pi] = sha[pi];
      
      edges = 0;
      for(int pi=0; pi<tedges; pi++)
	{
	  Vec v0, v1, t0, t1, s0, s1;
	  
	  v0 = tpoly[pi];
	  t0 = ttex[pi];
	  s0 = tsha[pi];
	  if (pi<tedges-1)
	    {
	      v1 = tpoly[pi+1];
	      t1 = ttex[pi+1];
	      s1 = tsha[pi+1];
	    }
	  else
	    {
	      v1 = tpoly[0];
	      t1 = ttex[0];
	      s1 = tsha[0];
	    }
	  
	  // clip on texture coordinates
	  int ret = 0;
	  if (!sha) 
	    ret = StaticFunctions::intersectType2WithTexture(cpo, cpn,
							      t0, t1,
							      v0, v1);
	  else
	    ret = StaticFunctions::intersectType3WithTexture(cpo, cpn,
							      t0, t1,
							      v0, v1,
							      s0, s1);

	  if (ret)
	    {
	      poly[edges] = v0;
	      tex[edges] = t0;
	      if (sha) sha[edges] = s0;
	      edges ++;
	      if (ret == 2)
		{
		  poly[edges] = v1;
		  tex[edges] = t1;
		  if (sha) sha[edges] = s1;
		  edges ++;
		}
	    }
	} // loop over edges
    } // loop over ci
  //---- clipping applied
}

QList<int>
DrawHiresVolume::getSlices(Vec poStart,
			   Vec step,
			   Vec pn,
			   int layers)
{
  m_clipPos = GeometryObjects::clipplanes()->positions();
  m_clipNormal = GeometryObjects::clipplanes()->normals();

  QList<int> tfset = GeometryObjects::clipplanes()->tfset();
  QList<bool> applyclip = GeometryObjects::clipplanes()->applyClip();
  Vec voxelScaling = Global::voxelScaling();

  QList<int> tfSet;
  QList< QList<bool> > clips;
  QList<Vec> subvol;
  QList<Vec> texture;

  for (int bno=0; bno<m_numBricks; bno++)
    {
      QList<bool> bclips;
      int tfset;
      Vec bsubvol[8],btexture[8], subcorner, subdim;
      Vec brickPivot, brickScale;
      
      m_drawBrickInformation.get(bno, 
				 tfset,
				 bsubvol, btexture,
				 subcorner, subdim,
				 bclips,
				 brickPivot, brickScale);
      tfSet.append(tfset);

      // bclip and applyclip should have same number of clip planes
      if (applyclip.count() > bclips.count())
	for(int ci=bclips.count(); ci<applyclip.count(); ci++)
	  bclips << false;

      // modify bclip to reflect applyclip
      for(int ci=0; ci<applyclip.count(); ci++)
	bclips[ci] = bclips[ci] && applyclip[ci];

      clips.append(bclips);

      for (int j=0; j<8; j++)
	subvol.append(bsubvol[j]);
      for (int j=0; j<8; j++)
	texture.append(btexture[j]);
    }

  Vec pnDir = step;
  if (m_backlit)
    pnDir = -step;
  Vec po = poStart;
  for(int s=0; s<layers; s++)
    { 
      po += pnDir;

      for (int bno=0; bno<m_numBricks; bno++)
	{
	  Vec bsubvol[8],btexture[8];

	  for (int j=0; j<8; j++)
	    bsubvol[j] = subvol[8*bno + j];
	  for (int j=0; j<8; j++)
	    btexture[j] = texture[8*bno + j];

	  ViewAlignedPolygon *vap = new ViewAlignedPolygon;
	  drawpoly(po, pn,
		   bsubvol, btexture,
		   vap);
	  m_polygon.append(vap);
	}
    }


  //-----------------------------------
  // for drawing clip planes
  m_clipPos = GeometryObjects::clipplanes()->positions();
  QList<bool> dummyclips;
  Vec bsubvol[8],btexture[8];
  for (int j=0; j<8; j++) bsubvol[j] = subvol[j];
  for (int j=0; j<8; j++) btexture[j] = texture[j];

  // undo brick-0 transformation on clip normal before slicing
  double XformI[16];
  memcpy(XformI, m_bricks->getMatrixInv(), 16*sizeof(double));

  for (int ic=0; ic<m_clipPos.count(); ic++)
    {
      Vec cnorm = m_clipNormal[ic];
      cnorm = Matrix::rotateVec(XformI, cnorm);      

      Vec cpos = VECPRODUCT(m_clipPos[ic], voxelScaling);

      ViewAlignedPolygon *vap = new ViewAlignedPolygon;
      drawpoly(cpos, cnorm,
	       bsubvol, btexture,
	       vap);
      m_polygon.append(vap);
    }
  //-----------------------------------

  return tfSet;
}

void
DrawHiresVolume::emptySpaceSkip()
{
  if (Global::volumeType() == Global::DummyVolume)
    return;

  if (Global::emptySpaceSkip())
    {
      glActiveTexture(GL_TEXTURE6);
      glEnable(GL_TEXTURE_RECTANGLE_ARB);
      glBindTexture(GL_TEXTURE_RECTANGLE_ARB, PruneHandler::texture());
    }

  glActiveTexture(GL_TEXTURE4);
  glEnable(GL_TEXTURE_RECTANGLE_ARB);
  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, LightHandler::texture());
}

void
DrawHiresVolume::releaseDataTextures(int b)
{
  GLuint target;
  
//  if (b == 0) // drag volume texture
//    {
//      glActiveTexture(GL_TEXTURE1);
//      target = GL_TEXTURE_RECTANGLE_EXT;
//    }
//  else // sub volume texture
    {
      glActiveTexture(GL_TEXTURE1);
      target = GL_TEXTURE_2D_ARRAY;
    }

  glDisable(target);
}

void
DrawHiresVolume::bindDataTextures(int b)
{
  GLuint target;
  
//  if (b == 0) // drag volume texture
//    {
//      glActiveTexture(GL_TEXTURE1);
//      target = GL_TEXTURE_RECTANGLE_EXT;
//    }
//  else // sub volume texture
    {
      glActiveTexture(GL_TEXTURE1);
      target = GL_TEXTURE_2D_ARRAY;
    }

  glEnable(target);
  glBindTexture(target, m_dataTex[b]);
  
  if (Global::interpolationType(Global::TextureInterpolation)) // linear
    {
      glTexParameteri(target,
		      GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(target,
		      GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
  else
    {
      glTexParameteri(target,
		      GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(target,
		      GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
}

void
DrawHiresVolume::renderDragSlice(ViewAlignedPolygon *vap,
				 bool shadow,
				 Vec dragTexsize)
{
  if (vap->edges == 0)
    return;

  glBegin(GL_POLYGON);
  for(int pi=0; pi<vap->edges; pi++)
    {
      Vec tex = vap->texcoord[pi];
      Vec otex = tex;
      glMultiTexCoord3f(GL_TEXTURE0, otex.x, otex.y, otex.z);
      if (shadow) glMultiTexCoord2f(GL_TEXTURE1, vap->tx[pi], vap->ty[pi]);
      glVertex3f(vap->vertex[pi].x, vap->vertex[pi].y, vap->vertex[pi].z);
    }
  glEnd();
}

void
DrawHiresVolume::renderSlicedSlice(int type, 
				   ViewAlignedPolygon *vap,
				   bool shadow,
				   int lenx2, int leny2, int b, int lod)
{
  Vec poly[100];
  Vec tex[100];
  Vec sha[100];
  int edges = vap->edges;
  for(int pi=0; pi<edges; pi++) poly[pi] = vap->vertex[pi];
  for(int pi=0; pi<edges; pi++) tex[pi] = vap->texcoord[pi];		      
  if (shadow)
    {
      for(int pi=0; pi<edges; pi++)
	sha[pi] = Vec(vap->tx[pi], vap->ty[pi], 0);
    }
		      
  int tminz = m_textureSlab[b].y;
  int tmaxz = m_textureSlab[b].z;
  
  if (m_dataTexSize > 1)
    glUniform1iARB(m_defaultParm[24], (tminz-m_dataMin.z)/lod); // zoffset
			  
  Vec vp0 = Vec(0, 0, tminz-lod);
  Vec vp1 = Vec(0, 0, tmaxz+lod);
			  
  // using array texture
  clipSlab(vp0, vp1, edges, poly, tex, sha);
			  
  if (edges == 0)
    return;

  glUniform1iARB(m_defaultParm[27], tminz);

  glBegin(GL_POLYGON);
  for(int pi=0; pi<edges; pi++)
    {
      Vec otex = tex[pi];
      glMultiTexCoord3dv(GL_TEXTURE0, otex);
      if (shadow) glMultiTexCoord2dv(GL_TEXTURE1, sha[pi]);
      glVertex3dv(poly[pi]);
    }
  glEnd();
}

void
DrawHiresVolume::getDragRenderInfo(Vec &dragTexsize,
				   int &lenx2, int &leny2,
				   int &lod)
{
  if (Global::volumeType() != Global::DummyVolume)
    {
      Vec draginfo = m_Volume->getDragTextureInfo();
      int dlod= draginfo.z;
      int dx2 = (m_dataSize.x+1);
      int dy2 = (m_dataSize.y+1);
      int dz2 = (m_dataSize.z+1);
      dx2 /= dlod;
      dy2 /= dlod;
      dz2 /= dlod;
      dragTexsize = Vec(dx2, dy2, dz2);
      dragTexsize -= Vec(2,2,2);

      lod = m_Volume->getSubvolumeSubsamplingLevel();  
      int lenx = m_dataSize.x+1;
      int leny = m_dataSize.y+1;
      lenx2 = lenx/lod;
      leny2 = leny/lod;
    }
}

void
DrawHiresVolume::drawPathInViewport(int pathOffset, Vec lpos, float depthcue,
				    int lenx2, int leny2, int lod,
				    Vec dragTexsize,
				    bool defaultShader)
{
  if (GeometryObjects::paths()->count() == 0)
    return;

  if (! GeometryObjects::paths()->viewportsVisible())
    return; // no path to render in a viewport

  int slabstart, slabend;
  slabstart = 1;
  slabend = m_dataTexSize;
  if (m_dataTexSize == 1)
    {
      slabstart = 0;
      slabend = 1;
    }

  GLint *parm; 
  parm = m_defaultParm;

  if (slabstart > 0)
    {
      Vec vsize = m_Volume->getSubvolumeTextureSize();
      glUniform3fARB(m_defaultParm[52], vsize.x, vsize.y, vsize.z);
    }
  else
    {
      Vec vsize = m_Volume->getDragSubvolumeTextureSize();
      glUniform3fARB(m_defaultParm[52], vsize.x, vsize.y, vsize.z);
    }
  
  glUniform3fARB(m_defaultParm[53], m_dataMin.x, m_dataMin.y, m_dataMin.z);


  glUniform3fARB(parm[6], lpos.x, lpos.y, lpos.z);

  // lightlod 0 means use basic lighting model
  int lightlod = 0;
  glUniform1iARB(parm[40], lightlod); // lightlod

  // use only ambient component
  glUniform1fARB(parm[7], 1.0);
  glUniform1fARB(parm[8], 0.0);
  glUniform1fARB(parm[9], 0.0);

  if (slabend > 1)
    setShader2DTextureParameter(true, defaultShader);
  else
    setShader2DTextureParameter(false, defaultShader);
  
  int ow = m_Viewer->width();
  int oh = m_Viewer->height();

  m_Viewer->startScreenCoordinatesSystem();
  glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE); // for frontlit volume

  //Vec voxelSize = VolumeInformation::volumeInformation().voxelSize;
  Vec voxelSize = Global::voxelScaling();
  
  QList<PathObject> po;
  po = GeometryObjects::paths()->paths();
  int npaths = po.count();
  for (int i=0; i<npaths; i++)
    {
      QVector4D vp = po[i].viewport();
      // render only when textured plane and viewport active
      if (po[i].viewportTF() >= 0 &&
	  po[i].viewportTF() < Global::lutSize() &&
	  vp.x() >= 0.0)
	{
	  glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE); // front to back

	  int vx, vy, vh, vw;
	  int mh, mw;
	  int shiftx;

	  vx = 0;
	  vy = 0;

	  int horizontal = po[i].viewportStyle();
	  if (horizontal)
	    {
	      //horizontal
	      vw = vp.z()*ow;
	      vh = vp.w()*oh;
	    }
	  else
	    {
	      //vertical
	      vw = vp.w()*oh;
	      vh = vp.z()*ow;
	    }

	  vx+=1; vy+=1;
	  vw-=2; vh-=2;
	  mh = vh/2;
	  mw = vw/2;
	  shiftx = 5;

	  int offsetx = vp.x()*ow;
	  int offsety = oh-vp.y()*oh;

	  QList<Vec> pathPoints = po[i].pathPoints();
	  QList<Vec> pathX = po[i].pathX();
	  QList<Vec> pathY = po[i].pathY();
	  QList<float> radX = po[i].pathradX();
	  QList<float> radY = po[i].pathradY();

	  for(int np=0; np<pathPoints.count(); np++)
	    pathX[np] = VECPRODUCT(voxelSize,pathX[np]);
	  for(int np=0; np<pathPoints.count(); np++)
	    pathY[np] = VECPRODUCT(voxelSize,pathY[np]);

	  int maxthick = radY[0];
	  for(int np=0; np<pathPoints.count(); np++)
	    maxthick = max(maxthick, (int)radY[np]);
	  maxthick /= Global::stepsizeStill();

	  float maxheight = 0;
	  for(int np=0; np<pathPoints.count(); np++)
	    {
	      float ht = (pathX[np]*radX[np]).norm();
	      maxheight = max(maxheight, ht);
	    }


	  float imglength = 0;
	  for(int np=1; np<pathPoints.count(); np++)
	    {
	      Vec p0 = VECPRODUCT(voxelSize,pathPoints[np]);
	      Vec p1 = VECPRODUCT(voxelSize,pathPoints[np-1]);
	      imglength += (p0-p1).norm();
	    }
	  float scale = (float)(vw-11)/imglength;
	  if (2*maxheight*scale > vh-21)
	    scale = (float)(vh-21)/(2*maxheight);

	  shiftx = 5 + ((vw-11) - (imglength*scale))*0.5;

	  glMatrixMode(GL_MODELVIEW);
	  glPushMatrix();
	  if (!horizontal)
	    {
	      glTranslatef(offsetx-mw+mh, offsety+shiftx-mw, 0);
	      glTranslatef(mw, 0, 0);
	      glRotatef(90,0,0,1);
	      glTranslatef(-mw, 0, 0);
	    }
	  else
	    glTranslatef(offsetx+shiftx, offsety-mh, 0);
	  
	  enableTextureUnits();
	  glUseProgramObjectARB(m_defaultShader);

	  if (Global::volumeType() != Global::RGBVolume &&
	      Global::volumeType() != Global::RGBAVolume)
	    glUniform1fARB(parm[3], (float)po[i].viewportTF()/(float)Global::lutSize());
	  else
	    {
	      float frc = Global::stepsizeStill();
	      glUniform1fARB(parm[3], frc);
	    }	      

	  for(int nt=0; nt<maxthick; nt++)
	    {
	      float tk = 1.0 - (float)nt/(float)(maxthick-1);
	      glUniform1fARB(parm[18], 0.2+0.8*tk); 
	      
	      for(int b=slabstart; b<slabend; b++)
		{
		  float tminz = m_dataMin.z;
		  float tmaxz = m_dataMax.z;
//		  if (slabend > 1)
//		    {
//		      tminz = m_textureSlab[b].y;
//		      tmaxz = m_textureSlab[b].z;
//
//		      glUniform1iARB(parm[24], (tminz-m_dataMin.z)/lod); // zoffset
//		      glUniform1iARB(parm[27], tminz);
//		    }		  
		  
		  glUniform3fARB(parm[42], m_dataMin.x, m_dataMin.y, tminz);
		  glUniform3fARB(parm[43], m_dataMax.x, m_dataMax.y, tmaxz);

		  bindDataTextures(b);
		  
		  if (Global::interpolationType(Global::TextureInterpolation)) // linear
		    {
		      glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,
				      GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		      glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,
				      GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		    }
		  else
		    {
		      glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,
				      GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		      glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,
				      GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		    }
		  
		  glBegin(GL_TRIANGLE_STRIP);
		  float clen = 0;
		  for(int np=0; np<pathPoints.count(); np++)
		    {
		      if (np > 0)
			{
			  Vec p0 = VECPRODUCT(voxelSize,pathPoints[np]);
			  Vec p1 = VECPRODUCT(voxelSize,pathPoints[np-1]);
			  clen += (p0-p1).norm();
			}
		      float frc = clen;
		      
		      float lenradx = pathX[np].norm()*radX[np];
		      Vec tv0 = pathPoints[np];
		      Vec tv1 = pathPoints[np]+pathX[np]*radX[np];
		      Vec tv2 = pathPoints[np]-pathX[np]*radX[np];
		      tv1 += pathY[np]*radY[np]*tk;
		      tv2 += pathY[np]*radY[np]*tk;
		      tv1 = VECDIVIDE(tv1, voxelSize);
		      tv2 = VECDIVIDE(tv2, voxelSize);

		      Vec v0 = Vec(frc, 0.0, 0.0);
		      Vec v1 = v0 - Vec(0.0,lenradx,0.0);
		      Vec v2 = v0 + Vec(0.0,lenradx,0.0);
		      
		      v1 *= scale;
		      v2 *= scale;
		      
		      glMultiTexCoord3dv(GL_TEXTURE0, tv1);
		      glVertex3f((float)v1.x, (float)v1.y, 0.0);
		      
		      glMultiTexCoord3dv(GL_TEXTURE0, tv2);
		      glVertex3f((float)v2.x, (float)v2.y, 0.0);
		    }
		  glEnd();

		} // slabs
	    } // depth slices

	  disableTextureUnits();  
	  glUseProgramObjectARB(0);
	  
	  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // back to front
	  po[i].drawViewportLineDots(m_Viewer, scale, vh); 
	  po[i].drawViewportLine(scale, vh);

	  glPopMatrix();
	}
    }
  m_Viewer->stopScreenCoordinatesSystem();


  //glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  // --------------------------------
  // draw viewport borders
  disableTextureUnits();  
  glUseProgramObjectARB(0);  
  glDisable(GL_DEPTH_TEST);

  GeometryObjects::paths()->drawViewportBorders(m_Viewer);

  enableTextureUnits();
  glUseProgramObjectARB(m_defaultShader);
  glEnable(GL_DEPTH_TEST);

  if (!m_backlit)
    glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE); // for frontlit volume
  else
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // back to front

  check_MIP();
  // --------------------------------

  glDepthMask(GL_TRUE);

  if (slabend > 1) // reset it to 0
    glUniform1iARB(parm[24], 0); // zoffset    


  int lightgridx, lightgridy, lightgridz, lightncols, lightnrows;
  LightHandler::lightBufferInfo(lightgridx, lightgridy, lightgridz,
				lightnrows, lightncols, lightlod);
  glUniform1iARB(parm[40], lightlod); // lightlod

  // restore lighting
  glUniform1fARB(parm[7], m_lightInfo.highlights.ambient);
  glUniform1fARB(parm[8], m_lightInfo.highlights.diffuse);
  glUniform1fARB(parm[9], m_lightInfo.highlights.specular);
}

void
DrawHiresVolume::drawClipPlaneInViewport(int clipOffset, Vec lpos, float depthcue,
					 int lenx2, int leny2, int lod,
					 Vec dragTexsize,
					 bool defaultShader)
{
  ClipInformation clipInfo = GeometryObjects::clipplanes()->clipInfo();
  if (clipInfo.size() == 0)
    return; // no clipplanes

  if (! GeometryObjects::clipplanes()->viewportsVisible())
    return; // no clipplane to render in a viewport

  // --------------------------------
  // draw viewport borders
  disableTextureUnits();  
  glUseProgramObjectARB(0);  
  //glDisable(GL_DEPTH_TEST);
  GeometryObjects::clipplanes()->drawViewportBorders(m_Viewer);

  enableTextureUnits();
  glUseProgramObjectARB(m_defaultShader);
  glEnable(GL_DEPTH_TEST);
  // --------------------------------

  int slabstart, slabend;
  slabstart = 1;
  slabend = m_dataTexSize;
  if (m_dataTexSize == 1)
    {
      slabstart = 0;
      slabend = 1;
    }
  setShader2DTextureParameter(true, defaultShader);

  if (slabstart > 0)
    {
      Vec vsize = m_Volume->getSubvolumeTextureSize();
      glUniform3fARB(m_defaultParm[52], vsize.x, vsize.y, vsize.z);
    }
  else
    {
      Vec vsize = m_Volume->getDragSubvolumeTextureSize();
      glUniform3fARB(m_defaultParm[52], vsize.x, vsize.y, vsize.z);
    }
  
  glUniform3fARB(m_defaultParm[53], m_dataMin.x, m_dataMin.y, m_dataMin.z);


  bool ok = false;
  Vec voxelScaling = Global::voxelScaling();
  int nclipPlanes = (GeometryObjects::clipplanes()->positions()).count();

  qglviewer::Camera::Type camType = m_Viewer->camera()->type();
  Quaternion camRot = m_Viewer->camera()->orientation();
  Vec sceneCenter = m_Viewer->camera()->sceneCenter();
  Vec camPos = m_Viewer->camera()->position();
  Vec camDir = m_Viewer->camera()->viewDirection();
  int ow = m_Viewer->camera()->screenWidth();
  int oh = m_Viewer->camera()->screenHeight();
  float aspectRatio = m_Viewer->aspectRatio();
  float focusDistance = m_Viewer->camera()->focusDistance();

  GLint *parm;
  
  parm = m_defaultParm;

  glUniform3fARB(parm[6], lpos.x, lpos.y, lpos.z);

  // lightlod 0 means use basic lighting model
  int lightlod = 0;
  glUniform1iARB(parm[40], lightlod); // lightlod

  // use only ambient component
  glUniform1fARB(parm[7], 1.0);
  glUniform1fARB(parm[8], 0.0);
  glUniform1fARB(parm[9], 0.0);

  QList<bool> clips;
  int btfset;
  Vec subvol[8],texture[8], subcorner, subdim;
  Vec brickPivot, brickScale;
 
  m_drawBrickInformation.get(0,
			     btfset,
			     subvol, texture,
			     subcorner, subdim,
			     clips,
			     brickPivot, brickScale);	

  // undo brick-0 transformation for clipplanes
  double XformI[16];
  memcpy(XformI, m_bricks->getMatrixInv(), 16*sizeof(double));
  for (int i=0; i<8; i++)
    subvol[i] = Matrix::xformVec(XformI, subvol[i]);
      
  for (int ic=0; ic<nclipPlanes; ic++)
    {
      QVector4D vp = clipInfo.viewport[ic];
      // render only when textured plane and viewport active
      if (clipInfo.tfSet[ic] >= 0 &&
	  clipInfo.tfSet[ic] < Global::lutSize() &&
	  vp.x() >= 0.0)
	{
	  // opmod
	  glUniform1fARB(m_defaultParm[49], clipInfo.opmod[ic]);

	  // change camera settings
	  //----------------
	  m_Viewer->camera()->setOrientation(clipInfo.rot[ic]);

	  Vec cpos = VECPRODUCT(clipInfo.pos[ic], voxelScaling);
	  Vec clipcampos = cpos - 
	    m_Viewer->camera()->viewDirection()*m_Viewer->sceneRadius()*2*(1.0/clipInfo.viewportScale[ic]);
	  
	  m_Viewer->camera()->setPosition(clipcampos);
	  m_Viewer->camera()->setSceneCenter(cpos);

	  if (clipInfo.viewportType[ic])
	    m_Viewer->camera()->setType(Camera::PERSPECTIVE);
	  else
	    m_Viewer->camera()->setType(Camera::ORTHOGRAPHIC);
	  
	  int vx, vy, vh, vw;
	  vx = vp.x()*ow;
	  vy = vp.y()*oh;
	  vw = vp.z()*ow;
	  vh = vp.w()*oh;
	  m_Viewer->camera()->setScreenWidthAndHeight(vw, vh);

	  // change focusdistance for stereo
	  m_focusDistance = focusDistance*clipInfo.stereo[ic];

	  loadCameraMatrices();
	  
	  glViewport(vx, vy, vw, vh);
	  //----------------

	  if (Global::volumeType() != Global::RGBVolume &&
	      Global::volumeType() != Global::RGBAVolume)
	    glUniform1fARB(parm[3], (float)clipInfo.tfSet[ic]/(float)Global::lutSize());
	  else
	    {
	      float frc = Global::stepsizeStill();
	      glUniform1fARB(parm[3], frc);
	    }	      

	  QList<bool> dummyclips;
	  ViewAlignedPolygon *vap = new ViewAlignedPolygon;
	  //for (int sl = clipInfo.thickness[ic]; sl>=-clipInfo.thickness[ic]; sl--)
	  for (int sl = 0; sl>=-2*clipInfo.thickness[ic]; sl--)
	    {
	      int sls = sl;
	      if (!defaultShader && !m_backlit) sls = -sl;

	      drawpoly(cpos+sls*m_clipNormal[ic],
		       m_clipNormal[ic],
		       subvol, texture,
		       vap);

	      if (clipInfo.thickness[ic] == 0)
		glUniform1fARB(parm[18], depthcue);
	      else
		glUniform1fARB(parm[18], (float)(sls+clipInfo.thickness[ic])/
			                        (2.0f*clipInfo.thickness[ic]));
	      
	      for(int b=slabstart; b<slabend; b++)
		{
		  float tminz = m_dataMin.z;
		  float tmaxz = m_dataMax.z;
//		  if (slabend > 1)
//		    {
//		      tminz = m_textureSlab[b].y;
//		      tmaxz = m_textureSlab[b].z;
//		    }
		  glUniform3fARB(parm[42], m_dataMin.x, m_dataMin.y, tminz);
		  glUniform3fARB(parm[43], m_dataMax.x, m_dataMax.y, tmaxz);

		  bindDataTextures(b);
		  
		  if (Global::interpolationType(Global::TextureInterpolation)) // linear
		    {
		      glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,
				      GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		      glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,
				      GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		    }
		  else
		    {
		      glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,
				      GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		      glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,
				      GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		    }
		  
		    renderSlicedSlice((defaultShader ? 0 : 1),
				      vap, false,
				      lenx2, leny2, b, lod);
		} // loop over b
	    } // loop over clip slab
	  delete vap;

	  disableTextureUnits();  
	  glUseProgramObjectARB(0);

	  glDisable(GL_DEPTH_TEST);
	  
	  GeometryObjects::clipplanes()->drawOtherSlicesInViewport(m_Viewer, ic);

	  GLdouble neqn[4];
	  neqn[0] = -m_clipNormal[ic].x;
	  neqn[1] = -m_clipNormal[ic].y;
	  neqn[2] = -m_clipNormal[ic].z;
	  neqn[3] = m_clipNormal[ic]*(cpos+m_clipNormal[ic]*1.5);
	  glEnable(GL_CLIP_PLANE0);
	  glClipPlane(GL_CLIP_PLANE0, neqn);
	  glEnable(GL_CLIP_DISTANCE0);
	  glUniform4fARB(m_vertParm[0], neqn[0], neqn[1], neqn[2], neqn[3]);

	  GLdouble feqn[4];
	  feqn[0] = m_clipNormal[ic].x;
	  feqn[1] = m_clipNormal[ic].y;
	  feqn[2] = m_clipNormal[ic].z;
	  feqn[3] = -m_clipNormal[ic]*(cpos-m_clipNormal[ic]*(2*clipInfo.thickness[ic]+1.5));
	  glEnable(GL_CLIP_PLANE1);
	  glClipPlane(GL_CLIP_PLANE1, feqn);      
	  glEnable(GL_CLIP_DISTANCE1);
	  glUniform4fARB(m_vertParm[1], feqn[0], feqn[1], feqn[2], feqn[3]);

	  GeometryObjects::hitpoints()->draw(m_Viewer, m_backlit);

	  Vec pn = -m_clipNormal[ic];
	  float pnear = neqn[3];
	  float pfar = feqn[3];
	  GeometryObjects::paths()->draw(m_Viewer, pn, pnear, pfar, m_backlit, m_lightPosition);

	  glDisable(GL_CLIP_DISTANCE0);
	  glDisable(GL_CLIP_DISTANCE1);
	  glDisable(GL_CLIP_PLANE0);
	  glDisable(GL_CLIP_PLANE1);

	  GeometryObjects::paths()->postdrawInViewport(m_Viewer,
						       vp, cpos,
						       m_clipNormal[ic], clipInfo.thickness[ic]);


	  enableTextureUnits();
	  glUseProgramObjectARB(m_defaultShader);

	  glEnable(GL_DEPTH_TEST);

	} // valid tfset
    } // loop over clipplanes

  m_Viewer->camera()->setPosition(camPos);
  m_Viewer->camera()->setSceneCenter(sceneCenter);
  m_Viewer->camera()->setOrientation(camRot);
  m_Viewer->camera()->setViewDirection(camDir);
  m_Viewer->camera()->setType(camType);
  m_Viewer->camera()->setScreenWidthAndHeight(ow, oh);
  // restore focusdistance
  m_focusDistance = focusDistance;
  loadCameraMatrices();
  glViewport(0,0, ow, oh); 

//  // --------------------------------
//  // draw viewport borders
//  disableTextureUnits();  
//  glUseProgramObjectARB(0);  
//  glDisable(GL_DEPTH_TEST);
//
//  GeometryObjects::clipplanes()->drawViewportBorders(m_Viewer);

  enableTextureUnits();
  glUseProgramObjectARB(m_defaultShader);
  glEnable(GL_DEPTH_TEST);
  // --------------------------------

  int lightgridx, lightgridy, lightgridz, lightncols, lightnrows;
  LightHandler::lightBufferInfo(lightgridx, lightgridy, lightgridz,
				lightnrows, lightncols, lightlod);
  glUniform1iARB(m_defaultParm[40], lightlod); // lightlod

  // restore lighting
  glUniform1fARB(m_defaultParm[7], m_lightInfo.highlights.ambient);
  glUniform1fARB(m_defaultParm[8], m_lightInfo.highlights.diffuse);
  glUniform1fARB(m_defaultParm[9], m_lightInfo.highlights.specular);
}

void
DrawHiresVolume::drawClipPlaneDefault(int s, int layers,
				      Vec po, Vec pn, Vec step,
				      int clipOffset, Vec lpos, float depthcue,
				      int slabstart, int slabend,
				      int lenx2, int leny2, int lod,
				      Vec dragTexsize)
{
  preDrawGeometry(s, layers,
		  po, pn, step, true);

  if (slabstart > 0)
    {
      Vec vsize = m_Volume->getSubvolumeTextureSize();
      glUniform3fARB(m_defaultParm[52], vsize.x, vsize.y, vsize.z);
    }
  else
    {
      Vec vsize = m_Volume->getDragSubvolumeTextureSize();
      glUniform3fARB(m_defaultParm[52], vsize.x, vsize.y, vsize.z);
    }

  glUniform3fARB(m_defaultParm[53], m_dataMin.x, m_dataMin.y, m_dataMin.z);

  glUniform1i(m_defaultParm[55], 0); // do not clip
  
  glUniform3fARB(m_defaultParm[6], lpos.x, lpos.y, lpos.z);

  // use only ambient component
  glUniform1fARB(m_defaultParm[7], 1.0);
  glUniform1fARB(m_defaultParm[8], 0.0);
  glUniform1fARB(m_defaultParm[9], 0.0);

  QList<bool> clips;
  int btfset;
  Vec subvol[8],texture[8], subcorner, subdim;
  Vec brickPivot, brickScale;
	      
  m_drawBrickInformation.get(0,
			     btfset,
			     subvol, texture,
			     subcorner, subdim,
			     clips,
			     brickPivot, brickScale);
	  
  int nclipPlanes = (GeometryObjects::clipplanes()->positions()).count();
  QList<int> tfset = GeometryObjects::clipplanes()->tfset();
  QList<bool> showSlice = GeometryObjects::clipplanes()->showSlice();
  for (int ic=0; ic<nclipPlanes; ic++)
    {
      if (showSlice[ic] &&
	  tfset[ic] >= 0 &&
	  tfset[ic] < Global::lutSize())
	{
	  ViewAlignedPolygon *vap = m_polygon[clipOffset + ic];
	  
	  if (Global::volumeType() != Global::RGBVolume &&
	      Global::volumeType() != Global::RGBAVolume)
	    glUniform1fARB(m_defaultParm[3], (float)tfset[ic]/(float)Global::lutSize());
		  
	  glUniform1fARB(m_defaultParm[18], depthcue);
	  
	  for(int b=slabstart; b<slabend; b++)
	    {
	      float tminz = m_dataMin.z;
	      float tmaxz = m_dataMax.z;
//	      if (slabend > 1)
//		{
//		  tminz = m_textureSlab[b].y;
//		  tmaxz = m_textureSlab[b].z;
//		}
	      glUniform3fARB(m_defaultParm[42], m_dataMin.x, m_dataMin.y, tminz);
	      glUniform3fARB(m_defaultParm[43], m_dataMax.x, m_dataMax.y, tmaxz);

	      bindDataTextures(b);
	      
	      if (Global::interpolationType(Global::TextureInterpolation)) // linear
		{
		  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,
				  GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,
				  GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}
	      else
		{
		  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,
				  GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,
				  GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		}
	      
	      if (b == 0)
		renderDragSlice(vap, false, dragTexsize);		  
	      else
		renderSlicedSlice(0,
				  vap, false,
				  lenx2, leny2, b, lod);
	      
//	      if (m_drawImageType != Enums::DragImage ||
//		  m_dataTexSize == 1)
//		renderSlicedSlice(0,
//				  vap, false,
//				  lenx2, leny2, b, lod);
//	      else
//		renderDragSlice(vap, false, dragTexsize);		  
	    } // loop over b
	} // valid tfset
    } // loop over clipplanes

  // restore lighting
  glUniform1fARB(m_defaultParm[7], m_lightInfo.highlights.ambient);
  glUniform1fARB(m_defaultParm[8], m_lightInfo.highlights.diffuse);
  glUniform1fARB(m_defaultParm[9], m_lightInfo.highlights.specular);

  int nclip = qMin(10,GeometryObjects::clipplanes()->positions().count()); // max 10 clip planes allowed
  glUniform1i(m_defaultParm[55], nclip); // restore clip
  
  postDrawGeometry();
}

void
DrawHiresVolume::checkCrops()
{
  if (GeometryObjects::crops()->count() == 0 &&
      m_crops.count() == 0)
    return;

  bool doall = false;
  if (GeometryObjects::crops()->count() != m_crops.count())
    doall = true;
  
  if (!doall)
    {
      QList<CropObject> co;
      co = GeometryObjects::crops()->crops();
      for(int i=0; i<m_crops.count(); i++)
	{
	  if (m_crops[i] != co[i])
	    {
	      doall = true;
	      break;
	    }
	}
    }
      
  if (doall)
    {
      m_crops.clear();
      m_crops = GeometryObjects::crops()->crops();    

      createDefaultShader();
    }
}

void
DrawHiresVolume::checkPaths()
{
  QList<PathObject> po;
  po = GeometryObjects::paths()->paths();

  QList<PathObject> vpo;
  for(int i=0; i<po.count(); i++)
    {
      if (po[i].crop() || po[i].blend())
	vpo.append(po[i]);
    }
  po.clear();

  bool doall = false;
  if (vpo.count() != m_paths.count())
    doall = true;

  if (!doall)
    {
      for(int i=0; i<vpo.count(); i++)
	{
	  if (m_paths[i] != vpo[i])
	    {
	      doall = true;
	      break;
	    }
	}
    }

  if (doall)
    {
      m_paths = vpo;
      createDefaultShader();
    }

  vpo.clear();
}

void
DrawHiresVolume::setLightInfo(LightingInformation lightInfo)
{
  bool doDS = false;
  bool doHQS = false;
  bool doISB = false;
  bool doBS = false;
  bool doBPS = false;
  bool doSS = false;

  if (m_lightInfo.applyLighting != lightInfo.applyLighting)
    {
      doDS = true;
      doHQS = true;
    }

  if (((m_lightInfo.highlights.ambient < 0.01 &&
	m_lightInfo.highlights.diffuse < 0.01 &&
	m_lightInfo.highlights.specular < 0.01) ||
       (lightInfo.highlights.ambient < 0.01 &&
	lightInfo.highlights.diffuse < 0.01 &&
	lightInfo.highlights.specular < 0.01)) &&
      (m_lightInfo.highlights.ambient != lightInfo.highlights.ambient ||
       m_lightInfo.highlights.diffuse != lightInfo.highlights.diffuse ||
       m_lightInfo.highlights.specular != lightInfo.highlights.specular))
    doHQS = true;

  if (m_lightInfo.applyEmissive != lightInfo.applyEmissive)
    {
      doHQS = true;
      doDS = true;
    }

  if (m_lightInfo.applyShadows != lightInfo.applyShadows)
    doHQS = true;

  if (m_lightInfo.applyColoredShadows != lightInfo.applyColoredShadows)
    {
      doHQS = true;
      doSS = true;
    }

  if (qAbs(m_lightInfo.shadowIntensity - lightInfo.shadowIntensity) > 0.05)
    {
      doHQS = true;
      doSS = true;
    }

  if (qAbs((m_lightInfo.colorAttenuation-lightInfo.colorAttenuation).norm()) > 0.001)
    doSS = true;

  if (qAbs(m_lightInfo.shadowScale - lightInfo.shadowScale) > 0.05)
    doISB = true;

  if (qAbs(m_lightInfo.shadowBlur - lightInfo.shadowBlur) > 0.1)
    doBS = true;

  if (qAbs(m_lightInfo.backplaneIntensity - lightInfo.backplaneIntensity) > 0.05)
    doBPS = true;

  if (qAbs(m_lightInfo.peelMin - lightInfo.peelMin) > 0.001 ||
      qAbs(m_lightInfo.peelMax - lightInfo.peelMax) > 0.001 ||
      qAbs(m_lightInfo.peelMix - lightInfo.peelMix) > 0.001 ||
      m_lightInfo.peelType != lightInfo.peelType ||
      m_lightInfo.peel != lightInfo.peel)
    {
      doDS = true;
      doHQS = true;
      doSS = true;
    }


  m_lightInfo = lightInfo;


  if (doISB) initShadowBuffers();
  if (doDS) createDefaultShader();
  if (doBPS) createBackplaneShader(m_lightInfo.backplaneIntensity);
}


void
DrawHiresVolume::updateLightVector(Vec dir)
{
  //m_lightInfo.userLightVector = dir.unit();

  // just apply linear scaling to x & y components
  // we want more details near the center
  Vec ut = Vec(dir.x*0.5, dir.y*0.5, dir.z);
  ut = ut.unit();
  m_lightInfo.userLightVector = ut;
}
void DrawHiresVolume::updateLightDistanceOffset(float val) { m_lightInfo.lightDistanceOffset = val; }
void DrawHiresVolume::updateShadowFOV(float val){ m_lightInfo.shadowFovOffset = val; }
void DrawHiresVolume::updateHighlights(Highlights highlights)
{
  bool doHQS = false;
  bool doDS = false;

  if (((m_lightInfo.highlights.ambient < 0.01 &&
	m_lightInfo.highlights.diffuse < 0.01 &&
	m_lightInfo.highlights.specular < 0.01) ||
       (highlights.ambient < 0.01 &&
	highlights.diffuse < 0.01 &&
	highlights.specular < 0.01)) &&
      (m_lightInfo.highlights.ambient != highlights.ambient ||
       m_lightInfo.highlights.diffuse != highlights.diffuse ||
       m_lightInfo.highlights.specular != highlights.specular))
    {
      doHQS = true;
      doDS = true;
    }

  m_lightInfo.highlights = highlights;

  if (doDS) createDefaultShader();
}

void DrawHiresVolume::applyBackplane(bool flag) { m_lightInfo.applyBackplane = flag; }
void DrawHiresVolume::updateBackplaneShadowScale(float val) { m_lightInfo.backplaneShadowScale = val; }

void
DrawHiresVolume::applyLighting(bool flag)
{
  m_lightInfo.applyLighting = flag;
  createDefaultShader();
}

void
DrawHiresVolume::applyEmissive(bool flag)
{
  m_lightInfo.applyEmissive = flag;
  createDefaultShader();
}

void
DrawHiresVolume::applyShadows(bool flag)
{
  m_lightInfo.applyShadows = flag;
}

void
DrawHiresVolume::applyColoredShadows(bool flag)
{
  m_lightInfo.applyColoredShadows = flag;
}

void DrawHiresVolume::updateShadowIntensity(float val)
{
  m_lightInfo.shadowIntensity = val;
}

void
DrawHiresVolume::updateBackplaneIntensity(float val)
{
  m_lightInfo.backplaneIntensity = val;
  createBackplaneShader(m_lightInfo.backplaneIntensity);
}

void
DrawHiresVolume::updateShadowBlur(float val)
{
  m_lightInfo.shadowBlur = val;
}

void
DrawHiresVolume::updateShadowScale(float val)
{
  m_lightInfo.shadowScale = val;
  initShadowBuffers();
}

void
DrawHiresVolume::updateShadowColorAttenuation(float r, float g, float b)
{
  m_lightInfo.colorAttenuation = Vec(r,g,b);
}

void
DrawHiresVolume::peel(bool flag)
{
  m_lightInfo.peel = flag;
  createDefaultShader();
}

void
DrawHiresVolume::peelInfo(int ptype, float pmin, float pmax, float pmix)
{
  m_lightInfo.peelType = ptype;
  m_lightInfo.peelMin = pmin;
  m_lightInfo.peelMax = pmax;
  m_lightInfo.peelMix = pmix;
  createDefaultShader();
}

void
DrawHiresVolume::setMix(int mv, bool mc, bool mo, float iv)
{
  if (mv != m_mixvol ||
      mc != m_mixColor ||
      mo != m_mixOpacity)
    {
      m_mixvol = mv;
      m_mixColor = mc;
      m_mixOpacity = mo;
      createDefaultShader();
    }

  if (iv <= 1.0f)
    m_interpVol = iv;
}

void
DrawHiresVolume::setInterpolateVolumes(int iv)
{
  m_interpolateVolumes = iv;
  createDefaultShader();
}

bool
DrawHiresVolume::keyPressEvent(QKeyEvent *event)
{
  // process brick events
  if (m_bricks->keyPressEvent(event))
    return true;

  // process clipplane events
  if (GeometryObjects::clipplanes()->keyPressEvent(event))
    return true;

  if (event->modifiers() != Qt::NoModifier)
    return false;

  if (event->key() == Qt::Key_G)
    {
      // toggle mouse grabs for geometry objects
      GeometryObjects::inPool = ! GeometryObjects::inPool;
      if (GeometryObjects::inPool)
	GeometryObjects::addInMouseGrabberPool();
      else
	GeometryObjects::removeFromMouseGrabberPool();

      // toggle mouse grabs for light objects
      LightHandler::inPool = ! LightHandler::inPool;
      if (LightHandler::inPool)
	LightHandler::addInMouseGrabberPool();
      else
	LightHandler::removeFromMouseGrabberPool();
    }

  // change empty space skip
  if (event->key() == Qt::Key_E)
    {
      Global::setEmptySpaceSkip(!Global::emptySpaceSkip());
      MainWindowUI::mainWindowUI()->actionEmptySpaceSkip->setChecked(Global::emptySpaceSkip());

      createDefaultShader();

      if (raised())
	{
	  updateAndLoadPruneTexture();
	  m_Viewer->updateGL();
	}

      return true;
    }  

  // change render-mode
  if (event->key() == Qt::Key_1)
    {
      if (m_renderQuality == Enums::RenderDefault)
	setRenderQuality(Enums::RenderHighQuality);
      else
	setRenderQuality(Enums::RenderDefault);

      return true;
    }

  if (event->key() == Qt::Key_2)
    {
      if (MainWindowUI::mainWindowUI()->actionRedCyan->isChecked())
	{
	  MainWindowUI::mainWindowUI()->actionRedCyan->setChecked(false);
	}
      else
	{
	  MainWindowUI::mainWindowUI()->actionFor3DTV->setChecked(false);
	  MainWindowUI::mainWindowUI()->actionCrosseye->setChecked(false);
	  MainWindowUI::mainWindowUI()->actionRedBlue->setChecked(false);
	  MainWindowUI::mainWindowUI()->actionRedCyan->setChecked(true);
	}

      return true;
    }

  if (event->key() == Qt::Key_3)
    {
      if (MainWindowUI::mainWindowUI()->actionRedBlue->isChecked())
	{
	  MainWindowUI::mainWindowUI()->actionRedBlue->setChecked(false);
	}
      else
	{
	  MainWindowUI::mainWindowUI()->actionFor3DTV->setChecked(false);
	  MainWindowUI::mainWindowUI()->actionCrosseye->setChecked(false);
	  MainWindowUI::mainWindowUI()->actionRedBlue->setChecked(true);
	  MainWindowUI::mainWindowUI()->actionRedCyan->setChecked(false);
	}

      return true;
    }
  
  if (event->key() == Qt::Key_4)
    {
      if (MainWindowUI::mainWindowUI()->actionCrosseye->isChecked())
	{
	  MainWindowUI::mainWindowUI()->actionCrosseye->setChecked(false);
	}
      else
	{
	  MainWindowUI::mainWindowUI()->actionFor3DTV->setChecked(false);
	  MainWindowUI::mainWindowUI()->actionCrosseye->setChecked(true);
	  MainWindowUI::mainWindowUI()->actionRedBlue->setChecked(false);
	  MainWindowUI::mainWindowUI()->actionRedCyan->setChecked(false);
	}

      return true;
    }

  if (event->key() == Qt::Key_5)
    {
      if (MainWindowUI::mainWindowUI()->actionFor3DTV->isChecked())
	{
	  MainWindowUI::mainWindowUI()->actionFor3DTV->setChecked(false);
	}
      else
	{
	  MainWindowUI::mainWindowUI()->actionFor3DTV->setChecked(true);
	  MainWindowUI::mainWindowUI()->actionCrosseye->setChecked(false);
	  MainWindowUI::mainWindowUI()->actionRedBlue->setChecked(false);
	  MainWindowUI::mainWindowUI()->actionRedCyan->setChecked(false);
	}

      return true;
    }

  if (event->key() == Qt::Key_6)
    {
      if (MainWindowUI::mainWindowUI()->actionMIP->isChecked())
	MainWindowUI::mainWindowUI()->actionMIP->setChecked(false);
      else
	MainWindowUI::mainWindowUI()->actionMIP->setChecked(true);

      return true;
    }

  // change imagequality
  if (event->key() == Qt::Key_0)
    {
      Global::setImageQuality(Global::_NormalQuality);
      MainWindowUI::mainWindowUI()->actionNormal->setChecked(true);
      MainWindowUI::mainWindowUI()->actionLow->setChecked(false);
      MainWindowUI::mainWindowUI()->actionVeryLow->setChecked(false);
      m_Viewer->createImageBuffers();
      return true;
    }
  if (event->key() == Qt::Key_9)
    {
      Global::setImageQuality(Global::_LowQuality);
      MainWindowUI::mainWindowUI()->actionNormal->setChecked(false);
      MainWindowUI::mainWindowUI()->actionLow->setChecked(true);
      MainWindowUI::mainWindowUI()->actionVeryLow->setChecked(false);
      m_Viewer->createImageBuffers();
      return true;
    }
  if (event->key() == Qt::Key_8)
    {
      Global::setImageQuality(Global::_VeryLowQuality);
      MainWindowUI::mainWindowUI()->actionNormal->setChecked(false);
      MainWindowUI::mainWindowUI()->actionLow->setChecked(false);
      MainWindowUI::mainWindowUI()->actionVeryLow->setChecked(true);
      m_Viewer->createImageBuffers();
      return true;
    }
  
  if (event->key() == Qt::Key_D)
    {
      Global::setDepthcue(!Global::depthcue());
      MainWindowUI::mainWindowUI()->actionDepthcue->setChecked(Global::depthcue());
      return true;
    }
//  if (event->key() == Qt::Key_H)
//    {
//      Global::setUseStillVolume(!Global::useStillVolume());
//      MainWindowUI::mainWindowUI()->actionUse_stillvolume->setChecked(Global::useStillVolume());
//      return true;
//    }
  if (event->key() == Qt::Key_J)
    {
      Global::setUseDragVolumeforShadows(!Global::useDragVolumeforShadows());
      MainWindowUI::mainWindowUI()->actionUse_dragvolumeforshadows->setChecked(Global::useDragVolumeforShadows());
      return true;
    }
  if (event->key() == Qt::Key_L)
    {
      if (Global::volumeType() == Global::DummyVolume)
	return true;

      Global::setUseDragVolume(!Global::useDragVolume());
      MainWindowUI::mainWindowUI()->actionUse_dragvolume->setChecked(Global::useDragVolume());

      if (!Global::useDragVolume())
	emit histogramUpdated(m_histogram1D, m_histogram2D);
      else
	emit histogramUpdated(m_histogramDrag1D, m_histogramDrag2D);

      return true;
    }
  if (event->key() == Qt::Key_V)
    {
      GeometryObjects::showGeometry = ! GeometryObjects::showGeometry;
      if (GeometryObjects::showGeometry)
	GeometryObjects::show();
      else
	GeometryObjects::hide();

      LightHandler::showLights = ! LightHandler::showLights;
      if (LightHandler::showLights)
	LightHandler::show();
      else
	LightHandler::hide();

      return true;
    }

  
  return false;
}

void
DrawHiresVolume::drawBackground()
{
  // if we have already drawn a backplane return
  // without drawing the background
  if (m_renderQuality == Enums::RenderHighQuality &&
      m_lightInfo.applyShadows &&
      m_lightInfo.applyBackplane &&
      !m_backlit)
    return;

  glDepthMask(GL_FALSE); // disable writing to depth buffer


  int width = m_Viewer->camera()->screenWidth();
  int height = m_Viewer->camera()->screenHeight();

  if (MainWindowUI::mainWindowUI()->actionFor3DTV->isChecked())
    {
      width /= 2;
      if (Global::saveImageType() == Global::LeftImage)
	StaticFunctions::pushOrthoView(0,0, width, height);
      else
	StaticFunctions::pushOrthoView(width, 0, width, height);
    }
  else
    {
      if (MainWindowUI::mainWindowUI()->actionCrosseye->isChecked() &&
	  Global::saveImageType() == Global::LeftImage)
	StaticFunctions::pushOrthoView(width, 0, width, height);
      else
	StaticFunctions::pushOrthoView(0,0, width, height);
    }

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);

  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE);

  glUseProgramObjectARB(0);

  QImage bgImage = Global::backgroundImage();
  if (bgImage.isNull())
    {
      disableTextureUnits();

      if (Global::backgroundColor().norm() > 0 ||
	  m_lightInfo.applyBackplane)
	glColor3dv(Global::backgroundColor());
      else
	glColor4f(0,0,0,0);

      glBegin(GL_QUADS);
      glVertex3f(0,     0,      1);
      glVertex3f(width, 0,      1);
      glVertex3f(width, height, 1);
      glVertex3f(0,     height, 1);
      glEnd();
    }
  else
    {
      int ht = bgImage.height();
      int wd = bgImage.width();

      int nbytes = bgImage.byteCount();
      int rgb = nbytes/(wd*ht);

      GLuint fmt;
      if (rgb == 1) fmt = GL_LUMINANCE;
      else if (rgb == 2) fmt = GL_LUMINANCE_ALPHA;
      else if (rgb == 3) fmt = GL_RGB;
      else if (rgb == 4) fmt = GL_RGBA;

      glEnable(GL_TEXTURE_RECTANGLE_ARB);
      glActiveTexture(GL_TEXTURE0);
      glTexEnvf(GL_TEXTURE_ENV,
		GL_TEXTURE_ENV_MODE,
		GL_MODULATE);
      glBindTexture(GL_TEXTURE_RECTANGLE_ARB, Global::backgroundTexture());
      glTexParameterf(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
      glTexParameterf(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
      glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexImage2D(GL_TEXTURE_RECTANGLE_ARB,
		   0,
		   rgb,
		   wd,
		   ht,
		   0,
		   fmt,
		   GL_UNSIGNED_BYTE,
		   bgImage.bits());

      glColor3dv(Global::backgroundColor());

      enableTextureUnits();
      glUseProgramObjectARB(Global::copyShader());
      glUniform1iARB(Global::copyParm(0), 0); // lutTex

      glBegin(GL_QUADS);

      glTexCoord2f(0, ht);
      glVertex3f(0,     0,      1);
      
      glTexCoord2f(wd, ht);
      glVertex3f(width, 0,      1);

      glTexCoord2f(wd, 0);
      glVertex3f(width, height, 1);

      glTexCoord2f(0, 0);
      glVertex3f(0,     height, 1);

      glEnd();

      glDisable(GL_TEXTURE_RECTANGLE_ARB);
    }

  StaticFunctions::popOrthoView();

  glDepthMask(GL_TRUE); // enable writing to depth buffer

  disableTextureUnits();
  glUseProgramObjectARB(0);
}

void
DrawHiresVolume::resliceVolume(Vec pos,
			       Vec normal, Vec xaxis, Vec yaxis,
			       float subsample,
			       int getVolumeSurfaceArea, int tagValue)
{
  //--- drop perpendiculars onto normal from all 8 vertices of the subvolume 
  Vec box[8];
  box[0] = Vec(m_dataMin.x, m_dataMin.y, m_dataMin.z);
  box[1] = Vec(m_dataMin.x, m_dataMin.y, m_dataMax.z);
  box[2] = Vec(m_dataMin.x, m_dataMax.y, m_dataMax.z);
  box[3] = Vec(m_dataMin.x, m_dataMax.y, m_dataMin.z);
  box[4] = Vec(m_dataMax.x, m_dataMin.y, m_dataMin.z);
  box[5] = Vec(m_dataMax.x, m_dataMin.y, m_dataMax.z);
  box[6] = Vec(m_dataMax.x, m_dataMax.y, m_dataMax.z);
  box[7] = Vec(m_dataMax.x, m_dataMax.y, m_dataMin.z);
  float boxdist[8];

  // get number of slices
  for (int i=0; i<8; i++) boxdist[i] = (box[i] - pos)*normal;
  float dmin = boxdist[0];
  for (int i=1; i<8; i++) dmin = qMin(dmin, boxdist[i]);
  float dmax = boxdist[0];
  for (int i=1; i<8; i++) dmax = qMax(dmax, boxdist[i]);
  //------------------------

  // get width
  for (int i=0; i<8; i++) boxdist[i] = (box[i] - pos)*xaxis;
  float wmin = boxdist[0];
  for (int i=1; i<8; i++) wmin = qMin(wmin, boxdist[i]);
  float wmax = boxdist[0];
  for (int i=1; i<8; i++) wmax = qMax(wmax, boxdist[i]);
  //------------------------

  // get height
  for (int i=0; i<8; i++) boxdist[i] = (box[i] - pos)*yaxis;
  float hmin = boxdist[0];
  for (int i=1; i<8; i++) hmin = qMin(hmin, boxdist[i]);
  float hmax = boxdist[0];
  for (int i=1; i<8; i++) hmax = qMax(hmax, boxdist[i]);
  //------------------------


  int vlod = m_Volume->getSubvolumeSubsamplingLevel();

  int nslices = (dmax-dmin+1)/subsample/vlod;
  int wd = (wmax-wmin+1)/subsample/vlod;
  int ht = (hmax-hmin+1)/subsample/vlod;

  if (normal*Vec(0,0,-1) > 0.999 && getVolumeSurfaceArea == 0)
    {
      bool ok;
      QString text;
      text = QInputDialog::getText(0,
				   "New Volume Size",
				   QString("Original Volume Size : %1 %2 %3\nNew Volume Size").\
				   arg(wmax-wmin+1).\
				   arg(hmax-hmin+1).\
				   arg(dmax-dmin+1),
				   QLineEdit::Normal,
				   QString("%1 %2 %3").\
				   arg(wd).arg(ht).arg(nslices),
				   &ok);
      QStringList list = text.split(" ", QString::SkipEmptyParts);
      if (list.size() == 3)
	{
	  int a1 = list[0].toInt();
	  int a2 = list[1].toInt();
	  int a3 = list[2].toInt();
	  if (a1 > 0 && a2 > 0 && a3 > 0)
	    {
	      wd = a1;
	      ht = a2;
	      nslices = a3;
	    }
	}
    }

  Vec sliceStart = pos + dmin*normal + wmin*xaxis + hmin*yaxis;
  Vec sliceEnd = pos + dmax*normal + wmin*xaxis + hmin*yaxis;
  Vec endW = (wmax-wmin+1)*xaxis;
  Vec endWH = (wmax-wmin+1)*xaxis + (hmax-hmin+1)*yaxis;
  Vec endH = (hmax-hmin+1)*yaxis;

  VolumeInformation pvlInfo = VolumeInformation::volumeInformation();
  Vec vs;
  vs.x = VECPRODUCT(xaxis, pvlInfo.voxelSize).norm();
  vs.y = VECPRODUCT(yaxis, pvlInfo.voxelSize).norm();
  vs.z = VECPRODUCT(normal,pvlInfo.voxelSize).norm();
  vs *= subsample*vlod;

  //int voxtype = pvlInfo.voxelType;
  int voxtype = Global::pvlVoxelType();

  VolumeFileManager pFileManager;
  QString pFile;
  QString newFile;
  bool saveValue=true;
  bool tightFit = false;
  int clearance = 0;
  int neighbours = 26;
  if (getVolumeSurfaceArea == 0)
    {      
      saveValue = getSaveValue();
      if (!saveValue) voxtype = 0;

      //-----------------------
      // get tightFit
      QStringList items;
      items << "Yes" << "No";
      bool ok;
      QString item = QInputDialog::getItem(0,
					   "Tight Fit",
					   "Want a tight fit - volume bounds tightly fitting visible voxels ? Default is tight fit.",
					   items,
					   0,
					   false,
					   &ok);      
      if (!ok || item == "Yes")
	{
	  tightFit = true;
	  clearance = QInputDialog::getInt(0,
					   "Clearance for tight fit",
					   "Gap from edge to first contributing voxel",
					   0, 0, 20);
	}
      //------------------

      //------------------
      // now get the filename
      if (tightFit)
	{
	  QFileInfo finfo(Global::previousDirectory(), "temporaryVolumeFile");
	  pFile = finfo.absoluteFilePath();
	  newFile = getResliceFileName(); // now get tightFit file name
	  if (newFile.isEmpty())
	    return;
	  saveReslicedVolume(pFile,
			     nslices, wd, ht, pFileManager,
			     true, vs, voxtype);
	}
      else
	{
	  pFile = getResliceFileName();
	  if (pFile.isEmpty())
	    return;
	  saveReslicedVolume(pFile,
			     nslices, wd, ht, pFileManager,
			     false, vs, voxtype);
	}

    }
  else if (getVolumeSurfaceArea == 1) // volume calculations
    {
      saveValue = false; // for volume calculation we need opacity    
      voxtype = 0;
    }
  else if (getVolumeSurfaceArea == 2)
    {      
      saveValue = false; // for surface area calculation we need opacity
      voxtype = 0;

      //-----------------------
      // get neighbourhood type
      QStringList items;
      items << "26" << "18" << "6";
      bool ok;
      QString item = QInputDialog::getItem(0,
					   "Neighbourhood type",
					   "Select neighbourhood for surface voxel selection",
					   items,
					   0,
					   false,
					   &ok);      
      if (item == "18") neighbours = 18;
      if (item == "6") neighbours = 6;
      //-----------------------

      //-----------------------
      // now get the filename
      pFile = getResliceFileName(true);
      if (!pFile.isEmpty())
	saveReslicedVolume(pFile,
			   nslices, wd, ht, pFileManager,
			   false, vs, voxtype);
      //-----------------------    }
    }

  //-------------------
  // delta
  float delta = 2.0;
  if (Global::volumeType() == Global::DoubleVolume)
    {
      QStringList items;
      items << "A-B"
	    << "|A-B|"
	    <<"A+B"
	    << "max(A,B)"
	    << "min(A,B)"
	    << "A/B"
	    << "A*B";
      bool ok;
      QString item = QInputDialog::getItem(0,
					   "Volume operations",
					   "Default is A-B.",
					   items,
					   0,
					   false,
					   &ok);
      
      if (!ok || item == "A-B") delta = 1.5;
      else if (!ok || item == "|A-B|") delta = 2.5;
      else if (item == "A+B") delta = 3.5;
      else if (item == "max(A,B)") delta = 4.5;
      else if (item == "min(A,B)") delta = 5.5;
      else if (item == "A/B") delta = 6.5;
      else if (item == "A*B") delta = 7.5;
    }
  //-------------------

  uchar *slice;
  uchar *tslice;
  if (voxtype == 0)
    slice = new uchar[wd*ht];
  else
    slice = new uchar[2*wd*ht];

  if (tightFit)
    tslice = new uchar[wd*ht];

  uchar *slice0, *slice1, *slice2;
  slice0 = slice1 = slice2 = 0;
  if (getVolumeSurfaceArea == 2) // surface area calculation
    {
      slice0 = new uchar[wd*ht];
      slice1 = new uchar[wd*ht];
      slice2 = new uchar[wd*ht];
    }

  uchar *tag = 0;
  if (tagValue >= 0)
    tag = new uchar[wd*ht];

  // save slices to shadowbuffer
  GLuint target = GL_TEXTURE_RECTANGLE_EXT;
  if (m_shadowBuffer) delete m_shadowBuffer;
  glActiveTexture(GL_TEXTURE3);
  m_shadowBuffer = new QGLFramebufferObject(QSize(wd, ht),
					    QGLFramebufferObject::NoAttachment,
					    GL_TEXTURE_RECTANGLE_EXT);
  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_shadowBuffer->texture());
//  if (getVolumeSurfaceArea == 0)
//    {
//	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//    }
  if (getVolumeSurfaceArea == 0 &&
      Global::interpolationType(Global::TextureInterpolation)) // linear
    {
      glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
  else
    {
      glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }


  m_shadowBuffer->bind();
  glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);

  glDisable(GL_DEPTH_TEST);

  enableTextureUnits();
  setRenderDefault();

  emptySpaceSkip();

  Vec dragTexsize;
  int lenx2, leny2, lod;
  getDragRenderInfo(dragTexsize, lenx2, leny2, lod);

  int slabstart, slabend;
  slabstart = 1;
  slabend = m_dataTexSize;
  if (m_dataTexSize == 1)
    {
      slabstart = 0;
      slabend = 1;
    }
  

  if (slabend > 1)
    setShader2DTextureParameter(true, true);
  else
    setShader2DTextureParameter(false, true);

  if (slabstart > 0)
    {
      Vec vsize = m_Volume->getSubvolumeTextureSize();
      glUniform3fARB(m_defaultParm[52], vsize.x, vsize.y, vsize.z);
    }
  else
    {
      Vec vsize = m_Volume->getDragSubvolumeTextureSize();
      glUniform3fARB(m_defaultParm[52], vsize.x, vsize.y, vsize.z);
    }
  
  glUniform3fARB(m_defaultParm[53], m_dataMin.x, m_dataMin.y, m_dataMin.z);


  StaticFunctions::pushOrthoView(0, 0, wd, ht);
  glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE); // for frontlit volume

  Vec voxelScaling = VolumeInformation::volumeInformation().voxelSize;

  GLint *parm = m_defaultParm;

  if (Global::volumeType() != Global::RGBVolume &&
      Global::volumeType() != Global::RGBAVolume)
    glUniform1fARB(parm[3], 0.0); // tfset
  else
    {
      float frc = Global::stepsizeStill();
      glUniform1fARB(parm[3], frc);
    }	      

  if (getVolumeSurfaceArea == 0 &&
      Global::interpolationType(Global::TextureInterpolation)) // linear
    glUniform1fARB(parm[18], 1.0); // depthcue
  else
    glUniform1fARB(parm[18], 2.0); // depthcue + additional info to do nearest neighbour interpolation.

  glUniform3fARB(parm[4], delta, delta, delta); // delta

  QProgressDialog progress("Reslicing volume",
			   QString(),
			   0, 100,
			   0);
  progress.setCancelButton(0);

  glDisable(GL_DEPTH_TEST);

  glClearDepth(0);
  glClearColor(0,0,0,0);

  //----------------------------
  // parameters for tightFit
  int xmin, xmax, ymin, ymax, zmin, zmax;
  xmax = ymax = 0;
  xmin = wd-1;
  ymin = ht-1;
  zmin = 0; zmax = nslices-1;
  bool zmindone = false;
  //----------------------------

  qint64 nonZeroVoxels=0;
  for(int sl=0; sl<nslices; sl++)
    {
      float sf = (float)sl/(float)(nslices-1);
      Vec po = pos +
	       (1-sf)*dmax*normal + sf*dmin*normal +
	       wmin*xaxis +
	       hmin*yaxis;

      progress.setLabelText(QString("%1").arg(sl));
      progress.setValue(100*(float)sl/(float)nslices);

      glClear(GL_DEPTH_BUFFER_BIT);
      glClear(GL_COLOR_BUFFER_BIT);

      for(int b=slabstart; b<slabend; b++)
	{
	  float tminz = m_dataMin.z;
	  float tmaxz = m_dataMax.z;

//	  if (slabend > 1)
//	    {
//	      tminz = m_textureSlab[b].y;
//	      tmaxz = m_textureSlab[b].z;
//	      
//	      glUniform1iARB(parm[24], (tminz-m_dataMin.z)/lod); // zoffset
//	      glUniform1iARB(parm[27], tminz);
//	    }		  
//	  
	  glUniform3fARB(parm[42], m_dataMin.x, m_dataMin.y, tminz);
	  glUniform3fARB(parm[43], m_dataMax.x, m_dataMax.y, tmaxz);
	  
	  bindDataTextures(b);
	  
	  if (getVolumeSurfaceArea == 0 &&
	      Global::interpolationType(Global::TextureInterpolation)) // linear
	    {
	      glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,
			      GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	      glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,
			      GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	    }
	  else
	    {
	      glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,
			      GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	      glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,
			      GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	    }
	  
	  glBegin(GL_QUADS);

	  Vec v = po;
	  glMultiTexCoord3dv(GL_TEXTURE0, v);
	  glVertex3f(0, 0, 0);

	  v = po + endW;
	  glMultiTexCoord3dv(GL_TEXTURE0, v);
	  glVertex3f(wd, 0, 0);

	  v = po + endWH;
	  glMultiTexCoord3dv(GL_TEXTURE0, v);
	  glVertex3f(wd, ht, 0);

	  v = po + endH;
	  glMultiTexCoord3dv(GL_TEXTURE0, v);
	  glVertex3f(0, ht, 0);

	  glEnd();
	} // slabs

      if (voxtype == 0)
	{
	  if (saveValue)
	    glReadPixels(0, 0, wd, ht, GL_RED, GL_UNSIGNED_BYTE, slice);
	  else
	    glReadPixels(0, 0, wd, ht, GL_GREEN, GL_UNSIGNED_BYTE, slice);
	}
      else
	{
	  glReadPixels(0, 0, wd, ht, GL_RED, GL_UNSIGNED_SHORT, slice);
	}

      if (tagValue >= 0)
	{
	  glReadPixels(0, 0, wd, ht, GL_BLUE, GL_UNSIGNED_BYTE, tag);
	  for(int p=0; p<wd*ht; p++)
	    {
	      if (tag[p] != tagValue) slice[p] = 0;
	    }
	}

      if (getVolumeSurfaceArea == 0)
	pFileManager.setSlice(sl, slice);
      else if (getVolumeSurfaceArea == 1) // volume calculation
	{
	  for(int p=0; p<wd*ht; p++)
	    if (slice[p] > 0) nonZeroVoxels++;
	}
      else if (getVolumeSurfaceArea == 2) // surface area calculations
	{
	  calculateSurfaceArea(neighbours,
			       sl, nslices,
			       slice0, slice1, slice, slice2,
			       wd, ht,
			       !pFile.isEmpty(),
			       pFileManager);
	  
	  for(int p=0; p<wd*ht; p++)
	    if (slice2[p] > 0) nonZeroVoxels++;
	}

      
      //----------------------------
      // find bounds for tightFit
      if (tightFit)
	{
	  if (voxtype > 0)
	    {
	      memset(tslice, 0, wd*ht);
	      ushort *spt = (ushort *)slice;
	      for(int p=0; p<wd*ht; p++)
		if (spt[p] > 0)
		  tslice[p] = 255;

	      getTightFit(sl,
			  tslice, wd, ht,
			  zmindone,
			  xmin, xmax,
			  ymin, ymax,
			  zmin, zmax);
	    }
	  else
	    getTightFit(sl,
			slice, wd, ht,
			zmindone,
			xmin, xmax,
			ymin, ymax,
			zmin, zmax);
	}
      
      //----------------------------
    }

  m_shadowBuffer->release();

  delete [] slice;
  if (tightFit)
    delete [] tslice;
  if (tagValue >= 0)
    delete [] tag;

  if (getVolumeSurfaceArea == 2)
    {
      delete [] slice0;
      delete [] slice1;
      delete [] slice2;
    }

  glUseProgramObjectARB(0);
  disableTextureUnits();  

  StaticFunctions::popOrthoView();

  // delete shadow buffer
  if (m_shadowBuffer)  delete m_shadowBuffer;
  m_shadowBuffer = 0;
  //initShadowBuffers(true);

  glEnable(GL_DEPTH_TEST);

  //----------------------------
  if (tightFit)
    {
      xmin = qMax(0, xmin-clearance);
      ymin = qMax(0, ymin-clearance);
      zmin = qMax(0, zmin-clearance);
      xmax = qMin(wd-1, xmax+clearance);
      ymax = qMin(ht-1, ymax+clearance);
      zmax = qMin(nslices-1, zmax+clearance);
      int newd = zmax-zmin+1;
      int neww = ymax-ymin+1;
      int newh = xmax-xmin+1;
      
      VolumeFileManager newManager;
      saveReslicedVolume(newFile,
			 newd, newh, neww, newManager,
			 false, vs, voxtype);
      if (voxtype == 0)
	{
	  uchar *slice = new uchar[wd*ht];
	  for(int sl=zmin; sl<=zmax; sl++)
	    {
	      memcpy(slice, pFileManager.getSlice(sl), wd*ht);
	      for(int y=ymin; y<=ymax; y++)
		for(int x=xmin; x<=xmax; x++)
		  slice[(y-ymin)*newh+(x-xmin)] = slice[y*wd+x];
	  
	      newManager.setSlice(sl-zmin, slice);
	      progress.setLabelText(QString("%1").arg(sl-zmin));
	      progress.setValue(100*(float)(sl-zmin)/(float)newd);
	    }
	}
      else
	{
	  ushort *slice = new ushort[wd*ht];
	  for(int sl=zmin; sl<=zmax; sl++)
	    {
	      memcpy((uchar*)slice, pFileManager.getSlice(sl), 2*wd*ht);
	      for(int y=ymin; y<=ymax; y++)
		for(int x=xmin; x<=xmax; x++)
		  slice[(y-ymin)*newh+(x-xmin)] = slice[y*wd+x];
	  
	      newManager.setSlice(sl-zmin, (uchar*)slice);
	      progress.setLabelText(QString("%1").arg(sl-zmin));
	      progress.setValue(100*(float)(sl-zmin)/(float)newd);
	    }
	}
      
      QMessageBox::information(0, "Saved Resliced Volume",
			       QString("Resliced volume saved to %1 and %1.001"). \
			       arg(newFile));
      
      pFileManager.removeFile(); // remove temporary file
    }
  //----------------------------

  progress.setValue(100);


  if (getVolumeSurfaceArea == 1)
    {
      VolumeInformation pvlInfo = VolumeInformation::volumeInformation();
      Vec voxelSize = pvlInfo.voxelSize;
      float voxvol = nonZeroVoxels*voxelSize.x*voxelSize.y*voxelSize.z*vlod*vlod*vlod;

      QString str;
      str = QString("Subsampling Level : %1 - ").arg(vlod);
      if (vlod==1) str += " (i.e. full resolution)\n";
      else str += QString(" (i.e. every %1 voxel)\n").arg(vlod);
      str += QString("\nNon-Zero Voxels : %1\n").arg(nonZeroVoxels);
      str += QString("Non-Zero Voxel Volume : %1 %2^3\n").	\
	arg(voxvol).						\
	arg(pvlInfo.voxelUnitStringShort());
      
      float totvol = (dmax-dmin+1);
      totvol *= (wmax-wmin); 
      totvol *= (hmax-hmin);
      float percent = (float)nonZeroVoxels*vlod*vlod*vlod/totvol;
      percent *= 100.0;
      str += QString("Percent Non-Zero Voxels : %1\n").arg(percent);
      
      QMessageBox::information(0, "Volume Calculation", str);
    }
  else if (getVolumeSurfaceArea == 2)
    {
      QString str;
      str = QString("Subsampling Level : %1 - ").arg(vlod);
      if (vlod==1) str += " (i.e. full resolution)\n";
      else str += QString(" (i.e. every %1 voxel)\n").arg(vlod);
      str += QString("\nNon-Zero Voxels : %1\n").arg(nonZeroVoxels);

      QMessageBox::information(0, "Surface Area Calculation", str);

      if (!pFile.isEmpty())
	QMessageBox::information(0, "Saved Border Voxels",
				 QString("Border voxels saved to %1 and %1.001"). \
				 arg(pFile));
    }
  else
    {
      if (!tightFit)
	QMessageBox::information(0, "Saved Resliced Volume",
				 QString("Resliced volume saved to %1 and %1.001"). \
				 arg(pFile));
    }
}

void
DrawHiresVolume::resliceUsingPath(int pathIdx, bool fullThickness,
				  int subsample, int tagValue)

{
  Vec voxelScaling = Global::voxelScaling();

  PathObject po;
  po = GeometryObjects::paths()->paths()[pathIdx];

  QList<Vec> pathPoints = po.pathPoints();
  QList<Vec> pathX = po.pathX();
  QList<Vec> pathY = po.pathY();
  QList<float> radX = po.pathradX();
  QList<float> radY = po.pathradY();

  for(int np=0; np<pathPoints.count(); np++)
    pathX[np] = VECPRODUCT(voxelScaling,pathX[np]);
  for(int np=0; np<pathPoints.count(); np++)
    pathY[np] = VECPRODUCT(voxelScaling,pathY[np]);

  float pathLength = 0;
  for(int np=1; np<pathPoints.count(); np++)
    {
      Vec p0 = VECPRODUCT(voxelScaling,pathPoints[np]);
      Vec p1 = VECPRODUCT(voxelScaling,pathPoints[np-1]);
      pathLength += (p0-p1).norm();
    }

  int maxthick = radY[0];
  for(int np=0; np<pathPoints.count(); np++)
    maxthick = max(maxthick, (int)radY[np]);

  float maxheight = 0;
  for(int np=0; np<pathPoints.count(); np++)
    {
      float ht = (pathX[np]*radX[np]).norm();
      maxheight = max(maxheight, ht);
    }

  int vlod = m_Volume->getSubvolumeSubsamplingLevel();
  int nslices = maxthick/subsample/vlod;
  if (fullThickness) nslices = 2*maxthick/subsample/vlod;
  int wd = pathLength/subsample/vlod;
  int ht = 2*maxheight/subsample/vlod;

  VolumeFileManager pFileManager;
  
  QString pFile = getResliceFileName();
  if (pFile.isEmpty())
    return;
  saveReslicedVolume(pFile,
		     nslices, wd, ht, pFileManager);
  bool saveValue = getSaveValue();

  if (pFile.isEmpty())
    return;

  uchar *slice = new uchar[wd*ht];
  uchar *tag = 0;
  if (tagValue >= 0)
    tag = new uchar[wd*ht];

  // save slices to shadowbuffer
  GLuint target = GL_TEXTURE_RECTANGLE_EXT;
  if (m_shadowBuffer) delete m_shadowBuffer;
  glActiveTexture(GL_TEXTURE3);
  m_shadowBuffer = new QGLFramebufferObject(QSize(wd, ht),
					    QGLFramebufferObject::NoAttachment,
					    GL_TEXTURE_RECTANGLE_EXT);
  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_shadowBuffer->texture());
  glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


  m_shadowBuffer->bind();
  glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);

  glDisable(GL_DEPTH_TEST);

  enableTextureUnits();
  setRenderDefault();

  emptySpaceSkip();

  Vec dragTexsize;
  int lenx2, leny2, lod;
  getDragRenderInfo(dragTexsize, lenx2, leny2, lod);

  int slabstart, slabend;
  slabstart = 1;
  slabend = m_dataTexSize;
  if (m_dataTexSize == 1)
    {
      slabstart = 0;
      slabend = 1;
    }

  if (slabend > 1)
    setShader2DTextureParameter(true, true);
  else
    setShader2DTextureParameter(false, true);
  
  if (slabstart > 0)
    {
      Vec vsize = m_Volume->getSubvolumeTextureSize();
      glUniform3fARB(m_defaultParm[52], vsize.x, vsize.y, vsize.z);
    }
  else
    {
      Vec vsize = m_Volume->getDragSubvolumeTextureSize();
      glUniform3fARB(m_defaultParm[52], vsize.x, vsize.y, vsize.z);
    }
  
  glUniform3fARB(m_defaultParm[53], m_dataMin.x, m_dataMin.y, m_dataMin.z);


  StaticFunctions::pushOrthoView(0, 0, wd, ht);
  glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE); // for frontlit volume

  GLint *parm = m_defaultParm;

  if (Global::volumeType() != Global::RGBVolume &&
      Global::volumeType() != Global::RGBAVolume)
    {
      if (po.viewportTF() >= 0 &&
	  po.viewportTF() < Global::lutSize())
	glUniform1fARB(parm[3], (float)po.viewportTF()/(float)Global::lutSize());
      else
	glUniform1fARB(parm[3], 0.0); // tfset
    }
  else
    {
      float frc = Global::stepsizeStill();
      glUniform1fARB(parm[3], frc);
    }	      

  glUniform1fARB(parm[18], 1.0); // depthcue

  glUniform3fARB(parm[4], 2,2,2); // delta

  QProgressDialog progress("Reslicing volume",
			   QString(),
			   0, 100,
			   0);
  progress.setCancelButton(0);

  glDisable(GL_DEPTH_TEST);

  glClearDepth(0);
  glClearColor(0,0,0,0);
  
  glTranslatef(0.0, maxheight/subsample/vlod, 0.0);

  for(int sl=0; sl<nslices; sl++)
    {
      float tk = (float)sl/(float)(nslices-1);
      progress.setLabelText(QString("%1").arg(sl));
      progress.setValue(100*(float)sl/(float)nslices);
      
      glClear(GL_DEPTH_BUFFER_BIT);
      glClear(GL_COLOR_BUFFER_BIT);

      for(int b=slabstart; b<slabend; b++)
	{
	  float tminz = m_dataMin.z;
	  float tmaxz = m_dataMax.z;

//	  if (slabend > 1)
//	    {
//	      tminz = m_textureSlab[b].y;
//	      tmaxz = m_textureSlab[b].z;
//	      
//	      glUniform1iARB(parm[24], (tminz-m_dataMin.z)/lod); // zoffset
//	      glUniform1iARB(parm[27], tminz);
//	    }		  
	  
	  glUniform3fARB(parm[42], m_dataMin.x, m_dataMin.y, tminz);
	  glUniform3fARB(parm[43], m_dataMax.x, m_dataMax.y, tmaxz);
	  
	  bindDataTextures(b);
	  
	  if (Global::interpolationType(Global::TextureInterpolation)) // linear
	    {
	      glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,
			      GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	      glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,
			      GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	    }
	  else
	    {
	      glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,
			      GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	      glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,
			      GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	    }
	  
	  glBegin(GL_TRIANGLE_STRIP);
	  float clen = 0;
	  for(int np=0; np<pathPoints.count(); np++)
	    {

	      if (np > 0)
	      {
		Vec p0 = VECPRODUCT(voxelScaling,pathPoints[np]);
		Vec p1 = VECPRODUCT(voxelScaling,pathPoints[np-1]);
		clen += (p0-p1).norm();
	      }

	      float lenradx = pathX[np].norm()*radX[np]/subsample/vlod;
	      Vec tv0 = pathPoints[np];
	      Vec tv1 = pathPoints[np]-pathX[np]*radX[np];
	      Vec tv2 = pathPoints[np]+pathX[np]*radX[np];
	      if (fullThickness)
		{
		  tv1 += pathY[np]*(2.0*tk-1.0)*radY[np];
		  tv2 += pathY[np]*(2.0*tk-1.0)*radY[np];
		}
	      else
		{
		  tv1 += pathY[np]*radY[np]*tk;
		  tv2 += pathY[np]*radY[np]*tk;
		}

	      tv1 = VECDIVIDE(tv1, voxelScaling);
	      tv2 = VECDIVIDE(tv2, voxelScaling);
	      
	      float x0 = clen/subsample/vlod;
	      Vec v1 = Vec(x0,-lenradx,0.0);
	      Vec v2 = Vec(x0, lenradx,0.0);

	      glMultiTexCoord3dv(GL_TEXTURE0, tv1);
	      glVertex3f((float)v1.x, (float)v1.y, 0.0);
	      
	      glMultiTexCoord3dv(GL_TEXTURE0, tv2);
	      glVertex3f((float)v2.x, (float)v2.y, 0.0);
	    }
	  glEnd();
	  
	} // slabs

      if (saveValue)
	glReadPixels(0, 0, wd, ht, GL_RED, GL_UNSIGNED_BYTE, slice);
      else
	glReadPixels(0, 0, wd, ht, GL_GREEN, GL_UNSIGNED_BYTE, slice);

      if (tagValue >= 0)
	{
	  glReadPixels(0, 0, wd, ht, GL_BLUE, GL_UNSIGNED_BYTE, tag);
	  for(int p=0; p<wd*ht; p++)
	    {
	      if (tag[p] != tagValue) slice[p] = 0;
	    }
	}

      pFileManager.setSlice(nslices-1-sl, slice);
    } // depth slices

  m_shadowBuffer->release();

  progress.setValue(100);

  delete [] slice;
  if (tagValue >= 0)
    delete [] tag;

  glUseProgramObjectARB(0);
  disableTextureUnits();  

  StaticFunctions::popOrthoView();

  // delete shadow buffer
  if (m_shadowBuffer)  delete m_shadowBuffer;
  m_shadowBuffer = 0;
  //initShadowBuffers(true);

  glEnable(GL_DEPTH_TEST);

  QMessageBox::information(0, "Saved Resliced Volume",
			   QString("Resliced volume saved to %1 and %1.001").arg(pFile));
}

void
DrawHiresVolume::resliceUsingClipPlane(Vec cpos, Quaternion rot, int thickness,
				       QVector4D vp, float viewportScale, int tfSet,
				       int subsample, int tagValue)
{
  Vec voxelScaling = Global::voxelScaling();

  Vec normal= Vec(0,0,1);
  Vec xaxis = Vec(1,0,0);
  Vec yaxis = Vec(0,1,0);
  normal= rot.rotate(normal);
  xaxis = rot.rotate(xaxis);
  yaxis = rot.rotate(yaxis);

  normal= VECDIVIDE(normal,voxelScaling);
  xaxis = VECDIVIDE(xaxis, voxelScaling);
  yaxis = VECDIVIDE(yaxis, voxelScaling);


  float aspectRatio = vp.z()/vp.w(); 
  float cdist = 2*m_Viewer->sceneRadius()/viewportScale; 
  float fov = m_Viewer->camera()->fieldOfView(); 
  float ydist = cdist*tan(fov*0.5); // orthographic
  float xdist = ydist*aspectRatio;

  int vlod = m_Volume->getSubvolumeSubsamplingLevel();
  int nslices = 2*thickness/subsample/vlod;
  if (nslices == 0) nslices = 1;
  int wd = 2*xdist/subsample/vlod;
  int ht = 2*ydist/subsample/vlod;

  Vec sliceStart = cpos - thickness*normal - xdist*xaxis - ydist*yaxis;
  Vec sliceEnd = cpos + thickness*normal - xdist*xaxis - ydist*yaxis;
  Vec endW = 2*xdist*xaxis;
  Vec endH = 2*ydist*yaxis;

  VolumeFileManager pFileManager;
  
  QPair<QString, bool> srv;
  QString pFile;
  bool saveValue = false;
  if (nslices > 1)
    {
      pFile = getResliceFileName();
      if (pFile.isEmpty())
	return;
      saveReslicedVolume(pFile,
			 nslices, wd, ht, pFileManager);
      saveValue = getSaveValue();
    }
  
  uchar *slice = new uchar[wd*ht];
  uchar *tag = 0;
  if (tagValue >= 0)
    tag = new uchar[wd*ht];

  // save slices to shadowbuffer
  GLuint target = GL_TEXTURE_RECTANGLE_EXT;
  if (m_shadowBuffer) delete m_shadowBuffer;
  glActiveTexture(GL_TEXTURE3);
  m_shadowBuffer = new QGLFramebufferObject(QSize(wd, ht),
					    QGLFramebufferObject::NoAttachment,
					    GL_TEXTURE_RECTANGLE_EXT);
  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_shadowBuffer->texture());
  if (nslices > 1)
    {
      glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
  else // cross sectional area
    {
      glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }


  m_shadowBuffer->bind();
  glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);

  glDisable(GL_DEPTH_TEST);

  enableTextureUnits();
  setRenderDefault();

  emptySpaceSkip();

  Vec dragTexsize;
  int lenx2, leny2, lod;
  getDragRenderInfo(dragTexsize, lenx2, leny2, lod);

  int slabstart, slabend;
  slabstart = 1;
  slabend = m_dataTexSize;
  if (m_dataTexSize == 1)
    {
      slabstart = 0;
      slabend = 1;
    }

  if (slabend > 1)
    setShader2DTextureParameter(true, true);
  else
    setShader2DTextureParameter(false, true);
  

  if (slabstart > 0)
    {
      Vec vsize = m_Volume->getSubvolumeTextureSize();
      glUniform3fARB(m_defaultParm[52], vsize.x, vsize.y, vsize.z);
    }
  else
    {
      Vec vsize = m_Volume->getDragSubvolumeTextureSize();
      glUniform3fARB(m_defaultParm[52], vsize.x, vsize.y, vsize.z);
    }

  glUniform3fARB(m_defaultParm[53], m_dataMin.x, m_dataMin.y, m_dataMin.z);

  
  StaticFunctions::pushOrthoView(0, 0, wd, ht);
  glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE); // for frontlit volume

  GLint *parm = m_defaultParm;

  if (Global::volumeType() != Global::RGBVolume &&
      Global::volumeType() != Global::RGBAVolume)
    {
      if (tfSet >= 0 &&
	  tfSet < Global::lutSize())
	glUniform1fARB(parm[3], (float)tfSet/(float)Global::lutSize());
      else
	glUniform1fARB(parm[3], 0.0); // tfset
    }
  else
    {
      float frc = Global::stepsizeStill();
      glUniform1fARB(parm[3], frc);
    }	      

  glUniform1fARB(parm[18], 1.0); // depthcue

  glUniform3fARB(parm[4], 2,2,2); // delta

  QProgressDialog progress("Reslicing volume",
			   QString(),
			   0, 100,
			   0);
  progress.setCancelButton(0);

  glDisable(GL_DEPTH_TEST);

  glClearDepth(0);
  glClearColor(0,0,0,0);
  
  qint64 nonZeroVoxels=0;
  for(int sl=0; sl<nslices; sl++)
    {
      float sf;
      if (nslices > 1)
	sf = (float)sl/(float)(nslices-1);
      else
	sf = 0.0;
      Vec po = (1.0-sf)*sliceStart + sf*sliceEnd;

      progress.setLabelText(QString("%1").arg(sl));
      progress.setValue(100*(float)sl/(float)nslices);

      glClear(GL_DEPTH_BUFFER_BIT);
      glClear(GL_COLOR_BUFFER_BIT);

      for(int b=slabstart; b<slabend; b++)
	{
	  float tminz = m_dataMin.z;
	  float tmaxz = m_dataMax.z;

//	  if (slabend > 1)
//	    {
//	      tminz = m_textureSlab[b].y;
//	      tmaxz = m_textureSlab[b].z;
//	      
//	      glUniform1iARB(parm[24], (tminz-m_dataMin.z)/lod); // zoffset
//	      glUniform1iARB(parm[27], tminz);
//	    }		  
	  
	  glUniform3fARB(parm[42], m_dataMin.x, m_dataMin.y, tminz);
	  glUniform3fARB(parm[43], m_dataMax.x, m_dataMax.y, tmaxz);
	  
	  bindDataTextures(b);
	  
	  if (nslices > 1 &&
	      Global::interpolationType(Global::TextureInterpolation)) // linear
	    {
	      glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,
			      GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	      glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,
			      GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	    }
	  else
	    {
	      glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,
			      GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	      glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,
			      GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	    }
	  
	  glBegin(GL_QUADS);

	  Vec v = po;
	  glMultiTexCoord3dv(GL_TEXTURE0, v);
	  glVertex3f(0, 0, 0);

	  v = po + endW;
	  glMultiTexCoord3dv(GL_TEXTURE0, v);
	  glVertex3f(wd, 0, 0);

	  v = po + endW + endH;
	  glMultiTexCoord3dv(GL_TEXTURE0, v);
	  glVertex3f(wd, ht, 0);

	  v = po + endH;
	  glMultiTexCoord3dv(GL_TEXTURE0, v);
	  glVertex3f(0, ht, 0);

	  glEnd();
	} // slabs

      if (saveValue)
	glReadPixels(0, 0, wd, ht, GL_RED, GL_UNSIGNED_BYTE, slice);
      else
	glReadPixels(0, 0, wd, ht, GL_GREEN, GL_UNSIGNED_BYTE, slice);

      if (tagValue >= 0)
	{
	  glReadPixels(0, 0, wd, ht, GL_BLUE, GL_UNSIGNED_BYTE, tag);
	  for(int p=0; p<wd*ht; p++)
	    {
	      if (tag[p] != tagValue) slice[p] = 0;
	    }
	}

      if (nslices == 1)
	{
	  for(int p=0; p<wd*ht; p++)
	    if (slice[p] > 0) nonZeroVoxels++;
	}
      else
	pFileManager.setSlice(sl, slice);
    }

  m_shadowBuffer->release();

  progress.setValue(100);

  if (nslices == 1)
    {
      // ask user whether they want to save the slice.
      QFileDialog fdialog(0,
			  "Save cross section as an image",
			  Global::previousDirectory(),
			  "Processed (*.png)");
      fdialog.setAcceptMode(QFileDialog::AcceptSave);
      
      if (fdialog.exec() == QFileDialog::Accepted)
	{
	  QString sfile = fdialog.selectedFiles().value(0);
	  if (!sfile.isEmpty())
	    {
	      if (sfile.endsWith(".png.png")) sfile.chop(4); // remove extra extensions
	      if (!sfile.endsWith(".png")) sfile += ".png"; // add if extension not present
	      uchar* rgba = new uchar[4*wd*ht];
	      for(int i=0; i<wd*ht; i++)
		{
		  rgba[4*i+0] = slice[i];
		  rgba[4*i+1] = slice[i];
		  rgba[4*i+2] = slice[i];
		  rgba[4*i+3] = 255;
		}
	      QImage simg = QImage(rgba, wd, ht, QImage::Format_RGB32);
	      simg = simg.mirrored(false, true);
	      simg.save(sfile);
	      QMessageBox::information(0, "Save cross section",
				   "Cross section saved in "+sfile);
	    }
	}
    }

  delete [] slice;
  if (tagValue >= 0)
    delete [] tag;

  glUseProgramObjectARB(0);
  disableTextureUnits();  

  StaticFunctions::popOrthoView();

  // delete shadow buffer
  if (m_shadowBuffer)  delete m_shadowBuffer;
  m_shadowBuffer = 0;
  //  initShadowBuffers(true);

  glEnable(GL_DEPTH_TEST);

  if (nslices == 1)
    {
      VolumeInformation pvlInfo = VolumeInformation::volumeInformation();
      Vec vs;
      vs.x = VECPRODUCT(xaxis, pvlInfo.voxelSize).norm();
      vs.y = VECPRODUCT(yaxis, pvlInfo.voxelSize).norm();
      vs *= subsample*vlod;
      float voxarea = nonZeroVoxels*vs.x*vs.y;

      QString str;
      str = "Opacity is used to calculate cross sectional area.\n";
      str += "Only non-zero opacity voxel are counted for the calculation.\n\n";
      str += QString("Subsampling Level : %1 - ").arg(vlod);
      if (vlod==1) str += " (i.e. full resolution)\n";
      else str += QString(" (i.e. every %1 voxel)\n").arg(vlod);
      str += QString("\nNon-Zero Voxels : %1\n").arg(nonZeroVoxels);
      str += QString("Cross sectional Area : %1 %2^2\n").	\
	arg(voxarea).						\
	arg(pvlInfo.voxelUnitStringShort());
      
      QMessageBox::information(0, "Volume Calculation", str);
    }
  else
    QMessageBox::information(0, "Saved Resliced Volume",
			     QString("Resliced volume saved to %1 and %1.001").arg(pFile));
}

QString
DrawHiresVolume::getResliceFileName(bool border)
{
  QString pFile;

  QString header = "Save Resliced Volume";
  if (border) // for surface voxels
    {
      QStringList items;
      items << "no" << "yes";
      QString yn = QInputDialog::getItem(0, "Save Border Voxels to Volume",
					 "Want to save border voxels to volume ?",
					 items,
					 0,
					 false);
      
      if (yn == "no") 
	return QString();
      
      header = "Save border voxels to volume";
    }
  //header += QString("(volume size : %1 %2 %3)").arg(wd).arg(ht).arg(nslices);
  
  QFileDialog fdialog(0,
		      header,
		      Global::previousDirectory(),
		      "Processed (*.pvl.nc)");
  
  fdialog.setAcceptMode(QFileDialog::AcceptSave);
  
  if (!fdialog.exec() == QFileDialog::Accepted)
    return QString();
  
  pFile = fdialog.selectedFiles().value(0);
  
  // mac sometimes adds on extra extensions at the end
  if (pFile.endsWith(".pvl.nc.pvl.nc"))
    pFile.chop(7);
  
  // yes again - remove extra extensions at the end
  if (pFile.endsWith(".pvl.nc.pvl.nc"))
    pFile.chop(7);

  if (!pFile.endsWith(".pvl.nc"))
    pFile += ".pvl.nc";
  
  return pFile;
}

bool
DrawHiresVolume::getSaveValue()
{
  bool saveValue = true;

  if (Global::volumeType() == Global::RGBVolume ||
      Global::volumeType() == Global::RGBAVolume)
    saveValue = false;

  QStringList items;
  items << "value" << "opacity";
  QString yn = QInputDialog::getItem(0, "Save Volume",
				     "Save Value or Opacity ?",
				     items,
				     0,
				     false);
  
  if (yn == "opacity") saveValue = false;
  
  return saveValue;
}

void
DrawHiresVolume::saveReslicedVolume(QString pFile,
				    int nslices, int wd, int ht,
				    VolumeFileManager &pFileManager,
				    bool tmpfile, Vec vs,
				    int voxtype)
{
  int slabSize = nslices+1;
  if (QFile::exists(pFile)) QFile::remove(pFile);
  
  pFileManager.setBaseFilename(pFile);
  pFileManager.setDepth(nslices);
  pFileManager.setWidth(ht);
  pFileManager.setHeight(wd);
  pFileManager.setHeaderSize(13);
  pFileManager.setSlabSize(slabSize);
  
  pFileManager.removeFile();
  
  pFileManager.setBaseFilename(pFile);
  pFileManager.setDepth(nslices);
  pFileManager.setWidth(ht);
  pFileManager.setHeight(wd);
  pFileManager.setHeaderSize(13);
  pFileManager.setSlabSize(slabSize);
  if (voxtype == 0)
    pFileManager.setVoxelType(VolumeInformation::_UChar);
  else
    pFileManager.setVoxelType(VolumeInformation::_UShort);
  pFileManager.createFile(true);
  
  
  if (!tmpfile)
    {
      VolumeInformation pvlInfo = VolumeInformation::volumeInformation();
      int vtype = VolumeInformation::_UChar;
      if (voxtype == 2)
	vtype = VolumeInformation::_UShort;
      float vx = vs.x;
      float vy = vs.y;
      float vz = vs.z;
      QList<float> rawMap;
      QList<int> pvlMap;
      for(int i=0; i<pvlInfo.mapping.count(); i++)
	{
	  float f = pvlInfo.mapping[i].x();
	  int b = pvlInfo.mapping[i].y();
	  rawMap << f;
	  pvlMap << b;
	}
      StaticFunctions::savePvlHeader(pFile,
				     false, "",
				     vtype,vtype, pvlInfo.voxelUnit,
				     nslices, ht, wd,
				     vx, vy, vz,
				     rawMap, pvlMap,
				     pvlInfo.description,
				     slabSize);
      
      
    }
}

void
DrawHiresVolume::getTightFit(int sl,
			     uchar* slice, int wd, int ht,
			     bool& zmindone,
			     int& xmin, int& xmax,
			     int& ymin, int& ymax,
			     int& zmin, int& zmax)
{
  if (!zmindone)
    {
      for(int p=0; p<wd*ht; p++)
	if (slice[p] > 0)
	  {
	    zmin = sl;
	    zmax = sl;
	    zmindone = true;
	    break;
	  }
    }
  else
    {
      for(int p=0; p<wd*ht; p++)
	if (slice[p] > 0)
	  {
	    zmax = sl;
	    break;
	  }
    }
  
  // find xmin, xmax
  for(int y=0; y<ht; y++)
    {
      for(int x=0; x<xmin; x++)
	{
	  if (slice[y*wd+x] > 0)
	    {
	      xmin = qMin(xmin, x);
	      break;
	    }
	}
    }
  for(int y=0; y<ht; y++)
    {
      for(int x=wd-1; x>xmax; x--)
	{
	  if (slice[y*wd+x] > 0)
	    {
	      xmax = qMax(xmax, x);
	      break;
	    }
	}
    }
  
  // find ymin, ymax
  for(int x=0; x<wd; x++)
    {
      for(int y=0; y<ymin; y++)
	{
	  if (slice[y*wd+x] > 0)
	    {
	      ymin = qMin(ymin, y);
	      break;
	    }
	}
    }
  for(int x=0; x<wd; x++)
    {
      for(int y=ht-1; y>ymax; y--)
	{
	  if (slice[y*wd+x] > 0)
	    {
	      ymax = qMax(ymax, y);
	      break;
	    }
	}
    }
}

void
DrawHiresVolume::calculateSurfaceArea(int neighbours,
				      int sl, int nslices,
				      uchar* slice0, uchar* slice1, uchar* slice,
				      uchar* slice2,
				      int wd, int ht,
				      bool pFilePresent,
				      VolumeFileManager& pFileManager)
{
  memset(slice2, 0, wd*ht);
  if (sl==0)
    {
      memcpy(slice0, slice, wd*ht);
      memcpy(slice1, slice, wd*ht);
      if (pFilePresent)
	pFileManager.setSlice(sl, slice2);
    }
  else if (sl==1)
    {
      memcpy(slice1, slice, wd*ht);
    }
  else // start surface area calculation from second slice
    {
      if (neighbours == 6)
	{
	  // count border voxels using 6 neighbours
	  for(int y=1; y<ht-1; y++)
	    for(int x=1; x<wd-1; x++)
	      {
		if (slice1[y*wd+x] > 0) // this is middle slice
		  {
		    bool bordervoxel = false;
		    if (slice1[(y-1)*wd+x] == 0) bordervoxel = true;
		    else if (slice1[y*wd+(x-1)] == 0) bordervoxel = true;
		    else if (slice1[(y+1)*wd+x] == 0) bordervoxel = true;
		    else if (slice1[y*wd+(x+1)] == 0) bordervoxel = true;
		    else if (slice0[y*wd+x] == 0) bordervoxel = true;
		    else if (slice[y*wd+x] == 0) bordervoxel = true;
		    if (bordervoxel)
		      slice2[y*wd+x] = 128;
		  }
	      }
	}
      else if (neighbours == 18)
	{
	  // count border voxels using 6 neighbours
	  for(int y=1; y<ht-1; y++)
	    for(int x=1; x<wd-1; x++)
	      {
		if (slice1[y*wd+x] > 0) // this is middle slice
		  {
		    bool bordervoxel = false;
		    for(int yy=y-1; yy<=y+1; yy++)
		      for(int xx=x-1; xx<=x+1; xx++)
			{
			  if (slice1[yy*wd+xx] == 0) // central voxel is on border
			    {
			      bordervoxel = true;
			      break;
			    }
			}
		    if (!bordervoxel)
		      {
			if (slice0[(y-1)*wd+x] == 0) bordervoxel = true;
			else if (slice0[y*wd+(x-1)] == 0) bordervoxel = true;
			else if (slice0[(y+1)*wd+x] == 0) bordervoxel = true;
			else if (slice0[y*wd+(x+1)] == 0) bordervoxel = true;
			else if (slice0[y*wd+x] == 0) bordervoxel = true;
		      }
		    if (!bordervoxel)
		      {
			if (slice[(y-1)*wd+x] == 0) bordervoxel = true;
			else if (slice[y*wd+(x-1)] == 0) bordervoxel = true;
			else if (slice[(y+1)*wd+x] == 0) bordervoxel = true;
			else if (slice[y*wd+(x+1)] == 0) bordervoxel = true;
			else if (slice[y*wd+x] == 0) bordervoxel = true;
		      }

		    if (bordervoxel)
		      slice2[y*wd+x] = 128;
		  }
	      }
	}
      else
	{
	  // count border voxels using 26 neighbours
	  for(int y=1; y<ht-1; y++)
	    for(int x=1; x<wd-1; x++)
	      {
		if (slice1[y*wd+x] > 0) // this is middle slice
		  {
		    bool bordervoxel = false;
		    for(int zz=0; zz<3; zz++)
		      {
			uchar *s = slice0;
			if (zz==1) s = slice1;
			if (zz==2) s = slice;
			for(int yy=y-1; yy<=y+1; yy++)
			  for(int xx=x-1; xx<=x+1; xx++)
			    {
			      if (s[yy*wd+xx] == 0) // central voxel is on border
				{
				  bordervoxel = true;
				  break;
				}
			    }
		      }
		    if (bordervoxel)
		      slice2[y*wd+x] = 128;
		  }
	      }
	} // 26 neighbour

      if (pFilePresent)
	{
	  pFileManager.setSlice(sl-1, slice2);
	  if (sl == nslices-1)
	    {
	      memset(slice2, 0, wd*ht);
	      pFileManager.setSlice(sl, slice2);
	    }
	}
      
      memcpy(slice0, slice1, wd*ht);
      memcpy(slice1, slice, wd*ht);
    }

}

void
DrawHiresVolume::saveForDrishtiPrayog(QString sfile)
{
  fstream fout(sfile.toLatin1().data(), ios::binary|ios::out);

  char keyword[100];

  sprintf(keyword, "drishtiPrayog v1.0");
  fout.write((char*)keyword, strlen(keyword)+1);

  VolumeInformation pvlInfo = VolumeInformation::volumeInformation();
  int vu = pvlInfo.voxelUnit;
  sprintf(keyword, "voxelunit");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&vu, sizeof(int));

  Vec voxelScaling = Global::voxelScaling();
  float f[3];
  f[0] = voxelScaling.x;
  f[1] = voxelScaling.y;
  f[2] = voxelScaling.z;
  sprintf(keyword, "voxelscaling");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&f, 3*sizeof(float));

  int ncols, nrows;
  sprintf(keyword, "columnsrows");
  fout.write((char*)keyword, strlen(keyword)+1);
  m_Volume->getColumnsAndRows(ncols, nrows);
  fout.write((char*)&ncols, sizeof(int));
  fout.write((char*)&nrows, sizeof(int));

  int stexX, stexY;
  sprintf(keyword, "slicetexturesize");
  fout.write((char*)keyword, strlen(keyword)+1);
  m_Volume->getSliceTextureSize(stexX, stexY);
  fout.write((char*)&stexX, sizeof(int));
  fout.write((char*)&stexY, sizeof(int));

  Vec dragInfo = m_Volume->getDragTextureInfo();
  //float f[3];
  f[0] = dragInfo.x;
  f[1] = dragInfo.y;
  f[2] = dragInfo.z;
  sprintf(keyword, "dragtextureinfo");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&f, 3*sizeof(float));

  int dtexX, dtexY;
  sprintf(keyword, "dragtexturesize");
  fout.write((char*)keyword, strlen(keyword)+1);
  m_Volume->getDragTextureSize(dtexX, dtexY);
  fout.write((char*)&dtexX, sizeof(int));
  fout.write((char*)&dtexY, sizeof(int));
 
  sprintf(keyword, "slicetexturesizeslabs");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_dataTexSize, sizeof(int));
  for(int i=0; i<m_dataTexSize; i++)
    {
      f[0] = m_textureSlab[i].x;
      f[1] = m_textureSlab[i].y;
      f[2] = m_textureSlab[i].z;
      fout.write((char*)&f, 3*sizeof(float));
    }

  glActiveTexture(GL_TEXTURE1);
  glEnable(GL_TEXTURE_RECTANGLE_ARB);	    
  int nbytes = qMax(stexX*stexY, dtexX*dtexY);
  uchar *imgData = new uchar[nbytes];
  sprintf(keyword, "slicetextureslabs");
  fout.write((char*)keyword, strlen(keyword)+1);
  for(int i=0; i<m_dataTexSize; i++)
    {
      glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_dataTex[i]);
      glGetTexImage(GL_TEXTURE_RECTANGLE_ARB,
		    0,
		    GL_RED,
		    GL_UNSIGNED_BYTE,
		    imgData);
      if (i > 0 || m_dataTexSize <= 1)  //slice texture
	fout.write((char *)imgData, stexX*stexY);
      else // drag texture
	fout.write((char *)imgData, dtexX*dtexY);
    }
  delete [] imgData;
  glDisable(GL_TEXTURE_RECTANGLE_ARB);	    

  sprintf(keyword, "fullvolumesize");
  fout.write((char*)keyword, strlen(keyword)+1);
  Vec fullVolSize = m_Volume->getFullVolumeSize();
  f[0] = fullVolSize.x;
  f[1] = fullVolSize.y;
  f[2] = fullVolSize.z;
  fout.write((char*)&f, 3*sizeof(float));

  sprintf(keyword, "subvolumesize");
  fout.write((char*)keyword, strlen(keyword)+1);
  Vec subVolSize = m_Volume->getSubvolumeSize();
  f[0] = subVolSize.x;
  f[1] = subVolSize.y;
  f[2] = subVolSize.z;
  fout.write((char*)&f, 3*sizeof(float));

  sprintf(keyword, "subvolumetexturesize");
  fout.write((char*)keyword, strlen(keyword)+1);
  Vec textureSize = m_Volume->getSubvolumeTextureSize();
  f[0] = textureSize.x;
  f[1] = textureSize.y;
  f[2] = textureSize.z;
  fout.write((char*)&f, 3*sizeof(float));

  sprintf(keyword, "subvolumesubsamplinglevel");
  int subsamplingLevel = m_Volume->getSubvolumeSubsamplingLevel();
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&subsamplingLevel, sizeof(int));

  sprintf(keyword, "enddrishtiprayog");
  fout.write((char*)keyword, strlen(keyword)+1);
}

Vec
DrawHiresVolume::pointUnderPixel(QPoint scr, bool& found)
{
  int sw = m_Viewer->camera()->screenWidth();
  int sh = m_Viewer->camera()->screenHeight();  

  Vec pos;
  int cx = scr.x();
  int cy = scr.y();
  GLfloat depth = 0;
  
  m_forceBackToFront = true;
  glEnable(GL_SCISSOR_TEST);
  glScissor(cx, sh-1-cy, 1, 1);
  drawDragImage(0.9);
  glDisable(GL_SCISSOR_TEST);
  m_forceBackToFront = false;

  glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
  glReadPixels(cx, sh-1-cy, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
  glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);

  found = false;
  if (depth > 0.0 && depth < 1.0)
    {
      pos = m_Viewer->camera()->unprojectedCoordinatesOf(Vec(cx, cy, depth));
      found = true;
    }

  return pos;
}
