DRISHTI_DEFINES = RENDERER

TEMPLATE = app

RESOURCES = paint.qrc

TARGET = 
DEPENDPATH += .

QT += opengl network
QT += widgets core gui xml concurrent

CONFIG += release

DESTDIR = ../../bin

TARGET = drishtipaint
VERSION = 4.0.4.9

INCLUDEPATH += graphcut

include( ../../drishti.pri )

# Input
FORMS += drishtipaint.ui viewermenu.ui \
	graphcutmenu.ui curvesmenu.ui \
	pywidgetmenu.ui \
        ../../common/src/widgets/propertyeditor.ui

#----------------------------------------------------------------
# Windows setup for 64-bit system
#contains(Windows_Setup, Win64) {
  win32 {
         DESTDIR = $$DRISHTI_BIN_DIR
         QMAKE_LFLAGS += /MANIFESTINPUT:$$shell_path($$PWD/../../windows_long_path.manifest)
         isEmpty(DRISHTI_COMMON_LIB_DIR): DRISHTI_COMMON_LIB_DIR = $$clean_path($$PWD/../../common/lib)

         RC_ICONS += images/drishtipaint.ico

         INCLUDEPATH += ../../common/src/vdb \
                        ../../common/src/widgets \
                        ../../common/src/mesh \
                        ..\..\common\src\videoencoder

         INCLUDEPATH += $$VCPKG_INCLUDE_PATH

         QMAKE_LIBDIR += $$DRISHTI_COMMON_LIB_DIR
	
         QMAKE_LIBDIR += $$VCPKG_LIBRARY_PATH

         PRE_TARGETDEPS += $$clean_path($$DRISHTI_COMMON_LIB_DIR/$$DRISHTI_VDB_LIB)

         LIBS += $$DRISHTI_QGLVIEWER_LIB \
                 $$DRISHTI_GLEW_LIB \
                 $$DRISHTI_BLOSC_LIB \
                 $$DRISHTI_OPENGL_LIBS
         LIBS += $$DRISHTI_IMATH_LIB \
                 $$DRISHTI_OPENVDB_LIB \
                 $$DRISHTI_VDB_LIB \
                 $$DRISHTI_GMSH_LIB

         # Set list of required FFmpeg libraries
         LIBS += $$DRISHTI_FFMPEG_LIBS
         

         ## /std:c++17 added because openvdb requires this
         QMAKE_CXXFLAGS*=/std:c++17
         }
#}

unix {
 !macx {
    INCLUDEPATH += /home/acl900/drishtilib/c-blosc/blosc
                        
    QMAKE_LIBDIR += /home/acl900/drishtilib/c-blosc/build/blosc

    INCLUDEPATH += ../../common/src/vdb \
                   ../../common/src/widgets \
                   ../../common/src/mesh \
                   /home/acl900/drishtilib/openvdb/openvdb \
                   /home/acl900/drishtilib/openvdb/build/openvdb/openvdb \
                   /home/acl900/drishtilib/openvdb/build/openvdb/openvdb/openvdb \
                   /home/acl900/drishtilib/oneTBB/include \
                  ..\..\common\src\videoencoder

    QMAKE_LIBDIR += ../../common/lib \
                   /home/acl900/drishtilib/openvdb/build/openvdb/openvdb \
                   /home/acl900/drishtilib/oneTBB/build/gnu_11.3_cxx11_64_relwithdebinfo

    
    LIBS += -lblosc -lvdb -lopenvdb -ltbb -lImath

    # Set list of required FFmpeg libraries
    LIBS += -lavutil \
            -lavcodec \
            -lavformat \
            -lswresample \
            -lswscale 
    }
 }

#----------------------------------------------------------------
# MacOSX setup
macx {
    LIBS += -lGLEW -framework QGLViewer -framework GLUT
}
#----------------------------------------------------------------

