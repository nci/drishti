DRISHTI_DEFINES = RENDERER NETCDF

RESOURCES = drishti.qrc

QT += opengl xml network
QT += multimedia multimediawidgets

CONFIG += release

TRANSLATIONS = drishtitr_ch.ts

FORMS += launcher.ui \
         mainwindow.ui \
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
	 volumeinformation.ui \
         raycastmenu.ui


TEMPLATE = app

DESTDIR = ../bin

TARGET = drishti
DEPENDPATH += .

include( ../drishti.pri )

win32 {
  RC_ICONS += images/drishti-256.ico

  contains(Windows_Setup, Win64) {
    message(drishti.exe : Win64 setup)
    DEFINES += _CRT_SECURE_NO_WARNINGS

    INCLUDEPATH += 16bit \
                   c:/cygwin64/home/acl900/drishtilib/assimp-5.0.1/include \
                   c:/cygwin64/home/acl900/drishtilib/assimp-5.0.1/build/include \
                   C:\cygwin64\home\acl900\vcpkg\vcpkg\installed\x64-windows\include \
                   ..\common\src\videoencoder

    INCLUDEPATH += $$FFMPEG_INCLUDE_PATH
                   
    QMAKE_LIBDIR += c:/cygwin64/home/acl900/drishtilib/assimp-5.0.1/libs \
                    C:\cygwin64\home\acl900\vcpkg\vcpkg\installed\x64-windows\lib
	
    QMAKE_LIBDIR += $$FFMPEG_LIBRARY_PATH
                   
    LIBS += -lQGLViewer2 \
            -lnetcdf-cxx4 \
            -lnetcdf \
  	    -lglew32 \
  	    -lfreeglut \
            -lopengl32 \
            -lglu32 \
            -lassimp-vc142-mt

     # Set list of required FFmpeg libraries
     LIBS += -lavutil \
             -lavcodec \
             -lavformat \
             -lswresample \
             -lswscale 
  }
}


unix {
!macx {

TARGET = drishti

INCLUDEPATH += 16bit

LIBS += -lGLU

###LIBS += -lQGLViewer \
###        -lnetcdf_c++ \
###        -lnetcdf \
###        -lGLEW \
### 	-lglut \
###	-lGLU
}
}


macx {

INCLUDEPATH += 16bit

LIBS += -lGLEW -lnetcdf -lnetcdf_c++ -framework QGLViewer -framework GLUT
}



# Input
HEADERS += launcher.h \
           boundingbox.h \
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
	   connectvolinfowidget.h \
	   crops.h \
	   cropobject.h \
	   cropgrabber.h \
           cropshaderfactory.h \
           cube2sphere.h \
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
           imglistdialog.h \
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
	   videoplayer.h \
	   mybitarray.h \
           popupslider.h \
           ../common/src/videoencoder/videoencoder.h


 SOURCES +=launcher.cpp \
           boundingbox.cpp \
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
           cube2sphere.cpp \
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
           imglistdialog.cpp \
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
	   menuviewerfunctions.cpp \
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
	   videoplayer.cpp \
	   mybitarray.cpp \
	   popupslider.cpp \
           ../common/src/videoencoder/videoencoder.cpp
