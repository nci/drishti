include(version.pri)

QT += widgets core gui

HEADERS += commonqtclasses.h

Windows_Setup = Win64

# Dependency locations can be supplied on the qmake command line or through
# matching environment variables.  The historical paths remain fallbacks so
# existing developer machines keep working.
isEmpty(DRISHTI_VCPKG_TRIPLET): DRISHTI_VCPKG_TRIPLET = $$(DRISHTI_VCPKG_TRIPLET)
isEmpty(DRISHTI_VCPKG_TRIPLET): DRISHTI_VCPKG_TRIPLET = x64-windows

isEmpty(DRISHTI_VCPKG_ROOT): DRISHTI_VCPKG_ROOT = $$(DRISHTI_VCPKG_ROOT)
isEmpty(DRISHTI_VCPKG_ROOT): DRISHTI_VCPKG_ROOT = $$(VCPKG_ROOT)
isEmpty(DRISHTI_VCPKG_ROOT): DRISHTI_VCPKG_ROOT = C:/Apps/vcpkg

isEmpty(VCPKG_INCLUDE_PATH): VCPKG_INCLUDE_PATH = $$clean_path($$DRISHTI_VCPKG_ROOT/installed/$$DRISHTI_VCPKG_TRIPLET/include)
isEmpty(VCPKG_LIBRARY_PATH): VCPKG_LIBRARY_PATH = $$clean_path($$DRISHTI_VCPKG_ROOT/installed/$$DRISHTI_VCPKG_TRIPLET/lib)

isEmpty(DRISHTI_QGLVIEWER_ROOT): DRISHTI_QGLVIEWER_ROOT = $$(DRISHTI_QGLVIEWER_ROOT)
isEmpty(DRISHTI_QGLVIEWER_ROOT): DRISHTI_QGLVIEWER_ROOT = C:/Qt/Qt-5/libQGLViewer/libQGLViewer-2.6.4
isEmpty(QGLVIEWER_INCLUDE_PATH): QGLVIEWER_INCLUDE_PATH = $$clean_path($$DRISHTI_QGLVIEWER_ROOT)
isEmpty(QGLVIEWER_LIBRARY_PATH): QGLVIEWER_LIBRARY_PATH = $$clean_path($$DRISHTI_QGLVIEWER_ROOT/lib)

isEmpty(DRISHTI_ITK_VERSION): DRISHTI_ITK_VERSION = $$(DRISHTI_ITK_VERSION)
isEmpty(DRISHTI_ITK_VERSION): DRISHTI_ITK_VERSION = 5.0
isEmpty(DRISHTI_ITK_SOURCE_ROOT): DRISHTI_ITK_SOURCE_ROOT = $$(DRISHTI_ITK_SOURCE_ROOT)
isEmpty(DRISHTI_ITK_SOURCE_ROOT): DRISHTI_ITK_SOURCE_ROOT = C:/InsightToolkit-$${DRISHTI_ITK_VERSION}.1
isEmpty(DRISHTI_ITK_BUILD_ROOT): DRISHTI_ITK_BUILD_ROOT = $$(DRISHTI_ITK_BUILD_ROOT)
isEmpty(DRISHTI_ITK_BUILD_ROOT): DRISHTI_ITK_BUILD_ROOT = C:/ITK

isEmpty(DRISHTI_PYTHON_ROOT): DRISHTI_PYTHON_ROOT = $$(DRISHTI_PYTHON_ROOT)
isEmpty(DRISHTI_PYTHON_ROOT): DRISHTI_PYTHON_ROOT = C:/Apps/Python314
isEmpty(DRISHTI_PYTHON_LIB): DRISHTI_PYTHON_LIB = $$(DRISHTI_PYTHON_LIB)
isEmpty(DRISHTI_PYTHON_LIB): DRISHTI_PYTHON_LIB = python314.lib

