#include "viewer.h"
#include "global.h"
#include "geometryobjects.h"
#include "staticfunctions.h"
#include "dialogs.h"
#include "captiondialog.h"
#include "enums.h"
#include "shaderfactory.h"
#include "propertyeditor.h"
#include "dcolordialog.h"
#include "mainwindowui.h"

#include "cube2sphere.h"

#include <stdio.h>
#include <math.h>
#include <fstream>
#include <time.h>

#include <QInputDialog>
#include <QFileDialog>

using namespace std;


//------------------------------------------------------------------
ViewerUndo::ViewerUndo() { clear(); }
ViewerUndo::~ViewerUndo() { clear(); }

void
ViewerUndo::clear()
{
  m_pos.clear();
  m_rot.clear();
  m_index = -1;
}

void
ViewerUndo::clearTop()
{
  if (m_index == m_pos.count()-1)
    return;

  while(m_index < m_pos.count()-1)
    m_pos.removeLast();
  
  while(m_index < m_rot.count()-1)
    m_rot.removeLast();  
}

void
ViewerUndo::append(Vec p, Quaternion r)
{
  clearTop();
  m_pos << p;
  m_rot << r;
  m_index = m_pos.count()-1;
}

void ViewerUndo::redo() { m_index = qMin(m_index+1, m_pos.count()-1); }
void ViewerUndo::undo() { m_index = qMax(m_index-1, 0); }

Vec
ViewerUndo::pos()
{
  Vec p = Vec(0,0,0);

  if (m_index >= 0 && m_index < m_pos.count())
    p = m_pos[m_index];

  return p;
}

Quaternion
ViewerUndo::rot()
{
  Quaternion r = Quaternion(Vec(1,0,0), 0);

  if (m_index >= 0 && m_index < m_pos.count())
    r = m_rot[m_index];

  return r;
}
//------------------------------------------------------------------


void
Viewer::closeEvent(QCloseEvent *event)
{
  showMinimized();
  event->ignore();
}

void Viewer::setHiresVolume(DrawHiresVolume *vol) { m_hiresVolume = vol; }
void Viewer::setKeyFrame(KeyFrame *keyframe) { m_keyFrame = keyframe; }
void Viewer::setImageMode(int im) { m_imageMode = im; }
void Viewer::setCurrentFrame(int fno)
{
  m_currFrame = fno;
  Global::setFrameNumber(m_currFrame);
}

bool Viewer::drawToFBO() { return (m_useFBO && savingImages()); }
void Viewer::setUseFBO(bool flag) { m_useFBO = flag; }
void Viewer::setDOF(int b, float nf)
{
  m_hiresVolume->setDOF(b, nf);
  emit updateGL();
}
void Viewer::setFieldOfView(float fov)
{
  camera()->setFieldOfView(fov);
}
void Viewer::endPlay()
{
  if (m_saveSnapshots || m_saveMovie)
    restoreOriginalWidgetSize();

  if (m_saveMovie)
    endMovie();
 
  m_saveSnapshots = false;
  m_saveMovie = false;

  Global::setSaveImageType(Global::NoImage);
}
void Viewer::setImageFileName(QString imgfl)
{
  m_imageFileName = imgfl;
  QFileInfo f(m_imageFileName);
  setSnapshotFormat(f.completeSuffix());
}
void Viewer::setSaveSnapshots(bool flag)
{
  if (flag)
    m_saveSnapshots = flag;
}
void Viewer::setSaveMovie(bool flag)
{
  if (flag)
    m_saveMovie = flag;
}

void
Viewer::switchToHires()
{
  m_undo.clear();

  if (Global::visualizationMode() == Global::Volumes)
    {
      Vec smin, smax;
      GeometryObjects::trisets()->allEnclosingBox(smin, smax);
      m_hiresVolume->overwriteDataMinMax(smin, smax);
      m_hiresVolume->setBrickBounds(smin, smax);
   }
}

void
Viewer::switchDrawVolume()
{
  qApp->setOverrideCursor(QCursor(Qt::WaitCursor));

  createImageBuffers();

  Global::enableViewerUpdate();
  MainWindowUI::changeDrishtiIcon(true);
  showFullScene();
  
  emit updateGL();
  qApp->processEvents();

  qApp->restoreOverrideCursor();
}

void
Viewer::showFullScene()
{
  Vec smin, smax;
  
  if (Global::visualizationMode() == Global::Surfaces)
    {
      GeometryObjects::trisets()->allEnclosingBox(smin, smax);
      m_hiresVolume->overwriteDataMinMax(smin, smax);
      m_hiresVolume->setBrickBounds(smin, smax);
    }

  if (Global::visualizationMode() == Global::Volumes)
    {
      GeometryObjects::trisets()->allEnclosingBox(smin, smax);
      m_hiresVolume->overwriteDataMinMax(smin, smax);
      m_hiresVolume->setBrickBounds(smin, smax);
      smin = VECPRODUCT(m_hiresVolume->volumeMin(),
			Global::voxelScaling());
      smax = VECPRODUCT(m_hiresVolume->volumeMax(),
			Global::voxelScaling());
    }
  
  setSceneBoundingBox(smin, smax);

  showEntireScene();
}


void
Viewer::updateScaling()
{
  Vec smin, smax;

  if (Global::visualizationMode() == Global::Surfaces)
    {
      GeometryObjects::trisets()->allEnclosingBox(smin, smax);
      m_hiresVolume->overwriteDataMinMax(smin, smax);
    }
  
  if (Global::visualizationMode() == Global::Volumes)
    {
      smin = VECPRODUCT(m_hiresVolume->volumeMin(),
			Global::voxelScaling());
      smax = VECPRODUCT(m_hiresVolume->volumeMax(),
			Global::voxelScaling());
    }
  
  setSceneBoundingBox(smin, smax);
  updateGL();
}

void
Viewer::resizeGL(int width, int height)
{
  if (savingImages())
    return;

  int wd = width;
  int ht = height;

  if (!m_imageSizeFlag)
    {
      m_origWidth = wd;
      m_origHeight = ht;
    }

  if (m_messageDisplayer->showingMessage())
    m_messageDisplayer->turnOffMessage();

  QGLViewer::resizeGL(wd, ht);

  createImageBuffers();

  if (GeometryObjects::trisets()->count() > 0)
    GeometryObjects::trisets()->resize(wd, ht);
}

void
Viewer::createImageBuffers()
{
  int ibw = m_origWidth;
  int ibh = m_origHeight;


  QGLFramebufferObjectFormat fbFormat;
  fbFormat.setInternalTextureFormat(GL_RGBA16F_ARB);
  fbFormat.setAttachment(QGLFramebufferObject::Depth);
  //fbFormat.setSamples(2);
  fbFormat.setTextureTarget(GL_TEXTURE_RECTANGLE_EXT);

  if (m_imageBuffer) delete m_imageBuffer;
  m_imageBuffer = new QGLFramebufferObject(ibw, ibh, fbFormat);

  if (! m_imageBuffer->isValid())
    QMessageBox::information(0, "", "invalid imageBuffer");


  if (m_movieFrame)
    delete [] m_movieFrame;
  m_movieFrame = new unsigned char[4*m_imageWidth*m_imageHeight];
}

void
Viewer::imageSize(int &wd, int &ht)
{
  wd = m_imageWidth;
  ht = m_imageHeight;
}

void
Viewer::setImageSize(int wd, int ht)
{
  m_imageWidth = wd;
  m_imageHeight = ht;

  if (Global::imageQuality() == Global::_LowQuality)
    {
      m_imageWidth = wd/2;
      m_imageHeight = ht/2;
    }
  else if (Global::imageQuality() == Global::_VeryLowQuality)
    {
      m_imageWidth = wd/4;
      m_imageHeight = ht/4;
    }

  if (m_imageBuffer) delete m_imageBuffer;
  QGLFramebufferObjectFormat fbFormat;
  fbFormat.setInternalTextureFormat(GL_RGBA16F_ARB);
  fbFormat.setAttachment(QGLFramebufferObject::Depth);
  //fbFormat.setSamples(8);
  fbFormat.setTextureTarget(GL_TEXTURE_RECTANGLE_EXT);
  m_imageBuffer = new QGLFramebufferObject(m_imageWidth,
					   m_imageHeight,
					   fbFormat);

  if (! m_imageBuffer->isValid())
    QMessageBox::information(0, "", "invalid imageBuffer");

  float ratio = qMax(1.0f, qMax((float)m_imageWidth/(float)m_origWidth,
			       (float)m_imageHeight/(float)m_origHeight));
  m_hiresVolume->setImageSizeRatio(ratio);


  if (GeometryObjects::trisets()->count() > 0)
    GeometryObjects::trisets()->resize(wd, ht);
}

