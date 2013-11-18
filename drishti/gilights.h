#ifndef GILIGHTS_H
#define GILIGHTS_H

#include <QtGui>
#include "gilightgrabber.h"

class GiLights : public QObject
{
 Q_OBJECT

 public :
  GiLights();
  ~GiLights();

  bool isInMouseGrabberPool(int);
  void addInMouseGrabberPool();
  void addInMouseGrabberPool(int);
  void removeFromMouseGrabberPool();
  void removeFromMouseGrabberPool(int);

  void setShow(int, bool);
  void show();
  bool show(int);
  void hide();

  bool grabsMouse();

  void clear();

  QList<GiLightObject> giLights();
  void setGiLights(QList<GiLightObject>);

  QList<GiLightGrabber*> giLightsPtr();

  bool setGiLightObjectInfo(QList<GiLightObjectInfo>);
  QList<GiLightObjectInfo> giLightObjectInfo();

  int count();
  void addGiDirectionLight(QList<Vec>);
  void addGiPointLight(QList<Vec>);
  void addGiLight(GiLightObject);
  void addGiLight(QString);

  void draw(QGLViewer*, bool);
  void postdraw(QGLViewer*);

  bool keyPressEvent(QKeyEvent*);

  void updateScaling();

  GiLightGrabber* checkIfGrabsMouse(int, int, Camera*);
  void mousePressEvent(QMouseEvent*, Camera*);
  void mouseMoveEvent(QMouseEvent*, Camera*);
  void mouseReleaseEvent(QMouseEvent*, Camera*);

 signals :
  void updateLightBuffers();
  void showMessage(QString, bool);
  void updateGL();

 private slots :
  void selectForEditing(int, int);
  void deselectForEditing();

 private :
  QList<GiLightGrabber*> m_giLights;
  bool m_sameForAll;

  void makeGiLightConnections();
  void processCommand(int, QString);

  bool openPropertyEditor(int);
};


#endif
