TEMPLATE = lib

DRISHTI_DEFINES = RENDERER
include( ../../../drishti.pri )

RESOURCES = mesh.qrc

QT += opengl xml network

CONFIG += release plugin

TARGET = meshplugin


FORMS += ../../propertyeditor.ui

win32 {
  DESTDIR = ../../../bin/renderplugins

  LIBS += common.lib \
	  QGLViewer2.lib \
	  glew32.lib \
          opengl32.lib \
          glu32.lib \
          vdb.lib

 contains(Windows_Setup, Win64) {
     message(drishti.exe : Win64 setup)
     INCLUDEPATH += ../../ \
                    ../../../common/src/vdb \
                    ../../../common/src/mesh

     DEFINES += _CRT_SECURE_NO_WARNINGS
     QMAKE_LIBDIR += ..\common \
                     ..\..\..\common\lib     

     INCLUDEPATH += C:\cygwin64\home\acl900\vcpkg\vcpkg\installed\x64-windows\include
     QMAKE_LIBDIR += C:\cygwin64\home\acl900\vcpkg\vcpkg\installed\x64-windows\lib

     ### /std:c++17 added because openvdb requires this
     QMAKE_CXXFLAGS*=/std:c++17
  
     LIBS += Imath-3_1.lib openvdb.lib  
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
        -framework QGLViewer \
        -framework GLUT
}

HEADERS = meshplugin.h \
 	  meshgenerator.h \
	  marchingcubes.h \
	  ply.h \
          lookuptable.h \
          ../../../common/src/mesh/meshtools.h


SOURCES = meshplugin.cpp \
	  meshgenerator.cpp \
	  marchingcubes.cpp \
          ply.c \
          ../../../common/src/mesh/meshtools.cpp
