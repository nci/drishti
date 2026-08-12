TEMPLATE = lib
CONFIG += release plugin

TARGET = rawslicesplugin

include(../plugins.pri)

INCLUDEPATH += ../../

FORMS += loadrawdialog.ui

HEADERS = rawslicesplugin.h \
	  loadrawdialog.h \
	  ../rawfileutils.h

SOURCES = rawslicesplugin.cpp \
	  loadrawdialog.cpp

