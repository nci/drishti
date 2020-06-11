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
#include "volume.h"


class RcViewer : public QObject
{
  Q_OBJECT

 public :
  RcViewer();
  ~RcViewer();

  void setViewer(QGLViewer *v) { m_viewer = v; }

  void init();

  void setVolume(Volume *vol) { m_Volume = vol; }
  
  void setTagTex(GLuint tt) { m_tagTex = tt; }
  void setMixTag(bool mt) { m_mixTag = mt; }

  void dummyDraw();
  void draw();
  void fastDraw();

  void setGridSize(int, int, int);
  void setVolDataPtr(VolumeFileManager*);
  void updateSubvolume(Vec, Vec);

  int* histogram1D() { return m_subvolume1dHistogram; }
  int* histogram2D() { return m_subvolume2dHistogram; }

 void resizeGL(int, int);

  void setLut(uchar*);

  void activateBounds(bool);

  void setBrickInfo(QList<BrickInformation>);
  
  int skipLayers() { return m_skipLayers; }
  int skipVoxels() { return m_skipVoxels; }
  int edge() { return m_edge; }
  int shadow() { return m_shadow; }
  int maxRayLen() { return m_maxRayLen; }
  int minGrad() { return m_minGrad; }
  int maxGrad() { return m_maxGrad; }
  
  bool getHit(const QMouseEvent*);

  void loadLookupTable();

  public slots :
    void boundingBoxChanged();
    void updateVoxelsForRaycast(GLuint*);
    void setSkipLayers(int l) { m_skipLayers = l; m_viewer->update(); }
    void setSkipVoxels(int l) { m_skipVoxels = l; m_viewer->update(); }
    void setEdge(int e) { m_edge = e; m_viewer->update(); }
    void setShadow(int e) { m_shadow = e; m_viewer->update(); }
    void setMaxRayLen(int r) { m_maxRayLen = r;  m_viewer->update();}
    void setMinGrad(int e) { m_minGrad = e; m_viewer->update(); }
    void setMaxGrad(int e) { m_maxGrad = e; m_viewer->update(); }

    void setLighting(QVector4D l)
    {
      m_ambient = l.x();
      m_diffuse = l.y();
      m_specular = l.z();
      m_roughness = l.w();
      //m_viewer->update();
    }
      
    void setShapeEnhancements(float edges, float blur)
    {
      m_shadow = blur;
      m_edge = edges;
      //m_viewer->update();
    }

 private :

  QGLViewer *m_viewer;

  Volume *m_Volume;

  VolumeFileManager *m_vfm;
  uchar *m_volPtr;
  uchar *m_lut;
  int m_bytesPerVoxel;

  qint64 m_depth, m_width, m_height;
  int m_minDSlice, m_maxDSlice;
  int m_minWSlice, m_maxWSlice;
  int m_minHSlice, m_maxHSlice;
  Vec m_dataMin, m_dataMax;

  int m_cminD, m_cmaxD;
  int m_cminW, m_cmaxW;
  int m_cminH, m_cmaxH;

  int m_dbox, m_wbox, m_hbox, m_boxSize;
  QList<int> m_boxMinMax;
  MyBitArray m_filledBoxes;
  //QList<int*> m_boxHistogram;
  
  float m_shadow;
  float m_edge;
  float m_minGrad, m_maxGrad;

  float m_roughness;
  float m_specular;
  float m_diffuse;
  float m_ambient;
  
  int m_max2DTexSize;
  int m_max3DTexSize;
  int m_skipLayers, m_skipVoxels;

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

//  GLhandleARB m_blurShader;
//  GLint m_blurParm[20];

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
  void raycast(Vec, float, bool);

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
  int m_numBoxes;
  //-------------

  QList<BrickInformation> m_brickInfo;
  
  QMatrix4x4 m_b0xform;
  float *m_flhist1D, *m_flhist2D;
  int *m_subvolume1dHistogram, *m_subvolume2dHistogram;


  void generateDrawBoxes();
  void loadAllBoxesToVBO();
  void drawVBOBox(GLenum);
  void generateBoxes();

};




#endif
