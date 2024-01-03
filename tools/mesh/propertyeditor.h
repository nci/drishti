#ifndef PROPERTYEDITOR_H
#define PROPERTYEDITOR_H

#include "commonqtclasses.h"
#include <QSignalMapper>
#include <QTextEdit>
#include <QListWidget>
#include <QCheckBox>
#include <QGridLayout>
#include "ui_propertyeditor.h"

class PropertyEditor : public QDialog
{
  Q_OBJECT

 public :
  PropertyEditor(QWidget *parent=0);
  ~PropertyEditor();

  void set(QString,
	   QMap<QString, QVariantList>,
	   QStringList,
	   QStringList icons=QStringList());

  QMap<QString, QPair<QVariant, bool> > get();
  QString getCommandString();

  QGradientStops getGradientStops(QString);

 private slots:
  void changeColor(const QString&);
  void changeFont(QString);
  void resetProperty(int);
  void helpItemSelected(int);
  void hotkeymouseClicked(bool);
  void selectAll(int);
  void itemClicked(QListWidgetItem*);
  
 private :
  Ui::PropertyEditor ui;
  QSignalMapper *m_signalMapper;
  QMap<QString, QVariantList> m_plist;
  QMap<QString, QWidget*> m_widgets;
  QStringList m_keys;

  QListWidget *m_helpList;
  QTextEdit *m_helpLabel;
  QTextEdit *m_constantLabel;
  QCheckBox *m_hotkeymouse;

  QStringList m_hotkeymouseString;
  QStringList m_helpCommandString;
  QStringList m_helpText;
  QStringList m_orighelpCS;

  int m_prevRow;
};

#endif
