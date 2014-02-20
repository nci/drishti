#ifndef SCALEBARGRABBER_H
#define SCALEBARGRABBER_H

#include <QGLViewer/mouseGrabber.h>
#include <QGLViewer/manipulatedCameraFrame.h>
using namespace qglviewer;

#include "scalebarobject.h"

class ScaleBarGrabber : public MouseGrabber, public ScaleBarObject
{
 public :
  ScaleBarGrabber();
  ~ScaleBarGrabber();

  ScaleBarObject scalebar();

  void checkIfGrabsMouse(int, int, const Camera* const);
  void mousePressEvent(QMouseEvent* const, Camera* const);
  void mouseReleaseEvent(QMouseEvent* const, Camera* const);
  void mouseMoveEvent(QMouseEvent* const, Camera* const);

 private :
  bool m_dragging;
  int m_prevx, m_prevy;
  QPointF m_prevpos;
};


#endif