Viewer::Viewer(QWidget *parent) :
  QGLViewer(parent)
{
  m_parent = parent;

  m_undo.clear();

  setMouseTracking(true);

  m_currFrame = 1;
  Global::setFrameNumber(m_currFrame);

  //m_copyShader = 0;
  //m_blurShader = 0;

  m_saveSnapshots = false;
  m_saveMovie = false;
  m_useFBO = true;

  m_hiresVolume = 0;

  m_pointMode = false;
  m_lengthMode = false;
  m_gotPoint0 = false;
  
  //m_autoUpdateTimer = new QTimer(this);
  connect(&m_autoUpdateTimer, SIGNAL(timeout()),
	  this, SLOT(updateGL()));

  camera()->setIODistance(0.06f);
  camera()->setPhysicalScreenWidth(4.3f);

  connect(this, SIGNAL(pointSelected(const QMouseEvent*)),
	  this, SLOT(checkPointSelected(const QMouseEvent*)));

  m_paintTex = 0;

  setMinimumSize(200, 200); 

  
#ifdef USE_GLMEDIA
  m_movieWriterLeft = 0;
  m_movieWriterRight = 0;
#endif // USE_GLMEDIA
  m_movieFrame = 0;

  m_messageDisplayer = new MessageDisplayer(this);

  m_imageSizeFlag = false;
  m_imageBuffer = 0;
  m_imageWidth = 128;
  m_imageHeight = 128;

  m_origWidth = 100;
  m_origHeight = 100;

  m_backBufferImage = 0;
  m_backBufferWidth = 0;
  m_backBufferHeight = 0;

  connect(this, SIGNAL(showMessage(QString, bool)),
	  m_messageDisplayer, SLOT(holdMessage(QString, bool)));
  connect(m_messageDisplayer, SIGNAL(updateGL()),
	  this, SLOT(updateGL()));


  m_undo.append(camera()->position(), camera()->orientation());

  setSnapshotQuality(100); // save uncompressed files

  initSocket();

  m_paintMode = false;
  m_paintRad = 5;
  m_paintColor = Vec(1,0.7,0.3);
  m_paintStyle = 0;
  m_paintAlpha = 0.05;
  m_paintOctave = 5;
  m_paintRoughness = 0;

  m_glVA = 0;
  m_glAB = 0;
  m_glEAB = 0;


  m_selectionWindow = QRect(0,0,0,0);
  
  camera()->frame()->setSpinningSensitivity(1.0);
}

//-----------------------------------
//-----------------------------------
void
Viewer::setPaintMode(bool b)
{  
  m_paintMode = b;

//  if (m_paintMode)
//    QMessageBox::information(0, "", "Entering Paint Mode");
//  else
//    QMessageBox::information(0, "", "Paint Mode Ended");
  
  if (m_paintMode)
    { 
      Vec bmin, bmax;
      GeometryObjects::trisets()->makeReadyForPainting();
      GeometryObjects::trisets()->allEnclosingBox(bmin, bmax);
      Vec extend = bmax-bmin;
      int me = 0;
      if (extend[1] > extend[me]) me = 1;
      if (extend[2] > extend[me]) me = 2;
      m_unitPaintRad = extend[me];
      m_unitPaintRad *= 0.01;
    }
}

void
Viewer::setPaintColor(Vec c)
{
  m_paintColor = c;
}
//-----------------------------------
//-----------------------------------



void
Viewer::initSocket()
{
  m_listeningSocket = new QUdpSocket(this);
  m_socketPort = 7760;
  if (m_listeningSocket->bind(QHostAddress::LocalHost, m_socketPort))
    {
      connect(m_listeningSocket, SIGNAL(readyRead()),
	      this, SLOT(readSocket()),
	      Qt::DirectConnection);
    }
}

void
Viewer::readSocket()
{
  while (m_listeningSocket->hasPendingDatagrams())
    {
      QByteArray datagram;
      datagram.resize(m_listeningSocket->pendingDatagramSize());
      QHostAddress sender;
      quint16 senderPort;

      m_listeningSocket->readDatagram(datagram.data(), datagram.size(),
				      &sender, &senderPort);

      processSocketData(QString(datagram));
    }
}

void
Viewer::processSocketData(QString data)
{
  processCommand(data);
  updateGL();
}


void
Viewer::checkPointSelected(const QMouseEvent *event)
{
//  if (GeometryObjects::trisets()->grabMode())
//    return;
  
  bool found;
  QPoint scr = event->pos();

  Vec target = m_hiresVolume->pointUnderPixel(scr, found);

  if (found && m_lengthMode)
    {
      if (!m_gotPoint0)
	{
	  m_gotPoint0 = true;
	  m_point0 = target;
	  QList<Vec> pts;
	  pts << m_point0;
	  pts << m_point0;
	  GeometryObjects::paths()->addPath(pts); // length only
	}
      else
	{
	  m_point1 = target;
	  QList<PathGrabber*> path = GeometryObjects::paths()->pathsPointer();	  
	  int last = path.count()-1;
	  path[last]->setPoint(1, m_point1);
	}
      return;
    }

  
  if (found)
    {
      if (m_paintMode)
	{  
	  if (event->buttons() == Qt::LeftButton) // change rotation pivot
	    GeometryObjects::trisets()->paint(target, m_paintRad*m_unitPaintRad,
					      m_paintColor, m_paintStyle, m_paintAlpha,
					      m_paintOctave, m_paintRoughness);
	  else
	    GeometryObjects::trisets()->paint(target, m_paintRad*m_unitPaintRad,
					      m_paintColor, m_paintStyle+10, m_paintAlpha,
					      m_paintOctave, m_paintRoughness);
	    
	  return;
	}


      if (event->buttons() == Qt::RightButton) // change rotation pivot
	{
	  camera()->setRevolveAroundPoint(target);
	  QMessageBox::information(0, "", tr("Rotation pivot changed.\n\nTo reset back to scene center just Shift+Right click in empty region on screen - that is do not Shift+Right click on volume or any widget.\n\nRotation pivot change has no effect on keyframe animation."));
	}
      else
	{
//	  if (GeometryObjects::paths()->continuousAdd())
//	    GeometryObjects::paths()->addPoint(target);
//	  else
//	    GeometryObjects::hitpoints()->add(target);
	  //if (m_pointMode)
	    {
	      if (!GeometryObjects::trisets()->addHitPoint(target)) // if a mesh is active, attach the point to that mesh
		GeometryObjects::hitpoints()->add(target); // otherwise add it to the pool of hitpoints
	    }
	}
    }
  else
    {
      if (!m_paintMode && event->buttons() == Qt::RightButton) // reset rotation pivot
	{
	  camera()->setRevolveAroundPoint(sceneCenter());
	  QMessageBox::information(0, "", tr("Rotation pivot reset to scene center"));
	}
    }
}

Viewer::~Viewer()
{
  delete m_messageDisplayer;

  if (m_paintTex)
    glDeleteTextures(1, &m_paintTex);

  if (m_backBufferImage)
    delete [] m_backBufferImage;

}

void
Viewer::GlewInit()
{
  GlewInit::initialise();

  Global::setUseFBO(QGLFramebufferObject::hasOpenGLFramebufferObjects());
						
  //createBlurShader();
  //createCopyShader();

  if (GlewInit::initialised())
    MainWindowUI::mainWindowUI()->statusBar->showMessage(tr("Ready"));
  else
    MainWindowUI::mainWindowUI()->statusBar->showMessage(tr("Error : Cannot Initialize Renderer"));

  update();

  if (format().stereo())
    setStereoDisplay(true);

  
  // create shaders
  ShaderFactory::ptShader();
  ShaderFactory::pnShader();
}

//void
//Viewer::createBlurShader()
//{
//  QString shaderString;
//  shaderString = ShaderFactory::genRectBlurShaderString(Global::viewerFilter());      
//  m_blurShader = glCreateProgramObjectARB();
//  if (! ShaderFactory::loadShader(m_blurShader,
//				  shaderString))
//    exit(0);
//  m_blurParm[0] = glGetUniformLocationARB(m_blurShader, "blurTex");
//}
//
//void
//Viewer::createCopyShader()
//{
//  QString shaderString;
//  shaderString = ShaderFactory::genCopyShaderString();
//  m_copyShader = glCreateProgramObjectARB();
//  if (! ShaderFactory::loadShader(m_copyShader,
//				  shaderString))
//    exit(0);
//  m_copyParm[0] = glGetUniformLocationARB(m_copyShader, "shadowTex");
//}


