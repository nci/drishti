#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "ui_mainwindow.h"

#include "viewer.h"
#include "transferfunctionmanager.h"
#include "transferfunctioneditorwidget.h"
#include "lightingwidget.h"
#include "volume.h"
#include "keyframe.h"
#include "keyframeeditor.h"
#include "viewseditor.h"
#include "bricks.h"
#include "brickswidget.h"
#include "volumeinformationwidget.h"
#include "preferenceswidget.h"
#include "classes.h"

class MainWindow : public QMainWindow
{
  Q_OBJECT

 public :
  MainWindow(QWidget *parent=0);

 signals :
   void showMessage(QString, bool);
   void playKeyFrames(int, int, int);
   void setVolumes(QList<int>);
   void refreshVolInfo(int, VolumeInformation);
   void refreshVolInfo(int, int, VolumeInformation);
   void addView(float,
		float,
		bool, bool,
		Vec,
		Vec, Quaternion,
		float,
		QImage,
		int,
		LightingInformation,
		QList<BrickInformation>,
		Vec, Vec,
		QList<SplineInformation>,
		int, int, QString, QString, QString);
   void histogramUpdated(QImage, QImage);
   void histogramUpdated(int*);

 protected :
   virtual void dragEnterEvent(QDragEnterEvent*);
   virtual void dropEvent(QDropEvent*);
   void closeEvent(QCloseEvent *event);

 private slots :
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
   void on_actionPoints_triggered();
   void on_actionPaths_triggered();
   void on_actionGrids_triggered();
   void on_actionNetwork_triggered();
   void on_actionTriset_triggered();
   void on_actionLoad_1_Volume_triggered();
   void on_actionLoad_2_Volumes_triggered();
   void on_actionLoad_3_Volumes_triggered();
   void on_actionLoad_4_Volumes_triggered();
   void on_actionLoad_Project_triggered();
   void on_actionLoad_TFfromproject_triggered();
   void on_actionImport_Keyframes_triggered();
   void on_actionSave_Project_triggered();
   void on_actionSave_ProjectAs_triggered();
   void on_actionSave_Image_triggered();
   void on_actionSave_ImageSequence_triggered();
   void on_actionSave_Movie_triggered();
   void on_actionExit_triggered();
   void on_actionImage_Size_triggered();
   void on_actionShadowRender_triggered();
   void on_actionPerspective_triggered();
   void on_actionOrthographic_triggered();
   void on_actionRedBlue_triggered();
   void on_actionRedCyan_triggered();
   void on_actionCrosseye_triggered();
   void on_actionFor3DTV_triggered();
   void on_actionStatusBar_triggered();
   void on_actionMouse_Grab_triggered();
   void on_actionSwitch_To1D_triggered();
   void on_actionEnable_Mask_triggered();
   void on_actionLinear_TextureInterpolation_triggered();
   void on_actionSpline_PositionInterpolation_triggered();
   void on_actionFlip_ImageX_triggered();
   void on_actionFlip_ImageY_triggered();
   void on_actionFlip_ImageZ_triggered();
   void on_actionBottom_Text_triggered();
   void on_actionDepthcue_triggered();
   void on_actionEmptySpaceSkip_triggered();
   void on_actionUse_dragvolume_triggered();
   void on_actionUse_dragvolumeforshadows_triggered();
   void on_actionUse_stillvolume_triggered();
   void on_actionVisibility_triggered();
   void on_actionAxes_triggered();
   void on_actionBoundingBox_triggered();

   void on_actionHiresMode_triggered();
   void on_actionNormal_triggered();
   void on_actionLow_triggered();
   void on_actionVeryLow_triggered();

   void quitDrishti();
   void changeTransferFunctionDisplay(int, QList<bool>);
   void updateComposite();
   void checkStateChanged(int, int, bool);
   void openRecentFile();   
   void loadProject(const char*);
   void loadTransferFunctionsOnly(const char*);
   void saveProject(const char*);
   void GlewInit();
   void loadLookupTable();
   void lightDirectionChanged(Vec);
   void applyLighting(bool);
   void applyEmissive(bool);
   void highlights(Highlights);
   void applyShadow(bool);
   void applyBackplane(bool);
   void lightDistanceOffset(float);
   void shadowBlur(float);
   void shadowScale(float);
   void shadowFOV(float);
   void shadowIntensity(float);
   void applyColoredShadow(bool);
   void shadowColorAttenuation(float, float, float);
   void backplaneShadowScale(float);
   void backplaneIntensity(float);
   void peel(bool);
   void peelInfo(int, float, float, float);
   void updateVolInfo(int);
   void updateVolInfo(int, int);
   void setVolumeNumber(int);
   void setVolumeNumber(int, int);
   void setRepeatType(int, bool);
   void updateScaling();
   void setView(Vec, Quaternion,
		QImage, float);
   void setKeyFrame(Vec, Quaternion,
		    int, float, float,
		   unsigned char*,
		   QImage);
   void moveToKeyframe(int);
   void updateTransferFunctionManager(QList<SplineInformation>);
   void updateMorph(bool);
   void updateFocus(float, float);
   void updateParameters(float, float, int, bool, bool, Vec,
			 QString,
			 int, int, QString, QString, QString);
   void updateParameters(bool, bool, Vec, QString,
			 int, int, QString, QString, QString,
			 int, bool, bool, float, bool, bool);
   void changeHistogram(int);
   void resetFlipImage();
   void switchBB();
   void switchAxis();
   void searchCaption(QStringList);  

