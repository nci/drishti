#include "loadrawdialog.h"
#include <QtGui>
#include <QFile>

LoadRawDialog::LoadRawDialog(QWidget *parent,
			     QString flnm) :
  QDialog(parent)
{
  ui.setupUi(this);
  
  char vt[5];
  short snX, snY;
  int nX=0, nY=0, nZ=0;
  int hb=56; // 56 byte header

  memset(vt, 0, 5);

  QFile fd(flnm);
  fd.open(QFile::ReadOnly);
  fd.read(vt, 4);
  fd.read((char*)&snX, 2);
  fd.read((char*)&snY, 2);
  fd.close();

  nX = snX;
  nY = snY;

  int vidx = 5;

  ui.voxelType->setCurrentIndex(vidx);
  ui.headerBytes->setValue(hb);

  QString grid = QString("%1 %2 %3").arg(nX).arg(nY).arg(nZ);
  ui.gridSize->setText(grid);
}

int
LoadRawDialog::voxelType()
{
  return ui.voxelType->currentIndex();
}

void
LoadRawDialog::gridSize(int& nX, int& nY, int& nZ)
{
  nX = nY = nZ = 0;
  QString str = ui.gridSize->text();
  QStringList xyz = str.split(" ");
  if (xyz.size() > 0) nX = xyz[0].toInt();
  if (xyz.size() > 1) nY = xyz[1].toInt();
  if (xyz.size() > 2) nZ = xyz[2].toInt();
}

int
LoadRawDialog::skipHeaderBytes()
{
  return ui.headerBytes->value();
}
