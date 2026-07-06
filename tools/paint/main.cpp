#include <GL/glew.h>
#include "drishtipaint.h"
#include "global.h"

#include <filesystem>
#include <iostream>

// Custom Qt message handler to redirect python output, cout, cerr, qDebug, qWarning, etc. to a QTextEdit
#include "streamredirect.h" 

namespace fs = std::filesystem;

#include <QApplication>
#include <QDockWidget>


int main(int argc, char **argv)
{
#if defined(Q_OS_WIN32)
  //QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
  int MYargc = 3;
  char *MYargv[] = {(char*)"Appname", (char*)"--platform", (char*)"windows:dpiawareness=0"};
  QApplication app(MYargc, MYargv);
#else
  QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
  QApplication app(argc, argv);   
#endif

  QGLFormat glFormat;
  glFormat.setDoubleBuffer(true);
  glFormat.setRgba(true);
  glFormat.setAlpha(true);
  glFormat.setDepth(true);

  QGLFormat::setDefaultFormat(glFormat);


  //-----------------------------------------
  QDockWidget *dock = new QDockWidget("Messages", nullptr, Qt::Widget);
  dock->setAllowedAreas(Qt::AllDockWidgetAreas);

  // Redirect std::cout and std::cerr
  static QtStreamRedirect coutRedirect(QtStreamRedirect::logWidget(), Qt::black);
  static QtStreamRedirect cerrRedirect(QtStreamRedirect::logWidget(), Qt::red);
  std::cout.rdbuf(&coutRedirect);
  std::cerr.rdbuf(&cerrRedirect);
  
  // Install Qt message handler
  qInstallMessageHandler(QtStreamRedirect::qtMessageHandler);

  dock->setWidget(QtStreamRedirect::logWidget());
  //-----------------------------------------


  DrishtiPaint mainWindow;
  mainWindow.addDockWidget(Qt::BottomDockWidgetArea, dock);
  mainWindow.addMessageWindow(dock);
  mainWindow.show();

  

  //if (Global::pythonInstalled())
  //  py::gil_scoped_release gil;

  return app.exec();
}
