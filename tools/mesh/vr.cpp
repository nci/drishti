#include "vr.h"
#include "global.h"
#include "staticfunctions.h"
#include "shaderfactory.h"

#include <QMessageBox>
#include <QtMath>
#include <QDir>

#define NEAR_CLIP 0.1f
#define FAR_CLIP 4.0f


VR::VR() : QObject()
{
  m_demoMode = false;
  
  m_hmd =0;
  m_eyeWidth = 0;
  m_eyeHeight = 0;
  m_leftBuffer = 0;
  m_rightBuffer = 0;

  m_gripActiveRight = false;
  m_gripActiveLeft = false;
  m_touchActiveRight = false;
  m_touchActiveLeft = false;
  m_touchPressActiveRight = false;
  m_touchPressActiveLeft = false;
  m_triggerActiveRight = false;
  m_triggerActiveLeft = false;
  m_triggerActiveBoth = false;

  m_leftController = vr::k_unTrackedDeviceIndexInvalid;
  m_rightController = vr::k_unTrackedDeviceIndexInvalid;
  m_leftControllerName.clear();
  m_rightControllerName.clear();
  m_leftComponentNames.clear();
  m_rightComponentNames.clear();
  m_leftRenderModels.clear();
  m_rightRenderModels.clear();
  
  m_menuTex.clear();
  m_menuMat.clear();
    
  m_modeType = 0; // rotation mode
  m_examineMode = true;

  m_showHUD = false;
  m_showSkybox = true;
  
  m_aActive = false;
  m_bActive = false;
  m_yActive = false;

  m_showLeft = true;
  m_showRight = true;
  
  m_final_xform.setToIdentity();    

  m_genDrawList = false;
  m_nextStep = 0;

  m_pointSize = 1.5;

  m_coordCen = QVector3D(0,0,0);
  m_coordScale = 1.0;
  m_scaleFactor = 1.0;
  m_teleportScale = 1.0;
  m_flightSpeed = 1.0;
  m_speedDamper = 0.1;
  m_currTeleportNumber = -1;

  m_flyTimer.setSingleShot(true);

  m_teleportTimer = new QTimer();
  connect(m_teleportTimer, SIGNAL(timeout()),
	  this, SLOT(updateTeleport()));
  m_teleports.clear();

  m_menuPanels = m_leftMenu.menuList();
  m_currPanel = -1;
  m_leftMenu.setCurrentMenu("none");

  m_edges = true;
  m_softShadows = true;

  m_autoRotate = false;

  m_clip = false;
  m_clipType = 0; // normal is Z
  
  connect(&m_leftMenu, SIGNAL(resetModel()),
	  this, SLOT(resetModel()));

//  connect(&m_leftMenu, SIGNAL(updateScale(int)),
//	  this, SLOT(updateScale(int)));

  connect(&m_leftMenu, SIGNAL(updateEdges(bool)),
	  this, SLOT(updateEdges(bool)));

  connect(&m_leftMenu, SIGNAL(updateSoftShadows(bool)),
	  this, SLOT(updateSoftShadows(bool)));

  connect(&m_leftMenu, SIGNAL(toggle(QString, float)),
	  this, SLOT(toggle(QString, float)));

  m_maxVolumeSeries = 1;

  m_allowMovement = true;
  m_grabMode = false;
}

void
VR::setDemoMode(bool d)
{
  m_grabMode = false;
  m_demoMode = d;
  if (m_demoMode)
    QMessageBox::information(0, "Demo Mode", "Demo Mode Selected");
  else
    QMessageBox::information(0, "Demo Mode", "Demo Mode Switched off");
}

void
VR::toggle(QString attrib, float val)
{
  QString attr = attrib.toLower().trimmed();
  
  if (attr == "reset")
    {
      resetModel();
      return;
    }
  
  if (attr == "scale")
    {
      scaleModel(val);
      return;
    }
  else if (attr == "edges")
    {
      emit setEdges(val);
    }
  else if (attr == "softness")
    {
      emit setSoftness(val);
    }
  else if (attr == "bright")
    {
      emit setGamma(val);
    }
  else if (attr == "explode")
    {
      emit setExplode(val);
    }
  else if (attr == "paint")
    {
      emit setPaintMode(val > 0.5);
    }
  else if (attr == "grab")
    {      
      m_grabMode = (val > 0);
      return;
    }


//  if (attr == "clip")
//    {
//      m_clip = (val > 0);
//
//      if (!m_clip)
//	emit removeClip();
//    }
//  else if (attr == "examine mode")
//    {
//      m_examineMode = (val > 0);
//    }
//  else if (attr == "translate")
//    {
//      m_modeType = val;
//    }
  
  
}

void VR::updateEdges(bool b) { m_edges = b; }
void VR::updateSoftShadows(bool b) { m_softShadows = b; }

void VR::setExamineMode(bool b) { m_examineMode = b; }
			
void
VR::setDataDir(QString d)
{  
  m_dataDir = QFileInfo(d).absolutePath();
  m_currTeleportNumber = -1;

  m_menuPanels = m_leftMenu.menuList();
  m_currPanel = -1;
  m_leftMenu.setCurrentMenu("none");

  for(int i=0; i<CaptionWidget::widgets.count(); i++)
    delete CaptionWidget::widgets[i];
  CaptionWidget::widgets.clear();

  CaptionWidget *capWidget = new CaptionWidget();
  capWidget->generateCaption("hud", QFileInfo(d).baseName());  
}

VR::~VR()
{
  shutdown();
}

void
VR::shutdown()
{
  delete m_leftBuffer;
  delete m_rightBuffer;
  delete m_resolveBuffer;
  
  if (m_hmd)
    {
      vr::VR_Shutdown();
        m_hmd = 0;
    }


  for( int i=0; i<m_vecRenderModels.count(); i++)
    delete m_vecRenderModels[i];
  m_vecRenderModels.clear();
	
}

void
VR::resetModel()
{
  initModel(m_coordMin, m_coordMax);
  m_leftMenu.setValue("scale", m_scaleFactor/m_coordScale-1.0);
}

void
VR::initModel(Vec bmin, Vec bmax)
{
  QVector3D cmin(bmin.x, bmin.y, bmin.z);
  QVector3D cmax(bmax.x, bmax.y, bmax.z);
  initModel(cmin, cmax);
}

void
VR::initModel(QVector3D cmin, QVector3D cmax)
{
  QVector3D box = cmax - cmin;
  m_coordScale = 1.0/qMax(box.x(), qMax(box.y(), box.z()));

  m_coordMin = cmin;
  m_coordMax = cmax;
  m_coordCen = (cmin+cmax)/2;

  m_scaleFactor = m_coordScale;
  m_flightSpeed = 1.0;
  m_teleportScale = 1.0;

  m_final_xform = initXform(1, 0, 0, 0);
  //m_final_xform = initXform(1, 0, 0, -90);

  genEyeMatrices();


  QMatrix4x4 mat = m_matrixDevicePose[m_leftController];
  m_leftMenu.generateHUD(mat);
}

QMatrix4x4
VR::initXform(float x, float y, float z, float a)
{
  Vec cmin, cmax;
  cmin = Vec(m_coordMin.x(),
	     m_coordMin.y(),
	     m_coordMin.z());

  cmax = Vec(m_coordMax.x(),
	     m_coordMax.y(),
	     m_coordMax.z());

  m_sceneCen = (cmax+cmin)/2;

  m_camPos = Vec(0, -1.2, 0); // bring it to eye level
  m_camRot = Quaternion(Vec(x, y, z), qDegreesToRadians(a));
  
  m_camRotLocal = Quaternion();

  QMatrix4x4 xform = modelViewFromCamera();
  
  return xform;
}


QMatrix4x4
VR::modelViewFromCamera()
{
  QMatrix4x4 xform;
  xform.setToIdentity();
  Vec axis = m_camRot.axis();
  xform.rotate(qRadiansToDegrees(m_camRot.angle()), axis.x, axis.y, axis.z);
  xform.translate(m_camPos.x, m_camPos.y, m_camPos.z);

  xform = xform.inverted();
  
  QMatrix4x4 scale_xform;
  scale_xform.setToIdentity();  
  scale_xform.scale(m_scaleFactor);
  scale_xform.translate(-m_coordCen);

  xform = xform * scale_xform;

  return xform;
}


void
VR::initVR()
{
  vr::EVRInitError error = vr::VRInitError_None;

  //-----------------------------
  m_hmd = vr::VR_Init(&error, vr::VRApplication_Scene);
  if (error != vr::VRInitError_None)
    {
      m_hmd = 0;
      
      QString message = vr::VR_GetVRInitErrorAsEnglishDescription(error);
      qCritical() << message;
      QMessageBox::critical(0, "Unable to init VR", message);
      //exit(0);
      return;
    }
  //-----------------------------
    
  m_pRenderModels = (vr::IVRRenderModels *)vr::VR_GetGenericInterface( vr::IVRRenderModels_Version, &error );


  genEyeMatrices();


  //-----------------------------
  // setup frame buffers for eyes
  m_hmd->GetRecommendedRenderTargetSize(&m_eyeWidth, &m_eyeHeight);
  
  QOpenGLFramebufferObjectFormat buffFormat;
  buffFormat.setAttachment(QOpenGLFramebufferObject::Depth);
  buffFormat.setInternalTextureFormat(GL_RGBA16F);
  //buffFormat.setSamples(8);
  buffFormat.setSamples(0);
  m_leftBuffer = new QOpenGLFramebufferObject(m_eyeWidth, m_eyeHeight, buffFormat);
  m_rightBuffer = new QOpenGLFramebufferObject(m_eyeWidth, m_eyeHeight, buffFormat);
  
  QOpenGLFramebufferObjectFormat resolveFormat;
  resolveFormat.setInternalTextureFormat(GL_RGBA);
  buffFormat.setSamples(0);
  m_resolveBuffer = new QOpenGLFramebufferObject(m_eyeWidth*2, m_eyeHeight, resolveFormat);
  //-----------------------------
  

  //-----------------------------
  // turn on compositor
  if (!vr::VRCompositor())
    {
      QString message = "Compositor initialization failed. See log file for details";
      qCritical() << message;
      QMessageBox::critical(0, "Unable to init VR", message);
      exit(0);
    }
  //-----------------------------
  
  //#ifdef QT_DEBUG
  vr::VRCompositor()->ShowMirrorWindow();
  //#endif

  createShaders();

  if (!getControllers())
    {
      QMessageBox::information(0, "", "Please switch on both the controllers");
      m_hmd = 0;      
      return;
    }

  //setupRenderModels();

  buildAxesVB();

  generateButtonInfo();
  
// loads a cubemap texture from 6 individual texture faces
// order:
// +X (right)
// -X (left)
// +Y (top)
// -Y (bottom)
// +Z (front) 
// -Z (back)
  QStringList clist;
  clist << ":/images/grid.jpg";
  clist << ":/images/grid.jpg";
  clist << ":/images/grid.jpg";
  clist << ":/images/grid.jpg";
  clist << ":/images/grid.jpg";
  clist << ":/images/grid.jpg";

  m_skybox.loadCubemap(clist);
}

bool
VR::getControllers()
{
  m_leftController = m_hmd->GetTrackedDeviceIndexForControllerRole(vr::ETrackedControllerRole::TrackedControllerRole_LeftHand);
  
  m_rightController = m_hmd->GetTrackedDeviceIndexForControllerRole(vr::ETrackedControllerRole::TrackedControllerRole_RightHand);


  // setup the render models for controllers
  if (m_hmd->GetControllerState(m_leftController,
				 &m_stateLeft,
				sizeof(m_stateLeft)) &&
      m_hmd->GetControllerState(m_rightController,
				&m_stateRight,
				sizeof(m_stateRight)))
    
    {
      setupRenderModels();
      return true;
    }
  
  return false;
}
//void
//VR::getControllers()
//{
//  m_leftControllerName.clear();
//  m_rightControllerName.clear();
//  m_leftComponentNames.clear();
//  m_rightComponentNames.clear();
//  m_leftRenderModels.clear();
//  m_rightRenderModels.clear();
//  
//  m_leftController = m_hmd->GetTrackedDeviceIndexForControllerRole(vr::ETrackedControllerRole::TrackedControllerRole_LeftHand);
//  
//  m_rightController = m_hmd->GetTrackedDeviceIndexForControllerRole(vr::ETrackedControllerRole::TrackedControllerRole_RightHand);
//}


