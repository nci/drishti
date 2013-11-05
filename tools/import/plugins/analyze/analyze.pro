TEMPLATE = lib
CONFIG += release plugin

TARGET = analyzeplugin

include(../plugins.pri)

INCLUDEPATH += ../../

HEADERS = analyzeplugin.h

SOURCES = analyzeplugin.cpp

