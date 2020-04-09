TEMPLATE = lib

DRISHTI_DEFINES = RENDERER NETCDF
include( ../../../drishti.pri )


RESOURCES = meshsimplify.qrc

QT += opengl xml network

CONFIG += release plugin

TARGET = meshsimplifyplugin


FORMS += ../../propertyeditor.ui

win32 {
  DESTDIR = ../../../bin/renderplugins

  LIBS += common.lib \
	  QGLViewer2.lib \
	  glew32.lib \
	  glmedia.lib \
          opengl32.lib \
          glu32.lib

 contains(Windows_Setup, Win64) {
  INCLUDEPATH += ../../
  QMAKE_LIBDIR += ..\common ..\..\..\glmedia-64
  LIBS += netcdfcpp.lib \
	  freeglut.lib
 }
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
 	meshsimplify.h \
	ply.h \
        lookuptable.h \
        simplify.h
	

SOURCES = meshplugin.cpp \
	meshsimplify.cpp \
	ply.c
