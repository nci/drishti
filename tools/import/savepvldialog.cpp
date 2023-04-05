#include "savepvldialog.h"
#include <QMessageBox>

SavePvlDialog::SavePvlDialog(QWidget *parent) :
  QDialog(parent)
{
  ui.setupUi(this);
}

void
SavePvlDialog::setVoxelUnit(int vu)
{
  ui.voxelUnit->setCurrentIndex(vu);
}

void
SavePvlDialog::setVoxelSize(float vx, float vy, float vz)
{
  if (vx <=0 || vy<=0 || vz <= 0)
    {
      QMessageBox::critical(0, "Voxel Size Error",
			    QString("Voxel size <= 0 not allowednDefaulting to 1 1 1"),
			    QString("%1 %2 %3").arg(vx).arg(vy).arg(vz));
      ui.voxelSize->setText("1 1 1");
    }
  else
    ui.voxelSize->setText(QString("%1 %2 %3").arg(vx).arg(vy).arg(vz));
}

void
SavePvlDialog::setDescription(QString txt)
{
  ui.description->setText(txt);
}

int SavePvlDialog::voxelUnit() { return ui.voxelUnit->currentIndex(); }
int SavePvlDialog::volumeFilter() { return ui.volumeFilter->currentIndex(); }
QString SavePvlDialog::description() { return ui.description->text(); }
bool SavePvlDialog::dilateFilter() { return ui.dilateFilter->isChecked(); }
bool SavePvlDialog::invertData() { return ui.invert->isChecked(); }

void
SavePvlDialog::voxelSize(float& vx, float& vy, float& vz)
{
  vx = vy = vz = 1;
  QString str = ui.voxelSize->text();
  QStringList xyz = str.split(" ");
  if (xyz.size() > 0) vx = xyz[0].toFloat();
  if (xyz.size() > 1) vy = xyz[1].toFloat();
  if (xyz.size() > 2) vz = xyz[2].toFloat();

  if (vx <=0 || vy<=0 || vz <= 0)
    {
      QMessageBox::critical(0, "Voxel Size Error",
			    "Voxel size <= 0 not allowed\nDefaulting to 1 1 1",
			    str);
      vx = vy = vz = 1;
    }
}
