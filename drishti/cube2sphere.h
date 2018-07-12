#ifndef CUBE2SPHERE_H
#define CUBE2SPHERE_H

#include <QGLViewer/qglviewer.h>
using namespace qglviewer;

#include <QImage>

class Cube2Sphere
{
 public :
  static QImage convert(QList<QImage>);
  
 private :
  static Vec sphericalCoordinates(float, float, float, float);
  static Vec vectorCoordinates(float, float);
  static int getFace(Vec);
  static Vec rawFaceCoordinates(int, Vec);
  static Vec rawCoordinates(Vec);
  static Vec findCorrespondingPixel(int, int, int, int, int);

};

#endif
