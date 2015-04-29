TEMPLATE = lib

DRISHTI_DEFINES = IMPORT
include(../../../../drishti.pri )

CONFIG += release plugin

TARGET = ncplugin

include(../plugins.pri)

win32 {
  INCLUDEPATH += ../../

 contains(Windows_Setup, Win32) {  
    LIBS += netcdf.lib
 }
 contains(Windows_Setup, Win64) {  
    LIBS += netcdfcpp.lib
 }
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

