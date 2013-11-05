#include <QtGui>

#include "opacityeditor.h"

OpacityEditor::OpacityEditor(QWidget *widget) : QDoubleSpinBox(widget)
{
  setRange(0.0, 1.0);
  setSingleStep(0.1);
  setValue(0.4);
}

double OpacityEditor::opacity() const
{
  double v = value();
  // this is done to keep one digit after decimal.
  //v = (int)v + (int)((v-(int)v)*10)*0.1;
  return v;
}

void OpacityEditor::setOpacity(double v)
{
  setValue(v);
}
