#include "viewer.h"
#include "global.h"
#include "geometryobjects.h"
#include "lighthandler.h"
#include "staticfunctions.h"
#include "dialogs.h"
#include "captiondialog.h"
#include "enums.h"
#include "shaderfactory.h"
#include "propertyeditor.h"
#include "dcolordialog.h"
#include "rawvolume.h"
#include "prunehandler.h"
#include "itksegmentation.h"
#include "mainwindowui.h"
#include "dcolordialog.h"

#include "cube2sphere.h"

#include <stdio.h>
#include <math.h>
#include <fstream>
#include <time.h>

#include <QInputDialog>
#include <QFileDialog>


using namespace std;

bool carveHitPointOK = false;
Vec carveHitPoint;

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

unsigned char* Viewer::lookupTable() { return m_lut; }

void Viewer::setTag(int t) { PruneHandler::setTag(t); updateGL(); }
void Viewer::setCarveRadius(int r)
{
  if (m_paintMode)
    {
      setPaintRadius(r);
      return;
    }
  
  PruneHandler::setCarveRad(r);
  updateGL();
}

void Viewer::setVolume(Volume *vol) { m_Volume = vol; }
void Viewer::setHiresVolume(DrawHiresVolume *vol) { m_hiresVolume = vol; }
void Viewer::setLowresVolume(DrawLowresVolume *vol) { m_lowresVolume = vol; }
void Viewer::setKeyFrame(KeyFrame *keyframe) { m_keyFrame = keyframe; }
void Viewer::setBricksWidget(BricksWidget *bw) { m_bricksWidget = bw; }
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
  m_focusDistance = camera()->focusDistance();
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
    {
      if (m_hiresVolume->raised())
	m_saveSnapshots = flag;
      else
	emit showMessage("Cannot save image sequence in Lowres mode", true);
    }
}
void Viewer::setSaveMovie(bool flag)
{
  if (flag && m_hiresVolume->raised())
    m_saveMovie = flag;
  else if (flag)
    emit showMessage("Cannot save movie in Lowres mode", true);
}
void
Viewer::reloadData()
{
  if (!m_hiresVolume->raised())
    return;

  qApp->setOverrideCursor(QCursor(Qt::WaitCursor));

  Vec bmin, bmax;
  m_lowresVolume->subvolumeBounds(bmin, bmax);

  if (Global::volumeType() == Global::RGBVolume)
    m_hiresVolume->updateSubvolume(Global::volumeNumber(),
				   bmin, bmax, true);
  else if (Global::volumeType() == Global::RGBAVolume)
    m_hiresVolume->updateSubvolume(Global::volumeNumber(),
				   bmin, bmax, true);
  else if (Global::volumeType() == Global::SingleVolume ||
	   Global::volumeType() == Global::DummyVolume)
    m_hiresVolume->updateSubvolume(Global::volumeNumber(),
				   bmin, bmax, true);
  else if (Global::volumeType() == Global::DoubleVolume)
    m_hiresVolume->updateSubvolume(Global::volumeNumber(0),
				   Global::volumeNumber(1),
				   bmin, bmax, true);
  else if (Global::volumeType() == Global::TripleVolume)
    m_hiresVolume->updateSubvolume(Global::volumeNumber(0),
				   Global::volumeNumber(1),
				   Global::volumeNumber(2),
				   bmin, bmax, true);
  else if (Global::volumeType() == Global::QuadVolume)
    m_hiresVolume->updateSubvolume(Global::volumeNumber(0),
				   Global::volumeNumber(1),
				   Global::volumeNumber(2),
				   Global::volumeNumber(3),
				   bmin, bmax, true);


  
  emit histogramUpdated(m_hiresVolume->histogramImage1D(),
			m_hiresVolume->histogramImage2D());
  

//  // force prunebuffer and lightbuffer regeneration
//  updateLookupTable();
//  updatePruneBuffer(true);
//  updateLightBuffers();
  
  qApp->restoreOverrideCursor();
}

void
Viewer::switchToHires()
{
  m_undo.clear();

  m_lowresVolume->lower();
  m_hiresVolume->raise();


  Vec bmin, bmax;
  m_lowresVolume->subvolumeBounds(bmin, bmax);

  if (Global::volumeType() == Global::RGBVolume)
    m_hiresVolume->updateSubvolume(Global::volumeNumber(),
				   bmin, bmax, true);
  else if (Global::volumeType() == Global::RGBAVolume)
    m_hiresVolume->updateSubvolume(Global::volumeNumber(),
				   bmin, bmax, true);
  else if (Global::volumeType() == Global::SingleVolume ||
	   Global::volumeType() == Global::DummyVolume)
    m_hiresVolume->updateSubvolume(Global::volumeNumber(),
				   bmin, bmax, true);
  else if (Global::volumeType() == Global::DoubleVolume)
    m_hiresVolume->updateSubvolume(Global::volumeNumber(0),
				   Global::volumeNumber(1),
				   bmin, bmax, true);
  else if (Global::volumeType() == Global::TripleVolume)
    m_hiresVolume->updateSubvolume(Global::volumeNumber(0),
				   Global::volumeNumber(1),
				   Global::volumeNumber(2),
				   bmin, bmax, true);
  else if (Global::volumeType() == Global::QuadVolume)
    m_hiresVolume->updateSubvolume(Global::volumeNumber(0),
				   Global::volumeNumber(1),
				   Global::volumeNumber(2),
				   Global::volumeNumber(3),
				   bmin, bmax, true);


  if (GlewInit::initialised())
    m_hiresVolume->initShadowBuffers(true);
  
  // always keep image captions in mouse grabber pool
  GeometryObjects::imageCaptions()->addInMouseGrabberPool();
}


void
Viewer::switchDrawVolume()
{
  qApp->setOverrideCursor(QCursor(Qt::WaitCursor));
  //updateLookupTable(); 

  if (m_lowresVolume->raised())
    {
      Global::disableViewerUpdate();
      MainWindowUI::changeDrishtiIcon(false);

      switchToHires();

      if (Global::volumeType() != Global::DummyVolume)
	emit histogramUpdated(m_hiresVolume->histogramImage1D(),
			      m_hiresVolume->histogramImage2D());

      
      emit setHiresMode(true);

      createImageBuffers();

      Global::enableViewerUpdate();
      MainWindowUI::changeDrishtiIcon(true);
      showFullScene();

      if (Global::volumeType() != Global::DummyVolume)
	updateLookupTable(); 

	MainWindowUI::mainWindowUI()->actionHiresMode->setChecked(true);
    }
  else
    {
      MainWindowUI::mainWindowUI()->actionHiresMode->setChecked(false);

      PruneHandler::setCarve(false);
      PruneHandler::setPaint(false);
      //GeometryObjects::hitpoints()->ignore(false);

      GeometryObjects::removeFromMouseGrabberPool();
      LightHandler::removeFromMouseGrabberPool();

      m_lowresVolume->raise();
      m_hiresVolume->lower();

      m_lowresVolume->load3dTexture();

      updateLookupTable();

      emit histogramUpdated(m_lowresVolume->histogramImage1D(),
			    m_lowresVolume->histogramImage2D());
      emit setHiresMode(false);

      Vec smin = m_lowresVolume->volumeMin();
      Vec smax = m_lowresVolume->volumeMax();

      setSceneBoundingBox(smin, smax);
      showEntireScene();
    }

  emit updateGL();
  qApp->processEvents();

  qApp->restoreOverrideCursor();
}

void
Viewer::showFullScene()
{
  Vec smin, smax;
  
  smin = VECPRODUCT(m_hiresVolume->volumeMin(),
		    Global::voxelScaling());
  smax = VECPRODUCT(m_hiresVolume->volumeMax(),
		    Global::voxelScaling());

  if (Global::volumeType() == Global::DummyVolume && GeometryObjects::trisets()->count() > 0)
    {
      GeometryObjects::trisets()->allEnclosingBox(smin, smax);
      if (GeometryObjects::networks()->count() > 0)
	{
	  Vec bmin, bmax;
	  GeometryObjects::networks()->allEnclosingBox(bmin, bmax);      
	  smin = StaticFunctions::minVec(smin, bmin);
	  smax = StaticFunctions::maxVec(smax, bmax);
	}
      m_hiresVolume->overwriteDataMinMax(smin, smax);
      m_hiresVolume->setBrickBounds(smin, smax);
    }
  

  setSceneBoundingBox(smin, smax);

  emit stereoSettings(camera()->focusDistance(),
		      camera()->IODistance(),
		      camera()->physicalScreenWidth());

  m_focusDistance = camera()->focusDistance();

  showEntireScene();
}


void
Viewer::updateScaling()
{
  if (m_hiresVolume->raised())
    {
      Vec smin = VECPRODUCT(m_hiresVolume->volumeMin(), Global::voxelScaling());
      Vec smax = VECPRODUCT(m_hiresVolume->volumeMax(), Global::voxelScaling());

      if (Global::volumeType() == Global::DummyVolume && GeometryObjects::trisets()->count() > 0)
	{
	  GeometryObjects::trisets()->allEnclosingBox(smin, smax);
	  if (GeometryObjects::networks()->count() > 0)
	    {
	      Vec bmin, bmax;
	      GeometryObjects::networks()->allEnclosingBox(bmin, bmax);      
	      smin = StaticFunctions::minVec(smin, bmin);
	      smax = StaticFunctions::maxVec(smax, bmax);
	    }
	  m_hiresVolume->overwriteDataMinMax(smin, smax);
	}

      setSceneBoundingBox(smin, smax);
    }
  else
    {
//      Vec smin = VECPRODUCT(m_lowresVolume->volumeMin(), Global::voxelScaling());
//      Vec smax = VECPRODUCT(m_lowresVolume->volumeMax(), Global::voxelScaling());
      Vec smin = m_lowresVolume->volumeMin();
      Vec smax = m_lowresVolume->volumeMax();
      setSceneBoundingBox(smin, smax);
    }

}

void
Viewer::resizeGL(int width, int height)
{
  if (savingImages())
    return;
  
  if (!m_imageSizeFlag)
    {
      m_origWidth = width;
      m_origHeight = height;
    }

  if (m_messageDisplayer->showingMessage())
    m_messageDisplayer->turnOffMessage();

  QGLViewer::resizeGL(width, height);

  createImageBuffers();


  if (GeometryObjects::trisets()->count() > 0)
    GeometryObjects::trisets()->resize(width, height);
}

void
Viewer::createImageBuffers()
{
  int ibw = m_origWidth;
  int ibh = m_origHeight;

  if (Global::imageQuality() == Global::_LowQuality)
   {
     ibw = m_origWidth/2;
     ibh = m_origHeight/2;
   }
  else if (Global::imageQuality() == Global::_VeryLowQuality)
   {
     ibw = m_origWidth/4;
     ibh = m_origHeight/4;
   }

  QGLFramebufferObjectFormat fbFormat;
  fbFormat.setInternalTextureFormat(GL_RGBA16F_ARB);
  fbFormat.setAttachment(QGLFramebufferObject::Depth);
  //fbFormat.setSamples(2);
  fbFormat.setTextureTarget(GL_TEXTURE_RECTANGLE_EXT);

  if (m_imageBuffer) delete m_imageBuffer;
  m_imageBuffer = new QGLFramebufferObject(ibw, ibh, fbFormat);

  if (! m_imageBuffer->isValid())
    QMessageBox::information(0, "", "invalid imageBuffer");

  if (m_lowresBuffer) delete m_lowresBuffer;
  m_lowresBuffer = new QGLFramebufferObject(m_origWidth/4,
					    m_origHeight/4,
					    fbFormat);
					    //GL_TEXTURE_RECTANGLE_EXT);
  if (! m_lowresBuffer->isValid())
    QMessageBox::information(0, "", "invalid lowresBuffer");

  if (GlewInit::initialised())
    m_hiresVolume->initShadowBuffers(true);


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

  m_Volume = 0;
  
  m_currFrame = 1;
  Global::setFrameNumber(m_currFrame);

  m_copyShader = 0;
  m_blurShader = 0;

  m_saveSnapshots = false;
  m_saveMovie = false;
  m_useFBO = true;

  m_hiresVolume = 0;
  m_lowresVolume = 0;

  //m_autoUpdateTimer = new QTimer(this);
  connect(&m_autoUpdateTimer, SIGNAL(timeout()),
	  this, SLOT(updateGL()));

  camera()->setIODistance(0.06f);
  camera()->setPhysicalScreenWidth(4.3f);

  connect(this, SIGNAL(pointSelected(const QMouseEvent*)),
	  this, SLOT(checkPointSelected(const QMouseEvent*)));

  connect(this, SIGNAL(processMops()),
	  this, SLOT(processMorphologicalOperations()));

  m_paintTex = 0;
  glGenTextures(1, &m_lutTex);

  LightHandler::setLutTex(m_lutTex);

  m_lutImage = QImage(256, 256, QImage::Format_RGB32);

  setMinimumSize(200, 200); 

  m_lut = new unsigned char[Global::lutSize()*256*256*4];
  memset(m_lut, 0, Global::lutSize()*256*256*4);

  m_prevLut = new unsigned char[Global::lutSize()*256*256*4];
  memset(m_prevLut, 0, Global::lutSize()*256*256*4);

#ifdef USE_GLMEDIA
  m_movieWriterLeft = 0;
  m_movieWriterRight = 0;
#endif // USE_GLMEDIA
  m_movieFrame = 0;

  m_messageDisplayer = new MessageDisplayer(this);

  m_imageSizeFlag = false;
  m_imageBuffer = 0;
  m_lowresBuffer = 0;
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

  m_paintMenuWidget = 0;
  m_paintMode = false;
  m_paintRad = 5;
  m_paintColor = Vec(1,1,1);
  m_paintStyle = 0;
  m_paintAlpha = 0.05;
  m_paintOctave = 1;
  
  m_tagSpinBox = new QSpinBox(this);
  m_tagSpinBox->setRange(0, 200);
  m_tagSpinBox->setValue(0);
  m_tagSpinBox->setFont(QFont("Helvetica", 20));
  m_tagSpinBox->setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint);
  m_tagSpinBox->setWindowTitle("Tag Selector");
  connect(m_tagSpinBox, SIGNAL(valueChanged(int)),
	  this, SLOT(setTag(int)));
  m_tagSpinBox->hide();

  m_radSpinBox = new QSpinBox(this);
  m_radSpinBox->setRange(0, 200);
  m_radSpinBox->setValue(10);
  m_radSpinBox->setFont(QFont("Helvetica", 20));
  m_radSpinBox->setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint);
  m_radSpinBox->setWindowTitle("Radius");
  connect(m_radSpinBox, SIGNAL(valueChanged(int)),
	  this, SLOT(setCarveRadius(int)));
  m_radSpinBox->hide();

  m_brickAngleFromMouse = false;
}

void
Viewer::brickAngleFromMouse(bool flag)
{
  m_brickAngleFromMouse = flag;
}

void
Viewer::initSocket()
{
  m_listeningSocket = new QUdpSocket(this);
  m_socketPort = 7755;
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
  if (m_lowresVolume->raised())
    {
      QMessageBox::information(0, "", "Please switch to hires slices mode for point selection");
      return;
    }


  bool found;
  QPoint scr = event->pos();
  int ow = camera()->screenWidth();
  int oh = camera()->screenHeight();
  int ic = GeometryObjects::clipplanes()->inViewport(scr.x(), scr.y(),
						     ow, oh);
  if (ic >= 0) // point selected is in a viewport
    {
      Vec target = checkPointSelectedInViewport(ic, scr, found);
      if (found)
	{
	  target = VECPRODUCT(target, Global::voxelScaling());
	  if (!PruneHandler::carve() && !PruneHandler::paint())
	    {
	      if (GeometryObjects::paths()->continuousAdd())
		GeometryObjects::paths()->addPoint(target);
	      else		
		GeometryObjects::hitpoints()->add(target);  
	    }
	}
      return;
    }

  Vec target = m_hiresVolume->pointUnderPixel(scr, found);
  target = VECPRODUCT(target, Global::voxelScaling());

  if (found)
    {
      if (m_paintMode)
	{
	    if (event->buttons() == Qt::LeftButton) // change rotation pivot
	      GeometryObjects::trisets()->paint(target, m_paintRad*m_unitPaintRad,
						m_paintColor, m_paintStyle, m_paintAlpha, m_paintOctave);
	    else
	      GeometryObjects::trisets()->paint(target, m_paintRad*m_unitPaintRad,
						Vec(100,100,100), m_paintStyle, m_paintAlpha, m_paintOctave);
	    
	    return;
	}


      if (!PruneHandler::carve() && !PruneHandler::paint())
	{
	  if (event->buttons() == Qt::RightButton) // change rotation pivot
	    {
	      target = m_hiresVolume->pointUnderPixel(scr, found); // don't want any voxelscaling for this one
	      camera()->setRevolveAroundPoint(target);
	      QMessageBox::information(0, "", "Rotation pivot changed.\n\nTo reset back to scene center just Shift+Right click in empty region on screen - that is do not Shift+Right click on volume or any widget.\n\nRotation pivot change has no effect on keyframe animation.");
	    }
	  else
	    {
	      if (GeometryObjects::paths()->continuousAdd())
		GeometryObjects::paths()->addPoint(target);
	      else
		GeometryObjects::hitpoints()->add(target);
	      carveHitPointOK = false;
	    }
	}
    }
  else
    {
      if (!m_paintMode && event->buttons() == Qt::RightButton) // reset rotation pivot
	{
	  camera()->setRevolveAroundPoint(sceneCenter());
	  QMessageBox::information(0, "", "Rotation pivot reset to scene center");
	}
    }
}

