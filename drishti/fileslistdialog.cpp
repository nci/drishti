#include "fileslistdialog.h"

FilesListDialog::FilesListDialog(QList<QString> files,
				 QWidget *parent) :
  QDialog(parent)
{
  ui.setupUi(this);

  ui.listWidget->addItems(files);
}
