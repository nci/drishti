//  Boot order is important:
//    1. Initialise pybind11 interpreter (scoped_interpreter owns lifetime)
//    2. Import the embedded 'vsgbox' module (defined in PyModule.cpp via
//       PYBIND11_EMBEDDED_MODULE) and wire up the global SceneController ptr
//    3. Create the Qt application + main window
//    4. Run the Qt event loop
//    5. ~scoped_interpreter() finalises Python on exit
#include "pythonengine.h"

#include <filesystem>
#include <iostream>

//namespace py = pybind11;
namespace fs = std::filesystem;

#include "drishtiimport.h"
#include <QMessageBox>

// Custom Qt message handler to redirect python output, cout, cerr, qDebug, qWarning, etc. to a QTextEdit
#include "streamredirect.h" 



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


//-----------------------------------------
// Create a simple message window with a QTextEdit to display logs
  QMainWindow mesgWindow;
  QWidget centralWidget; 
  QVBoxLayout layout;
  QTextEdit logView;
  logView.setReadOnly(true);
  
  g_logWidget = &logView; // Set the global pointer to the QTextEdit for logging

  layout.addWidget(&logView);
  centralWidget.setLayout(&layout);
  mesgWindow.setCentralWidget(&centralWidget);
  mesgWindow.setWindowTitle("Messages");
  mesgWindow.resize(1280, 1024);
  mesgWindow.show();

  // Redirect std::cout and std::cerr
  static QtStreamRedirect coutRedirect(&logView, Qt::black);
  static QtStreamRedirect cerrRedirect(&logView, Qt::red);
  std::cout.rdbuf(&coutRedirect);
  std::cerr.rdbuf(&cerrRedirect);
  
  // Install Qt message handler
  qInstallMessageHandler(qtMessageHandler);
//-----------------------------------------


  // Embedded Python interpreter 
  PythonEngine &pythonGuard = PythonEngine::instance();

  
  DrishtiImport mainWindow;
  mainWindow.show();

  
  return app.exec();
}
