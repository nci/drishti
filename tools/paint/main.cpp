#include <GL/glew.h>

//  Boot order is important:
//    1. Initialise pybind11 interpreter (scoped_interpreter owns lifetime)
//    2. Import the embedded 'vsgbox' module (defined in PyModule.cpp via
//       PYBIND11_EMBEDDED_MODULE) and wire up the global SceneController ptr
//    3. Create the Qt application + main window
//    4. Run the Qt event loop
//    5. ~scoped_interpreter() finalises Python on exit
//#include "pythonengine.h"
#include <pybind11/pybind11.h>

#include "global.h"

#include <filesystem>
#include <iostream>

// Custom Qt message handler to redirect python output, cout, cerr, qDebug, qWarning, etc. to a QTextEdit
#include "streamredirect.h" 

namespace fs = std::filesystem;

#include <QApplication>
#include <QDockWidget>
#include "drishtipaint.h"

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


  //// Embedded Python interpreter 
  //PythonEngine &pythonGuard = PythonEngine::instance();
  //(&pythonGuard)->init(true);
  //Global::setPythonInstalled(true);

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
  mainWindow.show();
  
  if (Global::pythonInstalled())
    py::gil_scoped_release gil;

  return app.exec();
}