isEmpty(DRISHTI_BIN_DIR): DRISHTI_BIN_DIR = $$(DRISHTI_BIN_DIR)
isEmpty(DRISHTI_BIN_DIR): DRISHTI_BIN_DIR = $$clean_path($$PWD/bin)
isEmpty(DRISHTI_IMPORT_PLUGIN_DIR): DRISHTI_IMPORT_PLUGIN_DIR = $$clean_path($$DRISHTI_BIN_DIR/importplugins)
isEmpty(DRISHTI_RENDER_PLUGIN_DIR): DRISHTI_RENDER_PLUGIN_DIR = $$clean_path($$DRISHTI_BIN_DIR/renderplugins)
isEmpty(DRISHTI_MOP_PLUGIN_DIR): DRISHTI_MOP_PLUGIN_DIR = $$clean_path($$DRISHTI_BIN_DIR/mopplugins)

isEmpty(DRISHTI_QGLVIEWER_LIB): DRISHTI_QGLVIEWER_LIB = QGLViewer2.lib
isEmpty(DRISHTI_COMMON_LIB): DRISHTI_COMMON_LIB = common.lib
isEmpty(DRISHTI_GLMEDIA_LIB): DRISHTI_GLMEDIA_LIB = glmedia.lib
isEmpty(DRISHTI_LEGACY_NETCDF_LIB): DRISHTI_LEGACY_NETCDF_LIB = netcdfcpp.lib
isEmpty(DRISHTI_ASSIMP_LIB): DRISHTI_ASSIMP_LIB = assimp-vc145-mt.lib
isEmpty(DRISHTI_IMATH_LIB): DRISHTI_IMATH_LIB = Imath-3_2.lib
isEmpty(DRISHTI_OPENVDB_LIB): DRISHTI_OPENVDB_LIB = openvdb.lib
isEmpty(DRISHTI_VDB_LIB): DRISHTI_VDB_LIB = vdb.lib
isEmpty(DRISHTI_GMSH_LIB): DRISHTI_GMSH_LIB = gmsh.dll.lib
isEmpty(DRISHTI_BLOSC_LIB): DRISHTI_BLOSC_LIB = blosc.lib
isEmpty(DRISHTI_GLEW_LIB): DRISHTI_GLEW_LIB = glew32.lib
isEmpty(DRISHTI_FREEGLUT_LIB): DRISHTI_FREEGLUT_LIB = freeglut.lib
isEmpty(DRISHTI_OPENGL_LIBS): DRISHTI_OPENGL_LIBS = opengl32.lib glu32.lib
isEmpty(DRISHTI_NETCDF_LIBS): DRISHTI_NETCDF_LIBS = netcdf-cxx4.lib netcdf.lib
isEmpty(DRISHTI_FFMPEG_LIBS): DRISHTI_FFMPEG_LIBS = avutil.lib avcodec.lib avformat.lib swresample.lib swscale.lib

!isEmpty(DRISHTI_EXTRA_INCLUDEPATH): INCLUDEPATH += $$DRISHTI_EXTRA_INCLUDEPATH
!isEmpty(DRISHTI_EXTRA_LIBDIR): QMAKE_LIBDIR += $$DRISHTI_EXTRA_LIBDIR
!isEmpty(DRISHTI_EXTRA_LIBS): LIBS += $$DRISHTI_EXTRA_LIBS

#----------------------------------------------------------------
# Windows setup for 64-bit system
contains(Windows_Setup, Win64) {
  win32 {
    message(Win64 setup)

    INCLUDEPATH += $$VCPKG_INCLUDE_PATH
    QMAKE_LIBDIR += $$VCPKG_LIBRARY_PATH

    contains(DRISHTI_DEFINES, RENDERER) {
      INCLUDEPATH += $$QGLVIEWER_INCLUDE_PATH
      QMAKE_LIBDIR += $$QGLVIEWER_LIBRARY_PATH
    }
  	
    contains(DRISHTI_DEFINES, ITK) {
      ITKVer = $$DRISHTI_ITK_VERSION
      InsightToolkit = $$DRISHTI_ITK_SOURCE_ROOT
      ITK = $$DRISHTI_ITK_BUILD_ROOT
  
      QMAKE_LIBDIR += $$ITK/lib/Release
    }
  }
}
#----------------------------------------------------------------

