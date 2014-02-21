QT += widgets core gui

win32 {
  DESTDIR = ../../../../bin/importplugins
}

unix {
 !macx {
  DESTDIR = ../../../../bin/importplugins
 }
}

macx {
  DESTDIR = ../../../../bin/drishtiimport.app/importplugins
}

