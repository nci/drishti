win32 {
  ITKVer = 4.3
  InsightToolkit = D:\InsightToolkit-$${ITKVer}.1
  ITK = D:\ITK

  QMAKE_LIBDIR += d:\ITK\lib\Release
}

unix {
 !macx {
  ITKVer = 4.3
  InsightToolkit = /home/acl900/InsightToolkit-$${ITKVer}.1
  ITK = /home/acl900/ITK

  QMAKE_LIBDIR += /home/acl900/ITK/lib
 }
}

macx {
  ITKVer = 4.3
  InsightToolkit = /Users/acl900/InsightToolkit-$${ITKVer}.1
  ITK = /Users/acl900/ITK

  QMAKE_LIBDIR += /Users/acl900/ITK/lib
}
