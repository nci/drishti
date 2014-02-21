#include "savepvldialog.h"

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

void
SavePvlDialog::voxelSize(float& vx, float& vy, float& vz)
{
  vx = vy = vz = 1;
  QString str = ui.voxelSize->text();
  QStringList xyz = str.split(" ");
  if (xyz.size() > 0) vx = xyz[0].toFloat();
  if (xyz.size() > 1) vy = xyz[1].toFloat();
  if (xyz.size() > 2) vz = xyz[2].toFloat();
}
