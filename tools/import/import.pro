TEMPLATE = app

DRISHTI_DEFINES = IMPORT
include(../../drishti.pri )

RESOURCES = import.qrc

DEPENDPATH += .

QT += widgets core gui xml concurrent

CONFIG += release

TARGET = drishtiimport
VERSION = 4.0.4.9

DESTDIR = ../../bin

win32 {
  DESTDIR = $$DRISHTI_BIN_DIR
  QMAKE_LFLAGS += /MANIFESTINPUT:$$shell_path($$PWD/../../windows_long_path.manifest)
  isEmpty(DRISHTI_COMMON_LIB_DIR): DRISHTI_COMMON_LIB_DIR = $$clean_path($$PWD/../../common/lib)

  INCLUDEPATH += ../../common/src/vdb \
                 ../../common/src/widgets \
                 ../../common/src/pybind \                 
                 ../../common/src/mesh
  INCLUDEPATH += $$VCPKG_INCLUDE_PATH

  QMAKE_LIBDIR += $$DRISHTI_COMMON_LIB_DIR
  QMAKE_LIBDIR += $$VCPKG_LIBRARY_PATH

  # /std:c++17 added because openvdb requires this
  QMAKE_CXXFLAGS*=/std:c++17
  
  LIBS += $$DRISHTI_IMATH_LIB \
          $$DRISHTI_OPENVDB_LIB \
          $$DRISHTI_VDB_LIB \
          $$DRISHTI_GMSH_LIB \
          tiff.lib

  INCLUDEPATH += $$clean_path($$DRISHTI_PYTHON_ROOT/include)
  QMAKE_LIBDIR += $$clean_path($$DRISHTI_PYTHON_ROOT/libs)
  LIBS += $$DRISHTI_PYTHON_LIB
  

  RC_ICONS += images/drishtiimport.ico
}

unix {
!macx {
  INCLUDEPATH += ../../common/src/vdb \
                 ../../common/src/widgets \
                 ../../common/src/pybind \
                 ../../common/src/mesh \
                 /home/acl900/drishtilib/openvdb/openvdb \
                 /home/acl900/drishtilib/openvdb/build/openvdb/openvdb \
                 /home/acl900/drishtilib/openvdb/build/openvdb/openvdb/openvdb \
                 /home/acl900/drishtilib/oneTBB/include

  QMAKE_LIBDIR += ../../common/lib \
                   /home/acl900/drishtilib/openvdb/build/openvdb/openvdb \
                   /home/acl900/drishtilib/oneTBB/build/gnu_11.3_cxx11_64_relwithdebinfo


  LIBS += -lvdb -lopenvdb -ltbb -lImath -ltiff
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
	    importmemoryadmission.h \
	    metaimagepathutils.h \
	    pluginoperationstatus.h \
	    tiffinputrouting.h \
	    tiffpagevalidation.h \
	    volumepluginvalidation.h \
	    volumevaluemapping.h \
	    savepvldialog.h \
	    volumefilemanager.h \
	    volumedata.h \ 
	    volinterface.h \
 	    lookuptable.h \
      scriptsplugin.h \
      ../../common/src/pybind/pythonengine.h \
      ../../common/src/widgets/streamredirect.h \
      ../../common/src/widgets/propertyeditor.h \
      ../../common/src/widgets/dcolordialog.h \
      ../../common/src/widgets/dcolorwheel.h \
      ../../common/src/widgets/gradienteditor.h \
	    ../../common/src/widgets/gradienteditorwidget.h \
      ../../common/src/mesh/meshtools.h

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
	    importmemoryadmission.cpp \
	    tiffinputrouting.cpp \
	    tiffpagevalidation.cpp \
	    volumepluginvalidation.cpp \
	    savepvldialog.cpp \
	    volumedata.cpp \
	    volumefilemanager.cpp \
      scriptsplugin.cpp \
      ../../common/src/pybind/pythonengine.cpp \
      ../../common/src/widgets/propertyeditor.cpp \
      ../../common/src/widgets/dcolordialog.cpp \
	    ../../common/src/widgets/dcolorwheel.cpp \
	    ../../common/src/widgets/gradienteditor.cpp \
	    ../../common/src/widgets/gradienteditorwidget.cpp \
      ../../common/src/mesh/meshtools.cpp

