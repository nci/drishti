TEMPLATE = app

DRISHTI_DEFINES = IMPORT
include(../../drishti.pri )

RESOURCES = import.qrc

DEPENDPATH += .

QT += widgets core gui xml

CONFIG += release

TARGET = drishtiimport

DESTDIR = ../../bin

win32
{
  INCLUDEPATH += C:\cygwin64\home\acl900\vcpkg\vcpkg\installed\x64-windows\include \
                 ../../common/src/vdb \
                 ../../common/src/widgets \
                 ../../common/src/mesh

  QMAKE_LIBDIR += C:\cygwin64\home\acl900\vcpkg\vcpkg\installed\x64-windows\lib \
                 ..\..\common\lib     

  # /std:c++17 added because openvdb requires this
  QMAKE_CXXFLAGS*=/std:c++17
  
  LIBS += Imath-3_1.lib openvdb.lib vdb.lib
  
  RC_ICONS += images/drishtiimport.ico
}
     

FORMS += remapwidget.ui \
	 savepvldialog.ui \
	 drishtiimport.ui \
         fileslistdialog.ui \
         ../../common/src/widgets/propertyeditor.ui

# Input
HEADERS += global.h \
	   common.h \
	   staticfunctions.h \
	   fileslistdialog.h \
	   remapwidget.h \
           remaphistogramline.h \
           remaphistogramwidget.h \
	   remapimage.h \
	   drishtiimport.h \
	   myslider.h \
	   raw2pvl.h \
	   savepvldialog.h \
	   volumefilemanager.h \
	   volumedata.h \
	   volinterface.h \
 	   lookuptable.h \
           ../../common/src/widgets/propertyeditor.h \
           ../../common/src/widgets/dcolordialog.h \
           ../../common/src/widgets/dcolorwheel.h \
	   ../../common/src/widgets/gradienteditor.h \
	   ../../common/src/widgets/gradienteditorwidget.h \
           ../../common/src/mesh/meshtools.h \
           ../../common/src/mesh/ply.h

SOURCES += global.cpp \
	   staticfunctions.cpp \
	   fileslistdialog.cpp \
	   main.cpp \
           remapwidget.cpp \
           remaphistogramline.cpp \
           remaphistogramwidget.cpp \
	   remapimage.cpp \
	   drishtiimport.cpp \
	   myslider.cpp \
	   raw2pvl.cpp \
	   savepvldialog.cpp \
	   volumedata.cpp \
	   volumefilemanager.cpp \
           ../../common/src/widgets/propertyeditor.cpp \
           ../../common/src/widgets/dcolordialog.cpp \
	   ../../common/src/widgets/dcolorwheel.cpp \
	   ../../common/src/widgets/gradienteditor.cpp \
	   ../../common/src/widgets/gradienteditorwidget.cpp \
           ../../common/src/mesh/meshtools.cpp \
           ../../common/src/mesh/ply.c

