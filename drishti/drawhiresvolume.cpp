#include <stdlib.h>
#include "drawhiresvolume.h"
#include "viewer.h"
#include <QGLWidget>
#include "volume.h"
#include "staticfunctions.h"
#include "matrix.h"
#include "shaderfactory.h"
#include "shaderfactory2.h"
#include "shaderfactoryrgb.h"
#include "global.h"
#include "geometryobjects.h"
#include "tick.h"
#include "enums.h"
#include "prunehandler.h"
#include "mainwindowui.h"

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
  m_highqualityShader=0;
  m_shadowShader=0;
  m_blurShader=0;
  m_backplaneShader1=0;
  m_backplaneShader2=0;

  renew();
}

DrawHiresVolume::~DrawHiresVolume()
{
  cleanup();
}

void
DrawHiresVolume::renew()
{
  m_showing = true;

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
}

void
DrawHiresVolume::cleanup()
{
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
  int shdSizeW = m_Viewer->camera()->screenWidth()*m_lightInfo.shadowScale;
  int shdSizeH = m_Viewer->camera()->screenHeight()*m_lightInfo.shadowScale;
  shdSizeW *= 2;
  shdSizeH *= 2;
  if (!force &&
      m_shadowWidth == shdSizeW &&
      m_shadowHeight == shdSizeH)
    return; // no need to resize shadow buffers

  m_shadowWidth = shdSizeW;
  m_shadowHeight = shdSizeH;

  GLuint target = GL_TEXTURE_RECTANGLE_EXT;

  if (Global::useFBO())
    {
      if (m_shadowBuffer) delete m_shadowBuffer;
      if (m_blurredBuffer) delete m_blurredBuffer;

      glActiveTexture(GL_TEXTURE3);
      m_shadowBuffer = new QGLFramebufferObject(QSize(m_shadowWidth,
						      m_shadowHeight),
						QGLFramebufferObject::NoAttachment,
						target);
      glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_shadowBuffer->texture());
      glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

      m_shadowBuffer->release();


      glActiveTexture(GL_TEXTURE2);
      m_blurredBuffer = new QGLFramebufferObject(QSize(m_shadowWidth,
						       m_shadowHeight),
						 QGLFramebufferObject::NoAttachment,
						 target);
      glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_blurredBuffer->texture());

      m_blurredBuffer->release();
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

  m_Volume->startHistogramCalculation();
  loadTextureMemory();
  m_Volume->endHistogramCalculation();

  loadDragTexture();

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

  PruneHandler::updateAndLoadPruneTexture(m_dataTex[0],
					  dtextureX, dtextureY,
					  dragInfo, subVolSize,
					  m_Viewer->lookupTable());

  MainWindowUI::mainWindowUI()->menubar->parentWidget()->\
    setWindowTitle("Drishti");
}

