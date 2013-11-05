#ifndef DOUBLESPINBOXDELEGATE_H
#define DOUBLESPINBOXDELEGATE_H

#include <QItemDelegate>
#include <QModelIndex>
#include <QObject>
#include <QSize>
#include <QDoubleSpinBox>

class DoubleSpinBoxDelegate : public QItemDelegate
{
 Q_OBJECT

 public:
  DoubleSpinBoxDelegate(QObject *parent = 0);

  QWidget *createEditor(QWidget *parent,
			const QStyleOptionViewItem &option,
			const QModelIndex &index) const;
  
  void setEditorData(QWidget *editor,
		     const QModelIndex &index) const;
  void setModelData(QWidget *editor,
		    QAbstractItemModel *model,
		    const QModelIndex &index) const;
  
  void updateEditorGeometry(QWidget *editor,
			    const QStyleOptionViewItem &option,
			    const QModelIndex &index) const;

 signals :
  void valueChanged(double);

};

#endif 
