QT += widgets

CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = plugin_empty_input_smoke

INCLUDEPATH += ..

SOURCES += plugin_empty_input_smoke.cpp

HEADERS += ../common.h \
           ../commonqtclasses.h \
           ../volinterface.h
