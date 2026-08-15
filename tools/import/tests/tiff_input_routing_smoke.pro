QT += core gui

CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = tiff_input_routing_smoke

INCLUDEPATH += ..

SOURCES += tiff_input_routing_smoke.cpp \
           ../tiffinputrouting.cpp \
           ../tiffpagevalidation.cpp

HEADERS += ../tiffinputrouting.h \
           ../tiffpagevalidation.h

win32 {
  isEmpty(DRISHTI_VCPKG_ROOT): DRISHTI_VCPKG_ROOT = $$(DRISHTI_VCPKG_ROOT)
  isEmpty(DRISHTI_VCPKG_ROOT): DRISHTI_VCPKG_ROOT = $$clean_path($$PWD/../../../.lab-agent/dependencies/install/vcpkg)
  INCLUDEPATH += $$clean_path($$DRISHTI_VCPKG_ROOT/installed/x64-windows/include)
  QMAKE_LIBDIR += $$clean_path($$DRISHTI_VCPKG_ROOT/installed/x64-windows/lib)
  LIBS += tiff.lib
}

unix: LIBS += -ltiff
