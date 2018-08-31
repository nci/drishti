#ifndef RCVIEWER_H
#define RCVIEWER_H

#include "brickinformation.h"

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

  void setBrickInfo(QList<BrickInformation>);
  
  int skipLayers() { return m_skipLayers; }
  int skipVoxels() { return m_skipVoxels; }
  int edge() { return m_edge; }
  int shadow() { return m_shadow; }
  float stillStep() { return m_stillStep; }
  float dragStep() { return m_dragStep; }
  int maxRayLen() { return m_maxRayLen; }
  int minGrad() { return m_minGrad; }
  int maxGrad() { return m_maxGrad; }
  
  bool getHit(const QMouseEvent*);

  void loadLookupTable();

  public slots :
    void boundingBoxChanged();
    void updateVoxelsForRaycast();
    void setStillAndDragStep(float, float);
    void setSkipLayers(int l) { m_skipLayers = l; m_viewer->update(); }
    void setSkipVoxels(int l) { m_skipVoxels = l; m_viewer->update(); }
    void setEdge(int e) { m_edge = e; m_viewer->update(); }
    void setShadow(int e) { m_shadow = e; m_viewer->update(); }
    void setMaxRayLen(int r) { m_maxRayLen = r;  m_viewer->update();}
    void setMinGrad(int e) { m_minGrad = e; m_viewer->update(); }
    void setMaxGrad(int e) { m_maxGrad = e; m_viewer->update(); }
    
 private :

  QGLViewer *m_viewer;

  VolumeFileManager *m_vfm;
  uchar *m_volPtr;
  uchar *m_lut;
  int m_bytesPerVoxel;

  qint64 m_depth, m_width, m_height;
  int m_minDSlice, m_maxDSlice;
  int m_minWSlice, m_maxWSlice;
  int m_minHSlice, m_maxHSlice;
  Vec m_dataMin, m_dataMax;

  int m_dbox, m_wbox, m_hbox, m_boxSize;
  QList<int> m_boxMinMax;
  MyBitArray m_filledBoxes;

  float m_shadow;
  float m_edge;
  float m_minGrad, m_maxGrad;
  
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

  GLhandleARB m_ircShader;
  GLint m_ircParm[50];

  GLhandleARB m_eeShader;
  GLint m_eeParm[20];

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

  void createIsoRaycastShader();
  void createShaders();
  void createFBO();

  void raycasting();
  void raycast(Vec, float, float, bool);

  void drawInfo();

  void checkCrops();

  void drawGeometry();


  //-------------
  // vbo for uploading valid boxes
  int m_lmin, m_lmax;
  QList<int> m_limits;
  int m_ntri;
  GLuint m_glVertBuffer;
  GLuint m_glIndexBuffer;
  GLuint m_glVertArray;
  int m_mdEle;
  GLsizei *m_mdCount;
  GLint *m_mdIndices;
  QList<QList<Vec> > m_boxSoup;
  //-------------

  QList<BrickInformation> m_brickInfo;
  
  void identifyBoxes();
  void loadAllBoxesToVBO();
  void drawVBOBox(GLenum);
  void generateBoxes();

};




#endif
