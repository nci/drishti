#ifndef TRISETiS_H
#define TRISETS_H


#include <GL/glew.h>

#include "trisetgrabber.h"
#include "cropobject.h"
#include "hitpoints.h"

class Trisets : public QObject
{
  Q_OBJECT

 public :
  Trisets();
  ~Trisets();

  int count() { return m_trisets.count(); }

  void setHitPoints(HitPoints* h) { m_hitpoints = h; }
  
  bool show(int);
  void setShow(int, bool);
  void show();
  void hide();

  bool clip(int);
  void setClip(int, bool);

  QString filename(int);
  
  void setLighting(QVector4D);
  
  void allEnclosingBox(Vec&, Vec&);
  void allGridSize(int&, int&, int&);

  bool isInMouseGrabberPool(int);
  void addInMouseGrabberPool();
  void addInMouseGrabberPool(int);
  void removeFromMouseGrabberPool();
  void removeFromMouseGrabberPool(int);

  bool grabsMouse();

  void clear();

  void addTriset(QString);
  void addMesh(QString);

  void setClipDistance0(float, float, float, float);
  void setClipDistance1(float, float, float, float);

  
  void predraw(QGLViewer*, double*);
  void draw(QGLViewer*,
	    Vec,
	    float, float, Vec,
	    bool, Vec,
	    QList<Vec>, QList<Vec>);
  void postdraw(QGLViewer*);

  bool keyPressEvent(QKeyEvent*);

  void createDefaultShader(QList<CropObject>);
  void createHighQualityShader(bool, float, QList<CropObject>);
  void createShadowShader(Vec, QList<CropObject>);

  QList<TrisetInformation> get();
  void set(QList<TrisetInformation>);

  void makeReadyForPainting(QGLViewer*);
  void releaseFromPainting();
  void paint(QGLViewer*, QBitArray, float*, Vec, float);

  void resize(int, int);

  void setShapeEnhancements(float, float);

  void checkMouseHover(QGLViewer*);

 signals :
  void updateGL();

 private :
  HitPoints *m_hitpoints;
  
  QList<TrisetGrabber*> m_trisets;

  void processCommand(int, QString);

  GLhandleARB m_geoShadowShader;
  GLhandleARB m_geoHighQualityShader;
  GLhandleARB m_geoDefaultShader;

  GLint m_highqualityParm[15];
  GLint m_defaultParm[15];
  GLint m_shadowParm[15];

  float *m_cpos;
  float *m_cnormal;

  GLuint m_vertexScreenBuffer;
  float m_scrGeo[8];

  GLuint m_depthBuffer;
  GLuint m_rbo;
  GLuint m_depthTex[4];

  float m_blur, m_edges;
    
  void createFBO(int, int);

  void render(Camera*, int);

  bool handleDialog(int);
  bool duplicate(int);
};


#endif
