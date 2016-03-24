#ifndef RCSHADERFACTORY_H
#define RCSHADERFACTORY_H

#include <GL/glew.h>
#include "commonqtclasses.h"

class RcShaderFactory
{
 public :
  static bool loadShader(GLhandleARB&, QString);

  static QString genRectBlurShaderString(int);

  static QString genFirstHitShader(int, bool);

  static QString genRaycastShader(int, bool, bool, float);

  static QString genXRayShader(int, bool, bool, float);

  static QString genEdgeEnhanceShader();

 private :
  static QString addLighting();
  static QString getGrad();
};

#endif
