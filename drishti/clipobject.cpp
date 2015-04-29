#include "global.h"
#include "staticfunctions.h"
#include "clipobject.h"
#include "volumeinformation.h"
#include "captiondialog.h"
#include "dcolordialog.h"
#include "propertyeditor.h"
#include "enums.h"
#include "matrix.h"

#include <QFileDialog>

//------------------------------------------------------------------
ClipObjectUndo::ClipObjectUndo() { clear(); }
ClipObjectUndo::~ClipObjectUndo() { clear(); }

void
ClipObjectUndo::clear()
{
  m_pos.clear();
  m_rot.clear();
  m_index = -1;
}

void
ClipObjectUndo::clearTop()
{
  if (m_index == m_pos.count()-1)
    return;

  while(m_index < m_pos.count()-1)
    m_pos.removeLast();
  
  while(m_index < m_rot.count()-1)
    m_rot.removeLast();  
}

void
ClipObjectUndo::append(Vec p, Quaternion r)
{
  clearTop();
  m_pos << p;
  m_rot << r;
  m_index = m_pos.count()-1;
}

void ClipObjectUndo::redo() { m_index = qMin(m_index+1, m_pos.count()-1); }
void ClipObjectUndo::undo() { m_index = qMax(m_index-1, 0); }

Vec
ClipObjectUndo::pos()
{
  Vec p = Vec(0,0,0);

  if (m_index >= 0 && m_index < m_pos.count())
    p = m_pos[m_index];

  return p;
}

Quaternion
ClipObjectUndo::rot()
{
  Quaternion r = Quaternion(Vec(1,0,0), 0);

  if (m_index >= 0 && m_index < m_pos.count())
    r = m_rot[m_index];

  return r;
}
//------------------------------------------------------------------


int ClipObject::tfset() { return m_tfset; }
void ClipObject::setTFset(int tf) { m_tfset = tf; }

bool ClipObject::viewportGrabbed() { return m_viewportGrabbed; }
void ClipObject::setViewportGrabbed(bool v) { m_viewportGrabbed = v; }

QVector4D ClipObject::viewport() { return m_viewport; }
void ClipObject::setViewport(QVector4D v) { m_viewport = v; }

bool ClipObject::viewportType() { return m_viewportType; }
void ClipObject::setViewportType(bool v) { m_viewportType = v; }

float ClipObject::viewportScale() { return m_viewportScale; }
void ClipObject::setViewportScale(float v) { m_viewportScale = v; }

int ClipObject::thickness() { return m_thickness; }
void ClipObject::setThickness(int v) { m_thickness = v; }

bool ClipObject::showSlice() { return m_showSlice; }
void ClipObject::setShowSlice(bool b) { m_showSlice = b; }

bool ClipObject::showThickness() { return m_showThickness; }
void ClipObject::setShowThickness(bool b) { m_showThickness = b; }

bool ClipObject::showOtherSlice() { return m_showOtherSlice; }
void ClipObject::setShowOtherSlice(bool b) { m_showOtherSlice = b; }

bool ClipObject::apply() { return m_apply; }
void ClipObject::setApply(bool b) { m_apply = b; }

void ClipObject::setActive(bool a) { m_active = a; }
float ClipObject::size() { return m_size; }

bool ClipObject::flip() { return m_applyFlip; }
void ClipObject::setFlip(bool v) { m_applyFlip = v; }

float ClipObject::stereo() { return m_stereo; }
void ClipObject::setStereo(float v) { m_stereo = v; }

float ClipObject::opacity() { return m_opacity; }
void ClipObject::setOpacity(float v) { m_applyOpacity = true; m_opacity = v; }

Vec ClipObject::color() { return m_color; }
void ClipObject::setColor(Vec color) { m_color = color; }

bool ClipObject::solidColor() { return m_solidColor; }
void ClipObject::setSolidColor(bool sc) { m_solidColor = sc; }

float ClipObject::scale1() { return m_scale1; }
float ClipObject::scale2() { return m_scale2; }
float ClipObject::tscale1() { return m_tscale1; }
float ClipObject::tscale2() { return m_tscale2; }
void ClipObject::setScale1(float v) { m_scale1 = v; }
void ClipObject::setScale2(float v) { m_scale2 = v; }

QString ClipObject::captionText() { return m_captionText; }
QFont ClipObject::captionFont() { return m_captionFont; }
QColor ClipObject::captionColor() { return m_captionColor; }
QColor ClipObject::captionHaloColor() { return m_captionHaloColor; }

QString ClipObject::imageName() { return m_imageName; }
int ClipObject::imageFrame() { return m_imageFrame; }
void ClipObject::setImage(QString v, int n) { loadImage(v, n); }

int ClipObject::moveAxis() { return m_moveAxis; }
void ClipObject::setMoveAxis(int type) { m_moveAxis = type; }

bool ClipObject::mopClip() { return m_mopClip; }
bool ClipObject::reorientCamera() { return m_reorientCamera; }
bool ClipObject::saveSliceImage() { return m_saveSliceImage; }
bool ClipObject::resliceVolume() { return m_resliceVolume; }
int ClipObject::resliceSubsample() { return m_resliceSubsample; }
int ClipObject::resliceTag() { return m_resliceTag; }

void ClipObject::setGridX(int g) { m_gridX = g; }
void ClipObject::setGridY(int g) { m_gridY = g; }
int ClipObject::gridX() { return m_gridX; }
int ClipObject::gridY() { return m_gridY; }

