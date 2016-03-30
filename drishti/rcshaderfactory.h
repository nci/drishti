#ifndef RCSHADERFACTORY_H
#define RCSHADERFACTORY_H

#include <GL/glew.h>
#include "commonqtclasses.h"

class RcShaderFactory
{
 public :
  static bool loadShader(GLhandleARB&, QString);

  static QString genRectBlurShaderString(int);

  static QString genFirstHitShader(bool);

  static QString genIsoRaycastShader(bool);

  static QString genRaycastShader(bool, float);

  static QString genXRayShader(bool, float);

  static QString genEdgeEnhanceShader();

 private :
  static QString addLighting();
  static QString getGrad();
};

#endif
