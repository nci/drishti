#include "global.h"
#include "pathobject.h"
#include "staticfunctions.h"
#include "volumeinformation.h"

#include <QFileDialog>

//------------------------------------------------------------------
PathObjectUndo::PathObjectUndo() { clear(); }
PathObjectUndo::~PathObjectUndo() { clear(); }

void
PathObjectUndo::clear()
{
  m_points.clear();
  m_pointRadX.clear();
  m_pointRadY.clear();
  m_pointAngle.clear();
  m_index = -1;
}

void
PathObjectUndo::clearTop()
{
  if (m_index == m_points.count()-1)
    return;

  while(m_index < m_points.count()-1)
    m_points.removeLast();
  
  while(m_index < m_pointRadX.count()-1)
    m_pointRadX.removeLast();  

  while(m_index < m_pointRadY.count()-1)
    m_pointRadY.removeLast();
  
  while(m_index < m_pointAngle.count()-1)
    m_pointAngle.removeLast();
  
}

void
PathObjectUndo::append(QList<Vec> p, QList<float> px, QList<float> py, QList<float> pa)
{
  clearTop();

  m_points << p;
  m_pointRadX << px;
  m_pointRadY << py;
  m_pointAngle << pa;

  m_index = m_points.count()-1;
}

void PathObjectUndo::redo() { m_index = qMin(m_index+1, m_points.count()-1); }
void PathObjectUndo::undo() { m_index = qMax(m_index-1, 0); }

QList<Vec>
PathObjectUndo::points()
{
  QList<Vec> p;

  if (m_index >= 0 && m_index < m_points.count())
    return m_points[m_index];

  return p;
}

QList<float>
PathObjectUndo::pointRadX()
{
  QList<float> p;

  if (m_index >= 0 && m_index < m_pointRadX.count())
    return m_pointRadX[m_index];

  return p;
}

QList<float>
PathObjectUndo::pointRadY()
{
  QList<float> p;

  if (m_index >= 0 && m_index < m_pointRadY.count())
    return m_pointRadY[m_index];

  return p;
}

QList<float>
PathObjectUndo::pointAngle()
{
  QList<float> p;

  if (m_index >= 0 && m_index < m_pointAngle.count())
    return m_pointAngle[m_index];

  return p;
}
//------------------------------------------------------------------


bool
PathObject::hasCaption(QStringList str)
{
  if (m_captionPresent)
    {
      for(int i=0; i<str.count(); i++)
	if (m_captionText.contains(str[i], Qt::CaseInsensitive) == false)
	  return false;
      return true;
    }

  return false;
}

bool PathObject::viewportStyle() { return m_viewportStyle; }
void PathObject::setViewportStyle(bool s) { m_viewportStyle = s; }

int PathObject::viewportTF() { return m_viewportTF; }
void PathObject::setViewportTF(int tf) { m_viewportTF = tf; }

bool PathObject::viewportGrabbed() { return m_viewportGrabbed; }
void PathObject::setViewportGrabbed(bool v) { m_viewportGrabbed = v; }

QVector4D PathObject::viewport() { return m_viewport; }
void PathObject::setViewport(QVector4D v) { m_viewport = v; }

Vec PathObject::viewportCamPos() { return m_viewportCamPos; }
void PathObject::setViewportCamPos(Vec v) { m_viewportCamPos = v; }

float PathObject::viewportCamRot() { return m_viewportCamRot; }
void PathObject::setViewportCamRot(float v) { m_viewportCamRot = v; }

QString PathObject::imageName() { return m_imageName; }
void PathObject::setImage(QString v) { loadImage(v); }

QString PathObject::captionText() { return m_captionText; }
QFont PathObject::captionFont() { return m_captionFont; }
QColor PathObject::captionColor() { return m_captionColor; }
QColor PathObject::captionHaloColor() { return m_captionHaloColor; }

bool PathObject::captionPresent() { return m_captionPresent; }
bool PathObject::captionLabel() { return m_captionLabel; }
void PathObject::setCaptionLabel(bool cl) { m_captionLabel = cl; }

bool PathObject::captionGrabbed() { return m_captionGrabbed; }
void PathObject::setCaptionGrabbed(bool cg) { m_captionGrabbed = cg; }

void
 PathObject::imageSize(int& wd, int& ht)
{
  wd = m_textureWidth;
  ht = m_textureHeight;
}

void PathObject::setCaption(QString ct, QFont cf,
			    QColor cc, QColor chc)
{
  bool doit = false;
  if (m_captionText != ct) doit = true;
  if (m_captionFont != cf) doit = true;
  if (m_captionColor != cc) doit = true;
  if (m_captionHaloColor != chc) doit = true;

  if (doit)
    loadCaption(ct, cf, cc, chc);
}

void
PathObject::resetCaption()
{
  m_captionPresent = false;
  m_captionLabel = true;
  m_captionText.clear();
  m_captionFont = QFont("Helvetica", 12);
  m_captionColor = Qt::white;
  m_captionHaloColor = Qt::black;

  if (!m_imagePresent && m_imageTex)
    {
      glDeleteTextures(1, &m_imageTex);
      m_imageTex = 0;
    }
}

void
PathObject::loadCaption(QString ct, QFont cf,
			QColor cc, QColor chc)
{
  m_captionText = ct;
  m_captionFont = cf;
  m_captionColor = cc;
  m_captionHaloColor = chc;

  if (m_captionText.isEmpty())
    {
      if (m_imageTex) glDeleteTextures(1, &m_imageTex);
      m_imageTex = 0;

      m_captionPresent = false;
      m_captionLabel = true;
      return;
    }

  QFontMetrics metric(m_captionFont);
  int mde = metric.descent();
  int fht = metric.height();
  int fwd = metric.width(m_captionText)+2;

  // replace symbol for micron
  QString ctstr = m_captionText;
  if (ctstr.endsWith(" um"))
    {
      ctstr.chop(2);
      ctstr += QChar(0xB5);
      ctstr += "m";
    }


  //-------------------
  QImage bImage = QImage(fwd, fht, QImage::Format_ARGB32);
  bImage.fill(0);
  {
    QPainter bpainter(&bImage);
    // we have image as ARGB, but we want RGBA
    // so switch red and blue colors here itself
    QColor penColor(m_captionHaloColor.blue(),
		    m_captionHaloColor.green(),
		    m_captionHaloColor.red());
    // do not use alpha(),
    // opacity will be modulated using clip-plane's opacity parameter  
    bpainter.setPen(penColor);
    bpainter.setFont(m_captionFont);
    //bpainter.drawText(1, fht-mde, m_captionText);
    bpainter.drawText(1, fht-mde, ctstr);
    
    uchar *dbits = new uchar[4*fht*fwd];
    uchar *bits = bImage.bits();
    for(int nt=0; nt < 4; nt++)
      {
	memcpy(dbits, bits, 4*fht*fwd);

	for(int i=2; i<fht-2; i++)
	  for(int j=2; j<fwd-2; j++)
	    {
	      for (int k=0; k<4; k++)
		{
		  int sum = 0;
		  
		  for(int i0=-2; i0<=2; i0++)
		    for(int j0=-2; j0<=2; j0++)
		      sum += dbits[4*((i+i0)*fwd+(j+j0)) + k];
		  
		  bits[4*(i*fwd+j) + k] = sum/25;
		}
	    }
      }
    delete [] dbits;
  }
  //-------------------

  QImage cImage = QImage(fwd, fht, QImage::Format_ARGB32);
  cImage.fill(0);
  QPainter cpainter(&cImage);

  // first draw the halo image
  cpainter.drawImage(0, 0, bImage);


  // we have image as ARGB, but we want RGBA
  // so switch red and blue colors here itself
  QColor penColor(m_captionColor.blue(),
		  m_captionColor.green(),
		  m_captionColor.red());
  // do not use alpha(),
  // opacity will be modulated using clip-plane's opacity parameter  
  cpainter.setPen(penColor);
  cpainter.setFont(m_captionFont);
  //cpainter.drawText(1, fht-mde, m_captionText);  
  cpainter.drawText(1, fht-mde, ctstr);

  m_textureWidth = fwd;
  m_textureHeight = fht;

  m_cImage = cImage.mirrored();
      
  if (m_imageTex) glDeleteTextures(1, &m_imageTex);
  glGenTextures(1, &m_imageTex);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_imageTex);
  glTexParameterf(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
  glTexParameterf(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_RECTANGLE_ARB,
	       0,
	       4,
	       m_textureWidth,
	       m_textureHeight,
	       0,
	       GL_RGBA,
	       GL_UNSIGNED_BYTE,
	       m_cImage.bits());
  glDisable(GL_TEXTURE_RECTANGLE_ARB);

  m_imagePresent = false;
  m_imageName = QString();

  m_captionPresent = true;

  // generate the displaylist
  glNewList(m_displayListCaption, GL_COMPILE);
  generateRibbon(1.0);
  glEndList();
}

void
PathObject::resetImage()
{
  m_imagePresent = false;
  m_imageName = QString();

  if (!m_captionPresent && m_imageTex)
    {
      glDeleteTextures(1, &m_imageTex);
      m_imageTex = 0;
    }
}

void
PathObject::loadImage()
{
  QString imgFile = QFileDialog::getOpenFileName(0,
	      QString("Load image to map on the clip plane"),
	      Global::previousDirectory(),
              "Image Files (*.png *.tif *.bmp *.jpg *.gif)");

  if (imgFile.isEmpty())
    return;

  QFileInfo f(imgFile);
  if (f.exists() == false)
    return;

  Global::setPreviousDirectory(f.absolutePath());


  VolumeInformation pvlInfo = VolumeInformation::volumeInformation();
  QFileInfo fileInfo(pvlInfo.pvlFile);
  imgFile = fileInfo.absoluteDir().relativeFilePath(imgFile);

  loadImage(imgFile);
}

void
PathObject::loadImage(QString imgFile)
{
  if (m_imageName == imgFile)
    return;

  m_imageName = imgFile;

  if (m_imageName.isEmpty())
    {
      if (m_imageTex) glDeleteTextures(1, &m_imageTex);
      m_imageTex = 0;
      m_imagePresent = false;
      return;
    }

  //----------------
  // file is assumed to be relative to .pvl.nc file
  // get the absolute path
  VolumeInformation pvlInfo = VolumeInformation::volumeInformation();
  QFileInfo fileInfo(pvlInfo.pvlFile);
  QString absoluteImageFile = QFileInfo(fileInfo.absolutePath(),
					m_imageName).absoluteFilePath();
  //----------------

  QFileInfo f(absoluteImageFile);
  if (f.exists() == false)
    {
      m_textureHeight = m_textureWidth = 10;      

      resetImage();
      return;
    }

  QMovie movie(absoluteImageFile);
  movie.setCacheMode(QMovie::CacheAll);
  movie.start();
  movie.setPaused(true);
  movie.jumpToFrame(0);

  QImage mapImage(movie.currentImage());
  mapImage = mapImage.convertToFormat(QImage::Format_ARGB32);
  m_textureHeight = mapImage.height();
  m_textureWidth = mapImage.width();
  int nbytes = mapImage.byteCount();
  int rgb = nbytes/(m_textureWidth*m_textureHeight);

  m_image = mapImage;

  GLuint fmt;
  if (rgb == 1) fmt = GL_LUMINANCE;
  else if (rgb == 2) fmt = GL_LUMINANCE_ALPHA;
  else if (rgb == 3) fmt = GL_RGB;
  else if (rgb == 4) fmt = GL_BGRA;

  if (m_imageTex) glDeleteTextures(1, &m_imageTex);
  glGenTextures(1, &m_imageTex);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_imageTex);
  glTexParameterf(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
  glTexParameterf(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_RECTANGLE_ARB,
	       0,
	       rgb,
	       m_textureWidth,
	       m_textureHeight,
	       0,
	       fmt,
	       GL_UNSIGNED_BYTE,
	       m_image.bits());
  glDisable(GL_TEXTURE_RECTANGLE_ARB);

  m_imagePresent = true;

  resetCaption();


  // generate the displaylist
  glNewList(m_displayListCaption, GL_COMPILE);
  generateRibbon(1.0);
  glEndList();
}


PathObject
PathObject::get()
{
  PathObject po;
  po = *this;
  return po;
}

void
PathObject::set(const PathObject &po)
{
  m_pointPressed = -1;

  if (m_imageTex) glDeleteTextures(1, &m_imageTex);
  if (m_displayList > 0) glDeleteLists(m_displayList, 1);
  if (m_displayListCaption > 0) glDeleteLists(m_displayListCaption, 1);

  m_imageTex = 0;
  m_displayList = 0;
  m_displayListCaption = 0;

  m_updateFlag = true; // for recomputation

  m_showPointNumbers = po.m_showPointNumbers;
  m_showPoints = po.m_showPoints;
  m_showLength = po.m_showLength;
  m_showAngle = po.m_showAngle;
  m_tube = po.m_tube;
  m_closed = po.m_closed;
  m_sections = po.m_sections;
  m_segments = po.m_segments;
  m_color = po.m_color;
  m_lengthColor = po.m_lengthColor;
  m_opacity = po.m_opacity;
  m_arrowHeadLength = po.m_arrowHeadLength;
  m_lengthTextDistance = po.m_lengthTextDistance;
  m_points = po.m_points;
  m_pointRadX = po.m_pointRadX;
  m_pointRadY = po.m_pointRadY;
  m_pointAngle = po.m_pointAngle;
  m_capType = po.m_capType;
  m_arrowDirection = po.m_arrowDirection;
  m_arrowForAll = po.m_arrowForAll;
  m_halfSection = po.m_halfSection;
  m_useType = po.m_useType;
  m_keepInside = po.m_keepInside;
  m_keepEnds = po.m_keepEnds;
  m_blendTF = po.m_blendTF;
  m_allowInterpolate = po.m_allowInterpolate;

  m_imagePresent = po.m_imagePresent;
  m_imageName = QString();
  QString imgName = po.m_imageName;

  m_captionPresent = po.m_captionPresent;
  m_captionLabel = po.m_captionLabel;
  m_captionText = po.m_captionText;
  m_captionFont = po.m_captionFont;
  m_captionColor = po.m_captionColor;
  m_captionHaloColor = po.m_captionHaloColor;

  // we take m_sections to be multiple of 4
  m_sections = (m_sections/4)*4;

  m_viewportStyle = po.m_viewportStyle;
  m_viewportTF = po.m_viewportTF;
  m_viewport = po.m_viewport;
  m_viewportGrabbed = po.m_viewportGrabbed;
  m_viewportCamPos = po.m_viewportCamPos;
  m_viewportCamRot = po.m_viewportCamRot;

  computeTangents();

  if (m_captionPresent)
    loadCaption(m_captionText,
		m_captionFont,
		m_captionColor,
		m_captionHaloColor);  
  else if (m_imagePresent)
    loadImage(imgName);

  m_undo.clear();
  m_undo.append(m_points, m_pointRadX, m_pointRadY, m_pointAngle);
}

