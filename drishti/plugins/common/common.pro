TEMPLATE = lib

DRISHTI_DEFINES = RENDERER NETCDF
include( ../../../drishti.pri )

QT += opengl xml network

CONFIG += release staticlib
CONFIG += no_batch

TARGET = common

FORMS += ../../propertyeditor.ui

win32 {
  DESTDIR = ../common

 contains(Windows_Setup, Win64) {
  message(drishti.exe : Win64 setup)
  DEFINES += _CRT_SECURE_NO_WARNINGS
  INCLUDEPATH += ../../ \
                 ..\..\..\common\src\widgets
  LIBS += QGLViewer2.lib \
	  netcdfcpp.lib \
	  glew32.lib
 }
}

unix {
  !macx {

  DESTDIR = ../common

  INCLUDEPATH += ../../ \
                 ..\..\..\common\src\widgets

  QMAKE_LIBDIR += /home/ajay/drishtilib/libQGLViewer-2.6.4/QGLViewer \
                  /home/ajay/drishtilib/glew-2.1.0/lib \

  LIBS += -lQGLViewer-qt5 \
          -lGLEW \
	  -lGLU

  }
}
  
macx {
  DESTDIR = ../common

  INCLUDEPATH += ../../

  LIBS += -lGLEW \
	  -lnetcdf \
	  -lnetcdf_c++ \
          -framework QGLViewer \
          -framework GLUT
}

HEADERS = ..\..\mainwindowui.h \
	..\..\cropobject.h \	 
	..\..\pathobject.h \	 
	..\..\..\common\src\widgets\dcolordialog.h \
	..\..\..\common\src\widgets\dcolorwheel.h \
	..\..\propertyeditor.h \
	..\..\staticfunctions.h \
	..\..\volumefilemanager.h \
	..\..\volumeinformation.h \
	..\..\gradienteditorwidget.h \
	..\..\gradienteditor.h \
	..\..\classes.h \
	..\..\matrix.h \
	..\..\global.h \
	..\..\ply.h
	

SOURCES = ../../mainwindowui.cpp \
	../../cropobject.cpp \	 
	../../pathobject.cpp \	 
	../../../common/src/widgets/dcolordialog.cpp \
	../../../common/src/widgets/dcolorwheel.cpp \
	../../propertyeditor.cpp \
	../../staticfunctions.cpp \
	../../volumefilemanager.cpp \
	../../volumeinformation.cpp \
	../../gradienteditorwidget.cpp \
	../../gradienteditor.cpp \
	../../classes.cpp \
	../../matrix.cpp \
	../../global.cpp \
	../../ply.c
