#include "vdbvolume.h"

#include <GL/glew.h>

#include "drishtipaint.h"
#include "global.h"
#include "staticfunctions.h"
#include "showhelp.h"
#include "dcolordialog.h"
#include "morphslice.h"
#include "propertyeditor.h"
#include "meshtools.h"

#include <QDockWidget>
#include <QFileDialog>
#include <QInputDialog>
#include <QScrollArea>
 
#include <exception>
#include "volumeinformation.h"
#include "volumeoperations.h"
#include "volumemeasure.h"

#include "blosc.h"

#include <exception>


//#pragma push_macro("slots")
//#undef slots
//#include "Python.h"
//#pragma pop_macro("slots")

void
DrishtiPaint::initTagColors()
{
  uchar *colors = Global::tagColors();
  memset(colors, 255, 65536*4);

  qsrand(1);

  for(int i=1; i<65536; i++)
    {
      float r,g,b,a;
      r = (float)qrand()/(float)RAND_MAX;
      g = (float)qrand()/(float)RAND_MAX;
      b = (float)qrand()/(float)RAND_MAX;
      a = 1.0f;

      float mm = qMax(r,qMax(g,b));
      if (mm < 0.8) // don't want too dark
	{
	  if (mm < 0.1)
	    {
	      r = g = b = 1.0;
	    }
	  else if (mm < 0.3)
	    {
	      r = 1 - r;
	      g = 1 - g;
	      b = 1 - b;
	    }
	  else
	    {
	      r *= 0.8/mm;
	      g *= 0.8/mm;
	      b *= 0.8/mm;
	    }
	}
      
      if (i > 65534)
	{
	  r = 0.9f; g = 0.3f; b = 0.2f; a = 1.0f;
	}
      
      colors[4*i+0] = 255*r;
      colors[4*i+1] = 255*g;
      colors[4*i+2] = 255*b;
      colors[4*i+3] = 255*a;
    }
}

void
DrishtiPaint::on_actionMesh_Viewer_triggered()
{
  if (m_meshViewer.state() != QProcess::Running)
    {
      QDir app = QCoreApplication::applicationDirPath();
      m_meshViewer.start(app.absolutePath()+"/drishtimesh.exe");
    }
  QTimer::singleShot(1000, this, SLOT(createMeshViewerSocket()));
}
      
void
DrishtiPaint::createMeshViewerSocket()
{
  m_meshViewerSocket = new QUdpSocket(this);
}


void
DrishtiPaint::changeImageSlice(int d, int w, int h)
{
  m_axialImage->setSlice(d);
  m_sagitalImage->setSlice(w);
  m_coronalImage->setSlice(h);
}

DrishtiPaint::DrishtiPaint(QWidget *parent) :
  QMainWindow(parent)
{
  Global::setMainWindow(this);
    
  ui.setupUi(this);
  
  qApp->setFont(QFont("MS Reference Sans Serif", 12));
  
  ui.statusbar->setEnabled(true);
  ui.statusbar->setSizeGripEnabled(true);
  //ui.statusbar->hide();

  m_meshViewerSocket = 0;
  m_CMDport = 7760;
  
//  QFont font;
//  font.setPointSize(10);
//  setFont(font);

  setWindowIcon(QPixmap(":/images/drishti_paint_32.png"));
  setWindowTitle(QString("DrishtiPaint v") + QString(DRISHTI_VERSION));

  ui.menuFile->addSeparator();

  for (int i=0; i<Global::maxRecentFiles(); i++)
    {
      m_recentFileActions.append(new QAction(this));
      m_recentFileActions[i]->setVisible(false);
      connect(m_recentFileActions[i], SIGNAL(triggered()),
	      this, SLOT(openRecentFile()));
      ui.menuFile->addAction(m_recentFileActions[i]);
    }

  { // add showVolumeInformation to menuCommands
    QAction *action = new QAction(this);
    action->setText("Volume Information");
    action->setVisible(true);
    connect(action, SIGNAL(triggered()),
	    this, SLOT(showVolumeInformation()));
    ui.menuCommands->addAction(action);
  }


  setAcceptDrops(true);

  initTagColors();

  StaticFunctions::initQColorDialog();

  m_pvlFile.clear();
  m_xmlFile.clear();

  m_tfContainer = new TransferFunctionContainer();
  m_tfManager = new TransferFunctionManager();
  m_tfManager->registerContainer(m_tfContainer);
  m_tfEditor = new TransferFunctionEditorWidget();
  m_tfEditor->setTransferFunction(NULL);    
  m_tfManager->setDisabled(true);

  m_graphcutMenu = new QFrame();
  graphcutUi.setupUi(m_graphcutMenu);

  m_curvesMenu = new QFrame();
  curvesUi.setupUi(m_curvesMenu);


  ui.help->setMaximumSize(50, 50);
  ui.help->setMinimumSize(50, 50);

  
  ui.sideframelayout->addWidget(m_graphcutMenu);
  ui.sideframelayout->addWidget(m_curvesMenu);


  m_tagColorEditor = new TagColorEditor();

  connect(m_tagColorEditor, SIGNAL(tagNamesChanged()),
	  this, SLOT(saveTagNames()));

  m_viewer3D = new Viewer3D(this);
  m_viewer = m_viewer3D->viewer();

  connect(m_viewer3D, SIGNAL(changeLayout()),
	  this, SLOT(on_action3dView_triggered()));

  connect(m_viewer, SIGNAL(tagsUsed(QList<int>)),
	  this, SLOT(tagsUsed(QList<int>)));
  //------------------------------
  // viewer menu
  QFrame *viewerMenu = new QFrame();
  viewerMenu->setFrameShape(QFrame::Box);
  viewerUi.setupUi(viewerMenu);
  connectViewerMenu();
  //----------------

  viewerUi.raycastParam->setVisible(true);
  //----------------

  //------------------------------

  //------------------------------
  #include "connectviewer.h"
  //------------------------------


  //----------------------------------------------------------
  QDockWidget *dock1 = new QDockWidget("Transfer Functions", this);
  {
    m_minGrad = new PopUpSlider(m_viewer, Qt::Horizontal);
    m_maxGrad = new PopUpSlider(m_viewer, Qt::Horizontal);
    m_minGrad->setText("Min Grad");
    m_maxGrad->setText("Max Grad");
    m_minGrad->setRange(0, 100, 200);
    m_maxGrad->setRange(0, 100, 200);
    m_minGrad->setValue(0);
    m_maxGrad->setValue(100);
    m_gradType = new QComboBox(m_viewer);
    QStringList slist;
    slist << "Type 1";
    slist << "Type 2";
    slist << "Type 3";
    m_gradType->addItems(slist);
    m_gradType->setCurrentIndex(0);
    QHBoxLayout *glayout = new QHBoxLayout();
    glayout->addWidget(m_minGrad);
    glayout->addWidget(m_maxGrad);
    glayout->addWidget(m_gradType);
    connect(m_minGrad, SIGNAL(valueChanged(int)),
	    this, SLOT(minGrad_valueChanged(int)));
    connect(m_maxGrad, SIGNAL(valueChanged(int)),
	    this, SLOT(maxGrad_valueChanged(int)));
    connect(m_gradType, SIGNAL(currentIndexChanged(int)),
	    this, SLOT(gradType_Changed(int)));
    QFrame *gframe = new QFrame();
    gframe->setFrameShape(QFrame::Box);
    gframe->setLayout(glayout);

    
    dock1->setAllowedAreas(Qt::LeftDockWidgetArea | 
			   Qt::RightDockWidgetArea);
    QSplitter *splitter = new QSplitter(Qt::Vertical, dock1);
    splitter->addWidget(m_tagColorEditor);
    splitter->addWidget(m_tfManager);
    splitter->addWidget(gframe);
    splitter->addWidget(m_tfEditor);
    QList<int> ssz;
    ssz << 150 << 75 << 1 << 150;
    splitter->setSizes(ssz);
    QFrame *dframe = new QFrame();
    dframe->setFrameShape(QFrame::Box);
    QVBoxLayout *layout = new QVBoxLayout();
    dframe->setLayout(layout);
    dframe->layout()->addWidget(splitter);
    dock1->setWidget(dframe);
  }
  //----------------------------------------------------------

  //----------------------------------------------------------
  QDockWidget *dock2 = new QDockWidget("Seg Param", this);
  {
    dock2->setAllowedAreas(Qt::LeftDockWidgetArea | 
			   Qt::RightDockWidgetArea);

    QFrame *dframe = new QFrame();
    QHBoxLayout *layout = new QHBoxLayout();
    dframe->setLayout(layout);
    dframe->layout()->addWidget(ui.sideframe);

    QScrollArea *scrollArea = new QScrollArea;
    scrollArea->setWidget(dframe);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    dock2->setWidget(scrollArea);
    dock2->setMaximumWidth(210);
  }
  //----------------------------------------------------------

  //----------------------------------------------------------
    QDockWidget *dock3 = new QDockWidget("3D Preview", this);
  {
    dock3->setAllowedAreas(Qt::LeftDockWidgetArea | 
			   Qt::RightDockWidgetArea);

    QFrame *dframe = new QFrame();
    QHBoxLayout *layout = new QHBoxLayout();
    dframe->setLayout(layout);
    dframe->layout()->addWidget(viewerMenu);

    QScrollArea *scrollArea = new QScrollArea;
    scrollArea->setWidget(dframe);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    dock3->setWidget(scrollArea);
    dock3->setMaximumWidth(210);
  }
  //----------------------------------------------------------
  //----------------------------------------------------------

  addDockWidget(Qt::LeftDockWidgetArea, dock2);
  addDockWidget(Qt::LeftDockWidgetArea, dock3, Qt::Vertical);
  addDockWidget(Qt::RightDockWidgetArea, dock1, Qt::Horizontal);


  m_volume = new Volume();

  //m_curves = new Curves(this, ui.statusbar);

  m_graphCutArea = createImageWindows();
  m_curvesArea = createCurveWindows();

  
  QVBoxLayout *layout1 = new QVBoxLayout();
  layout1->setContentsMargins(0,0,0,0);
  ui.imageFrame->setLayout(layout1);
  ui.imageFrame->layout()->addWidget(m_graphCutArea);

  ui.imageFrame->layout()->addWidget(m_curvesArea);


  ui.menuView->addAction(dock1->toggleViewAction());
  ui.menuView->addAction(dock2->toggleViewAction());
  ui.menuView->addAction(dock3->toggleViewAction());
  
  on_actionGraphCut_triggered();

  curvesUi.lwsettingpanel->setVisible(false);

  Global::setBoxSize(5);
  Global::setSpread(10);
  Global::setLambda(10);
  Global::setSmooth(1);
  Global::setPrevErode(5);

  curvesUi.endcurve->hide();
  ui.radius->setValue(Global::spread());
  graphcutUi.boxSize->setValue(Global::boxSize());
  graphcutUi.lambda->setValue(Global::lambda());
  graphcutUi.smooth->setValue(Global::smooth());
  graphcutUi.preverode->setValue(Global::prevErode());

  viewerUi.radiusSurface->setValue(Global::spread());
  viewerUi.radiusDepth->setValue(Global::spread());


  //------------------------
  miscConnections();
  connectImageWidget();
  connectCurvesWidget();
  connectCurvesMenu();
  connectGraphCutMenu();
  //------------------------

  QDir app = QCoreApplication::applicationDirPath();
  app.cd("assets");
  app.cd("scripts");
  app.cd("paint");
  Global::setScriptFolder(app.absolutePath());
			  
  loadSettings();

  m_axialImage->updateTagColors();
  m_sagitalImage->updateTagColors();
  m_coronalImage->updateTagColors();

  m_axialCurves->updateTagColors();
  m_sagitalCurves->updateTagColors();
  m_coronalCurves->updateTagColors();

  setGeometry(100, 100, 700, 700);

  m_blockList.clear();

  m_pyWidget = 0;

  resize(1600, 1024);

  
  m_handleExternalCMD = new HandleExternalCMD(m_CMDport);

  connect(m_handleExternalCMD, SIGNAL(loadRAW(QString)),
	  this, SLOT(loadRawMask(QString)));
}


void DrishtiPaint::on_actionHelp_triggered() { ShowHelp::showMainHelp(); }

void
DrishtiPaint::on_help_clicked()
{
  if (m_curvesMenu->isVisible())
    {
      ShowHelp::showCurvesHelp();
      return;
    }
  if (m_graphcutMenu->isVisible())
    {
      ShowHelp::showGraphCutHelp();
      return;
    }
}



//------------------
//------------------
QSplitter*
DrishtiPaint::createCurveWindows()
{
  m_axialFrameC = new QFrame();
  m_sagitalFrameC = new QFrame();
  m_coronalFrameC = new QFrame();

  m_axialFrameC->setFrameShape(QFrame::Box);
  m_sagitalFrameC->setFrameShape(QFrame::Box);
  m_coronalFrameC->setFrameShape(QFrame::Box);

  QSplitter *splitter_0;

  splitter_0 = new QSplitter();
  splitter_0->setOrientation(Qt::Horizontal);

  m_splitterOneC = new QSplitter();
  m_splitterTwoC = new QSplitter();

  m_splitterOneC->setOrientation(Qt::Vertical);
  m_splitterTwoC->setOrientation(Qt::Vertical);

  m_splitterOneC->addWidget(m_axialFrameC);
  //m_splitterOneC->addWidget(m_viewer3D);

  m_splitterTwoC->addWidget(m_sagitalFrameC);
  m_splitterTwoC->addWidget(m_coronalFrameC);

  splitter_0->addWidget(m_splitterOneC);
  splitter_0->addWidget(m_splitterTwoC);

  QList<int> ssz;
  ssz << 150;
  ssz << 150;
  splitter_0->setSizes(ssz);
  m_splitterOneC->setSizes(ssz);
  m_splitterTwoC->setSizes(ssz);

  m_axialCurves = new Curves(this, ui.statusbar);
  m_axialCurves->setSliceType(ImageWidget::DSlice);

  m_sagitalCurves = new Curves(this, ui.statusbar);
  m_sagitalCurves->setSliceType(ImageWidget::WSlice);

  m_coronalCurves = new Curves(this, ui.statusbar);
  m_coronalCurves->setSliceType(ImageWidget::HSlice);

  {
    QVBoxLayout *layout = new QVBoxLayout();
    m_axialFrameC->setLayout(layout);
    m_axialFrameC->layout()->addWidget(m_axialCurves);
  }
  {
    QVBoxLayout *layout = new QVBoxLayout();
    m_sagitalFrameC->setLayout(layout);
    m_sagitalFrameC->layout()->addWidget(m_sagitalCurves);
  }
  {
    QVBoxLayout *layout = new QVBoxLayout();
    m_coronalFrameC->setLayout(layout);
    m_coronalFrameC->layout()->addWidget(m_coronalCurves);
  }

  connect(m_axialCurves,  SIGNAL(changeLayout()), this, SLOT(axialCurvesLayout_triggered()));
  connect(m_sagitalCurves,SIGNAL(changeLayout()), this, SLOT(sagitalCurvesLayout_triggered()));
  connect(m_coronalCurves,SIGNAL(changeLayout()), this, SLOT(coronalCurvesLayout_triggered()));

  
  connect(m_axialCurves,  SIGNAL(getSlice(int)), this, SLOT(getAxialSlice(int)));  
  connect(m_axialCurves,  SIGNAL(getSlice(int)), m_viewer, SLOT(setDSlice(int)));
  connect(m_axialCurves, SIGNAL(getSlice(int)), m_sagitalCurves, SLOT(setVLine(int)));
  connect(m_axialCurves, SIGNAL(getSlice(int)), m_coronalCurves, SLOT(setVLine(int)));


  connect(m_sagitalCurves,SIGNAL(getSlice(int)), this, SLOT(getSagitalSlice(int)));  
  connect(m_sagitalCurves,SIGNAL(getSlice(int)), m_viewer, SLOT(setWSlice(int)));
  connect(m_sagitalCurves, SIGNAL(getSlice(int)), m_axialCurves, SLOT(setVLine(int)));
  connect(m_sagitalCurves, SIGNAL(getSlice(int)), m_coronalCurves, SLOT(setHLine(int)));

  
  connect(m_coronalCurves,SIGNAL(getSlice(int)), this, SLOT(getCoronalSlice(int)));  
  connect(m_coronalCurves,SIGNAL(getSlice(int)), m_viewer, SLOT(setHSlice(int)));
  connect(m_coronalCurves, SIGNAL(getSlice(int)), m_axialCurves, SLOT(setHLine(int)));
  connect(m_coronalCurves, SIGNAL(getSlice(int)), m_sagitalCurves, SLOT(setHLine(int)));

  
  connect(m_axialCurves, SIGNAL(xPos(int)), m_coronalCurves, SLOT(updateSliderValue(int)));
  connect(m_axialCurves, SIGNAL(yPos(int)), m_sagitalCurves, SLOT(updateSliderValue(int)));

  connect(m_sagitalCurves, SIGNAL(xPos(int)), m_coronalCurves, SLOT(updateSliderValue(int)));
  connect(m_sagitalCurves, SIGNAL(yPos(int)), m_axialCurves, SLOT(updateSliderValue(int)));

  connect(m_coronalCurves, SIGNAL(xPos(int)), m_sagitalCurves, SLOT(updateSliderValue(int)));
  connect(m_coronalCurves, SIGNAL(yPos(int)), m_axialCurves, SLOT(updateSliderValue(int)));


  return splitter_0;
}

void
DrishtiPaint::on_actionCurves_triggered()
{
  if (!m_volume->isValid())
    {
      ui.actionGraphCut->setChecked(true);
      ui.actionCurves->setChecked(false);
      return;
    }
  
  QString hss;
  hss += "QToolButton { border-width:5; border-style:solid; border-radius:25px;";
  hss += "color:#0077dd; }";
  ui.help->setStyleSheet(hss);

  m_graphCutArea->hide();
  m_curvesArea->show();
  
  m_curvesMenu->show();
  m_graphcutMenu->hide();
  ui.spreadframe->hide();

  ui.actionCurves->setChecked(true);
  ui.actionGraphCut->setChecked(false);

  m_axialCurves->setCurve(true);
  m_sagitalCurves->setCurve(true);
  m_coronalCurves->setCurve(true);


  if (m_volume->isValid())
    {
      curvesUi.livewire->setChecked(false);
      m_axialCurves->setLivewire(false);
      m_sagitalCurves->setLivewire(false);
      m_coronalCurves->setLivewire(false);
    }
  else
    {
      m_axialCurves->setGridSize(100,100,100);
      m_sagitalCurves->setGridSize(100,100,100);
      m_coronalCurves->setGridSize(100,100,100);
      m_axialCurves->resetSliceType();
      m_sagitalCurves->resetSliceType();
      m_coronalCurves->resetSliceType();
    }
  
  if (m_axialImage->enlarged())
      axialCurvesLayout_triggered();
  else if (m_sagitalImage->enlarged())
    sagitalCurvesLayout_triggered();
  else if (m_coronalImage->enlarged())
    m_coronalCurves->setLarge(false);
  else if (m_viewer3D->enlarged())
    on_action3dView_triggered();
  else      
    defaultCurvesLayout_triggered();

  {
    m_axialImage->setLarge(false);
    m_sagitalImage->setLarge(false);
    m_coronalImage->setLarge(false);
  }

}

void
DrishtiPaint::defaultCurvesLayout_triggered()
{
  m_viewer->stopDrawing();
    
  m_viewer3D->setLarge(false);
  m_axialCurves->setLarge(false);
  m_sagitalCurves->setLarge(false);
  m_coronalCurves->setLarge(false);

  m_splitterOneC->addWidget(m_axialFrameC); //Z
  m_splitterOneC->addWidget(m_viewer3D);

  m_splitterTwoC->addWidget(m_sagitalFrameC); // Y
  m_splitterTwoC->addWidget(m_coronalFrameC); // X


  QList<int> gcas;
  gcas << 1000 << 1000;
  m_curvesArea->setSizes(gcas);
  
  QList<int> ssz;
  ssz << 150;
  ssz << 150;
  m_splitterOneC->setSizes(ssz);
  m_splitterTwoC->setSizes(ssz);

  
  QTimer::singleShot(200, m_viewer, SLOT(startDrawing()));
  QTimer::singleShot(50, m_axialCurves, SLOT(fitImage()));
  QTimer::singleShot(50, m_sagitalCurves, SLOT(fitImage()));
  QTimer::singleShot(50, m_coronalCurves, SLOT(fitImage()));
}
void
DrishtiPaint::axialCurvesLayout_triggered()
{
  if (m_axialCurves->enlarged())
    {
      defaultCurvesLayout_triggered();
      return;
    }

  m_viewer->stopDrawing();
    
  m_axialCurves->setLarge(true);  
  m_sagitalCurves->setLarge(false);
  m_coronalCurves->setLarge(false);
  m_viewer3D->setLarge(false);

  m_splitterOneC->addWidget(m_axialFrameC);

  m_splitterTwoC->addWidget(m_sagitalFrameC);
  m_splitterTwoC->addWidget(m_coronalFrameC);
  m_splitterTwoC->addWidget(m_viewer3D);

  QList<int> ssz;
  ssz << 150;
  ssz << 150;
  ssz << 150;
  m_splitterTwoC->setSizes(ssz);

  QList<int> gcas;
  gcas << 1000 << 200;
  m_curvesArea->setSizes(gcas);

  //QTimer::singleShot(200, m_viewer, SLOT(startDrawing()));
  QTimer::singleShot(50, m_axialCurves, SLOT(fitImage()));
  QTimer::singleShot(50, m_sagitalCurves, SLOT(fitImage()));
  QTimer::singleShot(50, m_coronalCurves, SLOT(fitImage()));
}
void
DrishtiPaint::sagitalCurvesLayout_triggered()
{
  if (m_sagitalCurves->enlarged())
    {
      defaultCurvesLayout_triggered();
      return;
    }

  m_viewer->stopDrawing();
    
  m_axialCurves->setLarge(false);
  m_sagitalCurves->setLarge(true);
  m_coronalCurves->setLarge(false);
  m_viewer3D->setLarge(false);

  m_splitterOneC->addWidget(m_sagitalFrameC);

  m_splitterTwoC->addWidget(m_axialFrameC);
  m_splitterTwoC->addWidget(m_coronalFrameC);
  m_splitterTwoC->addWidget(m_viewer3D);

  QList<int> ssz;
  ssz << 150;
  ssz << 150;
  ssz << 150;
  m_splitterTwoC->setSizes(ssz);

  QList<int> gcas;
  gcas << 1000 << 200;
  m_curvesArea->setSizes(gcas);

  //QTimer::singleShot(200, m_viewer, SLOT(startDrawing()));
  QTimer::singleShot(50, m_axialCurves, SLOT(fitImage()));
  QTimer::singleShot(50, m_sagitalCurves, SLOT(fitImage()));
  QTimer::singleShot(50, m_coronalCurves, SLOT(fitImage()));
}
void
DrishtiPaint::coronalCurvesLayout_triggered()
{
  if (m_coronalCurves->enlarged())
    {
      defaultCurvesLayout_triggered();
      return;
    }

  m_viewer->stopDrawing();
    
  m_axialCurves->setLarge(false);
  m_sagitalCurves->setLarge(false);
  m_coronalCurves->setLarge(true);
  m_viewer3D->setLarge(false);

  m_splitterOneC->addWidget(m_coronalFrameC);

  m_splitterTwoC->addWidget(m_axialFrameC);
  m_splitterTwoC->addWidget(m_sagitalFrameC);
  m_splitterTwoC->addWidget(m_viewer3D);

  QList<int> ssz;
  ssz << 150;
  ssz << 150;
  ssz << 150;
  m_splitterTwoC->setSizes(ssz);

  QList<int> gcas;
  gcas << 1000 << 200;
  m_curvesArea->setSizes(gcas);

  //QTimer::singleShot(200, m_viewer, SLOT(startDrawing()));
  QTimer::singleShot(50, m_axialCurves, SLOT(fitImage()));
  QTimer::singleShot(50, m_sagitalCurves, SLOT(fitImage()));
  QTimer::singleShot(50, m_coronalCurves, SLOT(fitImage()));
}
//------------------
//------------------


//------------------
//------------------
QSplitter*
DrishtiPaint::createImageWindows()
{
  m_axialFrame = new QFrame();
  m_sagitalFrame = new QFrame();
  m_coronalFrame = new QFrame();

  m_axialFrame->setFrameShape(QFrame::Box);
  m_sagitalFrame->setFrameShape(QFrame::Box);
  m_coronalFrame->setFrameShape(QFrame::Box);

  QSplitter *splitter_0;

  splitter_0 = new QSplitter();
  splitter_0->setOrientation(Qt::Horizontal);

  m_splitterOne = new QSplitter();
  m_splitterTwo = new QSplitter();

  m_splitterOne->setOrientation(Qt::Vertical);
  m_splitterTwo->setOrientation(Qt::Vertical);

  m_splitterOne->addWidget(m_axialFrame);
  m_splitterOne->addWidget(m_viewer3D);

  m_splitterTwo->addWidget(m_sagitalFrame);
  m_splitterTwo->addWidget(m_coronalFrame);

  splitter_0->addWidget(m_splitterOne);
  splitter_0->addWidget(m_splitterTwo);

  QList<int> ssz;
  ssz << 150;
  ssz << 150;
  splitter_0->setSizes(ssz);
  m_splitterOne->setSizes(ssz);
  m_splitterTwo->setSizes(ssz);

  m_axialImage = new Slices(this);
  m_axialImage->setSliceType(ImageWidget::DSlice);

  m_sagitalImage = new Slices(this);
  m_sagitalImage->setSliceType(ImageWidget::WSlice);

  m_coronalImage = new Slices(this);
  m_coronalImage->setSliceType(ImageWidget::HSlice);

  {
    QVBoxLayout *layout = new QVBoxLayout();
    m_axialFrame->setLayout(layout);
    m_axialFrame->layout()->addWidget(m_axialImage);
  }
  {
    QVBoxLayout *layout = new QVBoxLayout();
    m_sagitalFrame->setLayout(layout);
    m_sagitalFrame->layout()->addWidget(m_sagitalImage);
  }
  {
    QVBoxLayout *layout = new QVBoxLayout();
    m_coronalFrame->setLayout(layout);
    m_coronalFrame->layout()->addWidget(m_coronalImage);
  }

  connect(m_axialImage, SIGNAL(changeLayout()), this, SLOT(on_actionZ_triggered()));
  connect(m_sagitalImage, SIGNAL(changeLayout()), this, SLOT(on_actionY_triggered()));
  connect(m_coronalImage, SIGNAL(changeLayout()), this, SLOT(on_actionX_triggered()));

  connect(m_axialImage, SIGNAL(sliceChanged(int)), m_sagitalImage, SLOT(setVLine(int)));
  connect(m_axialImage, SIGNAL(sliceChanged(int)), m_coronalImage, SLOT(setVLine(int)));

  connect(m_sagitalImage, SIGNAL(sliceChanged(int)), m_axialImage, SLOT(setVLine(int)));
  connect(m_sagitalImage, SIGNAL(sliceChanged(int)), m_coronalImage, SLOT(setHLine(int)));

  connect(m_coronalImage, SIGNAL(sliceChanged(int)), m_axialImage, SLOT(setHLine(int)));
  connect(m_coronalImage, SIGNAL(sliceChanged(int)), m_sagitalImage, SLOT(setHLine(int)));

  connect(m_axialImage, SIGNAL(xPos(int)), m_coronalImage, SLOT(setSlice(int)));
  connect(m_axialImage, SIGNAL(yPos(int)), m_sagitalImage, SLOT(setSlice(int)));

  connect(m_sagitalImage, SIGNAL(xPos(int)), m_coronalImage, SLOT(setSlice(int)));
  connect(m_sagitalImage, SIGNAL(yPos(int)), m_axialImage, SLOT(setSlice(int)));

  connect(m_coronalImage, SIGNAL(xPos(int)), m_sagitalImage, SLOT(setSlice(int)));
  connect(m_coronalImage, SIGNAL(yPos(int)), m_axialImage, SLOT(setSlice(int)));

  
  connect(m_axialImage, SIGNAL(sliceChanged(int)), m_viewer, SLOT(setDSlice(int)));
  connect(m_sagitalImage, SIGNAL(sliceChanged(int)), m_viewer, SLOT(setWSlice(int)));
  connect(m_coronalImage, SIGNAL(sliceChanged(int)), m_viewer, SLOT(setHSlice(int)));

  return splitter_0;
}

