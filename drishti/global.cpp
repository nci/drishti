#include "global.h"
#include <QFileDialog>
#include <QCoreApplication>
#include <QAction>

QString Global::DrishtiVersion() { return QString(DRISHTI_VERSION); }

bool Global::m_playFrames = false;
bool Global::playFrames() { return m_playFrames; }
void Global::setPlayFrames(bool pf) { m_playFrames = pf; }

float Global::m_gamma = 1.0;
void Global::setGamma(float g) { m_gamma = g; }
float Global::gamma() { return m_gamma; }

void Global::checkGLError(QString src, bool clearErrors)
{
  GLenum err = glGetError(); 
  while (err != GL_NO_ERROR)
    { 
      if (!clearErrors)
	QMessageBox::information(0, "glError",
				 QString("%1 : %2 %3").			\
				 arg(src).arg(err).arg((char*)gluErrorString(err)));
      err = glGetError(); 
    }
}

QString Global::m_documentationPath = "";
QString
Global::documentationPath()
{
  if (m_documentationPath.isEmpty() == false)
    {
      if (m_documentationPath == "dontask")
	return "";
      else
	return m_documentationPath;
    }

#if defined(Q_OS_LINUX)
  QDir app = QCoreApplication::applicationDirPath();
  app.cdUp();
  app.cd("doc");
  app.cd("drishti");

#elif defined(Q_OS_MAC)
  QDir app = QCoreApplication::applicationDirPath();

  app.cdUp();
  app.cdUp();
  app.cdUp();
  app.cd("Shared");
  app.cd("Docs");
  app.cd("drishti");

#else

  QDir app = QCoreApplication::applicationDirPath();
  app.cdUp();
  app.cd("docs");
  app.cd("drishti");
#endif // defined(Q_OS_LINUX)

  QString page = QFileInfo(app, "drishti.qhc").absoluteFilePath();
  QFileInfo f(page);

  if (f.exists())
    {
      m_documentationPath = f.absolutePath();
    }
  else
    {
      QString path;
      path = QFileDialog::getExistingDirectory(0,
			  "Drishti Documentation Directory",
			   QCoreApplication::applicationDirPath());
      if (path.isEmpty() == false)
	m_documentationPath = path;
      else
	m_documentationPath = "dontask";
    }
  m_documentationPath = QDir(m_documentationPath).canonicalPath();
  return m_documentationPath;
}

int Global::m_viewerFilter = _GaussianFilter;
int Global::viewerFilter() { return m_viewerFilter; }
void Global::setViewerFilter(int vf) { m_viewerFilter = vf; }


bool Global::m_emptySpaceSkip = true;
bool Global::emptySpaceSkip() { return m_emptySpaceSkip; }
void Global::setEmptySpaceSkip(bool ess) { m_emptySpaceSkip = ess; }

bool Global::m_updateViewer = true;
bool Global::updateViewer() { return m_updateViewer; }
void Global::enableViewerUpdate() { m_updateViewer = true; }
void Global::disableViewerUpdate() { m_updateViewer = false; }

int Global::m_imageQuality = _NormalQuality;
int Global::imageQuality() { return m_imageQuality; }
void Global::setImageQuality(int iq) { m_imageQuality = iq; }

bool Global::m_bottomText = true;
bool Global::bottomText() { return m_bottomText; }
void Global::setBottomText(bool flag) { m_bottomText = flag; }

bool Global::m_depthcue = false;
bool Global::depthcue() { return m_depthcue; }
void Global::setDepthcue(bool flag) { m_depthcue = flag; }

bool Global::m_useStillVolume = false;
bool Global::useStillVolume() { return m_useStillVolume; }
void Global::setUseStillVolume(bool flag) { m_useStillVolume = flag; }

bool Global::m_useDragVolume = false;
bool Global::useDragVolume() { return m_useDragVolume; }
void Global::setUseDragVolume(bool flag) { m_useDragVolume = flag; }

bool Global::m_loadDragOnly = false;
bool Global::loadDragOnly() { return m_loadDragOnly; }
void Global::setLoadDragOnly(bool flag) { m_loadDragOnly = flag; }

bool Global::m_useDragVolumeforShadows = false;
bool Global::useDragVolumeforShadows() { return m_useDragVolumeforShadows; }
void Global::setUseDragVolumeforShadows(bool flag) { m_useDragVolumeforShadows = flag; }

bool Global::m_flipImageX = false;
bool Global::m_flipImageY = false;
bool Global::m_flipImageZ = false;
bool Global::flipImage() { return m_flipImageX || m_flipImageY || m_flipImageZ; }
bool Global::flipImageX() { return m_flipImageX; }
bool Global::flipImageY() { return m_flipImageY; }
bool Global::flipImageZ() { return m_flipImageZ; }
void Global::setFlipImageX(bool flag) { m_flipImageX = flag; }
void Global::setFlipImageY(bool flag) { m_flipImageY = flag; }
void Global::setFlipImageZ(bool flag) { m_flipImageZ = flag; }

