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
  void vol1Clicked(bool);
  void vol2Clicked(bool);
  void vol3Clicked(bool);
  void vol4Clicked(bool);
  void changeVol(int);
  void gValueChanged();
  void gtopSliderReleased();
  void gbotSliderReleased();

 signals :
  void transferFunctionChanged(QImage);
  void selectEvent(QGradientStops);
  void deselectEvent();
  void switchHistogram();
  void giveHistogram(int);
  void applyUndo(bool);

 private :
  QWidget *m_parent;
  SplineEditor *m_splineEditor;
  RemapHistogramWidget *m_16bitEditor;
  QRadioButton *m_1dHist, *m_2dHist;
  QRadioButton *m_vol1, *m_vol2;
  QRadioButton *m_vol3, *m_vol4;
  QLineEdit *m_gtopValue, *m_gbotValue;
  QSlider *m_gtopSlider, *m_gbotSlider;
};

#endif
