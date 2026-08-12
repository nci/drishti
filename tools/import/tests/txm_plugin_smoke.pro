QT += widgets gui core

CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = txm_plugin_smoke

INCLUDEPATH += .. \
               ../plugins/txm

SOURCES += txm_plugin_smoke.cpp \
           ../plugins/txm/pole.cpp

HEADERS += ../common.h \
           ../commonqtclasses.h \
           ../volinterface.h \
           ../plugins/txm/pole.h