float Global::m_texSizeReduceFraction = 1.0f;
void Global::setTexSizeReduceFraction(float rf) { m_texSizeReduceFraction = qBound(0.1f, rf, 2.0f); }
float Global::texSizeReduceFraction() { return m_texSizeReduceFraction; }

GLint
Global::max2dTextureSize()
{
  GLint texSize;
  glGetIntegerv(GL_MAX_RECTANGLE_TEXTURE_SIZE_ARB, &texSize);
  
 // restrict max 2D texturesize to 8K
  //texSize = qMin(16384, texSize);

  // but if the user wants to modify it, so be it
  return texSize*m_texSizeReduceFraction;
  //return texSize;
}

int Global::m_textureSize = 25; // 512x256x256 (9+8+8)
int Global::textureSize() {return m_textureSize;}
void Global::setTextureSize(int sz) {m_textureSize = sz;}


int Global::m_textureMemorySize = 128;
int Global::textureMemorySize() {return m_textureMemorySize;}
void Global::setTextureMemorySize(int sz) { m_textureMemorySize = sz; }

void Global::calculate3dTextureSize()
{
  int comp = 1;

  if (volumeType() == SingleVolume)
    comp = 1;
  else if (volumeType() == DoubleVolume)
    comp = 2;
  else if (volumeType() == TripleVolume)
    comp = 3;
  else if (volumeType() == QuadVolume)
    comp = 4;
  else if (volumeType() == RGBVolume)
    comp = 3;
  else if (volumeType() == RGBAVolume)
    comp = 4;

  if (volumeType() != RGBVolume &&
      volumeType() != RGBAVolume &&
      useMask())
    comp++;

  
  qint64 tms = m_textureMemorySize;
  // set aside some texture memory for other buffers
  if (m_textureMemorySize >= 2048) tms -= 500;
  else if (m_textureMemorySize >= 1024) tms -= 256;
  else if (m_textureMemorySize >= 512) tms -= 64;
  else tms -= 32;

  tms /= comp;


  int p2 = 0;
  while (tms > 0)
    {
      tms /= 2;
      p2 ++;
    }
  // for memory size in Mb
  p2 += 20;
  p2 --;
  p2 = qMin(30, p2);

  Global::setTextureSize(p2);
}

int Global::m_lutSize = 4; // 4 lookup tables
int Global::lutSize() {return m_lutSize;}
void Global::setLutSize(int sz) {m_lutSize = sz;}

int Global::m_filteredData = _NoFilter;
int Global::filteredData() {return m_filteredData;}
void Global::setFilteredData(int filter) {m_filteredData = filter;}

bool Global::m_use1D = false;
bool Global::use1D() { return m_use1D; }
void Global::setUse1D(bool flag)
{
  m_use1D = flag;
}

Vec Global::m_relativeVoxelScaling = Vec(1,1,1);
Vec Global::relativeVoxelScaling() {return m_relativeVoxelScaling;}
void Global::setRelativeVoxelScaling(Vec v) {m_relativeVoxelScaling = v;}

Vec Global::voxelScaling()
{
  return Vec(m_relativeVoxelScaling.x,
	     m_relativeVoxelScaling.y,
	     m_relativeVoxelScaling.z);
}

QString Global::m_previousDirectory = "";
QString Global::previousDirectory() {return m_previousDirectory;}
void Global::setPreviousDirectory(QString d) {m_previousDirectory = d;}

bool Global::m_useFBO = true;
bool Global::useFBO() { return m_useFBO; }
void Global::setUseFBO(bool flag) { m_useFBO = flag; }

Vec Global::m_dataMin = Vec(0,0,0);
Vec Global::m_dataMax = Vec(0,0,0);
void Global::setBounds(Vec bmin, Vec bmax)
{
  m_dataMin = bmin;
  m_dataMax = bmax;
}
void Global::bounds(Vec& bmin, Vec& bmax)
{
  bmin = m_dataMin;
  bmax = m_dataMax;
}

bool Global::m_drawBox = true;
void Global::setDrawBox(bool db) { m_drawBox = db; }
bool Global::drawBox() { return m_drawBox; }

bool Global::m_drawAxis = false;
void Global::setDrawAxis(bool db) { m_drawAxis = db; }
bool Global::drawAxis() { return m_drawAxis; }

Vec Global::m_bgColor = Vec(100, 100, 100);
void Global::setBackgroundColor(Vec bgcolor) { m_bgColor = bgcolor; }
Vec Global::backgroundColor() { return m_bgColor; }

