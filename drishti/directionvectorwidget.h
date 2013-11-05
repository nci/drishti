#ifndef DIRECTIONVECTORWIDGET_H
#define DIRECTIONVECTORWIDGET_H

#include "ui_directionvectorwidget.h"

#include "lightdisc.h"

#include <QGLViewer/vec.h>
using namespace qglviewer;

class DirectionVectorWidget : public QWidget
{
 Q_OBJECT

 public :
  DirectionVectorWidget(QWidget *parent=NULL);

  void setDistance(float);
  void setVector(Vec);
  void setRange(float, float, float);
  Vec vector();

 signals :
  void directionChanged(float, Vec);

 private slots :
  void updateDirection(QPointF);
  void on_reverse_clicked(bool);
  void on_distance_valueChanged(double);

 private :
  Ui::DirectionVectorWidget ui;

  LightDisc *m_lightdisc;
  Vec m_direction;
};


#endif
