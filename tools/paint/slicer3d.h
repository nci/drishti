#ifndef SLICER3D_H
#define SLICER3D_H

#include <QGLViewer/qglviewer.h>
#include <QGLViewer/vec.h>
using namespace qglviewer;

class Slicer3D
{
 public :
  static void getMinMaxVertices(Camera*,
				Vec*, float&,
				Vec&, Vec&);

  static void drawSlices(Vec, Vec,
			 Vec, Vec,
			 Vec, Vec, Vec,
			 int, float,
			 QList<Vec>, QList<Vec>,
			 bool);

  static int intersectType1(Vec, Vec,
			    Vec, Vec,
			    Vec&);

  static int intersectType2(Vec, Vec,
			    Vec&, Vec&);

  static int drawpoly(Vec, Vec,
		      Vec*,
		      Vec, Vec,
		      QList<Vec>, QList<Vec>);
};

#endif