PathObject&
PathObject::operator=(const PathObject &po)
{
  //m_pointPressed = -1;
  m_pointPressed = po.m_pointPressed;

  if (m_imageTex) glDeleteTextures(1, &m_imageTex);
  if (m_displayList > 0) glDeleteLists(m_displayList, 1);
  if (m_displayListCaption > 0) glDeleteLists(m_displayListCaption, 1);

  m_imageTex = 0;
  m_displayList = 0;
  m_displayListCaption = 0;

  m_updateFlag = true; // for recomputation

  m_showPointNumbers = po.m_showPointNumbers;
  m_showPoints = po.m_showPoints;
  m_showLength = po.m_showLength;
  m_showAngle = po.m_showAngle;
  m_tube = po.m_tube;
  m_closed = po.m_closed;
  m_sections = po.m_sections;
  m_segments = po.m_segments;
  m_color = po.m_color;
  m_lengthColor = po.m_lengthColor;
  m_opacity = po.m_opacity;
  m_arrowHeadLength = po.m_arrowHeadLength;
  m_lengthTextDistance = po.m_lengthTextDistance;
  m_points = po.m_points;
  m_pointRadX = po.m_pointRadX;
  m_pointRadY = po.m_pointRadY;
  m_pointAngle = po.m_pointAngle;
  m_capType = po.m_capType;
  m_arrowDirection = po.m_arrowDirection;
  m_arrowForAll = po.m_arrowForAll;
  m_halfSection = po.m_halfSection;
  m_useType = po.m_useType;
  m_keepInside = po.m_keepInside;
  m_keepEnds = po.m_keepEnds;
  m_blendTF = po.m_blendTF;
  m_allowInterpolate = po.m_allowInterpolate;

  m_imagePresent = po.m_imagePresent;
  m_imageName = QString();
  QString imgName = po.m_imageName;

  m_captionPresent = po.m_captionPresent;
  m_captionLabel = po.m_captionLabel;
  m_captionText = po.m_captionText;
  m_captionFont = po.m_captionFont;
  m_captionColor = po.m_captionColor;
  m_captionHaloColor = po.m_captionHaloColor;

  // we take m_sections to be multiple of 4
  m_sections = (m_sections/4)*4;

  m_viewportStyle = po.m_viewportStyle;
  m_viewportTF = po.m_viewportTF;
  m_viewport = po.m_viewport;
  m_viewportGrabbed = po.m_viewportGrabbed;
  m_viewportCamPos = po.m_viewportCamPos;
  m_viewportCamRot = po.m_viewportCamRot;

  computeTangents();

  if (m_captionPresent)
    loadCaption(m_captionText,
		m_captionFont,
		m_captionColor,
		m_captionHaloColor);
  else if (m_imagePresent)
    loadImage(imgName);

  m_undo.clear();
  m_undo.append(m_points, m_pointRadX, m_pointRadY, m_pointAngle);

  return *this;
}

PathObject::PathObject()
{
  m_undo.clear();

  m_pointPressed = -1;

  m_useType = 0;
  m_keepInside = true;
  m_keepEnds = true;
  m_blendTF = 1;

  m_add = false;
  m_allowInterpolate = false;
  m_showPointNumbers = false;
  m_showPoints = true;
  m_showLength = false;
  m_showAngle = false;
  m_updateFlag = false;
  m_tube = false;
  m_closed = false;
  m_sections = 4;
  m_segments = 1;
  m_color = Vec(0.9,0.5,0.2);
  m_lengthColor = Vec(0.9,0.5,0.2);
  m_opacity = 1;
  m_arrowHeadLength = 10;
  m_lengthTextDistance = 10;
  m_points.clear();
  m_pointRadX.clear();
  m_pointRadY.clear();
  m_pointAngle.clear();
  m_capType = FLAT;
  m_arrowDirection = true;
  m_arrowForAll = false;
  m_halfSection = false;
  
  m_length = 0;
  m_tgP.clear();
  m_xaxis.clear();
  m_yaxis.clear();
  m_path.clear();
  m_radX.clear();
  m_radY.clear();
  m_angle.clear();
  m_pathT.clear();
  m_pathX.clear();
  m_pathY.clear();

  m_imagePresent = false;
  m_imageName = QString();

  m_captionGrabbed = false;
  m_captionPresent = false;
  m_captionLabel = true;
  m_captionText.clear();
  m_captionFont = QFont("Helvetica", 12);
  m_captionColor = Qt::white;
  m_captionHaloColor = Qt::black;

  m_imageTex = 0;
  m_displayList = 0;
  m_displayListCaption = 0;

  m_viewportStyle = true; // horizontal;
  m_viewportTF = -1;
  m_viewportGrabbed = false;
  m_viewport = QVector4D(-1,-1,-1,-1);
  m_viewportCamPos = Vec(0, 0, 0);
  m_viewportCamRot = 0.0;
}

PathObject::~PathObject()
{
  m_undo.clear();

  m_useType = 0;
  m_keepInside = true;
  m_keepEnds = true;
  m_blendTF = 1;

  m_points.clear();
  m_pointRadX.clear();
  m_pointRadY.clear();
  m_pointAngle.clear();
  m_tgP.clear();
  m_xaxis.clear();
  m_yaxis.clear();
  m_path.clear();
  m_radX.clear();
  m_radY.clear();
  m_angle.clear();
  m_pathT.clear();
  m_pathX.clear();
  m_pathY.clear();

  if (m_imageTex) glDeleteTextures(1, &m_imageTex);
  if (m_displayList > 0) glDeleteLists(m_displayList, 1);
  if (m_displayListCaption > 0) glDeleteLists(m_displayListCaption, 1);

  m_imageTex = 0;
  m_displayList = 0;
  m_displayListCaption = 0;
}

void
PathObject::translate(bool moveX, bool indir)
{
  QList<Vec> delta;

  int npoints = m_points.count();
  for (int i=0; i<npoints; i++)
    {
      if (moveX)
	delta << m_radX[i*m_segments]*m_pathX[i*m_segments];
      else
	delta << m_radY[i*m_segments]*m_pathY[i*m_segments];
    }	  

  if (indir)
    {
      for(int i=0; i<npoints; i++)
	m_points[i] += delta[i];
    }
  else
    {
      for(int i=0; i<npoints; i++)
	m_points[i] -= delta[i];
    }

  computeTangents();
  m_updateFlag = true;

  m_undo.append(m_points, m_pointRadX, m_pointRadY, m_pointAngle);
}

void
PathObject::translate(int idx, int moveType, float mag)
{
  int npoints = m_points.count();

  if (moveType == 0) // move along y-axis
    {
      if (idx<0 || idx>=npoints)
	{
	  for(int i=0; i<npoints; i++)
	    m_points[i] += mag*m_pathY[i*m_segments];
	}
      else
	m_points[idx] += mag*m_pathY[idx*m_segments];
    }
  else if (moveType == 1) // move along x-axis
    {
      if (idx<0 || idx>=npoints)
	{
	  for(int i=0; i<npoints; i++)
	    m_points[i] += mag*m_pathX[i*m_segments];
	}
      else
	m_points[idx] += mag*m_pathX[idx*m_segments];
    }
  else // move along tangent
    {
      if (idx<0 || idx>=npoints)
	{
	  for(int i=0; i<npoints; i++)
	    m_points[i] += mag*m_pathT[i*m_segments];
	}
      else
	m_points[idx] += mag*m_pathT[idx*m_segments];
    }

  computeTangents();
  m_updateFlag = true;

  m_undo.append(m_points, m_pointRadX, m_pointRadY, m_pointAngle);
}

void
PathObject::rotate(int idx, float mag)
{
  int npoints = m_points.count();

  if (idx<0 || idx>=npoints)
    {
      for(int i=0; i<npoints; i++)
	m_pointAngle[i] += mag;
    }
  else
    m_pointAngle[idx] += mag;

  computeTangents();
  m_updateFlag = true;

  m_undo.append(m_points, m_pointRadX, m_pointRadY, m_pointAngle);
}

void PathObject::setAdd(bool a) { m_add = a; }
bool PathObject::add() { return m_add; }

int PathObject::useType() { return m_useType; }
void PathObject::setUseType(int t) { m_useType = t; }
bool PathObject::crop() { return (m_useType > 0 && m_useType < 5); }
bool PathObject::blend() { return (m_useType > 4); }

int PathObject::blendTF() { return m_blendTF; }
void PathObject::setBlendTF(int tf) { m_blendTF = tf; }

bool PathObject::keepInside() { return m_keepInside; }
void PathObject::setKeepInside(bool c) { m_keepInside = c; }

bool PathObject::keepEnds() { return m_keepEnds; }
void PathObject::setKeepEnds(bool c) { m_keepEnds = c; }

bool PathObject::allowInterpolate() { return m_allowInterpolate; }
void PathObject::setAllowInterpolate(bool a) { m_allowInterpolate = a; }

bool PathObject::showPointNumbers() { return m_showPointNumbers; }
bool PathObject::showPoints() { return m_showPoints; }
bool PathObject::showLength() { return m_showLength; }
bool PathObject::showAngle() { return m_showAngle; }
bool PathObject::arrowDirection() { return m_arrowDirection; }
bool PathObject::arrowForAll() { return m_arrowForAll; }
bool PathObject::halfSection() { return m_halfSection; }
int PathObject::capType() { return m_capType; }
bool PathObject::tube() { return m_tube; }
bool PathObject::closed() { return m_closed; }
Vec PathObject::color() { return m_color; }
Vec PathObject::lengthColor() { return m_lengthColor; }
float PathObject::opacity() { return m_opacity; }
float PathObject::arrowHeadLength() { return m_arrowHeadLength; }
int PathObject::lengthTextDistance() { return m_lengthTextDistance; }
QList<Vec> PathObject::points() { return m_points; }
QList<Vec> PathObject::saxis() { return m_xaxis; }
QList<Vec> PathObject::taxis() { return m_yaxis; }
Vec PathObject::getPoint(int i)
{
  if (i >= 0 && i < m_points.count())
    return m_points[i];
  else
    return Vec(0,0,0);
}
float PathObject::getRadX(int i)
{
  if (i >= 0 && i < m_points.count())
    return m_pointRadX[i];
  else
    return 0;
}
float PathObject::getRadY(int i)
{
  if (i >= 0 && i < m_points.count())
    return m_pointRadY[i];
  else
    return 0;
}
float PathObject::getAngle(int i)
{
  if (i >= 0 && i < m_points.count())
    return m_pointAngle[i];
  else
    return 0;
}
QList<Vec> PathObject::tangents()
{
  if (m_updateFlag)
    computePathLength();
  return m_tgP;
}
QList<float> PathObject::pathradX()
{
  if (m_updateFlag)
    computePathLength();
  return m_radX;
}
QList<float> PathObject::pathradY()
{
  if (m_updateFlag)
    computePathLength();
  return m_radY;
}
QList<Vec> PathObject::pathPoints()
{
  if (m_updateFlag)
    computePathLength();
  return m_path;
}
QList<Vec> PathObject::pathT()
{
  if (m_updateFlag)
    computePathLength();
  return m_pathT;
}
QList<Vec> PathObject::pathX()
{
  if (m_updateFlag)
    computePathLength();
  return m_pathX;
}
QList<Vec> PathObject::pathY()
{
  if (m_updateFlag)
    computePathLength();
  return m_pathY;
}
QList<float> PathObject::pathAngles()
{
  if (m_updateFlag)
    computePathLength();
  return m_angle;
}
QList<float> PathObject::radX() { return m_pointRadX; }
QList<float> PathObject::radY() { return m_pointRadY; }
QList<float> PathObject::angle() { return m_pointAngle; }
int PathObject::segments() { return m_segments; }
int PathObject::sections() { return m_sections; }
float PathObject::length()
{
  if (m_updateFlag)
    computePathLength();
  return m_length;
}

