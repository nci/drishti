QT += widgets

CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = tiff_real_stack_smoke

INCLUDEPATH += ..

SOURCES += tiff_real_stack_smoke.cpp

HEADERS += ../volinterface.h \
           ../common.h
