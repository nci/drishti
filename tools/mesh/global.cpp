
#include "global.h"
#include <QFileDialog>
#include <QCoreApplication>
#include <QAction>

QString Global::DrishtiVersion() { return QString(DRISHTI_VERSION); }

bool Global::m_prayogMode = false;
void Global::setPrayogMode(bool b) { m_prayogMode = b; }
bool Global::prayogMode() { return m_prayogMode; }

QString Global::m_helpLanguage = "";
void Global::setHelpLanguage(QString hl) { m_helpLanguage = hl; }
QString Global::helpLanguage() { return m_helpLanguage; }

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

int Global::m_visualizationMode = ImportMode;
int Global::visualizationMode() { return m_visualizationMode; }
void Global::setVisualizationMode(int vm) { m_visualizationMode = vm; }


int Global::m_viewerFilter = _GaussianFilter;
int Global::viewerFilter() { return m_viewerFilter; }
void Global::setViewerFilter(int vf) { m_viewerFilter = vf; }



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
  texSize = qMin(8192, texSize);

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


int Global::m_filteredData = _NoFilter;
int Global::filteredData() {return m_filteredData;}
void Global::setFilteredData(int filter) {m_filteredData = filter;}


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

bool Global::m_drawBox = false;
void Global::setDrawBox(bool db) { m_drawBox = db; }
bool Global::drawBox() { return m_drawBox; }

bool Global::m_drawAxis = false;
void Global::setDrawAxis(bool db) { m_drawAxis = db; }
bool Global::drawAxis() { return m_drawAxis; }

Vec Global::m_bgColor = Vec(230, 230, 230);
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


int Global::m_saveImageType = Global::NoImage;
void Global::setSaveImageType(int si) { m_saveImageType = si; }
int Global::saveImageType() { return m_saveImageType; }

int Global::m_frameNumber = 1;
void Global::setFrameNumber(int f) { m_frameNumber = f; }
int Global::frameNumber() { return m_frameNumber; }


int Global::m_volumeType = Global::DummyVolume;
void Global::setVolumeType(int vt)
{
  m_volumeType = vt;
  calculate3dTextureSize();
}
int Global::volumeType() { return m_volumeType; }



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

int Global::m_textureSizeLimit = 512;
void Global::setTextureSizeLimit(int sz) { m_textureSizeLimit = sz; }
int Global::textureSizeLimit() { return m_textureSizeLimit; }

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

  QImage info(":/images/grab_white.png");
  int texsize = info.height();
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, m_hollowSpriteTexture);
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

  return m_hollowSpriteTexture;

////----------------------------------------
////--- hollow circle sprite ---
//  int texsize = 64;
//  int t2 = texsize/2;
//  QRadialGradient rg(t2, t2, t2-1, t2, t2);
//  rg.setColorAt(0.0, Qt::white);
//  rg.setColorAt(1.0, Qt::black);
//
//  QImage texImage(texsize, texsize, QImage::Format_ARGB32);
//  texImage.fill(0);
//  QPainter p(&texImage);
//  p.setBrush(QBrush(rg));
//  p.setPen(Qt::transparent);
//  p.drawEllipse(0, 0, texsize, texsize);
//
//  uchar *thetexture = new uchar[2*texsize*texsize];
//  const uchar *bits = texImage.bits();
//  //const uchar *bits = info.bits();
//  for(int i=0; i<texsize*texsize; i++)
//    {
//      uchar lum = 255;
//      float a = (float)bits[4*i+2]/255.0f;
//      a = 1-a;
//      
//      if (a < 0.8 || a >= 1.0)
//	{
//	  a = 0;
//	  lum = 0;
//	}
//      else
//	{
//	  lum *= 1-fabs(a-0.8f)/0.2f;
//	  a = 0.9f;
//	}
//      
//      a *= 255;
//      
//      thetexture[2*i] = lum;
//      thetexture[2*i+1] = a;
//    }
////----------------------------------------
//
//  glEnable(GL_TEXTURE_2D);
//  glBindTexture(GL_TEXTURE_2D, m_hollowSpriteTexture);
//  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
//  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
//  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
//  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//  glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA,
//	       texsize, texsize, 0,
//	       GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE,
//	       thetexture);
//  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
//  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
//
//  delete [] thetexture;
//
//
//  return m_hollowSpriteTexture;
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


Vec Global::m_relDataPos = Vec(0,0,0);
void Global::setRelDataPos(Vec rp) { m_relDataPos = rp; }
Vec Global::relDataPos() { return m_relDataPos; }


bool Global::m_hideBlack = false;
void Global::setHideBlack(bool b) { m_hideBlack = b; }
bool Global::hideBlack() { return m_hideBlack; }


bool Global::m_shadowBox = false;
bool Global::shadowBox() { return m_shadowBox; }
void Global::setShadowBox(bool b) { m_shadowBox = b; }

int Global::m_bytesPerVoxel = 1;
void Global::setBytesPerVoxel(int b) { m_bytesPerVoxel = b; }
int Global::bytesPerVoxel() { return (m_bytesPerVoxel); }


int Global::m_voxelUnit = 0;
QStringList Global::m_voxelUnitStrings;
QStringList Global::m_voxelUnitStringsShort;

void Global::setVoxelUnit(int i) { m_voxelUnit = i; }
int Global::voxelUnit() { return m_voxelUnit; }
QString
Global::voxelUnitString()
{
  if (m_voxelUnitStrings.count() == 0)
    {
      m_voxelUnitStrings << "Nounit";
      m_voxelUnitStrings << "Micron";
      m_voxelUnitStrings << "Millimeter";
      m_voxelUnitStrings << "Centimeter";
      m_voxelUnitStrings << "Meter";
    }
  
  if (m_voxelUnit >= 0 &&
      m_voxelUnit < m_voxelUnitStrings.size())
    return m_voxelUnitStrings[m_voxelUnit];
  else
    return m_voxelUnitStrings[0];
}

QString
Global::voxelUnitStringShort()
{
  if (m_voxelUnitStrings.count() == 0)
    {
      m_voxelUnitStringsShort << "";
      //m_voxelUnitStringsShort << QString("%1m").arg(QChar(0xB5));
      m_voxelUnitStringsShort << "um";
      m_voxelUnitStringsShort << "mm";
      m_voxelUnitStringsShort << "cm";
      m_voxelUnitStringsShort << "m";
    }
  
  if (m_voxelUnit >= 0 &&
      m_voxelUnit < m_voxelUnitStringsShort.size())
    return m_voxelUnitStringsShort[m_voxelUnit];
  else
    return m_voxelUnitStringsShort[0];
}

Vec Global::m_voxelSize = Vec(1,1,1);
void Global::setVoxelSize(Vec v) { m_voxelSize = v; }
Vec Global::voxelSize() { return m_voxelSize; }
