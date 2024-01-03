#ifndef DCOLORWHEEL_H
#define DCOLORWHEEL_H

#include <QWidget>
#include <QPushButton>

#include "commonqtclasses.h"

class DColorWheel : public QWidget
{
  Q_OBJECT

 public :
  DColorWheel(QWidget*, int);

  void paintEvent(QPaintEvent*);
  void resizeEvent(QResizeEvent*);
  bool eventFilter(QObject*, QEvent*);  
  void wheelEvent(QWheelEvent*);

  void setColor(QColor);
  QColor getColor();

  void setColorGridSize(int);
  int getColorGridSize() { return m_colorGridSize; }
 
 public slots :
  void moreShades();
  void lessShades();

 signals :
  void colorChanged(QColor);

 private :
  QWidget *m_parent;

  int m_margin;
  int m_colorGridSize;
  
  QPushButton *m_more, *m_less;

  QRectF m_wheelSize;
  QRectF m_innerWheelSize;
  QRectF m_innerRect;
  QRectF m_hueMarker;
  QPointF m_wheelCenter;

  uchar m_currFlag;

  float m_hue, m_saturation, m_value;

  void calculateHsv(QPointF);
  void setButtonColors();
};

#endif
