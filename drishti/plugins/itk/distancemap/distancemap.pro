TEMPLATE = lib

DRISHTI_DEFINES = RENDERER ITK
include( ../../../../drishti.pri )

QT += opengl xml network

CONFIG += release plugin

TARGET = distancemapplugin

FORMS += ../../../propertyeditor.ui


include(../plugin.itk)

win32 {
  DESTDIR = ../../../../bin/renderplugins/ITK
}
unix {
 !macx {
  DESTDIR = ../../../../bin/renderplugins/ITK

  QMAKE_LFLAGS += "-Wl,-rpath=\'\$${ORIGIN}/../../ITK\'"
  QMAKE_LFLAGS += "-Wl,-rpath=\'\$${ORIGIN}/../../sharedlibs\'"
 }
}
 macx {
  DESTDIR = ../../../../bin/drishti.app/renderplugins/ITK
}

HEADERS = distancemap.h \
	filter.h
	

SOURCES = distancemap.cpp \
	filter.cpp