HEADERS += connectviewer.h \
        commonqtclasses.h \
        binarydistancetransform.h \
	boundingbox.h \
        drishtipaint.h \
        cc3d.h \
        crops.h \
        cropobject.h \
        cropgrabber.h \
        cropshaderfactory.h \
	curvegroup.h \
        clipinformation.h \
        clipplane.h \
	clipobject.h \
        clipgrabber.h \
        colormaps.h \
        slices.h \
	imagewidget.h \
        curves.h \
        curveswidget.h \
        global.h \
        handleexternal.h \
        livewire.h \
	mybitarray.h \
	myslider.h \
	mymanipulatedframe.h \
 	morphcurve.h \
 	morphslice.h \
	splineeditor.h \
	splineeditorwidget.h \
	splineinformation.h \
	splinetransferfunction.h \
	staticfunctions.h \
	transferfunctioncontainer.h \
	transferfunctioneditorwidget.h \
	transferfunctionmanager.h \
	tagcoloreditor.h \
	coloreditor.h \
	opacityeditor.h \
	viewer.h \
	../../framebufferbudget.h \
	viewer3d.h \
	volume.h \
	volumefilemanager.h \
	slabsavetransaction.h \
        sliceorderutils.h \
        volumeinformation.h \
        maskimportutils.h \
        volumemask.h \
        volumemeasure.h \
        volumeoperations.h \
        geometryobjects.h \
	graphcut/graph.h \
	graphcut/graphcut.h \
	graphcut/block.h \
	graphcut/point.h \
	lookuptable.h \
	showhelp.h \
        structuringelement.h\
        getmemorysize.h \
	popupslider.h \
	shaderfactory.h \
	remaphistogramline.h \
	remaphistogramwidget.h \
        filehandler.h \
        checkpointhandler.h \
        pywidget.h \
        pywidgetmenu.h \
        ../../common/src/widgets/propertyeditor.h \
        ../../common/src/widgets/dcolordialog.h \
        ../../common/src/widgets/dcolorwheel.h \
	../../common/src/widgets/gradienteditor.h \
        ../../common/src/widgets/gradienteditorwidget.h \
        ../../common/src/mesh/meshtools.h \
        ../../common/src/videoencoder/videoencoder.h
        
SOURCES += drishtipaint.cpp \
	main.cpp \
        binarydistancetransform.cpp \
	boundingbox.cpp \
        crops.cpp \
        cropobject.cpp \
        cropgrabber.cpp \
        cropshaderfactory.cpp \
	curvegroup.cpp \
        clipinformation.cpp \
        clipplane.cpp \
	clipobject.cpp \
	clipgrabber.cpp \
        colormaps.cpp \
	slices.cpp \
	imagewidget.cpp \
        curves.cpp \
	curveswidget.cpp \
	global.cpp \
        handleexternal.cpp \
        livewire.cpp \
	mybitarray.cpp \
	myslider.cpp \
	mymanipulatedframe.cpp \
 	morphcurve.cpp \
 	morphslice.cpp \
	splineeditor.cpp \
	splineeditorwidget.cpp \
	splineinformation.cpp \
	splinetransferfunction.cpp \
	staticfunctions.cpp \
	transferfunctioncontainer.cpp \
	transferfunctioneditorwidget.cpp \
	transferfunctionmanager.cpp \
	tagcoloreditor.cpp \
	coloreditor.cpp \
	opacityeditor.cpp \
	viewer.cpp \
	viewer3d.cpp \
	volume.cpp \
	volumefilemanager.cpp \
	slabsavetransaction.cpp \
        sliceorderutils.cpp \
        volumeinformation.cpp \
        maskimportutils.cpp \
        volumemask.cpp \
        volumemeasure.cpp \
	volumeoperations.cpp \
        geometryobjects.cpp \
	graphcut/graph.cpp \
	graphcut/graphcut.cpp \
	showhelp.cpp \
        structuringelement.cpp\
	getmemorysize.cpp \
	popupslider.cpp \
	shaderfactory.cpp \
	remaphistogramline.cpp \
	remaphistogramwidget.cpp \
        filehandler.cpp \
        checkpointhandler.cpp \
        pywidget.cpp \
        pywidgetmenu.cpp \
        ../../common/src/widgets/propertyeditor.cpp \
        ../../common/src/widgets/dcolordialog.cpp \
	../../common/src/widgets/dcolorwheel.cpp \
	../../common/src/widgets/gradienteditor.cpp \
	../../common/src/widgets/gradienteditorwidget.cpp \
        ../../common/src/mesh/meshtools.cpp \
        ../../common/src/videoencoder/videoencoder.cpp
