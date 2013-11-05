#ifndef TRISETGRABBER_H
#define TRISETGRABBER_H

#include <QtGui>
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

  int pointPressed();

  void mousePosition(int&, int&);

  void checkIfGrabsMouse(int, int, const Camera* const);

  int moveAxis();
  void setMoveAxis(int);

  void mousePressEvent(QMouseEvent* const, Camera* const);
  void mouseMoveEvent(QMouseEvent* const, Camera* const);
  void mouseReleaseEvent(QMouseEvent* const, Camera* const);

 private :
  int m_lastX, m_lastY;
  int m_pointPressed;
  int m_moveAxis;
  bool m_pressed;
  QPoint m_prevPos;
};

#endif
