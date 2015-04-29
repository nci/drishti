DRISHTI_DEFINES = RENDERER NETCDF

RESOURCES = drishti.qrc

QT += opengl xml network
QT += multimedia multimediawidgets

CONFIG += release

FORMS += mainwindow.ui \
	 saveimgseq.ui \
	 savemovie.ui \
	 brickswidget.ui \
	 captiondialog.ui \
	 directionvectorwidget.ui \
	 fileslistdialog.ui \
	 lightingwidget.ui \
	 load2volumes.ui \
	 load3volumes.ui \
	 load4volumes.ui \
	 preferenceswidget.ui \
	 propertyeditor.ui \
	 profileviewer.ui \
	 viewseditor.ui \
	 volumeinformation.ui


TEMPLATE = app

DESTDIR = ../bin

TARGET = drishti
DEPENDPATH += .

include( ../drishti.pri )

win32 {
  DEFINES += USE_GLMEDIA

 contains(Windows_Setup, Win32) {
    message(drishti.exe : Win32 setup)
    INCLUDEPATH += ..\glmedia 16bit
    QMAKE_LIBDIR += ..\glmedia
    LIBS += QGLViewer2.lib \
  	netcdf.lib \
  	glew32.lib \
  	glmedia.lib
  }

  contains(Windows_Setup, Win64) {
    message(drishti.exe : Win64 setup)
    DEFINES += _CRT_SECURE_NO_WARNINGS
    INCLUDEPATH += ..\glmedia-64 16bit
    QMAKE_LIBDIR += ..\glmedia-64
    LIBS += QGLViewer2.lib \
  	netcdfcpp.lib \
  	glew32.lib \
  	freeglut.lib \
  	glmedia.lib
  }
}


unix {
!macx {

TARGET = drishti

DEFINES += NO_GLMEDIA

INCLUDEPATH += 16bit

LIBS += -lQGLViewer \
        -lnetcdf_c++ \
        -lnetcdf \
        -lGLEW \
 	-lglut \
	-lGLU
}
}


macx {

DEFINES += NO_GLMEDIA

INCLUDEPATH += 16bit

LIBS += -lGLEW -lnetcdf -lnetcdf_c++ -framework QGLViewer -framework GLUT
}



# Input
HEADERS += boundingbox.h \
           blendshaderfactory.h \
	   brickinformation.h \
	   bricks.h \
	   brickswidget.h \
           camerapathnode.h \
	   captions.h \
	   captiondialog.h \
	   captiongrabber.h \
	   captionobject.h \
	   colorbar.h \
	   colorbargrabber.h \
	   colorbarobject.h \
           classes.h \
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
	   connectpreferences.h \
	   connectshowmessage.h \
	   connecttfeditor.h \
	   connecttfmanager.h \
	   connectviewer.h \
	   connectviewseditor.h \
	   connectvolinfowidget.h \
	   crops.h \
	   cropobject.h \
	   cropgrabber.h \
           cropshaderfactory.h \
           dcolordialog.h \
           dcolorwheel.h \
	   doublespinboxdelegate.h \
	   dialogs.h \
	   directionvectorwidget.h \
           drawhiresvolume.h \
           drawlowresvolume.h \
           enums.h \
	   fileslistdialog.h \	   
	   geometryobjects.h \
	   geoshaderfactory.h \
           glewinitialisation.h \
           global.h \
           glowshaderfactory.h \
           gradienteditor.h \
           gradienteditorwidget.h \
	   grids.h \
	   gridgrabber.h \
	   gridobject.h \
	   hitpoints.h \
	   hitpointgrabber.h \
	   imagecaptions.h \
	   imagecaptiongrabber.h \
	   imagecaptionobject.h \
           keyframe.h \
           keyframeeditor.h \
           keyframeinformation.h \
	   landmarks.h \
	   landmarkinformation.h \
           lightdisc.h \
	   lightinginformation.h \
           lightingwidget.h \
	   load2volumes.h \
	   load3volumes.h \
	   load4volumes.h \
           mainwindow.h \
           mainwindowui.h \
	   matrix.h \
	   messagedisplayer.h \
	   mymanipulatedframe.h \
	   networkinformation.h \
	   networks.h \
	   networkgrabber.h \
	   networkobject.h \
	   opacityeditor.h \
	   paintball.h \
	   propertyeditor.h \
	   pathobject.h \
	   pathgrabber.h \
	   paths.h \
	   pathgroups.h \
	   pathgroupobject.h \
	   pathgroupgrabber.h \
	   pathshaderfactory.h \
	   ply.h \
	   plugininterface.h \
	   pluginthread.h \
	   preferenceswidget.h \
	   profileviewer.h \
           prunehandler.h \
           pruneshaderfactory.h \
	   rawvolume.h \
	   saveimageseqdialog.h \
	   savemoviedialog.h \
	   scalebar.h \
	   scalebargrabber.h \
	   scalebarobject.h \
           shaderfactory.h \
           shaderfactory2.h \
           shaderfactoryrgb.h \
           splineeditor.h \
           splineeditorwidget.h \
	   splineinformation.h \
           splinetransferfunction.h \
           staticfunctions.h \
	   tagcoloreditor.h \
           tearshaderfactory.h \
	   tick.h \
           transferfunctioncontainer.h \
           transferfunctioneditorwidget.h \
           transferfunctionmanager.h \
	   trisetinformation.h \
	   trisets.h \
	   trisetgrabber.h \
	   trisetobject.h \
           viewer.h \
	   viewinformation.h \
	   viewseditor.h \
	   volume.h \
           volumebase.h \
	   volumeinformation.h \
	   volumeinformationwidget.h \
           volumefilemanager.h \
           volumesingle.h \
	   volumergbbase.h \
	   volumergb.h \
	   xmlheaderfunctions.h \
	   16bit/remaphistogramline.h \
	   16bit/remaphistogramwidget.h \
	   mopplugininterface.h \
	   itksegmentation.h \
           lighthandler.h \
           lightshaderfactory.h \
	   gilights.h \
	   gilightgrabber.h \
	   gilightobject.h \
	   gilightinfo.h \
	   gilightobjectinfo.h \
	   videoplayer.h