QString Global::m_bgImageFile = "";
QImage Global::m_bgImage = QImage();
GLuint Global::m_bgTexture = 0;
void
Global::resetBackgroundImageFile()
{
  m_bgImageFile = "";
  m_bgImage = QImage();
}
void
Global::setBackgroundImageFile(QString flnm, QString reldir)
{
  m_bgImageFile = flnm;

  //----------------
  // file is assumed to be relative to reldir file
  // get the absolute path
  QString aflnm = QFileInfo(reldir, flnm).absoluteFilePath();
  //----------------

  m_bgImage = QImage(aflnm).rgbSwapped();
}
QString Global::backgroundImageFile() { return m_bgImageFile; }
QImage Global::backgroundImage() { return m_bgImage; }
GLuint
Global::backgroundTexture()
{
  if (! m_bgTexture) 
    glGenTextures(1, &m_bgTexture);

  return m_bgTexture;
}
void
Global::removeBackgroundTexture()
{
  if (m_bgTexture)
    glDeleteTextures( 1, &m_bgTexture );
  m_bgTexture = 0;
}

float Global::m_stepsizeStill = 1.0;
void Global::setStepsizeStill(float s) { m_stepsizeStill = s; }
float Global::stepsizeStill() { return m_stepsizeStill; }

float Global::m_stepsizeDrag = 2.0;
void Global::setStepsizeDrag(float s) { m_stepsizeDrag = s; }
float Global::stepsizeDrag()  { return m_stepsizeDrag; }

int Global::m_saveImageType = Global::NoImage;
void Global::setSaveImageType(int si) { m_saveImageType = si; }
int Global::saveImageType() { return m_saveImageType; }

int Global::m_frameNumber = 1;
void Global::setFrameNumber(int f) { m_frameNumber = f; }
int Global::frameNumber() { return m_frameNumber; }

int Global::m_volumeNumber[4] = {0,0,0,0};
void Global::setVolumeNumber(int vn, int vol) { if (vol < 4) m_volumeNumber[vol] = vn; }
int Global::volumeNumber(int vol)
{
  if (vol<4)
    return m_volumeNumber[vol];
  else
    return m_volumeNumber[0];
}

int Global::m_actualVolumeNumber[4] = {0,0,0,0};
void Global::setActualVolumeNumber(int vn, int vol) { if (vol < 4) m_actualVolumeNumber[vol] = vn; }
int Global::actualVolumeNumber(int vol)
{
  if (vol<4)
    return m_actualVolumeNumber[vol];
  else
    return m_actualVolumeNumber[0];
}

int Global::m_volumeType = Global::DummyVolume;
void Global::setVolumeType(int vt)
{
  m_volumeType = vt;
  calculate3dTextureSize();
}
int Global::volumeType() { return m_volumeType; }

bool Global::m_replaceTF = false;
void Global::setReplaceTF(bool flag) { m_replaceTF = flag; }
bool Global::replaceTF() { return m_replaceTF; }

bool Global::m_morphTF = false;
void Global::setMorphTF(bool flag) { m_morphTF = flag; }
bool Global::morphTF() { return m_morphTF; }

bool Global::m_useMask = false;
bool Global::useMask() { return m_useMask; }
void Global::setUseMask(bool flag)
{
  m_useMask = flag;
}


uchar* Global::m_tagColors = 0;
uchar* Global::tagColors()
{
  if (!m_tagColors)
    {
      m_tagColors = new uchar[1024];
      memset(m_tagColors, 0, 1024);
    }
  return m_tagColors;
}
void Global::setTagColors(uchar *colors)
{
  if (!m_tagColors)
    m_tagColors = new uchar[1024];
  memcpy(m_tagColors, colors, 1024);
}


qint64 Global::m_maxDragVolSize = 128;
void Global::setMaxDragVolSize(qint64 sz)
{
  m_maxDragVolSize = sz;
  m_updatePruneTexture = true;  // force to update prune texture when loading volume
}
qint64 Global::maxDragVolSize() { return m_maxDragVolSize; }

qint64 Global::m_actualDragVolSize = 0;
qint64 Global::actualDragVolSize() { return m_actualDragVolSize; }


//this is actually GL_MAX_ARRAY_TEXTURE_LAYERS
int Global::m_maxArrayTextureLayers = 512;
void Global::setMaxArrayTextureLayers(int sz) { m_maxArrayTextureLayers = sz; }
int Global::maxArrayTextureLayers() { return m_maxArrayTextureLayers; }

bool Global::m_interpolationType[10] = {true, true, true, true, true,
					true, true, true, true, true};
void Global::setInterpolationType(int parm, bool type) { m_interpolationType[parm] = type; }
bool Global::interpolationType(int parm) { return m_interpolationType[parm]; }

