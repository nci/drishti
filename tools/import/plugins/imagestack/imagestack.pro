TEMPLATE = lib

QT += xml concurrent

CONFIG += release plugin

TARGET = imagestackplugin

include(../plugins.pri)

INCLUDEPATH += ../../

HEADERS = imagestackplugin.h \
	  imagestackpixelconversion.h \
	  ../../tiffinputrouting.h \
	  ../../volumefilemanager.h \
	  ../../importmemoryadmission.h

SOURCES = imagestackplugin.cpp \
	  ../../volumefilemanager.cpp \
	  ../../importmemoryadmission.cpp

