#ifndef SCALEBAR_H
#define SCALEBAR_H

#include "scalebargrabber.h"
#include "clipinformation.h"

class ScaleBars
{
 public :
  ScaleBars();
  ~ScaleBars();

  bool grabsMouse();

  bool isValid();

  QList<ScaleBarObject> scalebars();

  void clear();
  void add(ScaleBarObject);
  void setScaleBars(QList<ScaleBarObject>);

  void draw(QGLViewer*, ClipInformation);

  bool keyPressEvent(QKeyEvent*);

 private :
  QList<ScaleBarGrabber*> m_scalebars;
};

#endif