void
VR::ProcessVREvent( const vr::VREvent_t & event )
{
  switch( event.eventType )
    {
    case vr::VREvent_TrackedDeviceActivated:
      {
	if (m_leftController == event.trackedDeviceIndex)
	  setupRenderModelForTrackedDevice( 0, event.trackedDeviceIndex );

	if (m_rightController == event.trackedDeviceIndex)
	  setupRenderModelForTrackedDevice( 1, event.trackedDeviceIndex );

	//QMessageBox::information(0, "", QString("Device %1 attached.").arg(event.trackedDeviceIndex));
      }
      break;
    case vr::VREvent_TrackedDeviceDeactivated:
      {
	//QMessageBox::information(0, "", QString("Device %1 detached.").arg(event.trackedDeviceIndex));
      }
      break;
    case vr::VREvent_TrackedDeviceUpdated:
      {
	//QMessageBox::information(0, "", QString("Device %1 updated.").arg(event.trackedDeviceIndex));
      }
      break;
    }
}

void
VR::updatePoses()
{
    vr::VRCompositor()->WaitGetPoses(m_trackedDevicePose, vr::k_unMaxTrackedDeviceCount, NULL, 0);

    for (unsigned int i=0; i<vr::k_unMaxTrackedDeviceCount; i++)
    {
        if (m_trackedDevicePose[i].bPoseIsValid)
        {
            m_matrixDevicePose[i] =  vrMatrixToQt(m_trackedDevicePose[i].mDeviceToAbsoluteTracking);
        }
    }

    if (m_trackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid)
    {
        m_hmdPose = m_matrixDevicePose[vr::k_unTrackedDeviceIndex_Hmd].inverted();
    }
}

bool
VR::updateInput()
{
  vr::VREvent_t event;
  
  //----------------------------------
  // Process SteamVR events
  while(m_hmd->PollNextEvent(&event, sizeof(event)))
    {
      ProcessVREvent( event );
    }
  //----------------------------------

  //----------------------------------
  // Process SteamVR controller state
  for( vr::TrackedDeviceIndex_t unDevice = 0;
       unDevice < vr::k_unMaxTrackedDeviceCount;
       unDevice++ )
    {
      vr::VRControllerState_t state;
      if( m_hmd->GetControllerState( unDevice, &state, sizeof(state) ) )
	{
	  m_rbShowTrackedDevice[ unDevice ] = state.ulButtonPressed == 0;
	}
    }
  //----------------------------------
  
  
  if (!m_hmd->GetControllerState(m_leftController,
				&m_stateLeft,
				sizeof(m_stateLeft)) ||
      !m_hmd->GetControllerState(m_rightController,
				&m_stateRight,
				sizeof(m_stateRight)))
    {
      if (!getControllers())
	return false;
    }
  

  bool bActive = isButtonTriggered(m_stateRight, vr::k_EButton_ApplicationMenu);
  bool aActive = isButtonTriggered(m_stateRight, vr::k_EButton_A);
// -----------------------
// left X/Y events
  if (!m_aActive && aActive)
    aButtonPressed();
  if (m_aActive && !aActive)
    aButtonReleased();

  if (!m_bActive && bActive)
    bButtonPressed();
  if (m_bActive && !bActive)
    bButtonReleased();
// -----------------------

  
  bool yActive = isButtonTriggered(m_stateLeft, vr::k_EButton_ApplicationMenu);


  bool leftTriggerActive = isTriggered(m_stateLeft);
  bool rightTriggerActive = isTriggered(m_stateRight);

  bool leftTouchActive = isTouched(m_stateLeft);
  bool rightTouchActive = isTouched(m_stateRight);

  bool leftTouchPressActive = isTouchPressed(m_stateLeft);
  bool rightTouchPressActive = isTouchPressed(m_stateRight);

  bool leftGripActive = isGripped(m_stateLeft);
  bool rightGripActive = isGripped(m_stateRight);

// -----------------------
// left Y events
  if (!m_yActive && yActive)
    yButtonPressed();
  if (m_yActive && !yActive)
    yButtonReleased();
// -----------------------


// -----------------------
// press events
  if (!m_triggerActiveBoth)
    {
      if (leftTriggerActive && !m_triggerActiveLeft)
	{
	  leftTriggerPressed();
	  return true;
	}

      if (rightTriggerActive && !m_triggerActiveRight)
	{
	  rightTriggerPressed();
	  return true;
	}
    }


  // touch
  if (leftTouchActive && !m_touchActiveLeft)
    leftTouched();

  if (rightTouchActive && !m_touchActiveRight)
    rightTouched();


  // touch press
  if (leftTouchPressActive && !m_touchPressActiveLeft)
    leftTouchPressed();

  if (rightTouchPressActive && !m_touchPressActiveRight)
    rightTouchPressed();


  // grip
  if (leftGripActive && !m_gripActiveLeft)
    leftGripPressed();

  if (rightGripActive && !m_gripActiveRight)
    rightGripPressed();

// -----------------------

  
  
// -----------------------
// move events
  if (m_triggerActiveBoth)
    bothTriggerMove();
  else
    {
      if (m_triggerActiveLeft && !m_gripActiveLeft)
	leftTriggerMove();
      if (m_triggerActiveRight && !m_gripActiveRight)
	rightTriggerMove();
    }


  if (!m_triggerActiveLeft) // check map/menu only if left trigger is not pressed
    {
      checkOptions(0);
    }

  //touch
  if (leftTouchActive && m_touchActiveLeft)
    leftTouchMove();

  if (rightTouchActive && m_touchActiveRight)
    rightTouchMove();


  //touch press
  if (leftTouchPressActive && m_touchPressActiveLeft)
    leftTouchPressMove();

  if (rightTouchPressActive && m_touchPressActiveRight)
    rightTouchPressMove();


  // grip
  if (leftGripActive && m_gripActiveLeft)
    leftGripMove();

  if (rightGripActive && m_gripActiveRight)
    rightGripMove();

// -----------------------


// -----------------------
// release events
  if (m_triggerActiveBoth)
    {
      if (!leftTriggerActive || !rightTriggerActive)
	bothTriggerReleased();
    }
  else
    {      
      if (!leftTriggerActive && m_triggerActiveLeft)
	leftTriggerReleased();

      if (!rightTriggerActive && m_triggerActiveRight)
	rightTriggerReleased();

    }


  // touch
  if (!leftTouchActive && m_touchActiveLeft)
    leftTouchReleased();

  if (!rightTouchActive && m_touchActiveRight)
    rightTouchReleased();


  // touch press
  if (!leftTouchPressActive && m_touchPressActiveLeft)
    leftTouchPressReleased();

  if (!rightTouchPressActive && m_touchPressActiveRight)
    rightTouchPressReleased();


  // grip
  if (!leftGripActive && m_gripActiveLeft)
    leftGripReleased();

  if (!rightGripActive && m_gripActiveRight)
    rightGripReleased();

  // -----------------------


  // -----------------------
  if (m_autoRotate)
    {
      m_camRot *= m_camRotLocal;      
      m_final_xform = modelViewFromCamera();
    }
  // -----------------------

  
  
  return true;
}
//---------------------------------------


void
VR::rightProbe()
{
  float ahead = -0.1;
  QMatrix4x4 mat = m_matrixDevicePose[m_rightController];
  QVector3D probe = QVector3D(m_final_xformInverted.map(mat * QVector4D(0,0,ahead,1)));
  emit pointProbe(probe);

  //QTimer::singleShot(2000, this, SLOT(rightProbe()));
}

void
VR::probeMeshName(QString text)
{
  if (text.isEmpty())
    emit resetProbe();
  
  QFont font = QFont("Helvetica", 48);
  QColor color(255, 255, 255);
  QColor bgcolor(120, 100, 100); 

  QImage img = StaticFunctions::renderText(text,
					   font,
					   bgcolor,
					   color, true).mirrored(false, true);
  StaticFunctions::loadTexture(img, m_menuTex[m_menuMeshNameIdx]);

  float twd = img.width();
  float tht = img.height();
  float mx = qMax(twd, tht);
  twd/=mx; tht/=mx;
  {
    float frc = 0.015/tht;
    twd *= frc;
    tht *= frc;
  }
  QMatrix4x4 txtMat;
  txtMat.setToIdentity();
  txtMat.translate(-twd/2, 0.01, -0.01);
  txtMat.rotate(30, 1,0,0);
  txtMat.scale(twd, 1, tht);

  m_menuMat[m_menuMeshNameIdx] = txtMat;
}

//---------------------------------------
//---------------------------------------
void
VR::bButtonPressed()
{
  m_bActive = true;
}
void
VR::bButtonReleased()
{
  m_bActive = false;
  nextTeleport();
}
void
VR::aButtonPressed()
{
  m_aActive = true;
}
void
VR::aButtonReleased()
{
  m_aActive = false;
  prevTeleport();
}


void
VR::yButtonPressed()
{
  m_yActive = true;

  m_showHUD = !m_showHUD;

  if (!m_showHUD)
    m_leftMenu.setCurrentMenu("none");
  else
    {
      QMatrix4x4 mat = m_matrixDevicePose[vr::k_unTrackedDeviceIndex_Hmd];
      //QMatrix4x4 mat = m_matrixDevicePose[m_leftController];
      m_leftMenu.generateHUD(mat);
      m_leftMenu.setCurrentMenu("01");
    }
}
void
VR::yButtonReleased()
{
  m_yActive = false;
}
//---------------------------------------
//---------------------------------------


//---------------------------------------
//---------------------------------------
void
VR::leftGripPressed()
{
  m_gripActiveLeft = true;
}
void
VR::leftGripMove()
{
}
void
VR::leftGripReleased()
{
  m_gripActiveLeft = false;
}
//---------------------------------------


//---------------------------------------
void
VR::rightGripPressed()
{
  m_gripActiveRight = true;
  rightProbe();

  m_startTranslate = getPosition(m_rightController);
  QMatrix4x4 xform;
  xform.setToIdentity();
  Vec axis = m_camRot.axis();
  xform.rotate(qRadiansToDegrees(m_camRot.angle()), axis.x, axis.y, axis.z);
  m_startTranslate = xform.map(m_startTranslate);  
}
void
VR::rightGripMove()
{
  if (!m_grabMode)
    return;
  
  if (m_triggerActiveRight)
    {
      QVector3D pos  = getPosition(m_rightController);
      QMatrix4x4 xform;
      xform.setToIdentity();
      Vec axis = m_camRot.axis();
      xform.rotate(qRadiansToDegrees(m_camRot.angle()), axis.x, axis.y, axis.z);
      pos = xform.map(pos);
      pos = (pos - m_startTranslate)/m_scaleFactor;
      emit probeMeshMove(pos);
    }  
}
void
VR::rightGripReleased()
{
  m_gripActiveRight = false;
  probeMeshName("");
}
//---------------------------------------
//---------------------------------------


//---------------------------------------
//---------------------------------------
void
VR::leftTriggerPressed()
{
  m_autoRotate = false;
  
  m_triggerActiveLeft = true;
  if (m_triggerActiveRight)
    {
      m_triggerActiveLeft = false;
      m_triggerActiveRight = false;

      m_final_xformInverted = m_final_xform.inverted();      

      bothTriggerPressed();
      return;
    }
		  
  m_startTranslate =  getPosition(m_leftController);
  m_prevPosL = m_startTranslate;

  m_camRotLocal = Quaternion();
}
void
VR::leftTriggerMove()
{  
  QVector3D pos, posL;
  posL =  getPosition(m_leftController);
  pos = (posL - m_prevPosL)*qSqrt(m_flightSpeed);
  m_camPos -= Vec(pos.x(), pos.y(), pos.z());

  m_final_xform = modelViewFromCamera();

  m_prevPosL = posL;
}
void
VR::leftTriggerReleased()
{
  m_triggerActiveLeft = false;
}
//---------------------------------------


