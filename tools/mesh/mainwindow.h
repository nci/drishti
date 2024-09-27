#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "ui_mainwindow.h"

#include "viewer.h"
#include "lightingwidget.h"
#include "keyframe.h"
#include "keyframeeditor.h"
#include "bricks.h"
#include "brickswidget.h"
#include "globalwidget.h"
#include "meshinfowidget.h"

#include <QInputDialog>
#include <QNetworkAccessManager>
#include <QUrl>
#include <QProcess>

class MainWindow : public QMainWindow
{
  Q_OBJECT

 public :
  MainWindow(QWidget *parent=0);

 signals :
   void showMessage(QString, bool);
   void playKeyFrames(int, int, int);
   
 protected :
   virtual void dragEnterEvent(QDragEnterEvent*);
   virtual void dropEvent(QDropEvent*);
   void closeEvent(QCloseEvent*);

 private slots :
   void clearScene();

   void on_menuFile_aboutToShow();
   void on_menuView_aboutToShow();
   void on_menuImage_Size_aboutToShow();
   void on_menuToggle_aboutToShow();
   void on_menuHelp_aboutToShow();

   void on_menuFile_aboutToHide();
   void on_menuView_aboutToHide();
   void on_menuImage_Size_aboutToHide();
   void on_menuToggle_aboutToHide();
   void on_menuHelp_aboutToHide();

   void on_actionAbout_triggered();
   void on_actionDocumentation_triggered();
   void on_actionTriset_triggered();
   void on_actionLoad_Project_triggered();
   void on_actionSave_Project_triggered();
   void on_actionSave_ProjectAs_triggered();
   void on_actionSave_Image_triggered();
   void on_actionSave_ImageSequence_triggered();
   void on_actionSave_Movie_triggered();
   void on_actionExit_triggered();
   void on_actionImage_Size_triggered();
   void on_actionPerspective_triggered();
   void on_actionOrthographic_triggered();
   void on_actionStatusBar_triggered();
   void on_actionSpline_PositionInterpolation_triggered();
   void on_actionBoundingBox_triggered();

   void on_actionGrabMode_triggered(bool);

   void on_actionSplitBinarySTL_triggered();
  
   void on_actionClipPartial_triggered();
   void on_actionPaintMode_triggered();
   void on_actionHideBlack_triggered();
   void setPaintColor(QColor);
  
   void quitDrishti();
   void openRecentFile();   
   void loadProject(const char*);
   void saveProject(QString);
   void loadSurfaceMesh(QString);
   void GlewInit();
   void highlights(Highlights);
   void softness(float);
   void edges(float);
   void shadowIntensity(float);
   void valleyIntensity(float);
   void peakIntensity(float);
   void updateScaling();
   void setKeyFrame(Vec, Quaternion, int, QImage);
   void moveToKeyframe(int);
   void updateParameters(bool, bool, bool, Vec, QString);
   void switchBB();

   void addToCameraPath(QList<Vec>,QList<Vec>,QList<Vec>,QList<Vec>);


   void addRotationAnimation(int, float, int);

   void showRequiredPackages();
   void loadScript();

   void loadPlugin();
   void menuViewerFunction();

   void setShadowBox(bool);
   void setBackgroundColor();
   void lightDirectionChanged(Vec);

  void setMeshVisible(int, bool);
  void setMeshActive(int, bool);
  void updateMeshList(QStringList);
  void updateMeshList();

  void updateVoxelUnit();
  void updateVoxelSize();

  
 private :
   Ui::MainWindow ui;

   QString m_projectFileName;

   Viewer *m_Viewer;
  
   DrawHiresVolume *m_Hires;

   Bricks *m_bricks;
   BricksWidget *m_bricksWidget;

   MeshInfoWidget *m_meshInfoWidget;
  
   GlobalWidget *m_globalWidget;
  
   KeyFrame *m_keyFrame;
   KeyFrameEditor *m_keyFrameEditor;
   QDockWidget *m_dockKeyframe;

   LightingWidget *m_lightingWidget;

   QList <QAction*> m_recentFileActions;

   int m_saveRotationAnimation;
   int m_sraAxis, m_sraFrames;
   float m_sraAngle;

   int m_savePathAnimation;
   Vec m_pathAnimationPrevTang;
   Vec m_pathAnimationPrevSaxis;
   Vec m_pathAnimationPrevTaxis;
   QList<Vec> m_pathAnimationPoints;
   QList<Vec> m_pathAnimationTang;
   QList<Vec> m_pathAnimationSaxis;
   QList<Vec> m_pathAnimationTaxis;
   QList<Vec> m_pathAnimationVd;
   QList<Vec> m_pathAnimationUp;


   QList<QStringList> m_pluginList;
   QStringList m_pluginDll;;


   QProcess* m_scriptProcess;
   QString m_scriptDir;
   QStringList m_scriptsList;
   QStringList m_scriptsTitle;
   QStringList m_jsonFileList;;

   QMap<QString, MenuViewerFncPtr> m_menuViewerFunctions;

   QWidget *m_paintMenuWidget;
   QInputDialog *m_paintRadInput;
   QInputDialog *m_paintAlphaInput;
  

   QNetworkAccessManager *m_licenseManager;  

  int m_velMojaInterval, m_licenseInterval;

  void parvanaTapasa();
  
   void initializeRecentFiles();

   void createHiresLowresWindows();

   void initTagColors();

   void loadDummyVolume(int, int, int);


   void loadKeyFrames(const char*);
   void saveKeyFrames(const char*);


   void setTextureMemory();
   void loadSettings();
   void saveSettings();


   void updateRecentFileAction();



   void registerScripts();
   void runScript(int, bool);
  
   void registerPlugins();
   void registerMenuViewerFunctions();

   void runPlugin(int, bool);

   void createPaintMenu();

};


#endif
