TEMPLATE = lib

DRISHTI_DEFINES = IMPORT TIFF
include(../../../../drishti.pri )

CONFIG += release plugin

TARGET = tiffplugin

include(../plugins.pri)

win32 {
  SOURCES = tiffplugin.cpp
  INCLUDEPATH += ./ ../../
  INCLUDEPATH += $$VCPKG_INCLUDE_PATH
  QMAKE_LIBDIR += $$VCPKG_LIBRARY_PATH
  LIBS += tiff.lib
}

unix {
 !macx {
  INCLUDEPATH += ../../
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

