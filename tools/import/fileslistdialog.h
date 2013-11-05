#ifndef FILESLISTDIALOG_H
#define FILESLISTDIALOG_H

#include "ui_fileslistdialog.h"

class FilesListDialog : public QDialog
{
  Q_OBJECT

 public :
  FilesListDialog(QList<QString>,
		  QWidget *parent=NULL);

 private:
  Ui::FilesListDialog ui;
};


#endif
