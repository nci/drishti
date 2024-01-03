#ifndef VR_H
#define VR_H

#include "vrmenu.h"
#include "cubemap.h"
#include "captionwidget.h"

#include <QFile>
#include <QMatrix4x4>
#include <QVector2D>
#include <QVector3D>
#include <QTimer>
#include <QOpenGLWidget>
#include <QOpenGLFramebufferObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>


#include <openvr.h>

#include "cglrendermodel.h"

#include <QGLViewer/qglviewer.h>
using namespace qglviewer;


class VR : public QObject
{
 Q_OBJECT

 public :
  VR();
  ~VR();


  void shutdown();

  void initVR();
  bool vrEnabled() { return (m_hmd > 0); }

  void setHeadSetType(int hs) { m_headSetType = hs; }

  int screenWidth() { return m_eyeWidth; }
  int screenHeight() { return m_eyeHeight; }

  void initModel(Vec, Vec);
  void initModel(QVector3D, QVector3D);

  void setDataDir(QString);

  bool pointingToMenu() { return m_leftMenu.pointingToMenu(); }

  void setGravity(bool g) { m_gravity = g; }
  void setSkybox(bool sm) { m_showSkybox = sm; }
  void setShowTimeseriesMenu(bool);
  void setGroundHeight(float gh) { m_groundHeight = gh; }
  void setTeleportScale(float gh) { m_teleportScale = gh; }

  void setGenDrawList(bool b) { m_genDrawList = b; }
  bool genDrawList() { return m_genDrawList; }
  void resetGenDrawList() { m_genDrawList = false; }

  void resetNextStep() { m_nextStep = 0; }
  int nextStep() { return m_nextStep; }
  void takeNextStep()
  {
    m_genDrawList = true;
    m_nextStep = 1;
  }

  void setTimeStep(QString);
  void setDataShown(QString);

  QVector3D hmdPosition()
  { return getModelSpacePosition(vr::k_unTrackedDeviceIndex_Hmd); }

  QVector3D leftControllerPosition()
  { return getModelSpacePosition(m_leftController); }

  QVector3D rightControllerPosition()
  { return getModelSpacePosition(m_rightController); }

  QVector3D vrHmdPosition()
  { return getPosition(vr::k_unTrackedDeviceIndex_Hmd); }

  QVector3D vrLeftControllerPosition()
  { return getPosition(m_leftController); }

  QVector3D vrRightControllerPosition()
  { return getPosition(m_rightController); }


  QVector3D viewDir();
  QVector3D upDir();
  QVector3D rightDir();

  QVector3D vrViewDir();
  QVector3D vrUpDir();
  QVector3D vrRightDir();


  QMatrix4x4 viewProjection(vr::Hmd_Eye);
  QMatrix4x4 currentViewProjection(vr::Hmd_Eye);
  QMatrix4x4 modelView(vr::Hmd_Eye eye);
  QMatrix4x4 modelView();

  QMatrix4x4 matrixDevicePoseLeft();
  QMatrix4x4 matrixDevicePoseRight();

  QMatrix4x4 final_xform() { return m_final_xform; }
  QMatrix4x4 final_xformInverted() { return m_final_xformInverted; }

  Vec getModelSpace(Vec);
  QVector3D getModelSpace(QVector3D);

  bool preDraw();
  void postDraw();

  void bindBuffer(vr::Hmd_Eye);

  void bindLeftBuffer();
  void bindRightBuffer();

  void postDrawLeftBuffer();
  void postDrawRightBuffer();


  bool play() { return m_play; }


  float coordScale() { return m_coordScale; }
  float scaleFactor() { return m_scaleFactor; }
  float flightSpeed() { return m_flightSpeed; }
  float pointSize() { return m_pointSize; }

  bool edges() { return m_edges; }
  bool softShadows() { return m_softShadows; }

  void renderSkyBox(vr::Hmd_Eye);
  void showHUD(vr::Hmd_Eye, GLuint, QSize);

  bool reUpdateMap() { return m_updateMap; }
  void resetUpdateMap() { m_updateMap = false; }

  GLuint leftTexture() { return m_leftBuffer->texture(); }
  GLuint rightTexture() { return m_rightBuffer->texture(); }
  GLuint resolveTexture() { return m_resolveBuffer->texture(); }

  QOpenGLFramebufferObject* leftBuffer() { return m_leftBuffer; }
  QOpenGLFramebufferObject* rightBuffer() { return m_rightBuffer; }
  QOpenGLFramebufferObject* resolveBuffer() { return m_resolveBuffer; }

  void setMaxVolumeSeries(int n) { m_maxVolumeSeries = n; }

