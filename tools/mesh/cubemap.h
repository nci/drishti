#ifndef CUBEMAP_H
#define CUBEMAP_H

#include <GL/glew.h>

#include <QImage>
#include <QString>
#include <QMatrix4x4>

class CubeMap : public QObject
{
  Q_OBJECT

 public :
  CubeMap();
  ~CubeMap();

  void loadCubemap(QStringList);

  void draw(QMatrix4x4, QVector3D, float);

  void setVisible(bool b) { m_visible = b; }
  bool isVisible() { return m_visible; }

 private :
  bool m_visible;

  GLuint m_glTexture;
  GLuint m_skyboxVAO;
  GLuint m_skyboxVBO;

  void genVertData();


};

#endif // CUBEMAP_H
