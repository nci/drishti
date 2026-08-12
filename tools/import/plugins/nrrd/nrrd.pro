TEMPLATE = lib

DRISHTI_DEFINES = ITK
include(../../../../drishti.pri )

CONFIG += release plugin
QT += concurrent

TARGET = nrrdplugin

include(../plugins.pri)


include(../plugins.itk)



# Input
HEADERS = nrrdplugin.h \
          ../../importmemoryadmission.h
SOURCES = nrrdplugin.cpp \
          ../../importmemoryadmission.cpp
