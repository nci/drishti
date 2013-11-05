#ifndef VOLUMEINFORMATIONWIDGET_H
#define VOLUMEINFORMATIONWIDGET_H

#include "ui_volumeinformation.h"
#include "volumeinformation.h"

class VolumeInformationWidget : public QWidget
{
  Q_OBJECT

 public :
  VolumeInformationWidget(QWidget *parent=NULL);

  QList<bool> repeatType();
  void setRepeatType(QList<bool>);

 public slots :
  void setVolumes(QList<int>);
  void refreshVolInfo(int, VolumeInformation);
  void refreshVolInfo2(int, VolumeInformation);
  void refreshVolInfo3(int, VolumeInformation);
  void refreshVolInfo4(int, VolumeInformation);
  void refreshVolInfo(int, int, VolumeInformation);

 signals :
  void volumeNumber(int);
  void volumeNumber(int, int);
  void updateGL();
  void updateScaling();
  void repeatType(int, bool);

 private slots :
  void on_tabWidget_currentChanged(int);
  void on_m_repeatType_clicked(bool);
  void on_m_repeatType_2_clicked(bool);
  void on_m_repeatType_3_clicked(bool);
  void on_m_repeatType_4_clicked(bool);
  void on_m_volnum_editingFinished();
  void on_m_volnum_2_editingFinished();
  void on_m_volnum_3_editingFinished();
  void on_m_volnum_4_editingFinished();
  void on_m_voxelUnit_currentIndexChanged(int);
  void on_m_voxelUnit_2_currentIndexChanged(int);
  void on_m_voxelUnit_3_currentIndexChanged(int);
  void on_m_voxelUnit_4_currentIndexChanged(int);
  void on_m_description_editingFinished();
  void on_m_description_2_editingFinished();
  void on_m_description_3_editingFinished();
  void on_m_description_4_editingFinished();
  void on_m_voxelSize_editingFinished();
  void on_m_voxelSize_2_editingFinished();
  void on_m_voxelSize_3_editingFinished();
  void on_m_voxelSize_4_editingFinished();
  void on_m_boxSize_editingFinished();
  void on_m_boxSize_2_editingFinished();
  void on_m_boxSize_3_editingFinished();
  void on_m_boxSize_4_editingFinished();
  void keyPressEvent(QKeyEvent*);


 private :
  Ui::processedVolumeInformation ui;

  bool m_replaceInHeader;

  VolumeInformation m_volInfo[4];
  QWidget *m_widget2;
  QWidget *m_widget3;
  QWidget *m_widget4;

  int m_currVol;

  void setVolumes(int);
  void setVolumes(int, int);
  void setVolumes(int, int, int);
  void setVolumes(int, int, int, int);

  void newVoxelUnit(int);
  void newVoxelSize();
  void newBoxSize();
  void newDescription();

  void showHelp();
};

#endif
