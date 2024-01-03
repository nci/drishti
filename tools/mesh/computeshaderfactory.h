#ifndef COMPUTESHADERFACTORY_H
#define COMPUTESHADERFACTORY_H

#include <GL/glew.h>

class ComputeShaderFactory
{
 public :
  static GLuint paintShader();
  static GLint* paintShaderParm();

 private :
  static GLuint m_paintShader;
  static GLint m_paintShaderParm[10];

  static void createPaintShader();
};

#endif