void
DrawHiresVolume::loadDragTexture()
{
  if (Global::volumeType() == Global::DummyVolume)
    return;

  if (m_dataTexSize <= 1) // no drag texture
    return;

  // -- disable screen updates 
  bool uv = Global::updateViewer();
  if (uv)
    {
      Global::disableViewerUpdate();
      MainWindowUI::changeDrishtiIcon(false);
    }

  int nvol = 1;
  if (Global::volumeType() == Global::DoubleVolume) nvol = 2;
  if (Global::volumeType() == Global::TripleVolume) nvol = 3;
  if (Global::volumeType() == Global::QuadVolume) nvol = 4;
  if (Global::volumeType() == Global::RGBVolume) nvol = 3;
  if (Global::volumeType() == Global::RGBAVolume) nvol = 4;
  
  int format = GL_LUMINANCE;
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

  Vec dragInfo = m_Volume->getDragTextureInfo();
  int texX, texY;
  m_Volume->getDragTextureSize(texX, texY);

  glActiveTexture(GL_TEXTURE1);
  glEnable(GL_TEXTURE_RECTANGLE_ARB);
  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_dataTex[0]);	 
  glTexParameterf(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
  glTexParameterf(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
  if (Global::interpolationType(Global::TextureInterpolation)) // linear
    {
      glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
  else
    {
      glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
  
  uchar *textureSlab = m_Volume->getDragTexture();
      
  glTexImage2D(GL_TEXTURE_RECTANGLE_ARB,
	       0, // single resolution
	       internalFormat, 
	       texX, texY,
	       0, // no border
	       format,
	       vtype,
	       textureSlab);
  
  glFlush();
  glFinish();

  if (uv)
    {
      Global::enableViewerUpdate();
      MainWindowUI::changeDrishtiIcon(true);
    }
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

  for(int i=0; i<m_dataTexSize; i++)
    {      
      if (m_dataTexSize > 1 && i == 0) i++; // do not load drag texture right now

      int minz, maxz, texX, texY; 
      minz = m_textureSlab[i].y;
      maxz = m_textureSlab[i].z;
      texX = textureX;
      texY = textureY;
     
      MainWindowUI::mainWindowUI()->statusBar->showMessage(	      \
			    QString("loading slab %1 [%2 %3]"). \
			    arg(i).arg(minz).arg(maxz));

      glActiveTexture(GL_TEXTURE1);
      glEnable(GL_TEXTURE_RECTANGLE_ARB);

      glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_dataTex[i]);
	  
      glTexParameterf(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
      glTexParameterf(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
      if (Global::interpolationType(Global::TextureInterpolation)) // linear
	{
	  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
      else
	{
	  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}
      
      uchar *textureSlab = NULL;

      if (!Global::loadDragOnly())
	textureSlab = m_Volume->getSliceTextureSlab(minz, maxz);
      
      glTexImage2D(GL_TEXTURE_RECTANGLE_ARB,
		   0, // single resolution
		   internalFormat,
		   texX, texY,
		   0, // no border
		   format,
		   vtype,
		   textureSlab);

      glFlush();
      glFinish();
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


void
DrawHiresVolume::createHighQualityShader()
{
  QString shaderString;

  GeometryObjects::trisets()->createHighQualityShader(m_lightInfo.applyShadows,
						      m_lightInfo.shadowIntensity,
						      m_crops);

  GeometryObjects::networks()->createSpriteShader();

  if (Global::volumeType() == Global::SingleVolume ||
      Global::volumeType() == Global::DummyVolume)
    {
      shaderString = ShaderFactory::genHighQualitySliceShaderString((m_Volume->pvlVoxelType(0) > 0),
								    m_lightInfo.applyLighting,
								    m_lightInfo.applyEmissive,
								    m_lightInfo.applyShadows,
								    m_crops,
								    m_lightInfo.peel,
								    m_lightInfo.peelType,
								    m_lightInfo.peelMin,
								    m_lightInfo.peelMax,
								    m_lightInfo.peelMix);
    }
  else if (Global::volumeType() == Global::RGBVolume ||
	   Global::volumeType() == Global::RGBAVolume)
    shaderString = ShaderFactoryRGB::genHighQualitySliceShaderString(
					m_lightInfo.applyLighting,
					m_lightInfo.applyEmissive,
					m_lightInfo.applyShadows,
					m_lightInfo.shadowIntensity,
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
    
      shaderString = ShaderFactory2::genHighQualitySliceShaderString(
						 m_lightInfo.applyLighting,
						 m_lightInfo.applyEmissive,
						 m_lightInfo.applyShadows,
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
  
  if (m_highqualityShader)
    glDeleteObjectARB(m_highqualityShader);

  m_highqualityShader = glCreateProgramObjectARB();
  if (! ShaderFactory::loadShader(m_highqualityShader,
				  shaderString))
    exit(0);
  m_highqualityParm[0] = glGetUniformLocationARB(m_highqualityShader, "lutTex");
  m_highqualityParm[1] = glGetUniformLocationARB(m_highqualityShader, "dataTex");
  m_highqualityParm[2] = glGetUniformLocationARB(m_highqualityShader, "shadowTex");
  if (Global::volumeType() != Global::RGBVolume &&
      Global::volumeType() != Global::RGBAVolume)
    m_highqualityParm[3] = glGetUniformLocationARB(m_highqualityShader, "tfSet");
  else
    m_highqualityParm[3] = glGetUniformLocationARB(m_highqualityShader, "layerSpacing");

  m_highqualityParm[4] = glGetUniformLocationARB(m_highqualityShader, "delta");
  m_highqualityParm[5] = glGetUniformLocationARB(m_highqualityShader, "eyepos");
  m_highqualityParm[6] = glGetUniformLocationARB(m_highqualityShader, "lightpos");
  m_highqualityParm[7] = glGetUniformLocationARB(m_highqualityShader, "ambient");
  m_highqualityParm[8] = glGetUniformLocationARB(m_highqualityShader, "diffuse");
  m_highqualityParm[9] = glGetUniformLocationARB(m_highqualityShader, "specular");
  m_highqualityParm[10]= glGetUniformLocationARB(m_highqualityShader, "speccoeff");
  m_highqualityParm[11] = glGetUniformLocationARB(m_highqualityShader, "paintCenter");
  m_highqualityParm[12] = glGetUniformLocationARB(m_highqualityShader, "paintSize");

  m_highqualityParm[13] = glGetUniformLocationARB(m_highqualityShader, "maskTex");
  m_highqualityParm[14] = glGetUniformLocationARB(m_highqualityShader, "paintTex");

  m_highqualityParm[15] = glGetUniformLocationARB(m_highqualityShader, "gridx");
  m_highqualityParm[16] = glGetUniformLocationARB(m_highqualityShader, "tsizex");
  m_highqualityParm[17] = glGetUniformLocationARB(m_highqualityShader, "tsizey");

  m_highqualityParm[18] = glGetUniformLocationARB(m_highqualityShader, "depthcue");

  m_highqualityParm[19] = glGetUniformLocationARB(m_highqualityShader, "pruneTex");
  m_highqualityParm[20] = glGetUniformLocationARB(m_highqualityShader, "prunegridx");
  m_highqualityParm[21] = glGetUniformLocationARB(m_highqualityShader, "prunetsizex");
  m_highqualityParm[22] = glGetUniformLocationARB(m_highqualityShader, "prunetsizey");
  m_highqualityParm[23] = glGetUniformLocationARB(m_highqualityShader, "prunelod");
  m_highqualityParm[24] = glGetUniformLocationARB(m_highqualityShader, "zoffset");

  m_highqualityParm[25] = glGetUniformLocationARB(m_highqualityShader, "dataMin");
  m_highqualityParm[26] = glGetUniformLocationARB(m_highqualityShader, "dataSize");
  m_highqualityParm[27] = glGetUniformLocationARB(m_highqualityShader, "tminz");

  m_highqualityParm[28] = glGetUniformLocationARB(m_highqualityShader, "lod");
  m_highqualityParm[29] = glGetUniformLocationARB(m_highqualityShader, "dirFront");
  m_highqualityParm[30] = glGetUniformLocationARB(m_highqualityShader, "dirUp");
  m_highqualityParm[31] = glGetUniformLocationARB(m_highqualityShader, "dirRight");

  m_highqualityParm[32] = glGetUniformLocationARB(m_highqualityShader, "interpVol");
  m_highqualityParm[33] = glGetUniformLocationARB(m_highqualityShader, "mixTag");




  m_highqualityParm[42] = glGetUniformLocationARB(m_highqualityShader, "brickMin");
  m_highqualityParm[43] = glGetUniformLocationARB(m_highqualityShader, "brickMax");
}

void DrawHiresVolume::setImageSizeRatio(float ratio) { m_imgSizeRatio = ratio; }

void
DrawHiresVolume::reCreateBlurShader(int blurSize)
{
  createBlurShader(true, blurSize, m_lightInfo.shadowBlur);  
}

void
DrawHiresVolume::createBlurShader(bool doBlur, int blurSize, float blurRadius)
{
  QString shaderString;
  if (blurRadius < 1.0)
    shaderString = ShaderFactory::genBlurShaderString(false, ceil(blurSize*m_imgSizeRatio), 0);
  else
    shaderString = ShaderFactory::genBlurShaderString(doBlur, ceil(blurSize*m_imgSizeRatio), 0.5*blurRadius);

  if (m_blurShader)
    glDeleteObjectARB(m_blurShader);

  m_blurShader = glCreateProgramObjectARB();
  if (! ShaderFactory::loadShader(m_blurShader,
				  shaderString))
    exit(0);
  m_blurParm[0] = glGetUniformLocationARB(m_blurShader, "shadowTex");
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
DrawHiresVolume::createShadowShader(Vec attenuation)
{
  float r = attenuation.x;
  float g = attenuation.y;
  float b = attenuation.z;

  GeometryObjects::trisets()->createShadowShader(attenuation, m_crops);
  GeometryObjects::networks()->createShadowShader(attenuation);

  QString shaderString;

  if (Global::volumeType() == Global::SingleVolume ||
      Global::volumeType() == Global::DummyVolume)
    {
      shaderString = ShaderFactory::genSliceShadowShaderString((m_Volume->pvlVoxelType(0) > 0),
							       m_lightInfo.shadowIntensity,
							       r,g,b,
							       m_crops,
							       m_lightInfo.peel,
							       m_lightInfo.peelType,
							       m_lightInfo.peelMin,
							       m_lightInfo.peelMax,
							       m_lightInfo.peelMix);
    }
  else if (Global::volumeType() == Global::RGBVolume ||
	   Global::volumeType() == Global::RGBAVolume)
    shaderString = ShaderFactoryRGB::genSliceShadowShaderString(r,g,b, m_crops,
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

      shaderString = ShaderFactory2::genSliceShadowShaderString(m_lightInfo.shadowIntensity,
								r,g,b,
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

  if (m_shadowShader)
    glDeleteObjectARB(m_shadowShader);

  m_shadowShader = glCreateProgramObjectARB();
  if (! ShaderFactory::loadShader(m_shadowShader,
				    shaderString))
    exit(0);
  m_shadowParm[0] = glGetUniformLocationARB(m_shadowShader, "lutTex");
  m_shadowParm[1] = glGetUniformLocationARB(m_shadowShader, "dataTex");
  if (Global::volumeType() != Global::RGBVolume &&
      Global::volumeType() != Global::RGBAVolume)
    m_shadowParm[2] = glGetUniformLocationARB(m_shadowShader, "tfSet");
  else
    m_shadowParm[2] = glGetUniformLocationARB(m_shadowShader, "layerSpacing");

  m_shadowParm[3] = glGetUniformLocationARB(m_shadowShader, "maskTex");
  m_shadowParm[4] = glGetUniformLocationARB(m_shadowShader, "paintTex");

  m_shadowParm[5] = glGetUniformLocationARB(m_shadowShader, "eyepos");
  m_shadowParm[6] = glGetUniformLocationARB(m_shadowShader, "gridx");
  m_shadowParm[7] = glGetUniformLocationARB(m_shadowShader, "tsizex");
  m_shadowParm[8] = glGetUniformLocationARB(m_shadowShader, "tsizey");

  m_shadowParm[9] =  glGetUniformLocationARB(m_shadowShader, "pruneTex");
  m_shadowParm[10] = glGetUniformLocationARB(m_shadowShader, "prunegridx");
  m_shadowParm[11] = glGetUniformLocationARB(m_shadowShader, "prunetsizex");
  m_shadowParm[12] = glGetUniformLocationARB(m_shadowShader, "prunetsizey");
  m_shadowParm[13] = glGetUniformLocationARB(m_shadowShader, "prunelod");
  m_shadowParm[14] = glGetUniformLocationARB(m_shadowShader, "zoffset");

  m_shadowParm[25] = glGetUniformLocationARB(m_shadowShader, "dataMin");
  m_shadowParm[26] = glGetUniformLocationARB(m_shadowShader, "dataSize");
  m_shadowParm[27] = glGetUniformLocationARB(m_shadowShader, "tminz");

  m_shadowParm[28] = glGetUniformLocationARB(m_shadowShader, "lod");
  m_shadowParm[29] = glGetUniformLocationARB(m_shadowShader, "dirFront");
  m_shadowParm[30] = glGetUniformLocationARB(m_shadowShader, "dirUp");
  m_shadowParm[31] = glGetUniformLocationARB(m_shadowShader, "dirRight");

  m_shadowParm[32] = glGetUniformLocationARB(m_shadowShader, "interpVol");
  m_shadowParm[33] = glGetUniformLocationARB(m_shadowShader, "mixTag");

  m_shadowParm[42] = glGetUniformLocationARB(m_shadowShader, "brickMin");
  m_shadowParm[43] = glGetUniformLocationARB(m_shadowShader, "brickMax");
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

  m_defaultParm[13] = glGetUniformLocationARB(m_defaultShader, "maskTex");
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




  m_defaultParm[42] = glGetUniformLocationARB(m_defaultShader, "brickMin");
  m_defaultParm[43] = glGetUniformLocationARB(m_defaultShader, "brickMax");
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

  createDefaultShader();
  createHighQualityShader();
  createBlurShader(true, 1, m_lightInfo.shadowBlur);
  createBackplaneShader(m_lightInfo.backplaneIntensity);
  createShadowShader(Vec(1,1,1));
}
//--------------------------------------------------------

void
DrawHiresVolume::enableTextureUnits()
{
  m_Viewer->enableTextureUnits();

  glActiveTexture(GL_TEXTURE1);
  glEnable(GL_TEXTURE_RECTANGLE_ARB);
  glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glActiveTexture(GL_TEXTURE2);
  glEnable(GL_TEXTURE_RECTANGLE_ARB);
  glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glActiveTexture(GL_TEXTURE3);
  glEnable(GL_TEXTURE_RECTANGLE_ARB);
  glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glActiveTexture(GL_TEXTURE6);
  glEnable(GL_TEXTURE_RECTANGLE_ARB);
  glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glActiveTexture(GL_TEXTURE7);
  glEnable(GL_TEXTURE_RECTANGLE_ARB);
  glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void
DrawHiresVolume::disableTextureUnits()
{
  m_Viewer->disableTextureUnits();

  glActiveTexture(GL_TEXTURE1);
  glDisable(GL_TEXTURE_RECTANGLE_ARB);

  glActiveTexture(GL_TEXTURE2);
  glDisable(GL_TEXTURE_RECTANGLE_ARB);

  glActiveTexture(GL_TEXTURE3);
  glDisable(GL_TEXTURE_RECTANGLE_ARB);

  glActiveTexture(GL_TEXTURE4);
  glDisable(GL_TEXTURE_RECTANGLE_ARB);

  glActiveTexture(GL_TEXTURE6);
  glDisable(GL_TEXTURE_RECTANGLE_ARB);

  glActiveTexture(GL_TEXTURE7);
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

//      int lod = m_Volume->getSubvolumeSubsamplingLevel();
//      for (int i=0; i<8; i++)
//	{
//	  Vec tx = subvol[i]-Vec(subcorner.x, subcorner.y, 0);
//	  tx.x /= lod;
//	  tx.y /= lod;
//	  texture[i] = VECDIVIDE(tx, voxelScaling);
//	}
//      QMessageBox::information(0, "", QString("%1 %2 %3\n%4 %5 %6").\
//			       arg(subvol[0].x).\
//			       arg(subvol[0].y).\
//			       arg(subvol[0].z).\
//			       arg(subvol[6].x).\
//			       arg(subvol[6].y).\
//			       arg(subvol[6].z));

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
	  // Cubic Images
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
	glColorMask(GL_TRUE, GL_FALSE, GL_FALSE, GL_TRUE);
      else // right image
	{
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


  Camera defaultCam = *(m_Viewer->camera());

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
  
  *(m_Viewer->camera()) = defaultCam;
  m_Viewer->camera()->setPosition(defaultCamPos);
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

  if (!stillimage || m_renderQuality == Enums::RenderDefault)
    drawDefault(pn, minvert, maxvert, layers, stepsize);
  else if (m_renderQuality == Enums::RenderHighQuality)
    drawHighQuality(pn, minvert, maxvert, layers, stepsize);

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
  glActiveTexture(GL_TEXTURE0);
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
			  QList<bool> clips,
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

  Vec voxelScaling = Global::voxelScaling();
  //---- apply clipping
  for(int ci=0; ci<clips.count(); ci++)
    {
      if (clips[ci])
	{
	  Vec cpo = m_clipPos[ci];
	  Vec cpn =  VECPRODUCT(m_clipNormal[ci], voxelScaling);

	  tedges = 0;
	  for(i=0; i<edges; i++)
	    {
	      Vec v0, v1, t0, t1;
	      
	      v0 = poly[i];
	      t0 = tex[i];
	      if (i<edges-1)
		{
		  v1 = poly[i+1];
		  t1 = tex[i+1];
		}
	      else
		{
		  v1 = poly[0];
		  t1 = tex[0];
		}
	      
	      // clip on texture coordinates
	      int ret = StaticFunctions::intersectType2WithTexture(cpo, cpn,
								   t0, t1,
								   v0, v1);
	      if (ret)
		{
		  tpoly[tedges] = v0;
		  ttex[tedges] = t0;
		  tedges ++;
		  if (ret == 2)
		    {
		      tpoly[tedges] = v1;
		      ttex[tedges] = t1;
		      tedges ++;
		    }
		}
	    }
	  edges = tedges;
	  for(i=0; i<tedges; i++)
	    poly[i] = tpoly[i];
	  for(i=0; i<tedges; i++)
	    tex[i] = ttex[i];
	}
    }
  //---- clipping applied


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


Vec
DrawHiresVolume::getMinZ(QList<Vec> v)
{
  float maxD;
  Vec pv, scr, scr0;
  maxD = -1;
  scr0 = m_Viewer->camera()->projectedCoordinatesOf(v[0]);
  for(int i=1; i<v.count(); i++)
    {
      scr = m_Viewer->camera()->projectedCoordinatesOf(v[i]);
      float d = (scr0.x-scr.x)*(scr0.x-scr.x) +
	        (scr0.y-scr.y)*(scr0.y-scr.y);
      if (i == 1 || maxD < d)
	{
	  pv = v[i];
	  maxD = d;
	}
    }
  return pv;
}

void
DrawHiresVolume::postDrawGeometry()
{
  glDisable(GL_CLIP_PLANE0);
  glDisable(GL_CLIP_PLANE1);
}

void
DrawHiresVolume::preDrawGeometry(int s, int layers,
				 Vec po, Vec pn, Vec step)
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
	}
      if (s < layers-1)
	{
	  eqn[0] = pn.x;
	  eqn[1] = pn.y;
	  eqn[2] = pn.z;
	  eqn[3] = -pn*(po-step);
	  glEnable(GL_CLIP_PLANE1);
	  glClipPlane(GL_CLIP_PLANE1, eqn);      
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
	}
      if (s < layers-1)
	{
	  eqn[0] = -pn.x;
	  eqn[1] = -pn.y;
	  eqn[2] = -pn.z;
	  eqn[3] = pn*(po+step);
	  glEnable(GL_CLIP_PLANE1);
	  glClipPlane(GL_CLIP_PLANE1, eqn);
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
      
      drawGeometry(pstart, pend, pstep,
		   shadow, shadowshader, eyepos);
      
      postDrawGeometry();
      
      glEnable(GL_DEPTH_TEST);
    }
}

void
DrawHiresVolume::drawGeometry(float pnear, float pfar, Vec step,
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
  GeometryObjects::paths()->draw(m_Viewer, m_backlit, m_lightPosition);
  GeometryObjects::grids()->draw(m_Viewer, m_backlit, m_lightPosition);
  GeometryObjects::pathgroups()->draw(m_Viewer, m_backlit, m_lightPosition, true);
  GeometryObjects::hitpoints()->draw(m_Viewer, m_backlit);

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


  if (defaultShader) // defaultShader
    {
      glUniform1iARB(m_defaultParm[15], gridx);
      glUniform1iARB(m_defaultParm[16], lenx2);
      glUniform1iARB(m_defaultParm[17], leny2);
      glUniform1fARB(m_defaultParm[23], plod); // prunelod
      glUniform1iARB(m_defaultParm[28], lod);
    }
  else
    {
      glUniform1iARB(m_highqualityParm[15], gridx); // dataTex2
      glUniform1iARB(m_highqualityParm[16], lenx2);
      glUniform1iARB(m_highqualityParm[17], leny2);
      glUniform1fARB(m_highqualityParm[23], plod); // prunelod
      glUniform1iARB(m_highqualityParm[28], lod);
    }
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

  if (Global::volumeType() != Global::DummyVolume)
    {
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

      glUniform1iARB(m_defaultParm[13], 4);
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

  if (Global::volumeType() == Global::RGBVolume ||
      Global::volumeType() == Global::RGBAVolume)
    {
      float frc = Global::stepsizeStill();
      glUniform1fARB(m_defaultParm[3], frc);
    }  
}

void
DrawHiresVolume::drawDefault(Vec pn,
			     Vec minvert, Vec maxvert,
			     int layers, float stepsize)
{
  glEnable(GL_DEPTH_TEST);

//  m_backlit = false; // going to render front to back
//  glDepthFunc(GL_GEQUAL); // front to back rendering
//  glClearDepth(0);
//  glClear(GL_DEPTH_BUFFER_BIT);

  m_backlit = true; // going to render back to front
  glDepthFunc(GL_LEQUAL); // back to front rendering
  glClearDepth(1);
  glClear(GL_DEPTH_BUFFER_BIT);
  

  enableTextureUnits();

  setRenderDefault();

  glEnable(GL_BLEND);
  if (!m_backlit)
    glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE); // for frontlit volume
  else
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // back to front

  drawSlicesDefault(pn, minvert, maxvert,
		    layers, stepsize);

  glDisable(GL_BLEND);
  disableTextureUnits();  
  glUseProgramObjectARB(0);

  m_bricks->draw();
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

  QList<int> thickness = GeometryObjects::clipplanes()->thickness();
  QList<QVector4D> viewport = GeometryObjects::clipplanes()->viewport();
  QList<int> tfset = GeometryObjects::clipplanes()->tfset();
  QList<bool> applyclip = GeometryObjects::clipplanes()->applyClip();
  Vec voxelScaling = Global::voxelScaling();
  for(int i=0; i<m_clipPos.count(); i++)
    {
      QVector4D vp = viewport[i];
      // change clip position when textured plane and viewport active
      if (tfset[i] >= 0 &&
	  tfset[i] < Global::lutSize() &&
	  vp.x() >= 0.0)
	{
	  Vec tang = VECDIVIDE(m_clipNormal[i], voxelScaling);
	  m_clipPos[i] += tang*thickness[i];
	}
    }

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

      // modify bclip to reflect applyclip
      for(int ci=0; ci<bclips.count(); ci++)
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
		   clips[bno],
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
  for (int ic=0; ic<m_clipPos.count(); ic++)
    {
      ViewAlignedPolygon *vap = new ViewAlignedPolygon;
      Vec cpos = VECPRODUCT(m_clipPos[ic], voxelScaling);
      drawpoly(cpos, m_clipNormal[ic],
	       bsubvol, btexture,
	       dummyclips,
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
}

void
DrawHiresVolume::bindDataTextures(int b)
{
  glActiveTexture(GL_TEXTURE1);
  glEnable(GL_TEXTURE_RECTANGLE_ARB);
  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_dataTex[b]);
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
    {
      if (type == 0)
	glUniform1iARB(m_defaultParm[24], (tminz-m_dataMin.z)/lod); // zoffset
      else if (type == 1)
	glUniform1iARB(m_highqualityParm[24], (tminz-m_dataMin.z)/lod); // zoffset
      if (type == 2)
	glUniform1iARB(m_shadowParm[14], (tminz-m_dataMin.z)/lod); // zoffset
    }
			  
  Vec vp0 = Vec(0, 0, tminz);
  Vec vp1 = Vec(0, 0, tmaxz);
			  
  clipSlab(vp0, vp1, edges, poly, tex, sha);
			  
  if (edges == 0)
    return;

  if (type == 0)
    glUniform1iARB(m_defaultParm[27], tminz);
  else if (type == 1)
    glUniform1iARB(m_highqualityParm[27], tminz);
  else if (type == 2)
    glUniform1iARB(m_shadowParm[27], tminz);

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
DrawHiresVolume::drawSlicesDefault(Vec pn, Vec minvert, Vec maxvert,
				   int layers, float stepsize)
{
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


  //-------------------------------
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


  Vec pnDir = step;
  if (m_backlit)
    pnDir = -step;
  int pno = 0;
  Vec po = poStart;
  for(int s=0; s<layers; s++)
    {
      po += pnDir;

      float depthcue = 1;     
      if (Global::depthcue())
	{
	  float sdist = qAbs((maxvert - po)*pn);
	  depthcue = 1.0 - qBound(0.0f, sdist/deplen, 0.95f);
	}

      if (s%Global::geoRenderSteps() == 0 || s == layers-1)
	renderGeometry(s, layers,
		       po, pn, Global::geoRenderSteps()*step,
		       false, false, Vec(0,0,0),
		       false);
		     
      //------------------------------------------------------
      if (Global::volumeType() != Global::DummyVolume)
	{
	  enableTextureUnits();
	  glUseProgramObjectARB(m_defaultShader);
      
	  for (int bno=0; bno<m_numBricks; bno++)
	    {
	      Vec lp = lpos[bno];
	      glUniform3fARB(m_defaultParm[6], lp.x, lp.y, lp.z);

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
	  
	      ViewAlignedPolygon *vap = m_polygon[pno];
	      pno++;
	  
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
    
		      if (m_drawImageType != Enums::DragImage ||
			  m_dataTexSize == 1)
			renderSlicedSlice(0,
					  vap, false,
					  lenx2, leny2, b, lod);
		      else
			renderDragSlice(vap, false, dragTexsize);
		      
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
    } // loop over s


  //------------------------------------------------------
  glUseProgramObjectARB(0);
  disableTextureUnits();	  
  Vec voxelScaling = Global::voxelScaling();
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
  if (defaultShader)
    parm = m_defaultParm;
  else
    parm = m_highqualityParm;

  glUniform3fARB(parm[6], lpos.x, lpos.y, lpos.z);

  if (slabend > 1)
    setShader2DTextureParameter(true, defaultShader);
  else
    setShader2DTextureParameter(false, defaultShader);
  
  int ow = m_Viewer->width();
  int oh = m_Viewer->height();

  m_Viewer->startScreenCoordinatesSystem();
  glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE); // for frontlit volume
  //glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // back to front

  //glDisable(GL_DEPTH_TEST);
  //glDepthMask(GL_FALSE);

  Vec voxelScaling = VolumeInformation::volumeInformation().voxelSize;
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
//	  vx = vp.x()*ow;
//	  vy = oh-vp.y()*oh;
//	  vw = vp.z()*ow;
//	  vh = vp.w()*oh;
//	  vx+=1; vy+=1;
//	  vw-=2; vh-=2;
//	  mh = vy-vh/2;
//	  mw = vx+vw/2;
//	  shiftx = 5;

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

	  float pathLength = po[i].length();
	  QList<Vec> pathPoints = po[i].pathPoints();
	  QList<Vec> pathX = po[i].pathX();
	  QList<Vec> pathY = po[i].pathY();
	  QList<float> radX = po[i].pathradX();
	  QList<float> radY = po[i].pathradY();

	  for(int np=0; np<pathPoints.count(); np++)
	    pathPoints[np] = VECPRODUCT(voxelScaling,pathPoints[np]);
	  for(int np=0; np<pathPoints.count(); np++)
	    pathX[np] = VECPRODUCT(voxelScaling,pathX[np]);
	  for(int np=0; np<pathPoints.count(); np++)
	    pathY[np] = VECPRODUCT(voxelScaling,pathY[np]);


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

	  float scale = (float)(vw-11)/pathLength;
	  if (2*maxheight*scale > vh-21)
	    scale = (float)(vh-21)/(2*maxheight);

	  shiftx = 5 + ((vw-11) - (pathLength*scale))*0.5;

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
	      float tk = (float)nt/(float)(maxthick-1);
	      glUniform1fARB(parm[18], 1.0-0.8*tk); 
	      
	      for(int b=slabstart; b<slabend; b++)
		{
		  float tminz = m_dataMin.z;
		  float tmaxz = m_dataMax.z;
		  if (slabend > 1)
		    {
		      tminz = m_textureSlab[b].y;
		      tmaxz = m_textureSlab[b].z;

		      glUniform1iARB(parm[24], (tminz-m_dataMin.z)/lod); // zoffset
		      glUniform1iARB(parm[27], tminz);
		    }		  
		  
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
			clen += (pathPoints[np]-pathPoints[np-1]).norm();
		      float frc = clen;
		      
		      float lenradx = pathX[np].norm()*radX[np];
		      Vec tv0 = pathPoints[np];
		      Vec tv1 = pathPoints[np]+pathX[np]*radX[np];
		      Vec tv2 = pathPoints[np]-pathX[np]*radX[np];
		      tv1 += pathY[np]*radY[np]*tk;
		      tv2 += pathY[np]*radY[np]*tk;
		      tv1 = VECDIVIDE(tv1, voxelScaling);
		      tv2 = VECDIVIDE(tv2, voxelScaling);

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
  if (defaultShader)
    glUseProgramObjectARB(m_defaultShader);
  else
    glUseProgramObjectARB(m_highqualityShader);  
  glEnable(GL_DEPTH_TEST);

  if (!m_backlit)
    glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE); // for frontlit volume
  else
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // back to front
  // --------------------------------

  glDepthMask(GL_TRUE);

  if (slabend > 1) // reset it to 0
    glUniform1iARB(parm[24], 0); // zoffset    
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

  int slabstart, slabend;
  slabstart = 1;
  slabend = m_dataTexSize;
  if (m_dataTexSize == 1)
    {
      slabstart = 0;
      slabend = 1;
    }
  setShader2DTextureParameter(true, defaultShader);


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
  
  if (defaultShader)
    parm = m_defaultParm;
  else
    parm = m_highqualityParm;

  glUniform3fARB(parm[6], lpos.x, lpos.y, lpos.z);

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

  for (int ic=0; ic<nclipPlanes; ic++)
    {
      QVector4D vp = clipInfo.viewport[ic];
      // render only when textured plane and viewport active
      if (clipInfo.tfSet[ic] >= 0 &&
	  clipInfo.tfSet[ic] < Global::lutSize() &&
	  vp.x() >= 0.0)
	{
	  // change camera settings
	  //----------------
	  m_Viewer->camera()->setOrientation(clipInfo.rot[ic]);

	  Vec cpos = VECPRODUCT(clipInfo.pos[ic], voxelScaling);
	  Vec clipcampos = cpos - 
	    m_Viewer->camera()->viewDirection()*m_Viewer->sceneRadius()*2*(1.0/clipInfo.viewportScale[ic]);
	  
	  m_Viewer->camera()->setPosition(clipcampos);
	  //m_Viewer->camera()->setSceneCenter(cpos);

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
	  for (int sl = -clipInfo.thickness[ic]; sl<=clipInfo.thickness[ic]; sl++)
	    {
	      int sls = sl;
	      if (!defaultShader && !m_backlit) sls = -sl;

	      drawpoly(cpos+sls*m_clipNormal[ic],
		       m_clipNormal[ic],
		       subvol, texture,
		       dummyclips,
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
		  if (slabend > 1)
		    {
		      tminz = m_textureSlab[b].y;
		      tmaxz = m_textureSlab[b].z;
		    }
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
		  
//		  if (m_drawImageType != Enums::DragImage ||
//		      m_dataTexSize == 1)
		    renderSlicedSlice((defaultShader ? 0 : 1),
				      vap, false,
				      lenx2, leny2, b, lod);
//		  else
//		    renderDragSlice(vap, false, dragTexsize);		  
		} // loop over b
	    } // loop over clip slab
	  delete vap;

	  disableTextureUnits();  
	  glUseProgramObjectARB(0);

	  glDisable(GL_DEPTH_TEST);
	  
	  GeometryObjects::clipplanes()->drawOtherSlicesInViewport(m_Viewer, ic);

	  GLdouble eqn[4];
	  eqn[0] = -m_clipNormal[ic].x;
	  eqn[1] = -m_clipNormal[ic].y;
	  eqn[2] = -m_clipNormal[ic].z;
	  eqn[3] = m_clipNormal[ic]*(cpos+m_clipNormal[ic]*(clipInfo.thickness[ic]+0.5));
	  glEnable(GL_CLIP_PLANE0);
	  glClipPlane(GL_CLIP_PLANE0, eqn);

	  eqn[0] = m_clipNormal[ic].x;
	  eqn[1] = m_clipNormal[ic].y;
	  eqn[2] = m_clipNormal[ic].z;
	  eqn[3] = -m_clipNormal[ic]*(cpos-m_clipNormal[ic]*(clipInfo.thickness[ic]+0.5));
	  glEnable(GL_CLIP_PLANE1);
	  glClipPlane(GL_CLIP_PLANE1, eqn);      

	  GeometryObjects::hitpoints()->draw(m_Viewer, m_backlit);

	  GeometryObjects::paths()->draw(m_Viewer, m_backlit, m_lightPosition);

	  glDisable(GL_CLIP_PLANE0);
	  glDisable(GL_CLIP_PLANE1);

	  GeometryObjects::paths()->postdrawInViewport(m_Viewer,
						       vp, cpos,
						       m_clipNormal[ic], clipInfo.thickness[ic]);


	  enableTextureUnits();
	  if (defaultShader)
	    glUseProgramObjectARB(m_defaultShader);
	  else
	    glUseProgramObjectARB(m_highqualityShader);

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

  // --------------------------------
  // draw viewport borders
  disableTextureUnits();  
  glUseProgramObjectARB(0);  
  glDisable(GL_DEPTH_TEST);

  GeometryObjects::clipplanes()->drawViewportBorders(m_Viewer);

  enableTextureUnits();
  if (defaultShader)
    glUseProgramObjectARB(m_defaultShader);
  else
    glUseProgramObjectARB(m_highqualityShader);  
  glEnable(GL_DEPTH_TEST);
  // --------------------------------
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
		  po, pn, step);

  glUniform3fARB(m_defaultParm[6], lpos.x, lpos.y, lpos.z);

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
	      if (slabend > 1)
		{
		  tminz = m_textureSlab[b].y;
		  tmaxz = m_textureSlab[b].z;
		}
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
	      
	      if (m_drawImageType != Enums::DragImage ||
		  m_dataTexSize == 1)
		renderSlicedSlice(0,
				  vap, false,
				  lenx2, leny2, b, lod);
	      else
		renderDragSlice(vap, false, dragTexsize);		  
	    } // loop over b
	} // valid tfset
    } // loop over clipplanes

  postDrawGeometry();
}

void
DrawHiresVolume::drawClipPlaneHighQuality(int s, int layers,
					  Vec po, Vec pn, Vec step,
					  int clipOffset, Vec lpos, float depthcue,
					  int slabstart, int slabend,
					  int lenx2, int leny2, int lod,
					  Vec dragTexsize)
{
  preDrawGeometry(s, layers,
		  po, pn, step);

  glUniform3fARB(m_highqualityParm[6], lpos.x, lpos.y, lpos.z);

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
	  if (vap->edges > 0)
	    {
	      if (Global::volumeType() != Global::RGBVolume &&
		  Global::volumeType() != Global::RGBAVolume)
		glUniform1fARB(m_highqualityParm[3], (float)tfset[ic]/(float)Global::lutSize());
	      else
		{
		  float frc = Global::stepsizeStill();
		  glUniform1fARB(m_highqualityParm[3], frc);
		}
	      
	      glUniform1fARB(m_highqualityParm[18], depthcue);
	      
	      for(int b=slabstart; b<slabend; b++)
		{
		  float tminz = m_dataMin.z;
		  float tmaxz = m_dataMax.z;
		  if (slabend > 1)
		    {
		      tminz = m_textureSlab[b].y;
		      tmaxz = m_textureSlab[b].z;
		    }
		  glUniform3fARB(m_highqualityParm[42], m_dataMin.x, m_dataMin.y, tminz);
		  glUniform3fARB(m_highqualityParm[43], m_dataMax.x, m_dataMax.y, tmaxz);

		  bindDataTextures(b);
		  
		  if (!Global::useDragVolume() || m_dataTexSize == 1)
		    renderSlicedSlice(1,
				      vap, true,
				      lenx2, leny2, b, lod);
		  else
		    renderDragSlice(vap, true, dragTexsize);
		} // loop over b
	    } // valid edges
	} // valid tfset
    } // loop over clipplanes

  postDrawGeometry();
}

void
DrawHiresVolume::drawClipPlaneShadow(int s, int layers,
				     Vec po, Vec pn, Vec step,
				     int clipOffset,
				     int slabstart, int slabend,
				     int lenx2, int leny2, int lod,
				     Vec dragTexsize)
{
  preDrawGeometry(s, layers,
		  po, pn, step);

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
	  
	  if (vap->edges > 0)
	    {
	      if (Global::volumeType() != Global::RGBVolume &&
		  Global::volumeType() != Global::RGBAVolume)
		glUniform1fARB(m_shadowParm[2], (float)tfset[ic]/(float)Global::lutSize());
	      else
		{
		  float frc = Global::stepsizeStill();
		  glUniform1fARB(m_shadowParm[2], frc);
		}
	      
	      for(int b=slabstart; b<slabend; b++)
		{
		  float tminz = m_dataMin.z;
		  float tmaxz = m_dataMax.z;
		  if (slabend > 1)
		    {
		      tminz = m_textureSlab[b].y;
		      tmaxz = m_textureSlab[b].z;
		    }
		  glUniform3fARB(m_shadowParm[42], m_dataMin.x, m_dataMin.y, tminz);
		  glUniform3fARB(m_shadowParm[43], m_dataMax.x, m_dataMax.y, tmaxz);

		  bindDataTextures(b);
		  
		  if ((Global::useDragVolume() ||
		       Global::useDragVolumeforShadows()) &&
		      m_dataTexSize > 1)
		    renderDragSlice(vap, false, dragTexsize);
		  else			      
		    renderSlicedSlice(2,
				      vap, false,
				      lenx2, leny2, b, lod);
		} // loop over b
	    } // valid edges
	} // valid tfset
    } // loop over clipplanes

  postDrawGeometry();
}

void
DrawHiresVolume::drawHighQuality(Vec pn,
				 Vec minvert, Vec maxvert,
				 int layers, float stepsize)
{
  //--------- draw in fbo -----------
  if (m_Viewer->drawToFBO() || Global::imageQuality() != Global::_NormalQuality)
    {
      if (m_Viewer->imageBuffer()->bind())
	glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
    }
  //---------------------------------


  if (m_Viewer->camera()->viewDirection() * m_lightVector < 0)
    m_backlit = true;
  else
    m_backlit = false;


  glEnable(GL_DEPTH_TEST);
  if (!m_backlit)
    {
      glDepthFunc(GL_GEQUAL); // front to back rendering
      glClearDepth(0);
      glClear(GL_DEPTH_BUFFER_BIT);
    }
  else
    {
      glDepthFunc(GL_LEQUAL); // back to front rendering
      glClearDepth(1);
      glClear(GL_DEPTH_BUFFER_BIT);
    }

  enableTextureUnits();

  if (Global::useFBO())
    {
      if (m_blurredBuffer->bind())
	{
	  glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
	  glClearColor(0, 0, 0, 0);
	  glClear(GL_COLOR_BUFFER_BIT);
	  m_blurredBuffer->release();
	}
      else
	QMessageBox::information(0, "this", "Cannot bind blurredBuffer");
      
      if (m_shadowBuffer->bind())
	{
	  glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
	  glClearColor(0, 0, 0, 0);
	  glClear(GL_COLOR_BUFFER_BIT);
	  m_shadowBuffer->release();
	}
      else
	QMessageBox::information(0, "this", "Cannot bind shadowBuffer");

      //--------- draw in fbo -----------
      if (m_Viewer->drawToFBO() || Global::imageQuality() != Global::_NormalQuality)
	{
	  if (m_Viewer->imageBuffer()->bind())
	    glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
	}
      //---------------------------------
    }


  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE); // front to back

  drawSlicesHighQuality(pn, minvert, maxvert,
			layers, stepsize);

  glDisable(GL_BLEND);
  disableTextureUnits();  
  glUseProgramObjectARB(0);
}


void
DrawHiresVolume::setRenderToScreen(Vec defaultCamPos,
				   Vec defaultViewDirection,
				   int camWidth, int camHeight,
				   float defaultFov)
{
  Vec voxelScaling = Global::voxelScaling();
  Vec bpos = VECPRODUCT(m_bricks->getTranslation(0), voxelScaling);
  Vec bpivot = VECPRODUCT(m_bricks->getPivot(0),voxelScaling);
  Vec baxis = m_bricks->getAxis(0);
  float bangle = m_bricks->getAngle(0);
  Quaternion q(baxis, -DEG2RAD(bangle));    

  //--------- draw in fbo -----------
  if (m_Viewer->drawToFBO() || Global::imageQuality() != Global::_NormalQuality)
    {
      if (m_Viewer->imageBuffer()->bind())
	glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
    }
  //---------------------------------

  m_Viewer->camera()->setPosition(defaultCamPos);
  m_Viewer->camera()->setViewDirection(defaultViewDirection);
  m_Viewer->camera()->setScreenWidthAndHeight(camWidth, camHeight);
  m_Viewer->camera()->setFieldOfView(defaultFov);
  m_Viewer->camera()->setFocusDistance(m_focusDistance);
  loadCameraMatrices();

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


  if (!m_backlit)
    glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE); // for frontlit volume
  else
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // back to front

  if (Global::volumeType() == Global::DummyVolume)
    return;

  glUseProgramObjectARB(m_highqualityShader);
  glUniform1iARB(m_highqualityParm[0], 0); // lutTex
  glUniform1iARB(m_highqualityParm[1], 1); // dataTex

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
      
  glUniform1iARB(m_highqualityParm[19], 6); // pruneTex
  glUniform1iARB(m_highqualityParm[20], pgridx); // prunegridx
  glUniform1iARB(m_highqualityParm[21], ptsizex); // prunetsizex
  glUniform1iARB(m_highqualityParm[22], ptsizey); // prunetsizey
  glUniform1fARB(m_highqualityParm[23], plod); // prunelod
  glUniform1iARB(m_highqualityParm[24], 0); // zoffset
  //----------------------------------


  bool useAllTex = (!Global::useDragVolume() || m_dataTexSize == 1);
  setShader2DTextureParameter(useAllTex, false);

  glUniform2fARB(m_highqualityParm[25], m_dataMin.x, m_dataMin.y);
  glUniform2fARB(m_highqualityParm[26], m_dataSize.x, m_dataSize.y);
  if (m_drawImageType == Enums::DragImage &&
      m_dataTexSize != 1)
    glUniform1iARB(m_highqualityParm[27], m_dataMin.z);
  
  {
    Vec front = m_Viewer->camera()->viewDirection();
    Vec up = m_Viewer->camera()->upVector();
    Vec right = m_Viewer->camera()->rightVector();
    front = q.rotate(front);
    up = q.rotate(up);
    right = q.rotate(right);
    glUniform3fARB(m_highqualityParm[29], front.x, front.y, front.z);
    glUniform3fARB(m_highqualityParm[30], up.x, up.y, up.z);
    glUniform3fARB(m_highqualityParm[31], right.x, right.y, right.z);
  }

  glUniform1fARB(m_highqualityParm[32], m_interpVol);
  glUniform1iARB(m_highqualityParm[33], m_mixTag);

  glUniform1iARB(m_highqualityParm[2], 2); // blurredTex

  glUniform1iARB(m_highqualityParm[13], 4);
  glUniform1iARB(m_highqualityParm[14], 5);

  // -- m_highqualityParm[3] -- tfSet for bricks --

  Vec textureSize = m_Volume->getSubvolumeTextureSize();
  Vec delta = Vec(1.0/textureSize.x,
		  1.0/textureSize.y,
		  1.0/textureSize.z);
  Vec eyepos = m_Viewer->camera()->position();

  eyepos -= bpos;
  eyepos -= bpivot;
  eyepos = q.rotate(eyepos);
  eyepos += bpivot; 
    
  glUniform3fARB(m_highqualityParm[4], delta.x, delta.y, delta.z);
  glUniform3fARB(m_highqualityParm[5], eyepos.x, eyepos.y, eyepos.z);
  glUniform3fARB(m_highqualityParm[6], m_lightPosition.x,
		                       m_lightPosition.y,
		                       m_lightPosition.z);

  glUniform1fARB(m_highqualityParm[7], m_lightInfo.highlights.ambient);
  glUniform1fARB(m_highqualityParm[8], m_lightInfo.highlights.diffuse);
  glUniform1fARB(m_highqualityParm[9], m_lightInfo.highlights.specular);
  glUniform1fARB(m_highqualityParm[10], (int)pow((float)2,(float)(m_lightInfo.highlights.specularCoefficient)));
}

void
DrawHiresVolume::setViewFromLight()
{
  m_Viewer->camera()->setPosition(m_lightPosition);
  m_Viewer->camera()->setViewDirection(m_lightVector);

  m_Viewer->camera()->setFieldOfView(M_PI/4.0f + 2*m_lightInfo.shadowFovOffset);
  //m_Viewer->camera()->setFocusDistance(m_focusDistance);

  m_Viewer->camera()->setScreenWidthAndHeight(m_shadowWidth,
					      m_shadowHeight);
  m_Viewer->camera()->loadProjectionMatrix(true);
  m_Viewer->camera()->loadModelViewMatrix(true);

  glViewport(0,0, m_shadowWidth, m_shadowHeight);
}

void
DrawHiresVolume::setRenderToShadowBuffer()
{
  Vec voxelScaling = Global::voxelScaling();
  Vec bpos = VECPRODUCT(m_bricks->getTranslation(0), voxelScaling);
  Vec bpivot = VECPRODUCT(m_bricks->getPivot(0),voxelScaling);
  Vec baxis = m_bricks->getAxis(0);
  float bangle = m_bricks->getAngle(0);
  Quaternion q(baxis, -DEG2RAD(bangle));    

  glClearColor(0,0,0,0);
  glClear(GL_COLOR_BUFFER_BIT);

  setViewFromLight();

  if (Global::volumeType() == Global::DummyVolume)
    return;

  glUseProgramObjectARB(m_shadowShader);
  glUniform1iARB(m_shadowParm[0], 0); // lutTex
  glUniform1iARB(m_shadowParm[1], 1); // dataTex

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

  if (m_drawImageType == Enums::DragImage ||
      Global::useDragVolumeforShadows())
    plod = 1;
  
  glUniform1iARB(m_shadowParm[9], 6); // pruneTex
  glUniform1iARB(m_shadowParm[10], pgridx); // prunegridx
  glUniform1iARB(m_shadowParm[11], ptsizex); // prunetsizex
  glUniform1iARB(m_shadowParm[12], ptsizey); // prunetsizey
  glUniform1fARB(m_shadowParm[13], plod); // prunelod
  glUniform1iARB(m_shadowParm[14], 0); // zoffset
  //----------------------------------


  Vec eyepos = m_Viewer->camera()->position();
  eyepos -= bpos;
  eyepos -= bpivot;
  eyepos = q.rotate(eyepos);
  eyepos += bpivot; 
  glUniform3fARB(m_shadowParm[5], eyepos.x, eyepos.y, eyepos.z);


  int gridx, gridy, lenx2, leny2, lod;
  if ((Global::useDragVolume() ||
       Global::useDragVolumeforShadows()) &&
      m_dataTexSize > 1)
    {
      Vec draginfo = m_Volume->getDragTextureInfo();
      gridx = draginfo.x;
      int lenx = (m_dataSize.x+1);
      int leny = (m_dataSize.y+1);
      lod = draginfo.z;
      lenx2 = lenx/lod;
      leny2 = leny/lod;
    }
  else
    {
      m_Volume->getColumnsAndRows(gridx, gridy);
      int lenx = m_dataSize.x+1;
      int leny = m_dataSize.y+1;
      lod = m_Volume->getSubvolumeSubsamplingLevel();
      lenx2 = lenx/lod;
      leny2 = leny/lod;
    }
  glUniform1iARB(m_shadowParm[6], gridx); // dataTex2
  glUniform1iARB(m_shadowParm[7], lenx2);
  glUniform1iARB(m_shadowParm[8], leny2);

  //if (m_crops.count() > 0)
    {
      glUniform2fARB(m_shadowParm[25], m_dataMin.x, m_dataMin.y);
      glUniform2fARB(m_shadowParm[26], m_dataSize.x, m_dataSize.y);
      if (m_drawImageType == Enums::DragImage &&
	  m_dataTexSize != 1)
	glUniform1iARB(m_shadowParm[27], m_dataMin.z);
    }
  glUniform1iARB(m_shadowParm[28], lod);

  {
    Vec front = m_Viewer->camera()->viewDirection();
    Vec up = m_Viewer->camera()->upVector();
    Vec right = m_Viewer->camera()->rightVector();
    front = q.rotate(front);
    up = q.rotate(up);
    right = q.rotate(right);
    glUniform3fARB(m_shadowParm[29], front.x, front.y, front.z);
    glUniform3fARB(m_shadowParm[30], up.x, up.y, up.z);
    glUniform3fARB(m_shadowParm[31], right.x, right.y, right.z);
  }

  glUniform1fARB(m_shadowParm[32], m_interpVol);
  glUniform1iARB(m_shadowParm[33], m_mixTag);

  glUniform1iARB(m_shadowParm[3], 4); // maskTex
  glUniform1iARB(m_shadowParm[4], 5); // paintTex
}

void
DrawHiresVolume::captureToShadowBuffer()
{
  if (Global::useFBO())
    {
      m_shadowBuffer->bind(); // write into shadowBuffer
      glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
    }
}

void
DrawHiresVolume::releaseShadowBuffer()
{
  if (Global::useFBO())
    m_shadowBuffer->release();


  //--------- draw in fbo -----------
  if (m_Viewer->drawToFBO() || Global::imageQuality() != Global::_NormalQuality)
    {
      if (m_Viewer->imageBuffer()->bind())
	glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
    }
  //---------------------------------
}

void
DrawHiresVolume::drawSlicesHighQuality(Vec pn, Vec minvert, Vec maxvert,
				       int layers, float stepsize)
{
  for(int bno=0; bno<m_polygon.size(); bno++)
    delete m_polygon[bno];
  m_polygon.clear();


  enableTextureUnits();

  int ScreenXMin, ScreenXMax, ScreenYMin, ScreenYMax;
  ScreenXMin = ScreenYMin = 100000;
  ScreenXMax = ScreenYMax = 0;

  Camera defaultCam = *(m_Viewer->camera());
  Vec defaultCamPos = m_Viewer->camera()->position();
  Vec defaultViewDirection = m_Viewer->camera()->viewDirection();
  int camWidth = m_Viewer->camera()->screenWidth();
  int camHeight = m_Viewer->camera()->screenHeight();
  float defaultFov = m_Viewer->camera()->fieldOfView();

  Vec step = stepsize*pn;
  Vec poStart;
  if (!m_backlit)
    poStart = maxvert;
  else
    poStart = maxvert+layers*step;


  Vec voxelScaling = Global::voxelScaling();
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


  if (m_lightInfo.applyShadows)
    setViewFromLight();

  GeometryObjects::trisets()->predraw(m_Viewer,
				      m_bricks->getMatrix(0),
				      pn,
				      m_lightInfo.applyShadows,
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

 
  //----------------------------------------------------------------

  QList<int> tfSet;
  QList<float> tfSetF;

  tfSet = getSlices(poStart, step, pn, layers);
  for (int s=0; s<tfSet.count(); s++)
    tfSetF.append((float)tfSet[s]/(float)Global::lutSize());

  //------
  int slabstart, slabend;
  if (Global::useDragVolume() ||
      m_dataTexSize == 1)
    {
      slabstart = 0;
      slabend = 1;
    }
  else
    {
      slabstart = 1;
      slabend = m_dataTexSize;
    }

  int shadowslabstart, shadowslabend;
  if (Global::volumeType() != Global::DummyVolume)
    {
      if (Global::useDragVolume() ||
	  Global::useDragVolumeforShadows() ||
	  m_dataTexSize == 1)
	{
	  shadowslabstart = 0;
	  shadowslabend = 1;
	}
      else
	{
	  shadowslabstart = 1;
	  shadowslabend = m_dataTexSize;
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

  // for drawing clipplanes
  int clipOffset = m_numBricks*layers;
  int nclipPlanes = (GeometryObjects::clipplanes()->positions()).count();

  //-------------------------------
  {
    *(m_Viewer->camera()) = defaultCam;
    setRenderToScreen(defaultCamPos,
		      defaultViewDirection,
		      camWidth, camHeight,
		      defaultFov);
    drawClipPlaneInViewport(m_numBricks*layers,
			    lpos[0],
			    1.0,
			    lenx2, leny2, lod,
			    dragTexsize,
			    false); //highqualityShader
    drawPathInViewport(m_numBricks*layers,
		       lpos[0],
		       1.0,
		       lenx2, leny2, lod,
		       dragTexsize,
		       false); //highqualityShader
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
	bool useAllTex = (!Global::useDragVolume() || m_dataTexSize == 1);
	setShader2DTextureParameter(useAllTex, false);
      }
    if (GeometryObjects::paths()->viewportsVisible())
      {
	// some shader parameters might have changed so reset them
	bool useAllTex = (!Global::useDragVolume() || m_dataTexSize == 1);
	setShader2DTextureParameter(useAllTex, false);
      }
  }
  //-------------------------------

  Vec pnDir = step;
  if (m_backlit)
    pnDir = -step;
  int pno = 0;
  Vec po = poStart;
  for(int s=0; s<layers; s++)
    {
      po += pnDir;

      float depthcue = 1;
      if (Global::depthcue())
	{
	  float sdist = qAbs((maxvert - po)*pn);
	  depthcue = 1.0 - qBound(0.0f, sdist/deplen, 0.95f);
	}

      pno = s * m_numBricks;
      //-------------------------------------------------------
      // -- generate shadow texture coordinates
      if (m_lightInfo.applyShadows)
	{
	  setViewFromLight();
	  for (int bn=0; bn<m_numBricks; bn++)
	    {
	      for(int pi=0; pi<m_polygon[pno]->edges; pi++)
		{
		  Vec v = m_polygon[pno]->vertex[pi];
		  Vec scr = m_Viewer->camera()->projectedCoordinatesOf(v);
		  m_polygon[pno]->tx[pi] = scr.x;
		  m_polygon[pno]->ty[pi] = m_shadowHeight-scr.y;
		}
	      pno ++;
	    }

	  // for drawing clip planes
	  for (int ic=0; ic<nclipPlanes; ic++)
	    {
	      int cic = clipOffset+ic;
	      for(int pi=0; pi<m_polygon[pno]->edges; pi++)
		{
		  Vec v = m_polygon[cic]->vertex[pi];
		  Vec scr = m_Viewer->camera()->projectedCoordinatesOf(v);
		  m_polygon[cic]->tx[pi] = scr.x;
		  m_polygon[cic]->ty[pi] = m_shadowHeight-scr.y;
		}
	    }

	}
      
      // -- now render polygon to screen using shadow texture coordinates
      if (m_lightInfo.applyShadows)
	{
	  // set view from eye
	  *(m_Viewer->camera()) = defaultCam;
	  setRenderToScreen(defaultCamPos,
			    defaultViewDirection,
			    camWidth, camHeight,
			    defaultFov);
	}
      
      if (s%Global::geoRenderSteps() == 0 || s == layers-1)
	renderGeometry(s, layers,
		       po, pn, Global::geoRenderSteps()*step,
		       m_lightInfo.applyShadows, false, eyepos,
		       false);

      enableTextureUnits();
      glUseProgramObjectARB(m_highqualityShader);
	  
      glEnable(GL_DEPTH_TEST);
      glColor3f(1.0, 0.0, 0.0);
      //------------------------------------------------------

      if (Global::volumeType() != Global::DummyVolume)
	{
	  pno = s * m_numBricks;
 	  for (int bn=0; bn<m_numBricks; bn++)
	    {
	      Vec lp = lpos[bn];
	      glUniform3fARB(m_highqualityParm[6], lp.x, lp.y, lp.z);

	      QList<bool> clips;
	      int tfset;
	      Vec subvol[8],texture[8], subcorner, subdim;
	      Vec brickPivot, brickScale;
	      
	      m_drawBrickInformation.get(bn, 
					 tfset,
					 subvol, texture,
					 subcorner, subdim,
					 clips,
					 brickPivot, brickScale);
	      
	      ViewAlignedPolygon *vap = m_polygon[pno];
	      pno++;
	      
	      if (vap->edges > 0 && tfSetF[bn] < 1.0)
		{
		  if (Global::volumeType() != Global::RGBVolume &&
		      Global::volumeType() != Global::RGBAVolume)
		    glUniform1fARB(m_highqualityParm[3], tfSetF[bn]);
		  else
		    {
		      float frc = Global::stepsizeStill();
		      glUniform1fARB(m_highqualityParm[3], frc);
		    }
		  
		  glUniform1fARB(m_highqualityParm[18], depthcue);
		  
		  for(int b=slabstart; b<slabend; b++)
		    {
		      float tminz = m_dataMin.z;
		      float tmaxz = m_dataMax.z;
		      if (slabend > 1)
			{
			  tminz = m_textureSlab[b].y;
			  tmaxz = m_textureSlab[b].z;
			}
		      glUniform3fARB(m_highqualityParm[42], m_dataMin.x, m_dataMin.y, tminz);
		      glUniform3fARB(m_highqualityParm[43], m_dataMax.x, m_dataMax.y, tmaxz);

		      bindDataTextures(b);
		      
		      if (!Global::useDragVolume() || m_dataTexSize == 1)
			renderSlicedSlice(1,
					  vap, true,
					  lenx2, leny2, b, lod);
		      else
			renderDragSlice(vap, true, dragTexsize);
		    } // loop over b
		}
	    } // loop over bricks

	  //--- for drawing clip planes if any
	  drawClipPlaneHighQuality(s, layers,
				   po, pn, step,
				   clipOffset,
				   lpos[0],  depthcue,
				   slabstart, slabend,
				   lenx2, leny2, lod,
				   dragTexsize);
			       
	} // not DummyVolume

      if (m_lightInfo.applyShadows)
	{
	  //-------------------------------------------------------
	  // -- render shadow polygon -- set view from light
	  captureToShadowBuffer();
	  setRenderToShadowBuffer();
	  
	  //-------- draw geometry for shadowing -----------------
	  setViewFromLight();
	  
	  if (s%Global::geoRenderSteps() == 0 || s == layers-1)
	    renderGeometry(s, layers,
			   po, pn, Global::geoRenderSteps()*step,
			   m_lightInfo.applyShadows, true, eyepos,
			   true);

	  enableTextureUnits();
	  glUseProgramObjectARB(m_shadowShader);
	  glUniform1iARB(m_shadowParm[0], 0); // lutTex
	  glUniform1iARB(m_shadowParm[1], 1); // dataTex	  

	  if (Global::volumeType() != Global::DummyVolume)
	    {
	      pno = s * m_numBricks;
	      for (int bn=0; bn<m_numBricks; bn++)
		{
		  QList<bool> clips;
		  int tfset;
		  Vec subvol[8],texture[8], subcorner, subdim;
		  Vec brickPivot, brickScale;
		  
		  m_drawBrickInformation.get(bn, 
					     tfset,
					     subvol, texture,
					     subcorner, subdim,
					     clips,
					     brickPivot, brickScale);
		  
		  ViewAlignedPolygon *vap = m_polygon[pno];
		  pno++;
		  
		  if (vap->edges > 0 && tfSetF[bn] < 1.0)
		    {
		      if (Global::volumeType() != Global::RGBVolume &&
			  Global::volumeType() != Global::RGBAVolume)
			glUniform1fARB(m_shadowParm[2], tfSetF[bn]);
		      else
			{
			  float frc = Global::stepsizeStill();
			  glUniform1fARB(m_shadowParm[2], frc);
			}
		      
		      for(int b=shadowslabstart; b<shadowslabend; b++)
			{
			  float tminz = m_dataMin.z;
			  float tmaxz = m_dataMax.z;
			  if (slabend > 1)
			    {
			      tminz = m_textureSlab[b].y;
			      tmaxz = m_textureSlab[b].z;
			    }
			  glUniform3fARB(m_shadowParm[42], m_dataMin.x, m_dataMin.y, tminz);
			  glUniform3fARB(m_shadowParm[43], m_dataMax.x, m_dataMax.y, tmaxz);

			  bindDataTextures(b);
		      
			  if ((Global::useDragVolume() ||
			       Global::useDragVolumeforShadows()) &&
			      m_dataTexSize > 1)
			    renderDragSlice(vap, false, dragTexsize);
			  else			      
			    renderSlicedSlice(2,
					      vap, false,
					      lenx2, leny2, b, lod);
			} // loop over b
		    }
		} // loop over bricks

	      //--- for drawing clip planes if any
	      drawClipPlaneShadow(s, layers,
				  po, pn, step,
				  clipOffset,
				  shadowslabstart, shadowslabend,
				  lenx2, leny2, lod,
				  dragTexsize);
			       
	    } // not DummyVolume
	  releaseShadowBuffer();
	  //--------------------------------------------------------
	  // -- now apply blur operation
	  pno = s * m_numBricks;
	  for (int bn=0; bn<m_numBricks; bn++)
	    {
	      for(int pi=0; pi<m_polygon[pno]->edges; pi++)
		{  
		  Vec scr = m_Viewer->camera()->projectedCoordinatesOf(m_polygon[pno]->vertex[pi]);
		  scr.y = m_shadowHeight - scr.y;
		  ScreenXMin = qMin(ScreenXMin, (int)scr.x);
		  ScreenXMax = qMax(ScreenXMax, (int)scr.x);
		  ScreenYMin = qMin(ScreenYMin, (int)scr.y);
		  ScreenYMax = qMax(ScreenYMax, (int)scr.y);
		}
	      pno++;
	    }
	  blurShadows(ScreenXMin, ScreenXMax, ScreenYMin, ScreenYMax);
	  //-------------------------------------------------------
	} // blur shadows

     } // loop over layers

  //----------------------------------------------------------------
  *(m_Viewer->camera()) = defaultCam;
  setRenderToScreen(defaultCamPos,
		    defaultViewDirection,
		    camWidth, camHeight,
		    defaultFov);
  glUseProgramObjectARB(0);
  disableTextureUnits();
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
  //----------------------------------------------------------------
 
  //----------------------------------------------------------------
  //--------- draw in fbo -----------
  if (m_Viewer->drawToFBO() || Global::imageQuality() != Global::_NormalQuality)
    {
      if (m_Viewer->imageBuffer()->bind())
	glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
    }
  //---------------------------------


  if (m_lightInfo.applyShadows && m_lightInfo.applyBackplane)
    drawBackplane(&defaultCam, defaultCamPos, defaultViewDirection,
		  camWidth, camHeight, defaultFov);

  //-------------------------------------------------------
  // -- reset default camera settings
  *(m_Viewer->camera()) = defaultCam;
  m_Viewer->camera()->setPosition(defaultCamPos);      
  m_Viewer->camera()->setViewDirection(defaultViewDirection);
  m_Viewer->camera()->setScreenWidthAndHeight(camWidth, camHeight);
  m_Viewer->camera()->setFieldOfView(defaultFov);
  m_Viewer->camera()->setFocusDistance(m_focusDistance);
  loadCameraMatrices();
  glViewport(0,0, camWidth, camHeight);
  //----------------------------------------------------------------
}

void
DrawHiresVolume::blurShadows(int ScreenXMin, int ScreenXMax,
			     int ScreenYMin, int ScreenYMax)
{
  int xmin, xmax, ymin, ymax;
  xmin = ymin = 0;
  xmax = m_shadowWidth;
  ymax = m_shadowHeight;

  int width = m_shadowWidth;
  int height = m_shadowHeight;

  //--------------------------------------------
  captureToShadowBuffer();
  
  glDisable(GL_DEPTH_TEST);
  
  // attenuate the light for color translucency
  glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ONE); 

  StaticFunctions::pushOrthoView(0, 0, width, height);

  glUseProgramObjectARB(Global::copyShader());
  glUniform1iARB(Global::copyParm(0), 2); // copy from blurredBuffer into shadowBuffer
  StaticFunctions::drawQuad(xmin, ymin, xmax, ymax, 1.0);
  glUseProgramObjectARB(0); // disable shaders 
  StaticFunctions::popOrthoView();

  releaseShadowBuffer();

  glEnable(GL_DEPTH_TEST);
  //------------------------------------------------




  //------------------------------------------------
  if (Global::useFBO())
    {
      m_blurredBuffer->bind(); // blur shadows into blurredBuffer
      glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
    }
  
  glDisable(GL_DEPTH_TEST);
	      
  // replace texture
  glBlendFunc(GL_ONE, GL_ZERO);
	      

  xmin = ScreenXMin;
  xmax = ScreenXMax;
  ymin = ScreenYMin;
  ymax = ScreenYMax;
  xmin = qMax(0, xmin-5);
  ymin = qMax(0, ymin-5);
  xmax = qMin(width, xmax+5);
  ymax = qMin(height, ymax+5);

  StaticFunctions::pushOrthoView(0, 0, width, height);

  glUseProgramObjectARB(m_blurShader);
  glUniform1iARB(m_blurParm[0], 3); // blur shadows from shadowBuffer into blurredBuffer
  StaticFunctions::drawQuad(xmin, ymin, xmax, ymax, 1.0);
  glUseProgramObjectARB(0);
  StaticFunctions::popOrthoView();

  if (Global::useFBO())
    m_blurredBuffer->release();
	      
  glEnable(GL_DEPTH_TEST);
  //------------------------------------------------
}


void
DrawHiresVolume::drawBackplane(Camera *defaultCam,
			       Vec defaultCamPos,
			       Vec defaultViewDirection,
			       int camWidth, int camHeight,
			       float defaultFov)
{
  if (m_backlit)
    return;

  glDepthMask(GL_FALSE); // disable writing to depth buffer


  *(m_Viewer->camera()) = *defaultCam;
  m_Viewer->camera()->setPosition(defaultCamPos);
  m_Viewer->camera()->setViewDirection(defaultViewDirection);
  m_Viewer->camera()->setScreenWidthAndHeight(camWidth, camHeight);
  m_Viewer->camera()->setFieldOfView(defaultFov);
  m_Viewer->camera()->setFocusDistance(m_focusDistance);

  loadCameraMatrices();

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



  ViewAlignedPolygon vap;

  //-------------------------------------------------------
  // -- generate backplane vertices


//------------ plot plane behind the model --------
  Vec front = m_Viewer->camera()->viewDirection();
  Vec up = m_Viewer->camera()->upVector();
  Vec right = m_Viewer->camera()->rightVector();
//------------------------------------------------

  Vec cen = m_Viewer->camera()->sceneCenter() +
            2*m_Viewer->camera()->sceneRadius()*front;

  float uproj = up * (cen - m_Viewer->camera()->position());
  float rproj = right * (cen - m_Viewer->camera()->position());
  
  cen = cen - uproj*up - rproj*right;

  Vec pv0 = m_Viewer->camera()->projectedCoordinatesOf(cen);
  Vec pv1 = cen + up;
  pv1 = m_Viewer->camera()->projectedCoordinatesOf(pv1);
  Vec pv2 = cen + right;
  pv2 = m_Viewer->camera()->projectedCoordinatesOf(pv2);

  float cH = 0.5*camHeight;
  float cW = 0.5*camWidth;
  float fup = cH/fabs(pv1.y-pv0.y);
  float fright = cW/fabs(pv2.x-pv0.x);

  vap.edges = 4;
  vap.vertex[0] = cen + up*fup - right*fright;
  vap.vertex[1] = cen - up*fup - right*fright;
  vap.vertex[2] = cen - up*fup + right*fright;
  vap.vertex[3] = cen + up*fup + right*fright;
  //----------------------------------

  //----------------------------------
  // -- generate shadow texture coordinates
  setViewFromLight();
  cen = m_Viewer->camera()->projectedCoordinatesOf(cen);
  float scale = (m_lightInfo.backplaneShadowScale-1)*0.2;
  for(int pi=0; pi<4; pi++)
    {
      Vec v = vap.vertex[pi];
      Vec scr = m_Viewer->camera()->projectedCoordinatesOf(v);
      v = scr-cen;
      vap.tx[pi] = scr.x - scale*v.x;
      vap.ty[pi] = m_shadowHeight-(scr.y - scale*v.y);
    }
  //----------------------------------
  
  //-------------------------------------------------------
  // -- now render polygon to screen using shadow texture coordinates
  *(m_Viewer->camera()) = *defaultCam;
  float zClippingCoeff = m_Viewer->camera()->zClippingCoefficient();  
  m_Viewer->camera()->setZClippingCoefficient(2*zClippingCoeff);  
  m_Viewer->camera()->setPosition(defaultCamPos);
  m_Viewer->camera()->setViewDirection(defaultViewDirection);
  m_Viewer->camera()->setScreenWidthAndHeight(camWidth, camHeight);
  m_Viewer->camera()->setFieldOfView(defaultFov);
  m_Viewer->camera()->setFocusDistance(m_focusDistance);

  loadCameraMatrices();

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


  
  if (!m_backlit)
    glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE); // for frontlit volume
  else
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // back to front


  //-------------------------------------------------------
  QImage bgImage = Global::backgroundImage();
  if (bgImage.isNull())
    {
      glColor3dv(Global::backgroundColor());
      glUseProgramObjectARB(m_backplaneShader1);
      glUniform1iARB(m_backplaneParm1[0], 2); // copy from blurredBuffer into frameBuffer
      glBegin(GL_QUADS);
      for(int pi=0; pi<vap.edges; pi++)
	{  
	  glMultiTexCoord2f(GL_TEXTURE0, vap.tx[pi],vap.ty[pi]);
	  glVertex3f(vap.vertex[pi].x, vap.vertex[pi].y, vap.vertex[pi].z);
	}
      glEnd();
    }
  else
    {
      int ht = bgImage.height();
      int wd = bgImage.width();

      int nbytes = bgImage.numBytes();
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

      glColor4f(1, 1, 1, 1);
      glUseProgramObjectARB(m_backplaneShader2);
      glUniform1iARB(m_backplaneParm2[0], 2); // copy from blurredBuffer into frameBuffer
      glUniform1iARB(m_backplaneParm2[1], 0); // background image

      glBegin(GL_QUADS);
      if (m_backlit)
	glMultiTexCoord2f(GL_TEXTURE1, 0, ht);   
      else
	glMultiTexCoord2f(GL_TEXTURE1, 0, 0); 
      glMultiTexCoord2f(GL_TEXTURE0, vap.tx[0],vap.ty[0]);
      glVertex3f(vap.vertex[0].x, vap.vertex[0].y, vap.vertex[0].z);
      
      if (m_backlit)
	glMultiTexCoord2f(GL_TEXTURE1, wd, ht);  
      else
	glMultiTexCoord2f(GL_TEXTURE1, 0, ht);  
      glMultiTexCoord2f(GL_TEXTURE0, vap.tx[1],vap.ty[1]);
      glVertex3f(vap.vertex[1].x, vap.vertex[1].y, vap.vertex[1].z);
      
      if (m_backlit)
	glMultiTexCoord2f(GL_TEXTURE1, wd, 0); 
      else
	glMultiTexCoord2f(GL_TEXTURE1, wd, ht);   
      glMultiTexCoord2f(GL_TEXTURE0, vap.tx[2],vap.ty[2]);
      glVertex3f(vap.vertex[2].x, vap.vertex[2].y, vap.vertex[2].z);
      
      if (m_backlit)
	glMultiTexCoord2f(GL_TEXTURE1, 0, 0);  
      else
	glMultiTexCoord2f(GL_TEXTURE1, wd, 0);  
      glMultiTexCoord2f(GL_TEXTURE0, vap.tx[3],vap.ty[3]);
      glVertex3f(vap.vertex[3].x, vap.vertex[3].y, vap.vertex[3].z);
      
      glEnd();

      glDisable(GL_TEXTURE_RECTANGLE_ARB);
    }
  //-------------------------------------------------------

  glUseProgramObjectARB(0);

  m_Viewer->camera()->setZClippingCoefficient(zClippingCoeff);  
  loadCameraMatrices();
  //-------------------------------------------------------

  glDepthMask(GL_TRUE); // enable writing to depth buffer
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

      genDefaultHighShadow();
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
      genDefaultHighShadow();
    }

  vpo.clear();
}

void
DrawHiresVolume::genDefaultHighShadow()
{
  createDefaultShader();
  createHighQualityShader();
  if (m_lightInfo.applyColoredShadows)
    createShadowShader(m_lightInfo.colorAttenuation);
  else
    createShadowShader(Vec(1,1,1));
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
      m_lightInfo.peelType != lightInfo.peelType)
    {
      doDS = true;
      doHQS = true;
      doSS = true;
    }


  m_lightInfo = lightInfo;


  if (doISB) initShadowBuffers();
  if (doDS) createDefaultShader();
  if (doHQS) createHighQualityShader();
  if (doBS) createBlurShader(true, 1, m_lightInfo.shadowBlur);
  if (doBPS) createBackplaneShader(m_lightInfo.backplaneIntensity);
  if (doSS)
    {
      if (m_lightInfo.applyColoredShadows)
	createShadowShader(m_lightInfo.colorAttenuation);
      else
	createShadowShader(Vec(1,1,1));
    }
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
  if (doHQS) createHighQualityShader();
}

void DrawHiresVolume::applyBackplane(bool flag) { m_lightInfo.applyBackplane = flag; }
void DrawHiresVolume::updateBackplaneShadowScale(float val) { m_lightInfo.backplaneShadowScale = val; }

void
DrawHiresVolume::applyLighting(bool flag)
{
  m_lightInfo.applyLighting = flag;
  createDefaultShader();
  createHighQualityShader();
}

void
DrawHiresVolume::applyEmissive(bool flag)
{
  m_lightInfo.applyEmissive = flag;
  createDefaultShader();
  createHighQualityShader();
}

void
DrawHiresVolume::applyShadows(bool flag)
{
  m_lightInfo.applyShadows = flag;
  createHighQualityShader();
}

void
DrawHiresVolume::applyColoredShadows(bool flag)
{
  m_lightInfo.applyColoredShadows = flag;
  createHighQualityShader();
  if (m_lightInfo.applyColoredShadows)
    createShadowShader(m_lightInfo.colorAttenuation);
  else
    createShadowShader(Vec(1,1,1));
}

void DrawHiresVolume::updateShadowIntensity(float val)
{
  m_lightInfo.shadowIntensity = val;

  createHighQualityShader();

  if (m_lightInfo.applyColoredShadows)
    createShadowShader(m_lightInfo.colorAttenuation);
  else
    createShadowShader(Vec(1,1,1));
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
  createBlurShader(true, 1, m_lightInfo.shadowBlur);
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

  if (m_lightInfo.applyColoredShadows)
    createShadowShader(m_lightInfo.colorAttenuation);
  else
    createShadowShader(Vec(1,1,1));
}

void
DrawHiresVolume::peel(bool flag)
{
  m_lightInfo.peel = flag;
  genDefaultHighShadow();
}

void
DrawHiresVolume::peelInfo(int ptype, float pmin, float pmax, float pmix)
{
  m_lightInfo.peelType = ptype;
  m_lightInfo.peelMin = pmin;
  m_lightInfo.peelMax = pmax;
  m_lightInfo.peelMix = pmix;
  genDefaultHighShadow();
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
      genDefaultHighShadow();
    }

  if (iv <= 1.0f)
    m_interpVol = iv;
}

void
DrawHiresVolume::setInterpolateVolumes(int iv)
{
  m_interpolateVolumes = iv;
  genDefaultHighShadow();
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
      // toggle mouse grabs
      GeometryObjects::inPool = ! GeometryObjects::inPool;
      if (GeometryObjects::inPool)
	GeometryObjects::addInMouseGrabberPool();
      else
	GeometryObjects::removeFromMouseGrabberPool();
    }

  // change empty space skip
  if (event->key() == Qt::Key_E)
    {
      Global::setEmptySpaceSkip(!Global::emptySpaceSkip());
      MainWindowUI::mainWindowUI()->actionEmptySpaceSkip->setChecked(Global::emptySpaceSkip());

      genDefaultHighShadow();

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
  if (event->key() == Qt::Key_H)
    {
      Global::setUseStillVolume(!Global::useStillVolume());
      MainWindowUI::mainWindowUI()->actionUse_stillvolume->setChecked(Global::useStillVolume());
      return true;
    }
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

      int nbytes = bgImage.numBytes();
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
			       int step1, int step2)
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

  int nslices = (dmax-dmin)/step2;
  int wd = (wmax-wmin)/step1;
  int ht = (hmax-hmin)/step1;
  Vec sliceZero = pos + dmin*normal + wmin*xaxis + hmin*yaxis;

  //----------------
  QFileDialog fdialog(0,
		      "Save Resliced Volume",
		      Global::previousDirectory(),
		      "Processed (*.pvl.nc)");

  fdialog.setAcceptMode(QFileDialog::AcceptSave);

  if (!fdialog.exec() == QFileDialog::Accepted)
    return;

  QString pFile = fdialog.selectedFiles().value(0);
  if (!pFile.endsWith(".pvl.nc"))
    pFile += ".pvl.nc";


  VolumeFileManager pFileManager;
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
  pFileManager.createFile(true);


  VolumeInformation pvlInfo = VolumeInformation::volumeInformation();
  int vtype = VolumeInformation::_UChar;
  float vx = pvlInfo.voxelSize.x;
  float vy = pvlInfo.voxelSize.y;
  float vz = pvlInfo.voxelSize.z;
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
  //----------------

  uchar *slice = new uchar[wd*ht];


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

  glUniform1fARB(parm[18], 1.0); // depthcue

  glUniform3fARB(parm[4], 0,0,0); // delta

  QProgressDialog progress("Reslicing volume",
			   QString(),
			   0, 100,
			   0);
  progress.setCancelButton(0);

  glDisable(GL_DEPTH_TEST);

  glClearDepth(0);
  glClearColor(0,0,0,0);
  
  for(int sl=0; sl<nslices; sl++)
    {
      Vec po = (sliceZero + sl*normal*step2);
      progress.setValue(100*(float)sl/(float)nslices);

      glClear(GL_DEPTH_BUFFER_BIT);
      glClear(GL_COLOR_BUFFER_BIT);

      for(int b=slabstart; b<slabend; b++)
	{
	  float tminz = m_dataMin.z;
	  float tmaxz = m_dataMax.z;
	  if (slabend > 1)
	    {
	      tminz = m_textureSlab[b].y;
	      tmaxz = m_textureSlab[b].z;
	      
	      glUniform1iARB(parm[24], (tminz-m_dataMin.z)/lod); // zoffset
	      glUniform1iARB(parm[27], tminz);
	    }		  
	  
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
	  
	  glBegin(GL_QUADS);

	  Vec v = po;
	  v = VECDIVIDE(v, voxelScaling);
	  glMultiTexCoord3dv(GL_TEXTURE0, v);
	  glVertex3f(0, 0, 0);

	  v = po + wd*xaxis*step1;
	  v = VECDIVIDE(v, voxelScaling);
	  glMultiTexCoord3dv(GL_TEXTURE0, v);
	  glVertex3f(wd, 0, 0);

	  v = po + wd*xaxis*step1 + ht*yaxis*step1;
	  v = VECDIVIDE(v, voxelScaling);
	  glMultiTexCoord3dv(GL_TEXTURE0, v);
	  glVertex3f(wd, ht, 0);

	  v = po + ht*yaxis*step1;
	  v = VECDIVIDE(v, voxelScaling);
	  glMultiTexCoord3dv(GL_TEXTURE0, v);
	  glVertex3f(0, ht, 0);

	  glEnd();
	} // slabs

      glReadPixels(0, 0, wd, ht, GL_RED, GL_UNSIGNED_BYTE, slice);
      pFileManager.setSlice(nslices-1-sl, slice);
    }

  m_shadowBuffer->release();

  progress.setValue(100);

  delete [] slice;

  glUseProgramObjectARB(0);
  disableTextureUnits();  

  StaticFunctions::popOrthoView();

  // restore shadow buffer
  initShadowBuffers(true);

  glEnable(GL_DEPTH_TEST);

  QMessageBox::information(0, "Saved Resliced Volume",
			   QString("Resliced volume saved to %1 and %1.001").arg(pFile));
}

void
DrawHiresVolume::resliceUsingPath(int pathIdx, bool fullThickness)
{
  Vec voxelScaling = VolumeInformation::volumeInformation().voxelSize;

  PathObject po;
  po = GeometryObjects::paths()->paths()[pathIdx];

  QVector4D vp = po.viewport();
  float pathLength = po.length();
  QList<Vec> pathPoints = po.pathPoints();
  QList<Vec> pathX = po.pathX();
  QList<Vec> pathY = po.pathY();
  QList<float> radX = po.pathradX();
  QList<float> radY = po.pathradY();

  for(int np=0; np<pathPoints.count(); np++)
    pathPoints[np] = VECPRODUCT(voxelScaling,pathPoints[np]);
  for(int np=0; np<pathPoints.count(); np++)
    pathX[np] = VECPRODUCT(voxelScaling,pathX[np]);
  for(int np=0; np<pathPoints.count(); np++)
    pathY[np] = VECPRODUCT(voxelScaling,pathY[np]);
  

  int maxthick = radY[0];
  for(int np=0; np<pathPoints.count(); np++)
    maxthick = max(maxthick, (int)radY[np]);

  float maxheight = 0;
  for(int np=0; np<pathPoints.count(); np++)
    {
      float ht = (pathX[np]*radX[np]).norm();
      maxheight = max(maxheight, ht);
    }

  int nslices = maxthick;
  if (fullThickness) nslices = 2*maxthick;
  int wd = pathLength;
  int ht = 2*maxheight;

  //----------------
  QFileDialog fdialog(0,
		      "Save Resliced Volume",
		      Global::previousDirectory(),
		      "Processed (*.pvl.nc)");

  fdialog.setAcceptMode(QFileDialog::AcceptSave);

  if (!fdialog.exec() == QFileDialog::Accepted)
    return;

  QString pFile = fdialog.selectedFiles().value(0);
  if (!pFile.endsWith(".pvl.nc"))
    pFile += ".pvl.nc";


  VolumeFileManager pFileManager;
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
  pFileManager.createFile(true);


  VolumeInformation pvlInfo = VolumeInformation::volumeInformation();
  int vtype = VolumeInformation::_UChar;
  float vx = pvlInfo.voxelSize.x;
  float vy = pvlInfo.voxelSize.y;
  float vz = pvlInfo.voxelSize.z;
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
  //----------------

  uchar *slice = new uchar[wd*ht];

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
  
  StaticFunctions::pushOrthoView(0, 0, wd, ht);
  glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE); // for frontlit volume

  GLint *parm = m_defaultParm;

  if (Global::volumeType() != Global::RGBVolume &&
      Global::volumeType() != Global::RGBAVolume)
    glUniform1fARB(parm[3], 0.0); // tfset
  else
    {
      float frc = Global::stepsizeStill();
      glUniform1fARB(parm[3], frc);
    }	      

  glUniform1fARB(parm[18], 1.0); // depthcue

  glUniform3fARB(parm[4], 0,0,0); // delta

  QProgressDialog progress("Reslicing volume",
			   QString(),
			   0, 100,
			   0);
  progress.setCancelButton(0);

  glDisable(GL_DEPTH_TEST);

  glClearDepth(0);
  glClearColor(0,0,0,0);
  
  glTranslatef(0.0, maxheight, 0.0);

  for(int sl=0; sl<nslices; sl++)
    {
      float tk = (float)sl/(float)(nslices-1);
      progress.setValue(100*(float)sl/(float)nslices);
      
      glClear(GL_DEPTH_BUFFER_BIT);
      glClear(GL_COLOR_BUFFER_BIT);

      for(int b=slabstart; b<slabend; b++)
	{
	  float tminz = m_dataMin.z;
	  float tmaxz = m_dataMax.z;
	  if (slabend > 1)
	    {
	      tminz = m_textureSlab[b].y;
	      tmaxz = m_textureSlab[b].z;
	      
	      glUniform1iARB(parm[24], (tminz-m_dataMin.z)/lod); // zoffset
	      glUniform1iARB(parm[27], tminz);
	    }		  
	  
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
		clen += (pathPoints[np]-pathPoints[np-1]).norm();
	      
	      float lenradx = pathX[np].norm()*radX[np];
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
	      
	      Vec v1 = Vec(clen,-lenradx,0.0);
	      Vec v2 = Vec(clen,lenradx,0.0);

	      glMultiTexCoord3dv(GL_TEXTURE0, tv1);
	      glVertex3f((float)v1.x, (float)v1.y, 0.0);
	      
	      glMultiTexCoord3dv(GL_TEXTURE0, tv2);
	      glVertex3f((float)v2.x, (float)v2.y, 0.0);
	    }
	  glEnd();
	  
	} // slabs

      glReadPixels(0, 0, wd, ht, GL_RED, GL_UNSIGNED_BYTE, slice);
      pFileManager.setSlice(nslices-1-sl, slice);
    } // depth slices

  m_shadowBuffer->release();

  progress.setValue(100);

  delete [] slice;

  glUseProgramObjectARB(0);
  disableTextureUnits();  

  StaticFunctions::popOrthoView();

  // restore shadow buffer
  initShadowBuffers(true);

  glEnable(GL_DEPTH_TEST);

  QMessageBox::information(0, "Saved Resliced Volume",
			   QString("Resliced volume saved to %1 and %1.001").arg(pFile));
}

