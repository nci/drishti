TEMPLATE = app

DRISHTI_DEFINES = IMPORT
include(../../drishti.pri )

RESOURCES = import.qrc

DEPENDPATH += .

QT += widgets core gui xml concurrent

CONFIG += release

TARGET = drishtiimport

DESTDIR = ../../bin

win32 {
  INCLUDEPATH += ../../common/src/vdb \
                 ../../common/src/widgets \
                 ../../common/src/mesh
  INCLUDEPATH += $$GMSH_INCLUDE_PATH
  INCLUDEPATH += $$VCPKG_INCLUDE_PATH

  QMAKE_LIBDIR += ..\..\common\lib     
  QMAKE_LIBDIR += $$GMSH_LIBRARY_PATH
  QMAKE_LIBDIR += $$VCPKG_LIBRARY_PATH

  # /std:c++17 added because openvdb requires this
  QMAKE_CXXFLAGS*=/std:c++17
  
  LIBS += Imath-3_1.lib openvdb.lib vdb.lib gmsh.lib
  
  RC_ICONS += images/drishtiimport.ico
}

unix {
!macx {
  INCLUDEPATH += ../../common/src/vdb \
                 ../../common/src/widgets \
                 ../../common/src/mesh \
                 /home/acl900/drishtilib/openvdb/openvdb \
                 /home/acl900/drishtilib/openvdb/build/openvdb/openvdb \
                 /home/acl900/drishtilib/openvdb/build/openvdb/openvdb/openvdb \
                 /home/acl900/drishtilib/oneTBB/include

  QMAKE_LIBDIR += ../../common/lib \
                   /home/acl900/drishtilib/openvdb/build/openvdb/openvdb \
                   /home/acl900/drishtilib/oneTBB/build/gnu_11.3_cxx11_64_relwithdebinfo


  LIBS += -lvdb -lopenvdb -ltbb -lImath
  }
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