void
DrishtiPaint::on_actionGraphCut_triggered()
{
  QString hss;
  hss += "QToolButton { border-width:5; border-style:solid; border-radius:25px;";
  hss += "color:#00aa55; }";
  ui.help->setStyleSheet(hss);

  m_graphCutArea->show();
  m_curvesArea->hide();

  m_curvesMenu->hide();
  m_graphcutMenu->show();
  ui.spreadframe->show();

  ui.actionGraphCut->setChecked(true);  
  ui.actionCurves->setChecked(false);

  m_axialImage->setModeType(0);
  m_sagitalImage->setModeType(0);
  m_coronalImage->setModeType(0);

  m_axialCurves->setCurve(false);
  m_sagitalCurves->setCurve(false);
  m_coronalCurves->setCurve(false);
  curvesUi.livewire->setChecked(false);


  if (m_axialCurves->enlarged())
    on_actionZ_triggered();
  else if (m_sagitalCurves->enlarged())
    on_actionY_triggered();
  else if (m_coronalCurves->enlarged())
    on_actionX_triggered();
  else if (m_viewer3D->enlarged())
    on_action3dView_triggered();
  else
    on_actionDefaultView_triggered();
  
  {
    m_axialCurves->setLarge(false);
    m_sagitalCurves->setLarge(false);
    m_coronalCurves->setLarge(false);
  }
  
}

void
DrishtiPaint::on_actionDefaultView_triggered()
{
  m_viewer->stopDrawing();
    
  m_viewer3D->setLarge(false);
  m_axialImage->setLarge(false);
  m_sagitalImage->setLarge(false);
  m_coronalImage->setLarge(false);

  m_splitterOne->addWidget(m_axialFrame); //Z
  m_splitterOne->addWidget(m_viewer3D);

  m_splitterTwo->addWidget(m_sagitalFrame); // Y
  m_splitterTwo->addWidget(m_coronalFrame); // X

  
  QList<int> gcas;
  gcas << 1000 << 1000;
  m_graphCutArea->setSizes(gcas);
  
  QList<int> ssz;
  ssz << 150;
  ssz << 150;
  m_splitterOne->setSizes(ssz);
  m_splitterTwo->setSizes(ssz);


  QTimer::singleShot(200, m_viewer, SLOT(startDrawing()));
  QTimer::singleShot(50, m_axialImage, SLOT(fitImage()));
  QTimer::singleShot(50, m_sagitalImage, SLOT(fitImage()));
  QTimer::singleShot(50, m_coronalImage, SLOT(fitImage()));
}
void
DrishtiPaint::on_actionZ_triggered()
{
  if (m_axialImage->enlarged())
    {
      on_actionDefaultView_triggered();
      return;
    }

  m_viewer->stopDrawing();
    
  m_axialImage->setLarge(true);
  m_sagitalImage->setLarge(false);
  m_coronalImage->setLarge(false);
  m_viewer3D->setLarge(false);

  m_splitterOne->addWidget(m_axialFrame);

  m_splitterTwo->addWidget(m_sagitalFrame);
  m_splitterTwo->addWidget(m_coronalFrame);
  m_splitterTwo->addWidget(m_viewer3D);

  QList<int> ssz;
  ssz << 150;
  ssz << 150;
  ssz << 150;
  m_splitterTwo->setSizes(ssz);

  QList<int> gcas;
  gcas << 1000 << 200;
  m_graphCutArea->setSizes(gcas);


  //QTimer::singleShot(200, m_viewer, SLOT(startDrawing()));
  QTimer::singleShot(50, m_axialImage, SLOT(fitImage()));
  QTimer::singleShot(50, m_sagitalImage, SLOT(fitImage()));
  QTimer::singleShot(50, m_coronalImage, SLOT(fitImage()));
}
void
DrishtiPaint::on_actionY_triggered()
{
  if (m_sagitalImage->enlarged())
    {
      on_actionDefaultView_triggered();
      return;
    }

  m_viewer->stopDrawing();
    
  m_axialImage->setLarge(false);
  m_sagitalImage->setLarge(true);
  m_coronalImage->setLarge(false);
  m_viewer3D->setLarge(false);

  m_splitterOne->addWidget(m_sagitalFrame);

  m_splitterTwo->addWidget(m_axialFrame);
  m_splitterTwo->addWidget(m_coronalFrame);
  m_splitterTwo->addWidget(m_viewer3D);

  QList<int> ssz;
  ssz << 150;
  ssz << 150;
  ssz << 150;
  m_splitterTwo->setSizes(ssz);

  QList<int> gcas;
  gcas << 1000 << 200;
  m_graphCutArea->setSizes(gcas);

  m_axialImage->zoomToSelection();
  m_sagitalImage->zoomToSelection();
  m_coronalImage->zoomToSelection();

  //QTimer::singleShot(200, m_viewer, SLOT(startDrawing()));
  QTimer::singleShot(50, m_axialImage, SLOT(fitImage()));
  QTimer::singleShot(50, m_sagitalImage, SLOT(fitImage()));
  QTimer::singleShot(50, m_coronalImage, SLOT(fitImage()));
}
void
DrishtiPaint::on_actionX_triggered()
{
  if (m_coronalImage->enlarged())
    {
      on_actionDefaultView_triggered();
      return;
    }

  m_viewer->stopDrawing();
    
  m_axialImage->setLarge(false);
  m_sagitalImage->setLarge(false);
  m_coronalImage->setLarge(true);
  m_viewer3D->setLarge(false);

  m_splitterOne->addWidget(m_coronalFrame);

  m_splitterTwo->addWidget(m_axialFrame);
  m_splitterTwo->addWidget(m_sagitalFrame);
  m_splitterTwo->addWidget(m_viewer3D);

  QList<int> ssz;
  ssz << 150;
  ssz << 150;
  ssz << 150;
  m_splitterTwo->setSizes(ssz);

  QList<int> gcas;
  gcas << 1000 << 200;
  m_graphCutArea->setSizes(gcas);

  m_axialImage->zoomToSelection();
  m_sagitalImage->zoomToSelection();
  m_coronalImage->zoomToSelection();

  //QTimer::singleShot(200, m_viewer, SLOT(startDrawing()));
  QTimer::singleShot(50, m_axialImage, SLOT(fitImage()));
  QTimer::singleShot(50, m_sagitalImage, SLOT(fitImage()));
  QTimer::singleShot(50, m_coronalImage, SLOT(fitImage()));
}
void
DrishtiPaint::on_action3dView_triggered()
{
  m_viewer->stopDrawing();
    
  QList<int> ssz;
  ssz << 150;
  ssz << 150;
  ssz << 150;

  QList<int> gcas;
  gcas << 1000 << 200;

  if (m_curvesMenu->isVisible())
    {
      if (m_viewer3D->enlarged())
	{
	  defaultCurvesLayout_triggered();
	  return;
	}

      m_axialCurves->setLarge(false);
      m_sagitalCurves->setLarge(false);
      m_coronalCurves->setLarge(false);
      m_viewer3D->setLarge(true);

      m_splitterOneC->addWidget(m_viewer3D);
      m_splitterTwoC->addWidget(m_axialFrameC);
      m_splitterTwoC->addWidget(m_sagitalFrameC);
      m_splitterTwoC->addWidget(m_coronalFrameC);

      m_splitterTwoC->setSizes(ssz);
      m_curvesArea->setSizes(gcas);

      QTimer::singleShot(200, m_axialCurves, SLOT(zoomToSelection()));
      QTimer::singleShot(200, m_sagitalCurves, SLOT(zoomToSelection()));
      QTimer::singleShot(200, m_coronalCurves, SLOT(zoomToSelection()));
    }
  
  if (m_graphcutMenu->isVisible())
    {
      if (m_viewer3D->enlarged())
	{
	  on_actionDefaultView_triggered();
	  return;
	}

      m_axialImage->setLarge(false);
      m_sagitalImage->setLarge(false);
      m_coronalImage->setLarge(false);
      m_viewer3D->setLarge(true);

      m_splitterOne->addWidget(m_viewer3D);
      m_splitterTwo->addWidget(m_axialFrame);
      m_splitterTwo->addWidget(m_sagitalFrame);
      m_splitterTwo->addWidget(m_coronalFrame);

      m_splitterTwo->setSizes(ssz);
      m_graphCutArea->setSizes(gcas);

      QTimer::singleShot(200, m_axialImage, SLOT(zoomToSelection()));
      QTimer::singleShot(200, m_sagitalImage, SLOT(zoomToSelection()));
      QTimer::singleShot(200, m_coronalImage, SLOT(zoomToSelection()));
    }
  

  QTimer::singleShot(200, m_viewer, SLOT(startDrawing()));
}
//------------------
//------------------

void
DrishtiPaint::on_actionPort_triggered()
{
  bool ok;
  m_CMDport = QInputDialog::getInt(0,
				   "Port",
				   "Set port for socket communication with Drishti Paint",
				   m_CMDport, 7000, 8000, 1, &ok);
  if (ok)
    QMessageBox::information(0, "Drishti Paint",
			     QString("Port %1 will be used for receiving communication next time you start the program").arg(m_CMDport));
}

void
DrishtiPaint::on_actionAbout_triggered()
{
  QString mesg;
  mesg = QString("Drishti v")+QString(DRISHTI_VERSION)+"\n\n";
  mesg += "Drishti is developed by\n";
  mesg += "Ajay Limaye\n";
  mesg += "National Computational Infrastructure,\n";
  mesg += "Australian National University,\n";
  mesg += "Canberra,\n";
  mesg += "Australia\n\n";
  mesg += "Contact :\nAjay.Limaye@anu.edu.au\n\n";
  mesg += "How to cite :\nAjay Limaye; Drishti: a volume exploration and presentation tool. Proc. SPIE 8506, Developments in X-Ray Tomography VIII, 85060X (October 17, 2012)\n\n";
  mesg += "Website :\nhttps://github.com/nci/drishti\n\n";
  mesg += "Drishti User Group :\nhttps://groups.google.com/group/drishti-user-group\n\n";
  mesg += "YouTube :\nhttps://www.youtube.com/user/900acl/videos?sort=dd&flow=list&page=1&view=1\n";

  QMessageBox::information(0, "Drishti", mesg);
}


void DrishtiPaint::on_saveImage_triggered()
{
  QStringList itype;
  itype << "Z";  
  itype << "Y";
  itype << "X";
  bool ok;
  QString option = QInputDialog::getItem(0,
					 "Save Image Slice",
					 "Image plane",
					 itype,
					 0,
					 false,
					 &ok);
  if (ok)
    {
      if (option.contains("Z"))
	m_axialImage->saveImage();
      else if (option.contains("Y"))
	m_sagitalImage->saveImage();
      else
	m_coronalImage->saveImage();
    }
}

void DrishtiPaint::on_saveImageSequence_triggered()
{
  QStringList itype;
  itype << "Z";  
  itype << "Y";
  itype << "X";
  bool ok;
  QString option = QInputDialog::getItem(0,
					 "Save All Image Slices ",
					 "Image plane",
					 itype,
					 0,
					 false,
					 &ok);
  if (ok)
    {
      if (option.contains("Z"))
	m_axialImage->saveImageSequence();
      else if (option.contains("Y"))
	m_sagitalImage->saveImageSequence();
      else
	m_coronalImage->saveImageSequence();
    }
}

void
DrishtiPaint::saveTagNames()
{
  if (m_volume->isValid())
    {
      QStringList tagNames = m_tagColorEditor->tagNames();
      m_volume->saveTagNames(tagNames);
    }
}


void
DrishtiPaint::loadTagNames()
{
  QStringList tagNames = m_volume->loadTagNames();  
  m_tagColorEditor->setTagNames(tagNames);  
}


void
DrishtiPaint::checkFileSave()
{
  m_volume->checkFileSave();
}

void
DrishtiPaint::on_saveWork_triggered()
{
  if (m_volume->isValid())
    {
      QString curvesfile = m_pvlFile;
      curvesfile.replace(".pvl.nc", ".curves");
      m_axialCurves->saveCurves(curvesfile);
      m_sagitalCurves->saveCurves(curvesfile);
      m_coronalCurves->saveCurves(curvesfile);
      
      //m_volume->saveIntermediateResults(true);
      m_volume->exiting();

      QMessageBox::information(0, "Save Work", "Saved");
    }
}
void
DrishtiPaint::saveWork()
{
  if (m_volume->isValid())
    {
      QString curvesfile = m_pvlFile;
      curvesfile.replace(".pvl.nc", ".curves");
      m_axialCurves->saveCurves(curvesfile);
      m_sagitalCurves->saveCurves(curvesfile);
      m_coronalCurves->saveCurves(curvesfile);
      
      m_volume->saveIntermediateResults();
    }
}


void
DrishtiPaint::tagDSlice(int currslice, uchar* tags)
{
  m_volume->tagDSlice(currslice, tags);
}
void
DrishtiPaint::tagWSlice(int currslice, uchar* tags)
{
  m_volume->tagWSlice(currslice, tags);
}
void
DrishtiPaint::tagHSlice(int currslice, uchar* tags)
{
  m_volume->tagHSlice(currslice, tags);
}

void
DrishtiPaint::updateSliceBounds(Vec bmin, Vec bmax)
{
  int minD,minW,minH, maxD,maxW,maxH;
  minD = bmin.z;
  maxD = bmax.z;
  minW = bmin.y;
  maxW = bmax.y;
  minH = bmin.x;
  maxH = bmax.x;

  m_axialImage->setBox(minD, maxD, minW, maxW, minH, maxH);
  m_sagitalImage->setBox(minD, maxD, minW, maxW, minH, maxH);
  m_coronalImage->setBox(minD, maxD, minW, maxW, minH, maxH);

  m_axialCurves->setBox(minD, maxD, minW, maxW, minH, maxH);
  m_sagitalCurves->setBox(minD, maxD, minW, maxW, minH, maxH);
  m_coronalCurves->setBox(minD, maxD, minW, maxW, minH, maxH);
}

//---------------------//---------------------
//---------------------//---------------------
void
DrishtiPaint::getAxialSlice(int slc)
{
  m_axialCurves->setSliderValue(slc);

  uchar *slice = m_volume->getDepthSliceImage(slc);
  uchar *maskslice = m_volume->getMaskDepthSliceImage(slc);

  m_axialCurves->setImage(slice, maskslice);
}
void
DrishtiPaint::getSagitalSlice(int slc)
{
  m_sagitalCurves->setSliderValue(slc);

  uchar *slice = m_volume->getWidthSliceImage(slc); 
  uchar *maskslice = m_volume->getMaskWidthSliceImage(slc);

  m_sagitalCurves->setImage(slice, maskslice);
}
void
DrishtiPaint::getCoronalSlice(int slc)
{
  m_coronalCurves->setSliderValue(slc);

  uchar *slice = m_volume->getHeightSliceImage(slc); 
  uchar *maskslice = m_volume->getMaskHeightSliceImage(slc);

  m_coronalCurves->setImage(slice, maskslice);
}
//---------------------//---------------------
//---------------------//---------------------

void
DrishtiPaint::reloadSlices()
{
  m_axialImage->reloadSlice();
  m_sagitalImage->reloadSlice();
  m_coronalImage->reloadSlice();

  m_axialCurves->sliceChanged();
  m_sagitalCurves->sliceChanged();
  m_coronalCurves->sliceChanged();
}

void
DrishtiPaint::tagSelected(int t, bool checkBoxClicked)
{
  Global::setTag(t);
  //ui.tag->setValue(t);

  m_axialImage->updateTagColors();
  m_sagitalImage->updateTagColors();
  m_coronalImage->updateTagColors();
      
  //if (checkBoxClicked)
    {
      m_axialCurves->updateTagColors();
      m_sagitalCurves->updateTagColors();
      m_coronalCurves->updateTagColors();

      m_viewer->updateFilledBoxes();
    }
}

void
DrishtiPaint::gradType_Changed(int t)
{
  m_viewer->setGradType(t);

  m_axialImage->setGradType(t);
  m_sagitalImage->setGradType(t);
  m_coronalImage->setGradType(t);

  m_axialCurves->setGradThresholdType(t);
  m_sagitalCurves->setGradThresholdType(t);
  m_coronalCurves->setGradThresholdType(t);

  m_viewer->update();  
}

void
DrishtiPaint::minGrad_valueChanged(int t)
{
  if (t > m_maxGrad->value())
    {
      m_maxGrad->setValue(t);
    }
  m_viewer->setMinGrad(t*0.01);

  m_axialImage->setMinGrad(t*0.01);
  m_sagitalImage->setMinGrad(t*0.01);
  m_coronalImage->setMinGrad(t*0.01);

  m_axialCurves->setMinGrad(t*0.01);
  m_sagitalCurves->setMinGrad(t*0.01);
  m_coronalCurves->setMinGrad(t*0.01);

  m_viewer->update();
}
void
DrishtiPaint::maxGrad_valueChanged(int t)
{
  if (t < m_minGrad->value())
    {
      m_minGrad->setValue(t);
    }
  m_viewer->setMaxGrad(t*0.01);

  m_axialImage->setMaxGrad(t*0.01);
  m_sagitalImage->setMaxGrad(t*0.01);
  m_coronalImage->setMaxGrad(t*0.01);

  m_axialCurves->setMaxGrad(t*0.01);
  m_sagitalCurves->setMaxGrad(t*0.01);
  m_coronalCurves->setMaxGrad(t*0.01);

  m_viewer->update();
}

void
DrishtiPaint::on_saveFreq_valueChanged(int t)
{
  m_volume->setSaveFrequency(t);
}

void DrishtiPaint::sliceLod_currentIndexChanged(int l)
{
  m_axialCurves->setSliceLOD(l+1);
  m_sagitalCurves->setSliceLOD(l+1);
  m_coronalCurves->setSliceLOD(l+1);
}
void DrishtiPaint::boxSize_valueChanged(int d) { Global::setBoxSize(d); }
void DrishtiPaint::lambda_valueChanged(int d) { Global::setLambda(d); }
void DrishtiPaint::smooth_valueChanged(int d) { Global::setSmooth(d); }
void DrishtiPaint::on_radius_valueChanged(int d)
{
  Global::setSpread(d);
  m_axialImage->update();
  m_sagitalImage->update();
  m_coronalImage->update();
}
void DrishtiPaint::resetLivewire()
{
  if (m_axialCurves->inFocus())
    {      
      m_sagitalCurves->setLivewire(false);
      m_coronalCurves->setLivewire(false);
    }
  if (m_sagitalCurves->inFocus())
    {
      m_axialCurves->setLivewire(false);
      m_coronalCurves->setLivewire(false);
    }
  if (m_coronalCurves->inFocus())
    {
      m_sagitalCurves->setLivewire(false);
      m_axialCurves->setLivewire(false);
    }

  if (curvesUi.livewire->isChecked())
    livewire_clicked(true);
}

void DrishtiPaint::livewireSetting_toggled(bool on)
{
  livewire_clicked(false);
  curvesUi.livewire->setChecked(false);
  m_axialCurves->setLivewire(false);
  m_sagitalCurves->setLivewire(false);
  m_coronalCurves->setLivewire(false);
}
void DrishtiPaint::lwsmooth_currentIndexChanged(int i)
{
  m_axialCurves->setSmoothType(i);
  m_sagitalCurves->setSmoothType(i);
  m_coronalCurves->setSmoothType(i);
}
void DrishtiPaint::lwgrad_currentIndexChanged(int i)
{
  m_axialCurves->setGradType(i);
  m_sagitalCurves->setGradType(i);
  m_coronalCurves->setGradType(i);
}
void DrishtiPaint::newcurve_clicked()
{
  livewire_clicked(false);
  curvesUi.livewire->setChecked(false);

  if (m_axialCurves->inFocus()) m_axialCurves->newCurve();
  if (m_sagitalCurves->inFocus()) m_sagitalCurves->newCurve();
  if (m_coronalCurves->inFocus()) m_coronalCurves->newCurve();
}
void DrishtiPaint::endcurve_clicked()
{
  m_axialCurves->endCurve();
  m_sagitalCurves->endCurve();
  m_coronalCurves->endCurve();
}
void DrishtiPaint::morphcurves_clicked()
{
  if (m_axialCurves->inFocus()) m_axialCurves->morphCurves();
  if (m_sagitalCurves->inFocus()) m_sagitalCurves->morphCurves();
  if (m_coronalCurves->inFocus()) m_coronalCurves->morphCurves();
}
void DrishtiPaint::morphUsingDT_clicked()
{
  if (m_axialCurves->inFocus()) m_axialCurves->morphSlices();
  if (m_sagitalCurves->inFocus()) m_sagitalCurves->morphSlices();
  if (m_coronalCurves->inFocus()) m_coronalCurves->morphSlices();
}
void DrishtiPaint::deleteallcurves_clicked()
{
  if (m_axialCurves->inFocus()) m_axialCurves->deleteAllCurves();
  if (m_sagitalCurves->inFocus()) m_sagitalCurves->deleteAllCurves();
  if (m_coronalCurves->inFocus()) m_coronalCurves->deleteAllCurves();
}


QPair<QString, QList<int> >
DrishtiPaint::getTags(QString text)
{
  QStringList tglist = text.split(" ", QString::SkipEmptyParts);
  QList<int> tag;
  tag.clear();
  for(int i=0; i<tglist.count(); i++)
    {
      if (tglist[i].count()>2 && tglist[i].contains("-"))
	{
	  QStringList tl = tglist[i].split("-", QString::SkipEmptyParts);
	  if (tl.count() == 2)
	    {
	      int t1 = tl[0].toInt();
	      int t2 = tl[1].toInt();
	      for(int t=t1; t<=t2; t++)
		tag << t;
	    }
	}
      else
	{
	  int t = tglist[i].toInt();
	  if (t == -1)
	    {
	      tag.clear();
	      tag << -1;
	      break;
	    }
	  else if (t == 0)
	    {
	      tag.clear();
	      tag << 0;
	      break;
	    }
	  else if (t > 0)
	    tag << t;
	}
    }

  QString tgstr;
  for(int i=0; i<tag.count(); i++)
    tgstr += QString("%1 ").arg(tag[i]);
  
  return qMakePair(tgstr, tag);
}


void
DrishtiPaint::livewire_clicked(bool c)
{
  Curves *curves = 0;
  if (m_axialCurves->inFocus()) curves = m_axialCurves;
  if (m_sagitalCurves->inFocus()) curves = m_sagitalCurves;
  if (m_coronalCurves->inFocus()) curves = m_coronalCurves;

  // no curvewidget is inFocus
  if (!curves)
    return;
  
  if (!c)
    curves->freezeLivewire(false);

  curves->setLivewire(c);
}


void
DrishtiPaint::copyprev_clicked(bool c)
{
  Global::setCopyPrev(c);

  m_axialImage->processPrevSliceTags();
  m_sagitalImage->processPrevSliceTags();
  m_coronalImage->processPrevSliceTags();
}
void
DrishtiPaint::preverode_valueChanged(int t)
{
  Global::setPrevErode(t);

  m_axialImage->processPrevSliceTags();
  m_sagitalImage->processPrevSliceTags();
  m_coronalImage->processPrevSliceTags();
}


void
DrishtiPaint::getRawValue(int d, int w, int h)
{
  if (! m_volume->isValid()) return;

  m_axialImage->setRawValue(m_volume->rawValue(d, w, h));
  m_sagitalImage->setRawValue(m_volume->rawValue(d, w, h));
  m_coronalImage->setRawValue(m_volume->rawValue(d, w, h));

  m_axialCurves->setRawValue(m_volume->rawValue(d, w, h));
  m_sagitalCurves->setRawValue(m_volume->rawValue(d, w, h));
  m_coronalCurves->setRawValue(m_volume->rawValue(d, w, h));
}

void
DrishtiPaint::on_actionLoad_triggered()
{
  QString flnm;
  flnm = QFileDialog::getOpenFileName(0,
				      "Load Processed Volume File",
				      Global::previousDirectory(),
				      "PVL Files (*.pvl.nc)",
				      0);
				      //QFileDialog::DontUseNativeDialog);

  
  if (flnm.isEmpty())
    return;

  setFile(flnm);
}

void
DrishtiPaint::on_actionExit_triggered()
{
  if (m_volume->isValid())
    {
      QString curvesfile = m_pvlFile;
      curvesfile.replace(".pvl.nc", ".curves");
      m_axialCurves->saveCurves(curvesfile);
      m_sagitalCurves->saveCurves(curvesfile);
      m_coronalCurves->saveCurves(curvesfile);

      m_volume->saveIntermediateResults(true);
    }

//  m_viewer->init();
//  m_volume->reset();

  m_viewer->close();

  saveSettings();
  close();
}
void
DrishtiPaint::closeEvent(QCloseEvent *)
{
  if (m_pyWidget)
    m_pyWidget->close();
  on_actionExit_triggered();
}


void
DrishtiPaint::updateComposite()
{
  QImage colorMap = m_tfContainer->composite(0);

  m_axialImage->loadLookupTable(colorMap);
  m_sagitalImage->loadLookupTable(colorMap);
  m_coronalImage->loadLookupTable(colorMap);

  m_axialCurves->loadLookupTable(colorMap);
  m_sagitalCurves->loadLookupTable(colorMap);
  m_coronalCurves->loadLookupTable(colorMap);
      

  m_viewer->updateTF();
  m_viewer->update();
}

