#include <GL/glew.h>

#include "shaderfactory.h"
#include "viewer.h"
#include "global.h"
#include "staticfunctions.h"
#include "propertyeditor.h"
#include "volumeoperations.h"
#include "slicer3d.h"

#include <QDockWidget>
#include <QInputDialog>
#include <QProgressDialog>
#include <QLabel>
#include <QImage>
#include <QPainter>
#include <QFileDialog>

#include "ui_viewermenu.h"

Viewer::Viewer(QWidget *parent) :
  QGLViewer(parent)
{
  setStateFileName(QString());
  setMouseTracking(true);

  m_draw = true;
  m_useMask = true;

  m_memSize = 1000; // size in MB

  m_clipPlanes = new ClipPlanes();

  m_renderMode = true; // 0-slicebased, 1-raycast

  m_gradType = 0;
  
  m_depth = 0;
  m_width = 0;
  m_height = 0;

  m_eBuffer = 0;
  m_ebId = 0;
  m_ebTex[0] = 0;
  m_ebTex[1] = 0;
  m_ebTex[2] = 0;

  m_slcBuffer = 0;
  m_rboId = 0;
  m_slcTex[0] = 0;
  m_slcTex[1] = 0;

  m_depthShader = 0;
  m_blurShader = 0;
  m_rcShader = 0;
  m_eeShader = 0;

  m_tagTex = 0;
  m_maskTex = 0;
  m_dataTex = 0;
  m_lutTex = 0;
  m_corner = Vec(0,0,0);
  m_vsize = Vec(1,1,1);
  m_sslevel = 1;

  m_stillStep = 0.7;
  m_dragStep = 1.5;

  m_minGrad = 0.0;
  m_maxGrad = 1.0;
  
  m_skipLayers = 0;

  m_glewInitdone = false;
  
  m_spW = m_spH = 0;
  m_sketchPad = 0;
  m_sketchPadMode = false;
  m_screenImageBuffer = 0;

  m_amb = 1.0;
  m_diff = 0.0;
  m_spec = 0.0;
  m_shadow = 5;
  m_edge = 3.0;
  m_shdX = 0;
  m_shdY = 0;

  m_shadowColor = Vec(0.0,0.0,0.0);
  m_edgeColor = Vec(0.0,0.0,0.0);
  m_bgColor = Vec(0.0,0.0,0.0);

  m_glVertBuffer = 0;
  m_glIndexBuffer = 0;
  m_glVertArray = 0;

  m_mdEle = 0;
  m_mdCount = 0;
  m_mdIndices = 0;
  m_numBoxes = 0;
  
#ifdef USE_GLMEDIA
  m_movieWriter = 0;
#endif // USE_GLMEDIA

  init();

  setMinimumSize(100, 100);

  QTimer::singleShot(2000, this, SLOT(GlewInit()));

  setTextureMemorySize();

  connect(this, SIGNAL(renderNextFrame()),
	  this, SLOT(nextFrame()));

// we are treating bounding box as 6 clip planes
//  connect(&m_boundingBox, SIGNAL(updated()),
//	  this, SLOT(updateFilledBoxes()));
  connect(&m_boundingBox, SIGNAL(updated()),
	  this, SLOT(boundingBoxChanged()));
}

Viewer::~Viewer()
{
  init();
}

bool Viewer::exactCoord() { return m_exactCoord; }

void Viewer::setDSlice(int d) { m_dslice = d; }
void Viewer::setWSlice(int w) { m_wslice = w; }
void Viewer::setHSlice(int h) { m_hslice = h; }

float Viewer::minGrad() { return m_minGrad; }
float Viewer::maxGrad() { return m_maxGrad; }
void Viewer::setMinGrad(float mG) { m_minGrad = mG; }
void Viewer::setMaxGrad(float mG) { m_maxGrad = mG; }

int Viewer::gradType() { return m_gradType; }
void
Viewer::setGradType(int g)
{
  m_gradType = g;
  createRaycastShader();
  update();  
}

void
Viewer::setExactCoord(bool b)
{
  m_exactCoord = b;
  createRaycastShader();
  update();
}

QList<Vec>
Viewer::clipPos()
{
  QList<Vec> cPos = m_clipPlanes->positions();
  Vec RvoxelScaling = Global::relativeVoxelScaling();
  for(int ci=0; ci<cPos.count(); ci++)
    cPos[ci] = VECDIVIDE(cPos[ci], RvoxelScaling);
  
  return cPos;
}
QList<Vec> Viewer::clipNorm()
{
  QList<Vec> cNorm = m_clipPlanes->normals();
  Vec RvoxelScaling = Global::relativeVoxelScaling();
  for(int ci=0; ci<cNorm.count(); ci++)
    {
      cNorm[ci] = VECPRODUCT(cNorm[ci], RvoxelScaling);
      cNorm[ci].normalize();
    }
  
  return cNorm;
}

void
Viewer::setUseMask(bool b)
{
  m_useMask = b;

  m_UI->sketchPad->setVisible(m_useMask);

  createRaycastShader();
  updateVoxels();

  if (m_sketchPadMode)
    {
      m_UI->sketchPad->setChecked(false);
      showSketchPad(false);
    }
}

float Viewer::stillStep() { return m_stillStep;}
float Viewer::dragStep() { return m_dragStep;}

void
Viewer::setStillAndDragStep(float ss, float ds)
{
  m_stillStep = qMax(0.1f,ss);
  m_dragStep = qMax(0.1f,ds);
  createRaycastShader();
  update();
}

void
Viewer::setTextureMemorySize()
{
  QString homePath = QDir::homePath();
  QFileInfo settingsFile(homePath, ".drishti.xml");

  if (! settingsFile.exists())
    return;

  QString flnm = settingsFile.absoluteFilePath();  


  QDomDocument document;
  QFile f(flnm.toLatin1().data());
  if (f.open(QIODevice::ReadOnly))
    {
      document.setContent(&f);
      f.close();
    }

  QDomElement main = document.documentElement();
  QDomNodeList dlist = main.childNodes();
  for(int i=0; i<dlist.count(); i++)
    {
      if (dlist.at(i).nodeName() == "texturememory")
	{
	  QString str = dlist.at(i).toElement().text();
	  m_memSize = str.toInt();
	  return;
	}
    }
}

void
Viewer::GlewInit()
{
  if (glewInit() != GLEW_OK)
    QMessageBox::information(0, "Glew",
			     "Failed to initialise glew");
  
  if (glewGetExtension("GL_ARB_fragment_shader")      != GL_TRUE ||
      glewGetExtension("GL_ARB_vertex_shader")        != GL_TRUE ||
      glewGetExtension("GL_ARB_shader_objects")       != GL_TRUE ||
      glewGetExtension("GL_ARB_shading_language_100") != GL_TRUE)
    QMessageBox::information(0, "Glew",
				 "Driver does not support OpenGL Shading Language.");
  
  
  if (glewGetExtension("GL_EXT_framebuffer_object") != GL_TRUE)
      QMessageBox::information(0, "Glew", 
			       "Driver does not support Framebuffer Objects (GL_EXT_framebuffer_object)");

  m_glewInitdone = true;

  createShaders();
  createFBO();

  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &m_max3DTexSize);

  // turn off autospin
  camera()->frame()->setSpinningSensitivity(100.0);
}

void
Viewer::setBoxSize(int b)
{ 
  m_boxSize = qPow(2,4+b);
  generateBoxMinMax();  
}

void
Viewer::init()
{
  m_lmax = -1;
  m_lmin = -1;
  
  m_skipVoxels = 0;

  m_infoText = true;
  m_savingImages = 0;

  m_tag1 = m_tag2 = -1;
  m_mergeTagTF = false;

  m_skipLayers = 0;
  m_dragMode = true;
  m_exactCoord = false;

  m_dbox = m_wbox = m_hbox = 0;
  m_boxSize = 8;
  m_boxMinMax.clear();
  m_filledBoxes.clear();

  m_spW = m_spH = 0;
  if (m_sketchPad)
    delete [] m_sketchPad;
  m_sketchPad = 0;
  m_sketchPadMode = false;

  if (m_screenImageBuffer)
    delete [] m_screenImageBuffer;
  m_screenImageBuffer = 0;

  m_paintHit = false;
  m_target = Vec(-1,-1,-1);

  m_Dcg = 0;
  m_Wcg = 0;
  m_Hcg = 0;
  m_Dmcg = 0;
  m_Wmcg = 0;
  m_Hmcg = 0;
  m_Dswcg = 0;
  m_Wswcg = 0;
  m_Hswcg = 0;

  m_fibers = 0;

  m_depth = 0;
  m_width = 0;
  m_height = 0;

  m_currSlice = 0;
  m_currSliceType = 0;

  m_maskPtr = 0;
  m_volPtr = 0;
  m_volPtrUS = 0;

  m_pointSkip = 5;
  m_pointSize = 5;
  m_pointScaling = 5;

  m_voxChoice = 0;
  m_voxels.clear();
  m_clipVoxels.clear();

  m_showSlices = false;
  m_dslice = 0;
  m_wslice = 0;
  m_hslice = 0;

  m_minDSlice = 0;
  m_maxDSlice = 0;
  m_minWSlice = 0;
  m_maxWSlice = 0;
  m_minHSlice = 0;
  m_maxHSlice = 0;  

  m_cminD = m_minDSlice;
  m_cminW = m_minWSlice;
  m_cminH = m_minHSlice;
  m_cmaxD = m_maxDSlice;
  m_cmaxW = m_maxWSlice;
  m_cmaxH = m_maxHSlice;

  m_paintedTags.clear();
  m_paintedTags << -1;

  m_curveTags.clear();
  m_curveTags << -1;

  m_fiberTags.clear();
  m_fiberTags << -1;

  m_showBox = true;

  m_clipPlanes->clear();

  if (m_rcShader)
    glDeleteObjectARB(m_rcShader);
  m_rcShader = 0;

  if (m_eeShader)
    glDeleteObjectARB(m_eeShader);
  m_eeShader = 0;

  if (m_depthShader)
    glDeleteObjectARB(m_depthShader);
  m_depthShader = 0;

  if (m_blurShader)
    glDeleteObjectARB(m_blurShader);
  m_blurShader = 0;

  if (m_eBuffer) glDeleteFramebuffers(1, &m_eBuffer);
  if (m_ebId) glDeleteRenderbuffers(1, &m_ebId);
  if (m_ebTex[0]) glDeleteTextures(3, m_ebTex);
  m_eBuffer = 0;
  m_ebId = 0;
  m_ebTex[0] = m_ebTex[1] = m_ebTex[2] = 0;

  if (m_slcBuffer) glDeleteFramebuffers(1, &m_slcBuffer);
  if (m_rboId) glDeleteRenderbuffers(1, &m_rboId);
  if (m_slcTex[0]) glDeleteTextures(2, m_slcTex);
  m_slcBuffer = 0;
  m_rboId = 0;
  m_slcTex[0] = m_slcTex[1] = 0;

  if (m_dataTex) glDeleteTextures(1, &m_dataTex);
  m_dataTex = 0;

  if (m_maskTex) glDeleteTextures(1, &m_maskTex);
  m_maskTex = 0;

  if (m_tagTex) glDeleteTextures(1, &m_tagTex);
  m_tagTex = 0;

  if (m_lutTex) glDeleteTextures(1, &m_lutTex);
  m_lutTex = 0;

  m_corner = Vec(0,0,0);
  m_vsize = Vec(1,1,1);
  m_sslevel = 1;

  if(m_glVertArray)
    {
      glDeleteBuffers(1, &m_glIndexBuffer);
      glDeleteVertexArrays( 1, &m_glVertArray );
      glDeleteBuffers(1, &m_glVertBuffer);
      m_glIndexBuffer = 0;
      m_glVertArray = 0;
      m_glVertBuffer = 0;
    }

  m_mdEle = 0;
  if (m_mdCount) delete [] m_mdCount;
  if (m_mdIndices) delete [] m_mdIndices;  
  m_mdCount = 0;
  m_mdIndices = 0;
}

void Viewer::stopDrawing() { m_draw = false; }
void
Viewer::startDrawing()
{
  m_draw = true;
  resizeGL(size().width(), size().height());
  updateGL();
}

void
Viewer::resizeGL(int width, int height)
{
  QGLViewer::resizeGL(width, height);

  if (!m_draw)
    return;
  
  createFBO();

  if (m_sketchPadMode)
    grabScreenImage();
}

void
Viewer::createFBO()
{
  if (!m_glewInitdone)
    return;
  
  int wd = camera()->screenWidth();
  int ht = camera()->screenHeight();

  //-----------------------------------------
  if (m_eBuffer) glDeleteFramebuffers(1, &m_eBuffer);
  if (m_ebId) glDeleteRenderbuffers(1, &m_ebId);
  if (m_ebTex[0]) glDeleteTextures(3, m_ebTex);
  glGenFramebuffers(1, &m_eBuffer);
  glGenRenderbuffers(1, &m_ebId);
  glGenTextures(3, m_ebTex);
  glBindFramebuffer(GL_FRAMEBUFFER, m_eBuffer);
  glBindRenderbuffer(GL_RENDERBUFFER, m_ebId);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, wd, ht);
  glBindRenderbuffer(GL_RENDERBUFFER, 0);
  // attach the renderbuffer to depth attachment point
  glFramebufferRenderbuffer(GL_FRAMEBUFFER,      // 1. fbo target: GL_FRAMEBUFFER
			    GL_DEPTH_ATTACHMENT, // 2. attachment point
			    GL_RENDERBUFFER,     // 3. rbo target: GL_RENDERBUFFER
			    m_ebId);              // 4. rbo ID
  for(int i=0; i<3; i++)
    {
      glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_ebTex[i]);
      glTexImage2D(GL_TEXTURE_RECTANGLE_ARB,
		   0,
		   GL_RGBA16F,
		   wd, ht,
		   0,
		   GL_RGBA,
		   GL_FLOAT,
		   0);
    }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  //-----------------------------------------


  //-----------------------------------------
  if (m_slcBuffer) glDeleteFramebuffers(1, &m_slcBuffer);
  if (m_rboId) glDeleteRenderbuffers(1, &m_rboId);
  if (m_slcTex[0]) glDeleteTextures(2, m_slcTex);  

  glGenFramebuffers(1, &m_slcBuffer);
  glGenRenderbuffers(1, &m_rboId);
  glGenTextures(2, m_slcTex);

  glBindFramebuffer(GL_FRAMEBUFFER, m_slcBuffer);
  glBindRenderbuffer(GL_RENDERBUFFER, m_rboId);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, wd, ht);
  glBindRenderbuffer(GL_RENDERBUFFER, 0);
  // attach the renderbuffer to depth attachment point
  glFramebufferRenderbuffer(GL_FRAMEBUFFER,      // 1. fbo target: GL_FRAMEBUFFER
			    GL_DEPTH_ATTACHMENT, // 2. attachment point
			    GL_RENDERBUFFER,     // 3. rbo target: GL_RENDERBUFFER
			    m_rboId);            // 4. rbo ID

  for(int i=0; i<2; i++)
    {
      glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_slcTex[i]);
      glTexImage2D(GL_TEXTURE_RECTANGLE_ARB,
		   0,
		   GL_RGBA16F,
		   wd, ht,
		   0,
		   GL_RGBA,
		   GL_FLOAT,
		   0);
    }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  //-----------------------------------------
}

