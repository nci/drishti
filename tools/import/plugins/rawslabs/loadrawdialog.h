#ifndef LOADRAWDIALOG_H
#define LOADRAWDIALOG_H

#include "ui_loadrawdialog.h"

class LoadRawDialog : public QDialog
{
 public :
  LoadRawDialog(QWidget *parent=NULL,
		QString flnm="");

  int voxelType();
  void gridSize(int&, int&, int&);
  int skipHeaderBytes();

 private :
  Ui::LoadRawDialog ui;
  int m_voxelType;
  int m_nX, m_nY, m_nZ;
  int m_headerBytes;
};

#endif
