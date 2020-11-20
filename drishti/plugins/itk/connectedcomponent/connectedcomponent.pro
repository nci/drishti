TEMPLATE = lib

DRISHTI_DEFINES = RENDERER ITK
include( ../../../../drishti.pri )

QT += opengl xml network

CONFIG += release plugin

TARGET = connectedcomponentplugin

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

HEADERS = connectedcomponent.h \
	label.h
	

SOURCES = connectedcomponent.cpp \
	label.cpp
