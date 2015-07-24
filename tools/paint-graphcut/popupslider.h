#ifndef POPUPSLIDER_H
#define POPUPSLIDER_H

#include <QToolButton>

QT_FORWARD_DECLARE_CLASS(QMenu)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QSlider)

class PopUpSlider : public QToolButton
{
  Q_OBJECT
  Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged)
  
 public:
  PopUpSlider(QWidget *parent = 0,
	      Qt::Orientation sliderType = Qt::Horizontal);

  int value() const;

public slots:
  void setRange(int, int);
  void increaseValue();
  void decreaseValue();
  void setValue(int value);

signals:
  void valueChanged(int value);

private:
  QMenu *menu;
  QLabel *label;
  QSlider *slider;
};

#endif // POPUPSLIDER_H
