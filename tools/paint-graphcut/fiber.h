#ifndef FIBER_H
#define FIBER_H

#include <QGLViewer/vec.h>
using namespace qglviewer;

class Fiber
{
 public :
  Fiber();
  ~Fiber();

  Fiber& operator=(const Fiber&);

  QVector<Vec> pts;
  int tag;
  int thickness;
  bool selected;
};

#endif
