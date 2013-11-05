#ifndef LOAD2VOLUMES_H
#define LOAD2VOLUMES_H

#include "ui_load2volumes.h"

class Load2Volumes : public QDialog
{
  Q_OBJECT

 public :
  Load2Volumes(QWidget *parent=NULL);

  QStringList volume1Files();
  QStringList volume2Files();

 private slots :
  void on_volButton1_pressed();
  void on_volButton2_pressed();

 private :
  Ui::Load2VolumesDialog ui;

  QStringList m_vol1Files;
  QStringList m_vol2Files;

};

#endif
