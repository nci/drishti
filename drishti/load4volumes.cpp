#include "global.h"
#include "load4volumes.h"

#include <QFileDialog>

QStringList Load4Volumes::volume1Files() { return m_vol1Files; }
QStringList Load4Volumes::volume2Files() { return m_vol2Files; }
QStringList Load4Volumes::volume3Files() { return m_vol3Files; }
QStringList Load4Volumes::volume4Files() { return m_vol4Files; }

Load4Volumes::Load4Volumes(QWidget *parent) :
  QDialog(parent)
{
  ui.setupUi(this);

  ui.volList1->clear();
  ui.volList2->clear();
  ui.volList3->clear();
  ui.volList4->clear();
}

void
Load4Volumes::on_volButton1_pressed()
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
Load4Volumes::on_volButton2_pressed()
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

void
Load4Volumes::on_volButton3_pressed()
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

  m_vol3Files = flnm;

  ui.volList3->clear();
  ui.volList3->addItems(flnm);
}

void
Load4Volumes::on_volButton4_pressed()
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

  m_vol4Files = flnm;

  ui.volList4->clear();
  ui.volList4->addItems(flnm);
}
