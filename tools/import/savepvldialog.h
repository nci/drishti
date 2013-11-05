#ifndef SAVEPVLDIALOG_H
#define SAVEPVLDIALOG_H

#include "ui_savepvldialog.h"

class SavePvlDialog : public QDialog
{
  Q_OBJECT

 public :
  SavePvlDialog(QWidget *parent=NULL);

  void setVoxelUnit(int);
  void setVoxelSize(float, float, float);
  void setDescription(QString);

  int voxelUnit();
  void voxelSize(float&, float&, float&);
  QString description ();
  int volumeFilter();
  bool dilateFilter();


 private :
  Ui::SavePvlDialog ui;
};

#endif
