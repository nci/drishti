#ifndef IMAGECAPTIONGRABBER_H
#define IMAGECAPTIONGRABBER_H

#include <QGLViewer/manipulatedCameraFrame.h>
#include <QGLViewer/mouseGrabber.h>
#include <QGLViewer/qglviewer.h>
using namespace qglviewer;

#include "imagecaptionobject.h"

class ImageCaptionGrabber : public MouseGrabber, public ImageCaptionObject
{
 public :
  ImageCaptionGrabber();
  ~ImageCaptionGrabber();

  ImageCaptionObject imageCaption();

  void checkIfGrabsMouse(int, int, const Camera* const);
  void mousePressEvent(QMouseEvent* const, Camera* const);
  void mouseMoveEvent(QMouseEvent* const, Camera* const);
  void mouseReleaseEvent(QMouseEvent* const, Camera* const);

 private :
  bool m_pressed;    
};

#endif