int Global::m_maxRecentFiles = 5;
QStringList Global::m_recentFiles;
int Global::maxRecentFiles() { return m_maxRecentFiles; }
void Global::setRecentFiles(QStringList s) { m_recentFiles = s; }
QStringList Global::recentFiles() { return m_recentFiles; }
void Global::addRecentFile(QString s)
{
  //---------------------
  // first remove duplicates of 's'
  QStringList t = m_recentFiles;
  m_recentFiles.clear();
  for(int i=0; i<t.count(); i++)
    if (t[i] != s)
      m_recentFiles.append(t[i]);
  m_recentFiles.append(s);
  //---------------------

  while (m_recentFiles.count() > m_maxRecentFiles)
    m_recentFiles.removeFirst();
}
QString Global::recentFile(int i)
{
  if (m_recentFiles.count() > i)
    return m_recentFiles[i];
  else
    return QString();
}

QString Global::m_currentProjectFile;
void Global::resetCurrentProjectFile() { m_currentProjectFile.clear(); }
void Global::setCurrentProjectFile(QString s) { m_currentProjectFile = s; }
QString Global::currentProjectFile() { return m_currentProjectFile; }

uchar* Global::m_doodleMask = 0;
uchar* Global::doodleMask() { return m_doodleMask; }
void Global::setDoodleMask(uchar *dm, int sz)
{
  if (m_doodleMask) delete [] m_doodleMask;

  m_doodleMask = new uchar[sz];
  memcpy(m_doodleMask, dm, sz);
}


GLuint Global::m_lightTexture = 0;
void Global::removeLightTexture()
{
  if (m_lightTexture)
    glDeleteTextures( 1, &m_lightTexture );
  m_lightTexture = 0;
}
GLuint Global::lightTexture()
{
  if (m_lightTexture)
    return m_lightTexture;

  glGenTextures( 1, &m_lightTexture );

  QImage info(":/images/light.png");
  int texsize = info.height();
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, m_lightTexture);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
	       texsize, texsize, 0,
	       GL_BGRA, GL_UNSIGNED_BYTE,
	       info.bits());
  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

  return m_lightTexture;
}

GLuint Global::m_spriteTexture = 0;
void Global::removeSpriteTexture()
{
  if (m_spriteTexture)
    glDeleteTextures( 1, &m_spriteTexture );
  m_spriteTexture = 0;
}
GLuint Global::spriteTexture()
{
  if (m_spriteTexture)
    return m_spriteTexture;

  glGenTextures( 1, &m_spriteTexture );

//----------------------------------------
//--- filled circle sprite ---
  int texsize = 64;
  float md = texsize/2-0.5;
  uchar *thetexture = new uchar[2*texsize*texsize];
  for (int x=0; x < texsize; x++) {
    for (int y=0; y < texsize; y++) {
      int index = x*texsize + y;
      float a = (x-md);
      float b = (y-md);
      float r2 = sqrt(a*a + b*b)/md;
      r2 = 1.0 - qBound(0.0f, r2, 1.0f);
      r2 = qBound(0.0f, 1.5f*r2, 1.0f);
      r2 *= 255;
      thetexture[2*index] = r2;
      thetexture[2*index+1] = r2;
    }
  }
//----------------------------------------

  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, m_spriteTexture);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA,
	       texsize, texsize, 0,
	       GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE,
	       thetexture);
  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

  delete [] thetexture;


  return m_spriteTexture;
}

GLuint Global::m_infoSpriteTexture = 0;
void Global::removeInfoSpriteTexture()
{
  if (m_infoSpriteTexture)
    glDeleteTextures( 1, &m_infoSpriteTexture );
  m_infoSpriteTexture = 0;
}
GLuint Global::infoSpriteTexture()
{
  if (m_infoSpriteTexture)
    return m_infoSpriteTexture;

  glGenTextures( 1, &m_infoSpriteTexture );

  QImage info(":/images/info-icon.png");
  int texsize = info.height();
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, m_infoSpriteTexture);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
	       texsize, texsize, 0,
	       GL_BGRA, GL_UNSIGNED_BYTE,
	       info.bits());
  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

  return m_infoSpriteTexture;
}

