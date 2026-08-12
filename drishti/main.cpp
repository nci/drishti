#include <GL/glew.h>

#include <QApplication>
#include "mainwindow.h"

#include <QTranslator>
#include <QMessageBox>

#include "launcher.h"
#include "../portableqtruntime.h"


int main(int argc, char** argv)
{
  
#if defined(Q_OS_WIN32)
  QApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
  configurePortableQtRuntime();
  QApplication application(argc, argv);
#else
  QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
  QApplication application(argc, argv);   
#endif

  bool activateLauncher = true;

  //------------------------------
  // check whether we go through launcher
  if (argc > 0)
    {
      for (int i=0; i<argc; i++)
	if (std::string(argv[i]) == "-drishti")
	  {
	    activateLauncher = false;
	    break;
	  }
    }
  //------------------------------
  
  //------------------------------
  //------------------------------
  // choose application to run
  if (activateLauncher)
    {
      Launcher launcher;
      if (launcher.exec() != QDialog::Accepted)
	exit(0);
    }
  //------------------------------
  //------------------------------
  
     
  QGLFormat glFormat;
#if defined(Q_OS_WIN32) || defined(Q_OS_LINUX)
  glFormat.setVersion(4, 5);
  glFormat.setProfile(QGLFormat::CompatibilityProfile);
#endif
  glFormat.setSampleBuffers(false);
  glFormat.setDoubleBuffer(true);
  glFormat.setRgba(true);
  glFormat.setAlpha(true);
  glFormat.setDepth(true);

//  //-----------------------------
//  // did not work - still getting 8bit buffers
//  glFormat.setAlphaBufferSize(16);
//  glFormat.setRedBufferSize(16);
//  glFormat.setGreenBufferSize(16);
//  glFormat.setBlueBufferSize(16);
//  //-----------------------------

  if (argc > 1)
    {
      if (QString::compare(argv[1], "-stereo", Qt::CaseInsensitive) == 0)
	glFormat.setStereo(true);
    }

  QGLFormat::setDefaultFormat(glFormat);

  //----------------------
  QTranslator translator;
  translator.load(qApp->applicationDirPath() +
		  QDir::separator() +
		  "drishtitr_ch");
  application.installTranslator(&translator);
  //----------------------
  
  MainWindow mainwindow;
  mainwindow.show();
  
  // Run main loop.
  return application.exec();
}
