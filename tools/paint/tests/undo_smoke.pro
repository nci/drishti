QT += core gui widgets

CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = undo_smoke

INCLUDEPATH += ..
!isEmpty(BLOSC_INCLUDE_PATH): INCLUDEPATH += $$BLOSC_INCLUDE_PATH

SOURCES += undo_smoke.cpp \
           ../filehandler.cpp

HEADERS += ../filehandler.h

!isEmpty(BLOSC_LIBRARY_PATH): LIBS += -L$$BLOSC_LIBRARY_PATH
LIBS += -lblosc