GLuint Global::m_hollowSpriteTexture = 0;
void Global::removeHollowSpriteTexture()
{
  if (m_hollowSpriteTexture)
    glDeleteTextures( 1, &m_hollowSpriteTexture );
  m_hollowSpriteTexture = 0;
}
GLuint Global::hollowSpriteTexture()
{
  if (m_hollowSpriteTexture)
    return m_hollowSpriteTexture;

  glGenTextures( 1, &m_hollowSpriteTexture );

//----------------------------------------
//--- hollow circle sprite ---
  int texsize = 64;
  int t2 = texsize/2;
  QRadialGradient rg(t2, t2, t2-1, t2, t2);
  rg.setColorAt(0.0, Qt::white);
  rg.setColorAt(1.0, Qt::black);

  QImage texImage(texsize, texsize, QImage::Format_ARGB32);
  texImage.fill(0);
  QPainter p(&texImage);
  p.setBrush(QBrush(rg));
  p.setPen(Qt::transparent);
  p.drawEllipse(0, 0, texsize, texsize);

  uchar *thetexture = new uchar[2*texsize*texsize];
  const uchar *bits = texImage.bits();
  //const uchar *bits = info.bits();
  for(int i=0; i<texsize*texsize; i++)
    {
      uchar lum = 255;
      float a = (float)bits[4*i+2]/255.0f;
      a = 1-a;
      
      if (a < 0.8 || a >= 1.0)
	{
	  a = 0;
	  lum = 0;
	}
      else
	{
	  lum *= 1-fabs(a-0.8f)/0.2f;
	  a = 0.9f;
	}
      
      a *= 255;
      
      thetexture[2*i] = lum;
      thetexture[2*i+1] = a;
    }
//----------------------------------------

  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, m_hollowSpriteTexture);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA,
	       texsize, texsize, 0,
	       GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE,
	       thetexture);
  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

  delete [] thetexture;


  return m_hollowSpriteTexture;
}

GLuint Global::m_boxTexture = 0;
void Global::removeBoxTexture()
{
  if (m_boxTexture)
    glDeleteTextures( 1, &m_boxTexture );
  m_boxTexture = 0;
}
GLuint Global::boxTexture()
{
  if (m_boxTexture)
    return m_boxTexture;

  glGenTextures( 1, &m_boxTexture );

  int texsize = 64;
  uchar *thetexture = new uchar[2*texsize*texsize];
  for (int x=0; x < texsize; x++) {
    for (int y=0; y < texsize; y++) {
      uchar alpha = 255;
      uchar lum = 255;
      if (x>5 && x<59 && y>5 && y<59)
	{
	  lum = 128;
	  alpha = 200;
	}
      int index = x*texsize + y;
      thetexture[2*index] = lum;
      thetexture[2*index+1] = alpha;
    }
  }

  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, m_boxTexture);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA,
	       texsize, texsize, 0,
	       GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE,
	       thetexture);
  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

  delete [] thetexture;

  return m_boxTexture;
}


GLuint Global::m_sphereTexture = 0;
void Global::removeSphereTexture()
{
  if (m_sphereTexture)
    glDeleteTextures( 1, &m_sphereTexture );
  m_sphereTexture = 0;
}
GLuint Global::sphereTexture()
{
  if (m_sphereTexture)
    return m_sphereTexture;

  glGenTextures( 1, &m_sphereTexture );

  int texsize = 64;
  float md = texsize/2-0.5f;
  uchar *thetexture = new uchar[2*texsize*texsize];
  for (int x=0; x < texsize; x++) {
    for (int y=0; y < texsize; y++) {
      float xmd = (x-md);
      float ymd = (y-md);
      float r2 = sqrt(xmd*xmd + ymd*ymd)/md;
      r2 = qBound(0.0f, r2, 1.0f);

      uchar lum = 255;

      if (r2 < 0.5f)
	r2 = qMax(r2/0.5f, 0.5f);
      else if (r2 >= 1)
	{
	  r2 = 0;
	  lum = 0;
	}
      else
	{
	  r2 = 1-(r2-0.5f)/0.5f;
	  lum *= r2;
	  r2 = 0.9f;
	  //r2 = 0.9*qMax(1-r2, 0.7f);
	}
	
      r2 *= 255;
      //if (r2 > 100) r2 = qMax(r2, 200.0f);

      int index = x*texsize + y;
      thetexture[2*index] = lum;
      thetexture[2*index+1] = r2;
    }
  }

  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, m_sphereTexture);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA,
	       texsize, texsize, 0,
	       GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE,
	       thetexture);
  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

  delete [] thetexture;


  return m_sphereTexture;
}

