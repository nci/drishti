#include "globalwidget.h"

#include <QMessageBox>

GlobalWidget::GlobalWidget(QWidget *parent) :
  QWidget(parent)
{
  ui.setupUi(this);

  // disabling shadowbox
  ui.shadowBox->hide();
}

void
GlobalWidget::on_bgColor_pressed()
{
  emit bgColor();
}

void
GlobalWidget::setShadowBox(bool b)
{
  ui.shadowBox->setChecked(b);
}

void
GlobalWidget::on_shadowBox_clicked(bool b)
{
  emit shadowBox(b);
}

void
GlobalWidget::addWidget(QWidget* w)
{
  ui.groupBox->layout()->addWidget(w);
}
