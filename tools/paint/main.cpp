#include <GL/glew.h>

#include <QApplication>
#include "drishtipaint.h"

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



  QGLFormat glFormat;
  glFormat.setDoubleBuffer(true);
  glFormat.setRgba(true);
  glFormat.setAlpha(true);
  glFormat.setDepth(true);

  QGLFormat::setDefaultFormat(glFormat);

  DrishtiPaint mainWindow;
  mainWindow.show();

  return app.exec();
}
