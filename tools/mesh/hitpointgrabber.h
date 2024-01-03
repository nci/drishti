#ifndef HITPOINTGRABBER_H
#define HITPOINTGRABBER_H

#include <QGLViewer/manipulatedCameraFrame.h>
#include <QGLViewer/mouseGrabber.h>
#include <QGLViewer/qglviewer.h>
using namespace qglviewer;

#include "commonqtclasses.h"

class HitPointGrabber : public QObject, public MouseGrabber
{
 Q_OBJECT

 public :
  HitPointGrabber(Vec);
  ~HitPointGrabber();

  enum MoveAxis
  {
    MoveX,
    MoveY,
    MoveZ,
    MoveAll
  };


  bool active();
  void resetActive();

  void setPoint(Vec);
  Vec point();

  int moveAxis();
  void setMoveAxis(int);

  void checkIfGrabsMouse(int, int, const Camera* const);
  void mousePressEvent(QMouseEvent* const, Camera* const);
  void mouseMoveEvent(QMouseEvent* const, Camera* const);
  void mouseReleaseEvent(QMouseEvent* const, Camera* const);

  bool isInActivePool();
  void addToActivePool();
  void removeFromActivePool();

  static void clearActivePool();
  static const QList<HitPointGrabber*> activePool();

 signals :
  void updatePoint();
  
 private :
  Vec m_point;
  bool m_active;  
  int m_moveAxis;
  bool m_pressed;
  QPoint m_prevPos;
    

  static QList<HitPointGrabber*> activeHitPointPool_;
};

#endif
