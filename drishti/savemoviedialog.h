#ifndef SAVEMOVIEDIALOG_H
#define SAVEMOVIEDIALOG_H

#include "ui_savemovie.h"

class SaveMovieDialog : public QDialog
{
  Q_OBJECT

 public :
  SaveMovieDialog(QWidget *parent=NULL,
		  QString dir="",
		  int start=1,
		  int end=1,
		  int step=1);

  QString fileName();
  int startFrame();
  int endFrame();
  int stepFrame();
  bool movieMode();

 private slots :
  void on_m_file_pressed();

 private :
  Ui::SaveMovieDialog ui; 

  QString m_dir;

};

#endif
