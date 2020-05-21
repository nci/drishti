#ifndef TRISETOBJECT_H
#define TRISETOBJECT_H

#include <QFile>

#include "trisetinformation.h"
#include <QVector4D>

class TrisetObject
{
 public :
  TrisetObject();
  ~TrisetObject();

  void gridSize(int&, int&, int&);

  bool show() { return m_show; }
  void setShow(bool s) { m_show = s; }

  bool clip() { return m_clip; }
  void setClip(bool s) { m_clip = s; }

  Vec tcentroid() { return m_tcentroid; }
  Vec centroid() { return m_centroid; }
  void enclosingBox(Vec&, Vec&);

  QString filename() { return m_fileName; }

  void setLighting(QVector4D);
  float roughness() { return m_roughness; }
  float specular() { return m_specular; }
  float diffuse() { return m_diffuse; }
  float ambient() { return m_ambient; }

  Vec color() { return m_color; }
  void setColor(Vec);
  //void setColor(Vec color) { m_color = color; }

  Vec cropBorderColor() { return m_cropcolor; }
  void setCropBorderColor(Vec color) { m_cropcolor = color; }

  Vec position() { return m_position; }
  void setPosition(Vec pos) { m_position = pos; }

  Vec scale() { return m_scale; }
  void setScale(Vec scl) { m_scale = scl; }

  float activeScale() { return m_activeScale; }
  void setActiveScale(float s) { m_activeScale = s; }

  Quaternion rotation() { return m_q; }
  void rotate(Vec, float);
  void rotate(Quaternion);
  void setRotation(Quaternion);
  void resetRotation() { m_q = Quaternion(); }
  
  int vertexCount() { return m_vertices.count(); }
  int triangleCount() { return m_triangles.count()/3; }

  bool load(QString);
  void save();

  bool set(TrisetInformation);
  TrisetInformation get();

  bool fromDomElement(QDomElement);
  QDomElement domElement(QDomDocument&);

  void clear();

  void predraw(QGLViewer*, bool, double*);
  void draw(Camera*, bool);
  void postdraw(QGLViewer*,
		int, int, bool, int);

  void makeReadyForPainting(QGLViewer*);
  void releaseFromPainting();
  void paint(QGLViewer*, QBitArray, float*, Vec, float);

  float m_activeScale;
  double m_localXform[16];

private :
  bool m_show, m_clip;

  QString m_fileName;

  bool m_updateFlag;
  int m_nX, m_nY, m_nZ;
  Vec m_centroid;
  Vec m_enclosingBox[8];
  Vec m_color;
  Vec m_cropcolor;
  Vec m_position;
  Vec m_scale;
  Quaternion m_q;
  float m_roughness;
  float m_specular;
  float m_diffuse;
  float m_ambient;
  QVector<Vec> m_vertices;
  QVector<Vec> m_normals;
  QVector<uint> m_triangles;
  QVector<Vec> m_vcolor;
  QVector<Vec> m_uv;

  QVector<Vec> m_drawcolor;

  Vec m_tcentroid;
  Vec m_tenclosingBox[8];
  QList<QPolygon> m_meshInfo;
  QList<QString> m_material;
  GLuint m_diffuseTex[100];
  
  QList<char*> plyStrings;

  uint *m_scrV;
  float *m_scrD;

  GLuint m_glVertBuffer;
  GLuint m_glIndexBuffer;
  GLuint m_glVertArray;
  
  float m_featherSize;
  
  void loadVertexBufferData();
  void drawTrisetBuffer(Camera*, float, float, bool);

  bool loadTriset(QString);
  bool loadPLY(QString);
  bool loadAssimpModel(QString);
};

#endif
