#ifndef COLOREDITOR_H
#define COLOREDITOR_H

#include <QLabel>

class QColor;
class QWidget;

class ColorEditor : public QLabel
{
    Q_OBJECT
    Q_PROPERTY(QColor color READ color WRITE setColor USER true)

public:
    ColorEditor(QWidget *widget = 0);

public:
    QColor color();
    void setColor(QColor c);

private:
    QColor m_color;
};

#endif
