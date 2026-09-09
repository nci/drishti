TEMPLATE = lib

include(../../../../drishti.pri )

CONFIG += release plugin
CONFIG += c++17

TARGET = zarrplugin

HEADERS = zarrplugin.h

SOURCES = zarrplugin.cpp

include(../plugins.pri)

win32 {
  INCLUDEPATH += ../../

  INCLUDEPATH += $$VCPKG_INCLUDE_PATH
  QMAKE_LIBDIR += $$VCPKG_LIBRARY_PATH

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
