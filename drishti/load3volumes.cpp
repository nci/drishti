#include <QtGui>
#include "load3volumes.h"
#include "global.h"

QStringList Load3Volumes::volume1Files() { return m_vol1Files; }
QStringList Load3Volumes::volume2Files() { return m_vol2Files; }
QStringList Load3Volumes::volume3Files() { return m_vol3Files; }

Load3Volumes::Load3Volumes(QWidget *parent) :
  QDialog(parent)
{
  ui.setupUi(this);

  ui.volList1->clear();
  ui.volList2->clear();
  ui.volList3->clear();
}

void
Load3Volumes::on_volButton1_pressed()
{
  QStringList flnm;
#ifndef Q_OS_MACX
  flnm = QFileDialog::getOpenFileNames(0,
				       "Load volume files",
				       Global::previousDirectory(),
				       "NetCDF Files (*.pvl.nc)",
				       0,
				       QFileDialog::DontUseNativeDialog);
#else
  flnm = QFileDialog::getOpenFileNames(0,
				       "Load volume files",
				       Global::previousDirectory(),
				       "NetCDF Files (*.pvl.nc)",
				       0);
#endif


  if (flnm.isEmpty())
    return;

  QFileInfo f(flnm[0]);
  Global::setPreviousDirectory(f.absolutePath());

  m_vol1Files = flnm;

  ui.volList1->clear();
  ui.volList1->addItems(flnm);
}

void
Load3Volumes::on_volButton2_pressed()
{
  QStringList flnm;
#ifndef Q_OS_MACX
  flnm = QFileDialog::getOpenFileNames(0,
				       "Load volume files",
				       Global::previousDirectory(),
				       "NetCDF Files (*.pvl.nc)",
				       0,
				       QFileDialog::DontUseNativeDialog);
#else
  flnm = QFileDialog::getOpenFileNames(0,
				       "Load volume files",
				       Global::previousDirectory(),
				       "NetCDF Files (*.pvl.nc)",
				       0);
#endif


  if (flnm.isEmpty())
    return;

  QFileInfo f(flnm[0]);
  Global::setPreviousDirectory(f.absolutePath());

  m_vol2Files = flnm;

  ui.volList2->clear();
  ui.volList2->addItems(flnm);
}

void
Load3Volumes::on_volButton3_pressed()
{
  QStringList flnm;
#ifndef Q_OS_MACX
  flnm = QFileDialog::getOpenFileNames(0,
				       "Load volume files",
				       Global::previousDirectory(),
				       "NetCDF Files (*.pvl.nc)",
				       0,
				       QFileDialog::DontUseNativeDialog);
#else
  flnm = QFileDialog::getOpenFileNames(0,
				       "Load volume files",
				       Global::previousDirectory(),
				       "NetCDF Files (*.pvl.nc)",
				       0);
#endif


  if (flnm.isEmpty())
    return;

  QFileInfo f(flnm[0]);
  Global::setPreviousDirectory(f.absolutePath());

  m_vol3Files = flnm;

  ui.volList3->clear();
  ui.volList3->addItems(flnm);
}
