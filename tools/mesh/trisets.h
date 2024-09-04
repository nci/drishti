#ifndef TRISETiS_H
#define TRISETS_H


#include <GL/glew.h>

#include "trisetgrabber.h"
#include "hitpoints.h"

#include <QVector3D>
#include <QVector4D>
#include <QDialog>


class Trisets : public QObject
{
  Q_OBJECT

 public :
  Trisets();
  ~Trisets();

  int count() { return m_trisets.count(); }

  void setHitPoints(HitPoints* h) { m_hitpoints = h; }
  
  bool show(int);
  void show();
  void hide();

  bool clip(int);
  bool clearView(int);
  
  QString filename(int);
  
  void setLighting(QVector4D);
  
  void allEnclosingBox(Vec&, Vec&);
  void allGridSize(int&, int&, int&);

  bool isInMouseGrabberPool(int);
  void addInMouseGrabberPool();
  void addInMouseGrabberPool(int);
  void removeFromMouseGrabberPool();
  void removeFromMouseGrabberPool(int);

  bool grabsMouse();

  void clear();

  void addTriset(QString);
  void addMesh(QString);

  
  void predraw(double*);
  void draw(QGLViewer*,
	    GLdouble*, GLfloat*, Vec,
	    GLdouble*, Vec, Vec,
	    int, int,
	    QList<Vec>, QList<Vec>);
  void postdraw(QGLViewer*);

  bool keyPressEvent(QKeyEvent*);

  QList<TrisetInformation> get();
  void set(QList<TrisetInformation>);

  void makeReadyForPainting();
  void paint(Vec, float, Vec, int, float, int, int);

  void resize(int, int);

  void setEdges(float e) { m_edges = e; }
  void setSoftness(float e) { m_blur = e; }

  void setShapeEnhancements(float, float, float, float, float);

  void checkHitPointsHover(QGLViewer*);
  void checkMouseHover(QGLViewer*);

  void setLightDirection(Vec);
  Vec lightDirection() { return m_lightDir; }
  QStringList getMeshList();

  void setExplode(float, Vec, Vec);
  void setExplode(QList<int>, float, Vec, Vec);

  bool grabMode() { return m_grab; }

  bool addHitPoint(Vec);
  void drawHitPoints();
  void removeHoveredHitPoint();

 public slots :
  void setShow(int, bool);
  void setShow(QList<bool>);
  void setClip(int, bool);
  void setClip(QList<bool>);
  void setClearView(int, bool);
  void setClearView(QList<bool>);
  void setActive(int, bool);
  void removeMesh(int);
  void removeMesh(QList<int>);
  void saveMesh(int);
  void duplicateMesh(int);
  void positionChanged(QVector3D);
  void rotationChanged(QVector4D);
  void scaleChanged(QVector3D);
  void colorChanged(QColor);
  void colorChanged(QList<int>, QColor);
  void bakeColors(QList<int>);
  void materialChanged(int);
  void materialChanged(QList<int>, int);
  void materialMixChanged(float);
  void materialMixChanged(QList<int>, float);
  void transparencyChanged(int);
  void revealChanged(int);
  void outlineChanged(int);
  void glowChanged(int);
  void darkenChanged(int);
  void sendParametersToMenu();
  void processCommand(QString);
  void processCommand(int, QString);
  void processCommand(QList<int>, QString);
  void setGrab(bool);
  void meshGrabbed();
  void multiSelection(QList<int>);
  void posChanged();
  void selectionWindow(Camera*, QRect);
  void pointProbe(Vec, float);
  void probeMeshMove(Vec);
  void resetProbe();

  void deleteDialog(int);
  void applyVertexColorSmoothing(int);
  
 signals :
  void updateGL();
  void updateScaling();
  void resetBoundingBox();
  void updateMeshList(QStringList);
  void setParameters(QMap<QString, QVariantList>);
  void removeAllKeyFrames();
  void clearScene();
  void clearSelection();
  void meshGrabbed(int);
  void meshesGrabbed(QList<int>);
  void probeMeshName(QString);
  void matcapFiles(QStringList);
  
 private :
  HitPoints *m_hitpoints;
  
  QList<TrisetGrabber*> m_trisets;

  int m_active;
  QList<int> m_multiActive;
  Vec m_prevActivePos;
  bool m_grab;
  
  GLfloat m_mvpShadow[16];
  Vec m_lightDir;
  
  int m_nclip;
  float *m_cpos;
  float *m_cnormal;

  GLuint m_vertexScreenBuffer;
  float m_scrGeo[8];

  GLuint m_depthBuffer;
  GLuint m_rbo;
  GLuint m_depthTex[8];
  
  
  float m_blur, m_edges;
  float m_shadowIntensity, m_valleyIntensity, m_peakIntensity;

  Vec m_probeStartPos;
  
  void createFBO(int, int);

  void render(GLdouble*, Vec, int, int, int, bool, bool);


  QStringList m_solidTexName;
  QList<uchar*> m_solidTexData;
  GLuint* m_solidTex;
  void loadMatCapTextures();


  void renderFromCamera(GLdouble*, Vec, int, int, int);
  void renderFromShadowCamera(GLdouble*, Vec, int, int, int);
  void renderShadows(GLint, int, int, Vec, float, Vec);
  void renderShadowBox(GLint, int, int, Vec, float, Vec);
  void renderOutline(GLint, GLdouble*, Vec, int, int, int, float);
  void renderGrabbedOutline(GLint, GLdouble*, Vec, int, int);
  void renderTransparent(GLint, GLdouble*, GLfloat*, Vec, Vec, int, int, int, float);
  void bindOITTextures();
  void drawOITTextures(int, int);

  void askGradientChoice(QList<int>);

  void mergeSurfaces(QList<int>);
  void saveHitPoints();
  void saveHitPoints(int);
  void loadHitPoints();
  void loadHitPoints(int);


  void renderFromCameraClearView(GLdouble*, Vec, int, int, int);
  void dilateClearViewBuffer(QGLViewer*, int, int);
  bool m_renderingClearView;

  QDialog *m_dialog;
  void smoothVertexColors(int);
};


#endif