void PathObject::setShowPointNumbers(bool flag) { m_showPointNumbers = flag; }
void PathObject::setShowPoints(bool flag) { m_showPoints = flag; }
void PathObject::setShowLength(bool flag) { m_showLength = flag; }
void PathObject::setShowAngle(bool flag) { m_showAngle = flag; }
void PathObject::setArrowDirection(bool flag)
{
  m_arrowDirection = flag;
  m_updateFlag = true;
}
void PathObject::setArrowForAll(bool flag)
{
  m_arrowForAll = flag;
  m_updateFlag = true;
}
void PathObject::setHalfSection(bool flag)
{
  m_halfSection = flag;
  m_updateFlag = true;
}
void PathObject::setCapType(int ct)
{
  m_capType = ct;
  m_updateFlag = true;
}
void PathObject::setTube(bool flag)
{
  m_tube = flag;
}
void PathObject::setClosed(bool flag)
{
  m_closed = flag;
  m_updateFlag = true;
}
void PathObject::setColor(Vec color)
{
  m_color = color;
}
void PathObject::setLengthColor(Vec color)
{
  m_lengthColor = color;
}
void PathObject::setOpacity(float op)
{
  m_opacity = op;
}
void PathObject::setArrowHeadLength(float ahl)
{
  m_arrowHeadLength = ahl;
  m_updateFlag = true;
}
void PathObject::setLengthTextDistance(int ltd)
{
  m_lengthTextDistance = ltd;
}
void PathObject::setPoint(int i, Vec pt)
{
  if (i >= 0 && i < m_points.count())
    {
      m_points[i] = pt;
      computeTangents();
      m_updateFlag = true;
    }
  else
    {
      for(int j=0; j<m_points.count(); j++)
	m_points[j] += pt;
      m_updateFlag = true;
    }
}
void PathObject::setRadX(int i, float v, bool sameForAll)
{
  if (i < m_points.count())
    {      
      if (sameForAll)
	{
	  for(int j=0; j<m_points.count(); j++)
	    m_pointRadX[j] = v;	    
	}
      else
	m_pointRadX[i] = v;

      m_updateFlag = true;
    }
}
void PathObject::setRadY(int i, float v, bool sameForAll)
{
  if (i < m_points.count())
    {
      if (sameForAll)
	{
	  for(int j=0; j<m_points.count(); j++)
	    m_pointRadY[j] = v;	    
	}
      else
	m_pointRadY[i] = v;

      m_updateFlag = true;
    }
  m_undo.append(m_points, m_pointRadX, m_pointRadY, m_pointAngle);
}
void PathObject::setAngle(int i, float v, bool sameForAll)
{
  if (i < m_points.count())
    {

      if (sameForAll)
	{
	  for(int j=0; j<m_points.count(); j++)
	    m_pointAngle[j] = v;
	}
      else
	m_pointAngle[i] = v;

      m_updateFlag = true;
    }
  m_undo.append(m_points, m_pointRadX, m_pointRadY, m_pointAngle);
}
void PathObject::setSameForAll(bool flag)
{
  if (flag)
    {
      float x = m_pointRadX[0];
      float y = m_pointRadY[0];
      float a = m_pointAngle[0];
      for(int j=0; j<m_points.count(); j++)
	{
	  m_pointRadX[j] = x;
	  m_pointRadY[j] = y;
	  m_pointAngle[j] = a;
	}
    }
  m_undo.append(m_points, m_pointRadX, m_pointRadY, m_pointAngle);
}
void PathObject::normalize()
{
  for(int i=0; i<m_points.count(); i++)
    {
      Vec pt = m_points[i];
      pt = Vec((int)pt.x, (int)pt.y, (int)pt.z);
      m_points[i] = pt;
    }
  m_updateFlag = true;
  m_undo.append(m_points, m_pointRadX, m_pointRadY, m_pointAngle);
}
void PathObject::replace(QList<Vec> pts,
			 QList<float> rdX,
			 QList<float> rdY,
			 QList<float> a)
{
  if (pts.count() == m_points.count())
    {
      m_points.clear();
      m_pointRadX.clear();
      m_pointRadY.clear();
      m_pointAngle.clear();

      m_points.append(pts);
      m_pointRadX.append(rdX);
      m_pointRadY.append(rdY);
      m_pointAngle.append(a);

      m_updateFlag = true;
      m_undo.append(m_points, m_pointRadX, m_pointRadY, m_pointAngle);
    }
}
void PathObject::flip()
{
  int npoints = m_points.count();
  for (int i=0; i<npoints/2; i++)
    {
      Vec pt = m_points[i];
      float px = m_pointRadX[i];
      float py = m_pointRadY[i];
      float pa = m_pointAngle[i];

      m_points[i] = m_points[npoints-1-i];
      m_pointRadX[i] = m_pointRadX[npoints-1-i];
      m_pointRadY[i] = m_pointRadY[npoints-1-i];
      m_pointAngle[i] = m_pointAngle[npoints-1-i];

      m_points[npoints-1-i] = pt;
      m_pointRadX[npoints-1-i] = px;
      m_pointRadY[npoints-1-i] = py;
      m_pointAngle[npoints-1-i] = pa;
    }

  computeTangents();

  m_updateFlag = true;
  m_undo.append(m_points, m_pointRadX, m_pointRadY, m_pointAngle);  
}
void PathObject::setPoints(QList<Vec> pts)
{
  m_pointPressed = -1;

  if (pts.count() < 2)
    {
      QMessageBox::information(0, QString("%1 points").arg(pts.count()),
			       "Number of points must be greater than 1");
      return;
    }

  m_points = pts;
  
  m_pointRadX.clear();
  m_pointRadY.clear();
  m_pointAngle.clear();
  for(int i=0; i<pts.count(); i++)
    {
      m_pointRadX.append(1);
      m_pointRadY.append(1);
      m_pointAngle.append(0);
    }

  computeTangents();

  m_updateFlag = true;
  m_undo.append(m_points, m_pointRadX, m_pointRadY, m_pointAngle);
}
void PathObject::setRadX(QList<float> rad)
{
  m_pointRadX = rad;  
  m_updateFlag = true;
  m_undo.append(m_points, m_pointRadX, m_pointRadY, m_pointAngle);
}
void PathObject::setRadY(QList<float> rad)
{
  m_pointRadY = rad;  
  m_updateFlag = true;
  m_undo.append(m_points, m_pointRadX, m_pointRadY, m_pointAngle);
}
void PathObject::setAngle(QList<float> rad)
{
  m_pointAngle = rad;  
  m_updateFlag = true;
  m_undo.append(m_points, m_pointRadX, m_pointRadY, m_pointAngle);
}
void PathObject::setSegments(int seg)
{
  m_segments = qMax(1, seg);
  m_updateFlag = true;
}
void PathObject::setSections(int sec)
{
  m_sections = qMax(4, sec);
  // we take m_sections to be multiple of 4
  m_sections = (m_sections/4)*4;
  m_updateFlag = true;
}

void PathObject::setPointPressed(int p) { m_pointPressed = p; }
int PathObject::getPointPressed() { return m_pointPressed; }

void
PathObject::removePoint(int idx)
{
  if (m_points.count() <= 2 ||
      idx >= m_points.count())
    return;

  m_pointPressed = -1;

  m_points.removeAt(idx);
  m_pointRadX.removeAt(idx);
  m_pointRadY.removeAt(idx);
  m_pointAngle.removeAt(idx);

  computeTangents();

  m_updateFlag = true;

  m_undo.append(m_points, m_pointRadX, m_pointRadY, m_pointAngle);
}

void
PathObject::addPoint(Vec pt)
{
  int npts = m_points.count();

  m_points.append(pt);
  m_pointRadX.append(m_pointRadX[npts-1]);
  m_pointRadY.append(m_pointRadY[npts-1]);
  m_pointAngle.append(m_pointAngle[npts-1]);

  m_pointPressed = -1;

  computeTangents();

  m_updateFlag = true;

  m_undo.append(m_points, m_pointRadX, m_pointRadY, m_pointAngle);
}

void
PathObject::insertPointAfter(int idx)
{
  int npts = m_points.count();
  if (idx == npts-1)
    {
      Vec v = m_points[npts-1]-m_points[npts-2];
      v += m_points[npts-1];
      m_points.append(v);
      m_pointRadX.append(m_pointRadX[npts-1]);
      m_pointRadY.append(m_pointRadY[npts-1]);
      m_pointAngle.append(m_pointAngle[npts-1]);

      setPointPressed(npts-1);
    }
  else
    {
      Vec v = (m_points[idx]+m_points[idx+1])/2;
      m_points.insert(idx+1, v);
      
      float f;
      f = (m_pointRadX[idx]+m_pointRadX[idx+1])/2;
      m_pointRadX.insert(idx+1, f);
      
      f = (m_pointRadY[idx]+m_pointRadY[idx+1])/2;
      m_pointRadY.insert(idx+1, f);
      
      f = (m_pointAngle[idx]+m_pointAngle[idx+1])/2;
      m_pointAngle.insert(idx+1, f);

      setPointPressed(idx+1);
    }

  computeTangents();

  m_updateFlag = true;

  m_undo.append(m_points, m_pointRadX, m_pointRadY, m_pointAngle);
}

void
PathObject::insertPointBefore(int idx)
{
  if (idx == 0)
    {
      Vec v = m_points[0]-m_points[1];
      v += m_points[0];

      m_points.insert(0, v);
      m_pointRadX.insert(0, m_pointRadX[0]);
      m_pointRadY.insert(0, m_pointRadY[0]);
      m_pointAngle.insert(0, m_pointAngle[0]);

      setPointPressed(0);
    }
  else
    {
      Vec v = (m_points[idx]+m_points[idx-1])/2;
      m_points.insert(idx, v);
  
      float f;
      f = (m_pointRadX[idx]+m_pointRadX[idx-1])/2;
      m_pointRadX.insert(idx, f);
  
      f = (m_pointRadY[idx]+m_pointRadY[idx-1])/2;
      m_pointRadY.insert(idx, f);
  
      f = (m_pointAngle[idx]+m_pointAngle[idx-1])/2;
      m_pointAngle.insert(idx, f);

      setPointPressed(idx);
    }

  computeTangents();

  m_updateFlag = true;

  m_undo.append(m_points, m_pointRadX, m_pointRadY, m_pointAngle);
}

void
PathObject::computePathLength()
{
  computePath(m_points);

  if (m_arrowDirection == false)
    { // just reverse points
      QList<Vec> path = m_path;
      QList<Vec> patht = m_pathT;
      QList<Vec> pathx = m_pathX;
      QList<Vec> pathy = m_pathY;
      QList<float> radx = m_radX;
      QList<float> rady = m_radY;
      QList<float> angle = m_angle;

      m_path.clear();      
      m_pathT.clear();      
      m_pathX.clear();      
      m_pathY.clear();      
      m_radX.clear();
      m_radY.clear();
      m_angle.clear();

      int np = path.count();
      for(int i=0; i<np; i++)
	{
	  m_path.append(path[np-1-i]);
	  m_pathT.append(patht[np-1-i]);
	  m_pathX.append(pathx[np-1-i]);
	  m_pathY.append(pathy[np-1-i]);
	  m_radX.append(radx[np-1-i]);
	  m_radY.append(rady[np-1-i]);
	  m_angle.append(angle[np-1-i]);
	}

      path.clear();
      radx.clear();
      rady.clear();
      angle.clear();
    }


  if (m_displayList > 0) glDeleteLists(m_displayList, 1);
  if (m_displayListCaption > 0) glDeleteLists(m_displayListCaption, 1);

  // generate the displaylists
  glNewList(m_displayListCaption, GL_COMPILE);
  generateRibbon(1.0);
  glEndList();

  glNewList(m_displayList, GL_COMPILE);
  generateTube(1.0);
  glEndList();

}

void
PathObject::computeLength(QList<Vec> points)
{
  m_length = 0;
  for(int i=1; i<points.count(); i++)
    m_length += (points[i]-points[i-1]).norm();
}

void
PathObject::computeTangents()
{
  int nkf = m_points.count();
  if (nkf < 2)
    return;

  m_xaxis.clear();
  m_yaxis.clear();
  m_tgP.clear();

  Vec pxaxis = Vec(1,0,0);
  Vec ptang = Vec(0,0,1);

  for(int kf=0; kf<nkf; kf++)
    {
      Vec prevP, nextP;

      if (kf == 0)
	{	  
	  if (!m_closed)
	    prevP = m_points[kf];
	  else
	    prevP = m_points[nkf-1];
	}
      else
	prevP = m_points[kf-1];

      if (kf == nkf-1)
	{
	  if (!m_closed)
	    nextP = m_points[kf];
	  else
	    nextP = m_points[0];
	}
      else
	nextP = m_points[kf+1];
      
      Vec tgP = 0.5*(nextP - prevP);
      m_tgP.append(tgP);

      //-------------------
      tgP.normalize();
      Vec xaxis, yaxis;
      Vec axis;
      float angle;      
      StaticFunctions::getRotationBetweenVectors(ptang, tgP, axis, angle);
      if (qAbs(angle) > 0.0 && qAbs(angle) < 3.1415)
	{
	  Quaternion q(axis, angle);	  
	  xaxis = q.rotate(pxaxis);
	}
      else
	xaxis = pxaxis;

      //apply offset rotation
      angle = m_pointAngle[kf];
      if (kf > 0) angle = m_pointAngle[kf]-m_pointAngle[kf-1];
      angle = DEG2RAD(angle);
      Quaternion q = Quaternion(tgP, angle);
      xaxis = q.rotate(xaxis);

      yaxis = tgP^xaxis;

      m_xaxis.append(xaxis);
      m_yaxis.append(yaxis);

      pxaxis = xaxis;
      ptang = tgP;
      //-------------------
    } 
}

Vec
PathObject::interpolate(int kf1, int kf2, float frc)
{
  Vec diff = m_points[kf2] - m_points[kf1];
  Vec pos = m_points[kf1];
  float len = diff.squaredNorm();
  if (len > 0.1)
    {
      Vec v1 = 3*diff - 2*m_tgP[kf1] - m_tgP[kf2];
      Vec v2 = -2*diff + m_tgP[kf1] + m_tgP[kf2];
      
      pos += frc*(m_tgP[kf1] + frc*(v1+frc*v2));
    }

  return pos;
}

void
PathObject::computePathVectors()
{
  // using Double Reflection Method
  m_pathT.clear();
  m_pathX.clear();
  m_pathY.clear();

  int npoints = m_path.count();
  Vec t0 = m_path[1] - m_path[0];
  if (m_closed) t0 = m_path[1]-m_path[npoints-2];
  t0.normalize();
  Vec r0 = Vec(1,0,0)^t0;
  if (r0.norm() < 0.1)
    {
      r0 = Vec(0,1,0)^t0;      
      if (r0.norm() < 0.1)
	r0 = Vec(0,0,1)^t0;	  
    }
  r0.normalize();
  Vec s0 = t0^r0;
  s0. normalize();

  m_pathT << t0;
  m_pathX << r0;
  m_pathY << s0;
  for(int i=0; i<npoints-1; i++)
    {
      Vec t1, r1, s1;
      Vec t0L, r0L, s0L;
      Vec v1, v2;
      float c1, c2;

      v1 = m_path[i+1]-m_path[i];
      c1 = v1*v1;
      r0L = r0-(2.0/c1)*(v1*r0)*v1;
      t0L = t0-(2.0/c1)*(v1*t0)*v1;

      t1 = m_path[i+1]-m_path[i];

      if (i < npoints-2) t1 = m_path[i+2]-m_path[i];
      else if (m_closed && i == npoints-2)
	t1 = m_path[1]-m_path[npoints-2];
	
      t1.normalize();
      v2 = t1 - t0L;
      c2 = v2*v2;
      r1 = r0L-(2.0/c2)*(v2*r0L)*v2;
      r1.normalize();
      s1 = t1^r1;
      s1.normalize();      

      m_pathT << t1;
      m_pathX << r1;
      m_pathY << s1;

      t0 = t1;
      r0 = r1;
      s0 = s1;
    }

  if (m_closed)
    {
      // modify angles for closed paths
      Vec r0, r1, s0, s1, t0, t1;
      s0 = m_pathY[0];
      s1 = m_pathY[npoints-1];
      t0 = m_pathT[0];

      float ca = (s0*s1); 
      float sa = (s0^s1).norm();
      float angle = atan2(sa, ca);
      if (t0*(s0^s1) < 0.0)
	angle = -angle;

      for(int i=1; i<npoints; i++)
	{
	  float a = -angle*(float)i/(float)(npoints-1);
	  Vec r = m_pathX[i];
	  Vec s = m_pathY[i];
	  Quaternion q = Quaternion(m_pathT[i], a);
	  r = q.rotate(r);
	  s = q.rotate(s);
	  m_pathX[i] = r;
	  m_pathY[i] = s;
 	}
    }

  for(int i=0; i<npoints; i++)
    {
      Vec r = m_pathX[i];
      Vec s = m_pathY[i];
      Quaternion q = Quaternion(m_pathT[i], m_angle[i]);
      r = q.rotate(r);      
      s = q.rotate(s);      
      m_pathX[i] = r;
      m_pathY[i] = s;
    }
}

