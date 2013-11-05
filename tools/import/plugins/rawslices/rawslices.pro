TEMPLATE = lib
CONFIG += release plugin

TARGET = rawslicesplugin

include(../plugins.pri)

INCLUDEPATH += ../../

FORMS += loadrawdialog.ui

HEADERS = rawslicesplugin.h \
	  loadrawdialog.h

SOURCES = rawslicesplugin.cpp \
	  loadrawdialog.cpp

