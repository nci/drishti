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
    MoveXY,
    MoveXZ,
    MoveYZ,
    MoveAll
  };

  bool mousePressed();
  void setGrabMode(bool);
  
  void setMouseGrab(bool);
  Vec checkForMouseHover(int, int, const Camera* const);  
  void checkIfGrabsMouse(int, int, const Camera* const);

  int moveAxis();
  void setMoveAxis(int);

  void mousePressEvent(QMouseEvent* const, Camera* const);
  void mouseMoveEvent(QMouseEvent* const, Camera* const);
  void mouseReleaseEvent(QMouseEvent* const, Camera* const);

  int labelGrabbed() { return (m_labelSelected); }
  
 signals :
  void updateParam();
  void meshGrabbed();
  void posChanged();
  
 private :
  int m_labelSelected;
  float m_screenWidth, m_screenHeight;
  
  int m_moveAxis;
  bool m_pressed;
  QPoint m_prevPos;

  bool m_allowMove;

  bool m_rotationMode;
  bool m_grabMode;

  bool m_flipRotAxis;
  
  float projectOnBall(float, float);

  bool checkLabel(int, int, const Camera* const);
  void moveLabel(QPoint);
};

#endif
