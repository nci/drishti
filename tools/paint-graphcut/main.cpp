#include <GL/glew.h>

#include <QApplication>
#include "drishtipaint.h"

int main(int argv, char **args)
{
  QApplication app(argv, args);

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