void
Viewer::displayMessage(QString mesg, bool warn)
{  
  m_messageDisplayer->holdMessage(mesg, warn);
}

 
void
Viewer::splashScreen()
{
  startScreenCoordinatesSystem();
    
  float fscl = 120.0/Global::dpi();  
  QFont tfont = QFont("Helvetica");
  tfont.setStyleStrategy(QFont::ForceOutline);
  
  QImage img(":/images/splashscreen.png");
  int px = qMax(1, (width()-img.width())/2);
  int py = qMin(height(), (height()-img.height())/2);
  QImage mimg = img.mirrored();
  glRasterPos2i(px, height()-py);
  glDrawPixels(mimg.width(), mimg.height(),
	       GL_BGRA,
	       GL_UNSIGNED_BYTE,
	       mimg.bits());

  QStringList version = Global::DrishtiVersion().split(".");
  QString majorVer = version[0];
  if (version.count() > 1)
    majorVer = majorVer+"."+version[1];

  QString minorVer;
  if (version.count() > 2)
    {
      version.removeFirst();
      version.removeFirst();
      minorVer = "("+version.join(".")+")";
    }
    
  {
    tfont.setPointSize(12*fscl);  
    if (!minorVer.isEmpty())
    StaticFunctions::renderText(px+520, height()-mimg.height()-py+10,
				minorVer,
				tfont,
				Qt::transparent, Qt::white,
				false); // textPath
  }

  {
    tfont.setPointSize(70*fscl);  
    StaticFunctions::renderText(px-200, height()-mimg.height()-py+2,
				QString("DrishtiMesh v"+majorVer),
				tfont,
				Qt::black, Qt::lightGray,
				false); // textPath
  }
  
  glColor4f(0,0,0,1);
  glBegin(GL_QUADS);
  glVertex2i(0,0);
  glVertex2i(width(),0);
  glVertex2i(width(),height());
  glVertex2i(0,height());
  glEnd();

  stopScreenCoordinatesSystem();
}

bool
Viewer::bindFBOs(int imagequality)
{
  bool fboBound = false;

  if (!format().stereo())
    {
      bool forceInitShadowBuffers = false;
      if (savingImages())
	{
	  if (m_imageBuffer->width() != m_imageWidth ||
	      m_imageBuffer->height() != m_imageHeight)
	    {
	      delete m_imageBuffer;
	      QGLFramebufferObjectFormat fbFormat;
	      fbFormat.setInternalTextureFormat(GL_RGBA16F_ARB);
	      fbFormat.setAttachment(QGLFramebufferObject::Depth);
	      //fbFormat.setSamples(8);
	      fbFormat.setTextureTarget(GL_TEXTURE_RECTANGLE_EXT);

	      m_imageBuffer = new QGLFramebufferObject(m_imageWidth,
						       m_imageHeight,
						       fbFormat);
	      forceInitShadowBuffers = true;
	    }
	}
      if (m_imageBuffer->bind())
	{
	  fboBound = true;
	  glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
	  
	  glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	  glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	  
	  camera()->setScreenWidthAndHeight(m_imageBuffer->width(),
					    m_imageBuffer->height());
	  camera()->loadProjectionMatrix(true);
	  camera()->loadModelViewMatrix(true);
 	  glViewport(0,0,
		     m_imageBuffer->width(),
		     m_imageBuffer->height());
	}
    }

  if (!fboBound)
    {
      if (Global::imageQuality() != Global::_NormalQuality)
	{
	  Global::setImageQuality(Global::_NormalQuality);
	}
      return false;
    }

  return true;
}

void
Viewer::releaseFBOs(int imagequality)
{
  if(!m_imageBuffer->isBound())
    return;

  bool imgBound = m_imageBuffer->isBound();

  if (imgBound)
    m_imageBuffer->release();

  glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
  makeCurrent();

  int ow = QGLViewer::size().width();
  int oh = QGLViewer::size().height();

  camera()->setScreenWidthAndHeight(ow, oh);
  camera()->loadProjectionMatrix(true);
  camera()->loadModelViewMatrix(true);
  glViewport(0,0, ow, oh); 
  
  glEnable(GL_TEXTURE_RECTANGLE_ARB);

  int wd, ht;
  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_imageBuffer->texture());
  wd = m_imageBuffer->width();
  ht = m_imageBuffer->height();
  
  glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

//  if (wd < ow || ht  < oh)
//    {
//      glUseProgramObjectARB(m_blurShader);
//      glUniform1iARB(m_blurParm[0], 0); // blur image from lowresBuffer into frameBuffer
//    }
//  else
    {
      //glUseProgramObjectARB(m_copyShader);
      //glUniform1iARB(m_copyParm[0], 0); // copy image from imageBuffer into frameBuffer
      
      glUseProgramObjectARB(ShaderFactory::copyShader());
      glUniform1iARB(ShaderFactory::copyShaderParm()[0], 0); // copy image from imageBuffer into frameBuffer
    }

  startScreenCoordinatesSystem();

  glColor4f(1, 1, 1, 1);
  glBegin(GL_QUADS);

  glTexCoord2f(0, ht);
  glVertex2f(0, 0);

  glTexCoord2f(0, 0);
  glVertex2f(0, oh);

  glTexCoord2f(wd, 0);
  glVertex2f(ow, oh);

  glTexCoord2f(wd, ht);
  glVertex2f(ow, 0);

  glEnd();

  glDisable(GL_TEXTURE_RECTANGLE_ARB);

  glUseProgramObjectARB(0);

  stopScreenCoordinatesSystem();
}


void
Viewer::drawInHires(int imagequality)
{
  m_hiresVolume->drawStillImage();


  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // blend on top

  GeometryObjects::hitpoints()->postdraw(this);
  GeometryObjects::paths()->postdraw(this);
  GeometryObjects::trisets()->postdraw(this);
  GeometryObjects::captions()->draw(this);

  GeometryObjects::scalebars()->draw(this,
				     GeometryObjects::clipplanes()->clipInfo());

  drawSelectionWindow();
}

void
Viewer::drawSelectionWindow()
{
  QSize sz = m_selectionWindow.size();
  if (sz.width() == 0 && sz.height() == 0)
    return;

  QPoint pos = m_selectionWindow.topLeft();
  int xe = pos.x() + m_selectionWindow.width();
  int ye = pos.y() + m_selectionWindow.height();
  
  startScreenCoordinatesSystem();
  
  glColor4f(1, 0.5, 0.3, 1);
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  glBegin(GL_QUADS);
  glVertex2f(pos.x(), pos.y());
  glVertex2f(pos.x(), ye);
  glVertex2f(xe, ye);
  glVertex2f(xe, pos.y());
  glEnd();
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  
  stopScreenCoordinatesSystem();
}


void
Viewer::renderVolume(int imagequality)
{
  if (m_saveSnapshots || m_saveMovie)
    bindFBOs(imagequality);
    

  glClearDepth(1);
  glClear(GL_DEPTH_BUFFER_BIT);
  glDisable(GL_LIGHTING);
  glDepthFunc(GL_LESS);
  glEnable(GL_DEPTH_TEST);
  glShadeModel(GL_SMOOTH);
  glEnable(GL_MULTISAMPLE);

  glClear(GL_COLOR_BUFFER_BIT);
  
  drawInHires(imagequality);
  
  if (m_saveSnapshots || m_saveMovie)
    releaseFBOs(imagequality);
}

bool
Viewer::startMovie(QString flnm,
		   int ofps, int quality,
		   bool checkfps)
{
#ifdef USE_GLMEDIA
  int fps = ofps;

  if (checkfps)
    {
      bool ok;
      QString text;
      text = QInputDialog::getText(this,
				   tr("Set Frame Rate"),
				   tr("Frame Rate"),
				   QLineEdit::Normal,
				   "25",
				   &ok);
      
      if (ok && !text.isEmpty())
	fps = text.toInt();
      
      if (fps <=0 || fps >= 100)
	fps = 25;
    }

  //---------------------------------------------------------
  // mono movie or left-eye movie
  QString movieFile;
  if (m_imageMode == Enums::MonoImageMode)
    movieFile = flnm;
  else
    {
      QString localImageFileName = flnm;
      QFileInfo f(localImageFileName);	  
      movieFile = f.absolutePath() + QDir::separator() +
	f.baseName() + QString("_left.") +
	f.completeSuffix();
    }

  m_movieWriterLeft = glmedia_movie_writer_create();
  if (m_movieWriterLeft == NULL) {
    QMessageBox::critical(0, tr("Movie"),
			  "Failed to create writer");
    return false;
  }

  if (glmedia_movie_writer_start(m_movieWriterLeft,
				 movieFile.toLatin1().data(),
				 m_imageWidth,
				 m_imageHeight,
				 fps,
				 quality) < 0) {
    QMessageBox::critical(0, tr("Movie"),
			  "Failed to start movie");
    return false;
  }
  //---------------------------------------------------------

  //---------------------------------------------------------
  // right-eye movie
  if (m_imageMode != Enums::MonoImageMode)
    {
      QString localImageFileName = flnm;
      QFileInfo f(localImageFileName);	  
      movieFile = f.absolutePath() + QDir::separator() +
	          f.baseName() + QString("_right.") +
	          f.completeSuffix();

      m_movieWriterRight = glmedia_movie_writer_create();
      if (m_movieWriterRight == NULL) {
	QMessageBox::critical(0, tr("Movie"),
			      "Failed to create writer");
	return false;
      }
      
      if (glmedia_movie_writer_start(m_movieWriterRight,
				     movieFile.toLatin1().data(),
				     m_imageWidth,
				     m_imageHeight,
				     fps,
				     quality) < 0) {
	QMessageBox::critical(0, tr("Movie"),
			      "Failed to start movie");
	return false;
      }
    }
  //---------------------------------------------------------



  if (m_movieFrame)
    delete [] m_movieFrame;
  m_movieFrame = new unsigned char[4*m_imageWidth*m_imageHeight];

  // change the widget size
  setWidgetSizeToImageSize();

#endif // USE_GLMEDIA
  return true;
}

