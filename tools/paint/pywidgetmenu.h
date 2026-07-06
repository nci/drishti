#ifndef PYWIDGETMENU_H
#define PYWIDGETMENU_H

#include "ui_pywidgetmenu.h"

#include <QLineEdit>
#include <QHash>
#include <QVariant>

class PyWidgetMenu : public QWidget
{
  Q_OBJECT

 public :
  PyWidgetMenu(QWidget *parent=NULL);
  ~PyWidgetMenu();

  void addRow(QString, QString, QString);

  void loadScripts(QString);    
  void genArgumentsFromTable();
  QHash<QString, QVariant> getArguments() { return m_arguments; }
  
  signals :
    void runCommand(QString, QHash<QString, QVariant>);
  
  public slots:
    void on_addRow_pressed();
    void on_runScript_pressed();
    void on_removeRow_pressed();
    void on_scriptChanged(int);
    
 private :
    Ui::PyWidgetMenu ui;

    QString m_scriptDir;
    QStringList m_jsonFileList;
    QString m_script;
    QString m_doc;
    QHash<QString, QVariant> m_arguments;

};

#endif
