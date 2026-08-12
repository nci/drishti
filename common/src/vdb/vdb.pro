TEMPLATE = lib

DRISHTI_DEFINES = RENDERER

include(../../../drishti.pri )

QT += opengl xml network

CONFIG += release staticlib

TARGET = vdb

isEmpty(DRISHTI_COMMON_LIB_DIR): DRISHTI_COMMON_LIB_DIR = $$clean_path($$PWD/../../lib)
DESTDIR = $$DRISHTI_COMMON_LIB_DIR
  
win32 {
     TARGET = $$section(DRISHTI_VDB_LIB, ., 0, 0)

      INCLUDEPATH += ..\..\..\drishti
      INCLUDEPATH += $$VCPKG_INCLUDE_PATH
      QMAKE_LIBDIR += $$VCPKG_LIBRARY_PATH

     ### /std:c++17 added because openvdb requires this
     QMAKE_CXXFLAGS*=/std:c++17 /bigobj
  
     LIBS += $$DRISHTI_IMATH_LIB $$DRISHTI_OPENVDB_LIB
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
