#ifndef SHADERFACTORY_H
#define SHADERFACTORY_H

#include <GL/glew.h>
#include "commonqtclasses.h"

class ShaderFactory
{
 public :
  static bool loadShader(GLhandleARB&, QString);

  static QString genDepthShader();

  static QString genFinalPointShader();

  static QString genRectBlurShaderString(int);

  static QString genSliceShader();

  static QString genRaycastShader(int, bool, bool);

  static QString genEdgeEnhanceShader();

};

#endif
