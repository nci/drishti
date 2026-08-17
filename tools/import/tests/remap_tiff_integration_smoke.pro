TEMPLATE = app

DRISHTI_DEFINES = IMPORT
include(../../../drishti.pri)

QT += widgets core gui xml concurrent
CONFIG += release console c++17
TARGET = remap_tiff_integration_smoke

INCLUDEPATH += .. \
               ../../../common/src/vdb \
               ../../../common/src/widgets \
               ../../../common/src/pybind \
               ../../../common/src/mesh \
               ../../../common/src \
               $$VCPKG_INCLUDE_PATH

win32 {
  isEmpty(DRISHTI_COMMON_LIB_DIR): DRISHTI_COMMON_LIB_DIR = $$clean_path($$PWD/../../../common/lib)
  isEmpty(TIFF_LIBRARY_PATH): TIFF_LIBRARY_PATH = $$VCPKG_LIBRARY_PATH
  QMAKE_LIBDIR += $$DRISHTI_COMMON_LIB_DIR $$VCPKG_LIBRARY_PATH $$TIFF_LIBRARY_PATH
  QMAKE_CXXFLAGS *= /std:c++17
  LIBS += $$DRISHTI_IMATH_LIB \
          $$DRISHTI_OPENVDB_LIB \
          $$DRISHTI_VDB_LIB \
          $$DRISHTI_GMSH_LIB \
          tiff.lib
  INCLUDEPATH += $$clean_path($$DRISHTI_PYTHON_ROOT/include)
  QMAKE_LIBDIR += $$clean_path($$DRISHTI_PYTHON_ROOT/libs)
  LIBS += $$DRISHTI_PYTHON_LIB
}

FORMS += ../remapwidget.ui \
         ../savepvldialog.ui \
         ../fileslistdialog.ui \
         ../../../common/src/widgets/propertyeditor.ui

HEADERS += ../global.h \
           ../common.h \
           ../commonqtclasses.h \
           ../staticfunctions.h \
           ../fileslistdialog.h \
           ../remapwidget.h \
           ../remaphistogramline.h \
           ../remaphistogramwidget.h \
           ../remapimage.h \
           ../myslider.h \
           ../raw2pvl.h \
           ../importmemoryadmission.h \
           ../tiffinputrouting.h \
           ../tiffpagevalidation.h \
           ../volumepluginvalidation.h \
           ../savepvldialog.h \
           ../volumefilemanager.h \
           ../volumedata.h \
           ../scriptsplugin.h \
           ../../../common/src/pybind/pythonengine.h \
           ../../../common/src/widgets/propertyeditor.h \
           ../../../common/src/widgets/dcolordialog.h \
           ../../../common/src/widgets/dcolorwheel.h \
           ../../../common/src/widgets/gradienteditor.h \
           ../../../common/src/widgets/gradienteditorwidget.h \
           ../../../common/src/mesh/meshtools.h \
           ../../../common/src/pvlmanifest.h \
           ../../../common/src/memoryreservation.h \
           ../../../common/src/recoveryjournal.h

SOURCES += remap_tiff_integration_smoke.cpp \
           ../global.cpp \
           ../staticfunctions.cpp \
           ../fileslistdialog.cpp \
           ../remapwidget.cpp \
           ../remaphistogramline.cpp \
           ../remaphistogramwidget.cpp \
           ../remapimage.cpp \
           ../myslider.cpp \
           ../raw2pvl.cpp \
           ../importmemoryadmission.cpp \
           ../tiffinputrouting.cpp \
           ../tiffpagevalidation.cpp \
           ../volumepluginvalidation.cpp \
           ../savepvldialog.cpp \
           ../volumedata.cpp \
           ../volumefilemanager.cpp \
           ../scriptsplugin.cpp \
           ../../../common/src/pybind/pythonengine.cpp \
           ../../../common/src/widgets/propertyeditor.cpp \
           ../../../common/src/widgets/dcolordialog.cpp \
           ../../../common/src/widgets/dcolorwheel.cpp \
           ../../../common/src/widgets/gradienteditor.cpp \
           ../../../common/src/widgets/gradienteditorwidget.cpp \
           ../../../common/src/mesh/meshtools.cpp \
           ../../../common/src/pvlmanifest.cpp \
           ../../../common/src/memoryreservation.cpp \
           ../../../common/src/recoveryjournal.cpp
