QT += core gui widgets

CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = volume_file_transaction_smoke

INCLUDEPATH += ..

SOURCES += volume_file_transaction_smoke.cpp \
           ../volumefilemanager.cpp

HEADERS += ../volumefilemanager.h
