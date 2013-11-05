#ifndef TRANSFERFUNCTIONEDITORWIDGET_H
#define TRANSFERFUNCTIONEDITORWIDGET_H

#include "splineeditorwidget.h"
#include "gradienteditorwidget.h"

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

 signals :
  void transferFunctionUpdated(QImage);
  void updateComposite();

 private :
  QWidget *m_parent;
  SplineEditorWidget *m_splineEditorWidget;
  GradientEditorWidget *m_gradientEditorWidget;
  
  QVBoxLayout *m_layout;

};
#endif
