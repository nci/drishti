DRISHTI_DEFINES =
include(../../../drishti.pri)

QT += core gui
CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = videoencoder_smoke

INCLUDEPATH += ../../../common/src/videoencoder

win32 {
  LIBS += $$DRISHTI_FFMPEG_LIBS Psapi.lib
}

SOURCES += videoencoder_smoke.cpp \
           ../../../common/src/videoencoder/videoencoder.cpp

HEADERS += ../../../common/src/videoencoder/videoencoder.h \
           ../../../common/src/videoencoder/ffmpeg.h
