TEMPLATE = lib

DRISHTI_DEFINES = RENDERER

QT += opengl xml network

CONFIG += release staticlib

TARGET = vdb
  
win32 {
     DESTDIR = ..\..\lib

     INCLUDEPATH += ..\..\..\drishti
     INCLUDEPATH += C:\cygwin64\home\acl900\vcpkg\vcpkg\installed\x64-windows\include
     QMAKE_LIBDIR += C:\cygwin64\home\acl900\vcpkg\vcpkg\installed\x64-windows\lib

     ### /std:c++17 added because openvdb requires this
     QMAKE_CXXFLAGS*=/std:c++17
  
     LIBS += Imath-3_1.lib openvdb.lib  
}

HEADERS = vdbvolume.h

SOURCES = vdbvolume.cpp