void
Viewer::createRaycastShader()
{
  QString shaderString;

  int maxSteps = qSqrt(m_vsize.x*m_vsize.x +
		       m_vsize.y*m_vsize.y +
		       m_vsize.z*m_vsize.z);
  maxSteps *= 1.0/m_stillStep;

  if (Global::bytesPerVoxel() == 1)
    shaderString = ShaderFactory::genIsoRaycastShader(m_exactCoord, m_useMask, false, m_gradType);
  else
    shaderString = ShaderFactory::genIsoRaycastShader(m_exactCoord, m_useMask, true, m_gradType);
  
  if (m_rcShader)
    glDeleteObjectARB(m_rcShader);

  m_rcShader = glCreateProgramObjectARB();
  if (! ShaderFactory::loadShader(m_rcShader,
				  shaderString))
    {
      m_rcShader = 0;
      QMessageBox::information(0, "", "Cannot create rc shader.");
    }

  m_rcParm[0] = glGetUniformLocationARB(m_rcShader, "dataTex");
  m_rcParm[1] = glGetUniformLocationARB(m_rcShader, "lutTex");
  m_rcParm[2] = glGetUniformLocationARB(m_rcShader, "exitTex");
  m_rcParm[3] = glGetUniformLocationARB(m_rcShader, "stepSize");
  m_rcParm[4] = glGetUniformLocationARB(m_rcShader, "eyepos");
  m_rcParm[5] = glGetUniformLocationARB(m_rcShader, "viewDir");
  m_rcParm[6] = glGetUniformLocationARB(m_rcShader, "vcorner");
  m_rcParm[7] = glGetUniformLocationARB(m_rcShader, "vsize");
  m_rcParm[10]= glGetUniformLocationARB(m_rcShader, "maskTex");
  m_rcParm[11]= glGetUniformLocationARB(m_rcShader, "saveCoord");
  m_rcParm[12]= glGetUniformLocationARB(m_rcShader, "skipLayers");
  m_rcParm[13]= glGetUniformLocationARB(m_rcShader, "tagTex");
  m_rcParm[14] = glGetUniformLocationARB(m_rcShader, "entryTex");
  m_rcParm[15] = glGetUniformLocationARB(m_rcShader, "bgcolor");
  m_rcParm[16] = glGetUniformLocationARB(m_rcShader, "skipVoxels");
  m_rcParm[17] = glGetUniformLocationARB(m_rcShader, "nclip");
  m_rcParm[18] = glGetUniformLocationARB(m_rcShader, "clipPos");
  m_rcParm[19] = glGetUniformLocationARB(m_rcShader, "clipNormal");
  m_rcParm[20] = glGetUniformLocationARB(m_rcShader, "voxelScale");
  m_rcParm[21] = glGetUniformLocationARB(m_rcShader, "minGrad");
  m_rcParm[22] = glGetUniformLocationARB(m_rcShader, "maxGrad");
}

void
Viewer::createShaders()
{
  QString shaderString;

  createRaycastShader();


  //----------------------
  if (Global::bytesPerVoxel() == 1)
    shaderString = ShaderFactory::genEdgeEnhanceShader(false);
  else
    shaderString = ShaderFactory::genEdgeEnhanceShader(true);

  if (m_eeShader)
    glDeleteObjectARB(m_eeShader);

  m_eeShader = glCreateProgramObjectARB();
  if (! ShaderFactory::loadShader(m_eeShader,
				  shaderString))
    {
      m_eeShader = 0;
      QMessageBox::information(0, "", "Cannot create ee shader.");
    }

  m_eeParm[1] = glGetUniformLocationARB(m_eeShader, "minZ");
  m_eeParm[2] = glGetUniformLocationARB(m_eeShader, "maxZ");
  m_eeParm[3] = glGetUniformLocationARB(m_eeShader, "eyepos");
  m_eeParm[4] = glGetUniformLocationARB(m_eeShader, "viewDir");
  m_eeParm[5] = glGetUniformLocationARB(m_eeShader, "dzScale");
  m_eeParm[6] = glGetUniformLocationARB(m_eeShader, "tagTex");
  m_eeParm[7] = glGetUniformLocationARB(m_eeShader, "lutTex");
  m_eeParm[8] = glGetUniformLocationARB(m_eeShader, "pvtTex");
  m_eeParm[9] = glGetUniformLocationARB(m_eeShader, "lightparm");
  m_eeParm[10] = glGetUniformLocationARB(m_eeShader, "isoshadow");
  m_eeParm[11] = glGetUniformLocationARB(m_eeShader, "shadowcolor");
  m_eeParm[12] = glGetUniformLocationARB(m_eeShader, "edgecolor");
  m_eeParm[13] = glGetUniformLocationARB(m_eeShader, "bgcolor");
  m_eeParm[14] = glGetUniformLocationARB(m_eeShader, "shdoffset");
  //----------------------



  //----------------------
  shaderString = ShaderFactory::genDepthShader();

  m_depthShader = glCreateProgramObjectARB();
  if (! ShaderFactory::loadShader(m_depthShader,
				    shaderString))
    {
      m_depthShader = 0;
      QMessageBox::information(0, "", "Cannot create depth shader.");
    }

  m_depthParm[0] = glGetUniformLocationARB(m_depthShader, "minZ");
  m_depthParm[1] = glGetUniformLocationARB(m_depthShader, "maxZ");
  m_depthParm[2] = glGetUniformLocationARB(m_depthShader, "eyepos");
  m_depthParm[3] = glGetUniformLocationARB(m_depthShader, "viewDir");
  //----------------------  
}


void Viewer::setShowSlices(bool b) { m_showSlices = b; update(); }

void Viewer::setVoxelChoice(int p) { m_voxChoice = p; }
void Viewer::setVoxelInterval(int p)
{
  m_pointSkip = p;
}

void
Viewer::updateCurrSlice(int cst, int cs)
{
  m_currSliceType = cst;
  m_currSlice = cs;
  update();
}

void
Viewer::setPaintedTags(QList<int> t)
{
  m_paintedTags = t;
  update();
}

void
Viewer::setCurveTags(QList<int> t)
{
  m_curveTags = t;
  update();
}

void
Viewer::setFiberTags(QList<int> t)
{
  m_fiberTags = t;
  update();
}


void Viewer::setMaskDataPtr(uchar *ptr) { m_maskPtr = ptr; }
void Viewer::setVolDataPtr(uchar *ptr)
{
  m_volPtr = ptr;

  if (Global::bytesPerVoxel() == 2)
    m_volPtrUS = (ushort*)ptr;
}

void
Viewer::getBox(int& minD, int& maxD, int& minW, int& maxW, int& minH, int& maxH)
{
  Vec bmin, bmax;
  m_boundingBox.bounds(bmin, bmax);

  Vec voxelScaling = Global::relativeVoxelScaling();
  bmin = VECDIVIDE(bmin, voxelScaling);
  bmax = VECDIVIDE(bmax, voxelScaling);

  minD = bmin.z;
  maxD = bmax.z;
  minW = bmin.y;
  maxW = bmax.y;
  minH = bmin.x;
  maxH = bmax.x;
}

void
Viewer::updateViewerBox(int minD, int maxD, int minW, int maxW, int minH, int maxH)
{
  m_minDSlice = minD;
  m_maxDSlice = maxD;

  m_minWSlice = minW;
  m_maxWSlice = maxW;

  m_minHSlice = minH;
  m_maxHSlice = maxH;

  m_cminD = m_minDSlice;
  m_cminW = m_minWSlice;
  m_cminH = m_minHSlice;
  m_cmaxD = m_maxDSlice;
  m_cmaxW = m_maxWSlice;
  m_cmaxH = m_maxHSlice;

  Vec voxelScaling = Global::relativeVoxelScaling();

  Vec bmin = Vec(m_minHSlice, m_minWSlice, m_minDSlice);
  Vec bmax = Vec(m_maxHSlice, m_maxWSlice, m_maxDSlice);
  bmin = VECPRODUCT(bmin, voxelScaling);
  bmax = VECPRODUCT(bmax, voxelScaling);

  setSceneCenter((bmin+bmax)/2);
  
  m_clipPlanes->setBounds(bmin, bmax);

  m_boundingBox.setPositions(bmin, bmax);
}

void
Viewer::setShowBox(bool b)
{
  m_showBox = b;
  if (m_showBox)
    m_boundingBox.activateBounds();
  else
    m_boundingBox.deactivateBounds();

  update();  
}

void
Viewer:: setRenderMode(bool flag)
{
  m_renderMode = flag;

  update();
}

void
Viewer::setSkipLayers(int l)
{
  m_skipLayers = l;
  update();
}

void
Viewer::setSkipVoxels(int l)
{
  m_skipVoxels = l;
  update();
}

void
Viewer::showSketchPad(bool b)
{  
  m_sketchPadMode = b;
  m_poly.clear();
  if (m_sketchPadMode)
    grabScreenImage();
  update();
}

void
Viewer::keyPressEvent(QKeyEvent *event)
{
  Vec voxelScaling = Global::relativeVoxelScaling();

  if (event->key() == Qt::Key_Escape)
    {
      if (m_savingImages > 0)
	{
	  if (m_savingImages == 2) // movie
	    endMovie();
	  
	  m_savingImages = 0;
	  QFrame *cv = (QFrame*)parent()->children()[1];
	  cv->show();
	  QMessageBox::information(0, "", "Stopped Saving Image Sequence");
	  return;
	}

      if (m_sketchPadMode)
	{
	  m_poly.clear();
	  update();
	}
      return;
    }

  if (m_boundingBox.keyPressEvent(event))
    return;

  if (event->key() == Qt::Key_Question)
    {
      m_infoText = !m_infoText;
      update();
      return;
    }

//  if (event->key() == Qt::Key_Z)
//    {  
//      if (event->modifiers() & Qt::ControlModifier)
//	{
//	  emit undoPaint3D();
//	  update();
//	}
//      return;
//    }

  if (event->key() == Qt::Key_A)
    {  
      toggleAxisIsDrawn();
      update();
      return;
    }

  if (event->key() == Qt::Key_B)
    {  
      setShowBox(!m_showBox);
      emit showBoxChanged(m_showBox);
      update();
      return;
    }

  if (event->key() == Qt::Key_F)
    {  
      if (event->modifiers() & Qt::ShiftModifier)
	regionGrowing(true); // shrinkwrap
      else
	regionGrowing(false);
      update();
      return;
    }

  if (event->key() == Qt::Key_H)
    {  
      hatch();
      update();
      return;
    }

  if (event->key() == Qt::Key_D)
    {  
      if (event->modifiers() & Qt::ShiftModifier)
	regionDilation(true);
      else
	regionDilation(false);
      update();
      return;
    }

  if (event->key() == Qt::Key_E)
    {  
      regionErosion();
      update();
      return;
    }

  if (event->key() == Qt::Key_M)
    {  
      Vec bmin, bmax;
      m_boundingBox.bounds(bmin, bmax);
      bmin = VECDIVIDE(bmin, voxelScaling);
      bmax = VECDIVIDE(bmax, voxelScaling);

      if (m_tag1 > -1 && m_tag2 >= -1)
	emit mergeTags(bmin, bmax, m_tag1, m_tag2, m_mergeTagTF);      
      else
	QMessageBox::information(0, "", "No previous tags specified");
      return;
    }

  if (event->key() == Qt::Key_T) // tag using sketch pad
    {  
      if (m_sketchPadMode)
	{
	  tagUsingScreenSketch();
	  update();
	}
      else
	QMessageBox::information(0, "", "Not in Sketch Pad Mode");
      return;
    }

  // process clipplane events
  Vec bmin, bmax;
  m_boundingBox.bounds(bmin, bmax);

  m_clipPlanes->setBounds(bmin, bmax);
  if (m_clipPlanes->keyPressEvent(event))
    {
      update();
      return;
    }

  if (event->key() == Qt::Key_P)
    {
      camera()->setType(Camera::PERSPECTIVE);
      update();
      return;
    }

  if (event->key() == Qt::Key_O)
    {
      camera()->setType(Camera::ORTHOGRAPHIC);
      update();
      return;
    }

  if (event->key() == Qt::Key_V)
    {
      if (m_clipPlanes->count() > 0)
	{
	  bool show = m_clipPlanes->show(0);
	  if (show)
	    m_clipPlanes->hide();
	  else
	    m_clipPlanes->show();
	}
      return;
    }

  if (event->key() == Qt::Key_Space)
    {
      commandEditor();
      return;
    }

  if (event->key() != Qt::Key_H)
    QGLViewer::keyPressEvent(event);
}

void
Viewer::commandEditor()
{
  PropertyEditor propertyEditor;
  QMap<QString, QVariantList> plist;
  QVariantList vlist;
  vlist.clear();
  plist["command"] = vlist;

  vlist.clear();
  QFile helpFile(":/viewer.help");
  if (helpFile.open(QFile::ReadOnly))
    {
      QTextStream in(&helpFile);
      QString line = in.readLine();
      while (!line.isNull())
	{
	  if (line == "#begin")
	    {
	      QString keyword = in.readLine();
	      QString helptext;
	      line = in.readLine();
	      while (!line.isNull())
		{
		  helptext += line;
		  helptext += "\n";
		  line = in.readLine();
		  if (line == "#end") break;
		}
	      vlist << keyword << helptext;
	    }
	  line = in.readLine();
	}
    }
  
  plist["commandhelp"] = vlist;
  
  //---------------------
  vlist.clear();
  QString mesg;

  mesg += "Subvolume Bounds :\n";
  mesg += QString("LOD : %1\n").arg(m_sslevel);

  mesg += QString("%1 %2 %3\n%4 %5 %6\n").			\
    arg(m_minHSlice).arg(m_minWSlice).arg(m_minDSlice).		\
    arg(m_maxHSlice).arg(m_maxWSlice).arg(m_maxDSlice);

  int d, w, h;
  bool gothit = getCoordUnderPointer(d, w, h);
  if (gothit)
    {
      int val;
      if (Global::bytesPerVoxel() == 1)
	val = m_volPtr[d*m_width*m_height + w*m_height + h];
      else
	val = m_volPtrUS[d*m_width*m_height + w*m_height + h];

      int tag = m_maskPtr[d*m_width*m_height + w*m_height + h];
      mesg += "\nPoint Information\n";
      mesg += QString("Coordinate : %1 %2 %3\n").arg(h).arg(w).arg(d);
      mesg += QString("Voxel value : %1\n").arg(val);
      mesg += QString("Tag value : %1\n").arg(tag);
      int op = Global::lut()[4*val+3]*Global::tagColors()[4*tag+3];
      mesg += QString("Visible : %1\n").arg(op>0);
    }
  else
    mesg += "\nNo point found under the pointer.";
  
  vlist << mesg;

  plist["message"] = vlist;
  //---------------------



  QStringList keys;
  keys << "command";
  keys << "commandhelp";
  keys << "message";
  
  propertyEditor.set("Command Help", plist, keys);
  
  QMap<QString, QPair<QVariant, bool> > vmap;
  
  if (propertyEditor.exec() == QDialog::Accepted)
    {
      QString cmd = propertyEditor.getCommandString();
      if (!cmd.isEmpty())
	processCommand(cmd);
    }
  else
    return;
}

