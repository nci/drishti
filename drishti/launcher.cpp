#include "launcher.h"
#include <QDir>


Launcher::Launcher(QWidget *parent) :
  QDialog(parent, Qt::WindowTitleHint|Qt::WindowCloseButtonHint)
{
  ui.setupUi(this);

  setStyleSheet("QWidget{background:black;}");
  ui.drishti->setStyleSheet("QWidget{background:aliceblue;}");
  ui.drishtiImport->setStyleSheet("QWidget{background:azure;}");
  ui.drishtiPaint->setStyleSheet("QWidget{background:floralwhite;}");

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
  
  connect(ui.drishti, SIGNAL(clicked(bool)),
	  this, SLOT(drishti(bool)));
  connect(ui.drishtiImport, SIGNAL(clicked(bool)),
	  this, SLOT(drishtiImport(bool)));
  connect(ui.drishtiPaint, SIGNAL(clicked(bool)),
	  this, SLOT(drishtiPaint(bool)));
}


void
Launcher::drishti(bool b)
{
  accept();
  close();
}

void
Launcher::drishtiImport(bool b)
{
#if defined(Q_OS_WIN32)
  QProcess::startDetached(qApp->applicationDirPath() + QDir::separator() + "drishtiimport.exe");
#else
  QProcess::startDetached(qApp->applicationDirPath() + QDir::separator() + "drishtiimport");
#endif
  exit(0);
}

void
Launcher::drishtiPaint(bool b)
{
#if defined(Q_OS_WIN32)
  QProcess::startDetached(qApp->applicationDirPath() + QDir::separator() + "drishtipaint.exe");
#else
  QProcess::startDetached(qApp->applicationDirPath() + QDir::separator() + "drishtipaint");
#endif
  exit(0);
}
