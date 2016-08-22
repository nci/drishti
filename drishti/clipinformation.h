#ifndef CLIPINFORMATION_H
#define CLIPINFORMATION_H

#include <GL/glew.h>

#include "commonqtclasses.h"

#include <QGLViewer/vec.h>
#include <QGLViewer/quaternion.h>
using namespace qglviewer;

#include <fstream>
using namespace std;

class ClipInformation
{
 public :
  ClipInformation();
  ~ClipInformation();
  ClipInformation(const ClipInformation&);

  ClipInformation& operator=(const ClipInformation&);

  static ClipInformation interpolate(const ClipInformation,
				     const ClipInformation,
				     float);

  void load(fstream&);
  void save(fstream&);
  
  void clear();
  int size();

  QList<bool> show;
  QList<Vec> pos;
  QList<Quaternion> rot;
  QList<Vec> color;
  QList<bool> solidColor;
  QList<bool> apply;
  QList<bool> applyFlip;
  QList<QString> imageName;
  QList<int> imageFrame;
  QList<QString> captionText;
  QList<QFont> captionFont;
  QList<QColor> captionColor;
  QList<QColor> captionHaloColor;
  QList<float> opacity;
  QList<float> stereo;
  QList<float> scale1;
  QList<float> scale2;
  QList<int> tfSet;
  QList<QVector4D> viewport;
  QList<bool> viewportType;
  QList<float> viewportScale;
  QList<int> thickness;
  QList<bool> showSlice;
  QList<bool> showOtherSlice;
  QList<bool> showThickness;
  QList<int> gridX;
  QList<int> gridY;
  QList<float> opmod;
};

#endif
