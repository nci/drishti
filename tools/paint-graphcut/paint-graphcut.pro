TEMPLATE = app

include( ../../version.pri )

RESOURCES = paint.qrc

TARGET = 
DEPENDPATH += .

QT += widgets core gui xml

CONFIG += release

DESTDIR = ../../5.2.1

TARGET = drishtipaint

INCLUDEPATH += graphcut

# Input
FORMS += drishtipaint.ui

HEADERS += commonqtclasses.h \
	drishtipaint.h \
	bitmapthread.h \
	dcolordialog.h \
	dcolorwheel.h \
	imagewidget.h \
	global.h \
	gradienteditor.h \
	gradienteditorwidget.h \
	myslider.h \
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
	volume.h \
	volumefilemanager.h \
	volumemask.h \
	graphcut/graph.h \
	graphcut/graphcut.h \
	graphcut/block.h \
	graphcut/point.h


SOURCES += drishtipaint.cpp \
	main.cpp \
	bitmapthread.cpp \
	dcolordialog.cpp \
	dcolorwheel.cpp \
	imagewidget.cpp \
	global.cpp \
	gradienteditor.cpp \
	gradienteditorwidget.cpp \
	myslider.cpp \
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
	volume.cpp \
	volumefilemanager.cpp \
	volumemask.cpp \
	graphcut/graph.cpp \
	graphcut/graphcut.cpp
