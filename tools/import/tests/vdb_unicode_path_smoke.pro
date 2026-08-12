QT += widgets

CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = vdb_unicode_path_smoke

INCLUDEPATH += ../../../common/src/vdb

win32 {
  INCLUDEPATH += $$VCPKG_INCLUDE_PATH $$DRISHTI_EXTRA_INCLUDEPATH
  QMAKE_LIBDIR += $$VCPKG_LIBRARY_PATH $$DRISHTI_EXTRA_LIBDIR
  isEmpty(DRISHTI_IMATH_LIB): DRISHTI_IMATH_LIB = Imath-3_2.lib
  isEmpty(DRISHTI_OPENVDB_LIB): DRISHTI_OPENVDB_LIB = openvdb.lib
  QMAKE_CXXFLAGS *= /std:c++17 /bigobj
  LIBS += $$DRISHTI_IMATH_LIB $$DRISHTI_OPENVDB_LIB
}

unix:!macx {
  LIBS += -lopenvdb -ltbb -lImath
}

SOURCES += vdb_unicode_path_smoke.cpp \
           ../../../common/src/vdb/vdbvolume.cpp

HEADERS += ../../../common/src/vdb/vdbvolume.h
