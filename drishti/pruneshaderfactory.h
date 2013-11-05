#ifndef PRUNESHADERFACTORY_H
#define PRUNESHADERFACTORY_H

#include <GL/glew.h>
#include <QtGui>

class PruneShaderFactory
{
 public :
  static QString genPruneTexture(bool);
  static QString dilate();
  static QString erode();
  static QString shrink();
  static QString setValue();
  static QString invert();
  static QString thicken();
  static QString edgeTexture();
  static QString dilateEdgeTexture();
  static QString copyChannel();
  static QString minTexture();
  static QString maxTexture();
  static QString xorTexture();
  static QString localMaximum();
  static QString restrictedDilate();
  static QString carve();
  static QString paint();
  static QString fillTriangle();
  static QString removePatch();
  static QString clip();
  static QString crop(QString);
  static QString maxValue();
  static QString localThickness();
  static QString smoothChannel();
  static QString average();
  static QString histogram();
  static QString pattern();
};

#endif