bool
Viewer::endMovie()
{
#ifdef USE_GLMEDIA
  if (glmedia_movie_writer_end(m_movieWriterLeft) < 0)
    {
      QMessageBox::critical(0, tr("Movie"),
			       "Failed to end movie");
      return false;
    }
  glmedia_movie_writer_free(m_movieWriterLeft);

  if (m_imageMode != Enums::MonoImageMode)
    {
      if (glmedia_movie_writer_end(m_movieWriterRight) < 0)
	{
	  QMessageBox::critical(0, tr("Movie"),
				   "Failed to end movie");
	  return false;
	}
      glmedia_movie_writer_free(m_movieWriterRight);
    }
#endif // USE_GLMEDIA
  return true;
}

void
Viewer::drawImageOnScreen()
{
  setBackgroundColor(QColor(0, 0, 0, 0));

  renderVolume(Enums::StillImage); 
}

void
Viewer::screenToMovieFrame()
{
  glReadBuffer(GL_BACK);
  glReadPixels(0,
	       0,
	       size().width(),
	       size().height(),
	       GL_RGBA,
	       GL_UNSIGNED_BYTE,
	       m_movieFrame);
}

void
Viewer::fboToMovieFrame()
{
  QImage bimg = m_imageBuffer->toImage();
  bimg = bimg.mirrored();
  bimg = bimg.rgbSwapped();

  memcpy(m_movieFrame, bimg.bits(),
	 4*bimg.width()*bimg.height());     
}

void
Viewer::saveMovie()
{
#ifdef USE_GLMEDIA
  if (m_imageMode == Enums::MonoImageMode)
    {
      Global::setSaveImageType(Global::MonoImage);
      drawImageOnScreen();
      glFinish();

      if (m_useFBO)
	fboToMovieFrame();
      else
	screenToMovieFrame();

      glmedia_movie_writer_add(m_movieWriterLeft, m_movieFrame);
    }
  else if (m_imageMode == Enums::StereoImageMode)
    {
      // --- left image
      Global::setSaveImageType(Global::LeftImage);
      drawImageOnScreen();	  
      glFinish();

      if (m_useFBO)
	fboToMovieFrame();
      else
	screenToMovieFrame();

      glmedia_movie_writer_add(m_movieWriterLeft, m_movieFrame);

      // --- right image
      Global::setSaveImageType(Global::RightImage);
      drawImageOnScreen();
      glFinish();

      if (m_useFBO)
	fboToMovieFrame();
      else
	screenToMovieFrame();

      glmedia_movie_writer_add(m_movieWriterRight, m_movieFrame);
    }

  //      having problem with the last movie frame ??
  //if (glmedia_movie_writer_add(m_movieWriter, m_movieFrame) < 0)
  //  QMessageBox::critical(0, "Movie",
  //  			 "Failed to add frame to movie");
#endif // USE_GLMEDIA
}

uint PREMUL(uint x)
{
  uint a = x >> 24;
  quint64 t = (((quint64(x)) | ((quint64(x)) << 24)) & 0x00ff00ff00ff00ff) * a;
  t = (t + ((t >> 8) & 0xff00ff00ff00ff) + 0x80008000800080) >> 8;
  t &= 0x000000ff00ff00ff;
  return (uint(t)) | (uint(t >> 24)) | (a << 24);
}

QImage
Viewer::getSnapshot()
{
  int wd, ht;
  uchar *imgbuf = 0;

  if (drawToFBO())
    {
      wd = m_imageBuffer->width();
      ht = m_imageBuffer->height();
      imgbuf = new uchar[wd*ht*4];

      if (m_imageBuffer->bind())
	{
	  glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
	  glReadPixels(0, 0, wd, ht, GL_RGBA, GL_UNSIGNED_BYTE, imgbuf);
	  m_imageBuffer->release();
	}
    }
  else
    {
      wd = width();
      ht = height();
      imgbuf = new uchar[wd*ht*4];
      glReadPixels(0, 0, wd, ht, GL_RGBA, GL_UNSIGNED_BYTE, imgbuf);
    }

  for(int i=0; i<wd*ht; i++)
    {
      uchar r = imgbuf[4*i+0];
      uchar g = imgbuf[4*i+1];
      uchar b = imgbuf[4*i+2];
      uchar a = imgbuf[4*i+3];
      
      uchar ma = qMax(qMax(r,g),qMax(b,a));
      imgbuf[4*i+3] = ma;
    }

  QImage img(wd, ht, QImage::Format_ARGB32);
  uchar *bits = img.bits();
  memcpy(bits, imgbuf, wd*ht*4);
  delete [] imgbuf;

  StaticFunctions::convertFromGLImage(img, wd, ht);

  return img;
}

