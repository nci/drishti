#ifndef REMAPCONTAINER_H
#define REMAPCONTAINER_H

#include "remaprawvolume.h"
#include "remaphistogramwidget.h"

class RemapContainer : public QWidget
{
  Q_OBJECT

 public :
  RemapContainer(QWidget *parent=NULL);

  void keyPressEvent(QKeyEvent*);
  void mousePressEvent(QMouseEvent*);
  void mouseMoveEvent(QMouseEvent*);
  void mouseReleaseEvent(QMouseEvent*);
  void wheelEvent(QWheelEvent*);

 public slots :
  void getHistogram();  
  void setRawMinMax();

 private :
  RemapRawVolume m_rawVolume;
  RemapHistogramWidget *m_histogramWidget;
};

#endif
