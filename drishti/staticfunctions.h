#ifndef STATICFUNCTIONS_H
#define STATICFUNCTIONS_H

#include <GL/glew.h>
#include <QtGui>
#include <QGLViewer/qglviewer.h>
using namespace qglviewer;

#include "classes.h"

#define DEG2RAD(angle) angle*3.1415926535897931/180.0
#define RAD2DEG(angle) angle*180.0/3.1415926535897931
#define VECPRODUCT(a, b) Vec(a.x*b.x, a.y*b.y, a.z*b.z)
#define VECDIVIDE(a, b) Vec(a.x/b.x, a.y/b.y, a.z/b.z)

class StaticFunctions
{
 public :
  static Vec getVec(QString);

  static int getPowerOf2(int);

  static Vec interpolate(Vec, Vec, float);

  static Vec clampVec(Vec, Vec, Vec);
  static Vec maxVec(Vec, Vec);
  static Vec minVec(Vec, Vec);

  static int getSubsamplingLevel(int, int, Vec, Vec);

  static void getRotationBetweenVectors(Vec, Vec, Vec&, float&);

  static QGradientStops resampleGradientStops(QGradientStops, int mapSize = 100);
  static void initQColorDialog();

  static void drawEnclosingCube(Vec*, Vec);
  static void drawEnclosingCube(Vec, Vec);
  static void drawEnclosingCubeWithTransformation(Vec, Vec,
						  double*,
						  Vec);

  static void drawAxis(Vec, Vec, Vec, Vec);

  static int intersectType1(Vec, Vec, Vec, Vec, Vec&);
  static int intersectType1WithTexture(Vec, Vec,
				       Vec, Vec,
				       Vec, Vec,
				       Vec&, Vec&);

  static int intersectType2(Vec, Vec, Vec&, Vec&);
  static int intersectType2WithTexture(Vec, Vec,
				       Vec&, Vec&,
				       Vec&, Vec&);

  static int intersectType3WithTexture(Vec, Vec,
				       Vec&, Vec&,
				       Vec&, Vec&,
				       Vec&, Vec&);

  static void getMinMaxBrickVertices(Camera*,
				     Vec, Vec,
				     float&,
				     Vec&, Vec&,
				     int&);
  static void getMinMaxProjectionVertices(Camera*,
					  Vec*,
					  float&, Vec&, Vec&, int&);

  static int getScaledown(int, int);

  static void pushOrthoView(float, float, float, float);
  static void popOrthoView();
  static void drawQuad(float, float, float, float, float);

  static bool checkExtension(QString, const char*);
  static bool checkURLs(QList<QUrl>, const char*);

  static float calculateAngle(Vec, Vec, Vec);

  static QList<Vec> voxelizeLine(Vec, Vec);
  static VoxelizedPath voxelizePath(QList<Vec>);
  static VoxelizedPath voxelizePath(QList< QPair<Vec, Vec> >);

  static void generateHistograms(float*, float*, int*, int*);

  static bool getClip(Vec, QList<Vec>, QList<Vec>);

  static QSize getImageSize(int, int);

  static float remapKeyframe(int, float);
  static float easeIn(float);
  static float easeOut(float);
  static float smoothstep(float);
  static float smoothstep(float, float, float);


  static QVector<Vec> generateUnitSphere(int);

  static bool inTriangle(Vec, Vec, Vec, Vec);

  static QString replaceDirectory(QString, QString);

  static void convertFromGLImage(QImage&, int, int);
};

#endif