void
Viewer::saveSnapshot(QString imgFile)
{
  int wd, ht;
  uchar *imgbuf = 0;

  if (drawToFBO())
    {
      wd = m_imageBuffer->width();
      ht = m_imageBuffer->height();
      imgbuf = new uchar[wd*ht*4];

      if (m_imageBuffer->bind())
	{
	  glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
	  glReadPixels(0, 0, wd, ht, GL_RGBA, GL_UNSIGNED_BYTE, imgbuf);
	  m_imageBuffer->release();
	}
    }
  else
    {
      wd = width();
      ht = height();
      imgbuf = new uchar[wd*ht*4];
      glReadPixels(0, 0, wd, ht, GL_RGBA, GL_UNSIGNED_BYTE, imgbuf);
    }

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

void
Viewer::saveCubicImage(QString localImageFileName,
		       QChar fillChar, int fieldWidth)
{
  QFileInfo f(localImageFileName);	  

  float fov = camera()->fieldOfView();
  Quaternion origOrientation = camera()->orientation();

  setFieldOfView((float)(M_PI/2.0));

  QString imgFile;
  
  Global::setSaveImageType(Global::CubicFrontImage);
  drawImageOnScreen();	  

  //---------------------------------------------------------
  imgFile = f.absolutePath() + QDir::separator() +
            QString("f_") + f.baseName();
  if (m_currFrame >= 0)
    imgFile += QString("%1").arg((int)m_currFrame, fieldWidth, 10, fillChar);
  imgFile += ".";
  imgFile += f.completeSuffix();
  //---------------------------------------------------------

  saveSnapshot(imgFile);
      

  Quaternion q, cq;
  
  Vec upVector = camera()->upVector();
  Vec rightVector = camera()->rightVector();
  
  q = Quaternion(upVector, -M_PI/2);
  cq = q*origOrientation;
  camera()->setOrientation(cq);
  Global::setSaveImageType(Global::CubicRightImage);
  drawImageOnScreen();
  //---------------------------------------------------------
  imgFile = f.absolutePath() + QDir::separator() +
            QString("r_") + f.baseName();
  if (m_currFrame >= 0)
    imgFile += QString("%1").arg((int)m_currFrame, fieldWidth, 10, fillChar);
  imgFile += ".";
  imgFile += f.completeSuffix();
  //---------------------------------------------------------
  saveSnapshot(imgFile);

  q = Quaternion(upVector, M_PI);
  cq = q*origOrientation;
  camera()->setOrientation(cq);
  Global::setSaveImageType(Global::CubicBackImage);
  drawImageOnScreen();	  
  //---------------------------------------------------------
  imgFile = f.absolutePath() + QDir::separator() +
            QString("b_") + f.baseName();
  if (m_currFrame >= 0)
    imgFile += QString("%1").arg((int)m_currFrame, fieldWidth, 10, fillChar);
  imgFile += ".";
  imgFile += f.completeSuffix();
  //---------------------------------------------------------
  saveSnapshot(imgFile);
      
  q = Quaternion(upVector, M_PI/2);
  cq = q*origOrientation;
  camera()->setOrientation(cq);
  Global::setSaveImageType(Global::CubicLeftImage);
  drawImageOnScreen();
  //---------------------------------------------------------
  imgFile = f.absolutePath() + QDir::separator() +
            QString("l_") + f.baseName();
  if (m_currFrame >= 0)
    imgFile += QString("%1").arg((int)m_currFrame, fieldWidth, 10, fillChar);
  imgFile += ".";
  imgFile += f.completeSuffix();
  //---------------------------------------------------------
  saveSnapshot(imgFile);


  q = Quaternion(rightVector, M_PI/2);
  cq = q*origOrientation;
  camera()->setOrientation(cq);
  Global::setSaveImageType(Global::CubicTopImage);
  drawImageOnScreen();
  //---------------------------------------------------------
  imgFile = f.absolutePath() + QDir::separator() +
            QString("t_") + f.baseName();
  if (m_currFrame >= 0)
    imgFile += QString("%1").arg((int)m_currFrame, fieldWidth, 10, fillChar);
  imgFile += ".";
  imgFile += f.completeSuffix();
  //---------------------------------------------------------
  saveSnapshot(imgFile);

  q = Quaternion(rightVector, -M_PI/2);
  cq = q*origOrientation;
  camera()->setOrientation(cq);
  Global::setSaveImageType(Global::CubicBottomImage);
  drawImageOnScreen();
  //---------------------------------------------------------
  imgFile = f.absolutePath() + QDir::separator() +
            QString("d_") + f.baseName();
  if (m_currFrame >= 0)
    imgFile += QString("%1").arg((int)m_currFrame, fieldWidth, 10, fillChar);
  imgFile += ".";
  imgFile += f.completeSuffix();
  //---------------------------------------------------------
  saveSnapshot(imgFile);

  camera()->setOrientation(origOrientation);
  setFieldOfView(fov);
}

void
Viewer::save360Image(QString localImageFileName,
		     QChar fillChar, int fieldWidth)
{
  QFileInfo f(localImageFileName);	  

  float fov = camera()->fieldOfView();
  Quaternion origOrientation = camera()->orientation();

  setFieldOfView((float)(M_PI/2.0));

  QString imgFile;
  QList<QImage> cubicImages;

  Global::setSaveImageType(Global::CubicFrontImage);
  drawImageOnScreen();	  
  QImage frontImage = getSnapshot();

  Quaternion q, cq;  
  Vec upVector = camera()->upVector();
  Vec rightVector = camera()->rightVector(); 
  
  //----
  q = Quaternion(upVector, -M_PI/2);
  cq = q*origOrientation;
  camera()->setOrientation(cq);
  Global::setSaveImageType(Global::CubicRightImage);
  drawImageOnScreen();
  cubicImages << getSnapshot();

  q = Quaternion(upVector, M_PI/2);
  cq = q*origOrientation;
  camera()->setOrientation(cq);
  Global::setSaveImageType(Global::CubicLeftImage);
  drawImageOnScreen();
  cubicImages << getSnapshot();
  //----

  //----
  q = Quaternion(rightVector, M_PI/2);
  cq = q*origOrientation;
  camera()->setOrientation(cq);
  Global::setSaveImageType(Global::CubicTopImage);
  drawImageOnScreen();
  cubicImages << getSnapshot().mirrored(true, false);

  q = Quaternion(rightVector, -M_PI/2);
  cq = q*origOrientation;
  camera()->setOrientation(cq);
  Global::setSaveImageType(Global::CubicBottomImage);
  drawImageOnScreen();
  cubicImages << getSnapshot().mirrored(true, false);
  //----

  
  //----
  q = Quaternion(upVector, M_PI);
  cq = q*origOrientation;
  camera()->setOrientation(cq);
  Global::setSaveImageType(Global::CubicBackImage);
  drawImageOnScreen();	  
  cubicImages << getSnapshot();

  cubicImages << frontImage;
  //----


  
  QImage panoImage = Cube2Sphere::convert(cubicImages);


  
  //---------------------------------------------------------
  imgFile = f.absolutePath() + QDir::separator() + f.baseName();
  if (m_currFrame >= 0)
    imgFile += QString("%1").arg((int)m_currFrame, fieldWidth, 10, fillChar);
  imgFile += ".";
  imgFile += f.completeSuffix();

  panoImage.save(imgFile);
  //---------------------------------------------------------
  
  camera()->setOrientation(origOrientation);
  setFieldOfView(fov);
}

void
Viewer::saveStereoImage(QString localImageFileName,
			QChar fillChar, int fieldWidth)
{
  QFileInfo f(localImageFileName);	  

  Global::setSaveImageType(Global::LeftImage);
  drawImageOnScreen();	  

  //---------------------------------------------------------
  QString imgFileLeft = f.absolutePath() + QDir::separator() +
                        QString("left_") + f.baseName();
  if (m_currFrame >= 0)
    imgFileLeft += QString("%1").arg((int)m_currFrame, fieldWidth, 10, fillChar);
  imgFileLeft += ".";
  imgFileLeft += f.completeSuffix(); 
  //---------------------------------------------------------

  saveSnapshot(imgFileLeft);
  
  Global::setSaveImageType(Global::RightImage);
  drawImageOnScreen();
  
  //---------------------------------------------------------
  QString imgFileRight = f.absolutePath() + QDir::separator() +
                         QString("right_") + f.baseName();
  if (m_currFrame >= 0)
    imgFileRight += QString("%1").arg((int)m_currFrame, fieldWidth, 10, fillChar);
  imgFileRight += ".";
  imgFileRight += f.completeSuffix();
  //---------------------------------------------------------
  
  saveSnapshot(imgFileRight);
}

void
Viewer::saveMonoImage(QString localImageFileName,
		      QChar fillChar, int fieldWidth)
{
  QFileInfo f(localImageFileName);	  

  Global::setSaveImageType(Global::MonoImage);
  drawImageOnScreen();

  //---------------------------------------------------------
  QString imgFile = f.absolutePath() + QDir::separator() +
	            f.baseName();
  if (m_currFrame >= 0)
    imgFile += QString("%1").arg((int)m_currFrame, fieldWidth, 10, fillChar);
  imgFile += ".";
  imgFile += f.completeSuffix();
  //---------------------------------------------------------

  saveSnapshot(imgFile);
}

void
Viewer::saveImage()
{
  QString localImageFileName = m_imageFileName;
  QChar fillChar = '0';
  int fieldWidth = 0;
  QRegExp rx("\\$[0-9]*[f|F]");
  if (rx.indexIn(m_imageFileName) > -1)
    {
      localImageFileName.remove(rx);
      
      QString txt = rx.cap();
      if (txt.length() > 2)
	{
	  txt.remove(0,1);
	  txt.chop(1);
	  fieldWidth = txt.toInt();
	}
    }

  if (m_imageMode == Enums::MonoImageMode)
    saveMonoImage(localImageFileName, fillChar, fieldWidth);
  else if (m_imageMode == Enums::StereoImageMode)
    saveStereoImage(localImageFileName, fillChar, fieldWidth);
  else if (m_imageMode == Enums::CubicImageMode)
    saveCubicImage(localImageFileName, fillChar, fieldWidth);
  else if (m_imageMode == Enums::PanoImageMode)
    save360Image(localImageFileName, fillChar, fieldWidth);
}

void
Viewer::updateLookFrom(Vec pos, Quaternion rot)
{
  camera()->setPosition(pos);
  camera()->setOrientation(rot);
}

void
Viewer::setKeyFrame(int fno)
{
  // draw image again to get the correct pixmap
  draw();

  Vec pos;
  Quaternion rot;
  pos = camera()->position();
  rot = camera()->orientation();
	  
  Vec bmin, bmax;
  GeometryObjects::trisets()->allEnclosingBox(bmin, bmax);

  QImage image = grabFrameBuffer();
  image = image.scaled(100, 100);

  emit setKeyFrame(pos, rot, fno, image);
}

void
Viewer::captureKeyFrameImage(int kfn)
{
  // draw image again to get the correct pixmap
  draw();

  QImage image = grabFrameBuffer();
  image = image.scaled(512, 512);

  emit replaceKeyFrameImage(kfn, image);
}

void 
Viewer::fastDraw()
{
  if (!m_hiresVolume->raised())
    {
      splashScreen();
      return;
    }

  if (!Global::updateViewer())
    {
      showBackBufferImage();
      return;
    }
  
  setBackgroundColor(QColor(0, 0, 0, 0));
      
  if (m_messageDisplayer->showingMessage())
    m_messageDisplayer->turnOffMessage();
      
  if (m_hiresVolume)
    {
      renderVolume(Enums::StillImage);
    }

  Global::setPlayFrames(false);

  grabBackBufferImage();
}

void 
Viewer::dummydraw()
{
  //bool fboBound = bindFBOs(Enums::StillImage);
  m_hiresVolume->drawDragImage();
  //if (fboBound) releaseFBOs(Enums::StillImage);
}

void
Viewer::paintGL()
{
  glClearColor(0,0,0,0);

  preDraw();
  
  if (camera()->frame()->isManipulated())
    fastDraw();
  else
    draw();
  
  postDraw();  
}


void 
Viewer::draw()
{
  if (!m_hiresVolume->raised())
    {
      splashScreen();
      return;
    }


  if (!Global::updateViewer())
    {
      if (!Global::playFrames())
	showBackBufferImage();
      return;
    }

  
  setBackgroundColor(QColor(0, 0, 0, 0));

  glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
  glEnable(GL_POINT_SMOOTH);

  if (m_messageDisplayer->renderScene())
    {
      if (m_saveSnapshots) // save snapshots without using fbo
	saveImage();
      else if (m_saveMovie) // save movie without using fbo
	saveMovie();
      else
	drawImageOnScreen();
    }

  m_messageDisplayer->drawMessage(size());

  glFinish();


  Global::setPlayFrames(false);

  grabBackBufferImage();

}

void
Viewer::setWidgetSizeToImageSize()
{
  m_imageSizeFlag = true;
//  m_origWidth = size().width();
//  m_origHeight = size().height();
  
  //--------------------------------------------------------
  //if widget width and height are different from image width
  //and height then we have problems rendering text using
  //QGLWidget's rendertext facilty.  We therefore change
  //QGLWidget's width and height to image width and height.
  //We also change camera screen parameters. This adjustment
  //is required so that rendered images can be of different
  //size to that of the window size
  //--------------------------------------------------------
  QWidget::resize(m_imageWidth, m_imageHeight);
}

void
Viewer::restoreOriginalWidgetSize()
{
  if (m_imageWidth != m_origWidth ||
      m_imageHeight != m_origHeight)
    {
      m_hiresVolume->setImageSizeRatio(1);
    }

  m_imageSizeFlag = false;
  // restore original Width and Height
  QWidget::resize(m_origWidth, m_origHeight);
  createImageBuffers();
}

void
Viewer::wheelEvent(QWheelEvent *event)
{
  if (!Global::updateViewer())
    {
      updateGL();
      return;
    }

  QGLViewer::wheelEvent(event);

  m_undo.append(camera()->position(), camera()->orientation());
}



void
Viewer::mousePressEvent(QMouseEvent *event)
{
  m_selectionWindow = QRect(0,0,0,0);
  m_mouseDrag = true;
  m_mousePressPos = event->pos();
  m_mousePrevPos = event->pos();

  if (!Global::updateViewer())
    {
      updateGL();
      return;
    }

    if (GeometryObjects::trisets()->grabMode())
    {      
      if (event->buttons() == Qt::LeftButton &&
	  event->modifiers() & Qt::ShiftModifier)
	{
	  // selection window
	  m_selectionWindow.setTopLeft(event->pos());
	  m_selectionWindow.setSize(QSize(0,0));
	}      
    }


  QGLViewer::mousePressEvent(event);
}

void
Viewer::mouseMoveEvent(QMouseEvent *event)
{
//  if (m_lengthMode)
//    {
//      if (event->buttons() == Qt::NoButton &&
//	  m_gotPoint0)
//	{
//	  checkPointSelected(event);
//	  update();
//	  return;
//	}
//    }

  
  if (!Global::updateViewer())
    {
      updateGL();
      m_mousePrevPos = event->pos();
      return;
    }


  if (event->buttons() == Qt::NoButton)
    GeometryObjects::trisets()->checkMouseHover(this);
  
  if (GeometryObjects::grabsMouse())
    {
      QGLViewer::mouseMoveEvent(event);
      return;
    }

  if (m_paintMode)
    {
      if (event->modifiers() & Qt::ShiftModifier &&
	  (event->buttons() == Qt::LeftButton ||
	   event->buttons() == Qt::RightButton))
	{
	  checkPointSelected(event);
	  update();
	  return;
	}
    }

  if (GeometryObjects::trisets()->grabMode())
    {      
      if (event->buttons() == Qt::LeftButton && event->modifiers() & Qt::ShiftModifier)
	{
	  // selection window
	  QPoint pos = m_selectionWindow.topLeft();
	  QPoint sz = event->pos()-pos;
	  m_selectionWindow.setSize(QSize(sz.x(), sz.y()));

	  GeometryObjects::trisets()->selectionWindow(camera(), m_selectionWindow);

	  update();
	  return;
	}
    }

  QGLViewer::mouseMoveEvent(event);
  m_mousePrevPos = event->pos();
}

void
Viewer::mouseReleaseEvent(QMouseEvent *event)
{
//  if (m_lengthMode)
//    {
//      if (m_gotPoint0)
//	m_lengthMode = false;
//      else
//	{
//	  checkPointSelected(event);
//	  //update();
//	  //return;
//	}
//
//      m_mouseDrag = false;
//      QGLViewer::mouseReleaseEvent(event);      
//      m_undo.append(camera()->position(), camera()->orientation());
//      return;
//    }

  
  m_mouseDrag = false;

  QGLViewer::mouseReleaseEvent(event);

  m_undo.append(camera()->position(), camera()->orientation());
}

void
Viewer::grabBackBufferImage()
{
  glReadBuffer(GL_BACK);

  if (m_backBufferWidth != camera()->screenWidth() &&
      m_backBufferHeight != camera()->screenHeight())
    {
      m_backBufferWidth = camera()->screenWidth();
      m_backBufferHeight = camera()->screenHeight();
      delete [] m_backBufferImage;
      m_backBufferImage = new uchar[4*m_backBufferWidth*m_backBufferHeight];
    }

  glReadPixels(0,
	       0,
	       m_backBufferWidth,
	       m_backBufferHeight,
	       GL_RGBA,
	       GL_UNSIGNED_BYTE,
	       m_backBufferImage);
}

void
Viewer::showBackBufferImage()
{
  if (!m_backBufferImage)
    grabBackBufferImage();

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho(0, m_backBufferWidth, 0, m_backBufferHeight, -1, 1);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
  glRasterPos2i(0,0);
  glDrawPixels(m_backBufferWidth,
	       m_backBufferHeight,
	       GL_RGBA,
	       GL_UNSIGNED_BYTE,
	       m_backBufferImage);
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();

  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();

  if (! m_mouseDrag)
    return;


////  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
////  glEnable(GL_BLEND);
//////  glEnable(GL_LINE_SMOOTH);
//////  glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
////  
////  glDisable(GL_DEPTH_TEST);
////
////  QString toggleUpdates = "Update Disabled (u : toggle updates)";
////  QFont tfont = QFont("Helvetica");
////  tfont.setStyleStrategy(QFont::PreferAntialias);
////
////  //-------------
////  // calculate font scale based on dpi
////  float fscl = 120.0/Global::dpi();
////  //-------------
////
////  tfont.setPointSize(20*fscl);
////
////  StaticFunctions::renderText(10, size().height()/2,
////			      toggleUpdates, tfont,
////			      Qt::transparent, Qt::lightGray);
////
////  glEnable(GL_DEPTH_TEST);
}

void
Viewer::enterEvent(QEvent *e)
{
  grabKeyboard();
}

void
Viewer::leaveEvent(QEvent *e)
{
  releaseKeyboard();
}

void
Viewer::undoParameters()
{
  camera()->setPosition(m_undo.pos());
  camera()->setOrientation(m_undo.rot());
}

void
Viewer::measureLength()
{
//  m_lengthMode = true;
//  m_gotPoint0 = false;
}

void
Viewer::keyPressEvent(QKeyEvent *event)
{  
  // Toggle FullScreen - hide menubar on fullscreen
  if (event->key() == Qt::Key_Return &&
      event->modifiers() & Qt::AltModifier)
    {
      QWidget *mw = MainWindowUI::mainWindowUI()->menubar->parentWidget();
      mw->setWindowState(mw->windowState() ^ Qt::WindowFullScreen);
      if (mw->windowState() == Qt::WindowFullScreen)
	{
	  MainWindowUI::mainWindowUI()->menubar->hide();
	  MainWindowUI::mainWindowUI()->statusBar->hide();
	}
      else
	MainWindowUI::mainWindowUI()->menubar->show();
      return;
    }

  if (GeometryObjects::grabsMouse())
    {
      if (GeometryObjects::keyPressEvent(event))
	{
	  updateGL();
	  return;
	}
    }

  // process clipplane events
  if (GeometryObjects::clipplanes()->keyPressEvent(event))
    {
      updateGL();
      return;
    }
  
  if (event->key() == Qt::Key_V)
    {
      GeometryObjects::showGeometry = ! GeometryObjects::showGeometry;
      if (GeometryObjects::showGeometry)
	GeometryObjects::show();
      else
	GeometryObjects::hide();

      updateGL();
    }

  if (event->key() == Qt::Key_G)
    {
      // toggle mouse grabs for geometry objects
      GeometryObjects::inPool = ! GeometryObjects::inPool;

      emit changeSelectionMode(GeometryObjects::inPool);
    }
  
	    
  if (event->key() == Qt::Key_H)
    {
      if (event->modifiers() & Qt::ControlModifier ||
	  event->modifiers() & Qt::MetaModifier)
	showHelp();
      return;
    }

  if (event->key() == Qt::Key_P)
    {
//      m_lengthMode = !m_lengthMode;
//      m_gotPoint0 = false;
//      GeometryObjects::hitpoints()->clear();

      QList<Vec> pts;
      if (GeometryObjects::hitpoints()->activeCount())
	pts = GeometryObjects::hitpoints()->activePoints();
      else
	pts = GeometryObjects::hitpoints()->points();

      if (pts.count() > 1)
	{
	  ///QMessageBox::information(0, "", QString("%1 %2 %3").arg(pts[0].x).arg(pts[0].y).arg(pts[0].z));
	  //Vec scrpt = camera()->projectedCoordinatesOf(pts[0]);
	  ///QMessageBox::information(0, "", QString("%1 %2 %3").arg(scrpt.x).arg(scrpt.y).arg(scrpt.z));
	  //Vec target = camera()->unprojectedCoordinatesOf(Vec(scrpt.x, scrpt.y-10, scrpt.z));
	  ///QMessageBox::information(0, "", QString("%1 %2 %3").arg(target.x).arg(target.y).arg(target.z));
	  //Vec targetRed = target-pts[0];
	  
	  //GeometryObjects::paths()->addPath(pts, camera()->upVector());
	  GeometryObjects::paths()->addPath(pts);

	  // now remove points that were used to make the path
	  if (GeometryObjects::hitpoints()->activeCount())
	    GeometryObjects::hitpoints()->removeActive();
	  else
	    GeometryObjects::hitpoints()->clear();

	  updateGL();
	}
      else
	QMessageBox::critical(0, "Error", "Need at least 2 points to form a path"); 
    }

  if (!m_hiresVolume->raised())
    return;

  
  if (event->key() == Qt::Key_Z &&
      (event->modifiers() & Qt::ControlModifier ||
       event->modifiers() & Qt::MetaModifier) )
    {
      m_undo.undo();
      undoParameters();
      updateGL();
      return;
    }
  if (event->key() == Qt::Key_Y &&
      (event->modifiers() & Qt::ControlModifier ||
       event->modifiers() & Qt::MetaModifier) )
    {
      m_undo.redo();
      undoParameters();
      updateGL();
      return;
    }
 
  if (m_hiresVolume->raised())
    {
      if (m_hiresVolume->keyPressEvent(event))
	{
	  updateGL();
	  return;
	}

      if (event->key() == Qt::Key_T)
	{
	  CaptionDialog cd(0,
			   "Caption",
			   QFont("Helvetica", 15),
			   QColor::fromRgbF(1,1,1,1),
			   QColor::fromRgbF(1,1,1,1),
			   0);
	  cd.hideAngle(false);
	  int cdW = cd.width();
	  int cdH = cd.height();
	  cd.move(QCursor::pos() - QPoint(cdW/2, cdH/2));
	  if (cd.exec() == QDialog::Accepted)
	    {
	      QString text = cd.text();
	      QFont font = cd.font();
	      QColor color = cd.color();
	      QColor haloColor = cd.haloColor();
	      float angle = cd.angle();

	      CaptionObject co;
	      co.set(QPointF(0.5, 0.5),
		     text, font,
		     color, haloColor,
		     angle);
	      GeometryObjects::captions()->add(co);
	    }
	  
	  updateGL();
	  return;
	}
    }

  if (event->key() == Qt::Key_B)
    {
      emit switchBB();
      updateGL();
      return;
    }


  if (event->key() == Qt::Key_Escape)
    {
      int ok = QMessageBox::question(0, tr("Exit DrishtiMesh"),
				     QString(tr("Do you really want to exit DrishtiMesh ?")),
				     QMessageBox::Yes | QMessageBox::No);
      if (ok == QMessageBox::Yes)
	emit quitDrishti();

      return;
    }

  if (event->key() == Qt::Key_S &&
      event->modifiers() & Qt::AltModifier)
    {
      grabScreenShot();
      return;
    }

  QGLViewer::keyPressEvent(event);

  if (event->key() == Qt::Key_C &&
      (event->modifiers() & Qt::ControlModifier ||
       event->modifiers() & Qt::MetaModifier) )
    QMessageBox::information(0, "", tr("Image copied to clipboard."));

  if (event->key() == Qt::Key_Delete ||
      event->key() == Qt::Key_Backspace ||
      event->key() == Qt::Key_Backtab)
    {
      GeometryObjects::trisets()->removeHoveredHitPoint(); 
      emit updateGL();
    }

}

void
Viewer::grabScreenShot()
{
  QSize imgSize = StaticFunctions::getImageSize(size().width(),size().height());
  //setImageSize(imgSize.width(), imgSize.height());

  QString flnm;
  flnm = QFileDialog::getSaveFileName(0,
				      tr("Save snapshot"),
				      Global::previousDirectory(),
       "Image Files (*.png *.tif *.bmp *.jpg *.ppm *.xbm *.xpm)");
  
  if (flnm.isEmpty())
    return;

  // --- get image type ---
  QStringList items;
  items << "Mono Image";
  items << "Stereo Image";
  items << "Cubic Image";
  items << "Pano Image";
  bool ok;
  QString str;
  str = QInputDialog::getItem(0,
			      tr("Image Type"),
			      tr("Image Type"),
			      items,
			      0,
			      false, // text is not editable
			      &ok);
  if (!ok ||
      str == "Mono Image")
    setImageMode(Enums::MonoImageMode);
  else
    {
      if (str == "Stereo Image")
	setImageMode(Enums::StereoImageMode);
      else if (str == "Cubic Image")
	setImageMode(Enums::CubicImageMode);
      else
	setImageMode(Enums::PanoImageMode);
    }

  setCurrentFrame(-1);
  setImageFileName(flnm);
  setSaveSnapshots(true);

  setImageSize(imgSize.width(), imgSize.height());

  //dummydraw();
  draw();
  endPlay();
  emit showMessage(tr("Snapshot saved"), false);
}

void
Viewer::init()
{
  // remove keyboards shortcuts
  setPathKey(-Qt::Key_F1);
  setPathKey(-Qt::Key_F2);
  setPathKey(-Qt::Key_F3);
  setPathKey(-Qt::Key_F4);
  setPathKey(-Qt::Key_F5);
  setPathKey(-Qt::Key_F6);
  setPathKey(-Qt::Key_F7);
  setPathKey(-Qt::Key_F8);
  setPathKey(-Qt::Key_F9);
  setPathKey(-Qt::Key_F10);
  setPathKey(-Qt::Key_F11);
  setPathKey(-Qt::Key_F12);
  setShortcut(EXIT_VIEWER,	0);
  setShortcut(DRAW_GRID,	0);
  setShortcut(ANIMATION,	0);
  setShortcut(EDIT_CAMERA,	0);
  setShortcut(MOVE_CAMERA_LEFT,	0);
  setShortcut(MOVE_CAMERA_RIGHT,0);
  setShortcut(MOVE_CAMERA_UP,	0);
  setShortcut(MOVE_CAMERA_DOWN,	0);
  setShortcut(INCREASE_FLYSPEED,0);
  setShortcut(DECREASE_FLYSPEED,0);

  setShortcut(CAMERA_MODE, Qt::ALT+Qt::Key_F);
  setShortcut(SAVE_SCREENSHOT, Qt::ALT+Qt::Key_S);

  setMouseBinding(Qt::SHIFT+Qt::RightButton, SELECT);
  setMouseBinding(Qt::SHIFT+Qt::LeftButton, SELECT);

  setMouseBinding(Qt::ControlModifier, Qt::LeftButton, CAMERA, SCREEN_ROTATE);

  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
  //  glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
  //  glEnable(GL_LINE_SMOOTH);  // antialias lines	
  glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
  glEnable(GL_POINT_SMOOTH);  // antialias lines 
  glDisable(GL_LIGHTING);

  //Global::setBackgroundColor(Vec(0, 0, 0));
  glClearColor(0.1,0.1,0.1,0.1);
  
  // Restore previous viewer state.
  //restoreStateFromFile();
  // Opens help window
  // help();

  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
}


void
Viewer::processCommand(QString cmd)
{
  bool ok;
  QString ocmd = cmd;
  cmd = cmd.toLower();
  QStringList list = cmd.split(" ", QString::SkipEmptyParts);
 
  if (list[0] == "load")
    {
      if (list.size() == 2)
	emit loadSurfaceMesh(list[1]);
      return;
    }
  
  if (list[0] == "resetcamera")
    {
      // if in fly mode switch it off 
      if (mouseButtonState(CAMERA, ROTATE) == Qt::NoButton)
	toggleCameraMode();
      // now show the entire scene
      showEntireScene();
    }
  else if (list[0] == "autospin")
    {
      if (list.size() > 1)
	{
	  if (list[1] == "off" || list[1] == "no" )
	    camera()->frame()->setSpinningSensitivity(100.0);
	}
      else
	camera()->frame()->setSpinningSensitivity(0.3);
    }
  else if (list[0] == "autoupdate")
    {
      bool au = true;
      if (list.size() > 1)
	if (list[1] == "no") au = false;
      if (au)
	m_autoUpdateTimer.start();
      else
	m_autoUpdateTimer.stop();
    }
  else if (list[0] == "addrotationanimation")
    {
      int axis = 0;
      float angle = 360;
      int frames = 360;
      if (list.size() > 1)
	{
	  if (list[1] == "x") axis = 0;
	  if (list[1] == "y") axis = 1;
	  if (list[1] == "z") axis = 2;
	}
      if (list.size() > 2) angle = list[2].toFloat(&ok);
      if (list.size() > 3) frames = list[3].toInt(&ok);
      emit addRotationAnimation(axis, angle, frames);
    }
  else if (list[0] == "imagesize")
    {
      m_imageWidth = list[1].toInt(&ok);
      m_imageHeight = list[2].toInt(&ok);
    }
  else if (list[0] == "addpoint" ||
	   list[0] == "point")
    {
      Vec pos;
      float x=0,y=0,z=0;
      if (list.size() > 1) x = list[1].toFloat(&ok);
      if (list.size() > 2) y = list[2].toFloat(&ok);
      if (list.size() > 3) z = list[3].toFloat(&ok);
      pos = Vec(x,y,z);
      GeometryObjects::hitpoints()->add(pos);
    }
  else if (list[0] == "deselectall")
    {
      GeometryObjects::hitpoints()->resetActive();
    }
  else if (list[0] == "removepoints")
    {
      if (list[1] == "all")
	GeometryObjects::hitpoints()->clear();
      else if (list[1] == "selected")
	GeometryObjects::hitpoints()->removeActive();
    }
  else if (list[0]== "pointsize")
    {
      if (list.size() > 1)
	{
	  int ptsz = list[1].toInt(&ok);
	  GeometryObjects::hitpoints()->setPointSize(ptsz);
	}
    }
  else if (list[0]== "floatprecision")
    {
      if (list.size() > 1)
	{
	  int fp = list[1].toInt(&ok);
	  Global::setFloatPrecision(fp);
	}
    }
  else if (list[0]== "pointcolor")
    {
      Vec pcol = GeometryObjects::hitpoints()->pointColor();
      QColor qcol = QColor(pcol.x*255, pcol.y*255, pcol.z*255);
      QColor color = DColorDialog::getColor(qcol);
      if (color.isValid())
	{
	  pcol = Vec(color.redF(),
		     color.greenF(),
		     color.blueF());

	  GeometryObjects::hitpoints()->setPointColor(pcol);
	}
    }
  else if (list[0]== "loadimage")
    {
      QString flnm;
      flnm = QFileDialog::getOpenFileName(0,
					  "Load background image",
					  Global::previousDirectory(),
            "Image Files (*.png *.tif *.tga *.bmp *.jpg *.jpeg *.ppm *.xbm *.xpm)");
  
      if (flnm.isEmpty())
	return;

      //----------------
      // bgimage file is assumed to be relative to .pvl.nc file
      // get the absolute path
      //VolumeInformation pvlInfo = VolumeInformation::volumeInformation();
      //QFileInfo fileInfo(pvlInfo.pvlFile);
      //flnm = fileInfo.absoluteDir().relativeFilePath(flnm);
      QFileInfo fileInfo(Global::previousDirectory());
      flnm = fileInfo.absoluteDir().relativeFilePath(flnm);
      Global::setBackgroundImageFile(flnm, fileInfo.absolutePath());
      //----------------
    }
  else if (list[0]== "resetimage")
    {
      Global::resetBackgroundImageFile();
    }
  else if (list[0]== "path")
    {
      QList<Vec> pts;
      if (GeometryObjects::hitpoints()->activeCount())
	pts = GeometryObjects::hitpoints()->activePoints();
      else
	pts = GeometryObjects::hitpoints()->points();

      if (pts.count() > 1)
	{
	  GeometryObjects::paths()->addPath(pts);

	  // now remove points that were used to make the path
	  if (GeometryObjects::hitpoints()->activeCount())
	    GeometryObjects::hitpoints()->removeActive();
	  else
	    GeometryObjects::hitpoints()->clear();
	}
      else
	QMessageBox::critical(0, "Error", "Need at least 2 points to form a path"); 
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
	  Vec cpos = camera()->position();
	  pos = pos + cpos;
	}

      Vec prevpos = camera()->position();
      camera()->setPosition(pos);
      Vec view = camera()->sceneCenter() - camera()->position();
      if (view.norm() > 0)
	view.normalize();
      else
	view = camera()->sceneCenter() - prevpos;

      camera()->setViewDirection(view);
    }
  else if (list[0] == "movescreenx" ||
	   list[0] == "movescreeny" ||
	   list[0] == "movescreenz")
    {
      Vec pos;
      float v=0;
      if (list.size() > 1) v = list[1].toFloat(&ok);
      if (list[0] == "movescreenx")
	pos = v * camera()->rightVector();
      else if (list[0] == "movescreeny")
	pos = v * camera()->upVector();
      else if (list[0] == "movescreenz")
	pos = v * camera()->viewDirection();

      Vec cpos = camera()->position();
      pos = pos + cpos;

      camera()->setPosition(pos);
      // do not recalculate view vector
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
	  Quaternion orot = camera()->orientation();
	  rot = orot*rot;
	}
      // set camera orientation
      camera()->setOrientation(rot);

      // now reposition the camera so that it faces the scene
      Vec center = camera()->sceneCenter();
      float dist = (camera()->position()-center).norm();
      Vec viewDir = camera()->viewDirection();
      camera()->setPosition(center - dist*viewDir);
    }
  else if (list[0] == "rotatescreenx" ||
	   list[0] == "rotatescreeny" ||
	   list[0] == "rotatescreenz")
    {
      Quaternion rot;
      float a=0;
      if (list.size() > 1) a = list[1].toFloat(&ok);

      if (list[0] == "rotatescreenx")
	rot = Quaternion(camera()->rightVector(), DEG2RAD(a));
      else if (list[0] == "rotatescreeny")
	rot = Quaternion(camera()->upVector(), DEG2RAD(a));
      else if (list[0] == "rotatescreenz")
	rot = Quaternion(camera()->viewDirection(), DEG2RAD(a));

      Quaternion orot = camera()->orientation();
      rot = orot*rot;
      
      Vec offset;
      {
	Vec center = camera()->sceneCenter();
	float dist = (camera()->position()-center).norm();
	Vec viewDir = camera()->viewDirection();
	Vec pos = (center - dist*viewDir);
	offset = pos - camera()->position();
      }

      // set camera orientation
      camera()->setOrientation(rot);

      // now reposition the camera so that it faces the scene
      Vec center = camera()->sceneCenter();
      float dist = (camera()->position()-center).norm();
      Vec viewDir = camera()->viewDirection();
      //camera()->setPosition(center - dist*viewDir);
      camera()->setPosition(center - dist*viewDir - offset);
    }
  else if (list[0] == "clip")
    {
      QList<Vec> pts;
      if (GeometryObjects::hitpoints()->activeCount())
	pts = GeometryObjects::hitpoints()->activePoints();
      else
	pts = GeometryObjects::hitpoints()->points();

      if (pts.count() == 3)
	{
	  GeometryObjects::clipplanes()->addClip(pts[0],
						 pts[1],
						 pts[2]);

	  // now remove points that were used to make the clip
	  if (GeometryObjects::hitpoints()->activeCount())
	    GeometryObjects::hitpoints()->removeActive();
	  else
	    GeometryObjects::hitpoints()->clear();
	}
      else
	QMessageBox::critical(0, "Error", "Need 3 points to add clipplane"); 
    }
  else if (list[0] == "getangle")
    {
      QList<Vec> pts;
      if (GeometryObjects::hitpoints()->activeCount())
	pts = GeometryObjects::hitpoints()->activePoints();
      else
	pts = GeometryObjects::hitpoints()->points();

      if (pts.count() == 3)
	{
	  float angle = StaticFunctions::calculateAngle(pts[0],
							pts[1],
							pts[2]);
	  QMessageBox::information(0, "Angle",
				   QString("%1").arg(angle));
	}
      else
	QMessageBox::critical(0, "Error", "Need 3 points to find angle"); 
    }
  else if (list[0] == "setfov")
    {
      if (list.size() == 2)
	{
	  float fov = list[1].toFloat(&ok);
	  setFieldOfView(DEG2RAD(fov));
	}
      else
	QMessageBox::critical(0, "Error", "Field of view not specified"); 
    }
  else if (list[0] == "resetfov")
    {
      setFieldOfView((float)(M_PI/4.0));
    }
  else
    QMessageBox::critical(0, "Error",
			  QString("Cannot understand the command : ") +
			  cmd);
}

void
Viewer::showHelp()
{
  PropertyEditor propertyEditor;
  QMap<QString, QVariantList> plist;
  QVariantList vlist;

  vlist.clear();
  QFile helpFile(":/general.help"+Global::helpLanguage());
  if (helpFile.open(QFile::ReadOnly))
    {
      QTextStream in(&helpFile);
      in.setCodec("Utf-8");
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


  QStringList keys;
  keys << "commandhelp";
  
  propertyEditor.set("General Help", plist, keys);
  propertyEditor.exec();
}


//----------------------------------
// these are the functions called via menubar
#include "menuviewerfunctions.cpp" 
//----------------------------------

void
Viewer::setPaintRoughness(QString t)
{
  if (t == "Ridge") m_paintRoughness = 0;

  if (t == "Marble") m_paintRoughness = 1;

  // Fractal Brownian Motion
  if (t == "FBM") m_paintRoughness = 2;
}