//---------------------------------------
void
VR::rightTriggerPressed()
{  
  m_autoRotate = false;

  m_triggerActiveRight = true;

  if (m_triggerActiveLeft)
    {
      m_triggerActiveLeft = false;
      m_triggerActiveRight = false;

      m_final_xformInverted = m_final_xform.inverted();      

      bothTriggerPressed();
      return;
    }
  
  m_startTranslate =  getPosition(m_rightController);

  if (m_examineMode && !m_grabMode)
    {
      m_rotCenter = m_final_xform.map(m_coordCen);
      QVector3D p0 = getPosition(m_rightController);
      m_prevDirection = (p0 - m_rotCenter).normalized();
      m_rotQuat = QQuaternion(1,0,0,0); // identity quaternion
      m_ctrlerMoveTime.start();

      m_startTranslate = p0;
    }

  m_camRotLocal = Quaternion();
}
void
VR::rightTriggerMove()
{
  //----------------------
  // check some options in the menu
  //if (pos.length() < 0.1)
  {
    if (!m_triggerActiveLeft) // check menu only if left trigger is not pressed
      {
	if (checkOptions(1) > -1) // check only some options
	  return;
      }
  }    

  if (m_examineMode && !m_grabMode)
    {
      QVector3D p0 = getPosition(m_rightController);
      QVector3D cdirec = (p0 - m_rotCenter).normalized();

      Vec to = Vec(cdirec.x(), cdirec.y(), cdirec.z());
      Vec from = Vec(m_prevDirection.x(),
		     m_prevDirection.y(),
		     m_prevDirection.z());

      m_camRotLocal = Quaternion(to, from);
      m_camRotLocal = Quaternion(m_camRotLocal.axis(),
				 m_camRotLocal.angle()*qSqrt(m_flightSpeed));
      m_camRot *= m_camRotLocal;
      
      m_final_xform = modelViewFromCamera();

      m_prevDirection = cdirec;      
    }
  else
    {
      QVector3D pos;
      pos =  getPosition(m_rightController);
      pos = (pos - m_startTranslate) * qSqrt(m_flightSpeed);
    }
  
}
void
VR::rightTriggerReleased()
{
  m_triggerActiveRight = false;

  //----------------------
  // trigger teleport only if not used to move the model
  QVector3D pos =  getPosition(m_rightController);
  pos = (pos - m_startTranslate);
  if (pos.length() < 0.1)
    {
      if (!m_triggerActiveLeft) // check map/menu only if left trigger is not pressed
	{
	  if (checkOptions(2) > -1)
	    return;			     
	}
    }
  //----------------------

  
  //----------------------
  m_autoRotate = false;
  if (m_examineMode && !m_grabMode)
    {
      int delay = m_ctrlerMoveTime.restart();
      if (delay > 100 && delay < 500) // if controller moved quickly and released
	m_autoRotate = true;
    }
  //----------------------
}
//---------------------------------------
//---------------------------------------



//---------------------------------------
//---------------------------------------
void
VR::leftTouched()
{
  m_autoRotate = false;

  m_touchActiveLeft = true;

  m_startTouchX = m_stateLeft.rAxis[0].x;
  m_startTouchY = m_stateLeft.rAxis[0].y;
}
void
VR::leftTouchMove()
{
  m_touchX = m_stateLeft.rAxis[0].x;
  m_touchY = m_stateLeft.rAxis[0].y;

  // swipe
  if (m_stateLeft.rAxis[0].x - m_startTouchX > 0.5)
    emit nextVolume();
  else if (m_startTouchX - m_stateLeft.rAxis[0].x > 0.5)
    emit prevVolume();
}
void
VR::leftTouchReleased()
{
  m_touchActiveLeft = false;
}
//---------------------------------------


//---------------------------------------
void
VR::rightTouched()
{
  m_autoRotate = false;

  m_touchActiveRight = true;
  
  m_startTouchX = m_stateRight.rAxis[0].x;
  m_startTouchY = m_stateRight.rAxis[0].y;

  // for flight 
  QMatrix4x4 mat = m_matrixDevicePose[m_rightController];    
  QVector4D center = mat * QVector4D(0,0,0,1);
  QVector4D point = mat * QVector4D(0,0,1,1);
  m_rcDir = QVector3D(center-point);
  m_rcDir.normalize();
}
void
VR::rightTouchMove()
{
  if (m_touchPressActiveRight)
    {
      rightTouchReleased();
      return;
    }
  
  m_touchX = m_stateRight.rAxis[0].x;
  m_touchY = m_stateRight.rAxis[0].y;

//  if (m_flyMode)
//    {
//      // *************************
//      // fly
//      float acc = (m_touchY-m_startTouchY);
//      
//      QMatrix4x4 mat = m_matrixDevicePose[m_rightController];    
//      QVector4D center = mat * QVector4D(0,0,0,1);
//      QVector4D point = mat * QVector4D(0,0,1,1);
//      QVector3D moveD = QVector3D(center-point);
//      moveD.normalize();
//      
//      QVector3D move = moveD;
//      move *= acc;
//      move *= qSqrt(m_scaleFactor)*m_speedDamper;
//      
//      m_camPos += Vec(move.x(), move.y(), move.z());
//
//      {
//	Vec to = Vec(m_rcDir.x(), 0, m_rcDir.z());
//	Vec from = Vec(moveD.x(), 0, moveD.z());
//	m_camRotLocal = Quaternion(from, to);
//	m_camRotLocal = Quaternion(m_camRotLocal.axis(), -m_camRotLocal.angle()*0.005);
//	m_camRot *= m_camRotLocal;
//
//	QVector3D v3 = vrHmdPosition() - m_final_xform.map(m_coordCen);
//	Vec pp(v3.x(),v3.y(),v3.z());
//	m_camPos += (pp-m_camRotLocal.rotate(pp));
//      }
//      
//      m_final_xform = modelViewFromCamera();
//      m_final_xformInverted = m_final_xform.inverted();
//      
//      genEyeMatrices();
//      // *************************
//    }
}

void
VR::rightTouchReleased()
{
  m_touchActiveRight = false;
}
//---------------------------------------
//---------------------------------------



//---------------------------------------
//---------------------------------------
void
VR::leftTouchPressed()
{
  m_touchPressActiveLeft = true;

  m_startTouchX = m_stateLeft.rAxis[0].x;
  m_startTouchY = m_stateLeft.rAxis[0].y;

////  if (m_startTouchX >= 0.0)
////    emit nextVolume();
////  else
////    emit prevVolume();
////
////  //if (m_touchPressActiveRight)
////  // save teleport when right touched and left pressed
////  if (m_touchActiveRight)
////    {
////      if (!m_demoMode)
////	saveTeleportNode();
////    }
////  else
////    {
////      if (m_startTouchY >= 0.0)
////	nextTeleport();
////      else
////	prevTeleport();
////    }
}
void
VR::leftTouchPressMove()
{
  m_touchX = m_stateLeft.rAxis[0].x;
  m_touchY = m_stateLeft.rAxis[0].y;
}
void
VR::leftTouchPressReleased()
{
  m_touchPressActiveLeft = false;
}
//---------------------------------------


//---------------------------------------
void
VR::rightTouchPressed()
{
  m_touchPressActiveRight = true;
}
void
VR::rightTouchPressMove()
{
  // left clip plane activated when left touch pad is touched
  float ahead = -0.05;
  
  {
    QMatrix4x4 mat = m_matrixDevicePose[m_rightController];
    QVector3D cen, p1, p2;
    cen= QVector3D(m_final_xformInverted.map(mat * QVector4D(0,0,0,1)));
    p1 = QVector3D(m_final_xformInverted.map(mat * QVector4D(1,0,0,1)));
    p2 = QVector3D(m_final_xformInverted.map(mat * QVector4D(0,0,ahead,1)));
    emit modifyClip(0, cen, p1, p2);
  }
}
void
VR::rightTouchPressReleased()
{
  m_touchPressActiveRight = false;
}
//---------------------------------------
//---------------------------------------

void
VR::scaleModel(int val)
{
  float sf;
  //sf = (val*0.02f+1.0f)*m_coordScale/m_scaleFactor;
  sf = (val*0.04f+1.0f)*m_coordScale/m_scaleFactor;

  m_scaleFactor *= sf;
  m_flightSpeed *= sf;
  
  m_final_xform = modelViewFromCamera();

  genEyeMatrices();
}


//---------------------------------------
//---------------------------------------
void
VR::bothTriggerPressed()
{
  m_triggerActiveBoth = true;

  QVector3D posL =  getPosition(m_leftController);
  QVector3D posR =  getPosition(m_rightController);

  m_lrDist = posL.distanceToPoint(posR);

  m_prevPosL = posL;
  m_prevPosR = posR;
  m_prevDirection = (m_prevPosL - m_prevPosR).normalized();

  m_scaleCenter = (posL + posR)*0.5;
  
  m_prevScaleFactor = m_scaleFactor;
  m_prevFlightSpeed = m_flightSpeed;

  m_rotQuat = QQuaternion(1,0,0,0); // identity quaternion

  m_startPS = 1;
}

void
VR::bothTriggerMove()
{
  QVector3D posL =  getPosition(m_leftController);
  QVector3D posR =  getPosition(m_rightController);

  float newDist = posL.distanceToPoint(posR);

  float sf = newDist/m_lrDist;

  // make sure that the new scaling is not less than the original scale
  if (m_prevScaleFactor * sf < m_coordScale)
    {
      sf = m_coordScale/m_prevScaleFactor;
    }
  else if (m_prevScaleFactor * sf > 5*m_coordScale)
    {
      sf = 5*m_coordScale/m_prevScaleFactor;
    }

  //--------------------------
  // translation
  {
    QVector3D pos = ( (posL+posR) - (m_prevPosL+m_prevPosR) )/2;
    m_camPos -= (Vec(pos.x(), pos.y(), pos.z())*qSqrt(m_flightSpeed));
    m_prevPosL = posL;
    m_prevPosR = posR;
  }
  //--------------------------
  
  //--------------------------
  // rotation
  {
    QVector3D cdirec = (posL - posR).normalized();
    Vec to = Vec(cdirec.x(), cdirec.y(), cdirec.z());
    Vec from = Vec(m_prevDirection.x(),
		   m_prevDirection.y(),
		   m_prevDirection.z());
    m_prevDirection = cdirec;

    m_camRotLocal = Quaternion(to, from);
    m_camRot *= m_camRotLocal;
  }
  //--------------------------
  
  //--------------------------
  // move camera towards or away from scene center relative to scaling
  {
    QVector3D v3 = m_scaleCenter - m_final_xform.map(m_coordCen);
    Vec v(v3.x(),v3.y(),v3.z());
    m_camPos += (sf/m_startPS-1.0)*v;
    m_startPS = sf;
  }
  //--------------------------
  

  //--------------------------
  {
    QVector3D v3 = m_scaleCenter - m_final_xform.map(m_coordCen);
    Vec pp(v3.x(),v3.y(),v3.z());
    m_camPos += (pp-m_camRotLocal.rotate(pp));
  }
  //--------------------------

  
  //--------------------------  
  m_scaleFactor = m_prevScaleFactor * sf;
  m_flightSpeed = m_prevFlightSpeed * sf;
  
  m_final_xform = modelViewFromCamera();

  genEyeMatrices();
  //--------------------------  

  
  m_leftMenu.setValue("scale", (m_scaleFactor/m_coordScale-1.0)*25);
}

void
VR::bothTriggerReleased()
{
  m_triggerActiveBoth = false;
}
//---------------------------------------
//---------------------------------------



//---------------------------------------
bool
VR::isButtonTriggered(vr::VRControllerState_t &state, vr::EVRButtonId button)
{
  return (state.ulButtonPressed &
	  vr::ButtonMaskFromId(button));
}

bool
VR::isTriggered(vr::VRControllerState_t &state)
{
  return (state.ulButtonPressed &
	  vr::ButtonMaskFromId(vr::k_EButton_SteamVR_Trigger));
}

bool
VR::isGripped(vr::VRControllerState_t &state)
{
  return (state.ulButtonPressed &
	  vr::ButtonMaskFromId(vr::k_EButton_Grip));
}

bool
VR::isTouched(vr::VRControllerState_t &state)
{
  return (state.ulButtonTouched &
	  vr::ButtonMaskFromId(vr::k_EButton_SteamVR_Touchpad));
}

bool
VR::isTouchPressed(vr::VRControllerState_t &state)
{
  return (state.ulButtonPressed &
	  vr::ButtonMaskFromId(vr::k_EButton_SteamVR_Touchpad));
}



QMatrix4x4
VR::currentViewProjection(vr::Hmd_Eye eye)
{
    if (eye == vr::Eye_Left)
      return m_leftProjection * m_leftPose * m_hmdPose;
    else
      return m_rightProjection * m_rightPose * m_hmdPose;
}