void
PathObject::computePath(QList<Vec> points)
{
  Vec voxelScaling = Global::voxelScaling();
  Vec voxelSize = VolumeInformation::volumeInformation().voxelSize;


  // -- collect path points for length computation
  QList<Vec> lengthPath;
  lengthPath.clear();

  m_path.clear();
  m_radX.clear();
  m_radY.clear();
  m_angle.clear();

  int npts = points.count();
  if (npts < 2)
    return;

  Vec prevPt;
  if (m_segments == 1)
    {
      for(int i=0; i<npts; i++)
	{
	  Vec v = VECPRODUCT(points[i], voxelScaling);
	  m_path.append(v);
	  m_radX.append(m_pointRadX[i]);
	  m_radY.append(m_pointRadY[i]);
	  float o = DEG2RAD(m_pointAngle[i]);
	  m_angle.append(o);

	  // for length calculation
	  v = VECPRODUCT(points[i], voxelSize);
	  lengthPath.append(v);
	}
      if (m_closed)
	{
	  Vec v = VECPRODUCT(points[0], voxelScaling);
	  m_path.append(v);
	  m_radX.append(m_pointRadX[0]);
	  m_radY.append(m_pointRadY[0]);
	  float o = DEG2RAD(m_pointAngle[0]);
	  m_angle.append(o);

	  // for length calculation
	  v = VECPRODUCT(points[0], voxelSize);
	  lengthPath.append(v);
	}

      computePathVectors();
  
      computeLength(lengthPath);
      return;
    }

  // for number of segments > 1 apply spline-based interpolation
  for(int i=1; i<npts; i++)
    {
      float radX0 = m_pointRadX[i-1];
      float radY0 = m_pointRadY[i-1];
      float angle0 = DEG2RAD(m_pointAngle[i-1]);
      float radX1 = m_pointRadX[i];
      float radY1 = m_pointRadY[i];
      float angle1 = DEG2RAD(m_pointAngle[i]);
      for(int j=0; j<m_segments; j++)
	{
	  float frc = (float)j/(float)m_segments;
	  Vec pos = interpolate(i-1, i, frc);

	  Vec pv = VECPRODUCT(pos, voxelScaling);
	  m_path.append(pv);

	  float rfrc = StaticFunctions::smoothstep(0.0, 1.0, frc);
	  float v;
	  v = radX0 + rfrc*(radX1-radX0);
	  m_radX.append(v);
	  v = radY0 + rfrc*(radY1-radY0);
	  m_radY.append(v);
	  v = angle0 + frc*(angle1-angle0);
	  m_angle.append(v);


	  // for length calculation
	  pv = VECPRODUCT(pos, voxelSize);
	  lengthPath.append(pv);
	}
    }
  if (!m_closed)
    {
      Vec pos = VECPRODUCT(points[points.count()-1],
			   voxelScaling);
      m_path.append(pos);

      m_radX.append(m_pointRadX[points.count()-1]);
      m_radY.append(m_pointRadY[points.count()-1]);
      float o = DEG2RAD(m_pointAngle[points.count()-1]);
      m_angle.append(o);

      // for length calculation
      pos = VECPRODUCT(points[points.count()-1],
		       voxelSize);
      lengthPath.append(pos);
    }
  else
    {
      // for closed path
      float radX0 = m_pointRadX[npts-1];
      float radY0 = m_pointRadY[npts-1];
      float angle0 = DEG2RAD(m_pointAngle[npts-1]);
      float radX1 = m_pointRadX[0];
      float radY1 = m_pointRadY[0];
      float angle1 = DEG2RAD(m_pointAngle[0]);
      for(int j=0; j<m_segments; j++)
	{
	  float frc = (float)j/(float)m_segments;
	  Vec pos = interpolate(npts-1, 0, frc);

	  Vec pv = VECPRODUCT(pos, voxelScaling);
	  m_path.append(pv);

	  float v;
	  v = radX0 + frc*(radX1-radX0);
	  m_radX.append(v);
	  v = radY0 + frc*(radY1-radY0);
	  m_radY.append(v);
	  v = angle0 + frc*(angle1-angle0);
	  m_angle.append(v);

	  // for length calculation
	  pv = VECPRODUCT(pos, voxelSize);
	  lengthPath.append(pv);
	}
      Vec pos = VECPRODUCT(points[0], voxelScaling);
      m_path.append(pos);

      m_radX.append(m_pointRadX[0]);
      m_radY.append(m_pointRadY[0]);
      float o = DEG2RAD(m_pointAngle[0]);
      m_angle.append(o);

      // for length calculation
      pos = VECPRODUCT(points[0], voxelSize);
      lengthPath.append(pos);
    }

  computePathVectors();

  computeLength(lengthPath);
}

QList<Vec>
PathObject::getPointPath()
{
  QList<Vec> path;

  int npts = m_points.count();
  if (npts < 2)
    return path;

  Vec prevPt;
  if (m_segments == 1)
    {
      for(int i=0; i<npts; i++)
	path.append(m_points[i]);
      if (m_closed)
	path.append(m_points[0]);

      return path;
    }

  // for number of segments > 1 apply spline-based interpolation
  for(int i=1; i<npts; i++)
    {
      for(int j=0; j<m_segments; j++)
	{
	  float frc = (float)j/(float)m_segments;
	  Vec pos = interpolate(i-1, i, frc);
	  path.append(pos);
	}
    }
  if (!m_closed)
    path.append(m_points[m_points.count()-1]);
  else
    {
      // for closed path
      for(int j=0; j<m_segments; j++)
	{
	  float frc = (float)j/(float)m_segments;
	  Vec pos = interpolate(npts-1, 0, frc);
	  path.append(pos);
	}
      path.append(m_points[0]);
    }

  return path;
}

QList< QPair<Vec, Vec> >
PathObject::getPointAndNormalPath()
{
  computePath(m_points);

  QList< QPair<Vec, Vec> > pathNormal;

  if (m_path.count() == 0)
    return pathNormal;

  for (int i=0; i<m_path.count(); i++)
    pathNormal.append(qMakePair(m_path[i],
				m_radX[i]*m_pathX[i]));

  return pathNormal;
}

void
PathObject::postdrawInViewport(QGLViewer *viewer,
			       int x, int y,
			       bool grabsMouse,
			       Vec cp, Vec cn, int ct, float textscale)
{
  Vec voxelScaling = Global::voxelScaling();
  bool ok = true;
  for(int i=0; i<m_points.count();i++)
    {
      Vec pt = VECPRODUCT(m_points[i], voxelScaling);
      if (qAbs((pt-cp)*cn) > ct+0.5)
	{
	  ok = false;
	  break;
	}
    }

  if (ok)
    postdraw(viewer, x, y, grabsMouse, 0.15*textscale);
}


void
PathObject::draw(QGLViewer *viewer,
		 bool active,
		 bool backToFront,
		 Vec lightPosition)
{
  // create display lists if not already created
  if (m_displayList == 0)
    m_displayList = glGenLists(1);

  if (m_displayListCaption == 0)
    m_displayListCaption = glGenLists(1);

  if (m_updateFlag)
    {
      m_updateFlag = false;

      computePathLength();
    }

  if (m_tube)
    drawTube(viewer, active, lightPosition);
  else
    drawLines(viewer, active, backToFront);
}

void
PathObject::drawTube(QGLViewer *viewer,
		     bool active,
		     Vec lightPosition)
{
  float pos[4];
  float diff[4] = { 1.0, 1.0, 1.0, 1.0 };
  float spec[4] = { 1.0, 1.0, 1.0, 1.0 };
  float shine = 128;

  glEnable(GL_LIGHTING);
  glShadeModel(GL_SMOOTH);
  glEnable(GL_LIGHT0);

  glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
  glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);

  if (shine < 1)
    {
      spec[0] = spec[1] = spec[2] = 0;
    }

  glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, spec);
  glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, &shine);

  glLightfv(GL_LIGHT0, GL_DIFFUSE, diff);

  glEnable(GL_COLOR_MATERIAL);
  glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

  pos[0] = lightPosition.x;
  pos[1] = lightPosition.y;
  pos[2] = lightPosition.z;
  pos[3] = 0;
  glLightfv(GL_LIGHT0, GL_POSITION, pos);

  glColor4f(m_color.x*m_opacity,
	    m_color.y*m_opacity,
	    m_color.z*m_opacity,
	    m_opacity);

  // emissive when active
  if (active)
    {
      float emiss[] = { 0.5, 0.0, 0.0, 1.0 };
      glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emiss);
    }
  else
    {
      float emiss[] = { 0.0, 0.0, 0.0, 1.0 };
      glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emiss);
    }


  if ( m_imagePresent ||
      (m_captionPresent && !m_captionLabel) )
    {
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_imageTex);

      if (m_imagePresent)
	{
	  int nbytes = m_image.byteCount();
	  int rgb = nbytes/(m_textureWidth*m_textureHeight);
	  GLuint fmt;
	  if (rgb == 1) fmt = GL_LUMINANCE;
	  else if (rgb == 2) fmt = GL_LUMINANCE_ALPHA;
	  else if (rgb == 3) fmt = GL_RGB;
	  else if (rgb == 4) fmt = GL_BGRA;
	  glTexImage2D(GL_TEXTURE_RECTANGLE_ARB,
		       0,
		       rgb,
		       m_textureWidth,
		       m_textureHeight,
		       0,
		       fmt,
		       GL_UNSIGNED_BYTE,
		       m_image.bits());
	}
      else
	{
	  glTexImage2D(GL_TEXTURE_RECTANGLE_ARB,
		       0,
		       4,
		       m_textureWidth,
		       m_textureHeight,
		       0,
		       GL_RGBA,
		       GL_UNSIGNED_BYTE,
		       m_cImage.bits());
	}

      glTexEnvf(GL_TEXTURE_ENV,
		GL_TEXTURE_ENV_MODE,
		GL_MODULATE);
      glEnable(GL_TEXTURE_RECTANGLE_ARB);
      
      glColor4f(m_opacity,
		m_opacity,
		m_opacity,
		m_opacity);
      
      glCallList(m_displayListCaption);
      
      glDisable(GL_TEXTURE_RECTANGLE_ARB);
    }
  else
    glCallList(m_displayList);


  { // reset emissivity
    float emiss[] = { 0.0, 0.0, 0.0, 1.0 };
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emiss);
  }

  glDisable(GL_LIGHTING);
}

void
PathObject::drawLines(QGLViewer *viewer,
		      bool active,
		      bool backToFront)
{
  glEnable(GL_BLEND);
//  glEnable(GL_LINE_SMOOTH);
//  glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

  Vec col = m_opacity*m_color;
  if (backToFront)
    {
      if (active)
	glLineWidth(7);
      else
	glLineWidth(3);
	
      glColor4f(col.x*0.5,
		col.y*0.5,
		col.z*0.5,
		m_opacity*0.5);

      glBegin(GL_LINE_STRIP);
      for(int i=0; i<m_path.count(); i++)
	glVertex3fv(m_path[i]);
      glEnd();
    }

  if (m_showPoints)
    {
      glColor3f(m_color.x,
		m_color.y,
		m_color.z);


      glEnable(GL_POINT_SPRITE);
      glActiveTexture(GL_TEXTURE0);
      glEnable(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D, Global::spriteTexture());
      glTexEnvf(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE );
      glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
      glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
      glEnable(GL_POINT_SMOOTH);

      Vec voxelScaling = Global::voxelScaling();
      glPointSize(20);
      glBegin(GL_POINTS);
      for(int i=0; i<m_points.count();i++)
	{
	  Vec pt = VECPRODUCT(m_points[i], voxelScaling);
	  glVertex3fv(pt);
	}
      glEnd();


      if (m_pointPressed > -1)
	{
	  glColor3f(1,0,0);
	  Vec voxelScaling = Global::voxelScaling();
	  glPointSize(25);
	  glBegin(GL_POINTS);
	  Vec pt = VECPRODUCT(m_points[m_pointPressed], voxelScaling);
	  glVertex3fv(pt);
	  glEnd();
	}

      glPointSize(1);  

      glDisable(GL_POINT_SPRITE);
      glDisable(GL_TEXTURE_2D);
  
      glDisable(GL_POINT_SMOOTH);
    }

  if (active)
    glLineWidth(3);
  else
    glLineWidth(1);

  glColor4f(col.x, col.y, col.z, m_opacity);

  //---------------------------------
  // draw arrows
  if (m_capType == ARROW)
    {
      Vec voxelScaling = Global::voxelScaling();
      glBegin(GL_TRIANGLES);
      for(int i=0; i<m_points.count();i++)
	{
	  Vec v0, v1;

	  if (!m_closed && i == m_points.count()-1)
	    break;

	  v1 = m_path[i*m_segments];
	  if (i < m_points.count()-1)
	    v0 = m_path[(i+1)*m_segments];
	  else
	    v0 = m_path[0];
	  
	  Vec sc0, sc1;
	  sc0 = viewer->camera()->projectedCoordinatesOf(v0);
	  sc1 = viewer->camera()->projectedCoordinatesOf(v1);
	  
	  float rl = qMin(m_arrowHeadLength, (float)(sc1-sc0).norm());
	  Vec dv = (sc1-sc0).unit();
	  v1 = sc0 + rl*dv;
	  
	  float px = (sc1.y-sc0.y);
	  float py = -(sc1.x-sc0.x);
	  float dlen = sqrt(px*px + py*py);
	  px/=dlen; py/=dlen;
	  px *= qBound(1, (int)(rl/3), 7);
	  py *= qBound(1, (int)(rl/3), 7);
	  
	  Vec v2 = viewer->camera()->unprojectedCoordinatesOf(v1 + Vec(px, py, 0));
	  Vec v3 = viewer->camera()->unprojectedCoordinatesOf(v1 - Vec(px, py, 0));

	  glVertex3fv(v0);
	  glVertex3fv(v2);
	  glVertex3fv(v3);

	  if (!m_arrowForAll)
	    break;
	}
      glEnd();
    }
  //---------------------------------

  glBegin(GL_LINE_STRIP);
  for(int i=0; i<m_path.count(); i++)
    glVertex3fv(m_path[i]);
  glEnd();

  if (!backToFront)
    {
      if (active)
        glLineWidth(7);
      else
        glLineWidth(3);

      glColor4f(col.x*0.5,
		col.y*0.5,
		col.z*0.5,
		m_opacity*0.5);

      glBegin(GL_LINE_STRIP);
      for(int i=0; i<m_path.count(); i++)
	glVertex3fv(m_path[i]);
      glEnd();
    }

  glLineWidth(1);


  if (m_showPoints || m_pointPressed > -1)
    {
      glColor3f(1,0,0);
      Vec pxaxis = Vec(1,0,0);
      Vec ptang = Vec(0,0,1);
      int npoints = m_path.count();
      for (int i=0; i<npoints; i++)
	{
	  if (i%m_segments == 0)
	    glColor3f(1, 0.2, 0);
	  else
	    glColor3f(0.4,0.4,0.4);
	  glBegin(GL_LINES);
	  glVertex3fv(m_path[i]);
	  glVertex3fv(m_path[i]+m_radX[i]*m_pathX[i]);
	  glEnd();
	  
	  if (i%m_segments == 0)
	    glColor3f(0.2,1,0.3);
	  else
	    glColor3f(0.8,0.8,0.8);
	  glBegin(GL_LINES);
	  glVertex3fv(m_path[i]);
	  glVertex3fv(m_path[i]+m_radY[i]*m_pathY[i]);
	  glEnd();

	  if (i/m_segments == m_pointPressed)
	    {
	      glColor3f(col.x, col.y, col.z);
	      QList<Vec> csec;
	      csec = getCrossSection(1.0,
				     m_radX[i], m_radY[i],
				     m_sections,
				     m_pathT[i],
				     m_pathX[i],
				     m_pathY[i]);
	      
	      for(int j=0; j<csec.count(); j++)
		csec[j] += m_path[i];
	      
	      glBegin(GL_LINE_STRIP);
	      for(int j=0; j<csec.count(); j++)
		glVertex3fv(csec[j]);
	      glEnd();
	    }
	}
    }

  //  glDisable(GL_LINE_SMOOTH);
}

