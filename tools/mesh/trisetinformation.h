#ifndef TRISETINFORMATION_H
#define TRISETINFORMATION_H

#include <QGLViewer/qglviewer.h>
using namespace qglviewer;

#include <fstream>
using namespace std;

#include "commonqtclasses.h"

class TrisetInformation
{
 public :
  TrisetInformation();
  TrisetInformation& operator=(const TrisetInformation&);

  static TrisetInformation interpolate(const TrisetInformation,
				       const TrisetInformation,
				       float);

  static QList<TrisetInformation> interpolate(const QList<TrisetInformation>,
					      const QList<TrisetInformation>,
					      float);

  void clear();

  void save(fstream&);
  void load(fstream&);

  bool show;
  bool clip;
  bool clearView;
  QString filename;
  Vec position;
  Vec scale;
  Quaternion q;
  Vec color;
  int materialId;
  float materialMix;
  Vec cropcolor;
  float roughness;
  float ambient;
  float diffuse;
  float specular;
  float reveal;
  float outline;
  float glow;
  float dark;
  Vec pattern;
  float opacity;
  bool lineMode;
  float lineWidth;
  
  QList<QString> captionText;
  QList<QColor> captionColor;
  QList<QFont> captionFont;
  QList<Vec> captionPosition;
  QList<float> cpDx;
  QList<float> cpDy;
};

#endif
