#ifndef DRAWHIRESVOLUME_H
#define DRAWHIRESVOLUME_H

#include <GL/glew.h>
#include <QGLViewer/qglviewer.h>
using namespace qglviewer;

#include "clipplane.h"
#include "classes.h"
#include "lightinginformation.h"
#include "bricks.h"
#include "cropobject.h"
#include "pathobject.h"
#include "volumefilemanager.h"

#include <QGLFramebufferObject>

class Viewer;
class Volume;

class DrawHiresVolume : public QObject
{
  Q_OBJECT

 public :

  DrawHiresVolume(Viewer*, Volume*);
  ~DrawHiresVolume();

  bool loadingData() { return m_loadingData; }
  void setRaycastMode(bool m) { m_rcMode = m; }
  
  void setDOF(int b, float fp) { m_dofBlur = b, m_focalPoint = fp; }
  void dof(int& b, float& fp) { b = m_dofBlur, fp = m_focalPoint; }

  Vec pointUnderPixel(QPoint, bool&);

  double* brick0Xform();

  void overwriteDataMinMax(Vec, Vec);
  
  int numOfTextureSlabs() { return m_textureSlab.count(); }
  int getSubvolumeSubsamplingLevel();
  int getDragSubsamplingLevel();
  void getSliceTextureSize(int&, int&);
  void getDragTextureSize(int&, int&);

  void setSubvolumeUpdates(bool);
  bool subvolumeUpdates();
  void disableSubvolumeUpdates();
  void enableSubvolumeUpdates();

  void renew();
  void cleanup();

  bool raised();
  void raise();
  void lower();

  void loadVolume();

  void setBricks(Bricks*);

  QImage histogramImage1D();
  QImage histogramImage2D();
  int* histogram1D();
  int* histogram2D();

  LightingInformation lightInfo();
  QList<BrickInformation> bricks();

  Vec volumeSize();
  Vec volumeMin();
  Vec volumeMax();

  void initShadowBuffers(bool force=false);

  void drawDragImage(float);
  void drawStillImage(float);
  void createShaders();

  void setImageSizeRatio(float);

  bool keyPressEvent(QKeyEvent*);

  int renderQuality();

  void getClipForMask(QList<Vec>&,
		      QList<Vec>&);

  float focusDistance() { return m_focusDistance; }

  void getMix(int &mv, bool &mc, bool &mo, bool &mt)
  {
    mv=m_mixvol;
    mc=m_mixColor;
    mo=m_mixOpacity;
    mt=m_mixTag;
  }

  GLuint* dataTextureID() { return m_dataTex; }

 public slots :
  QString getResliceFileName(bool border=false);
  void saveImage2Volume(QString);
  void collectBrickInformation(bool force=false);
  void updateAndLoadPruneTexture();
  void updateAndLoadLightTexture();
  void loadTextureMemory();  
  void updateScaling();
  void updateSubvolume();
  void updateSubvolume(int, Vec, Vec, bool force=false);
  void updateSubvolume(int, int, Vec, Vec, bool force=false);
  void updateSubvolume(int, int, int, Vec, Vec, bool force=false);
  void updateSubvolume(int, int, int, int, Vec, Vec, bool force=false);
  void setLightInfo(LightingInformation);
  void setRenderQuality(int);

  void updateLightVector(Vec);
  void applyEmissive(bool);
  void applyLighting(bool);
  void applyShadows(bool);
  void applyBackplane(bool);
  void applyColoredShadows(bool);
  void updateLightDistanceOffset(float);
  void updateShadowBlur(float);
  void updateShadowScale(float);
  void updateShadowFOV(float);
  void updateShadowIntensity(float);
  void updateShadowColorAttenuation(float, float, float);
  void updateBackplaneShadowScale(float);
  void updateBackplaneIntensity(float);
  void updateHighlights(Highlights);
  void peel(bool);
  void peelInfo(int, float, float, float);

  void setCurrentVolume(int vnum=0);
  void generateHistogramImage();

  void checkCrops();
  void checkPaths();

