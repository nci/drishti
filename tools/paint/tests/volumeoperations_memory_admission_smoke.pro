QT -= gui

CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = volumeoperations_memory_admission_smoke

INCLUDEPATH += ..

SOURCES += volumeoperations_memory_admission_smoke.cpp \
           ../getmemorysize.cpp

HEADERS += ../volumeoperations.h \
           ../getmemorysize.h