QMatrix4x4
VR::viewProjection(vr::Hmd_Eye eye)
{
  m_final_xformInverted = m_final_xform.inverted();

  return currentViewProjection(eye)*m_final_xform;
}

QMatrix4x4
VR::modelViewNoHmd()
{
  return m_final_xform;
}

QMatrix4x4
VR::modelView()
{
  QMatrix4x4 model;
  model = m_hmdPose;

  return model*m_final_xform;
}

QMatrix4x4
VR::modelView(vr::Hmd_Eye eye)
{
  QMatrix4x4 model;
  if (eye == vr::Eye_Left)
     model = m_leftPose * m_hmdPose;
  else
    model = m_rightPose * m_hmdPose;

  return model*m_final_xform;
}



QMatrix4x4
VR::vrMatrixToQt(const vr::HmdMatrix34_t &mat)
{
  return QMatrix4x4(
		    mat.m[0][0], mat.m[0][1], mat.m[0][2], mat.m[0][3],
		    mat.m[1][0], mat.m[1][1], mat.m[1][2], mat.m[1][3],
		    mat.m[2][0], mat.m[2][1], mat.m[2][2], mat.m[2][3],
		    0.0,         0.0,         0.0,         1.0f
		    );
}

QMatrix4x4
VR::vrMatrixToQt(const vr::HmdMatrix44_t &mat)
{
  return QMatrix4x4(
		    mat.m[0][0], mat.m[0][1], mat.m[0][2], mat.m[0][3],
		    mat.m[1][0], mat.m[1][1], mat.m[1][2], mat.m[1][3],
		    mat.m[2][0], mat.m[2][1], mat.m[2][2], mat.m[2][3],
		    mat.m[3][0], mat.m[3][1], mat.m[3][2], mat.m[3][3]
		    );
}

QString
VR::getTrackedDeviceString(vr::TrackedDeviceIndex_t device,
			   vr::TrackedDeviceProperty prop,
			   vr::TrackedPropertyError *error)
{
    uint32_t len = m_hmd->GetStringTrackedDeviceProperty(device, prop, NULL, 0, error);
    if(len == 0)
        return "";

    char *buf = new char[len];
    m_hmd->GetStringTrackedDeviceProperty(device, prop, buf, len, error);

    QString result = QString::fromLocal8Bit(buf);
    delete [] buf;

    return result;
}

QMatrix4x4 VR::matrixDevicePoseLeft() { return m_matrixDevicePose[m_leftController]; }
QMatrix4x4 VR::matrixDevicePoseRight() { return m_matrixDevicePose[m_rightController]; }

QVector3D
VR::getModelSpacePosition(vr::TrackedDeviceIndex_t trackedDevice)
{
  QMatrix4x4 mat = m_matrixDevicePose[trackedDevice];    
  QVector4D center = mat * QVector4D(0,0,0,1);
  QVector4D cpos = m_final_xformInverted.map(center);
  QVector3D pos(cpos.x(),cpos.y(),cpos.z());
  return pos;
}

Vec
VR::getModelSpace(Vec v)
{
  QVector4D cv = m_final_xformInverted.map(QVector4D(v.x, v.y, v.z, 1));
  return Vec(cv.x(), cv.y(), cv.z());
}
QVector3D
VR::getModelSpace(QVector3D v)
{
  return QVector3D(m_final_xformInverted.map(QVector4D(v.x(), v.y(), v.z(), 1)));
}

QMatrix4x4
VR::hmdMVP(vr::Hmd_Eye eye)
{
  QMatrix4x4 matDeviceToTracking = m_matrixDevicePose[vr::k_unTrackedDeviceIndex_Hmd];
  QMatrix4x4 mvp = currentViewProjection(eye) * matDeviceToTracking;  
  return mvp;
}
QMatrix4x4
VR::leftMVP(vr::Hmd_Eye eye)
{
  QMatrix4x4 matDeviceToTracking = m_matrixDevicePose[m_leftController];
  QMatrix4x4 mvp = currentViewProjection(eye) * matDeviceToTracking;  
  return mvp;
}
QMatrix4x4
VR::rightMVP(vr::Hmd_Eye eye)
{
  QMatrix4x4 matDeviceToTracking = m_matrixDevicePose[m_rightController];
  QMatrix4x4 mvp = currentViewProjection(eye) * matDeviceToTracking;  
  return mvp;
}

QVector3D
VR::getPosition(vr::TrackedDeviceIndex_t trackedDevice)
{
  QMatrix4x4 mat = m_matrixDevicePose[trackedDevice];    
  QVector4D center = mat * QVector4D(0,0,0,1);
  QVector3D pos(center.x(),center.y(),center.z());
  return pos;		
}

QVector3D
VR::getPosition(const vr::HmdMatrix34_t mat)
{
  return QVector3D(mat.m[0][3],
		   mat.m[1][3],
		   mat.m[2][3]);
		
}

QVector3D
VR::getPosition(QMatrix4x4 mat)
{
  return QVector3D(mat(0,3), mat(1,3), mat(2,3));
		
}


float
VR::copysign(float x, float y)
{
  return y == 0.0 ? qAbs(x) : qAbs(x)*y/qAbs(y);
}

// Get the quaternion representing the rotation
QQuaternion
VR::getQuaternion(QMatrix4x4 mat)
{
  float w = sqrt(qMax(0.0f, 1 + mat(0,0) + mat(1,1) + mat(2,2))) / 2;
  float x = sqrt(qMax(0.0f, 1 + mat(0,0) - mat(1,1) - mat(2,2))) / 2;
  float y = sqrt(qMax(0.0f, 1 - mat(0,0) + mat(1,1) - mat(2,2))) / 2;
  float z = sqrt(qMax(0.0f, 1 - mat(0,0) - mat(1,1) + mat(2,2))) / 2;

  x = copysign(x, (mat(2,1) - mat(1,2)));
  y = copysign(y, (mat(0,2) - mat(2,0)));
  z = copysign(z, (mat(1,0) - mat(0,1)));

  QQuaternion q(x,y,z,w);
  
  return q;
}

// Get the quaternion representing the rotation
QQuaternion
VR::getQuaternion(vr::HmdMatrix34_t mat)
{
  float w = sqrt(qMax(0.0f, 1 + mat.m[0][0] + mat.m[1][1] + mat.m[2][2])) / 2;
  float x = sqrt(qMax(0.0f, 1 + mat.m[0][0] - mat.m[1][1] - mat.m[2][2])) / 2;
  float y = sqrt(qMax(0.0f, 1 - mat.m[0][0] + mat.m[1][1] - mat.m[2][2])) / 2;
  float z = sqrt(qMax(0.0f, 1 - mat.m[0][0] - mat.m[1][1] + mat.m[2][2])) / 2;

  x = copysign(x, (mat.m[2][1] - mat.m[1][2]));
  y = copysign(y, (mat.m[0][2] - mat.m[2][0]));
  z = copysign(z, (mat.m[1][0] - mat.m[0][1]));

  QQuaternion q(x,y,z,w);
  
  return q;
}

bool
VR::preDraw()
{
  updatePoses();
  bool ok = updateInput();
  if (!ok)
    return false;

  buildAxes();

  return true;
}

void
VR::bindBuffer(vr::Hmd_Eye eye)
{
  if (eye == vr::Eye_Left)
    bindLeftBuffer();
  else
    bindRightBuffer();
}

void
VR::bindLeftBuffer()
{
  //glEnable(GL_MULTISAMPLE);
  m_leftBuffer->bind();
}

void
VR::bindRightBuffer()
{
  //glEnable(GL_MULTISAMPLE);
  m_rightBuffer->bind();
}

void
VR::postDrawLeftBuffer()
{
  renderSkyBox(vr::Eye_Left);

  if (m_touchPressActiveRight)
    drawClipInVR(vr::Eye_Left);

  renderMenu(vr::Eye_Left);
  
  renderControllers(vr::Eye_Left);

  renderAxes(vr::Eye_Left);

  m_leftBuffer->release();

  QRect sourceRect(0, 0, m_eyeWidth, m_eyeHeight);
  QRect targetLeft(0, 0, m_eyeWidth, m_eyeHeight);
  QOpenGLFramebufferObject::blitFramebuffer(m_resolveBuffer, targetLeft,
					    m_leftBuffer, sourceRect);
}

void
VR::postDrawRightBuffer()
{
  renderSkyBox(vr::Eye_Right);

  if (m_touchPressActiveRight)
    drawClipInVR(vr::Eye_Right);
  
  renderMenu(vr::Eye_Right);

  renderControllers(vr::Eye_Right);

  renderAxes(vr::Eye_Right);

  m_rightBuffer->release();

  QRect sourceRect(0, 0, m_eyeWidth, m_eyeHeight);
  QRect targetRight(m_eyeWidth, 0, m_eyeWidth, m_eyeHeight);
  QOpenGLFramebufferObject::blitFramebuffer(m_resolveBuffer, targetRight,
					    m_rightBuffer, sourceRect);
}

void
VR::postDraw()
{
  if (m_hmd)
    {
      vr::VRTextureBounds_t leftRect = { 0.0f, 0.0f, 0.5f, 1.0f };
      vr::VRTextureBounds_t rightRect = { 0.5f, 0.0f, 1.0f, 1.0f };
      vr::Texture_t composite = { (void*)m_resolveBuffer->texture(),
				  vr::TextureType_OpenGL,
				  vr::ColorSpace_Gamma };
      
      vr::VRCompositor()->Submit(vr::Eye_Left, &composite, &leftRect);
      vr::VRCompositor()->Submit(vr::Eye_Right, &composite, &rightRect);
    }
}


QVector3D
VR::vrViewDir()
{
  QMatrix4x4 mat = m_matrixDevicePose[vr::k_unTrackedDeviceIndex_Hmd];
  QVector4D center = mat * QVector4D(0,0,0,1);
  QVector4D point = mat * QVector4D(0,0,1,1);
  QVector4D dir = point-center;
  QVector3D dir3 = QVector3D(dir.x(), dir.y(), dir.z());
  dir3.normalize();
  return dir3;
}
QVector3D
VR::vrUpDir()
{
  QMatrix4x4 mat = m_matrixDevicePose[vr::k_unTrackedDeviceIndex_Hmd];
  QVector4D center = mat * QVector4D(0,0,0,1);
  QVector4D point = mat * QVector4D(0,1,0,1);
  QVector4D dir = point-center;
  QVector3D dir3 = QVector3D(dir.x(), dir.y(), dir.z());
  dir3.normalize();
  return dir3;
}
QVector3D
VR::vrRightDir()
{
  QMatrix4x4 mat = m_matrixDevicePose[vr::k_unTrackedDeviceIndex_Hmd];
  QVector4D center = mat * QVector4D(0,0,0,1);
  QVector4D point = mat * QVector4D(1,0,0,1);
  QVector4D dir = point-center;
  QVector3D dir3 = QVector3D(dir.x(), dir.y(), dir.z());
  dir3.normalize();
  return dir3;
}

QVector3D
VR::viewDir()
{
  QMatrix4x4 mat = m_matrixDevicePose[vr::k_unTrackedDeviceIndex_Hmd];

  QVector4D center = mat * QVector4D(0,0,0,1);
  QVector4D point = mat * QVector4D(0,0,1,1);

  center = m_final_xformInverted.map(center);
  point = m_final_xformInverted.map(point);

  QVector4D dir = point-center;

  QVector3D dir3 = QVector3D(dir.x(), dir.y(), dir.z());
  dir3.normalize();

  return dir3;
}

QVector3D
VR::upDir()
{
  QMatrix4x4 mat = m_matrixDevicePose[vr::k_unTrackedDeviceIndex_Hmd];

  QVector4D center = mat * QVector4D(0,0,0,1);
  QVector4D point = mat * QVector4D(0,1,0,1);

  center = m_final_xformInverted.map(center);
  point = m_final_xformInverted.map(point);

  QVector4D dir = point-center;

  QVector3D dir3 = QVector3D(dir.x(), dir.y(), dir.z());
  dir3.normalize();

  return dir3;
}

QVector3D
VR::rightDir()
{
  QMatrix4x4 mat = m_matrixDevicePose[vr::k_unTrackedDeviceIndex_Hmd];

  QVector4D center = mat * QVector4D(0,0,0,1);
  QVector4D point = mat * QVector4D(1,0,0,1);

  center = m_final_xformInverted.map(center);
  point = m_final_xformInverted.map(point);

  QVector4D dir = point-center;

  QVector3D dir3 = QVector3D(dir.x(), dir.y(), dir.z());
  dir3.normalize();

  return dir3;
}

