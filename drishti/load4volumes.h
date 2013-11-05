#ifndef LOAD4VOLUMES_H
#define LOAD4VOLUMES_H

#include "ui_load4volumes.h"

class Load4Volumes : public QDialog
{
  Q_OBJECT

 public :
  Load4Volumes(QWidget *parent=NULL);

  QStringList volume1Files();
  QStringList volume2Files();
  QStringList volume3Files();
  QStringList volume4Files();

 private slots :
  void on_volButton1_pressed();
  void on_volButton2_pressed();
  void on_volButton3_pressed();
  void on_volButton4_pressed();

 private :
  Ui::Load4VolumesDialog ui;

  QStringList m_vol1Files;
  QStringList m_vol2Files;
  QStringList m_vol3Files;
  QStringList m_vol4Files;
};

#endif
