#ifndef RCSHADERFACTORY_H
#define RCSHADERFACTORY_H

#include <GL/glew.h>
#include "commonqtclasses.h"

class RcShaderFactory
{
 public :
  static bool loadShader(GLhandleARB&, QString);

  static QString genRectBlurShaderString(int);

  static QString genRaycastShader(int, bool, bool, bool);

  static QString genXRayShader(int, bool, bool, bool);

  static QString genEdgeEnhanceShader();

 private :
  static QString addLighting();
  static QString getGrad();
};

#endif
