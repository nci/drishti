QT += core xml

CONFIG += console c++11 release
CONFIG -= app_bundle
TEMPLATE = app
TARGET = pvl_manifest_smoke

INCLUDEPATH += ../../..

SOURCES += pvl_manifest_smoke.cpp \
           ../../../common/src/pvlmanifest.cpp

HEADERS += ../../../common/src/pvlmanifest.h