void
VR::buildAxes()
{
  QMatrix4x4 matL = m_matrixDevicePose[m_leftController];
  QMatrix4x4 matR = m_matrixDevicePose[m_rightController];

  QVector<float> vert;
  QVector<uchar> col;
  int npt = 0;

  QVector3D colorL(0,255,255);
  QVector3D colorR(255,255,0);
  QVector4D point(0,0,-0.05,1);


  QVector4D centerR = matR * QVector4D(0,0,0,1);
  QVector4D frontR = matR * QVector4D(0,0,-0.1,1) - centerR; 
  vert << centerR.x();
  vert << centerR.y();
  vert << centerR.z();      

  QVector3D pp = m_leftMenu.pinPoint();  
  if (pp.x() < -100)
    {
      vert << centerR.x() + frontR.x();
      vert << centerR.y() + frontR.y();
      vert << centerR.z() + frontR.z();
    }
  else
    {
      vert << pp.x();
      vert << pp.y();
      vert << pp.z();
    }
  
  col << colorR.x();
  col << colorR.y();
  col << colorR.z();
  if (pp.x() < -100)
    col << 128;
  else // change transparency if pointing towards menu
    col << 255;

  if (pp.x() < -100)
    {
      col << 0;
      col << 0;
      col << 0;
      col << 0;
    }
  else // change transparency if pointing towards menu
    {
      col << colorR.x()*0.5;
      col << colorR.y()*0.5;
      col << colorR.z()*0.5;
      col << 128;
    }
  
  QVector4D centerL = matL * QVector4D(0,0,0,1);
  QVector4D frontL = matL * QVector4D(0,0,-0.1,1) - centerL; 
  vert << centerL.x();
  vert << centerL.y();
  vert << centerL.z();      
  vert << centerL.x() + frontL.x();
  vert << centerL.y() + frontL.y();
  vert << centerL.z() + frontL.z();      
      
  col << colorL.x();
  col << colorL.y();
  col << colorL.z();      
  col << 128;
  col << 0;
  col << 0;
  col << 0;
  col << 0;
  
  npt = 4;


  uchar vc[1000];
  for(int v=0; v<vert.count()/3; v++)
    {
      float *vt = (float*)(vc + 16*v);
      vt[0] = vert[3*v+0];
      vt[1] = vert[3*v+1];
      vt[2] = vert[3*v+2];
      
      uchar *cl = (uchar*)(vc + 16*v + 12);
      float a = (float)col[4*v+3]/255.0f;
      cl[0] = col[4*v+0]*a;
      cl[1] = col[4*v+1]*a;
      cl[2] = col[4*v+2]*a;
      cl[3] = 255*a;
    }
  
  glBindBuffer(GL_ARRAY_BUFFER, m_boxV);
  glBufferSubData(GL_ARRAY_BUFFER,
		  0,
		  vert.count()*16,
		  &vc[0]);

  m_axesPoints = npt;
  m_nboxPoints = npt;
}

QMatrix4x4
VR::quatToMat(QQuaternion q)
{
  //based on algorithm on wikipedia
  // http://en.wikipedia.org/wiki/Rotation_matrix#Quaternion
  float w = q.scalar ();
  float x = q.x();
  float y = q.y();
  float z = q.z();
  
  float n = q.lengthSquared();
  float s =  n == 0?  0 : 2 / n;
  float wx = s * w * x, wy = s * w * y, wz = s * w * z;
  float xx = s * x * x, xy = s * x * y, xz = s * x * z;
  float yy = s * y * y, yz = s * y * z, zz = s * z * z;
  
  float m[16] = { 1 - (yy + zz),         xy + wz ,         xz - wy ,0,
		  xy - wz ,    1 - (xx + zz),         yz + wx ,0,
		  xz + wy ,         yz - wx ,    1 - (xx + yy),0,
		  0 ,               0 ,               0 ,1  };
  QMatrix4x4 result =  QMatrix4x4(m,4,4);
  result.optimize ();
  return result;
}

void
VR::renderAxes(vr::Hmd_Eye eye)
{  
  glEnable(GL_BLEND);
  //glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
  glBlendFunc(GL_ONE, GL_ONE);

  QMatrix4x4 mvp = currentViewProjection(eye);

  glUseProgram(m_pshader);
  glUniformMatrix4fv(m_pshaderParm[0], 1, GL_FALSE, mvp.data());
  glUniform1f(m_pshaderParm[1], 20);
  // send dummy positions
  glUniform3f(m_pshaderParm[2], -1, -1, -1); // head position
  glUniform3f(m_pshaderParm[3], -1, -1, -1); // left controller
  glUniform3f(m_pshaderParm[4], -1, -1, -1); // right controller
  glUniform1i(m_pshaderParm[5], 1); // lines


  glLineWidth(2.0);

  glBindVertexArray(m_boxVID);
  
  glBindBuffer(GL_ARRAY_BUFFER, m_boxV);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0,  // attribute 0
			3,  // size
			GL_FLOAT, // type
			GL_FALSE, // normalized
			16, // stride
			(char *)NULL ); // array buffer offset
  
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1,  // attribute 1
			4,  // size
			GL_UNSIGNED_BYTE, // type
			GL_FALSE, // normalized
			16, // stride
			(char *)NULL + 12 ); // array buffer offset
  
  glDrawArrays(GL_LINES, 0, m_axesPoints);

    
  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);  

  glBindVertexArray(0);

  glUseProgram(0);
}

void
VR::createShaders()
{
  //------------------------
  m_pshader = glCreateProgram();
  if (!ShaderFactory::loadShadersFromFile(m_pshader,
					  qApp->applicationDirPath() + QDir::separator() + "assets/shaders/punlit.vert",
					  qApp->applicationDirPath() + QDir::separator() + "assets/shaders/punlit.frag"))
    {
      QMessageBox::information(0, "", "Cannot load shaders");
    }

  m_pshaderParm[0] = glGetUniformLocation(m_pshader, "MVP");
  m_pshaderParm[1] = glGetUniformLocation(m_pshader, "pointSize");
  m_pshaderParm[2] = glGetUniformLocation(m_pshader, "hmdPos");
  m_pshaderParm[3] = glGetUniformLocation(m_pshader, "leftController");
  m_pshaderParm[4] = glGetUniformLocation(m_pshader, "rightController");
  m_pshaderParm[5] = glGetUniformLocation(m_pshader, "gltype");
  //------------------------


  //ShaderFactory::createTextureShader();
  
  //ShaderFactory::createCubeMapShader();
}


void
VR::buildAxesVB()
{
  glGenVertexArrays(1, &m_boxVID);
  glBindVertexArray(m_boxVID);

  glGenBuffers(1, &m_boxV);

  glBindBuffer(GL_ARRAY_BUFFER, m_boxV);
  glBufferData(GL_ARRAY_BUFFER,
	       1000*sizeof(float),
	       NULL,
	       GL_STATIC_DRAW);


  { // clip
    float vertData[50];
    memset(vertData, 0, 50*sizeof(float));  
    vertData[6] = 0.0;
    vertData[7] = 0.0;
    vertData[14] = 0.0;
    vertData[15] = 1.0;
    vertData[22] = 1.0;
    vertData[23] = 1.0;
    vertData[30] = 1.0;
    vertData[31] = 0.0;
    
    vertData[0] = -0.5;  vertData[1] = 0;  vertData[2] = -0.05;
    vertData[8] = -0.5;  vertData[9] = 0; vertData[10] = -1.05;
    vertData[16] = 0.5; vertData[17] = 0; vertData[18] = -1.05;
    vertData[24] = 0.5; vertData[25] = 0; vertData[26] = -0.05;

    glBufferSubData(GL_ARRAY_BUFFER,
		    200, // offset
		    sizeof(float)*8*4,
		    &vertData[0]);
  }
  
  {
    float vertData[50];
    memset(vertData, 0, 50*sizeof(float));  
    vertData[6] = 0.0;
    vertData[7] = 0.0;
    vertData[14] = 0.0;
    vertData[15] = 1.0;
    vertData[22] = 1.0;
    vertData[23] = 1.0;
    vertData[30] = 1.0;
    vertData[31] = 0.0;
    
    vertData[0] =  0;  vertData[1] = 0;  vertData[2] = 0;
    vertData[8] =  0;  vertData[9] = 0; vertData[10] = 1;
    vertData[16] = 1; vertData[17] = 0; vertData[18] = 1;
    vertData[24] = 1; vertData[25] = 0; vertData[26] = 0;

    glBufferSubData(GL_ARRAY_BUFFER,
		    500, // offset
		    sizeof(float)*8*4,
		    &vertData[0]);
  }
  

  //------------------------
  uchar indexData[12];
  indexData[0] = 0;
  indexData[1] = 1;
  indexData[2] = 2;
  indexData[3] = 0;
  indexData[4] = 2;
  indexData[5] = 3;
  indexData[6] = 4;
  indexData[7] = 5;
  indexData[8] = 6;
  indexData[9] = 4;
  indexData[10] = 6;
  indexData[11] = 7;
  
  glGenBuffers( 1, &m_boxIB );
  glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_boxIB );
  glBufferData( GL_ELEMENT_ARRAY_BUFFER,
		sizeof(uchar) * 12,
		&indexData[0],
		GL_STATIC_DRAW );
  //------------------------
}

CGLRenderModel*
VR::findOrLoadRenderModel(QString pchRenderModelName)
{
  CGLRenderModel *pRenderModel = NULL;
  for(int i=0; i<m_vecRenderModels.count(); i++)
    {
      if(m_vecRenderModels[i]->GetName() == pchRenderModelName)
	{
	  pRenderModel = m_vecRenderModels[i];
	  break;
	}
    }

  // load the model if we didn't find one
  if( !pRenderModel )
    {
      vr::RenderModel_t *pModel;
      vr::EVRRenderModelError error;
      while ( 1 )
	{
	  error = vr::VRRenderModels()->LoadRenderModel_Async( pchRenderModelName.toLatin1(),
							       &pModel );
	  if ( error != vr::VRRenderModelError_Loading )
	    {
	      break;
	    }
	  
	  Sleep( 1 );
	}
      
      if ( error != vr::VRRenderModelError_None )
	{
	  QMessageBox::information(0, "", QString("Unable to load render model %1 - %2").\
				   arg(pchRenderModelName).\
				   arg(vr::VRRenderModels()->GetRenderModelErrorNameFromEnum(error)));
	  return NULL; // move on to the next tracked device
	}
      
      vr::RenderModel_TextureMap_t *pTexture;
      while ( 1 )
	{
	  error = vr::VRRenderModels()->LoadTexture_Async( pModel->diffuseTextureId, &pTexture );
	  //error = vr::VRRenderModels()->LoadTexture( pModel->diffuseTextureId, &pTexture );
	  if ( error != vr::VRRenderModelError_Loading )
	    {
	      break;
	    }
	  
	  Sleep( 1 );
	}
      
      if ( error != vr::VRRenderModelError_None )
	{
	  QMessageBox::information(0, "", QString("Unable to load render texture id:%1 for render model %2"). \
				   arg(pModel->diffuseTextureId).arg(pchRenderModelName));
	  vr::VRRenderModels()->FreeRenderModel( pModel );
	  return NULL; // move on to the next tracked device
	}
      
      pRenderModel = new CGLRenderModel( pchRenderModelName );
      if ( !pRenderModel->BInit( *pModel, *pTexture ) )
	{
	  QMessageBox::information(0, "", "Unable to create GL model from render model " + pchRenderModelName);
	  delete pRenderModel;
	  pRenderModel = NULL;
	}
      else
	{
	  m_vecRenderModels << pRenderModel;
	}
      vr::VRRenderModels()->FreeRenderModel( pModel );
      vr::VRRenderModels()->FreeTexture( pTexture );
    }
  return pRenderModel;
}


