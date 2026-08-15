TEMPLATE = lib

DRISHTI_DEFINES = RENDERER NETCDF
include( ../../../drishti.pri )


RESOURCES = meshpaint.qrc

QT += opengl xml network

CONFIG += release plugin

TARGET = meshpaintplugin


FORMS += ../../propertyeditor.ui

win32 {
  DESTDIR = $$DRISHTI_RENDER_PLUGIN_DIR

 contains(Windows_Setup, Win64) {
  isEmpty(DRISHTI_PLUGIN_COMMON_LIB_DIR): DRISHTI_PLUGIN_COMMON_LIB_DIR = $$clean_path($$PWD/../common)
  isEmpty(DRISHTI_GLMEDIA_LIBRARY_PATH): DRISHTI_GLMEDIA_LIBRARY_PATH = $$clean_path($$PWD/../../../glmedia-64)

  INCLUDEPATH += ../../ \
                 ../../../common/src \
                 ../../../common/src/widgets
  QMAKE_LIBDIR += $$DRISHTI_PLUGIN_COMMON_LIB_DIR \
                  $$DRISHTI_GLMEDIA_LIBRARY_PATH
  PRE_TARGETDEPS += $$clean_path($$DRISHTI_PLUGIN_COMMON_LIB_DIR/$$DRISHTI_COMMON_LIB)

  LIBS += $$DRISHTI_COMMON_LIB \
	  $$DRISHTI_QGLVIEWER_LIB \
	  $$DRISHTI_GLEW_LIB \
          $$DRISHTI_OPENGL_LIBS \
          $$DRISHTI_LEGACY_NETCDF_LIB \
	  $$DRISHTI_FREEGLUT_LIB

  exists($$clean_path($$DRISHTI_GLMEDIA_LIBRARY_PATH/$$DRISHTI_GLMEDIA_LIB)) {
    LIBS += $$DRISHTI_GLMEDIA_LIB
  }
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