Vec
Viewer::checkPointSelectedInViewport(int ic, QPoint screenPt, bool &found)
{
  Vec voxelScaling = Global::voxelScaling();

  //----------------
  int ow = camera()->screenWidth();
  int oh = camera()->screenHeight();
  //----------------

  //----------------
  ClipInformation clipInfo = GeometryObjects::clipplanes()->clipInfo();
  QList<Vec> clipNormal = GeometryObjects::clipplanes()->normals();
  QList<Vec> cxaxis = GeometryObjects::clipplanes()->xaxis();
  QList<Vec> cyaxis = GeometryObjects::clipplanes()->yaxis();
  QList<float> viewportScale = GeometryObjects::clipplanes()->viewportScale();
  QVector4D vp = GeometryObjects::clipplanes()->viewport()[ic];

  int vx, vy, vw, vh;
  vx = vp.x()*ow;
  vy = vp.y()*oh;
  vw = vp.z()*ow;
  vh = vp.w()*oh;


  // we get more accurate hit when considering only a single slice
  //if (clipInfo.thickness[ic] == 0)
    { 
      float camar = (float)ow/(float)oh;
      Vec cpos = VECPRODUCT(clipInfo.pos[ic], voxelScaling);
      float aspectRatio = vp.z()/vp.w(); 
      float cdist = 2*sceneRadius()/viewportScale[ic]; 
      float fov = camera()->fieldOfView(); 
      float yn = cdist*tan(fov*0.5); 
      float xn = yn*aspectRatio*camar; 
      float xfrc = (float)(screenPt.x()-vx)/(float)vw;
      float yfrc = (float)(screenPt.y()-(oh-vh-vy))/(float)vh;
      xfrc = 2*xfrc - 1;
      yfrc = 1 - 2*yfrc;
      Vec target = cpos + xfrc*xn*cxaxis[ic] + yfrc*yn*cyaxis[ic];
      found = true;
      return target;
    }
//  else // otherwise we have to interrogate depth buffer to get the point
//    {
//      //----------------
//      Camera clipCam;
//      clipCam = *camera();
//      clipCam.setOrientation(clipInfo.rot[ic]);	  
//      Vec cpos = VECPRODUCT(clipInfo.pos[ic], voxelScaling);
//      cpos = cpos -
//	clipCam.viewDirection()*sceneRadius()*2*(1.0/viewportScale[ic]);
//      clipCam.setPosition(cpos);
//      clipCam.setScreenWidthAndHeight(vw, vh);
//      clipCam.loadProjectionMatrix(true);
//      clipCam.loadModelViewMatrix(true);
//      //----------------
//      
//      float depth;
//      glReadPixels(screenPt.x(), oh-1-screenPt.y(),
//		   1, 1,
//		   GL_DEPTH_COMPONENT, GL_FLOAT,
//		   &depth);
//      found = depth < 1.0;
//      
//      if (depth < 1.0)
//	{     
//	  Vec pt = Vec((screenPt.x()-vx), // scale coordinates accordingly
//		       (screenPt.y()-(oh-vh-vy)), // scale coordinates accordingly
//		       depth);
//	  
//	  Vec target = clipCam.unprojectedCoordinatesOf(pt);
//      
//	  return target;
//	}
//      else
//	return Vec(-1000000, -1000000,-1000000);
//    }
}

Viewer::~Viewer()
{
  delete [] m_lut;
  delete [] m_prevLut;
  delete m_messageDisplayer;

  if (m_lutTex)
    glDeleteTextures(1, &m_lutTex);

  if (m_paintTex)
    glDeleteTextures(1, &m_paintTex);

  if (m_backBufferImage)
    delete [] m_backBufferImage;
}

QImage Viewer::histogramImage1D()
{
  if (m_lowresVolume->raised() == 0)
    return m_lowresVolume->histogramImage1D(); 
  else
    return m_hiresVolume->histogramImage1D(); 
}

QImage Viewer::histogramImage2D()
{
  if (m_lowresVolume->raised() == 0)
    return m_lowresVolume->histogramImage2D(); 
  else
    return m_hiresVolume->histogramImage2D(); 
}

void
Viewer::GlewInit()
{
  GlewInit::initialise();

  Global::setUseFBO(QGLFramebufferObject::hasOpenGLFramebufferObjects());
						
  createBlurShader();
  createCopyShader();

  if (GlewInit::initialised())
    MainWindowUI::mainWindowUI()->statusBar->showMessage("Ready");
  else
    MainWindowUI::mainWindowUI()->statusBar->showMessage("Error : Cannot Initialize Renderer");

  update();

  if (format().stereo())
    setStereoDisplay(true);


  
  // create shaders
  ShaderFactory::ptShader();
  ShaderFactory::pnShader();
}

void
Viewer::createBlurShader()
{
  QString shaderString;
  shaderString = ShaderFactory::genRectBlurShaderString(Global::viewerFilter());      
  m_blurShader = glCreateProgramObjectARB();
  if (! ShaderFactory::loadShader(m_blurShader,
				  shaderString))
    exit(0);
  m_blurParm[0] = glGetUniformLocationARB(m_blurShader, "blurTex");
}

void
Viewer::createCopyShader()
{
  QString shaderString;
  shaderString = ShaderFactory::genCopyShaderString();
  m_copyShader = glCreateProgramObjectARB();
  if (! ShaderFactory::loadShader(m_copyShader,
				  shaderString))
    exit(0);
  m_copyParm[0] = glGetUniformLocationARB(m_copyShader, "shadowTex");
}

void
Viewer::resetLookupTable()
{
  if (m_lut) delete [] m_lut;
  if (m_prevLut) delete [] m_prevLut;

  m_lut = new unsigned char[Global::lutSize()*256*256*4];
  m_prevLut = new unsigned char[Global::lutSize()*256*256*4];

  memset(m_lut, 0, Global::lutSize()*256*256*4);
  memset(m_prevLut, 0, Global::lutSize()*256*256*4);

  updateLookupTable();
  update();
}

void
Viewer::loadLookupTable(QList<QImage> image)
{
  memset(m_lut, 0, Global::lutSize()*256*256*4);

  int imgSize = 256*256*4;
  int imgCount = qMin(Global::lutSize(), image.count());
  for(int img=0; img<imgCount; img++)
    {
      QImage lutImage = image[img].mirrored(false, true);
      unsigned char *ibits = lutImage.bits();
      for (int i=0; i<256*256; i++)
	{
	  uchar r,g,b,a;
	  r = ibits[4*i+2];
	  g = ibits[4*i+1];
	  b = ibits[4*i+0];
	  a = ibits[4*i+3];

	  m_lut[img*imgSize + 4*i+0] = r;
	  m_lut[img*imgSize + 4*i+1] = g;
	  m_lut[img*imgSize + 4*i+2] = b;
	  m_lut[img*imgSize + 4*i+3] = a;
	}
    }
  updateLookupTable();


  update();
}

void
Viewer::loadLookupTable(unsigned char *lut)
{
  glActiveTexture(GL_TEXTURE0);
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
	       GL_RGBA,
	       GL_UNSIGNED_BYTE,
	       lut);

  //LightHandler::setLutTex(m_lutTex);
}

void
Viewer::updateLookupTable(unsigned char *kflut)
{
  memcpy(m_lut, kflut, Global::lutSize()*256*256*4);

  updateLookupTable();
}

void
Viewer::updateLookupTable()
{
  //--------------
  // check whether emptyspaceskip data structure
  // needs to be reevaluated
  bool prune = false;
  if (Global::volumeType() != Global::RGBVolume &&
      Global::volumeType() != Global::RGBAVolume)
    {
      for (int i=0; i<Global::lutSize()*256*256; i++)
	{
	  bool pl = m_prevLut[4*i+3] > 0;
	  bool ml = m_lut[4*i+3] > 0;
	  if (pl != ml)
	    {
	      prune = true;
	      break;
	    }
	}
    }
  //--------------
  
  //--------------
  // check whether light data structure
  // needs to be reevaluated
  bool gilite = false;
  for (int i=0; i<Global::lutSize()*256*256; i++)
    {
      if (fabs((float)(m_prevLut[4*i+0] - m_lut[4*i+0])) > 2 ||
	  fabs((float)(m_prevLut[4*i+1] - m_lut[4*i+1])) > 2 ||
	  fabs((float)(m_prevLut[4*i+2] - m_lut[4*i+2])) > 2 ||
	  fabs((float)(m_prevLut[4*i+3] - m_lut[4*i+3])) > 1)
	{
	  gilite = true;
	  break;
	}
    }
  memcpy(m_prevLut, m_lut, Global::lutSize()*4*256*256);
  //--------------

  unsigned char *lut;
  lut = new unsigned char[Global::lutSize()*256*256*4];
  memset(lut, 0, Global::lutSize()*256*256*4);      

  if (Global::volumeType() != Global::RGBVolume &&
      Global::volumeType() != Global::RGBAVolume)
    {
      float frc = Global::stepsizeStill();
      frc = qMax(0.2f, frc);

      for (int i=0; i<Global::lutSize()*256*256; i++)
	{      
	  qreal r,g,b,a;
	  
	  r = (float)m_lut[4*i+0]/255.0f;
	  g = (float)m_lut[4*i+1]/255.0f;
	  b = (float)m_lut[4*i+2]/255.0f;
	  a = (float)m_lut[4*i+3]/255.0f;	  
	  
	  r*=a; g*=a; b*=a;
	  r = 1-pow((float)(1-r), (float)frc);
	  g = 1-pow((float)(1-g), (float)frc);
	  b = 1-pow((float)(1-b), (float)frc);
	  a = 1-pow((float)(1-a), (float)frc);	  
	  
	  lut[4*i+0] = 255*r;
	  lut[4*i+1] = 255*g;
	  lut[4*i+2] = 255*b;
	  lut[4*i+3] = 255*a;
	}
    }
  else 
    memcpy(lut, m_lut, Global::lutSize()*4*256*256);

  if (Global::emptySpaceSkip() &&
      m_hiresVolume->raised() &&
      prune)
    {
      m_updatePruneBuffer = true;
    }

  loadLookupTable(lut);
  
  if (m_hiresVolume->raised() &&
      (gilite || LightHandler::willUpdateLightBuffers()))
    {
      m_hiresVolume->updateAndLoadPruneTexture();
      LightHandler::setLut(m_lut);
      m_hiresVolume->initShadowBuffers(true);
      bool fboBound = bindFBOs(Enums::StillImage);
      releaseFBOs(Enums::StillImage);
    }


  delete [] lut;
}

void
Viewer::enableTextureUnits()
{
  if (m_paintTex)
    {
      glActiveTexture(GL_TEXTURE5);
      glBindTexture(GL_TEXTURE_1D, m_paintTex);
      glEnable(GL_TEXTURE_1D);
    }

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, m_lutTex);
  glEnable(GL_TEXTURE_2D);
}

void
Viewer::disableTextureUnits()
{
  glActiveTexture(GL_TEXTURE5);
  glDisable(GL_TEXTURE_1D);

  glActiveTexture(GL_TEXTURE0);
  glDisable(GL_TEXTURE_2D);
}

void
Viewer::displayMessage(QString mesg, bool warn)
{  
  m_messageDisplayer->holdMessage(mesg, warn);
}
 
// variables used only for splashscreen
QStringList ssmesg;
int ssdmn = 0;
void
Viewer::splashScreen()
{
  if (GlewInit::initialised())
    {
      if (ssmesg.count() == 0)
	{
	  ssmesg << "When data is loaded, press spacebar to bring up command dialog";
	  ssmesg << "While hovering over geometry, press spacebar to bring up related parameter dialog.";
	  ssmesg << "Within panels, press Ctrl+H to bring up related help menu.";
	  ssmesg << "When data is loaded, you usually start in lowres mode.";
	  ssmesg << "Press F2 (or Fn+F2) to toggle between lowres and hires modes.";
	  ssmesg << "Press 1 to toggle shadow rendering.";
	  ssmesg << "To get (non transparent) black background switch on \"Backplane\" under \"Shader Widget\"";
	}
      
      MainWindowUI::mainWindowUI()->statusBar->showMessage(ssmesg[ssdmn]);
      ssdmn = (ssdmn+1)%7;
    }
  
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
    StaticFunctions::renderText(px+520, height()-mimg.height()-py-100,
				minorVer,
				tfont,
				Qt::transparent, Qt::white,
				false); // textPath
  }

  {
    tfont.setPointSize(70*fscl);  
    StaticFunctions::renderText(px-60, height()-mimg.height()-py+2,
				QString("Drishti v"+majorVer),
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
  if (m_lowresVolume->raised())
    return false;



  bool fboBound = false;

  if (imagequality == Enums::DragImage)
    {
      if (m_lowresBuffer->bind())
	{
	  fboBound = true;
	  glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);

	  glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	  glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	  
	  camera()->setScreenWidthAndHeight(m_lowresBuffer->width(),
					    m_lowresBuffer->height());
	  camera()->loadProjectionMatrix(true);
	  camera()->loadModelViewMatrix(true);
	  glViewport(0,0,
		     m_lowresBuffer->width(),
		     m_lowresBuffer->height());
	}
    }
//  else if (drawToFBO() ||
//	   Global::imageQuality() != Global::_NormalQuality)
  else if (!format().stereo())
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
	  	  
	  m_hiresVolume->initShadowBuffers(forceInitShadowBuffers);
	}
    }

  if (!fboBound)
    {
      if (Global::imageQuality() != Global::_NormalQuality)
	{
	  Global::setImageQuality(Global::_NormalQuality);
	  MainWindowUI::mainWindowUI()->actionNormal->setChecked(true);
	  MainWindowUI::mainWindowUI()->actionLow->setChecked(false);
	  MainWindowUI::mainWindowUI()->actionVeryLow->setChecked(false);
	}
      return false;
    }

  return true;
}

