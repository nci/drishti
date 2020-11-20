TEMPLATE = lib

DRISHTI_DEFINES = IMPORT
include(../../../../drishti.pri )

CONFIG += release plugin

TARGET = tiffplugin

include(../plugins.pri)

win32 {
  INCLUDEPATH += ./ ../../

  SOURCES = tiffplugin.cpp

  QMAKE_LFLAGS += /NODEFAULTLIB:LIBCMT
  QMAKE_LIBDIR += c:\cygwin64\home\acl900\drishtilib\libtiff-4.1\lib\Release
  LIBS += user32.lib libtiff.lib
}

unix {
 !macx {
  INCLUDEPATH += ../../ \
                 /home/acl900/drishtilib/tiff-4.1.0/build/include

  QMAKE_LIBDIR += /home/acl900/drishtilib/tiff-4.1.0/build/lib
  LIBS += -ltiff
  
  QMAKE_LFLAGS += "-Wl,-rpath=\'\$${ORIGIN}/../ITK\'"
  QMAKE_LFLAGS += "-Wl,-rpath=\'\$${ORIGIN}/../sharedlibs\'"

  SOURCES = tiffplugin.cpp
 }
}

macx {
  INCLUDEPATH += ../../

  LIBS += -ltiff
  
  SOURCES = tiffplugin.cpp
}

HEADERS = tiffplugin.h

