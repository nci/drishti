QT -= gui

CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = import_memory_admission_smoke

INCLUDEPATH += ..

SOURCES += import_memory_admission_smoke.cpp \
           ../importmemoryadmission.cpp \
           ../../../common/src/memoryreservation.cpp

HEADERS += ../importmemoryadmission.h \
           ../../../common/src/memoryreservation.h
