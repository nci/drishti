TEMPLATE = lib

DRISHTI_DEFINES = IMPORT TIFF
include(../../../../drishti.pri )

CONFIG += release plugin

TARGET = tiffplugin

include(../plugins.pri)

win32 {
  INCLUDEPATH += ./ ../../

  SOURCES = tiffplugin.cpp \
		libtiff\tif_aux.c \
		libtiff\tif_close.c \
		libtiff\tif_codec.c \
		libtiff\tif_color.c \
		libtiff\tif_compress.c \
		libtiff\tif_dir.c \
		libtiff\tif_dirinfo.c \
		libtiff\tif_dirread.c \
		libtiff\tif_dirwrite.c \
		libtiff\tif_dumpmode.c \
		libtiff\tif_error.c \
		libtiff\tif_extension.c \
		libtiff\tif_fax3.c \
		libtiff\tif_fax3sm.c \
		libtiff\tif_flush.c \
		libtiff\tif_getimage.c \
		libtiff\tif_luv.c \
		libtiff\tif_lzw.c \
		libtiff\tif_next.c \
		libtiff\tif_open.c \
		libtiff\tif_packbits.c \
		libtiff\tif_pixarlog.c \
		libtiff\tif_predict.c \
		libtiff\tif_print.c \
		libtiff\tif_read.c \
		libtiff\tif_strip.c \
		libtiff\tif_swab.c \
		libtiff\tif_thunder.c \
		libtiff\tif_tile.c \
		libtiff\tif_version.c \
		libtiff\tif_warning.c \
		libtiff\tif_write.c \
		libtiff\tif_zip.c \
		libtiff\tif_win32.c 

   QMAKE_LIBDIR += C:\cygwin64\home\acl900\drishtilib\zlib-1.2.8\Release
   LIBS += zlibstatic.lib
}

unix {
 !macx {
  INCLUDEPATH += ../../
  LIBS += -ltiff
  
  QMAKE_LFLAGS += "-Wl,-rpath=\'\$${ORIGIN}/../ITK\'"
  QMAKE_LFLAGS += "-Wl,-rpath=\'\$${ORIGIN}/../sharedlibs\'"

  SOURCES = tiffplugin.cpp
 }
}

macx {
  INCLUDEPATH += ../../

  LIBS += -ltiff
  
  SOURCES = tiffplugin.cpp
}

HEADERS = tiffplugin.h

