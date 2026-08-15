TEMPLATE = lib

DRISHTI_DEFINES = RENDERER NETCDF
include( ../../../drishti.pri )

QT += opengl xml network

CONFIG += release staticlib
CONFIG += no_batch

TARGET = common

isEmpty(DRISHTI_PLUGIN_COMMON_LIB_DIR): DRISHTI_PLUGIN_COMMON_LIB_DIR = $$clean_path($$PWD)
DESTDIR = $$DRISHTI_PLUGIN_COMMON_LIB_DIR

FORMS += ../../mainwindow.ui \
         ../../brickswidget.ui \
         ../../captiondialog.ui \
         ../../directionvectorwidget.ui \
         ../../fileslistdialog.ui \
         ../../lightingwidget.ui \
         ../../load2volumes.ui \
         ../../load3volumes.ui \
         ../../load4volumes.ui \
         ../../preferenceswidget.ui \
         ../../propertyeditor.ui \
         ../../profileviewer.ui \
         ../../volumeinformation.ui \
         ../../raycastmenu.ui \
         ../../../common/src/widgets/saveimgseq.ui \
         ../../../common/src/widgets/savemovie.ui

win32 {
  TARGET = $$section(DRISHTI_COMMON_LIB, ., 0, 0)

 contains(Windows_Setup, Win64) {
	message(drishti.exe : Win64 setup)
	
	DEFINES += _CRT_SECURE_NO_WARNINGS
	INCLUDEPATH +=../../ \
                  ..\..\..\common\src\widgets	

    LIBS += $$DRISHTI_QGLVIEWER_LIB \
			$$DRISHTI_LEGACY_NETCDF_LIB \
			$$DRISHTI_GLEW_LIB
 }
}

unix {
  !macx {

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
	..\..\..\common\src\mesh\binaryplywriter.h \
	..\..\..\common\src\pvlmanifest.h \
	..\..\..\common\src\memoryreservation.h \
	..\..\..\common\src\recoveryjournal.h
	

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
	../../../common/src/mesh/binaryplywriter.cpp \
	../../../common/src/pvlmanifest.cpp \
	../../../common/src/memoryreservation.cpp \
	../../../common/src/recoveryjournal.cpp
