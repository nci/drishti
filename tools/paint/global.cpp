#include "global.h"
#include <QFileDialog>
#include <QCoreApplication>

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
  app.cd("paint");

#elif defined(Q_OS_MAC)
  QDir app = QCoreApplication::applicationDirPath();

  app.cdUp();
  app.cdUp();
  app.cdUp();
  app.cd("Shared");
  app.cd("Docs");
  app.cd("paint");

#elif defined(Q_OS_WIN32)

  QDir app = QCoreApplication::applicationDirPath();
  app.cdUp();
  app.cd("docs");
  app.cd("paint");

#else
  #error Unsupported platform.
#endif

  QString page = QFileInfo(app, "drishtipaint.qhc").absoluteFilePath();
  QFileInfo f(page);

  if (f.exists())
    {
      m_documentationPath = f.absolutePath();
    }
  else
    {
      QString path;
      path = QFileDialog::getExistingDirectory(0,
			  "Drishti Paint Documentation Directory",
			   QCoreApplication::applicationDirPath());
      if (path.isEmpty() == false)
	m_documentationPath = path;
      else
	m_documentationPath = "dontask";
    }
  m_documentationPath = QDir(m_documentationPath).canonicalPath();
  return m_documentationPath;
}


int Global::m_lutSize = 1; // 1 lookup table
int Global::lutSize() {return m_lutSize;}
void Global::setLutSize(int sz) {m_lutSize = sz;}

uchar* Global::m_lut = 0;
uchar* Global::lut()
{
  if (!m_lut)
    {
      m_lut = new uchar[4*256*256];
      memset(m_lut, 0, 4*256*256);
    }
  return m_lut;
}
void Global::setLut(uchar *lt)
{
  if (!m_lut)
    m_lut = new uchar[4*256*256];
  memcpy(m_lut, lt, 4*256*256);
}


bool Global::m_use1D = true;
bool Global::use1D() { return m_use1D; }
void Global::setUse1D(bool flag) { m_use1D = flag; }

QString Global::m_previousDirectory = "";
QString Global::previousDirectory() {return m_previousDirectory;}
void Global::setPreviousDirectory(QString d) {m_previousDirectory = d;}

uchar* Global::m_tagColors = 0;
uchar* Global::tagColors()
{
  if (!m_tagColors)
    {
      m_tagColors = new uchar[1024];
      memset(m_tagColors, 255, 1024);
    }
  return m_tagColors;
}
void Global::setTagColors(uchar *colors)
{
  if (!m_tagColors)
    {
      m_tagColors = new uchar[1024];
      memset(m_tagColors, 255, 1024);
    }
  memcpy(m_tagColors, colors, 1024);
}

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


bool Global::m_line = false;
bool Global::line() { return m_line; }
void Global::setLine(bool l) { m_line = l; }

int Global::m_tag = 1;
int Global::tag() { return m_tag; }
void Global::setTag(int t) { m_tag = t; }

int Global::m_boxSize = 5;
int Global::boxSize() { return m_boxSize; }
void Global::setBoxSize(int d) { m_boxSize = qMax(1, d); }

int Global::m_lambda = 10;
int Global::lambda() { return m_lambda; }
void Global::setLambda(int d) { m_lambda = qMax(1, d); }

int Global::m_spread = 10;
int Global::spread() { return m_spread; }
void Global::setSpread(int d) { m_spread = qMax(1, d); }

bool Global::m_tagSimilar = false;
bool Global::tagSimilar() { return m_tagSimilar; }
void Global::setTagSimilar(bool t) { m_tagSimilar = t; }

int Global::m_prevErode = 5;
int Global::prevErode() { return m_prevErode; }
void Global::setPrevErode(int d) { m_prevErode = d; }

bool Global::m_copyPrev = false;
bool Global::copyPrev() { return m_copyPrev; }
void Global::setCopyPrev(bool l) { m_copyPrev = l; }

int Global::m_smooth = 1;
int Global::smooth() { return m_smooth; }
void Global::setSmooth(int d) { m_smooth = d; }

int Global::m_thickness = 1;
int Global::thickness() { return m_thickness; }
void Global::setThickness(int d) { m_thickness = d; }

bool Global::m_closed = true;
bool Global::closed() { return m_closed; }
void Global::setClosed(bool b) { m_closed = b; }

int Global::m_selpres = 15;
int Global::selectionPrecision() { return m_selpres; }
void Global::setSelectionPrecision(int s) { m_selpres = s; }

int Global::m_bytesPerVoxel = 1;
void Global::setBytesPerVoxel(int b) { m_bytesPerVoxel = b; }
int Global::bytesPerVoxel() { return (m_bytesPerVoxel); }

Vec Global::m_voxelScaling = Vec(1,1,1);
Vec Global::m_relativeVoxelScaling = Vec(1,1,1);
Vec Global::voxelScaling() { return m_voxelScaling; }
void Global::setVoxelScaling(Vec v)
{
  m_voxelScaling = v;
  float mvs = qMin(v.x, qMin(v.y, v.z));
  m_relativeVoxelScaling = v/mvs;

}
Vec Global::relativeVoxelScaling() { return m_relativeVoxelScaling; }


QString Global::m_voxelUnit = "";
QString Global::voxelUnit() { return m_voxelUnit; }
void Global::setVoxelUnit(QString s) { m_voxelUnit = s; }

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
