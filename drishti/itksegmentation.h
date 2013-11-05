#ifndef ITKSEGMENTATION_H
#define ITKSEGMENTATION_H

#include <QtGui>
#include <QGLViewer/qglviewer.h>
using namespace qglviewer;

class ITKSegmentation
{
 public :
  static bool applyITKFilter(int, int, int, uchar*, QList<Vec>);
};

#endif