void
Viewer::processCommand(QString cmd)
{
  bool ok;
  QString ocmd = cmd;
  cmd = cmd.toLower();
  QStringList list = cmd.split(" ", QString::SkipEmptyParts);
 
  Vec voxelScaling = Global::relativeVoxelScaling();

  Vec bmin, bmax;
  m_boundingBox.bounds(bmin, bmax);
  bmin = VECDIVIDE(bmin, voxelScaling);
  bmax = VECDIVIDE(bmax, voxelScaling);

  if (list[0].contains("tagsused"))
    {
      QList<int> ut = usedTags();
//      QString tmesg;  
//      tmesg += "Tags Used\n";
//      for(int ti=0; ti<ut.count(); ti++)
//	{
//	  if (ut[ti] > 0)
//	    tmesg += QString("%1 ").arg(ut[ti]);
//
//	  if (ti%10 == 9)
//	    tmesg += "\n";
//	}
//      QMessageBox::information(0, "Tags Used", tmesg);

      emit tagsUsed(ut);
      return;
    }

  
  if (list[0].contains("shrinkwrap"))
    {
      int tag1 = Global::tag();
      if (list.size() == 2)
	{
	  tag1 = list[1].toInt(&ok);
	  if (tag1 < 0 || tag1 > 255)
	    {
	      QMessageBox::information(0, "", QString("Incorrect tags specified : %1").\
				       arg(tag1));
	      return;
	    }
	}
      emit shrinkwrap(bmin, bmax, tag1, false, 1);
      return;
    }


  if (list[0].contains("shell"))
    {
      int tag1 = Global::tag();
      int width = 1;
      if (list.size() >= 2)
	{
	  tag1 = list[1].toInt(&ok);
	  if (tag1 < 0 || tag1 > 255)
	    {
	      QMessageBox::information(0, "", QString("Incorrect tags specified : %1").\
				       arg(tag1));
	      return;
	    }
	}
      if (list.size() >= 3)
	{
	  width = list[2].toInt(&ok);
	  if (width < 0)
	    {
	      QMessageBox::information(0, "", QString("Incorrect width specified : %1").\
				       arg(width));
	      return;
	    }
	}
      emit shrinkwrap(bmin, bmax, tag1, true, width);
      return;
    }

  
  if (list[0].contains("tube"))
    {
      int tag1 = Global::tag();
      if (list.size() >= 2)
	{
	  tag1 = list[1].toInt(&ok);
	  if (tag1 < 0 || tag1 > 255)
	    {
	      QMessageBox::information(0, "", QString("Incorrect tags specified : %1").\
				       arg(tag1));
	      return;
	    }
	}
      emit tagTubes(bmin, bmax, tag1);
      return;
    }


  if (list[0].contains("reset"))
    {
      int tag1 = 0;
      if (list.size() == 2)
	{
	  tag1 = list[1].toInt(&ok);
	  if (tag1 < 0 || tag1 > 255)
	    {
	      QMessageBox::information(0, "", QString("Incorrect tags specified : %1").\
				       arg(tag1));
	      return;
	    }
	}
      emit resetTag(bmin, bmax, tag1);
      return;
    }

  if (list[0].contains("reload"))
    {
      emit reloadMask();
      return;
    }
  if (list[0].contains("loadmask"))
    {
      QString flnm;
      flnm = QFileDialog::getOpenFileName(0,
					  "RawMask File",
					  Global::previousDirectory(),
					  "RawMask Files (*.raw)");
      
      if (flnm.isEmpty())
	return;
      emit loadRawMask(flnm);
    }
  
  if (list[0].contains("getvolume"))
    {
      int tag1 = -1;
      if (list.size() == 2)
	{
	  tag1 = list[1].toInt(&ok);
	  if (tag1 < -1 || tag1 > 255)
	    {
	      QMessageBox::information(0, "", QString("Incorrect tags specified : %1").\
				       arg(tag1));
	      return;
	    }
	}
      QList<Vec> cPos =  clipPos();
      QList<Vec> cNorm = clipNorm();
      VolumeOperations::setClip(cPos, cNorm);
      VolumeOperations::getVolume(bmin, bmax, tag1);
      return;
    }

  if (list[0].contains("setvisible"))
    {
      if (list.size() == 2)
	{
	  int tag1 = list[1].toInt(&ok);
	  if (tag1 < 0 || tag1 > 255)
	    {
	      QMessageBox::information(0, "", QString("Incorrect tags specified : %1").\
				       arg(tag1));
	      return;
	    }
	  emit setVisible(bmin, bmax, tag1, true);
	}
      return;
    }

  if (list[0].contains("setinvisible"))
    {
      if (list.size() == 2)
	{
	  int tag1 = list[1].toInt(&ok);
	  if (tag1 < 0 || tag1 > 255)
	    {
	      QMessageBox::information(0, "", QString("Incorrect tags specified : %1").\
				       arg(tag1));
	      return;
	    }
	  emit setVisible(bmin, bmax, tag1, false);
	}
      return;
    }

  if (list[0].contains("mergetf"))
    {
      if (list.size() == 3)
	{
	  int tag1 = list[1].toInt(&ok);
	  int tag2 = list[2].toInt(&ok);
	  if (tag1 < 0 || tag1 > 255 ||
	      tag2 < -1 || tag2 > 255) // tag2 can be -1
	    {
	      QMessageBox::information(0, "", QString("Incorrect tags specified : %1 %2").\
				       arg(tag1).arg(tag2));
	      return;
	    }
	  emit mergeTags(bmin, bmax, tag1, tag2, true);
	  m_tag1 = tag1;
	  m_tag2 = tag2;
	  m_mergeTagTF = true;
	}
      else
	QMessageBox::information(0, "", "Incorrect parameters : merge <tag1> <tag2>");

      return;
    }

  if (list[0].contains("tagstep"))
    {
      if (list.size() > 1)
	{
	  int tagStep = list[1].toInt(&ok);
	  int tagVal = Global::tag();
	  if (list.size() > 2)
	    tagVal = list[2].toInt(&ok);
	  if (tagStep < 1 || tagStep > 254)
	    {
	      QMessageBox::information(0, "", QString("Incorrect tagStep [1-254] specified : %1").\
				       arg(tagStep));
	      return;
	    }
	  if (tagVal < 1 || tagStep > 254)
	    {
	      QMessageBox::information(0, "", QString("Incorrect tagVal [0-254] specified : %1").\
				       arg(tagVal));
	      return;
	    }
	  emit stepTag(bmin, bmax, tagStep, tagVal);	  
	}
      return;
    }
  
  if (list[0].contains("merge"))
    {
      if (list.size() == 3)
	{
	  int tag1 = list[1].toInt(&ok);
	  int tag2 = list[2].toInt(&ok);
	  if (tag1 < 0 || tag1 > 255 ||
	      tag2 < -1 || tag2 > 255) // tag2 can be -1
	    {
	      QMessageBox::information(0, "", QString("Incorrect tags specified : %1 %2").\
				       arg(tag1).arg(tag2));
	      return;
	    }
	  emit mergeTags(bmin, bmax, tag1, tag2, false);
	  m_tag1 = tag1;
	  m_tag2 = tag2;
	  m_mergeTagTF = false;
	}
      else
	QMessageBox::information(0, "", "Incorrect parameters : merge <tag1> <tag2>");

      return;
    }

  if (list[0].contains("modifyoriginalvolume"))
    {
      QStringList dtypes;
      dtypes << "No";
      dtypes << "Yes"; 
      bool ok;
      QString option = QInputDialog::getItem(0,
				  "Modify Original Volume",
				   QString("BE VERY CAREFUL WHEN USING THIS FUNCTION\nOriginal Volume will be modified.\nValues for the voxels in the region that is invisible in the 3D Preview window\nwill be replaced with the value that you specify.\nDo you want to proceed ?"),
				   dtypes,
				   0,
				   false,
				   &ok);

      if (!ok)
	return;

      if (option != "Yes")
	return;
      
      int val = 0;
      val = QInputDialog::getInt(0,
				  "Modify Original Volume",
				  "Please specify value for voxels in the transparent region.",
				  0, 0, 255, 1);

      
      emit modifyOriginalVolume(bmin, bmax, val);
      return;
    }

}

void
Viewer::setGridSize(int d, int w, int h)
{
  createShaders();

  m_depth = d;
  m_width = w;
  m_height = h;

  m_minDSlice = 0;
  m_minWSlice = 0;
  m_minHSlice = 0;

  m_maxDSlice = d-1;
  m_maxWSlice = w-1;
  m_maxHSlice = h-1;
//  m_maxDSlice = d;
//  m_maxWSlice = w;
//  m_maxHSlice = h;

  m_cminD = m_minDSlice;
  m_cminW = m_minWSlice;
  m_cminH = m_minHSlice;
  m_cmaxD = m_maxDSlice;
  m_cmaxW = m_maxWSlice;
  m_cmaxH = m_maxHSlice;

  Vec voxelScaling = Global::relativeVoxelScaling();

  Vec bmax = Vec(m_maxHSlice, m_maxWSlice, m_maxDSlice);
  bmax = VECPRODUCT(bmax, voxelScaling);

  m_clipPlanes->setBounds(Vec(0,0,0), bmax);

  m_boundingBox.setBounds(Vec(0,0,0), bmax);


  Vec fullBox = Vec(m_height, m_width, m_depth);
  fullBox = VECPRODUCT(fullBox, voxelScaling);
  setSceneBoundingBox(Vec(0,0,0), fullBox);

  setSceneCenter(fullBox/2);
  showEntireScene();


  // set optimal box size
  m_boxSize = qMax(m_height/128, m_width/128);
  m_boxSize = qMax((qint64)m_boxSize, m_depth/128);
  m_boxSize = qMax(m_boxSize, 8);

  generateBoxMinMax();

  generateBoxes();

  loadAllBoxesToVBO();
}

void
Viewer::drawEnclosingCube(Vec subvolmin,
			  Vec subvolmax)
{
  glBegin(GL_QUADS);  
  glVertex3f(subvolmin.x, subvolmin.y, subvolmin.z);
  glVertex3f(subvolmax.x, subvolmin.y, subvolmin.z);
  glVertex3f(subvolmax.x, subvolmax.y, subvolmin.z);
  glVertex3f(subvolmin.x, subvolmax.y, subvolmin.z);
  glEnd();
  
  // FRONT 
  glBegin(GL_QUADS);  
  glVertex3f(subvolmin.x, subvolmin.y, subvolmax.z);
  glVertex3f(subvolmax.x, subvolmin.y, subvolmax.z);
  glVertex3f(subvolmax.x, subvolmax.y, subvolmax.z);
  glVertex3f(subvolmin.x, subvolmax.y, subvolmax.z);
  glEnd();
  
  // TOP
  glBegin(GL_QUADS);  
  glVertex3f(subvolmin.x, subvolmax.y, subvolmin.z);
  glVertex3f(subvolmax.x, subvolmax.y, subvolmin.z);
  glVertex3f(subvolmax.x, subvolmax.y, subvolmax.z);
  glVertex3f(subvolmin.x, subvolmax.y, subvolmax.z);
  glEnd();
  
  // BOTTOM
  glBegin(GL_QUADS);  
  glVertex3f(subvolmin.x, subvolmin.y, subvolmin.z);
  glVertex3f(subvolmax.x, subvolmin.y, subvolmin.z);
  glVertex3f(subvolmax.x, subvolmin.y, subvolmax.z);
  glVertex3f(subvolmin.x, subvolmin.y, subvolmax.z);  
  glEnd();    
}

void
Viewer::drawCurrentSlice()
{
  glColor4d(1.0,0.5,0.2, 0.7);

  glLineWidth(1);

  Vec voxelScaling = Global::relativeVoxelScaling();
  Vec pos = VECPRODUCT(Vec(m_hslice, m_wslice, m_dslice), voxelScaling);

  glBegin(GL_LINES);
  glVertex3d(pos.x-50, pos.y,    pos.z);
  glVertex3d(pos.x+50, pos.y,    pos.z);
  glVertex3d(pos.x,    pos.y-50, pos.z);
  glVertex3d(pos.x,    pos.y+50, pos.z);
  glVertex3d(pos.x,    pos.y,    pos.z-50);
  glVertex3d(pos.x,    pos.y,    pos.z+50);
  glEnd();
}

void
Viewer::drawWireframeBox()
{
  Vec voxelScaling = Global::relativeVoxelScaling();

  //setAxisIsDrawn();
  
  glColor3d(0.5,0.5,0.5);

  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

  Vec fullBox = Vec(m_height, m_width, m_depth);
  fullBox = VECPRODUCT(fullBox, voxelScaling);

  Vec bmin = Vec(m_minHSlice, m_minWSlice, m_minDSlice);
  Vec bmax = Vec(m_maxHSlice, m_maxWSlice, m_maxDSlice);
  bmin = VECPRODUCT(bmin, voxelScaling);
  bmax = VECPRODUCT(bmax, voxelScaling);
  
  
  glLineWidth(1);
  drawEnclosingCube(Vec(0,0,0), fullBox);
    
  glLineWidth(2);
  glColor3d(0.8,0.8,0.8);
  drawEnclosingCube(bmin, bmax);
  
  
//  glLineWidth(3);
//  glColor3d(1.0,0.85,0.7);
//  drawCurrentSlice(Vec(0,0,0), fullBox);
  
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  glLineWidth(1);
}

void
Viewer::drawBoxes2D()
{
  if (!Global::showBox2D())
    return;

  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

  QList<Vec> bl = Global::boxList2D();
  glLineWidth(1);
  glColor4d(0.3,0.2,0.1, 0.3);
  for (int i=0; i<bl.count()/2; i++)
    {
      Vec bmin = Vec(bl[2*i].z, bl[2*i].y, bl[2*i].x);
      Vec bmax = Vec(bl[2*i+1].z-1, bl[2*i+1].y-1, bl[2*i+1].x-1);
      drawEnclosingCube(bmin, bmax);
    }  

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void
Viewer::drawBoxes3D()
{
  if (!Global::showBox3D)
    return;

  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

  Vec bsize = Global::boxSize3D();
  QList<Vec> bl = Global::boxList3D();
  glLineWidth(1);
  glColor4d(0.1,0.2,0.3,0.3);
  for (int i=0; i<bl.count(); i++)
    {
      Vec bmin = Vec(bl[i].z, bl[i].y, bl[i].x);
      Vec bmax = bmin + bsize;
      drawEnclosingCube(bmin, bmax);
    }  

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}


void
Viewer::setMultiMapCurves(int type, QMultiMap<int, Curve*> *cg)
{
  if (type == 0) m_Dcg = cg;
  if (type == 1) m_Wcg = cg;
  if (type == 2) m_Hcg = cg;
}

void
Viewer::setListMapCurves(int type, QList< QMap<int, Curve> > *cg)
{
  if (type == 0) m_Dmcg = cg;
  if (type == 1) m_Wmcg = cg;
  if (type == 2) m_Hmcg = cg;
}

void
Viewer::setShrinkwrapCurves(int type, QList< QMultiMap<int, Curve*> > *cg)
{
  if (type == 0) m_Dswcg = cg;
  if (type == 1) m_Wswcg = cg;
  if (type == 2) m_Hswcg = cg;
}

void
Viewer::setFibers(QList<Fiber*> *fb)
{
  m_fibers = fb;
}

void
Viewer::draw()
{
  if (!m_dataTex)
    return;

  if (!m_draw)
    {
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      drawInfo();
      return;
    }
  
  if (m_sketchPadMode)
    {
      glDisable(GL_DEPTH_TEST);

      drawScreenImage();

      int wd = camera()->screenWidth();
      int ht = camera()->screenHeight();
      StaticFunctions::pushOrthoView(0, 0, wd, ht);

      int tag = Global::tag();
      float r = Global::tagColors()[4*tag+0]*1.0/255.0;
      float g = Global::tagColors()[4*tag+1]*1.0/255.0;
      float b = Global::tagColors()[4*tag+2]*1.0/255.0;
      glColor3f(r,g,b);
      glDisable(GL_LIGHTING);
      glLineWidth(2);
      glBegin(GL_LINE_STRIP);
      for(int j=0; j<m_poly.count(); j++)
	glVertex2f(m_poly[j].x(),ht-m_poly[j].y());
      glEnd();

      QFont tfont = QFont("Helvetica", 12);  
      StaticFunctions::renderText(10,50, "Sketch Pad Mode", tfont, Qt::lightGray, Qt::black);
      
      StaticFunctions::popOrthoView();
      glEnable(GL_DEPTH_TEST);
      glDisable(GL_BLEND);
      return;
    }


  Vec voxelScaling = Global::relativeVoxelScaling();

  //Vec bmin, bmax;
  //m_boundingBox.bounds(bmin, bmax);
  //camera()->setRevolveAroundPoint((bmax+bmin)/2);


  glClearColor(0, 0, 0, 1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);

  glDisable(GL_LIGHTING);

  if ((!m_volPtr || !m_maskPtr) && m_showBox)
    {
      m_boundingBox.draw();
      drawWireframeBox();

      drawBoxes2D();
      drawBoxes3D();
    }

  if (!m_volPtr || !m_maskPtr)
    {
      if (m_savingImages > 0)
	saveImageFrame();
      return;
    }

  raycasting();

  if (m_savingImages > 0)
    saveImageFrame();
  else
    drawInfo();
}

void
Viewer::raycasting()
{
  Vec voxelScaling = Global::relativeVoxelScaling();

  Vec box[8];
  box[0] = Vec(m_minHSlice, m_minWSlice, m_minDSlice);
  box[1] = Vec(m_minHSlice, m_minWSlice, m_maxDSlice);
  box[2] = Vec(m_minHSlice, m_maxWSlice, m_maxDSlice);
  box[3] = Vec(m_minHSlice, m_maxWSlice, m_minDSlice);
  box[4] = Vec(m_maxHSlice, m_minWSlice, m_minDSlice);
  box[5] = Vec(m_maxHSlice, m_minWSlice, m_maxDSlice);
  box[6] = Vec(m_maxHSlice, m_maxWSlice, m_maxDSlice);
  box[7] = Vec(m_maxHSlice, m_maxWSlice, m_minDSlice);

  Vec eyepos = camera()->position();
  Vec viewDir = camera()->viewDirection();
  float minZ = 1000000;
  float maxZ = -1000000;
  for(int b=0; b<8; b++)
    {
      box[b] = VECPRODUCT(box[b], voxelScaling);
      float zv = (box[b]-eyepos)*viewDir;
      minZ = qMin(minZ, zv);
      maxZ = qMax(maxZ, zv);
    }
  
  glEnable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);


  volumeRaycast(minZ, maxZ, false); // run full raycast process

 
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE);
      
  m_clipPlanes->draw(this, false);

  if (m_showSlices)
    drawCurrentSlice();

  if (m_showBox)
    {
      m_boundingBox.draw();
      drawWireframeBox();
    }

  drawBoxes2D();
  drawBoxes3D();
      
  glEnable(GL_DEPTH_TEST);
}


