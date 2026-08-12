QT -= gui

CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = plugin_error_bridge_smoke

INCLUDEPATH += ..

SOURCES += plugin_error_bridge_smoke.cpp

HEADERS += ../pluginoperationstatus.h