  QVector3D rightPosInObjectSpace(float);
  QVector3D leftPosInObjectSpace(float);
  void setAllowMovement(bool b) { m_allowMovement = b; }
  bool triggerRight() { return m_triggerActiveRight; }
  bool triggerLeft() { return m_triggerActiveLeft; }
  bool gripRight() { return m_gripActiveRight; }
  bool griprLeft() { return m_gripActiveLeft; }
  bool rightA() { return (m_stateRight.ulButtonPressed &
			  vr::ButtonMaskFromId(vr::k_EButton_ApplicationMenu));}
  bool rightB() { return (m_stateRight.ulButtonPressed &
			  vr::ButtonMaskFromId(vr::k_EButton_A));}

			     
 public slots:
   void scaleModel(int);
   void resetModel();
   void updateScale(int);  
   void updateSoftShadows(bool);
   void updateEdges(bool);

   void setExamineMode(bool);

   void toggle(QString, float);
    
   void setDemoMode(bool);

   void updateTeleport();
   void nextTeleport();
   void prevTeleport();

   void probeMeshName(QString);
  
 signals :
  void removeClip();
  void modifyClip(int, QVector3D,QVector3D,QVector3D);

  void setEdges(float);
  void setSoftness(float);
  void setGamma(float);
  void setExplode(float);
  void setHeadGap(float);
  void setPaintMode(bool);

  void prevVolume();
  void nextVolume();

  void prevKeyFrame();
  void nextKeyFrame();

  void playTeleportSound();

  void pointProbe(QVector3D);
  void probeMeshMove(QVector3D);
  void resetProbe();
  
 private :

  Vec m_camPos;
  Quaternion m_camRot;
  Quaternion m_camRotLocal;
  Vec m_sceneCen;

  bool m_allowMovement;
  
  vr::IVRSystem *m_hmd;
  vr::TrackedDevicePose_t m_trackedDevicePose[vr::k_unMaxTrackedDeviceCount];
  QMatrix4x4 m_matrixDevicePose[vr::k_unMaxTrackedDeviceCount];
  
  vr::TrackedDeviceIndex_t m_leftController;
  vr::TrackedDeviceIndex_t m_rightController;
  
  vr::IVRRenderModels *m_pRenderModels;
  bool m_rbShowTrackedDevice[ vr::k_unMaxTrackedDeviceCount ];
  QList< CGLRenderModel * > m_vecRenderModels;
  CGLRenderModel *m_rTrackedDeviceToRenderModel[ vr::k_unMaxTrackedDeviceCount ];

  QString m_leftControllerName;
  QString m_rightControllerName;
  QStringList m_leftComponentNames;
  QStringList m_rightComponentNames;
  QList< CGLRenderModel* > m_leftRenderModels;
  QList< CGLRenderModel* > m_rightRenderModels;

  QMatrix4x4 m_leftProjection, m_leftPose;
  QMatrix4x4 m_rightProjection, m_rightPose;
  QMatrix4x4 m_hmdPose;

  bool m_demoMode;
  
  bool m_autoRotate;

  QMatrix4x4 m_final_xform;
  QMatrix4x4 m_final_xformInverted;
  
  int m_headSetType;
  uint32_t m_eyeWidth, m_eyeHeight;
  
  QOpenGLFramebufferObject *m_leftBuffer;
  QOpenGLFramebufferObject *m_rightBuffer;
  QOpenGLFramebufferObject *m_resolveBuffer;

  bool m_triggerActiveRight;
  bool m_triggerActiveLeft;
  bool m_triggerActiveBoth;
  
  bool m_touchPressActiveRight;
  bool m_touchPressActiveLeft;

  bool m_touchActiveRight;
  bool m_touchActiveLeft;

  bool m_gripActiveRight;
  bool m_gripActiveLeft;

  bool m_flightActive;
  bool m_touchTriggerActiveLeft;
  
  bool m_updateMap;

  QTimer m_flyTimer;

  QVector3D m_prevDirection;
  QVector3D m_prevPosL, m_prevPosR;
  float m_lrDist;

  bool m_genDrawList;

  int m_nextStep;

  int m_maxVolumeSeries;
  
  bool m_play;

  QVector3D m_startTranslate;
  QVector3D m_scaleCenter;
  QVector3D m_rcDir;
  
  QTime m_ctrlerMoveTime;
  QQuaternion m_rotQuat;
  QMatrix4x4 m_rotQ;
  QVector3D m_rotCenter;
  
  QVector2D m_touchPoint;
  
  float m_touchX, m_touchY;
  float m_startTouchX, m_startTouchY;
  float m_pointSize;
  float m_startPS;

  float m_scaleFactor;
  float m_prevScaleFactor;

  float m_speedDamper;
  float m_flightSpeed;
  float m_prevFlightSpeed;

  QVector3D m_coordMin;
  QVector3D m_coordMax;
  QVector3D m_coordCen;
  float m_coordScale;

  bool m_showLeft, m_showRight;  

  //-----------------
  GLhandleARB m_pshader;
  GLint m_pshaderParm[10];
  //-----------------

  //-----------------
  GLuint m_boxVID;
  GLuint m_boxV;
  GLuint m_boxIB;
  int m_axesPoints;
  int m_nboxPoints;
  //-----------------

  QString m_dataDir;
  float *m_depthBuffer;


