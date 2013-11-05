#ifndef COLORBAR_H
#define COLORBAR_H

#include "colorbargrabber.h"

class ColorBars
{
 public :
  ColorBars();
  ~ColorBars();

  bool grabsMouse();

  bool isValid();

  QList<ColorBarObject> colorbars();

  void clear();
  void add(ColorBarObject);
  void setColorBars(QList<ColorBarObject>);

  void draw(QGLViewer*);

  bool keyPressEvent(QKeyEvent*);

 private :
  QList<ColorBarGrabber*> m_colorbars;
};

#endif
