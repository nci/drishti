#ifndef VIEWER_H
#define VIEWER_H

#include <QGLViewer/qglviewer.h>
#include <QGLViewer/vec.h>
using namespace qglviewer;

#include <QGLFramebufferObject>

#include "curvegroup.h"
#include "fiber.h"
#include "clipplane.h"

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
    void saveImage() { saveSnapshot(false); };
    void setPaintedTags(QList<int>);
    void setCurveTags(QList<int>);
    void setFiberTags(QList<int>);
    void setDSlice(int);
    void setWSlice(int);
    void setHSlice(int);
    void setShowSlices(bool);
    void updateSlices();

    void getHit(QMouseEvent*);
    Vec pointUnderPixel(QPoint, bool&);

 signals :
    void paint3D(int, int, int, int);
    void paint3DEnd();

 private :
  bool m_glewInitdone;

  int m_depth, m_width, m_height;
  int m_minDSlice, m_maxDSlice;
  int m_minWSlice, m_maxWSlice;
  int m_minHSlice, m_maxHSlice;

  int m_voxChoice;
  bool m_showBox;

  uchar *m_volPtr;
  uchar *m_maskPtr;
  int m_pointSkip;
  int m_pointSize;
  int m_pointScaling;
  float m_dzScale;

  int m_currSlice, m_currSliceType;

  bool m_findHit;

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

  ClipPlanes* m_clipPlanes;

  GLhandleARB m_depthShader;
  GLint m_depthParm[50];

  GLhandleARB m_finalPointShader;
  GLint m_fpsParm[50];

  GLhandleARB m_blurShader;
  GLint m_blurParm[5];

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
  void drawAllPoints();

  void updateVoxelsWithTF();
  void updateClipVoxels();

  void drawEnclosingCube(Vec, Vec);
  void drawCurrentSlice(Vec, Vec);

  void drawSlices();

  bool clip(int, int, int);
  void drawClip();

  void createShaders();
  void createFBO();
};

#endif
