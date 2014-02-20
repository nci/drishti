#ifndef TRANSFERFUNCTIONEDITORWIDGET_H
#define TRANSFERFUNCTIONEDITORWIDGET_H

#include "splineeditorwidget.h"
#include "gradienteditorwidget.h"

#include <QSplitter>
#include <QVBoxLayout>

class TransferFunctionEditorWidget : public QSplitter
{
  Q_OBJECT
  
 public :
  TransferFunctionEditorWidget(QWidget *parent=0);
  ~TransferFunctionEditorWidget();

  void setTransferFunction(SplineTransferFunction*);

 public  slots :
  void transferFunctionChanged(QImage);
  void setMapping(QPolygonF);
  void setHistogramImage(QImage, QImage);
  void setHistogram2D(int*);
  void changeHistogram(int);
  void changeVol(int);

 signals :
  void transferFunctionUpdated();
  void updateComposite();
  void giveHistogram(int);
  void applyUndo(bool);

 private :
  QWidget *m_parent;
  SplineEditorWidget *m_splineEditorWidget;
  GradientEditorWidget *m_gradientEditorWidget;
  
  QVBoxLayout *m_layout;

};
#endif
