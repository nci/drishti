QT += widgets

CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = plugin_lifecycle_smoke

INCLUDEPATH += ..

SOURCES += plugin_lifecycle_smoke.cpp

HEADERS += ../common.h \
           ../commonqtclasses.h \
           ../volinterface.h
