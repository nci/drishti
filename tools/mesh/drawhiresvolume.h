#ifndef DRAWHIRESVOLUME_H
#define DRAWHIRESVOLUME_H

#include <GL/glew.h>
#include <QGLViewer/qglviewer.h>
using namespace qglviewer;

#include "clipplane.h"
#include "lightinginformation.h"
#include "bricks.h"
#include "pathobject.h"

#include <QGLFramebufferObject>

class Viewer;
class Volume;

class DrawHiresVolume : public QObject
{
  Q_OBJECT

 public :

  DrawHiresVolume(Viewer*);
  ~DrawHiresVolume();

  bool loadingData() { return m_loadingData; }
  void setRaycastMode(bool m) { m_rcMode = m; }
  
  void setDOF(int b, float fp) { m_dofBlur = b, m_focalPoint = fp; }
  void dof(int& b, float& fp) { b = m_dofBlur, fp = m_focalPoint; }

  Vec pointUnderPixel(QPoint, bool&);

  double* brick0Xform();

  void overwriteDataMinMax(Vec, Vec);
  void setBrickBounds(Vec, Vec);
  
  void renew();
  void cleanup();

  bool raised();
  void raise();
  void lower();

  void loadVolume();

  void setBricks(Bricks*);

  LightingInformation lightInfo();
  QList<BrickInformation> bricks();

  Vec volumeMin();
  Vec volumeMax();


  void drawDragImage();
  void drawStillImage();

  void setImageSizeRatio(float);

  bool keyPressEvent(QKeyEvent*);


  void getClipForMask(QList<Vec>&,
		      QList<Vec>&);

  float focusDistance() { return m_focusDistance; }

 public slots :
  void updateScaling();
  void setLightInfo(LightingInformation);

  void updateLightVector(Vec);
  void applyBackplane(bool);
  void updateLightDistanceOffset(float);
  void updateSoftness(float);
  void updateEdges(float);
  void updateShadowIntensity(float);
  void updateValleyIntensity(float);
  void updatePeakIntensity(float);
  void updateShadowFOV(float);
  void updateBackplaneShadowScale(float);
  void updateBackplaneIntensity(float);
  void updateHighlights(Highlights);

  void saveForDrishtiPrayog(QString);

  void drawVR(GLdouble*, GLfloat*, Vec, 
	      GLdouble*, Vec, Vec,
	      int, int);

 private :
  float m_focalPoint;
  int m_dofBlur;

  float m_frontOpMod;
  float m_backOpMod;

  bool m_showing;
  bool m_forceBackToFront;
  
  bool m_useScreenShadows;
  int m_shadowLod;

  int m_shdNum;
  GLuint m_shdBuffer;
  GLuint m_shdTex[2];

  GLuint m_dofBuffer;
  GLuint m_dofTex[3];

  int m_currentVolume;

  GLint m_currentDrawbuf;

  int m_renderQuality;
  int m_drawImageType;

  bool m_drawGeometryPresent;

  Viewer *m_Viewer;

  unsigned char *m_histData1D;
  unsigned char *m_histData2D;
  QImage m_histogram1D;
  QImage m_histogram2D;
  unsigned char *m_histDragData1D;
  unsigned char *m_histDragData2D;
  QImage m_histogramDrag1D;
  QImage m_histogramDrag2D;

  Vec m_dataMin, m_dataMax, m_dataSize;
  Vec m_virtualTextureSize;
  Vec m_virtualTextureMin;
  Vec m_virtualTextureMax;

  LightingInformation m_lightInfo;
  LightingInformation m_savedLightInfo;
  Bricks *m_bricks;

  bool m_backlit;
  Vec m_lightPosition;
  Vec m_lightVector;

  int m_shadowWidth;
  int m_shadowHeight;
  int m_dataTexSize;
  GLuint* m_dataTex;
  GLuint m_lutTex;
  QGLFramebufferObject *m_shadowBuffer;
  QGLFramebufferObject *m_blurredBuffer;

  int m_loadFromDisk;
  QList<Vec> m_textureSlab;

  GLhandleARB m_lutShader;
  GLhandleARB m_passthruShader;
  GLhandleARB m_defaultShader;
  GLhandleARB m_blurShader;
  GLhandleARB m_backplaneShader1;
  GLhandleARB m_backplaneShader2;


  GLint m_vertParm[50];

  GLint m_lutParm[50];
  GLint m_defaultParm[60];
  GLint m_blurParm[50];
  GLint m_backplaneParm1[50];
  GLint m_backplaneParm2[50];

  Vec m_enclosingBox[8];
  Vec m_axisArrow[3];
  
  int m_numBricks;
  DrawBrickInformation m_drawBrickInformation;

  float m_focusDistance, m_screenWidth;
  int m_cameraWidth, m_cameraHeight;
  float m_projectionMatrix[16];
  float m_adjustedProjectionMatrix[16];

  bool m_rcMode;
  bool m_loadingData;

  bool m_updateSubvolume;

  QList<Vec> m_clipPos;
  QList<Vec> m_clipNormal;
  QList<PathObject> m_paths;

  int m_mixvol;
  bool m_mixColor, m_mixOpacity;
  float m_interpVol;
  int m_interpolateVolumes;
  bool m_mixTag;

  float m_imgSizeRatio;

  QString m_image2VolumeFile;
  bool m_saveImage2Volume;

  void draw(bool);

  void loadCameraMatrices();

  void drawGeometryOnly();
  void drawGeometry();
  
  void drawGeometryOnlyVR(GLdouble*, GLfloat*, Vec, 
			  GLdouble*, Vec, Vec,
			  int, int);
  void drawGeometryVR(GLdouble*, GLfloat*, Vec, 
		      GLdouble*, Vec, Vec,
		      int, int);

  void collectEnclosingBoxInfo();
  void drawAxisText();

  void drawBackground();
};

#endif
