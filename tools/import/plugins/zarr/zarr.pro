TEMPLATE = lib

include(../../../../drishti.pri )

CONFIG += release plugin

TARGET = zarrplugin

HEADERS = zarrplugin.h

SOURCES = zarrplugin.cpp

include(../plugins.pri)

win32 {
  INCLUDEPATH += ../../

  INCLUDEPATH += $$VCPKG_INCLUDE_PATH
  QMAKE_LIBDIR += $$VCPKG_LIBRARY_PATH

  LIBS += blosc.lib
}

unix {
!macx {
  INCLUDEPATH += ../../
  LIBS += -lblosc
}
}
