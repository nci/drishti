QT -= gui
QT += core

CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = mask_import_smoke

INCLUDEPATH += .. $$VCPKG_INCLUDE_PATH
QMAKE_LIBDIR += $$VCPKG_LIBRARY_PATH

win32: LIBS += blosc.lib
unix: LIBS += -lblosc

SOURCES += mask_import_smoke.cpp \
           ../maskimportutils.cpp \
           ../getmemorysize.cpp \
           ../../../common/src/memoryreservation.cpp

HEADERS += ../maskimportutils.h \
           ../getmemorysize.h
