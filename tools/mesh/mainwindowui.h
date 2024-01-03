#ifndef MAINWINDOWUI_H
#define MAINWINDOWUI_H

#include "ui_mainwindow.h"
class MainWindowUI
{
 public :
  static void setMainWindowUI(Ui::MainWindow*);
  static Ui::MainWindow* mainWindowUI();

  static void changeDrishtiIcon(bool);

 private :
  static Ui::MainWindow *m_mainUI;
};

#endif