GLuint Global::m_cylinderTexture = 0;
void Global::removeCylinderTexture()
{
  if (m_cylinderTexture)
    glDeleteTextures( 1, &m_cylinderTexture );
  m_cylinderTexture = 0;
}
GLuint Global::cylinderTexture()
{
  if (m_cylinderTexture)
    return m_cylinderTexture;

  glGenTextures( 1, &m_cylinderTexture );

  int texsize = 64;
  float md = texsize/2-0.5f;
  uchar *thetexture = new uchar[2*texsize*texsize];
  for (int x=0; x < texsize; x++)
    {
      for (int y=0; y < texsize; y++)
	{
	  float r2;
	  uchar lum = 255;

	  r2 = (y-md);
	  r2 = sqrt(r2*r2)/md;

	  if (r2 < 0.5f)
	    r2 = qMax(r2/0.5f, 0.5f);
	  else if (r2 >= 1)
	    {
	      r2 = 0;
	      lum = 0;
	    }
	  else
	    {
	      r2 = 1-(r2-0.5f)/0.5f;
	      lum *= r2;
	      r2 = 0.9f;
	      //r2 = 0.9*qMax(1-r2, 0.5f);
	    }

	  r2 *= 255;
	  
	  int index = x*texsize + y;
	  thetexture[2*index] = lum;
	  thetexture[2*index+1] = r2;
	}
    }

  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, m_cylinderTexture);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA,
	       texsize, texsize, 0,
	       GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE,
	       thetexture);
  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

  delete [] thetexture;


  return m_cylinderTexture;
}

Vec
Global::getDragInfo(int bytesPerVoxel, Vec dataMin, Vec dataMax, int lod0)
{
  int texSize = max2dTextureSize();
  qint64 lenx = dataMax.x - dataMin.x + 1;
  qint64 leny = dataMax.y - dataMin.y + 1;
  qint64 lenz = dataMax.z - dataMin.z + 1;

  qint64 mDVS = maxDragVolSize()*1024*1024/bytesPerVoxel; // reduce maxDragVolSize by nuber of bytes per voxel
  int mATL = maxArrayTextureLayers();
  int lod = lod0;
  qint64 lenx2 = lenx/lod;
  qint64 leny2 = leny/lod;
  qint64 lenz2 = lenz/lod;
  while(lenz2 > mATL ||
	lenx2*leny2*lenz2 > mDVS)
    {
      lod++;
      lenz2 = lenz/lod;
      lenx2 = lenx/lod;
      leny2 = leny/lod;
    }

  int dgridx = texSize/lenx2;
  int dgridy = texSize/leny2;


  // although the subsampled volume may fit in
  // the available texture memory, it may not
  // do so once the data is packed in 2d texture
  while (dgridx*dgridy < lenz2)
    {
      lod++;
      lenx2 = lenx/lod;
      leny2 = leny/lod;
      lenz2 = lenz/lod;
      dgridx = texSize/lenx2;
      dgridy = texSize/leny2;
    }
  
  // reduce the number of rows if the
  // subsampled volume is much smaller 
  while (dgridx*dgridy > lenz2)
    dgridy--;
  
  if (dgridx*dgridy < lenz2)
    dgridy++;


  // actual drag volume size in MB
  m_actualDragVolSize = qRound(lenx2*leny2*lenz2/1024.0/1024.0);
  //m_actualDragVolSize = qRound(dgridx*lenx2*dgridy*leny2/1024.0/1024.0);
  
  
  return Vec(dgridx, dgridy, lod);
}

QList<Vec>
Global::getSlabs(int samplingLevel,
		 int bytesPerVoxel,
		 Vec dataMin, Vec dataMax,
		 int &nrows, int &ncols)
{
  QList<Vec> slabinfo;

  int texSize = max2dTextureSize();
  int lenx = dataMax.x - dataMin.x + 1;
  int leny = dataMax.y - dataMin.y + 1;
  int lenz = dataMax.z - dataMin.z + 1;

  Vec draginfo = getDragInfo(bytesPerVoxel, dataMin, dataMax, 1);
  slabinfo.append(draginfo);

  int lenx2 = lenx/samplingLevel;
  int leny2 = leny/samplingLevel;
  int lenz2 = lenz/samplingLevel;

  int gridx = texSize/lenx2;
  int gridy = texSize/leny2;
  
  bool done = false;
  int slc = 0;
  int tSL = maxArrayTextureLayers();
  while(slc*(tSL-1) < (lenz2-1))
    {
      int zmin = slc*(tSL-1);
      int zmax = qMin(lenz2, (slc+1)*(tSL-1));
      int dmin = m_dataMin.z + samplingLevel*zmin;
      int dmax = m_dataMin.z + samplingLevel*zmax;
      dmax = qMin((int)m_dataMax.z , dmax);
      slabinfo.append(Vec(zmax-zmin+1,  // no. of slices in the slab
			  dmin, dmax));
      slc ++;
    }

  // taking rectangular textures
  ncols = gridx;
  nrows = gridy;

  return slabinfo;
}