ClipObject::ClipObject()
{
  m_undo.clear();

  m_mopClip = false;
  m_reorientCamera = false;
  m_saveSliceImage = false;
  m_resliceVolume = false;
  m_resliceSubsample = 1;
  m_resliceTag = -1;

  m_solidColor = false;
  m_show = true;
  m_displaylist = 0;
  m_moveAxis = MoveAll;

  m_color = Vec(0.8,0.7,0.9);
  m_opacity = 1;
  m_active = false;

  m_stereo = 1;

  m_tfset = -1; // i.e. do not texture clipplane
  m_viewport = QVector4D(-1,-1,-1,-1);
  m_viewportType = true;
  m_viewportScale = 1.0f;
  m_viewportGrabbed = false;
  m_thickness = 0;
  m_showSlice = true;
  m_showThickness = true;
  m_showOtherSlice = true;
  m_apply = true;

  m_applyOpacity = true;
  m_applyFlip = false;

  m_scale1 = 1.0;
  m_scale2 = 1.0;
  m_tscale1 = 1.0;
  m_tscale2 = 1.0;

  m_position = Vec(0,0,0);
  m_quaternion = Quaternion(Vec(0,0,1), 0);
  m_tang = Vec(0,0,1);
  m_xaxis = Vec(1,0,0);
  m_yaxis = Vec(0,1,0);

  m_textureWidth = 0;
  m_textureHeight = 0;

  m_imagePresent = false;
  m_imageName.clear();
  m_imageFrame = 0;

  m_captionPresent = false;
  m_captionText.clear();
  m_captionFont = QFont("Helvetica", 48);
  m_captionColor = Qt::white;
  m_captionHaloColor = Qt::white;

  m_gridX = m_gridY = 0;

  Matrix::identity(m_xform);

  glGenTextures(1, &m_imageTex);
}

ClipObject::~ClipObject()
{
  if (m_displaylist)
    glDeleteLists(m_displaylist, 1);

  glDeleteTextures(1, &m_imageTex);

  m_undo.clear();
}

void
ClipObject::clearCaption()
{
  m_captionPresent = false;
  m_captionText.clear();
  m_captionFont = QFont("Helvetica", 48);
  m_captionColor = Qt::white;
  m_captionHaloColor = Qt::white;
}

void
ClipObject::setCaption(QString ct, QFont cf,
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
ClipObject::loadCaption()
{
  CaptionDialog cd(0,
		   m_captionText,
		   m_captionFont,
		   m_captionColor,
		   m_captionHaloColor);
  cd.hideOpacity(true);
  cd.move(QCursor::pos());
  if (cd.exec() != QDialog::Accepted)
    return;

  loadCaption(cd.text(),
	      cd.font(),
	      cd.color(),
	      cd.haloColor());
}

void
ClipObject::loadCaption(QString ct, QFont cf,
			QColor cc, QColor chc)
{
  m_captionText = ct;
  m_captionFont = cf;
  m_captionColor = cc;
  m_captionHaloColor = chc;

  if (m_captionText.isEmpty())
    {
      m_captionPresent = false;
      return;
    }

  QFontMetrics metric(m_captionFont);
  int mde = metric.descent();
  int fht = metric.height();
  int fwd = metric.width(m_captionText)+2;

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
    bpainter.drawText(1, fht-mde, m_captionText);

    uchar *dbits = new uchar[4*fht*fwd];
    uchar *bits = bImage.bits();
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
  cpainter.drawText(1, fht-mde, m_captionText);
  m_textureWidth = fwd;
  m_textureHeight = fht;
      
  unsigned char *image = new unsigned char[4*m_textureWidth*m_textureHeight];
  memcpy(image, cImage.bits(), 4*m_textureWidth*m_textureHeight); 

  if (m_imageTex)
    glDeleteTextures(1, &m_imageTex);
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
	       image);
  glDisable(GL_TEXTURE_RECTANGLE_ARB);

  delete [] image;

  clearImage();

  m_captionPresent = true;
}

void
ClipObject::clearImage()
{
  m_imagePresent = false;
  m_imageName.clear();
  m_imageFrame = 0;
}

void
ClipObject::loadImage()
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

  loadImage(imgFile, 0);
}

void
ClipObject::loadImage(QString imgFile, int imgFrame)
{
  if (m_imageName == imgFile && m_imageFrame == imgFrame)
    return;

  m_imageName = imgFile;
  m_imageFrame = imgFrame;

  if (m_imageName.isEmpty())
    {
      m_imageFrame = 0;
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
      m_imagePresent = true;

      clearCaption();
      return;
    }

  QMovie movie(absoluteImageFile);
  movie.setCacheMode(QMovie::CacheAll);
  movie.start();
  movie.setPaused(true);
  movie.jumpToFrame(0);

  if (movie.jumpToFrame(m_imageFrame) == false)
    movie.jumpToFrame(0);

  QImage mapImage(movie.currentImage());
  m_textureHeight = mapImage.height();
  m_textureWidth = mapImage.width();
  int nbytes = mapImage.byteCount();
  int rgb = nbytes/(m_textureWidth*m_textureHeight);

  unsigned char *image = new unsigned char[rgb*m_textureWidth*m_textureHeight];
  memcpy(image, mapImage.bits(), rgb*m_textureWidth*m_textureHeight);

  GLuint fmt;
  if (rgb == 1) fmt = GL_LUMINANCE;
  else if (rgb == 2) fmt = GL_LUMINANCE_ALPHA;
  else if (rgb == 3) fmt = GL_RGB;
  else if (rgb == 4) fmt = GL_BGRA;

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
	       image);
  glDisable(GL_TEXTURE_RECTANGLE_ARB);

  delete [] image;

  clearCaption();

  m_imagePresent = true;
}

void
ClipObject::translate(Vec trans)
{
  m_position += trans;  

  m_undo.append(m_position, m_quaternion);
}

Vec ClipObject::position() { return m_position; }
Quaternion ClipObject::orientation() { return m_quaternion; }

void ClipObject::setPosition(Vec pos)
{
  m_position = pos;

  m_undo.append(m_position, m_quaternion);
}

void
ClipObject::setOrientation(Quaternion q)
{
  m_quaternion = q;
  m_tang = m_quaternion.rotate(Vec(0,0,1));
  m_xaxis = m_quaternion.rotate(Vec(1,0,0));
  m_yaxis = m_quaternion.rotate(Vec(0,1,0));

  m_undo.append(m_position, m_quaternion);
}
void
ClipObject::rotate(Vec axis, float angle)
{
  Quaternion q(axis, DEG2RAD(angle));
  m_quaternion = q*m_quaternion;

  m_tang = m_quaternion.rotate(Vec(0,0,1));
  m_xaxis = m_quaternion.rotate(Vec(1,0,0));
  m_yaxis = m_quaternion.rotate(Vec(0,1,0));

  m_undo.append(m_position, m_quaternion);
}