void
DrishtiPaint::changeTransferFunctionDisplay(int tfnum, QList<bool> on)
{
  if (tfnum >= 0)
    {
      SplineTransferFunction *sptr = m_tfContainer->transferFunctionPtr(tfnum);
      m_tfEditor->setTransferFunction(sptr);
    }
  else
    m_tfEditor->setTransferFunction(NULL);    

  updateComposite();
}

void
DrishtiPaint::checkStateChanged(int i, int j, bool flag)
{
  updateComposite();
}

void
DrishtiPaint::dragEnterEvent(QDragEnterEvent *event)
{
  if (event && event->mimeData())
    {
      const QMimeData *data = event->mimeData();
      if (data->hasUrls())
	{
	  QList<QUrl> urls = data->urls();

	  if (StaticFunctions::checkURLs(urls, ".pvl.nc") ||
	      StaticFunctions::checkURLs(urls, ".xml") ||
	      StaticFunctions::checkURLs(urls, ".curves") ||
	      StaticFunctions::checkURLs(urls, ".checkpoint"))
	    event->acceptProposedAction();
	}
    }
}

void
DrishtiPaint::dropEvent(QDropEvent *event)
{
  if (event && event->mimeData())
    {
      const QMimeData *data = event->mimeData();
      if (data->hasUrls())
	{
	  QUrl url = data->urls()[0];
	  QFileInfo info(url.toLocalFile());
	  if (info.exists() && info.isFile())
	    {
	      if (StaticFunctions::checkExtension(url.toLocalFile(), ".curves"))
		{
		  QString flnm = (data->urls())[0].toLocalFile();
		  m_axialCurves->loadCurves(flnm);
		  m_sagitalCurves->loadCurves(flnm);
		  m_coronalCurves->loadCurves(flnm);
		}
	      else if (StaticFunctions::checkExtension(url.toLocalFile(), ".pvl.nc") ||
		  StaticFunctions::checkExtension(url.toLocalFile(), ".xml"))
		{
		  QString flnm = (data->urls())[0].toLocalFile();
		  setFile(flnm);
		}
	      else if (StaticFunctions::checkExtension(url.toLocalFile(), ".checkpoint"))
		{
		  QString flnm = (data->urls())[0].toLocalFile();
		  loadCheckPoint(flnm);
		}
	    }
	}
    }
}


void
DrishtiPaint::setFile(QString filename)
{
  if (m_volume->isValid())
    {
      QString curvesfile = m_pvlFile;
      curvesfile.replace(".pvl.nc", ".curves");
      m_axialCurves->saveCurves(curvesfile);
      m_sagitalCurves->saveCurves(curvesfile);
      m_coronalCurves->saveCurves(curvesfile);
    }

  m_blockList.clear();

  QString flnm;

  if (StaticFunctions::checkExtension(filename, "mask.pvl.nc"))
    {
      QString fnm = filename;
      fnm.replace("mask.pvl.nc", "pvl.nc");
      flnm = fnm;
      m_tfEditor->setTransferFunction(NULL);
      m_tfManager->clearManager();
    }
  else if (StaticFunctions::checkExtension(filename, ".pvl.nc"))
    {
      flnm = filename;
      m_tfEditor->setTransferFunction(NULL);
      m_tfManager->clearManager();
    }
  else
    {
      flnm = loadVolumeFromProject(filename.toUtf8().data());

      if (flnm.isEmpty())
	{
	  QMessageBox::critical(0, "Error",
				"No volumes found in the project");
	  return;
	}
    }

  m_volume->reset();

//  m_axialImage->setGridSize(0,0,0);
//  m_sagitalImage->setGridSize(0,0,0);
//  m_coronalImage->setGridSize(0,0,0);

  m_axialCurves->setGridSize(0,0,0);
  m_sagitalCurves->setGridSize(0,0,0);
  m_coronalCurves->setGridSize(0,0,0);
  m_axialCurves->resetCurves();
  m_sagitalCurves->resetCurves();
  m_coronalCurves->resetCurves();


  //----------------------------
  // save volume information from .pvl.nc file
  VolumeInformation pvlinfo;
  VolumeInformation::volInfo(flnm.toUtf8().data(),
			     pvlinfo);
  VolumeInformation::setVolumeInformation(pvlinfo);

  if (StaticFunctions::checkExtension(filename, ".xml"))
    {
      m_tfManager->load(filename.toUtf8().data());
      m_pvlFile = flnm;
      m_xmlFile = filename;
    }
  else
    {
      m_pvlFile = flnm;
      m_xmlFile.clear();
    }

  Global::setVoxelScaling(StaticFunctions::getVoxelSizeFromHeader(m_pvlFile));
  Global::setVoxelUnit(StaticFunctions::getVoxelUnitFromHeader(m_pvlFile));
  //----------------------------

  ((QMainWindow *)Global::mainWindow())->statusBar()->showMessage(QString("loading %1").arg(flnm));

  if (m_volume->setFile(flnm) == false)
    {
      QMessageBox::critical(0, "Error", "Cannot load "+flnm);
      return;
    }

  ((QMainWindow *)Global::mainWindow())->statusBar()->showMessage(QString("%1").arg(flnm));

  
  m_viewer->setVolDataPtr(m_volume->memVolDataPtr());
  m_viewer->setMaskDataPtr(m_volume->memMaskDataPtr());

  m_axialImage->setVolPtr(m_volume->memVolDataPtr());
  m_axialImage->setMaskPtr(m_volume->memMaskDataPtr());

  m_sagitalImage->setVolPtr(m_volume->memVolDataPtr());
  m_sagitalImage->setMaskPtr(m_volume->memMaskDataPtr());

  m_coronalImage->setVolPtr(m_volume->memVolDataPtr());
  m_coronalImage->setMaskPtr(m_volume->memMaskDataPtr());

  m_axialCurves->setVolPtr(m_volume->memVolDataPtr());
  m_sagitalCurves->setVolPtr(m_volume->memVolDataPtr());
  m_coronalCurves->setVolPtr(m_volume->memVolDataPtr());

  int d, w, h;
  m_volume->gridSize(d, w, h);

  m_axialImage->setGridSize(d, w, h);
  m_sagitalImage->setGridSize(d, w, h);
  m_coronalImage->setGridSize(d, w, h);

  m_axialCurves->setGridSize(d, w, h);
  m_sagitalCurves->setGridSize(d, w, h);
  m_coronalCurves->setGridSize(d, w, h);
      
  m_viewer->setGridSize(d, w, h);

  m_axialCurves->setSliderRange(0, d-1, 0, d-1, 0);
  m_sagitalCurves->setSliderRange(0, w-1, 0, w-1, 0);
  m_coronalCurves->setSliderRange(0, h-1, 0, h-1, 0);
  
  m_axialImage->resetSliceType();
  m_sagitalImage->resetSliceType();
  m_coronalImage->resetSliceType();

  m_axialCurves->resetSliceType();
  m_sagitalCurves->resetSliceType();
  m_coronalCurves->resetSliceType();

  m_tfManager->setDisabled(false);


  if (Global::bytesPerVoxel() == 1)
    m_tfEditor->setHistogram2D(m_volume->histogram1D());
  else
    m_tfEditor->setHistogram2D(m_volume->histogram2D());

  m_tfManager->addNewTransferFunction();

  QFileInfo f(filename);
  Global::setPreviousDirectory(f.absolutePath());
  Global::addRecentFile(filename);
  updateRecentFileAction();


  
  loadTagNames();

      
  QString curvesfile = m_pvlFile;
  curvesfile.replace(".pvl.nc", ".curves");
  m_axialCurves->loadCurves(curvesfile);
  m_sagitalCurves->loadCurves(curvesfile);
  m_coronalCurves->loadCurves(curvesfile);

  on_actionGraphCut_triggered();


  viewerUi.stillStep->setValue(m_viewer->stillStep());

  viewerUi.sketchPad->setChecked(false);
  m_viewer->showSketchPad(false);

  VolumeOperations::setVolData(m_volume->memVolDataPtr());
  VolumeOperations::setMaskData(m_volume->memMaskDataPtr());
  VolumeOperations::setGridSize(d, w, h);

  VolumeMeasure::setVolData(m_volume->memVolDataPtr());
  VolumeMeasure::setMaskData(m_volume->memMaskDataPtr());
  VolumeMeasure::setGridSize(d, w, h);
  
  m_viewer->setShowBox(viewerUi.box->isChecked());
  m_viewer->updateVoxels();
}

void
DrishtiPaint::loadSettings()
{
  QString homePath = QDir::homePath();
  QFileInfo settingsFile(homePath, ".drishti.paint");
  QString flnm = settingsFile.absoluteFilePath();  

  if (! settingsFile.exists())
    return;

  QDomDocument document;
  QFile f(flnm.toUtf8().data());
  if (f.open(QIODevice::ReadOnly))
    {
      document.setContent(&f);
      f.close();
    }

  QDomElement main = document.documentElement();
  QDomNodeList dlist = main.childNodes();
  for(int i=0; i<dlist.count(); i++)
    {
      if (dlist.at(i).nodeName() == "port")
	{
	  m_CMDport = dlist.at(i).toElement().text().toInt();
	}
      else if (dlist.at(i).nodeName() == "previousdirectory")
	{
	  QString str = dlist.at(i).toElement().text();
	  Global::setPreviousDirectory(str);
	}
      else if (dlist.at(i).nodeName() == "scriptfolder")
	{
	  QString str = dlist.at(i).toElement().text();
	  Global::setScriptFolder(str);
	}
      else if (dlist.at(i).nodeName() == "recentfile")
	{
	  QString str = dlist.at(i).toElement().text();
	  Global::addRecentFile(str);
	}
      else if (dlist.at(i).nodeName() == "tagcolors")
	{
	  QString str = dlist.at(i).toElement().text();
	  QStringList col = str.split("\n",
				      QString::SkipEmptyParts);
	  uchar *colors = Global::tagColors();
	  for(int i=0; i<qMin(65536, col.size()); i++)
	    {
	      QStringList clr = col[i].split(" ",
					     QString::SkipEmptyParts);
	      colors[4*i+0] = clr[0].toInt();
	      colors[4*i+1] = clr[1].toInt();
	      colors[4*i+2] = clr[2].toInt();
	      colors[4*i+3] = 255;
	    }
	  m_tagColorEditor->setColors();
	}
    }

  updateRecentFileAction();
}

void
DrishtiPaint::saveSettings()
{
  QString str;
  QDomDocument doc("Drishti_Paint_v1.0");


  QDomElement topElement = doc.createElement("DrishtiPaintSettings");
  doc.appendChild(topElement);

  {
    QDomElement de0 = doc.createElement("port");
    QDomText tn0;
    tn0 = doc.createTextNode(QString("%1").arg(m_CMDport));
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }

  {
    QDomElement de0 = doc.createElement("previousdirectory");
    QDomText tn0;
    tn0 = doc.createTextNode(Global::previousDirectory());
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }

  {
    QDomElement de0 = doc.createElement("scriptfolder");
    QDomText tn0;
    tn0 = doc.createTextNode(Global::scriptFolder());
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }

  {
    QStringList rf = Global::recentFiles();
    for (int i=0; i<rf.count(); i++)
      {
	QDomElement de0 = doc.createElement("recentfile");
	QDomText tn0;
	tn0 = doc.createTextNode(rf[i]);
	de0.appendChild(tn0);
	topElement.appendChild(de0);
      }
  }

  {
    QDomElement de0 = doc.createElement("tagcolors");
    QString str;
    uchar *colors = Global::tagColors();
    for(int i=0; i<65536; i++)
      {
	if (i > 0) str += "               ";
	str += QString(" %1 %2 %3 %4\n").\
	              arg(colors[4*i+0]).\
	              arg(colors[4*i+1]).\
	              arg(colors[4*i+2]).\
	              arg(colors[4*i+3]);
      }
    QDomText tn0 = doc.createTextNode(str);
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }

  QString homePath = QDir::homePath();
  QFileInfo settingsFile(homePath, ".drishti.paint");
  QString flnm = settingsFile.absoluteFilePath();  

  QFile f(flnm.toUtf8().data());
  if (f.open(QIODevice::WriteOnly))
    {
      QTextStream out(&f);
      doc.save(out, 2);
      f.close();
    }
  else
    QMessageBox::information(0, "Cannot save ", flnm.toUtf8().data());
}

void
DrishtiPaint::updateRecentFileAction()
{
  QStringList rf = Global::recentFiles();
  for(int i=0; i<rf.count(); i++)
    {
      QString flnm = QFileInfo(rf[i]).fileName();
      QString text = QString("&%1. %2").arg(i+1).arg(flnm);
      m_recentFileActions[i]->setText(text);
      m_recentFileActions[i]->setData(rf[i]);
      m_recentFileActions[i]->setVisible(true);
    }
  for(int i=rf.count(); i<Global::maxRecentFiles(); i++)
    m_recentFileActions[i]->setVisible(false);

}

void
DrishtiPaint::openRecentFile()
{
  QAction *action = qobject_cast<QAction *>(sender());
  if (action)
    {
      QString filename = action->data().toString();
      QFileInfo fileInfo(filename);
      if (! fileInfo.exists())
	{
	  QMessageBox::information(0, "Error",
				   QString("Cannot locate ") +
				   filename +
				   QString(" for loading"));
	  return;
	}

      if (StaticFunctions::checkExtension(filename, ".pvl.nc") ||
	  StaticFunctions::checkExtension(filename, ".xml"))
	setFile(filename);
    }
}

QString
DrishtiPaint::loadVolumeFromProject(const char *flnm)
{
  QDomDocument document;
  QFile f(flnm);
  if (f.open(QIODevice::ReadOnly))
    {
      document.setContent(&f);
      f.close();
    }

  QDomElement main = document.documentElement();
  QDomNodeList dlist = main.childNodes();
  for(int i=0; i<dlist.count(); i++)
    {
      if (dlist.at(i).nodeName() == "volumetype")
	{
	  QString str = dlist.at(i).toElement().text();
	  if (str != "single")
	    {
	      QMessageBox::critical(0, "Error",
		QString("volumetype must be single.\n  found %1").arg(str));
	      return "";
	    }
	}
      else if (dlist.at(i).nodeName() == "volumefiles")
	{
	  // for loading volume file names with relative path
	  QFileInfo fileInfo(flnm);
	  QDir direc = fileInfo.absoluteDir();

	  QDomNodeList vlist = dlist.at(i).childNodes();
	  
	  // take only the first file
	  QString str = vlist.at(0).toElement().text();
	  QString vfile = direc.absoluteFilePath(str);

	  if (vlist.count() > 1)
	    QMessageBox::information(0, "Volume series",
	       QString("Choosing only first volume\n %1 volumes in the list").arg(vlist.count()));

	  return vfile;
	}
    }

  return "";
}


// smooth dilate/erode based on threshold
void
DrishtiPaint::sliceSmooth(int tag, int spread,
			  ushort *pv, ushort *p,
			  int width, int height,
			  int thresh)
{ 
  int a;
  for(int i=0; i<height; i++)				  
    for(int j=0; j<width; j++)			  
      {						  
	float pj = 0;					  
	int jst = qMax(0, j-spread);			  
	int jed = qMin(width-1, j+spread);			  
	for(int j1=jst; j1<=jed; j1++)		  
	  {						  
	    int idx = qBound(0, j1, width-1)*height+i;  
	    if (tag > -1)
	      a = (pv[idx]==tag ? 255 : 0);
	    else
	      a = (pv[idx]>tag ? 255 : 0);
	    pj += a;
	  }
	pj /= (jed-jst+1);
	if (tag > -1)
	  p[j*height+i] = (pj>thresh ? tag : 0);
	else
	  p[j*height+i] = (pj>thresh ? 255 : 0);
      }						  
  
  for(int j=0; j<width; j++)				  
    for(int i=0; i<height; i++)			  
      {						  
	float pi = 0;					  
	int ist = qMax(0, i-spread);			  
	int ied = qMin(height-1, i+spread);		  
	for(int i1=ist; i1<=ied; i1++)		  
	  {						  
	    int idx = j*height+qBound(0, i1, height-1); 
	    if (tag > -1)
	      a = (p[idx]==tag ? 255 : 0);
	    else
	      a = (p[idx]>tag ? 255 : 0);
	    pi += a;
	  }						  
	pi /= (ied-ist+1);
	if (tag > -1)
	  pv[j*height+i] = (pi>thresh ? tag : 0);
	else
	  pv[j*height+i] = (pi>thresh ? 255 : 0);
      }						  
}

// smooth dilate/erode based on threshold
void
DrishtiPaint::smooth(int tag, int spread,
		     ushort **pv, ushort *p,
		     int width, int height,
		     int thresh)
{ 
  int a;
  for(int j=0; j<width; j++) 
    for(int k=0; k<height; k++) 
      {
	float avg = 0; 
	for(int i=0; i<2*spread+1; i++) 
	  {
	    if (tag > -1)
	      a = (pv[i][j*height+k]==tag ? 255 : 0);
	    else
	      a = (pv[i][j*height+k]>tag ? 255 : 0);
	    avg += a;
	  }
	avg /= (2*spread+1);
	if (tag > -1)
	  p[j*height + k] = (avg>thresh ? tag : 0);
	else
	  p[j*height + k] = (avg>thresh ? 255 : 0);
      } 
}



void
DrishtiPaint::savePvlHeader(QString volfile,
			    QString pvlfile,
			    int d, int w, int h,
			    int bpv)
{  
  QString rawFile;
  QString description;
  QString voxelSize;
  QString voxelType;
  QString voxelUnit;
  QString pvlmap;
  QString rawmap;

  QDomDocument document;
  QFile f(volfile.toUtf8().data());
  if (f.open(QIODevice::ReadOnly))
    {
      document.setContent(&f);
      f.close();
    }
  
  QDomElement main = document.documentElement();
  QDomNodeList dlist = main.childNodes();
  
  for(int i=0; i<dlist.count(); i++)
    {
      if (dlist.at(i).nodeName() == "rawfile")
	rawFile = dlist.at(i).toElement().text();
      else if (dlist.at(i).nodeName() == "description")
	description = dlist.at(i).toElement().text();
      else if (dlist.at(i).nodeName() == "voxelunit")
	voxelUnit = dlist.at(i).toElement().text();
      else if (dlist.at(i).nodeName() == "voxelsize")
	voxelSize = dlist.at(i).toElement().text();
      else if (dlist.at(i).nodeName() == "rawmap")
	rawmap = dlist.at(i).toElement().text();
      else if (dlist.at(i).nodeName() == "pvlmap")
	pvlmap = dlist.at(i).toElement().text();
    }

  voxelType = "unsigned char";
  if (bpv == 2) voxelType = "unsigned short";
  
  
  QDomDocument doc("Drishti_Header");

  QDomElement topElement = doc.createElement("PvlDotNcFileHeader");
  doc.appendChild(topElement);

  {      
    QDomElement de0 = doc.createElement("rawfile");
    QDomText tn0;
    tn0 = doc.createTextNode(rawFile);
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }
  {      
    QDomElement de0 = doc.createElement("voxeltype");
    QDomText tn0;
    tn0 = doc.createTextNode(voxelType);
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }
  {      
    QDomElement de0 = doc.createElement("pvlvoxeltype");
    QDomText tn0;
    tn0 = doc.createTextNode(voxelType);
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }
  {      
    QDomElement de0 = doc.createElement("gridsize");
    QDomText tn0;
    tn0 = doc.createTextNode(QString("%1 %2 %3").arg(d).arg(w).arg(h));
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }
  {      
    QDomElement de0 = doc.createElement("voxelunit");
    QDomText tn0;
    tn0 = doc.createTextNode(voxelUnit);
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }
  {      
    QDomElement de0 = doc.createElement("voxelsize");
    QDomText tn0;
    tn0 = doc.createTextNode(voxelSize);
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }
  {      
    QDomElement de0 = doc.createElement("description");
    QDomText tn0;
    tn0 = doc.createTextNode(description);
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }
  {      
    QDomElement de0 = doc.createElement("slabsize");
    QDomText tn0;
    tn0 = doc.createTextNode(QString("%1").arg(d+1));
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }
  {      
    QDomElement de0 = doc.createElement("rawmap");
    QDomText tn0;
    tn0 = doc.createTextNode(rawmap);
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }
  {      
    QDomElement de0 = doc.createElement("pvlmap");
    QDomText tn0;
    tn0 = doc.createTextNode(pvlmap);
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }
      
  QFile pf(pvlfile.toUtf8().data());
  if (pf.open(QIODevice::WriteOnly))
    {
      QTextStream out(&pf);
      doc.save(out, 2);
      pf.close();
    }

  return;
}

void
DrishtiPaint::applyMaskOperation(int tag,
				 int smoothType,
				 int spread)
{
  int thresh = 127;
  if (smoothType == 1) thresh = 64;
  else if (smoothType == 2) thresh = 192;

  int depth, width, height;
  m_volume->gridSize(depth, width, height);
  
  int minDSlice, maxDSlice;
  int minWSlice, maxWSlice;
  int minHSlice, maxHSlice;
  m_viewer->getBox(minDSlice, maxDSlice,
		   minWSlice, maxWSlice,
		   minHSlice, maxHSlice);
  qint64 tdepth = maxDSlice-minDSlice+1;
  qint64 twidth = maxWSlice-minWSlice+1;
  qint64 theight = maxHSlice-minHSlice+1;
  
  //----------------
  QString mesg;
  if (smoothType == 0) mesg = "Smoothing";
  if (smoothType == 1) mesg = "Dilating";
  if (smoothType == 2) mesg = "Eroding";
  if (smoothType == 3) mesg = "Closing";
  if (smoothType == 4) mesg = "Opening";
  //----------------
  QProgressDialog progress(QString("%1 tagged(%2) region").arg(mesg).arg(tag),
			   QString(),
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);

  int nbytes = width*height;
  ushort *tagData = new ushort[nbytes];
  ushort *raw = new ushort[nbytes];
  ushort **val;
  val = new ushort*[2*spread+1];
  for (int i=0; i<2*spread+1; i++)
    val[i] = new ushort[nbytes];

  for(int d=minDSlice; d<=maxDSlice; d++)
    {
      int slc = d-minDSlice;
      progress.setValue((int)(100*(float)slc/(float)tdepth));
      qApp->processEvents();

      memcpy(tagData, m_volume->getMaskDepthSliceImage(d), 2*nbytes);

      
      if (slc == 0)
	{
	  memcpy(val[spread], m_volume->getMaskDepthSliceImage(d), 2*nbytes);

	  if (smoothType == 0 || smoothType == 3)
	    {
	      sliceSmooth(tag, spread,
			  val[spread], raw,
			  width, height,
			  64);
	      sliceSmooth(tag, spread,
			  val[spread], raw,
			  width, height,
			  192);
	    }
	  else if (smoothType == 4)
	    {
	      sliceSmooth(tag, spread,
			  val[spread], raw,
			  width, height,
			  192);
	      sliceSmooth(tag, spread,
			  val[spread], raw,
			  width, height,
			  64);
	    }
	  else
	    sliceSmooth(tag, spread,
			val[spread], raw,
			width, height,
			thresh);


	  
	  for(int i=-spread; i<0; i++)
	    {
	      if (d+i >= 0)
		memcpy(val[spread+i], m_volume->getMaskDepthSliceImage(d+i), 2*nbytes);
	      else
		memcpy(val[spread+i], m_volume->getMaskDepthSliceImage(0), 2*nbytes);
	      
	      if (smoothType == 0 || smoothType == 3)
		{
		  sliceSmooth(tag, spread,
			      val[spread+i], raw,
			      width, height,
			      64);
		  sliceSmooth(tag, spread,
			      val[spread+i], raw,
			      width, height,
			      192);
		}
	      else if (smoothType == 4)
		{
		  sliceSmooth(tag, spread,
			      val[spread+i], raw,
			      width, height,
			      192);
		  sliceSmooth(tag, spread,
			      val[spread+i], raw,
			      width, height,
			      64);
		}
	      else
		sliceSmooth(tag, spread,
			    val[spread+i], raw,
			    width, height,
			    thresh);
	    }
	  

	  for(int i=1; i<=spread; i++)
	    {
	      if (d+i < depth)
		memcpy(val[spread+i], m_volume->getMaskDepthSliceImage(d+i), 2*nbytes);
	      else
		memcpy(val[spread+i], m_volume->getMaskDepthSliceImage(depth-1), 2*nbytes);
	      
	      if (smoothType == 0 || smoothType == 3)
		{
		  sliceSmooth(tag, spread,
			      val[spread+i], raw,
			      width, height,
			      64);
		  sliceSmooth(tag, spread,
			      val[spread+i], raw,
			      width, height,
			      192);
		}
	      else if (smoothType == 4)
		{
		  sliceSmooth(tag, spread,
			      val[spread+i], raw,
			      width, height,
			      192);
		  sliceSmooth(tag, spread,
			      val[spread+i], raw,
			      width, height,
			      64);
		}
	      else
		sliceSmooth(tag, spread,
			    val[spread+i], raw,
			    width, height,
			    thresh);
	    }
	} // slc == 0
      else if (d < depth-spread)
	{
	  memcpy(val[2*spread], m_volume->getMaskDepthSliceImage(d+spread), 2*nbytes);
	  
	  if (smoothType == 0 || smoothType == 3)
	    {
	      sliceSmooth(tag, spread,
			  val[2*spread], raw,
			  width, height,
			  64);
	      sliceSmooth(tag, spread,
			  val[2*spread], raw,
			  width, height,
			  192);
	    }
	  else if (smoothType == 4)
	    {
	      sliceSmooth(tag, spread,
			  val[2*spread], raw,
			  width, height,
			  192);
	      sliceSmooth(tag, spread,
			  val[2*spread], raw,
			  width, height,
			  64);
	    }
	  else
	    sliceSmooth(tag, spread,
			val[2*spread], raw,
			width, height,
			thresh);

	} // d < depth-spread 
      else
	{
	  memcpy(val[2*spread], m_volume->getMaskDepthSliceImage(depth-1), 2*nbytes);
	  

	  if (smoothType == 0 || smoothType == 3)
	    {
	      sliceSmooth(tag, spread,
			  val[2*spread], raw,
			  width, height,
			  64);
	      sliceSmooth(tag, spread,
			  val[2*spread], raw,
			  width, height,
			  192);
	    }
	  else if (smoothType == 4)
	    {
	      sliceSmooth(tag, spread,
			  val[2*spread], raw,
			  width, height,
			  192);
	      sliceSmooth(tag, spread,
			  val[2*spread], raw,
			  width, height,
			  64);
	    }
	  else
	    sliceSmooth(tag, spread,
			val[2*spread], raw,
			width, height,
			thresh);

	} // d >= depth-spread 
      
      if (smoothType == 0 || smoothType == 3)
	{
	  smooth(tag, spread,
		 val, raw,
		 width, height,
		 64);
	  memcpy(val[0], raw, nbytes);
	  smooth(tag, spread,
		 val, raw,
		 width, height,
		 192);
	}
      else if (smoothType == 4)
	{
	  smooth(tag, spread,
		 val, raw,
		 width, height,
		 192);
	  memcpy(val[0], raw, nbytes);
	  smooth(tag, spread,
		 val, raw,
		 width, height,
		 64);
	}
      else
	smooth(tag, spread,
	       val, raw,
	       width, height,
	       thresh);
      
      
      // now shift the planes
      ushort *tmp = val[0];
      for(int i=0; i<2*spread; i++)
	val[i] = val[i+1];
      val[2*spread] = tmp;
  
      for(int w=minWSlice; w<=maxWSlice; w++)
	for(int h=minHSlice; h<=maxHSlice; h++)
	  {
	    if (tagData[w*height+h] == 0 || tagData[w*height+h] == tag)
	      tagData[w*height+h] = raw[w*height+h];
	  }
      
      m_volume->setMaskDepthSlice(d, (uchar*)tagData);
    }
  
  delete [] raw;
  delete [] tagData;
  if (spread > 0)
    {
      for (int i=0; i<2*spread+1; i++)
	delete [] val[i];
      delete [] val;
    }
  
  progress.setValue(100);  

  reloadSlices();
}

