TEMPLATE = lib
CONFIG += release plugin

TARGET = vgiplugin

include(../plugins.pri)

INCLUDEPATH += ../../

HEADERS = vgiplugin.h

SOURCES = vgiplugin.cpp

