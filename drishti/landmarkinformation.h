#ifndef LANDMARKINFORMATION_H
#define LANDMARKINFORMATION_H

#include <QGLViewer/qglviewer.h>
using namespace qglviewer;

#include "commonqtclasses.h"

#include <fstream>
using namespace std;

class LandmarkInformation
{
 public :
  LandmarkInformation();
  ~LandmarkInformation();
  
  LandmarkInformation& operator=(const LandmarkInformation&);

  void clear();

  void save(fstream&);
  void load(fstream&);

  Vec textColor;
  int textSize;

  Vec pointColor;
  int pointSize;

  QList<Vec> points;
  QList<QString> text;

  bool showCoordinates;
  bool showText;
  bool showNumber;

  QString distance;
  QString projectline;
  QString projectplane;
};

#endif
