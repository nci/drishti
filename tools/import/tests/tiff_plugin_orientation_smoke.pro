QT += widgets

CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = tiff_plugin_orientation_smoke

INCLUDEPATH += ..

win32 {
  isEmpty(TIFF_INCLUDE_PATH): TIFF_INCLUDE_PATH = $$VCPKG_INCLUDE_PATH
  isEmpty(TIFF_LIBRARY_PATH): TIFF_LIBRARY_PATH = $$VCPKG_LIBRARY_PATH
  INCLUDEPATH += $$TIFF_INCLUDE_PATH
  QMAKE_LIBDIR += $$TIFF_LIBRARY_PATH
  LIBS += tiff.lib
}

unix:!macx {
  LIBS += -ltiff
}

macx {
  LIBS += -ltiff
}

SOURCES += tiff_plugin_orientation_smoke.cpp

HEADERS += ../volinterface.h \
           ../sourcefilesprovider.h \
           ../common.h
