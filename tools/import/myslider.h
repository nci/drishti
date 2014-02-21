#ifndef MYSLIDER_H
#define MYSLIDER_H

#include <QWidget>
#include "commonqtclasses.h"

class MySlider : public QWidget
{
  Q_OBJECT

 public :
  MySlider(QWidget *parent=NULL);

  void set(int, int, int, int, int);
  void setRange(int, int);
  void setUserRange(int, int);
  void setValue(int);
  
  void paintEvent(QPaintEvent*);
  void resizeEvent(QResizeEvent*);

  void keyPressEvent(QKeyEvent*);
  void mousePressEvent(QMouseEvent*);
  void mouseMoveEvent(QMouseEvent*);
  void mouseReleaseEvent(QMouseEvent*);
  void wheelEvent(QWheelEvent*);

  int value();
  void userRange(int&, int&);
  void range(int&, int&);

 signals :
  void valueChanged(int);
  void userRangeChanged(int, int);

 private :
  int m_rangeMin, m_rangeMax;
  int m_userMin, m_userMax;
  int m_value;
  bool m_keepGrabbingValue;
  bool m_keepGrabbingUserMin;
  bool m_keepGrabbingUserMax;

  int m_range;
  int m_baseX, m_baseY;
  int m_height;

  bool checkSlider(int, int);
  void drawSlider(QPainter*);
  void drawBoundText(QPainter*,
		     QFont,
		     QString,
		     int, int);
  void drawSliderText(QPainter*,
		      QFont,
		      QString,
		      int, int);
  bool updateValue(int, int);
  bool updateUserMin(int, int);
  bool updateUserMax(int, int);
};

#endif
