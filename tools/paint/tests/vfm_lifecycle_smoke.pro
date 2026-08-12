QT += core gui widgets xml opengl

CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = vfm_lifecycle_smoke

INCLUDEPATH += ..
!isEmpty(BLOSC_INCLUDE_PATH): INCLUDEPATH += $$BLOSC_INCLUDE_PATH
!isEmpty(QGLVIEWER_INCLUDE_PATH): INCLUDEPATH += $$QGLVIEWER_INCLUDE_PATH

win32-msvc {
  QMAKE_CXXFLAGS += /Gy
  QMAKE_LFLAGS += /OPT:REF
}

SOURCES += vfm_lifecycle_smoke.cpp \
           ../volumefilemanager.cpp \
           ../filehandler.cpp \
           ../slabsavetransaction.cpp

HEADERS += ../volumefilemanager.h \
           ../filehandler.h \
           ../slabsavetransaction.h

!isEmpty(BLOSC_LIBRARY_PATH): LIBS += -L$$BLOSC_LIBRARY_PATH
LIBS += -lblosc