   void sculpt(int, QList<Vec>, float, float, int);
   void fillPathPatch(QList<Vec>, int, int);
   void paintPathPatch(QList<Vec>, int, int);
   void addToCameraPath(QList<Vec>,QList<Vec>,QList<Vec>,QList<Vec>);

   void mopClip(Vec, Vec);
   void mopCrop(int);
   void reorientCameraUsingClipPlane(int);
   void saveSliceImage(int, int);
   void saveVolume();
   void maskRawVolume();
   void countIsolatedRegions();
   void getSurfaceArea();
   void getSurfaceArea(unsigned char);

   void viewProfile(int, int, QList<Vec>);
   void viewThicknessProfile(int, int, QList< QPair<Vec, Vec> >);

   void extractClip(int, int, int);
   void extractPath(int, bool, int, int);

   void addRotationAnimation(int, float, int);

   void gridStickToSurface(int, int, QList< QPair<Vec, Vec> >);

   void applyTFUndo(bool);
   void transferFunctionUpdated();

   void loadPlugin();

 private :
   Ui::MainWindow ui;

   Volume *m_Volume;

   Viewer *m_Viewer;

   DrawHiresVolume *m_Hires;
   DrawLowresVolume *m_Lowres;

   Bricks *m_bricks;
   BricksWidget *m_bricksWidget;

   KeyFrame *m_keyFrame;
   KeyFrameEditor *m_keyFrameEditor;
   QDockWidget *m_dockKeyframe;
   QDockWidget *m_dockTF;

   ViewsEditor *m_gallery;
   QDockWidget *m_dockGallery;

   TransferFunctionContainer *m_tfContainer;
   TransferFunctionManager *m_tfManager;
   TransferFunctionEditorWidget *m_tfEditor;
   LightingWidget *m_lightingWidget;
   VolumeInformationWidget *m_volInfoWidget;
   PreferencesWidget *m_preferencesWidget;

   QList <QAction*> m_recentFileActions;

   QList<QString> m_volFiles1, m_volFiles2;
   QList<QString> m_volFiles3, m_volFiles4;

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


   QList<QStringList> m_pluginList;
   QStringList m_pluginDll;;

   void initializeRecentFiles();

   void createHiresLowresWindows();

   void initTagColors();

   void loadDummyVolume(int, int, int);

   void loadVolumeFromUrls(QList<QUrl>);
   void loadVolumeRGBFromUrls(QList<QUrl>);

   void loadViewsAndKeyFrames(const char*);
   void saveViewsAndKeyFrames(const char*);

   void loadVolumeList(QList<QString>, bool);
   void loadVolume(QList<QString>);
   void loadVolumeRGB(char*);

   void loadVolume2List(QList<QString>, QList<QString>, bool);
   bool loadVolume2(QList<QString>, QList<QString>);

   void loadVolume3List(QList<QString>, QList<QString>,
			QList<QString>, bool);
   bool loadVolume3(QList<QString>, QList<QString>,
		    QList<QString>);

   void loadVolume4List(QList<QString>, QList<QString>,
			QList<QString>, QList<QString>, bool);
   bool loadVolume4(QList<QString>, QList<QString>,
		    QList<QString>, QList<QString>);

   void saveVolumeIntoProject(const char*);
   int loadVolumeFromProject(const char*);

   void setTextureMemory();
   void loadSettings();
   void saveSettings();

   void preLoadVolume();
   void postLoadVolume();

   void updateRecentFileAction();

   void loadProjectRunKeyframesAndExit();
   bool fromStringList(QStringList, BatchJob&);
   bool fromFile(QString, BatchJob&);

   void loadSingleVolume(QStringList);

   bool haveGrid();

   void registerPlugins();
};


#endif
