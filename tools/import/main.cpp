#include "drishtiimport.h"

int main(int argv, char **args)
{
  //QApplication app(argv, args);
#if defined(Q_OS_WIN32)
  //QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
  int MYargc = 3;
  char *MYargv[] = {(char*)"Appname", (char*)"--platform", (char*)"windows:dpiawareness=0"};
  QApplication app(MYargc, MYargv);
#else
  QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
  QApplication app(argc, argv);   
#endif
  

    DrishtiImport mainWindow;
    mainWindow.show();

    return app.exec();
}