SOURCES += boundingbox.cpp \
           blendshaderfactory.cpp \
	   brickinformation.cpp \
	   bricks.cpp \
	   brickswidget.cpp \
           camerapathnode.cpp \
	   captions.cpp \
	   captiondialog.cpp \
	   captiongrabber.cpp \
	   captionobject.cpp \
	   classes.cpp \
	   colorbar.cpp \
	   colorbargrabber.cpp \
	   colorbarobject.cpp \
           clipinformation.cpp \
           clipplane.cpp \
	   clipobject.cpp \
	   clipgrabber.cpp \
	   coloreditor.cpp \
	   crops.cpp \
	   cropobject.cpp \
	   cropgrabber.cpp \
           cropshaderfactory.cpp \
           dcolordialog.cpp \
           dcolorwheel.cpp \
	   doublespinboxdelegate.cpp \
	   dialogs.cpp \
	   directionvectorwidget.cpp \
           drawhiresvolume.cpp \
           drawlowresvolume.cpp \
	   fileslistdialog.cpp \
	   geometryobjects.cpp \
	   geoshaderfactory.cpp \
           glewinitialisation.cpp \
           global.cpp \
           glowshaderfactory.cpp \
           gradienteditor.cpp \
           gradienteditorwidget.cpp \
	   grids.cpp \
	   gridgrabber.cpp \
	   gridobject.cpp \
	   hitpoints.cpp \
	   hitpointgrabber.cpp \
	   imagecaptions.cpp \
	   imagecaptiongrabber.cpp \
	   imagecaptionobject.cpp \
           keyframe.cpp \
           keyframeeditor.cpp \
           keyframeinformation.cpp \
	   landmarks.cpp \
	   landmarkinformation.cpp \
           lightdisc.cpp \
	   lightinginformation.cpp \
           lightingwidget.cpp \
	   load2volumes.cpp \
	   load3volumes.cpp \
	   load4volumes.cpp \
           main.cpp \
           mainwindow.cpp \
           mainwindowui.cpp \
	   matrix.cpp \
	   messagedisplayer.cpp \
	   mymanipulatedframe.cpp \
	   networkinformation.cpp \
	   networks.cpp \
	   networkgrabber.cpp \
	   networkobject.cpp \
	   opacityeditor.cpp \
	   paintball.cpp \
	   propertyeditor.cpp \
	   pathobject.cpp \
	   pathgrabber.cpp \
	   paths.cpp \
	   pathgroups.cpp \
	   pathgroupobject.cpp \
	   pathgroupgrabber.cpp \
	   pathshaderfactory.cpp \
	   ply.c \
	   pluginthread.cpp \
	   preferenceswidget.cpp \
	   profileviewer.cpp \
           prunehandler.cpp \
           pruneshaderfactory.cpp \
	   rawvolume.cpp \
	   saveimageseqdialog.cpp \
	   savemoviedialog.cpp \
	   scalebar.cpp \
	   scalebargrabber.cpp \
	   scalebarobject.cpp \
           shaderfactory.cpp \
           shaderfactory2.cpp \
           shaderfactoryrgb.cpp \
           splineeditor.cpp \
           splineeditorwidget.cpp \
	   splineinformation.cpp \
           splinetransferfunction.cpp \
           staticfunctions.cpp \
	   tagcoloreditor.cpp \
           tearshaderfactory.cpp \
	   tick.cpp \
           transferfunctioncontainer.cpp \
           transferfunctioneditorwidget.cpp \
           transferfunctionmanager.cpp \
	   trisetinformation.cpp \
	   trisets.cpp \
	   trisetgrabber.cpp \
	   trisetobject.cpp \
           viewer.cpp \
	   viewinformation.cpp \
	   viewseditor.cpp \
	   volume.cpp \
           volumebase.cpp \
	   volumeinformation.cpp \
	   volumeinformationwidget.cpp \
           volumefilemanager.cpp \
           volumesingle.cpp \
	   volumergbbase.cpp \
	   volumergb.cpp \
	   xmlheaderfunctions.cpp \
	   16bit/remaphistogramline.cpp \
	   16bit/remaphistogramwidget.cpp \
	   itksegmentation.cpp \
           lighthandler.cpp \
           lightshaderfactory.cpp \
	   gilights.cpp \
	   gilightgrabber.cpp \
	   gilightobject.cpp \
	   gilightinfo.cpp \
	   gilightobjectinfo.cpp \
	   videoplayer.cpp
