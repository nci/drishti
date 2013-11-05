#ifndef CROPGRABBER_H
#define CROPGRABBER_H

#include <QtGui>

#include "cropobject.h"

class CropGrabber : public QObject, public MouseGrabber, public CropObject
{
 Q_OBJECT

 public :
  CropGrabber();
  ~CropGrabber();

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
  bool m_pressed;
  QPoint m_prevPos;
  bool m_moved;
};

#endif
