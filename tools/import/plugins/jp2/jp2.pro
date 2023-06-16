TEMPLATE = lib

DRISHTI_DEFINES = IMPORT
include(../../../../drishti.pri )

CONFIG += release plugin

TARGET = jp2plugin

include(../plugins.pri)

win32 {
  INCLUDEPATH += ./ ../../

  SOURCES = jp2plugin.cpp

  INCLUDEPATH += ../../
  INCLUDEPATH += C:\cygwin64\home\acl900\vcpkg\vcpkg\installed\x64-windows\include
  QMAKE_LIBDIR += C:\cygwin64\home\acl900\vcpkg\vcpkg\installed\x64-windows\lib

  LIBS += openjp2.lib

}

unix {
 !macx {
  INCLUDEPATH += ../../ \
                 /home/acl900/drishtilib/openjpeg/build/include

  QMAKE_LIBDIR += /home/acl900/drishtilib/openjpeg/build/lib
  LIBS += -lopenjp2
  
  QMAKE_LFLAGS += "-Wl,-rpath=\'\$${ORIGIN}/../ITK\'"
  QMAKE_LFLAGS += "-Wl,-rpath=\'\$${ORIGIN}/../sharedlibs\'"

  SOURCES = jp2plugin.cpp
 }
}


HEADERS = jp2plugin.h

