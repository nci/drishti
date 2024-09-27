#include "globalwidget.h"
#include "staticfunctions.h"
#include "global.h"

#include <QMessageBox>

GlobalWidget::GlobalWidget(QWidget *parent) :
  QWidget(parent)
{
  ui.setupUi(this);

  // disabling shadowbox
  ui.shadowBox->hide();


  int vunit = Global::voxelUnit();
  ui.voxelUnit->setCurrentIndex(vunit);

  Vec vsize = Global::voxelSize();
  QString vtxt = QString("%1 %2 %3"). 
                   arg(vsize.x).arg(vsize.y).arg(vsize.z);
  ui.voxelSize->setText(vtxt);
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

void
GlobalWidget:: on_voxelUnit_currentIndexChanged(int i)
{
  Global::setVoxelUnit(i);
  emit newVoxelUnit();
}


void
GlobalWidget::on_voxelSize_editingFinished()
{
  Vec vsize;
  vsize = StaticFunctions::getVec(ui.voxelSize->text());

  if (vsize.x <= 0 || vsize.x <= 0 || vsize.x <= 0)
    {
      QMessageBox::critical(0, "Voxel Size Error",
			    QString("Voxel size <= 0 not allowed\nDefaulting to 1 1 1"),
			    QString("%1 %2 %3").arg(vsize.x).arg(vsize.y).arg(vsize.z));
      vsize.x = vsize.y = vsize.z = 1;
    }

  QString vtxt = QString("%1 %2 %3"). 
                   arg(vsize.x).arg(vsize.y).arg(vsize.z);
  ui.voxelSize->setText(vtxt);


  Global::setVoxelSize(vsize);
  emit newVoxelSize();
}
