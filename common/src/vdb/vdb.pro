TEMPLATE = lib

DRISHTI_DEFINES = RENDERER

QT += opengl xml network

CONFIG += release staticlib

TARGET = vdb

DESTDIR = ..\..\lib
  
win32 {
     INCLUDEPATH += ..\..\..\drishti
     INCLUDEPATH += C:\cygwin64\home\acl900\vcpkg\vcpkg\installed\x64-windows\include
     QMAKE_LIBDIR += C:\cygwin64\home\acl900\vcpkg\vcpkg\installed\x64-windows\lib

     ### /std:c++17 added because openvdb requires this
     QMAKE_CXXFLAGS*=/std:c++17
  
     LIBS += Imath-3_1.lib openvdb.lib  
}


unix {
!macx {
   INCLUDEPATH += /home/acl900/drishtilib/openvdb/openvdb
   INCLUDEPATH += /home/acl900/drishtilib/openvdb/build/openvdb/openvdb
   INCLUDEPATH += /home/acl900/drishtilib/openvdb/build/openvdb/openvdb/openvdb
   INCLUDEPATH += /home/acl900/drishtilib/oneTBB/include
   
   QMAKE_LIBDIR += /home/acl900/drishtilib/openvdb/build/openvdb/openvdb \
                   /home/acl900/drishtilib/oneTBB/build/gnu_11.3_cxx11_64_relwithdebinfo

   LIBS += -lopenvdb -ltbb -lImath
}
}


HEADERS = vdbvolume.h

SOURCES = vdbvolume.cpp
