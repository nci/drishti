#include "fileslistdialog.h"

#include <QCollator>
#include <QListWidgetItem>
#include <algorithm>

FilesListDialog::FilesListDialog(QList<QString> files,
				 QWidget *parent) :
  QDialog(parent)
{
  ui.setupUi(this);

  QStringList ordered;
  for (const QString& file : files)
    ordered << file;
  QCollator collator;
  collator.setCaseSensitivity(Qt::CaseInsensitive);
  collator.setNumericMode(true);
  std::sort(ordered.begin(), ordered.end(),
            [&collator](const QString& left, const QString& right)
            {
              const int comparison = collator.compare(left, right);
              return comparison == 0 ? left < right : comparison < 0;
            });
  ui.listWidget->addItems(ordered);
  ui.listWidget->setDragEnabled(true);
  ui.listWidget->setAcceptDrops(true);
  ui.listWidget->setDropIndicatorShown(true);
  ui.listWidget->setDragDropMode(QAbstractItemView::InternalMove);
  ui.listWidget->setDefaultDropAction(Qt::MoveAction);
}

QStringList
FilesListDialog::files() const
{
  QStringList result;
  for (int i = 0; i < ui.listWidget->count(); ++i)
    result << ui.listWidget->item(i)->text();
  return result;
}
