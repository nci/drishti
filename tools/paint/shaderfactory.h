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

  static QString genSliceShader(bool);

  static QString genShadowSliceShader();

  static QString genShadowBlurShader();

  static QString genIsoRaycastShader(bool, bool, bool);

  static QString genRaycastShader(int, bool, bool, bool, bool);

  static QString genXRayShader(int, bool, bool, bool, bool);

  static QString genEdgeEnhanceShader(bool);

 private :
  static QString addLighting();
  static QString getGrad();
};

#endif
