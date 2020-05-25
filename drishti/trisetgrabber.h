#ifndef TRISETGRABBER_H
#define TRISETGRABBER_H


#include <QGLViewer/mouseGrabber.h>
#include <QGLViewer/manipulatedCameraFrame.h>
using namespace qglviewer;

#include "trisetobject.h"

class TrisetGrabber : public QObject, public MouseGrabber, public TrisetObject
{
  Q_OBJECT

 public :
  TrisetGrabber();
  ~TrisetGrabber();

  enum MoveAxis
  {
    MoveX,
    MoveY,
    MoveZ,
    MoveAll
  };

  bool mousePressed();

  void setMouseGrab(bool);
  Vec checkForMouseHover(int, int, const Camera* const);  
  void checkIfGrabsMouse(int, int, const Camera* const);

  int moveAxis();
  void setMoveAxis(int);

  void mousePressEvent(QMouseEvent* const, Camera* const);
  void mouseMoveEvent(QMouseEvent* const, Camera* const);
  void mouseReleaseEvent(QMouseEvent* const, Camera* const);

 private :
  int m_moveAxis;
  bool m_pressed;
  QPoint m_prevPos;

  float projectOnBall(float, float);
};

#endif
