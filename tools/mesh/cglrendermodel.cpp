#include <GL/glew.h>

#include "cglrendermodel.h"

#include <QMessageBox>


CGLRenderModel::CGLRenderModel(QString sRenderModelName)
{
  m_sModelName = sRenderModelName;
  m_glIndexBuffer = 0;
  m_glVertArray = 0;
  m_glVertBuffer = 0;
  m_glTexture = 0;
}


CGLRenderModel::~CGLRenderModel()
{
  Cleanup();
}


bool
CGLRenderModel::BInit(vr::RenderModel_t & vrModel,
		      vr::RenderModel_TextureMap_t & vrDiffuseTexture)
{
  // create and bind a VAO to hold state for this model
  glGenVertexArrays( 1, &m_glVertArray );
  glBindVertexArray( m_glVertArray );
  
  // Populate a vertex buffer
  glGenBuffers( 1, &m_glVertBuffer );
  glBindBuffer( GL_ARRAY_BUFFER, m_glVertBuffer );
  glBufferData( GL_ARRAY_BUFFER,
		sizeof( vr::RenderModel_Vertex_t ) * vrModel.unVertexCount,
		vrModel.rVertexData,
		GL_STATIC_DRAW );
  
  // Identify the components in the vertex buffer
  glEnableVertexAttribArray( 0 );
  glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE,
			 sizeof( vr::RenderModel_Vertex_t ),
			 (void *)offsetof( vr::RenderModel_Vertex_t, vPosition ) );
  glEnableVertexAttribArray( 1 );
  glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE,
			 sizeof( vr::RenderModel_Vertex_t ),
			 (void *)offsetof( vr::RenderModel_Vertex_t, vNormal ) );
  glEnableVertexAttribArray( 2 );
  glVertexAttribPointer( 2, 2, GL_FLOAT, GL_FALSE, 
			 sizeof( vr::RenderModel_Vertex_t ),
			 (void *)offsetof( vr::RenderModel_Vertex_t, rfTextureCoord ) );
  
  // Create and populate the index buffer
  glGenBuffers( 1, &m_glIndexBuffer );
  glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_glIndexBuffer );
  glBufferData( GL_ELEMENT_ARRAY_BUFFER,
		sizeof( uint16_t ) * vrModel.unTriangleCount * 3,
		vrModel.rIndexData,
		GL_STATIC_DRAW );
  
  glBindVertexArray( 0 );
  
  // create and populate the texture
  glGenTextures(1, &m_glTexture );
  glBindTexture( GL_TEXTURE_2D, m_glTexture );
  
  glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, vrDiffuseTexture.unWidth, vrDiffuseTexture.unHeight,
		0, GL_RGBA, GL_UNSIGNED_BYTE, vrDiffuseTexture.rubTextureMapData );
  
  // If this renders black - what's wrong.
  glGenerateMipmap(GL_TEXTURE_2D);
  
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
  
  GLfloat fLargest;
  glGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest );
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, fLargest );
  
  glBindTexture( GL_TEXTURE_2D, 0 );
  
  m_unVertexCount = vrModel.unTriangleCount * 3;

  return true;
}

void CGLRenderModel::Cleanup()
{
  if( m_glVertBuffer )
    {
      glDeleteBuffers(1, &m_glIndexBuffer);
      glDeleteVertexArrays( 1, &m_glVertArray );
      glDeleteBuffers(1, &m_glVertBuffer);
      m_glIndexBuffer = 0;
      m_glVertArray = 0;
      m_glVertBuffer = 0;
    }
}

void CGLRenderModel::Draw()
{
  glBindVertexArray(m_glVertArray);
  glBindBuffer(GL_ARRAY_BUFFER, m_glVertBuffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_glIndexBuffer);  
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glEnableVertexAttribArray(2);

  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, m_glTexture);
  
  glDrawElements(GL_TRIANGLES, m_unVertexCount, GL_UNSIGNED_SHORT, 0);
  
  glActiveTexture(GL_TEXTURE0);
  glDisable(GL_TEXTURE_2D);

  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
  glDisableVertexAttribArray(2);
  glBindVertexArray( 0 );
}