//QList<Vec>
//Global::getSlabs(int samplingLevel,
//		 Vec dataMin, Vec dataMax,
//		 int &nrows, int &ncols)
//{
//  QList<Vec> slabinfo;
//
//  int texSize = max2dTextureSize();
//  int lenx = dataMax.x - dataMin.x + 1;
//  int leny = dataMax.y - dataMin.y + 1;
//  int lenz = dataMax.z - dataMin.z + 1;
//
//  Vec draginfo = getDragInfo(dataMin, dataMax, 1);
//  slabinfo.append(draginfo);
//
//  int lenx2 = lenx/samplingLevel;
//  int leny2 = leny/samplingLevel;
//  int lenz2 = lenz/samplingLevel;
//
//  int gridx = texSize/lenx2;
//  int gridy = texSize/leny2;
//  
//  bool done = false;
//  int slc = 0;
//  while(!done)
//    {
//      int pslc = slc;
//      //slc += (gridx*gridy-1);
//
//      // -2 for additional slice at top & bottom
//      slc += (gridx*gridy-1) - 2;
//
//      if (slc >= lenz2)
//	{
//	  done = true;
//	  slc = lenz2;
//	}
//      int ntex = slc-pslc+1;
//      slabinfo.append(Vec(ntex,
//			  (int)m_dataMin.z+(samplingLevel*pslc),
//			  (int)m_dataMin.z+(samplingLevel*slc)));
//
//////for array texture
//      done = true;
//    }
//
//
//  int endslab = slabinfo.count()-1;
//  slabinfo[endslab].x--; // number of slices in the slab
//  slabinfo[endslab].z--; // last slice number
//
//  // taking rectangular textures
//  ncols = gridx;
//  nrows = gridy;
//
////----------------------------  
//////removed for array texture
//////we will return slabinfo which contains atleast 2 elements
//////ensuring that there will always be dragVol and subVol
////  if (slabinfo.count() == 2)
////    {
////      nrows = (lenz2+2)/ncols;
////      if (nrows*ncols <= (lenz2+2)) nrows++;
////      if ((lenz2+2) < ncols) ncols = (lenz2+2);
////
////      //slabinfo.removeFirst(); // we don't need any drag volume
////    }
////----------------------------  
//
//  return slabinfo;
//}

QStatusBar* Global::m_statusBar = 0;
QAction* Global::m_actionStatusBar = 0;
QStatusBar* Global::statusBar() { return m_statusBar; }
void Global::setStatusBar(QStatusBar *sb, QAction *asb)
{
 m_statusBar = sb;
 m_actionStatusBar = asb;
}

QProgressBar* Global::m_progressBar = 0;
QProgressBar* Global::progressBar(bool forceShow)
{
//  if (forceShow)
//    m_statusBar->show();

  if (!m_progressBar)
    m_progressBar = new QProgressBar();
  return m_progressBar;
}
void Global::hideProgressBar()
{
  if (m_actionStatusBar->isChecked())
    m_statusBar->show();
  else
    m_statusBar->hide();
}

int Global::m_lod = 1;
int Global::lod() { return m_lod; }
void Global::setLod(int rl) { m_lod = qMax(1, rl); }

bool Global::m_batchMode = false;
void Global::setBatchMode(bool flag) { m_batchMode = flag; }
bool Global::batchMode() { return m_batchMode; }

QString Global::m_tempDir = "";
QString Global::tempDir() { return m_tempDir; }
void Global::setTempDir(QString d) { m_tempDir = d; }

int Global::m_floatPrecision = 2;
int Global::floatPrecision() { return m_floatPrecision; }
void Global::setFloatPrecision(int p) { m_floatPrecision = qMax(0, p); }

int Global::m_geoRenderSteps = 1;
void Global::setGeoRenderSteps(int s) { m_geoRenderSteps = qMax(1, qAbs(s)); }
int Global::geoRenderSteps() { return m_geoRenderSteps; }

bool Global::m_updatePruneTexture = true;
void Global::setUpdatePruneTexture(bool b) { m_updatePruneTexture = b; }
bool Global::updatePruneTexture() { return m_updatePruneTexture; }

int Global::m_mopOp = 0;
int Global::m_mopSz = 1;
int Global::mopOp() { return m_mopOp; }
int Global::mopSz() { return m_mopSz; }
void Global::setMopOp(int mo, int sz)
{
  m_mopOp = mo;
  m_mopSz = sz;
}

int Global::m_maskTF = -1;
void Global::setMaskTF(int t) { m_maskTF = t; }
int Global::maskTF() { return m_maskTF; }

GLhandleARB Global::m_copyShader=0;
GLint Global::m_copyParm[5];
void Global::setCopyShader(GLhandleARB h) { m_copyShader = h; }
GLhandleARB Global::copyShader() { return m_copyShader; }
void Global::setCopyParm(GLint *cp, int n) { memcpy(m_copyParm, cp, qMin(n,4)*sizeof(GLint));}
GLint Global::copyParm(int i) { return m_copyParm[qBound(0,i,4)]; }
GLint* Global::copyParm() { return &m_copyParm[0]; }

