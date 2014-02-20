#ifndef LIGHTDISC_H
#define LIGHTDISC_H

#include <QWidget>
#include "commonqtclasses.h"

class LightDisc : public QWidget
{
  Q_OBJECT

 public :
  LightDisc(QWidget *parent = NULL);

  void setBacklit(bool);
  bool backlit();
  QPointF direction();

  virtual QSize sizeHint() const;

 public slots:
  void setDirection(bool, QPointF);

  void mousePressEvent(QMouseEvent*);
  void mouseMoveEvent(QMouseEvent*);
  void paintEvent(QPaintEvent*);
  void resizeEvent(QResizeEvent*);
  void keyPressEvent(QKeyEvent*);
  void enterEvent(QEvent*);
  void leaveEvent(QEvent*);

 signals :
  void directionChanged(QPointF);

 private :
  QWidget *m_parent;
  
  QPointF m_direction;
  bool m_backlit;
  float m_size;
  QRectF m_wheelSize;
  QPointF m_wheelCenter;
  QPointF m_mousePos;
  bool m_mousePressed;
  
  void calculateDirection();
};

#endif
