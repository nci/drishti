QT -= gui
QT += core

CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = slabsavetransaction_smoke

SOURCES += slabsavetransaction_smoke.cpp \
           ../slabsavetransaction.cpp

HEADERS += ../slabsavetransaction.h