void ClipObject::normalize()
{
  Vec pt = m_position;
  pt = Vec((int)pt.x, (int)pt.y, (int)pt.z);
  m_position = pt;

  m_undo.append(m_position, m_quaternion);
}

void
ClipObject::draw(QGLViewer *viewer,
		 bool backToFront, float widgetSize)
{
  m_size = widgetSize;
  computeTscale();

  if (!m_show)
    return;

  if (m_imagePresent ||
      m_captionPresent ||
      (m_gridX > 0 && m_gridY > 0) )
    {
      if (m_gridX > 0 && m_gridY > 0)
	drawGrid();

      if (m_imagePresent || m_captionPresent)
	drawCaptionImage();

      if (m_active) drawLines(viewer, backToFront);
    }
  else
    drawLines(viewer, backToFront);
}

void
ClipObject::drawGrid()
{
  float s1 = m_tscale1;
  float s2 = m_tscale2;

  glColor4f(m_color.x*m_opacity,
	    m_color.y*m_opacity,
	    m_color.z*m_opacity,
	    m_opacity);

  Vec tang = m_tang;
  Vec xaxis = m_xaxis;
  Vec yaxis = m_yaxis;
  tang = Matrix::rotateVec(m_xform, tang);
  xaxis = Matrix::rotateVec(m_xform, xaxis);
  yaxis = Matrix::rotateVec(m_xform, yaxis);

  Vec voxelScaling = Global::voxelScaling();
  Vec opt = VECPRODUCT(m_position, voxelScaling);
  opt = Matrix::xformVec(m_xform, opt);
  Vec c0, c1, c2, c3;
  c0 = opt - s1*xaxis + s2*yaxis;
  c1 = opt - s1*xaxis - s2*yaxis;
  c2 = opt + s1*xaxis - s2*yaxis;
  c3 = opt + s1*xaxis + s2*yaxis;

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, Global::boxTexture());
  glEnable(GL_TEXTURE_2D);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); 
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); 

  glEnable(GL_POLYGON_OFFSET_FILL);
  glPolygonOffset(1.0, 1.0);
  glBegin(GL_QUADS); 
  glTexCoord2f(0, 0);              glVertex3fv(c0);
  glTexCoord2f(0, m_gridY);        glVertex3fv(c1);
  glTexCoord2f(m_gridX, m_gridY);  glVertex3fv(c2);
  glTexCoord2f(m_gridX, 0);        glVertex3fv(c3);
  glEnd();
  glPolygonOffset(0.0, 0.0);
  glDisable(GL_POLYGON_OFFSET_FILL);

  glDisable(GL_TEXTURE_2D);
}

void
ClipObject::drawCaptionImage()
{
  glLineWidth(1);
 
  float s1 = m_tscale1;
  float s2 = m_tscale2;

  glColor4f(0, 0, 0, m_opacity);
  if (m_applyOpacity)
    glColor4f(m_opacity,
	      m_opacity,
	      m_opacity,
	      m_opacity);
  else
    glColor4f(1,1,1,1);
  
  if (m_applyFlip)
    {
      s2 = -s2;
    }

  Vec tang = m_tang;
  Vec xaxis = m_xaxis;
  Vec yaxis = m_yaxis;
  tang = Matrix::rotateVec(m_xform, tang);
  xaxis = Matrix::rotateVec(m_xform, xaxis);
  yaxis = Matrix::rotateVec(m_xform, yaxis);

  Vec voxelScaling = Global::voxelScaling();
  Vec opt = VECPRODUCT(m_position, voxelScaling);
  opt = Matrix::xformVec(m_xform, opt);
  Vec c0, c1, c2, c3;
  c0 = opt - s1*xaxis + s2*yaxis;
  c1 = opt - s1*xaxis - s2*yaxis;
  c2 = opt + s1*xaxis - s2*yaxis;
  c3 = opt + s1*xaxis + s2*yaxis;

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_imageTex);
  glTexEnvf(GL_TEXTURE_ENV,
	    GL_TEXTURE_ENV_MODE,
	    GL_MODULATE);
  glEnable(GL_TEXTURE_RECTANGLE_ARB);
  
  glEnable(GL_POLYGON_OFFSET_FILL);
  glPolygonOffset(1.0, 1.0);
  glBegin(GL_QUADS);
  glTexCoord2f(0, 0);                             glVertex3fv(c0);
  glTexCoord2f(0, m_textureHeight);               glVertex3fv(c1);
  glTexCoord2f(m_textureWidth, m_textureHeight);  glVertex3fv(c2);
  glTexCoord2f(m_textureWidth, 0);                glVertex3fv(c3);
  glEnd();
  glPolygonOffset(0.0, 0.0);
  glDisable(GL_POLYGON_OFFSET_FILL);

  glDisable(GL_TEXTURE_RECTANGLE_ARB);
}


