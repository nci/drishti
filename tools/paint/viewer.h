#ifndef VIEWER_H
#define VIEWER_H

#include <QGLViewer/qglviewer.h>
#include <QGLViewer/vec.h>
using namespace qglviewer;

#include <QGLFramebufferObject>

#include "ui_viewermenu.h"

#include "clipplane.h"
#include "crops.h"

#include "boundingbox.h"
#include "mybitarray.h"

#include"videoencoder.h"

class Viewer : public QGLViewer
{
  Q_OBJECT

 public :
  Viewer(QWidget *parent=0);
  ~Viewer();

  void init();
  void draw();

  void setGridSize(int, int, int);

  void setVolDataPtr(uchar*);
  void setMaskDataPtr(uchar*);

  void keyPressEvent(QKeyEvent*);
  void mousePressEvent(QMouseEvent*);
  void mouseReleaseEvent(QMouseEvent*);
  void mouseMoveEvent(QMouseEvent*);
  void enterEvent(QEvent*);
  void leaveEvent(QEvent*);
  
  float stillStep();
  float dragStep();

  float minGrad();
  float maxGrad();
  int gradType();
  void setMinGrad(float);
  void setMaxGrad(float);
  void setGradType(int);

  
  bool exactCoord();

  uchar* sketchPad() { return m_sketchPad; }
  void setUIPointer(Ui::ViewerMenu *vUI) { m_UI = vUI; }

  QList<Vec> clipPos();
  QList<Vec> clipNorm();


  Vec viewDir() { return camera()->viewDirection(); }
  Vec viewRight() { return camera()->rightVector(); }
  Vec viewUp() { return camera()->upVector(); }
  Vec camPos() { return camera()->position(); }
  bool perspectiveProjection()
  {
    if (camera()->type() == Camera::ORTHOGRAPHIC)
      return false;

    return true;
  }

  void generateBoxMinMax();

  QList<int> usedTags();

  void getBox(int&, int&, int&, int&, int&, int&);

  public slots :
    void GlewInit();  
    void resizeGL(int, int);
    void setPointSize(int p) { m_pointSize = p; update(); }
    void setPointScaling(int p) { m_pointScaling = p; update(); }
    void updateVoxels();
    void updateViewerBox(int, int, int, int, int, int);
    void setShowBox(bool);
    void setDSlice(int);
    void setWSlice(int);
    void setHSlice(int);
    void setShowPosition(bool);
    void uploadMask(int,int,int, int,int,int);
    void setSkipLayers(int);
    void setSkipVoxels(int);
    void setStillAndDragStep(float, float);
    void setExactCoord(bool);
    void showSketchPad(bool);
    void saveImage();
    void saveImageSequence();
    void nextFrame();
    void updateFilledBoxes();
    void boundingBoxChanged();
    void updateTF();
    void setAmb(int a) { m_amb = (float)a/10.0f; update(); };
    void setDiff(int a) { m_diff = (float)a/10.0f; update(); };
    void setSpec(int a) { m_spec = (float)a/10.0f; update(); };
    void setEdge(int e) { m_edge = e; update(); }
    void setShadow(int e) { m_shadow = e; update(); }
    void setShadowColor(Vec c) { m_shadowColor = c; update(); }
    void setEdgeColor(Vec c) { m_edgeColor = c; update(); }
    void setBGColor(Vec c) { m_bgColor = c; update(); }
    Vec shadowColor() { return m_shadowColor; }
    Vec edgeColor() { return m_edgeColor; }
    Vec bgColor() { return m_bgColor; }

    void setShadowOffsetX(int x) { m_shdX = x; update(); }
    void setShadowOffsetY(int y) { m_shdY = y; update(); }
    int shadowOffsetX() { return m_shdX; }
    int shadowOffsetY() { return m_shdY; }

    void setBoxSize(int);

    void stopDrawing();
    void startDrawing();

 private slots :
      void createRaycastShader();

 signals :
    void checkFileSave();
    void showBoxChanged(bool);

    void tagsUsed(QList<int>);

    void undoPaint3D();
    void paint3DStart();
    void paint3DEnd();
    void paint3D(Vec, Vec,
		 int, int, int,
		 int, int, bool);

    void changeImageSlice(int, int, int);
  
    void dilateConnected(int, int, int, Vec, Vec, int, bool);
    void erodeConnected(int, int, int, Vec, Vec, int);

    void dilateAllTags(Vec, Vec, int);
    void dilateAll(Vec, Vec, int, int);
    void erodeAll(Vec, Vec, int, int, int);
    void openAll(Vec, Vec, int, int, int);
    void closeAll(Vec, Vec, int, int, int);
  
    void tagUsingSketchPad(Vec, Vec);

    void mergeTags(Vec, Vec, int, int, bool);
    void stepTag(Vec, Vec, int, int);

    void sortLabels(Vec, Vec);
  
    void updateSliceBounds(Vec, Vec);

    void setVisible(Vec, Vec, int, bool);
    void resetTag(Vec, Vec, int);
    void reloadMask();
    void loadRawMask(QString);

    void shrinkwrap(Vec, Vec, int, bool, int);
    void shrinkwrap(Vec, Vec, int, bool, int,
		    bool, int, int, int, int);

    void tagTubes(Vec, Vec, int);
    void tagTubes(Vec, Vec, int,
		    bool, int, int, int, int);

    void connectedRegion(int, int, int,
			 Vec, Vec,
			 int, int);
    void smoothConnectedRegion(int, int, int,
			       Vec, Vec,
			       int, int);
    void smoothAllRegion(Vec, Vec,
		       int, int);

