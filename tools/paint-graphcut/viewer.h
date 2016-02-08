#ifndef VIEWER_H
#define VIEWER_H

#include <QGLViewer/qglviewer.h>
#include <QGLViewer/vec.h>
using namespace qglviewer;

#include <QGLFramebufferObject>

#include "curvegroup.h"
#include "fiber.h"
#include "clipplane.h"
#include "boundingbox.h"
#include "mybitarray.h"

class Viewer : public QGLViewer
{
  Q_OBJECT

 public :
  Viewer(QWidget *parent=0);

  void init();
  void draw();

  void setGridSize(int, int, int);
  void setMultiMapCurves(int, QMultiMap<int, Curve*>*);
  void setListMapCurves(int, QList< QMap<int, Curve> >*);
  void setShrinkwrapCurves(int, QList< QMultiMap<int, Curve*> >*);

  void setFibers(QList<Fiber*>*);

  void setVolDataPtr(uchar*);
  void setMaskDataPtr(uchar*);

  void keyPressEvent(QKeyEvent*);
  void mousePressEvent(QMouseEvent*);
  void mouseReleaseEvent(QMouseEvent*);
  void mouseMoveEvent(QMouseEvent*);

  float stillStep();
  float dragStep();

  bool exactCoord();

  uchar* sketchPad() { return m_sketchPad; }

  public slots :
    void GlewInit();  
    void resizeGL(int, int);
    void setPointSize(int p) { m_pointSize = p; update(); }
    void setPointScaling(int p) { m_pointScaling = p; update(); }
    void setVoxelInterval(int);
    void setEdge(int dz) { m_dzScale = dz; update(); }
    void updateVoxels();
    void updateViewerBox(int, int, int, int, int, int);
    void updateCurrSlice(int, int);
    void setVoxelChoice(int);
    void setShowBox(bool);
    void setPaintedTags(QList<int>);
    void setCurveTags(QList<int>);
    void setFiberTags(QList<int>);
    void setDSlice(int);
    void setWSlice(int);
    void setHSlice(int);
    void setShowSlices(bool);
    void updateSlices();
    void uploadMask(int,int,int, int,int,int);
    void setPointRender(bool);
    void setRaycastRender(bool);
    void setRaycastStyle(int);
    void setSkipLayers(int);
    void setStillAndDragStep(float, float);
    void setExactCoord(bool);
    void showSketchPad(bool);
    void saveImage();
    void saveImageSequence();
    void nextFrame();
    
 signals :
    void paint3D(int, int, int, int, int);
    void paint3D(int, int, int, Vec, Vec, int);
    void dilateConnected(int, int, int, Vec, Vec, int);
    void erodeConnected(int, int, int, Vec, Vec, int);
    void paint3DEnd();

    void tagUsingSketchPad(Vec, Vec);

    void mergeTags(Vec, Vec, int, int, bool);
    void mergeTags(Vec, Vec, int, int, int, bool);

    void setVisible(Vec, Vec, int, bool);

    void updateSliceBounds(Vec, Vec);
    void renderNextFrame();

 private :
  bool m_glewInitdone;

  BoundingBox m_boundingBox;

  int m_max3DTexSize;
  float m_memSize;
  int m_skipLayers;

  float m_stillStep, m_dragStep;

  qint64 m_depth, m_width, m_height;
  int m_minDSlice, m_maxDSlice;
  int m_minWSlice, m_maxWSlice;
  int m_minHSlice, m_maxHSlice;

  int m_tag1, m_tag2, m_tag3;
  bool m_mergeTagTF;

  int m_renderMode;

  int m_voxChoice;
  bool m_showBox;

  uchar *m_volPtr;
  uchar *m_maskPtr;
  int m_pointSkip;
  int m_pointSize;
  int m_pointScaling;
  float m_dzScale;

  int m_currSlice, m_currSliceType;

  MyBitArray m_bitmask;
  bool m_paintHit, m_carveHit;
  Vec m_target;