void
VR::setupRenderModelForTrackedDevice(int type,
				     vr::TrackedDeviceIndex_t unTrackedDeviceIndex)
{
  if(unTrackedDeviceIndex >= vr::k_unMaxTrackedDeviceCount)
    return;
  

  QString sRenderModelName = getTrackedDeviceString(unTrackedDeviceIndex,
						    vr::Prop_RenderModelName_String);

  if (type == 0)
    m_leftControllerName = sRenderModelName;
  else
    m_rightControllerName = sRenderModelName;

  int cc = m_pRenderModels->GetComponentCount(sRenderModelName.toLatin1());

  QStringList cnames;
  for(int i=0; i<cc; i++)
    {
      char cnm[1024];
      int nb = m_pRenderModels->GetComponentName(sRenderModelName.toLatin1(),
						 i,
						 &cnm[0], 1024);      
      if (nb > 0)
	cnames << QString(cnm);
    }

  
  QList< CGLRenderModel* > cglRM;  
  for(int i=0; i<cnames.count(); i++)
    {
      char crmnm[1024];
      int idx = m_pRenderModels->GetComponentRenderModelName(sRenderModelName.toLatin1(),
							     cnames[i].toLatin1(),
							     &crmnm[0], 1024);
      if (idx > 0)
	{
	  CGLRenderModel *pRenderModel = findOrLoadRenderModel( QString(crmnm) );
	  if (pRenderModel)
	    {
	      cglRM << pRenderModel;
	      if (type == 0)
		{
		  m_leftComponentNames << cnames[i];
		  m_leftRenderModels << pRenderModel;
		}
	      else
		{
		  m_rightComponentNames << cnames[i];
		  m_rightRenderModels << pRenderModel;
		}
	    }
	}
    }
}


void
VR::setupRenderModels()
{
  memset( m_rTrackedDeviceToRenderModel, 0, sizeof( m_rTrackedDeviceToRenderModel ) );

  if( !m_hmd )
    return;

  setupRenderModelForTrackedDevice( 0, m_leftController );
  setupRenderModelForTrackedDevice( 1, m_rightController );
}

void
VR::generateButtonInfo()
{
  m_menuTex.clear();
  m_menuMat.clear();
  
  QFont font = QFont("Helvetica", 48);
  QColor color(255, 255, 255);
  QColor bgcolor(120, 100, 100); 

  glActiveTexture(GL_TEXTURE0);

  //----------------------
  // left controller
  {
    QImage img = StaticFunctions::renderText("Swipe For Next",
					     font,
					     Qt::black,
					     color, true).mirrored(false, true);
    
    GLuint glTex;
    glGenTextures(1, &glTex);

    StaticFunctions::loadTexture(img, glTex);

    float twd = img.width();
    float tht = img.height();
    float mx = qMax(twd, tht);
    twd/=mx; tht/=mx;
    {
      float frc = 0.01/tht;
      twd *= frc;
      tht *= frc;
    }
    QMatrix4x4 txtMat;
    txtMat.setToIdentity();
    txtMat.translate(0.025, 0.005, 0.04);
    txtMat.scale(twd, 1, tht);

    m_menuTex << glTex;
    m_menuMat << txtMat;
  }
  
  {
    QImage img = StaticFunctions::renderText("Swipe For Prev",
					     font,
					     Qt::black,
					     color, true).mirrored(false, true);
    
    GLuint glTex;
    glGenTextures(1, &glTex);

    StaticFunctions::loadTexture(img, glTex);

    float twd = img.width();
    float tht = img.height();
    float mx = qMax(twd, tht);
    twd/=mx; tht/=mx;
    {
      float frc = 0.01/tht;
      twd *= frc;
      tht *= frc;
    }
    QMatrix4x4 txtMat;
    txtMat.setToIdentity();
    txtMat.translate(-0.025-twd, 0.005, 0.04);
    txtMat.scale(twd, 1, tht);

    m_menuTex << glTex;
    m_menuMat << txtMat;
  }

  {
    QImage img = StaticFunctions::renderText("Next Keyframe",
					     font,
					     bgcolor,
					     color, false).mirrored(false, true);
    
    GLuint glTex;
    glGenTextures(1, &glTex);

    StaticFunctions::loadTexture(img, glTex);

    float twd = img.width();
    float tht = img.height();
    float mx = qMax(twd, tht);
    twd/=mx; tht/=mx;
    {
      float frc = 0.01/tht;
      twd *= frc;
      tht *= frc;
    }
    QMatrix4x4 txtMat;
    txtMat.setToIdentity();
    //txtMat.translate(-twd/2, 0.01, 0.03);
    txtMat.rotate(-40, 1,0,0);
    txtMat.translate(-twd, -0.02, tht/2+0.007);
    txtMat.scale(twd, 1, tht);

    m_menuTex << glTex;
    m_menuMat << txtMat;
  }

  {
    QImage img = StaticFunctions::renderText("Prev Keyframe",
					     font,
					     bgcolor,
					     color, false).mirrored(false, true);
    
    GLuint glTex;
    glGenTextures(1, &glTex);

    StaticFunctions::loadTexture(img, glTex);

    float twd = img.width();
    float tht = img.height();
    float mx = qMax(twd, tht);
    twd/=mx; tht/=mx;
    {
      float frc = 0.01/tht;
      twd *= frc;
      tht *= frc;
    }
    QMatrix4x4 txtMat;
    txtMat.setToIdentity();
    //txtMat.translate(-twd/2, 0.008, 0.065);
    txtMat.rotate(-40, 1,0,0);
    txtMat.translate(-twd, -0.02, tht/2+0.032);
    txtMat.scale(twd, 1, tht);

    m_menuTex << glTex;
    m_menuMat << txtMat;
  }

  {
    QImage img = StaticFunctions::renderText("Menu",
					     font,
					     bgcolor,
					     color, true).mirrored(false, true);
    
    GLuint glTex;
    glGenTextures(1, &glTex);

    StaticFunctions::loadTexture(img, glTex);

    float twd = img.width();
    float tht = img.height();
    float mx = qMax(twd, tht);
    twd/=mx; tht/=mx;
    {
      float frc = 0.01/tht;
      twd *= frc;
      tht *= frc;
    }
    QMatrix4x4 txtMat;
    txtMat.setToIdentity();
    //txtMat.translate(0.01, 0.007, 0.01);
    txtMat.rotate(-20, 1,0,0);
    txtMat.translate(-twd-0.01, -0.015, tht/2+0.045);
    txtMat.scale(twd, 1, tht);

    m_menuTex << glTex;
    m_menuMat << txtMat;
  }
  
  {
    QImage img = StaticFunctions::renderText("Translate",
					     font,
					     bgcolor,
					     color, true).mirrored(false, true);
    
    GLuint glTex;
    glGenTextures(1, &glTex);

    StaticFunctions::loadTexture(img, glTex);

    float twd = img.width();
    float tht = img.height();
    float mx = qMax(twd, tht);
    twd/=mx; tht/=mx;
    {
      float frc = 0.02/tht;
      twd *= frc;
      tht *= frc;
    }
    QMatrix4x4 txtMat;
    txtMat.setToIdentity();
    txtMat.translate(0.03, -0.04, 0.045);
    txtMat.scale(twd, 1, tht);

    m_menuTex << glTex;
    m_menuMat << txtMat;
  }
  //----------------------

  
  //----------------------
  // right controller
  {
    QImage img = StaticFunctions::renderText("Rotate",
					     font,
					     bgcolor,
					     color, true).mirrored(false, true);
    
    GLuint glTex;
    glGenTextures(1, &glTex);

    StaticFunctions::loadTexture(img, glTex);

    float twd = img.width();
    float tht = img.height();
    float mx = qMax(twd, tht);
    twd/=mx; tht/=mx;
    {
      float frc = 0.02/tht;
      twd *= frc;
      tht *= frc;
    }
    QMatrix4x4 txtMat;
    txtMat.setToIdentity();
    //txtMat.translate(0.01, -0.04, 0.045);
    txtMat.translate(-twd-0.03, -0.04, 0.045);
    txtMat.scale(twd, 1, tht);

    m_menuTex << glTex;
    m_menuMat << txtMat;
  }

  {
    QImage img = StaticFunctions::renderText("Press To Slice",
					     font,
					     bgcolor,
					     color, true).mirrored(false, true);
    
    GLuint glTex;
    glGenTextures(1, &glTex);

    StaticFunctions::loadTexture(img, glTex);

    float twd = img.width();
    float tht = img.height();
    float mx = qMax(twd, tht);
    twd/=mx; tht/=mx;
    {
      float frc = 0.015/tht;
      twd *= frc;
      tht *= frc;
    }
    QMatrix4x4 txtMat;
    txtMat.setToIdentity();
    //txtMat.translate(-twd/2, 0.005, 0.05);
    //txtMat.rotate(-20, 1,0,0);
    txtMat.translate(-twd/2, 0.0, tht/2+0.01);
    txtMat.scale(twd, 1, tht);

    m_menuTex << glTex;
    m_menuMat << txtMat;
  }


  {
    QImage img = StaticFunctions::renderText(".",
					     font,
					     bgcolor,
					     color, true).mirrored(false, true);
    
    GLuint glTex;
    glGenTextures(1, &glTex);

    StaticFunctions::loadTexture(img, glTex);

    float twd = img.width();
    float tht = img.height();
    float mx = qMax(twd, tht);
    twd/=mx; tht/=mx;
    {
      float frc = 0.015/tht;
      twd *= frc;
      tht *= frc;
    }
    QMatrix4x4 txtMat;
    txtMat.setToIdentity();
    //txtMat.translate(-twd/2, 0.005, 0.05);
    //txtMat.rotate(-20, 1,0,0);
    txtMat.translate(-twd/2, 0.0, -0.01);
    txtMat.scale(twd, 1, tht);

    m_menuMeshNameIdx = m_menuMat.count();
    m_menuTex << glTex;
    m_menuMat << txtMat;
  }


}

void
VR::renderControllers(vr::Hmd_Eye eye)
{
  if (!m_hmd)
    return;

  if (!m_hmd->IsInputAvailable())
    return;

//  bool bIsInputCapturedByAnotherProcess = m_hmd->IsInputFocusCapturedByAnotherProcess();
//  if(bIsInputCapturedByAnotherProcess)
//    return;

  if( !m_hmd->IsTrackedDeviceConnected(m_leftController) ||
      !m_hmd->IsTrackedDeviceConnected(m_rightController) )
    return;
  
  QVector3D vd = vrViewDir();
  
//-------------------------------------------------
//-------------------------------------------------
  glUseProgram(ShaderFactory::rcShader());

  GLint *rcShaderParm = ShaderFactory::rcShaderParm();
  glUniform1i(rcShaderParm[1], 0); // texture
  glUniform3f(rcShaderParm[3], vd.x(), vd.y(), vd.z()); // view direction
  glUniform1f(rcShaderParm[4], 1); // opacity modulator
  glUniform1i(rcShaderParm[5], 2); // applytexture
  glUniform1f(rcShaderParm[7], 0.2); // mixcolor
  glUniform1i(rcShaderParm[8], 0); // apply light
  glUniform1i(rcShaderParm[11], 0); // no shadows


  //---------------------------------------
  // draw left controller
  QMatrix4x4 matDeviceToTracking = m_matrixDevicePose[m_leftController];
  QMatrix4x4 mvp = currentViewProjection(eye) * matDeviceToTracking;  
  glUniform3f(rcShaderParm[2], 0,0,0); // color
  for(int ic=0; ic<m_leftRenderModels.count(); ic++)
    {
      vr::VRControllerState_t pControllerState;
      m_hmd->GetControllerState(m_leftController, &pControllerState, sizeof(pControllerState));
      vr::RenderModel_ControllerMode_State_t pState;
      pState.bScrollWheelVisible = false;
      vr::RenderModel_ComponentState_t pComponentState;
      if (m_pRenderModels->GetComponentState(m_leftControllerName.toLatin1(),
					     m_leftComponentNames[ic].toLatin1(),
					     &pControllerState,
					     &pState,
					     &pComponentState))
	{
	  if (pComponentState.uProperties & vr::VRComponentProperty_IsVisible)
	    {
	      QMatrix4x4 cmvp;
	      cmvp = mvp * vrMatrixToQt(pComponentState.mTrackingToComponentRenderModel);
	      glUniformMatrix4fv(rcShaderParm[0], 1, GL_FALSE, cmvp.data() );  
	      
	      QVector3D clr = QVector3D(0,0,0);
	      if (!(pComponentState.uProperties & vr::VRComponentProperty_IsStatic))
	      {
		if (pComponentState.uProperties & vr::VRComponentProperty_IsTouched)
		  clr = QVector3D(0.0,0.8,1.0);
		if (pComponentState.uProperties & vr::VRComponentProperty_IsPressed)
		  clr = QVector3D(0.0,0.5,1.0);
	      }
	      
	      glUniform3f(rcShaderParm[2], clr.x(), clr.y(), clr.z()); // color
	      
	      m_leftRenderModels[ic]->Draw();
	    }
	}
    }
  //---------------------------------------
  

  //---------------------------------------
  // draw right controller
  matDeviceToTracking = m_matrixDevicePose[m_rightController];
  mvp = currentViewProjection(eye) * matDeviceToTracking;
  glUniform3f(rcShaderParm[2], 0,0,0); // color
  for(int ic=0; ic<m_rightRenderModels.count(); ic++)
    {
      vr::VRControllerState_t pControllerState;
      m_hmd->GetControllerState(m_rightController, &pControllerState, sizeof(pControllerState));
      vr::RenderModel_ControllerMode_State_t pState;
      pState.bScrollWheelVisible = false;
      vr::RenderModel_ComponentState_t pComponentState;
      if (m_pRenderModels->GetComponentState(m_rightControllerName.toLatin1(),
					     m_rightComponentNames[ic].toLatin1(),
					     &pControllerState,
					     &pState,
					     &pComponentState))
	{
	  if (pComponentState.uProperties & vr::VRComponentProperty_IsVisible)
	    {
	      QMatrix4x4 cmvp;
	      cmvp = mvp * vrMatrixToQt(pComponentState.mTrackingToComponentRenderModel);
	      glUniformMatrix4fv(rcShaderParm[0], 1, GL_FALSE, cmvp.data() );  
	      
	      QVector3D clr = QVector3D(0,0,0);
	      
	      if (!(pComponentState.uProperties & vr::VRComponentProperty_IsStatic))
		{
		  if (pComponentState.uProperties & vr::VRComponentProperty_IsTouched)
		    clr = QVector3D(1.0,0.8,0.0);
		  if (pComponentState.uProperties & vr::VRComponentProperty_IsPressed)
		    clr = QVector3D(1.0,0.5,0.0);
		}
	      
	      glUniform3f(rcShaderParm[2], clr.x(), clr.y(), clr.z()); // color
	      
	      m_rightRenderModels[ic]->Draw();
	    }
	}
    }
  //---------------------------------------


  //---------------------------------------
  // render button information for both the controllers
  renderControllerLabels(eye);
  //---------------------------------------


  glUseProgram( 0 );
//-------------------------------------------------
//-------------------------------------------------
}

