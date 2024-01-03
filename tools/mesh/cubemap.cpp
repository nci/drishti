#include "cubemap.h"
#include "shaderfactory.h"

#include <QMessageBox>

float skyboxVertices[] = {
    // positions          
    -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

    -1.0f,  1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f,  1.0f
};


CubeMap::CubeMap() : QObject()
{
  m_visible = true;

  m_glTexture = 0;
  m_skyboxVAO = 0;
  m_skyboxVBO = 0;
}

CubeMap::~CubeMap()
{
  if (m_glTexture)
    glDeleteTextures(1, &m_glTexture);
  m_glTexture = 0;

  if (m_skyboxVBO)
    glDeleteVertexArrays(1, &m_skyboxVAO);
  m_skyboxVAO = 0;
  
  if (m_skyboxVBO)
    glDeleteBuffers(1, &m_skyboxVBO);
  m_skyboxVBO = 0;
}

void
CubeMap::loadCubemap(QStringList lst)
{
  if (!m_glTexture)
    glGenTextures(1, &m_glTexture);

  glActiveTexture(GL_TEXTURE4);
  glBindTexture(GL_TEXTURE_CUBE_MAP, m_glTexture);

  for (int i = 0; i < lst.count(); i++)
    {
      QImage img(lst[i]);
      int wd = img.width();
      int ht = img.height();

      glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 
		   0,
		   4,
		   wd, ht,
		   0,
		   GL_BGRA,
		   GL_UNSIGNED_BYTE,
		   img.bits());
    }
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);  
}

void
CubeMap::genVertData()
{
  // skybox VAO
  if (!m_skyboxVAO)
    glGenVertexArrays(1, &m_skyboxVAO);

  if (!m_skyboxVBO)
    glGenBuffers(1, &m_skyboxVBO);

  glBindVertexArray(m_skyboxVAO);
  glBindBuffer(GL_ARRAY_BUFFER, m_skyboxVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
}

void
CubeMap::draw(QMatrix4x4 mvp, QVector3D hmdPos, float scale)
{
  if (!m_visible)
    return;

  if (!m_skyboxVAO)
    genVertData();
  
  glDepthMask(GL_FALSE);
  glDepthFunc(GL_LEQUAL);


  glUseProgram(ShaderFactory::cubemapShader());
  GLint *cubemapShaderParm = ShaderFactory::cubemapShaderParm();
  glUniformMatrix4fv(cubemapShaderParm[0], 1, GL_FALSE, mvp.data() );  
  glUniform3f(cubemapShaderParm[1], hmdPos.x(), hmdPos.y(), hmdPos.z());  
  glUniform1f(cubemapShaderParm[2], scale); // scale
  glUniform1i(cubemapShaderParm[3], 4); // texture
 
  glActiveTexture(GL_TEXTURE4);
  glBindTexture(GL_TEXTURE_CUBE_MAP, m_glTexture);
  glEnable(GL_TEXTURE_CUBE_MAP);
  
  glBindVertexArray(m_skyboxVAO);
  glBindBuffer(GL_ARRAY_BUFFER, m_skyboxVBO);  
  glDrawArrays(GL_TRIANGLES, 0, 36);  
  glBindVertexArray(0);

  glDisable(GL_TEXTURE_CUBE_MAP);
  
  glUseProgram( 0 );

  glDepthMask(GL_TRUE);  
  glDepthFunc(GL_LESS);
}
