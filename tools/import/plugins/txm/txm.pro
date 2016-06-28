TEMPLATE = lib
CONFIG += release plugin

TARGET = txmplugin

include(../plugins.pri)

INCLUDEPATH += ../../

HEADERS = txmplugin.h \
	  pole.h

SOURCES = txmplugin.cpp \
	  pole.cpp

