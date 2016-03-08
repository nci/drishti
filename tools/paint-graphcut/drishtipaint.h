#ifndef DRISHTIPAINT_H
#define DRISHTIPAINT_H

#include "ui_drishtipaint.h"
#include "ui_viewermenu.h"
#include "ui_graphcutmenu.h"
#include "ui_curvesmenu.h"
#include "ui_fibersmenu.h"

#include "tagcoloreditor.h"
#include "transferfunctionmanager.h"
#include "transferfunctioneditorwidget.h"
#include "imagewidget.h"
#include "myslider.h"
#include "volume.h"
#include "viewer.h"
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
  void keyPressEvent(QKeyEvent*);

 private slots :
  void openRecentFile();   
  void on_actionAltLayout_triggered();
  void on_actionHelp_triggered();
  void on_saveWork_triggered();
  void saveWork();
  void on_actionLoad_TF_triggered();
  void on_actionSave_TF_triggered();
  void on_saveImage_triggered();
  void on_actionLoad_triggered();
  void on_actionLoad_Curves_triggered();
  void on_actionSave_Curves_triggered();
  void on_actionLoad_Fibers_triggered();
  void on_actionSave_Fibers_triggered();
  void on_actionExit_triggered();
  void on_actionExtractTag_triggered();
  void on_actionMeshTag_triggered();
  void on_actionCurves_clicked(bool);
  void on_actionFibers_clicked(bool);
  void on_actionGraphCut_clicked(bool);
  void on_sliceLod_currentIndexChanged(int);
  void on_butZ_clicked();
  void on_butY_clicked();
  void on_butX_clicked();
  void on_help_clicked();
  void on_tagcurves_editingFinished();
  void curvetag_editingFinished();
  void fibertag_editingFinished();
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
  void getMaskSlice(int);
  void getRawValue(int, int, int);
  void tagDSlice(int, QImage);
  void tagWSlice(int, QImage);
  void tagHSlice(int, QImage);
  void tagDSlice(int, uchar*);
  void tagWSlice(int, uchar*);
  void tagHSlice(int, uchar*);
  void fillVolume(int, int,
		  int, int,
		  int, int,
		  QList<int>,
		  bool);
  void tagAllVisible(int, int,
		     int, int,
		     int, int);
  void dilate();
  void dilate(int, int,
	      int, int,
	      int, int);
  void erode();
  void erode(int, int,
	     int, int,
	     int, int);

  void applyMaskOperation(int, int, int);

  void paint3D(int, int, int, int, int);
  void paintLayer(int, int, int, int, int);
  void paint3D(int, int, int, Vec, Vec, int);
  void paint3DEnd();
  void dilateConnected(int, int, int, Vec, Vec, int);
  void erodeConnected(int, int, int, Vec, Vec, int);
  void tagUsingSketchPad(Vec, Vec);
  void mergeTags(Vec, Vec, int, int, bool);
  void mergeTags(Vec, Vec, int, int, int, bool);
  void setVisible(Vec, Vec, int, bool);

  void updateSliceBounds(Vec, Vec);

  void on_pointRender_clicked(bool);
  void on_raycastRender_clicked(bool);

  void on_stillStep_changed(double);
  void on_dragStep_changed(double);
  void lightOnOff(int);

  void getShadowColor();
  void getEdgeColor();
  void getBGColor();

 private :
  Ui::DrishtiPaint ui;
  Ui::ViewerMenu viewerUi;
  Ui::GraphCutMenu graphcutUi;
  Ui::CurvesMenu curvesUi;
  Ui::FibersMenu fibersUi;
  QFrame *m_curvesMenu;
  QFrame *m_graphcutMenu;
  QFrame *m_fibersMenu;

  TagColorEditor *m_tagColorEditor;

  TransferFunctionContainer *m_tfContainer;
  TransferFunctionManager *m_tfManager;
  TransferFunctionEditorWidget *m_tfEditor;

  ImageWidget *m_imageWidget;
  MySlider *m_slider;
  Volume *m_volume;
  BitmapThread *m_bitmapThread;

  int m_currSlice;

  QString m_pvlFile;
  QString m_xmlFile;

  QList <QAction*> m_recentFileActions;

  QList< QList<int> > m_blockList;

  Viewer *m_viewer;
  PopUpSlider *m_viewDslice;
  PopUpSlider *m_viewWslice;
  PopUpSlider *m_viewHslice;
  PopUpSlider *m_viewSpec;
  PopUpSlider *m_viewEdge;
  PopUpSlider *m_viewShadow;
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

  void savePvlHeader(QString, QString, int, int, int, bool);

  void processAndSaveMesh(int,
			  QString,
			  MarchingCubes*,
			  uchar*,
			  int, int, int,
			  int, int, int,
			  int, Vec, bool);

  void colorMesh(QList<Vec>&, QList<Vec>,
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
  void connectCurvesMenu();
  void connectFibersMenu();
  void connectImageWidget();
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
};

#endif
