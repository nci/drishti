TEMPLATE = lib

DRISHTI_DEFINES = ITK
include(../../../../drishti.pri )

CONFIG += release plugin

TARGET = dicomplugin

include(../plugins.pri)

include(../plugins.itk)

QMAKE_LFLAGS += "-Wl,-rpath=\'\$${ORIGIN}/../ITK\'"
QMAKE_LFLAGS += "-Wl,-rpath=\'\$${ORIGIN}/../sharedlibs\'"

HEADERS = dicomplugin.h

SOURCES = dicomplugin.cpp