void
PathObject::generateRibbon(float scale)
{
  float prevtex = 0;
  Vec prevp0, prevp1;
  int npaths = m_path.count();

  float totlength = 0;
  for(int i=1; i<npaths; i++)
    totlength += (m_path[i]-m_path[i-1]).norm();

  float pathlength = 0;
  for(int i=0; i<npaths; i++)
    {
      Vec voxelScaling = Global::voxelScaling();
      Vec shft = VECPRODUCT(m_radX[i]*scale*m_pathX[i], voxelScaling);

      Vec pp0 = m_path[i] - shft;
      Vec pp1 = m_path[i] + shft;

      if (i > 0)
	pathlength += (m_path[i]-m_path[i-1]).norm();
      float tex = pathlength/totlength;

      //---------------------------------------------
      if (i > 0)
	{
	  glBegin(GL_TRIANGLE_STRIP);

	  glTexCoord2f(prevtex*m_textureWidth, 0);
	  glNormal3fv(-m_pathY[i-1]);
	  glVertex3fv(prevp0);

	  glTexCoord2f(prevtex*m_textureWidth, m_textureHeight);
	  glNormal3fv(-m_pathY[i-1]);
	  glVertex3fv(prevp1);
	      
	  glTexCoord2f(tex*m_textureWidth, 0);
	  glNormal3fv(-m_pathY[i]);
	  glVertex3fv(pp0);

	  glTexCoord2f(tex*m_textureWidth, m_textureHeight);
	  glNormal3fv(-m_pathY[i]);
	  glVertex3fv(pp1);
	  glEnd();
	}
      //---------------------------------------------

      prevp0 = pp0;
      prevp1 = pp1;
      prevtex = tex;
    }
}

void
PathObject::generateTube(float scale)
{
  QList<Vec> csec1, norm1;

  Vec pxaxis = Vec(1,0,0);
  Vec ptang = Vec(0,0,1);

  int npaths = m_path.count();
  int nextArrowIdx=0;
  Vec nextArrowHead;
  float nextArrowHeight = m_arrowHeadLength;
  for(int i=0; i<npaths; i++)
    {
      QList<Vec> csec2, norm2;

      //---------------------------------------------
      csec2 = getCrossSection(scale,
			      m_radX[i], m_radY[i],
			      m_sections,
			      m_pathT[i], m_pathX[i], m_pathY[i]);
      norm2 = getNormals(csec2, m_pathT[i]);
      //---------------------------------------------


      //---------------------------------------------
      if (m_capType == FLAT)
	{
	  if (!m_closed && (i == 0 || i==npaths-1))
	    addFlatCaps(i, m_pathT[i], csec2);
	}
      //---------------------------------------------

      //---------------------------------------------
      if (m_capType == ROUND)
	{
	  if (!m_closed && (i == 0 || i==npaths-1))
	    addRoundCaps(i, m_pathT[i], csec2, norm2);
	}
      //---------------------------------------------



      //---------------------------------------------
      // generate the tubular mesh
      if (i > 0)
	{
	  float frc1 = 2;
	  float frc2 = 2;
	  if (m_capType == ARROW)
	    {
	      frc1 = (m_path[i-1]-nextArrowHead).norm()/nextArrowHeight;
	      frc2 = (m_path[i]-nextArrowHead).norm()/nextArrowHeight;
	    }
	  if (frc1 >= 1 && frc2 < 1)
	    {
	      addArrowHead(i, scale,
			   nextArrowHead,
			   m_pathT[i-1], m_pathX[i-1], m_pathY[i-1],
			   frc1, frc2,
			   csec1, norm1);
	    }
	  else if (frc2 >= 1)
	    {
	      glBegin(GL_TRIANGLE_STRIP);
	      for(int j=0; j<csec1.count(); j++)
		{
		  Vec vox1 = m_path[i-1] + csec1[j];
		  glNormal3fv(norm1[j]);
		  glVertex3fv(vox1);
		  
		  Vec vox2 = m_path[i] + csec2[j];
		  glNormal3fv(norm2[j]);
		  glVertex3fv(vox2);
		}	      
	      glEnd();
	    }
	}
      //---------------------------------------------

      csec1 = csec2;
      norm1 = norm2;      

      if (m_capType == ARROW)
	{
	  // calculate nextArrowHead
	  if (!m_arrowForAll && i == 0)
	    {
	      if (!m_closed)
		{
		  nextArrowIdx = npaths-1;
		  nextArrowHead = m_path[npaths-1];
		}
	      else
		{
		  nextArrowIdx = npaths-2;
		  nextArrowHead = m_path[npaths-2];
		}
	    }
	  else if (m_arrowForAll)
	    {
	      if (i%m_segments == 0)
		{
		  nextArrowIdx += m_segments;
		  if (nextArrowIdx > npaths-1)
		    nextArrowIdx = npaths-1;
		  nextArrowHead = m_path[nextArrowIdx];
		}
	    }
	}

    }

  csec1.clear();
  norm1.clear();
}

void
PathObject::addFlatCaps(int i,
			Vec tang,
			QList<Vec> csec)
{
  Vec norm = -tang;
  int sections = csec.count()-1;
  int halfway = sections/2;
  glBegin(GL_TRIANGLE_STRIP);
  for(int j=0; j<=halfway; j++)
    {
      Vec vox2 = m_path[i] + csec[j];
      glNormal3fv(norm);
      glVertex3fv(vox2);

      if (j < halfway)
	{
	  vox2 = m_path[i] + csec[sections-j];
	  glNormal3fv(norm);
	  glVertex3fv(vox2);
	}
    }
  glEnd();
}

void
PathObject::addRoundCaps(int i,
			 Vec tang,
			 QList<Vec> csec2,
			 QList<Vec> norm2)
{
  int npaths = m_path.count();
  int sections = csec2.count()-1;
  int ksteps = 4;
  Vec ctang = -tang;
  if (i==0) ctang = tang;
  QList<Vec> csec = csec2;
  float rad = qMin(m_radX[i], m_radY[i]);
  for(int k=0; k<ksteps-1; k++)
    {
      float ct1 = cos(1.57*(float)k/(float)ksteps);
      float ct2 = cos(1.57*(float)(k+1)/(float)ksteps);
      float st1 = sin(1.57*(float)k/(float)ksteps);
      float st2 = sin(1.57*(float)(k+1)/(float)ksteps);
      glBegin(GL_TRIANGLE_STRIP);	  
      for(int j=0; j<csec2.count(); j++)
	{
	  Vec norm = csec2[j]*ct1 - ctang*rad*st1;
	  Vec vox2 = m_path[i] + norm;
	  if (k==0)
	    {
	      if (i==0)
		glNormal3fv(-norm2[j]);
	      else
		glNormal3fv(norm2[j]);
	    }
	  else
	    {  
	      norm.normalize();
	      if (i==npaths-1) norm=-norm;
	      glNormal3fv(norm);
	    }
	  glVertex3fv(vox2);
	  
	  norm = csec2[j]*ct2 - ctang*rad*st2;
	  vox2 = m_path[i] + norm;
	  norm.normalize();
	  if (i==npaths-1) norm=-norm;
	  glNormal3fv(norm);
	  glVertex3fv(vox2);
	}
      glEnd();
    }
  
  // add flat ends
  float ct2 = cos(1.57*(float)(ksteps-1)/(float)ksteps);
  float st2 = sin(1.57*(float)(ksteps-1)/(float)ksteps);
  int halfway = sections/2;
  glBegin(GL_TRIANGLE_STRIP);
  for(int j=0; j<=halfway; j++)
    {
      Vec norm = csec2[j]*ct2 - ctang*rad*st2;
      Vec vox2 = m_path[i] + norm;
      norm.normalize();
      if (i==npaths-1) norm=-norm;
      glNormal3fv(norm);
      glVertex3fv(vox2);
      
      if (j < halfway)
	{
	  norm = csec2[sections-j]*ct2 -
	    ctang*rad*st2;
	  vox2 = m_path[i] + norm;
	  norm.normalize();
	  if (i==npaths-1) norm=-norm;
	  glNormal3fv(norm);
	  glVertex3fv(vox2);
	}
    }
  glEnd();
}
void
PathObject::addArrowHead(int i, float scale,
			 Vec nextArrowHead,
			 Vec ptang, Vec pxaxis, Vec pyaxis,
			 float frc1, float frc2,
			 QList<Vec> csec1,
			 QList<Vec> norm1)
{
  //---------------------------------------------
  Vec tangm, xaxism, yaxism;
//  tangm = m_path[i]-m_path[i-1];
//  if (tangm.norm() > 0)
//    tangm.normalize();
//  else
//    tangm = Vec(1,0,0); // should really scold the user
//
//  //----------------
//  if (ptang*tangm > 0.99) // ptang and tang are same
//    {
//      xaxism = pxaxis;
//    }
//  else
//    {
//      Vec axis;
//      float angle;      
//      StaticFunctions::getRotationBetweenVectors(ptang, tangm, axis, angle);
//      Quaternion q(axis, angle);
//      
//      xaxism = q.rotate(pxaxis);
//    }
//
//  //apply offset rotation
//  float angle = m_angle[i];
//  if (i > 0) angle = m_angle[i]-m_angle[i-1];
//  Quaternion q = Quaternion(tangm, angle);
//  xaxism = q.rotate(xaxism);
//  
//  yaxism = tangm^xaxism;

  tangm = m_pathT[i];
  xaxism = m_pathX[i];
  yaxism = m_pathY[i];

  QList<Vec> csecm, normm;
  csecm = getCrossSection(scale,
			  m_radX[i], m_radY[i],
			  m_sections,
			  tangm, xaxism, yaxism);
  normm = getNormals(csecm, tangm);
  //---------------------------------------------
  
  float t = (1.0-frc2)/(frc1-frc2);
  Vec mid = m_path[i] + t*(m_path[i-1]-m_path[i]);

  glBegin(GL_TRIANGLE_STRIP);
  for(int j=0; j<csec1.count(); j++)
    {
      Vec vox1 = m_path[i-1] + csec1[j];
      glNormal3fv(norm1[j]);
      glVertex3fv(vox1);
      
      Vec vox2 = mid + csecm[j];
      glNormal3fv(normm[j]);
      glVertex3fv(vox2);
    }
  glEnd();

  glBegin(GL_TRIANGLE_STRIP);
  for(int j=0; j<csecm.count(); j++)
    {
      Vec vox1 = mid + csecm[j];
      glNormal3fv(normm[j]);
      glVertex3fv(vox1);
      
      Vec vox2 = mid + 2*csecm[j];
      glNormal3fv(normm[j]);
      glVertex3fv(vox2);
    }
  glEnd();

  glBegin(GL_TRIANGLE_STRIP);
  for(int j=0; j<csecm.count(); j++)
    {
      Vec vox1 = mid + 2*csecm[j];
      glNormal3fv(normm[j]);
      glVertex3fv(vox1);
      
      Vec vox2 = nextArrowHead;
      glNormal3fv(tangm);
      glVertex3fv(vox2);
    }
  glEnd();
}

QList<Vec>
PathObject::getCrossSection(float scale,
			    float a, float b,
			    int sections,
			    Vec tang, Vec xaxis, Vec yaxis)
{
  Vec voxelScaling = Global::voxelScaling();

  int jend = sections;
  if (m_halfSection)
    jend = sections/2+1;

  QList<Vec> csec;
  for(int j=0; j<jend; j++)
    {
      float t = (float)j/(float)sections;

      // change 't' to get a smoother cross-section
      if (j<0.25*sections && j>0)
	{
	  if (a/b > 1) t -= (1-b/a)/sections;
	  else if (b/a > 1) t += (1-a/b)/sections;
	}
      else if (j<0.5*sections && j>0.25*sections)
	{
	  if (b/a > 1) t -= (1-a/b)/sections;
	  else if (a/b > 1) t += (1-b/a)/sections;
	}
      else if (j<0.75*sections && j>0.5*sections)
	{
	  if (a/b > 1) t -= (1-b/a)/sections;
	  else if (b/a > 1) t += (1-a/b)/sections;
	}
      else if (j<sections && j>0.75*sections)
	{
	  if (b/a > 1) t -= (1-a/b)/sections;
	  else if (a/b > 1) t += (1-b/a)/sections;
	}

      float st = a*sin(6.2831853*t);
      float ct = b*cos(6.2831853*t);
      float r = (a*b)/sqrt(ct*ct + st*st);
      float x = r*cos(6.2831853*t)*scale;
      float y = r*sin(6.2831853*t)*scale;
      Vec v = x*xaxis + y*yaxis;
      v = VECPRODUCT(v, voxelScaling);
      csec.append(v);
    }

  if (!m_halfSection)
    csec.append(csec[0]);

  return csec;
}

QList<Vec>
PathObject::getNormals(QList<Vec> csec, Vec tang)
{
  QList<Vec> norm;
  int sections = csec.count();
  for(int j=0; j<sections; j++)
    {
      Vec v;
      if (j==0 || j==sections-1)
	v = csec[1]-csec[sections-2];
      else
	v = csec[j+1]-csec[j-1];
      
      v.normalize();
      v = tang^v;

      norm.append(v);
    }

  return norm;
}

