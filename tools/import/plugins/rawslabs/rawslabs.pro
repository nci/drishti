TEMPLATE = lib
CONFIG += release plugin

TARGET = rawslabsplugin

include(../plugins.pri)

INCLUDEPATH += ../../

FORMS += loadrawdialog.ui

HEADERS = rawslabsplugin.h \
	  loadrawdialog.h

SOURCES = rawslabsplugin.cpp \
	  loadrawdialog.cpp

