QT += widgets gui

CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = nifti_plugin_smoke

INCLUDEPATH += ..

SOURCES += nifti_plugin_smoke.cpp

HEADERS += ../common.h \
           ../commonqtclasses.h \
           ../volinterface.h