void
Viewer::releaseFBOs(int imagequality)
{
  if (m_lowresVolume->raised())
    return;

  if (!m_lowresBuffer->isBound() &&
      !m_imageBuffer->isBound())
    return;

  bool imgBound = m_imageBuffer->isBound();
  bool lowBound = m_lowresBuffer->isBound();

  if (imgBound)
    m_imageBuffer->release();

  if (lowBound)
    m_lowresBuffer->release();

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

  //if (imagequality == Enums::DragImage)
  if (lowBound)
    {
      glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_lowresBuffer->texture());
      wd = m_lowresBuffer->width();
      ht = m_lowresBuffer->height();
    }
  else
    {
      glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_imageBuffer->texture());
      wd = m_imageBuffer->width();
      ht = m_imageBuffer->height();
    }

  glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  if (wd < ow || ht  < oh)
    {
      glUseProgramObjectARB(m_blurShader);
      glUniform1iARB(m_blurParm[0], 0); // blur image from lowresBuffer into frameBuffer
    }
  else
    {
      glUseProgramObjectARB(m_copyShader);
      glUniform1iARB(m_copyParm[0], 0); // copy image from imageBuffer into frameBuffer
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
Viewer::drawInfoString(int imagequality,
		       float stepsize)
{
  int posx = 10;
  int posy = size().height()-1;
  int screenWidth = size().width();
  int screenHeight = size().height();

  startScreenCoordinatesSystem();

  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // blend on top

  if (Global::bottomText())
    {
      glColor4f(0, 0, 0, 0.8f);
      glBegin(GL_QUADS);
      glVertex2f(0, posy-55);
      glVertex2f(0, screenHeight);
      glVertex2f(screenWidth, screenHeight);
      glVertex2f(screenWidth, posy-55);
      glEnd();
    }

  QFont tfont = QFont("Helvetica", 8);
  tfont.setStyleStrategy(QFont::PreferAntialias);  

  Vec dataMin = m_hiresVolume->volumeMin();
  Vec dataMax = m_hiresVolume->volumeMax();
  int minX, maxX, minY, maxY, minZ, maxZ;
  minX = dataMin.x;
  minY = dataMin.y;
  minZ = dataMin.z;
  maxX = dataMax.x;
  maxY = dataMax.y;
  maxZ = dataMax.z;

  int gimgq = Global::imageQuality();
  int dragvolume = (Global::loadDragOnly() ||
		    Global::useDragVolume() ||
		    imagequality == Enums::DragImage);

  //-------------
  // calculate font scale based on dpi
  float fscl = 120.0/Global::dpi();
  //-------------

  QString msg;

  if (!Global::updatePruneTexture())
    msg = QString("(update:off) ");

  if (PruneHandler::carve())
    {
      float r,d;
      PruneHandler::carveRad(r,d);
      msg += QString("(carve : %1 %2)").arg(r).arg(d);
    }
  if (PruneHandler::paint())
    {
      float r,d;
      PruneHandler::carveRad(r,d);
      int t = PruneHandler::tag();
      msg += QString("(paint : %1 %2)").arg(r).arg(t);
    }
  if (PruneHandler::blend())
    msg += QString("(blend : on)");

  if (m_disableRotationInViewport &&
      GeometryObjects::clipplanes()->viewportGrabbed()>=0)
    msg += " DRV";

  if (!msg.isEmpty())
    {
      msg = "mop "+msg;
      tfont.setPointSize(12*fscl);
      StaticFunctions::renderText(5, 40, msg, tfont, Qt::black, Qt::white);
    }

  if (!Global::bottomText())
    {
      stopScreenCoordinatesSystem();
      return;
    }

  

  // ---------------------------------
  // show current files at the top
  QString flnms;
  int nvol = 0;
  if (Global::volumeType() == Global::SingleVolume) nvol = 1;
  if (Global::volumeType() == Global::DoubleVolume) nvol = 2;
  if (Global::volumeType() == Global::TripleVolume) nvol = 3;
  if (Global::volumeType() == Global::QuadVolume) nvol = 4;  
  if (nvol > 0)
    {
      for (int nv=0; nv<nvol; nv++)
	{
	  VolumeInformation volinfo = m_Volume->volInfo(Global::volumeNumber(nv), nv);
	  QString flnm = volinfo.pvlFile;
	  flnms += QString("%1  ").arg(QFileInfo(flnm).fileName());
	}
      tfont.setPointSize(10*fscl);
      StaticFunctions::renderText(5, 20, flnms, tfont, QColor(0,0,0,64), Qt::white);
    }
  // ---------------------------------
  
  
  
  
  if (gimgq == Global::_NormalQuality) msg = "normal";
  else if (gimgq == Global::_LowQuality) msg = "low";
  else msg = "very low";

  if (dragvolume) msg += "+drag";
  else if (m_hiresVolume->renderQuality()) msg += "+ss";
  else msg += "+default";

  if (Global::emptySpaceSkip()) msg += "+skip";
  if (Global::useDragVolumeforShadows()) msg += "+dshadows";

  tfont.setPointSize(8*fscl);

  StaticFunctions::renderText(posx, posy-24*fscl,
			      QString("HiRes : image (%1) : step(%2)"). \
			      arg(msg).arg(stepsize),
			      tfont,
			      Qt::transparent,
			      Qt::lightGray);


  tfont.setPointSize(10*fscl);

  StaticFunctions::renderText(posx, posy-13*fscl,
			      QString("Bounds : %1-%2, %3-%4, %5-%6").	\
			      arg(minX).arg(maxX).arg(minY).		\
			      arg(maxY).arg(minZ).arg(maxZ),
			      tfont,
			      Qt::transparent,
			      Qt::lightGray);

  tfont.setPointSize(12*fscl);
  glColor3f(1,1,1);

  int lod, textureX, textureY, ntex;
  if (!dragvolume)
    {
      m_hiresVolume->getSliceTextureSize(textureX, textureY);
      lod = m_hiresVolume->getSubvolumeSubsamplingLevel();
      ntex = m_hiresVolume->numOfTextureSlabs();
    }
  else
    {
      m_hiresVolume->getDragTextureSize(textureX, textureY);
      lod = m_hiresVolume->getDragSubsamplingLevel();
      ntex = 1;
    }

  QString lodMesg;
  if (lod == 1)
    lodMesg = QString("LoD(%1) : Size : %2x%3x%4").			\
	   arg(lod).							\
	   arg(maxX-minX+1).						\
	   arg(maxY-minY+1).						\
           arg(maxZ-minZ+1);
  else
    lodMesg = QString("LoD(%1) : Size : %2x%3x%4 (%5x%6x%7)").		\
	   arg(lod).							\
	   arg(maxX-minX+1).						\
	   arg(maxY-minY+1).						\
	   arg(maxZ-minZ+1).						\
	   arg((maxX-minX+1)/lod).					\
	   arg((maxY-minY+1)/lod).					\
           arg((maxZ-minZ+1)/lod);


  StaticFunctions::renderText(posx, posy,
			      lodMesg, tfont,
			      Qt::transparent, Qt::white);

  stopScreenCoordinatesSystem();
}

void
Viewer::drawCarveCircleInViewport(int ic)
{
  QList<float> viewportScale = GeometryObjects::clipplanes()->viewportScale();
  QVector4D vp = GeometryObjects::clipplanes()->viewport()[ic];
  int ow = camera()->screenWidth();
  int oh = camera()->screenHeight();
  int vx, vy, vw, vh;
  vx = vp.x()*ow;
  vy = vp.y()*oh;
  vw = vp.z()*ow;
  vh = vp.w()*oh;

  float camar = (float)ow/(float)oh;
  float aspectRatio = vp.z()/vp.w(); 
  float cdist = 2*sceneRadius()/viewportScale[ic]; 
  float fov = camera()->fieldOfView(); 
  float yn = cdist*tan(fov*0.5); 
  float xn = yn*aspectRatio*camar; 
  float p = 0.5*vw/xn;
  float q = 0.5*vh/yn;

  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, Global::hollowSpriteTexture());
  glDisable(GL_DEPTH_TEST);

  float lod = m_hiresVolume->getDragSubsamplingLevel();
  float r,d;
  PruneHandler::carveRad(r,d);
  r *= lod;
  d *= lod;

  if (PruneHandler::carve())
    glColor4f(0.0, 0.9, 0.6, 0.9);
  else // paint
    {
      int t = PruneHandler::tag();
      uchar *tc = Global::tagColors();
      if (t == 0)
	glColor4f(1.0,1.0,1.0,1.0);
      else
	glColor4f(tc[4*t+0]/255.0, tc[4*t+1]/255.0, tc[4*t+2]/255.0, tc[4*t+3]/255.0);
    }
  QPoint scr = mapFromGlobal(QCursor::pos());
  startScreenCoordinatesSystem();
  glBegin(GL_QUADS);
  glTexCoord2f(0, 0);      glVertex2f(scr.x() - r*p, scr.y() - r*q);
  glTexCoord2f(0, 1);      glVertex2f(scr.x() - r*p, scr.y() + r*q);
  glTexCoord2f(1, 1);      glVertex2f(scr.x() + r*p, scr.y() + r*q);
  glTexCoord2f(1, 0);      glVertex2f(scr.x() + r*p, scr.y() - r*q);
  glEnd();
  if (PruneHandler::carve())
    {
      glColor4f(0.0, 0.0, 0.0, 0.5);
      glBegin(GL_QUADS);
      glTexCoord2f(0, 0);      glVertex2f(scr.x() - d*p, scr.x() - d*q);
      glTexCoord2f(0, 1);      glVertex2f(scr.x() - d*p, scr.x() + d*q);
      glTexCoord2f(1, 1);      glVertex2f(scr.x() + d*p, scr.x() + d*q);
      glTexCoord2f(1, 0);      glVertex2f(scr.x() + d*p, scr.x() - d*q);
      glEnd();
    }
  stopScreenCoordinatesSystem();
  
  glEnable(GL_DEPTH_TEST);
  glActiveTexture(GL_TEXTURE0);
  glDisable(GL_TEXTURE_2D);  
}

void
Viewer::drawCarveCircle()
{
  QPoint scr = mapFromGlobal(QCursor::pos());
  int ow = camera()->screenWidth();
  int oh = camera()->screenHeight();
  int ic = GeometryObjects::clipplanes()->inViewport(scr.x(), scr.y(),
							 ow, oh);
  if (ic >= 0)
    {
      drawCarveCircleInViewport(ic);
      return;
    }

  Vec voxelScaling = Global::voxelScaling();
  Vec chp = VECPRODUCT(carveHitPoint, voxelScaling);

  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, Global::hollowSpriteTexture());
  glDisable(GL_DEPTH_TEST);

  Vec p = VECPRODUCT(camera()->upVector(), voxelScaling);
  Vec q = VECPRODUCT(camera()->rightVector(), voxelScaling);

  float lod = m_hiresVolume->getDragSubsamplingLevel();
  float r,d;
  PruneHandler::carveRad(r,d);
  r *= lod;
  d *= lod;
  if (PruneHandler::carve())
    glColor4f(0.0, 0.9, 0.6, 0.9);
  else // paint
    {
      int t = PruneHandler::tag();
      uchar *tc = Global::tagColors();
      glColor4f(tc[4*t+0]/255.0, tc[4*t+1]/255.0, tc[4*t+2]/255.0, tc[4*t+3]/255.0);
    }
  glBegin(GL_QUADS);
  glTexCoord2f(0, 0);      glVertex3dv(chp - r*p - r*q);
  glTexCoord2f(0, 1);      glVertex3dv(chp - r*p + r*q);
  glTexCoord2f(1, 1);      glVertex3dv(chp + r*p + r*q);
  glTexCoord2f(1, 0);      glVertex3dv(chp + r*p - r*q);
  glEnd();
  if (PruneHandler::carve())
    {
      glColor4f(0.0, 0.0, 0.0, 0.5);
      glBegin(GL_QUADS);
      glTexCoord2f(0, 0);      glVertex3dv(chp - d*p - d*q);
      glTexCoord2f(0, 1);      glVertex3dv(chp - d*p + d*q);
      glTexCoord2f(1, 1);      glVertex3dv(chp + d*p + d*q);
      glTexCoord2f(1, 0);      glVertex3dv(chp + d*p - d*q);
      glEnd();
    }
  
  glEnable(GL_DEPTH_TEST);
  glActiveTexture(GL_TEXTURE0);
  glDisable(GL_TEXTURE_2D);  
}

void
Viewer::drawInHires(int imagequality)
{
  if (imagequality == Enums::DragImage)
    m_hiresVolume->drawDragImage(Global::stepsizeDrag());
  else
    m_hiresVolume->drawStillImage(Global::stepsizeStill());
  

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // blend on top

  LightHandler::giLights()->postdraw(this);

  if (carveHitPointOK)
    drawCarveCircle();


  QList<Vec> renewHitPoints = GeometryObjects::hitpoints()->renewValues();
  if (renewHitPoints.count() > 0)
    {
      QList<QVariant> raw;
      QList<QVariant> tag;

      raw = RawVolume::rawValues(renewHitPoints);
      if (raw.count() == 0)
	raw = m_Volume->rawValues(renewHitPoints);

      tag = RawVolume::tagValues(renewHitPoints);
      GeometryObjects::hitpoints()->setRawTagValues(raw, tag);
    }

  GeometryObjects::landmarks()->postdraw(this);
  GeometryObjects::hitpoints()->postdraw(this);
  GeometryObjects::paths()->postdraw(this);
  GeometryObjects::grids()->postdraw(this);
  GeometryObjects::crops()->postdraw(this);
  GeometryObjects::pathgroups()->postdraw(this);
  GeometryObjects::trisets()->postdraw(this);
  GeometryObjects::networks()->postdraw(this);
  GeometryObjects::captions()->draw(this);
  GeometryObjects::imageCaptions()->postdraw(this);

  GeometryObjects::scalebars()->draw(this,
				     GeometryObjects::clipplanes()->clipInfo());


  if (GeometryObjects::colorbars()->isValid())
    {
      m_hiresVolume->runLutShader(true);
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, m_lutTex);
      glEnable(GL_TEXTURE_2D);
      GeometryObjects::colorbars()->draw(this);
      glActiveTexture(GL_TEXTURE0);
      glDisable(GL_TEXTURE_2D);
      m_hiresVolume->runLutShader(false);
    }
}

