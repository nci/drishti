#ifndef CLIPGRABBER_H
#define CLIPGRABBER_H

#include "clipobject.h"

#include <QGLViewer/mouseGrabber.h>
#include <QGLViewer/manipulatedCameraFrame.h>
using namespace qglviewer;

class ClipGrabber : public QObject, public MouseGrabber, public ClipObject
{
 Q_OBJECT

 public :
  ClipGrabber();
  ~ClipGrabber();

  int pointPressed();

  void mousePosition(int&, int&);
  void checkIfGrabsMouse(int, int, const Camera* const);


  void mousePressEvent(QMouseEvent* const, Camera* const);
  void mouseMoveEvent(QMouseEvent* const, Camera* const);
  void mouseReleaseEvent(QMouseEvent* const, Camera* const);

  void wheelEvent(QWheelEvent* const, Camera* const);

 signals :
  void selectForEditing();
  void deselectForEditing();

 private :
  int m_lastX, m_lastY;
  int m_pointPressed;
  bool m_pressed;
  QPoint m_prevPos;
  

  
  bool xActive(const Camera* const, Vec, Vec, bool&);
  bool yActive(const Camera* const, Vec, Vec, bool&);
  bool zActive(const Camera* const, Vec, Vec);
};

#endif
