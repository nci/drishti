QT += gui

CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = imagestack_contract_smoke

INCLUDEPATH += ..

SOURCES += imagestack_contract_smoke.cpp

HEADERS += ../plugins/imagestack/imagestackpixelconversion.h \
           ../common.h
