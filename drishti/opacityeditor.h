#ifndef OPACITYEDITOR_H
#define OPACITYEDITOR_H

#include <QDoubleSpinBox>

class QWidget;

class OpacityEditor : public QDoubleSpinBox
{
  Q_OBJECT
  Q_PROPERTY(double opacity READ opacity WRITE setOpacity USER true)

 public :
  OpacityEditor(QWidget *widget=0);

  double opacity() const;
  void setOpacity(double); 
};

#endif