void
DrishtiPaint::stillStep_changed(double step)
{
  m_viewer->setStillAndDragStep(step, step);
}

void
DrishtiPaint::connectImageWidget()
{
  connect(m_axialImage, SIGNAL(saveWork()),
	  this, SLOT(saveWork()));  
  connect(m_sagitalImage, SIGNAL(saveWork()),
	  this, SLOT(saveWork()));  
  connect(m_coronalImage, SIGNAL(saveWork()),
	  this, SLOT(saveWork()));  

  connect(m_axialImage, SIGNAL(getRawValue(int, int, int)),
	  this, SLOT(getRawValue(int, int, int)));
  connect(m_coronalImage, SIGNAL(getRawValue(int, int, int)),
	  this, SLOT(getRawValue(int, int, int)));
  connect(m_sagitalImage, SIGNAL(getRawValue(int, int, int)),
	  this, SLOT(getRawValue(int, int, int)));

  connect(m_axialImage, SIGNAL(applyMaskOperation(int, int, int)),
	  this, SLOT(applyMaskOperation(int, int, int)));
  connect(m_sagitalImage, SIGNAL(applyMaskOperation(int, int, int)),
	  this, SLOT(applyMaskOperation(int, int, int)));
  connect(m_coronalImage, SIGNAL(applyMaskOperation(int, int, int)),
	  this, SLOT(applyMaskOperation(int, int, int)));


  connect(m_axialImage, SIGNAL(tagDSlice(int, uchar*)),
	  this, SLOT(tagDSlice(int, uchar*)));

  connect(m_sagitalImage, SIGNAL(tagWSlice(int, uchar*)),
	  this, SLOT(tagWSlice(int, uchar*)));

  connect(m_coronalImage, SIGNAL(tagHSlice(int, uchar*)),
	  this, SLOT(tagHSlice(int, uchar*)));  


  connect(m_axialImage, SIGNAL(viewerUpdate()),
	  m_viewer, SLOT(update()));
  connect(m_sagitalImage, SIGNAL(viewerUpdate()),
	  m_viewer, SLOT(update()));
  connect(m_coronalImage, SIGNAL(viewerUpdate()),
	  m_viewer, SLOT(update()));


  connect(m_axialImage, SIGNAL(shrinkwrap(Vec, Vec, int, bool, int,
					   bool, int, int, int, int)),
	  this, SLOT(shrinkwrap(Vec, Vec, int, bool, int,
				bool, int, int, int, int)));
  connect(m_sagitalImage, SIGNAL(shrinkwrap(Vec, Vec, int, bool, int,
					   bool, int, int, int, int)),
	  this, SLOT(shrinkwrap(Vec, Vec, int, bool, int,
				bool, int, int, int, int)));
  connect(m_coronalImage, SIGNAL(shrinkwrap(Vec, Vec, int, bool, int,
					   bool, int, int, int, int)),
	  this, SLOT(shrinkwrap(Vec, Vec, int, bool, int,
				bool, int, int, int, int)));

  connect(m_axialImage, SIGNAL(connectedRegion(int,int,int,Vec,Vec,int,int)),
	  this, SLOT(connectedRegion(int,int,int,Vec,Vec,int,int)));
  connect(m_sagitalImage, SIGNAL(connectedRegion(int,int,int,Vec,Vec,int,int)),
	  this, SLOT(connectedRegion(int,int,int,Vec,Vec,int,int)));
  connect(m_coronalImage, SIGNAL(connectedRegion(int,int,int,Vec,Vec,int,int)),
	  this, SLOT(connectedRegion(int,int,int,Vec,Vec,int,int)));
}

void
DrishtiPaint::connectCurvesWidget()
{
  connect(m_axialCurves, SIGNAL(saveWork()), this, SLOT(saveWork()));
  connect(m_sagitalCurves, SIGNAL(saveWork()), this, SLOT(saveWork()));
  connect(m_coronalCurves, SIGNAL(saveWork()), this, SLOT(saveWork()));  

  connect(m_axialCurves,  SIGNAL(gotFocus()), this, SLOT(resetLivewire()));
  connect(m_sagitalCurves,  SIGNAL(gotFocus()), this, SLOT(resetLivewire()));
  connect(m_coronalCurves,  SIGNAL(gotFocus()), this, SLOT(resetLivewire()));

  connect(m_axialCurves,  SIGNAL(gotFocus()), m_sagitalCurves,SLOT(releaseFocus()));
  connect(m_axialCurves,  SIGNAL(gotFocus()), m_coronalCurves,SLOT(releaseFocus()));
  connect(m_sagitalCurves,SIGNAL(gotFocus()), m_axialCurves,  SLOT(releaseFocus()));
  connect(m_sagitalCurves,SIGNAL(gotFocus()), m_coronalCurves,SLOT(releaseFocus()));
  connect(m_coronalCurves,SIGNAL(gotFocus()), m_axialCurves,  SLOT(releaseFocus()));
  connect(m_coronalCurves,SIGNAL(gotFocus()), m_sagitalCurves,SLOT(releaseFocus()));

//  connect(m_axialCurves, SIGNAL(getRawValue(int, int, int)),
//	  this, SLOT(getRawValue(int, int, int)));
//  connect(m_sagitalCurves, SIGNAL(getRawValue(int, int, int)),
//	  this, SLOT(getRawValue(int, int, int)));
//  connect(m_coronalCurves, SIGNAL(getRawValue(int, int, int)),
//	  this, SLOT(getRawValue(int, int, int)));  

//  connect(m_curves, SIGNAL(showEndCurve()),
//	  curvesUi.endcurve, SLOT(show()));
//  connect(m_curves, SIGNAL(hideEndCurve()),
//	  curvesUi.endcurve, SLOT(hide()));
//
//  connect(m_curves, SIGNAL(viewerUpdate()),
//	  m_viewer, SLOT(update()));
}

void
DrishtiPaint::setShowPosition(bool b)
{
  m_axialImage->setShowPosition(b);
  m_sagitalImage->setShowPosition(b);
  m_coronalImage->setShowPosition(b);

  m_axialCurves->setShowPosition(b);
  m_sagitalCurves->setShowPosition(b);
  m_coronalCurves->setShowPosition(b);
}

void
DrishtiPaint::connectViewerMenu()
{
  m_viewer->setUIPointer(&viewerUi);

  connect(ui.radius, SIGNAL(valueChanged(int)),
	  viewerUi.radiusSurface, SLOT(setValue(int)));
  
  connect(viewerUi.update, SIGNAL(clicked()),
	  m_viewer, SLOT(updateVoxels()));

  connect(viewerUi.box, SIGNAL(clicked(bool)),
	  m_viewer, SLOT(setShowBox(bool)));
  connect(m_viewer, SIGNAL(showBoxChanged(bool)),
	  viewerUi.box, SLOT(setChecked(bool)));
  connect(viewerUi.showPosition, SIGNAL(clicked(bool)),
	  m_viewer, SLOT(setShowPosition(bool)));

  connect(viewerUi.showPosition, SIGNAL(clicked(bool)),
	  this, SLOT(setShowPosition(bool)));

  connect(viewerUi.snapshot, SIGNAL(clicked()),
	  m_viewer, SLOT(saveImage()));

  connect(viewerUi.skipLayers, SIGNAL(valueChanged(int)),
	  m_viewer, SLOT(setSkipLayers(int)));
  connect(viewerUi.skipVoxels, SIGNAL(valueChanged(int)),
	  m_viewer, SLOT(setSkipVoxels(int)));
  connect(viewerUi.nearest, SIGNAL(clicked(bool)),
	  m_viewer, SLOT(setExactCoord(bool)));

  connect(viewerUi.stillStep, SIGNAL(valueChanged(double)),
	  this, SLOT(stillStep_changed(double)));


  connect(viewerUi.sketchPad, SIGNAL(clicked(bool)),
	  m_viewer, SLOT(showSketchPad(bool)));


  setupLightParameters();
}

void
DrishtiPaint::setupLightParameters()
{
  m_viewEdge = new PopUpSlider(m_viewer, Qt::Horizontal);
  m_viewShadow = new PopUpSlider(m_viewer, Qt::Horizontal);

  m_viewEdge->setText("Edges");
  m_viewShadow->setText("Shadow");

  m_viewEdge->setRange(0, 10);
  m_viewEdge->setValue(5);
  m_viewShadow->setRange(0, 20);
  m_viewShadow->setValue(5);

  QSpacerItem *spitem1 = new QSpacerItem(5,5,QSizePolicy::Minimum, QSizePolicy::Fixed);
  QSpacerItem *spitem2 = new QSpacerItem(5,5,QSizePolicy::Minimum, QSizePolicy::Fixed);
  
  viewerUi.popupLight->setMargin(2);
  viewerUi.popupLight->addWidget(m_viewEdge);
  viewerUi.popupLight->addWidget(m_viewShadow);

  connect(m_viewEdge, SIGNAL(valueChanged(int)),
	  m_viewer, SLOT(setEdge(int)));
  connect(m_viewShadow, SIGNAL(valueChanged(int)),
	  m_viewer, SLOT(setShadow(int)));
}

void
DrishtiPaint::getShadowColor()
{
  Vec sclr = m_viewer->shadowColor();

  QColor clr = QColor(sclr.x, sclr.y, sclr.z);
  clr = DColorDialog::getColor(clr);
  if (!clr.isValid())
    return;

  sclr = Vec(clr.red(), clr.green(), clr.blue());
  m_viewer->setShadowColor(sclr);
}

void
DrishtiPaint::getEdgeColor()
{
  Vec sclr = m_viewer->edgeColor();

  QColor clr = QColor(sclr.x, sclr.y, sclr.z);
  clr = DColorDialog::getColor(clr);
  if (!clr.isValid())
    return;

  sclr = Vec(clr.red(), clr.green(), clr.blue());
  m_viewer->setEdgeColor(sclr);
}

void
DrishtiPaint::getBGColor()
{
  Vec sclr = m_viewer->bgColor();

  QColor clr = QColor(sclr.x, sclr.y, sclr.z);
  clr = DColorDialog::getColor(clr);
  if (!clr.isValid())
    return;

  sclr = Vec(clr.red(), clr.green(), clr.blue());
  m_viewer->setBGColor(sclr);
}

void
DrishtiPaint::miscConnections()
{
  connect(m_tfManager,
	  SIGNAL(changeTransferFunctionDisplay(int, QList<bool>)),
	  this,
	  SLOT(changeTransferFunctionDisplay(int, QList<bool>)));

  connect(m_tfManager,
	  SIGNAL(checkStateChanged(int, int, bool)),
	  this,
	  SLOT(checkStateChanged(int, int, bool)));

  connect(m_tfEditor, SIGNAL(updateComposite()),
	  this, SLOT(updateComposite()));



  connect(m_tagColorEditor, SIGNAL(tagColorChanged()),
	  m_axialImage, SLOT(updateTagColors()));
  connect(m_tagColorEditor, SIGNAL(tagColorChanged()),
	  m_sagitalImage, SLOT(updateTagColors()));
  connect(m_tagColorEditor, SIGNAL(tagColorChanged()),
	  m_coronalImage, SLOT(updateTagColors()));

  
  connect(m_tagColorEditor, SIGNAL(tagSelected(int, bool)),
	  this, SLOT(tagSelected(int, bool)));

  connect(m_tagColorEditor, SIGNAL(tagColorChanged()),
	  m_axialCurves, SLOT(updateTagColors()));
  connect(m_tagColorEditor, SIGNAL(tagColorChanged()),
	  m_sagitalCurves, SLOT(updateTagColors()));
  connect(m_tagColorEditor, SIGNAL(tagColorChanged()),
	  m_coronalCurves, SLOT(updateTagColors()));

}

void
DrishtiPaint::connectCurvesMenu()
{
  //ui.zoomup->setIcon(QIcon(":/images/zoom-in.png"));
  //ui.zoomdown->setIcon(QIcon(":/images/zoom-out.png"));
  
  
  connect(curvesUi.bakeCurves, SIGNAL(clicked()),
	  this, SLOT(bakeCurves_clicked()));

  connect(curvesUi.livewire, SIGNAL(clicked(bool)),
	  this, SLOT(livewire_clicked(bool)));

  connect(curvesUi.sliceLod, SIGNAL(currentIndexChanged(int)),
	  this, SLOT(sliceLod_currentIndexChanged(int)));
  connect(curvesUi.lwsmooth, SIGNAL(currentIndexChanged(int)),
	  this, SLOT(lwsmooth_currentIndexChanged(int)));
  connect(curvesUi.lwgrad, SIGNAL(currentIndexChanged(int)),
	  this, SLOT(lwgrad_currentIndexChanged(int)));

  connect(curvesUi.morphcurves, SIGNAL(clicked()),
  	  this, SLOT(morphcurves_clicked()));

  connect(curvesUi.morphUsingDT, SIGNAL(clicked()),
	  this, SLOT(morphUsingDT_clicked()));

  connect(curvesUi.newcurve, SIGNAL(clicked()),
	  this, SLOT(newcurve_clicked()));
  connect(curvesUi.endcurve, SIGNAL(clicked()),
	  this, SLOT(endcurve_clicked()));

  connect(curvesUi.deleteallcurves, SIGNAL(clicked()),
	  this, SLOT(deleteallcurves_clicked()));
}

void
DrishtiPaint::connectGraphCutMenu()
{
  connect(graphcutUi.copyprev, SIGNAL(clicked(bool)),
	  this, SLOT(copyprev_clicked(bool)));

  connect(graphcutUi.preverode, SIGNAL(valueChanged(int)),
	  this, SLOT(preverode_valueChanged(int)));
  connect(graphcutUi.smooth, SIGNAL(valueChanged(int)),
	  this, SLOT(smooth_valueChanged(int)));
  connect(graphcutUi.lambda, SIGNAL(valueChanged(int)),
	  this, SLOT(lambda_valueChanged(int)));
  connect(graphcutUi.boxSize, SIGNAL(valueChanged(int)),
	  this, SLOT(boxSize_valueChanged(int)));
}


void
DrishtiPaint::on_actionSave_TF_triggered()
{
  QString tflnm = QFileDialog::getSaveFileName(0,
					       "Save Transfer Functions",
					       Global::previousDirectory(),
					       "Files (*.xml)",
					       0);
					       //QFileDialog::DontUseNativeDialog);


  QDomDocument doc("Drishti_v1.0");

  QDomElement topElement = doc.createElement("Drishti");
  doc.appendChild(topElement);

  QFile f(tflnm);
  if (f.open(QIODevice::WriteOnly))
    {
      QTextStream out(&f);
      doc.save(out, 2);
      f.close();
    }
  
  m_tfManager->save(tflnm.toUtf8().data());

}

void
DrishtiPaint::on_actionLoad_TF_triggered()
{
  QString tflnm = QFileDialog::getOpenFileName(0,
					       "Load Transfer Functions",
					       Global::previousDirectory(),
					       "Files (*.xml)",
					       0);
                                               //QFileDialog::DontUseNativeDialog);

  
  m_tfManager->load(tflnm.toUtf8().data());
}

bool
DrishtiPaint::sliceZeroAtTop()
{
  bool save0attop = true;
  bool ok = false;
  QStringList slevels;
  slevels << "Yes - (default)";  
  slevels << "No - slice 0 is bottom slice";
  QString option = QInputDialog::getItem(0,
		   "Load Mask",
		   "Slice 0 is top slice ?",
		    slevels,
			  0,
		      false,
		       &ok);
  if (ok)
    {
      QStringList op = option.split(' ');
      if (op[0] == "No")
	{
	  save0attop = false;
	  QMessageBox::information(0, "Load Mask", "First slice is now bottom slice.");
	}
    }

  return save0attop;
}
//-----------------------------


//-----------------------------
void
DrishtiPaint::on_actionLoadMask_triggered()
{
  QString flnm;
  flnm = QFileDialog::getOpenFileName(0,
				      "Load Mask File",
				      Global::previousDirectory(),
				      "MASK Files (*.mask.sc | *.mask)",
				      0);
				      //QFileDialog::DontUseNativeDialog);

  
  if (flnm.isEmpty())
    return;

  uchar vt;
  int lrd, lrw, lrh;

  int m_depth, m_width, m_height;
  m_volume->gridSize(m_depth, m_width, m_height);

  QFile mfile;
  if (StaticFunctions::checkExtension(flnm, ".mask"))
    {
      mfile.setFileName(flnm);
      mfile.open(QFile::ReadOnly);
      mfile.read((char*)&vt, 1);
      mfile.read((char*)&lrd, 4);
      mfile.read((char*)&lrw, 4);
      mfile.read((char*)&lrh, 4);
    }
  else if (StaticFunctions::checkExtension(flnm, ".mask.sc"))
    {
      char chkver[10];
      mfile.setFileName(flnm);
      mfile.open(QFile::ReadOnly);
      mfile.read((char*)chkver, 6);
      mfile.read((char*)&vt, 1);
      mfile.read((char*)&lrd, 4);
      mfile.read((char*)&lrw, 4);
      mfile.read((char*)&lrh, 4);
    }
  
  float scld = (float)m_depth/lrd;
  float sclw = (float)m_width/lrw;
  float sclh = (float)m_height/lrh;
  
  QString mesg;
  mesg += QString("Volume Size : %1 %2 %3\n").			\
	              arg(m_height).arg(m_width).arg(m_depth);
  mesg += QString("Input Mask Size : %1 %2 %3\n").	\
	              arg(lrh).arg(lrw).arg(lrd);
  mesg += QString("Scaling applied : %1 %2 %3").	\
	              arg(sclh).arg(sclw).arg(scld);
  QMessageBox::information(0, "", mesg);

  uchar *lmask;
  lmask = new uchar[(qint64)lrd*(qint64)lrw*(qint64)lrh];

  if (StaticFunctions::checkExtension(flnm, ".mask"))
    {
      mfile.read((char*)lmask, (qint64)lrd*(qint64)lrw*(qint64)lrh);
    }
  else if (StaticFunctions::checkExtension(flnm, ".mask.sc"))
    {
      int mb100, nblocks;
      mfile.read((char*)&nblocks, 4);
      mfile.read((char*)&mb100, 4);
      uchar *vBuf = new uchar[mb100];
      for(qint64 i=0; i<nblocks; i++)
	{
	  int vbsize;
	  mfile.read((char*)&vbsize, 4);
	  mfile.read((char*)vBuf, vbsize);
	  int bufsize = blosc_decompress(vBuf, lmask+i*mb100, mb100);
	  if (bufsize < 0)
	    {
	      QMessageBox::information(0, "", "Error in decompression : .mask.sc file not read");
	      mfile.close();
	      return;
	    }
	}
    }
  
  mfile.close();

  
  uchar *maskptr = m_volume->memMaskDataPtr();

  bool s0top = sliceZeroAtTop();

  QProgressDialog progress("Updating voxel structure",
			   QString(),
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);
  int d;
  for(qint64 slc=0; slc<lrd; slc++)
    {      
      if (s0top)
	d = slc;
      else
	d = lrd-1-slc;
      progress.setValue((95.0*d)/lrd);
      for(qint64 w=0; w<lrw; w++)
      for(qint64 h=0; h<lrh; h++)
	{
	  if (lmask[slc*lrw*lrh + w*lrh + h] > 0)
	    {
	      int ds = qMax(0, (int)(d*scld-scld/2));
	      int ws = qMax(0, (int)(w*sclw-sclw/2));
	      int hs = qMax(0, (int)(h*sclh-sclh/2));
	      int de = qMin(m_depth-1, (int)((d+1)*scld-scld/2));
	      int we = qMin(m_width-1, (int)((w+1)*sclw-sclw/2));
	      int he = qMin(m_height-1,(int)((h+1)*sclh-sclh/2));

	      uchar mv = lmask[slc*lrw*lrh + w*lrh + h];
	      for(qint64 d0=ds; d0<de; d0++)
		for(qint64 w0=ws; w0<we; w0++)
		  for(qint64 h0=hs; h0<he; h0++)
		    maskptr[d0*m_width*m_height + w0*m_height + h0] = mv;
	    }
	}
    }

  delete [] lmask;
  progress.setValue(100);

  QMessageBox::information(0, "", "Transfer Done.\n  Check 2D slice view.\n  Update 3D viewer.\n  If everything looks alright Save Work using File menu to save this change to mask file.");
}
//-----------------------------


//-----------------------------
void
DrishtiPaint::on_actionExtractTag_triggered()
{
  QStringList dtypes;
  QList<int> tag;

  bool ok;
  //----------------
  QString tagstr = QInputDialog::getText(0, "Extract volume data for given label",
	    "Label Numbers (labels should be separated by space.\n\n-1 for all labels;\nFor e.g. 1 2 5 will extract labels 1, 2 and 5\n2-5 is same as specifying  2 3 4 5)",
					 QLineEdit::Normal,
					 "-1",
					 &ok);
  tag.clear();
  if (ok && !tagstr.isEmpty())
    {
      QStringList tglist = tagstr.split(" ", QString::SkipEmptyParts);
      for(int i=0; i<tglist.count(); i++)
	{
	  if (tglist[i].count()>2 && tglist[i].contains("-"))
	    {
	      QStringList tl = tglist[i].split("-", QString::SkipEmptyParts);
	      if (tl.count() == 2)
		{
		  int t1 = tl[0].toInt();
		  int t2 = tl[1].toInt();
		  for(int t=t1; t<=t2; t++)
		    tag << t;
		}
	    }
	  else
	    {
	      int t = tglist[i].toInt();
	      if (t == -3)
		{
		  tag.clear();
		  tag << -3;
		  break;
		}
	      else if (t == -2)
		{
		  tag.clear();
		  tag << -2;
		  break;
		}
	      else if (t == -1)
		{
		  tag.clear();
		  tag << -1;
		  break;
		}
	      else if (t == 0)
		{
		  tag.clear();
		  tag << 0;
		  break;
		}
	      else
		tag << t;
	    }
	}
    }
  else
    tag << -1;
  //----------------

  //  QMessageBox::information(0, "", QString("%1 : %2").arg(tag.count()).arg(tag[0]));

  //----------------
  int extractType = 1; // extract using label

  dtypes.clear();
  dtypes << "Label Only"
	 << "Label + Transfer Function";
  //dtypes << "Labels From Another Volume";

  QString option = QInputDialog::getItem(0,
					 "Extract Volume Data",
					 "Extract Using Label + Transfer Function",
					 dtypes,
					 0,
					 false,
					 &ok);
  if (!ok)
    return;
      
  if (option == "Label Only") extractType = 1;
  else if (option == "Label + Transfer Function") extractType = 2;
//  else if (option == "Labels From Another Volume")
//    {
//      extractFromAnotherVolume(tag);
//      return;
//    }
  //----------------

  //----------------
  int outsideVal = 0;
  int maxVal = 255;
  if (Global::bytesPerVoxel() == 2)
    maxVal = 65535;

  outsideVal = QInputDialog::getInt(0,
				    "Outside value",
				    QString("Set outside value (0-%1) to").arg(maxVal),
				    0, 0, maxVal, 1);

  //----------------


  int depth, width, height;
  m_volume->gridSize(depth, width, height);
  
  int minDSlice, maxDSlice;
  int minWSlice, maxWSlice;
  int minHSlice, maxHSlice;
  m_viewer->getBox(minDSlice, maxDSlice,
		   minWSlice, maxWSlice,
		   minHSlice, maxHSlice);
  qint64 tdepth = maxDSlice-minDSlice+1;
  qint64 twidth = maxWSlice-minWSlice+1;
  qint64 theight = maxHSlice-minHSlice+1;
  
  QString pvlFilename = m_volume->fileName();
  QString tflnm = QFileDialog::getSaveFileName(0,
					       "Save volume",
					       QFileInfo(pvlFilename).absolutePath(),
					       "Volume Data Files (*.pvl.nc)",
					       0);
					       //QFileDialog::DontUseNativeDialog);
  
  if (tflnm.isEmpty())
    return;

  if (StaticFunctions::checkExtension(tflnm, ".pvl.nc.pvl.nc"))
    tflnm = tflnm.chopped(7);

  QMessageBox::information(0, "", tflnm);
  if (!StaticFunctions::checkExtension(tflnm, ".pvl.nc"))
    tflnm += ".pvl.nc";
  
  savePvlHeader(m_volume->fileName(),
		tflnm,
		tdepth, twidth, theight,
		Global::bytesPerVoxel());

  QStringList tflnms;
  tflnms << tflnm+".001";
  VolumeFileManager tFile;
  tFile.setFilenameList(tflnms);
  if (Global::bytesPerVoxel() == 1)
    tFile.setVoxelType(VolumeFileManager::_UChar);
  else
    tFile.setVoxelType(VolumeFileManager::_UShort);
  tFile.setDepth(tdepth);
  tFile.setWidth(twidth);
  tFile.setHeight(theight);
  tFile.setHeaderSize(13);
  tFile.setSlabSize(tdepth+1);
  tFile.createFile(true, false);

  QProgressDialog progress("Extracting labeled region from volume data",
			   QString(),
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);

  uchar *lut = Global::lut();
  int nbytes = width*height*Global::bytesPerVoxel();
  int nbytesRAW = width*height*2; // 16 bit mask
  uchar *raw = new uchar[nbytesRAW];
  
  QList<Vec> cPos =  m_viewer->clipPos();
  QList<Vec> cNorm = m_viewer->clipNorm();
  //----------------------------------

  

  for(int d=minDSlice; d<=maxDSlice; d++)
    {
      int slc = d-minDSlice;
      progress.setValue((int)(100*(float)slc/(float)tdepth));
      qApp->processEvents();

      uchar *slice = m_volume->getDepthSliceImage(d);
      ushort *sliceUS = 0;
      if (Global::bytesPerVoxel() == 2)
	sliceUS = (ushort*)slice;
      
      // we get value+grad from volume
      // we need only value part
      if (Global::bytesPerVoxel() == 1)
	{
	  int i=0;
	  for(int w=minWSlice; w<=maxWSlice; w++)
	    for(int h=minHSlice; h<=maxHSlice; h++)
	      {
		slice[i] = slice[w*height+h];
		i++;
	      }
	}
      else
	{
	  int i=0;
	  for(int w=minWSlice; w<=maxWSlice; w++)
	    for(int h=minHSlice; h<=maxHSlice; h++)
	      {
		sliceUS[i] = sliceUS[w*height+h];
		i++;
	      }
	}
      	  
      memcpy(raw, m_volume->getMaskDepthSliceImage(d), nbytesRAW);

      //-----------------------------
      {
	// convert 16bit labels into 8bit mask
	if (tag[0] == -1)
	  {
	    for(int w=minWSlice; w<=maxWSlice; w++)
	      for(int h=minHSlice; h<=maxHSlice; h++)
		raw[w*height+h] = (((ushort*)raw)[w*height+h] > 0 ? 255 : 0);
	  }
	else if (tag[0] == 0)
	  {
	    for(int w=minWSlice; w<=maxWSlice; w++)
	      for(int h=minHSlice; h<=maxHSlice; h++)
		raw[w*height+h] = (((ushort*)raw)[w*height+h] == 0 ? 255 : 0);
	  }
	else
	  {
	    for(int w=minWSlice; w<=maxWSlice; w++)
	      for(int h=minHSlice; h<=maxHSlice; h++)
		raw[w*height+h] = (tag.contains(((ushort*)raw)[w*height+h]) ? 255 : 0);
	  }
      }
      //-----------------------------


      //-----------------------------
      {
	int i=0;
	for(int w=minWSlice; w<=maxWSlice; w++)
	  for(int h=minHSlice; h<=maxHSlice; h++)
	    {
	      raw[i] = raw[w*height+h];
	      i++;
	    }
      }
      //-----------------------------
      

      //-----------------------------
      // use tag mask + transfer function to extract volume data
      if (extractType == 2)
	{
	  int sval = 0;
	  if (tag[0] == -1) sval = 0;
	  else if (tag[0] == 0) sval = 255;
	  else sval = 0;
	  
	  if (Global::bytesPerVoxel() == 1)
	    {
	      int i=0;
	      for(int w=minWSlice; w<=maxWSlice; w++)
		for(int h=minHSlice; h<=maxHSlice; h++)
		  {
		    if (raw[i] != sval &&
			lut[4*slice[i]+3] < 5)
		      raw[i] = sval;
		    i++;
		  }
	    }
	  else
	    {
	      int i=0;
	      for(int w=minWSlice; w<=maxWSlice; w++)
		for(int h=minHSlice; h<=maxHSlice; h++)
		  {
		    int v = sliceUS[i];
		    if (raw[i] != sval &&
			lut[4*v+3] < 5)
		      raw[i] = sval;
		    i++;
		  }
	    }

	}
      //-----------------------------

      
      // now mask data with labels
      int i=0;
      for(int w=minWSlice; w<=maxWSlice; w++)
	for(int h=minHSlice; h<=maxHSlice; h++)
	  {
	    bool clipped = VolumeOperations::checkClipped(Vec(h,w,d));
	    
	    if (Global::bytesPerVoxel() == 1)
	      {
		if (!clipped)
		  {
		    if (raw[i] != 255)
		      slice[i] = outsideVal;
		  }
	      }
	    else
	      {
		if (!clipped)
		  {
		    if (raw[i] != 255)
		      sliceUS[i] = outsideVal;
		  }
	      }
	    i++;
	  }	  
      tFile.setSlice(slc, slice);
    }

  delete [] raw;

  
  progress.setValue(100);  
  QMessageBox::information(0, "Save", "-----Done-----");
}


