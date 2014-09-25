include(version.pri)

QT += widgets core gui

HEADERS += commonqtclasses.h

Windows_Setup = Win64

# Windows setup for 32-bit system
contains(Windows_Setup, Win32) {
  win32 {
    message(Win32 setup)

    contains(DRISHTI_DEFINES, RENDERER) {
      INCLUDEPATH += c:\Qt\5.2.1\include \
  	c:\Qt\libQGLViewer-2.5.1 \
  	c:\drishtilib \
  	c:\drishtilib\glew-1.11.0\include
  
      QMAKE_LIBDIR += c:\Qt\5.2.1\lib \
  	c:\Qt\libQGLViewer-2.5.1\lib \
  	c:\drishtilib\GL \
  	c:\drishtilib\glew-1.11.0\lib\Release\Win32
    }
  
    contains(DRISHTI_DEFINES, IMPORT) {
      INCLUDEPATH += c:\drishtilib\netcdf\include
      QMAKE_LIBDIR += c:\drishtilib\netcdf\lib
    }
  
    contains(DRISHTI_DEFINES, NETCDF) {
       INCLUDEPATH += c:\drishtilib\netcdf\include
       QMAKE_LIBDIR += c:\drishtilib\netcdf\lib
    }
  
    contains(DRISHTI_DEFINES, ITK) {
      ITKVer = 4.3
      InsightToolkit = D:\InsightToolkit-$${ITKVer}.1
      ITK = D:\ITK
  
      QMAKE_LIBDIR += d:\ITK\lib\Release
    }
  }
}
#----------------------------------------------------------------

#----------------------------------------------------------------
# Windows setup for 64-bit system
contains(Windows_Setup, Win64) {
  win32 {
    message(Win64 setup)

    contains(DRISHTI_DEFINES, RENDERER) {
      INCLUDEPATH += c:\Qt\5.2.1\include \
  	c:\Qt\libQGLViewer-2.5.1 \
  	c:\cygwin\home\acl900\drishtilib \
  	c:\cygwin\home\acl900\drishtilib\GL\glut-3.7.6\include \
 	c:\cygwin\home\acl900\drishtilib\glew-1.11.0\include
  
      QMAKE_LIBDIR += c:\Qt\5.2.1\lib \
  	c:\Qt\libQGLViewer-2.5.1\lib \
  	c:\cygwin\home\acl900\drishtilib\GL \
  	c:\cygwin\home\acl900\drishtilib\GL\glut-3.7.6\lib\glut\Release \
	c:\cygwin\home\acl900\drishtilib\glew-1.11.0\lib\Release\x64
    }
  
    contains(DRISHTI_DEFINES, IMPORT) {
      INCLUDEPATH += c:\cygwin\home\acl900\drishtilib\netcdf\include
      QMAKE_LIBDIR += c:\cygwin\home\acl900\drishtilib\netcdf\lib \
    }
  
    contains(DRISHTI_DEFINES, NETCDF) {
       INCLUDEPATH += c:\cygwin\home\acl900\drishtilib\netcdf\include
       QMAKE_LIBDIR += c:\cygwin\home\acl900\drishtilib\netcdf\lib \
    }
  
    contains(DRISHTI_DEFINES, ITK) {
      ITKVer = 4.3
      InsightToolkit = D:\InsightToolkit-$${ITKVer}.1
      ITK = D:\ITK
  
      QMAKE_LIBDIR += d:\ITK\lib\Release
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

  contains(DRISHTI_DEFINES, TIFF) {
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

Facility_Name = MassiveAtMonashUniversity

contains(Facility_Name, VirtualBox) {
  unix {
   !macx {
    message(VirtualBox setup)
    contains(DRISHTI_DEFINES, RENDERER) {
      QMAKE_LFLAGS += -Xlinker -rpath -Xlinker \'\$\$ORIGIN/sharedlibs\' 
      QMAKE_LFLAGS += -Xlinker -rpath -Xlinker \'\$\$ORIGIN/ITK\' 
  
      QMAKE_LIBDIR += /usr/lib /usr/lib/x86_64-linux-gnu
    }
  
    contains(DRISHTI_DEFINES, IMPORT) {
      QMAKE_LFLAGS += -Xlinker -rpath -Xlinker \'\$\$ORIGIN/sharedlibs\' 
      QMAKE_LFLAGS += -Xlinker -rpath -Xlinker \'\$\$ORIGIN/ITK\' 
  
      QMAKE_LIBDIR += /usr/lib /usr/lib/x86_64-linux-gnu
    }
  
    contains(DRISHTI_DEFINES, NETCDF) {
      INCLUDEPATH += /usr/include/netcdf-3
    }
  
    contains(DRISHTI_DEFINES, TIFF) {
      QMAKE_LIBDIR += /usr/lib/x86_64-linux-gnu
    }
  
    contains(DRISHTI_DEFINES, ITK) {
      ITKVer = 4.3
      InsightToolkit = /home/acl900/InsightToolkit-$${ITKVer}.1
      ITK = /home/acl900/ITK
  
      QMAKE_LIBDIR += /home/acl900/ITK/lib
      
      options = $$find(DRISHTI_DEFINES, "RENDERER")
      count(options, 0) {
         QMAKE_LIBDIR += /usr/lib /usr/lib/x86_64-linux-gnu
       }
     }
    }
  }
}

contains(Facility_Name, MassiveAtMonashUniversity) {
  unix {
   !macx {
    message(MASSIVE facility at Monash University setup)
    contains(DRISHTI_DEFINES, RENDERER) {
     INCLUDEPATH += /usr/local/libqglviewer/2.4.0/include \
                    /usr/local/glut/3.7/include \
                    /usr/local/glew/1.10.0/include
  
      QMAKE_LIBDIR += /usr/lib /usr/lib/x86_64-linux-gnu /usr/local/libqglviewer/2.4.0/lib /usr/local/glut/3.7/lib /usr/local/glew/1.10.0/lib64 
    }
  
    contains(DRISHTI_DEFINES, IMPORT) {
      QMAKE_LIBDIR += /usr/lib /usr/lib/x86_64-linux-gnu
    }
  
    contains(DRISHTI_DEFINES, NETCDF) {
      INCLUDEPATH += /usr/local/netcdf/4.1.1-gcc/include
      QMAKE_LIBDIR += /usr/local/netcdf/4.1.1-gcc/lib
    }
  
    contains(DRISHTI_DEFINES, TIFF) {
      QMAKE_LIBDIR += /usr/lib/x86_64-linux-gnu
    }
  
    contains(DRISHTI_DEFINES, ITK) {
      INCLUDEPATH += /usr/local/include \
                     /usr/local/itk/4.4.0/include/ITK-4.4
  
      ITKVer = 4.4
      InsightToolkit = /home/acl900/InsightToolkit-$${ITKVer}.1
      ITK = /home/acl900/ITK
  
      QMAKE_LIBDIR += /usr/local/itk/4.4.0/lib
      
      options = $$find(DRISHTI_DEFINES, "RENDERER")
      count(options, 0) {
         QMAKE_LIBDIR += /usr/lib /usr/lib/x86_64-linux-gnu
       }
     }
    }
  }
}

#----------------------------------------------------------------
