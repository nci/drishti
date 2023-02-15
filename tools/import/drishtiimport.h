#ifndef DRISHTIIMPORT
#define DRISHTIIMPORT

#include "commonqtclasses.h"
#include "ui_drishtiimport.h"

class RemapWidget;

class DrishtiImport : public QMainWindow
{
  Q_OBJECT

 public :
  DrishtiImport(QWidget *parent=0);

 protected :
  void dragEnterEvent(QDragEnterEvent*);
  void dropEvent(QDropEvent*);
  void closeEvent(QCloseEvent *event);
  
 private slots :
  void on_action8_bit_triggered();
  void on_action16_bit_triggered();

  void on_actionTimeSeries_triggered();
  void on_actionMergeVolumes_triggered();

  void on_actionSave_Images_triggered();
  void on_actionSave_As_triggered();
  void on_actionBatch_Process_triggered();
  void on_actionExit_triggered();

  void on_actionExport_Mesh_triggered();

  void on_loadLimits_triggered();
  void on_saveLimits_triggered();
  void on_saveImage_triggered();

  void on_actionMimics_triggered();

  void loadDirectory();
  void loadFiles();

 private :
  Ui::DrishtiImport ui;

  QStringList m_pluginFileTypes;
  QStringList m_pluginDirTypes;
  QStringList m_pluginFileDLib;
  QStringList m_pluginDirDLib;

  RemapWidget *m_remapWidget;

  void registerPlugins();

  void loadSettings();
  void saveSettings();

  void Old2New(QStringList);

  void loadDirectory(QString, int);
  void loadFiles(QStringList, int);

  void convertDirectories(QStringList, int);
};

#endif
