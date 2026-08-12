QT += widgets core gui

isEmpty(DRISHTI_BIN_DIR): DRISHTI_BIN_DIR = $$(DRISHTI_BIN_DIR)
isEmpty(DRISHTI_BIN_DIR): DRISHTI_BIN_DIR = $$clean_path($$PWD/../../../bin)
isEmpty(DRISHTI_IMPORT_PLUGIN_DIR): DRISHTI_IMPORT_PLUGIN_DIR = $$clean_path($$DRISHTI_BIN_DIR/importplugins)

win32 {
  DESTDIR = $$DRISHTI_IMPORT_PLUGIN_DIR
}

unix {
 !macx {
  DESTDIR = ../../../../bin/importplugins
 }
}

macx {
  DESTDIR = ../../../../bin/drishtiimport.app/importplugins
}

