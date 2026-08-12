QT += widgets

CONFIG += console c++17 release
CONFIG -= app_bundle
TEMPLATE = app
TARGET = python_script_plugin_smoke

INCLUDEPATH += .. \
               ../../../common/src/pybind \
               $$PYBIND11_INCLUDE_PATH \
               $$PYTHON_INCLUDE_PATH

win32 {
  QMAKE_LIBDIR += $$PYTHON_LIBRARY_PATH
  LIBS += $$PYTHON_LIBRARY
}

SOURCES += python_script_plugin_smoke.cpp \
           ../scriptsplugin.cpp \
           ../../../common/src/pybind/pythonengine.cpp

HEADERS += ../scriptsplugin.h \
           ../common.h \
           ../commonqtclasses.h \
           ../../../common/src/pybind/pythonengine.h
