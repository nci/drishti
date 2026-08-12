QT -= gui

CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = import_memory_admission_smoke

INCLUDEPATH += ..

SOURCES += import_memory_admission_smoke.cpp \
           ../importmemoryadmission.cpp

HEADERS += ../importmemoryadmission.h
