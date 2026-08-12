TEMPLATE = lib
CONFIG += release plugin

TARGET = rawplugin

include(../plugins.pri)

INCLUDEPATH += ../../

FORMS += loadrawdialog.ui

HEADERS = rawplugin.h \
	  loadrawdialog.h \
	  ../rawfileutils.h

SOURCES = rawplugin.cpp \
	  loadrawdialog.cpp