void
PathObject::postdrawAngle(QGLViewer *viewer)
{
  if (m_points.count() < 3)
    return;

  glColor4f(m_lengthColor.x*0.5,
	    m_lengthColor.y*0.5,
	    m_lengthColor.z*0.5,
	    0.5);
      
  Vec voxelScaling = Global::voxelScaling();
  Vec pt = VECPRODUCT(m_points[0], voxelScaling);
  Vec scr = viewer->camera()->projectedCoordinatesOf(pt);
  int x0 = scr.x;
  int y0 = scr.y;
  //---------------------
  x0 *= viewer->size().width()/viewer->camera()->screenWidth();
  y0 *= viewer->size().height()/viewer->camera()->screenHeight();
  //---------------------

  pt = VECPRODUCT(m_points[1], voxelScaling);
  scr = viewer->camera()->projectedCoordinatesOf(pt);
  int x1 = scr.x;
  int y1 = scr.y;
  //---------------------
  x1 *= viewer->size().width()/viewer->camera()->screenWidth();
  y1 *= viewer->size().height()/viewer->camera()->screenHeight();
  //---------------------

  pt = VECPRODUCT(m_points[2], voxelScaling);
  scr = viewer->camera()->projectedCoordinatesOf(pt);
  int x2 = scr.x;
  int y2 = scr.y;
  //---------------------
  x2 *= viewer->size().width()/viewer->camera()->screenWidth();
  y2 *= viewer->size().height()/viewer->camera()->screenHeight();
  //---------------------

  Vec v0 = Vec(x0, y0, 0);
  Vec v1 = Vec(x1, y1, 0);
  Vec v2 = Vec(x2, y2, 0);
  
  Vec v01 = v0-v1;
  Vec v21 = v2-v1;
  
  float vl01 = v01.norm();
  float vl21 = v21.norm();
  
  v01.normalize();
  v21.normalize();
  
  vl01 = qMin(100.0f, vl01*0.5f);
  vl21 = qMin(100.0f, vl21*0.5f);
  float l = qMin(vl01, vl21);
  v01 *= l;
  v21 *= l;
  
  v0 = v1 + v01;
  v2 = v1 + v21;
  
  glBegin(GL_TRIANGLES);
  glVertex2f(v0.x, v0.y);
  glVertex2f(v1.x, v1.y);
  glVertex2f(v2.x, v2.y);
  glEnd();   
  
  float pangle = StaticFunctions::calculateAngle(m_points[0],
						 m_points[1],
						 m_points[2]);
  QString str = QString("%1").arg(pangle, 0, 'f', Global::floatPrecision());      
  
  glPushMatrix();
  glLoadIdentity();
  
  str += QChar(0xB0);
  float fscl = 120.0/Global::dpi();
  QFont font = QFont("Helvetica");
  font.setStyleStrategy(QFont::PreferAntialias);
  font.setPointSize(12*fscl);
  StaticFunctions::renderText(v1.x, v1.y,
			       str, font,
			      //Qt::transparent, Qt::white);
			      QColor(50,50,50,150), Qt::white);

  glPopMatrix();
}

void
PathObject::postdrawLength(QGLViewer *viewer)
{
  Vec voxelScaling = Global::voxelScaling();

  glColor4f(m_lengthColor.x*0.9,
	    m_lengthColor.y*0.9,
	    m_lengthColor.z*0.9,
	    0.9);

  Vec pt = VECPRODUCT(m_points[0], voxelScaling);
  Vec scr = viewer->camera()->projectedCoordinatesOf(pt);
  int x0 = scr.x;
  int y0 = scr.y;
  //---------------------
  x0 *= viewer->size().width()/viewer->camera()->screenWidth();
  y0 *= viewer->size().height()/viewer->camera()->screenHeight();
  //---------------------
  
  pt = VECPRODUCT(m_points[m_points.count()-1], voxelScaling);
  scr = viewer->camera()->projectedCoordinatesOf(pt);
  int x1 = scr.x;
  int y1 = scr.y;
  //---------------------
  x1 *= viewer->size().width()/viewer->camera()->screenWidth();
  y1 *= viewer->size().height()/viewer->camera()->screenHeight();
  //---------------------
  
  
  float perpx = (y1-y0);
  float perpy = -(x1-x0);
  float dlen = sqrt(perpx*perpx + perpy*perpy);
  perpx/=dlen; perpy/=dlen;
  
  float angle = atan2(-perpx, perpy);
  bool angleFixed = false;
  angle *= 180.0/3.1415926535;
  if (perpy < 0) { angle = 180+angle; angleFixed = true; }
  
  float px = perpx * m_lengthTextDistance;
  float py = perpy * m_lengthTextDistance;
  
  glLineWidth(3);
  glEnable(GL_LINE_STIPPLE);
  glLineStipple(2, 0xAAAA);
  glBegin(GL_LINES);
  glVertex2f(x0, y0);
  glVertex2f(x0+(px*1.1), y0+(py*1.1));
  glVertex2f(x1, y1);
  glVertex2f(x1+(px*1.1), y1+(py*1.1));
  glEnd();
  glDisable(GL_LINE_STIPPLE);
  
  glBegin(GL_LINES);
  glVertex2f(x0+px, y0+py);
  glVertex2f(x1+px, y1+py);
  glEnd();
  
  int x2 = x0 + 0.1*(x1-x0);
  int y2 = y0 + 0.1*(y1-y0);
  int x3 = x1 - 0.1*(x1-x0);
  int y3 = y1 - 0.1*(y1-y0);
  int pxs = perpx * (m_lengthTextDistance+7);
  int pys = perpy * (m_lengthTextDistance+7);
  int pxe = perpx * (m_lengthTextDistance-7);
  int pye = perpy * (m_lengthTextDistance-7);
  glBegin(GL_TRIANGLES);
  glVertex2f(x0+px, y0+py);
  glVertex2f(x2+pxs, y2+pys);
  glVertex2f(x2+pxe, y2+pye);
  glVertex2f(x1+px, y1+py);
  glVertex2f(x3+pxs, y3+pys);
  glVertex2f(x3+pxe, y3+pye);
  glEnd();
  
  glLineWidth(2);
  
  VolumeInformation pvlInfo = VolumeInformation::volumeInformation();
  QString str = QString("%1 %2").					\
                         arg(length(), 0, 'f', Global::floatPrecision()).\
                         arg(pvlInfo.voxelUnitStringShort()); 
  
  glPushMatrix();
  glLoadIdentity();

  if (str.endsWith("um"))
    {
      str.chop(2);
      str += QChar(0xB5);
      str += "m";
    }
  float fscl = 120.0/Global::dpi();
  QFont font = QFont("Helvetica");
  font.setStyleStrategy(QFont::PreferAntialias);
  font.setPointSize(12*fscl);
  int x = (x0+x1)/2 + px*1.1;
  int y = (y0+y1)/2 + py*1.1;
//  StaticFunctions::renderRotatedText(x,y,
//				     str, font,
//				     Qt::transparent, Qt::white,
//				     -angle,
//				     true); // (0,0) is bottom left
  StaticFunctions::renderRotatedText(x,y,
				     str, font,
				     QColor(50,50,50,150), Qt::white,
				     -angle,
				     true); // (0,0) is bottom left
    
  glPopMatrix();
}

void
PathObject::postdrawPointNumbers(QGLViewer *viewer)
{
  Vec voxelScaling = Global::voxelScaling();
  for(int i=0; i<m_points.count();i++)
    {
      Vec pt = VECPRODUCT(m_points[i], voxelScaling);
      Vec scr = viewer->camera()->projectedCoordinatesOf(pt);
      int x = scr.x;
      int y = scr.y;
      
      //---------------------
      x *= viewer->size().width()/viewer->camera()->screenWidth();
      y *= viewer->size().height()/viewer->camera()->screenHeight();
      //---------------------
      
      QString str = QString("%1").arg(i);
      QFont font = QFont();
      QFontMetrics metric(font);
      int ht = metric.height();
      int wd = metric.width(str);
      y += ht/2;
      
      StaticFunctions::renderText(x+2, y, str, font, Qt::black, Qt::cyan);
    }
}

void
PathObject::postdrawCaption(QGLViewer *viewer)
{
  QImage cimage = m_cImage;
  
  int screenWidth = viewer->size().width();
  int screenHeight = viewer->size().height();
  
  float frcw = (float(viewer->camera()->screenWidth())/
		float(screenWidth));
  float frch = (float(viewer->camera()->screenHeight())/
		float(screenHeight));

  Vec voxelScaling = Global::voxelScaling();
  Vec pt = VECPRODUCT(m_path[0], voxelScaling);
  Vec pp0 = viewer->camera()->projectedCoordinatesOf(pt);
  pt = VECPRODUCT(m_path[m_path.count()-1], voxelScaling);
  Vec pp1 = viewer->camera()->projectedCoordinatesOf(pt);

  pp0.x /= frcw;
  pp0.y /= frch;
  pp1.x /= frcw;
  pp1.y /= frch;

  int px = pp0.x + 10;
  if (pp0.x < pp1.x)
    px = pp0.x - m_textureWidth - 10;
  int py = pp0.y + m_textureHeight/2;
  
  if (px < 0 || py > screenHeight)
    {
      int wd = cimage.width();
      int ht = cimage.height();
      int sx = 0;
      int sy = 0;
      if (px < 0)
	{
	  wd = cimage.width()+px;
	  sx = -px;
	  px = 0;
	}
      if (py > screenHeight)
	{
	  ht = cimage.height()-(py-screenHeight);
	  sy = (py-screenHeight);
	  py = screenHeight;
	}
      
      cimage = cimage.copy(sx, sy, wd, ht);
    }
  
  cimage = cimage.scaled(cimage.width()*frcw,
			 cimage.height()*frch);
  
  const uchar *bits = cimage.bits();
  
  glRasterPos2i(px, py);
  glDrawPixels(cimage.width(), cimage.height(),
	       GL_RGBA,
	       GL_UNSIGNED_BYTE,
	       bits);
}

void
PathObject::postdrawGrab(QGLViewer *viewer,
			 int x, int y)
{
  VolumeInformation pvlInfo = VolumeInformation::volumeInformation();
  float len = length();
  QString str = QString("%1 %2").\
	                    arg(len, 0, 'f', Global::floatPrecision()).\
	                    arg(pvlInfo.voxelUnitStringShort());      
  if (str.endsWith("um"))
    {
      str.chop(2);
      str += QChar(0xB5);
      str += "m";
    }
  QFont font = QFont();
  QFontMetrics metric(font);
  int ht = metric.height();
  int wd = metric.width(str);
  x += 10;
  
  StaticFunctions::renderText(x+2, y, str, font, Qt::black, Qt::cyan);
}


void
PathObject::postdraw(QGLViewer *viewer,
		     int x, int y,
		     bool grabsMouse,
		     float scale)
{
  if (!grabsMouse &&
      !m_showPointNumbers &&
      !m_showLength &&
      !m_showAngle &&
      !(m_captionPresent && m_captionLabel))
    return;

//  glEnable(GL_LINE_SMOOTH);
//  glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

  glDisable(GL_DEPTH_TEST);
  viewer->startScreenCoordinatesSystem();

  if (m_captionPresent && m_captionLabel)
    postdrawCaption(viewer);
  
  if (grabsMouse)
    postdrawGrab(viewer, x, y);
  
  if (m_showPointNumbers)
    postdrawPointNumbers(viewer);
  
  if (m_showAngle)
    postdrawAngle(viewer);
  
  if (m_showLength)
    postdrawLength(viewer);

  viewer->stopScreenCoordinatesSystem();
  glEnable(GL_DEPTH_TEST);
}

void
PathObject::save(fstream& fout)
{
  char keyword[100];
  int len;
  float f[3];

  memset(keyword, 0, 100);
  sprintf(keyword, "pathobjectstart");
  fout.write((char*)keyword, strlen(keyword)+1);

  memset(keyword, 0, 100);
  sprintf(keyword, "usetype");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_useType, sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "keepinside");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_keepInside, sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "keepends");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_keepEnds, sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "blendtf");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_blendTF, sizeof(int));

  memset(keyword, 0, 100);
  sprintf(keyword, "allowinterpolate");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_allowInterpolate, sizeof(int));

  memset(keyword, 0, 100);
  sprintf(keyword, "showpoints");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_showPoints, sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "showlength");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_showLength, sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "showangle");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_showAngle, sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "tube");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_tube, sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "closed");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_closed, sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "captype");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_capType, sizeof(int));

  memset(keyword, 0, 100);
  sprintf(keyword, "arrowdirection");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_arrowDirection, sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "arrowforall");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_arrowForAll, sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "halfsection");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_halfSection, sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "sections");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_sections, sizeof(int));

  memset(keyword, 0, 100);
  sprintf(keyword, "segments");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_segments, sizeof(int));

  memset(keyword, 0, 100);
  sprintf(keyword, "color");
  fout.write((char*)keyword, strlen(keyword)+1);
  f[0] = m_color.x;
  f[1] = m_color.y;
  f[2] = m_color.z;
  fout.write((char*)&f, 3*sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "lengthcolor");
  fout.write((char*)keyword, strlen(keyword)+1);
  f[0] = m_lengthColor.x;
  f[1] = m_lengthColor.y;
  f[2] = m_lengthColor.z;
  fout.write((char*)&f, 3*sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "opacity");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_opacity, sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "arrowheadlength");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_arrowHeadLength, sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "lengthtextdistance");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_lengthTextDistance, sizeof(int));

  int npts = m_points.count();
  memset(keyword, 0, 100);
  sprintf(keyword, "points");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&npts, sizeof(int));
  for(int i=0; i<npts; i++)
    {
      f[0] = m_points[i].x;
      f[1] = m_points[i].y;
      f[2] = m_points[i].z;
      fout.write((char*)&f, 3*sizeof(float));
    }

  memset(keyword, 0, 100);
  sprintf(keyword, "crosssection");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&npts, sizeof(int));
  for(int i=0; i<npts; i++)
    {
      f[0] = m_pointRadX[i];
      f[1] = m_pointRadY[i];
      f[2] = m_pointAngle[i];
      fout.write((char*)&f, 3*sizeof(float));
    }

  memset(keyword, 0, 100);
  sprintf(keyword, "captionpresent");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_captionPresent, sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "captionlabel");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_captionLabel, sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "captiontext");
  fout.write((char*)keyword, strlen(keyword)+1);
  len = m_captionText.size()+1;
  fout.write((char*)&len, sizeof(int));
  if (len > 0)
    fout.write((char*)m_captionText.toLatin1().data(), len*sizeof(char));

  memset(keyword, 0, 100);
  sprintf(keyword, "captionfont");
  fout.write((char*)keyword, strlen(keyword)+1);
  QString fontStr = m_captionFont.toString();
  len = fontStr.size()+1;
  fout.write((char*)&len, sizeof(int));
  if (len > 0)	
    fout.write((char*)fontStr.toLatin1().data(), len*sizeof(char));
  
  memset(keyword, 0, 100);
  sprintf(keyword, "captioncolor");
  fout.write((char*)keyword, strlen(keyword)+1);
  unsigned char r = m_captionColor.red();
  unsigned char g = m_captionColor.green();
  unsigned char b = m_captionColor.blue();
  unsigned char a = m_captionColor.alpha();
  fout.write((char*)&r, sizeof(unsigned char));
  fout.write((char*)&g, sizeof(unsigned char));
  fout.write((char*)&b, sizeof(unsigned char));
  fout.write((char*)&a, sizeof(unsigned char));

  memset(keyword, 0, 100);
  sprintf(keyword, "captionhalocolor");
  fout.write((char*)keyword, strlen(keyword)+1);
  r = m_captionHaloColor.red();
  g = m_captionHaloColor.green();
  b = m_captionHaloColor.blue();
  a = m_captionHaloColor.alpha();
  fout.write((char*)&r, sizeof(unsigned char));
  fout.write((char*)&g, sizeof(unsigned char));
  fout.write((char*)&b, sizeof(unsigned char));
  fout.write((char*)&a, sizeof(unsigned char));

  memset(keyword, 0, 100);
  sprintf(keyword, "imagepresent");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_imagePresent, sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "imagename");
  fout.write((char*)keyword, strlen(keyword)+1);
  len = m_imageName.size()+1;
  fout.write((char*)&len, sizeof(int));
  if (len > 0)
    fout.write((char*)m_imageName.toLatin1().data(), len*sizeof(char));

  memset(keyword, 0, 100);
  sprintf(keyword, "viewportstyle");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_viewportStyle, sizeof(bool));
  
  memset(keyword, 0, 100);
  sprintf(keyword, "viewporttf");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_viewportTF, sizeof(int));
  
  memset(keyword, 0, 100);
  sprintf(keyword, "viewport");
  fout.write((char*)keyword, strlen(keyword)+1);
  f[0] = m_viewport.x();
  f[1] = m_viewport.y();
  f[2] = m_viewport.z();
  f[3] = m_viewport.w();
  fout.write((char*)&f, 4*sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "viewportcam");
  fout.write((char*)keyword, strlen(keyword)+1);
  f[0] = m_viewportCamPos.x;
  f[1] = m_viewportCamPos.y;
  f[2] = m_viewportCamPos.z;
  f[3] = m_viewportCamRot;
  fout.write((char*)&f, 4*sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "pathobjectend");
  fout.write((char*)keyword, strlen(keyword)+1);
}

