#include <GL/glew.h>

#include <QApplication>
#include "drishtipaint.h"
#include "../../portableqtruntime.h"

int main(int argc, char **argv)
{
#if defined(Q_OS_WIN32)
  QApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
  configurePortableQtRuntime();
  QApplication app(argc, argv);
#else
  QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
  QApplication app(argc, argv);   
#endif



  QGLFormat glFormat;
#if defined(Q_OS_WIN32) || defined(Q_OS_LINUX)
  glFormat.setVersion(4, 2);
  glFormat.setProfile(QGLFormat::CompatibilityProfile);
#endif
  glFormat.setSampleBuffers(false);
  glFormat.setDoubleBuffer(true);
  glFormat.setRgba(true);
  glFormat.setAlpha(true);
  glFormat.setDepth(true);

  QGLFormat::setDefaultFormat(glFormat);

  DrishtiPaint mainWindow;
  mainWindow.show();

  return app.exec();
}
