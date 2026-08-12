QT += widgets

CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = tiff_to_volume_file_smoke

INCLUDEPATH += ..

SOURCES += tiff_to_volume_file_smoke.cpp \
           ../volumefilemanager.cpp

HEADERS += ../volinterface.h \
           ../common.h \
           ../volumefilemanager.h