void
PathObject::load(fstream &fin)
{
  m_add = false;
  m_points.clear();
  m_pointRadX.clear();
  m_pointRadY.clear();
  m_pointAngle.clear();
  m_tgP.clear();
  m_path.clear();
  m_radX.clear();
  m_radY.clear();
  m_angle.clear();
  resetCaption();
  resetImage();

  m_viewportStyle = true;
  m_viewportTF = -1;
  m_viewportGrabbed = false;
  m_viewport = QVector4D(-1,-1,-1,-1);
  m_viewportCamPos = Vec(0, 0, 0);
  m_viewportCamRot = 0.0;

  bool done = false;
  char keyword[100];
  float f[5];
  while (!done)
    {
      fin.getline(keyword, 100, 0);

      if (strcmp(keyword, "pathobjectend") == 0)
	done = true;
      else if (strcmp(keyword, "usetype") == 0)
	fin.read((char*)&m_useType, sizeof(bool));
      else if (strcmp(keyword, "keepinside") == 0)
	fin.read((char*)&m_keepInside, sizeof(bool));
      else if (strcmp(keyword, "keepends") == 0)
	fin.read((char*)&m_keepEnds, sizeof(bool));
      else if (strcmp(keyword, "blendtf") == 0)
	fin.read((char*)&m_blendTF, sizeof(int));
      else if (strcmp(keyword, "allowinterpolate") == 0)
	fin.read((char*)&m_allowInterpolate, sizeof(int));
      else if (strcmp(keyword, "showpoints") == 0)
	fin.read((char*)&m_showPoints, sizeof(bool));
      else if (strcmp(keyword, "showlength") == 0)
	fin.read((char*)&m_showLength, sizeof(bool));
      else if (strcmp(keyword, "showangle") == 0)
	fin.read((char*)&m_showAngle, sizeof(bool));
      else if (strcmp(keyword, "tube") == 0)
	fin.read((char*)&m_tube, sizeof(bool));
      else if (strcmp(keyword, "closed") == 0)
	fin.read((char*)&m_closed, sizeof(bool));
      else if (strcmp(keyword, "captype") == 0)
	fin.read((char*)&m_capType, sizeof(int));
      else if (strcmp(keyword, "arrowdirection") == 0)
	fin.read((char*)&m_arrowDirection, sizeof(bool));
      else if (strcmp(keyword, "arrowforall") == 0)
	fin.read((char*)&m_arrowForAll, sizeof(bool));
      else if (strcmp(keyword, "halfsection") == 0)
	fin.read((char*)&m_halfSection, sizeof(bool));
      else if (strcmp(keyword, "sections") == 0)
	fin.read((char*)&m_sections, sizeof(int));
      else if (strcmp(keyword, "segments") == 0)
	fin.read((char*)&m_segments, sizeof(int));
      else if (strcmp(keyword, "color") == 0)
	{
	  fin.read((char*)&f, 3*sizeof(float));
	  m_color = Vec(f[0], f[1], f[2]);
	}
      else if (strcmp(keyword, "lengthcolor") == 0)
	{
	  fin.read((char*)&f, 3*sizeof(float));
	  m_lengthColor = Vec(f[0], f[1], f[2]);
	}
      else if (strcmp(keyword, "opacity") == 0)
	fin.read((char*)&m_opacity, sizeof(float));
      else if (strcmp(keyword, "arrowheadlength") == 0)
	fin.read((char*)&m_arrowHeadLength, sizeof(float));
      else if (strcmp(keyword, "lengthtextdistance") == 0)
	fin.read((char*)&m_lengthTextDistance, sizeof(int));
      else if (strcmp(keyword, "points") == 0)
	{
	  int npts;
	  fin.read((char*)&npts, sizeof(int));
	  for(int i=0; i<npts; i++)
	    {
	      fin.read((char*)&f, 3*sizeof(float));
	      m_points.append(Vec(f[0], f[1], f[2]));
	    }
	}
      else if (strcmp(keyword, "crosssection") == 0)
	{
	  int npts;
	  fin.read((char*)&npts, sizeof(int));
	  for(int i=0; i<npts; i++)
	    {
	      fin.read((char*)&f, 3*sizeof(float));
	      m_pointRadX.append(f[0]);
	      m_pointRadY.append(f[1]);
	      m_pointAngle.append(f[2]);
	    }
	}
      else if (strcmp(keyword, "captionpresent") == 0)
	fin.read((char*)&m_captionPresent, sizeof(bool));
      else if (strcmp(keyword, "captionlabel") == 0)
	fin.read((char*)&m_captionLabel, sizeof(bool));
      else if (strcmp(keyword, "captiontext") == 0)
	{
	  int len;
	  fin.read((char*)&len, sizeof(int));
	  if (len > 0)
	    {
	      char *str = new char[len];
	      fin.read((char*)str, len*sizeof(char));
	      m_captionText = QString(str);
	      delete [] str;
	    }
	}
      else if (strcmp(keyword, "captionfont") == 0)
	{
	  int len;
	  fin.read((char*)&len, sizeof(int));
	  if (len > 0)
	    {
	      char *str = new char[len];
	      fin.read((char*)str, len*sizeof(char));
	      QString fontStr = QString(str);
	      m_captionFont.fromString(fontStr); 
	      delete [] str;
	    }
	}
      else if (strcmp(keyword, "captioncolor") == 0)
	{
	  unsigned char r, g, b, a;
	  fin.read((char*)&r, sizeof(unsigned char));
	  fin.read((char*)&g, sizeof(unsigned char));
	  fin.read((char*)&b, sizeof(unsigned char));
	  fin.read((char*)&a, sizeof(unsigned char));
	  m_captionColor = QColor(r,g,b,a);
	}
      else if (strcmp(keyword, "captionhalocolor") == 0)
	{
	  unsigned char r, g, b, a;
	  fin.read((char*)&r, sizeof(unsigned char));
	  fin.read((char*)&g, sizeof(unsigned char));
	  fin.read((char*)&b, sizeof(unsigned char));
	  fin.read((char*)&a, sizeof(unsigned char));
	  m_captionHaloColor = QColor(r,g,b,a);
	}
      else if (strcmp(keyword, "imagepresent") == 0)
	fin.read((char*)&m_imagePresent, sizeof(bool));
      else if (strcmp(keyword, "imagename") == 0)
	{
	  int len;
	  fin.read((char*)&len, sizeof(int));
	  if (len > 0)
	    {
	      char *str = new char[len];
	      fin.read((char*)str, len*sizeof(char));
	      m_imageName = QString(str);
	      delete [] str;
	    }
	}
      else if (strcmp(keyword, "viewportstyle") == 0)
	fin.read((char*)&m_viewportStyle, sizeof(bool));
      else if (strcmp(keyword, "viewporttf") == 0)
	fin.read((char*)&m_viewportTF, sizeof(int));
      else if (strcmp(keyword, "viewport") == 0)
	{
	  fin.read((char*)&f, 4*sizeof(float));	      
	  m_viewport = QVector4D(f[0],f[1],f[2],f[3]);
	}
      else if (strcmp(keyword, "viewportcam") == 0)
	{
	  fin.read((char*)&f, 4*sizeof(float));	      
	  m_viewportCamPos = Vec(f[0],f[1],f[2]);
	  m_viewportCamRot = f[3];
	}
    }


  m_undo.clear();
  m_undo.append(m_points, m_pointRadX, m_pointRadY, m_pointAngle);
}

void
PathObject::undo()
{
  m_undo.undo();

  m_points = m_undo.points();
  m_pointRadX = m_undo.pointRadX();
  m_pointRadY = m_undo.pointRadY();
  m_pointAngle = m_undo.pointAngle();

  computeTangents();
  m_updateFlag = true;
}

void
PathObject::redo()
{
  m_undo.redo();

  m_points = m_undo.points();
  m_pointRadX = m_undo.pointRadX();
  m_pointRadY = m_undo.pointRadY();
  m_pointAngle = m_undo.pointAngle();

  computeTangents();
  m_updateFlag = true;
}

void
PathObject::updateUndo()
{
  m_undo.append(m_points, m_pointRadX, m_pointRadY, m_pointAngle);
}

bool
operator==(const PathObject& a,
	   const PathObject& b)
{
  return !(a!=b);
}

bool
operator!=(const PathObject& a,
	   const PathObject& b)
{
  if (a.m_useType != b.m_useType)
    return true;

  if (a.m_keepInside != b.m_keepInside)
    return true;

  if (a.m_keepEnds != b.m_keepEnds)
    return true;

  if (a.m_blendTF != b.m_blendTF)
    return true;

  if(a.m_points.count() != b.m_points.count())
    return true;

  for (int i=0; i<a.m_points.count(); i++)
    if (a.m_points[i] != b.m_points[i])
      return true;

  for (int i=0; i<a.m_points.count(); i++)
    if (qAbs(a.m_pointRadX[i]-b.m_pointRadX[i]) > 0.001)
      return true;

  for (int i=0; i<a.m_points.count(); i++)
    if (qAbs(a.m_pointRadY[i]-b.m_pointRadY[i]) > 0.001)
      return true;

  for (int i=0; i<a.m_points.count(); i++)
    if (qAbs(a.m_pointAngle[i]-b.m_pointAngle[i]) > 0.001)
      return true;

  return false;
}

PathObject
PathObject::interpolate(PathObject po1,
			PathObject po2,
			float frc)
{
  PathObject po;
  po = po1;

  QList<Vec> pt1 = po1.points();
  QList<Vec> pt2 = po2.points();
  if (pt1.count() == pt2.count())
    {
      QList<Vec> pt;
      for(int i=0; i<pt1.count(); i++)
	{
	  Vec v;
	  v = (1-frc)*pt1[i] + frc*pt2[i];
	  pt << v;
	}

      QList<float> rd1 = po1.radX();
      QList<float> rd2 = po2.radX();
      QList<float> rdX;
      for(int i=0; i<pt1.count(); i++)
	{
	  float v;
	  v = (1-frc)*rd1[i] + frc*rd2[i];
	  rdX << v;
	}

      rd1 = po1.radY();
      rd2 = po2.radY();
      QList<float> rdY;
      for(int i=0; i<pt1.count(); i++)
	{
	  float v;
	  v = (1-frc)*rd1[i] + frc*rd2[i];
	  rdY << v;
	}

      rd1 = po1.angle();
      rd2 = po2.angle();
      QList<float> a;
      for(int i=0; i<pt1.count(); i++)
	{
	  float v;
	  v = (1-frc)*rd1[i] + frc*rd2[i];
	  a << v;
	}

      po.replace(pt, rdX, rdY, a);
    }

  po.m_sections = (1-frc)*po1.m_sections + frc*po2.m_sections;
  po.m_segments = (1-frc)*po1.m_segments + frc*po2.m_segments;
  po.m_opacity = (1-frc)*po1.m_opacity + frc*po2.m_opacity;
  po.m_color = (1-frc)*po1.m_color + frc*po2.m_color;

  po.m_viewport = po1.m_viewport + frc*(po2.m_viewport-
					po1.m_viewport);
  po.m_viewportCamPos = po1.m_viewportCamPos + frc*(po2.m_viewportCamPos-
						    po1.m_viewportCamPos);
  po.m_viewportCamRot = po1.m_viewportCamRot + frc*(po2.m_viewportCamRot-
						    po1.m_viewportCamRot);


  return po;
}
QList<PathObject>
PathObject::interpolate(QList<PathObject> po1,
			QList<PathObject> po2,
			float frc)
{
  QList<PathObject> po;
  for(int i=0; i<po1.count(); i++)
    {
      PathObject pi;
      if (i < po2.count() && po1[i].allowInterpolate())
	{
	  pi = interpolate(po1[i], po2[i], frc);
	  po.append(pi);
	}
      else
	{
	  pi = po1[i];
	  po.append(pi);
	}
    }

  return po;
}