  QList<QVector3D> m_teleports;
  int m_currTeleportNumber;
  QJsonObject m_teleportJsonInfo;
  QStringList m_dataToTeleport;
  QString m_captionToTeleport;
  float m_rayDepthToTeleport;
  
  QTimer *m_teleportTimer;
  Vec m_startCamPos, m_endCamPos;
  Quaternion m_startCamRot, m_endCamRot;
  int m_telTime;
  float m_scaleStart, m_scaleEnd;
  
  bool m_gravity;
  bool m_showSkybox;
  float m_groundHeight;
  float m_teleportScale;

  VrMenu m_leftMenu;
  QStringList m_menuPanels;
  int m_currPanel;

  QList<CaptionWidget*> m_captionWidgets;

  CubeMap m_skybox;

  bool m_edges;
  bool m_softShadows;

  int m_modeType;
  bool m_examineMode;
  bool m_showHUD;

  bool m_clip;
  int m_clipType;

  bool m_grabMode;
  
  
  void buildAxes();
  bool buildTeleport();
  void renderAxes(vr::Hmd_Eye);
  void createShaders();
  void buildAxesVB();


  QString getTrackedDeviceString(vr::TrackedDeviceIndex_t device,
				 vr::TrackedDeviceProperty prop,
				 vr::TrackedPropertyError *error = 0);
  
  void updatePoses();
  bool updateInput();

  void ProcessVREvent(const vr::VREvent_t & event);
  
  void renderEye(vr::Hmd_Eye eye);

  QMatrix4x4 hmdMVP(vr::Hmd_Eye);
  QMatrix4x4 leftMVP(vr::Hmd_Eye);
  QMatrix4x4 rightMVP(vr::Hmd_Eye);

  QVector3D getPosition(const vr::HmdMatrix34_t);
  QVector3D getPosition(QMatrix4x4);
  float copysign(float, float);
  QQuaternion getQuaternion(vr::HmdMatrix34_t);
  QQuaternion getQuaternion(QMatrix4x4);
  QVector3D getModelSpacePosition(vr::TrackedDeviceIndex_t);
  QVector3D getPosition(vr::TrackedDeviceIndex_t);
  
  
  QMatrix4x4 vrMatrixToQt(const vr::HmdMatrix34_t&);
  QMatrix4x4 vrMatrixToQt(const vr::HmdMatrix44_t&);
    
  QMatrix4x4 modelViewNoHmd();
  
  vr::VRControllerState_t m_stateRight;
  vr::VRControllerState_t m_stateLeft;
    
  bool isTriggered(vr::VRControllerState_t&);
  bool isTouched(vr::VRControllerState_t&);
  bool isTouchPressed(vr::VRControllerState_t&);
  bool isGripped(vr::VRControllerState_t&);

  void leftTriggerPressed();
  void leftTriggerMove();
  void leftTriggerReleased();

  void rightTriggerPressed();
  void rightTriggerMove();
  void rightTriggerReleased();

  void bothTriggerPressed();
  void bothTriggerMove();
  void bothTriggerReleased();

  void leftTouchPressed();
  void leftTouchPressMove();
  void leftTouchPressReleased();

  void leftTouched();
  void leftTouchMove();
  void leftTouchReleased();

  void rightTouchPressed();
  void rightTouchPressMove();
  void rightTouchPressReleased();

  void rightTouched();
  void rightTouchMove();
  void rightTouchReleased();

  void leftGripPressed();
  void leftGripMove();
  void leftGripReleased();

  void rightGripPressed();
  void rightGripMove();
  void rightGripReleased();
  
  bool isButtonTriggered(vr::VRControllerState_t&, vr::EVRButtonId);
  bool m_aActive;
  void aButtonPressed();
  void aButtonReleased();
  bool m_bActive;
  void bButtonPressed();
  void bButtonReleased();
  bool m_yActive;
  void yButtonPressed();
  void yButtonReleased();

  QMatrix4x4 quatToMat(QQuaternion); 

  bool getControllers();
  void renderControllers(vr::Hmd_Eye);
  void renderControllerLabels(vr::Hmd_Eye);
  void setupRenderModels();
  void setupRenderModelForTrackedDevice(int, vr::TrackedDeviceIndex_t);
  CGLRenderModel *findOrLoadRenderModel(QString);


  void genEyeMatrices();


  void saveTeleportNode();
  void teleport();


  void nextMenu();
  void previousMenu();
  void renderMenu(vr::Hmd_Eye);
  int checkOptions(int);

  bool showRight() { return m_showRight; }
  bool showLeft() { return m_showLeft; }

  QMatrix4x4 initXform(float, float, float, float);

  void genClipPlaneWidget(QMatrix4x4, float*);
  void drawClipInVR(vr::Hmd_Eye);

  QMatrix4x4 modelViewFromCamera();

  int m_menuMeshNameIdx;
  QList<GLuint> m_menuTex;
  QList<QMatrix4x4> m_menuMat;
  void generateButtonInfo();

  void rightProbe();
};


#endif
