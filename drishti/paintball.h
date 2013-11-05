#ifndef PAINTBALL_H
#define PAINTBALL_H

#include "mymanipulatedframe.h"

class PaintBall : public QObject
{
 Q_OBJECT

 public :
  PaintBall();
  ~PaintBall();

  bool grabsMouse();

  void setBounds(Vec, Vec);

  void setShowPaintBall(bool);
  bool showPaintBall();
  
  void setPosition(Vec);
  Vec position();

  void setSize(Vec);
  Vec size();
  
  void draw();

  bool keyPressEvent(QKeyEvent*);

 signals :
  void updateGL();
  void tagVolume(int, bool);
  void fillVolume(int, bool);
  void dilateVolume(int, int, bool);
  void erodeVolume(int, int, bool);
  void tagSurface(int, int, bool, bool);
  
 private slots :
  void bound();

 private : 
  bool m_show;
  Vec m_size;

  Vec m_dataMin;
  Vec m_dataMax;

  int m_tag;
  int m_thickness;

  LocalConstraint *m_constraints;
  MyManipulatedFrame m_frame;

  MyManipulatedFrame m_bounds[6];

  void processCommand(QString);
  void setSizeBounds();

  Vec m_fpos, m_pb0, m_pb1;
};

#endif
