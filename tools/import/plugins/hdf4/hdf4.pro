TEMPLATE = lib
CONFIG += release plugin

TARGET = hdf4plugin

include(../plugins.pri)

INCLUDEPATH += ../../ \
	       c:\drishtilib\netcdf\include \
	       c:\drishtilib\hdf4\include

QMAKE_LIBDIR += c:\drishtilib\netcdf\lib \
	   	c:\drishtilib\hdf4\dll

LIBS +=	netcdf.lib \
	hd423m.lib \
	mfhdf_fcstubdll.lib \
	hdf_fcstubdll.lib \
	hm423m.lib

HEADERS = hdf4plugin.h

SOURCES = hdf4plugin.cpp

