TEMPLATE = lib

DRISHTI_DEFINES = IMPORT TIFF
include(../../../../drishti.pri )

CONFIG += release plugin
QT += concurrent

TARGET = tiffplugin

include(../plugins.pri)

win32 {
  SOURCES = tiffplugin.cpp \
            ../../tiffpagevalidation.cpp
  INCLUDEPATH += ./ ../../

  isEmpty(TIFF_INCLUDE_PATH) {
    TIFF_INCLUDE_PATH = $$VCPKG_INCLUDE_PATH
  }
  isEmpty(TIFF_LIBRARY_PATH) {
    TIFF_LIBRARY_PATH = $$VCPKG_LIBRARY_PATH
  }
  !exists($$TIFF_INCLUDE_PATH/tiffio.h) {
    TIFF_INCLUDE_PATH = $$[QT_INSTALL_PREFIX]/include
    TIFF_LIBRARY_PATH = $$[QT_INSTALL_PREFIX]/lib
  }

  INCLUDEPATH = $$TIFF_INCLUDE_PATH $$INCLUDEPATH
  QMAKE_LIBDIR = $$TIFF_LIBRARY_PATH $$QMAKE_LIBDIR
  LIBS += tiff.lib
}

unix {
 !macx {
  INCLUDEPATH += ../../
  LIBS += -ltiff
  
  QMAKE_LFLAGS += "-Wl,-rpath=\'\$${ORIGIN}/../ITK\'"
  QMAKE_LFLAGS += "-Wl,-rpath=\'\$${ORIGIN}/../sharedlibs\'"

  SOURCES = tiffplugin.cpp \
            ../../tiffpagevalidation.cpp
 }
}

macx {
  INCLUDEPATH += ../../

  LIBS += -ltiff
  
  SOURCES = tiffplugin.cpp \
            ../../tiffpagevalidation.cpp
}

SOURCES += ../../importmemoryadmission.cpp

HEADERS = tiffplugin.h \
          ../../importmemoryadmission.h \
          ../../tiffpagevalidation.h

