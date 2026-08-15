TEMPLATE = lib
CONFIG += release plugin

TARGET = txmplugin

include(../plugins.pri)

INCLUDEPATH += ../../

HEADERS = txmplugin.h \
	  pole.h \
          ../../importmemoryadmission.h

SOURCES = txmplugin.cpp \
	  pole.cpp \
          ../../importmemoryadmission.cpp \
          ../../../../common/src/memoryreservation.cpp

