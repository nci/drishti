TEMPLATE = lib

include( ../../../version.pri )

QT += opengl xml network

CONFIG += release staticlib

TARGET = common

FORMS += ../../propertyeditor.ui

win32 {
  DESTDIR = ../common

  INCLUDEPATH +=  . \
		  ../../ \
		  ..\..\..\glmedia \
		  c:\Qt\include \
		  c:\drishtilib\netcdf\include \
		  c:\drishtilib \
		  c:\drishtilib\glew-1.5.4\include

  QMAKE_LIBDIR += ..\..\..\glmedia \
		  c:\Qt\lib \
		  c:\drishtilib\netcdf\lib \
		  c:\drishtilib\GL \
		  c:\drishtilib\glew-1.5.4\lib


  LIBS += QGLViewer2.lib \
	  netcdf.lib \
	  glew32.lib \
	  glmedia.lib
}

macx {
  DESTDIR = ../common

  INCLUDEPATH += . \
                 ../../ \
	         ../../../../Library/Frameworks/QGLViewer.framework/Headers \
	         /usr/local/include 
	
  LIBS += -L/usr/local/lib -F../../../../Library/Frameworks -L../../../../Library/Frameworks
  LIBS += \
	-lGLEW \
	-lnetcdf \
	-lnetcdf_c++ \
        -framework QGLViewer \
        -framework GLUT
}

HEADERS = ..\..\mainwindowui.h \
	..\..\cropobject.h \	 
	..\..\pathobject.h \	 
	..\..\dcolordialog.h \
	..\..\dcolorwheel.h \
	..\..\propertyeditor.h \
	..\..\staticfunctions.h \
	..\..\volumefilemanager.h \
	..\..\volumeinformation.h \
	..\..\gradienteditorwidget.h \
	..\..\gradienteditor.h \
	..\..\classes.h \
	..\..\matrix.h \
	..\..\global.h
	

SOURCES = ..\..\mainwindowui.cpp \
	..\..\cropobject.cpp \	 
	..\..\pathobject.cpp \	 
	..\..\dcolordialog.cpp \
	..\..\dcolorwheel.cpp \
	..\..\propertyeditor.cpp \
	..\..\staticfunctions.cpp \
	..\..\volumefilemanager.cpp \
	..\..\volumeinformation.cpp \
	..\..\gradienteditorwidget.cpp \
	..\..\gradienteditor.cpp \
	..\..\classes.cpp \
	..\..\matrix.cpp \
	..\..\global.cpp
