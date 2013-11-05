TEMPLATE = lib

RESOURCES = mesh.qrc

QT += opengl xml network

CONFIG += release plugin

TARGET = meshplugin


FORMS += ../../propertyeditor.ui

win32 {
  DESTDIR = ../../../bin/renderplugins

  INCLUDEPATH +=  . \
		  ../../ \
		  ..\..\..\glmedia \
		  c:\Qt\include \
		  c:\drishtilib\netcdf\include \
		  c:\drishtilib \
		  c:\drishtilib\glew-1.5.4\include

  QMAKE_LIBDIR += ..\common \
	          ..\..\..\glmedia \
		  c:\Qt\lib \
		  c:\drishtilib\netcdf\lib \
		  c:\drishtilib\GL \
		  c:\drishtilib\glew-1.5.4\lib


  LIBS += common.lib \
	  QGLViewer2.lib \
	  netcdf.lib \
	  glew32.lib \
	  glmedia.lib
}

unix {
!macx {

DESTDIR = ../../../bin/renderplugins

INCLUDEPATH += ../../ \

QMAKE_LIBDIR += ../common /usr/lib /usr/lib/x86_64-linux-gnu

LIBS += -lcommon \
	-lQGLViewer \
        -lGLEW \
 	-lglut \
	-lGLU

  }
}


macx {
  DESTDIR = ../../../bin/drishti.app/renderplugins

  INCLUDEPATH += . \
                 ../../ \
	         ../../../../Library/Frameworks/QGLViewer.framework/Headers \
	         /usr/local/include 
	
  LIBS += -L../common -L/usr/local/lib -F../../../../Library/Frameworks -L../../../../Library/Frameworks
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
