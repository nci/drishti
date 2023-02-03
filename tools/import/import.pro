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
  INCLUDEPATH += C:\cygwin64\home\acl900\vcpkg\vcpkg\installed\x64-windows\include
  QMAKE_LIBDIR += C:\cygwin64\home\acl900\vcpkg\vcpkg\installed\x64-windows\lib

  # /std:c++17 added because openvdb requires this
  QMAKE_CXXFLAGS*=/std:c++17
  
  LIBS += Imath-3_1.lib openvdb.lib  
  
  RC_ICONS += images/drishtiimport.ico
}
     

FORMS += remapwidget.ui \
	 savepvldialog.ui \
	 drishtiimport.ui \
	 fileslistdialog.ui

# Input
HEADERS += global.h \
	   common.h \
	   staticfunctions.h \
	   fileslistdialog.h \
	   remapwidget.h \
           remaphistogramline.h \
           remaphistogramwidget.h \
	   remapimage.h \
	   gradienteditor.h \
	   gradienteditorwidget.h \
	   dcolordialog.h \
	   dcolorwheel.h \
	   drishtiimport.h \
	   myslider.h \
	   raw2pvl.h \
	   savepvldialog.h \
	   volumefilemanager.h \
	   volumedata.h \
	   volinterface.h \
	   lookuptable.h

SOURCES += global.cpp \
	   staticfunctions.cpp \
	   fileslistdialog.cpp \
	   main.cpp \
           remapwidget.cpp \
           remaphistogramline.cpp \
           remaphistogramwidget.cpp \
	   remapimage.cpp \
	   gradienteditor.cpp \
	   gradienteditorwidget.cpp \
	   dcolordialog.cpp \
	   dcolorwheel.cpp \
	   drishtiimport.cpp \
	   myslider.cpp \
	   raw2pvl.cpp \
	   savepvldialog.cpp \
	   volumedata.cpp \
	   volumefilemanager.cpp

