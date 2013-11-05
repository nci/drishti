TEMPLATE = lib
CONFIG += release plugin

TARGET = grdplugin

include(../plugins.pri)

INCLUDEPATH += ../../

FORMS += loadrawdialog.ui

HEADERS = grdplugin.h \
	  loadrawdialog.h

SOURCES = grdplugin.cpp \
	  loadrawdialog.cpp

