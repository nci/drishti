#ifndef PATHGRABBER_H
#define PATHGRABBER_H


#include <QGLViewer/mouseGrabber.h>
#include <QGLViewer/manipulatedCameraFrame.h>
using namespace qglviewer;

#include "pathobject.h"

class PathGrabber : public QObject, public MouseGrabber, public PathObject
{
 Q_OBJECT

 public :
  PathGrabber();
  ~PathGrabber();

  enum MoveAxis
  {
    MoveX,
    MoveY,
    MoveZ,
    MoveAll
  };

  int moveAxis();
  void setMoveAxis(int);

  int pointPressed();

  void mousePosition(int&, int&);
  void checkIfGrabsMouse(int, int, const Camera* const);

  void mousePressEvent(QMouseEvent* const, Camera* const);
  void mouseMoveEvent(QMouseEvent* const, Camera* const);
  void mouseReleaseEvent(QMouseEvent* const, Camera* const);

 signals :
  void selectForEditing(int, int);
  void deselectForEditing();

 private :
  int m_lastX, m_lastY;
  int m_pointPressed;
  int m_moveAxis;
  bool m_pressed;
  QPoint m_prevPos;
  bool m_moved;
};

#endif
