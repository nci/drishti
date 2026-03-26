TEMPLATE = lib

DRISHTI_DEFINES = IMPORT
include(../../../../drishti.pri )

CONFIG += release plugin

TARGET = jp2plugin

include(../plugins.pri)

win32 {
  SOURCES = jp2plugin.cpp

  INCLUDEPATH += ./ ../../

  INCLUDEPATH += $$VCPKG_INCLUDE_PATH
  QMAKE_LIBDIR += $$VCPKG_LIBRARY_PATH

  LIBS += openjp2.lib

}

unix {
 !macx {
  INCLUDEPATH += ../../ \
                 /usr/include/openjpeg-2.4

  LIBS += -lopenjp2
  
  QMAKE_LFLAGS += "-Wl,-rpath=\'\$${ORIGIN}/../ITK\'"
  QMAKE_LFLAGS += "-Wl,-rpath=\'\$${ORIGIN}/../sharedlibs\'"

  SOURCES = jp2plugin.cpp
 }
}


HEADERS = jp2plugin.h

