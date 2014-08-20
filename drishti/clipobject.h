#ifndef CLIPOBJECT_H
#define CLIPOBJECT_H

#include <GL/glew.h>
#include <QGLViewer/qglviewer.h>
using namespace qglviewer;

#include <fstream>
using namespace std;

#include "commonqtclasses.h"

class ClipObjectUndo
{
 public :
  ClipObjectUndo();
  ~ClipObjectUndo();
  
  void clear();
  void append(Vec, Quaternion);

  void undo();
  void redo();

  Vec pos();
  Quaternion rot();

 private :
  int m_index;
  QList<Vec> m_pos;
  QList<Quaternion> m_rot;

  void clearTop();
};

class ClipObject
{
 public :
  ClipObject();
  ~ClipObject();
  
  bool show() { return m_show; }
  void setShow(bool s) { m_show = s; }

  float size();
  void setActive(bool);

  Vec position();
  void setPosition(Vec);

  Quaternion orientation();
  void setOrientation(Quaternion);

  Vec color();
  void setColor(Vec);

  bool solidColor();
  void setSolidColor(bool);

  float opacity();
  void setOpacity(float);

  float stereo();
  void setStereo(float);

  int tfset();
  void setTFset(int);

  bool viewportGrabbed();
  void setViewportGrabbed(bool);

  QVector4D viewport();
  void setViewport(QVector4D);

  bool viewportType();
  void setViewportType(bool);

  float viewportScale();
  void setViewportScale(float);

  int thickness();
  void setThickness(int);

  bool showThickness();
  void setShowThickness(bool);

  bool showSlice();
  void setShowSlice(bool);

  bool showOtherSlice();
  void setShowOtherSlice(bool);

  bool apply();
  void setApply(bool);

  Vec m_tang, m_xaxis, m_yaxis;
  double m_xform[16];

  void translate(Vec);
  void rotate(Vec, float);

  void normalize();

  bool flip();
  void setFlip(bool);

  float tscale1();
  float tscale2();
  float scale1();
  float scale2();
  void setScale1(float);
  void setScale2(float);

  QString captionText();
  QFont captionFont();
  QColor captionColor();
  QColor captionHaloColor();

  void setCaption(QString, QFont, QColor, QColor);

  QString imageName();
  int imageFrame();
  void setImage(QString, int);

  void setGridX(int);
  void setGridY(int);
  int gridX();
  int gridY();

  void draw(QGLViewer*, bool, float);
  void postdraw(QGLViewer*, int, int, bool);

  bool keyPressEvent(QKeyEvent*);

  bool mopClip();
  bool reorientCamera();
  bool saveSliceImage();
  bool resliceVolume();  
  int resliceSubsample();
  int resliceTag();
  
  void setXform(double xf[16]) { memcpy(m_xform, xf, 16*sizeof(double)); }

  enum MoveAxis
  {
    MoveX0,
    MoveX1,
    MoveY0,
    MoveY1,
    MoveZ,
    MoveAll
  };

  int moveAxis();
  void setMoveAxis(int);


 private :
  ClipObjectUndo m_undo;

  bool m_mopClip;
  bool m_reorientCamera;
  bool m_saveSliceImage, m_resliceVolume;
  int m_resliceSubsample, m_resliceTag;

  bool m_solidColor;
  bool m_show;
  Vec m_color;
  float m_opacity;
  float m_stereo;
  Vec m_position;
  float m_angle;
  int m_tfset;
  QVector4D m_viewport;
  bool m_viewportType;
  int m_thickness;
  float m_viewportScale;
  bool m_viewportGrabbed;
  bool m_showSlice;
  bool m_showThickness;
  bool m_showOtherSlice;
  bool m_apply;

  Quaternion m_quaternion;

  int m_moveAxis;
  
  float m_size, m_scale1, m_scale2;
  float m_tscale1, m_tscale2;

  bool m_applyOpacity, m_applyFlip;  
  bool m_active;
  
  GLuint m_displaylist;
  GLuint m_imageTex;

  bool m_imagePresent;
  QString m_imageName;
  int m_imageFrame;

  bool m_captionPresent;
  QString m_captionText;
  QFont m_captionFont;
  QColor m_captionColor;
  QColor m_captionHaloColor;

  int m_gridX, m_gridY;

  int m_textureHeight;
  int m_textureWidth;

  void drawLines(QGLViewer*, bool);
  void computeTangents(Vec);

  void clearCaption();
  void loadCaption();
  void loadCaption(QString, QFont, QColor, QColor);
  void drawCaptionImage();
  void drawGrid();

  void clearImage();
  void loadImage();
  void loadImage(QString, int);

  bool commandEditor();
  bool processCommand(QString);
  void computeTscale();

  void applyUndo();
  void applyRedo();
  void undoParameters();
};

#endif
