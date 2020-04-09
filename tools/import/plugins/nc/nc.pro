TEMPLATE = lib

DRISHTI_DEFINES = NETCDF
include(../../../../drishti.pri )

CONFIG += release plugin

TARGET = ncplugin

include(../plugins.pri)

win32 {
  INCLUDEPATH += ../../

  LIBS += netcdfcpp.lib
}

unix {
!macx {
  INCLUDEPATH += ../../

  LIBS += -lnetcdf_c++ \
          -lnetcdf \
  }
}

macx {
  DRISHTI_DEFINES = IMPORT

  INCLUDEPATH += ../../

  LIBS += -lnetcdf \
	  -lnetcdf_c++ \
}


HEADERS = ncplugin.h

SOURCES = ncplugin.cpp

