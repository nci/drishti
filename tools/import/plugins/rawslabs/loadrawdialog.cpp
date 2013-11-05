#include "loadrawdialog.h"
#include <QtGui>
#include <QFile>

LoadRawDialog::LoadRawDialog(QWidget *parent,
			     QString flnm) :
  QDialog(parent)
{
  ui.setupUi(this);
  
  uchar vt=0;
  int nX=0, nY=0, nZ=0;
  int hb=0;


  QFile fd(flnm);
  fd.open(QFile::ReadOnly);
  fd.read((char*)&vt, sizeof(unsigned char));
  fd.read((char*)&nX, sizeof(int));
  fd.read((char*)&nY, sizeof(int));
  fd.read((char*)&nZ, sizeof(int));
  fd.close();

  int vidx = 0;
  if (vt == 0) vidx = 0;
  if (vt == 1) vidx = 1;
  if (vt == 2) vidx = 2;
  if (vt == 3) vidx = 3;
  if (vt == 4) vidx = 4;
  if (vt == 8) vidx = 5;

  if (vt == '0') vidx = 0;
  if (vt == '1') vidx = 1;
  if (vt == '2') vidx = 2;
  if (vt == '3') vidx = 3;
  if (vt == '4') vidx = 4;
  if (vt == '8') vidx = 5;

//  if (nX<=0 || nY<=0 || nZ<=0 ||
//      nX >5000 || nY >5000 || nZ > 5000)
//    {
//      nX = nY = nZ = 0;
//    }


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
