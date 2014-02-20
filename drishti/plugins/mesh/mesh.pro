TEMPLATE = lib

DRISHTI_DEFINES = RENDERER NETCDF
include( ../../../drishti.pri )

RESOURCES = mesh.qrc

QT += opengl xml network

CONFIG += release plugin

TARGET = meshplugin


FORMS += ../../propertyeditor.ui

win32 {
  DESTDIR = ../../../5.2.1/renderplugins

  INCLUDEPATH += ../../ ..\..\..\glmedia

  QMAKE_LIBDIR += ..\common ..\..\..\glmedia

  LIBS += common.lib \
	  QGLViewer2.lib \
	  netcdf.lib \
	  glew32.lib \
	  glmedia.lib
}

unix {
!macx {

DESTDIR = ../../../bin/renderplugins

INCLUDEPATH += ../../

QMAKE_LIBDIR += ../common

LIBS += -lcommon \
	-lQGLViewer \
        -lGLEW \
 	-lglut \
	-lGLU

  }
}


macx {
  DESTDIR = ../../../bin/drishti.app/renderplugins

  INCLUDEPATH += ../../

  LIBS += -L../common 

  LIBS += -lcommon \
	-lGLEW \
	-lnetcdf \
	-lnetcdf_c++ \
        -framework QGLViewer \
        -framework GLUT
}

HEADERS = meshplugin.h \
 	meshgenerator.h \
	marchingcubes.h \
	ply.h \
	lookuptable.h
	

SOURCES = meshplugin.cpp \
	meshgenerator.cpp \
	marchingcubes.cpp \
	ply.c