void
DrishtiPaint::updateCurveMask(uchar *curveMask,
			      int depth, int width, int height,
			      int tdepth, int twidth, int theight,
			      int minDSlice, int minWSlice, int minHSlice,
			      int maxDSlice, int maxWSlice, int maxHSlice)
{
  QProgressDialog progress("Generating Curve Mask",
			   QString(),
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);

  if (m_axialCurves->inFocus() && m_axialCurves->curvesPresent())
    {
      uchar *mask = new uchar[width*height]; 
      for(qint64 d=minDSlice; d<=maxDSlice; d++)
	{
	  int slc = d-minDSlice;
	  progress.setValue((int)(100*(float)slc/(float)tdepth));
	  qApp->processEvents();
	  
	  memset(mask, 0, width*height);
	  m_axialCurves->paintUsingCurves(0, d, height, width, mask);
	  for(qint64 w=minWSlice; w<=maxWSlice; w++)
	    for(qint64 h=minHSlice; h<=maxHSlice; h++)
	      {
		if (mask[w*height+h] > 0)
		  curveMask[(d-minDSlice)*twidth*theight +
			    (w-minWSlice)*theight +
			    (h-minHSlice)] = mask[w*height+h];
	      }
	}
      delete [] mask;
    }
  if (m_sagitalCurves->inFocus() && m_sagitalCurves->curvesPresent())
    {
      uchar *mask = new uchar[depth*height]; 
      for(qint64 w=minWSlice; w<=maxWSlice; w++)
	{
	  int slc = w-minWSlice;
	  progress.setValue((int)(100*(float)slc/(float)twidth));
	  qApp->processEvents();
	  
	  memset(mask, 0, depth*height);
	  m_sagitalCurves->paintUsingCurves(1, w, height, depth, mask);
	  for(qint64 d=minDSlice; d<=maxDSlice; d++)
	    for(qint64 h=minHSlice; h<=maxHSlice; h++)
	      {
		if (mask[d*height+h] > 0)
		  curveMask[(d-minDSlice)*twidth*theight +
			    (w-minWSlice)*theight +
			    (h-minHSlice)] = mask[d*height+h];
	      }
	}
      delete [] mask;
    }
  if (m_coronalCurves->inFocus() && m_coronalCurves->curvesPresent())
    {
      uchar *mask = new uchar[depth*width]; 
      for(qint64 h=minHSlice; h<=maxHSlice; h++)
	{
	  int slc = h-minHSlice;
	  progress.setValue((int)(100*(float)slc/(float)theight));
	  qApp->processEvents();
	  
	  memset(mask, 0, depth*width);
	  m_coronalCurves->paintUsingCurves(2, h, width, depth, mask);
	  for(qint64 d=minDSlice; d<=maxDSlice; d++)
	    for(qint64 w=minWSlice; w<=maxWSlice; w++)
	      {
		if (mask[d*width+w] > 0)
		  curveMask[(d-minDSlice)*twidth*theight +
			    (w-minWSlice)*theight +
			    (h-minHSlice)] = mask[d*width+w];
	      }
	}
      delete [] mask;
    }

  progress.setValue(100);
}


void
DrishtiPaint::dilateAndSmooth(uchar* data,
			      int d, int w, int h,
			      int spread)
{
  // now apply final smoothing
  smoothData(data, d, w, h, qMax(1,spread-1));

 // set boundary
  // x=0
  for(int j=0; j<w; j++)
    for(int k=0; k<h; k++)
      data[j*h + k] = 255;

  // x=d-1
  for(int j=0; j<w; j++)
    for(int k=0; k<h; k++)
      data[(d-1)*w*h + j*h + k] = 255;

  // y=0
  for(int i=0; i<d; i++)
    for(int k=0; k<h; k++)
      data[i*w*h + k] = 255;

  // y=w-1
  for(int i=0; i<d; i++)
    for(int k=0; k<h; k++)
      data[i*w*h + (w-1)*h + k] = 255;

  // z=0
  for(int i=0; i<d; i++)
    for(int j=0; j<w; j++)
      data[i*w*h + j*h] = 255;

  // z=h-1
  for(int i=0; i<d; i++)
    for(int j=0; j<w; j++)
      data[i*w*h + j*h + (h-1)] = 255;
}

void
DrishtiPaint::smoothData(uchar *gData,
			 int nX, int nY, int nZ,
			 int spread)
{
  // ------------------
  // calculate weights for Gaussian filter
  float weights[100];
  float wsum = 0.0;
  for(int i=-spread; i<=spread; i++)
    {
      float wgt = qExp(-qAbs(i)/(2.0*spread*spread))/(M_PI*2*spread*spread);
      wsum +=  wgt;
      weights[i+spread] = wgt;
    }
  //weights[2*spread+2] = wsum;
  // ------------------

  QProgressDialog progress("Applying gaussian smoothing before meshing",
			   QString(),
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);

  uchar *tmp = new uchar[qMax(nX, qMax(nY, nZ))];

  for(int k=0; k<nX; k++)
    {
      progress.setValue((int)(100.0*(float)k/(float)(nX)));
      qApp->processEvents();

      for(int j=0; j<nY; j++)
	{
	  memset(tmp, 0, nZ);
	  for(int i=0; i<nZ; i++)
	    {
	      float v = 0.0f;
	      int nt = 0;
	      for(int i0=qMax(0,i-spread); i0<=qMin(nZ-1,i+spread); i0++)
		{
		  v += gData[k*nY*nZ + j*nZ + i0] * weights[nt];
		  nt++;
		}
	      tmp[i] = v/wsum;
	    }
	  
	  for(int i=0; i<nZ; i++)
	    gData[k*nY*nZ + j*nZ + i] = tmp[i];
	}
    }


  for(int k=0; k<nX; k++)
    {
      progress.setValue((int)(100.0*(float)k/(float)(nX)));
      qApp->processEvents();

      for(int i=0; i<nZ; i++)
	{
	  memset(tmp, 0, nY);
	  for(int j=0; j<nY; j++)
	    {
	      float v = 0.0f;
	      int nt = 0;
	      for(int j0=qMax(0,j-spread); j0<=qMin(nY-1,j+spread); j0++)
		{
		  v += gData[k*nY*nZ + j0*nZ + i] * weights[nt];
		  nt++;
		}
	      tmp[j] = v/wsum;
	    }	  
	  for(int j=0; j<nY; j++)
	    gData[k*nY*nZ + j*nZ + i] = tmp[j];
	}
    }
  
  for(int j=0; j<nY; j++)
    {
      progress.setValue((int)(100.0*(float)j/(float)(nY)));
      qApp->processEvents();

      for(int i=0; i<nZ; i++)
	{
	  memset(tmp, 0, nX);
	  for(int k=0; k<nX; k++)
	    {
	      float v = 0.0f;
	      int nt = 0;
	      for(int k0=qMax(0,k-spread); k0<=qMin(nX-1,k+spread); k0++)
		{
		  v += gData[k0*nY*nZ + j*nZ + i] * weights[nt];
		  nt++;
		}
	      tmp[k] = v/wsum;
	    }
	  for(int k=0; k<nX; k++)
	    gData[k*nY*nZ + j*nZ + i] = tmp[k];
	}
    }
  
  delete [] tmp;

  progress.setValue(100);
}

void
DrishtiPaint::on_actionMeshTag_triggered()
{
  // get mesh file name  
  QString pvlFilename = m_volume->fileName();
  QString tflnm = QFileDialog::getSaveFileName(0,
					       "Save mesh",
					       QFileInfo(pvlFilename).absolutePath(),
					       "*.ply ;; *.obj ;; *.stl");
					       //QFileDialog::DontUseNativeDialog);
  
  if (tflnm.isEmpty())
    return;

  if (!StaticFunctions::checkExtension(tflnm, ".ply") &&
      !StaticFunctions::checkExtension(tflnm, ".obj") &&
      !StaticFunctions::checkExtension(tflnm, ".stl"))
    tflnm += ".ply";


  QStringList dtypes;
  QList<int> tag;

  bool ok;
  QString tagstr = QInputDialog::getText(0, "Save Mesh for Label",
	    "Label Numbers\nlabels should be separated by space.\n-2 mesh whatever is visible.\n-1 mesh all labeled region.\n 0 mesh region that is not labeled.\n 1-254 for consecutive labels.\nFor e.g. 1 2 5 will mesh region labeled with labels 1, 2 and 5)",
					 QLineEdit::Normal,
					 "-2",
					 &ok);
  tag.clear();
  if (ok && !tagstr.isEmpty())
    {
      QStringList tglist = tagstr.split(" ", QString::SkipEmptyParts);
      for(int i=0; i<tglist.count(); i++)
	{
	  QStringList parts = tglist[i].split("-", QString::SkipEmptyParts);
	  if (parts.count() == 2)
	    {
	      int tag1 = parts[0].toInt();
	      int tag2 = parts[1].toInt();
	      for(int t=tag1; t<=tag2; t++)
		tag << t;
	    }
	  else
	    {
	      int t = tglist[i].toInt();
	      if (t == -2)
		{
		  tag.clear();
		  tag << -2;
		  break;
		}
	      else if (t == -1)
		{
		  tag.clear();
		  tag << -1;
		  break;
		}
	      else if (t == 0)
		{
		  tag.clear();
		  tag << 0;
		  break;
		}
	      else
		tag << t;
	    }
	}
    }
  else
    tag << -2;  
  //----------------

  //----------------
  // check whether user wants to save each label in its own separate mesh file.
  bool saveIndividual = false;
  QList<int> labels = tag;
  if (tag.count() > 1)
    {
      dtypes << "No"
	     << "Yes";
      
      QString option = QInputDialog::getItem(0,
					     "Save Mesh",
					     "Save each label independently.\nEach label will be saved in a different mesh.",
					     dtypes,
					     0,
					     false,
					     &ok);
      if (!ok)
	return;
      
      if (option == "Yes") saveIndividual = true;
    }
  //----------------
  
  
  //----------------
  int colorType = 1; // apply tag colors 
  Vec userColor = Vec(255, 255, 255);


  // ask for colour only while saving PLY files
  if (tflnm.right(3).toLower() == "ply")
    {
      dtypes.clear();
      if (tag[0] == -2)
	{
	  dtypes << "User Color"
		 << "Label Color"
		 << "Transfer Function"
		 << "Label Color + Transfer Function";
	  
	  QString option = QInputDialog::getItem(0,
						 "Mesh Color",
						 "Color Mesh with",
						 dtypes,
						 0,
						 false,
						 &ok);
	  if (!ok)
	    return;
      
	  colorType = 0;
	  if (option == "Label Color") colorType = 1;
	  else if (option == "Label Color + Transfer Function") colorType = 3;
	  else if (option == "Transfer Function") colorType = 5;
	}

      if (tag[0] > -2)
	{
	  dtypes << "Label Color"
		 << "Transfer Function"
		 << "Label Color + Transfer Function"
		 << "Label Mask + Transfer Function"
		 << "User Color";

	  QString option = QInputDialog::getItem(0,
						 "Mesh Color",
						 "Color Mesh with",
						 dtypes,
						 0,
						 false,
						 &ok);
	  if (!ok)
	    return;
      
	  if (option == "Label Color") colorType = 1;
	  else if (option == "Transfer Function") colorType = 2;
	  else if (option == "Label Color + Transfer Function") colorType = 3;
	  else if (option == "Label Mask + Transfer Function") colorType = 4;
	  else colorType = 0;
	}
      
      if (colorType == 0 || colorType == 4)
	{
	  QColor clr = QColor(255, 255, 255);
	  clr = DColorDialog::getColor(clr);
	  if (clr.isValid())
	    userColor = Vec(clr.red(), clr.green(), clr.blue());
	}
    }
  //----------------



  //----------------
  int depth, width, height;
  m_volume->gridSize(depth, width, height);
  
  int minDSlice, maxDSlice;
  int minWSlice, maxWSlice;
  int minHSlice, maxHSlice;
  m_viewer->getBox(minDSlice, maxDSlice,
		   minWSlice, maxWSlice,
		   minHSlice, maxHSlice);
  qint64 tdepth = maxDSlice-minDSlice+1;
  qint64 twidth = maxWSlice-minWSlice+1;
  qint64 theight = maxHSlice-minHSlice+1;



  float isoValue, adaptivity, resample;
  int dataSmooth, meshSmooth;
  bool applyVoxelScaling;
  int morphoType;
  int morphoRadius;
  
  getValues(isoValue,
	    adaptivity,
	    resample,
	    morphoType, morphoRadius,
	    dataSmooth,
	    meshSmooth,
	    applyVoxelScaling);

  
  QProgressDialog progress("Meshing tagged region from volume data",
			   "",
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);

  //----------------------------------
  ushort *curveMask;
  curveMask = new ushort[tdepth*twidth*theight];
  memset(curveMask, 0, 2*tdepth*twidth*theight);
  
  uchar *lut = Global::lut();

  QList<Vec> cPos =  m_viewer->clipPos();
  QList<Vec> cNorm = m_viewer->clipNorm();
  

  int nbytes = width*height*2;
  uchar *raw = new uchar[nbytes];
  ushort *mask = new ushort[nbytes];

  int startLabel = 0;
  int endLabel = 1;
  if (saveIndividual)
    endLabel = labels.count();
  QString labelflnm = tflnm;
  QString extension = tflnm.right(4);
  for(int lbl=startLabel; lbl<endLabel; lbl++)
    {
      VdbVolume vdb;
      if (saveIndividual)
	{
	  tag.clear();
	  tag << labels[lbl];
	  
	  tflnm = labelflnm.chopped(4) + QString("_%1").arg(tag[0],4,10,QChar('0')) + extension;
	}
      
      for(int dsn=0; dsn<tdepth; dsn++)
	{
	  int d = minDSlice + dsn;
	  int slc = dsn;
	  
	  //int slc = d-minDSlice;
	  progress.setValue((int)(100*(float)slc/(float)tdepth));
	  qApp->processEvents();
	  
	  uchar *slice = 0;
	  if (tag[0] == -2 || colorType == 4) // using tag mask + transfer function to extract mesh
	    {
	      slice = m_volume->getDepthSliceImage(d);
	      int i=0;
	      for(int w=minWSlice; w<=maxWSlice; w++)
		for(int h=minHSlice; h<=maxHSlice; h++)
		  {
		    if (Global::bytesPerVoxel() == 1)
		      slice[i] = slice[(w*height+h)];
		    else
		      ((ushort*)slice)[i] = ((ushort*)slice)[(w*height+h)];
		    i++;
		  }
	    }
	  
	  if (tag[0] == -2)
	    {
	      memset(raw, 0, width*height);
	      
	      memcpy(mask, m_volume->getMaskDepthSliceImage(d), nbytes);
	      int i=0;
	      for(int w=minWSlice; w<=maxWSlice; w++)
		for(int h=minHSlice; h<=maxHSlice; h++)
		  {
		    mask[i] = mask[w*height+h];
		    i++;
		  }
	      memcpy(curveMask+slc*twidth*theight, mask, 2*twidth*theight);
	    }
	  else
	    {
	      memcpy(mask, m_volume->getMaskDepthSliceImage(d), nbytes);
	      
	      if (tag[0] == -1)
		{
		  for(int w=minWSlice; w<=maxWSlice; w++)
		    for(int h=minHSlice; h<=maxHSlice; h++)
		      raw[w*height+h] = (mask[w*height+h] > 0 ? 0 : 255);
		}
	      else if (tag[0] == 0)
		{
		  for(int w=minWSlice; w<=maxWSlice; w++)
		    for(int h=minHSlice; h<=maxHSlice; h++)
		      raw[w*height+h] = (mask[w*height+h] > 0 ? 255 : 0);
		}
	      else
		{
		  for(int w=minWSlice; w<=maxWSlice; w++)
		    for(int h=minHSlice; h<=maxHSlice; h++)
		      raw[w*height+h] = (tag.contains(mask[w*height+h]) ? 0 : 255);
		}
	      
	      //-----------------------------
	      // update curveMask using painted mask data
	      if (tag[0] == -1 || tag[0] == 0)
		{
		  for(int w=minWSlice; w<=maxWSlice; w++)
		    for(int h=minHSlice; h<=maxHSlice; h++)
		      {	    
			if (curveMask[slc*twidth*theight +
				      (w-minWSlice)*theight +
				      (h-minHSlice)] == 0)
			  curveMask[slc*twidth*theight +
				    (w-minWSlice)*theight +
				    (h-minHSlice)] = mask[w*height+h];	    
		      }
		}
	      else // only copy selected tags
		{
		  for(int w=minWSlice; w<=maxWSlice; w++)
		    for(int h=minHSlice; h<=maxHSlice; h++)
		      {	    
			if (curveMask[slc*twidth*theight +
				      (w-minWSlice)*theight +
				      (h-minHSlice)] == 0 &&
			    tag.contains(mask[w*height+h]))
			  curveMask[slc*twidth*theight +
				    (w-minWSlice)*theight +
				    (h-minHSlice)] = mask[w*height+h];	    
		      }
		}
	      //-----------------------------
	    }
	  
	  int i=0;
	  for(int w=minWSlice; w<=maxWSlice; w++)
	    for(int h=minHSlice; h<=maxHSlice; h++)
	      {
		raw[i] = raw[w*height+h];
		i++;
	      }
	  
	  //-----------------------------
	  // use tag mask + transfer function to generate mesh
	  if (tag[0] == -2 || colorType == 4)
	    {
	      int i=0;
	      for(int w=minWSlice; w<=maxWSlice; w++)
		for(int h=minHSlice; h<=maxHSlice; h++)
		  {
		    bool clipped = VolumeOperations::checkClipped(Vec(h,w,d));
		    
		    if (!clipped)
		      {
			if (Global::bytesPerVoxel() == 1)
			  {
			    if (raw[i] != 255 &&
				lut[4*slice[i]+3] < 5)
			      raw[i] = 255;
			  }
			else
			  {
			    if (raw[i] != 255 &&
				lut[4*((ushort*)slice)[i]+3] < 5)
			      raw[i] = 255;
			  }
		      }
		    else
		      raw[i] = 255;
		    
		    i++;
		  }
	    }
	  if (tag[0] == -2)
	    {
	      int i=0;
	      for(int w=minWSlice; w<=maxWSlice; w++)
		for(int h=minHSlice; h<=maxHSlice; h++)
		  {
		    bool clipped = VolumeOperations::checkClipped(Vec(h,w,d));
		    
		    if (!clipped)
		      {
			if (lut[4*slice[i]+3]*Global::tagColors()[4*mask[i]+3] == 0)
			  raw[i] = 255;
		      }
		    else
		      raw[i] = 255;
		    
		    i++;
		  }
	    }
	  //-----------------------------
	  
	  // flip 0 and 255 so that isosurface is generated properly
	  for(int i=0; i<twidth*theight; i++)
	    raw[i] = ~raw[i];
	  
	  vdb.addSliceToVDB(raw,
			    slc, twidth, theight,
			    -1, 1);  // values less than 1 are background
	  
	}

      if (!saveIndividual)
	{
	  delete [] raw;
	  delete [] mask;
	}
      
      progress.setValue(90);  
      
      
      //==========================================================================
      
      // convert to levelset
      vdb.convertToLevelSet(isoValue);
      
      // Apply Morphological Operations
      if (morphoType > 0 && morphoRadius > 0)
	{
	  float offset = morphoRadius;
	  if (morphoType == 1)
	    {
	      progress.setLabelText("Applying Morphological Dilation");
	      vdb.offset(-offset); // dilate
	    }
	  if (morphoType == 2)
	    {
	      progress.setLabelText("Applying Morphological Erosion");
	      vdb.offset(offset); // erode
	    }
	  if (morphoType == 3)
	    {
	      progress.setLabelText("Applying Morphological Closing");
	      vdb.close(-offset, offset);
	      //vdb.offset(-offset); // dilate
	      //vdb.offset(offset); // erode
	    }
	  if (morphoType == 4)
	    {
	      progress.setLabelText("Applying Morphological Opening");
	      vdb.open(offset, -offset);
	      //vdb.offset(offset); // erode
	      //vdb.offset(-offset); // dilate
	    }
	  
	  progress.setValue(60);  
	  qApp->processEvents();      
	}
      
      // smoothing  
      if (dataSmooth > 0)
	{
	  progress.setLabelText("Smoothing volume before meshing");
	  progress.setValue(70);  
	  qApp->processEvents();      
	  
	  float gwd;
	  gwd = 1.0/(float)qMax(depth, qMax(width,height));
	  vdb.mean(gwd, dataSmooth); // width, iterations
	}
      
      // resample
      if (qAbs(resample-1.0) > 0.001)
	{
	  progress.setLabelText("Downsampling volume before meshing");
	  progress.setValue(80);  
	  qApp->processEvents();      
	  
	  vdb.resample(resample);
	}
      
      
      progress.setLabelText("Generating Mesh");
      progress.setValue(90);  
      qApp->processEvents();      
      
      // extract surface mesh
      QVector<QVector3D> V;
      QVector<QVector3D> N;
      QVector<QVector3D> C;
      QVector<int> T;
      
      // generating isosurface from level set therefore using 0 as isoValue
      // isoValue < 0.5 result in dilated surface
      // isoValue > 0.5 result in eroded surface
      vdb.generateMesh(0, 3*(0.5-isoValue), adaptivity, V, N, T);
      
      
      // mesh smoothing
      if (meshSmooth > 0)
	MeshTools::smoothMesh(V, N, T, 5*meshSmooth);
      
      
      // if saving ply/obj apply color
      // for colorType 0 and 4 apply user defined color
      if (tflnm.right(3).toLower() == "ply" ||
	  tflnm.right(3).toLower() == "obj")
	{
	  C.resize(V.count());
	  C.fill(QVector3D(userColor.x, userColor.y, userColor.z));
	  if (colorType != 0 && colorType != 4)
	    colorMesh(C, V, N,
		      colorType, curveMask,
		      minHSlice, minWSlice, minDSlice,
		      theight, twidth, tdepth, meshSmooth,
		      resample);
	}
      
      
      // shift vertices
      {
	int minDSlice, maxDSlice;
	int minWSlice, maxWSlice;
	int minHSlice, maxHSlice;
	m_viewer->getBox(minDSlice, maxDSlice,
			 minWSlice, maxWSlice,
			 minHSlice, maxHSlice);
	
	for(int i=0; i<V.count(); i++)
	  V[i] += QVector3D(minHSlice, minWSlice, minDSlice);
      }
      
      
      //apply voxel size to vertices
      if (applyVoxelScaling)
	{
	  Vec vs = Global::voxelScaling();
	  QVector3D voxelScaling(vs.x, vs.y, vs.z);
	  for(int i=0; i<V.count(); i++)
	    V[i] *= voxelScaling;
	}
      
      
      // save mesh
      if (tflnm.right(3).toLower() == "obj")
	MeshTools::saveToOBJ(tflnm, V, N, C, T);
      else if (tflnm.right(3).toLower() == "ply")
	MeshTools::saveToPLY(tflnm, V, N, C, T);
      else if (tflnm.right(3).toLower() == "stl")
	MeshTools::saveToSTL(tflnm, V, N, T);
      
    } // End Label
  
  //QMessageBox::information(0, "Save", "-----Done-----");
  QMessageBox mb;
  mb.setWindowTitle("Save");
  mb.setText("-----Done-----");
  mb.setWindowFlags(Qt::Dialog|Qt::WindowStaysOnTopHint);
  mb.exec();
  

  if (saveIndividual)
    {
      delete [] raw;
      delete [] mask;
    }
  delete [] curveMask;

  
  if (m_meshViewer.state() == QProcess::Running)
    {
      QByteArray Data;
      Data.append(QString("load %1").arg(tflnm));
      m_meshViewerSocket->writeDatagram(Data, QHostAddress::LocalHost, 7760);
    }

  return;
//==========================================================================
}