//#define DRAWTHICKNESS()							\
//  {									\
//    float aspectRatio = m_viewport.z()/m_viewport.w();			\
//    float cdist = 2*viewer->sceneRadius()/m_viewportScale;		\
//    float fov = viewer->camera()->fieldOfView();			\
//    float yn = (cdist-m_thickness)*tan(fov*0.5);			\
//    float yf = (cdist+m_thickness)*tan(fov*0.5);			\
//    if (!m_viewportType)						\
//      {									\
//	yf = yn = cdist*tan(fov*0.5);					\
//      }									\
//    float xn = yn*aspectRatio;						\
//    float xf = yf*aspectRatio;						\
//									\
//    glBegin(GL_LINE_STRIP);						\
//    glVertex3fv(opt - xn*xaxis - yn*yaxis + m_thickness*tang);		\
//    glVertex3fv(opt - xn*xaxis + yn*yaxis + m_thickness*tang);		\
//    glVertex3fv(opt + xn*xaxis + yn*yaxis + m_thickness*tang);		\
//    glVertex3fv(opt + xn*xaxis - yn*yaxis + m_thickness*tang);		\
//    glVertex3fv(opt - xn*xaxis - yn*yaxis + m_thickness*tang);		\
//    glEnd();								\
//    glBegin(GL_LINE_STRIP);						\
//    glVertex3fv(opt - xf*xaxis - yf*yaxis - m_thickness*tang);		\
//    glVertex3fv(opt - xf*xaxis + yf*yaxis - m_thickness*tang);		\
//    glVertex3fv(opt + xf*xaxis + yf*yaxis - m_thickness*tang);		\
//    glVertex3fv(opt + xf*xaxis - yf*yaxis - m_thickness*tang);		\
//    glVertex3fv(opt - xf*xaxis - yf*yaxis - m_thickness*tang);		\
//    glEnd();								\
//									\
//    glBegin(GL_LINES);							\
//    glVertex3fv(opt - xn*xaxis - yn*yaxis + m_thickness*tang);		\
//    glVertex3fv(opt - xf*xaxis - yf*yaxis - m_thickness*tang);		\
//    glVertex3fv(opt - xn*xaxis + yn*yaxis + m_thickness*tang);		\
//    glVertex3fv(opt - xf*xaxis + yf*yaxis - m_thickness*tang);		\
//    glVertex3fv(opt + xn*xaxis + yn*yaxis + m_thickness*tang);		\
//    glVertex3fv(opt + xf*xaxis + yf*yaxis - m_thickness*tang);		\
//    glVertex3fv(opt + xn*xaxis - yn*yaxis + m_thickness*tang);		\
//    glVertex3fv(opt + xf*xaxis - yf*yaxis - m_thickness*tang);		\
//    glEnd();								\
//  }

#define DRAWTHICKNESS()							\
  {									\
    float aspectRatio = m_viewport.z()/m_viewport.w();			\
    float cdist = 2*viewer->sceneRadius()/m_viewportScale;		\
    float fov = viewer->camera()->fieldOfView();			\
    float yn = cdist*tan(fov*0.5);					\
    float yf = (cdist+2*m_thickness)*tan(fov*0.5);			\
    if (!m_viewportType)						\
      {									\
	yf = yn = cdist*tan(fov*0.5);					\
      }									\
    float xn = yn*aspectRatio;						\
    float xf = yf*aspectRatio;						\
									\
    glBegin(GL_LINE_STRIP);						\
    glVertex3fv(opt - xn*xaxis - yn*yaxis);				\
    glVertex3fv(opt - xn*xaxis + yn*yaxis);				\
    glVertex3fv(opt + xn*xaxis + yn*yaxis);				\
    glVertex3fv(opt + xn*xaxis - yn*yaxis);				\
    glVertex3fv(opt - xn*xaxis - yn*yaxis);				\
    glEnd();								\
    glBegin(GL_LINE_STRIP);						\
    glVertex3fv(opt - xf*xaxis - yf*yaxis - 2*m_thickness*tang);	\
    glVertex3fv(opt - xf*xaxis + yf*yaxis - 2*m_thickness*tang);	\
    glVertex3fv(opt + xf*xaxis + yf*yaxis - 2*m_thickness*tang);	\
    glVertex3fv(opt + xf*xaxis - yf*yaxis - 2*m_thickness*tang);	\
    glVertex3fv(opt - xf*xaxis - yf*yaxis - 2*m_thickness*tang);	\
    glEnd();								\
									\
    glBegin(GL_LINES);							\
    glVertex3fv(opt - xn*xaxis - yn*yaxis);				\
    glVertex3fv(opt - xf*xaxis - yf*yaxis - 2*m_thickness*tang);	\
    glVertex3fv(opt - xn*xaxis + yn*yaxis);				\
    glVertex3fv(opt - xf*xaxis + yf*yaxis - 2*m_thickness*tang);	\
    glVertex3fv(opt + xn*xaxis + yn*yaxis);				\
    glVertex3fv(opt + xf*xaxis + yf*yaxis - 2*m_thickness*tang);	\
    glVertex3fv(opt + xn*xaxis - yn*yaxis);				\
    glVertex3fv(opt + xf*xaxis - yf*yaxis - 2*m_thickness*tang);	\
    glEnd();								\
  }

#define DRAWCLIPWIDGET()						\
  {									\
    if (!m_solidColor)							\
      {									\
	glColor3f(m_color.x, m_color.y, m_color.z);			\
	glBegin(GL_LINE_STRIP);						\
	glVertex3fv(opt - s1*xaxis - s2*yaxis);				\
	glVertex3fv(opt - s1*xaxis + s2*yaxis);				\
	glVertex3fv(opt + s1*xaxis + s2*yaxis);				\
	glVertex3fv(opt + s1*xaxis - s2*yaxis);				\
	glVertex3fv(opt - s1*xaxis - s2*yaxis);				\
	glEnd();							\
      }									\
    else								\
      {									\
	glColor4f(m_color.x*m_opacity,					\
		  m_color.y*m_opacity,					\
		  m_color.z*m_opacity,					\
		  m_opacity);						\
	glBegin(GL_QUADS);						\
	glVertex3fv(opt - s1*xaxis - s2*yaxis);				\
	glVertex3fv(opt - s1*xaxis + s2*yaxis);				\
	glVertex3fv(opt + s1*xaxis + s2*yaxis);				\
	glVertex3fv(opt + s1*xaxis - s2*yaxis);				\
	glEnd();							\
      }									\
  }

