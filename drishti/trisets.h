#ifndef TRISETiS_H
#define TRISETS_H


#include <GL/glew.h>

#include "trisetgrabber.h"
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

  
  void predraw(QGLViewer*, double*);
  void draw(QGLViewer*,
	    Vec,
	    float, float, Vec,
	    bool, Vec,
	    QList<Vec>, QList<Vec>);
  void postdraw(QGLViewer*);

  bool keyPressEvent(QKeyEvent*);

  QList<TrisetInformation> get();
  void set(QList<TrisetInformation>);

  void makeReadyForPainting();
  void paint(Vec, float, Vec, int, float);

  void resize(int, int);

  void setShapeEnhancements(float, float);

  void checkMouseHover(QGLViewer*);

  void setLightDirection(Vec);
 
 signals :
  void updateGL();
  void resetBoundingBox();
  
 private :
  HitPoints *m_hitpoints;
  
  QList<TrisetGrabber*> m_trisets;

  void processCommand(int, QString);

  GLfloat m_mvpShadow[16];
  Vec m_lightDir;
  
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


  QStringList m_solidTexName;
  QList<uchar*> m_solidTexData;
  GLuint* m_solidTex;
  void loadSolidTextures();


  void renderFromCamera(Camera*, int);
  void renderFromShadowCamera(Camera*, int);
  void renderShadows(GLint, int, int);
  void renderOutline(GLint, QGLViewer*, int, float);
  void renderTransparent(GLint, QGLViewer*, int, float);
  void bindOITTextures();
  void drawOITTextures(int, int);

  void askGradientChoice();
  
};


#endif
