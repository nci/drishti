#include "global.h"
#include "load2volumes.h"
#include <QFileDialog>

QStringList Load2Volumes::volume1Files() { return m_vol1Files; }
QStringList Load2Volumes::volume2Files() { return m_vol2Files; }

Load2Volumes::Load2Volumes(QWidget *parent) :
  QDialog(parent)
{
  ui.setupUi(this);

  ui.volList1->clear();
  ui.volList2->clear();
}

void
Load2Volumes::on_volButton1_pressed()
{
  QStringList flnm;
  flnm = QFileDialog::getOpenFileNames(0,
				      "Load volume files",
				       Global::previousDirectory(),
				       "NetCDF Files (*.pvl.nc)",
				       0,
				       QFileDialog::DontUseNativeDialog);


  if (flnm.isEmpty())
    return;

  QFileInfo f(flnm[0]);
  Global::setPreviousDirectory(f.absolutePath());

  m_vol1Files = flnm;

  ui.volList1->clear();
  ui.volList1->addItems(flnm);
}

void
Load2Volumes::on_volButton2_pressed()
{
  QStringList flnm;
  flnm = QFileDialog::getOpenFileNames(0,
				       "Load volume files",
				       Global::previousDirectory(),
				       "NetCDF Files (*.pvl.nc)",
				       0,
				       QFileDialog::DontUseNativeDialog);

				       
  if (flnm.isEmpty())
    return;

  QFileInfo f(flnm[0]);
  Global::setPreviousDirectory(f.absolutePath());

  m_vol2Files = flnm;

  ui.volList2->clear();
  ui.volList2->addItems(flnm);
}
