#ifndef CAPTIONGRABBER_H
#define CAPTIONGRABBER_H

#include "captionobject.h"

#include <QGLViewer/mouseGrabber.h>
#include <QGLViewer/qglviewer.h>
using namespace qglviewer;

class CaptionGrabber : public MouseGrabber, public CaptionObject
{
 public :
  CaptionGrabber();
  ~CaptionGrabber();

  CaptionObject caption();

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
