TEMPLATE = lib

RESOURCES = ../import.qrc

TARGET = pyi3.14
DEPENDPATH += .

QT += widgets core gui

CONFIG += release plugin

DESTDIR = ../../../bin/pyversion

PYTHON_VERSION = 314

# Input
#----------------------------------------------------------------
# Windows setup for 64-bit system
#contains(Windows_Setup, Win64) {
  win32 {
        VCPKG_INCLUDE_PATH = C:\Apps\vcpkg\installed\x64-windows\include
        VCPKG_LIBRARY_PATH = C:\Apps\vcpkg\installed\x64-windows\lib
        
        INCLUDEPATH += ../ ../../../common/src/pybind
        QMAKE_LIBDIR += ..\..\..\common\lib   

        INCLUDEPATH += $$VCPKG_INCLUDE_PATH
        QMAKE_LIBDIR += $$VCPKG_LIBRARY_PATH

        INCLUDEPATH += C:\Apps\Python$${PYTHON_VERSION}\include
        QMAKE_LIBDIR += C:\Apps\Python$${PYTHON_VERSION}\libs

        LIBS += python$${PYTHON_VERSION}.lib

        ## /std:c++17 added because openvdb requires this
        QMAKE_CXXFLAGS*=/std:c++17
        }
#}

unix {
 !macx {
    INCLUDEPATH += ../../../common/src/pybind

    QMAKE_LIBDIR += ../../../common/lib              
    }
 }

#----------------------------------------------------------------
# MacOSX setup
macx {
}
#----------------------------------------------------------------

HEADERS += pyversion.h \
           ../pyplugininterface.h \
           ../../../common/src/pybind/pywidget.h \
           ../../../common/src/pybind/pythonengine.h

SOURCES += pyversion.cpp \
           ../../../common/src/pybind/pywidget.cpp \
           ../../../common/src/pybind/pythonengine.cpp
