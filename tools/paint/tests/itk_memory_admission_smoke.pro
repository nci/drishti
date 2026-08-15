QT += core
QT -= gui

CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = itk_memory_admission_smoke

INCLUDEPATH += ../../../drishti/plugins/itk \
               ..

SOURCES += itk_memory_admission_smoke.cpp \
           ../getmemorysize.cpp \
           ../../../common/src/memoryreservation.cpp

HEADERS += ../../../drishti/plugins/itk/itkmemoryadmission.h \
           ../getmemorysize.h
