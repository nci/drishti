#include <QApplication>
#include <QtGui>

#include "mainwindow.h"

int main(int argc, char** argv)
{
  char *flnm;

  QApplication application(argc,argv);
  
  QGLFormat glFormat;
  glFormat.setSampleBuffers(true);
  glFormat.setDoubleBuffer(true);
  glFormat.setRgba(true);
  glFormat.setAlpha(true);

//  //-----------------------------
//  // did not work - still getting 8bit buffers
//  glFormat.setAlphaBufferSize(16);
//  glFormat.setRedBufferSize(16);
//  glFormat.setGreenBufferSize(16);
//  glFormat.setBlueBufferSize(16);
//  //-----------------------------

  flnm = argv[1];
  if (QString::compare(argv[1], "-stereo", Qt::CaseInsensitive) == 0)
    {
      flnm = argv[2];
      glFormat.setStereo(true);
    }
  QGLFormat::setDefaultFormat(glFormat);

  MainWindow mainwindow;
  mainwindow.show();
  
  // Run main loop.
  return application.exec();
}