void
Viewer::drawInfo()
{
  if (!m_infoText)
    return;

  glDisable(GL_DEPTH_TEST);

  QFont tfont = QFont("Helvetica", 12);  
  QString mesg;

  mesg += QString("LOD(%1) Vol(%2 %3 %4) ").				\
    arg(m_sslevel).arg(m_vsize.x+1).arg(m_vsize.y+1).arg(m_vsize.z+1);
//  mesg += QString("   %1").\
//    arg(m_numBoxes*(4+4+36*4*3)/(1024.0*1024.0));

      
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_POINT_SMOOTH);

  Vec voxelScaling = Global::relativeVoxelScaling();
      
  if (m_paintHit && m_target.x > -1)
    {
      float cpgl = camera()->pixelGLRatio(m_target);
      int d = m_target.z * voxelScaling.z;
      int w = m_target.y * voxelScaling.y;
      int h = m_target.x * voxelScaling.x;
      int tag = Global::tag();
      float r = Global::tagColors()[4*tag+0]*1.0/255.0;
      float g = Global::tagColors()[4*tag+1]*1.0/255.0;
      float b = Global::tagColors()[4*tag+2]*1.0/255.0;

      glActiveTexture(GL_TEXTURE0);
      glEnable(GL_POINT_SPRITE_ARB);
      glEnable(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D, Global::hollowSpriteTexture());
      glTexEnvi(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE);
      glPointSize(2*Global::spread()/cpgl);
      glBegin(GL_POINTS);
      glColor4f(r*0.5, g*0.5, b*0.5, 0.5);
      glVertex3f(h,w,d);
      glEnd();

      glDisable(GL_POINT_SPRITE);
      glDisable(GL_TEXTURE_2D);

      mesg += QString("%1 %2 %3").arg(h).arg(w).arg(d);
    }
  
  int wd = camera()->screenWidth();
  int ht = camera()->screenHeight();
  StaticFunctions::pushOrthoView(0, 0, wd, ht);
  StaticFunctions::renderText(10,10, mesg, tfont, Qt::black, Qt::lightGray);

  tfont = QFont("Helvetica", 10); 
  Vec bmin, bmax;
  m_boundingBox.bounds(bmin, bmax);
  bmin = VECDIVIDE(bmin, voxelScaling);
  bmax = VECDIVIDE(bmax, voxelScaling);
  int bminz = bmin.z;
  int bminy = bmin.y;
  int bminx = bmin.x;
  int bmaxz = qCeil(bmax.z); // because we are getting it as float
  int bmaxy = qCeil(bmax.y);
  int bmaxx = qCeil(bmax.x);
  mesg = QString("box(%1 %2 %3 : %4 %5 %6 : %7 %8 %9) ").		\
    arg(bminx).arg(bminy).arg(bminz).				\
    arg(bmaxx).arg(bmaxy).arg(bmaxz).		\
    arg(bmaxx-bminx+1).arg(bmaxy-bminy+1).arg(bmaxz-bminz+1);
  float vszgb = (float)(bmaxx-bminx)*(float)(bmaxy-bminy)*(float)(bmaxz-bminz);
  vszgb /= m_sslevel;
  vszgb /= m_sslevel;
  vszgb /= m_sslevel;
  vszgb /= 1024;
  vszgb /= 1024;
  if (vszgb < 1000)
    mesg += QString("mb(%1 @ %2)").arg(vszgb).arg(m_sslevel);
  else
    mesg += QString("gb(%1 @ %2)").arg(vszgb/1024).arg(m_sslevel);
  StaticFunctions::renderText(10,30, mesg, tfont, Qt::black, Qt::lightGray);

  int sh = camera()->screenHeight();  
  StaticFunctions::renderText(10,sh-30, QString("Current Tag : %1").arg(Global::tag()),
			      tfont, Qt::black, Qt::lightGray);

  StaticFunctions::popOrthoView();

  glEnable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);
}

void
Viewer::drawFibers()
{
  if (!m_fibers) return;

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  bool noneselected = true;
  for(int i=0; i<m_fibers->count(); i++)
    {
      Fiber *fb = m_fibers->at(i);
      if (fb->selected)
	{
	  noneselected = false;
	  break;
	}
    }
 
  for(int i=0; i<m_fibers->count(); i++)
    {
      Fiber *fb = m_fibers->at(i);
      int tag = fb->tag;
      if (m_fiberTags.count() == 0 ||
	  m_fiberTags[0] == -1 ||
	  m_fiberTags.contains(tag))
	{
	  float r = Global::tagColors()[4*tag+0]*1.0/255.0;
	  float g = Global::tagColors()[4*tag+1]*1.0/255.0;
	  float b = Global::tagColors()[4*tag+2]*1.0/255.0;
	  glColor3f(r,g,b);

	  if (noneselected ||
	      (!noneselected && fb->selected))
	    {
	      glEnable(GL_LIGHTING);
	      QList<Vec> tube = fb->tube();
	      glBegin(GL_TRIANGLE_STRIP);
	      for(int t=0; t<tube.count()/2; t++)
		{
		  glNormal3fv(tube[2*t+0]);	      
		  glVertex3fv(tube[2*t+1]);
		}
	      glEnd();
	    }
	  else
	    {
	      glDisable(GL_LIGHTING);
	      glLineWidth(1);
	      glBegin(GL_LINE_STRIP);
	      for(int j=0; j<fb->smoothSeeds.count(); j++)
		glVertex3fv(fb->smoothSeeds[j]);
	      glEnd();
	    }
	}
    }

  glLineWidth(1);
}

bool
Viewer::clip(int d, int w, int h)
{
  QList<Vec> cPos =  clipPos();
  QList<Vec> cNorm = clipNorm();

  for(int i=0; i<cPos.count(); i++)
    {
      Vec cpos = cPos[i];
      Vec cnorm = cNorm[i];
      
      Vec p = Vec(h, w, d) - cpos;
      if (cnorm*p > 0)
	return true;
    }

  return false;
}

void
Viewer::updateClipVoxels()
{
  m_clipVoxels.clear();

  if (!m_volPtr || !m_maskPtr)
    return;
  
  QList<Vec> cPos =  clipPos();
  QList<Vec> cNorm = clipNorm();

  uchar *lut = Global::lut();

  Vec box[8];
  box[0] = Vec(m_minHSlice, m_minWSlice, m_minDSlice);
  box[1] = Vec(m_minHSlice, m_minWSlice, m_maxDSlice);
  box[2] = Vec(m_minHSlice, m_maxWSlice, m_maxDSlice);
  box[3] = Vec(m_minHSlice, m_maxWSlice, m_minDSlice);
  box[4] = Vec(m_maxHSlice, m_minWSlice, m_minDSlice);
  box[5] = Vec(m_maxHSlice, m_minWSlice, m_maxDSlice);
  box[6] = Vec(m_maxHSlice, m_maxWSlice, m_maxDSlice);
  box[7] = Vec(m_maxHSlice, m_maxWSlice, m_minDSlice);

  Vec voxelScaling = Global::relativeVoxelScaling();

  for(int i=0; i<cPos.count(); i++)
    {
      if (m_clipPlanes->show(i))
	{
	  Vec cpos = cPos[i];
	  Vec cnorm = cNorm[i];

	  Vec xaxis = cnorm.orthogonalVec().unit();
	  Vec yaxis = cnorm ^ xaxis;

	  //--- drop perpendiculars onto normal from all 8 vertices of the subvolume 
	  float boxdist[8];
	  
	  // get width
	  for (int i=0; i<8; i++) boxdist[i] = (box[i] - cpos)*xaxis;
	  float wmin = boxdist[0];
	  for (int i=1; i<8; i++) wmin = qMin(wmin, boxdist[i]);
	  float wmax = boxdist[0];
	  for (int i=1; i<8; i++) wmax = qMax(wmax, boxdist[i]);
	  //------------------------
	  
	  // get height
	  for (int i=0; i<8; i++) boxdist[i] = (box[i] - cpos)*yaxis;
	  float hmin = boxdist[0];
	  for (int i=1; i<8; i++) hmin = qMin(hmin, boxdist[i]);
	  float hmax = boxdist[0];
	  for (int i=1; i<8; i++) hmax = qMax(hmax, boxdist[i]);
	  //------------------------
	  
	  
	  for(int a=(int)wmin; a<(int)wmax; a++)
	    for(int b=(int)hmin; b<(int)hmax; b++)
	      {
		Vec co = cpos+a*xaxis+b*yaxis;	
		int d = co.z;
		int w = co.y;
		int h = co.x;
		if (d > m_minDSlice && d < m_maxDSlice &&
		    w > m_minWSlice && w < m_maxWSlice &&
		    h > m_minHSlice && h < m_maxHSlice)
		  {
		    uchar vol = m_volPtr[d*m_width*m_height + w*m_height + h];
		    if (lut[4*vol+3] > 10)
		      m_clipVoxels << d << w << h << vol;
		  }
	      }
	}
    }
  //-------------------------------------
}


void
Viewer::updateVoxels()
{
  m_voxels.clear();
  
  if (!m_volPtr || !m_maskPtr)
    {
      QMessageBox::information(0, "",
			       "Data not loaded into memory, therefore cannot show the voxels");
      return;
    }

  Vec voxelScaling = Global::relativeVoxelScaling();

  Vec bmin, bmax;
  m_boundingBox.bounds(bmin, bmax);

  setSceneCenter((bmax+bmin)/2);

  bmin = VECDIVIDE(bmin, voxelScaling);
  bmax = VECDIVIDE(bmax, voxelScaling);

  emit updateSliceBounds(bmin, bmax);

  m_minDSlice = bmin.z;
  m_minWSlice = bmin.y;
  m_minHSlice = bmin.x;
  m_maxDSlice = qCeil(bmax.z); // because we are getting it as float
  m_maxWSlice = qCeil(bmax.y);
  m_maxHSlice = qCeil(bmax.x);

  updateVoxelsForRaycast();

  updateFilledBoxes();
}

void
Viewer::updateVoxelsForRaycast()
{
  m_voxels.clear();
  
  if (!m_volPtr || !m_maskPtr || m_pointSkip == 0)
    return;

  uchar *lut = Global::lut();

  if (!m_lutTex) glGenTextures(1, &m_lutTex);
  if (!m_tagTex) glGenTextures(1, &m_tagTex);

  qint64 dsz = (m_maxDSlice-m_minDSlice);
  qint64 wsz = (m_maxWSlice-m_minWSlice);
  qint64 hsz = (m_maxHSlice-m_minHSlice);
  qint64 tsz = dsz*wsz*hsz*Global::bytesPerVoxel();

//  if ((m_vsize-Vec(hsz, wsz, dsz)).squaredNorm() > 1)
    {
      m_sslevel = 1;
      while (tsz/1024.0/1024.0 > m_memSize ||
	     dsz > m_max3DTexSize ||
	     wsz > m_max3DTexSize ||
	     hsz > m_max3DTexSize)
	{
	  m_sslevel++;
	  dsz = (m_maxDSlice-m_minDSlice)/m_sslevel;
	  wsz = (m_maxWSlice-m_minWSlice)/m_sslevel;
	  hsz = (m_maxHSlice-m_minHSlice)/m_sslevel;
	  
	  if (dsz*m_sslevel < m_maxDSlice-m_minDSlice) dsz++;
	  if (wsz*m_sslevel < m_maxWSlice-m_minWSlice) wsz++;
	  if (hsz*m_sslevel < m_maxHSlice-m_minHSlice) hsz++;
	  
	  tsz = dsz*wsz*hsz*Global::bytesPerVoxel();
	}

      //-------------------------
      m_sslevel = QInputDialog::getInt(this,
				       "Subsampling Level",
				       "Subsampling Level",
				       m_sslevel, m_sslevel, 5, 1);
      
      dsz = (m_maxDSlice-m_minDSlice)/m_sslevel;
      wsz = (m_maxWSlice-m_minWSlice)/m_sslevel;
      hsz = (m_maxHSlice-m_minHSlice)/m_sslevel;
      if (dsz*m_sslevel < m_maxDSlice-m_minDSlice) dsz++;
      if (wsz*m_sslevel < m_maxWSlice-m_minWSlice) wsz++;
      if (hsz*m_sslevel < m_maxHSlice-m_minHSlice) hsz++;
      tsz = dsz*wsz*hsz*Global::bytesPerVoxel();      
      //-------------------------
    }
  
  m_corner = Vec(m_minHSlice, m_minWSlice, m_minDSlice);
  m_vsize = Vec(hsz, wsz, dsz);
  
  createRaycastShader();

  uchar *voxelVol = new uchar[tsz];


  QProgressDialog progress("Updating voxel structure",
			   QString(),
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);
  //----------------------------
  // load data volume
  progress.setValue(20);
  qApp->processEvents();
  if (Global::bytesPerVoxel() == 1)
    {
      int i = 0;
      for(int d=m_minDSlice; d<m_maxDSlice; d+=m_sslevel)
	for(int w=m_minWSlice; w<m_maxWSlice; w+=m_sslevel)
	  for(int h=m_minHSlice; h<m_maxHSlice; h+=m_sslevel)
	    {
	      voxelVol[i] = m_volPtr[d*m_width*m_height + w*m_height + h];
	      i++;
	    }
    }
  else
    {
      int i = 0;
      for(int d=m_minDSlice; d<m_maxDSlice; d+=m_sslevel)
	for(int w=m_minWSlice; w<m_maxWSlice; w+=m_sslevel)
	  for(int h=m_minHSlice; h<m_maxHSlice; h+=m_sslevel)
	    {
	      ((ushort*)voxelVol)[i] = m_volPtrUS[d*m_width*m_height + w*m_height + h];
	      i++;
	    }
    }
  progress.setValue(50);
  qApp->processEvents();

  if (!m_dataTex) glGenTextures(1, &m_dataTex);
  glActiveTexture(GL_TEXTURE2);
  glEnable(GL_TEXTURE_3D);
  glBindTexture(GL_TEXTURE_3D, m_dataTex);	 
  glTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
  glTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
  glTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE); 
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  progress.setValue(70);
  if (Global::bytesPerVoxel() == 1)
    glTexImage3D(GL_TEXTURE_3D,
		 0, // single resolution
		 1,
		 hsz, wsz, dsz,
		 0, // no border
		 GL_LUMINANCE,
		 GL_UNSIGNED_BYTE,
		 voxelVol);
  else
    glTexImage3D(GL_TEXTURE_3D,
		 0, // single resolution
		 1,
		 hsz, wsz, dsz,
		 0, // no border
		 GL_LUMINANCE,
		 GL_UNSIGNED_SHORT,
		 voxelVol);
  glDisable(GL_TEXTURE_3D);
  //----------------------------


  if (!m_maskTex) glGenTextures(1, &m_maskTex);

  //----------------------------
  // load mask volume
  if (m_useMask)
    {
      progress.setValue(60);
      qApp->processEvents();
      int i = 0;
      for(int d=m_minDSlice; d<m_maxDSlice; d+=m_sslevel)
	for(int w=m_minWSlice; w<m_maxWSlice; w+=m_sslevel)
	  for(int h=m_minHSlice; h<m_maxHSlice; h+=m_sslevel)
	    {
	      voxelVol[i] = m_maskPtr[d*m_width*m_height + w*m_height + h];
	      i++;
	    }
      progress.setValue(80);
      qApp->processEvents();
      
      //if (!m_maskTex) glGenTextures(1, &m_maskTex);
      glActiveTexture(GL_TEXTURE4);
      glEnable(GL_TEXTURE_3D);
      glBindTexture(GL_TEXTURE_3D, m_maskTex);	 
      glTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
      glTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
      glTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE); 
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      progress.setValue(70);
      glTexImage3D(GL_TEXTURE_3D,
		   0, // single resolution
		   1,
		   hsz, wsz, dsz,
		   0, // no border
		   GL_LUMINANCE,
		   GL_UNSIGNED_BYTE,
		   voxelVol);
      glDisable(GL_TEXTURE_3D);
    }
  else
    {
      if (m_maskTex) glDeleteTextures(1, &m_maskTex);
      m_maskTex = 0;
    }
  //----------------------------


  delete [] voxelVol;
  
  progress.setValue(100);

  update();
}


