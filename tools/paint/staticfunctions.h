#ifndef STATICFUNCTIONS_H
#define STATICFUNCTIONS_H

#include "commonqtclasses.h"

#include <QProgressDialog>
#include <QColorDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QDomDocument>

#include <QGLViewer/vec.h>
using namespace qglviewer;

#define DEG2RAD(angle) angle*3.1415926535897931/180.0
#define RAD2DEG(angle) angle*180.0/3.1415926535897931
#define VECPRODUCT(a, b) Vec(a.x*b.x, a.y*b.y, a.z*b.z)
#define VECDIVIDE(a, b) Vec(a.x/b.x, a.y/b.y, a.z/b.z)

class StaticFunctions
{
 public :
  static Vec getVec(QString);

  static Vec clampVec(Vec, Vec, Vec);
  static Vec maxVec(Vec, Vec);
  static Vec minVec(Vec, Vec);

  static void getRotationBetweenVectors(Vec, Vec, Vec&, float&);

  static QGradientStops resampleGradientStops(QGradientStops, int mapSize = 100);
  static void initQColorDialog();

  static bool checkExtension(QString, const char*);
  static bool checkURLs(QList<QUrl>, const char*);

  static bool checkRGB(QString);
  static bool checkRGBA(QString);

  static void generateHistograms(float*, float*, int*, int*);

  static bool xmlHeaderFile(QString);
  static void getDimensionsFromHeader(QString, int&, int&, int&);
  static Vec getVoxelSizeFromHeader(QString);
  static QString getVoxelUnitFromHeader(QString);
  static int getSlabsizeFromHeader(QString);
  static int getPvlVoxelTypeFromHeader(QString);
  static int getPvlHeadersizeFromHeader(QString);
  static int getRawHeadersizeFromHeader(QString);
  static QStringList getPvlNamesFromHeader(QString);
  static QStringList getRawNamesFromHeader(QString);

  static bool inTriangle(Vec, Vec, Vec, Vec);

  static void renderText(int, int,
			 QString, QFont,
			 QColor, QColor,
			 bool useTextPath = false);
  static void renderRotatedText(int, int,
				QString, QFont,
				QColor, QColor,
				float, bool,
				bool useTextPath = false);

  static void pushOrthoView(float, float, float, float);
  static void popOrthoView();
  static void drawQuad(float, float, float, float, float);

  static void drawEnclosingCube(Vec, Vec);

  static int intersectType1(Vec, Vec, Vec, Vec, Vec&);
  static int intersectType1WithTexture(Vec, Vec,
				       Vec, Vec,
				       Vec, Vec,
				       Vec&, Vec&);

  static int intersectType2(Vec, Vec, Vec&, Vec&);
  static int intersectType2WithTexture(Vec, Vec,
				       Vec&, Vec&,
				       Vec&, Vec&);

  static void convertFromGLImage(QImage&, int, int);

  static QSize getImageSize(int, int);

  static QList<Vec> line3d(Vec, Vec);

  static QList<int> dda2D(int, int, int, int);

  static float easeIn(float);
  static float easeOut(float);
  static float smoothstep(float);
  static float smoothstep(float, float, float);

  static void smooth2DArray(float*, float*, int, int);

  static void showMessage(QString, QString);

  static void imageFromDataAndColor(QImage&, ushort*, uchar*);

  static void saveMesgToFile(QString, QString);
  static void saveVolumeToFile(QString,
			       uchar, char*,
			       int, int, int);

  static void savePvlHeader(QString,
			    bool, QString,
			    int, int, int,
			    int, int, int,
			    float, float, float,
			    QList<float>, QList<int>,
			    QString,
			    int);
};

#endif
