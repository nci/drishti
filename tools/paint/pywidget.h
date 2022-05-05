#ifndef PYWIDGET_H
#define PYWIDGET_H

#include <QWidget>
#include <QProcess>
#include <QPlainTextEdit>
#include <QLineEdit>

#include "pywidgetmenu.h"

class PyWidget : public QWidget
{
  Q_OBJECT

 public :
  PyWidget(QWidget *parent=0);
  ~PyWidget();

  void setFilename(QString);
  void setSize(int, int, int);
  void setVolPtr(uchar*);
  void setMaskPtr(uchar*);
  
  //int runPythonCode(QString, QString);

 signals :
  void pyWidgetClosed();
    
 private slots :
   void readOutput();
   void readError();
   void processLine();
   void closeEvent(QCloseEvent*);
  void runCommand(QString);
  
 private :
   QString m_fileName;
   QString m_maskName;
   
   int m_d, m_w, m_h;
   uchar *m_volPtr;
   uchar *m_maskPtr;
   
   QProcess *m_process;
   QPlainTextEdit *m_plainTextEdit;
   QLineEdit *m_lineEdit;

   PyWidgetMenu *m_menu;
  
   QStringList m_scripts;
  
  //int initPy();

  void loadScripts();
};

#endif
