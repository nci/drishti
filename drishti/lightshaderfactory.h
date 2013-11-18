#ifndef LIGHTSHADERFACTORY_H
#define LIGHTSHADERFACTORY_H


#include <GL/glew.h>
#include <QtGui>

class LightShaderFactory
{
 public :
  static QString genOpacityShader(bool);
  static QString genOpacityShader2(int);
  static QString genOpacityShaderRGB();

  static QString genAOLightShader();

  static QString genInitEmissiveShader();
  static QString genEmissiveShader();

  static QString genInitDLightShader();
  static QString genDLightShader();

  static QString genInitTubeLightShader();
  static QString genTubeLightShader();

  static QString genExpandLightShader();

  static QString genFinalLightShader();
  static QString genEFinalLightShader();

  static QString genDiffuseLightShader();

  static QString genMergeOpPruneShader();
};

#endif
