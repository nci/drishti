TEMPLATE = lib

DRISHTI_DEFINES = ITK
include(../../../../drishti.pri )

CONFIG += release plugin

TARGET = nrrdplugin

include(../plugins.pri)


include(../plugins.itk)



# Input
HEADERS = nrrdplugin.h
SOURCES = nrrdplugin.cpp
