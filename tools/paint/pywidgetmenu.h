#ifndef PYWIDGETMENU_H
#define PYWIDGETMENU_H

#include "ui_pywidgetmenu.h"

#include <QLineEdit>

class PyWidgetMenu : public QWidget
{
  Q_OBJECT

 public :
  PyWidgetMenu(QWidget *parent=NULL);
  ~PyWidgetMenu();

  void addRow(QString, QString);
  QStringList getData();

  void loadScripts(QString);  

  signals :
    void runCommand(QString);
  
  public slots:
    void on_addRow_pressed();
    void on_runScript_pressed();
    void on_removeRow_pressed();
    void on_scriptChanged(int);
    
 private :
    Ui::PyWidgetMenu ui;

    QString m_scriptDir;
    QStringList m_jsonFileList;
    QString m_executable;
    QString m_interpreter;
    QString m_script;
    
};

#endif
