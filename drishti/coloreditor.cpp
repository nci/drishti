#include <QtGui>
#include "dcolordialog.h"

#include "coloreditor.h"

ColorEditor::ColorEditor(QWidget *widget) : QLabel(widget)
{
  m_color = Qt::white;
}

QColor ColorEditor::color()
{
  QColor clr = DColorDialog::getColor(m_color);

  if (! clr.isValid())
    clr = m_color;

  return clr;
}

void ColorEditor::setColor(QColor clr)
{  
  m_color = clr;
}
