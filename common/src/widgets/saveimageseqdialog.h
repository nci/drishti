#ifndef SAVEIMAGESEQDIALOG_H
#define SAVEIMAGESEQDIALOG_H

#include "ui_saveimgseq.h"

class SaveImageSeqDialog : public QDialog
{
  Q_OBJECT

 public :
  SaveImageSeqDialog(QWidget *parent=NULL,
		     QString dir="",
		     int start=1,
		     int end=1,
		     int step=1);

  QString fileName();
  int startFrame();
  int endFrame();
  int stepFrame();
  int imageMode();

 private slots :
  void on_m_file_pressed();

 private :
  Ui::SaveImageSeqDialog ui; 

  QString m_dir;

};

#endif
