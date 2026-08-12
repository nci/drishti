QT += core widgets

CONFIG += console c++11 release
CONFIG -= app_bundle
TEMPLATE = app
TARGET = volume_plugin_validation_smoke

INCLUDEPATH += ..

SOURCES += volume_plugin_validation_smoke.cpp \
           ../volumepluginvalidation.cpp

HEADERS += ../volumepluginvalidation.h \
           ../pluginoperationstatus.h \
           ../volinterface.h
