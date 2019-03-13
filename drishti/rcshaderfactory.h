#ifndef RCSHADERFACTORY_H
#define RCSHADERFACTORY_H

#include <GL/glew.h>
#include "commonqtclasses.h"
#include "cropobject.h"

class RcShaderFactory
{
 public :
  static bool loadShader(GLhandleARB&, QString);

  static QString genRectBlurShaderString(int);

  static QString genRaycastShader(QList<CropObject>,
				    bool);

  static QString genEdgeEnhanceShader();

  static GLuint boxShader();
  static GLint* boxShaderParm();

  static bool loadShader(GLhandleARB&, QString, QString);

 private :
  static GLuint m_boxShader;
  static GLint m_boxShaderParm[20];

  static QString addLighting();
  static QString getGrad();
  static QString gradMagnitude();
  static QString getVal();
  
  static QString getExactVoxelCoord();
  static QString clip();

  static QString boxShaderV();
  static QString boxShaderF();

};

#endif