  QMultiMap<int, Curve*> *m_Dcg;
  QMultiMap<int, Curve*> *m_Wcg;
  QMultiMap<int, Curve*> *m_Hcg;
  QList< QMap<int, Curve> > *m_Dmcg;  
  QList< QMap<int, Curve> > *m_Wmcg;  
  QList< QMap<int, Curve> > *m_Hmcg;  

  QList< QMultiMap<int, Curve*> > *m_Dswcg;
  QList< QMultiMap<int, Curve*> > *m_Wswcg;
  QList< QMultiMap<int, Curve*> > *m_Hswcg;

  QList<Fiber*> *m_fibers;

  QList<ushort> m_voxels;
  QList<ushort> m_clipVoxels;

  QList<int> m_paintedTags;
  QList<int> m_curveTags;
  QList<int> m_fiberTags;

  bool m_showSlices;
  int m_dslice, m_wslice, m_hslice;
  QList<ushort> m_dvoxels;
  QList<ushort> m_wvoxels;
  QList<ushort> m_hvoxels;

  GLuint m_slcBuffer;
  GLuint m_rboId;
  GLuint m_slcTex[2];

  GLuint m_dataTex;
  GLuint m_maskTex;
  GLuint m_tagTex;
  GLuint m_lutTex;
  int m_sslevel;
  Vec m_corner, m_vsize;

  ClipPlanes* m_clipPlanes;

  GLhandleARB m_depthShader;
  GLint m_depthParm[20];

  GLhandleARB m_finalPointShader;
  GLint m_fpsParm[20];

  GLhandleARB m_blurShader;
  GLint m_blurParm[20];

  GLhandleARB m_sliceShader;
  GLint m_sliceParm[20];

  GLhandleARB m_rcShader;
  GLint m_rcParm[20];

  GLhandleARB m_eeShader;
  GLint m_eeParm[20];

  bool m_exactCoord;
  bool m_fullRender;
  bool m_dragMode;

  int m_spH, m_spW;
  uchar* m_sketchPad;
  uchar* m_screenImageBuffer;
  bool m_sketchPadMode;
  QPolygonF m_poly;

  bool m_infoText;
  bool m_savingImages;
  QString m_frameImageFile;
  int m_currFrame, m_totFrames;
  float m_startAngle, m_endAngle;
  Quaternion m_stepRot;

  void grabScreenImage();
  void drawScreenImage();

  void drawBox();

  void drawMMDCurve();
  void drawMMWCurve();
  void drawMMHCurve();

  void drawLMDCurve();
  void drawLMWCurve();
  void drawLMHCurve();

  void drawSWDCurve();
  void drawSWWCurve();
  void drawSWHCurve();

  void drawCurve(int, Curve*, int);

  void drawFibers();

  void drawVolMask();
  void drawVol();

  void drawPointsWithoutShader();

  void updateVoxelsWithTF();
  void updateClipVoxels();

  void updateVoxelsForRaycast();
  void raycasting();

  void drawEnclosingCube(Vec, Vec);
  void drawCurrentSlice(Vec, Vec);

  bool clip(int, int, int);
  void drawClip();

  void createRaycastShader();
  void createShaders();
  void createFBO();

  void drawInfo();

  void carve(int, int, int, bool);

  void drawSlices();
  void drawClipSlices();
  int drawPoly(Vec, Vec, Vec*);

  void drawBox(GLenum);

  void volumeRaycast(float, float, bool);

  Vec pointUnderPixel(QPoint, bool&);
  Vec pointUnderPixel_RC(QPoint, bool&);
  void getHit(QMouseEvent*);
  
  void setTextureMemorySize();

  void drawFace(int, Vec*, Vec*);
  void drawClipFaces(Vec*, Vec*);

  void pointRendering();

  void regionGrowing();
  void regionDilation();
  void regionErosion();
  void tagUsingScreenSketch();

  void commandEditor();
  void processCommand(QString);

  Vec getHit(QPoint, bool&);
  bool getCoordUnderPointer(int&, int&, int&);

  void saveSnapshot(QString);
  void saveImageFrame();
};

#endif
