QT += core widgets

CONFIG += console c++11
CONFIG -= app_bundle

TEMPLATE = app
TARGET = native_raw_plugin_smoke

INCLUDEPATH += ..

SOURCES = native_raw_plugin_smoke.cpp
HEADERS = ../common.h \
          ../volinterface.h
