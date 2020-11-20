TEMPLATE = lib

DRISHTI_DEFINES = ITK
include(../../../../drishti.pri )

CONFIG += release plugin

TARGET = niftiplugin

include(../plugins.pri)

include(../plugins.itk)


# Input
HEADERS = niftiplugin.h
SOURCES = niftiplugin.cpp
