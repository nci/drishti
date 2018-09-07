#ifndef SHADERFACTORY_H
#define SHADERFACTORY_H

#include <GL/glew.h>
#include "cropobject.h"

class ShaderFactory
{
 public :
  static bool loadShader(GLhandleARB&, QString);
  static bool loadShader(GLhandleARB&, QString, QString);


  static QString genDefaultShaderString(bool, bool, int);

  static QString genBlurShaderString(bool, int, float);
  static QString genBoxShaderString();
  static QString genRectBlurShaderString(int);
  static QString genCopyShaderString();
  static QString genReduceShaderString();
  static QString genExtractSliceShaderString();
  static QString genBackplaneShaderString1(float);
  static QString genBackplaneShaderString2(float);

  static QString genPassThruShaderString();

  static int loadShaderFromFile(GLhandleARB, const char*);


  static QString genDefaultSliceShaderString(bool, bool, bool,
					     QList<CropObject>,
					     bool, int, float, float, float);

  static QString genTextureCoordinate();

  static QString genLutShaderString(bool);

  static GLuint meshShader();
  static GLint* meshShaderParm();

  static GLuint ptShader();
  static GLint* ptShaderParm();

 private :
  static GLuint m_meshShader;
  static GLint m_meshShaderParm[20];

  static GLuint m_ptShader;
  static GLint m_ptShaderParm[20];

  static QString getNormal();
  static QString addLighting();
  static QString tagVolume();
  static QString blendVolume();
  static QString genPeelShader(bool, int, float, float, float, bool);
  static QString genVgx();

  static QString meshShaderV();
  static QString meshShaderF();

  static QString ptShaderV();
  static QString ptShaderF();
};

#endif
