QT += core gui widgets
CONFIG += console c++11 release
CONFIG -= app_bundle
TEMPLATE = app
TARGET = legacy_plugins_smoke
SOURCES += legacy_plugins_smoke.cpp
INCLUDEPATH += ..

win32 {
  INCLUDEPATH += D:/drishti-deps/vcpkg/installed/x64-windows/include
  LIBS += /LIBPATH:D:/drishti-deps/vcpkg/installed/x64-windows/lib netcdf.lib
}
