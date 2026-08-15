#ifndef FILESLISTDIALOG_H
#define FILESLISTDIALOG_H

#include "ui_fileslistdialog.h"
#include <QStringList>

class FilesListDialog : public QDialog
{
  Q_OBJECT

 public :
  FilesListDialog(QList<QString>,
		  QWidget *parent=NULL);

  QStringList files() const;

 private:
  Ui::FilesListDialog ui;
};


#endif
