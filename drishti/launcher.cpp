#include "launcher.h"
#include <QDir>

#ifdef Q_OS_WIN
    #include <windows.h>
    #include <shellapi.h>
#endif


Launcher::Launcher(QWidget *parent) :
  QDialog(parent, Qt::WindowTitleHint|Qt::WindowCloseButtonHint)
{
  ui.setupUi(this);
  
  
  setStyleSheet("QWidget{background:black;}");
        ui.drishti->setStyleSheet("QWidget{background:#e7feff;}");
  ui.drishtiImport->setStyleSheet("QWidget{background:#e4f5ff;}");
   ui.drishtiPaint->setStyleSheet("QWidget{background:#e0eafe;}");
    ui.drishtiMesh->setStyleSheet("QWidget{background:#ddddfe;}");

  {
    QIcon icon;
    icon.addFile(QString::fromUtf8(":/images/drishti-run.png"), QSize(), QIcon::Normal, QIcon::Off);
    ui.drishti->setIcon(icon);
  }
  {
    QIcon icon;
    icon.addFile(QString::fromUtf8(":/images/drishtiimport.png"), QSize(), QIcon::Normal, QIcon::Off);
    ui.drishtiImport->setIcon(icon);
  }
  {
    QIcon icon;
    icon.addFile(QString::fromUtf8(":/images/drishtipaint.png"), QSize(), QIcon::Normal, QIcon::Off);
    ui.drishtiPaint->setIcon(icon);
  }
  {
    QIcon icon;
    icon.addFile(QString::fromUtf8(":/images/drishtimesh.png"), QSize(), QIcon::Normal, QIcon::Off);
    ui.drishtiMesh->setIcon(icon);
  }
  
  connect(ui.drishti, SIGNAL(clicked(bool)),
	  this, SLOT(drishti(bool)));
  connect(ui.drishtiImport, SIGNAL(clicked(bool)),
	  this, SLOT(drishtiImport(bool)));
  connect(ui.drishtiPaint, SIGNAL(clicked(bool)),
	  this, SLOT(drishtiPaint(bool)));
  connect(ui.drishtiMesh, SIGNAL(clicked(bool)),
	  this, SLOT(drishtiMesh(bool)));

  //ui.drishtiMesh->hide();
}


void
Launcher::drishti(bool b)
{
  accept();
  close();
}


//When running under Microsoft Windows with User Account Control (UAC) enabled,
//the following code always fails if starting a program that requires elevated
//privileges.
//The inner reason for this failure is that QProcess::startDetached method
//under Windows internally calls CreateProcess function. CreateProcess called
//from a non-elevated process fails with ERROR_ELEVATION_REQUIRED if it tries
//to start an executable that requires elevation.
//Corresponding error report in Qt Bug Tracker was rejected because it "sounds
//like a reasonable limitation of QProcess".
//Hence QProcess::startDetached is replaced with ShellExecute function.

void
Launcher::drishtiImport(bool b)
{
#if defined(Q_OS_WIN32)
  QString exeFileName = qApp->applicationDirPath() + QDir::separator() + "drishtiimport.exe";
  int result = (int)::ShellExecuteA(0, "open", exeFileName.toUtf8().constData(), 0, 0, SW_SHOWNORMAL);
  if (SE_ERR_ACCESSDENIED == result)
    {
      // Requesting elevation
      result = (int)::ShellExecuteA(0, "runas", exeFileName.toUtf8().constData(), 0, 0, SW_SHOWNORMAL);
    }
  if (result <= 32)
    {
      // error handling
    }  
#else
  QProcess::startDetached(qApp->applicationDirPath() + QDir::separator() + "drishtiimport");
#endif
  exit(0);
}

void
Launcher::drishtiPaint(bool b)
{
#if defined(Q_OS_WIN32)
  QString exeFileName = qApp->applicationDirPath() + QDir::separator() + "drishtipaint.exe";
  int result = (int)::ShellExecuteA(0, "open", exeFileName.toUtf8().constData(), 0, 0, SW_SHOWNORMAL);
  if (SE_ERR_ACCESSDENIED == result)
    {
      // Requesting elevation
      result = (int)::ShellExecuteA(0, "runas", exeFileName.toUtf8().constData(), 0, 0, SW_SHOWNORMAL);
    }
  if (result <= 32)
    {
      // error handling
    }  
#else
  QProcess::startDetached(qApp->applicationDirPath() + QDir::separator() + "drishtipaint");
#endif
  exit(0);
}

void
Launcher::drishtiMesh(bool b)
{
#if defined(Q_OS_WIN32)
  QString exeFileName = qApp->applicationDirPath() + QDir::separator() + "drishtimesh.exe";
  int result = (int)::ShellExecuteA(0, "open", exeFileName.toUtf8().constData(), 0, 0, SW_SHOWNORMAL);
  if (SE_ERR_ACCESSDENIED == result)
    {
      // Requesting elevation
      result = (int)::ShellExecuteA(0, "runas", exeFileName.toUtf8().constData(), 0, 0, SW_SHOWNORMAL);
    }
  if (result <= 32)
    {
      // error handling
    }  
#else
  QProcess::startDetached(qApp->applicationDirPath() + QDir::separator() + "drishtimesh");
#endif
  exit(0);
}
