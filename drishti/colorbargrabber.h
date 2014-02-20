#ifndef COLORBARGRABBER_H
#define COLORBARGRABBER_H

#include <QGLViewer/mouseGrabber.h>
#include <QGLViewer/qglviewer.h>
using namespace qglviewer;

#include "colorbarobject.h"

class ColorBarGrabber : public MouseGrabber, public ColorBarObject
{
 public :
  ColorBarGrabber();
  ~ColorBarGrabber();

  ColorBarObject colorbar();

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
