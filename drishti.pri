include(version.pri)

win32 {
  contains(DRISHTI_DEFINES, RENDERER)
  {
    INCLUDEPATH += c:\Qt\include \
	c:\drishtilib \
	c:\drishtilib\glew-1.5.4\include

    QMAKE_LIBDIR += c:\Qt\lib \
	c:\drishtilib\GL \
	c:\drishtilib\glew-1.5.4\lib
  }

  contains(DRISHTI_DEFINES, IMPORT)
  {
    INCLUDEPATH += c:\drishtilib\netcdf\include
    QMAKE_LIBDIR += c:\drishtilib\netcdf\lib
  }

  contains(DRISHTI_DEFINES, NETCDF)
  {
     INCLUDEPATH += c:\drishtilib\netcdf\include
     QMAKE_LIBDIR += c:\drishtilib\netcdf\lib
  }

  contains(DRISHTI_DEFINES, ITK)
  {
    ITKVer = 4.3
    InsightToolkit = D:\InsightToolkit-$${ITKVer}.1
    ITK = D:\ITK

    QMAKE_LIBDIR += d:\ITK\lib\Release
  }
}

unix {
 !macx {
  contains(DRISHTI_DEFINES, RENDERER)
  {
    QMAKE_LFLAGS += -Xlinker -rpath -Xlinker \'\$\$ORIGIN/sharedlibs\' 
    QMAKE_LFLAGS += -Xlinker -rpath -Xlinker \'\$\$ORIGIN/ITK\' 

    QMAKE_LIBDIR += /usr/lib /usr/lib/x86_64-linux-gnu
  }

  contains(DRISHTI_DEFINES, IMPORT)
  {
    QMAKE_LFLAGS += -Xlinker -rpath -Xlinker \'\$\$ORIGIN/sharedlibs\' 
    QMAKE_LFLAGS += -Xlinker -rpath -Xlinker \'\$\$ORIGIN/ITK\' 

    QMAKE_LIBDIR += /usr/lib /usr/lib/x86_64-linux-gnu
  }

  contains(DRISHTI_DEFINES, NETCDF)
  {
    INCLUDEPATH += /usr/include/netcdf-3
  }

  contains(DRISHTI_DEFINES, TIFF)
  {
    QMAKE_LIBDIR += /usr/lib/x86_64-linux-gnu
  }

  contains(DRISHTI_DEFINES, ITK)
  {
    ITKVer = 4.3
    InsightToolkit = /home/acl900/InsightToolkit-$${ITKVer}.1
    ITK = /home/acl900/ITK

    QMAKE_LIBDIR += /home/acl900/ITK/lib
    
    options = $$find(DRISHTI_DEFINES, "RENDERER")
    count(options, 0)
     {
       QMAKE_LIBDIR += /usr/lib /usr/lib/x86_64-linux-gnu
     }
   }
  }
}

macx {
  contains(DRISHTI_DEFINES, RENDERER)
  {
    INCLUDEPATH += /Users/acl900/Library/Frameworks/QGLViewer.framework/Headers \
	/usr/local/include

    LIBS += -L/usr/local/lib
    LIBS += -F/Users/acl900/Library/Frameworks
    LIBS += -L/Users/acl900/Library/Frameworks
    LIBS += -framework ApplicationServices
  }

  contains(DRISHTI_DEFINES, IMPORT)
  {
    INCLUDEPATH += /usr/local/include	
    QMAKE_LIBDIR += /usr/local/lib
  }

  contains(DRISHTI_DEFINES, TIFF)
  {
    INCLUDEPATH += /usr/local/include
    QMAKE_LIBDIR += /usr/local/lib
  }

  contains(DRISHTI_DEFINES, ITK)
  {
    QMAKE_CFLAGS_X86_64 += -mmacosx-version-min=10.7
    QMAKE_CXXFLAGS_X86_64 = $$QMAKE_CFLAGS_X86_64

    ITKVer = 4.3
    InsightToolkit = /Users/acl900/InsightToolkit-$${ITKVer}.1
    ITK = /Users/acl900/ITK

    QMAKE_LIBDIR += /Users/acl900/ITK/lib
  }
}
