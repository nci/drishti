#ifndef DRISHTIPAINT_H
#define DRISHTIPAINT_H

#include <QScrollArea>
#include <QSplitter>
#include <QTextEdit>
#include <QProcess>
#include <QUdpSocket>

#include "ui_drishtipaint.h"
#include "ui_viewermenu.h"
#include "ui_graphcutmenu.h"
#include "ui_curvesmenu.h"

#include "tagcoloreditor.h"
#include "transferfunctionmanager.h"
#include "transferfunctioneditorwidget.h"
#include "slices.h"
#include "curves.h"
#include "volume.h"
#include "viewer3d.h"
#include "popupslider.h"
#include "pywidget.h"

#include "handleexternal.h"

class DrishtiPaint : public QMainWindow
{
  Q_OBJECT

 public :
  DrishtiPaint(QWidget *parent=0);

 protected :
  void dragEnterEvent(QDragEnterEvent*);
  void dropEvent(QDropEvent*);
  void closeEvent(QCloseEvent*);

 private slots :
  void saveTagNames();
  void loadTagNames();
  void tagsUsed(QList<int>);
  void showVolumeInformation();
  void openRecentFile();   

  void on_actionScriptFolder_triggered();
  void on_actionCommand_triggered();

  void on_actionMesh_Viewer_triggered();
  void on_actionHelp3D_triggered();
  void on_action3DBoxSize_triggered();
  void on_action3DBoxList_triggered();
  void on_actionDrawBoxes3D_triggered();
  void on_actionSave3DList_triggered();
  void on_actionLoad3DList_triggered();
  void on_actionClear3DList_triggered();

  void on_actionHelp2D_triggered();
  void on_action2DBoxSize_triggered();
  void on_action2DBoxList_triggered();
  void on_actionDrawBoxes2D_triggered();
  void on_actionSave2DList_triggered();
  void on_actionLoad2DList_triggered();
  void on_actionClear2DList_triggered();

  void on_actionPort_triggered();

  void on_actionAbout_triggered();
  void on_actionHelp_triggered();
  void on_saveWork_triggered();
  void on_actionLoadMask_triggered();
  void checkFileSave();
  void saveWork();
  void on_actionExportMask_triggered();
  void on_actionImportMask_triggered();
  void on_actionCheckpoint_triggered();
  void on_actionLoadCheckpoint_triggered();
  void on_actionDeleteCheckpoint_triggered();
  void on_actionLoad_TF_triggered();
  void on_actionSave_TF_triggered();
  void on_saveImage_triggered();
  void on_saveImageSequence_triggered();
  void on_actionLoad_triggered();
  void bakeCurves_clicked();
  void on_actionExit_triggered();
  void on_actionExtractTag_triggered();
  void on_actionMeshTag_triggered();
  void on_actionCurves_triggered();
  void on_actionGraphCut_triggered();
  void sliceLod_currentIndexChanged(int);
  void on_help_clicked();

  void minGrad_valueChanged(int);
  void maxGrad_valueChanged(int);
  void gradType_Changed(int);
    
  void on_actionDefaultView_triggered();
  void on_actionZ_triggered();
  void on_actionY_triggered();
  void on_actionX_triggered();
  void on_action3dView_triggered();

  void defaultCurvesLayout_triggered();
  void axialCurvesLayout_triggered();
  void sagitalCurvesLayout_triggered();
  void coronalCurvesLayout_triggered();

  void on_saveFreq_valueChanged(int);
  void boxSize_valueChanged(int);
  void lambda_valueChanged(int);
  void preverode_valueChanged(int);
  void smooth_valueChanged(int);
  void copyprev_clicked(bool);
  void on_radius_valueChanged(int);
  void resetLivewire();
  void livewireSetting_toggled(bool);
  void livewire_clicked(bool);
  void lwsmooth_currentIndexChanged(int);
  void lwgrad_currentIndexChanged(int);
  void newcurve_clicked();
  void endcurve_clicked();
  void morphcurves_clicked();
  void deleteallcurves_clicked();
  void changeTransferFunctionDisplay(int, QList<bool>);
  void checkStateChanged(int, int, bool);
  void updateComposite();
  void tagSelected(int, bool);

  void getAxialSlice(int);
  void getSagitalSlice(int);
  void getCoronalSlice(int);

  void getRawValue(int, int, int);
  void tagDSlice(int, uchar*);
  void tagWSlice(int, uchar*);
  void tagHSlice(int, uchar*);
  void changeImageSlice(int, int, int);


  void setShowPosition(bool);

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
  void stepTags(Vec, Vec, int, int);

  void updateSliceBounds(Vec, Vec);

  void stillStep_changed(double);

  void getShadowColor();
  void getEdgeColor();
  void getBGColor();