void
VR::renderControllerLabels(vr::Hmd_Eye eye)
{
  //glEnable(GL_BLEND);
  //glBlendFunc(GL_ONE, GL_ONE);

  GLint *rcShaderParm = ShaderFactory::rcShaderParm();

  glBindVertexArray(m_boxVID);  
  glBindBuffer(GL_ARRAY_BUFFER, m_boxV);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_boxIB);  

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0,  // attribute 0
			3,  // size
			GL_FLOAT, // type
			GL_FALSE, // normalized
			sizeof(float)*8, // stride
			(char *)NULL + 500); // starting offset
    
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1,  // attribute 1
			3,  // size
			GL_UNSIGNED_BYTE, // type
			GL_FALSE, // normalized
			sizeof(float)*8, // stride
			(char *)NULL + 500 + sizeof(float)*3 );
  
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2,
			2,
			GL_FLOAT,
			GL_FALSE, 
			sizeof(float)*8,
			(char *)NULL + 500 + sizeof(float)*6 );
    
  glEnable(GL_TEXTURE_2D);
    
  glUniform3f(rcShaderParm[3], 0, 0, 0); // view direction
  glUniform1f(rcShaderParm[4], 0.5); // opacity modulator

  int bs = 0;
  if (m_maxVolumeSeries == 1)
    bs = 2;
  if (m_currTeleportNumber == -1)
    bs = 4;
  for (int bi=bs; bi<6; bi++) // left controller
    {
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, m_menuTex[bi]);
      QMatrix4x4 mvp = currentViewProjection(eye) *
		       m_matrixDevicePose[m_leftController] *
		       m_menuMat[bi];  
      glUniformMatrix4fv(rcShaderParm[0], 1, GL_FALSE, mvp.data() );      
      glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0);  
    }
  
  for (int bi=6; bi<m_menuTex.count(); bi++) // right controller
    {
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, m_menuTex[bi]);
      QMatrix4x4 mvp = currentViewProjection(eye) *
		       m_matrixDevicePose[m_rightController] *
		       m_menuMat[bi];  
      glUniformMatrix4fv(rcShaderParm[0], 1, GL_FALSE, mvp.data() );      
      glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0);  
    }
  
  glBindVertexArray(0);
  
  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);  
  glDisableVertexAttribArray(2);  
  
  glBindVertexArray(0);

  glDisable(GL_BLEND);
}

void
VR::genEyeMatrices()
{
  m_rightProjection = vrMatrixToQt(m_hmd->GetProjectionMatrix(vr::Eye_Right,
							      NEAR_CLIP,
							      qMax(1.0f,m_flightSpeed)*FAR_CLIP));
  m_rightPose = vrMatrixToQt(m_hmd->GetEyeToHeadTransform(vr::Eye_Right)).inverted();
  

  m_leftProjection = vrMatrixToQt(m_hmd->GetProjectionMatrix(vr::Eye_Left,
							     NEAR_CLIP,
							     qMax(1.0f,m_flightSpeed)*FAR_CLIP));
  m_leftPose = vrMatrixToQt(m_hmd->GetEyeToHeadTransform(vr::Eye_Left)).inverted();
}

void
VR::updateTeleport()
{
  int telframes = 150;
    
  m_telTime ++;
  if (m_telTime > telframes)
    {
      m_teleportTimer->stop();
      m_camPos = m_endCamPos;
      m_camRot = m_endCamRot;
      m_scaleFactor = m_scaleEnd;

      CaptionWidget::setText("hud", m_captionToTeleport);
      CaptionWidget::blinkAndHide("hud", 1000);	  

      emit removeClip();
    }
  else
    {
      float frc = (float)m_telTime/(float)telframes;
      m_camPos = (1.0-frc)*m_startCamPos + frc*m_endCamPos;
      m_camRot = Quaternion::slerp(m_startCamRot, m_endCamRot, frc);
      m_scaleFactor = (1.0-frc)*m_scaleStart + frc*m_scaleEnd;

      if (m_telTime == telframes*0.75)
	{
	  // save parameters here if we need to restore their values after HUD regeneration

	  m_leftMenu.reGenerateHUD();

	  // restore the parameter values after HUD regeneration
	}
    }
  
  m_final_xform = modelViewFromCamera();

  genEyeMatrices();
}

void
VR::nextTeleport()
{
  emit nextKeyFrame();
  //m_currTeleportNumber++;
  //teleport();
  emit playTeleportSound();
}
void
VR::prevTeleport()
{
  emit prevKeyFrame();
  //m_currTeleportNumber--;
  //teleport();
  emit playTeleportSound();
}

void
VR::teleport()
{
  QDir jsondir(m_dataDir);
  QString jsonfile = jsondir.absoluteFilePath("teleport.json");

  QJsonArray jsonTeleportData;

  //------------------------------

  if (jsondir.exists("teleport.json"))
    {
      QFile loadFile(jsonfile);
      loadFile.open(QIODevice::ReadOnly);
      
      QByteArray data = loadFile.readAll();
      
      QJsonDocument jsonDoc(QJsonDocument::fromJson(data));
      
      jsonTeleportData = jsonDoc.array();
    }

  if (jsonTeleportData.count() > 0)
    {
      // we need this to restore after regeneration of menu texture
      //int isoscale = m_leftMenu.value("vert scale");
	
      
      if (m_currTeleportNumber >= jsonTeleportData.count())
	m_currTeleportNumber = 0;

      if (m_currTeleportNumber < 0)
	m_currTeleportNumber = jsonTeleportData.count()-1;
      
      QJsonObject jsonTeleportNode = jsonTeleportData[m_currTeleportNumber].toObject();
      QJsonObject jsonInfo = jsonTeleportNode["teleport"].toObject();

      QStringList pstr = jsonInfo["pos"].toString().split(" ", QString::SkipEmptyParts);
      if (pstr.count() != 3)
	return;
      m_startCamPos = m_camPos;
      m_endCamPos = Vec(pstr[0].toFloat(),
			pstr[1].toFloat(),
			pstr[2].toFloat());
	
      QStringList rstr = jsonInfo["rot"].toString().split(" ", QString::SkipEmptyParts);
      if (rstr.count() != 4)
	return;
      Vec axis;
      float angle;
      axis = Vec(rstr[0].toFloat(),
		 rstr[1].toFloat(),
		 rstr[2].toFloat());
      angle = rstr[3].toFloat();

      m_startCamRot = m_camRot;
      m_endCamRot.setAxisAngle(axis, angle);
      
      QStringList sclstr = jsonInfo["scale"].toString().split(" ", QString::SkipEmptyParts);
      m_scaleStart = m_scaleFactor;
      m_scaleEnd = sclstr[0].toFloat();
      
      m_flightSpeed = sclstr[1].toFloat();

      m_captionToTeleport = "DrishtiMesh-VR";
      if (jsonInfo.contains("caption"))
	{
	  QString caption = jsonInfo["caption"].toString();
	  m_captionToTeleport = caption;
	}

      if (jsonInfo.contains("edges"))
	{
	  float val = jsonInfo["edges"].toString().toFloat();
	  m_leftMenu.setValue("edges", val);
	  emit setEdges(val);
	}
      if (jsonInfo.contains("softness"))
	{
	  float val = jsonInfo["softness"].toString().toFloat();
	  m_leftMenu.setValue("softness", val);
	  emit setSoftness(val);
	}
      if (jsonInfo.contains("bright"))
	{
	  float val = jsonInfo["bright"].toString().toFloat();
	  m_leftMenu.setValue("bright", val);
	  emit setGamma(val);
	}
      if (jsonInfo.contains("explode"))
	{
	  float val = jsonInfo["explode"].toString().toFloat();
	  m_leftMenu.setValue("explode", val);
	  emit setExplode(val);
	}


      m_final_xform = modelViewFromCamera();
      
      m_genDrawList = true;

      genEyeMatrices();

      m_autoRotate = false;
	
      m_telTime = 0;
      m_teleportTimer->start();
    }
}

void
VR::saveTeleportNode()
{
  QDir jsondir(m_dataDir);
  QString jsonfile = jsondir.absoluteFilePath("teleport.json");

  QJsonArray jsonTeleportData;
  QJsonObject jsonTeleportNode;

  //------------------------------

  if (jsondir.exists("teleport.json"))
    {
      QFile loadFile(jsonfile);
      loadFile.open(QIODevice::ReadOnly);
      
      QByteArray data = loadFile.readAll();
      
      QJsonDocument jsonDoc(QJsonDocument::fromJson(data));
      
      jsonTeleportData = jsonDoc.array();

      loadFile.close();
    }

  //------------------------------

  Vec axis;
  qreal angle;
  m_camRot.getAxisAngle(axis, angle);

 
  QJsonObject jsonInfo;

  jsonInfo["pos"] = QString("%1 %2 %3").arg(m_camPos.x).arg(m_camPos.y).arg(m_camPos.z);
  jsonInfo["rot"] = QString("%1 %2 %3 %4").arg(axis.x).arg(axis.y).arg(axis.z).arg(angle);
  jsonInfo["scale"] = QString("%1 %2").arg(m_scaleFactor).arg(m_flightSpeed);
  {
    float val = m_leftMenu.value("edges");
    jsonInfo["edges"] = QString("%1").arg(val);
  }
  {
    float val = m_leftMenu.value("softness");
    jsonInfo["softness"] = QString("%1").arg(val);
  }
  {
    float val = m_leftMenu.value("bright");
    jsonInfo["bright"] = QString("%1").arg(val);
  }
  {
    float val = m_leftMenu.value("explode");
    jsonInfo["explode"] = QString("%1").arg(val);
  }
  jsonInfo["caption"] = "DrishtiMesh-VR";

  
  jsonTeleportNode["teleport"] = jsonInfo ;

  jsonTeleportData << jsonTeleportNode;

  //------------------------------

  QJsonDocument saveDoc(jsonTeleportData);  
  QFile saveFile(jsonfile);
  saveFile.open(QIODevice::WriteOnly);
  saveFile.write(saveDoc.toJson());
  saveFile.flush();
  saveFile.close();

  if (!jsondir.exists("teleport.json"))
    QMessageBox::information(0, "Problem", QString("Data not written to %1 -- WHY ???").arg(jsonfile)); 

  CaptionWidget::setText("hud", "Keyframe Saved");
  CaptionWidget::blinkAndHide("hud", 500);
  emit playTeleportSound();
}

