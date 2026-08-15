QT -= gui

CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = algorithm_memory_admission_smoke

INCLUDEPATH += ..

SOURCES += algorithm_memory_admission_smoke.cpp \
           ../getmemorysize.cpp \
           ../../../common/src/memoryreservation.cpp

HEADERS += ../getmemorysize.h \
           ../../../common/src/memoryreservation.h