void
Viewer::drawMMDCurve()
{
  if (!m_Dcg) return;

  QList<int> cgkeys = m_Dcg->uniqueKeys();
  for(int i=0; i<cgkeys.count(); i++)
    {
      QList<Curve*> curves = m_Dcg->values(cgkeys[i]);
      for (int j=0; j<curves.count(); j++)
	drawCurve(0, curves[j], cgkeys[i]);
    }
  glLineWidth(1);
}

void
Viewer::drawMMWCurve()
{
  if (!m_Wcg) return;

  QList<int> cgkeys = m_Wcg->uniqueKeys();
  for(int i=0; i<cgkeys.count(); i++)
    {
      QList<Curve*> curves = m_Wcg->values(cgkeys[i]);
      for (int j=0; j<curves.count(); j++)
	drawCurve(1, curves[j], cgkeys[i]);
    }
  glLineWidth(1);
}

void
Viewer::drawMMHCurve()
{
  if (!m_Hcg) return;

  QList<int> cgkeys = m_Hcg->uniqueKeys();
  for(int i=0; i<cgkeys.count(); i++)
    {
      QList<Curve*> curves = m_Hcg->values(cgkeys[i]);
      for (int j=0; j<curves.count(); j++)
	drawCurve(2, curves[j], cgkeys[i]);
    }
  glLineWidth(1);
}

void
Viewer::drawLMDCurve()
{
  if (!m_Dmcg) return;

  for(int i=0; i<m_Dmcg->count(); i++)
    {
      QMap<int, Curve> mcg = m_Dmcg->at(i);
      QList<int> cgkeys = mcg.keys();
      for(int j=0; j<cgkeys.count(); j++)
	{
	  Curve c = mcg.value(cgkeys[j]);
	  drawCurve(0, &c, cgkeys[j]);
	}
    }
  glLineWidth(1);
}

void
Viewer::drawLMWCurve()
{
  if (!m_Wmcg) return;

  for(int i=0; i<m_Wmcg->count(); i++)
    {
      QMap<int, Curve> mcg = m_Wmcg->at(i);
      QList<int> cgkeys = mcg.keys();
      for(int j=0; j<cgkeys.count(); j++)
	{
	  Curve c = mcg.value(cgkeys[j]);
	  drawCurve(1, &c, cgkeys[j]);
	}
    }
  glLineWidth(1);
}

void
Viewer::drawLMHCurve()
{
  if (!m_Hmcg) return;

  for(int i=0; i<m_Hmcg->count(); i++)
    {
      QMap<int, Curve> mcg = m_Hmcg->at(i);
      QList<int> cgkeys = mcg.keys();
      for(int j=0; j<cgkeys.count(); j++)
	{
	  Curve c = mcg.value(cgkeys[j]);
	  drawCurve(2, &c, cgkeys[j]);
	}
    }
  glLineWidth(1);
}


void
Viewer::drawCurve(int type, Curve *c, int slc)
{
  int tag = c->tag;
  if (m_curveTags.count() == 0 ||
      m_curveTags[0] == -1 ||
      m_curveTags.contains(tag))
    {
      float r = Global::tagColors()[4*tag+0]*1.0/255.0;
      float g = Global::tagColors()[4*tag+1]*1.0/255.0;
      float b = Global::tagColors()[4*tag+2]*1.0/255.0;
      glColor3f(r,g,b);
      glLineWidth(c->thickness);
      if (type == 0)
	{
	  glBegin(GL_LINE_STRIP);
	  for(int k=0; k<c->pts.count(); k++)
	    glVertex3f(c->pts[k].x(), c->pts[k].y(), slc);
	  if (c->closed)
	    glVertex3f(c->pts[0].x(), c->pts[0].y(), slc);
	  glEnd();
	}
      else if (type == 1)
	{
	  glBegin(GL_LINE_STRIP);
	  for(int k=0; k<c->pts.count(); k++)
	    glVertex3f(c->pts[k].x(), slc, c->pts[k].y());
	  if (c->closed)
	    glVertex3f(c->pts[0].x(), slc, c->pts[0].y());
	  glEnd();
	}
      else if (type == 2)
	{
	  glBegin(GL_LINE_STRIP);
	  for(int k=0; k<c->pts.count(); k++)
	    glVertex3f(slc, c->pts[k].x(), c->pts[k].y());
	  if (c->closed)
	    glVertex3f(slc, c->pts[0].x(), c->pts[0].y());
	  glEnd();
	}
    }
}

void
Viewer::drawSWDCurve()
{
  if (!m_Dswcg) return;

  for(int ix=0; ix<m_Dswcg->count(); ix++)
    {
      QMultiMap<int, Curve*> mcg = m_Dswcg->at(ix);

      QList<int> cgkeys = mcg.uniqueKeys();
      for(int i=0; i<cgkeys.count(); i++)
	{
	  QList<Curve*> curves = mcg.values(cgkeys[i]);
	  for (int j=0; j<curves.count(); j++)
	    drawCurve(0, curves[j], cgkeys[i]);
	}
    }

  glLineWidth(1);
}

void
Viewer::drawSWWCurve()
{
  if (!m_Wswcg) return;

  for(int ix=0; ix<m_Wswcg->count(); ix++)
    {
      QMultiMap<int, Curve*> mcg = m_Wswcg->at(ix);

      QList<int> cgkeys = mcg.uniqueKeys();
      for(int i=0; i<cgkeys.count(); i++)
	{
	  QList<Curve*> curves = mcg.values(cgkeys[i]);
	  for (int j=0; j<curves.count(); j++)
	    drawCurve(1, curves[j], cgkeys[i]);
	}
    }

  glLineWidth(1);
}

void
Viewer::drawSWHCurve()
{
  if (!m_Hswcg) return;

  for(int ix=0; ix<m_Hswcg->count(); ix++)
    {
      QMultiMap<int, Curve*> mcg = m_Hswcg->at(ix);

      QList<int> cgkeys = mcg.uniqueKeys();
      for(int i=0; i<cgkeys.count(); i++)
	{
	  QList<Curve*> curves = mcg.values(cgkeys[i]);
	  for (int j=0; j<curves.count(); j++)
	    drawCurve(2, curves[j], cgkeys[i]);
	}
    }

  glLineWidth(1);
}

Vec
Viewer::getHit(QPoint scr, bool &found)
{
  Vec target;
  found = false;

  target = pointUnderPixel_RC(scr, found);

  if (!found)
    target = Vec(-1,-1,-1);

  return target;
}

void
Viewer::getHit(QMouseEvent *event)
{
  bool found;
  QPoint scr = event->pos();

  m_target = getHit(scr, found);

  //-------------------------
  // rotation pivot
  if (event->modifiers() & Qt::ControlModifier)
    {
      if (found) // move rotation pivot
	{
	  Vec voxelScaling = Global::relativeVoxelScaling();
	  Vec pivot = VECPRODUCT(m_target, voxelScaling);
	  camera()->setRevolveAroundPoint(pivot);
	  QMessageBox::information(0, "", "Rotation pivot changed");
	}
      else // reset rotation pivot
	{
	  camera()->setRevolveAroundPoint(sceneCenter());
	  QMessageBox::information(0, "", "Rotation pivot reset to scene center");
	}
      return;
    }
  //-------------------------
  

  //-------------------------
  // paint
  if (found)
    {
      if (!m_useMask)
	{
	  QMessageBox::information(0, "Error", "Switch on Load Tags before applying the operation.");
	  m_paintHit = false;
	  m_target = Vec(-1,-1,-1);
	  return;
	}

      int d, w, h;
      d = m_target.z;
      w = m_target.y;
      h = m_target.x;

      if (d < 0 || d > m_depth-1 &&
	  w < 0 || w > m_width-1 &&
	  h < 0 || h > m_height-1)
	return;
	
      int b = 0;
      if (event->buttons() == Qt::LeftButton) b = 1;
      else if (event->buttons() == Qt::RightButton) b = 2;
      else if (event->buttons() == Qt::MiddleButton) b = 3;
      
      if (m_paintHit)
	{
	  Vec bmin, bmax;
	  m_boundingBox.bounds(bmin, bmax);
	  Vec voxelScaling = Global::relativeVoxelScaling();
	  bmin = VECDIVIDE(bmin, voxelScaling);
	  bmax = VECDIVIDE(bmax, voxelScaling);
	  emit paint3D(bmin, bmax,
		       d, w, h, b,
		       Global::tag(),
		       m_UI->paintOnlyConnected->isChecked());
	}
    }
}

Vec
Viewer::pointUnderPixel_RC(QPoint scr, bool& found)
{
  found = false;
      
  Vec box[8];
  box[0] = Vec(m_minHSlice, m_minWSlice, m_minDSlice);
  box[1] = Vec(m_minHSlice, m_minWSlice, m_maxDSlice);
  box[2] = Vec(m_minHSlice, m_maxWSlice, m_maxDSlice);
  box[3] = Vec(m_minHSlice, m_maxWSlice, m_minDSlice);
  box[4] = Vec(m_maxHSlice, m_minWSlice, m_minDSlice);
  box[5] = Vec(m_maxHSlice, m_minWSlice, m_maxDSlice);
  box[6] = Vec(m_maxHSlice, m_maxWSlice, m_maxDSlice);
  box[7] = Vec(m_maxHSlice, m_maxWSlice, m_minDSlice);

  //--------------------------------
  Vec eyepos = camera()->position();
  Vec viewDir = camera()->viewDirection();
  float minZ = 1000000;
  float maxZ = -1000000;
  for(int b=0; b<8; b++)
    {
      float zv = (box[b]-eyepos)*viewDir;
      minZ = qMin(minZ, zv);
      maxZ = qMax(maxZ, zv);
    }
  //--------------------------------


  int sw = camera()->screenWidth();
  int sh = camera()->screenHeight();  

  Vec pos;
  int cx = scr.x();
  int cy = scr.y();
  GLfloat depth = 0;
  
  glEnable(GL_SCISSOR_TEST);
  glScissor(cx, sh-cy, 1, 1);
  volumeRaycast(minZ, maxZ, true); // run only one part of raycast process
  glDisable(GL_SCISSOR_TEST);

  glBindFramebuffer(GL_FRAMEBUFFER, m_eBuffer);
  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_ebTex[0]);

  GLfloat d4[4];
  glReadPixels(cx, sh-cy, 1, 1, GL_RGBA, GL_FLOAT, &d4);
  if (d4[3] > 0.0)
    {
      pos = Vec(d4[0], d4[1], d4[2]);
      Vec vsz = m_sslevel*m_vsize;
      pos = m_corner + VECPRODUCT(pos, vsz);
      //-----------
      {
	int v;
	if (Global::bytesPerVoxel() == 1)
	  v = m_volPtr[(int)pos.z*m_width*m_height + (int)pos.y*m_height + (int)pos.x];
	else
	  v = m_volPtrUS[(int)pos.z*m_width*m_height + (int)pos.y*m_height + (int)pos.x];

	int tg = m_maskPtr[(int)pos.z*m_width*m_height + (int)pos.y*m_height + (int)pos.x];

	uchar *lut = Global::lut();

	int a = Global::tagColors()[4*tg+3];

	if (lut[4*v+3]*a == 0) // if we have hit transparent region go a voxel deep
	  {
	    pos += 2*camera()->viewDirection();
	    int ax = pos.x;
	    int ay = pos.y;
	    int az = pos.z;
	    ax = qBound(m_minHSlice, ax, m_maxHSlice);
	    ay = qBound(m_minWSlice, ay, m_maxWSlice);
	    az = qBound(m_minDSlice, az, m_maxDSlice);
	    pos = Vec(ax, ay, az);
	  }
      }
      //-----------

      found = true;
    }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  return pos;
}

void
Viewer::mousePressEvent(QMouseEvent *event)
{
  m_paintHit = false;
  m_target = Vec(-1,-1,-1);

  if (m_sketchPadMode)
    {
      m_poly << event->pos();
      update();
      return;
    }

  if (event->modifiers() & Qt::ControlModifier)
    {
      // change rotation pivot
      if (event->modifiers() & Qt::ControlModifier &&
	  event->buttons() == Qt::RightButton)
	{
	  getHit(event);
	  return;
	}

      // change orthogonal slices
      int d, w, h;
      bool gothit = getCoordUnderPointer(d, w, h);
      if (gothit)
	{
	  m_dslice = d;
	  m_wslice = w;
	  m_hslice = h;
	  emit changeImageSlice(d, w, h);
	  return;
	}
    }

  if (event->modifiers() & Qt::ShiftModifier)
    {
      m_paintHit = true;
      emit paint3DStart();
      getHit(event);
      return;
    }

  QGLViewer::mousePressEvent(event);
}


void
Viewer::mouseReleaseEvent(QMouseEvent *event)
{
  m_dragMode = false;

  if (m_sketchPadMode)
    return;
  
  if (m_paintHit)
    emit paint3DEnd();
  
  m_paintHit = false;
  m_target = Vec(-1,-1,-1);

  QGLViewer::mouseReleaseEvent(event);
}


void
Viewer::mouseMoveEvent(QMouseEvent *event)
{
  m_dragMode = event->buttons() != Qt::NoButton;

  m_target = Vec(-1,-1,-1);
  
  if (m_sketchPadMode && m_dragMode)
    {
      m_poly << event->pos();
      update();
      return;
    }
      
  if (m_paintHit)
    {
      getHit(event);
      return;
    }

  QGLViewer::mouseMoveEvent(event);
}

void
Viewer::drawClip()
{
  updateClipVoxels();

  m_clipPlanes->draw(this, false);

  uchar *lut = Global::lut();

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_POINT_SMOOTH);

  Vec ptpos = camera()->sceneCenter();
  float pglr = camera()->pixelGLRatio(ptpos);
  int ptsz = m_pointSize/pglr;
  glPointSize(ptsz);
  int nv = m_clipVoxels.count()/4;
  for(int i=0; i<nv; i++)
    {
      int d = m_clipVoxels[4*i+0];
      int w = m_clipVoxels[4*i+1];
      int h = m_clipVoxels[4*i+2];
      int v = m_clipVoxels[4*i+3];

      int t = m_maskPtr[d*m_width*m_height + w*m_height + h];

      glBegin(GL_POINTS);
      float r = lut[4*v+2]*1.0/255.0;
      float g = lut[4*v+1]*1.0/255.0;
      float b = lut[4*v+0]*1.0/255.0;

      float rt = r;
      float gt = g;
      float bt = b;
      if (t > 0)
	{
	  rt = Global::tagColors()[4*t+0]*1.0/255.0;
	  gt = Global::tagColors()[4*t+1]*1.0/255.0;
	  bt = Global::tagColors()[4*t+2]*1.0/255.0;
	}

      r = r*0.5 + rt*0.5;
      g = g*0.5 + gt*0.5;
      b = b*0.5 + bt*0.5;

      glColor3f(r,g,b);
      glVertex3f(h, w, d);
      glEnd();
    }

  glDisable(GL_POINT_SMOOTH);
  glBlendFunc(GL_NONE, GL_NONE);
  glDisable(GL_BLEND);
}


