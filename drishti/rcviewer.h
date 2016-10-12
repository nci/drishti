#ifndef RCVIEWER_H
#define RCVIEWER_H


#include <QGLViewer/qglviewer.h>
#include <QGLViewer/vec.h>
using namespace qglviewer;

#include <QGLFramebufferObject>

#include "mybitarray.h"
#include "volumefilemanager.h"
#include "boundingbox.h"
#include "cropobject.h"

class RcViewer : public QObject
{
  Q_OBJECT

 public :
  RcViewer();
  ~RcViewer();

  void setViewer(QGLViewer *v) { m_viewer = v; }

  void init();

  void setTagTex(GLuint tt) { m_tagTex = tt; }
  void setMixTag(bool mt) { m_mixTag = mt; }

  void draw();
  void fastDraw();

  void setGridSize(int, int, int);
  void setVolDataPtr(VolumeFileManager*);
  void updateSubvolume(Vec, Vec);

  void resizeGL(int, int);

  void setLut(uchar*);

  void activateBounds(bool);

  void setXformMatrix(double*);

  int aoLevel() { return m_aoLevel; }
  bool exactCoord() { return m_exactCoord; }
  int skipLayers() { return m_skipLayers; }
  int skipVoxels() { return m_skipVoxels; }
  int amb() { return m_amb; }
  int diff() { return m_diff; }
  int spec() { return m_spec; }
  int edge() { return m_edge; }
  int shadow() { return m_shadow; }
  Vec shadowColor() { return m_shadowColor; }
  Vec edgeColor() { return m_edgeColor; }
  float stillStep() { return m_stillStep; }
  float dragStep() { return m_dragStep; }
  int smoothDepth() { return m_smoothDepth; }
  float edgeThickness() { return m_edgeThickness; }
  int maxRayLen() { return m_maxRayLen; }

  bool getHit(const QMouseEvent*);

  void resetNextLot() { m_currSlab = 0; }
  bool doNextLot();

  void loadLookupTable();

  public slots :
    void updateVoxelsForRaycast();
    void setRaycastStyle(int);
    void setSkipLayers(int l) { m_skipLayers = l; m_viewer->update(); }
    void setSkipVoxels(int l) { m_skipVoxels = l; m_viewer->update(); }
    void setShadowColor();
    void setEdgeColor();
    void setSpec(int a) { m_spec = a; m_viewer->update(); }
    void setEdge(int e) { m_edge = e; m_viewer->update(); }
    void setShadow(int e) { m_shadow = e; m_viewer->update(); }
    void setSmoothDepth(int e) { m_smoothDepth = e; m_viewer->update(); }
    void setShadowOffsetX(int x) { m_shdX = x; m_viewer->update(); }
    void setShadowOffsetY(int y) { m_shdY = y; m_viewer->update(); }
    void setExactCoord(bool);
    void setStillAndDragStep(float, float);
    void setEdgeThickness(int e) { m_edgeThickness = 0.1*e; m_viewer->update(); }
    void setMaxRayLen(int r) { m_maxRayLen = r;  m_viewer->update();}
    void setAmbient(int r) { m_amb = r;  m_viewer->update();}
    void setDiffuse(int r) { m_diff = r;  m_viewer->update();}
    void setSpecular(int r) { m_spec = r;  m_viewer->update();}
    void setAOLevel(int ao) { m_aoLevel = ao;  m_viewer->update();} 
    
 private :

  QGLViewer *m_viewer;

  VolumeFileManager *m_vfm;
  uchar *m_volPtr;
  uchar *m_lut;
  int m_bytesPerVoxel;

  double m_b0xform[16];
  double m_b0xformInv[16];

  qint64 m_depth, m_width, m_height;
  int m_minDSlice, m_maxDSlice;
  int m_minWSlice, m_maxWSlice;
  int m_minHSlice, m_maxHSlice;
  Vec m_dataMin, m_dataMax;

  int m_dbox, m_wbox, m_hbox, m_boxSize;
  QList<int> m_boxMinMax;
  MyBitArray m_filledBoxes;

  int m_amb, m_diff, m_spec;
  int m_shadow;
  float m_edge;
  Vec m_shadowColor, m_edgeColor;
  int m_shdX, m_shdY;
  int m_smoothDepth;
  float m_edgeThickness;
  int m_aoLevel;

  int m_max3DTexSize;
  int m_skipLayers, m_skipVoxels;
  float m_stillStep, m_dragStep;

  GLuint m_slcBuffer;
  GLuint m_rboId;
  GLuint m_slcTex[5];

  bool m_mixTag;
  GLuint m_tagTex;
  
  GLuint m_dataTex;
  GLuint m_lutTex;
  int m_sslevel;
  Vec m_corner, m_vsize;

  GLuint m_filledTex;
  uchar *m_ftBoxes;

  GLhandleARB m_blurShader;
  GLint m_blurParm[20];

  GLhandleARB m_enexShader;
  GLint m_enexParm[20];

  GLhandleARB m_fhShader;
  GLint m_fhParm[20];

  GLhandleARB m_ircShader;
  GLint m_ircParm[50];

  GLhandleARB m_rcShader;
  GLint m_rcParm[50];

  GLhandleARB m_eeShader;
  GLint m_eeParm[20];

  bool m_exactCoord;
  bool m_fullRender;
  bool m_dragMode;
  int m_renderMode;
  int m_maxRayLen, m_maxSteps;
  
  BoundingBox m_boundingBox;
  
  QList<CropObject> m_crops;

  int m_nslabs, m_currSlab;
  int m_currEntryPoints;
  int m_nextEntryPoints;
  int m_currAccColor;
  int m_nextAccColor;

  void generateBoxMinMax();
  void updateFilledBoxes();

  void createFirstHitShader();
  void createIsoRaycastShader();
  void createRaycastShader();
  void createShaders();
  void createFBO();

  void raycasting();
  void surfaceRaycast(float, float, bool);
  void volumeRaycast(float, float);

  void drawBox(GLenum);
  void drawFace(int, Vec*, Vec*);
  void drawClipFaces(Vec*, Vec*);

  void drawInfo();

  void checkCrops();

  void pre_vray();
  void vray();

  void drawGeometry(bool);
};




#endif
