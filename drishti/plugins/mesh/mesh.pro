TEMPLATE = lib

DRISHTI_DEFINES = RENDERER
include( ../../../drishti.pri )

RESOURCES = mesh.qrc

QT += opengl xml network

CONFIG += release plugin

TARGET = meshplugin


FORMS += ../../propertyeditor.ui

win32 {
  DESTDIR = $$DRISHTI_RENDER_PLUGIN_DIR

  contains(Windows_Setup, Win64) {
     message(drishti.exe : Win64 setup)

     DEFINES += _CRT_SECURE_NO_WARNINGS

     isEmpty(DRISHTI_PLUGIN_COMMON_LIB_DIR): DRISHTI_PLUGIN_COMMON_LIB_DIR = $$clean_path($$PWD/../common)
     isEmpty(DRISHTI_COMMON_LIB_DIR): DRISHTI_COMMON_LIB_DIR = $$clean_path($$PWD/../../../common/lib)

     INCLUDEPATH += ../../ \
                    ../../../common/src/vdb \
                    ../../../common/src/mesh
                    
     QMAKE_LIBDIR += $$DRISHTI_PLUGIN_COMMON_LIB_DIR \
                     $$DRISHTI_COMMON_LIB_DIR
                     
     PRE_TARGETDEPS += $$clean_path($$DRISHTI_PLUGIN_COMMON_LIB_DIR/$$DRISHTI_COMMON_LIB) \
                       $$clean_path($$DRISHTI_COMMON_LIB_DIR/$$DRISHTI_VDB_LIB)


     ### /std:c++17 added because openvdb requires this
     QMAKE_CXXFLAGS*=/std:c++17
  
     LIBS += $$DRISHTI_COMMON_LIB \
             $$DRISHTI_QGLVIEWER_LIB \
             $$DRISHTI_GLEW_LIB \
             $$DRISHTI_OPENGL_LIBS \
             $$DRISHTI_VDB_LIB \
             $$DRISHTI_IMATH_LIB \
             $$DRISHTI_OPENVDB_LIB \
             $$DRISHTI_GMSH_LIB
 }
}

unix {
!macx {

DESTDIR = ../../../bin/renderplugins

INCLUDEPATH += ../../ \
                /home/acl900/drishtilib/openvdb/openvdb \
                /home/acl900/drishtilib/openvdb/build/openvdb/openvdb \
                /home/acl900/drishtilib/openvdb/build/openvdb/openvdb/openvdb \
                /home/acl900/drishtilib/oneTBB/include \
                ../../../common/src/vdb \
                ../../../common/src/mesh \
                ../../../common/src/widgets

QMAKE_LIBDIR += ../common \
                ../../../common/lib \
                /home/ajay/drishtilib/libQGLViewer-2.6.4/QGLViewer \
                /home/ajay/drishtilib/glew-2.1.0/lib \
                /home/acl900/drishtilib/openvdb/build/openvdb/openvdb \
                /home/acl900/drishtilib/oneTBB/build/gnu_11.3_cxx11_64_relwithdebinfo
                
LIBS += -lcommon \
	-lQGLViewer-qt5 \
        -lGLEW -lGLU \
        -lvdb -lopenvdb -lImath -ltbb
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
          lookuptable.h \
          ../../../common/src/mesh/meshtools.h


SOURCES = meshplugin.cpp \
	  meshgenerator.cpp \
          ../../../common/src/mesh/meshtools.cpp