void
VR::updateScale(int scl)
{
}

void
VR::setShowTimeseriesMenu(bool sm)
{  
  m_currPanel = 0;
  //m_leftMenu.setCurrentMenu(m_menuPanels[m_currPanel]);
}

void
VR::nextMenu()
{
  //m_currPanel = (m_currPanel+1)%m_menuPanels.count();
  //m_leftMenu.setCurrentMenu(m_menuPanels[m_currPanel]);
}

void
VR::previousMenu()
{
  //m_currPanel = (m_menuPanels.count() + m_currPanel-1)%m_menuPanels.count();
  //m_leftMenu.setCurrentMenu(m_menuPanels[m_currPanel]);
}

void
VR::renderMenu(vr::Hmd_Eye eye)
{
  { // draw caption widgets
    QMatrix4x4 mvp = currentViewProjection(eye);
    QMatrix4x4 hmdMat = m_matrixDevicePose[vr::k_unTrackedDeviceIndex_Hmd];
      
    for(int i=0; i<CaptionWidget::widgets.count(); i++)
      CaptionWidget::widgets[i]->draw(mvp, hmdMat);
  }

  {
    QMatrix4x4 mvp = currentViewProjection(eye);
    QMatrix4x4 matL = m_matrixDevicePose[m_leftController];
    m_leftMenu.draw(mvp, matL, m_triggerActiveRight);
  }
}


int
VR::checkOptions(int triggered)
{
  QMatrix4x4 matL = m_matrixDevicePose[m_leftController];
  QMatrix4x4 matR = m_matrixDevicePose[m_rightController];
  return m_leftMenu.checkOptions(matL, matR, triggered);
}

void
VR::renderSkyBox(vr::Hmd_Eye eye)
{
  if (!m_showSkybox)
    return;

  // blend front to back
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE);
  //glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  QVector3D hpos = hmdPosition();
  //QMatrix4x4 mvp = viewProjection(eye);

  // ignore model transformations for skybox
  // just apply HMD transforms
  QMatrix4x4 mvp = currentViewProjection(eye);
  
  m_skybox.draw(mvp, hpos, 100.0/m_coordScale);

  glDisable(GL_BLEND);
}

void
VR::showHUD(vr::Hmd_Eye eye,
	    GLuint texId, QSize texSize)
{
//  //glDepthMask(GL_FALSE); // disable writing to depth buffer
  glDisable(GL_DEPTH_TEST);

  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  QMatrix4x4 mvp = currentViewProjection(eye);

  glUseProgram(ShaderFactory::rcShader());
  GLint *rcShaderParm = ShaderFactory::rcShaderParm();
  glUniformMatrix4fv(rcShaderParm[0], 1, GL_FALSE, mvp.data() );  
  glUniform1i(rcShaderParm[1], 4); // texture
  glUniform3f(rcShaderParm[2], 0,0,0); // color
  glUniform3f(rcShaderParm[3], 0,0,0); // view direction
  glUniform1f(rcShaderParm[4], 0.5); // opacity modulator
  glUniform1i(rcShaderParm[5], 5); // apply texture
  glUniform1f(rcShaderParm[6], 50); // pointsize
  glUniform1f(rcShaderParm[7], 0); // mix color

  QMatrix4x4 mat = m_matrixDevicePose[vr::k_unTrackedDeviceIndex_Hmd];

  QVector3D cen =QVector3D(mat * QVector4D(0,0,0,1));
  QVector3D cx = QVector3D(mat * QVector4D(1,0,0,1));
  QVector3D cy = QVector3D(mat * QVector4D(0,1,0,1));
  QVector3D cz = QVector3D(mat * QVector4D(0,0,1,1));

  int texWd = texSize.width();
  int texHt = texSize.height();
  float frc = 1.0/qMax(texWd, texHt);

  QVector3D rDir = (cx-cen).normalized();
  QVector3D uDir = (cy-cen).normalized();
  QVector3D vDir = (cz-cen).normalized();

  cen = cen - vDir - 0.5*uDir;

  QVector3D fv = 0.5*(uDir-vDir).normalized();
  QVector3D fr = 0.5*rDir;

  QVector3D vu = frc*texHt*fv;
  QVector3D vr0 = cen - frc*texWd*0.5*fr;
  QVector3D vr1 = cen + frc*texWd*0.5*fr;

  QVector3D v0 = vr0;
  QVector3D v1 = vr0 + vu;
  QVector3D v2 = vr1 + vu;
  QVector3D v3 = vr1;

  float vertData[50];

  vertData[0] = v0.x();
  vertData[1] = v0.y();
  vertData[2] = v0.z();
  vertData[3] = 0;
  vertData[4] = 0;
  vertData[5] = 0;
  vertData[6] = 0.0;
  vertData[7] = 0.0;

  vertData[8] = v1.x();
  vertData[9] = v1.y();
  vertData[10] = v1.z();
  vertData[11] = 0;
  vertData[12] = 0;
  vertData[13] = 0;
  vertData[14] = 0.0;
  vertData[15] = 1.0;

  vertData[16] = v2.x();
  vertData[17] = v2.y();
  vertData[18] = v2.z();
  vertData[19] = 0;
  vertData[20] = 0;
  vertData[21] = 0;
  vertData[22] = 1.0;
  vertData[23] = 1.0;

  vertData[24] = v3.x();
  vertData[25] = v3.y();
  vertData[26] = v3.z();
  vertData[27] = 0;
  vertData[28] = 0;
  vertData[29] = 0;
  vertData[30] = 1.0;
  vertData[31] = 0.0;


  // Create and populate the index buffer

  glBindVertexArray(m_boxVID);  
  glBindBuffer(GL_ARRAY_BUFFER, m_boxV);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_boxIB);  

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0,  // attribute 0
			3,  // size
			GL_FLOAT, // type
			GL_FALSE, // normalized
			sizeof(float)*8, // stride
			(char *)NULL + 200); // starting offset
  
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1,  // attribute 1
			3,  // size
			GL_UNSIGNED_BYTE, // type
			GL_FALSE, // normalized
			sizeof(float)*8, // stride
			(char *)NULL + 200 + sizeof(float)*3 );

  glEnableVertexAttribArray( 2 );
  glVertexAttribPointer( 2,
			 2,
			 GL_FLOAT,
			 GL_FALSE, 
			 sizeof(float)*8,
			 (char *)NULL + 200 + sizeof(float)*6 );

  glActiveTexture(GL_TEXTURE4);
  glBindTexture(GL_TEXTURE_2D, texId);
  //glBindTexture(GL_TEXTURE_2D, Global::boxSpriteTexture());
  glEnable(GL_TEXTURE_2D);
  
  glBufferSubData(GL_ARRAY_BUFFER,
		  200, // offset of 200 bytes
		  sizeof(float)*8*4,
		  &vertData[0]);

  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0);  

  glBindVertexArray(0);

  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);  
  glDisableVertexAttribArray(2);  

  glBindVertexArray(0);

  glUseProgram(0);

  glDisable(GL_TEXTURE_2D);

  glDisable(GL_BLEND);

  //glDepthMask(GL_TRUE); // enable writing to depth buffer
  glEnable(GL_DEPTH_TEST);
}

void
VR::genClipPlaneWidget(QMatrix4x4 mat,
		       float *vertData)
{
  QVector3D cen= QVector3D(mat * QVector4D(0,0,0,1));
  QVector3D cx = QVector3D(mat * QVector4D(1,0,0,1));
  QVector3D cy = QVector3D(mat * QVector4D(0,1,0,1));
  QVector3D cz = QVector3D(mat * QVector4D(0,0,1,1));

  QVector3D rDir = (cx-cen).normalized();
  QVector3D uDir = (cy-cen).normalized();
  QVector3D vDir = (cz-cen).normalized();

  cen -= vDir;
  QVector3D fv = vDir;
  QVector3D fr = rDir;

  QVector3D vu = fv;
  QVector3D vr0 = cen - 0.5*fr;
  QVector3D vr1 = cen + 0.5*fr;
  
  
  QVector3D v0 = vr0;
  QVector3D v1 = vr0 + vu;
  QVector3D v2 = vr1 + vu;
  QVector3D v3 = vr1;

  
  vertData[0] = v0.x();
  vertData[1] = v0.y();
  vertData[2] = v0.z();
  vertData[3] = 0;
  vertData[4] = 0;
  vertData[5] = 0;
  vertData[6] = 0.0;
  vertData[7] = 0.0;

  vertData[8] =  v1.x();
  vertData[9] =  v1.y();
  vertData[10] = v1.z();
  vertData[11] = 0;
  vertData[12] = 0;
  vertData[13] = 0;
  vertData[14] = 0.0;
  vertData[15] = 1.0;

  vertData[16] = v2.x();
  vertData[17] = v2.y();
  vertData[18] = v2.z();
  vertData[19] = 0;
  vertData[20] = 0;
  vertData[21] = 0;
  vertData[22] = 1.0;
  vertData[23] = 1.0;

  vertData[24] = v3.x();
  vertData[25] = v3.y();
  vertData[26] = v3.z();
  vertData[27] = 0;
  vertData[28] = 0;
  vertData[29] = 0;
  vertData[30] = 1.0;
  vertData[31] = 0.0;
}

void
VR::drawClipInVR(vr::Hmd_Eye eye)
{  
  glDepthMask(GL_TRUE); // enable writing to depth buffer

  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE);

  QMatrix4x4 mvp = currentViewProjection(eye) *
                   m_matrixDevicePose[m_rightController];


  glUseProgram(ShaderFactory::rcShader());
  GLint *rcShaderParm = ShaderFactory::rcShaderParm();
  glUniformMatrix4fv(rcShaderParm[0], 1, GL_FALSE, mvp.data() );  
  glUniform1i(rcShaderParm[1], 4); // texture
  glUniform3f(rcShaderParm[2], 1.0,1.0,1.0); // color
  glUniform3f(rcShaderParm[3], 0,0,0); // view direction
  glUniform1f(rcShaderParm[4], 0.3); // opacity modulator
  glUniform1i(rcShaderParm[5], 1); // just color
  glUniform1f(rcShaderParm[6], 100); // pointsize
  glUniform1f(rcShaderParm[7], 1.0); // mix color

    
  // Create and populate the index buffer
  glBindVertexArray(m_boxVID);  
  glBindBuffer(GL_ARRAY_BUFFER, m_boxV);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_boxIB);  

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0,  // attribute 0
			3,  // size
			GL_FLOAT, // type
			GL_FALSE, // normalized
			sizeof(float)*8, // stride
			(char *)NULL + 200); // starting offset
  
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1,  // attribute 1
			3,  // size
			GL_UNSIGNED_BYTE, // type
			GL_FALSE, // normalized
			sizeof(float)*8, // stride
			(char *)NULL + 200 + sizeof(float)*3 );

  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2,
			2,
			GL_FLOAT,
			GL_FALSE, 
			sizeof(float)*8,
			(char *)NULL + 200 + sizeof(float)*6 );


  glUniform3f(rcShaderParm[2], 1.0,1.0,0.5); // color
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0);

  
  glBindVertexArray(0);

  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);  
  glDisableVertexAttribArray(2);  

  glBindVertexArray(0);

  glUseProgram(0);

  glDisable(GL_TEXTURE_2D);

  glDisable(GL_BLEND);
}

QVector3D
VR::rightPosInObjectSpace(float ahead)
{
  QMatrix4x4 mat = m_matrixDevicePose[m_rightController];
  QVector3D pos = QVector3D(m_final_xformInverted.map(mat * QVector4D(0,0,ahead,1)));
  return pos;
}

QVector3D
VR::leftPosInObjectSpace(float ahead)
{
  QMatrix4x4 mat = m_matrixDevicePose[m_leftController];
  QVector3D pos = QVector3D(m_final_xformInverted.map(mat * QVector4D(0,0,ahead,1)));
  return pos;
}