void
Viewer::boundingBoxChanged()
{
  if (m_depth == 0) // we don't have a volume yet
    return;
    
  //----
  Vec bmin, bmax;
  m_boundingBox.bounds(bmin, bmax);
  Vec voxelScaling = Global::relativeVoxelScaling();
  bmin = VECDIVIDE(bmin, voxelScaling);
  bmax = VECDIVIDE(bmax, voxelScaling);
  int minD = bmin.z;
  int minW = bmin.y;
  int minH = bmin.x;
  int maxD = qCeil(bmax.z); // because we are getting it as float
  int maxW = qCeil(bmax.y);
  int maxH = qCeil(bmax.x);

  if (minD == m_cminD &&
      maxD == m_cmaxD &&
      minW == m_cminW &&
      maxW == m_cmaxW &&
      minH == m_cminH &&
      maxH == m_cmaxH)
    return; // no change

  bmin = StaticFunctions::maxVec(bmin, Vec(m_minHSlice, m_minWSlice, m_minDSlice));
  bmax = StaticFunctions::minVec(bmax, Vec(m_maxHSlice, m_maxWSlice, m_maxDSlice));
  m_cminD = bmin.z;
  m_cminW = bmin.y;
  m_cminH = bmin.x;
  m_cmaxD = qCeil(bmax.z); // because we are getting it as float
  m_cmaxW = qCeil(bmax.y);
  m_cmaxH = qCeil(bmax.x);
  //----

//  generateBoxes();
//
//  loadAllBoxesToVBO();  
//
  updateFilledBoxes();
}

void
Viewer::generateBoxes()
{
  QProgressDialog progress("Updating box structure",
			   QString(),
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);

  m_boxSoup.clear();
  
  int faces[] = {1, 5, 7, 3,
		 0, 2, 6, 4,
		 0, 1, 3, 2,
		 7, 5, 4, 6,
		 2, 3, 7, 6,
		 1, 0, 4, 5};
	  
  Vec voxelScaling = Global::relativeVoxelScaling();


  Vec bminO, bmaxO;
  m_boundingBox.bounds(bminO, bmaxO);

  bminO = VECDIVIDE(bminO, voxelScaling);
  bmaxO = VECDIVIDE(bmaxO, voxelScaling);

  bminO = StaticFunctions::maxVec(bminO, Vec(m_minHSlice, m_minWSlice, m_minDSlice));
  bmaxO = StaticFunctions::minVec(bmaxO, Vec(m_maxHSlice, m_maxWSlice, m_maxDSlice));

  m_cminD = bminO.z;
  m_cminW = bminO.y;
  m_cminH = bminO.x;
  m_cmaxD = qCeil(bmaxO.z); // because we are getting it as float
  m_cmaxW = qCeil(bmaxO.y);
  m_cmaxH = qCeil(bmaxO.x);


  for(int k=0; k<m_dbox; k++)
    {
      progress.setValue(100*(float)k/(float)m_dbox);
      qApp->processEvents();

      for(int j=0; j<m_wbox; j++)
	for(int i=0; i<m_hbox; i++)
	  {
	    int idx = k*m_wbox*m_hbox+j*m_hbox+i;
	    
	    Vec bmin, bmax;
	    bmin = Vec(qMax(i*m_boxSize, (int)bminO.x),
		       qMax(j*m_boxSize, (int)bminO.y),
		       qMax(k*m_boxSize, (int)bminO.z));
	    
	    bmax = Vec(qMin((i+1)*m_boxSize, (int)bmaxO.x),
		       qMin((j+1)*m_boxSize, (int)bmaxO.y),
		       qMin((k+1)*m_boxSize, (int)bmaxO.z));
	    
	    Vec box[8];
	    box[0] = Vec(bmin.x,bmin.y,bmin.z);
	    box[1] = Vec(bmin.x,bmin.y,bmax.z);
	    box[2] = Vec(bmin.x,bmax.y,bmin.z);
	    box[3] = Vec(bmin.x,bmax.y,bmax.z);
	    box[4] = Vec(bmax.x,bmin.y,bmin.z);
	    box[5] = Vec(bmax.x,bmin.y,bmax.z);
	    box[6] = Vec(bmax.x,bmax.y,bmin.z);
	    box[7] = Vec(bmax.x,bmax.y,bmax.z);
	    
	    float xmin, xmax, ymin, ymax, zmin, zmax;
	    xmin = (bmin.x-m_minHSlice)/(m_maxHSlice-m_minHSlice);
	    xmax = (bmax.x-m_minHSlice)/(m_maxHSlice-m_minHSlice);
	    ymin = (bmin.y-m_minWSlice)/(m_maxWSlice-m_minWSlice);
	    ymax = (bmax.y-m_minWSlice)/(m_maxWSlice-m_minWSlice);
	    zmin = (bmin.z-m_minDSlice)/(m_maxDSlice-m_minDSlice);
	    zmax = (bmax.z-m_minDSlice)/(m_maxDSlice-m_minDSlice);
	    
	    for(int b=0; b<8; b++)
	      box[b] = VECPRODUCT(box[b], voxelScaling);
	      
	    QList<Vec> vboSoup;
	    vboSoup.reserve(36);
	    for(int v=0; v<6; v++)
	      {
		Vec poly[4];
		for(int w=0; w<4; w++)
		  {
		    int idx = faces[4*v+w];
		    poly[w] = box[idx];
		  }
		
		// put in vboSoup
		for(int t=0; t<2; t++)
		  {
		    vboSoup << poly[0];
		    vboSoup << poly[t+1];		    
		    vboSoup << poly[t+2];
		  }
	      }
	    m_boxSoup << vboSoup;
	  } //i j
    } // k
  
  progress.setValue(100);

  m_numBoxes = m_boxSoup.count();
  m_boxSoup.squeeze();
}

void
Viewer::volumeRaycast(float minZ, float maxZ, bool firstPartOnly)
{
  Vec eyepos = camera()->position();
  Vec viewDir = camera()->viewDirection();
  Vec subvolcorner = Vec(m_minHSlice, m_minWSlice, m_minDSlice);
  Vec subvolsize = Vec(m_maxHSlice-m_minHSlice+1,
		       m_maxWSlice-m_minWSlice+1,
		       m_maxDSlice-m_minDSlice+1);

  glClearDepth(0);
  glClearColor(0, 0, 0, 0);

  glBindFramebuffer(GL_FRAMEBUFFER_EXT, m_slcBuffer);
  for(int fbn=0; fbn<2; fbn++)
    {
      glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
			     GL_COLOR_ATTACHMENT0_EXT,
			     GL_TEXTURE_RECTANGLE_ARB,
			     m_slcTex[fbn],
			     0);
      glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

  glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);


  //----------------------------
  // create exit points
  glBindFramebuffer(GL_FRAMEBUFFER_EXT, m_slcBuffer);
  glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
			 GL_COLOR_ATTACHMENT0_EXT,
			 GL_TEXTURE_RECTANGLE_ARB,
			 m_slcTex[1],
			 0);
  glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);  
  glDepthFunc(GL_GEQUAL);
  drawVBOBox(GL_FRONT);
  glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
  //----------------------------
  
  //----------------------------
  // create entry points
  glBindFramebuffer(GL_FRAMEBUFFER_EXT, m_slcBuffer);
  glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
			 GL_COLOR_ATTACHMENT0_EXT,
			 GL_TEXTURE_RECTANGLE_ARB,
			 m_slcTex[0],
			 0);
  glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
  glClearDepth(1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glDepthFunc(GL_LEQUAL);
  drawVBOBox(GL_BACK);
  glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
  //----------------------------


  //----------------------------
  glBindFramebuffer(GL_FRAMEBUFFER_EXT, m_eBuffer);
  
  glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
			 GL_COLOR_ATTACHMENT0_EXT,
			 GL_TEXTURE_RECTANGLE_ARB,
			 m_ebTex[0],
			 0);
  
  glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
  
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  //----------------------------


  float stepsize = m_stillStep;

  stepsize /= qMax(m_vsize.x,qMax(m_vsize.y,m_vsize.z));

  Vec voxelScaling = Global::relativeVoxelScaling();

  glUseProgramObjectARB(m_rcShader);
  glUniform1iARB(m_rcParm[0], 2); // dataTex
  glUniform1iARB(m_rcParm[1], 3); // lutTex
  glUniform1iARB(m_rcParm[2], 1); // slcTex[1] - contains exit coordinates
  glUniform1fARB(m_rcParm[3], stepsize); // stepSize
  glUniform3fARB(m_rcParm[4], eyepos.x, eyepos.y, eyepos.z); // eyepos
  glUniform3fARB(m_rcParm[5], viewDir.x, viewDir.y, viewDir.z); // viewDir
  glUniform3fARB(m_rcParm[6], subvolcorner.x, subvolcorner.y, subvolcorner.z);
  //glUniform3fARB(m_rcParm[7], m_vsize.x, m_vsize.y, m_vsize.z);
  glUniform3fARB(m_rcParm[7], m_sslevel*m_vsize.x, m_sslevel*m_vsize.y, m_sslevel*m_vsize.z);
  glUniform3fARB(m_rcParm[20], voxelScaling.x, voxelScaling.y, voxelScaling.z);
  glUniform1iARB(m_rcParm[10],4); // maskTex
  glUniform1iARB(m_rcParm[11],firstPartOnly); // save voxel coordinates
  glUniform1iARB(m_rcParm[12],m_skipLayers); // skip first layers
  glUniform1iARB(m_rcParm[13],5); // tagTex
  glUniform1iARB(m_rcParm[14],6); // slcTex[0] - contains entry coordinates
  glUniform3fARB(m_rcParm[15], m_bgColor.x/255,
		               m_bgColor.y/255,
		               m_bgColor.z/255);
  glUniform1iARB(m_rcParm[16],m_skipVoxels); // skip first voxels

  glUniform1fARB(m_rcParm[21],m_minGrad-0.0001); // reduce for step function
  glUniform1fARB(m_rcParm[22],m_maxGrad+0.0001); // increase for step function

  { // apply clip planes to modify entry and exit points
    QList<Vec> cPos =  m_clipPlanes->positions();
    QList<Vec> cNorm = m_clipPlanes->normals();
    cPos << camera()->position()+50*camera()->viewDirection();
    cNorm << -camera()->viewDirection();

    //--------------------------
    // 6 planes of bounding box
    {
      Vec bmin, bmax;
      m_boundingBox.bounds(bmin, bmax);
      
      bmin = VECDIVIDE(bmin, voxelScaling);
      bmax = VECDIVIDE(bmax, voxelScaling);

      bmin = StaticFunctions::maxVec(bmin, Vec(m_minHSlice, m_minWSlice, m_minDSlice));     
      { cPos << bmin; cNorm << Vec(-1,0,0); }
      { cPos << bmin; cNorm << Vec(0,-1,0); }
      { cPos << bmin; cNorm << Vec(0,0,-1); }

      bmax = StaticFunctions::minVec(bmax, Vec(m_maxHSlice, m_maxWSlice, m_maxDSlice));
      { cPos << bmax; cNorm << Vec(1,0,0); }
      { cPos << bmax; cNorm << Vec(0,1,0); }
      { cPos << bmax; cNorm << Vec(0,0,1); }
    }
    //--------------------------

    int nclip = cPos.count();
    float cpos[100];
    float cnormal[100];
    for(int c=0; c<nclip; c++)
      {
	cpos[3*c+0] = (cPos[c].x-m_minHSlice)/(m_maxHSlice-m_minHSlice);
	cpos[3*c+1] = (cPos[c].y-m_minWSlice)/(m_maxWSlice-m_minWSlice);
	cpos[3*c+2] = (cPos[c].z-m_minDSlice)/(m_maxDSlice-m_minDSlice);
      }
    for(int c=0; c<nclip; c++)
      {
	cnormal[3*c+0] = -cNorm[c].x;
	cnormal[3*c+1] = -cNorm[c].y;
	cnormal[3*c+2] = -cNorm[c].z;
      }
    glUniform1i(m_rcParm[17], nclip); // clipplanes
    glUniform3fv(m_rcParm[18], nclip, cpos); // clipplanes
    glUniform3fv(m_rcParm[19], nclip, cnormal); // clipplanes
  }

  
  glActiveTexture(GL_TEXTURE1);
  glEnable(GL_TEXTURE_RECTANGLE_ARB);
  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_slcTex[1]);

  glActiveTexture(GL_TEXTURE6);
  glEnable(GL_TEXTURE_RECTANGLE_ARB);
  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_slcTex[0]);

  glActiveTexture(GL_TEXTURE2);
  glEnable(GL_TEXTURE_3D);
  glBindTexture(GL_TEXTURE_3D, m_dataTex);

  if (firstPartOnly || m_exactCoord)
    {
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
  else
    {
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }


  glActiveTexture(GL_TEXTURE4);
  glEnable(GL_TEXTURE_3D);
  glBindTexture(GL_TEXTURE_3D, m_maskTex);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  uchar *lut = Global::lut();
  glActiveTexture(GL_TEXTURE3);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, m_lutTex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D,
	       0, // single resolution
	       GL_RGBA,
	       256, Global::lutSize()*256, // width, height
	       0, // no border
	       GL_BGRA,
	       GL_UNSIGNED_BYTE,
	       lut);

  uchar *tagColors = Global::tagColors();
  glActiveTexture(GL_TEXTURE5);
  glEnable(GL_TEXTURE_1D);
  glBindTexture(GL_TEXTURE_1D, m_tagTex);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER,GL_NEAREST);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER,GL_NEAREST);
  glTexImage1D(GL_TEXTURE_1D,
	       0, // single resolution
	       GL_RGBA,
	       256,
	       0, // no border
	       GL_RGBA,
	       GL_UNSIGNED_BYTE,
	       tagColors);


  int wd = camera()->screenWidth();
  int ht = camera()->screenHeight();
  StaticFunctions::pushOrthoView(0, 0, wd, ht);
  StaticFunctions::drawQuad(0, 0, wd, ht, 1);
  StaticFunctions::popOrthoView();
  //----------------------------

  glActiveTexture(GL_TEXTURE4);
  glDisable(GL_TEXTURE_3D);

  glActiveTexture(GL_TEXTURE6);
  glDisable(GL_TEXTURE_RECTANGLE_ARB);

  if (firstPartOnly)
    {
      glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
      glUseProgramObjectARB(0);
      
      glActiveTexture(GL_TEXTURE5);
      glDisable(GL_TEXTURE_1D);

      glActiveTexture(GL_TEXTURE3);
      glDisable(GL_TEXTURE_2D);
      
      glActiveTexture(GL_TEXTURE2);
      glDisable(GL_TEXTURE_3D);
      
      glActiveTexture(GL_TEXTURE1);
      glDisable(GL_TEXTURE_RECTANGLE_ARB);
      return;
    }
    
  
  //--------------------------------
  glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
  glUseProgram(m_eeShader);
  
  glActiveTexture(GL_TEXTURE1);
  glEnable(GL_TEXTURE_RECTANGLE_ARB);
  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_ebTex[0]);
  