bool
PathObject::checkCropped(Vec po)
{
  int npts = qMin(20, m_points.count());

  Vec tgPu0 = m_tgP[0].unit();
  Vec tgPu1 = m_tgP[npts-1].unit();

  // find nearest point
  int ptn = 0;
  float d = (po-m_points[0])*(po-m_points[0]);
  Vec proj = m_points[0];
  Vec xaxis = m_xaxis[0];
  Vec yaxis = m_yaxis[0];
  float rd = m_pointRadX[0];
  for(int i=0; i<npts; i++)
    {
      Vec v0 = po - m_points[i];
      float dd = v0*v0;
      if (dd < d)
	{
	  ptn = i;
	  d = dd;
	  proj = m_points[i];
	  xaxis = m_xaxis[i];
	  yaxis = m_yaxis[i];
	  rd = m_pointRadX[i];
	}
      Vec tang, tp0, tp1;
      float ltang;
      int i0, i1;
      if (i < npts-1)
	{ i0 = i; i1 = i+1; }
      else
	{ i0 = i-1; i1 = i; }
      tang = m_points[i1]-m_points[i0];
      ltang = tang.norm();
      tang.normalize();
      tp0 = po - m_points[i0];
      float d0 = tp0*tang;
      if (d0 >= 0.0 && d0 <= ltang)
	{
	  float frc = d0/ltang;
	  Vec pp = interpolate(i0, i1, frc);
	  float dd = (po-pp)*(po-pp);
	  if (dd < d)
	    {
	      ptn = -1;
	      d = dd;
	      proj = pp;
	      xaxis = (1-frc)*m_xaxis[i0] + frc*m_xaxis[i1];
	      yaxis = (1-frc)*m_yaxis[i0] + frc*m_yaxis[i1];
	      rd = (1-frc)*m_pointRadX[i0] + frc*m_pointRadX[i1];
	    }
	}
    }

  float op;
  if (m_useType == 1)
    op = StaticFunctions::smoothstep(qMax(0.0,rd-3.0), rd, (po-proj).norm());
  else
    {
      float a = (po-proj)*xaxis;
      if (m_useType == 2)
	{
	  // mystery : why is this slow !!!!
	  //shader += "  float op = smoothstep(0.0, 3.0, a);

	  // this is faster ?????
	  op = StaticFunctions::smoothstep(qMax(0.0,rd-300.0), 0.0, qAbs(a));
	  float b = (po-proj)*yaxis;
	  if (b < 0.0)
	    op *= StaticFunctions::smoothstep(-3.0, 0.0, b);
	}
      else
	{
	  op = StaticFunctions::smoothstep(qMax(0.0,rd-3.0), rd, qAbs(a));
	  if (m_useType == 4)
		{
		  float b = (po-proj)*yaxis;
		  if (b < 0.0)
		    op *= StaticFunctions::smoothstep(-3.0, 0.0, b);
		}
	}
    }


  if (m_keepInside) op = 1.0-op;

  if (!m_keepEnds)
    {
      if (ptn == 0 && tgPu0*(po-proj) <= 0) return false;
      else if (ptn == npts-1 && tgPu1*(po-proj) >= 0) return false;
    }
  else
    {
      if (ptn == 0 && tgPu0*(po-proj) <= 0) return true;
      else if (ptn == npts-1 && tgPu1*(po-proj) >= 0) return true;
    }

  if (op <= 0.0) return false;

  return true;
}

float
PathObject::checkBlend(Vec po)
{
  int npts = qMin(20, m_points.count());

  Vec tgPu0 = m_tgP[0].unit();
  Vec tgPu1 = m_tgP[npts-1].unit();

  // find nearest point
  int ptn = 0;
  float d = (po-m_points[0])*(po-m_points[0]);
  Vec proj = m_points[0];
  Vec xaxis = m_xaxis[0];
  Vec yaxis = m_yaxis[0];
  float rd = m_pointRadX[0];
  for(int i=0; i<npts; i++)
    {
      Vec v0 = po - m_points[i];
      float dd = v0*v0;
      if (dd < d)
	{
	  ptn = i;
	  d = dd;
	  proj = m_points[i];
	  xaxis = m_xaxis[i];
	  yaxis = m_yaxis[i];
	  rd = m_pointRadX[i];
	}
      Vec tang, tp0, tp1;
      float ltang;
      int i0, i1;
      if (i < npts-1)
	{ i0 = i; i1 = i+1; }
      else
	{ i0 = i-1; i1 = i; }
      tang = m_points[i1]-m_points[i0];
      ltang = tang.norm();
      tang.normalize();
      tp0 = po - m_points[i0];
      float d0 = tp0*tang;
      if (d0 >= 0.0 && d0 <= ltang)
	{
	  float frc = d0/ltang;
	  Vec pp = interpolate(i0, i1, frc);
	  float dd = (po-pp)*(po-pp);
	  if (dd < d)
	    {
	      ptn = -1;
	      d = dd;
	      proj = pp;
	      xaxis = (1-frc)*m_xaxis[i0] + frc*m_xaxis[i1];
	      yaxis = (1-frc)*m_yaxis[i0] + frc*m_yaxis[i1];
	      rd = (1-frc)*m_pointRadX[i0] + frc*m_pointRadX[i1];
	    }
	}
    }

  float op;
  if (m_useType == 5)
    op = StaticFunctions::smoothstep(qMax(0.0,rd-3.0), rd, (po-proj).norm());
  else
    {
      float a = (po-proj)*xaxis;
      if (m_useType == 6)
	{
	  // mystery : why is this slow !!!!
	  //shader += "  float op = smoothstep(0.0, 3.0, a);

	  // this is faster ?????
	  op = StaticFunctions::smoothstep(qMax(0.0,rd-300.0), 0.0, qAbs(a));
	  float b = (po-proj)*yaxis;
	  if (b < 0.0)
	    op *= StaticFunctions::smoothstep(-3.0, 0.0, b);
	}
      else
	{
	  op = StaticFunctions::smoothstep(qMax(0.0,rd-3.0), rd, qAbs(a));
	  if (m_useType == 8)
		{
		  float b = (po-proj)*yaxis;
		  if (b < 0.0)
		    op *= StaticFunctions::smoothstep(-3.0, 0.0, b);
		}
	}
    }

  op = 1.0-op;
  if (m_keepInside) op = 1.0-op;

  if (!m_keepEnds)
    {
      if (ptn == 0 && tgPu0*(po-proj) <= 0)
	op = 1;
      else if (ptn == npts-1 && tgPu1*(po-proj) >= 0)
	op = 1;
    }
  else
    {
      if (ptn == 0 && tgPu0*(po-proj) <= 0)
	op = 0;
      else if (ptn == npts-1 && tgPu1*(po-proj) >= 0)
	op = 0;
    }

  return op;
}

void
PathObject::makePlanar()
{
  if (m_points.count() < 3)
    return;
  makePlanar(0,1,2);
}

void
PathObject::makePlanar(int v0, int v1, int v2)
{
  int npoints = m_points.count();

  if (npoints < 3)
    return;
    
  Vec cen = m_points[0];
  for(int i=1; i<npoints; i++)
    cen += m_points[i];
  cen /= npoints;

  Vec normal;
  normal = (m_points[v0]-m_points[v1]).unit()^(m_points[v2]-m_points[v1]).unit();
  normal.normalize();
  // now shift all points into the plane
  for(int i=0; i<npoints; i++)
    {
      Vec v0 = m_points[i]-cen;
      float dv = normal*v0;
      m_points[i] -= dv*normal;
    }

  computeTangents();

  m_updateFlag = true;
  m_undo.append(m_points, m_pointRadX, m_pointRadY, m_pointAngle);
}

void
PathObject::makeCircle()
{
  int npoints = m_points.count();

  if (npoints < 3)
    return;
    
  Vec cen = m_points[0];
  for(int i=1; i<npoints; i++)
    cen += m_points[i];
  cen /= npoints;

  float avgD = 0;
  for(int i=1; i<npoints; i++)
    avgD += (m_points[i]-cen).norm();
  avgD /= npoints;

  Vec axis = Vec(0,0,0);
  for(int i=1; i<npoints; i++)
    axis += ((m_points[i]-cen)^(m_points[i-1]-cen)).unit();
  axis.normalize();

  Vec inplane = avgD*axis.orthogonalVec();

  float angle = 0;
  float stepangle = 2.0f*3.14159/npoints;
  // now shift all points into the plane
  for(int i=0; i<npoints; i++)
    {
      Quaternion q(axis, angle);
      m_points[i] = cen + q.rotate(inplane);
      angle += stepangle;
    }

  computeTangents();

  m_updateFlag = true;
  m_undo.append(m_points, m_pointRadX, m_pointRadY, m_pointAngle);
}


void PathObject::drawViewportLine(float scale, int vh)
{
  if (!m_viewportGrabbed)
    return;

  //Vec voxelSize = VolumeInformation::volumeInformation().voxelSize;
  Vec voxelSize = Global::voxelScaling();

  float clen = 0;
  glColor4f(m_color.x*m_opacity,
	    m_color.y*m_opacity,
	    m_color.z*m_opacity,
	    m_opacity);
  glBegin(GL_LINE_STRIP);
  for(int np=0; np<m_path.count(); np++)
    {
      if (np > 0)
	{
	  Vec p0 = VECPRODUCT(voxelSize,m_path[np]);
	  Vec p1 = VECPRODUCT(voxelSize,m_path[np-1]);
	  clen += (p0-p1).norm();
	}
      
      glVertex3f(clen*scale, -vh/2+5, 0.0);
    }
  glEnd();
}
void PathObject::drawViewportLineDots(QGLViewer *viewer, float scale, int vh)
{
  if (!m_viewportGrabbed)
    return;

  //Vec voxelSize = VolumeInformation::volumeInformation().voxelSize;
  Vec voxelSize = Global::voxelScaling();

  glEnable(GL_BLEND);

  glColor3f(0,0,0);
  glBegin(GL_QUADS);
  glVertex2f(-5, -vh/2);
  glVertex2f(m_length*scale+5, -vh/2);
  glVertex2f(m_length*scale+5, -vh/2+20);
  glVertex2f(-5, -vh/2+20);
  glEnd();

  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_TEXTURE_2D);

  glEnable(GL_POINT_SPRITE);
  //glBindTexture(GL_TEXTURE_2D, Global::hollowSpriteTexture());
  glBindTexture(GL_TEXTURE_2D, Global::spriteTexture());
  glTexEnvf(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE );
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
  glEnable(GL_POINT_SMOOTH);

  float clen = 0;

  glColor4f(m_color.x*m_opacity,
	    m_color.y*m_opacity,
	    m_color.z*m_opacity,
	    m_opacity);
  glPointSize(10);
  glBegin(GL_POINTS);

  QString str;
  clen = 0;
  for(int np=0; np<m_path.count(); np++)
    {
      if (np > 0)
	{
	  Vec p0 = VECPRODUCT(voxelSize,m_path[np]);
	  Vec p1 = VECPRODUCT(voxelSize,m_path[np-1]);
	  clen += (p0-p1).norm();
	}
     
      glVertex3f(clen*scale, -vh/2+5, 0.0);
    }
  glEnd();


  glColor4f(0.5,0.5,0.5,0.5);
  glPointSize(15);
  glBegin(GL_POINTS);
  clen = 0;
  for(int np=0; np<m_path.count(); np++)
    {
      if (np > 0)
	{
	  Vec p0 = VECPRODUCT(voxelSize,m_path[np]);
	  Vec p1 = VECPRODUCT(voxelSize,m_path[np-1]);
	  clen += (p0-p1).norm();
	}
      
      if (np%m_segments == 0)
	glVertex3f(clen*scale, -vh/2+5, 0.0);
    }
  glEnd();
    
  if (m_pointPressed > -1)
    {
      glColor4f(0.0,0.8,0.0,0.8);
      clen = 0;
      for(int np=0; np<m_path.count(); np++)
	{
	  if (np > 0)
	    {
	      Vec p0 = VECPRODUCT(voxelSize,m_path[np]);
	      Vec p1 = VECPRODUCT(voxelSize,m_path[np-1]);
	      clen += (p0-p1).norm();
	    }
	  
	  if (np%m_segments == 0 && np/m_segments == m_pointPressed)
	    {
	      glPointSize(20);
	      glBegin(GL_POINTS);
	      glVertex3f(clen*scale, -vh/2+5, 0.0);
	      glEnd();
	    }
	}
    }

  glDisable(GL_POINT_SPRITE);
  glDisable(GL_TEXTURE_2D);  
  glDisable(GL_POINT_SMOOTH);
  glPointSize(10);

  {
    float fscl = 120.0/Global::dpi();
    QFont font = QFont();
    QFontMetrics metric(font);
    int fht = metric.height();
    int fwd = metric.width("0")/2;
    glColor3f(1,1,1);
    float clen = 0;
    for(int np=0; np<m_path.count(); np++)
      {
	if (np > 0)
	  {
	    Vec p0 = VECPRODUCT(voxelSize,m_path[np]);
	    Vec p1 = VECPRODUCT(voxelSize,m_path[np-1]);
	    clen += (p0-p1).norm();
	  }
	
	if (np%m_segments == 0)
	  {
	    QString str = QString("%1").arg(np/m_segments);
	    StaticFunctions::renderText(clen*scale-fwd, -vh/2+2*fht,
					str, font,
					Qt::transparent, Qt::white);
	  }
      }
  }

}

void
PathObject::makeEquidistant(int npts0)
{
  int npts = npts0;
  if (npts <= 0)
    npts = m_points.count();
  npts = qMax(2, npts);

  if (m_closed) npts++;

  int xcount = m_path.count(); 
  float plen = 0;
  for (int i=1; i<xcount; i++)
    {
      Vec v = m_path[i]-m_path[i-1];
      plen += v.norm();
    }

  float delta = plen/(npts-1);

  QList<Vec> mpoints;
  QList<float> mradx;
  QList<float> mrady;
  mpoints << m_path[0];
  mradx << m_radX[0];
  mrady << m_radY[0];

  float clen = 0;
  float pclen = 0;
  int j = mpoints.count();

  for (int i=1; i<xcount; i++)
    {
      Vec a, b;
      b = m_path[i];
      a = m_path[i-1];

      Vec dv = b-a;
      clen += dv.norm();

      while (j*delta <= clen)
	{
	  double frc = (j*delta - pclen)/(clen-pclen);
	  mpoints << (a + frc*dv);
	  mradx << m_radX[i];
	  mrady << m_radY[i];

	  j = mpoints.count();
	}
      
      pclen = clen;
    }

  if (mpoints.count() < npts)
    {
      mpoints << m_path[xcount-1];
      mradx << m_radX[xcount-1];
      mrady << m_radY[xcount-1];
    }

  if (m_closed)
    {
      mpoints.removeLast();
      mradx.removeLast();
      mrady.removeLast();
    }
  
  m_points = mpoints;
  m_pointRadX = mradx;
  m_pointRadY = mrady;

  computeTangents();

  m_updateFlag = true;
  m_undo.append(m_points, m_pointRadX, m_pointRadY, m_pointAngle);
}