void
DrishtiPaint::colorMesh(QVector<QVector3D>& C,
			QVector<QVector3D> V,
			QVector<QVector3D> N,
			int colorType,
			ushort *tagdata,
			int minHSlice, int minWSlice, int minDSlice,
			int theight, int twidth, int tdepth,
			int spread, int lod)
{
  uchar *lut = Global::lut();

  QProgressDialog progress("Colouring mesh",
			   QString(),
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);
  
  int bsz = spread+1;
  int nv = V.count();

  for(int ni=0; ni<V.count(); ni++)
    {
      if (ni%10000 == 0)
	{
	  progress.setValue((int)(100.0*(float)ni/(float)(nv)));
	  qApp->processEvents();
	}
      
      // get tag
      int h = qBound(0, (int)V[ni].x(), theight-1);
      int w = qBound(0, (int)V[ni].y(), twidth-1);
      int d = qBound(0, (int)V[ni].z(), tdepth-1);
      int tag = tagdata[d*twidth*theight + w*theight + h];

      if (tag == 0 &&
	  (colorType != 2 || colorType != 5)) // tag not needed apply transfer function
	{	  
	  for(int sp=1; sp<=bsz+1; sp++)
	    {
	      for(int dd=qMax(d-sp, 0); dd<=qMin(tdepth-1,d+sp); dd++)
		for(int ww=qMax(w-sp, 0); ww<=qMin(twidth-1,w+sp); ww++)
		  for(int hh=qMax(h-sp, 0); hh<=qMin(theight-1,h+sp); hh++)
		    if (qAbs(dd-d) == sp ||
			qAbs(ww-w) == sp ||
			qAbs(hh-h) == sp)
		      tag = qMax(tag, (int)tagdata[dd*twidth*theight +
						   ww*theight + hh]);
	      if (tag > 0)
		break;
	    }
	}
      
      // get color
      uchar r,g,b;
      if (colorType == 1) // apply tag colors
	{
	  r = Global::tagColors()[4*tag+0];
	  g = Global::tagColors()[4*tag+1];
	  b = Global::tagColors()[4*tag+2];
	}
      else if (colorType == 2 ||
	       colorType == 3 ||
	       colorType == 5) // apply transfer function
	{
	  QList<int> val;
	  int ncl = 0;
	  int vr,vg,vb;
	  vr = vg = vb = 0;
	  for(int sp=0; sp<=bsz; sp++)
	    {
	      Vec pt = Vec(h,w,d) - sp*Vec(N[ni].x(), N[ni].y(), N[ni].z());
	      int hh = qBound(0, (int)pt.x, theight-1);
	      int ww = qBound(0, (int)pt.y, twidth-1);
	      int dd = qBound(0, (int)pt.z, tdepth-1);
	      val = m_volume->rawValue(lod*dd+minDSlice,
				       lod*ww+minWSlice,
				       lod*hh+minHSlice);
	      int pr = lut[4*val[0]+2];
	      int pg = lut[4*val[0]+1];
	      int pb = lut[4*val[0]+0];
	      if (lut[4*val[0]+3] > 1)
		{
		  vr += pr;
		  vg += pg;
		  vb += pb;
		  ncl ++;
		}
	    }
	  if (ncl > 0)
	    {
	      r = vr/ncl;
	      g = vg/ncl;
	      b = vb/ncl;
	    }
	}

      if (colorType == 3) // merge tag and transfer function colors
	{
	  if (tag > 0)
	    {
	      r = Global::tagColors()[4*tag+0];
	      g = Global::tagColors()[4*tag+1];
	      b = Global::tagColors()[4*tag+2];
	    }
	}

      if (r == 0 && g == 0 && b == 0)
	r = g = b = 255;

      C[ni] = QVector3D(r,g,b);
    }

  progress.setValue(100);
}

void
DrishtiPaint::paint3DStart()
{
  m_prevSeed = Vec(-1,-1,-1);
  m_blockList.clear();
}

void
DrishtiPaint::paint3D(Vec bmin, Vec bmax,
		      int d, int w, int h,
		      int button, int otag, bool onlyConnected)
{
  // block the signals coming from viewer till we complete this process
  QSignalBlocker blockSignals(m_viewer);
  
  
  int m_depth, m_width, m_height;
  m_volume->gridSize(m_depth, m_width, m_height);

  if (d<0 || w<0 || h<0 ||
      d>m_depth-1 ||
      w>m_width-1 ||
      h>m_height-1)
    return;


  uchar *lut = Global::lut();
  int tag = otag;
  if (button == 2) // right button
    tag = 0;

  Vec viewD = m_viewer->camera()->viewDirection();
  Vec viewR = m_viewer->camera()->rightVector();
  Vec viewU = m_viewer->camera()->upVector();
  Vec camPos = m_viewer->camera()->position();
  bool persp = m_viewer->perspectiveProjection();

  int vrad1 = viewerUi.radiusSurface->value();
  int vrad2 = viewerUi.radiusDepth->value();
  int rad0 = qMax(vrad1, vrad2);


  int minDSlice, maxDSlice;
  int minWSlice, maxWSlice;
  int minHSlice, maxHSlice;

  minDSlice = bmin.z;
  minWSlice = bmin.y;
  minHSlice = bmin.x;

  maxDSlice = bmax.z;
  maxWSlice = bmax.y;
  maxHSlice = bmax.x;

  uchar *volData = m_volume->memVolDataPtr();
  ushort *volDataUS = 0;
  if (Global::bytesPerVoxel() == 2)
    volDataUS = (ushort*)volData;
  
  ushort *maskDataUS = m_volume->memMaskDataPtrUS();
  

  //----------------
  QList<Vec> seeds;
  if (m_prevSeed.x > -1)
    {
      float dpt0 = (m_prevSeed-camPos)*viewD;
      float dpt1 = (Vec(h,w,d)-camPos)*viewD;
      if (qAbs(dpt0-dpt1) > 50) // if the depth difference is too much just ignore
	return;
      
      seeds = StaticFunctions::line3d(m_prevSeed, Vec(h,w,d));
      seeds.removeFirst(); // remove prevSeed from the list;
    }
  else
    seeds << Vec(h,w,d);

  m_prevSeed = Vec(h,w,d);  

  if (seeds.count() == 0)
    return;
  //----------------

  
  int ds, de, ws, we, hs, he;
  ds = d;
  ws = w;
  hs = h;
  de = d;
  we = w;
  he = h;
  for(int i=0; i<seeds.count(); i++)
    {
      int h0 = seeds[i].x;
      int w0 = seeds[i].y;
      int d0 = seeds[i].z;
      ds = qMin(ds, d0-rad0);
      ws = qMin(ws, w0-rad0);
      hs = qMin(hs, h0-rad0);
      de = qMax(de, d0+rad0);
      we = qMax(we, w0+rad0);
      he = qMax(he, h0+rad0);
    }  
  ds = qMax(minDSlice, ds);
  de = qMin(maxDSlice, de);
  ws = qMax(minWSlice, ws);
  we = qMin(maxWSlice, we);
  hs = qMax(minHSlice, hs);
  he = qMin(maxHSlice, he);
  
  qint64 dm = qMax(de-ds+1, qMax(we-ws+1, he-hs+1));;
  qint64 dm2 = dm*dm;


  MyBitArray bitmask;
  bitmask.resize(dm*dm*dm);
  bitmask.fill(false);

  float minGrad = m_viewer->minGrad();
  float maxGrad = m_viewer->maxGrad();
  int gradType = m_viewer->gradType();

  
  for(int i=0; i<seeds.count(); i++)
    {
      int h0 = seeds[i].x;
      int w0 = seeds[i].y;
      int d0 = seeds[i].z;
      int ds0 = qMax(minDSlice, d0-rad0);
      int de0 = qMin(maxDSlice, d0+rad0);
      int ws0 = qMax(minWSlice, w0-rad0);
      int we0 = qMin(maxWSlice, w0+rad0);
      int hs0 = qMax(minHSlice, h0-rad0);
      int he0 = qMin(maxHSlice, h0+rad0);      
      
      float gradMag = 0.5;

      if (minGrad > 0 || maxGrad < 1)
	{
	  gradMag = VolumeOperations::calcGrad(gradType, d0, w0, h0,
					       m_depth, m_width, m_height,
					       volData, volDataUS);
	  
	  gradMag = qBound(0.0f, gradMag, 1.0f);
	}

      //--------
      if (gradMag >= minGrad && gradMag <= maxGrad)
	{
	  for(qint64 dd=ds0; dd<=de0; dd++)
	    for(qint64 ww=ws0; ww<=we0; ww++)
	      for(qint64 hh=hs0; hh<=he0; hh++)
		{
		  Vec v0 = Vec(hh-h0,ww-w0,dd-d0);
		  //Vec dv = viewD;
		  //if (persp)
		  Vec dv = Vec(hh-camPos.x, ww-camPos.y, dd-camPos.z).unit();
		  
		  float pr = v0*viewR;
		  float pu = v0*viewU;
		  //float pvd= v0*viewD;
		  float pvd = v0*dv;
		  if (qAbs(pvd) <= vrad2 && qSqrt(pr*pr +pu*pu) <= vrad1)
		    {
		      int val;
		      if (!volDataUS)
			val = volData[dd*m_width*m_height + ww*m_height + hh];
		      else
			val = volDataUS[dd*m_width*m_height + ww*m_height + hh];

		      int mtag = maskDataUS[dd*m_width*m_height + ww*m_height + hh];

		      bool opaque =  (lut[4*val+3]*Global::tagColors()[4*mtag+3] > 0);

		      //-----------
		      if (opaque &&
			  (minGrad > 0 || maxGrad < 1))
			{
			  float gradMag = VolumeOperations::calcGrad(gradType, dd, ww, hh,
								     m_depth, m_width, m_height,
								     volData, volDataUS);
            			  
			  gradMag = qBound(0.0f, gradMag, 1.0f);
			  if (gradMag < minGrad || gradMag > maxGrad)
			    opaque = false;

			} // if (opaque)
		      //-----------

		      
		      if (opaque)
			bitmask.setBit((dd-ds)*dm2 + (ww-ws)*dm + (hh-hs), true);	      
		    }
		}
	}
    } // seeds

  int minD,maxD, minW,maxW, minH,maxH;
  minD = maxD = d;
  minW = maxW = w;
  minH = maxH = h;

  if (!onlyConnected)
    {
      for(qint64 dd=ds; dd<=de; dd++)
	for(qint64 ww=ws; ww<=we; ww++)
	  for(qint64 hh=hs; hh<=he; hh++)
	    {
	      if (bitmask.testBit((dd-ds)*dm2 + (ww-ws)*dm + (hh-hs)))
		{
		  maskDataUS[dd*m_width*m_height + ww*m_height + hh] = tag;

		  minD = qMin(minD, (int)dd);
		  maxD = qMax(maxD, (int)dd);
		  minW = qMin(minW, (int)ww);
		  maxW = qMax(maxW, (int)ww);
		  minH = qMin(minH, (int)hh);
		  maxH = qMax(maxH, (int)hh);
		}
	    }
    }
  else
    {      
      int indices[] = {-1, 0, 0,
		        1, 0, 0,
		        0,-1, 0,
		        0, 1, 0,
		        0, 0,-1,
		        0, 0, 1};
      
      // tag only connected region
      
      QStack<Vec> stack;
      
      for(int i=0; i<seeds.count(); i++)
	{
	  qint64 h0 = seeds[i].x;
	  qint64 w0 = seeds[i].y;
	  qint64 d0 = seeds[i].z;

	  maskDataUS[d0*m_width*m_height + w0*m_height + h0] = tag;

	  bitmask.setBit((d0-ds)*dm2 + (w0-ws)*dm + (h0-hs), false);
	  stack.push(Vec(d0, w0, h0));
	}
      
      while(!stack.isEmpty())
	{
	  Vec dwh = stack.pop();
	  int dx = dwh.x;
	  int wx = dwh.y;
	  int hx = dwh.z;
	  
	  for(int i=0; i<6; i++)
	    {
	      int da = indices[3*i+0];
	      int wa = indices[3*i+1];
	      int ha = indices[3*i+2];
	      
	      qint64 d2 = qBound(ds, dx+da, de);
	      qint64 w2 = qBound(ws, wx+wa, we);
	      qint64 h2 = qBound(hs, hx+ha, he);
	      
	      qint64 bidx = (d2-ds)*dm2+(w2-ws)*dm+(h2-hs);
	      if (bitmask.testBit(bidx))
		{
		  bitmask.setBit(bidx, false);

		  maskDataUS[d2*m_width*m_height + w2*m_height + h2] = tag;

		  stack.push(Vec(d2,w2,h2));
		  
		  minD = qMin(minD, (int)d2);
		  maxD = qMax(maxD, (int)d2);
		  minW = qMin(minW, (int)w2);
		  maxW = qMax(maxW, (int)w2);
		  minH = qMin(minH, (int)h2);
		  maxH = qMax(maxH, (int)h2);
		}
	    }
	}
    } // onlyConnected

  reloadSlices();

  for(int i=0; i<seeds.count(); i++)
    {
      int h0 = seeds[i].x;
      int w0 = seeds[i].y;
      int d0 = seeds[i].z;
      QList<int> dwhr;
      dwhr << d0 << w0 << h0 << rad0;
      m_blockList << dwhr;
    }

  minD = qMax(minD-1, 0);
  minW = qMax(minW-1, 0);
  minH = qMax(minH-1, 0);
  maxD = qMin(maxD+1, m_depth-1);
  maxW = qMin(maxW+1, m_width-1);
  maxH = qMin(maxH+1, m_height-1);
  m_viewer->uploadMask(minD,minW,minH, maxD,maxW,maxH);
}

void
DrishtiPaint::paint3DEnd()
{
  if (m_blockList.count() == 0)
    return;

  m_volume->saveMaskBlock(m_blockList);
  m_blockList.clear();
}


void
DrishtiPaint::tagUsingSketchPad(Vec bmin, Vec bmax)
{
  // use smaller blocks for tagging
  // so that we don't overflow
  int xmin = bmin.x;
  int xmax = bmax.x;
  int ymin = bmin.y;
  int ymax = bmax.y;
  int zmin = bmin.z;
  int zmax = bmax.z;
  int xstep = 500;
  int ystep = 500;
  int zstep = 500;

  bool found = false;
  for(int x=xmin; x<xmax; x+=xstep)
  for(int y=ymin; y<ymax; y+=ystep)
  for(int z=zmin; z<zmax; z+=zstep)
    {
      int x0 = qMax(x-1, 0);
      int y0 = qMax(y-1, 0);
      int z0 = qMax(z-1, 0);
      int x1 = qMin(x+xstep, xmax);
      int y1 = qMin(y+ystep, ymax);
      int z1 = qMin(z+zstep, zmax);

      found |= tagUsingSketchPad(Vec(x0,y0,z0), Vec(x1,y1,z1), Global::tag());
    }

  if (!found)
    QMessageBox::information(0, "", "No painted region found");
}

bool
DrishtiPaint::tagUsingSketchPad(Vec bmin, Vec bmax, int tag)
{
  int m_depth, m_width, m_height;
  m_volume->gridSize(m_depth, m_width, m_height);

  int spH = m_viewer->size().height();
  int spW = m_viewer->size().width();
  uchar *sketchPad = m_viewer->sketchPad();


  QProgressDialog progress("Updating voxel structure",
			   QString(),
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);
  progress.setLabelText("Locating initial seed");

  uchar *lut = Global::lut();

  int ds = bmin.z;
  int ws = bmin.y;
  int hs = bmin.x;
  
  int de = bmax.z;
  int we = bmax.y;
  int he = bmax.x;

  qint64 mx = he-hs+1;
  qint64 my = we-ws+1;
  qint64 mz = de-ds+1;

  MyBitArray bitmask;
  bitmask.resize(mx*my*mz);
  bitmask.fill(false);

  uchar *volData = m_volume->memVolDataPtr();
  ushort *volDataUS = 0;
  if (Global::bytesPerVoxel() == 2)
    volDataUS = m_volume->memVolDataPtrUS();

  ushort *maskDataUS = m_volume->memMaskDataPtrUS();

  int indices[] = {-1, 0, 0,
		    1, 0, 0,
		    0,-1, 0,
		    0, 1, 0,
		    0, 0,-1,
		    0, 0, 1};

  qint64 dr,wr,hr;

  Vec RvoxelScaling = Global::relativeVoxelScaling();

  //-----------------------------
  // get first voxel to start region growing
  // just take random samples to find it out
  bool found = false;
  for(int strd=0; strd<10 && !found; strd++)
    {
      for(int d=ds+qrand()%10; d<=de && !found; d+=10)
	{
	  progress.setValue(90*(d-ds)/((de-ds+1)));
	  if (d%10 == 0)
	    qApp->processEvents();
	  for(int w=ws+qrand()%10; w<=we && !found; w+=10)
	  for(int h=hs+qrand()%10; h<=he && !found; h+=10)
	    {
	      int d2 = qBound(ds, d, de);
	      int w2 = qBound(ws, w, we);
	      int h2 = qBound(hs, h, he);

	      Vec vpt = Vec(h2,w2,d2);
	      vpt = VECPRODUCT(vpt, RvoxelScaling);
	      Vec scr = m_viewer->camera()->projectedCoordinatesOf(vpt);
	      if (scr.x >= 0 && scr.x < spW &&
		  scr.y >= 0 && scr.y < spH)
		{
		  if (sketchPad[int(scr.x)+int(scr.y)*spW] > 0)
		    {
		      dr = d2;
		      wr = w2;
		      hr = h2;
		      found = true;
		      break;
		    }
		}
	    }
	}
    }
  //-----------------------------

  if (!found)
    return false;
  
  
  float minGrad = m_viewer->minGrad();
  float maxGrad = m_viewer->maxGrad();
  int gradType = m_viewer->gradType();


  progress.setLabelText("Region growing");
  qApp->processEvents();

  QStack<Vec> stack;
  stack.push(Vec(dr,wr,hr));

  qint64 idx = ((qint64)dr)*m_width*m_height + (qint64)wr*m_height + hr;
  maskDataUS[idx] = tag;

  qint64 bidx = (dr-ds)*mx*my+(wr-ws)*mx+(hr-hs);
  bitmask.setBit(bidx);

  int minD,maxD, minW,maxW, minH,maxH;
  minD = maxD = dr;
  minW = maxW = wr;
  minH = maxH = hr;

  int pgstep = 10*m_width*m_height;
  int prevpgv = 0;
  int ip=0;
  while(!stack.isEmpty())
    {
      ip = (ip+1)%pgstep;
      int pgval = 99*(float)ip/(float)pgstep;
      progress.setValue(pgval);
      if (pgval != prevpgv)
	qApp->processEvents();
      prevpgv = pgval;
      
      Vec dwh = stack.pop();
      int dx = qCeil(dwh.x);
      int wx = qCeil(dwh.y);
      int hx = qCeil(dwh.z);

      for(int i=0; i<6; i++)
	{
	  int da = indices[3*i+0];
	  int wa = indices[3*i+1];
	  int ha = indices[3*i+2];

	  qint64 d2 = qBound(ds, dx+da, de);
	  qint64 w2 = qBound(ws, wx+wa, we);
	  qint64 h2 = qBound(hs, hx+ha, he);

	  qint64 bidx = (d2-ds)*mx*my+(w2-ws)*mx+(h2-hs);
	  if (bitmask.testBit(bidx) == false)
	    {
	      bitmask.setBit(bidx, true);

	      Vec vpt = Vec(h2,w2,d2);
	      vpt = VECPRODUCT(vpt, RvoxelScaling);
	      Vec scr = m_viewer->camera()->projectedCoordinatesOf(vpt);
	      if (scr.x >= 0 && scr.x < spW &&
		  scr.y >= 0 && scr.y < spH)
		{
		  if (sketchPad[int(scr.x)+int(scr.y)*spW] > 0)
		    {
		      stack.push(Vec(d2,w2,h2));
		      
		      qint64 idx = d2*m_width*m_height + w2*m_height + h2;
		      int val = volData[idx];
		      if (volDataUS) val = volDataUS[idx];

		      int mtag = maskDataUS[idx];
			
		      bool visible = (lut[4*val+3] > 0) && (Global::tagColors()[4*mtag+3] > 0);

		      //-----------
		      if (visible &&
			  (minGrad > 0.0 || maxGrad < 1.0))
			{
			  float gradMag = VolumeOperations::calcGrad(gradType, d2, w2, h2,
								     m_depth, m_width, m_height,
								     volData, volDataUS);

			  gradMag = qBound(0.0f, gradMag, 1.0f);
			  if (gradMag < minGrad || gradMag > maxGrad)
			    visible = false;
			}
		      //-----------

		      if (visible) // tag only visible region
			{
			  maskDataUS[idx] = tag;
			  
			  minD = qMin(minD, (int)d2);
			  maxD = qMax(maxD, (int)d2);
			  minW = qMin(minW, (int)w2);
			  maxW = qMax(maxW, (int)w2);
			  minH = qMin(minH, (int)h2);
			  maxH = qMax(maxH, (int)h2);
			}
		    }
		}
	    }
	}
    }

  reloadSlices();
  
  minD = qMax(minD-1, 0);
  minW = qMax(minW-1, 0);
  minH = qMax(minH-1, 0);
  maxD = qMin(maxD+1, m_depth);
  maxW = qMin(maxW+1, m_width);
  maxH = qMin(maxH+1, m_height);
  
  progress.setLabelText("Update modified region on gpu");
  qApp->processEvents();
  m_viewer->uploadMask(minD,minW,minH, maxD,maxW,maxH);

  QList<int> dwh;  
  m_blockList.clear();  
  dwh << minD << minW << minH;
  m_blockList << dwh;
  dwh.clear();
  dwh << maxD << maxW << maxH;
  m_blockList << dwh;

  progress.setLabelText("Save modified region to mask file");
  qApp->processEvents();
  m_volume->saveMaskBlock(m_blockList);

  progress.setValue(100);
 
  m_viewer->showSketchPad(false);
  qApp->processEvents();
  m_viewer->showSketchPad(true);
  qApp->processEvents();

  return true;
}

void
DrishtiPaint::updateModifiedRegion(int minD, int maxD,
				   int minW, int maxW,
				   int minH, int maxH)
{
  QProgressDialog progress("Update modified region in 2D slice viewer",
			   QString(),
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);

  reloadSlices();
  
  int m_depth, m_width, m_height;
  m_volume->gridSize(m_depth, m_width, m_height);

  minD = qMax(minD-1, 0);
  minW = qMax(minW-1, 0);
  minH = qMax(minH-1, 0);
  maxD = qMin(maxD+1, m_depth);
  maxW = qMin(maxW+1, m_width);
  maxH = qMin(maxH+1, m_height);
    
  progress.setLabelText("Update modified region on gpu");
  qApp->processEvents();
  m_viewer->uploadMask(minD,minW,minH, maxD,maxW,maxH);

  QList<int> dwh;  
  m_blockList.clear();  
  dwh << minD << minW << minH;
  m_blockList << dwh;
  dwh.clear();
  dwh << maxD << maxW << maxH;
  m_blockList << dwh;

  progress.setLabelText("Save modified region to mask file");
  qApp->processEvents();
  m_volume->saveMaskBlock(m_blockList);

  progress.setValue(100);
}

void
DrishtiPaint::shrinkwrap(Vec bmin, Vec bmax, int tag,
			 bool shellOnly, int shellThickness)
{
  shrinkwrap(bmin, bmax, tag, shellOnly, shellThickness,
	     true, 0, 0, 0, 0);  // shrinkwrap all
}

void
DrishtiPaint::shrinkwrap(Vec bmin, Vec bmax, int tag,
			 bool shellOnly, int shellThickness,
			 bool all,
			 int dr, int wr, int hr, int ctag)
{
  int minD,maxD, minW,maxW, minH,maxH;

  QList<Vec> cPos =  m_viewer->clipPos();
  QList<Vec> cNorm = m_viewer->clipNorm();
  
  float minGrad = m_viewer->minGrad();
  float maxGrad = m_viewer->maxGrad();
  int gradType = m_viewer->gradType();
  
  VolumeOperations::setClip(cPos, cNorm);
  VolumeOperations::shrinkwrap(bmin, bmax,
			       tag,
			       shellOnly,
			       shellThickness,
			       all, dr, wr, hr, ctag,
			       minD, maxD,
			       minW, maxW,
			       minH, maxH,
			       gradType, minGrad, maxGrad);

  if (minD < 0)
    return;

  updateModifiedRegion(minD, maxD,
		       minW, maxW,
		       minH, maxH);

  QMessageBox::information(0, "", "Shrinkwrap done");
}

void
DrishtiPaint::tagTubes(Vec bmin, Vec bmax, int tag)
{
  tagTubes(bmin, bmax, tag,
	   true, 0, 0, 0, 0);  // shrinkwrap all
}

void
DrishtiPaint::tagTubes(Vec bmin, Vec bmax, int tag,
		       bool all,
		       int dr, int wr, int hr, int ctag)
{
  int minD,maxD, minW,maxW, minH,maxH;

  QList<Vec> cPos =  m_viewer->clipPos();
  QList<Vec> cNorm = m_viewer->clipNorm();
  
  float minGrad = m_viewer->minGrad();
  float maxGrad = m_viewer->maxGrad();
  int gradType = m_viewer->gradType();
  
  VolumeOperations::setClip(cPos, cNorm);
  VolumeOperations::tagTubes(bmin, bmax,
			     tag,
			     all, dr, wr, hr, ctag,
			     minD, maxD,
			     minW, maxW,
			     minH, maxH,
			     gradType, minGrad, maxGrad);

  if (minD < 0)
    return;

  updateModifiedRegion(minD, maxD,
		       minW, maxW,
		       minH, maxH);

  QMessageBox::information(0, "", "Tag Tubes done");
}

