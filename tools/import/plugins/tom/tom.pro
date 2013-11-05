TEMPLATE = lib
CONFIG += release plugin

TARGET = tomplugin

include(../plugins.pri)

INCLUDEPATH += ../../

HEADERS = tomhead.h \
	  tomplugin.h

SOURCES = tomplugin.cpp

