QT += core gui widgets

CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = meshtools_io_smoke

INCLUDEPATH += ../../../common/src/mesh
!isEmpty(VCPKG_INCLUDE_PATH): INCLUDEPATH += $$VCPKG_INCLUDE_PATH
!isEmpty(VCPKG_LIBRARY_PATH): QMAKE_LIBDIR += $$VCPKG_LIBRARY_PATH

isEmpty(DRISHTI_GMSH_LIB): DRISHTI_GMSH_LIB = gmsh.dll.lib
LIBS += $$DRISHTI_GMSH_LIB

SOURCES += meshtools_io_smoke.cpp \
           ../../../common/src/mesh/meshtools.cpp

HEADERS += ../../../common/src/mesh/meshtools.h
