TEMPLATE = lib

DRISHTI_DEFINES = NETCDF
include(../../../../drishti.pri )

CONFIG += release plugin

TARGET = ncplugin


HEADERS = ncplugin.h

SOURCES = ncplugin.cpp


include(../plugins.pri)

win32 {
  INCLUDEPATH += ../../

  LIBS += netcdfcpp.lib
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