    void saveToROI(Vec, Vec, int);
    void roiOperation(Vec, Vec, int);
    void connectedComponents(Vec, Vec, int);
    void connectedComponentsPlus(Vec, Vec, int, int);
    void distanceTransform(Vec, Vec, int, int);
    void localThickness(Vec, Vec, int);

    void removeComponents(Vec, Vec, int);
    void removeLargestComponents(Vec, Vec, int);
  
    void hatchConnectedRegion(int, int, int,
			      Vec, Vec,
			      int, int,
			      int, int);
    void getVolume(Vec, Vec, int);
    void getSurfaceArea(Vec, Vec, int);

    void modifyOriginalVolume(Vec, Vec, int);
 private :
  Ui::ViewerMenu *m_UI;

  bool m_glewInitdone;
  bool m_draw;

  BoundingBox m_boundingBox;
  QList<Vec> m_bclipPos;
  QList<Vec> m_bclipNormal;
  
  float m_amb, m_diff, m_spec;
  int m_shadow;
  float m_edge;
  Vec m_shadowColor, m_edgeColor, m_bgColor;
  int m_shdX, m_shdY;

  int m_max3DTexSize;
  float m_memSize;
  int m_skipLayers;
  int m_skipVoxels;

  float m_stillStep, m_dragStep;

  float m_minGrad, m_maxGrad;
  
  qint64 m_depth, m_width, m_height;
  int m_minDSlice, m_maxDSlice;
  int m_minWSlice, m_maxWSlice;
  int m_minHSlice, m_maxHSlice;

  int m_cminD, m_cmaxD;
  int m_cminW, m_cmaxW;
  int m_cminH, m_cmaxH;

  
  int m_tag1, m_tag2;
  bool m_mergeTagTF;

  int m_voxChoice;
  bool m_showBox;

  uchar *m_volPtr;
  ushort *m_volPtrUS;
  ushort *m_maskPtrUS;

  int m_pointSkip;
  int m_pointSize;
  int m_pointScaling;

  MyBitArray m_bitmask;
  bool m_paintHit, m_carveHit;
  Vec m_target;

  bool m_showPosition;
  int m_dslice, m_wslice, m_hslice;

  int m_dbox, m_wbox, m_hbox, m_boxSize;
  QList<int> m_boxMinMax;
  MyBitArray m_filledBoxes;

  GLuint m_slcBuffer;
  GLuint m_rboId;
  GLuint m_slcTex[2];

  GLuint m_eBuffer;
  GLuint m_ebId;
  GLuint m_ebTex[3];

  GLuint m_dataTex;
  GLuint m_maskTex;
  GLuint m_tagTex;
  GLuint m_lutTex;
  int m_sslevel;
  Vec m_corner, m_vsize;

  ClipPlanes* m_clipPlanes;
  Crops* m_crops;
  
  GLhandleARB m_depthShader;
  GLint m_depthParm[20];

  GLhandleARB m_blurShader;
  GLint m_blurParm[20];

  GLhandleARB m_rcShader;
  GLint m_rcParm[30];

  GLhandleARB m_eeShader;
  GLint m_eeParm[20];

  bool m_exactCoord;
  bool m_dragMode;

  int m_gradType;
  
  int m_spH, m_spW;
  uchar* m_sketchPad;
  uchar* m_screenImageBuffer;
  bool m_sketchPadMode;
  QPolygonF m_poly;

  bool m_infoText;
  int m_savingImages;
  QString m_frameImageFile;
  int m_currFrame, m_totFrames;
  float m_startAngle, m_endAngle;
  Quaternion m_stepRot;


  //-------------
  // vbo for uploading valid boxes
  int m_lmin, m_lmax;
  int m_ntri;
  GLuint m_glVertBuffer;
  GLuint m_glIndexBuffer;
  GLuint m_glVertArray;
  void loadAllBoxesToVBO();
  void drawVBOBox(GLenum);
  void generateBoxes();
  GLint m_mdEle;
  GLsizei *m_mdCount;
  GLint *m_mdIndices;
  QVector<QList<Vec> > m_boxSoup;
  int m_numBoxes;
  //-------------
  
  
  VideoEncoder m_videoEncoder;

  void grabScreenImage();
  void drawScreenImage();

  void drawWireframeBox();

  void updateVoxelsForRaycast();
  void raycasting();

  void drawEnclosingCube(Vec, Vec);
  void drawCurrentPosition();

  bool clip(int, int, int);

  void createShaders();
  void createFBO();

  void drawInfo();

  void generateDrawBoxes();

  void volumeRaycast(float, float, bool);

  Vec pointUnderPixel_RC(QPoint, bool&);
  void getHit(QMouseEvent*);
  
  void setTextureMemorySize();

  void hatch();
  void saveToROI(int);
  void roiOperation(int);
  void connectedComponents(int);
  void connectedComponentsPlus(int, int);
  void distanceTransform(int, int);
  void localThickness(int);
  void removeComponents(int);
  void removeLargestComponents(int);
  void smoothRegion(bool, int, int);
  void regionGrowing(bool);
  void regionDilation(bool);
  void regionErosion();
  void regionDilationAll(int, int tag=-1);
  void regionErosionAll(int, int, int);
  void tagUsingScreenSketch();
  void sortLabels();

  void openRegion(int, int, int);
  void closeRegion(int, int, int);
  
  void commandEditor();
  void processCommand(QString);

  Vec getHit(QPoint, bool&);
  bool getCoordUnderPointer(int&, int&, int&);

  void saveSnapshot(QString);
  void saveImageFrame();

  bool startMovie(QString, int, int, bool);
  bool endMovie();
  void saveMovie();

  void drawVolume();

  void drawBoxes2D();
  void drawBoxes3D();

  void markValidBoxes();
  static void parMarkValidBoxes(QList<QVariant>);
  
};

#endif