#----------------------------------------------------------------
# MacOSX setup
macx {
  contains(DRISHTI_DEFINES, RENDERER) {
    INCLUDEPATH += /Users/acl900/Library/Frameworks/QGLViewer.framework/Headers \
	/usr/local/include

    LIBS += -L/usr/local/lib
    LIBS += -F/Users/acl900/Library/Frameworks
    LIBS += -L/Users/acl900/Library/Frameworks
    LIBS += -framework ApplicationServices
  }

  contains(DRISHTI_DEFINES, IMPORT) {
    INCLUDEPATH += /usr/local/include	
    QMAKE_LIBDIR += /usr/local/lib
  }

  contains(DRISHTI_DEFINES, ITK) {
    QMAKE_CFLAGS_X86_64 += -mmacosx-version-min=10.7
    QMAKE_CXXFLAGS_X86_64 = $$QMAKE_CFLAGS_X86_64

    ITKVer = 4.3
    InsightToolkit = /Users/acl900/InsightToolkit-$${ITKVer}.1
    ITK = /Users/acl900/ITK

    QMAKE_LIBDIR += /Users/acl900/ITK/lib
  }
}
#----------------------------------------------------------------

#----------------------------------------------------------------

#Linux setup

Facility_Name = Ubuntu 

contains(Facility_Name, Ubuntu) {
  unix {
   !macx {
    message($$Facility_Name setup)

    QMAKE_LFLAGS += "-Wl,-rpath=\'\$${ORIGIN}/ITK\'"
    QMAKE_LFLAGS += "-Wl,-rpath=\'\$${ORIGIN}/sharedlibs\'"
    QMAKE_LFLAGS += "-Wl,-rpath=\'\$${ORIGIN}/../sharedlibs\'"

    contains(DRISHTI_DEFINES, RENDERER) {

      INCLUDEPATH += /usr/include \
                     /home/acl900/drishtilib/libQGLViewer-2.6.4 \
                     /home/acl900/drishtilib/glew-2.2.0/include \
                     /home/acl900/drishtilib/assimp/include \
                     /home/acl900/drishtilib/assimp/build/include

      QMAKE_CXXFLAGS += -fno-stack-protector

      QMAKE_LIBDIR += /usr/lib \
      		      /usr/lib/x86_64-linux-gnu \
		      /home/acl900/drishtilib/libQGLViewer-2.6.4/QGLViewer \
                      /home/acl900/drishtilib/glew-2.2.0/lib \
                      /home/acl900/drishtilib/assimp/build/bin

      LIBS += -lQGLViewer-qt5 \
      	      -lGLEW \
	      -lassimp
    }
  
    contains(DRISHTI_DEFINES, IMPORT) {
      QMAKE_CXXFLAGS += -fno-stack-protector

      QMAKE_LIBDIR += /usr/lib /usr/lib/x86_64-linux-gnu
    }
  
  
    contains(DRISHTI_DEFINES, TIFF) {
      QMAKE_LIBDIR += /usr/lib/x86_64-linux-gnu
    }
  
    contains(DRISHTI_DEFINES, ITK) {
      ITKVer = 5.2
      InsightToolkit = /home/acl900/drishtilib/InsightToolkit-$${ITKVer}.1
      ITK = /home/acl900/drishtilib/ITK

      QMAKE_LIBDIR += /home/acl900/drishtilib/ITK/lib
      
      options = $$find(DRISHTI_DEFINES, "RENDERER")
      count(options, 0) {
         QMAKE_LIBDIR += /usr/lib /usr/lib/x86_64-linux-gnu
       }
     }
    }
  }
}

#----------------------------------------------------------------
#----------------------------------------------------------------
