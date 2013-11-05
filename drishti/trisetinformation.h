#ifndef TRISETINFORMATION_H
#define TRISETINFORMATION_H

#include <QGLViewer/qglviewer.h>
using namespace qglviewer;

#include <fstream>
using namespace std;

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
  
  QString filename;
  Vec position;
  Vec scale;
  Vec color;
  Vec cropcolor;
  float opacity;
  float ambient;
  float diffuse;
  float specular;
  bool pointMode;
  int pointStep;
  int pointSize;
  bool blendMode;
  bool shadows;
  bool screenDoor;
  bool flipNormals;
};

#endif
