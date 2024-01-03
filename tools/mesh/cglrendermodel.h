#ifndef CGLRENDERMODEL
#define CGLRENDERMODEL

#include <QString>
#include <openvr.h>

class CGLRenderModel
{
public:
  CGLRenderModel(QString);
  ~CGLRenderModel();
  
  bool BInit( vr::RenderModel_t&,
	      vr::RenderModel_TextureMap_t&);
  void Cleanup();
  void Draw();
  QString GetName() { return m_sModelName; }
  
 private:
  GLuint m_glVertBuffer;
  GLuint m_glIndexBuffer;
  GLuint m_glVertArray;
  GLuint m_glTexture;
  GLsizei m_unVertexCount;
  QString m_sModelName;
};

#endif
