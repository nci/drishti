#include "mainwindowui.h"

Ui::MainWindow* MainWindowUI::m_mainUI = NULL;
void MainWindowUI::setMainWindowUI(Ui::MainWindow *ui) { m_mainUI = ui; }
Ui::MainWindow* MainWindowUI::mainWindowUI() { return m_mainUI; }

void
MainWindowUI::changeDrishtiIcon(bool run)
{
  if (m_mainUI)
    {
      QWidget* pw = m_mainUI->menubar->parentWidget();
      if (run)
	pw->setWindowIcon(QIcon(":/images/drishti-run.png"));
      else
	pw->setWindowIcon(QIcon(":/images/drishti-idle.png"));
    }
}
