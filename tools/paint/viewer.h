#ifndef VIEWER_H
#define VIEWER_H

#include <QGLViewer/qglviewer.h>
#include <QGLViewer/vec.h>
using namespace qglviewer;

#include <QGLFramebufferObject>

#include "ui_viewermenu.h"

#include "curvegroup.h"
#include "fiber.h"
#include "clipplane.h"
#include "boundingbox.h"
#include "mybitarray.h"

#ifdef USE_GLMEDIA
#include "glmedia.h"
#endif // USE_GLMEDIA

class Viewer : public QGLViewer
{
  Q_OBJECT

 public :
  Viewer(QWidget *parent=0);
  ~Viewer();

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
    void setVoxelInterval(int);
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
    void uploadMask(int,int,int, int,int,int);
    void setRenderMode(bool);
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

    void setUseMask(bool);
    void setBoxSize(int);

    void stopDrawing();
    void startDrawing();

    
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

    void tagUsingSketchPad(Vec, Vec);

    void mergeTags(Vec, Vec, int, int, bool);
    void stepTag(Vec, Vec, int, int);
  
    void updateSliceBounds(Vec, Vec);
    void renderNextFrame();

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
    void hatchConnectedRegion(int, int, int,
			      Vec, Vec,
			      int, int,
			      int, int);
    void getVolume(Vec, Vec, int);

    void modifyOriginalVolume(Vec, Vec, int);
 private :
  Ui::ViewerMenu *m_UI;

  bool m_glewInitdone;
  bool m_draw;
  bool m_useMask;

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

  bool m_renderMode;

  int m_voxChoice;
  bool m_showBox;

  uchar *m_volPtr;
  uchar *m_maskPtr;
  ushort *m_volPtrUS;

  int m_pointSkip;
  int m_pointSize;
  int m_pointScaling;

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
  
  
#ifdef USE_GLMEDIA
  glmedia_movie_writer_t m_movieWriter;
#endif // USE_GLMEDIA

  void grabScreenImage();
  void drawScreenImage();

  void drawWireframeBox();

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

  void drawPointsWithoutShader();

  void updateClipVoxels();

  void updateVoxelsForRaycast();
  void raycasting();

  void drawEnclosingCube(Vec, Vec);
  void drawCurrentSlice();

  bool clip(int, int, int);
  void drawClip();

  void createRaycastShader();
  void createShaders();
  void createFBO();

  void drawInfo();

  void generateDrawBoxes();

  void volumeRaycast(float, float, bool);

  Vec pointUnderPixel_RC(QPoint, bool&);
  void getHit(QMouseEvent*);
  
  void setTextureMemorySize();

  void hatch();
  void regionGrowing(bool);
  void regionDilation(bool);
  void regionErosion();
  void tagUsingScreenSketch();

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
};

#endif
