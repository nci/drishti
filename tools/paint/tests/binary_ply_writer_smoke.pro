QT += core gui
CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = binary_ply_writer_smoke

INCLUDEPATH += ../../../common/src/mesh

SOURCES += binary_ply_writer_smoke.cpp \
           ../../../common/src/mesh/binaryplywriter.cpp

HEADERS += ../../../common/src/mesh/binaryplywriter.h
