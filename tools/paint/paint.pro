DRISHTI_DEFINES = RENDERER

TEMPLATE = app

RESOURCES = paint.qrc

TARGET = 
DEPENDPATH += .

QT += opengl
QT += widgets core gui xml

CONFIG += release

DESTDIR = ../../bin

TARGET = drishtipaint

INCLUDEPATH += graphcut \
	       slic

include( ../../drishti.pri )

# Input
FORMS += drishtipaint.ui viewermenu.ui \
	graphcutmenu.ui curvesmenu.ui \
	fibersmenu.ui propertyeditor.ui \
	superpixelmenu.ui pywidgetmenu.ui

#----------------------------------------------------------------
# Windows setup for 64-bit system
#contains(Windows_Setup, Win64) {
  win32 {
	 DEFINES += USE_GLMEDIA

         INCLUDEPATH += C:\Qt\Qt-5.15.2\libQGLViewer\libQGLViewer-2.6.4 \
                        ..\..\glmedia-64 \
                        C:\cygwin64\home\acl900\drishtilib\c-blosc-1.14.3\blosc
##                        C:\Users\acl900\AppData\Local\Programs\Python\Python38\include \
##                        C:\Users\acl900\AppData\Local\Programs\Python\Python38\Lib\site-packages\numpy\core\include
                        
         QMAKE_LIBDIR += C:\Qt\Qt-5.15.2\libQGLViewer\libQGLViewer-2.6.4\lib \
                         ..\..\glmedia-64 \
                         C:\cygwin64\home\acl900\drishtilib\c-blosc-1.14.3\libs
##                         C:\Users\acl900\AppData\Local\Programs\Python\Python38\libs \
##                         C:\Users\acl900\AppData\Local\Programs\Python\Python38\Lib\site-packages\numpy\core\lib

##         LIBS += QGLViewer2.lib glew32.lib glmedia.lib blosc.lib opengl32.lib glu32.lib python38.lib npymath.lib
         LIBS += QGLViewer2.lib glew32.lib glmedia.lib blosc.lib opengl32.lib glu32.lib
        }
#}

unix {
    DEFINES += NO_GLMEDIA

    INCLUDEPATH += /home/acl900/drishtilib/c-blosc/blosc
                        
    QMAKE_LIBDIR += /home/acl900/drishtilib/c-blosc/build/blosc

    LIBS += -lblosc

}

#----------------------------------------------------------------
# MacOSX setup
macx {
    LIBS += -lGLEW -framework QGLViewer -framework GLUT
}
#----------------------------------------------------------------

HEADERS += commonqtclasses.h \
	boundingbox.h \
	drishtipaint.h \
	curvegroup.h \
        clipinformation.h \
        clipplane.h \
	clipobject.h \
	clipgrabber.h \
	dcolordialog.h \
	dcolorwheel.h \
	fiber.h \
	fibergroup.h\
	slices.h \
	imagewidget.h \
	curveswidget.h \
	global.h \
	gradienteditor.h \
	gradienteditorwidget.h \
        livewire.h \
	mybitarray.h \
	myslider.h \
	mymanipulatedframe.h \
 	morphcurve.h \
 	morphslice.h \
	propertyeditor.h \
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
	viewer3d.h \
	volume.h \
	volumefilemanager.h \
	volumeinformation.h \
	volumemask.h \
	volumeoperations.h \
	graphcut/graph.h \
	graphcut/graphcut.h \
	graphcut/block.h \
	graphcut/point.h \
	ply.h \
	lookuptable.h \
	marchingcubes.h \
	showhelp.h \
	getmemorysize.h \
	popupslider.h \
	shaderfactory.h \
	remaphistogramline.h \
	remaphistogramwidget.h \
	slicer3d.h \
        slic/slic.h \
        filehandler.h \
        checkpointhandler.h \
        pywidget.h \
        pywidgetmenu.h


SOURCES += drishtipaint.cpp \
	main.cpp \
	boundingbox.cpp \
	curvegroup.cpp \
        clipinformation.cpp \
        clipplane.cpp \
	clipobject.cpp \
	clipgrabber.cpp \
	dcolordialog.cpp \
	dcolorwheel.cpp \
	fiber.cpp \
	fibergroup.cpp\
	slices.cpp \
	imagewidget.cpp \
	curveswidget.cpp \
	global.cpp \
	gradienteditor.cpp \
	gradienteditorwidget.cpp \
        livewire.cpp \
	mybitarray.cpp \
	myslider.cpp \
	mymanipulatedframe.cpp \
 	morphcurve.cpp \
 	morphslice.cpp \
	propertyeditor.cpp \
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
	volumeinformation.cpp \
	volumemask.cpp \
	volumeoperations.cpp \
	graphcut/graph.cpp \
	graphcut/graphcut.cpp \
	ply.c \
	marchingcubes.cpp \
	showhelp.cpp \
	getmemorysize.cpp \
	popupslider.cpp \
	shaderfactory.cpp \
	remaphistogramline.cpp \
	remaphistogramwidget.cpp \
	slicer3d.cpp \
	slic/slic.cpp \
        filehandler.cpp \
        checkpointhandler.cpp \
        pywidget.cpp \
        pywidgetmenu.cpp
