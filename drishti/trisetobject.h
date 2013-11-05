#ifndef TRISETOBJECT_H
#define TRISETOBJECT_H

#include <QFile>

#include "trisetinformation.h"

class TrisetObject
{
 public :
  TrisetObject();
  ~TrisetObject();

  void gridSize(int&, int&, int&);

  Vec centroid() { return m_tcentroid; }
  void enclosingBox(Vec&, Vec&);

  QString filename() { return m_fileName; }

  bool screenDoor() { return m_screenDoor; }
  void setScreenDoor(bool sd) {m_screenDoor = sd; }

  bool flipNormals() { return m_flipNormals; }
  void setFlipNormals(bool fn) { m_flipNormals = fn; }

  bool pointMode() { return m_pointMode; }
  void setPointMode(bool pm) {m_pointMode = pm; }

  bool blendMode() { return m_blendMode; }
  void setBlendMode(bool pm) {m_blendMode = pm; }

  bool shadows() { return m_shadows; }
  void setShadows(bool cs) {m_shadows = cs; }

  int pointsize() { return m_pointSize; }
  void setPointSize(int ps) { m_pointSize = qMax(1, ps); }

  int pointstep() { return m_pointStep; }
  void setPointStep(int ps) { m_pointStep = qMax(1, ps); }

  float opacity() { return m_opacity; }
  void setOpacity(float);

  float specular() { return m_specular; }
  void setSpecular(float shine) { m_specular = qBound(0.0f, shine, 1.0f); }

  float diffuse() { return m_diffuse; }
  void setDiffuse(float diffuse) { m_diffuse = qBound(0.0f, diffuse, 1.0f); }

  float ambient() { return m_ambient; }
  void setAmbient(float ambient) { m_ambient = qBound(0.0f, ambient, 1.0f); }

  Vec color() { return m_color; }
  void setColor(Vec color) { m_color = color; }

  Vec cropBorderColor() { return m_cropcolor; }
  void setCropBorderColor(Vec color) { m_cropcolor = color; }

  Vec position() { return m_position; }
  void setPosition(Vec pos) { m_position = pos; }

  Vec scale() { return m_scale; }
  void setScale(Vec scl) { m_scale = scl; }

  int vertexCount() { return m_vertices.count(); }
  int triangleCount() { return m_triangles.count()/3; }

  bool load(QString);
  void save();

  bool set(TrisetInformation);
  TrisetInformation get();

  bool fromDomElement(QDomElement);
  QDomElement domElement(QDomDocument&);

  void clear();

  void predraw(QGLViewer*,
	       double*,
	       Vec,
	       bool, int, int);
  void draw(QGLViewer*,
	    bool,
	    Vec,
	    float, float, Vec);
  void postdraw(QGLViewer*,
		int, int, bool, int);

  void makeReadyForPainting(QGLViewer*);
  void releaseFromPainting();
  void paint(QGLViewer*, QBitArray, float*, Vec, float);

 private :
  QString m_fileName;

  bool m_updateFlag;
  int m_nX, m_nY, m_nZ;
  Vec m_centroid;
  Vec m_enclosingBox[8];
  bool m_shadows;
  bool m_pointMode;
  bool m_blendMode;
  bool m_screenDoor;
  bool m_flipNormals;
  int m_pointStep;
  Vec m_color;
  Vec m_cropcolor;
  Vec m_position;
  Vec m_scale;
  float m_opacity;
  float m_specular;
  float m_diffuse;
  float m_ambient;
  int m_pointSize;
  QVector<Vec> m_vertices;
  QVector<Vec> m_normals;
  QVector<uint> m_triangles;
  QVector<Vec> m_vcolor;
  QVector<Vec> m_drawcolor;

  Vec m_tcentroid;
  Vec m_tenclosingBox[8];
  QVector<Vec> m_tvertices;
  QVector<Vec> m_tnormals;
  QVector<Vec> m_texValues;

  uint *m_scrV;
  float *m_scrD;

  void drawTriset(float, float, Vec);
  void drawTriset();

  bool loadTriset(QString);
  bool loadPLY(QString);
};

#endif
