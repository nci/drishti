#ifndef VOLUMEMEASURE_H
#define VOLUMEMEASURE_H

#include "commonqtclasses.h"
#include <QProgressDialog>
#include <QMap>

#include <QGLViewer/vec.h>
using namespace qglviewer;

#include "mybitarray.h"

class VolumeMeasure
{
 public :
  static void setVolData(uchar*);
  static void setMaskData(uchar*);
  static void setGridSize(int, int, int);

  static void getVolume(Vec, Vec, int);
  static void getSurfaceArea(Vec, Vec, int);
  static void getFeretDiameter(Vec, Vec, int);
  
 private :
  static int m_depth, m_width, m_height;
  static uchar *m_volData;
  static ushort *m_volDataUS;
  static uchar *m_maskData;

  static QMap<int, float> volume(Vec, Vec, int);
  static QMap<int, float> surfaceArea(Vec, Vec, int);
  static float feretDiameter(int, int, int, MyBitArray&);

  static void parFeret(QList<QVariant>);
};

#endif
