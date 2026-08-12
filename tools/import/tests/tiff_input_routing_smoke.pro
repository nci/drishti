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
  INCLUDEPATH += D:/drishti-deps/vcpkg/installed/x64-windows/include
  QMAKE_LIBDIR += D:/drishti-deps/vcpkg/installed/x64-windows/lib
  LIBS += tiff.lib
}

unix: LIBS += -ltiff
