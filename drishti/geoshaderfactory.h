#ifndef GEOSHADERFACTORY_H
#define GEOSHADERFACTORY_H

#include <GL/glew.h>
#include <QtGui>
#include "cropobject.h"

class GeoShaderFactory
{
 public :
  static bool loadShader(GLhandleARB&, QString);

  static QString genVertexShaderString(QList<CropObject>);

  static QString genDefaultShaderString(QList<CropObject>);
  static QString genHighQualityShaderString(bool, float, QList<CropObject>);
  static QString genShadowShaderString(float, float, float, QList<CropObject>);

  static QString genSpriteShaderString();
  static QString genSpriteShadowShaderString(float, float, float);
};

#endif
