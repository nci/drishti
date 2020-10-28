#ifndef TRISETOBJECT_H
#define TRISETOBJECT_H

#include <QFile>

#include "trisetinformation.h"
#include <QVector4D>
#include "captionobject.h"

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

  float opacity() { return m_opacity; }
  void setOpacity(float o) { m_opacity = o; }
  
  Vec color() { return m_color; }
  void setColor(Vec);

  Vec cropBorderColor() { return m_cropcolor; }
  void setCropBorderColor(Vec color) { m_cropcolor = color; }

  Vec position() { return m_position; }
  void setPosition(Vec pos) { m_position = pos; }

  Vec scale() { return m_scale; }
  void setScale(Vec scl) { m_scale = scl; }

  float activeScale() { return m_activeScale; }
  void setActiveScale(float s) { m_activeScale = s; }

  float reveal() { return m_reveal; }
  void setReveal(float r) { m_reveal = r; }
  
  float outline() { return m_outline; }
  void setOutline(float r) { m_outline = r; }
  
  float glow() { return m_glow; }
  void setGlow(float r) { m_glow = r; }
  
  float dark() { return m_dark; }
  void setDark(float r) { m_dark = r; }

  Vec pattern() { return m_pattern; }
  void setPattern(Vec p) { m_pattern = p; }
  
  void setCaptionPosition(Vec);
  void setCaptionOffset(int dx, int dy) { m_cpDx = dx; m_cpDy = dy; }
  QFont captionFont() { return m_captionFont; }
  void setCaptionFont(QFont);
  QColor captionColor() { return m_captionColor; }
  void setCaptionColor(QColor);
  QString captionText() { return m_captionText; }
  void setCaptionText(QString);
  
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


  void clear();

  void predraw(QGLViewer*, bool, double*);
  void draw(Camera*, bool);
  void drawOIT(Camera*, bool);
  void postdraw(QGLViewer*,
		int, int, bool, int);

  void makeReadyForPainting(QGLViewer*);
  void releaseFromPainting();
  void paint(QGLViewer*, QBitArray, float*, Vec, float);

  void mirror(int);
  
  float m_activeScale;
  double m_localXform[16];

  QList<Vec> m_samplePoints;
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
  float m_reveal;
  float m_outline;
  float m_glow;
  float m_dark;
  Vec m_pattern;
  Quaternion m_q;
  float m_roughness;
  float m_specular;
  float m_diffuse;
  float m_ambient;
  float m_opacity;
  QVector<Vec> m_vertices;
  QVector<Vec> m_normals;
  QVector<uint> m_triangles;
  QVector<Vec> m_vcolor;
  QVector<Vec> m_uv;

  QVector<Vec> m_drawcolor;

  double m_brick0Xform[16];
    
  Vec m_tcentroid;
  Vec m_tenclosingBox[8];
  QList<QPolygon> m_meshInfo;
  QList<QString> m_material;
  GLuint m_diffuseTex[100];
  
  QList<char*> plyStrings;

  uint *m_scrV;
  float *m_scrD;

  QString m_captionText;
  Vec m_captionPosition;
  int m_cpDx, m_cpDy;
  QFont m_captionFont;
  QColor m_captionColor;
  CaptionObject *m_co;
  
  GLuint m_glVertBuffer;
  GLuint m_glIndexBuffer;
  GLuint m_glVertArray;
  
  float m_featherSize;
  
  void loadVertexBufferData();
  void drawTrisetBuffer(Camera*, float, float, bool,
			GLuint, GLint*, bool);

  bool loadTriset(QString);
  bool loadPLY(QString);
  bool loadAssimpModel(QString);

  void drawCaption(QGLViewer*);

  void genLocalXform();
};

#endif