void
DrishtiPaint::loadRawMask(QString flnm)
{
  m_volume->loadRawMask(flnm);

  m_viewer->setMaskDataPtr(m_volume->memMaskDataPtr());

  int m_depth, m_width, m_height;
  m_volume->gridSize(m_depth, m_width, m_height);
  m_viewer->uploadMask(0,0,0, m_depth-1,m_width-1,m_height-1);
  QMessageBox::information(0, "", "done");
}

void
DrishtiPaint::reloadMask()
{
  if (!m_volume->isValid())
    return;
    
  m_volume->setMaskVoxelType(2);
			     
  m_volume->reloadMask();

  m_viewer->setMaskDataPtr(m_volume->memMaskDataPtr());
  m_axialImage->setMaskPtr(m_volume->memMaskDataPtr());
  m_sagitalImage->setMaskPtr(m_volume->memMaskDataPtr());
  m_coronalImage->setMaskPtr(m_volume->memMaskDataPtr());

  VolumeOperations::setMaskData(m_volume->memMaskDataPtr());
  VolumeMeasure::setMaskData(m_volume->memMaskDataPtr());

  
  int m_depth, m_width, m_height;
  m_volume->gridSize(m_depth, m_width, m_height);
  m_viewer->uploadMask(0,0,0, m_depth-1,m_width-1,m_height-1);

  QMessageBox::information(0, "", "done");
}

void
DrishtiPaint::resetTag(Vec bmin, Vec bmax, int tag)
{
  int minD,maxD, minW,maxW, minH,maxH;

  QList<Vec> cPos =  m_viewer->clipPos();
  QList<Vec> cNorm = m_viewer->clipNorm();

  VolumeOperations::setClip(cPos, cNorm);
  VolumeOperations::resetTag(bmin, bmax,
			     tag,
			     minD, maxD,
			     minW, maxW,
			     minH, maxH);

  if (minD < 0)
    return;

  updateModifiedRegion(minD, maxD,
		       minW, maxW,
		       minH, maxH);
}

void
DrishtiPaint::hatchConnectedRegion(int dr, int wr, int hr,
				   Vec bmin, Vec bmax,
				   int tag, int ctag,
				   int thickness, int interval)
{
  int minD,maxD, minW,maxW, minH,maxH;

  QList<Vec> cPos =  m_viewer->clipPos();
  QList<Vec> cNorm = m_viewer->clipNorm();

  VolumeOperations::setClip(cPos, cNorm);
  VolumeOperations::hatchConnectedRegion(dr, wr, hr,
					 bmin, bmax,
					 tag, ctag,
					 thickness, interval,
					 minD, maxD,
					 minW, maxW,
					 minH, maxH);

  if (minD < 0)
    return;

  updateModifiedRegion(minD, maxD,
		       minW, maxW,
		       minH, maxH);
}

void
DrishtiPaint::connectedRegion(int dr, int wr, int hr,
			      Vec bmin, Vec bmax,
			      int tag, int ctag)
{
  int minD,maxD, minW,maxW, minH,maxH;

  QList<Vec> cPos =  m_viewer->clipPos();
  QList<Vec> cNorm = m_viewer->clipNorm();

  float minGrad = m_viewer->minGrad();
  float maxGrad = m_viewer->maxGrad();
  int gradType = m_viewer->gradType();
  
  VolumeOperations::setClip(cPos, cNorm);
  VolumeOperations::connectedRegion(dr, wr, hr,
				    bmin, bmax,
				    tag, ctag,
				    minD, maxD,
				    minW, maxW,
				    minH, maxH,
				    gradType, minGrad, maxGrad);

  if (minD < 0)
    return;

  updateModifiedRegion(minD, maxD,
		       minW, maxW,
		       minH, maxH);
}

void
DrishtiPaint::smoothConnectedRegion(int dr, int wr, int hr,
				    Vec bmin, Vec bmax,
				    int ctag, int filterWidth)
{
  int minD,maxD, minW,maxW, minH,maxH;
  
  QList<Vec> cPos =  m_viewer->clipPos();
  QList<Vec> cNorm = m_viewer->clipNorm();

  float minGrad = m_viewer->minGrad();
  float maxGrad = m_viewer->maxGrad();
  int gradType = m_viewer->gradType();
  
  VolumeOperations::setClip(cPos, cNorm);
  VolumeOperations::smoothConnectedRegion(dr, wr, hr,
					  bmin, bmax,
					  ctag,
					  minD, maxD,
					  minW, maxW,
					  minH, maxH,
					  gradType, minGrad, maxGrad,
					  filterWidth);

  if (minD < 0)
    return;

  updateModifiedRegion(minD, maxD,
		       minW, maxW,
		       minH, maxH);
}

void
DrishtiPaint::smoothAllRegion(Vec bmin, Vec bmax,
			      int tag, int filterWidth)
{
  int minD,maxD, minW,maxW, minH,maxH;
  
  QList<Vec> cPos =  m_viewer->clipPos();
  QList<Vec> cNorm = m_viewer->clipNorm();

  float minGrad = m_viewer->minGrad();
  float maxGrad = m_viewer->maxGrad();
  int gradType = m_viewer->gradType();
  
  VolumeOperations::setClip(cPos, cNorm);
  VolumeOperations::smoothAllRegion(bmin, bmax,
				    tag,
				    minD, maxD,
				    minW, maxW,
				    minH, maxH,
				    gradType, minGrad, maxGrad,
				    filterWidth);

  if (minD < 0)
    return;

  updateModifiedRegion(minD, maxD,
		       minW, maxW,
		       minH, maxH);
}

void
DrishtiPaint::saveToMask(Vec bmin, Vec bmax, int tag)
{
  int minD,maxD, minW,maxW, minH,maxH;

  QList<Vec> cPos =  m_viewer->clipPos();
  QList<Vec> cNorm = m_viewer->clipNorm();

  float minGrad = m_viewer->minGrad();
  float maxGrad = m_viewer->maxGrad();
  int gradType = m_viewer->gradType();

  VolumeOperations::setClip(cPos, cNorm);
  if (VolumeOperations::saveToMask(bmin, bmax,
				   tag,
				   minD, maxD,
				   minW, maxW,
				   minH, maxH,
				   gradType, minGrad, maxGrad))
    {
      int m_depth, m_width, m_height;
      m_volume->gridSize(m_depth, m_width, m_height);
      
      updateModifiedRegion(0, m_depth,
			   0, m_width,
			   0, m_height);
    }
}

void
DrishtiPaint::maskOperation(Vec bmin, Vec bmax, int tag)
{
  int minD,maxD, minW,maxW, minH,maxH;

  QList<Vec> cPos =  m_viewer->clipPos();
  QList<Vec> cNorm = m_viewer->clipNorm();

  float minGrad = m_viewer->minGrad();
  float maxGrad = m_viewer->maxGrad();
  int gradType = m_viewer->gradType();

  VolumeOperations::setClip(cPos, cNorm);
  if (VolumeOperations::maskOperation(bmin, bmax,
				      tag,
				      minD, maxD,
				      minW, maxW,
				      minH, maxH,
				      gradType, minGrad, maxGrad))
    {
      int m_depth, m_width, m_height;
      m_volume->gridSize(m_depth, m_width, m_height);
      
      updateModifiedRegion(0, m_depth,
			   0, m_width,
			   0, m_height);
    }
}

void
DrishtiPaint::connectedComponents(Vec bmin, Vec bmax, int tag)
{
  int minD,maxD, minW,maxW, minH,maxH;

  QList<Vec> cPos =  m_viewer->clipPos();
  QList<Vec> cNorm = m_viewer->clipNorm();

  float minGrad = m_viewer->minGrad();
  float maxGrad = m_viewer->maxGrad();
  int gradType = m_viewer->gradType();

  VolumeOperations::setClip(cPos, cNorm);
  VolumeOperations::connectedComponents(bmin, bmax,
					tag,
					minD, maxD,
					minW, maxW,
					minH, maxH,
					gradType, minGrad, maxGrad);

  if (minD < 0)
    return;

  updateModifiedRegion(minD, maxD,
		       minW, maxW,
		       minH, maxH);
}

void
DrishtiPaint::connectedComponentsPlus(Vec bmin, Vec bmax, int tag, int size)
{
  int minD,maxD, minW,maxW, minH,maxH;

  QList<Vec> cPos =  m_viewer->clipPos();
  QList<Vec> cNorm = m_viewer->clipNorm();

  float minGrad = m_viewer->minGrad();
  float maxGrad = m_viewer->maxGrad();
  int gradType = m_viewer->gradType();

  VolumeOperations::setClip(cPos, cNorm);
  VolumeOperations::connectedComponentsPlus(bmin, bmax, tag,
					    size,
					    minD, maxD,
					    minW, maxW,
					    minH, maxH,
					    gradType, minGrad, maxGrad);
  
  if (minD < 0)
    return;

  updateModifiedRegion(minD, maxD,
		       minW, maxW,
		       minH, maxH);
}

void
DrishtiPaint::distanceTransform(Vec bmin, Vec bmax, int tag, int size)
{
  int minD,maxD, minW,maxW, minH,maxH;

  QList<Vec> cPos =  m_viewer->clipPos();
  QList<Vec> cNorm = m_viewer->clipNorm();

  float minGrad = m_viewer->minGrad();
  float maxGrad = m_viewer->maxGrad();
  int gradType = m_viewer->gradType();

  VolumeOperations::setClip(cPos, cNorm);
  VolumeOperations::distanceTransform(bmin, bmax, tag,
				      //size,
				      minD, maxD,
				      minW, maxW,
				      minH, maxH,
				      gradType, minGrad, maxGrad);

  if (minD < 0)
    return;

  updateModifiedRegion(minD, maxD,
		       minW, maxW,
		       minH, maxH);
}

void
DrishtiPaint::removeComponents(Vec bmin, Vec bmax, int tag)
{
  int minD,maxD, minW,maxW, minH,maxH;

  QList<Vec> cPos =  m_viewer->clipPos();
  QList<Vec> cNorm = m_viewer->clipNorm();

  float minGrad = m_viewer->minGrad();
  float maxGrad = m_viewer->maxGrad();
  int gradType = m_viewer->gradType();

  VolumeOperations::setClip(cPos, cNorm);
  VolumeOperations::removeComponents(bmin, bmax,
				     tag,
				     minD, maxD,
				     minW, maxW,
				     minH, maxH,
				     gradType, minGrad, maxGrad);

  if (minD < 0)
    return;

  updateModifiedRegion(minD, maxD,
		       minW, maxW,
		       minH, maxH);
}

void
DrishtiPaint::removeLargestComponents(Vec bmin, Vec bmax, int tag)
{
  int minD,maxD, minW,maxW, minH,maxH;

  QList<Vec> cPos =  m_viewer->clipPos();
  QList<Vec> cNorm = m_viewer->clipNorm();

  float minGrad = m_viewer->minGrad();
  float maxGrad = m_viewer->maxGrad();
  int gradType = m_viewer->gradType();

  VolumeOperations::setClip(cPos, cNorm);
  VolumeOperations::removeLargestComponents(bmin, bmax,
					    tag,
					    minD, maxD,
					    minW, maxW,
					    minH, maxH,
					    gradType, minGrad, maxGrad);
  
  if (minD < 0)
    return;

  updateModifiedRegion(minD, maxD,
		       minW, maxW,
		       minH, maxH);
}


void
DrishtiPaint::setVisible(Vec bmin, Vec bmax, int tag, bool visible)
{
  int minD,maxD, minW,maxW, minH,maxH;

  QList<Vec> cPos =  m_viewer->clipPos();
  QList<Vec> cNorm = m_viewer->clipNorm();

  float minGrad = m_viewer->minGrad();
  float maxGrad = m_viewer->maxGrad();
  int gradType = m_viewer->gradType();

  VolumeOperations::setClip(cPos, cNorm);
  VolumeOperations::setVisible(bmin, bmax,
			       tag, visible,
			       minD, maxD,
			       minW, maxW,
			       minH, maxH,
			       gradType, minGrad, maxGrad);

  if (minD < 0)
    return;

  updateModifiedRegion(minD, maxD,
		       minW, maxW,
		       minH, maxH);
}


void
DrishtiPaint::stepTags(Vec bmin, Vec bmax, int tagStep, int tagVal)
{
  int minD,maxD, minW,maxW, minH,maxH;

  QList<Vec> cPos =  m_viewer->clipPos();
  QList<Vec> cNorm = m_viewer->clipNorm();

  VolumeOperations::setClip(cPos, cNorm);
  VolumeOperations::stepTags(bmin, bmax,
			     tagStep, tagVal,
			     minD, maxD,
			     minW, maxW,
			     minH, maxH);


  if (minD < 0)
    return;

  updateModifiedRegion(minD, maxD,
		       minW, maxW,
		       minH, maxH);
}
void
DrishtiPaint::mergeTags(Vec bmin, Vec bmax, int tag1, int tag2, bool useTF)
{
  int minD,maxD, minW,maxW, minH,maxH;

  QList<Vec> cPos =  m_viewer->clipPos();
  QList<Vec> cNorm = m_viewer->clipNorm();

  VolumeOperations::setClip(cPos, cNorm);
  VolumeOperations::mergeTags(bmin, bmax,
			      tag1, tag2, useTF,
			      minD, maxD,
			      minW, maxW,
			      minH, maxH);

  if (minD < 0)
    return;

  updateModifiedRegion(minD, maxD,
		       minW, maxW,
		       minH, maxH);
}

void
DrishtiPaint::erodeConnected(int dr, int wr, int hr,
			     Vec bmin, Vec bmax, int tag)
{
  int minD,maxD, minW,maxW, minH,maxH;

  float minGrad = m_viewer->minGrad();
  float maxGrad = m_viewer->maxGrad();
  int gradType = m_viewer->gradType();

  QList<Vec> cPos =  m_viewer->clipPos();
  QList<Vec> cNorm = m_viewer->clipNorm();

  VolumeOperations::setClip(cPos, cNorm);
  VolumeOperations::erodeConnected(dr, wr, hr,
				   bmin, bmax, tag,
				   viewerUi.dilateRad->value(),
				   minD, maxD,
				   minW, maxW,
				   minH, maxH,
				   gradType, minGrad, maxGrad);

  if (minD < 0)
    return;

  updateModifiedRegion(minD, maxD,
		       minW, maxW,
		       minH, maxH);
}

void
DrishtiPaint::erodeAll(Vec bmin, Vec bmax, int tag, int size)
{
  int minD,maxD, minW,maxW, minH,maxH;

  float minGrad = m_viewer->minGrad();
  float maxGrad = m_viewer->maxGrad();
  int gradType = m_viewer->gradType();

  QList<Vec> cPos =  m_viewer->clipPos();
  QList<Vec> cNorm = m_viewer->clipNorm();

  VolumeOperations::setClip(cPos, cNorm);
  VolumeOperations::erodeAll(bmin, bmax, tag,
			     size,
			     minD, maxD,
			     minW, maxW,
			     minH, maxH,
			     gradType, minGrad, maxGrad);

  if (minD < 0)
    return;

  updateModifiedRegion(minD, maxD,
		       minW, maxW,
		       minH, maxH);
}

void
DrishtiPaint::dilateConnected(int dr, int wr, int hr,
			      Vec bmin, Vec bmax, int tag,
			      bool allVisible)
{
  int minD,maxD, minW,maxW, minH,maxH;

  QList<Vec> cPos =  m_viewer->clipPos();
  QList<Vec> cNorm = m_viewer->clipNorm();

  float minGrad = m_viewer->minGrad();
  float maxGrad = m_viewer->maxGrad();
  int gradType = m_viewer->gradType();

  VolumeOperations::setClip(cPos, cNorm);
  VolumeOperations::dilateConnected(dr, wr, hr,
				    bmin, bmax, tag,
				    viewerUi.dilateRad->value()+1,
				    minD, maxD,
				    minW, maxW,
				    minH, maxH,
				    allVisible,
				    gradType, minGrad, maxGrad);
  
  if (minD < 0)
    return;

  updateModifiedRegion(minD, maxD,
		       minW, maxW,
		       minH, maxH);
}

void
DrishtiPaint::dilateAllTags(Vec bmin, Vec bmax,
			    int size)
{
  int minD,maxD, minW,maxW, minH,maxH;
  
  QList<Vec> cPos =  m_viewer->clipPos();
  QList<Vec> cNorm = m_viewer->clipNorm();
  
  float minGrad = m_viewer->minGrad();
  float maxGrad = m_viewer->maxGrad();
  int gradType = m_viewer->gradType();

  VolumeOperations::setClip(cPos, cNorm);
  VolumeOperations::dilateAllTags(bmin, bmax,
				  size,
				  minD, maxD,
				  minW, maxW,
				  minH, maxH,
				  gradType, minGrad, maxGrad);
  
  if (minD < 0)
    return;

  updateModifiedRegion(minD, maxD,
		       minW, maxW,
		       minH, maxH);
}

void
DrishtiPaint::dilateAll(Vec bmin, Vec bmax, int tag,
			int size)
{
  int minD,maxD, minW,maxW, minH,maxH;

  QList<Vec> cPos =  m_viewer->clipPos();
  QList<Vec> cNorm = m_viewer->clipNorm();

  float minGrad = m_viewer->minGrad();
  float maxGrad = m_viewer->maxGrad();
  int gradType = m_viewer->gradType();

  VolumeOperations::setClip(cPos, cNorm);
  VolumeOperations::dilateAll(bmin, bmax, tag,
			      size,
			      minD, maxD,
			      minW, maxW,
			      minH, maxH,
			      true,
			      gradType, minGrad, maxGrad);
  
  if (minD < 0)
    return;

  updateModifiedRegion(minD, maxD,
		       minW, maxW,
		       minH, maxH);
}

void
DrishtiPaint::openAll(Vec bmin, Vec bmax,
		      int tag,
		      int nErode, int nDilate)
{
  int minD,maxD, minW,maxW, minH,maxH;

  QList<Vec> cPos =  m_viewer->clipPos();
  QList<Vec> cNorm = m_viewer->clipNorm();

  float minGrad = m_viewer->minGrad();
  float maxGrad = m_viewer->maxGrad();
  int gradType = m_viewer->gradType();

  VolumeOperations::setClip(cPos, cNorm);
  VolumeOperations::openAll(bmin, bmax, tag,
			    nErode, nDilate,
			    minD, maxD,
			    minW, maxW,
			    minH, maxH,
			    gradType, minGrad, maxGrad);
  
  if (minD < 0)
    return;

  updateModifiedRegion(minD, maxD,
		       minW, maxW,
		       minH, maxH);
}

void
DrishtiPaint::closeAll(Vec bmin, Vec bmax,
		       int tag,
		       int nDilate, int nErode)
{
  int minD,maxD, minW,maxW, minH,maxH;

  QList<Vec> cPos =  m_viewer->clipPos();
  QList<Vec> cNorm = m_viewer->clipNorm();

  float minGrad = m_viewer->minGrad();
  float maxGrad = m_viewer->maxGrad();
  int gradType = m_viewer->gradType();

  VolumeOperations::setClip(cPos, cNorm);
  VolumeOperations::closeAll(bmin, bmax, tag,
			     nDilate, nErode,
			     minD, maxD,
			     minW, maxW,
			     minH, maxH,
			     gradType, minGrad, maxGrad);
  
  if (minD < 0)
    return;

  updateModifiedRegion(minD, maxD,
		       minW, maxW,
		       minH, maxH);
}


void
DrishtiPaint::sortLabels(Vec bmin, Vec bmax)
{
  int minD,maxD, minW,maxW, minH,maxH;

  QList<Vec> cPos =  m_viewer->clipPos();
  QList<Vec> cNorm = m_viewer->clipNorm();

  float minGrad = m_viewer->minGrad();
  float maxGrad = m_viewer->maxGrad();
  int gradType = m_viewer->gradType();

  VolumeOperations::setClip(cPos, cNorm);
  VolumeOperations::sortLabels(bmin, bmax,
			       gradType, minGrad, maxGrad);

  minD = bmin.z;     maxD = bmax.z;
  minW = bmin.y;     maxW = bmax.y;
  minH = bmin.x;     maxH = bmax.x;

  updateModifiedRegion(minD, maxD,
		       minW, maxW,
		       minH, maxH);
}


void
DrishtiPaint::modifyOriginalVolume(Vec bmin, Vec bmax, int val)
{
  int minD,maxD, minW,maxW, minH,maxH;

  QList<Vec> cPos =  m_viewer->clipPos();
  QList<Vec> cNorm = m_viewer->clipNorm();

  VolumeOperations::setClip(cPos, cNorm);
  VolumeOperations::modifyOriginalVolume(bmin, bmax, val,
					 minD, maxD,
					 minW, maxW,
					 minH, maxH);
  
  if (minD < 0)
    return;
  
  m_volume->genHistogram(true);
  m_volume->generateHistogramImage();

  m_tfEditor->setHistogramImage(m_volume->histogramImage1D(),
				m_volume->histogramImage2D());
  
  m_viewer->generateBoxMinMax();

  m_volume->saveModifiedOriginalVolume();
  
  QMessageBox::information(0, "", "Modified Volume Saved.");
}

void
DrishtiPaint::bakeCurves_clicked()
{
  if (!m_volume->isValid())
    {
      QMessageBox::information(0, "Error", "No volume data found !");
      return;
    }
  

  bool ok;
  int tag = QInputDialog::getInt(0,
				 "Bake curves for Label",
				 "Value (0-65535)\n Everything visible inside the interpolated\n curves will be labelled with given label value.",
				 0, 0, 65535, 1,
				 &ok);

  if (!ok)
    return;
  
  
  int depth, width, height;
  m_volume->gridSize(depth, width, height);
  
  int minDSlice, maxDSlice;
  int minWSlice, maxWSlice;
  int minHSlice, maxHSlice;
  m_viewer->getBox(minDSlice, maxDSlice,
		   minWSlice, maxWSlice,
		   minHSlice, maxHSlice);
  qint64 tdepth = maxDSlice-minDSlice+1;
  qint64 twidth = maxWSlice-minWSlice+1;
  qint64 theight = maxHSlice-minHSlice+1;
  
  QProgressDialog progress("Baking curves into label data",
			   QString(),
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);

  //----------------------------------
  
  uchar *curveMask = 0;
  try
    {
      curveMask = new uchar[tdepth*twidth*theight];
    }
  catch (std::exception &e)
    {
      QMessageBox::information(0, "", "Not enough memory : Cannot create curve mask.\nOffloading volume data and mask.");
      m_volume->offloadMemFile();
      
      curveMask = new uchar[tdepth*twidth*theight];
    };
  
  memset(curveMask, 0, tdepth*twidth*theight);

  //----------------------------------
  // bake for all the curves
  updateCurveMask(curveMask,
		  depth, width, height,
		  tdepth, twidth, theight,
		  minDSlice, minWSlice, minHSlice,
		  maxDSlice, maxWSlice, maxHSlice);
  //----------------------------------

  float minGrad = m_viewer->minGrad();
  float maxGrad = m_viewer->maxGrad();
  int gradType = m_viewer->gradType();

  VolumeOperations::bakeCurves(curveMask,
			       minDSlice, maxDSlice,
			       minWSlice, maxWSlice,
			       minHSlice, maxHSlice,
			       tag,
			       gradType, minGrad, maxGrad);

  delete [] curveMask;

  m_viewer->uploadMask(minDSlice,minWSlice,minHSlice,
		       maxDSlice,maxWSlice,maxHSlice);
  
  QList<int> dwh;  
  m_blockList.clear();  
  dwh << minDSlice << minWSlice << minHSlice;
  m_blockList << dwh;
  dwh.clear();
  dwh << maxDSlice << maxWSlice << maxHSlice;
  m_blockList << dwh;

  progress.setLabelText("Save modified region to mask file");
  qApp->processEvents();
  m_volume->saveMaskBlock(m_blockList);

  m_axialCurves->sliceChanged();
  m_sagitalCurves->sliceChanged();
  m_coronalCurves->sliceChanged();

  progress.setValue(100);  
  QMessageBox::information(0, "Converted", "-----Done-----");
}

void
DrishtiPaint::on_changeSliceOrdering_triggered()
{
  if (!m_volume->isValid())
    {
      QMessageBox::information(0, "Error", "No volume data found !");
      return;
    }

  uchar *vol = m_volume->memVolDataPtr();
  ushort *volUS = 0;
  if (Global::bytesPerVoxel() == 2)
    volUS = (ushort*)vol;
  uchar *mask = m_volume->memMaskDataPtr();

  int depth, width, height;
  m_volume->gridSize(depth, width, height);

  {
    int nbytes = width*height*Global::bytesPerVoxel();
    uchar *tmp = new uchar[nbytes];
    if (!volUS)
      {
	for(int d=0; d<depth/2; d++)
	  {
	    memcpy(tmp, vol+d*nbytes, nbytes);
	    memcpy(vol+d*nbytes, vol+(depth-1-d)*nbytes, nbytes);
	    memcpy(vol+(depth-1-d)*nbytes, tmp, nbytes);
	  }
      }
    else
      {
	for(int d=0; d<depth/2; d++)
	  {
	    memcpy(tmp, volUS+d*nbytes, nbytes);
	    memcpy(volUS+d*nbytes, volUS+(depth-1-d)*nbytes, nbytes);
	    memcpy(volUS+(depth-1-d)*nbytes, tmp, nbytes);
	  }
      }
    delete [] tmp;
  }
  {
    int nbytes = 2*width*height;    // mask is 16bit per voxel
    uchar *tmp = new uchar[nbytes];
    for(int d=0; d<depth/2; d++)
      {
	memcpy(tmp, mask+d*nbytes, nbytes);
	memcpy(mask+d*nbytes, mask+(depth-1-d)*nbytes, nbytes);
	memcpy(mask+(depth-1-d)*nbytes, tmp, nbytes);
      }

    delete [] tmp;
  }

  m_viewer->generateBoxMinMax();
  m_viewer->updateVoxels();

  m_axialImage->reloadSlice();
  m_sagitalImage->reloadSlice();
  m_coronalImage->reloadSlice();

  m_axialCurves->sliceChanged();
  m_sagitalCurves->sliceChanged();
  m_coronalCurves->sliceChanged();

  m_volume->saveModifiedOriginalVolume();
  m_volume->saveIntermediateResults(true);
}

void
DrishtiPaint::showVolumeInformation()
{
  if (!m_volume->isValid())
    {
      QMessageBox::information(0, "Error", "No volume data found !");
      return;
    }

  QString mesg;
  mesg += "File : " + m_volume->fileName() + "\n";
  
  int d, w, h;
  m_volume->gridSize(d, w, h);
  mesg += QString("Size : %1 %2 %3\n").arg(h).arg(w).arg(d);

  QMessageBox::information(0, "Volume Information", mesg);
}

