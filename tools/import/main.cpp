// Python is initialized lazily when a Python-backed import script is selected.
// Native import plugins must remain usable without an installed Python runtime.
#include <iostream>

#include "drishtiimport.h"
#include <QMessageBox>
#include <QDockWidget>
#include "../../portableqtruntime.h"

// Custom Qt message handler to redirect python output, cout, cerr, qDebug, qWarning, etc. to a QTextEdit
#include "streamredirect.h" 



int main(int argc, char **argv)
{
#if defined(Q_OS_WIN32)
  configurePortableQtRuntime();
  QApplication app(argc, argv);
#else
  QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
  QApplication app(argc, argv);   
#endif

  //-----------------------------------------
  QDockWidget *dock = new QDockWidget("Messages", nullptr, Qt::Widget);
  dock->setAllowedAreas(Qt::AllDockWidgetAreas);

  // Redirect std::cout and std::cerr
  QtStreamRedirect coutRedirect(QtStreamRedirect::logWidget(), Qt::black);
  QtStreamRedirect cerrRedirect(QtStreamRedirect::logWidget(), Qt::red);
  std::streambuf *const originalCout = std::cout.rdbuf(&coutRedirect);
  std::streambuf *const originalCerr = std::cerr.rdbuf(&cerrRedirect);
  
  // Install Qt message handler
  qInstallMessageHandler(QtStreamRedirect::qtMessageHandler);

  dock->setWidget(QtStreamRedirect::logWidget());
  //-----------------------------------------

  DrishtiImport mainWindow;
  mainWindow.addDockWidget(Qt::BottomDockWidgetArea, dock);
  mainWindow.show();
    
  const int result = app.exec();
  qInstallMessageHandler(nullptr);
  std::cout.rdbuf(originalCout);
  std::cerr.rdbuf(originalCerr);
  return result;
}
