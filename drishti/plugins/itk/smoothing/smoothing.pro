TEMPLATE = lib

DRISHTI_DEFINES = RENDERER ITK
include( ../../../../drishti.pri )

QT += opengl xml network

CONFIG += release plugin

TARGET = smoothingplugin

FORMS += ../../../propertyeditor.ui


include(../plugin.itk)

win32 {
  DESTDIR = $$clean_path($$DRISHTI_RENDER_PLUGIN_DIR/ITK/Smoothing)
}
unix {
 !macx {
  DESTDIR = ../../../../bin/renderplugins/ITK/Smoothing

  QMAKE_LFLAGS += "-Wl,-rpath=\'\$${ORIGIN}/../../../ITK\'"
  QMAKE_LFLAGS += "-Wl,-rpath=\'\$${ORIGIN}/../../../sharedlibs\'"
 }
}
 macx {
  DESTDIR = ../../../../bin/drishti.app/renderplugins/ITK/Smoothing
}


HEADERS = smoothing.h \
	filter.h	

SOURCES = smoothing.cpp \
	filter.cpp

HEADERS += ../itkmemoryadmission.h \
	../../../../tools/paint/getmemorysize.h

SOURCES += ../../../../tools/paint/getmemorysize.cpp