void
Viewer::renderVolume(int imagequality)
{

  bool fboBound = bindFBOs(imagequality);

  if ((m_mouseDrag || mouseGrabber()) &&
      !Global::useStillVolume())
    imagequality = Enums::DragImage;
    

  glClearDepth(1);
  glClear(GL_DEPTH_BUFFER_BIT);
  glDisable(GL_LIGHTING);
  glDepthFunc(GL_LESS);
  glEnable(GL_DEPTH_TEST);
  glShadeModel(GL_SMOOTH);
  glEnable(GL_MULTISAMPLE);

  if (m_lowresVolume->raised())
    {
      int ow = QGLViewer::size().width();
      int oh = QGLViewer::size().height();
      glViewport(0,0, ow, oh); 

      glClearColor(0,0,0,1);
      glClear(GL_COLOR_BUFFER_BIT);

      m_lowresVolume->draw(Global::stepsizeStill(),
			   10, size().height()-1);

    }
  else
    {
      glClear(GL_COLOR_BUFFER_BIT);
      camera()->setFocusDistance(m_focusDistance);
      
      if (! MainWindowUI::mainWindowUI()->actionRedBlue->isChecked() &&
	  ! MainWindowUI::mainWindowUI()->actionRedCyan->isChecked() &&
	  ! MainWindowUI::mainWindowUI()->actionCrosseye->isChecked() &&
	  ! MainWindowUI::mainWindowUI()->actionFor3DTV->isChecked())
	drawInHires(imagequality);
      else if (MainWindowUI::mainWindowUI()->actionCrosseye->isChecked() ||
	       MainWindowUI::mainWindowUI()->actionFor3DTV->isChecked())
	{
	  int sit = Global::saveImageType();
	  
	  int camW = camera()->screenWidth();
	  int camH = camera()->screenHeight();

	  if (MainWindowUI::mainWindowUI()->actionCrosseye->isChecked())
	    camera()->setScreenWidthAndHeight(camW/2,camH);
	  else
	    camera()->setScreenWidthAndHeight(camW,camH);
	  camera()->loadProjectionMatrixStereo(false);
	  camera()->loadModelViewMatrixStereo(false);
	  glViewport(0,0, camW/2, camH);
	  if (MainWindowUI::mainWindowUI()->actionCrosseye->isChecked())
	    Global::setSaveImageType(Global::RightImage);
	  else
	    Global::setSaveImageType(Global::LeftImage);
	  drawInHires(imagequality);


	  if (MainWindowUI::mainWindowUI()->actionCrosseye->isChecked())
	    camera()->setScreenWidthAndHeight(camW/2,camH);
	  else
	    camera()->setScreenWidthAndHeight(camW,camH);
	  camera()->loadProjectionMatrixStereo(true);
	  camera()->loadModelViewMatrixStereo(true);
	  glViewport(camW/2,0, camW/2, camH);
	  if (MainWindowUI::mainWindowUI()->actionCrosseye->isChecked())
	    Global::setSaveImageType(Global::LeftImage);
	  else
	    Global::setSaveImageType(Global::RightImage);
	  drawInHires(imagequality);

	  // restore camera parameters
	  camera()->setScreenWidthAndHeight(camW,camH);
	  camera()->loadProjectionMatrix(true);
	  camera()->loadModelViewMatrix(true);
	  glViewport(0,0, camW, camH);

	  Global::setSaveImageType(sit);
	}
      else if (MainWindowUI::mainWindowUI()->actionRedBlue->isChecked() ||
	       MainWindowUI::mainWindowUI()->actionRedCyan->isChecked())
	{
	  int sit = Global::saveImageType();

	  Global::setSaveImageType(Global::LeftImageAnaglyph);
	  drawInHires(imagequality);

	  Global::setSaveImageType(Global::RightImageAnaglyph);
	  drawInHires(imagequality);

	  Global::setSaveImageType(sit);
	  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	}
    }

  releaseFBOs(imagequality);

  if (m_hiresVolume->raised())
    {
      if (imagequality == Enums::DragImage)
	drawInfoString(imagequality,
		       Global::stepsizeDrag());
      else
	drawInfoString(imagequality,
		       Global::stepsizeStill());
    }
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
				   "Set Frame Rate",
				   "Frame Rate",
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
    QMessageBox::critical(0, "Movie",
			  "Failed to create writer");
    return false;
  }

  if (glmedia_movie_writer_start(m_movieWriterLeft,
				 movieFile.toLatin1().data(),
				 m_imageWidth,
				 m_imageHeight,
				 fps,
				 quality) < 0) {
    QMessageBox::critical(0, "Movie",
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
	QMessageBox::critical(0, "Movie",
			      "Failed to create writer");
	return false;
      }
      
      if (glmedia_movie_writer_start(m_movieWriterRight,
				     movieFile.toLatin1().data(),
				     m_imageWidth,
				     m_imageHeight,
				     fps,
				     quality) < 0) {
	QMessageBox::critical(0, "Movie",
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
      QMessageBox::critical(0, "Movie",
			       "Failed to end movie");
      return false;
    }
  glmedia_movie_writer_free(m_movieWriterLeft);

  if (m_imageMode != Enums::MonoImageMode)
    {
      if (glmedia_movie_writer_end(m_movieWriterRight) < 0)
	{
	  QMessageBox::critical(0, "Movie",
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

  if (m_lowresVolume && m_hiresVolume)
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
Viewer::updateLookFrom(Vec pos, Quaternion rot, float focusDistance, float es)
{
  camera()->setPosition(pos);
  camera()->setOrientation(rot);
  camera()->setIODistance(es);
  if (focusDistance > 0.1)
    {
      camera()->setFocusDistance(focusDistance);
      emit focusSetting(camera()->focusDistance());
    }
  m_focusDistance = camera()->focusDistance();
}

void
Viewer::currentView()
{
  // draw image again to get the correct pixmap
  draw();

  Vec pos;
  Quaternion rot;
  pos = camera()->position();
  rot = camera()->orientation();
	  
  QImage image = grabFrameBuffer();
  image = image.scaled(100, 100);

  emit setView(pos, rot,
	       image,
	       camera()->focusDistance());
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
  m_lowresVolume->subvolumeBounds(bmin, bmax);

  QImage image = grabFrameBuffer();
  image = image.scaled(100, 100);

  emit setKeyFrame(pos, rot, fno,
		   camera()->focusDistance(),
		   camera()->IODistance(),
		   m_lut,
		   image);
}

void
Viewer::captureKeyFrameImage(int kfn)
{
  // draw image again to get the correct pixmap
  draw();

  QImage image = grabFrameBuffer();
  image = image.scaled(100, 100);

  emit replaceKeyFrameImage(kfn, image);
}

void 
Viewer::fastDraw()
{
  if (!m_lowresVolume->raised() &&
      !m_hiresVolume->raised())
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
  
  if (m_lowresVolume && m_hiresVolume)
    {
      if (Global::useStillVolume() ||
	  Global::volumeType() == Global::DummyVolume)
	renderVolume(Enums::StillImage);
      else
	renderVolume(Enums::DragImage);
    }

  Global::setPlayFrames(false);

  grabBackBufferImage();
}

void Viewer::updatePruneBuffer(bool b)
{
  m_updatePruneBuffer = b;
}

void
Viewer::updateLightBuffers()
{
  if (Global::volumeType() == Global::DummyVolume)
    return;

  QList<Vec> cpos, cnorm;
  m_hiresVolume->getClipForMask(cpos, cnorm);

  bool redolighting = LightHandler::checkClips(cpos, cnorm);
  redolighting = redolighting || LightHandler::checkCrops();
  redolighting = redolighting || LightHandler::updateOnlyLightBuffers();
  redolighting = redolighting || !m_updatePruneBuffer;

  
  // update prune buffer
  if (m_updatePruneBuffer)
    {
      m_updatePruneBuffer = false;
      m_hiresVolume->updateAndLoadPruneTexture();
    }

  LightHandler::generateOpacityTexture();
  
  //QList<Vec> cpos, cnorm;
  //m_hiresVolume->getClipForMask(cpos, cnorm);
  //
  //bool redolighting = LightHandler::checkClips(cpos, cnorm);
  //redolighting = redolighting || LightHandler::checkCrops();
  //redolighting = redolighting || LightHandler::updateOnlyLightBuffers();
  //redolighting = redolighting || !m_updatePruneBuffer;

  if (redolighting)
    {
      LightHandler::updateLightBuffers();
      m_hiresVolume->initShadowBuffers(true);
      bool fboBound = bindFBOs(Enums::StillImage);
      if (fboBound) releaseFBOs(Enums::StillImage);
    }
}

void 
Viewer::dummydraw()
{
  bool fboBound = bindFBOs(Enums::StillImage);
  m_hiresVolume->drawDragImage(100*Global::stepsizeStill());
  if (fboBound) releaseFBOs(Enums::StillImage);
}

void 
Viewer::draw()
{
  if (!m_lowresVolume->raised() &&
      !m_hiresVolume->raised())
    {
      splashScreen();
      return;
    }

  if (m_lowresVolume->raised())
    {
      fastDraw();
      return;
    }

  if (!Global::updateViewer())
    {
      if (!Global::playFrames())
	showBackBufferImage();
      return;
    }

  //-----------------
  // update lightbuffer
  if (Global::volumeType() != Global::DummyVolume)
    {
      // update prune buffer
      if (m_updatePruneBuffer)
	{
	  m_updatePruneBuffer = false;
	  m_hiresVolume->updateAndLoadPruneTexture();
	}
      
      QList<Vec> cpos, cnorm;
      m_hiresVolume->getClipForMask(cpos, cnorm);
      //bool redolighting = LightHandler::checkClips(cpos, cnorm);
      bool redolighting = false;
      if (!m_mouseDrag)
	{
	  redolighting = redolighting || LightHandler::checkClips(cpos, cnorm);
	  redolighting = redolighting || LightHandler::checkCrops();
	}
      redolighting = redolighting || LightHandler::updateOnlyLightBuffers();
      if (redolighting)
	{
	  LightHandler::updateLightBuffers();
	  m_hiresVolume->initShadowBuffers(true);
	  if (!(m_saveSnapshots || m_saveMovie || Global::playFrames()))
	    dummydraw();
	}
    }
  //-----------------
  
//  if (m_saveSnapshots || m_saveMovie || Global::playFrames())
//    dummydraw();

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

  if (m_hiresVolume->raised())
    {
      QPoint scr = mapFromGlobal(QCursor::pos());
      int ow = camera()->screenWidth();
      int oh = camera()->screenHeight();
      int ic = GeometryObjects::clipplanes()->inViewport(scr.x(), scr.y(),
							 ow, oh);
      if (ic >= 0)
	{
	  int mag = event->delta()/8.0f/15.0f;
	  if (event->modifiers() & Qt::ShiftModifier)
	    {
	      GeometryObjects::clipplanes()->modThickness(ic, mag);
	    }
	  else
	    {
	      Vec cnorm = GeometryObjects::clipplanes()->normals()[ic];
	      if (event->modifiers() & Qt::ControlModifier)
		mag *= 10;
	      GeometryObjects::clipplanes()->translate(ic, mag*cnorm);
	    }
	  updateGL();
	  return;
	}
      else
	{
	  int ip = GeometryObjects::paths()->inViewport(scr.x(), scr.y(),
							ow, oh);
	  if (ip >= 0) // in path viewport
	    {
	      bool flag = false;
	      Vec voxelScaling = VolumeInformation::volumeInformation().voxelSize;
	      int mag = event->delta()/8.0f/15.0f;
	      if (event->modifiers() & Qt::ShiftModifier)
		{
		  GeometryObjects::paths()->modThickness(true, // modulate thickness
							 ip, mag,
							 scr, voxelScaling,
							 ow, oh);
		}
	      else if (!(event->modifiers() & Qt::AltModifier))
		{
		  if (event->modifiers() & Qt::ControlModifier ||
		      event->modifiers() & Qt::MetaModifier)
		    mag *= 10;
		  GeometryObjects::paths()->translate(ip, mag, 1,
						      scr, voxelScaling,
						      ow, oh);
		}
	      else
		{
		  GeometryObjects::paths()->rotate(ip, mag,
						   scr, voxelScaling,
						   ow, oh);
		}
	      updateGL();
	      return;
	    }
	}

    }

  QGLViewer::wheelEvent(event);

  m_undo.append(camera()->position(), camera()->orientation());
}


int mouseButtonPressed = 0;

Vec
Viewer::setViewportCamera(int ic, Camera& clipCam)
{
  Vec voxelScaling = Global::voxelScaling();
  ClipInformation clipInfo = GeometryObjects::clipplanes()->clipInfo();
  QList<float> viewportScale = GeometryObjects::clipplanes()->viewportScale();
  int vx, vy, vh, vw;
  QVector4D vp = clipInfo.viewport[ic];
  clipCam.setOrientation(clipInfo.rot[ic]);	  
  Vec cpos = VECPRODUCT(clipInfo.pos[ic], voxelScaling);
  cpos = cpos -
    clipCam.viewDirection()*sceneRadius()*2*(1.0/viewportScale[ic]);
  clipCam.setPosition(cpos);
  vx = vp.x()*camera()->screenWidth();
  vy = vp.y()*camera()->screenHeight();
  vw = vp.z()*camera()->screenWidth();
  vh = vp.w()*camera()->screenHeight();
  clipCam.setScreenWidthAndHeight(vw, vh);
  clipCam.loadProjectionMatrix(true);
  clipCam.loadModelViewMatrix(true);
  glViewport(vx, vy, vw, vh);
  return Vec(vx, vy, vh);
}

bool
Viewer::mousePressEventInViewport(int ic, QMouseEvent *event)
{
  // do not check for grab if Shift is pressed
  if (event->modifiers() & Qt::ShiftModifier)
    return false;

  GLint ovp[4];
  camera()->getViewport(ovp);

  int oh = camera()->screenHeight();

  Camera clipCam;
  clipCam = *camera();
  Vec cp = setViewportCamera(ic, clipCam);

  QPoint scr = event->pos();
  int sx = scr.x()-cp.x;
  int sy = cp.y+cp.z+scr.y()-oh;
  MouseGrabber *mg = GeometryObjects::checkIfGrabsMouse(sx, sy, &clipCam);
  if (mg)
    {
      // send modified cursor position for mouse events in viewport
      QMouseEvent me = QMouseEvent(QEvent::MouseButtonPress,
				   QPoint(sx, sy),
				   event->button(),
				   event->buttons(),
				   event->modifiers());
      if (GeometryObjects::mouseGrabberType() == GeometryObjects::PATH)
	((PathGrabber*)mg)->mousePressEvent(&me, &clipCam);
      updateGL();
      glViewport(ovp[0], ovp[1], ovp[2], ovp[3]);
      return true;
    }
  glViewport(ovp[0], ovp[1], ovp[2], ovp[3]);
  return false;
}

bool
Viewer::mouseMoveEventInViewport(int ic, QMouseEvent *event)
{
  // do not check for grab if Shift is pressed
  if (event->modifiers() & Qt::ShiftModifier)
    return false;

  GLint ovp[4];
  camera()->getViewport(ovp);

  int oh = camera()->screenHeight();

  Camera clipCam;
  clipCam = *camera();
  Vec cp = setViewportCamera(ic, clipCam);

  QPoint scr = event->pos();
  int sx = scr.x()-cp.x;
  int sy = cp.y+cp.z+scr.y()-oh;
  MouseGrabber *mg = GeometryObjects::checkIfGrabsMouse(sx, sy, &clipCam);

  if (mg)
    {
      // send modified cursor position for mouse events in viewport
      QMouseEvent me = QMouseEvent(QEvent::MouseMove,
				   QPoint(sx, sy),
				   event->button(),
				   event->buttons(),
				   event->modifiers());
      setMouseGrabber(mg);
      if (GeometryObjects::mouseGrabberType() == GeometryObjects::HITPOINT)
	((HitPointGrabber*)mg)->mouseMoveEvent(&me, &clipCam);
      else if (GeometryObjects::mouseGrabberType() == GeometryObjects::PATH)
	((PathGrabber*)mg)->mouseMoveEvent(&me, &clipCam);
      
      glViewport(ovp[0], ovp[1], ovp[2], ovp[3]);
      updateGL();
      return true;
    }
  glViewport(ovp[0], ovp[1], ovp[2], ovp[3]);

  // don't pass the event any further if Shift is pressed
  // and either of carve or paint are active
  if (event->modifiers() & Qt::ShiftModifier)
    {
      if (!PruneHandler::carve() && !PruneHandler::paint())
	return true;
      else
	return false;
    }
  else
    return false;
}

bool
Viewer::mouseMoveEventInPathViewport(int ip, QMouseEvent *event)
{
  // do not proceed for vertical style
  if ((GeometryObjects::paths()->paths())[ip].viewportStyle() == false)
    return true;

  Vec voxelScaling = VolumeInformation::volumeInformation().voxelSize;
  int ow = camera()->screenWidth();
  int oh = camera()->screenHeight();
  QPoint scr = event->pos();
  int mag = scr.y() - m_mousePrevPos.y();
  if (event->modifiers() & Qt::ControlModifier ||
      event->modifiers() & Qt::MetaModifier)
    mag *= 10;
  
  if (event->modifiers() & Qt::ShiftModifier)
    {
      GeometryObjects::paths()->modThickness(false, // modulate height
					     ip, mag,
					     scr, voxelScaling,
					     ow, oh);
    }
  else if (!(event->modifiers() & Qt::AltModifier))
    {
      int magx = (scr.x() - m_mousePrevPos.x());
//  do not magnify sideways movement
//      if (event->modifiers() & Qt::ControlModifier ||
//	  event->modifiers() & Qt::MetaModifier)
//	magx *= 10;
      if (abs(mag) > abs(magx))
	GeometryObjects::paths()->translate(ip, mag, 0,
					    scr, voxelScaling,
					    ow, oh);
      else
	GeometryObjects::paths()->translate(ip, magx, 2,
					    scr, voxelScaling,
					    ow, oh);
    }
  
  else
    GeometryObjects::paths()->rotate(ip, mag,
				     scr, voxelScaling,
				     ow, oh);
  
  m_mousePrevPos = event->pos();
  updateGL();

  return true;
}

void
Viewer::mousePressEvent(QMouseEvent *event)
{
  m_mouseDrag = true;
  m_mousePressPos = event->pos();
  m_mousePrevPos = event->pos();

  if (!Global::updateViewer())
    {
      updateGL();
      return;
    }

  if (m_hiresVolume->raised())
    {
      if (event->modifiers() & Qt::ShiftModifier)
	mouseButtonPressed = event->button();

      int ow = camera()->screenWidth();
      int oh = camera()->screenHeight();
      int ic = GeometryObjects::clipplanes()->inViewport(m_mousePressPos.x(), m_mousePressPos.y(),
							 ow, oh);
      if (ic >= 0)
	{
	  GeometryObjects::clipplanes()->setViewportGrabbed(ic, true);
	  if (mousePressEventInViewport(ic, event))
	    return;
	}

      int ip = GeometryObjects::paths()->inViewport(m_mousePressPos.x(),
						    m_mousePressPos.y(),
						    ow, oh);
      if (ip >= 0)
	{
	  GeometryObjects::paths()->setViewportGrabbed(ip, true);
	  return;
	}
    }

  QGLViewer::mousePressEvent(event);
}

void
Viewer::mouseMoveEvent(QMouseEvent *event)
{
  carveHitPointOK = false;

  if (!Global::updateViewer())
    {
      updateGL();
      m_mousePrevPos = event->pos();
      return;
    }

  if (m_lowresVolume->raised())
    {
      QList<MouseGrabber*> mousegrabbers = MouseGrabber::MouseGrabberPool();
      
      for(int i=0; i<mousegrabbers.count(); i++)
	mousegrabbers[i]->removeFromMouseGrabberPool();

      m_lowresVolume->activateBounds();

      QGLViewer::mouseMoveEvent(event);
      
      for(int i=0; i<mousegrabbers.count(); i++)
	mousegrabbers[i]->addInMouseGrabberPool();

      return;
    }

  int ow = camera()->screenWidth();
  int oh = camera()->screenHeight();
  bool found = false;
  Vec target;
  QPoint scr = event->pos();
  int ic = GeometryObjects::clipplanes()->inViewport(scr.x(),
						     scr.y(),
						     ow, oh);

  if (ic >= 0)
    {
      if (mouseMoveEventInViewport(ic, event))
	return;
    }
  else if (event->buttons())
    {
      int ip = GeometryObjects::paths()->viewportGrabbed();
      if (ip >= 0) // in path viewport
	{
	  if (mouseMoveEventInPathViewport(ip, event))
	    return;
	}
    }

  if (event->buttons() == Qt::NoButton)
    GeometryObjects::trisets()->checkMouseHover(this);
  

  if (GeometryObjects::grabsMouse())
    {
      QGLViewer::mouseMoveEvent(event);
      return;
    }
  
  bool rst = true;
  if (PruneHandler::carve() || PruneHandler::paint())
    {
      if (ic >= 0) // point selected is in a viewport
	{
	  target = checkPointSelectedInViewport(ic, scr, found);

	  if (mouseButtonPressed)
	    rst = false;
	}
      else
	{
	  if (m_mouseDrag && !mouseButtonPressed)
	    {
	      QGLViewer::mouseMoveEvent(event);
	      m_mousePrevPos = event->pos();
	      return;
	    }
	  
	  target = m_hiresVolume->pointUnderPixel(scr, found);
	}
    }

  if (rst || (!found && !GeometryObjects::grabsMouse()))
    {
      //int ic = GeometryObjects::clipplanes()->viewportGrabbed();
      if (ic >= 0) // viewport grabbed
	{
	  if (m_mouseDrag && !GeometryObjects::grabsMouse())
	    {
	      float mx = event->pos().x()-m_mousePrevPos.x();
	      float my = event->pos().y()-m_mousePrevPos.y();
	      Vec cnorm = GeometryObjects::clipplanes()->normals()[ic];
	      Vec cxaxis = GeometryObjects::clipplanes()->xaxis()[ic];
	      Vec cyaxis = GeometryObjects::clipplanes()->yaxis()[ic];
	      if (event->buttons() == Qt::RightButton) // translate
		{
		  Vec trans = -mx*cxaxis + my*cyaxis;
		  if (event->modifiers() & Qt::ControlModifier)
		    { // translate along blue axis
		      if (qAbs(mx) > qAbs(my))
			trans = mx*cnorm;
		      else
			trans = my*cnorm;
		    }
		  GeometryObjects::clipplanes()->translate(ic, trans);
		}
	      else if (event->buttons() == Qt::MiddleButton) // scale
		{
		  int scl = my;
		  if (qAbs(mx) > qAbs(my)) scl = mx;
		  GeometryObjects::clipplanes()->modViewportScale(ic, scl);
		}
	      else if (!m_disableRotationInViewport &&
		       event->buttons() == Qt::LeftButton) // rotate
		{
		  int axis = 0;
		  float angle = 0;
		  if (qAbs(mx) > qAbs(my))
		    {
		      axis = 1; // rotate about green axis
		      angle = mx;
		    }
		  else
		    {
		      axis = 0; // rotate about red axis
		      angle = my;
		    }
		  if (event->modifiers() & Qt::ControlModifier)
		    axis = 2; // rotate about blue axis

		  GeometryObjects::clipplanes()->rotate(ic, axis, angle);
		}
	      updateGL();
	      m_mousePrevPos = event->pos();
	      return;
	    }
	}
    }
	 
  // -------------------------
	  
  if ((PruneHandler::carve() ||
       PruneHandler::paint()) &&
      found)    
    {
      Vec voxelScaling = Global::voxelScaling();
      Vec pt = VECDIVIDE(target, voxelScaling);
      if (!mouseButtonPressed)
	{
	  carveHitPointOK = true;
	  carveHitPoint = pt;
	  emit updateGL();
	  return;
	}
      
      if (!m_mouseDrag)
	return;
      
      m_mousePrevPos = event->pos();
      
      QList<Vec> points;
      points << pt;
      int docarve = 0;
      if (mouseButtonPressed == Qt::LeftButton)
	docarve = 1; // carve
      else if (mouseButtonPressed == Qt::RightButton)
	docarve = 2; // restore
      else if (mouseButtonPressed == Qt::MiddleButton)
	docarve = 3; // set to 1
      
      Vec cp, cn; 
      cp = Vec(0,0,0);
      cn = Vec(0,0,0);
      QList<Vec> clipPos = GeometryObjects::clipplanes()->positions();
      QList<Vec> clipNormal = GeometryObjects::clipplanes()->normals();
      QList<Vec> cxaxis = GeometryObjects::clipplanes()->xaxis();
      QList<Vec> cyaxis = GeometryObjects::clipplanes()->yaxis();
      QList<int> clipThickness = GeometryObjects::clipplanes()->thickness();
      Vec dmin = m_hiresVolume->volumeMin();
      if (ic >= 0) // we are in a viewport
	{
	  cp = VECDIVIDE(clipPos[ic], voxelScaling);
	  PruneHandler::setPlanarCarve(cp,
				       clipNormal[ic], cxaxis[ic], cyaxis[ic],
				       clipThickness[ic], dmin);
	}
//      else
//	{
//	  for(int c=0; c<clipPos.count(); c++)
//	    {
//	      if (fabs(clipNormal[c] *(pt - clipPos[c])) < 0.1)
//		{
//		  cp = VECDIVIDE(clipPos[c], voxelScaling);
//		  PruneHandler::setPlanarCarve(cp,
//					       clipNormal[c], cxaxis[c], cyaxis[c],
//					       clipThickness[c], dmin);
//		  break;
//		}
//	    }
//	}
      
      if (docarve > 0)
	PruneHandler::sculpt(docarve,
			     m_hiresVolume->volumeMin(),
			     points);
      
      PruneHandler::setPlanarCarve();
      
      renderVolume(Enums::StillImage);
      updateGL();
      
      m_mousePrevPos = event->pos();
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

  if (m_mouseDrag && GeometryObjects::imageCaptions()->isActive())
    GeometryObjects::imageCaptions()->setActive(false);


  if (m_brickAngleFromMouse && m_mouseDrag)
    {
      float angle = m_bricksWidget->getAngle();
      float len = m_mousePrevPos.x() - event->pos().x();
      angle += 0.1f*len; 
      m_bricksWidget->setAngle(angle);

      updateGL();     
      m_mousePrevPos = event->pos();
      return;
    }

  
  QGLViewer::mouseMoveEvent(event);
  m_mousePrevPos = event->pos();
}

void
Viewer::mouseReleaseEvent(QMouseEvent *event)
{
  mouseButtonPressed = 0;

  m_mouseDrag = false;

  QGLViewer::mouseReleaseEvent(event);

  GeometryObjects::clipplanes()->resetViewportGrabbed();
  GeometryObjects::paths()->resetViewportGrabbed();

  LightHandler::mouseReleaseEvent(event, camera());

  m_undo.append(camera()->position(), camera()->orientation());

  if (LightHandler::lightsChanged() ||
      PruneHandler::carve())
    {
      if (PruneHandler::carve())
	LightHandler::generateOpacityTexture();
      
      LightHandler::updateLightBuffers();
      m_hiresVolume->initShadowBuffers(true);
      updateGL();
    }
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


  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);
//  glEnable(GL_LINE_SMOOTH);
//  glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
  
  glDisable(GL_DEPTH_TEST);

  QString toggleUpdates = "Update Disabled (u : toggle updates)";
  QFont tfont = QFont("Helvetica");
  tfont.setStyleStrategy(QFont::PreferAntialias);

  //-------------
  // calculate font scale based on dpi
  float fscl = 120.0/Global::dpi();
  //-------------

  tfont.setPointSize(20*fscl);

  StaticFunctions::renderText(10, size().height()/2,
			      toggleUpdates, tfont,
			      Qt::transparent, Qt::lightGray);

  glEnable(GL_DEPTH_TEST);
}

void
Viewer::enterEvent(QEvent *e)
{
  //setFocus();
  grabKeyboard();
}

void
Viewer::leaveEvent(QEvent *e)
{
  //clearFocus();
  releaseKeyboard();
}

void
Viewer::undoParameters()
{
  camera()->setPosition(m_undo.pos());
  camera()->setOrientation(m_undo.rot());
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

  if (event->key() == Qt::Key_Question)
    {
      Global::setBottomText(!Global::bottomText());
      MainWindowUI::mainWindowUI()->actionBottom_Text->setChecked(Global::bottomText());
      updateGL();
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
	    
  if (LightHandler::grabsMouse())
    {
      if (LightHandler::keyPressEvent(event))
	{
	  if (LightHandler::lightsChanged())
	    {
	      LightHandler::updateLightBuffers();
	      m_hiresVolume->initShadowBuffers(true);
	      updateGL();
	    }
	  return;
	}
    }
	    
  if (event->key() == Qt::Key_H)
    {
      if (event->modifiers() & Qt::ControlModifier ||
	  event->modifiers() & Qt::MetaModifier)
	showHelp();
      else
	{
	  Global::setUseStillVolume(!Global::useStillVolume());
	  MainWindowUI::mainWindowUI()->actionUse_stillvolume->setChecked(Global::useStillVolume());
	}
      return;
    }

  if (event->key() == Qt::Key_Space)
    {
      commandEditor();
      updateGL();

      return;
    }

  if (!m_lowresVolume->raised() &&
      !m_hiresVolume->raised())
    return;

  if (m_hiresVolume->raised())
    {
      QPoint scr = mapFromGlobal(QCursor::pos());
      int ow = camera()->screenWidth();
      int oh = camera()->screenHeight();
      int ic = GeometryObjects::clipplanes()->inViewport(scr.x(), scr.y(),
							 ow, oh);
      if (ic >= 0)
	{
	  bool flag = false;
	  
	  if (event->key() == Qt::Key_R &&
	      event->modifiers() & Qt::ShiftModifier)
	    {
	      m_disableRotationInViewport = !m_disableRotationInViewport;
	      return;
	    }

	  if (event->key() == Qt::Key_R)
	    {
	      if (m_radSpinBox->isVisible())
		m_radSpinBox->hide();
	      else
		{ 
		  QPoint cur = QCursor::pos();
		  m_radSpinBox->setGeometry(cur.x(), cur.y(),
					    m_radSpinBox->width(),
					    m_radSpinBox->height());
		  m_radSpinBox->show();
		}
	      return;
	    }
	  if (event->key() == Qt::Key_T)
	    {
	      if (m_tagSpinBox->isVisible())
		m_tagSpinBox->hide();
	      else
		{ 
		  QPoint cur = QCursor::pos();
		  m_tagSpinBox->setGeometry(cur.x(), cur.y(),
					    m_tagSpinBox->width(),
					    m_tagSpinBox->height());
		  m_tagSpinBox->show();
		}
	      return;
	    }
	  if (event->key() == Qt::Key_Delete ||
	      event->key() == Qt::Key_Space)
	    { 
	      flag = GeometryObjects::clipplanes()->viewportKeypressEvent(ic, event);
	    }
	  else if ( (event->key() >= Qt::Key_Left &&
		     event->key() <= Qt::Key_Down) ||
		    event->key() == Qt::Key_X ||
		    event->key() == Qt::Key_Y )
	    {
	      // translate/rotate clipplane from within viewport
	      int mag = 1;
	      Vec cnorm = GeometryObjects::clipplanes()->normals()[ic];
	      if (event->modifiers() & Qt::ShiftModifier)
		mag = 10;
	      if (event->key() == Qt::Key_Up)
		GeometryObjects::clipplanes()->translate(ic, mag*cnorm);
	      if (event->key() == Qt::Key_Down)
		GeometryObjects::clipplanes()->translate(ic, -mag*cnorm);
	      
	      if (event->key() == Qt::Key_Left)
		GeometryObjects::clipplanes()->rotate(ic, 2, mag);
	      if (event->key() == Qt::Key_Right)
		GeometryObjects::clipplanes()->rotate(ic, 2, -mag);
	      
	      if (event->key() == Qt::Key_X)
		{
		  if (event->modifiers() & Qt::ControlModifier)
		    GeometryObjects::clipplanes()->rotate(ic, 0, -mag);
		  else
		    GeometryObjects::clipplanes()->rotate(ic, 0, mag);
		}
	      if (event->key() == Qt::Key_Y)
		{
		  if (event->modifiers() & Qt::ControlModifier)
		    GeometryObjects::clipplanes()->rotate(ic, 1, -mag);
		  else
		    GeometryObjects::clipplanes()->rotate(ic, 1, mag);
		}
	      
	      updateGL();
	      flag = true;
	    }
	  if (flag) return;
	}
      else
	{
	  int ip = GeometryObjects::paths()->inViewport(scr.x(), scr.y(),
							ow, oh);
	  if (ip >= 0) // in path viewport
	    {
	      bool flag = false;
	      Vec voxelScaling = VolumeInformation::volumeInformation().voxelSize;
	      flag = GeometryObjects::paths()->viewportKeypressEvent(ip, event,
								     scr, voxelScaling,
								     ow, oh);
	      if (flag)
		{
		  updateGL();
		  return;
		}
	    }
	}
    }

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

  if (Global::volumeType() != Global::DummyVolume &&
      event->key() == Qt::Key_R)
    {
      if (PruneHandler::paint() || PruneHandler::carve())
	{
	  if (m_radSpinBox->isVisible())
	    m_radSpinBox->hide();
	  else
	    { 
	      QPoint cur = QCursor::pos();
	      m_radSpinBox->setGeometry(cur.x(), cur.y(),
					m_radSpinBox->width(),
					m_radSpinBox->height());
	      m_radSpinBox->show();
	    }
	  return;
	}

      reloadData();
      updateGL();
      return;
    }

//  if (event->key() == Qt::Key_U)
//    {
//      if (Global::updateViewer())
//	{
//	  grabBackBufferImage();
//	  Global::disableViewerUpdate();
//        MainWindowUI::changeDrishtiIcon(false);
//	}
//      else
//	{
//	  Global::enableViewerUpdate();
//        MainWindowUI::changeDrishtiIcon(true);
//	  updateGL();
//	}
//      return;
//    }
	    
  if (event->key() == Qt::Key_F2)
    {
      switchDrawVolume();
      return ;
    }

  
  if (m_hiresVolume->raised() &&
      (PruneHandler::carve() || PruneHandler::paint()))
    {
      bool ok = false;
      if (PruneHandler::paint())
	{
	  if (event->key() == Qt::Key_Z)
	    {
	      QList<Vec> clipNormal = GeometryObjects::clipplanes()->normals();
	      if (clipNormal.count() > 0)
		{
		  if (event->modifiers() & Qt::ShiftModifier)
		    GeometryObjects::clipplanes()->translate(0, clipNormal[0]);
		  else
		    GeometryObjects::clipplanes()->translate(0, -clipNormal[0]);
		  updateGL();
		  return;
		}
	    }

	  float r,d;
	  int t;
	  PruneHandler::carveRad(r,d);
	  t = PruneHandler::tag();
	  if (event->key() == Qt::Key_Up) { r++; ok = true; }
	  if (event->key() == Qt::Key_Down) { r--; ok = true; }
	  if (event->key() == Qt::Key_Left) { t--; ok = true; }
	  if (event->key() == Qt::Key_Right) { t++; ok = true; }
	  if (ok) 
	    {
	      r = qBound(1.0f, r, 200.0f);
	      PruneHandler::setCarveRad(r,d);
	      m_radSpinBox->setValue(r);
	      t = qBound(0, t, 255);
	      PruneHandler::setTag(t);
	      m_tagSpinBox->setValue(t);
	      updateGL();
	      return;
	    }
	}

      float r,d;
      PruneHandler::carveRad(r,d);
      if (event->key() == Qt::Key_Up) { r++; ok = true; }
      if (event->key() == Qt::Key_Down) { r--; ok = true; }
      if (event->key() == Qt::Key_Left) { d--; ok = true; }
      if (event->key() == Qt::Key_Right) { d++; ok = true; }
      if (ok) 
	{
	  r = qBound(1.0f, r, 200.0f);
	  d = qBound(0.0f, d, 255.0f);
	  //d = qBound(0.0f, d, r);
	  PruneHandler::setCarveRad(r,d);
	  m_radSpinBox->setValue(r);
	  updateGL();
	  return;
	}
    }


  if (event->key() == Qt::Key_PageUp ||
      (event->modifiers() & Qt::ShiftModifier &&
       event->key() == Qt::Key_Up))
    { emit changeStill(1); return; }
  else if (event->key() == Qt::Key_PageDown ||
      (event->modifiers() & Qt::ShiftModifier &&
       event->key() == Qt::Key_Down))
    { emit changeStill(-1); return; }
  else if (event->key() == Qt::Key_Home ||
      (event->modifiers() & Qt::ShiftModifier &&
       event->key() == Qt::Key_Left))
    { emit changeDrag(1); return; }
  else if (event->key() == Qt::Key_End ||
      (event->modifiers() & Qt::ShiftModifier &&
       event->key() == Qt::Key_Right))
    { emit changeDrag(-1); return; }



  if (m_lowresVolume->raised())
    {
      if (m_lowresVolume->keyPressEvent(event))
	{
	  updateGL();
	  return;
	}
    }
  else if (m_hiresVolume->raised())
    {
      if (event->key() == Qt::Key_Tab)
	{
	  if (LightHandler::openPropertyEditor())
	    {
	      m_hiresVolume->initShadowBuffers(true);
	      updateGL();
	    }
	  return;
	}

      if (m_hiresVolume->keyPressEvent(event))
	{
	  updateGL();
	  return;
	}

      if (event->key() == Qt::Key_P)
	{
	  if (GeometryObjects::trisets()->count() > 0)
	    {
	      setPaintMode(!m_paintMode);
	      GeometryObjects::trisets()->setGrab(false);
	    }
	  return;
	}
      
      if (event->key() == Qt::Key_T)
	{
	  if (PruneHandler::paint())
	    {
	      if (m_tagSpinBox->isVisible())
		m_tagSpinBox->hide();
	      else
		{ 
		  QPoint cur = QCursor::pos();
		  m_tagSpinBox->setGeometry(cur.x(), cur.y(),
					    m_tagSpinBox->width(),
					    m_tagSpinBox->height());
		  m_tagSpinBox->show();
		}
	      return;
	    }

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

  if (event->key() == Qt::Key_A)
    {
      emit switchAxis();
      updateGL();
      return;
    }


//  if (event->key() == Qt::Key_Space)
//    {
//      commandEditor();
//      updateGL();
//
//      return;
//    }

  if (event->key() == Qt::Key_Escape)
    {
      int ok = QMessageBox::question(0, "Exit Drishti",
				     QString("Do you really want to exit Drishti ?"),
				     QMessageBox::Yes | QMessageBox::No);
      if (ok != QMessageBox::No)
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
    QMessageBox::information(0, "", "Image copied to clipboard.");
}

void
Viewer::grabScreenShot()
{
  QSize imgSize = StaticFunctions::getImageSize(size().width(),size().height());
  //setImageSize(imgSize.width(), imgSize.height());

  QString flnm;
  flnm = QFileDialog::getSaveFileName(0,
				      "Save snapshot",
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
			      "Image Type",
			      "Image Type",
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
  emit showMessage("Snapshot saved", false);
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

  setKeyDescription(Qt::Key_1, "Toggles Shading Mode in Hires Window");
  setKeyDescription(Qt::Key_0, "Set Image Quality to Normal in Hires Window");
  setKeyDescription(Qt::Key_9, "Set Image Quality to Low in Hires Window");
  setKeyDescription(Qt::Key_8, "Set Image Quality to VeryLow in Hires Window");
  setKeyDescription(Qt::Key_D, "Toggle Depth cueing in Hires Window");
  setKeyDescription(Qt::Key_L, "Toggle use of lowres volume for rendering in Hires Window");
  setKeyDescription(Qt::Key_H, "Toggle high quality render during drag in Hires Window");

  setKeyDescription(Qt::Key_F2, "Toggles between Lowres and Hires Window");
  setKeyDescription(Qt::Key_PageUp, "Increase still image quality");
  setKeyDescription(Qt::Key_PageDown, "Decrease still image quality");
  setKeyDescription(Qt::Key_Home, "Increase drag image quality");
  setKeyDescription(Qt::Key_End, "Decrease drag image quality");
  setKeyDescription(Qt::Key_M, "Toggles transfer function morphing for keyframe animation");
  setKeyDescription(Qt::Key_C, "Add clip plane - clip plane always added at center");

  setMouseBinding(Qt::SHIFT+Qt::RightButton, SELECT);

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
Viewer::updateStereoSettings(float dist, float es, float width)
{
  camera()->setFocusDistance(dist);
  camera()->setIODistance(es);
  camera()->setPhysicalScreenWidth(width);
  m_focusDistance = camera()->focusDistance();

  updateGL();
}

QString Viewer::helpString() const
{
  QString text("<h2>D r i s h t i</h2>");
  text += "Use the mouse to move the camera around the object. ";
  text += "You can respectively revolve around, zoom and translate with the three mouse buttons. ";
  text += "Left and middle buttons pressed together rotate around the camera view direction axis<br><br>";
  text += "Press <b>F</b> to display the frame rate, <b>A</b> for the world axis, ";
  text += "<b>Alt+Return</b> for full screen mode and <b>Alt+S</b> to save a snapshot. ";
  text += "See the <b>Keyboard</b> tab in this window for a complete shortcut list.<br><br>";
  text += "Double clicks automates single click actions: A left button double click aligns the closer axis with the camera (if close enough). ";
  text += "A middle button double click fits the zoom of the camera and the right button re-centers the scene.<br><br>";
  text += "A left button double click while holding right button pressed defines the camera <i>Revolve Around Point</i>. ";
  text += "See the <b>Mouse</b> tab and the documentation web pages for details.<br><br>";
  text += "Press <b>Escape</b> to exit the viewer.";
  return text;
}

void
Viewer::processLight(QStringList list)
{
  if (list[0] == "addplight" ||
      list[0] == "adddlight")
    {
      QList<Vec> pts;
      if (GeometryObjects::hitpoints()->activeCount())
	pts = GeometryObjects::hitpoints()->activePoints();
      else
	pts = GeometryObjects::hitpoints()->points();
      
      if (pts.count() == 0)
	{
	  QMessageBox::information(0, "Error", "Need atleast one point to add a light");
	  return;
	}
	
      if (list[0] == "addplight")
	LightHandler::giLights()->addGiPointLight(pts);
      else
	LightHandler::giLights()->addGiDirectionLight(pts);
      
      // now remove points that were used to make the path
      if (GeometryObjects::hitpoints()->activeCount())
	GeometryObjects::hitpoints()->removeActive();
      else
	GeometryObjects::hitpoints()->clear();
      
      LightHandler::updateLightBuffers();
      m_hiresVolume->initShadowBuffers(true);

      updateGL();
    }
}

void
Viewer::processCommand(QString cmd)
{
  bool ok;
  QString ocmd = cmd;
  cmd = cmd.toLower();
  QStringList list = cmd.split(" ", QString::SkipEmptyParts);

  if (list[0] == "histogram")
    {
      bool flag = false;
      if (list.size() > 1)
	{
	  if (list[1] == "off")
	    flag = true;
	  else if (list[1] == "on")
	    flag = false;
	}
      Global::setDisableHistogram(flag);
      return;
    }
  
  if (list[0].contains("switchresmode"))
    {
      switchDrawVolume();      
      return;
    }
  else if (list[0].contains("light"))
    {
      processLight(list);
      return;
    }
  else if (list[0] == "depthcue")
    {
      bool flag = true;
      if (list.size() > 1)
	{
	  if (list[1] == "off")
	    flag = false;
	  else if (list[1] == "on")
	    flag = true;
	}
      Global::setDepthcue(flag);
      MainWindowUI::mainWindowUI()->actionDepthcue->setChecked(Global::depthcue());
      updateGL();
      return;
    }
//  else if (list[0] == "image2volume")
//    {
//      QString pfile = m_hiresVolume->getResliceFileName();
//      if (! pfile.isEmpty())
//	{
//	  m_hiresVolume->saveImage2Volume(pfile);
//	  updateGL();
//	}
//      return;
//    }
  else if (list[0] == "fullscreen")
    {
      bool flag = true;
      if (list.size() > 1)
	{
	  if (list[1] == "off")
	    flag = false;
	  else if (list[1] == "on")
	    flag = true;
	}
      QWidget *mw = MainWindowUI::mainWindowUI()->menubar->parentWidget();
      if (flag)
	mw->setWindowState(mw->windowState() | Qt::WindowFullScreen);
      else if (mw->windowState() == Qt::WindowFullScreen)
	mw->setWindowState(mw->windowState() ^ Qt::WindowFullScreen);

      if (mw->windowState() == Qt::WindowFullScreen)
	MainWindowUI::mainWindowUI()->menubar->hide();
      else
	MainWindowUI::mainWindowUI()->menubar->show();
      return;
    }
  else if (list[0].contains("tempdir"))
    {
      if (list[0].contains("reset"))
	{
	  Global::setTempDir("");
	  QMessageBox::information(0, "", "No temp directory set.");
	}
      else
	{
	  QString flnm;
	  flnm = QFileDialog::getExistingDirectory(0,
						   "Select Directory to save all temporary files",
						   Global::previousDirectory(),
						   QFileDialog::ShowDirsOnly |
						   QFileDialog::DontUseNativeDialog);
	  if (!flnm.isEmpty())
	    Global::setTempDir(flnm);
      
	  flnm = Global::tempDir();
	  QMessageBox::information(0, "", QString("Temp directory set to %1").arg(flnm));
	}
    }
  else if (list[0] == "resetcamera")
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
  else if (list[0] == "mop") // morphological operations
    {
      list.removeFirst();
      if (list.count() == 0)
	emit processMops();
      else
	handleMorphologicalOperations(list);
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
  else if (list[0] == "enablevolumeupdates")
    {
      m_hiresVolume->enableSubvolumeUpdates();
    }
  else if (list[0] == "disablevolumeupdates")
    {
      m_hiresVolume->disableSubvolumeUpdates();
    }
//  else if (list[0] == "colorbar")
//    {
//      int tfset = 0;
//      if (list.size() > 1)
//	tfset = list[1].toInt(&ok);
//      ColorBarObject cbo;
//      cbo.set(QPointF(0.5, 0.5),
//	      1, // vertical
//	      tfset,
//	      50, // width
//	      200, // height
//	      true); // color only
//      GeometryObjects::colorbars()->add(cbo);
//    }
  else if (list[0] == "scalebar")
    {
      float nvox = 100;
      if (list.size() > 1)
	nvox = list[1].toFloat(&ok);
      ScaleBarObject sbo;
      sbo.set(QPointF(0.5, 0.5), nvox, true, true); // horizontal
      GeometryObjects::scalebars()->add(sbo);
    }
  else if (list[0] == "setlod")
    {
      int rlod = 0;
      if (list.size() > 1)
	rlod = list[1].toInt(&ok);
      Global::setLod(rlod);
      reloadData();
    }
  else if (list[0] == "scalebb")
    {
      float vs = 1;
      if (list.size() > 1)
	vs = list[1].toFloat(&ok);
      vs = qMax(1.0f, vs);
      
      m_Volume->setBBScale(vs);
    }
  else if (list[0] == "voloffset")
    {
      int v = -1;
      float offd, offw, offh;
      offd = offw = offh = 0.5;
      if (list.size() > 1) v = list[1].toInt(&ok);
      if (list.size() > 2) offh = list[2].toFloat(&ok);
      if (list.size() > 3) offw = list[3].toFloat(&ok);
      if (list.size() > 4) offd = list[4].toFloat(&ok);
      offh = qBound(0.0f, offh, 1.0f);
      offw = qBound(0.0f, offw, 1.0f);
      offd = qBound(0.0f, offd, 1.0f);
      m_Volume->setOffsets(v, offd, offw, offh);
      reloadData();
    }
  else if (list[0] == "depthoffield")
    {
      int b;
      float fp;
      m_hiresVolume->dof(b, fp);
      if (list.size() > 1) fp = list[1].toFloat(&ok);
      if (list.size() > 2) b = list[2].toInt(&ok);
      m_hiresVolume->setDOF(b, fp);
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
  else if (list[0] == "mix")
    {
      int mv = 0;
      bool mc = false;
      bool mo = false;
      bool mt = false;
      bool mtok = false;
      int idx = 1;
      if (list.size() > 1)
	{
	  if (list[1] == "0") { mv = 0; idx++; }
	  else if (list[1] == "1") { mv = 1; idx++; }
	  else if (list[1] == "2") { mv = 2; idx++; }
	}
      while (idx < list.size())
	{
	  if (list[idx] == "no" || list[idx] == "off") { mc = false; mo = false; }
	  else if (list[idx] == "color") mc = true;
	  else if (list[idx] == "opacity") mo = true;	  
	  else if (list[idx] == "tag" || list[idx] == "tags")
	    {
	      mtok = true;
	      mt = true;	  
	      if (idx+1 < list.size())
		{
		  idx++;
		  if (list[idx] == "no" || list[idx] == "off")
		    mt = false;
		}
	    }
	  idx ++;
	}
      if (!mtok)
	m_hiresVolume->setMix(mv, mc, mo);
      else
	{
	  updateTagColors();
	  m_hiresVolume->setMixTag(mt);
	}
    }
  else if (list[0] == "amr")
    {
      bool amr = true;
      if (list.size() > 1)
	if (list[1] == "off") amr = false;
      
      m_hiresVolume->setAMR(amr);

    }
  else if (list[0] == "interpolatevolumes")
    {
      int iv = 1;
      if (list.size() > 1)
	{
	  iv = 0;
	  if (list[1] == "color") iv = 1;
	  if (list[1] == "value") iv = 2;
	}	

      m_hiresVolume->setInterpolateVolumes(iv);

    }
//  else if (list[0] == "texsizereducefraction")
//    {
//      float rlod = list[1].toFloat(&ok);
//      Global::setTexSizeReduceFraction(rlod);
//      reloadData();
//    }
  else if (list[0] == "imagesize")
    {
      m_imageWidth = list[1].toInt(&ok);
      m_imageHeight = list[2].toInt(&ok);
    }
  else if (list[0] == "viewfilter")
    {
      if (list[1] == "gauss")
	Global::setViewerFilter(Global::_GaussianFilter);
      else if (list[1] == "sharp")
	Global::setViewerFilter(Global::_SharpnessFilter);
      else // default gaussian filter
	Global::setViewerFilter(Global::_GaussianFilter);

      createBlurShader();
    }
  else if (list[0] == "filter")
    {
      if (list[1] == "no")
	Global::setFilteredData(Global::_NoFilter);
      else if (list[1] == "conservative")
	Global::setFilteredData(Global::_ConservativeFilter);
      else if (list[1] == "mean")
	Global::setFilteredData(Global::_MeanFilter);
      else if (list[1] == "median")
	Global::setFilteredData(Global::_MedianFilter);
    }  
  else if (list[0] == "addpoint" ||
	   list[0] == "point")
    {
      Vec pos;
      float x=0,y=0,z=0;
      if (list.size() > 1) x = list[1].toFloat(&ok);
      if (list.size() > 2) y = list[2].toFloat(&ok);
      if (list.size() > 3) z = list[3].toFloat(&ok);
      pos = VECPRODUCT(Vec(x,y,z), Global::voxelScaling());
      GeometryObjects::hitpoints()->add(pos);
    }
  else if (list[0] == "deselectall")
    {
      GeometryObjects::hitpoints()->resetActive();
    }
  else if (list[0] == "enablegrabpoints")
    {
      GeometryObjects::hitpoints()->setMouseGrab(true);
    }
  else if (list[0] == "disablegrabpoints")
    {
      GeometryObjects::hitpoints()->setMouseGrab(false);
    }
  else if (list[0] == "enablegrabpoint")
    {
      if (list.size() > 1)
	{
	  int ptno = list[1].toInt(&ok);
	  GeometryObjects::hitpoints()->addInMouseGrabberPool(ptno);
	}
      else
	QMessageBox::information(0, "Error", "Need point number");
    }
  else if (list[0] == "disablegrabpoint")
    {
      if (list.size() > 1)
	{
	  int ptno = list[1].toInt(&ok);
	  GeometryObjects::hitpoints()->removeFromMouseGrabberPool(ptno);
	}
      else
	QMessageBox::information(0, "Error", "Need point number");
    }
  else if (list[0] == "removepoints")
    {
      if (list[1] == "all")
	GeometryObjects::hitpoints()->clear();
      else if (list[1] == "selected")
	GeometryObjects::hitpoints()->removeActive();
    }
//  else if (list[0]== "loadtriset")
//    {
//      QString flnm;
//      flnm = QFileDialog::getOpenFileName(0,
//					  "Load triset file",
//					  Global::previousDirectory(),
//					  "Triset Files (*.triset)");
//      
//      if (flnm.isEmpty())
//	return;
//
//      GeometryObjects::trisets()->addTriset(flnm);
//
//    }
//  else if (list[0]== "loadply")
//    {
//      QString flnm;
//      flnm = QFileDialog::getOpenFileName(0,
//					  "Load ply file",
//					  Global::previousDirectory(),
//					  "PLY Files (*.ply)");
//      
//      if (flnm.isEmpty())
//	return;
//
//      GeometryObjects::trisets()->addMesh(flnm);
//
//    }
//  else if (list[0]== "loadnetwork")
//    {
//      QString flnm;
//      flnm = QFileDialog::getOpenFileName(0,
//					  "Load network file",
//					  Global::previousDirectory(),
//					  "NetCDF Files (*.nc)");
//      
//      if (flnm.isEmpty())
//	return;
//
//      GeometryObjects::networks()->addNetwork(flnm);
//
//    }
  else if (list[0]== "loadpoints" ||
	   list[0]== "loadpoint")
    {
      QString flnm;
      flnm = QFileDialog::getOpenFileName(0,
					  "Load points file",
					  Global::previousDirectory(),
					  "Files (*.*)");
      
      if (flnm.isEmpty())
	return;

      GeometryObjects::hitpoints()->addPoints(flnm);
    }
  else if (list[0]== "loadbarepoints" ||
	   list[0]== "loadbarepoint")
    {
      QString flnm;
      flnm = QFileDialog::getOpenFileName(0,
					  "Load points file",
					  Global::previousDirectory(),
					  "Files (*.*)");
      
      if (flnm.isEmpty())
	return;

      GeometryObjects::hitpoints()->addBarePoints(flnm);
    }
  else if (list[0]== "removebarepoints")
    {
      GeometryObjects::hitpoints()->removeBarePoints();
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
  else if (list[0]== "geosteps")
    {
      if (list.size() > 1)
	{
	  int fp = list[1].toInt(&ok);
	  Global::setGeoRenderSteps(fp);
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
  else if (list[0]== "savepoints" ||
	   list[0]== "savepoint")
    {
      QString flnm;
      flnm = QFileDialog::getSaveFileName(0,
					  "Save points to text file",
					  Global::previousDirectory(),
					  "Files (*.*)");
      
      if (flnm.isEmpty())
	return;

      GeometryObjects::hitpoints()->savePoints(flnm);
    }
  else if (list[0]== "savepaths" ||
	   list[0]== "savepath")
    {
      QString flnm;
      flnm = QFileDialog::getSaveFileName(0,
					  "Save all paths to text file",
					  Global::previousDirectory(),
					  "Files (*.*)");
      
      if (flnm.isEmpty())
	return;

      QFile fpath(flnm);
      fpath.open(QFile::WriteOnly | QFile::Text);
      QTextStream fd(&fpath);

      QList<PathObject> pathobj = GeometryObjects::paths()->paths();
      for (int npi = 0; npi < pathobj.count(); npi++)
	{
	  QList<Vec> pts = pathobj[npi].points();
	  fd << pts.count() << "\n";
	  for(int pi=0; pi < pts.count(); pi++)
	    fd << pts[pi].x << " " << pts[pi].y << " " << pts[pi].z << "\n";
	}
      fd.flush();
    }
  else if (list[0]== "loadimage")
    {
      QString flnm;
      flnm = QFileDialog::getOpenFileName(0,
					  "Load background image",
					  Global::previousDirectory(),
            "Image Files (*.png *.tif *.bmp *.jpg *.ppm *.xbm *.xpm)");
  
      if (flnm.isEmpty())
	return;

      //----------------
      // bgimage file is assumed to be relative to .pvl.nc file
      // get the absolute path
      VolumeInformation pvlInfo = VolumeInformation::volumeInformation();
      QFileInfo fileInfo(pvlInfo.pvlFile);
      flnm = fileInfo.absoluteDir().relativeFilePath(flnm);
      Global::setBackgroundImageFile(flnm, fileInfo.absolutePath());
      //----------------
    }
  else if (list[0]== "resetimage")
    {
      Global::resetBackgroundImageFile();
    }
  else if (list[0]== "loadpath")
    {
      QString flnm;
      flnm = QFileDialog::getOpenFileName(0,
					  "Load points file for path",
					  Global::previousDirectory(),
					  "Files (*.*)");
      
      if (flnm.isEmpty())
	return;
      
      GeometryObjects::paths()->addPath(flnm);
    }
  else if (list[0]== "loadpathgroup")
    {
      QString flnm;
      flnm = QFileDialog::getOpenFileName(0,
					  "Load points file for path",
					  Global::previousDirectory(),
					  "Files (*.*)");
      
      if (flnm.isEmpty())
	return;
      
      GeometryObjects::pathgroups()->addPath(flnm);
    }
  else if (list[0]== "loadvector")
    {
      QString flnm;
      flnm = QFileDialog::getOpenFileName(0,
					  "Load file for vector field",
					  Global::previousDirectory(),
					  "Files (*.*)");
      
      if (flnm.isEmpty())
	return;
      
      GeometryObjects::pathgroups()->addVector(flnm);
    }
  else if (list[0].contains("landmark"))
    {
      QList<Vec> pts;
      if (GeometryObjects::hitpoints()->activeCount())
	pts = GeometryObjects::hitpoints()->activePoints();
      else
	pts = GeometryObjects::hitpoints()->points();

      if (pts.count() > 0)
	{
	  GeometryObjects::landmarks()->addPoints(pts);

	  // now remove points that were used to make the path
	  if (GeometryObjects::hitpoints()->activeCount())
	    GeometryObjects::hitpoints()->removeActive();
	  else
	    GeometryObjects::hitpoints()->clear();
	}
      else
	QMessageBox::critical(0, "Error", "Need at least 1 point to add as landmark"); 
    }
  else if (list[0]== "addpath" || list[0]== "path")
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
  else if (list[0]== "addgrid" || list[0]== "grid")
    {
      if (list.count() != 3)
	{
	  QMessageBox::critical(0, "Error", "Need to specify number of colums and rows.  Format is addgrid <cols> <rows>");
	  return;
	}

      QList<Vec> pts;
      if (GeometryObjects::hitpoints()->activeCount())
	pts = GeometryObjects::hitpoints()->activePoints();
      else
	pts = GeometryObjects::hitpoints()->points();

      int cols = list[1].toInt();
      int rows = list[2].toInt();

      if (pts.count() < cols*rows)
	{
	  QMessageBox::critical(0, "Error", QString("Number of points (%1) must be atleast colums*rows (%2 * %3)").arg(pts.count()).arg(cols).arg(rows));
	  return;
	}

      GeometryObjects::grids()->addGrid(pts, cols, rows);
      
      // now remove points that were used to make the path
      if (GeometryObjects::hitpoints()->activeCount())
	GeometryObjects::hitpoints()->removeActive();
      else
	GeometryObjects::hitpoints()->clear();
    }
  else if (list[0]== "loadgrid")
    {
      QString flnm;
      flnm = QFileDialog::getOpenFileName(0,
					  "Load file for grid",
					  Global::previousDirectory(),
					  "Files (*.*)");
      
      if (flnm.isEmpty())
	return;
      
      GeometryObjects::grids()->addGrid(flnm);
    }
  else if (list[0] == "crop" ||
	   list[0] == "dissect" ||
	   list[0] == "disect" ||
	   list[0] == "blend" ||
	   list[0] == "displace" ||
	   list[0] == "glow")
    {
      QList<Vec> pts;
      if (GeometryObjects::hitpoints()->activeCount())
	pts = GeometryObjects::hitpoints()->activePoints();
      else
	pts = GeometryObjects::hitpoints()->points();

      if (pts.count() == 2)
	{
	  if (list[0] == "crop")
	    GeometryObjects::crops()->addCrop(pts);
	  else if (list[0] == "dissect" ||
		   list[0] == "disect")
	    GeometryObjects::crops()->addTear(pts);
	  else if (list[0] == "blend")
	    GeometryObjects::crops()->addView(pts);
	  else if (list[0] == "displace")
	    GeometryObjects::crops()->addDisplace(pts);
	  else if (list[0] == "glow")
	    GeometryObjects::crops()->addGlow(pts);

	  // now remove points that were used to make the crop
	  if (GeometryObjects::hitpoints()->activeCount())
	    GeometryObjects::hitpoints()->removeActive();
	  else
	    GeometryObjects::hitpoints()->clear();
	}
      else
	QMessageBox::critical(0, "Error", "Need exactly 2 points to form a crop/dissect/blend/displace/glow"); 
    }
//  else if (list[0] == "wheel")
//    {
//      int ow = camera()->screenWidth();
//      int oh = camera()->screenHeight();
//      int delta = 0;
//      if (list.size() > 1) delta = list[1].toInt(&ok);
//      QWheelEvent event(QPoint(ow/2, oh/2), delta, Qt::MidButton, Qt::NoModifier);
//      bool usv = Global::useStillVolume();
//      Global::setUseStillVolume(true);
//      wheelEvent(&event);
//      Global::setUseStillVolume(usv);
//      return;      
//    }
  else if (list[0] == "keyframe")
    {
      if (list.size() > 1)
	emit moveToKeyframe(list[1].toInt(&ok));	  

      return;
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

      // set camera orientation
      camera()->setOrientation(rot);

      // now reposition the camera so that it faces the scene
      Vec center = camera()->sceneCenter();
      float dist = (camera()->position()-center).norm();
      Vec viewDir = camera()->viewDirection();
      camera()->setPosition(center - dist*viewDir);
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
  else if (list[0] == "relativedatapos")
    {
      Vec pos;
      float x=0,y=0,z=0;
      if (list.size() > 1) x = qBound(-1.0f, list[1].toFloat(&ok), 1.0f);
      if (list.size() > 2) y = qBound(-1.0f, list[2].toFloat(&ok), 1.0f);
      if (list.size() > 3) z = qBound(-1.0f, list[3].toFloat(&ok), 1.0f);
      pos = Vec(x,y,z);
      Global::setRelDataPos(pos);
    }
  else if (list[0] == "countcells")
    {
      if (!m_hiresVolume->raised())
	{
	  QMessageBox::critical(0, "Error", "Cannot apply command in Lowres mode");
	  return;
	}

      emit countIsolatedRegions();
    }
//  else if (list[0].contains("changesliceorder"))
//    {
//      changeSliceOrdering();
//      return;
//    }
//  else if (list[0].contains("reslice") ||
//	   list[0].contains("rescale"))
//    {
//      if (!m_hiresVolume->raised())
//	{
//	  QMessageBox::critical(0, "Error", "Cannot apply command in Lowres mode");
//	  return;
//	}
//
//      float subsample = 1;
//      int tagvalue = -1;
//      if (list.size() > 1) subsample = qMax(0.0f, list[1].toFloat(&ok));
//      if (list.size() > 2) tagvalue = list[2].toInt(&ok);
//
//      if (list[0] == "rescale")      
//	{
//
//	  Vec smin = m_lowresVolume->volumeMin();
//	  Vec smax = m_lowresVolume->volumeMax();
//	  Vec pos = Vec((smax.x+smin.x)*0.5,(smax.y+smin.y)*0.5,smax.z+10);
//	  m_hiresVolume->resliceVolume(pos,
//				       Vec(0,0,-1), Vec(1,0,0), Vec(0,1,0),
//				       subsample,
//				       0, tagvalue);
//	}
//      else
//	{
//	  m_hiresVolume->resliceVolume(camera()->position(),
//				       camera()->viewDirection(),
//				       camera()->rightVector(),
//				       camera()->upVector(),
//				       subsample,
//				       0, tagvalue);
//	}
//
//      return;
//    }
//  else if ((list[0] == "getvolume" ||
//	    list[0] == "getsurfacearea") &&
//	   list.size() <= 2)
//    {
//      if (!m_hiresVolume->raised())
//	{
//	  QMessageBox::critical(0, "Error", "Cannot apply command in Lowres mode");
//	  return;
//	}
//
//      int getVolume = 1;
//      if (list[0] == "getsurfacearea")
//	getVolume = 2;
//	      
//      Vec smin = m_lowresVolume->volumeMin();
//      Vec smax = m_lowresVolume->volumeMax();
//      if (list.size() == 1)
//	{
//	  Vec pos = Vec((smax.x+smin.x)*0.5,(smax.y+smin.y)*0.5,smax.z+10);
//	  m_hiresVolume->resliceVolume(pos,
//				       Vec(0,0,-1), Vec(1,0,0), Vec(0,1,0),
//				       1,
//				       getVolume, -1); // use opacity to getVolume/SurfaceArea
//	}
//      else
//	{
//	  int tag = list[1].toInt(&ok);
//	  if (ok && tag >= 0 && tag <= 255)
//	    m_hiresVolume->resliceVolume((smax+smin)*0.5,
//					 Vec(0,0,1), Vec(1,0,0), Vec(0,1,0),
//					 1,
//					 getVolume, tag); // use opacity to getVolume/SurfaceArea
//	  else
//	    QMessageBox::critical(0, "Error",
//				     "Tag value should be between 0 and 255");
//	}
//    }
//  else if (list[0] == "caption")
//    {
//      CaptionDialog cd(0,
//		       "Caption",
//		       QFont("Helvetica", 15),
//		       QColor::fromRgbF(1,1,1,1),
//		       QColor::fromRgbF(1,1,1,1),
//		       0);
//      cd.hideAngle(false);
//      cd.move(QCursor::pos());
//      if (cd.exec() == QDialog::Accepted)
//	{
//	  QString text = cd.text();
//	  QFont font = cd.font();
//	  QColor color = cd.color();
//	  QColor haloColor = cd.haloColor();
//	  float angle = cd.angle();
//	  
//	  CaptionObject co;
//	  co.set(QPointF(0.5, 0.5),
//		 text, font,
//		 color, haloColor,
//		 angle);
//	  GeometryObjects::captions()->add(co);
//	}
//    }
  else if (list[0] == "search")
    {
      if (list.size() > 1)
	{
	  QStringList strlist;
	  for(int sl=1; sl<list.count(); sl++)
	    strlist << list[sl];
	  emit searchCaption(strlist);
	}
      else
	QMessageBox::critical(0, "Error",
			      "Please specify search text");	
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
  else if (list[0] == "backgroundrender")
    {
      if (list.size() == 2)
	m_useFBO = !(list[1] == "no");
      else
	m_useFBO = true;
    }
  else if (list[0] == "dragonly")
    {
      if (list.size() == 2)
	{
	  if (list[1] == "no")
	    Global::setLoadDragOnly(false);
	  else
	    {
	      Global::setLoadDragOnly(true);
	      Global::setUseDragVolume(true);
	    }
	}
      else
	{
	  Global::setLoadDragOnly(true);
	  Global::setUseDragVolume(true);
	}
      MainWindowUI::mainWindowUI()->actionUse_dragvolume->setChecked(Global::useDragVolume());
    }
  else if (list[0] == "opmod")
    {
      if (list.size() > 1)
	{
	  float fop = 1;
	  float bop = 1;
//	  bop = fop = qMax(0.0f, list[1].toFloat(&ok));
//	  if (list.size() > 2)
//	    bop = qMax(0.0f, list[2].toFloat(&ok));

	  bop = fop = list[1].toFloat(&ok);
	  if (list.size() > 2)
	    bop = list[2].toFloat(&ok);
	  
	  m_hiresVolume->setOpMod(fop, bop);
	}
    }
  else
    QMessageBox::critical(0, "Error",
			  QString("Cannot understand the command : ") +
			  cmd);
}

void
Viewer::updateTagColors()
{
  if (!m_hiresVolume->raised())
    return;

  if (!m_paintTex)
    {
      glGenTextures(1, &m_paintTex);
    }

  glActiveTexture(GL_TEXTURE5);
  glBindTexture(GL_TEXTURE_1D, m_paintTex);
  glTexParameterf(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
  glTexParameterf(GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
  glTexParameterf(GL_TEXTURE_1D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexImage1D(GL_TEXTURE_1D,
	       0,
	       GL_RGBA,
	       256,
	       0,
	       GL_RGBA,
	       GL_UNSIGNED_BYTE,
	       Global::tagColors());  
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

  {
    {
      int nbytes = 1;
      if (m_Volume->pvlVoxelType(0) > 0)
	nbytes = 2;

      if (Global::volumeType() == Global::DoubleVolume) nbytes *= 2;
      if (Global::volumeType() == Global::TripleVolume) nbytes *= 3;
      if (Global::volumeType() == Global::QuadVolume) nbytes *= 4;
      if (Global::volumeType() == Global::RGBVolume) nbytes *= 3;
      if (Global::volumeType() == Global::RGBAVolume) nbytes *= 4;

      Vec volSize = m_hiresVolume->volumeSize();
      qint64 vmb = volSize.x*volSize.y*volSize.z;
      vmb = vmb*nbytes/1024/1024;
      mesg += QString("selected Volume Size : %1MB\n").arg(vmb);

      mesg += QString("drag Volume Size : (max)%1MB  (actual)%2MB\n").	\
	arg(Global::maxDragVolSize()).arg(nbytes*Global::actualDragVolSize());

      mesg += QString("array texture slabs : %1\n").
	arg(m_hiresVolume->dataTexSize());
    }

    if (PruneHandler::pruneBuffer())
      {
	mesg += QString("max 2d Texture Size : %1\n").arg(Global::max2dTextureSize());
	mesg += QString("EmptySpaceSkip Buffer Size : %1 x %2\n\n").	\
	  arg(PruneHandler::pruneBuffer()->width()).			\
	  arg(PruneHandler::pruneBuffer()->height());
      }
    


    Vec cpos = camera()->position();
    if (Global::updatePruneTexture())
      mesg += QString("EmptySpaceSkip : update : on\n");
    else
      mesg += QString("EmptySpaceSkip : update : off\n");

    if (PruneHandler::carve())
      mesg += QString("carve : on\n");
    else
      mesg += QString("carve : off\n");
    
    if (PruneHandler::paint())
      mesg += QString("paint : on\n");
    else
      mesg += QString("paint : off\n");
    
    mesg += QString("camera position : %1 %2 %3\n").	\
            arg(cpos.x).arg(cpos.y).arg(cpos.z);
  }

  {
    Quaternion crot = camera()->orientation();
    Vec axis = crot.axis();
    float angle = crot.angle();
    mesg += QString("camera rotation : axis(%1 %2 %3) angle(%4)\n").	\
            arg(axis.x).arg(axis.y).arg(axis.z).arg(RAD2DEG(angle));
  }

  {
    mesg += "backgroundrender : ";
    if (m_useFBO) mesg += "yes\n";
    else mesg += "no\n";

    mesg += "dragonly : ";
    if (Global::loadDragOnly()) mesg += "yes\n";
    else mesg += "no\n";

//    mesg += QString("texsizereducefraction : %1\n").	\
//      arg(Global::texSizeReduceFraction());

    if (m_hiresVolume->subvolumeUpdates())
      mesg += "subvolume updates : true\n";
    else
      mesg += "subvolume updates : false\n";
  }

  {
    float fop, bop;
    m_hiresVolume->getOpMod(fop, bop);
    mesg += QString("opacity modifiers : %1 %2\n").arg(fop).arg(bop);
  }

  {
    QString tdir = Global::tempDir();  
    if (!tdir.isEmpty())
      mesg += QString("\nTemp directory is set to %1\n").arg(tdir);
  }

  {
    float vscl = m_Volume->bbScale();
    mesg += QString("\nBounding Box Scale : %1").arg(vscl);
    QList<Vec> voffsets;
    voffsets = m_Volume->offsets();
    mesg += "\nVolume Offsets :\n";
    for(int i=0; i<voffsets.count(); i++)
      {
	mesg += QString("%1 : %2 %3 %4\n").arg(i).\
	  arg(voffsets[i].z).arg(voffsets[i].y).arg(voffsets[i].x);
      }
  }
  
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
Viewer::showHelp()
{
  PropertyEditor propertyEditor;
  QMap<QString, QVariantList> plist;
  QVariantList vlist;

  vlist.clear();
  QFile helpFile(":/general.help");
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


  QStringList keys;
  keys << "commandhelp";
  
  propertyEditor.set("General Help", plist, keys);
  propertyEditor.exec();
}

void
Viewer::processMorphologicalOperations()
{
  PropertyEditor propertyEditor;
  QMap<QString, QVariantList> plist;
  QVariantList vlist;
  vlist.clear();
  vlist << "mop ";
  plist["command"] = vlist;

  vlist.clear();
  QFile helpFile(":/mops.help");
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
  Vec cpos = camera()->position();

  {
    int nbytes = 1;
    if (m_Volume->pvlVoxelType(0) > 0)
      nbytes = 2;
    
    if (Global::volumeType() == Global::DoubleVolume) nbytes *= 2;
    if (Global::volumeType() == Global::TripleVolume) nbytes *= 3;
    if (Global::volumeType() == Global::QuadVolume) nbytes *= 4;
    if (Global::volumeType() == Global::RGBVolume) nbytes *= 3;
    if (Global::volumeType() == Global::RGBAVolume) nbytes *= 4;

    Vec volSize = m_hiresVolume->volumeSize();
    qint64 vmb = volSize.x*volSize.y*volSize.z;
    vmb = vmb*nbytes/1024/1024;
    mesg += QString("selected Volume Size : %1MB\n").arg(vmb);

    mesg += QString("drag Volume Size : (max)%1MB  (actual)%2MB\n").		\
      arg(Global::maxDragVolSize()).arg(nbytes*Global::actualDragVolSize());

    mesg += QString("array texture slabs : %1\n").
      arg(m_hiresVolume->dataTexSize());
  }

  if (Global::updatePruneTexture())
    {
      mesg += QString("max 2d Texture Size : %1\n").arg(Global::max2dTextureSize());
      mesg += QString("EmptySpaceSkip Buffer Size : %1 x %2\n\n").	\
	arg(PruneHandler::pruneBuffer()->width()).\
	arg(PruneHandler::pruneBuffer()->height());

      mesg += QString("EmptySpaceSkip : update : on\n");
    }
  else
    mesg += QString("EmptySpaceSkip : update : off\n");

  if (PruneHandler::carve())
    mesg += QString("carve : on\n");
  else
    mesg += QString("carve : off\n");

  if (PruneHandler::paint())
    mesg += QString("paint : on\n");
  else
    mesg += QString("paint : off\n");
  
  if (PruneHandler::blend())
    mesg += QString("blend : on\n");
  else
    mesg += QString("blend : off\n");
  
//  mesg += QString("texsizereducefraction : %1\n").	\
//    arg(Global::texSizeReduceFraction());

  vlist << mesg;

  plist["message"] = vlist;
  //---------------------


  QStringList keys;
  keys << "command";
  keys << "commandhelp";
  keys << "message";
  
  propertyEditor.set("MOP Help", plist, keys);
  
  QMap<QString, QPair<QVariant, bool> > vmap;
  
  if (propertyEditor.exec() == QDialog::Accepted)
    {
      QString cmd = propertyEditor.getCommandString();
      if (!cmd.isEmpty())
	{
	  QStringList list = cmd.toLower().split(" ", QString::SkipEmptyParts);	  
	  if (list.count() > 0)
	    {
	      if (list[0] != "mop")
		cmd = "mop " + cmd;
	      processCommand(cmd);
	    }
	}
    }
  else
    return;
}

#define SETMOUSEBINDINGFORCARVEPAINT(action)             \
  {                                                      \
    setMouseBinding(Qt::SHIFT+Qt::RightButton, action);  \
    setMouseBinding(Qt::SHIFT+Qt::MiddleButton, action); \
    setMouseBinding(Qt::CTRL+Qt::LeftButton, action);    \
    setMouseBinding(Qt::ALT+Qt::LeftButton, action);     \
  }


void
Viewer::handleMorphologicalOperations(QStringList list)
{  
  if (! m_hiresVolume->raised())
    {
      emit showMessage("Cannot apply morphological operations in Lowres mode", true);
      return;
    }

  if (list[0] == "save")
    {
      PruneHandler::saveBuffer();
      return;
    }

  if (list[0] == "histogram")
    {
      int chan = 2;
      if (list.size() > 1) chan = qBound(0, list[1].toInt(), 2);
      QList<int> data = PruneHandler::getHistogram(chan);

      //------
      {      
	QString pflnm;
	pflnm = QFileDialog::getSaveFileName(0,
					     "Save histogram to text file ?",
					     Global::previousDirectory(),
					     "Files (*.txt)");
//					     0,
//					     QFileDialog::DontUseNativeDialog);
	
	
	if (! pflnm.isEmpty())
	  {  
	    if (!StaticFunctions::checkExtension(pflnm, ".txt"))
	      pflnm += ".txt";
	    
	    QFile pfile(pflnm);
	    if (!pfile.open(QIODevice::WriteOnly | QIODevice::Text))
	      {
		QMessageBox::information(0, "Error",
					 QString("Cannot open %1 for writing").arg(pflnm));
	      }
	    else
	      {
		QTextStream out(&pfile);
		out << "Value     Frequency\n";
		int nvals = 0;
		out << "-----------------------------\n";
		for(int i=0; i<data.count(); i++)
		  {
		    out << i+1 << "        " << data[i] << "\n";
		    nvals += data[i];
		  }
		out << "-----------------------------\n";
		out << "Total    " << nvals << "\n";
	      }
	  }
      }
      //------

      return;
    }

  if (list[0] == "maxvalue")
    {
      QList<int> data = PruneHandler::getMaxValue();
      QString maxVals;
      for(int dv=0; dv<data.count(); dv++)
	maxVals += QString("%1 ").arg(data[dv]);
      QMessageBox::information(0, "Channel max values", maxVals);
      return;
    }

  if (list[0] == "update")
    {
      bool flag = true;
      if (list.size() > 1) flag = false;
      Global::setUpdatePruneTexture(flag);

      if (flag && m_hiresVolume->raised())
	{
	  m_hiresVolume->updateAndLoadPruneTexture();
	  if (!savingImages()) updateGL();
	}
      return;
    }

  if (list[0] == "copy")
    {
      bool flag = true;
      if (list.size() > 2)
	{
	  int src = list[1].toInt();
	  int dst = list[2].toInt();
	  if (src >= 0 && src <= 3 && dst >=0 && dst <= 3)
	    PruneHandler::copyChannel(src, dst);
	  else
	    QMessageBox::information(0, "", "mop copy <src 0/1/2> <dst 0/1/2>");
	}
      else
	QMessageBox::information(0, "", "mop copy <src 0/1/2> <dst 0/1/2>");
      return;
    }

  if (list[0] == "channel")
    {
      int ch = -1;
      if (list.size() > 1) ch = list[1].toInt();
      if (ch >=-1 && ch<=2)
	PruneHandler::setChannel(ch);
      else
	QMessageBox::information(0, "Error", " Channel value can be -1/0/1/2.\n -1 means update all channels");
      return;
    }


  int sz = 1;
  bool mopApplied = false;
  
  if (list[0] == "masktf")
    {
      int tf = -1;
      if (list.size() > 1) tf = qMax(-1, list[1].toInt());
      Global::setMaskTF(tf);

      if (m_hiresVolume->raised())
	m_hiresVolume->updateAndLoadPruneTexture();

      mopApplied = true;
    }
  else if (list[0] == "nomop" || list[0] == "noop" || list[0] == "nop")
    {
      if (m_hiresVolume->raised())
	m_hiresVolume->updateAndLoadPruneTexture();

      mopApplied = true;
    }
  else if (list[0] == "sat0")
    {
      int chan = 0;
      sz = 255;
      if (list.size() > 1) sz = qBound(0, list[1].toInt(), 255);
      if (list.size() > 2) chan = qBound(0, list[2].toInt(), 2);
      PruneHandler::setValue(0, chan, 0, sz);

      mopApplied = true;
    }
  else if (list[0] == "sat1")
    {
      int chan = 0;
      sz = 0;
      if (list.size() > 1) sz = qBound(0, list[1].toInt(), 255);
      if (list.size() > 2) chan = qBound(0, list[2].toInt(), 2);
      PruneHandler::setValue(255, chan, sz, 255);

      mopApplied = true;
    }
  else if (list[0] == "setvalue")
    {
      if (list.size() > 3)
	{
	  int chan = 0;
 	  int val = qBound(0, list[1].toInt(), 255);
	  int minval = qBound(0, list[2].toInt(), 255);
	  int maxval = qBound(0, list[3].toInt(), 255);
	  if (list.size() > 4) chan = qBound(0, list[4].toInt(), 2);
	  PruneHandler::setValue(val, chan, minval, maxval);
	  mopApplied = true;
	}
      else
	QMessageBox::information(0, "", "mop setvalue val minval maxval channel");
    }
  else if (list[0] == "invert")
    {
      int chan = -1;
      if (list.size() > 1) chan = qBound(0, list[1].toInt(), 2);
      PruneHandler::invert(chan);

      mopApplied = true;
    }
  else if (list[0] == "removepatch")
    {
      PruneHandler::removePatch(true); // remove patch

      mopApplied = true;
    }
  else if (list[0] == "fusepatch")
    {
      PruneHandler::removePatch(false); // fuse patch

      mopApplied = true;
    }
  else if (list[0] == "localmax")
    {
      PruneHandler::localMaximum();

      mopApplied = true;
    }
  else if (list[0] == "localthickness")
    {
      sz = 100;
      if (list.size() > 1) sz = qBound(1, list[1].toInt(), 255);
      PruneHandler::localThickness(sz);

      mopApplied = true;
    }
  else if (list[0] == "smoothchannel")
    {
      int chan = 0;
      if (list.size() > 1) chan = qBound(0, list[1].toInt(), 2);
      PruneHandler::smoothChannel(chan);

      mopApplied = true;
    }
  else if (list[0] == "average")
    {
      int ch1 = -1;
      int ch2 = -1;
      int dst = -1;
      if (list.size() > 1) ch1 = qBound(0, list[1].toInt(), 2);
      if (list.size() > 2) ch2 = qBound(0, list[2].toInt(), 2);
      if (list.size() > 3) dst = qBound(0, list[3].toInt(), 2);
      if (ch1 < 0 || ch2 < 0 || dst < 0)
	{
	  QMessageBox::information(0, "average",
				   "Needs chan1, chan2 and dst for dst = chan1+chan2");
	  return;
	}
      PruneHandler::average(ch1, ch2, dst);
    }
  else if (list[0] == "copytosaved")
    {
      int ch1 = -1;
      int ch2 = -1;
      if (list.size() > 1) ch1 = qBound(-1, list[1].toInt(), 2);
      if (list.size() > 2) ch2 = qBound(-1, list[2].toInt(), 2);
      if (ch1 < 0 || ch2 < 0) ch1 = ch2 = -1;
      PruneHandler::copyToFromSavedChannel(true, ch1, ch2);
    }
  else if (list[0] == "copyfromsaved")
    {
      int ch1 = -1;
      int ch2 = -1;
      if (list.size() > 1) ch1 = qBound(-1, list[1].toInt(), 2);
      if (list.size() > 2) ch2 = qBound(-1, list[2].toInt(), 2);
      if (ch1 < 0 || ch2 < 0) ch1 = ch2 = -1;
      PruneHandler::copyToFromSavedChannel(false, ch1, ch2);

      mopApplied = true;
    }
  else if (list[0] == "swap")
    {
      PruneHandler::swapBuffer();

      mopApplied = true;
    }
  else if (list[0] == "min")
    {
      int ch1 = -1;
      int ch2 = -1;
      if (list.size() > 1) ch1 = qBound(-1, list[1].toInt(), 2);
      if (list.size() > 2) ch2 = qBound(-1, list[2].toInt(), 2);
      if (ch1 < 0 || ch2 < 0) ch1 = ch2 = -1;
      PruneHandler::minmax(true, ch1, ch2);

      mopApplied = true;
    }
  else if (list[0] == "max")
    {
      int ch1 = -1;
      int ch2 = -1;
      if (list.size() > 1) ch1 = qBound(-1, list[1].toInt(), 2);
      if (list.size() > 2) ch2 = qBound(-1, list[2].toInt(), 2);
      if (ch1 < 0 || ch2 < 0) ch1 = ch2 = -1;
      PruneHandler::minmax(false, ch1, ch2);

      mopApplied = true;
    }
  else if (list[0] == "xor")
    {
      int ch1 = -1;
      int ch2 = -1;
      if (list.size() > 1) ch1 = qBound(-1, list[1].toInt(), 2);
      if (list.size() > 2) ch2 = qBound(-1, list[2].toInt(), 2);
      if (ch1 < 0 || ch2 < 0) ch1 = ch2 = -1;
      PruneHandler::xorTexture(ch1, ch2);

      mopApplied = true;
    }
  else if (list[0] == "edge")
    {
      int val = 0;
      if (list.size() > 1) val = qBound(0, list[1].toInt(), 255);
      if (list.size() > 2) sz = qMax(1, list[2].toInt());
      PruneHandler::edge(val,sz);

      mopApplied = true;
    }
  else if (list[0] == "dilateedge")
    {
      int sz1 = 0;
      int sz2 = 0;
      if (list.size() > 1) sz1 = qBound(1, list[1].toInt(), 255);
      if (list.size() > 2) sz2 = qBound(sz1+1, list[2].toInt(), 255);
      if (sz1 == 0) return;
      if (sz2 == 0)
	{
	  sz2 = sz1;
	  sz1 = 1;
	}
      PruneHandler::dilateEdge(sz1, sz2);

      mopApplied = true;
    }
  else if (list[0] == "dilate")
    {
      int chan = 0;
      if (list.size() > 1) sz = qBound(1, list[1].toInt(), 255);
      if (list.size() > 2) chan = qBound(0, list[2].toInt(), 2);
      PruneHandler::dilate(sz, chan);

      mopApplied = true;
    }
  else if (list[0] == "rdilate")
    {
      int val = 255;
      if (list.size() > 1) val = qBound(1, list[1].toInt(), 255);
      if (list.size() > 2) sz = qBound(1, list[2].toInt(), 255);
      PruneHandler::restrictedDilate(val, sz);

      mopApplied = true;
    }
  else if (list[0] == "erode")
    {
      int chan = 0;
      if (list.size() > 1) sz = qBound(1, list[1].toInt(), 255);
      if (list.size() > 2) chan = qBound(0, list[2].toInt(), 2);
      PruneHandler::erode(sz, chan);

      mopApplied = true;
    }
  else if (list[0] == "shrink")
    {
      sz = 10;
      int chan = 1;
      if (list.size() > 1) sz = qBound(1, list[1].toInt(), 255);
      if (list.size() > 2) chan = qBound(0, list[2].toInt(), 2);
      PruneHandler::shrink(sz, chan);

      mopApplied = true;
    }
  else if (list[0] == "open")
    {
      int chan = 0;
      if (list.size() > 1) sz = qBound(1, list[1].toInt(), 255);
      if (list.size() > 2) chan = qBound(0, list[2].toInt(), 2);
      PruneHandler::open(sz, chan);

      mopApplied = true;
    }
  else if (list[0] == "close")
    {
      int chan = 0;
      if (list.size() > 1) sz = qBound(1, list[1].toInt(), 255);
      if (list.size() > 2) chan = qBound(0, list[2].toInt(), 2);
      PruneHandler::close(sz, chan);

      mopApplied = true;
    }
  else if (list[0] == "shrinkwrap")
    {
      sz = 50;
      int sz2 = 0;
      if (list.size() > 1) sz = qBound(1, list[1].toInt(), 255);
      if (list.size() > 2) sz2 = list[1].toInt();
      PruneHandler::shrinkwrap(sz, sz2);

      mopApplied = true;
    }
  else if (list[0] == "thicken")
    {
      if (list.size() > 1) sz = qBound(1, list[1].toInt(), 255);
      PruneHandler::thicken(sz);

      mopApplied = true;
    }
  else if (list[0] == "cityblock")
    {
      sz = 100;
      if (list.size() > 1) sz = qBound(1, list[1].toInt(), 255);
      PruneHandler::distanceTransform(sz);

      mopApplied = true;
    }
  else if (list[0] == "chessboard")
    {
      sz = 100;
      if (list.size() > 1) sz = qBound(1, list[1].toInt(), 255);
      PruneHandler::distanceTransform(sz, false);

      mopApplied = true;
    }
  else if (list[0] == "blend")
    {
      if (list.size() > 1) PruneHandler::setBlend(false);	  
      else PruneHandler::setBlend(true);
      m_hiresVolume->createDefaultShader();
    }
  else if (list[0] == "carve")
    {
      if (list.size() > 1)
	{
	  PruneHandler::setCarve(false);
	  //SETMOUSEBINDINGFORCARVEPAINT(NO_CLICK_ACTION);
	}
      else
	{
//	  if (!Global::useDragVolume() && m_hiresVolume->dataTexSize() > 1)
//	    QMessageBox::information(0, "", "Carving operation will be more responsive if dragVolume is selected for rendering by pressing \"l\". \"l\" toggles the use of dragVolume for rendering.");
	  PruneHandler::setPaint(false);	  
	  PruneHandler::setCarve(true);
	  //SETMOUSEBINDINGFORCARVEPAINT(SELECT);
	}
    }
  else if (list[0] == "paint")
    {
      if (list.size() > 1)
	{
	  PruneHandler::setPaint(false);	  
	  //SETMOUSEBINDINGFORCARVEPAINT(NO_CLICK_ACTION);
	}
      else
	{
//	  if (!Global::useDragVolume() && m_hiresVolume->dataTexSize() > 1)
//	    QMessageBox::information(0, "", "Painting operation will be more responsive if dragVolume is selected for rendering by pressing \"l\". \"l\" toggles the use of dragVolume for rendering.");
	  PruneHandler::setCarve(false);	  
	  PruneHandler::setPaint(true);
	  //SETMOUSEBINDINGFORCARVEPAINT(SELECT);
	}
    }
  else if (list[0] == "carverad")
    {
      float rad = -1.0f;
      float decay = -1.0f;
      if (list.size() > 1) rad = qBound(0.0f, list[1].toFloat(), 255.0f);
      if (list.size() > 2) decay = qBound(0.0f, list[2].toFloat(), rad);

      PruneHandler::setCarveRad(rad, decay);
    }
  else if (list[0] == "tag")
    {
      int tag = -1;
      if (list.size() > 1) tag = qBound(0, list[1].toInt(), 255);

      PruneHandler::setTag(tag);
    }
  else if (list[0] == "itk")
    {
      Vec subvolumeSize = m_Volume->getSubvolumeSize();
      Vec dragTextureInfo = m_Volume->getDragTextureInfo();
      Vec dmin = m_hiresVolume->volumeMin();
      int lod = dragTextureInfo.z;
      int px = subvolumeSize.x/lod;
      int py = subvolumeSize.y/lod;
      int pz = subvolumeSize.z/lod;
      uchar *prune = new uchar[px*py*pz];
      QList<Vec> hpts, pts;
      if (GeometryObjects::hitpoints()->activeCount())
	hpts = GeometryObjects::hitpoints()->activePoints();
      else
	hpts = GeometryObjects::hitpoints()->points();
      for(int i=0; i<hpts.count(); i++)
	{
	  Vec p = hpts[i];
	  p -= dmin;
	  p /= lod;
	  pts << p;
	}
      hpts.clear();
      PruneHandler::getRaw(prune,
			   1, // get channel 1
			   dragTextureInfo,
			   subvolumeSize,
			   true); // mask with R channel
      
      if (ITKSegmentation::applyITKFilter(px, py, pz, prune, pts))
	{
	  // update paint texture
	  PruneHandler::setRaw(prune,
			       0, // set channel 0 (we set BGR - that is why channel 0)
			       dragTextureInfo,
			       subvolumeSize);

	  // now remove points that were used as seeds
	  if (GeometryObjects::hitpoints()->activeCount())
	    GeometryObjects::hitpoints()->removeActive();
	  else
	    GeometryObjects::hitpoints()->clear();
	}

      delete [] prune;
    }
  else if (list[0] == "hatch")
    {
      bool flag = false;
      int xn,yn,zn,xd,yd,zd;
      xn=xd=yn=yd=zn=zd=0;
      if (list.size() > 1) flag = (list[1] == "grid");
      if (list.size() > 2) xn = qBound(0, list[2].toInt(), 255);
      if (list.size() > 3) xd = qBound(0, list[3].toInt(), 255);
      if (list.size() > 4) yn = qBound(0, list[4].toInt(), 255);
      if (list.size() > 5) yd = qBound(0, list[5].toInt(), 255);
      if (list.size() > 6) zn = qBound(0, list[6].toInt(), 255);
      if (list.size() > 7) zd = qBound(0, list[7].toInt(), 255);

      PruneHandler::pattern(flag, xn,xd, yn,yd, zn,zd);
    }
  else
    {
      QMessageBox::information(0, "Error", QString("Cannot understand %1").arg(list[0]));
    }

  if (mopApplied && !savingImages())
    updateGL();
}


//-----------------------------------
void
Viewer::setPaintMode(bool b)
{  
  m_paintMode = b;

//  if (m_paintMode)
//    QMessageBox::information(0, "", "Entering Surface Mesh Paint Mode ");
//  else
//    QMessageBox::information(0, "", "Paint Surface Mesh Mode Ended");
  
  if (m_paintMode)
    {
      createPaintMenu();
      m_paintMenuWidget->move(QCursor::pos());

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

  if (m_paintMenuWidget)
    {
      if (m_paintMode)
	m_paintMenuWidget->show();
      else
	m_paintMenuWidget->hide();
    }


}

void
Viewer::setPaintColor()
{
  Vec bg = m_paintColor;
  QColor bgc = QColor(255*bg.x,
		      255*bg.y,
		      255*bg.z);

  QColor color = DColorDialog::getColor(bgc);
  if (color.isValid())
    {      
      m_paintColor = Vec(color.redF(),
			 color.greenF(),
			 color.blueF());

      QString cname = color.name();
      QPushButton *but = (QPushButton *)(m_paintMenuWidget->layout()->itemAt(0)->widget());
      QString ss = "QPushButton {background-color:"+cname+"}";
      but->setStyleSheet(ss);
    }
}

void
Viewer::createPaintMenu()
{
  if (m_paintMenuWidget)
    return;
  
  QVBoxLayout *vbox = new QVBoxLayout();
  vbox->setSpacing(5);

  {
    QPushButton *but = new QPushButton("Color");
    but->setFont(QFont("MS Reference Sans Serif", 10));
    connect(but, SIGNAL(pressed()),
	    this, SLOT(setPaintColor()));
    vbox->addWidget(but);
  }
  
  {
    QInputDialog *dlg = new QInputDialog(this);
    dlg->setInputMode(QInputDialog::DoubleInput);
    dlg->setLabelText("Radius");
    dlg->setDoubleValue(m_paintRad);
    dlg->setDoubleRange(0.1, 50.0);
    dlg->setDoubleDecimals(1);
    dlg->setFont(QFont("MS Reference Sans Serif", 10));
    dlg->setOptions(QInputDialog::NoButtons);
    dlg->setWindowFlags(Qt::Tool);
    connect(dlg, SIGNAL(doubleValueChanged(double)),
	    this, SLOT(setPaintRadius(double)));
    vbox->addWidget(dlg);
  }

  {
    QInputDialog *dlg = new QInputDialog(this);
    dlg->setInputMode(QInputDialog::DoubleInput);
    dlg->setLabelText("Alpha");
    dlg->setDoubleValue(m_paintAlpha);
    dlg->setDoubleRange(0.01, 1.0);
    dlg->setDoubleDecimals(2);
    dlg->setDoubleStep(0.01);
    dlg->setFont(QFont("MS Reference Sans Serif", 10));
    dlg->setOptions(QInputDialog::NoButtons);
    dlg->setWindowFlags(Qt::Tool);
    connect(dlg, SIGNAL(doubleValueChanged(double)),
	    this, SLOT(setPaintAlpha(double)));
    vbox->addWidget(dlg);
  }

  {
    QInputDialog *dlg = new QInputDialog(this);
    dlg->setInputMode(QInputDialog::IntInput);
    dlg->setLabelText("Roughness");
    dlg->setIntValue(m_paintOctave);
    dlg->setIntRange(1, 10);
    dlg->setIntStep(1);
    dlg->setFont(QFont("MS Reference Sans Serif", 10));
    dlg->setOptions(QInputDialog::NoButtons);
    dlg->setWindowFlags(Qt::Tool);
    connect(dlg, SIGNAL(intValueChanged(int)),
	    this, SLOT(setPaintOctave(int)));
    vbox->addWidget(dlg);
  }


  m_paintMenuWidget = new QWidget(this);
  m_paintMenuWidget->setLayout(vbox);
  m_paintMenuWidget->setWindowFlags(Qt::Tool | Qt::CustomizeWindowHint | Qt::WindowTitleHint);
  m_paintMenuWidget->setWindowTitle("Paint Menu");
  m_paintMenuWidget->hide();
}

//-----------------------------------

//----------------------------------
// these are the functions called via menubar
#include "menuviewerfunctions.cpp" 
//----------------------------------
