TEMPLATE = lib
CONFIG += release plugin

TARGET = ncplugin

include(../plugins.pri)

win32 {
  INCLUDEPATH += ../../ \
	         c:\drishtilib\netcdf\include \

  QMAKE_LIBDIR += c:\drishtilib\netcdf\lib

  LIBS += netcdf.lib

}

unix {
!macx {
  INCLUDEPATH += ../../ \
              /usr/include/netcdf-3

  QMAKE_LIBDIR += /usr/lib /usr/lib/x86_64-linux-gnu

  LIBS += -lnetcdf_c++ \
          -lnetcdf \
  }
}

macx {

  INCLUDEPATH += ../../ \
               /usr/local/include 
	
  QTMAKE_LIBDIR += /usr/local/lib 

  LIBS += -lnetcdf \
	  -lnetcdf_c++ \
}


HEADERS = ncplugin.h

SOURCES = ncplugin.cpp

