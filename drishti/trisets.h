#ifndef TRISETS_H
#define TRISETS_H


#include <GL/glew.h>
#include "trisetgrabber.h"
#include "cropobject.h"

class Trisets : public QObject
{
  Q_OBJECT

 public :
  Trisets();
  ~Trisets();

  int count() { return m_trisets.count(); }

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
  void addPLY(QString);

  void predraw(QGLViewer*,
	       double*,
	       Vec,
	       bool, int, int);
  void draw(QGLViewer*, Vec,
	    float, float, Vec,
	    bool, bool, Vec,
	    QList<Vec>, QList<Vec>,
	    bool);
  void postdraw(QGLViewer*);

  bool keyPressEvent(QKeyEvent*);

  void createDefaultShader(QList<CropObject>);
  void createHighQualityShader(bool, float, QList<CropObject>);
  void createShadowShader(Vec, QList<CropObject>);

  QList<TrisetInformation> get();
  void set(QList<TrisetInformation>);

  void load(const char*);
  void save(const char*);

  void makeReadyForPainting(QGLViewer*);
  void releaseFromPainting();
  void paint(QGLViewer*, QBitArray, float*, Vec, float);

 signals :
  void updateGL();

 private :
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
};


#endif
