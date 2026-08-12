QT += core gui widgets
CONFIG += console c++11
CONFIG -= app_bundle
TEMPLATE = app
TARGET = stream_redirect_smoke

SOURCES += stream_redirect_smoke.cpp

HEADERS += ../../../common/src/widgets/streamredirect.h