void
DrishtiPaint::extractFromAnotherVolume(QList<int> tags)
{
  QString flnm;
  flnm = QFileDialog::getOpenFileName(0,
				      "Another Volume file to extract from",
				      Global::previousDirectory(),
				      "PVL Files (*.pvl.nc)",
				      0);
				      //QFileDialog::DontUseNativeDialog);
  
  
  if (flnm.isEmpty())
    return;
  
  if (!StaticFunctions::xmlHeaderFile(flnm))
    {
      QMessageBox::information(0, "Error",
			       QString("%1 is not a valid preprocessed volume file").
			       arg(flnm));
      return;
    }

  VolumeFileManager aVolume;
  
  int aDepth,aWidth,aHeight;
  StaticFunctions::getDimensionsFromHeader(flnm,
					   aDepth, aWidth, aHeight);

  int slabSize = StaticFunctions::getSlabsizeFromHeader(flnm);	  
  int voxelType = StaticFunctions::getPvlVoxelTypeFromHeader(flnm);
  int headerSize = StaticFunctions::getPvlHeadersizeFromHeader(flnm);
  QStringList pvlnames = StaticFunctions::getPvlNamesFromHeader(flnm);
  if (pvlnames.count() > 0)
    aVolume.setFilenameList(pvlnames);
  aVolume.setBaseFilename(flnm);
  aVolume.setVoxelType(voxelType);
  aVolume.setDepth(aDepth);
  aVolume.setWidth(aWidth);
  aVolume.setHeight(aHeight);
  aVolume.setHeaderSize(headerSize);
  aVolume.setSlabSize(slabSize);

  int bpv = 1;
  if (voxelType <2)
    bpv = 1;
  else if (voxelType < 4)
    bpv = 2;
  else
    bpv = 4;


  //----------------
  int outsideVal = 0;
  if (bpv == 1)
    {
      outsideVal = QInputDialog::getInt(0,
					"Outside value",
					"Set outside value (0-255) to",
					0, 0, 255, 1);
    }
  else
    {
      outsideVal = QInputDialog::getInt(0,
					"Outside value",
					"Set outside value (0-65535) to",
					0, 0, 65535, 1);
    }
  //----------------

  //----------------
  int clearance = QInputDialog::getInt(0,
				       "Clearance for tight fit",
				       "Gap from edge to first contributing voxel",
				       0, 0, 20);
  //----------------

  
  //----------------
  QString pvlFilename = m_volume->fileName();
  QString tagflnm = QFileDialog::getSaveFileName(0,
						 "Save extracted volume into",
						 QFileInfo(pvlFilename).absolutePath(),
						 "Volume Data Files (*.pvl.nc)",
						 0);
						 //QFileDialog::DontUseNativeDialog);
  
  if (tagflnm.isEmpty())
    return;
  //----------------
  
  //----------------
  // original volume
  int depth, width, height;
  m_volume->gridSize(depth, width, height);
  //----------------

  
  QProgressDialog progress("Extracting tagged region from volume data",
			   QString(),
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);
      
  // currently take only first tag
  for (int tt=0; tt<tags.count(); tt++)
    {
      int tag = tags[tt];
      
      QString tflnm = tagflnm;
      if (StaticFunctions::checkExtension(tflnm, ".pvl.nc"))
	tflnm = tflnm.chopped(7);
      tflnm += QString("-%1.pvl.nc").arg(tag);

      progress.setLabelText(QString("Extracting tag %1 to %2").	\
			    arg(tag).arg(tflnm));
  
      int minDSlice, maxDSlice;
      int minWSlice, maxWSlice;
      int minHSlice, maxHSlice;
      m_volume->findStartEndForTag(tag,
				   minDSlice, maxDSlice,
				   minWSlice, maxWSlice,
				   minHSlice, maxHSlice);
      
      minDSlice = qMax(0, minDSlice-clearance);
      maxDSlice = qMin(depth-1, maxDSlice+clearance);
      minWSlice = qMax(0, minWSlice-clearance);
      maxWSlice = qMin(width-1, maxWSlice+clearance);
      minHSlice = qMax(0, minHSlice-clearance);
      maxHSlice = qMin(height-1, maxHSlice+clearance);
      
//      QMessageBox::information(0, "", QString("Volume Size :\n%1 %2\n%3 %4\n%5 %6"). \
//			       arg(minD).arg(maxD).arg(minW).arg(maxW).	\
//			       arg(minH).arg(maxH));
      
      float scld = (float)aDepth/(float)depth;
      float sclw = (float)aWidth/(float)width;
      float sclh = (float)aHeight/(float)height;
      
      
      int aminD = scld*minDSlice;
      int amaxD = scld*maxDSlice;
      int aminW = sclw*minWSlice;
      int amaxW = sclw*maxWSlice;
      int aminH = sclh*minHSlice;
      int amaxH = sclh*maxHSlice;
      qint64 atdepth = amaxD-aminD+1;
      qint64 atwidth = amaxW-aminW+1;
      qint64 atheight = amaxH-aminH+1;
      
      
      savePvlHeader(m_volume->fileName(),
		    tflnm,
		    atdepth, atwidth, atheight,
		    bpv);
      
      QStringList tflnms;
      tflnms << tflnm+".001";
      VolumeFileManager tFile;
      tFile.setFilenameList(tflnms);
      if (bpv == 1)
	tFile.setVoxelType(VolumeFileManager::_UChar);
      else
	tFile.setVoxelType(VolumeFileManager::_UShort);
      tFile.setDepth(atdepth);
      tFile.setWidth(atwidth);
      tFile.setHeight(atheight);
      tFile.setSlabSize(atdepth+1);
      tFile.createFile(true, false);
      
      uchar *maskData = m_volume->memMaskDataPtr();
      
      int nbytes = aWidth*aHeight*bpv;
      uchar *raw = new uchar[nbytes];
      
      for(int d=aminD; d<=amaxD; d++)
	{
	  int d2 = d/scld;
	  
	  int slc = d-aminD;
	  progress.setValue((int)(100*(float)slc/(float)atdepth));
	  qApp->processEvents();
	  
	  uchar *slice = aVolume.getSlice(d);
	  ushort *sliceUS = 0;
	  ushort *rawUS = 0;
	  if (bpv == 2)
	    {
	      sliceUS = (ushort*)slice;
	      rawUS = (ushort*)raw;
	    }
	  
	  // we get value+grad from volume
	  // we need only value part
	  if (bpv == 1)
	    {
	      int i=0;
	      for(int w=aminW; w<=amaxW; w++)
		{
		  int w2 = w/sclw;
		  for(int h=aminH; h<=amaxH; h++)
		    {
		      int h2 = h/sclh;
		      if (maskData[d2*width*height + w2*height + h2] == tag)
			raw[i] = slice[w*aHeight+h];
		      else
			raw[i] = outsideVal;
		      i++;
		    }
		}
	    }
	  else
	    {
	      int i=0;
	      for(int w=aminW; w<=amaxW; w++)
		{
		  int w2 = w/sclw;
		  for(int h=aminH; h<=amaxH; h++)
		    {
		      int h2 = h/sclh;
		      if (maskData[d2*width*height + w2*height + h2] == tag)
			rawUS[i] = sliceUS[w*aHeight+h];
		      else
			rawUS[i] = outsideVal;
		      i++;
		    }
		}
	    }
	  
	  tFile.setSlice(slc, raw);
	} // loop on slices

      delete [] raw;
    } // loop on tags

  progress.setValue(100);  
  QMessageBox::information(0, "Save", "-----Done-----");
}

void
DrishtiPaint::on_actionExportMask_triggered()
{
  m_volume->exportMask();
}

void
DrishtiPaint::on_actionImportMask_triggered()
{
  QString flnm;
  flnm = QFileDialog::getOpenFileName(0,
				      "Raw Mask File",
				      Global::previousDirectory(),
				      "Raw Mask Files (*.raw)");
  
  if (flnm.isEmpty())
    return;

  m_volume->loadRawMask(flnm);

  m_viewer->setMaskDataPtr(m_volume->memMaskDataPtr());

  int m_depth, m_width, m_height;
  m_volume->gridSize(m_depth, m_width, m_height);
  m_viewer->uploadMask(0,0,0, m_depth-1,m_width-1,m_height-1);
  QMessageBox::information(0, "", "done");
}

void
DrishtiPaint::on_actionCheckpoint_triggered()
{
  m_volume->checkPoint();
}

void
DrishtiPaint::on_actionLoadCheckpoint_triggered()
{
  if (!m_volume->loadCheckPoint())
    return;

  // update all windows
  m_axialImage->reloadSlice();
  m_sagitalImage->reloadSlice();
  m_coronalImage->reloadSlice();

  m_viewer->updateVoxels();

  QMessageBox::information(0, "Checkpoint Restored", "If you are happy with the restore,\nplease save it to the mask file using Save Work");
}
void
DrishtiPaint::loadCheckPoint(QString flnm)
{
  if (!m_volume->loadCheckPoint(flnm))
    return;

  // update all windows
  m_axialImage->reloadSlice();
  m_sagitalImage->reloadSlice();
  m_coronalImage->reloadSlice();

  m_viewer->updateVoxels();

  QMessageBox::information(0, "Checkpoint Restored", "If you are happy with the restore,\nplease save it to the mask file using Save Work");
}
void
DrishtiPaint::on_actionDeleteCheckpoint_triggered()
{
  m_volume->deleteCheckPoint();
}


void
DrishtiPaint::undoPaint3D()
{
  m_volume->undo();

  int m_depth, m_width, m_height;
  m_volume->gridSize(m_depth, m_width, m_height);
  m_viewer->uploadMask(0,0,0, m_depth-1,m_width-1,m_height-1);

  QMessageBox::information(0, "", "done");    
}

void
DrishtiPaint::tagsUsed(QList<int> ut)
{
  QStringList tagNames = m_tagColorEditor->tagNames();

  QString mesg;
  mesg = "Number of non-zero labels used : ";
  if (ut.contains(0))
    mesg += QString("%1\n").arg(ut.count()-1);
  else
    mesg += QString("%1\n").arg(ut.count());
  int x=0;
  for(int ti=0; ti<ut.count(); ti++)
    {
      if (ut[ti] > 0)
	{
	  if (x%5 == 0) mesg += "\n";
	  mesg += QString("%1   ").arg(ut[ti]);
	  x++;
	}
    }

  StaticFunctions::showMessage("Labels Used", mesg);
}
//---------------------


//---------------------
// execute python scripts or start command prompt
void
DrishtiPaint::on_actionScriptFolder_triggered()
{
  bool ok = false;
  QString folder = QFileDialog::getExistingDirectory(0,
						     "Scripts Folder",
						     Global::scriptFolder());
  if (!folder.isEmpty())
    {
      Global::setScriptFolder(folder);
    }
  
}
void
DrishtiPaint::on_actionCommand_triggered()
{
  if (m_pyWidget)
    {
      m_pyWidget->close();
    }

  m_pyWidget = new PyWidget();

  connect(m_pyWidget, &PyWidget::pyWidgetClosed,
	  [=]() {m_pyWidget = 0;});
  
  int d, w, h;
  m_volume->gridSize(d, w, h);
  m_pyWidget->setSize(d, w, h);
  m_pyWidget->setFilename(m_volume->fileName());
  m_pyWidget->setVolPtr(m_volume->memVolDataPtr());
  m_pyWidget->setMaskPtr(m_volume->memMaskDataPtr());
}
//---------------------

//---------------------
void
DrishtiPaint::on_action3DBoxSize_triggered()
{
  bool ok = false;
  Vec bsz = Global::boxSize3D();
  QString bsztxt = QString("%1 %2 %3").arg(bsz.x).arg(bsz.y).arg(bsz.z); 
  QString boxstr = QInputDialog::getText(0,
					 "3D Box Size",
					 "3D Box Size for creating training set",
					 QLineEdit::Normal,
					 bsztxt,
					 &ok);
  if (ok && !boxstr.isEmpty())
    {
      QStringList szlist = boxstr.split(" ", QString::SkipEmptyParts);
      int x,y,z;
      if (szlist.count() == 1)
	{
	  x = y = z = szlist[0].toInt();
	  Global::setBoxSize3D(Vec(x,y,z));
	}
      if (szlist.count() == 3)
	{
	  x = szlist[0].toInt();
	  y = szlist[1].toInt();
	  z = szlist[2].toInt();
	  Global::setBoxSize3D(Vec(x,y,z));
	}
    }
}
void
DrishtiPaint::on_actionDrawBoxes3D_triggered()
{
  Global::setShowBox3D(ui.actionDrawBoxes3D->isChecked());
}
void
DrishtiPaint::on_actionClear3DList_triggered()
{
  Global::clearBoxList3D();
  QMessageBox::information(0, "", "3D Box List Cleared");
}
void
DrishtiPaint::on_action3DBoxList_triggered()
{
  QList<Vec> bl = Global::boxList3D();
  QString str;
  for (int i=0; i<bl.count(); i++)
    {
      str += QString("%1 %2 %3\n").arg(bl[i].x).arg(bl[i].y).arg(bl[i].z);
    }  
  
  QTextEdit *tedit = new QTextEdit();
  tedit->insertPlainText("------------ Box List ---------------\n\n");
  tedit->insertPlainText(str);
  
  QVBoxLayout *layout = new QVBoxLayout();
  layout->addWidget(tedit);
  
  QDialog *blist = new QDialog();
  blist->setWindowTitle("Training Data 3D Box List");
  blist->setSizeGripEnabled(true);
  blist->setModal(true);
  blist->setLayout(layout);
  blist->exec();
}
void
DrishtiPaint::on_actionLoad3DList_triggered()
{
  QString boxflnm;
  boxflnm = QFileDialog::getOpenFileName(0,
					 "Load 3D Box List To Text File",
					 QFileInfo(m_pvlFile).absolutePath(),
					 "Text Files (*.txt)",
					 0);
  QFile box(boxflnm);
  if (box.open(QIODevice::ReadOnly | QIODevice::Text))
    {
      Global::clearBoxList3D();
      QTextStream in(&box);
      while(!in.atEnd())
	{
	  QString line = in.readLine();
	  QStringList w = line.split(" ", QString::SkipEmptyParts);
	  if (w.count() == 3)
	    {
	      Global::addToBoxList3D(Vec(w[0].toInt(), w[1].toInt(), w[2].toInt()));
	    }
	}
    }
}
void
DrishtiPaint::on_actionSave3DList_triggered()
{
  QString boxflnm;
  boxflnm = QFileDialog::getSaveFileName(0,
					 "Save 3D Box List To Text File",
					 QFileInfo(m_pvlFile).absolutePath(),
					 "Text Files (*.txt)",
					 0);
  QFile box(boxflnm);
  if (box.open(QIODevice::WriteOnly | QIODevice::Text))
    {
      QList<Vec> bl = Global::boxList3D();
      // take only unique ones
      QList<Vec> boxList;
      for(int i=0; i<bl.count(); i++)
	{
	  if (!boxList.contains(bl[i]))
	    boxList<<bl[i];
	}
      
      QTextStream out(&box);
      for (int i=0; i<boxList.count(); i++)
	out << QString("%1 %2 %3\n").arg(boxList[i].x).arg(boxList[i].y).arg(boxList[i].z);
    }
  QMessageBox::information(0, "", "Saved box list");
}
//---------------------

//---------------------
void
DrishtiPaint::on_action2DBoxSize_triggered()
{
  bool ok = false;
  int bsz = Global::boxSize2D();
  QString bsztxt = QString("%1").arg(bsz);
  QString boxstr = QInputDialog::getText(0,
					 "2D Box Size",
					 "2D Box Size for creating training set",
					 QLineEdit::Normal,
					 bsztxt,
					 &ok);
  if (ok && !boxstr.isEmpty())
    {
      QStringList szlist = boxstr.split(" ", QString::SkipEmptyParts);
      int x,y;
      if (szlist.count() >= 1)
	{
	  x = szlist[0].toInt();
	  Global::setBoxSize2D(x);
	}
    }
}
void
DrishtiPaint::on_actionDrawBoxes2D_triggered()
{
  Global::setShowBox2D(ui.actionDrawBoxes2D->isChecked());
}
void
DrishtiPaint::on_action2DBoxList_triggered()
{
  QList<Vec> bl = Global::boxList2D();
  QString str;
  for (int i=0; i<bl.count()/2; i++)
    {
      str += QString("%1 %2 %3 - %4 %5 %6\n").\
	arg(bl[2*i].x).arg(bl[2*i].y).arg(bl[2*i].z).\
	arg(bl[2*i+1].x).arg(bl[2*i+1].y).arg(bl[2*i+1].z);
    }  
  
  QTextEdit *tedit = new QTextEdit();
  tedit->insertPlainText("------------ Box List ---------------\n\n");
  tedit->insertPlainText(str);
  
  QVBoxLayout *layout = new QVBoxLayout();
  layout->addWidget(tedit);
  
  QDialog *blist = new QDialog();
  blist->setWindowTitle("Training Data 2D Box List");
  blist->setSizeGripEnabled(true);
  blist->setModal(true);
  blist->setLayout(layout);
  blist->exec();
}

void
DrishtiPaint::on_actionLoad2DList_triggered()
{
  QString boxflnm;
  boxflnm = QFileDialog::getOpenFileName(0,
					 "Load 2D Box List To Text File",
					 QFileInfo(m_pvlFile).absolutePath(),
					 "Text Files (*.txt)",
					 0);
  QFile box(boxflnm);
  if (box.open(QIODevice::ReadOnly | QIODevice::Text))
    {
      Global::clearBoxList2D();
      QTextStream in(&box);
      while(!in.atEnd())
	{
	  QString line = in.readLine();
	  QStringList w = line.split(" ", QString::SkipEmptyParts);
	  if (w.count() == 6)
	    {
	      Global::addToBoxList2D(Vec(w[0].toInt(), w[1].toInt(), w[2].toInt()),
				     Vec(w[3].toInt(), w[4].toInt(), w[5].toInt()));
	    }
	}
    }
}

void
DrishtiPaint::on_actionSave2DList_triggered()
{
  QString boxflnm;
  boxflnm = QFileDialog::getSaveFileName(0,
					 "Save 2D Box List To Text File",
					 QFileInfo(m_pvlFile).absolutePath(),
					 "Text Files (*.txt)",
					 0);
  QFile box(boxflnm);
  if (box.open(QIODevice::WriteOnly | QIODevice::Text))
    {
      QList<Vec> bl = Global::boxList2D();
      QString str;
      for (int i=0; i<bl.count()/2; i++)
	{
	  str += QString("%1 %2 %3  %4 %5 %6\n").		\
	    arg(bl[2*i].x).arg(bl[2*i].y).arg(bl[2*i].z).	\
	    arg(bl[2*i+1].x).arg(bl[2*i+1].y).arg(bl[2*i+1].z);
	}  
      QTextStream out(&box);
      out << str;
    }
  QMessageBox::information(0, "", "Saved 2D box list to ");
}

void
DrishtiPaint::on_actionClear2DList_triggered()
{
  Global::clearBoxList2D();
  QMessageBox::information(0, "", "2D Box List Cleared");
}

void
DrishtiPaint::on_actionHelp2D_triggered()
{
  QString help;
  help += "Help for 2D Boxes\n";
  help += "-----------------\n";
  help += "Press 2 to define a 2D box centered around the voxel under the current cursor position.\n\n";

  help += "Double click middle mouse to reset 2D box.\n\n";

  help += "2D Box Size : specify 2D image size for training and prediction. ";
  help += "The box size can be smaller than the actual grid size.\n\n";

  help += "Draw Boxes : display currently selected training boxes in 2D and 3D views.\n\n";

  help += "Box List : list currently defined boxes.\n\n";

  help += "To File : save current box list to file. ";
  help += "Access this file in your script and extract relevant sections ";
  help += "from the volume and mask for training.\n\n"; 

  help += "From File : remove existing box list and load new one from file.\n\n";

  help += "Clear : remove existing box list.\n\n";
  help += "\n\n";

  help += "Script Folder : Location to store scripts to be call from Command.\n\n";

  help += "Command : A command promt is provided to run scripts from Script Folder.";
  help += " Script arguments are provided from the dialog adjoining the";
  help += " command prompt panel.  Each script is stored in its own directory.";
  help += " Each script directory contains a .json file which exposes the";
  help += " argument list to the user to modify.\n\n";

  help += "%DIR% parameter specifies the directory where volume and mask files";
  help += " reside.\n\n";  

  help += "Two arguments are provided by default to the script -";
  help += " volume=<volume file name> and mask=<mask file name>\n";
  help += "\n";
  
  QMessageBox::about(0, "Help for 2D boxes", help);
}

void
DrishtiPaint::on_actionHelp3D_triggered()
{
  QString help;
  help += "Help for 3D Boxes\n";
  help += "-----------------\n";
  help += "Press 3 to define a 3D box centered around the voxel under the current cursor position.\n\n";

  help += "Double click middle mouse to reset 2D box.\n\n";
    
  help += "3D Box Size : specify 3D image size for training and prediction.  The box size can be smaller than the actual grid size.\n\n";

  help += "Draw Boxes : display currently selected training boxes in 3D view only.\n\n";

  help += "Box List : list currently defined boxes.\n\n";

  help += "To File : save current box list to file.";
  help += " access this file in your script and extract relevant sections";
  help += " from the volume and mask for training.\n\n"; 

  help += "From File : remove existing box list and load new one from file.\n\n";

  help += "Clear : remove existing box list.\n";
  help += "\n\n";

  help += "Script Folder : Location to store scripts to be call from Command.\n\n";

  help += "Command : A command promt is provided to run scripts from Script Folder.";
  help += " Script arguments are provided from the dialog adjoining the";
  help += " command prompt panel.  Each script is stored in its own directory.";
  help += " Each script directory contains a .json file which exposes the";
  help += " argument list to the user to modify.\n\n";

  help += "%DIR% parameter specifies the directory where volume and mask files";
  help += " reside.\n";  

  help += "Two arguments are provided by default to the script -";
  help += " volume=<volume file name> and mask=<mask file name>\n";
  help += "\n";
  
  QMessageBox::about(0, "Help for 3D boxes", help);
}


bool
DrishtiPaint::getValues(float& isoValue,
			float& adaptivity,
			float& resample,
			int& morphoType, int& morphoRadius,
			int& dataSmooth,
			int& meshSmooth,
			bool& applyVoxelScaling)  
{
  isoValue = 0.5;
  adaptivity = 0.1;
  dataSmooth = 0;
  meshSmooth = 0;
  resample = 1.0;
  morphoType = 0;
  morphoRadius = 0;
  applyVoxelScaling = true;

  PropertyEditor propertyEditor;
  QMap<QString, QVariantList> plist;
  
  QVariantList vlist;

  vlist.clear();
  vlist << QVariant("float");
  vlist << QVariant(isoValue);
  vlist << QVariant(0.1);
  vlist << QVariant(0.9);
  vlist << QVariant(0.1); // singlestep
  vlist << QVariant(3); // decimals
  plist["isosurface value"] = vlist;

  vlist.clear();
  vlist << QVariant("float");
  vlist << QVariant(adaptivity);
  vlist << QVariant(0.0);
  vlist << QVariant(1.0);
  vlist << QVariant(0.01); // singlestep
  vlist << QVariant(3); // decimals
  plist["adaptivity"] = vlist;
  
  vlist.clear();
  vlist << QVariant("float");
  vlist << QVariant(resample);
  vlist << QVariant(1.0);
  vlist << QVariant(10.0);
  vlist << QVariant(1); // singlestep
  vlist << QVariant(1); // decimals
  plist["downsample"] = vlist;
  
  vlist.clear();
  vlist << QVariant("int");
  vlist << QVariant(dataSmooth);
  vlist << QVariant(0);
  vlist << QVariant(10);
  plist["smooth data"] = vlist;

  vlist.clear();
  vlist << QVariant("int");
  vlist << QVariant(meshSmooth);
  vlist << QVariant(0);
  vlist << QVariant(10);
  plist["mesh smoothing"] = vlist;

  
  vlist.clear();
  vlist << QVariant("combobox");
  vlist << "0";
  vlist << "";
  vlist << "Dilate";
  vlist << "Erode";
  vlist << "Close";
  vlist << "Open";
  plist["morpho operator"] = vlist;

  vlist.clear();
  vlist << QVariant("int");
  vlist << QVariant(morphoRadius);
  vlist << QVariant(0);
  vlist << QVariant(100);
  plist["morpho radius"] = vlist;



// Apply voxel scaling to mesh.
// Check only for loading the generated mesh in Drishti Renderer
// Otherwise leave it unchecked
  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(applyVoxelScaling);
  plist["apply voxel size"] = vlist;

  

  vlist.clear();
  QFile helpFile(":/mesh.help");
  if (helpFile.open(QFile::ReadOnly))
    {
      QTextStream in(&helpFile);
      QString line = in.readLine();
      while (!line.isNull())
	{
	  if (line == "#begin")
	    {
	      QString keyword = in.readLine();
	      QString helptext;
	      line = in.readLine();
	      while (!line.isNull())
		{
		  helptext += line;
		  helptext += "\n";
		  line = in.readLine();
		  if (line == "#end") break;
		}
	      vlist << keyword << helptext;
	    }
	  line = in.readLine();
	}
    }	      
  plist["commandhelp"] = vlist;
  

  QStringList keys;
  keys << "isosurface value";
  keys << "adaptivity";
  keys << "downsample";
  keys << "smooth data";
  keys << "mesh smoothing";
  keys << "morpho operator";
  keys << "morpho radius";
  keys << "apply voxel size";
  keys << "commandhelp";
  //keys << "message";

  
  propertyEditor.set("Mesh Generation Parameters", plist, keys);
  propertyEditor.resize(700, 400);

  QMap<QString, QPair<QVariant, bool> > vmap;
  
  if (propertyEditor.exec() == QDialog::Accepted)
    vmap = propertyEditor.get();
  else
    return false;
  
  for(int ik=0; ik<keys.count(); ik++)
    {
      QPair<QVariant, bool> pair = vmap.value(keys[ik]);

      if (pair.second)
	{
	  if (keys[ik] == "isosurface value")
	    isoValue = pair.first.toFloat();
	  else if (keys[ik] == "adaptivity")
	    adaptivity = pair.first.toFloat();
	  else if (keys[ik] == "downsample")
	    resample = pair.first.toFloat();
	  else if (keys[ik] == "smooth data")
	    dataSmooth = pair.first.toInt();
	  else if (keys[ik] == "mesh smoothing")
	    meshSmooth = pair.first.toInt();
	  else if (keys[ik] == "morpho operator")
	    morphoType = pair.first.toInt();
	  else if (keys[ik] == "morpho radius")
	    morphoRadius = pair.first.toInt();
	  else if (keys[ik] == "apply voxel size")
	    applyVoxelScaling = pair.first.toBool();
	}
    }

  return true;
}
