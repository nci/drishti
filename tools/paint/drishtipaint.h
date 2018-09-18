#ifndef DRISHTIPAINT_H
#define DRISHTIPAINT_H

#include <QScrollArea>
#include <QSplitter>

#include "ui_drishtipaint.h"
#include "ui_viewermenu.h"
#include "ui_superpixelmenu.h"
#include "ui_graphcutmenu.h"
#include "ui_curvesmenu.h"
#include "ui_fibersmenu.h"

#include "tagcoloreditor.h"
#include "transferfunctionmanager.h"
#include "transferfunctioneditorwidget.h"
#include "slices.h"
#include "curveswidget.h"
#include "myslider.h"
#include "volume.h"
#include "viewer3d.h"
#include "marchingcubes.h"
#include "ply.h"
#include "popupslider.h"


class DrishtiPaint : public QMainWindow
{
  Q_OBJECT

 public :
  DrishtiPaint(QWidget *parent=0);

 protected :
  void dragEnterEvent(QDragEnterEvent*);
  void dropEvent(QDropEvent*);
  void closeEvent(QCloseEvent*);
  //void keyPressEvent(QKeyEvent*);

 private slots :
  void showVolumeInformation();
  void openRecentFile();   
  void on_actionAbout_triggered();
  void on_actionHelp_triggered();
  void on_saveWork_triggered();
  void on_loadMask_triggered();
  void saveWork();
  void on_actionExportMask_triggered();
  void on_actionCheckpoint_triggered();
  void on_actionLoadCheckpoint_triggered();
  void on_actionLoad_TF_triggered();
  void on_actionSave_TF_triggered();
  void on_saveImage_triggered();
  void on_actionLoad_triggered();
  void on_actionLoad_Curves_triggered();
  void on_actionSave_Curves_triggered();
  void on_actionLoad_Fibers_triggered();
  void on_actionSave_Fibers_triggered();
  void on_actionBakeCurves_triggered();
  void on_actionExit_triggered();
  void on_actionExtractTag_triggered();
  void on_actionMeshTag_triggered();
  void on_actionCurves_triggered();
  void on_actionGraphCut_triggered();
  void on_actionSuperpixels_triggered();
  void on_sliceLod_currentIndexChanged(int);
  void on_butZ_clicked();
  void on_butY_clicked();
  void on_butX_clicked();
  void on_help_clicked();

  void on_actionDefaultView_triggered();
  void on_actionZ_triggered();
  void on_actionY_triggered();
  void on_actionX_triggered();
  void on_action3dView_triggered();

  void on_tagcurves_editingFinished();
  void curvetag_editingFinished();
  void fibertag_editingFinished();
  void on_saveFreq_valueChanged(int);
  void on_tag_valueChanged(int);
  void on_boxSize_valueChanged(int);
  void on_lambda_valueChanged(int);
  void on_preverode_valueChanged(int);
  void on_smooth_valueChanged(int);
  void on_copyprev_clicked(bool);
  void on_thickness_valueChanged(int);
  void on_radius_valueChanged(int);
  void on_pointsize_valueChanged(int);
  void on_mincurvelen_valueChanged(int);
  void on_livewire_clicked(bool);
  void on_modify_clicked(bool);
  void on_closed_clicked(bool);
  void on_lwsmooth_currentIndexChanged(int);
  void on_lwgrad_currentIndexChanged(int);
  void on_newcurve_clicked();
  void on_endcurve_clicked();
  void on_newfiber_clicked();
  void on_endfiber_clicked();
  void on_morphcurves_clicked();
  void on_propagate_clicked(bool);
  void on_deselect_clicked();
  void on_deleteallcurves_clicked();
  void on_zoom0_clicked();
  void on_zoom9_clicked();
  void on_zoomup_clicked();
  void on_zoomdown_clicked();
  void changeTransferFunctionDisplay(int, QList<bool>);
  void checkStateChanged(int, int, bool);
  void updateComposite();
  void tagSelected(int);
  void getSlice(int);
  void getSliceC(int);
  void getMaskSlice(int);
  void getRawValue(int, int, int);
  void tagDSlice(int, uchar*);
  void tagWSlice(int, uchar*);
  void tagHSlice(int, uchar*);
  void changeImageSlice(int, int, int);

  void on_autoGenSupPix_clicked(bool);
  void on_hideSupPix_clicked(bool);
  void on_supPixSize();

  void setShowSlices(bool);

  void applyMaskOperation(int, int, int);

  void paint3DStart();
  void paint3D(Vec, Vec,
	       int, int, int,
	       int, int, bool);
  void paint3DEnd();

  void dilateConnected(int, int, int, Vec, Vec, int, bool);
  void erodeConnected(int, int, int, Vec, Vec, int);
  void tagUsingSketchPad(Vec, Vec);
  void mergeTags(Vec, Vec, int, int, bool);

  void updateSliceBounds(Vec, Vec);

  void on_pointRender_clicked(bool);
  void on_raycastRender_clicked(bool);

  void on_stillStep_changed(double);
  void on_dragStep_changed(double);

