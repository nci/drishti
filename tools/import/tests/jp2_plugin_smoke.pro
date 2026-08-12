QT += widgets gui core

CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = jp2_plugin_smoke

INCLUDEPATH += ..

SOURCES += jp2_plugin_smoke.cpp

HEADERS += ../common.h \
           ../commonqtclasses.h \
           ../volinterface.h