//  uchar *tagColors = Global::tagColors();
  glActiveTexture(GL_TEXTURE5);
  glEnable(GL_TEXTURE_1D);
  glBindTexture(GL_TEXTURE_1D, m_tagTex);
  
  glUniform1f(m_eeParm[1], minZ); // minZ
  glUniform1f(m_eeParm[2], maxZ); // maxZ
  glUniform3f(m_eeParm[3], eyepos.x, eyepos.y, eyepos.z); // eyepos
  glUniform3f(m_eeParm[4], viewDir.x, viewDir.y, viewDir.z); // viewDir
  glUniform1f(m_eeParm[5], m_edge);
  glUniform1i(m_eeParm[6], 5); // tagtex
  glUniform1i(m_eeParm[7], 3); // luttex
  glUniform1i(m_eeParm[8], 1); // pos, val, tag tex
  glUniform3f(m_eeParm[9], m_amb, m_diff, m_spec); // lightparm
  glUniform1i(m_eeParm[10], m_shadow); // shadows
  glUniform3f(m_eeParm[11], m_shadowColor.x/255,
	      m_shadowColor.y/255,
	      m_shadowColor.z/255);
  glUniform3f(m_eeParm[12], m_edgeColor.x/255,
	      m_edgeColor.y/255,
	      m_edgeColor.z/255);
  glUniform3f(m_eeParm[13], m_bgColor.x/255,
	      m_bgColor.y/255,
	      m_bgColor.z/255);
  glUniform2f(m_eeParm[14], m_shdX, -m_shdY);
  
  StaticFunctions::pushOrthoView(0, 0, wd, ht);
  StaticFunctions::drawQuad(0, 0, wd, ht, 1);
  StaticFunctions::popOrthoView();
  //----------------------------

  
  glUseProgram(0);

  glActiveTexture(GL_TEXTURE6);
  glDisable(GL_TEXTURE_RECTANGLE_ARB);

  glActiveTexture(GL_TEXTURE5);
  glDisable(GL_TEXTURE_1D);

  glActiveTexture(GL_TEXTURE3);
  glDisable(GL_TEXTURE_2D);

  glActiveTexture(GL_TEXTURE2);
  glDisable(GL_TEXTURE_3D);

  glActiveTexture(GL_TEXTURE1);
  glDisable(GL_TEXTURE_RECTANGLE_ARB);
}

void
Viewer::uploadMask(int dst, int wst, int hst, int ded, int wed, int hed)
{
//  QProgressDialog progress("Updating the mask",
//			   QString(),
//			   0, 100,
//			   0);
//  progress.setMinimumDuration(0);

  int ds = m_minDSlice + m_sslevel*qFloor((dst-m_minDSlice)/m_sslevel);
  int ws = m_minWSlice + m_sslevel*qFloor((wst-m_minWSlice)/m_sslevel);
  int hs = m_minHSlice + m_sslevel*qFloor((hst-m_minHSlice)/m_sslevel);

  int de = m_minDSlice + m_sslevel*qCeil((ded-m_minDSlice)/m_sslevel);
  int we = m_minWSlice + m_sslevel*qCeil((wed-m_minWSlice)/m_sslevel);
  int he = m_minHSlice + m_sslevel*qCeil((hed-m_minHSlice)/m_sslevel);

  ds = qBound(m_minDSlice, ds, m_maxDSlice);
  ws = qBound(m_minWSlice, ws, m_maxWSlice);
  hs = qBound(m_minHSlice, hs, m_maxHSlice);

  de = qBound(m_minDSlice, de, m_maxDSlice);
  we = qBound(m_minWSlice, we, m_maxWSlice);
  he = qBound(m_minHSlice, he, m_maxHSlice);

  qint64 dsz = (de-ds)/m_sslevel;
  qint64 wsz = (we-ws)/m_sslevel;
  qint64 hsz = (he-hs)/m_sslevel;
  if (dsz*m_sslevel < de-ds) dsz++;
  if (wsz*m_sslevel < we-ws) wsz++;
  if (hsz*m_sslevel < he-hs) hsz++;
  qint64 tsz = dsz*wsz*hsz;      
  uchar *voxelVol = new uchar[tsz];
  int i = 0;
  for(int d=ds; d<de; d+=m_sslevel)
    {
//      progress.setValue(100*d/de);
//      qApp->processEvents();
      for(int w=ws; w<we; w+=m_sslevel)
      for(int h=hs; h<he; h+=m_sslevel)
	{
	  voxelVol[i] = m_maskPtr[d*m_width*m_height + w*m_height + h];
	  i++;
	}
    }
  
//  progress.setValue(90);
//  qApp->processEvents();

  int doff = (ds-m_minDSlice)/m_sslevel;
  int woff = (ws-m_minWSlice)/m_sslevel;
  int hoff = (hs-m_minHSlice)/m_sslevel;

  glActiveTexture(GL_TEXTURE4);
  glEnable(GL_TEXTURE_3D);
  glBindTexture(GL_TEXTURE_3D, m_maskTex);	 
  glTexSubImage3D(GL_TEXTURE_3D,
		  0, // level
		  hoff, woff, doff, // offset
		  hsz, wsz, dsz,
		  GL_LUMINANCE,
		  GL_UNSIGNED_BYTE,
		  voxelVol);
  glDisable(GL_TEXTURE_3D);

  
  update();

  delete [] voxelVol;

//  progress.setValue(100);
//  qApp->processEvents();
}

void
Viewer::hatch()
{
  if (!m_useMask)
    {
      QMessageBox::information(0, "Error", "Switch on Load Tags before applying the operation.");
      return;
    }

  int d, w, h;
  bool gothit = getCoordUnderPointer(d, w, h);
  if (!gothit) return;


  Vec bmin, bmax;
  m_boundingBox.bounds(bmin, bmax);

  Vec voxelScaling = Global::relativeVoxelScaling();
  bmin = VECDIVIDE(bmin, voxelScaling);
  bmax = VECDIVIDE(bmax, voxelScaling);

  bool ok;
  int ctag = -1;
  ctag = QInputDialog::getInt(0,
			      "Hatch Region",
			      QString("Region will be hatched with current tag value (%1).\nSpecify tag value of connected region (-1 for connected visible region).").arg(Global::tag()),
			      -1, -1, 255, 1,
			      &ok);
  if (!ok)
    return;

  QString text;
  text = QInputDialog::getText(this,
			       "Hatch Region",
			       "Interval and Thickness in terms of number of voxels",
			       QLineEdit::Normal,
			       "20 5",
			       &ok);
  
  int interval = 20;
  int thickness = 5;
  
  if (ok && !text.isEmpty())
    {
      QStringList list = text.split(" ", QString::SkipEmptyParts);
      if (list.count() == 2)
	{
	  interval = list[0].toInt();
	  thickness = list[1].toInt();
	}
      else
	{
	  QMessageBox::information(0, "Error",
				   QString("Wrong number of parameters specified [%1]").arg(text));
	  return;
	}
    }
  
  emit hatchConnectedRegion(d, w, h,
			    bmin, bmax,
			    Global::tag(), ctag,
			    thickness, interval);
}

void
Viewer::regionGrowing(bool sw)
{
  if (!m_useMask)
    {
      QMessageBox::information(0, "Error", "Switch on Load Tags before applying the operation.");
      return;
    }

  int d, w, h;
  bool gothit = getCoordUnderPointer(d, w, h);
  if (!gothit) return;


  Vec bmin, bmax;
  m_boundingBox.bounds(bmin, bmax);

  Vec voxelScaling = Global::relativeVoxelScaling();
  bmin = VECDIVIDE(bmin, voxelScaling);
  bmax = VECDIVIDE(bmax, voxelScaling);

  if (!sw)
    {
      bool ok;
      int ctag = -1;
      ctag = QInputDialog::getInt(0,
				  "Fill",
				  QString("Connected region will be filled with current tag value (%1).\nSpecify tag value of connected region (-1 for connected visible region).").arg(Global::tag()),
				  -1, -1, 255, 1,
				  &ok);
      if (!ok)
	return;

      emit connectedRegion(d, w, h, bmin, bmax, Global::tag(), ctag);
    }
  else
    {
      QStringList dtypes;
      dtypes << "Shrinkwrap";
      dtypes << "Shell";
      dtypes << "Tubes";
      bool ok;
      QString option = QInputDialog::getItem(0,
					     "Shrinkwrap",
					     "Shrinkwrap/Shell/Tubes",
					     dtypes,
					     0,
					     false,
					     &ok);
      if (!ok)
	return;

      
      bool tubes = (option == "Tubes");
      bool shell = (option == "Shell");

      QString mesg = "Shrinkwrap ";
      if (shell)
	mesg = "Generate shell surrounding ";
      if (tubes)
	mesg = "Identify tubes in ";

      int ctag = -1;
      ctag = QInputDialog::getInt(0,
				  "Shrinkwrap/Shell/Tubes",
				  QString("%1 connected region with current tag value (%2).\nSpecify tag value of connected region (-1 for connected visible region).").\
				  arg(mesg).\
				  arg(Global::tag()),
				  -1, -1, 255, 1);

      if (tubes)
	{
	  emit tagTubes(bmin, bmax, Global::tag(),
			false, d, w, h, ctag);
	  return;
	}

      int thickness = 1;
      if (shell)
	thickness = QInputDialog::getInt(0,
					 "Shell thickness",
					 "Shell thickness",
					 1, 1, 50, 1);

      emit shrinkwrap(bmin, bmax, Global::tag(), shell, thickness,
		      false, d, w, h, ctag);

    } // shrinkwrap/shell/tubes
}

void
Viewer::regionDilation(bool allVisible)
{
  if (!m_useMask)
    {
      QMessageBox::information(0, "Error", "Switch on Load Tags before applying the operation.");
      return;
    }

  int d, w, h;
  bool gothit = getCoordUnderPointer(d, w, h);
  if (!gothit) return;

  Vec bmin, bmax;
  m_boundingBox.bounds(bmin, bmax);

  Vec voxelScaling = Global::relativeVoxelScaling();
  bmin = VECDIVIDE(bmin, voxelScaling);
  bmax = VECDIVIDE(bmax, voxelScaling);

  emit dilateConnected(d, w, h, bmin, bmax, Global::tag(), allVisible);
}

void
Viewer::regionErosion()
{
  if (!m_useMask)
    {
      QMessageBox::information(0, "Error", "Switch on Load Tags before applying the operation.");
      return;
    }

  int d, w, h;
  bool gothit = getCoordUnderPointer(d, w, h);
  if (!gothit) return;

  Vec bmin, bmax;
  m_boundingBox.bounds(bmin, bmax);

  Vec voxelScaling = Global::relativeVoxelScaling();
  bmin = VECDIVIDE(bmin, voxelScaling);
  bmax = VECDIVIDE(bmax, voxelScaling);

  emit erodeConnected(d, w, h, bmin, bmax, Global::tag());
}

void
Viewer::tagUsingScreenSketch()
{
  if (!m_useMask)
    {
      QMessageBox::information(0, "Error", "Switch on Load Tags before applying the operation.");
      return;
    }

  Vec bmin, bmax;
  m_boundingBox.bounds(bmin, bmax);

  Vec voxelScaling = Global::relativeVoxelScaling();
  bmin = VECDIVIDE(bmin, voxelScaling);
  bmax = VECDIVIDE(bmax, voxelScaling);

  m_spW = size().width();
  m_spH = size().height();

  if (m_sketchPad) delete [] m_sketchPad;
  m_sketchPad = new uchar[m_spW*m_spH];
  memset(m_sketchPad, 0, m_spW*m_spH);

  QImage pimg = QImage(m_spW, m_spH, QImage::Format_RGB32);
  pimg.fill(0);

  QPainter p(&pimg);
  p.setPen(QPen(Qt::white, 1));
  p.setBrush(Qt::white);
  p.drawPolygon(m_poly);

  QRgb *rgb = (QRgb*)(pimg.bits());
  for(int yp = 0; yp < m_spH; yp ++)
    for(int xp = 0; xp < m_spW; xp ++)
      {
	int rp = xp + yp*m_spW;
	if (qRed(rgb[rp]) > 0)
	  m_sketchPad[rp] = 255;
      }

  emit tagUsingSketchPad(bmin, bmax);
  m_poly.clear();
}

void
Viewer::grabScreenImage()
{
  if (m_screenImageBuffer)
    delete [] m_screenImageBuffer;
  m_screenImageBuffer = new uchar[size().width()*size().height()*4];
  
  glReadBuffer(GL_FRONT);
  glReadPixels(0,
	       0,
	       size().width(),
	       size().height(),
	       GL_RGBA,
	       GL_UNSIGNED_BYTE,
	       m_screenImageBuffer);
}

void
Viewer::drawScreenImage()
{
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho(0, size().width(), 0, size().height(), -1, 1);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
  glRasterPos2i(0,0);
  glDrawPixels(size().width(),
	       size().height(),
	       GL_RGBA,
	       GL_UNSIGNED_BYTE,
	       m_screenImageBuffer);
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();

  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
}

bool
Viewer::getCoordUnderPointer(int &d, int &w, int &h)
{
  Vec target;
  bool found;
  
  QPoint scr = mapFromGlobal(QCursor::pos());
  target = getHit(scr, found);
  if (!found)
    return false;

  d = qCeil(target.z);
  w = qCeil(target.y);
  h = qCeil(target.x);

  return true;
}

void
Viewer::saveImage()
{
  QStringList savelist;
  savelist << "Save single image";
  savelist << "Save image sequence/movie";
  QString savetype = QInputDialog::getItem(this,
					   "Save Snapshot/Movie",
					   "Save Snapshot/Movie",
					   savelist,
					   0,
					   false);

  if (savetype == "Save image sequence/movie")
    saveImageSequence();
  else
    {
      QString flnm;
      flnm = QFileDialog::getSaveFileName(0,
		  "Save snapshot",
		  Global::previousDirectory(),
		  "Image Files (*.png *.tif *.bmp *.jpg *.ppm *.xbm *.xpm)");
      
      if (flnm.isEmpty())
	return;

      saveSnapshot(flnm);
    }
}


void
Viewer::saveImageSequence()
{
  QString flnm;
#if USE_GLMEDIA
  flnm = QFileDialog::getSaveFileName(0,
				      "Save Image Sequence/Movie",
				      Global::previousDirectory(),
       "Image/Movie Files (*.png *.tif *.bmp *.jpg *.ppm *.xbm *.xpm *.wmv)");
#endif
#if NO_GLMEDIA
  flnm = QFileDialog::getSaveFileName(0,
				      "Save Image Sequence",
				      Global::previousDirectory(),
       "Image Files (*.png *.tif *.bmp *.jpg *.ppm *.xbm *.xpm)");
#endif


  if (flnm.isEmpty())
    return;

  QStringList yxaxis;
  yxaxis << "Y";
  yxaxis << "X";
  QString axistype = QInputDialog::getItem(this,
					   "Rotation Axis",
					   "Rotation Axis",
					   yxaxis,
					   0,
					   false);

  m_startAngle = QInputDialog::getDouble(this,
				     "Start Angle",
				     "Start Angle",
				     0, -360, 360, 1);

  m_endAngle = QInputDialog::getDouble(this,
				     "End Angle",
				     "End Angle",
				     360, -360, 360, 1);


  m_totFrames = QInputDialog::getInt(this,
				     "Number of Frames",
				     "Number of Frames",
				     360, 1, 10000, 1);


  if (StaticFunctions::checkExtension(flnm, ".wmv"))
    {
      m_savingImages = 2;
      startMovie(flnm, 25, 100, true);
    }
  else
    m_savingImages = 1;

  m_frameImageFile = flnm;
  m_currFrame = 0;
  float angle = (m_endAngle-m_startAngle)/m_totFrames;
  if (axistype == "X")
    m_stepRot = Quaternion(camera()->rightVector(), DEG2RAD(angle)); // X-axis
  else
    m_stepRot = Quaternion(camera()->upVector(), DEG2RAD(angle)); // Y-axis


  //----------------
  //-- go to start angle
  Quaternion startRot;
  if (axistype == "X")
    startRot = Quaternion(camera()->rightVector(), DEG2RAD(m_startAngle)); // X-axis
  else
    startRot = Quaternion(camera()->upVector(), DEG2RAD(m_startAngle)); // X-axis

  startRot = startRot*camera()->orientation();

  // set camera orientation
  camera()->setOrientation(startRot);
  
  // now reposition the camera so that it faces the scene
  Vec center = camera()->sceneCenter();
  float dist = (camera()->position()-center).norm();
  Vec viewDir = camera()->viewDirection();
  camera()->setPosition(center - dist*viewDir);
  //----------------
  
  nextFrame();
}

