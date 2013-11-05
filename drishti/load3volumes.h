#ifndef LOAD3VOLUMES_H
#define LOAD3VOLUMES_H

#include "ui_load3volumes.h"

class Load3Volumes : public QDialog
{
  Q_OBJECT

 public :
  Load3Volumes(QWidget *parent=NULL);

  QStringList volume1Files();
  QStringList volume2Files();
  QStringList volume3Files();

 private slots :
  void on_volButton1_pressed();
  void on_volButton2_pressed();
  void on_volButton3_pressed();

 private :
  Ui::Load3VolumesDialog ui;

  QStringList m_vol1Files;
  QStringList m_vol2Files;
  QStringList m_vol3Files;

};

#endif
