#include "doublespinboxdelegate.h"

DoubleSpinBoxDelegate::DoubleSpinBoxDelegate(QObject *parent)
     : QItemDelegate(parent)
{
}

QWidget*
DoubleSpinBoxDelegate::createEditor(QWidget *parent,
				    const QStyleOptionViewItem &option,
				    const QModelIndex &index) const
{
  QDoubleSpinBox *editor = new QDoubleSpinBox(parent);
  editor->setMinimum(-1000000.0);
  editor->setMaximum(1000000.0);

  DoubleSpinBoxDelegate *that = const_cast<DoubleSpinBoxDelegate *>(this);

  connect(editor, SIGNAL(valueChanged(double)),
	    that, SIGNAL(valueChanged(double)));

  return editor;
}

void
DoubleSpinBoxDelegate::setEditorData(QWidget *editor,
                                     const QModelIndex &index) const
{
  double value = index.model()->data(index, Qt::DisplayRole).toDouble();

  QDoubleSpinBox *dspinBox = static_cast<QDoubleSpinBox*>(editor);
  dspinBox->setValue(value);
}

void
DoubleSpinBoxDelegate::setModelData(QWidget *editor,
				    QAbstractItemModel *model,
                                    const QModelIndex &index) const
{
  QDoubleSpinBox *dspinBox = static_cast<QDoubleSpinBox*>(editor);
  dspinBox->interpretText();
  double value = dspinBox->value();

  model->setData(index, value, Qt::EditRole);
}

void
DoubleSpinBoxDelegate::updateEditorGeometry(QWidget *editor,
					    const QStyleOptionViewItem &option,
					    const QModelIndex &/* index */) const
{
  editor->setGeometry(option.rect);
} 
