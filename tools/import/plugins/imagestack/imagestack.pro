TEMPLATE = lib

QT += xml

CONFIG += release plugin

TARGET = imagestackplugin

include(../plugins.pri)

INCLUDEPATH += ../../

HEADERS = volumefilemanager.h \
	  imagestackplugin.h

SOURCES = imagestackplugin.cpp \
	  volumefilemanager.cpp

