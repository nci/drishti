#ifndef LAUNCHER_H
#define LAUNCHER_H

#include "ui_launcher.h"

#include <QProcess>
#include <QApplication>

class Launcher : public QDialog
{
  Q_OBJECT

 public :
  Launcher(QWidget *parent=0);

  private slots :
    void drishti(bool);
    void drishtiImport(bool);
    void drishtiPaint(bool);
    void drishtiMesh(bool);

 private :
    Ui::launcher ui;
};

#endif
