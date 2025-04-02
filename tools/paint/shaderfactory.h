#ifndef SHADERFACTORY_H
#define SHADERFACTORY_H

#include <GL/glew.h>
#include "cropobject.h"
#include "commonqtclasses.h"

class ShaderFactory
{
 public :
  static bool loadShader(GLhandleARB&, QString);
  static bool loadShader(GLhandleARB&, QString, QString);

  static QString genDepthShader();

  static QString genFinalPointShader();

  static QString genRectBlurShaderString(int);

  static QString genSliceShader(bool);

  static QString genShadowSliceShader();

  static QString genShadowBlurShader();

  static QString genIsoRaycastShader(bool, bool, bool, int, QList<CropObject>);

  static QString genEdgeEnhanceShader(bool);

  static GLuint boxShader();
  static GLint* boxShaderParm();

 private :
  static QString addLighting();
  static QString getGrad(bool);
  static QString getGrad2(bool);
  static QString getGrad3(bool);

  static GLuint m_boxShader;
  static GLint m_boxShaderParm[20];

  static QString boxShaderV();
  static QString boxShaderF();
};

#endif
