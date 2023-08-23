TEMPLATE = lib
CONFIG += release plugin

TARGET = scriptsplugin

include(../plugins.pri)

QT += network

INCLUDEPATH += ../../

HEADERS = scriptsplugin.h

SOURCES = scriptsplugin.cpp

