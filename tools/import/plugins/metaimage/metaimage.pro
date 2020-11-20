TEMPLATE = lib

DRISHTI_DEFINES = ITK
include(../../../../drishti.pri )

CONFIG += release plugin

TARGET = metaimageplugin

include(../plugins.pri)


include(../plugins.itk)


QMAKE_LFLAGS += "-Wl,-rpath=\'\$${ORIGIN}/../ITK\'"
QMAKE_LFLAGS += "-Wl,-rpath=\'\$${ORIGIN}/../sharedlibs\'"

# Input
HEADERS = metaimageplugin.h
SOURCES = metaimageplugin.cpp
