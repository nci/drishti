#include "popupslider.h"

#include <QtWidgets>

PopUpSlider::PopUpSlider(QWidget *parent, Qt::Orientation sliderType) :
    QToolButton(parent), menu(0), label(0), slider(0)
{
  setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed));
  setPopupMode(QToolButton::InstantPopup);
  
  QWidget *popup = new QWidget(this);
  
  slider = new QSlider(sliderType, popup);
  slider->setRange(0, 1000);
  connect(slider, SIGNAL(valueChanged(int)), this, SIGNAL(valueChanged(int)));
  connect(slider, SIGNAL(sliderReleased()), this, SLOT(emitSliderReleased()));
  
  label = new QLabel(popup);
  label->setAlignment(Qt::AlignCenter);
  label->setNum(1000);
  label->setMinimumWidth(label->sizeHint().width());
  connect(slider, SIGNAL(valueChanged(int)), label, SLOT(setNum(int)));

  QBoxLayout *popupLayout;  
  if (sliderType == Qt::Horizontal)
    popupLayout = new QHBoxLayout(popup);
  else
    popupLayout = new QVBoxLayout(popup);

  popupLayout->setMargin(2);
  popupLayout->addWidget(slider);
  popupLayout->addWidget(label);

  QWidgetAction *action = new QWidgetAction(this);
  action->setDefaultWidget(popup);
  
  menu = new QMenu(this);
  menu->addAction(action);
  setMenu(menu);
}

void PopUpSlider::setRange(int min, int max, int sz)
{
  slider->setRange(min, max);
  slider->setValue(min);
  if (slider->orientation() == Qt::Horizontal)
    slider->setMinimumWidth(sz);
  else
    slider->setMinimumHeight(sz);
  label->setNum(min);
}

void PopUpSlider::increaseValue()
{
    slider->triggerAction(QSlider::SliderPageStepAdd);
}

void PopUpSlider::decreaseValue()
{
    slider->triggerAction(QSlider::SliderPageStepSub);
}

int PopUpSlider::value() const
{
    return slider->value();
}

void PopUpSlider::setValue(int value)
{
    slider->setValue(value);
}

void PopUpSlider::emitSliderReleased()
{
  emit sliderReleased(slider->value());
}
