#include <GL/glew.h>

#include <QApplication>
#include "mainwindow.h"

#include <QTranslator>

#include "launcher.h"


int main(int argc, char** argv)
{  
#if defined(Q_OS_WIN32)
  //QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
  int MYargc = 3;
  char *MYargv[] = {(char*)"Appname", (char*)"--platform", (char*)"windows:dpiawareness=0"};
  QApplication application(MYargc, MYargv);
#else
  QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
  QApplication application(argc, argv);   
#endif


  //------------------------------
  //------------------------------
  // choose application to run
  Launcher launcher;
  if (launcher.exec() != QDialog::Accepted)
    exit(0);
  //------------------------------
  //------------------------------

  
     
  QGLFormat glFormat;
  glFormat.setSampleBuffers(true);
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
