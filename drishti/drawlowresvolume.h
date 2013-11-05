#ifndef DRAWLOWRESVOLUME_H
#define DRAWLOWRESVOLUME_H

#include <GL/glew.h>

#include <QGLViewer/qglviewer.h>
using namespace qglviewer;

#include "boundingbox.h"
class Viewer;
class Volume;

class DrawLowresVolume : public QObject
{
  Q_OBJECT

 public :
  DrawLowresVolume(Viewer*, Volume*);
  ~DrawLowresVolume();

  bool raised();
  void raise();
  void lower();
  void activateBounds();
  void init();
  void load3dTexture();
  void loadVolume();
  QImage histogramImage1D();
  QImage histogramImage2D();
  int* histogram2D();

  Vec volumeSize();
  Vec volumeMin();
  Vec volumeMax();

  void subvolumeBounds(Vec&, Vec&);

  bool keyPressEvent(QKeyEvent*);

  void load(const char*);
  void save(const char*);

  void setCurrentVolume(int vnum=0);

 public slots :
  void setSubvolumeBounds(Vec, Vec);

  void createShaders();

  void updateScaling();

  void draw(float, int, int);
  
  void generateHistogramImage();

  void switchInterpolation();


 private :
  bool showing;

  int m_currentVolume;

  Viewer *m_Viewer;
  Volume *m_Volume;

  unsigned char *m_histImageData1D;
  unsigned char *m_histImageData2D;
  QImage m_histogramImage1D;
  QImage m_histogramImage2D;

  Vec m_dataMin, m_dataMax;
  Vec m_virtualTextureSize;
  Vec m_virtualTextureMin;
  Vec m_virtualTextureMax;

  GLuint m_dataTex;
  GLhandleARB m_fragObj, m_progObj;
  GLint m_parm[20];

  BoundingBox m_boundingBox;

  int drawpoly(Vec, Vec, Vec*, Vec, Vec);
  void drawSlices(Vec, Vec, Vec, int, float);

  void enableTextureUnits();
  void disableTextureUnits();
};

#endif