void
ClipObject::drawLines(QGLViewer *viewer,
		      bool backToFront)
{
  bool noimage = !m_imagePresent && !m_captionPresent;
  bool quad = noimage && m_active;

  glEnable(GL_BLEND);
//  glEnable(GL_LINE_SMOOTH);
//  glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

  Vec voxelScaling = Global::voxelScaling();
  Vec opt = VECPRODUCT(m_position, voxelScaling);
  opt = Matrix::xformVec(m_xform, opt);

  float r = m_size;
  float s1 = m_tscale1;
  float s2 = m_tscale2;

  Vec tang = m_tang;
  Vec xaxis = m_xaxis;
  Vec yaxis = m_yaxis;

  tang = Matrix::rotateVec(m_xform, tang);
  xaxis = Matrix::rotateVec(m_xform, xaxis);
  yaxis = Matrix::rotateVec(m_xform, yaxis);

  if (backToFront)
    {
      if (m_active)
        glLineWidth(3);
      else
        glLineWidth(1);

      glColor3f(m_color.x, m_color.y, m_color.z);

      if (m_thickness > 0 && m_showThickness)
	DRAWTHICKNESS()
      else
	DRAWCLIPWIDGET()
    }


  glLineWidth(1);

  if (!m_solidColor || m_active)
    {
      Vec c0, ca, cb, c1;
      c0 = opt + s1*xaxis;
      ca = opt - 0.2*s2*yaxis;
      c1 = opt - s1*xaxis;
      cb = opt + 0.2*s2*yaxis;
      glColor4f(m_opacity, 0.5*m_opacity, 0, m_opacity);
      if (quad &&
	  m_moveAxis >= MoveX0 &&
	  m_moveAxis <= MoveX1) glBegin(GL_QUADS);
      else glBegin(GL_LINE_STRIP);
      glVertex3fv(c0);
      glVertex3fv(ca);
      glVertex3fv(c1);
      glVertex3fv(cb);
      if (!(quad && m_moveAxis == MoveZ)) glVertex3fv(c0);
      glEnd();

      c0 = opt + s2*yaxis;
      ca = opt - 0.2*s1*xaxis;
      c1 = opt - s2*yaxis;
      cb = opt + 0.2*s1*xaxis;
      glColor4f(0.5*m_opacity, m_opacity, 0, m_opacity);
      if (quad &&
	  m_moveAxis >= MoveY0 &&
	  m_moveAxis <= MoveY1) glBegin(GL_QUADS);
      else glBegin(GL_LINE_STRIP);
      glVertex3fv(c0);
      glVertex3fv(ca);
      glVertex3fv(c1);
      glVertex3fv(cb);
      if (!quad) glVertex3fv(c0);
      glEnd();
      
      c0 = opt + r*tang;
      Vec cax = opt - 0.2*s1*xaxis;
      Vec cbx = opt + 0.2*s1*xaxis;
      Vec cay = opt - 0.2*s2*yaxis;
      Vec cby = opt + 0.2*s2*yaxis;
      glColor4f(0, 0.5*m_opacity, m_opacity, m_opacity);
      if (quad && m_moveAxis == MoveZ) glBegin(GL_TRIANGLES);
      else glBegin(GL_LINE_STRIP);
      glVertex3fv(c0);
      glVertex3fv(cax);
      glVertex3fv(cbx);
      glVertex3fv(c0);
      glVertex3fv(cay);
      glVertex3fv(cby);
      if (!(quad && m_moveAxis == MoveZ)) glVertex3fv(c0);
      glEnd();
    }

  //  glDisable(GL_LINE_SMOOTH);


  if (!backToFront)
    {
      if (m_active)
        glLineWidth(3);
      else
        glLineWidth(1);

      glColor3f(m_color.x, m_color.y, m_color.z);

      if (m_thickness > 0 && m_showThickness)
	DRAWTHICKNESS()
      else
	DRAWCLIPWIDGET()
    }

  if (!m_solidColor || m_active)
    {
      glEnable(GL_POINT_SPRITE);
      glActiveTexture(GL_TEXTURE0);
      glEnable(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D, Global::spriteTexture());
      glTexEnvf( GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE );
      glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
      glEnable(GL_POINT_SMOOTH);

      if (m_active)
	{
	  glColor3f(1,0,0);
	  glPointSize(25);
	}
      else
	{
	  glColor3f(m_color.x, m_color.y, m_color.z);
	  glPointSize(20);
	}	
      glBegin(GL_POINTS);
      glVertex3fv(opt);
      glEnd();

      glPointSize(1);  
      
      glDisable(GL_POINT_SPRITE);
      glActiveTexture(GL_TEXTURE0);
      glDisable(GL_TEXTURE_2D);
  
      glDisable(GL_POINT_SMOOTH);
    }

  glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);      
}

void
ClipObject::postdraw(QGLViewer *viewer,
		     int x, int y,
		     bool grabsMouse)
{
  if (!grabsMouse)
    return;

//  glEnable(GL_LINE_SMOOTH);
//  glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // blend on top

  glDisable(GL_DEPTH_TEST);

  viewer->startScreenCoordinatesSystem();

  Vec voxelScaling = Global::voxelScaling();

  if (grabsMouse)
    {
      VolumeInformation pvlInfo = VolumeInformation::volumeInformation();
      QString str;
      str = QString("clip : %1 %2 %3").\
	arg(m_position.x).\
	arg(m_position.y).\
	arg(m_position.z);

      QFont font = QFont();
      QFontMetrics metric(font);
      int ht = metric.height();
      int wd = metric.width(str);
      x += 10;

      StaticFunctions::renderText(x+2, y, str, font, Qt::black, Qt::white);
    }

  glEnable(GL_DEPTH_TEST);

  viewer->stopScreenCoordinatesSystem();
}

void
ClipObject::applyUndo()
{
  m_undo.undo();
  undoParameters();
}
void
ClipObject::applyRedo()
{
  m_undo.redo();
  undoParameters();
}
void
ClipObject::undoParameters()
{
  m_position = m_undo.pos();
  m_quaternion = m_undo.rot();
  m_tang = m_quaternion.rotate(Vec(0,0,1));
  m_xaxis = m_quaternion.rotate(Vec(1,0,0));
  m_yaxis = m_quaternion.rotate(Vec(0,1,0));
}

bool
ClipObject::keyPressEvent(QKeyEvent *event)
{
  m_mopClip = false;
  m_reorientCamera = false;
  m_saveSliceImage = false;
  m_resliceVolume = false;

  if (event->key() == Qt::Key_F)
    {
      m_applyFlip = !m_applyFlip;
      return true;
    }
  else if (event->key() == Qt::Key_O)
    {
      m_applyOpacity = !m_applyOpacity;
      return true;
    }
  else if (event->key() == Qt::Key_Z &&
	   (event->modifiers() & Qt::ControlModifier ||
	    event->modifiers() & Qt::MetaModifier) )
    {
      applyUndo();
      return true;
    }
  else if (event->key() == Qt::Key_Y &&
	   (event->modifiers() & Qt::ControlModifier ||
	    event->modifiers() & Qt::MetaModifier) )
    {
      applyRedo();
      return true;
    }
  else if (event->key() == Qt::Key_Space)
    {      
      return commandEditor();
    }

  return false;
}

