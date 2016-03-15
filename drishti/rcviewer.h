#ifndef RCVIEWER_H
#define RCVIEWER_H


#include <QGLViewer/qglviewer.h>
#include <QGLViewer/vec.h>
using namespace qglviewer;

#include <QGLFramebufferObject>

#include "mybitarray.h"
#include "volumefilemanager.h"

class RcViewer : public QObject
{
  Q_OBJECT

 public :
  RcViewer();
  ~RcViewer();

  void setViewer(QGLViewer *v) { m_viewer = v; }

  void init();

  void draw();
  void fastDraw();

  void setGridSize(int, int, int);
  void setVolDataPtr(VolumeFileManager*);
  void updateSubvolume(Vec, Vec);

  void resizeGL(int, int);

  void setLut(uchar*);

  void updateVoxelsForRaycast();


 private :

  QGLViewer *m_viewer;

  uchar *m_volPtr;
  uchar *m_lut;

  qint64 m_depth, m_width, m_height;
  int m_minDSlice, m_maxDSlice;
  int m_minWSlice, m_maxWSlice;
  int m_minHSlice, m_maxHSlice;
  Vec m_dataMin, m_dataMax;

  int m_dbox, m_wbox, m_hbox, m_boxSize;
  QList<int> m_boxMinMax;
  MyBitArray m_filledBoxes;

  float m_amb, m_diff, m_spec;
  int m_shadow;
  float m_edge;
  Vec m_shadowColor, m_edgeColor;
  int m_shdX, m_shdY;

  int m_max3DTexSize;
  int m_skipLayers;

  GLuint m_slcBuffer;
  GLuint m_rboId;
  GLuint m_slcTex[2];

  GLuint m_eBuffer;
  GLuint m_ebId;
  GLuint m_ebTex[3];

  GLuint m_dataTex;
  GLuint m_lutTex;
  int m_sslevel;
  Vec m_corner, m_vsize;

  GLhandleARB m_blurShader;
  GLint m_blurParm[20];

  GLhandleARB m_rcShader;
  GLint m_rcParm[20];

  GLhandleARB m_eeShader;
  GLint m_eeParm[20];

  bool m_exactCoord;
  bool m_fullRender;
  bool m_dragMode;
  
  float m_stillstep;

  void generateBoxMinMax();
  void updateFilledBoxes();

  void createRaycastShader();
  void createShaders();
  void createFBO();

  void raycasting();
  void volumeRaycast(float, float, bool);

  void drawBox(GLenum);
  void drawFace(int, Vec*, Vec*);
  void drawClipFaces(Vec*, Vec*);
};




#endif
