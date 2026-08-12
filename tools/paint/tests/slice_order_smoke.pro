QT -= gui
QT += core

CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = slice_order_smoke

SOURCES += slice_order_smoke.cpp \
           ../sliceorderutils.cpp

HEADERS += ../sliceorderutils.h