void
Viewer::nextFrame()
{
  if (m_currFrame < m_totFrames)
    {
      m_currFrame ++;
      if (m_currFrame > 1)
	{
	  Quaternion rot = m_stepRot*camera()->orientation();

	  // set camera orientation
	  camera()->setOrientation(rot);
	  
	  // now reposition the camera so that it faces the scene
	  Vec center = camera()->sceneCenter();
	  float dist = (camera()->position()-center).norm();
	  Vec viewDir = camera()->viewDirection();
	  camera()->setPosition(center - dist*viewDir);
	}

      emit updateGL();
      qApp->processEvents();
    }
  else
    {
      if (m_savingImages == 2) // movie
	endMovie();

      m_savingImages = 0;

      QMessageBox::information(this, "", "Saved Image Sequence");
    }
}

void
Viewer::saveImageFrame()
{
  if (m_savingImages == 2)
    {
      saveMovie();
    }
  else
    {
      QFileInfo f(m_frameImageFile);

      QString imgFile = f.absolutePath() + QDir::separator() +
	                f.baseName();
      imgFile += QString("%1").arg((int)m_currFrame, 5, 10, QChar('0'));
      imgFile += ".";
      imgFile += f.completeSuffix();

      saveSnapshot(imgFile);
    }

  QTimer::singleShot(200, this, SLOT(nextFrame()));
}

void
Viewer::saveSnapshot(QString imgFile)
{
  int wd = width();
  int ht = height();
  uchar *imgbuf = new uchar[wd*ht*4];
  glReadPixels(0, 0, wd, ht, GL_RGBA, GL_UNSIGNED_BYTE, imgbuf);

  for(int i=0; i<wd*ht; i++)
    {
      uchar r = imgbuf[4*i+0];
      uchar g = imgbuf[4*i+1];
      uchar b = imgbuf[4*i+2];
      uchar a = imgbuf[4*i+3];
      
      uchar ma = qMax(qMax(r,g),qMax(b,a));
      imgbuf[4*i+3] = ma;
    }

  if (imgFile.endsWith(".png"))
    {
      QImage bimg(imgbuf, wd, ht, QImage::Format_ARGB32_Premultiplied);
      StaticFunctions::convertFromGLImage(bimg, wd, ht);
      bimg.save(imgFile);      
    }
  else
    {
      QImage bimg(imgbuf, wd, ht, QImage::Format_ARGB32);
      StaticFunctions::convertFromGLImage(bimg, wd, ht);
      bimg.save(imgFile);      
    }

  delete [] imgbuf;
}

bool
Viewer::startMovie(QString movieFile,
		   int ofps, int quality,
		   bool checkfps)
{
#ifdef USE_GLMEDIA

  int fps = ofps;
//  if (checkfps)
//    {
//      bool ok;
//      QString text;
//      text = QInputDialog::getText(this,
//				   "Set Frame Rate",
//				   "Frame Rate",
//				   QLineEdit::Normal,
//				   "25",
//				   &ok);
//      
//      if (ok && !text.isEmpty())
//	fps = text.toInt();
//      
//      if (fps <=0 || fps >= 100)
//	fps = 25;
//    }

  //---------------------------------------------------------
  // mono movie or left-eye movie
  m_movieWriter = glmedia_movie_writer_create();
  if (m_movieWriter == NULL) {
    QMessageBox::critical(0, "Movie",
			  "Failed to create writer");
    return false;
  }

  if (glmedia_movie_writer_start(m_movieWriter,
				 movieFile.toLatin1().data(),
				 width(),
				 height(),
				 fps,
				 quality) < 0) {
    QMessageBox::critical(0, "Movie",
			  "Failed to start movie");
    return false;
  }

#endif // USE_GLMEDIA
  return true;
}

bool
Viewer::endMovie()
{
#ifdef USE_GLMEDIA
  if (glmedia_movie_writer_end(m_movieWriter) < 0)
    {
      QMessageBox::critical(0, "Movie",
			       "Failed to end movie");
      return false;
    }
  glmedia_movie_writer_free(m_movieWriter);
#endif // USE_GLMEDIA
  return true;
}

void
Viewer::saveMovie()
{
#ifdef USE_GLMEDIA
  int wd = width();
  int ht = height();
  uchar *imgbuf = new uchar[wd*ht*4];
  glReadPixels(0, 0, wd, ht, GL_RGBA, GL_UNSIGNED_BYTE, imgbuf);

  glmedia_movie_writer_add(m_movieWriter, imgbuf);
  delete [] imgbuf;
#endif // USE_GLMEDIA
}

void
Viewer::generateBoxMinMax()
{
  QProgressDialog progress(QString("Updating min-max structure (%1)").arg(m_boxSize),
			   QString(),
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);

  m_dbox = m_depth/m_boxSize;
  m_wbox = m_width/m_boxSize;
  m_hbox = m_height/m_boxSize;
  if (m_depth%m_boxSize > 0) m_dbox++;
  if (m_width%m_boxSize > 0) m_wbox++;
  if (m_height%m_boxSize > 0) m_hbox++;

  m_boxMinMax.clear();
  m_boxMinMax.reserve(2*m_dbox*m_wbox*m_hbox);

  m_filledBoxes.resize(m_dbox*m_wbox*m_hbox);

  for(int d=0; d<m_dbox; d++)
    {
      progress.setValue(100*(float)d/m_dbox);
      qApp->processEvents();

      for(int w=0; w<m_wbox; w++)
	for(int h=0; h<m_hbox; h++)
	  {
	    int vmax = -1;
	    int vmin = 65535;
	    int dmin = d*m_boxSize;
	    int wmin = w*m_boxSize;
	    int hmin = h*m_boxSize;
	    int dmax = qMin((d+1)*m_boxSize, (int)m_depth);
	    int wmax = qMin((w+1)*m_boxSize, (int)m_width);
	    int hmax = qMin((h+1)*m_boxSize, (int)m_height);
	    if (Global::bytesPerVoxel() == 1)
	      {
		for(int dm=dmin; dm<dmax; dm++)
		  for(int wm=wmin; wm<wmax; wm++)
		    for(int hm=hmin; hm<hmax; hm++)
		      {
			int v = m_volPtr[dm*m_width*m_height + wm*m_height + hm];
			vmin = qMin(vmin, v);
			vmax = qMax(vmax, v);
		      }
	      }
	    else
	      {
		for(int dm=dmin; dm<dmax; dm++)
		  for(int wm=wmin; wm<wmax; wm++)
		    for(int hm=hmin; hm<hmax; hm++)
		      {
			int v = m_volPtrUS[dm*m_width*m_height + wm*m_height + hm];
			vmin = qMin(vmin, v);
			vmax = qMax(vmax, v);
		      }
	      }
	    m_boxMinMax << vmin;
	    m_boxMinMax << vmax;
	  }
    }

  progress.setValue(100);
}


QList<int>
Viewer::usedTags()
{
  QProgressDialog progress("Calculating Tags Used",
			   QString(),
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);

  QList<int> ut;
  qint64 tvox = m_depth*m_width*m_height;
  for(qint64 i=0; i<tvox; i++)
    {
      if (i%100000 == 0)
	{
	  progress.setValue(100*(float)i/(float)tvox);
	  qApp->processEvents();
	}
      if (!ut.contains(m_maskPtr[i]))
	ut << m_maskPtr[i];      
    }
  qSort(ut);
  return ut;
}

//---------------------------
//---------------------------
void
Viewer::updateTF()
{
  uchar *lut = Global::lut();
  int lmin = 255;
  int lmax = 0;

  int iend = 255;
  if (Global::bytesPerVoxel() == 2)
    {
      lmin = 65535;
      iend = 65535;
    }

  for(int i=0; i<iend; i++)
    {
      if (lut[4*i+3] > 2)
	{
	  lmin = i;
	  break;
	}
    }
  
  for(int i=iend; i>0; i--)
    {
      if (lut[4*i+3] > 2)
	{
	  lmax = i;
	  break;
	}
    }

  if (m_lmax != lmax ||
      m_lmin != lmin)
    {
      m_lmax = lmax;
      m_lmin = lmin;
      updateFilledBoxes();
    }
}
void
Viewer::updateFilledBoxes()
{
  if (m_boxMinMax.count() == 0)
    return;
  
  QProgressDialog progress("Marking valid boxes - (1/2)",
			   QString(),
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);


  progress.setValue(10);
  qApp->processEvents();

  Vec bminO, bmaxO;
  m_boundingBox.bounds(bminO, bmaxO);

  Vec voxelScaling = Global::relativeVoxelScaling();
  bminO = VECDIVIDE(bminO, voxelScaling);
  bmaxO = VECDIVIDE(bmaxO, voxelScaling);

  bminO = StaticFunctions::maxVec(bminO, Vec(m_minHSlice, m_minWSlice, m_minDSlice));
  bmaxO = StaticFunctions::minVec(bmaxO, Vec(m_maxHSlice, m_maxWSlice, m_maxDSlice));

  progress.setValue(20);
  qApp->processEvents();
  
  m_filledBoxes.fill(true);
  for(int d=0; d<m_dbox; d++)
    {
      progress.setValue(100*d/m_dbox);
      qApp->processEvents();
  
    for(int w=0; w<m_wbox; w++)
      for(int h=0; h<m_hbox; h++)
	{
	  bool ok = true;
	  // consider only current bounding box	 
	  if ((d*m_boxSize < bminO.z && (d+1)*m_boxSize < bminO.z) ||
	      (d*m_boxSize > bmaxO.z && (d+1)*m_boxSize > bmaxO.z) ||
	      (w*m_boxSize < bminO.y && (w+1)*m_boxSize < bminO.y) ||
	      (w*m_boxSize > bmaxO.y && (w+1)*m_boxSize > bmaxO.y) ||
	      (h*m_boxSize < bminO.x && (h+1)*m_boxSize < bminO.x) ||
	      (h*m_boxSize > bmaxO.x && (h+1)*m_boxSize > bmaxO.x))
	    ok = false;
	  
	  int idx = d*m_wbox*m_hbox+w*m_hbox+h;
	  if (ok)
	    {
	      int bmin = m_boxMinMax[2*idx+0];
	      int bmax = m_boxMinMax[2*idx+1];
	      if ((bmin < m_lmin && bmax < m_lmin) || 
		  (bmin > m_lmax && bmax > m_lmax))
		m_filledBoxes.setBit(idx, false);
	    }
	  else
	    m_filledBoxes.setBit(idx, false);
	}
    }
  progress.setValue(100);
  qApp->processEvents();

  generateDrawBoxes();
}

void
Viewer::generateDrawBoxes()
{ 
  QProgressDialog progress("Marking valid boxes - (2/2)",
			   QString(),
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);


  progress.setValue(10);
  qApp->processEvents();


  m_mdEle = 0;
  
  int imin,jmin,kmin;
  imin = jmin = kmin = 0;
  int imax = m_hbox;
  int jmax = m_wbox;
  int kmax = m_dbox;
  
  int mdC = 0;
  int mdI = -1;

  int bid = 0;
  for(int k=kmin; k<kmax; k++)
    {
      progress.setValue(100*k/kmax);
      qApp->processEvents();
  
      for(int j=jmin; j<jmax; j++)
	for(int i=imin; i<imax; i++)
	  {
	    int idx = k*m_wbox*m_hbox+j*m_hbox+i;
	    if (m_filledBoxes.testBit(idx))
	      {
		if (mdI < 0) mdI = bid;
		mdC++;
	      }
	    else
	      {
		if (mdI > -1)
		  {
		    // 2 triangles per face - 36 triangles in all for a cube
		    m_mdIndices[m_mdEle] = mdI*36;
		    m_mdCount[m_mdEle] = mdC*36;
		    m_mdEle++;
		    if (m_mdEle >= m_numBoxes)
		      QMessageBox::information(0, "", QString("ele > %1").arg(m_numBoxes));
		    
		    mdI = -1;
		    mdC = 0;
		  }
	      }
	    bid++;
	  }
    }

  if (mdI > -1)
    {
      m_mdIndices[m_mdEle] = mdI*36;
      m_mdCount[m_mdEle] = mdC*36;
      m_mdEle++;
    }

  progress.setValue(100);
  qApp->processEvents();
}

void
Viewer::loadAllBoxesToVBO()
{
  QProgressDialog progress("Loading box structure to vbo",
			   QString(),
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);

  m_mdEle = 0;
  if (m_mdCount) delete [] m_mdCount;
  if (m_mdIndices) delete [] m_mdIndices;  
  m_mdCount = new GLsizei[m_numBoxes];
  m_mdIndices = new GLint[m_numBoxes];
  
  //---------------------
  int nvert = 0;
  for(int i=0; i<m_numBoxes; i++)
    nvert += m_boxSoup[i].count();

  m_ntri = nvert/3;
  int nv = 3*nvert;
  int ni = 3*m_ntri;
  float *vertData;
  vertData = new float[nv];
  int vi=0;
  for(int i=0; i<m_numBoxes; i++)
    {
      if ((i/100)%10 == 0)
	{
	  progress.setValue(100*(float)i/(float)m_numBoxes);
	  qApp->processEvents();
	}
      for(int b=0; b<m_boxSoup[i].count(); b++)
	{
	  vertData[3*vi+0] = m_boxSoup[i][b].x;
	  vertData[3*vi+1] = m_boxSoup[i][b].y;
	  vertData[3*vi+2] = m_boxSoup[i][b].z;
	  vi++;
	}
    }
  //---------------------


  
  unsigned int *indexData;
  indexData = new unsigned int[ni];
  for(int i=0; i<ni; i++)
    indexData[i] = i;
  //---------------------

  progress.setValue(50);
  qApp->processEvents();


  if(m_glVertArray)
    {
      glDeleteBuffers(1, &m_glIndexBuffer);
      glDeleteVertexArrays( 1, &m_glVertArray );
      glDeleteBuffers(1, &m_glVertBuffer);
      m_glIndexBuffer = 0;
      m_glVertArray = 0;
      m_glVertBuffer = 0;
    }

  glGenVertexArrays(1, &m_glVertArray);
  glBindVertexArray(m_glVertArray);
      
  // Populate a vertex buffer
  glGenBuffers(1, &m_glVertBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, m_glVertBuffer);
  glBufferData(GL_ARRAY_BUFFER,
	       sizeof(float)*nv,
	       vertData,
	       GL_STATIC_DRAW);

  progress.setValue(70);
  qApp->processEvents();


  // Identify the components in the vertex buffer
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
			sizeof(float)*3, // stride
			(void *)0); // starting offset

  
  // Create and populate the index buffer
  glGenBuffers(1, &m_glIndexBuffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_glIndexBuffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
	       sizeof(unsigned int)*ni,
	       indexData,
	       GL_STATIC_DRAW);
  
  progress.setValue(90);
  qApp->processEvents();


  glBindVertexArray(0);

  delete [] vertData;
  delete [] indexData;

  m_boxSoup.clear();
  
  progress.setValue(100);
  qApp->processEvents();
}

void
Viewer::drawVBOBox(GLenum glFaces)
{  
  glEnable(GL_CULL_FACE);
  glCullFace(glFaces);

  glBindVertexArray(m_glVertArray);
  glBindBuffer(GL_ARRAY_BUFFER, m_glVertBuffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_glIndexBuffer);  

  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);

  glUseProgram(ShaderFactory::boxShader());

  // model-view-projection matrix
  GLdouble m[16];
  GLfloat mvp[16];
  camera()->getModelViewProjectionMatrix(m);
  for(int i=0; i<16; i++) mvp[i] = m[i];

  GLint *boxShaderParm = ShaderFactory::boxShaderParm();  

  glUniformMatrix4fv(boxShaderParm[0], 1, GL_FALSE, mvp);

  glMultiDrawArrays(GL_TRIANGLES, m_mdIndices, m_mdCount, m_mdEle);  
  

  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);

  glBindVertexArray(0);

  glUseProgram(0);

  glDisable(GL_CULL_FACE);
}
