#ifndef ITKSEGMENTATION_H
#define ITKSEGMENTATION_H

#include <QGLViewer/qglviewer.h>
using namespace qglviewer;

#include "commonqtclasses.h"

class ITKSegmentation
{
 public :
  static bool applyITKFilter(int, int, int, uchar*, QList<Vec>);
};

#endif
