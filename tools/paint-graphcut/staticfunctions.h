#ifndef STATICFUNCTIONS_H
#define STATICFUNCTIONS_H

#include "commonqtclasses.h"

#include <QProgressDialog>
#include <QColorDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QDomDocument>

#define DEG2RAD(angle) angle*3.1415926535897931/180.0
#define RAD2DEG(angle) angle*180.0/3.1415926535897931
#define VECPRODUCT(a, b) Vec(a.x*b.x, a.y*b.y, a.z*b.z)
#define VECDIVIDE(a, b) Vec(a.x/b.x, a.y/b.y, a.z/b.z)

class StaticFunctions
{
 public :
  static QGradientStops resampleGradientStops(QGradientStops, int mapSize = 100);
  static void initQColorDialog();

  static bool checkExtension(QString, const char*);
  static bool checkURLs(QList<QUrl>, const char*);

  static bool checkRGB(QString);
  static bool checkRGBA(QString);

  static void generateHistograms(float*, float*, int*, int*);

  static bool xmlHeaderFile(QString);
  static void getDimensionsFromHeader(QString, int&, int&, int&);
  static int getSlabsizeFromHeader(QString);
  static int getPvlHeadersizeFromHeader(QString);
  static int getRawHeadersizeFromHeader(QString);
  static QStringList getPvlNamesFromHeader(QString);
  static QStringList getRawNamesFromHeader(QString);

};

#endif
