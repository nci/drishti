#include "computeshaderfactory.h"
#include "shaderfactory.h"

#include <QFile>
#include <QMessageBox>
#include <QCoreApplication>
#include <QDir>

//---------------
GLuint ComputeShaderFactory::m_paintShader = 0;
GLuint ComputeShaderFactory::paintShader()
{
  if (!m_paintShader)
    createPaintShader();
  
  return m_paintShader;
}

GLint ComputeShaderFactory::m_paintShaderParm[10];
GLint* ComputeShaderFactory::paintShaderParm() { return &m_paintShaderParm[0]; }

void
ComputeShaderFactory::createPaintShader()
{
  m_paintShader = glCreateProgram();
  


  QFile pfile(qApp->applicationDirPath() + QDir::separator() + "assets/shaders/paintShader.glsl");
  if (!pfile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
      QMessageBox::information(0, "", "PaintShader not found in assets/shaders");
      return;
    }
  QString computeShaderString = QString::fromLatin1(pfile.readAll());

  if (!ShaderFactory::addShader(m_paintShader,
				GL_COMPUTE_SHADER,
				computeShaderString))
    {
      QMessageBox::information(0, "", "Cannot load Paint shader");
      return;
    }


  
  if (! ShaderFactory::finalize(m_paintShader) )
    return;
  
  m_paintShaderParm[0] = glGetUniformLocation(m_paintShader, "hitPt");
  m_paintShaderParm[1] = glGetUniformLocation(m_paintShader, "radius");
  m_paintShaderParm[2] = glGetUniformLocation(m_paintShader, "hitColor");
  m_paintShaderParm[3] = glGetUniformLocation(m_paintShader, "blendType");
  m_paintShaderParm[4] = glGetUniformLocation(m_paintShader, "blendFraction");
  m_paintShaderParm[5] = glGetUniformLocation(m_paintShader, "blendOctave");
  m_paintShaderParm[6] = glGetUniformLocation(m_paintShader, "bmin");
  m_paintShaderParm[7] = glGetUniformLocation(m_paintShader, "blen");
  m_paintShaderParm[8] = glGetUniformLocation(m_paintShader, "roughnessType");
}
//---------------
