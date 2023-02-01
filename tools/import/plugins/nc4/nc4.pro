TEMPLATE = lib

#DRISHTI_DEFINES = NETCDF
include(../../../../drishti.pri )

CONFIG += release plugin

TARGET = nc4plugin


HEADERS = nc4plugin.h

SOURCES = nc4plugin.cpp


include(../plugins.pri)

win32 {
  INCLUDEPATH += ../../
  INCLUDEPATH += C:\cygwin64\home\acl900\vcpkg\vcpkg\installed\x64-windows\include
  QMAKE_LIBDIR += C:\cygwin64\home\acl900\vcpkg\vcpkg\installed\x64-windows\lib

  LIBS += netcdf-cxx4.lib netcdf.lib
}

unix {
!macx {
  DRISHTI_DEFINES = IMPORT

  INCLUDEPATH += ../../

  HEADERS += netcdf.hh ncvalues.h netcdfcpp.h
  SOURCES += netcdf.cpp ncvalues.cpp
  SOURCES += attr.c  dim.c  error.c  libvers.c  nc.c  ncx.c  posixio.c  putget.c  string.c  utf8proc.c  v1hpg.c  v2i.c  var.c
  }
}

macx {
  DRISHTI_DEFINES = IMPORT

  INCLUDEPATH += ../../

  LIBS += -lnetcdf \
	  -lnetcdf_c++ \
}


