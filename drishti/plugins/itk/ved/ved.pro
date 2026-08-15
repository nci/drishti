TEMPLATE = lib

DRISHTI_DEFINES = RENDERER ITK
include( ../../../../drishti.pri )

QT += opengl xml network

CONFIG += release plugin

# ITK 5 keeps this filter's template implementation in the .txx file.
# Force instantiation in this plugin so the public filter methods are linked.
DEFINES += ITK_TEMPLATE_TXX=1

TARGET = vedplugin

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

HEADERS = ved.h \
          filter.h

SOURCES = ved.cpp \
          filter.cpp

HEADERS += ../itkmemoryadmission.h \
           ../../../../tools/paint/getmemorysize.h

SOURCES += ../../../../tools/paint/getmemorysize.cpp