  void getShadowColor();
  void getEdgeColor();
  void getBGColor();

  void setVisible(Vec, Vec, int, bool);
  void resetTag(Vec, Vec, int);
  void reloadMask();
  void modifyOriginalVolume(Vec, Vec, int);
  void shrinkwrap(Vec, Vec, int, bool, int);
  void shrinkwrap(Vec, Vec, int, bool, int,
		  bool, int, int, int, int);
  void connectedRegion(int, int, int,
		       Vec, Vec,
		       int, int);
  void hatchConnectedRegion(int, int, int,
			    Vec, Vec,
			    int, int,
			    int, int);

  void on_changeSliceOrdering_triggered();

 private :
  Ui::DrishtiPaint ui;
  Ui::ViewerMenu viewerUi;
  Ui::GraphCutMenu graphcutUi;
  Ui::SuperPixelMenu superpixelUi;
  Ui::CurvesMenu curvesUi;
  Ui::FibersMenu fibersUi;
  QFrame *m_curvesMenu;
  QFrame *m_graphcutMenu;
  QFrame *m_superpixelMenu;
  QFrame *m_fibersMenu;

  TagColorEditor *m_tagColorEditor;

  TransferFunctionContainer *m_tfContainer;
  TransferFunctionManager *m_tfManager;
  TransferFunctionEditorWidget *m_tfEditor;

  QScrollArea *m_scrollAreaC;
  QSplitter *m_graphCutArea;
  QSplitter *m_splitterOne;
  QSplitter *m_splitterTwo;

  QFrame* m_axialFrame;
  QFrame* m_sagitalFrame;
  QFrame* m_coronalFrame;

  Slices *m_axialImage;
  Slices *m_sagitalImage;
  Slices *m_coronalImage;
  CurvesWidget *m_curvesWidget;
  MySlider *m_slider;
  Volume *m_volume;

  int m_currSlice;

  QString m_pvlFile;
  QString m_xmlFile;

  QList <QAction*> m_recentFileActions;

  QList< QList<int> > m_blockList;

  Vec m_prevSeed;

  Viewer3D *m_viewer3D;
  Viewer *m_viewer;
  PopUpSlider *m_viewDslice;
  PopUpSlider *m_viewWslice;
  PopUpSlider *m_viewHslice;
  PopUpSlider *m_viewSpec;
  PopUpSlider *m_viewEdge;
  PopUpSlider *m_viewShadow;
  PopUpSlider *m_shadowX;
  PopUpSlider *m_shadowY;
  QPushButton *m_shadowButton;
  QPushButton *m_edgeButton;
  QPushButton *m_bgButton;

  void setFile(QString);
  void initTagColors();

  void loadSettings();
  void saveSettings();

  void updateRecentFileAction();
  QString loadVolumeFromProject(const char*);

  void sliceDilate(int, int, uchar*, uchar*, int, int);
  void dilate(int, int, uchar**, uchar*, int, int);

  void sliceErode(int, int, uchar*, uchar*, int, int);
  void erode(int, int, uchar**, uchar*, int, int);

  void sliceSmooth(int, int, uchar*, uchar*, int, int, int);
  void smooth(int, int, uchar**, uchar*, int, int, int);

  void savePvlHeader(QString, QString, int, int, int, bool, int);

  void processAndSaveMesh(int,
			  QString,
			  MarchingCubes*,
			  uchar*,
			  int, int, int,
			  int, int, int,
			  int, Vec, bool);

  void colorMesh(QList<Vec>&,
		 QList<Vec>,
		 QList<Vec>,
		 int, uchar*,
		 int, int, int,
		 int, int, int, int);

  void smoothMesh(QList<Vec>&,
		  QList<Vec>&,
		  QList<Vec>&,
		  int);

  void saveMesh(QList<Vec>, QList<Vec>,
		QList<Vec>, QList<Vec>,
		QString, bool);

  QPair<QString, QList<int> > getTags(QString);

  void connectViewerMenu();
  void connectGraphCutMenu();
  void connectSuperPixelMenu();
  void connectCurvesMenu();
  void connectFibersMenu();
  void connectImageWidget();
  void connectCurvesWidget();
  void miscConnections();

  void updateCurveMask(uchar*, QList<int>,
		       int, int, int,
		       int, int, int,
		       int, int, int,
		       int, int, int);

  void updateFiberMask(uchar*, QList<int>,
		       int, int, int,
		       int, int, int,
		       int, int, int);

  void meshFibers(QString);

  void dilateAndSmooth(uchar*, int, int, int, int);
  void smoothData(uchar*, int, int, int, int);

  void processHoles(uchar*, int, int, int, int);

  bool tagUsingSketchPad(Vec, Vec, int);

  void setupSlicesParameters();
  void setupLightParameters();

  void updateModifiedRegion(int, int, int,
			    int, int, int);

  bool sliceZeroAtTop();

  QSplitter* createImageWindows();

  void extractFromAnotherVolume(QList<int>);

  void loadCheckPoint(QString);
};

#endif