  void setMix(int, bool, bool, float iv = 2.0f);
  void setMixTag(float mt) { m_mixTag = mt; }
  void setInterpolateVolumes(int);

  void runLutShader(bool);
  
  int dataTexSize() { return m_dataTexSize; }
  
  void resliceVolume(Vec, Vec, Vec, Vec, float, int, int);
  void resliceUsingPath(int, bool, int, int);
  void resliceUsingClipPlane(Vec, Quaternion, int,
			     QVector4D, float, int,
			     int, int);

  void saveForDrishtiPrayog(QString);

  void createDefaultShader();

  void getOpMod(float&, float&);
  void setOpMod(float, float);

 signals :
  void histogramUpdated(QImage, QImage);

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
  Volume *m_Volume;

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

  QList<ViewAlignedPolygon*> m_polygon;

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
  QList<CropObject> m_crops;
  QList<PathObject> m_paths;

  int m_mixvol;
  bool m_mixColor, m_mixOpacity;
  float m_interpVol;
  int m_interpolateVolumes;
  bool m_mixTag;

  float m_imgSizeRatio;

  QString m_image2VolumeFile;
  bool m_saveImage2Volume;

  void generateDragHistogramImage();

  void createPassThruShader();
  void createCopyShader();
  void createReduceShader();
  void createExtractSliceShader();
  void createBlurShader();
  void createBackplaneShader(float);

  int drawpoly(Vec, Vec,
	       Vec*, Vec*,
	       ViewAlignedPolygon*);

  void getDragRenderInfo(Vec&, int&, int&, int&);

  void drawSlicesDefault(Vec, Vec, Vec, int, float);

  void enableTextureUnits();
  void disableTextureUnits();

  void draw(float, bool);
  void drawDefault(Vec, Vec, Vec, int, float);

  void drawGeometryOnly();
  
  void getMinMaxVertices(float&, Vec&, Vec&);

  void loadCameraMatrices();

  void preDrawGeometry(int, int, Vec, Vec, Vec, bool fromclip=false);
  void postDrawGeometry();
  void drawGeometry(Vec, float, float, Vec,
		    bool, bool, Vec);

  void collectEnclosingBoxInfo();
  void drawAxisText();

  void drawBackground();

  void clipSlab(Vec, Vec, int&, Vec*, Vec*, Vec*);

  QList<int> getSlices(Vec, Vec, Vec, int);

  void postUpdateSubvolume(Vec, Vec);

  void getGeoSliceBound(int, int,
			Vec, Vec, Vec,
			float&, float&);
  void renderGeometry(int, int,
		      Vec, Vec, Vec,
		      bool, bool, Vec,
		      bool);
  void renderDragSlice(ViewAlignedPolygon*, bool, Vec);
  void renderSlicedSlice(int,
			 ViewAlignedPolygon*, bool,
			 int, int, int, int);

  void emptySpaceSkip();
  void bindDataTextures(int);
  void releaseDataTextures(int);

  void drawClipPlaneDefault(int, int, Vec, Vec, Vec,
			    int, Vec, float, int, int,
			    int, int, int, Vec);
  void drawClipPlaneInViewport(int, Vec, float,
			       int, int, int, Vec, bool);

  void drawPathInViewport(int, Vec, float,
			  int, int, int, Vec, bool);
  
  void setShader2DTextureParameter(bool, bool);
  void setRenderDefault();

  bool getSaveValue();
  void saveReslicedVolume(QString,
			  int, int, int, VolumeFileManager&,
			  bool tmpfile=false,
			  Vec vs=Vec(1,1,1),
			  int voxtype=0);

  void getTightFit(int, uchar*, int, int,
		   bool&, int&, int&, int&, int&, int&, int&);

  void calculateSurfaceArea(int,
			    int, int,
			    uchar*, uchar*, uchar*, uchar*,
			    int, int,
			    bool,
			    VolumeFileManager&);

  void screenShadow(int, int, int, int, float);

  void check_MIP();

  void depthOfFieldBlur(int,int,int,int,int);
};

#endif