bool
ClipObject::processCommand(QString cmd)
{
  bool ok;
  cmd = cmd.toLower();
  QStringList list = cmd.split(" ", QString::SkipEmptyParts);
  
  if (list[0] == "mop")
    {
      if (list.size() == 2 && list[1] == "clip")
	{
	  m_mopClip = true;
	  return true;
	}
      else
	return false;
    }
  else if (list[0] == "tfset")
    {
      int tf = 1000;
      if (list.size() == 2) tf = qMax(0, list[1].toInt());
      m_tfset = tf;
      return true;
    }
  else if (list[0] == "reorientcamera")
    {
      m_reorientCamera = true;
      return true;
    }
  else if (list[0] == "color")
    {
      QColor dcolor = QColor::fromRgbF(m_color.x,
				       m_color.y,
				       m_color.z);
      QColor color = DColorDialog::getColor(dcolor);
      if (color.isValid())
	{
	  float r = color.redF();
	  float g = color.greenF();
	  float b = color.blueF();
	  m_color = Vec(r,g,b);
	}
    }
  else if (list[0] == "solidcolor")
    {
      if (list.size() == 2 &&
	  list[1] == "no")
	m_solidColor = false;
      else
	m_solidColor = true;
      return true;
    }
  else if (list[0] == "savesliceimage")
    {
      m_resliceSubsample = 1;
      m_saveSliceImage = true;
      if (list.size() == 2) m_resliceSubsample = qMax(1, list[1].toInt(&ok));
      return true;
    }
  else if (list[0] == "reslice")
    {
      m_resliceSubsample = 1;
      m_resliceTag = -1;
      m_resliceVolume = true;
      if (list.size() > 1) m_resliceSubsample = qMax(1, list[1].toInt(&ok));
      if (list.size() > 2) m_resliceTag = list[2].toInt(&ok);
      return true;
    }
  else if (list[0] == "grid")
    {
      if (list.size() == 1)
	{
	  m_gridX = m_gridY = 10;
	}
      else if (list.size() == 2 && list[1] == "no")
	{
	  m_gridX = m_gridY = 0;
	}
      else if (list.size() == 3)
	{
	  m_gridX = list[1].toInt();
	  m_gridY = list[2].toInt();
	}
      return true;
    }
  else if (list[0] == "image")
    {
      if (list.size() == 2 &&
	  list[1] == "no")
	clearImage();
      else
	loadImage();
      return true;
    }
  else if (list[0] == "imageframe")
    {
      if (list.size() == 2)
	{
	  int frm = list[1].toInt(&ok);
	  if (frm >= 0)
	    {
	      loadImage(m_imageName, frm);
	    }
	  else
	    QMessageBox::information(0, "Error",
				     "ImageFrame not changed.  Positive values required");
	}
      else
	QMessageBox::information(0, "Error",
				 "Please specify ImageFrame number for the clipplane");
      return true;
    }
  else if (list[0] == "caption")
    {
      if (list.count() == 2 &&
	  list[1] == "no")
	clearCaption();
      else
	loadCaption();

      return true;
    }
  else if (list[0] == "vscale" ||
	   list[0] == "scale")
    {
      if (list.size() > 1)
	{
	  float scl1, scl2;
	  if (list.size() == 2)
	    {
	      scl1 = list[1].toFloat(&ok);
	      scl2 = scl1;
	    }
	  else
	    {
	      scl1 = list[1].toFloat(&ok);
	      scl2 = list[2].toFloat(&ok);
	    }
	  if (list[0] == "scale")
	    {
	      m_scale1 = -qAbs(scl1);
	      m_scale2 = -qAbs(scl2);
	    }
	  else
	    {
	      m_scale1 = scl1;
	      m_scale2 = scl2;
	    }
	}
      else
	QMessageBox::information(0, "Error",
				 "Please specify both scalings for the clipplane");
      return true;
    }
  else if (list[0] == "opacity")
    {
      if (list.size() == 2)
	{
	  float scl = list[1].toFloat(&ok);
	  if (scl >= 0 && scl <= 1)
	    {
	      m_opacity = scl;
	      m_opacity = qMax(0.02f, qMin(1.0f, m_opacity));
	    }
	  else
	    QMessageBox::information(0, "Error",
				     "Opacity not changed.  Value between 0 and 1 required");
	}
      else
	QMessageBox::information(0, "Error",
				 "Please specify opacity for the clipplane");
      return true;
    }
  else if (list[0] == "translate" ||
	   list[0] == "translatex" ||
	   list[0] == "translatey" ||
	   list[0] == "translatez" ||
	   list[0] == "move" ||
	   list[0] == "movex" ||
	   list[0] == "movey" ||
	   list[0] == "movez")
    {
      Vec pos;
      float x=0,y=0,z=0;

      if (list[0] == "translate" || list[0] == "move")
	{
	  if (list.size() > 1) x = list[1].toFloat(&ok);
	  if (list.size() > 2) y = list[2].toFloat(&ok);
	  if (list.size() > 3) z = list[3].toFloat(&ok);
	  pos = Vec(x,y,z);
	}
      else
	{
	  float v=0;
	  if (list.size() > 1) v = list[1].toFloat(&ok);
	  if (list[0] == "translatex" || list[0] == "movex")
	    pos = Vec(v,0,0);
	  else if (list[0] == "translatey" || list[0] == "movey")
	    pos = Vec(0,v,0);
	  else if (list[0] == "translatez" || list[0] == "movez")
	    pos = Vec(0,0,v);
	}

      if (list[0].contains("move"))
	{
	  Vec cpos = position();
	  pos = pos + cpos;
	}
      setPosition(pos);
      return true;
    }
  else if (list[0] == "rotatea" ||
	   list[0] == "rotateb" ||
	   list[0] == "rotatec")
    {
      float angle = 0;
      if (list.size() > 1)
	{
	  angle = list[1].toFloat(&ok);
	  if (list[0] == "rotatea") rotate(m_xaxis, angle);
	  if (list[0] == "rotateb") rotate(m_yaxis, angle);
	  if (list[0] == "rotatec") rotate(m_tang, angle);
	}
      else
	{
	  QMessageBox::information(0, "", "No angle specified");
	}       
      return true;
    }
  else if (list[0] == "movea" ||
	   list[0] == "moveb" ||
	   list[0] == "movec")
    {
      float shift = 0;
      if (list.size() > 1)
	{
	  shift = list[1].toFloat(&ok);
	  if (list[0] == "movea") translate(shift*m_xaxis);
	  if (list[0] == "moveb") translate(shift*m_yaxis);
	  if (list[0] == "movec") translate(shift*m_tang);
	}
      else
	{
	  QMessageBox::information(0, "", "No distance specified");
	}       
      return true;
    }
  else if (list[0] == "rotate" ||
	   list[0] == "rotatex" ||
	   list[0] == "rotatey" ||
	   list[0] == "rotatez" ||
	   list[0] == "addrotation" ||
	   list[0] == "addrotationx" ||
	   list[0] == "addrotationy" ||
	   list[0] == "addrotationz")
    {
      Quaternion rot;
      float x=0,y=0,z=0,a=0;
      if (list[0] == "rotate" || list[0] == "addrotation")
	{
	  if (list.size() > 1) x = list[1].toFloat(&ok);
	  if (list.size() > 2) y = list[2].toFloat(&ok);
	  if (list.size() > 3) z = list[3].toFloat(&ok);
	  if (list.size() > 4) a = list[4].toFloat(&ok);
	  rot = Quaternion(Vec(x,y,z), DEG2RAD(a));
	}
      else
	{
	  float a=0;
	  if (list.size() > 1) a = list[1].toFloat(&ok);
	  if (list[0] == "rotatex" || list[0] == "addrotationx")
	    rot = Quaternion(Vec(1,0,0), DEG2RAD(a));
	  else if (list[0] == "rotatey" || list[0] == "addrotationy")
	    rot = Quaternion(Vec(0,1,0), DEG2RAD(a));
	  else if (list[0] == "rotatez" || list[0] == "addrotationz")
	    rot = Quaternion(Vec(0,0,1), DEG2RAD(a));
	}

      if (list[0].contains("addrotation"))
	{
	  Quaternion orot = orientation();
	  rot = rot*orot;
	}
      setOrientation(rot);
      return true;
    }
  else
    QMessageBox::information(0, "Error",
			     QString("Cannot understand the command : ") +
			     cmd);

  return false;
}

