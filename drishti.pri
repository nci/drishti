include(version.pri)

QT += widgets core gui

HEADERS += commonqtclasses.h

Windows_Setup = Win64

#----------------------------------------------------------------
# Windows setup for 64-bit system
contains(Windows_Setup, Win64) {
  win32 {
    message(Win64 setup)

    contains(DRISHTI_DEFINES, RENDERER) {
      INCLUDEPATH += c:\Qt\Qt-5.15.2\libQGLViewer\libQGLViewer-2.6.4 \
  	c:\cygwin64\home\acl900\drishtilib\freeglut\include \
 	c:\cygwin64\home\acl900\drishtilib\glew-2.1.0\include
  
      QMAKE_LIBDIR += c:\Qt\Qt-5.15.2\libQGLViewer\libQGLViewer-2.6.4\lib \
  	c:\cygwin64\home\acl900\drishtilib\freeglut\lib\x64 \
	c:\cygwin64\home\acl900\drishtilib\glew-2.1.0\build\lib\Release
    }
  
    contains(DRISHTI_DEFINES, ITK) {
      ITKVer = 5.0
      InsightToolkit = C:\InsightToolkit-$${ITKVer}.1
      ITK = C:\ITK
  
      QMAKE_LIBDIR += C:\ITK\lib\Release
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
