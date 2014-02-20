#ifndef NETWORKGRABBER_H
#define NETWORKGRABBER_H

#include <QGLViewer/mouseGrabber.h>
#include <QGLViewer/manipulatedCameraFrame.h>
using namespace qglviewer;

#include "networkobject.h"

class NetworkGrabber : public QObject, public MouseGrabber, public NetworkObject
{
  Q_OBJECT

 public :
  NetworkGrabber();
  ~NetworkGrabber();

  int pointPressed();

  void mousePosition(int&, int&);

  void checkIfGrabsMouse(int, int, const Camera* const);

  void mousePressEvent(QMouseEvent* const, Camera* const);
  void mouseMoveEvent(QMouseEvent* const, Camera* const);
  void mouseReleaseEvent(QMouseEvent* const, Camera* const);

 private :
  int m_lastX, m_lastY;
  int m_pointPressed;
  bool m_pressed;
};

#endif
