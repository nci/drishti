//  Boot order is important:
//    1. Initialise pybind11 interpreter (scoped_interpreter owns lifetime)
//    2. Import the embedded 'vsgbox' module (defined in PyModule.cpp via
//       PYBIND11_EMBEDDED_MODULE) and wire up the global SceneController ptr
//    3. Create the Qt application + main window
//    4. Run the Qt event loop
//    5. ~scoped_interpreter() finalises Python on exit
#include <pybind11/embed.h>

#include <filesystem>
#include <iostream>

namespace py = pybind11;
namespace fs = std::filesystem;

#include "drishtiimport.h"
#include <QMessageBox>

// Custom Qt message handler to redirect python output, cout, cerr, qDebug, qWarning, etc. to a QTextEdit
#include "streamredirect.h" 
#include "pythonengine.h"


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


//-----------------------------------------
  // Embedded Python interpreter 
  // Must live longer than the QApplication so Python is alive during
  // the entire event loop.
  PythonEngine &pythonGuard = PythonEngine::instance(); // wrapper for py::scoped_interpreter

//  py::scoped_interpreter pythonGuard{};
//  
//  // Determine the path to the extracted Python library
//  fs::path exePath = fs::current_path(); // Adjust if needed
//  //fs::path pythonLibDir = exePath / "python314";
//  fs::path pythonLibDir = exePath; // Assuming the Python library is in the same directory as the executable";
//
//  // Set the Python path to include the directory
//  py::module sys = py::module::import("sys");
//  py::object path = sys.attr("path");
//  path.attr("insert")(0, pythonLibDir.string());
//
//  // Example Python code execution
//  try
//  {
//      py::exec(R"(
//          import sys
//          import pyredir
//          
//          # Redirect Python stdout/stderr to C++ streams
//          sys.stdout = pyredir.CoutRedirect()
//          sys.stderr = pyredir.CerrRedirect()
//      )");
//  }
//  catch (const std::exception &e)
//  {
//    QMessageBox::critical(nullptr, "Python Error", e.what());
//  }
//-----------------------------------------


  
  DrishtiImport mainWindow;
  mainWindow.show();

  
  return app.exec();
}
