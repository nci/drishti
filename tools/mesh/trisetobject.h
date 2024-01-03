#ifndef TRISETOBJECT_H
#define TRISETOBJECT_H

#include <QFile>

#include "trisetinformation.h"
#include "captionobject.h"
#include <QVector4D>
#include <QDialog>


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

  bool clearView() { return m_clearView; }
  void setClearView(bool s) { m_clearView = s; }

  Vec tcentroid();
  Vec centroid() { return m_centroid; }
  void enclosingBox(Vec&, Vec&);
  void tenclosingBox(Vec&, Vec&);
  void getAxes(Vec&, Vec&, Vec&);
  
  QString filename() { return m_fileName; }

  void setLighting(QVector4D);
  float roughness() { return m_roughness; }
  float specular() { return m_specular; }
  float diffuse() { return m_diffuse; }
  float ambient() { return m_ambient; }

  float opacity() { return m_opacity; }
  void setOpacity(float o) { m_opacity = o; }
  
  Vec color() { return m_color; }
  void setColor(Vec, bool ignoreBlack = false);
  void bakeColors();
  bool haveBlack();
  
  int material() { return m_materialId; }
  void setMaterial(int m) { m_materialId = m; }

  float materialMix() { return m_materialMix; }
  void setMaterialMix(float m) { m_materialMix = m; }
  
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
  
  int numOfLabels() { return m_co.count(); }


  QList<Vec> captionPositions();
  Vec captionPosition();
  Vec captionPosition(int);
  void setCaptionPosition(Vec);
  void setCaptionPosition(int, Vec);

  QList<Vec> captionOffsets();
  Vec captionOffset();
  Vec captionOffset(int);
  void setCaptionOffset(float, float);
  void setCaptionOffset(int, float, float);

  QFont captionFont();
  QFont captionFont(int);
  void setCaptionFont(QFont);
  void setCaptionFont(int, QFont);

  QColor captionColor();
  QColor captionColor(int);
  void setCaptionColor(QColor);
  void setCaptionColor(int, QColor);

  QString captionText();
  QString captionText(int);
  void setCaptionText(QString);
  void setCaptionText(int, QString);

  QList<Vec> captionSizes();
  Vec captionSize();

  
  Quaternion rotation() { return m_q; }
  void rotate(Vec, float);
  void rotate(Quaternion);
  void setRotation(Quaternion);
  void resetRotation() { m_q = Quaternion(); }
  
  int vertexCount() { return m_vertices.count()/3; }
  int triangleCount() { return m_triangles.count()/3; }

  QVector<float> vertices() { return m_vertices; }
  QVector<float> normals() { return m_normals; }
  QVector<float> colors() { return m_vcolor; }
  QVector<uint> triangles() { return m_triangles; }

  QVector<float> tangents() { return m_tangents; }
  
  bool load(QString);
  void save();

  bool set(TrisetInformation);
  TrisetInformation get();


  void clear();

  void predraw(bool, double*);
  void draw(GLdouble*, Vec, bool);
  void drawOIT(GLdouble*, Vec, bool);
  void postdraw(QGLViewer*,
		int, int,
		bool, bool,
		int, int, int);

  void makeReadyForPainting();
  bool paint(Vec);
  
  void mirror(int);
  
  float m_activeScale;
  double m_localXform[16];

  int m_interactive_axis;
  int m_interactive_mode;
  
  QList<Vec> m_samplePoints;


  void addHitPoint(Vec v) { m_hitpoints << v; }
  void clearHitPoints() { m_hitpoints.clear(); }
  QList<Vec> hitPoints() { return m_hitpoints; }
  void drawHitPoints();

  void copyToOrigVcolor();
  void copyFromOrigVcolor();  
  void smoothVertexColors(int);
  
private :
  bool m_show, m_clip, m_clearView;

  QString m_fileName;

  bool m_updateFlag;
  int m_nX, m_nY, m_nZ;
  Vec m_centroid;
  Vec m_enclosingBox[8];
  Vec m_color;
  int m_materialId;
  float m_materialMix;
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
  QVector<float> m_vertices;
  QVector<float> m_normals;
  QVector<uint> m_triangles;
  QVector<float> m_vcolor;
  QVector<float> m_OrigVcolor;
  QVector<float> m_uv;

  QVector<float> m_tangents;

  double m_brick0Xform[16];
  
  Vec m_tcentroid;
  Vec m_tenclosingBox[8];
  QList<QPolygon> m_meshInfo;
  QList<QString> m_diffuseMat;
  QList<QString> m_normalMat;
  GLuint m_diffuseTex[100];
  GLuint m_normalTex[100];
  
  QList<char*> plyStrings;

  
  QList<Vec> m_captionPosition;
  QList<CaptionObject*> m_co;
  
  GLuint m_glVertBuffer;
  GLuint m_glIndexBuffer;
  GLuint m_glVertArray;
  GLuint m_origColorBuffer;
  
  
  float m_featherSize;  

  QList<Vec> m_hitpoints;

  QDialog *m_dialog;
  
  
  void loadVertexBufferData();
  void drawTrisetBuffer(GLdouble*, Vec,
			float, float, bool,
			GLuint, GLint*, bool);

  bool loadTriset(QString);
  bool loadPLY(QString);
  bool loadAssimpModel(QString);
  bool loadSTLModel(QString);

  void drawCaption(int, QGLViewer*);

  void genLocalXform();

  void postdrawInteractionWidget(QGLViewer*);
};

#endif
