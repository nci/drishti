TEMPLATE = app
TARGET = tiffdecodehelper
QT += core
CONFIG += console release

# Reuse the canonical vcpkg/Qt dependency variables used by the other import
# plugins.  Without this include VCPKG_INCLUDE_PATH is empty and the helper
# cannot see the unified TIFF headers.
include(../../../../drishti.pri)

isEmpty(DRISHTI_BIN_DIR): DRISHTI_BIN_DIR = $$(DRISHTI_BIN_DIR)
isEmpty(DRISHTI_BIN_DIR): DRISHTI_BIN_DIR = $$clean_path($$PWD/../../../../bin)
DESTDIR = $$DRISHTI_BIN_DIR

win32 {
  isEmpty(TIFF_INCLUDE_PATH): TIFF_INCLUDE_PATH = $$VCPKG_INCLUDE_PATH
  isEmpty(TIFF_LIBRARY_PATH): TIFF_LIBRARY_PATH = $$VCPKG_LIBRARY_PATH
  INCLUDEPATH += $$TIFF_INCLUDE_PATH
  QMAKE_LIBDIR += $$TIFF_LIBRARY_PATH
  LIBS += tiff.lib
}

unix:!macx {
  INCLUDEPATH += /usr/include
  QMAKE_LIBDIR += /usr/lib /usr/lib/x86_64-linux-gnu
  LIBS += -ltiff
}

macx {
  LIBS += -ltiff
}

INCLUDEPATH += ../../

SOURCES += main.cpp \
           ../../tiffpagevalidation.cpp

HEADERS += ../../tiffpagevalidation.h
