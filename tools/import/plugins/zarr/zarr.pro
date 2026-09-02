TEMPLATE = lib

include(../../../../drishti.pri )

CONFIG += release plugin
CONFIG += c++17

TARGET = zarrplugin

HEADERS = zarrplugin.h

SOURCES = zarrplugin.cpp

include(../plugins.pri)

# libzarr (https://github.com/kharchenkolab/libzarr) is a header-only C++17 library.
LIBZARR_INCLUDE_PATH = C:/Apps/libzarr

win32 {
  INCLUDEPATH += ../../

  INCLUDEPATH += $$LIBZARR_INCLUDE_PATH/third_party   # vendored nlohmann/json (first)
  INCLUDEPATH += $$LIBZARR_INCLUDE_PATH/include       # libzarr core headers
  INCLUDEPATH += $$VCPKG_INCLUDE_PATH                 # blosc.h / zlib.h / zstd.h

  QMAKE_LIBDIR += $$VCPKG_LIBRARY_PATH

  DEFINES += LIBZARR_HAS_ZLIB LIBZARR_HAS_BLOSC LIBZARR_HAS_ZSTD
  DEFINES += NOMINMAX  # windows.h min/max macros clobber libzarr's std::min/std::max

  LIBS += blosc.lib zlib.lib zstd.lib
}

unix {
!macx {
  INCLUDEPATH += ../../
  INCLUDEPATH += $$LIBZARR_INCLUDE_PATH/third_party   # vendored nlohmann/json (first)
  INCLUDEPATH += $$LIBZARR_INCLUDE_PATH/include       # libzarr core headers

  DEFINES += LIBZARR_HAS_ZLIB LIBZARR_HAS_BLOSC LIBZARR_HAS_ZSTD

  LIBS += -lblosc -lz -lzstd
}
}