GLhandleARB Global::m_reduceShader=0;
GLint Global::m_reduceParm[5];
void Global::setReduceShader(GLhandleARB h) { m_reduceShader = h; }
GLhandleARB Global::reduceShader() { return m_reduceShader; }
void Global::setReduceParm(GLint *cp, int n) { memcpy(m_reduceParm, cp, qMin(n,4)*sizeof(GLint));}
GLint Global::reduceParm(int i) { return m_reduceParm[qBound(0,i,4)]; }
GLint* Global::reduceParm() { return &m_reduceParm[0]; }

GLhandleARB Global::m_extractSliceShader=0;
GLint Global::m_extractSliceParm[5];
void Global::setExtractSliceShader(GLhandleARB h) { m_extractSliceShader = h; }
GLhandleARB Global::extractSliceShader() { return m_extractSliceShader; }
void Global::setExtractSliceParm(GLint *cp, int n) { memcpy(m_extractSliceParm, cp, qMin(n,4)*sizeof(GLint));}
GLint Global::extractSliceParm(int i) { return m_extractSliceParm[qBound(0,i,4)]; }
GLint* Global::extractSliceParm() { return &m_extractSliceParm[0]; }

int Global::m_dpi = -1;
int Global::dpi()
{
  if (m_dpi > 0) return m_dpi;
  
  QImage texImage(100, 100, QImage::Format_ARGB32);
  QPainter p(&texImage);
  QPaintDevice *pd = p.device();
  m_dpi = pd->logicalDpiX();
  
  return m_dpi;
}

int Global::m_pvlVoxelType = 0; // uchar
void Global::setPvlVoxelType(int v) { m_pvlVoxelType = v; }
int Global::pvlVoxelType() { return m_pvlVoxelType; }

Vec Global::m_relDataPos = Vec(0,0,0);
void Global::setRelDataPos(Vec rp) { m_relDataPos = rp; }
Vec Global::relDataPos() { return m_relDataPos; }


//-----
// Material Capture Textures
//-----
QStringList Global::m_matCapTexNames = QStringList();
GLuint* Global::m_matCapTex = 0;
void Global::loadMatCapTextures()
{
  if (m_matCapTexNames.count() > 0)
    return;
    
  QString texdir = qApp->applicationDirPath() + QDir::separator()  + "assets" + QDir::separator() + "matcap";
  QDir dir(texdir);  
  QStringList filters;
  filters << "*.png";
  dir.setNameFilters(filters);
  dir.setFilter(QDir::Files |
		QDir::NoSymLinks |
		QDir::NoDotAndDotDot);


  QFileInfoList list = dir.entryInfoList();
  if (list.size() == 0)
    return;

  
  m_matCapTex = new GLuint[list.size()];
  glGenTextures(list.size(), m_matCapTex);
  
  QRandomGenerator::global()->bounded(0,list.size()-1);

  int texSize;
  for (int i=0; i<list.size(); i++)
    {
      QString flnm = list.at(i).absoluteFilePath();
      m_matCapTexNames << flnm;
      
      QImage img(flnm);
      img = img.rgbSwapped();
      texSize = img.width();
      int ht = img.height();
      int wd = img.width();
      int nbytes = img.byteCount();
      int rgb = nbytes/(wd*ht);

      GLuint fmt;
      if (rgb == 1) fmt = GL_LUMINANCE;
      else if (rgb == 2) fmt = GL_LUMINANCE_ALPHA;
      else if (rgb == 3) fmt = GL_RGB;
      else if (rgb == 4) fmt = GL_RGBA;

      glActiveTexture(GL_TEXTURE7);
      glEnable(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D, m_matCapTex[i]);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER); 
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER); 
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexImage2D(GL_TEXTURE_2D,
		   0, // single resolution
		   rgb,
		   texSize, texSize,
		   0, // no border
		   fmt,
		   GL_UNSIGNED_BYTE,
		   img.bits());
      glDisable(GL_TEXTURE_2D);
    }
}

QStringList Global::matCapTexNames() { return m_matCapTexNames; }

GLuint Global::matCapTex(int i)
{
  if (i >= 0 &&
      i < m_matCapTexNames.count())
    return m_matCapTex[i];
  else
    return 0;
}

int Global::m_matId = 0;
float Global::m_matMix = 0.5;
void Global::setMatId(int m) { m_matId = m; }
int Global::matId() { return m_matId; }
void Global::setMatMix(float f) { m_matMix = f; }
float Global::matMix() { return m_matMix; }

bool Global::m_disableHist = false;
void Global::setDisableHistogram(bool b) { m_disableHist = b; }
bool Global::histogramDisabled() { return m_disableHist; }
