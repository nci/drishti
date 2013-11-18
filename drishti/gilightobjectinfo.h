#ifndef GILIGHTOBJECTINFO_H
#define GILIGHTOBJECTINFO_H

#include <QtGui>

#include <QGLViewer/qglviewer.h>
using namespace qglviewer;

#include <fstream>
using namespace std;

class GiLightObjectInfo
{
 public :
  GiLightObjectInfo();
  ~GiLightObjectInfo();

  void clear();

  GiLightObjectInfo& operator=(const GiLightObjectInfo&);

  static QList<GiLightObjectInfo> interpolate(QList<GiLightObjectInfo>,
					      QList<GiLightObjectInfo>,
					      float);

  void load(fstream&);
  void save(fstream&);

  QList<Vec> points;
  bool allowInterpolation;
  bool doShadows;
  bool show;
  int lightType;
  int rad;
  float decay;
  float angle;
  Vec color;
  float opacity;
  int segments;
  int lod;
  int smooth;

 private :
  static GiLightObjectInfo interpolate(GiLightObjectInfo,
				       GiLightObjectInfo,
				       float);
};

#endif