void
ClipObject::computeTscale()
{
  float s1, s2;
  s1 = s2 = 1.0;
  if (m_imagePresent || m_captionPresent)
    {
      if (m_textureHeight > m_textureWidth)
	s1 = (float)m_textureWidth/(float)m_textureHeight;
      else
	s2 = (float)m_textureHeight/(float)m_textureWidth;
    }

  if (m_scale1 > 0 && m_scale2 > 0)
    { // scale according to widget size
      s1*=m_size;
      s2*=m_size;
    }
  // otherwise do no scale according to widget size
  // negative scale value means take the absolute
  // grid size values

  s1*=qAbs(m_scale1);
  s2*=qAbs(m_scale2);

  m_tscale1 = s1;
  m_tscale2 = s2;
}

bool
ClipObject::commandEditor()
{
  PropertyEditor propertyEditor;
  QMap<QString, QVariantList> plist;
  QVariantList vlist;
  vlist.clear();
  plist["command"] = vlist;


  vlist.clear();
  vlist << QVariant("double");
  vlist << QVariant(m_opacity);
  vlist << QVariant(0.0);
  vlist << QVariant(1.0);
  vlist << QVariant(0.1); // singlestep
  vlist << QVariant(1); // decimals
  plist["opacity"] = vlist;
  
  vlist.clear();
  vlist << QVariant("color");
  Vec pcolor = m_color;
  QColor dcolor = QColor::fromRgbF(pcolor.x,
				   pcolor.y,
				   pcolor.z);
  vlist << dcolor;
  plist["color"] = vlist;
  
  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(m_apply);
  plist["apply clipping"] = vlist;

  vlist.clear();
  vlist << QVariant("int");
  vlist << QVariant(m_tfset);
  vlist << QVariant(-1);
  vlist << QVariant(15);
  plist["tfset"] = vlist;

  vlist.clear();
  vlist << QVariant("int");
  vlist << QVariant(m_thickness);
  vlist << QVariant(0);
  vlist << QVariant(200);
  plist["thickness"] = vlist;

  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(m_solidColor);
  plist["solid color"] = vlist;

  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(m_showSlice);
  plist["show slice"] = vlist;

  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(m_showThickness);
  plist["show thickness"] = vlist;

  vlist.clear();
  vlist << QVariant("combobox");
  if (m_viewportType)
    vlist << QVariant(1);
  else
    vlist << QVariant(0);
  vlist << QVariant("orthographic");
  vlist << QVariant("perspective");
  plist["camera type"] = vlist;

  vlist.clear();
  vlist << QVariant("double");
  vlist << QVariant(m_stereo);
  vlist << QVariant(0.0);
  vlist << QVariant(1.0);
  vlist << QVariant(0.1); // singlestep
  vlist << QVariant(1); // decimals
  plist["stereo"] = vlist;
  
  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(m_showOtherSlice);
  plist["show other slice"] = vlist;


  QString vpstr = QString("%1 %2 %3 %4").\
                  arg(m_viewport.x()).\
                  arg(m_viewport.y()).\
                  arg(m_viewport.z()).\
                  arg(m_viewport.w());
  vlist.clear();
  vlist << QVariant("string");
  vlist << QVariant(vpstr);
  plist["viewport"] = vlist;

  vlist.clear();
  vlist << QVariant("double");
  vlist << QVariant(m_viewportScale);
  vlist << QVariant(0.5);
  vlist << QVariant(30.0);
  vlist << QVariant(0.1); // singlestep
  vlist << QVariant(1); // decimals
  plist["viewport scale"] = vlist;  


  vlist.clear();
  QFile helpFile(":/clipobject.help");
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
  if (m_scale1 < 0 || m_scale2 < 0)
    mesg += QString("scales : %1 %2\n").arg(-m_scale1).arg(-m_scale2);
  else
    mesg += QString("vscales : %1 %2\n").arg(m_scale1).arg(m_scale2);

  mesg += QString("opacity : %1\n").arg(m_opacity);
  mesg += QString("position : %1 %2 %3\n").			\
    arg(m_position.x).arg(m_position.y).arg(m_position.z);

  Quaternion q = orientation();
  Vec axis;
  qreal angle;
  q.getAxisAngle(axis, angle);
  mesg += QString("rotation : %1 %2 %3 : %4\n").			\
    arg(axis.x).arg(axis.y).arg(axis.z).arg(RAD2DEG(angle));
  
  
  mesg += QString("Red axis : %1 %2 %3\n").		\
    arg(m_xaxis.x).arg(m_xaxis.y).arg(m_xaxis.z);
  mesg += QString("Green axis : %1 %2 %3\n").		\
    arg(m_yaxis.x).arg(m_yaxis.y).arg(m_yaxis.z);
  mesg += QString("Blue axis : %1 %2 %3\n").		\
    arg(m_tang.x).arg(m_tang.y).arg(m_tang.z);
  
  vlist << mesg;
  
  plist["message"] = vlist;
  //---------------------



  QStringList keys;
  keys << "apply clipping";
  keys << "solid color";
  keys << "show slice";
  keys << "show thickness";
  keys << "show other slice";
  keys << "gap";
  keys << "color";
  keys << "opacity";
  keys << "gap";
  keys << "viewport";
  keys << "tfset";
  keys << "thickness";
  keys << "viewport scale";
  keys << "camera type";
  keys << "stereo";
  keys << "gap";
  keys << "command";
  keys << "commandhelp";
  keys << "message";
  
  propertyEditor.set("Clip Plane Dialog", plist, keys);
  
  QMap<QString, QPair<QVariant, bool> > vmap;

  if (propertyEditor.exec() == QDialog::Accepted)
    vmap = propertyEditor.get();
  else
    return true;
	      
  keys = vmap.keys();

  for(int ik=0; ik<keys.count(); ik++)
    {
      QPair<QVariant, bool> pair = vmap.value(keys[ik]);
      
      
      if (pair.second)
	{
	  if (keys[ik] == "color")
	    {
	      QColor color = pair.first.value<QColor>();
	      float r = color.redF();
	      float g = color.greenF();
	      float b = color.blueF();
	      m_color = Vec(r,g,b);
	    }
	  else if (keys[ik] == "opacity")
	    m_opacity = pair.first.toDouble();
	  else if (keys[ik] == "solid color")
	    m_solidColor = pair.first.toBool();
	  else if (keys[ik] == "apply clipping")
	    m_apply = pair.first.toBool();
	  else if (keys[ik] == "show slice")
	    m_showSlice = pair.first.toBool();
	  else if (keys[ik] == "show thickness")
	    m_showThickness = pair.first.toBool();
	  else if (keys[ik] == "show other slice")
	    m_showOtherSlice = pair.first.toBool();
	  else if (keys[ik] == "tfset")
	    m_tfset = pair.first.toInt();
	  else if (keys[ik] == "thickness")
	    m_thickness = pair.first.toInt();
	  else if (keys[ik] == "viewport scale")
	    m_viewportScale = pair.first.toDouble();
	  else if (keys[ik] == "camera type")
	    m_viewportType = (pair.first.toInt() == 1);
	  else if (keys[ik] == "stereo")
	    m_stereo = pair.first.toDouble();
	  else if (keys[ik] == "viewport")
	    {
	      vpstr = pair.first.toString();
	      QStringList list = vpstr.split(" ", QString::SkipEmptyParts);
	      if (list.count() == 4)
		{
		  float x = list[0].toFloat();
		  float y = list[1].toFloat();
		  float z = list[2].toFloat();
		  float w = list[3].toFloat();
		  if (x < 0.0f || x > 1.0f ||
		      y < 0.0f || y > 1.0f ||
		      z < 0.0f || z > 1.0f ||
		      w < 0.0f || w > 1.0f)
		    QMessageBox::information(0, "",
		      QString("Values for viewport must be between 0.0 and 1.0 : %1 %2 %3 %4").\
					     arg(x).arg(y).arg(z).arg(w));
		  else
		    m_viewport = QVector4D(x,y,z,w);
		}
	      else if (list.count() == 3)
		{
		  float x = list[0].toFloat();
		  float y = list[1].toFloat();
		  float z = list[2].toFloat();
		  if (x < 0.0f || x > 1.0f ||
		      y < 0.0f || y > 1.0f ||
		      z < 0.0f || z > 1.0f)
		    QMessageBox::information(0, "",
		      QString("Values for viewport must be between 0.0 and 1.0 : %1 %2 %3").\
					     arg(x).arg(y).arg(z));
		  else
		    m_viewport = QVector4D(x,y,z,z);
		}
	      else if (list.count() == 2)
		{
		  float x = list[0].toFloat();
		  float y = list[1].toFloat();
		  if (x < 0.0f || x > 1.0f ||
		      y < 0.0f || y > 1.0f)
		    QMessageBox::information(0, "",
		      QString("Values for viewport must be between 0.0 and 1.0 : %1 %2").\
					     arg(x).arg(y));
		  else
		    m_viewport = QVector4D(x,y,0.5,0.5);
		}
	      else
		{
		  QMessageBox::information(0, "", "Switching off the viewport");
		  m_viewport = QVector4D(-1,-1,-1,-1);
		}
	    }
	}
    }

  QString cmd = propertyEditor.getCommandString();
  if (!cmd.isEmpty())
    return processCommand(cmd);
  else
    return true;

//  if (propertyEditor.exec() == QDialog::Accepted)
//    {
//      QString cmd = propertyEditor.getCommandString();
//      if (!cmd.isEmpty())
//	return processCommand(cmd);
//    }
//  else
//    return true;
}
