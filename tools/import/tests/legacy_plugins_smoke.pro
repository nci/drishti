QT += core gui widgets
CONFIG += console c++11 release
CONFIG -= app_bundle
TEMPLATE = app
TARGET = legacy_plugins_smoke
SOURCES += legacy_plugins_smoke.cpp
INCLUDEPATH += ..

win32 {
  isEmpty(DRISHTI_VCPKG_ROOT): DRISHTI_VCPKG_ROOT = $$(DRISHTI_VCPKG_ROOT)
  isEmpty(DRISHTI_VCPKG_ROOT): DRISHTI_VCPKG_ROOT = $$clean_path($$PWD/../../../.lab-agent/dependencies/install/vcpkg)
  INCLUDEPATH += $$clean_path($$DRISHTI_VCPKG_ROOT/installed/x64-windows/include)
  LIBS += /LIBPATH:$$clean_path($$DRISHTI_VCPKG_ROOT/installed/x64-windows/lib) netcdf.lib
}