  void setVisible(Vec, Vec, int, bool);
  void resetTag(Vec, Vec, int);
  void reloadMask();
  void loadRawMask(QString);
  void modifyOriginalVolume(Vec, Vec, int);

  void shrinkwrap(Vec, Vec, int, bool, int);
  void shrinkwrap(Vec, Vec, int, bool, int,
		  bool, int, int, int, int);

  void tagTubes(Vec, Vec, int);
  void tagTubes(Vec, Vec, int,
		  bool, int, int, int, int);

  void connectedRegion(int, int, int,
		       Vec, Vec,
		       int, int);
  void hatchConnectedRegion(int, int, int,
			    Vec, Vec,
			    int, int,
			    int, int);

  void on_changeSliceOrdering_triggered();

  void undoPaint3D();

  void createMeshViewerSocket();

  
 private :
  Ui::DrishtiPaint ui;
  Ui::ViewerMenu viewerUi;
  Ui::GraphCutMenu graphcutUi;
  Ui::CurvesMenu curvesUi;
  QFrame *m_curvesMenu;
  QFrame *m_graphcutMenu;

  
  HandleExternalCMD *m_handleExternalCMD;


  QUdpSocket *m_meshViewerSocket;
  QProcess m_meshViewer;
  
  
  TagColorEditor *m_tagColorEditor;

  TransferFunctionContainer *m_tfContainer;
  TransferFunctionManager *m_tfManager;
  TransferFunctionEditorWidget *m_tfEditor;

  QSplitter *m_graphCutArea;
  QSplitter *m_splitterOne;
  QSplitter *m_splitterTwo;
  QFrame* m_axialFrame;
  QFrame* m_sagitalFrame;
  QFrame* m_coronalFrame;
  Slices *m_axialImage;
  Slices *m_sagitalImage;
  Slices *m_coronalImage;

  //Curves *m_curves;
  QSplitter *m_curvesArea;
  QSplitter *m_splitterOneC;
  QSplitter *m_splitterTwoC;
  QFrame* m_axialFrameC;
  QFrame* m_sagitalFrameC;
  QFrame* m_coronalFrameC;
  Curves *m_axialCurves;
  Curves *m_sagitalCurves;
  Curves *m_coronalCurves;
  
  Volume *m_volume;

  QString m_pvlFile;
  QString m_xmlFile;

  QList <QAction*> m_recentFileActions;

  QList< QList<int> > m_blockList;
  
  Vec m_prevSeed;

  Viewer3D *m_viewer3D;
  Viewer *m_viewer;
  PopUpSlider *m_viewEdge;
  PopUpSlider *m_viewShadow;

  PopUpSlider *m_minGrad, *m_maxGrad;
  QComboBox *m_gradType;

  PyWidget *m_pyWidget;

  int m_CMDport;
  
  void setFile(QString);
  void initTagColors();

  void loadSettings();
  void saveSettings();

  void updateRecentFileAction();
  QString loadVolumeFromProject(const char*);

  void reloadSlices();

  void sliceDilate(int, int, uchar*, uchar*, int, int);
  void dilate(int, int, uchar**, uchar*, int, int);

  void sliceErode(int, int, uchar*, uchar*, int, int);
  void erode(int, int, uchar**, uchar*, int, int);

  void sliceSmooth(int, int, uchar*, uchar*, int, int, int);
  void smooth(int, int, uchar**, uchar*, int, int, int);

  void savePvlHeader(QString, QString, int, int, int, bool, int);

  void colorMesh(QVector<QVector3D>&,
		 QVector<QVector3D>,
		 QVector<QVector3D>,
		 int, uchar*,
		 int, int, int,
		 int, int, int, int,
		 int);


  QPair<QString, QList<int> > getTags(QString);

  void connectViewerMenu();
  void connectGraphCutMenu();
  void connectCurvesMenu();
  void connectImageWidget();
  void connectCurvesWidget();
  void miscConnections();

  void updateCurveMask(uchar*,
		       int, int, int,
		       int, int, int,
		       int, int, int,
		       int, int, int);


  void dilateAndSmooth(uchar*, int, int, int, int);
  void smoothData(uchar*, int, int, int, int);

  void processHoles(uchar*, int, int, int, int);

  bool tagUsingSketchPad(Vec, Vec, int);

  void setupLightParameters();

  void updateModifiedRegion(int, int, int,
			    int, int, int);

  bool sliceZeroAtTop();

  QSplitter* createImageWindows();
  QSplitter* createCurveWindows();

  void extractFromAnotherVolume(QList<int>);

  void loadCheckPoint(QString);

  bool getValues(float&, float&, float&, int&, int&, int&, int&, bool&);
};

#endif
