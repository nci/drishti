#ifndef PYWIDGET_H
#define PYWIDGET_H

#include <QWidget>
#include <QProcess>
#include <QPlainTextEdit>
#include <QLineEdit>

class PyWidget : public QWidget
{
  Q_OBJECT

 public :
  PyWidget(QWidget *parent=0);
  ~PyWidget();

 private slots :
   void readOutput();
   void readError();
   void processLine();
   
 private :
  QProcess *m_process;
  QPlainTextEdit *m_plainTextEdit;
  QLineEdit *m_lineEdit;
};

#endif
