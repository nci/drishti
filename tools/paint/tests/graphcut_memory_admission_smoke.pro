QT += core
QT -= gui

CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = graphcut_memory_admission_smoke

INCLUDEPATH += .. \
               ../graphcut

SOURCES += graphcut_memory_admission_smoke.cpp \
           ../getmemorysize.cpp \
           ../graphcut/graphcut.cpp \
           ../graphcut/graph.cpp

HEADERS += ../getmemorysize.h \
           ../graphcut/graphcut.h \
           ../graphcut/graph.h \
           ../graphcut/block.h \
           ../graphcut/point.h
