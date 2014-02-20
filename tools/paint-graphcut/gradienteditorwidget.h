#ifndef GRADIENTWIDGET_H
#define GRADIENTWIDGET_H

#include "gradienteditor.h"

class GradientEditorWidget : public QWidget
{
  Q_OBJECT

 public :
  GradientEditorWidget(QWidget *parent=0);

  void paintEvent(QPaintEvent*);
  void resizeEvent(QResizeEvent*);

 public slots:
  void setColorGradient(QGradientStops);
  void setGeneralLock(GradientEditor::LockType);
  void setDrawBox(bool);

 signals :
  void gradientChanged(QGradientStops);


 private slots:
  void updateColorGradient();

 private :
  void generateShade();

  bool m_drawBox;

  QPixmap m_backgroundPixmap;
  QImage m_shade;

  QWidget *m_parent;
  GradientEditor *m_gradientEditor;
};

#endif
