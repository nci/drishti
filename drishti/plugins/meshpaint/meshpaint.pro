TEMPLATE = lib

DRISHTI_DEFINES = RENDERER NETCDF
include( ../../../drishti.pri )


RESOURCES = meshpaint.qrc

QT += opengl xml network

CONFIG += release plugin

TARGET = meshpaintplugin


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


QMAKE_LIBDIR += ../common \
                /home/ajay/drishtilib/libQGLViewer-2.6.4/QGLViewer \
                /home/ajay/drishtilib/glew-2.1.0/lib
                
LIBS += -lcommon \
	-lQGLViewer-qt5 \
        -lGLEW \
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
	ply.h \
	lookuptable.h
	

SOURCES = meshplugin.cpp \
	meshgenerator.cpp \
	ply.c
