#ifndef SPLINEEDITORWIDGET_H
#define SPLINEEDITORWIDGET_H

#include "splineeditor.h"
#include "remaphistogramwidget.h"

#include <QRadioButton>
#include <QLineEdit>
#include <QSlider>

class SplineEditorWidget : public QWidget
{
  Q_OBJECT

 public :
  SplineEditorWidget(QWidget *parent=0);

  void paintEvent(QPaintEvent*);

  void setMapping(QPolygonF);
  void setHistogramImage(QImage, QImage);
  void setHistogram2D(int*);
  void setTransferFunction(SplineTransferFunction*);

 public slots :
  void setGradientStops(QGradientStops);
  void updateTransferFunction();
  void updateTransferFunction(float, float);
  void selectSpineEvent(QGradientStops);
  void deselectSpineEvent();
  void hist1DClicked(bool);
  void hist2DClicked(bool);

 signals :
  void transferFunctionChanged(QImage);
  void selectEvent(QGradientStops);
  void deselectEvent();
  void switchHistogram();

 private :
  QWidget *m_parent;
  SplineEditor *m_splineEditor;
  RemapHistogramWidget *m_16bitEditor;
  QRadioButton *m_1dHist, *m_2dHist;
};

#endif
