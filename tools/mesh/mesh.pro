DRISHTI_DEFINES = RENDERER NETCDF

RESOURCES = mesh.qrc

QT += opengl widgets core gui xml network
QT += multimedia multimediawidgets

CONFIG += release

TRANSLATIONS = chinese.ts

FORMS += mainwindow.ui \
	 saveimgseq.ui \
	 savemovie.ui \
	 brickswidget.ui \
	 captiondialog.ui \
	 lightingwidget.ui \
	 globalwidget.ui \
         propertyeditor.ui \
         meshinfowidget.ui


TEMPLATE = app

DESTDIR = ../../bin

TARGET = drishtimesh
DEPENDPATH += .

include( ../../drishti.pri )

win32 {
  RC_ICONS += images/drishtimesh.ico

  DEFINES += USE_GLMEDIA
  OPENVR_VERSION = 1.14.15

  contains(Windows_Setup, Win64) {
    DEFINES += _CRT_SECURE_NO_WARNINGS

    QMAKE_CXXFLAGS += -Ob3
    QMAKE_CXXFLAGS += -GL
    QMAKE_CXXFLAGS += -Gw

    QMAKE_LFLAGS += /OPT:ICF /LTCG
    
    INCLUDEPATH += ..\..\glmedia-64 \
                   c:/cygwin64/home/acl900/drishtilib/assimp-5.0.1/include \
                   c:/cygwin64/home/acl900/drishtilib/assimp-5.0.1/build/include
                   
    QMAKE_LIBDIR += ..\..\glmedia-64 \
                    c:/cygwin64/home/acl900/drishtilib/assimp-5.0.1/libs

                   
    LIBS += QGLViewer2.lib \
            glew32.lib \
            glmedia.lib \
            opengl32.lib \
            glu32.lib \
            assimp-vc142-mt.lib \
            iphlpapi.lib
  }
}


unix {
!macx {
  DEFINES += NO_GLMEDIA

  INCLUDEPATH +=  /home/acl900/drishtilib/assimp-5.0.1/include \
                  /home/acl900/drishtilib/assimp-5.0.1/build/include


  QMAKE_LIBDIR += /home/acl900/drishtilib/assimp-5.0.1/libs 

  LIBS += -lGLU
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
           computeshaderfactory.h \
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
           dcolordialog.h \
           dcolorwheel.h \
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
	   ply.h \
	   plugininterface.h \
	   pluginthread.h \
	   saveimageseqdialog.h \
	   savemoviedialog.h \
	   scalebar.h \
	   scalebargrabber.h \
	   scalebarobject.h \
           shaderfactory.h \
           staticfunctions.h \
	   trisetinformation.h \
	   trisets.h \
	   trisetgrabber.h \
	   trisetobject.h \
           viewer.h \
           xmlheaderfunctions.h \
           popupslider.h \
           captionwidget.h

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
           computeshaderfactory.cpp \
           cube2sphere.cpp \
           dcolordialog.cpp \
           dcolorwheel.cpp \
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
	   ply.c \
	   pluginthread.cpp \
	   saveimageseqdialog.cpp \
	   savemoviedialog.cpp \
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
