#ifndef SHADERFACTORY_H
#define SHADERFACTORY_H

#include <GL/glew.h>
#include <QString>

class ShaderFactory
{
 public :
  static bool loadShader(GLuint&, QString);
  static bool loadShader(GLuint&, QString, QString);

  static QString genBlurShaderString(bool, int, float);
  static QString genDilateShaderString();
  static QString genCopyShaderString();
  static QString genBackplaneShaderString1(float);
  static QString genBackplaneShaderString2(float);

  static QString genPassThruShaderString();

  static bool loadShaderFromFile(GLuint, QString);
  static bool loadShadersFromFile(GLuint&, QString, QString);


  static GLuint meshShader();
  static GLint* meshShaderParm();

  static GLuint meshShadowShader();
  static GLint* meshShadowShaderParm();

  static GLuint planeShadowShader();
  static GLint* planeShadowShaderParm();

  static GLuint outlineShader();
  static GLint* outlineShaderParm();

  static GLuint oitShader();
  static GLint* oitShaderParm();

  static GLuint oitFinalShader();
  static GLint* oitFinalShaderParm();

  static GLuint ptShader();
  static GLint* ptShaderParm();

  static GLuint pnShader();
  static GLint* pnShaderParm();

  static GLuint cubemapShader();
  static GLint* cubemapShaderParm();

  static GLuint rcShader();
  static GLint* rcShaderParm();

  static GLuint blurShader();
  static GLint* blurShaderParm();

  static GLuint copyShader();
  static GLint* copyShaderParm();

  static GLuint dilateShader();
  static GLint* dilateShaderParm();

  static QString OrenNayarDiffuseShader();
  static QString ggxShader();
  static QString rgb2hsv();
  static QString hsv2rgb();
  static QString noise2d();
  static QString noise3d();

  static bool addShader(GLuint, GLenum, QString);
  static bool finalize(GLuint);

 private :
  static QList<GLuint> m_shaderList;
  
  static GLuint m_meshShader;
  static GLint m_meshShaderParm[40];

  static GLuint m_meshShadowShader;
  static GLint m_meshShadowShaderParm[20];

  static GLuint m_planeShadowShader;
  static GLint m_planeShadowShaderParm[10];

  static GLuint m_outlineShader;
  static GLint m_outlineShaderParm[30];

  static GLuint m_oitShader;
  static GLint m_oitShaderParm[30];

  static GLuint m_oitFinalShader;
  static GLint m_oitFinalShaderParm[30];

  static GLuint m_ptShader;
  static GLint m_ptShaderParm[20];

  static GLuint m_pnShader;
  static GLint m_pnShaderParm[20];

  static GLuint m_rcShader;
  static GLint m_rcShaderParm[10];

  static GLuint m_cubemapShader;
  static GLint m_cubemapShaderParm[10];

  static GLuint m_blurShader;
  static GLint m_blurShaderParm[10];

  static GLuint m_copyShader;
  static GLint m_copyShaderParm[10];

  static GLuint m_dilateShader;
  static GLint m_dilateShaderParm[10];
  
  static void createCubeMapShader();

  static void createTextureShader();

  static QString meshShaderV();
  static QString meshShaderF();

  static QString meshShadowShaderV();
  static QString meshShadowShaderF();

  static QString planeShadowShaderV();
  static QString planeShadowShaderF();

  static QString outlineShaderF();

  static QString ptShaderV();
  static QString ptShaderF();

  static QString pnShaderV();
  static QString pnShaderF();

  static QString oitShaderV();
  static QString oitShaderF();

  static QString oitFinalShaderF();

  
  static QString genBoxShaderString();
  static QString genRectBlurShaderString(int);
  static QString genSmoothDilatedShaderString();

};

#endif
