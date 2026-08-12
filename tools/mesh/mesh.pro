DRISHTI_DEFINES = RENDERER NETCDF

RESOURCES = mesh.qrc

QT += opengl widgets core gui xml network
QT += multimedia multimediawidgets

CONFIG += release
CONFIG += no_batch

TRANSLATIONS = chinese.ts

FORMS += mainwindow.ui \
	 brickswidget.ui \
	 captiondialog.ui \
	 lightingwidget.ui \
	 globalwidget.ui \
         propertyeditor.ui \
         meshinfowidget.ui \
	 ../../common/src/widgets/saveimgseq.ui \
	 ../../common/src/widgets/savemovie.ui


TEMPLATE = app

DESTDIR = ../../bin

TARGET = drishtimesh
VERSION = 4.0.4.9
DEPENDPATH += .

include( ../../drishti.pri )

win32 {
    DESTDIR = $$DRISHTI_BIN_DIR
    RC_ICONS += images/drishtimesh.ico
    QMAKE_LFLAGS += /MANIFESTINPUT:$$shell_path($$PWD/../../windows_long_path.manifest)

  OPENVR_VERSION = 1.14.15

  contains(Windows_Setup, Win64) {
    DEFINES += _CRT_SECURE_NO_WARNINGS

    QMAKE_CXXFLAGS += -Ob3
    QMAKE_CXXFLAGS += -GL
    QMAKE_CXXFLAGS += -Gw

    QMAKE_LFLAGS += /OPT:ICF /LTCG
    
    INCLUDEPATH += ..\..\common\src\videoencoder

    INCLUDEPATH += $$VCPKG_INCLUDE_PATH

    QMAKE_LIBDIR += $$VCPKG_LIBRARY_PATH

                   
    LIBS += $$DRISHTI_QGLVIEWER_LIB \
            $$DRISHTI_GLEW_LIB \
            $$DRISHTI_OPENGL_LIBS \
            $$DRISHTI_ASSIMP_LIB \
            iphlpapi.lib

    # Set list of required FFmpeg libraries
    LIBS += $$DRISHTI_FFMPEG_LIBS
  }
}


unix {
!macx {
  INCLUDEPATH +=  /home/acl900/drishtilib/assimp-5.0.1/include \
                  /home/acl900/drishtilib/assimp-5.0.1/build/include \
                  ..\..\common\src\videoencoder


  QMAKE_LIBDIR += /home/acl900/drishtilib/assimp-5.0.1/libs 

  LIBS += -lGLU

  # Set list of required FFmpeg libraries
  LIBS += -lavutil \
          -lavcodec \
          -lavformat \
          -lswresample \
          -lswscale 
  }
}



# Input
HEADERS += boundingbox.h \
	   brickinformation.h \
	   bricks.h \
	   brickswidget.h \
           camerapathnode.h \
	   captions.h \
	   captiondialog.h \
	   captiongrabber.h \
	   captionobject.h \
           clipinformation.h \
           clipplane.h \
	   clipobject.h \
	   clipgrabber.h \
	   coloreditor.h \
	   connectbricks.h \
	   connectbrickswidget.h \	
	   connectclipplanes.h \
	   connectgeometryobjects.h \
	   connecthires.h \
	   connectkeyframe.h \
	   connectkeyframeeditor.h \
           connectlightingwidget.h \
           connectshowmessage.h \
	   connectviewer.h \
	   connectmeshinfowidget.h \
           cube2sphere.h \
	   doublespinboxdelegate.h \
	   dialogs.h \
           drawhiresvolume.h \
           enums.h \
	   geometryobjects.h \
           glewinitialisation.h \
           global.h \
           globalwidget.h \
           gradienteditor.h \
           gradienteditorwidget.h \
	   hitpoints.h \
           hitpointgrabber.h \
           imglistdialog.h \
           keyframe.h \
           keyframeeditor.h \
           keyframeinformation.h \
           lightdisc.h \
	   lightinginformation.h \
           lightingwidget.h \
           mainwindow.h \
           mainwindowui.h \
	   matrix.h \
           meshinfowidget.h \
           messagedisplayer.h \
	   mymanipulatedframe.h \
	   opacityeditor.h \
	   propertyeditor.h \
	   pathobject.h \
	   pathgrabber.h \
	   paths.h \
	   pathshaderfactory.h \
	   ../../common/src/mesh/binaryplywriter.h \
	   plugininterface.h \
	   pluginthread.h \
	   scalebar.h \
	   scalebargrabber.h \
	   scalebarobject.h \
           shaderfactory.h \
           staticfunctions.h \
	   trisetinformation.h \
	   trisets.h \
	   trisetgrabber.h \
	   trisetobject.h \
	   ../../cpumeshpaint.h \
	   ../../framebufferbudget.h \
	   ../../meshvertexbuffer.h \
           viewer.h \
           xmlheaderfunctions.h \
           popupslider.h \
           captionwidget.h \
           ../../common/src/widgets/dcolordialog.h \
           ../../common/src/widgets/dcolorwheel.h \
	   ../../common/src/widgets/saveimageseqdialog.h \
           ../../common/src/widgets/savemoviedialog.h \
           ../../common/src/videoencoder/videoencoder.h

SOURCES += boundingbox.cpp \
	   brickinformation.cpp \
	   bricks.cpp \
	   brickswidget.cpp \
           camerapathnode.cpp \
	   captions.cpp \
	   captiondialog.cpp \
	   captiongrabber.cpp \
	   captionobject.cpp \
           clipinformation.cpp \
           clipplane.cpp \
	   clipobject.cpp \
	   clipgrabber.cpp \
	   coloreditor.cpp \
           cube2sphere.cpp \
	   doublespinboxdelegate.cpp \
	   dialogs.cpp \
           drawhiresvolume.cpp \
	   geometryobjects.cpp \
           glewinitialisation.cpp \
           global.cpp \
           globalwidget.cpp \
           gradienteditor.cpp \
           gradienteditorwidget.cpp \
	   hitpoints.cpp \
	   hitpointgrabber.cpp \
           imglistdialog.cpp \
           keyframe.cpp \
           keyframeeditor.cpp \
           keyframeinformation.cpp \
           lightdisc.cpp \
	   lightinginformation.cpp \
           lightingwidget.cpp \
           main.cpp \
           mainwindow.cpp \
           mainwindowui.cpp \
	   matrix.cpp \
           meshinfowidget.cpp \
	   menuviewerfunctions.cpp \
	   messagedisplayer.cpp \
	   mymanipulatedframe.cpp \
	   opacityeditor.cpp \
	   propertyeditor.cpp \
	   pathobject.cpp \
	   pathgrabber.cpp \
	   paths.cpp \
	   pathshaderfactory.cpp \
	   ../../common/src/mesh/binaryplywriter.cpp \
	   pluginthread.cpp \
	   scalebar.cpp \
	   scalebargrabber.cpp \
	   scalebarobject.cpp \
           shaderfactory.cpp \
           staticfunctions.cpp \
	   trisetinformation.cpp \
	   trisets.cpp \
	   trisetgrabber.cpp \
	   trisetobject.cpp \
           viewer.cpp \
	   xmlheaderfunctions.cpp \
           popupslider.cpp \
           captionwidget.cpp \
           ../../common/src/widgets/dcolordialog.cpp \
           ../../common/src/widgets/dcolorwheel.cpp \
	   ../../common/src/widgets/saveimageseqdialog.cpp \
           ../../common/src/widgets/savemoviedialog.cpp \
           ../../common/src/videoencoder/videoencoder.cpp
