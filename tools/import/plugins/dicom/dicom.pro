TEMPLATE = lib

DRISHTI_DEFINES = ITK
include(../../../../drishti.pri )

CONFIG += release plugin
QT += concurrent

TARGET = dicomplugin

include(../plugins.pri)

include(../plugins.itk)

unix:!macx {
  QMAKE_LFLAGS += "-Wl,-rpath=\'\$${ORIGIN}/../ITK\'"
  QMAKE_LFLAGS += "-Wl,-rpath=\'\$${ORIGIN}/../sharedlibs\'"
}

HEADERS = dicomplugin.h \
          dicomhistogramutils.h \
          ../../importmemoryadmission.h

SOURCES = dicomplugin.cpp \
          ../../importmemoryadmission.cpp \
          ../../../../common/src/memoryreservation.cpp

