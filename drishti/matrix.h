#ifndef MATRIX_H
#define MATRIX_H

#include <GL/glew.h>
#include <QGLViewer/qglviewer.h>
using namespace qglviewer;

class Matrix
{
 public :
  static void identity(double*);
  static void matmult(double*, double*, double*);
  static void matrixFromAxisAngle(double*, Vec, float);
  static void createTransformationMatrix(double*, Vec, Vec, Vec, float);
  static Vec xformVec(double*, Vec);
  static Vec rotateVec(double*, Vec);
  static void inverse(double*, double*);
};

#endif
