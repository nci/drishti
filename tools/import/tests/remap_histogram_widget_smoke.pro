QT += widgets

CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = remap_histogram_widget_smoke

INCLUDEPATH += ..

SOURCES += remap_histogram_widget_smoke.cpp \
           ../remaphistogramwidget.cpp \
           ../remaphistogramline.cpp

HEADERS += ../remaphistogramwidget.h \
           ../remaphistogramline.h
