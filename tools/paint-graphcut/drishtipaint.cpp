#include <GL/glew.h>

#include "drishtipaint.h"
#include "global.h"
#include "staticfunctions.h"
#include "showhelp.h"
#include "dcolordialog.h"
#include "morphslice.h"

#include <QDockWidget>
#include <QFileDialog>
#include <QInputDialog>
#include <QScrollArea>

#include <exception>
#include "volumeinformation.h"

void
DrishtiPaint::initTagColors()
{
  uchar *colors;
  colors = new uchar[1024];
  memset(colors, 0, 1024);

  qsrand(1);

  for(int i=1; i<256; i++)
    {
      float r,g,b,a;
      if (i < 255)
	{
	  r = (float)qrand()/(float)RAND_MAX;
	  g = (float)qrand()/(float)RAND_MAX;
	  b = (float)qrand()/(float)RAND_MAX;
	  a = 0.5f;
	}
      else
	{
	  r = 0.9f; g = 0.3f; b = 0.2f; a = 1.0f;
	}
      colors[4*i+0] = 255*r;
      colors[4*i+1] = 255*g;
      colors[4*i+2] = 255*b;
      colors[4*i+3] = 255*a;
    }
  
  Global::setTagColors(colors);
  delete [] colors;
}

DrishtiPaint::DrishtiPaint(QWidget *parent) :
  QMainWindow(parent)
{
  ui.setupUi(this);
  ui.statusbar->setEnabled(true);
  ui.statusbar->setSizeGripEnabled(true);

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

  ui.actionFibers->setStyleSheet("QPushButton:checked { background-color: #ff7700; }");
  ui.actionCurves->setStyleSheet("QPushButton:checked { background-color: #0077dd; }");
  ui.actionGraphCut->setStyleSheet("QPushButton:checked { background-color: #00aa55; }");

  ui.help->setMaximumSize(50, 50);
  ui.help->setMinimumSize(50, 50);

  m_fibersMenu = new QFrame();
  fibersUi.setupUi(m_fibersMenu);

  ui.sideframelayout->addWidget(m_graphcutMenu);
  ui.sideframelayout->addWidget(m_curvesMenu);
  ui.sideframelayout->addWidget(m_fibersMenu);


  m_tagColorEditor = new TagColorEditor();

  m_viewer = new Viewer();

  //------------------------------
  // viewer menu
  QFrame *viewerMenu = new QFrame();
  viewerUi.setupUi(viewerMenu);
  connectViewerMenu();
  //------------------------------

  //------------------------------
  connect(m_viewer, SIGNAL(paint3DStart()),
	  this, SLOT(paint3DStart()));
  connect(m_viewer, SIGNAL(paint3D(int,int,int,int,int)),
	  this, SLOT(paint3D(int,int,int,int,int)));
  connect(m_viewer, SIGNAL(paint3DEnd()),
	  this, SLOT(paint3DEnd()));

  connect(m_viewer, SIGNAL(connectedRegion(int,int,int,Vec,Vec,int)),
	  this, SLOT(connectedRegion(int,int,int,Vec,Vec,int)));

  connect(m_viewer, SIGNAL(mergeTags(Vec, Vec, int, int, bool)),
	  this, SLOT(mergeTags(Vec, Vec, int, int, bool)));

  connect(m_viewer, SIGNAL(dilateConnected(int,int,int,Vec,Vec,int)),
	  this, SLOT(dilateConnected(int,int,int,Vec,Vec,int)));

  connect(m_viewer, SIGNAL(erodeConnected(int,int,int,Vec,Vec,int)),
	  this, SLOT(erodeConnected(int,int,int,Vec,Vec,int)));

  connect(m_viewer, SIGNAL(tagUsingSketchPad(Vec,Vec)),
	  this, SLOT(tagUsingSketchPad(Vec,Vec)));

  connect(m_viewer, SIGNAL(setVisible(Vec, Vec, int, bool)),
	  this, SLOT(setVisible(Vec, Vec, int, bool)));

  connect(m_viewer, SIGNAL(resetTag(Vec, Vec, int)),
	  this, SLOT(resetTag(Vec, Vec, int)));

  connect(m_viewer, SIGNAL(updateSliceBounds(Vec, Vec)),
	  this, SLOT(updateSliceBounds(Vec, Vec)));

  connect(m_viewer, SIGNAL(shrinkwrap(Vec, Vec, int, bool, int)),
	  this, SLOT(shrinkwrap(Vec, Vec, int, bool, int)));

  connect(m_viewer, SIGNAL(shrinkwrap(Vec, Vec, int, bool, int,
				      bool, int, int, int, int)),
	  this, SLOT(shrinkwrap(Vec, Vec, int, bool, int,
				bool, int, int, int, int)));

  connect(m_viewer, SIGNAL(getVolume(Vec, Vec, int)),
	  this, SLOT(getVolume(Vec, Vec, int)));

  //------------------------------


  //----------------------------------------------------------
  QDockWidget *dock1 = new QDockWidget("Transfer Function Editor", this);
  {
    dock1->setAllowedAreas(Qt::LeftDockWidgetArea | 
			   Qt::RightDockWidgetArea);
    QSplitter *splitter = new QSplitter(Qt::Vertical, dock1);
    splitter->addWidget(m_tfManager);
    splitter->addWidget(m_tfEditor);
    dock1->setWidget(splitter);
  }
  //----------------------------------------------------------

  //----------------------------------------------------------
    QDockWidget *dockV = new QDockWidget("3D Preview", this);
  {
    dock1->setAllowedAreas(Qt::LeftDockWidgetArea | 
			   Qt::RightDockWidgetArea);
    QSplitter *splitter = new QSplitter(Qt::Horizontal, dockV);
    splitter->addWidget(viewerMenu);
    splitter->addWidget(m_viewer);
    dockV->setWidget(splitter);
  }
  //----------------------------------------------------------

  //----------------------------------------------------------
  QDockWidget *dock2 = new QDockWidget("Tag Color Editor", this);
  {
    dock2->setAllowedAreas(Qt::LeftDockWidgetArea | 
			   Qt::RightDockWidgetArea);
    dock2->setWidget(m_tagColorEditor);
    dock2->hide();
  }
  //----------------------------------------------------------

  addDockWidget(Qt::RightDockWidgetArea, dockV);
  addDockWidget(Qt::RightDockWidgetArea, dock1, Qt::Horizontal);
  addDockWidget(Qt::LeftDockWidgetArea, dock2);


  m_volume = new Volume();

  m_slider = new MySlider();
  QVBoxLayout *slout = new QVBoxLayout();
  slout->setContentsMargins(0,0,0,0);
  ui.sliderFrame->setLayout(slout);
  ui.sliderFrame->layout()->addWidget(m_slider);


  m_imageWidget = new ImageWidget(this, ui.statusbar);

  QScrollArea *scrollArea = new QScrollArea;
  scrollArea->setBackgroundRole(QPalette::Dark);
  scrollArea->setWidget(m_imageWidget);
  m_imageWidget->setScrollBars(scrollArea->horizontalScrollBar(),
			       scrollArea->verticalScrollBar());

  QVBoxLayout *layout1 = new QVBoxLayout();
  layout1->setContentsMargins(0,0,0,0);
  ui.imageFrame->setLayout(layout1);
  ui.imageFrame->layout()->addWidget(scrollArea);


  ui.menuView->addAction(dock1->toggleViewAction());
  ui.menuView->addAction(dockV->toggleViewAction());
  ui.menuView->addAction(dock2->toggleViewAction());
  
  on_actionGraphCut_clicked(true);

  //on_actionCurves_clicked(true);
  curvesUi.lwsettingpanel->setVisible(false);
  curvesUi.closed->setChecked(true);

  Global::setBoxSize(5);
  Global::setSpread(10);
  Global::setLambda(10);
  Global::setSmooth(1);
  Global::setPrevErode(5);

  fibersUi.endfiber->hide();
  curvesUi.endcurve->hide();
  curvesUi.pointsize->setValue(7);
  curvesUi.mincurvelen->setValue(20);
  ui.tag->setValue(Global::tag());
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
  connectCurvesMenu();
  connectGraphCutMenu();
  connectFibersMenu();
  //------------------------

  loadSettings();
  m_imageWidget->updateTagColors();

  setGeometry(100, 100, 700, 700);
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
  if (m_fibersMenu->isVisible())
    {
      ShowHelp::showFibersHelp();
      return;
    }
}

void
DrishtiPaint::on_actionAltLayout_triggered()
{
  QLayoutItem *item0 = ui.horizontalLayout_2->itemAt(0);
  QLayoutItem *item1 = ui.horizontalLayout_2->itemAt(1);
  QLayoutItem *item2 = ui.horizontalLayout_2->itemAt(2);
  
  ui.horizontalLayout_2->removeItem(item0);
  ui.horizontalLayout_2->removeItem(item1);
  ui.horizontalLayout_2->removeItem(item2);

  ui.horizontalLayout_2->insertItem(0, item2);
  ui.horizontalLayout_2->insertItem(1, item1);
  ui.horizontalLayout_2->insertItem(2, item0);
}


void DrishtiPaint::on_saveImage_triggered() { m_imageWidget->saveImage(); }
void
DrishtiPaint::on_saveWork_triggered()
{
  if (m_volume->isValid())
    {
      QString curvesfile = m_pvlFile;
      curvesfile.replace(".pvl.nc", ".curves");
      m_imageWidget->saveCurves(curvesfile);

      m_volume->saveIntermediateResults(true);

      QMessageBox::information(0, "Save Work", "Tags, Curves and Fibers saved");
    }
}
void
DrishtiPaint::saveWork()
{
  if (m_volume->isValid())
    {
      QString curvesfile = m_pvlFile;
      curvesfile.replace(".pvl.nc", ".curves");
      m_imageWidget->saveCurves(curvesfile);
      
      m_volume->saveIntermediateResults();
    }
}

void
DrishtiPaint::on_actionFibers_clicked(bool b)
{
  QString hss;
  hss += "QToolButton { border-width:5; border-style:solid; border-radius:25px;";
  hss += "color:#ff7700; }";
  ui.help->setStyleSheet(hss);

  if (m_fibersMenu->isVisible() && !b)
    {
      ui.actionFibers->setChecked(true);  
      return;
    }
    
  m_fibersMenu->show();
  m_curvesMenu->hide();
  m_graphcutMenu->hide();
  ui.wdptszframe->show();
  ui.spreadframe->hide();

  ui.actionCurves->setChecked(false);
  ui.actionGraphCut->setChecked(false);
  ui.actionFibers->setChecked(true);  

  m_imageWidget->setCurve(false);
  curvesUi.livewire->setChecked(false);
  curvesUi.modify->setChecked(false);
  curvesUi.propagate->setChecked(false);
  m_imageWidget->freezeModifyUsingLivewire();

  m_imageWidget->setFiberMode(true);
}

void
DrishtiPaint::on_actionCurves_clicked(bool b)
{
  QString hss;
  hss += "QToolButton { border-width:5; border-style:solid; border-radius:25px;";
  hss += "color:#0077dd; }";
  ui.help->setStyleSheet(hss);

  if (m_curvesMenu->isVisible() && !b)
    {
      ui.actionCurves->setChecked(true);  
      return;
    }

  m_curvesMenu->show();
  m_graphcutMenu->hide();
  m_fibersMenu->hide();
  ui.wdptszframe->show();
  ui.spreadframe->hide();

  ui.actionCurves->setChecked(true);
  ui.actionGraphCut->setChecked(false);
  ui.actionFibers->setChecked(false);  

  m_imageWidget->setFiberMode(false);
  m_imageWidget->setCurve(true);
 
  if (m_volume->isValid())
    {
      curvesUi.livewire->setChecked(true);
      m_imageWidget->setLivewire(true);
    }

  m_imageWidget->endFiber();
}

void
DrishtiPaint::on_actionGraphCut_clicked(bool b)
{
  QString hss;
  hss += "QToolButton { border-width:5; border-style:solid; border-radius:25px;";
  hss += "color:#00aa55; }";
  ui.help->setStyleSheet(hss);


  if (m_graphcutMenu->isVisible() && !b)
    {
      ui.actionGraphCut->setChecked(true);  
      return;
    }

  m_curvesMenu->hide();
  m_graphcutMenu->show();
  m_fibersMenu->hide();
  ui.wdptszframe->hide();
  ui.spreadframe->show();

  ui.actionCurves->setChecked(false);
  ui.actionGraphCut->setChecked(true);
  ui.actionFibers->setChecked(false);

  m_imageWidget->setFiberMode(false);
  m_imageWidget->setCurve(false);
  curvesUi.livewire->setChecked(false);

  curvesUi.modify->setChecked(false);
  curvesUi.propagate->setChecked(false);
  m_imageWidget->freezeModifyUsingLivewire();

  m_imageWidget->endFiber();
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

  m_imageWidget->setBox(minD, maxD, minW, maxW, minH, maxH);

  if (ui.butZ->isChecked()) on_butZ_clicked();
  else if (ui.butY->isChecked()) on_butY_clicked();
  else if (ui.butX->isChecked()) on_butX_clicked();
}

void
DrishtiPaint::getSlice(int slc)
{
  m_currSlice = slc;

  m_slider->setValue(slc);

  uchar *slice;
  uchar *maskslice;

  if (ui.butZ->isChecked())
    {
      slice = m_volume->getDepthSliceImage(m_currSlice);
      maskslice = m_volume->getMaskDepthSliceImage(m_currSlice);
      m_viewer->updateCurrSlice(0, m_currSlice);  
    }
  else if (ui.butY->isChecked())
    {
      slice = m_volume->getWidthSliceImage(m_currSlice); 
      maskslice = m_volume->getMaskWidthSliceImage(m_currSlice);
      m_viewer->updateCurrSlice(1, m_currSlice);  
    }
  else if (ui.butX->isChecked())
    {
      slice = m_volume->getHeightSliceImage(m_currSlice); 
      maskslice = m_volume->getMaskHeightSliceImage(m_currSlice);
      m_viewer->updateCurrSlice(2, m_currSlice);  
    }

  m_imageWidget->setImage(slice, maskslice);
}

void
DrishtiPaint::getMaskSlice(int slc)
{
  m_currSlice = slc;

  m_slider->setValue(slc);

  uchar *maskslice;

  if (ui.butZ->isChecked())
    maskslice = m_volume->getMaskDepthSliceImage(m_currSlice);
  else if (ui.butY->isChecked())
    maskslice = m_volume->getMaskWidthSliceImage(m_currSlice);
  else if (ui.butX->isChecked())
    maskslice = m_volume->getMaskHeightSliceImage(m_currSlice);

  m_imageWidget->setMaskImage(maskslice);
}

void
DrishtiPaint::tagSelected(int t)
{
  Global::setTag(t);
  ui.tag->setValue(t);

  m_imageWidget->updateTagColors();
}

void
DrishtiPaint::on_saveFreq_valueChanged(int t)
{
  m_volume->setSaveFrequency(t);
}

void
DrishtiPaint::on_tag_valueChanged(int t)
{
  Global::setTag(t);
  m_imageWidget->processPrevSliceTags();
}
void DrishtiPaint::on_sliceLod_currentIndexChanged(int l) { m_imageWidget->setSliceLOD(l+1); }
void DrishtiPaint::on_boxSize_valueChanged(int d) { Global::setBoxSize(d); }
void DrishtiPaint::on_lambda_valueChanged(int d) { Global::setLambda(d); }
void DrishtiPaint::on_smooth_valueChanged(int d) { Global::setSmooth(d); }
void DrishtiPaint::on_thickness_valueChanged(int d) { Global::setThickness(d); }
void DrishtiPaint::on_radius_valueChanged(int d) { Global::setSpread(d); m_imageWidget->update(); }
void DrishtiPaint::on_pointsize_valueChanged(int d) { m_imageWidget->setPointSize(d); }
void DrishtiPaint::on_mincurvelen_valueChanged(int d) { m_imageWidget->setMinCurveLength(d); }
void DrishtiPaint::on_closed_clicked(bool c) { Global::setClosed(c); }
void DrishtiPaint::on_lwsmooth_currentIndexChanged(int i){ m_imageWidget->setSmoothType(i); }
void DrishtiPaint::on_lwgrad_currentIndexChanged(int i){ m_imageWidget->setGradType(i); }
void DrishtiPaint::on_newcurve_clicked()
{
  on_livewire_clicked(false);
  curvesUi.livewire->setChecked(false);
  m_imageWidget->newCurve(true);
}
void DrishtiPaint::on_endcurve_clicked() { m_imageWidget->endCurve(); }
void DrishtiPaint::on_newfiber_clicked() { m_imageWidget->newFiber(); }
void DrishtiPaint::on_endfiber_clicked() { m_imageWidget->endFiber(); }
void DrishtiPaint::on_morphcurves_clicked() { m_imageWidget->morphCurves(); }
void DrishtiPaint::on_deselect_clicked() { m_imageWidget->deselectAll(); }
void DrishtiPaint::on_deleteallcurves_clicked() { m_imageWidget->deleteAllCurves(); }
void DrishtiPaint::on_zoom0_clicked() { m_imageWidget->zoom0(); }
void DrishtiPaint::on_zoom9_clicked() { m_imageWidget->zoom9(); }
void DrishtiPaint::on_zoomup_clicked() { m_imageWidget->zoomUp(); }
void DrishtiPaint::on_zoomdown_clicked() { m_imageWidget->zoomDown(); }

QPair<QString, QList<int> >
DrishtiPaint::getTags(QString text)
{
  QStringList tglist = text.split(" ", QString::SkipEmptyParts);
  QList<int> tag;
  tag.clear();
  for(int i=0; i<tglist.count(); i++)
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

  QString tgstr;
  for(int i=0; i<tag.count(); i++)
    tgstr += QString("%1 ").arg(tag[i]);

  return qMakePair(tgstr, tag);
}

void
DrishtiPaint::on_tagcurves_editingFinished()
{
  QString text = ui.tagcurves->text();

  QPair<QString, QList<int> > tags;
  tags = getTags(text);

  ui.tagcurves->setText(tags.first);
  
  m_imageWidget->showTags(tags.second);
}

void
DrishtiPaint::curvetag_editingFinished()
{
  QString text = viewerUi.curvetags->text();

  QPair<QString, QList<int> > tags;
  tags = getTags(text);

  viewerUi.curvetags->setText(tags.first);
  
  m_viewer->setCurveTags(tags.second);
}

void
DrishtiPaint::fibertag_editingFinished()
{
  QString text = viewerUi.fibertags->text();

  QPair<QString, QList<int> > tags;
  tags = getTags(text);

  viewerUi.fibertags->setText(tags.first);
  
  m_viewer->setFiberTags(tags.second);
}

void
DrishtiPaint::on_livewire_clicked(bool c)
{
  if (!c)
    {
      if (!m_imageWidget->seedMoveMode())
	m_imageWidget->freezeLivewire(false);
      else
	m_imageWidget->freezeModifyUsingLivewire();
      curvesUi.modify->setChecked(false);
    }

  m_imageWidget->setLivewire(c);
}

void
DrishtiPaint::on_propagate_clicked(bool c)
{
  m_imageWidget->propagateCurves(c);
}

void
DrishtiPaint::on_modify_clicked(bool c)
{
  if (c)
    {
      curvesUi.livewire->setChecked(true);
      m_imageWidget->setLivewire(true);
      m_imageWidget->modifyUsingLivewire();
    }
  else
    m_imageWidget->freezeModifyUsingLivewire();

  update();
}
void
DrishtiPaint::on_copyprev_clicked(bool c)
{
  Global::setCopyPrev(c);
  m_imageWidget->processPrevSliceTags();
}
void
DrishtiPaint::on_preverode_valueChanged(int t)
{
  Global::setPrevErode(t);
  m_imageWidget->processPrevSliceTags();
}

void
DrishtiPaint::on_butZ_clicked()
{
  if (! m_volume->isValid()) return;

  m_imageWidget->setSliceType(ImageWidget::DSlice);

  int d, w, h, u0, u1;
  m_volume->gridSize(d, w, h);
  m_imageWidget->depthUserRange(u0, u1);

  QVector3D pp = m_imageWidget->pickedPoint();
  int s = qBound(u0, (int)pp.x(), u1);
  m_slider->set(0, d-1, u0, u1, 0);
  m_imageWidget->sliceChanged(s);
}
void
DrishtiPaint::on_butY_clicked()
{
  if (! m_volume->isValid()) return;

  m_imageWidget->setSliceType(ImageWidget::WSlice);

  int d, w, h, u0, u1;
  m_volume->gridSize(d, w, h);
  m_imageWidget->widthUserRange(u0, u1);

  QVector3D pp = m_imageWidget->pickedPoint();
  int s = qBound(u0, (int)pp.y(), u1);
  m_slider->set(0, w-1, u0, u1, 0);
  m_imageWidget->sliceChanged(s);
}
void
DrishtiPaint::on_butX_clicked()
{
  if (! m_volume->isValid()) return;

  m_imageWidget->setSliceType(ImageWidget::HSlice);

  int d, w, h, u0, u1;
  m_volume->gridSize(d, w, h);
  m_imageWidget->heightUserRange(u0, u1);

  QVector3D pp = m_imageWidget->pickedPoint();
  int s = qBound(u0, (int)pp.z(), u1);
  m_slider->set(0, h-1, u0, u1, 0);
  m_imageWidget->sliceChanged(s);
}


void
DrishtiPaint::getRawValue(int d, int w, int h)
{
  if (! m_volume->isValid()) return;

  m_imageWidget->setRawValue(m_volume->rawValue(d, w, h));
}

void DrishtiPaint::on_actionLoad_Curves_triggered() {m_imageWidget->loadCurves();}
void DrishtiPaint::on_actionSave_Curves_triggered() {m_imageWidget->saveCurves();}

void DrishtiPaint::on_actionLoad_Fibers_triggered() {m_imageWidget->loadFibers();}
void DrishtiPaint::on_actionSave_Fibers_triggered() {m_imageWidget->saveFibers();}

void
DrishtiPaint::on_actionLoad_triggered()
{
  QString flnm;
  flnm = QFileDialog::getOpenFileName(0,
				      "Load Processed Volume File",
				      Global::previousDirectory(),
				      "PVL Files (*.pvl.nc)",
				      0,
				      QFileDialog::DontUseNativeDialog);

  
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
      m_imageWidget->saveCurves(curvesfile);
    }

  m_viewer->init();
  m_volume->reset();

  m_viewer->close();

  saveSettings();
  close();
}
void
DrishtiPaint::closeEvent(QCloseEvent *)
{
  on_actionExit_triggered();
}


void
DrishtiPaint::updateComposite()
{
  QImage colorMap = m_tfContainer->composite(0);
  m_imageWidget->loadLookupTable(colorMap);
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
	      StaticFunctions::checkURLs(urls, ".fibers"))
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
		  m_imageWidget->loadCurves(flnm);
		}
	      else if (StaticFunctions::checkExtension(url.toLocalFile(), ".fibers"))
		{
		  QString flnm = (data->urls())[0].toLocalFile();
		  m_imageWidget->loadFibers(flnm);
		}
	      else if (StaticFunctions::checkExtension(url.toLocalFile(), ".pvl.nc") ||
		  StaticFunctions::checkExtension(url.toLocalFile(), ".xml"))
		{
		  QString flnm = (data->urls())[0].toLocalFile();
		  setFile(flnm);
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
      m_imageWidget->saveCurves(curvesfile);
    }

  m_blockList.clear();

  QString flnm;

  if (StaticFunctions::checkExtension(filename, ".pvl.nc"))
    {
      flnm = filename;
      m_tfEditor->setTransferFunction(NULL);
      m_tfManager->clearManager();
    }
  else
    {
      flnm = loadVolumeFromProject(filename.toLatin1().data());

      if (flnm.isEmpty())
	{
	  QMessageBox::critical(0, "Error",
				"No volumes found in the project");
	  return;
	}
    }

  m_volume->reset();
  m_imageWidget->setGridSize(0,0,0);
  m_imageWidget->resetCurves();


  //----------------------------
  // save volume information from .pvl.nc file
  VolumeInformation pvlinfo;
  VolumeInformation::volInfo(flnm.toLatin1().data(),
			     pvlinfo);
  VolumeInformation::setVolumeInformation(pvlinfo);
  //----------------------------

  if (m_volume->setFile(flnm) == false)
    {
      QMessageBox::critical(0, "Error", "Cannot load "+flnm);
      return;
    }

  
  m_viewer->setVolDataPtr(m_volume->memVolDataPtr());
  m_viewer->setMaskDataPtr(m_volume->memMaskDataPtr());

  int d, w, h;
  m_volume->gridSize(d, w, h);
  m_imageWidget->setGridSize(d, w, h);
  m_viewer->setGridSize(d, w, h);

  m_viewer->setMultiMapCurves(0, m_imageWidget->multiMapCurvesD());
  m_viewer->setMultiMapCurves(1, m_imageWidget->multiMapCurvesW());
  m_viewer->setMultiMapCurves(2, m_imageWidget->multiMapCurvesH());

  m_viewer->setListMapCurves(0, m_imageWidget->morphedCurvesD());
  m_viewer->setListMapCurves(1, m_imageWidget->morphedCurvesW());
  m_viewer->setListMapCurves(2, m_imageWidget->morphedCurvesH());

  m_viewer->setShrinkwrapCurves(0, m_imageWidget->shrinkwrapCurvesD());
  m_viewer->setShrinkwrapCurves(1, m_imageWidget->shrinkwrapCurvesW());
  m_viewer->setShrinkwrapCurves(2, m_imageWidget->shrinkwrapCurvesH());

  m_viewer->setFibers(m_imageWidget->fibers());

  ui.butZ->setChecked(true);
  m_slider->set(0, d-1, 0, d-1, 0);

  m_imageWidget->setSliceType(ImageWidget::DSlice);

  m_tfManager->setDisabled(false);

  m_tfEditor->setHistogramImage(m_volume->histogramImage1D(),
				m_volume->histogramImage2D());

  m_tfManager->addNewTransferFunction();

  QFileInfo f(filename);
  Global::setPreviousDirectory(f.absolutePath());
  Global::addRecentFile(filename);
  updateRecentFileAction();

      
  if (StaticFunctions::checkExtension(filename, ".xml"))
    {
      m_tfManager->load(filename.toLatin1().data());
      m_pvlFile = flnm;
      m_xmlFile = filename;
    }
  else
    {
      m_pvlFile = flnm;
      m_xmlFile.clear();
    }

  QString curvesfile = m_pvlFile;
  curvesfile.replace(".pvl.nc", ".curves");
  m_imageWidget->loadCurves(curvesfile);

  on_actionGraphCut_clicked(true);

  //on_actionCurves_clicked(true);
  //curvesUi.lwsettingpanel->setVisible(false);
  //curvesUi.closed->setChecked(true);
  //curvesUi.livewire->setChecked(true);
  //m_imageWidget->setLivewire(true);

  ui.tagcurves->setText("-1");
  viewerUi.fibertags->setText("-1");
  viewerUi.curvetags->setText("-1");

  viewerUi.dragStep->setValue(m_viewer->dragStep());
  viewerUi.stillStep->setValue(m_viewer->stillStep());

  viewerUi.sketchPad->setChecked(false);
  m_viewer->showSketchPad(false);

  m_viewDslice->setRange(0, d-1);
  m_viewWslice->setRange(0, w-1);
  m_viewHslice->setRange(0, h-1);

  Global::setVoxelScaling(StaticFunctions::getVoxelSizeFromHeader(m_pvlFile));
  Global::setVoxelUnit(StaticFunctions::getVoxelUnitFromHeader(m_pvlFile));
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
  QFile f(flnm.toLatin1().data());
  if (f.open(QIODevice::ReadOnly))
    {
      document.setContent(&f);
      f.close();
    }

  QDomElement main = document.documentElement();
  QDomNodeList dlist = main.childNodes();
  for(int i=0; i<dlist.count(); i++)
    {
      if (dlist.at(i).nodeName() == "previousdirectory")
	{
	  QString str = dlist.at(i).toElement().text();
	  Global::setPreviousDirectory(str);
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
	  for(int i=0; i<qMin(256, col.size()); i++)
	    {
	      QStringList clr = col[i].split(" ",
					     QString::SkipEmptyParts);
	      colors[4*i+0] = clr[0].toInt();
	      colors[4*i+1] = clr[1].toInt();
	      colors[4*i+2] = clr[2].toInt();
	      //colors[4*i+3] = clr[3].toInt();
	      colors[4*i+3] = 255;
	    }
	  Global::setTagColors(colors);
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
    QDomElement de0 = doc.createElement("previousdirectory");
    QDomText tn0;
    tn0 = doc.createTextNode(Global::previousDirectory());
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
    for(int i=0; i<256; i++)
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

  QFile f(flnm.toLatin1().data());
  if (f.open(QIODevice::WriteOnly))
    {
      QTextStream out(&f);
      doc.save(out, 2);
      f.close();
    }
  else
    QMessageBox::information(0, "Cannot save ", flnm.toLatin1().data());
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

void
DrishtiPaint::keyPressEvent(QKeyEvent *event)
{
  if (event->key() == Qt::Key_S &&
      (event->modifiers() & Qt::ControlModifier) )
    {
      on_saveWork_triggered();
      return;
    } 

  // pass all keypressevents on to image widget
  m_imageWidget->keyPressEvent(event);
}

// smooth dilate/erode based on threshold
void
DrishtiPaint::sliceSmooth(int tag, int spread,
			  uchar *pv, uchar *p,
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
		     uchar **pv, uchar *p,
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
DrishtiPaint::sliceDilate(int tag, int spread,
			  uchar *pv, uchar *p,
			  int width, int height)
{ 
  int a;
  for(int i=0; i<height; i++)				  
    for(int j=0; j<width; j++)			  
      {						  
	int pj = 0;					  
	int jst = qMax(0, j-spread);			  
	int jed = qMin(width-1, j+spread);			  
	for(int j1=jst; j1<=jed; j1++)		  
	  {						  
	    int idx = qBound(0, j1, width-1)*height+i;  
	    if (tag > -1)
	      a = (pv[idx]==tag ? 1 : 0);
	    else
	      a = (pv[idx]>tag ? 1 : 0);
	    pj = qMax(a,pj);
	  }
	if (tag > -1)
	  p[j*height+i] = tag*pj;
	else
	  p[j*height+i] = 255*pj;
      }						  
  
  for(int j=0; j<width; j++)				  
    for(int i=0; i<height; i++)			  
      {						  
	int pi = 0;					  
	int ist = qMax(0, i-spread);			  
	int ied = qMin(height-1, i+spread);		  
	for(int i1=ist; i1<=ied; i1++)		  
	  {						  
	    int idx = j*height+qBound(0, i1, height-1); 
	    if (tag > -1)
	      a = (p[idx]==tag ? 1 : 0);
	    else
	      a = (p[idx]>tag ? 1 : 0);
	    pi = qMax(a,pi);		  
	  }						  
	if (tag > -1)
	  pv[j*height+i] = tag*pi;
	else
	  pv[j*height+i] = 255*pi;
      }						  
}

void
DrishtiPaint::dilate(int tag, int spread,
		     uchar **pv, uchar *p,
		     int width, int height)
{ 
  int a;
  for(int j=0; j<width; j++) 
    for(int k=0; k<height; k++) 
      {
	int avg = 0; 
	for(int i=0; i<2*spread+1; i++) 
	  {
	    if (tag > -1)
	      a = (pv[i][j*height+k]==tag ? 1 : 0);
	    else
	      a = (pv[i][j*height+k]>tag ? 1 : 0);
	    avg = qMax(avg,a); 
	  }
	if (tag > -1)
	  p[j*height + k] = tag*avg;
	else
	  p[j*height + k] = 255*avg;
      } 
}

void
DrishtiPaint::sliceErode(int tag, int spread,
			 uchar *pv, uchar *p,
			 int width, int height)
{ 
  int a;
  for(int i=0; i<height; i++)				  
    for(int j=0; j<width; j++)			  
      {						  
	int pj = 1;					  
	int jst = qMax(0, j-spread);			  
	int jed = qMin(width-1, j+spread);			  
	for(int j1=jst; j1<=jed; j1++)		  
	  {						  
	    int idx = qBound(0, j1, width-1)*height+i;  
	    if (tag > -1)
	      a = (pv[idx]==tag ? 1 : 0);
	    else
	      a = (pv[idx]>tag ? 1 : 0);
	    pj = qMin(a,pj);
	  }
	if (tag > -1)
	  p[j*height+i] = tag*pj;
	else
	  p[j*height+i] = 255*pj;
      }						  
  
  for(int j=0; j<width; j++)				  
    for(int i=0; i<height; i++)			  
      {						  
	int pi = 1;					  
	int ist = qMax(0, i-spread);			  
	int ied = qMin(height-1, i+spread);		  
	for(int i1=ist; i1<=ied; i1++)		  
	  {						  
	    int idx = j*height+qBound(0, i1, height-1); 
	    if (tag > -1)
	      a = (p[idx]==tag ? 1 : 0);
	    else
	      a = (p[idx]>tag ? 1 : 0);
	    pi = qMin(a,pi);		  
	  }						  
	if (tag > -1)
	  pv[j*height+i] = tag*pi;
	else
	  pv[j*height+i] = 255*pi;
      }						  
}

void
DrishtiPaint::erode(int tag, int spread,
		    uchar **pv, uchar *p,
		    int width, int height)
{ 
  int a;
  for(int j=0; j<width; j++) 
    for(int k=0; k<height; k++) 
      {
	int avg = 1; 
	for(int i=0; i<2*spread+1; i++) 
	  {
	    if (tag > -1)
	      a = (pv[i][j*height+k]==tag ? 1 : 0);
	    else
	      a = (pv[i][j*height+k]>tag ? 1 : 0);
	    avg = qMin(avg,a); 
	  }
	if (tag > -1)
	  p[j*height + k] = tag*avg;
	else
	  p[j*height + k] = 255*avg;
      } 
}

void
DrishtiPaint::savePvlHeader(QString volfile,
			    QString pvlfile,
			    int d, int w, int h,
			    bool saveImageData)
{  
  QString rawFile;
  QString description;
  QString voxelSize;
  QString voxelType;
  QString voxelUnit;
  QString pvlmap;
  QString rawmap;

  QDomDocument document;
  QFile f(volfile.toLatin1().data());
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
      else if (dlist.at(i).nodeName() == "voxeltype")
	voxelType = dlist.at(i).toElement().text();
      else if (dlist.at(i).nodeName() == "voxelunit")
	voxelUnit = dlist.at(i).toElement().text();
      else if (dlist.at(i).nodeName() == "voxelsize")
	voxelSize = dlist.at(i).toElement().text();
      else if (dlist.at(i).nodeName() == "rawmap")
	rawmap = dlist.at(i).toElement().text();
      else if (dlist.at(i).nodeName() == "pvlmap")
	pvlmap = dlist.at(i).toElement().text();
    }

  if (!saveImageData)
    {
      rawFile = "";
      description = "tag data";
      voxelType = "unsigned char";
      voxelUnit = "no units";
      rawmap = "1 255";
      pvlmap = "1 255";
    }

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
    QDomElement de0 = doc.createElement("description");
    QDomText tn0;
    tn0 = doc.createTextNode(description);
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
    QDomElement de0 = doc.createElement("gridsize");
    QDomText tn0;
    tn0 = doc.createTextNode(QString("%1 %2 %3").arg(d).arg(w).arg(h));
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
      
  QFile pf(pvlfile.toLatin1().data());
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
  m_imageWidget->getBox(minDSlice, maxDSlice,
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
			   0);
  progress.setMinimumDuration(0);

  int nbytes = width*height;
  uchar *tagData = new uchar[nbytes];
  uchar *raw = new uchar[nbytes];
  uchar **val;
  val = new uchar*[2*spread+1];
  for (int i=0; i<2*spread+1; i++)
    val[i] = new uchar[nbytes];

  for(int d=minDSlice; d<=maxDSlice; d++)
    {
      int slc = d-minDSlice;
      progress.setValue((int)(100*(float)slc/(float)tdepth));
      qApp->processEvents();

      uchar *slice = m_volume->getDepthSliceImage(d);
      // we get value+grad from volume
      // we need only value part
      int i=0;
      for(int w=minWSlice; w<=maxWSlice; w++)
	for(int h=minHSlice; h<=maxHSlice; h++)
	  {
	    slice[i] = slice[2*(w*height+h)];
	    i++;
	  }

      memcpy(tagData, m_volume->getMaskDepthSliceImage(d), nbytes);

      if (slc == 0)
	{
	  memcpy(val[spread], m_volume->getMaskDepthSliceImage(d), nbytes);

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
		memcpy(val[spread+i], m_volume->getMaskDepthSliceImage(d+i), nbytes);
	      else
		memcpy(val[spread+i], m_volume->getMaskDepthSliceImage(0), nbytes);
	      
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
		memcpy(val[spread+i], m_volume->getMaskDepthSliceImage(d+i), nbytes);
	      else
		memcpy(val[spread+i], m_volume->getMaskDepthSliceImage(depth-1), nbytes);
	      
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
	}
      else if (d < depth-spread)
	{
	  memcpy(val[2*spread], m_volume->getMaskDepthSliceImage(d+spread), nbytes);
	  
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
	}		  
      else
	{
	  memcpy(val[2*spread], m_volume->getMaskDepthSliceImage(depth-1), nbytes);
	  
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
	}		  
      
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
      uchar *tmp = val[0];
      for(int i=0; i<2*spread; i++)
	val[i] = val[i+1];
      val[2*spread] = tmp;
  
      for(int w=minWSlice; w<=maxWSlice; w++)
	for(int h=minHSlice; h<=maxHSlice; h++)
	  {
	    if (tagData[w*height+h] == 0 || tagData[w*height+h] == tag)
	      tagData[w*height+h] = raw[w*height+h];
	  }
      m_volume->setMaskDepthSlice(d, tagData);
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

  getSlice(m_slider->value());
}


void
DrishtiPaint::on_pointRender_clicked(bool flag)
{  
  viewerUi.raycastParam->setVisible(!flag);
  viewerUi.lightBox->setVisible(!flag);

  m_viewer->setPointRender(flag);
  viewerUi.pointParam->setVisible(flag);
}

void
DrishtiPaint::on_raycastRender_clicked(bool flag)
{
  viewerUi.pointParam->setVisible(!flag);

  m_viewer->setRaycastRender(flag);
  viewerUi.raycastParam->setVisible(flag);

  if (viewerUi.voxchoice->currentIndex() == 0)
    viewerUi.lightBox->setVisible(true);
  else
    viewerUi.lightBox->setVisible(false);
}

void
DrishtiPaint::on_stillStep_changed(double step)
{
  float ds = m_viewer->dragStep();

  if (step > ds)
    {
      viewerUi.dragStep->setValue(step);
      ds = step;
    }

  m_viewer->setStillAndDragStep(step, ds);
}

void
DrishtiPaint::on_dragStep_changed(double step)
{
  float ss = m_viewer->stillStep();

  if (ss > step)
    {
      viewerUi.stillStep->setValue(step);
      ss = step;
    }

  m_viewer->setStillAndDragStep(ss, step);
}

void
DrishtiPaint::connectImageWidget()
{
  connect(m_imageWidget, SIGNAL(saveWork()),
	  this, SLOT(saveWork()));  

  connect(m_imageWidget, SIGNAL(getSlice(int)),
	  this, SLOT(getSlice(int)));  

  connect(m_imageWidget, SIGNAL(getRawValue(int, int, int)),
	  this, SLOT(getRawValue(int, int, int)));

  connect(m_imageWidget, SIGNAL(applyMaskOperation(int, int, int)),
	  this, SLOT(applyMaskOperation(int, int, int)));

  connect(m_imageWidget, SIGNAL(tagDSlice(int, uchar*)),
	  this, SLOT(tagDSlice(int, uchar*)));

  connect(m_imageWidget, SIGNAL(tagWSlice(int, uchar*)),
	  this, SLOT(tagWSlice(int, uchar*)));

  connect(m_imageWidget, SIGNAL(tagHSlice(int, uchar*)),
	  this, SLOT(tagHSlice(int, uchar*)));

  connect(m_imageWidget, SIGNAL(polygonLevels(QList<int>)),
	  m_slider, SLOT(polygonLevels(QList<int>)));
  

  connect(m_imageWidget, SIGNAL(updateViewerBox(int, int, int, int, int, int)),
	  m_viewer, SLOT(updateViewerBox(int, int, int, int, int, int)));

  connect(m_imageWidget, SIGNAL(showEndCurve()),
	  curvesUi.endcurve, SLOT(show()));
  connect(m_imageWidget, SIGNAL(hideEndCurve()),
	  curvesUi.endcurve, SLOT(hide()));

  connect(m_imageWidget, SIGNAL(showEndFiber()),
	  fibersUi.endfiber, SLOT(show()));
  connect(m_imageWidget, SIGNAL(hideEndFiber()),
	  fibersUi.endfiber, SLOT(hide()));

  connect(m_imageWidget, SIGNAL(viewerUpdate()),
	  m_viewer, SLOT(update()));

  connect(m_imageWidget, SIGNAL(setPropagation(bool)),
	  curvesUi.propagate, SLOT(setChecked(bool)));

  connect(m_imageWidget, SIGNAL(saveMask()),
	  m_volume, SLOT(saveIntermediateResults()));
}

void
DrishtiPaint::connectViewerMenu()
{
  m_viewer->setUIPointer(&viewerUi);

  connect(ui.radius, SIGNAL(valueChanged(int)),
	  viewerUi.radiusSurface, SLOT(setValue(int)));

  connect(viewerUi.pointRender, SIGNAL(clicked(bool)),
	  this, SLOT(on_pointRender_clicked(bool)));
  connect(viewerUi.raycastRender, SIGNAL(clicked(bool)),
	  this, SLOT(on_raycastRender_clicked(bool)));
  
  connect(viewerUi.update, SIGNAL(clicked()),
	  m_viewer, SLOT(updateVoxels()));
  connect(viewerUi.interval, SIGNAL(sliderReleased()),
	  m_viewer, SLOT(updateVoxels()));
  connect(viewerUi.interval, SIGNAL(valueChanged(int)),
	  m_viewer, SLOT(setVoxelInterval(int)));
  connect(viewerUi.ptsize, SIGNAL(valueChanged(int)),
	  m_viewer, SLOT(setPointSize(int)));
  connect(viewerUi.ptscaling, SIGNAL(valueChanged(int)),
	  m_viewer, SLOT(setPointScaling(int)));
  connect(viewerUi.voxchoice, SIGNAL(currentIndexChanged(int)),
	  m_viewer, SLOT(setVoxelChoice(int)));

  connect(viewerUi.box, SIGNAL(clicked(bool)),
	  m_viewer, SLOT(setShowBox(bool)));
  connect(m_viewer, SIGNAL(showBoxChanged(bool)),
	  viewerUi.box, SLOT(setChecked(bool)));

  connect(viewerUi.snapshot, SIGNAL(clicked()),
	  m_viewer, SLOT(saveImage()));
  connect(viewerUi.curvetags, SIGNAL(editingFinished()),
	  this, SLOT(curvetag_editingFinished()));
  connect(viewerUi.fibertags, SIGNAL(editingFinished()),
	  this, SLOT(fibertag_editingFinished()));

  connect(viewerUi.slicesBox, SIGNAL(clicked(bool)),
	  m_viewer, SLOT(setShowSlices(bool)));
  connect(viewerUi.updateSlices, SIGNAL(clicked()),
	  m_viewer, SLOT(updateSlices()));

  // disable full raycasting and xray option
  viewerUi.raycastStyle->hide();
//  connect(viewerUi.raycastStyle, SIGNAL(currentIndexChanged(int)),
//	  m_viewer, SLOT(setRaycastStyle(int)));
//  connect(viewerUi.raycastStyle, SIGNAL(currentIndexChanged(int)),
//	  this, SLOT(lightOnOff(int)));

  connect(viewerUi.skipLayers, SIGNAL(valueChanged(int)),
	  m_viewer, SLOT(setSkipLayers(int)));
  connect(viewerUi.skipVoxels, SIGNAL(valueChanged(int)),
	  m_viewer, SLOT(setSkipVoxels(int)));
  connect(viewerUi.nearest, SIGNAL(clicked(bool)),
	  m_viewer, SLOT(setExactCoord(bool)));

  connect(viewerUi.stillStep, SIGNAL(valueChanged(double)),
	  this, SLOT(on_stillStep_changed(double)));
  connect(viewerUi.dragStep, SIGNAL(valueChanged(double)),
	  this, SLOT(on_dragStep_changed(double)));


  connect(viewerUi.sketchPad, SIGNAL(clicked(bool)),
	  m_viewer, SLOT(showSketchPad(bool)));

  connect(viewerUi.useMask, SIGNAL(clicked(bool)),
	  m_viewer, SLOT(setUseMask(bool)));

  m_viewer->setPointScaling(viewerUi.ptscaling->value());
  m_viewer->setPointSize(viewerUi.ptsize->value());
  m_viewer->setVoxelInterval(viewerUi.interval->value());

  setupSlicesParameters();
  setupLightParameters();


  viewerUi.pointParam->setVisible(true);
  viewerUi.raycastParam->setVisible(false);
}

void
DrishtiPaint::setupSlicesParameters()
{
  m_viewDslice = new PopUpSlider(m_viewer, Qt::Vertical);
  m_viewWslice = new PopUpSlider(m_viewer, Qt::Vertical);
  m_viewHslice = new PopUpSlider(m_viewer, Qt::Vertical);

  m_viewHslice->setText("X");
  m_viewWslice->setText("Y");
  m_viewDslice->setText("Z");
  
  viewerUi.popupSlices->setMargin(0);
  viewerUi.popupSlices->addWidget(m_viewHslice);
  viewerUi.popupSlices->addWidget(m_viewWslice);
  viewerUi.popupSlices->addWidget(m_viewDslice);

  connect(m_viewDslice, SIGNAL(valueChanged(int)),
	  m_viewer, SLOT(setDSlice(int)));
  connect(m_viewWslice, SIGNAL(valueChanged(int)),
	  m_viewer, SLOT(setWSlice(int)));
  connect(m_viewHslice, SIGNAL(valueChanged(int)),
	  m_viewer, SLOT(setHSlice(int)));
}

void
DrishtiPaint::setupLightParameters()
{
  m_viewSpec = new PopUpSlider(m_viewer, Qt::Horizontal);
  m_viewEdge = new PopUpSlider(m_viewer, Qt::Horizontal);
  m_viewShadow = new PopUpSlider(m_viewer, Qt::Horizontal);
  m_shadowX = new PopUpSlider(m_viewer, Qt::Horizontal);
  m_shadowY = new PopUpSlider(m_viewer, Qt::Horizontal);
  m_shadowButton = new QPushButton("Shadow Color");
  m_edgeButton = new QPushButton("Edge Color");
  m_bgButton = new QPushButton("Background Color");

  m_viewSpec->setText("Specular");
  m_viewEdge->setText("Edges");
  m_viewShadow->setText("Shadow");
  m_shadowX->setText("Shadow X");
  m_shadowY->setText("Shadow Y");

  m_viewSpec->setRange(0, 10);
  m_viewSpec->setValue(10);
  m_viewEdge->setRange(0, 10);
  m_viewEdge->setValue(3);
  m_viewShadow->setRange(0, 20);
  m_viewShadow->setValue(10);
  m_shadowX->setRange(-5, 5);
  m_shadowX->setValue(0);
  m_shadowY->setRange(-5, 5);
  m_shadowY->setValue(0);

  QSpacerItem *spitem0 = new QSpacerItem(5,5,QSizePolicy::Minimum, QSizePolicy::Fixed);
  QSpacerItem *spitem1 = new QSpacerItem(5,5,QSizePolicy::Minimum, QSizePolicy::Fixed);
  QSpacerItem *spitem2 = new QSpacerItem(5,5,QSizePolicy::Minimum, QSizePolicy::Fixed);
  
  viewerUi.popupLight->setMargin(2);
  viewerUi.popupLight->addWidget(m_viewSpec);
  viewerUi.popupLight->addItem(spitem0);
  viewerUi.popupLight->addWidget(m_viewEdge);
  viewerUi.popupLight->addWidget(m_edgeButton);
  viewerUi.popupLight->addItem(spitem1);
  viewerUi.popupLight->addWidget(m_viewShadow);
  viewerUi.popupLight->addWidget(m_shadowButton);
  viewerUi.popupLight->addWidget(m_shadowX);
  viewerUi.popupLight->addWidget(m_shadowY);
  viewerUi.popupLight->addItem(spitem2);
  viewerUi.popupLight->addWidget(m_bgButton);

  connect(m_viewSpec, SIGNAL(valueChanged(int)),
	  m_viewer, SLOT(setSpec(int)));
  connect(m_viewEdge, SIGNAL(valueChanged(int)),
	  m_viewer, SLOT(setEdge(int)));
  connect(m_viewShadow, SIGNAL(valueChanged(int)),
	  m_viewer, SLOT(setShadow(int)));
  connect(m_shadowX, SIGNAL(valueChanged(int)),
	  m_viewer, SLOT(setShadowOffsetX(int)));
  connect(m_shadowY, SIGNAL(valueChanged(int)),
	  m_viewer, SLOT(setShadowOffsetY(int)));

  connect(viewerUi.lightBox, SIGNAL(clicked(bool)),
	  m_viewSpec, SLOT(setVisible(bool)));
  connect(viewerUi.lightBox, SIGNAL(clicked(bool)),
	  m_viewEdge, SLOT(setVisible(bool)));
  connect(viewerUi.lightBox, SIGNAL(clicked(bool)),
	  m_viewShadow, SLOT(setVisible(bool)));
  connect(viewerUi.lightBox, SIGNAL(clicked(bool)),
	  m_shadowX, SLOT(setVisible(bool)));
  connect(viewerUi.lightBox, SIGNAL(clicked(bool)),
	  m_shadowY, SLOT(setVisible(bool)));
  connect(viewerUi.lightBox, SIGNAL(clicked(bool)),
	  m_shadowButton, SLOT(setVisible(bool)));
  connect(viewerUi.lightBox, SIGNAL(clicked(bool)),
	  m_edgeButton, SLOT(setVisible(bool)));
  connect(viewerUi.lightBox, SIGNAL(clicked(bool)),
	  m_bgButton, SLOT(setVisible(bool)));

  connect(m_shadowButton, SIGNAL(clicked()),
	  this, SLOT(getShadowColor()));
  connect(m_edgeButton, SIGNAL(clicked()),
	  this, SLOT(getEdgeColor()));
  connect(m_bgButton, SIGNAL(clicked()),
	  this, SLOT(getBGColor()));
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
DrishtiPaint::lightOnOff(int voxchoice)
{
  if (voxchoice > 0)
    viewerUi.lightBox->setVisible(false);
  else
    viewerUi.lightBox->setVisible(true);
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
	  m_imageWidget, SLOT(updateTagColors()));

  connect(m_tagColorEditor, SIGNAL(tagSelected(int)),
	  this, SLOT(tagSelected(int)));

  connect(m_slider, SIGNAL(valueChanged(int)),
	  m_imageWidget, SLOT(sliceChanged(int)));

  connect(m_slider, SIGNAL(userRangeChanged(int, int)),
	  m_imageWidget, SLOT(userRangeChanged(int, int)));

}

void
DrishtiPaint::connectCurvesMenu()
{
  connect(curvesUi.livewire, SIGNAL(clicked(bool)),
	  this, SLOT(on_livewire_clicked(bool)));

  connect(curvesUi.sliceLod, SIGNAL(currentIndexChanged(int)),
	  this, SLOT(on_sliceLod_currentIndexChanged(int)));
  connect(curvesUi.lwsmooth, SIGNAL(currentIndexChanged(int)),
	  this, SLOT(on_lwsmooth_currentIndexChanged(int)));
  connect(curvesUi.lwgrad, SIGNAL(currentIndexChanged(int)),
	  this, SLOT(on_lwgrad_currentIndexChanged(int)));

  connect(curvesUi.modify, SIGNAL(clicked(bool)),
	  this, SLOT(on_modify_clicked(bool)));
  connect(curvesUi.propagate, SIGNAL(clicked(bool)),
	  this, SLOT(on_propagate_clicked(bool)));

  connect(curvesUi.closed, SIGNAL(clicked(bool)),
	  this, SLOT(on_closed_clicked(bool)));
  connect(curvesUi.morphcurves, SIGNAL(clicked()),
	  this, SLOT(on_morphcurves_clicked()));
//  connect(curvesUi.deselect, SIGNAL(clicked()),
//	  this, SLOT(on_deselect_clicked()));


  connect(curvesUi.mincurvelen, SIGNAL(valueChanged(int)),
	  this, SLOT(on_mincurvelen_valueChanged(int)));
  connect(curvesUi.pointsize, SIGNAL(valueChanged(int)),
	  this, SLOT(on_pointsize_valueChanged(int)));

  connect(curvesUi.newcurve, SIGNAL(clicked()),
	  this, SLOT(on_newcurve_clicked()));
  connect(curvesUi.endcurve, SIGNAL(clicked()),
	  this, SLOT(on_endcurve_clicked()));

  connect(curvesUi.deleteallcurves, SIGNAL(clicked()),
	  this, SLOT(on_deleteallcurves_clicked()));
}

void
DrishtiPaint::connectGraphCutMenu()
{
  connect(graphcutUi.copyprev, SIGNAL(clicked(bool)),
	  this, SLOT(on_copyprev_clicked(bool)));

  connect(graphcutUi.preverode, SIGNAL(valueChanged(int)),
	  this, SLOT(on_preverode_valueChanged(int)));
  connect(graphcutUi.smooth, SIGNAL(valueChanged(int)),
	  this, SLOT(on_smooth_valueChanged(int)));
  connect(graphcutUi.lambda, SIGNAL(valueChanged(int)),
	  this, SLOT(on_lambda_valueChanged(int)));
  connect(graphcutUi.boxSize, SIGNAL(valueChanged(int)),
	  this, SLOT(on_boxSize_valueChanged(int)));
}

void
DrishtiPaint::connectFibersMenu()
{
  connect(fibersUi.newfiber, SIGNAL(clicked()),
	  this, SLOT(on_newfiber_clicked()));
  connect(fibersUi.endfiber, SIGNAL(clicked()),
	  this, SLOT(on_endfiber_clicked()));
}

void
DrishtiPaint::on_actionExtractTag_triggered()
{
  QStringList dtypes;
  QList<int> tag;

  bool ok;
  //----------------
  QString tagstr = QInputDialog::getText(0, "Extract volume data for Tag",
	    "Tag Numbers (tags should be separated by space.\n-1 for all tags;\nFor e.g. 1 2 5 will extract tags 1, 2 and 5)",
					 QLineEdit::Normal,
					 "-1",
					 &ok);
  tag.clear();
  if (ok && !tagstr.isEmpty())
    {
      QStringList tglist = tagstr.split(" ", QString::SkipEmptyParts);
      for(int i=0; i<tglist.count(); i++)
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
	  else
	    tag << t;
	}
    }
  else
    tag << -1;
  //----------------

  bool saveImageData = true;
  int shiftVox = 128;
  float scaleVox = 0.5;

  //----------------
  int extractType = 1; // extract using tag
  dtypes.clear();
  dtypes << "Tag Only"
	 << "Tag + Transfer Function";
  if (m_imageWidget->fibersPresent())
    {
      dtypes << "Fibers Only";
      dtypes << "Fibers + Transfer Function";
      dtypes << "Tags + Fibers";
      dtypes << "Tags + Fibers + Transfer Function";
    }
  dtypes << "Tags + Fibers values";
  dtypes << "Curves + Fibers values";
  dtypes << "Tomogram + Tags";

  QString option = QInputDialog::getItem(0,
					 "Extract Data",
					 "Extract Using Tag + Transfer Function",
					 dtypes,
					 0,
					 false,
					 &ok);
  if (!ok)
    return;
  
  if (option == "Tag Only") extractType = 1;
  else if (option == "Tag + Transfer Function") extractType = 2;
  else if (option == "Fibers Only") extractType = 3;
  else if (option == "Fibers + Transfer Function") extractType = 4;
  else if (option == "Tags + Fibers") extractType = 5;
  else if (option == "Tags + Fibers + Transfer Function") extractType = 6;
  else if (option == "Tags + Fibers values")
    {
      extractType = 7;
      saveImageData = false;
    }
  else if (option == "Curves + Fibers values")
    {
      extractType = 8;
      saveImageData = false;
    }
  else if (option == "Tomogram + Tags")
    {
      extractType = 9;
      bool ok;
      QString tagstr = QInputDialog::getText(0, "Merge Tag values with Tomogram",
					     "Scale and shift tomogram values before merging tag values.\nSpecify two numbers - for e.g. 0.5 128 -> this means 0.5*tomoval + 128 + tagval\nFirst value is scaling factor between 0.0 and 1.0, and second value is shift factor between 0 and 255.\n1.0 0 -> means no scaling and shifting of tomogram values",
					     QLineEdit::Normal,
					     "0.5 128",
					     &ok);
      if (ok && !tagstr.isEmpty())
	{
	  QStringList tglist = tagstr.split(" ", QString::SkipEmptyParts);
	  if (tglist.count() == 2)
	    {
	      scaleVox = tglist[0].toFloat();
	      shiftVox = tglist[1].toInt();
	    }
	}
    }
  //----------------

  //----------------
  uchar outsideVal = 0;
  outsideVal = QInputDialog::getInt(0,
				    "Outside value",
				    "Set outside value to",
				    0, 0, 255, 1);
  //----------------


  int depth, width, height;
  m_volume->gridSize(depth, width, height);
  
  int minDSlice, maxDSlice;
  int minWSlice, maxWSlice;
  int minHSlice, maxHSlice;
  m_imageWidget->getBox(minDSlice, maxDSlice,
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
					       0,
					       QFileDialog::DontUseNativeDialog);
  
  if (tflnm.isEmpty())
    return;

  if (!tflnm.endsWith(".pvl.nc"))
    tflnm += ".pvl.nc";

  savePvlHeader(m_volume->fileName(),
		tflnm,
		tdepth, twidth, theight,
		saveImageData);

  QStringList tflnms;
  tflnms << tflnm+".001";
  VolumeFileManager tFile;
  tFile.setFilenameList(tflnms);
  tFile.setDepth(tdepth);
  tFile.setWidth(twidth);
  tFile.setHeight(theight);
  tFile.setSlabSize(tdepth+1);
  tFile.createFile(true, false);

  QProgressDialog progress("Extracting tagged region from volume data",
			   QString(),
			   0, 100,
			   0);
  progress.setMinimumDuration(0);

  uchar *lut = Global::lut();
  int nbytes = width*height;
  uchar *raw = new uchar[nbytes];

  //----------------------------------

  bool reloadData = false;
  uchar *curveMask = 0;
  try
    {
      curveMask = new uchar[tdepth*twidth*theight];
    }
  catch (exception &e)
    {
      QMessageBox::information(0, "", "Not enough memory : Cannot create curve mask.\nOffloading volume data and mask.");
      m_volume->offLoadMemFile();
      reloadData = true;
      
      curveMask = new uchar[tdepth*twidth*theight];
    };

  memset(curveMask, 0, tdepth*twidth*theight);

  if (extractType < 3)
    updateCurveMask(curveMask, tag,
		    depth, width, height,
		    tdepth, twidth, theight,
		    minDSlice, minWSlice, minHSlice,
		    maxDSlice, maxWSlice, maxHSlice);
  else if (extractType < 5)
    updateFiberMask(curveMask, tag,
		    tdepth, twidth, theight,
		    minDSlice, minWSlice, minHSlice,
		    maxDSlice, maxWSlice, maxHSlice);
  else
    {
      updateCurveMask(curveMask, tag,
		      depth, width, height,
		      tdepth, twidth, theight,
		      minDSlice, minWSlice, minHSlice,
		      maxDSlice, maxWSlice, maxHSlice);
      updateFiberMask(curveMask, tag,
		      tdepth, twidth, theight,
		      minDSlice, minWSlice, minHSlice,
		      maxDSlice, maxWSlice, maxHSlice);
    }
  //----------------------------------


  for(int d=minDSlice; d<=maxDSlice; d++)
    {
      int slc = d-minDSlice;
      progress.setValue((int)(100*(float)slc/(float)tdepth));
      qApp->processEvents();

      uchar *slice = m_volume->getDepthSliceImage(d);

      if (extractType < 7 || extractType == 9)
	{
	  // we get value+grad from volume
	  // we need only value part
	  int i=0;
	  for(int w=minWSlice; w<=maxWSlice; w++)
	    for(int h=minHSlice; h<=maxHSlice; h++)
	      {
		slice[i] = slice[2*(w*height+h)];
		i++;
	      }
	}
	  
      if (extractType < 7)
	{
	  memcpy(raw, m_volume->getMaskDepthSliceImage(d), nbytes);
	  
	  if (tag[0] == -1)
	    {
	      for(int w=minWSlice; w<=maxWSlice; w++)
		for(int h=minHSlice; h<=maxHSlice; h++)
		  raw[w*height+h] = (raw[w*height+h] > 0 ? 255 : 0);
	      
	      // apply curve mask
	      for(int w=minWSlice; w<=maxWSlice; w++)
		for(int h=minHSlice; h<=maxHSlice; h++)
		  {
		    if (curveMask[slc*twidth*theight +
				  (w-minWSlice)*theight +
				  (h-minHSlice)] > 0)
		      raw[w*height+h] = 255;
		  }
	    }
	  else if (tag[0] == 0)
	    {
	      for(int w=minWSlice; w<=maxWSlice; w++)
		for(int h=minHSlice; h<=maxHSlice; h++)
		  raw[w*height+h] = (raw[w*height+h] == 0 ? 255 : 0);
	      
	      // apply curve mask
	      for(int w=minWSlice; w<=maxWSlice; w++)
		for(int h=minHSlice; h<=maxHSlice; h++)
		  {
		    if (curveMask[slc*twidth*theight +
				  (w-minWSlice)*theight +
				  (h-minHSlice)] > 0)
		      raw[w*height+h] = 0;
		  }
	    }
	  else
	    {
	      for(int w=minWSlice; w<=maxWSlice; w++)
		for(int h=minHSlice; h<=maxHSlice; h++)
		  raw[w*height+h] = (tag.contains(raw[w*height+h]) ? 255 : 0);
	      
	      // apply curve mask
	      for(int w=minWSlice; w<=maxWSlice; w++)
		for(int h=minHSlice; h<=maxHSlice; h++)
		  {
		    if (tag.contains(curveMask[slc*twidth*theight +
					       (w-minWSlice)*theight +
					       (h-minHSlice)]))
		      raw[w*height+h] = 255;
		  }
	    }
	}
      else // extractType == 7 or 8
	{
	  if (extractType == 7 || extractType == 9)
	    memcpy(raw, m_volume->getMaskDepthSliceImage(d), nbytes);
	  else
	    memset(raw, 0, nbytes);

	  // copy curve/fiber mask
	  if (tag[0] == -1)
	    {
	      for(int w=minWSlice; w<=maxWSlice; w++)
		for(int h=minHSlice; h<=maxHSlice; h++)
		  {
		    if (curveMask[slc*twidth*theight +
				  (w-minWSlice)*theight +
				  (h-minHSlice)] > 0)
		      raw[w*height+h] = curveMask[slc*twidth*theight +
						  (w-minWSlice)*theight +
						  (h-minHSlice)];
		  }
	    }
	  else
	    {
	      for(int w=minWSlice; w<=maxWSlice; w++)
		for(int h=minHSlice; h<=maxHSlice; h++)
		  {
		    if (tag.contains(curveMask[slc*twidth*theight +
					       (w-minWSlice)*theight +
					       (h-minHSlice)]))
		      raw[w*height+h] = curveMask[slc*twidth*theight +
						  (w-minWSlice)*theight +
						  (h-minHSlice)];		
		  }
	    }
	}


      //-----------------------------
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
      // use tag/fiber mask + transfer function to extract volume data
      if (extractType == 2 || extractType == 4 || extractType == 6)
	{
	  int sval = 0;
	  if (tag[0] == -1) sval = 0;
	  else if (tag[0] == 0) sval = 255;
	  else sval = 0;
	  
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
      //-----------------------------

      
      // now mask data with tag
      if (saveImageData)
	{
	  if (extractType != 9)
	    {			      
	      for(int i=0; i<twidth*theight; i++)
		{
		  if (raw[i] != 255)
		    slice[i] = outsideVal;
		}
	    }
	  else // extract Tomogram + Tags
	    {
	      // scale and shift tomogram
	      // clamp between (0,255)
	      for(int i=0; i<twidth*theight; i++)
		slice[i] = qMin(qMax((int)(scaleVox*slice[i] + shiftVox), 0), 255);
	      // merge in tag values
	      for(int i=0; i<twidth*theight; i++)
		{
		  if (raw[i] != 0)
		    slice[i] = raw[i];
		}
	    }
	  tFile.setSlice(slc, slice);
	}
      else
	tFile.setSlice(slc, raw);
    }

  delete [] raw;
  delete [] curveMask;

  if (reloadData)
    m_volume->loadMemFile();
  
  progress.setValue(100);  
  QMessageBox::information(0, "Save", "-----Done-----");
}

void
DrishtiPaint::meshFibers(QString flnm)
{
  Vec voxelScaling = Global::voxelScaling();

  QList<Fiber*> *fibers = m_imageWidget->fibers();
  int nvertices = 0;
  for(int i=0; i<fibers->count(); i++)
    {
      Fiber *fb = fibers->at(i);
      nvertices += fb->generateTriangles().count()/2;
    }
  int ntriangles = nvertices/3;  


  QStringList ps;
  ps << "x";
  ps << "y";
  ps << "z";
  ps << "nx";
  ps << "ny";
  ps << "nz";
  ps << "red";
  ps << "green";
  ps << "blue";
  ps << "vertex_indices";
  ps << "vertex";
  ps << "face";

  QList<char *> plyStrings;
  for(int i=0; i<ps.count(); i++)
    {
      char *s;
      s = new char[ps[i].size()+1];
      strcpy(s, ps[i].toLatin1().data());
      plyStrings << s;
    }

  typedef struct PlyFace
  {
    unsigned char nverts;    /* number of Vertex indices in list */
    int *verts;              /* Vertex index list */
  } PlyFace;

  typedef struct
  {
    real  x,  y,  z ;  /**< Vertex coordinates */
    real nx, ny, nz ;  /**< Vertex normal */
    uchar r, g, b;
  } myVertex ;

  PlyProperty vert_props[] = { /* list of property information for a vertex */
    {plyStrings[0], Float32, Float32, offsetof(myVertex,x), 0, 0, 0, 0},
    {plyStrings[1], Float32, Float32, offsetof(myVertex,y), 0, 0, 0, 0},
    {plyStrings[2], Float32, Float32, offsetof(myVertex,z), 0, 0, 0, 0},
    {plyStrings[3], Float32, Float32, offsetof(myVertex,nx), 0, 0, 0, 0},
    {plyStrings[4], Float32, Float32, offsetof(myVertex,ny), 0, 0, 0, 0},
    {plyStrings[5], Float32, Float32, offsetof(myVertex,nz), 0, 0, 0, 0},
    {plyStrings[6], Uint8, Uint8, offsetof(myVertex,r), 0, 0, 0, 0},
    {plyStrings[7], Uint8, Uint8, offsetof(myVertex,g), 0, 0, 0, 0},
    {plyStrings[8], Uint8, Uint8, offsetof(myVertex,b), 0, 0, 0, 0},
  };

  PlyProperty face_props[] = { /* list of property information for a face */
    {plyStrings[9], Int32, Int32, offsetof(PlyFace,verts),
     1, Uint8, Uint8, offsetof(PlyFace,nverts)},
  };

  PlyFile    *ply;
  FILE       *fp = fopen(flnm.toLatin1().data(),
			 bin ? "wb" : "w");

  PlyFace     face ;
  int         verts[3] ;
  char       *elem_names[]  = {plyStrings[10], plyStrings[11]};
  ply = write_ply (fp,
		   2,
		   elem_names,
		   bin? PLY_BINARY_LE : PLY_ASCII );

  /* describe what properties go into the PlyVertex elements */
  describe_element_ply ( ply, plyStrings[10], nvertices );
  describe_property_ply ( ply, &vert_props[0] );
  describe_property_ply ( ply, &vert_props[1] );
  describe_property_ply ( ply, &vert_props[2] );
  describe_property_ply ( ply, &vert_props[3] );
  describe_property_ply ( ply, &vert_props[4] );
  describe_property_ply ( ply, &vert_props[5] );
  describe_property_ply ( ply, &vert_props[6] );
  describe_property_ply ( ply, &vert_props[7] );
  describe_property_ply ( ply, &vert_props[8] );

  /* describe PlyFace properties (just list of PlyVertex indices) */
  describe_element_ply ( ply, plyStrings[11], ntriangles );
  describe_property_ply ( ply, &face_props[0] );

  header_complete_ply ( ply );


  /* set up and write the PlyVertex elements */
  put_element_setup_ply ( ply, plyStrings[10] );

  for(int i=0; i<fibers->count(); i++)
    {
      Fiber *fb = fibers->at(i);
      QList<Vec> triangles = fb->generateTriangles();
      int tag = fb->tag;
      int nv = triangles.count()/2;
      uchar r = Global::tagColors()[4*tag+0];
      uchar g = Global::tagColors()[4*tag+1];
      uchar b = Global::tagColors()[4*tag+2];
      for(int t=0; t<nv; t++)
	{
	  myVertex vertex;
	  vertex.nx = triangles[2*t+0].x;
	  vertex.ny = triangles[2*t+0].y;
	  vertex.nz = triangles[2*t+0].z;
	  vertex.x = triangles[2*t+1].x;
	  vertex.y = triangles[2*t+1].y;
	  vertex.z = triangles[2*t+1].z;
	  vertex.r = r;
	  vertex.g = g;
	  vertex.b = b;
	  vertex.x *= voxelScaling.x;
	  vertex.y *= voxelScaling.y;
	  vertex.z *= voxelScaling.z;
	  put_element_ply ( ply, ( void * ) &vertex );
	}
    }

  /* set up and write the PlyFace elements */
  put_element_setup_ply ( ply, plyStrings[11] );
  face.nverts = 3 ;
  face.verts  = verts ;
  for(int i=0; i<ntriangles; i++)
    {
      face.verts[0] = 3*i+0;
      face.verts[1] = 3*i+1;
      face.verts[2] = 3*i+2;
      put_element_ply ( ply, ( void * ) &face );
    }
     
  close_ply ( ply );
  free_ply ( ply );
  
  for(int i=0; i<plyStrings.count(); i++)
    delete [] plyStrings[i];
  
  QMessageBox::information(0, "", "Fibers save to "+flnm);
}

void
DrishtiPaint::updateCurveMask(uchar *curveMask, QList<int> tag,
			      int depth, int width, int height,
			      int tdepth, int twidth, int theight,
			      int minDSlice, int minWSlice, int minHSlice,
			      int maxDSlice, int maxWSlice, int maxHSlice)
{
  QProgressDialog progress("Generating Curve Mask",
			   QString(),
			   0, 100,
			   0);
  progress.setMinimumDuration(0);

  if (m_imageWidget->dCurvesPresent())
    {
      uchar *mask = new uchar[width*height]; 
      for(int d=minDSlice; d<=maxDSlice; d++)
	{
	  int slc = d-minDSlice;
	  progress.setValue((int)(100*(float)slc/(float)tdepth));
	  qApp->processEvents();
	  
	  memset(mask, 0, width*height);
	  m_imageWidget->paintUsingCurves(0, d, height, width, mask, tag);
	  for(int w=minWSlice; w<=maxWSlice; w++)
	    for(int h=minHSlice; h<=maxHSlice; h++)
	      {
		if (mask[w*height+h] > 0)
		  curveMask[(d-minDSlice)*twidth*theight +
			    (w-minWSlice)*theight +
			    (h-minHSlice)] = mask[w*height+h];
	      }
	}
      delete [] mask;
    }
  if (m_imageWidget->wCurvesPresent())
    {
      uchar *mask = new uchar[depth*height]; 
      for(int w=minWSlice; w<=maxWSlice; w++)
	{
	  int slc = w-minWSlice;
	  progress.setValue((int)(100*(float)slc/(float)twidth));
	  qApp->processEvents();
	  
	  memset(mask, 0, depth*height);
	  m_imageWidget->paintUsingCurves(1, w, height, depth, mask, tag);
	  for(int d=minDSlice; d<=maxDSlice; d++)
	    for(int h=minHSlice; h<=maxHSlice; h++)
	      {
		if (mask[d*height+h] > 0)
		  curveMask[(d-minDSlice)*twidth*theight +
			    (w-minWSlice)*theight +
			    (h-minHSlice)] = mask[d*height+h];
	      }
	}
      delete [] mask;
    }
  if (m_imageWidget->hCurvesPresent())
    {
      uchar *mask = new uchar[depth*width]; 
      for(int h=minHSlice; h<=maxHSlice; h++)
	{
	  int slc = h-minHSlice;
	  progress.setValue((int)(100*(float)slc/(float)theight));
	  qApp->processEvents();
	  
	  memset(mask, 0, depth*width);
	  m_imageWidget->paintUsingCurves(2, h, width, depth, mask, tag);
	  for(int d=minDSlice; d<=maxDSlice; d++)
	    for(int w=minWSlice; w<=maxWSlice; w++)
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
DrishtiPaint::updateFiberMask(uchar *curveMask, QList<int> tag,
			      int tdepth, int twidth, int theight,
			      int minDSlice, int minWSlice, int minHSlice,
			      int maxDSlice, int maxWSlice, int maxHSlice)
{
  if (! m_imageWidget->fibersPresent())
    return;
  
  QProgressDialog progress("Generating Fiber Mask",
			   QString(),
			   0, 100,
			   0);
  progress.setMinimumDuration(0);

  QList<Fiber*> *fibers = m_imageWidget->fibers();
  for(int i=0; i<fibers->count(); i++)
    {
      progress.setValue(100*(float)i/(float)fibers->count());
      Fiber *fb = fibers->at(i);
      int tag = fb->tag;
      int rad = fb->thickness/2;

      QList<Vec> trace = fb->trace;
      for(int t=0; t<trace.count(); t++)
	{
	  int d = trace[t].z;
	  int w = trace[t].y;
	  int h = trace[t].x;
	  if (d > minDSlice && d < maxDSlice &&
	      w > minWSlice && w < maxWSlice &&
	      h > minHSlice && h < maxHSlice)
	    {
	      int ds, de, ws, we, hs, he;
	      ds = qMax(minDSlice+1, d-rad);
	      de = qMin(maxDSlice-1, d+rad);
	      ws = qMax(minWSlice+1, w-rad);
	      we = qMin(maxWSlice-1, w+rad);
	      hs = qMax(minHSlice+1, h-rad);
	      he = qMin(maxHSlice-1, h+rad);
	      for(int dd=ds; dd<=de; dd++)
		for(int ww=ws; ww<=we; ww++)
		  for(int hh=hs; hh<=he; hh++)
		    {
		      float p = ((d-dd)*(d-dd)+
				 (w-ww)*(w-ww)+
				 (h-hh)*(h-hh));
		      if (p < rad*rad)
			curveMask[(dd-minDSlice)*twidth*theight +
				  (ww-minWSlice)*theight +
				  (hh-minHSlice)] = tag;
		    }
	    }
	}
    }
  progress.setValue(100);
}

void
DrishtiPaint::processHoles(uchar* data,
			  int d, int w, int h,
			  int holeSize)
{
  QProgressDialog progress("Closing holes",
			   QString(),
			   0, 100,
			   0);
  if (holeSize < 0)
    progress.setLabelText("Opening holes");

  progress.setMinimumDuration(0);


  QList<Vec> voxels;
  
  for(int nc=0; nc < 2; nc++)
    {
      int aval, bval;
      if (holeSize > 0)
	{
	  aval = (nc==0) ? 0 : 255;
	  bval = (nc==0) ? 255 : 0;
	}
      else
	{
	  aval = (nc>0) ? 0 : 255;
	  bval = (nc>0) ? 255 : 0;
	}

      // find edge voxels
      progress.setLabelText("Identifying boundary voxels ... ");
      voxels.clear();
      for(int i=1; i<d-1; i++)
	{
	  progress.setValue((int)(100.0*(float)i/(float)(d)));
	  qApp->processEvents();
	  for(int j=1; j<w-1; j++)
	    for(int k=1; k<h-1; k++)
	      {
		if (data[i*w*h + j*h + k] == aval)
		  {
		    int is = qMax(0, i-1);
		    int js = qMax(0, j-1);
		    int ks = qMax(0, k-1);
		    int ie = qMin(d-1, i+1);
		    int je = qMin(w-1, j+1);
		    int ke = qMin(h-1, k+1);
		    for(int ii=is; ii<=ie; ii++)
		      for(int jj=js; jj<=je; jj++)
			for(int kk=ks; kk<=ke; kk++)
			  if (data[ii*w*h + jj*h + kk] == bval)
			    {
			      voxels << Vec(i,j,k);
			      break;
			    }
		  }
	      }
	}

      // now dilate
      if (nc)
	progress.setLabelText("Eroding boundary ... ");
      else
	progress.setLabelText("Dilating boundary ... ");

      int nv = voxels.count();
      for(int v=0; v<nv; v++)
	{
	  progress.setValue((int)(100.0*(float)v/(float)(nv)));
	  qApp->processEvents();

	  int i = voxels[v].x;
	  int j = voxels[v].y;
	  int k = voxels[v].z;
	  int is = qBound(0, i-holeSize, d-1);
	  int js = qBound(0, j-holeSize, w-1);
	  int ks = qBound(0, k-holeSize, h-1);
	  int ie = qBound(0, i+holeSize, d-1);
	  int je = qBound(0, j+holeSize, w-1);
	  int ke = qBound(0, k+holeSize, h-1);
	  for(int ii=is; ii<=ie; ii++)
	    for(int jj=js; jj<=je; jj++)
	      for(int kk=ks; kk<=ke; kk++)
		{
		  float p = ((i-ii)*(i-ii)+
			     (j-jj)*(j-jj)+
			     (k-kk)*(k-kk));
		  if (p <= holeSize*holeSize)
		    data[ii*w*h + jj*h + kk] = aval;
		}
	}
    }

  progress.setValue(100);
}

void
DrishtiPaint::dilateAndSmooth(uchar* data,
			      int d, int w, int h,
			      int spread)
{
  QProgressDialog progress("Dilating before smoothing",
			   QString(),
			   0, 100,
			   0);
  progress.setMinimumDuration(0);

  // dilate spread times before applying smoothing
  QList<uchar*> slc;
  for(int diter=0; diter<=spread; diter++)
    slc << new uchar[w*h];

  for(int diter=0; diter<=spread; diter++)
    memcpy(slc[diter], data+(diter+1)*w*h, w*h);

  for(int i=1; i<d-1; i++)
    {
      progress.setValue((int)(100.0*(float)i/(float)(d)));
      qApp->processEvents();

      for(int j=1; j<w-1; j++)
	for(int k=1; k<h-1; k++)
	  {
	    int val = slc[0][j*h + k];
	    if (val == 0)
	      {
		int is = qMax(0, i-spread);
		int js = qMax(0, j-spread);
		int ks = qMax(0, k-spread);
		int ie = qMin(d-1, i+spread);
		int je = qMin(w-1, j+spread);
		int ke = qMin(h-1, k+spread);
		for(int ii=is; ii<=ie; ii++)
		  for(int jj=js; jj<=je; jj++)
		    for(int kk=ks; kk<=ke; kk++)
		      {
			float p = ((i-ii)*(i-ii)+
				   (j-jj)*(j-jj)+
				   (k-kk)*(k-kk));
			if (p < spread*spread)
			  data[ii*w*h + jj*h + kk] = 0;
		      }
	      }
	  }
      
      uchar *tmp = slc[0];
      for(int diter=0; diter<=spread; diter++)
	{
	  if (diter<spread)
	    slc[diter] = slc[diter+1];
	  else
	    slc[diter] = tmp;
	}
      if (i < d-spread-1)
	memcpy(slc[spread], data+(i+spread+1)*w*h, w*h);
    }

  for(int diter=0; diter<=spread; diter++)
    delete [] slc[diter];

  progress.setValue(100);
  
  smoothData(data, d, w, h, qMax(1,spread-1));
}

void
DrishtiPaint::smoothData(uchar *gData,
			 int nX, int nY, int nZ,
			 int spread)
{
  QProgressDialog progress("Smoothing before meshing",
			   QString(),
			   0, 100,
			   0);
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
		  nt++;
		  v += gData[k*nY*nZ + j*nZ + i0];
		}
	      tmp[i] = v/nt;
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
		  nt++;
		  v += gData[k*nY*nZ + j0*nZ + i];
		}
	      tmp[j] = v/nt;
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
		  nt++;
		  v += gData[k0*nY*nZ + j*nZ + i];
		}
	      tmp[k] = v/nt;
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
  QStringList dtypes;
  QList<int> tag;

  bool ok;
  QString tagstr = QInputDialog::getText(0, "Save Mesh for Tag",
	    "Tag Numbers\ntags should be separated by space.\n-2 to ignore tags and use opacity of transfer function for meshing\n-1 mesh all tagged region\n 0 mesh region that is not tagged\n 1-254 for individual tags\nFor e.g. 1 2 5 will mesh region tagged with tags 1, 2 and 5)",
					 QLineEdit::Normal,
					 "-1",
					 &ok);
  tag.clear();
  if (ok && !tagstr.isEmpty())
    {
      QStringList tglist = tagstr.split(" ", QString::SkipEmptyParts);
      for(int i=0; i<tglist.count(); i++)
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
  else
    tag << -1;  
  //----------------

  //----------------
  bool saveFibers = false;
  bool noScaling = false;
  int colorType = 1; // apply tag colors 
  Vec userColor = Vec(255, 255, 255);

  dtypes.clear();
  if (tag[0] == -2)
    {
      dtypes << "User Color"
	     << "Transfer Function";
      
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
      if (option == "Transfer Function") colorType = 5;
    }
  else
    {
      dtypes << "Tag Color"
	     << "Transfer Function"
	     << "Tag Color + Transfer Function"
	     << "Tag Mask + Transfer Function"
	     << "User Color";
      if (m_imageWidget->fibersPresent())
	dtypes << "Fibers";

      QString option = QInputDialog::getItem(0,
					     "Mesh Color",
					     "Color Mesh with",
					     dtypes,
					     0,
					     false,
					     &ok);
      if (!ok)
	return;
      
      if (option == "Fibers") saveFibers = true;
      else if (option == "Tag Color") colorType = 1;
      else if (option == "Transfer Function") colorType = 2;
      else if (option == "Tag Color + Transfer Function") colorType = 3;
      else if (option == "Tag Mask + Transfer Function") colorType = 4;
      else colorType = 0;
    }

  if (colorType == 0 || colorType == 4)
    {
      QColor clr = QColor(255, 255, 255);
      clr = DColorDialog::getColor(clr);
      if (clr.isValid())
	userColor = Vec(clr.red(), clr.green(), clr.blue());
    }
  //----------------


  //----------------
  if (!saveFibers)
    {
      dtypes.clear();
      dtypes << "Yes"
	     << "No Scaling";
      
      QString option = QInputDialog::getItem(0,
					     "Voxel Scaling For Mesh/Fibers",
					     "Apply voxel scaling to mesh/fibers.\nChoose No Scaling only if\nyou want to load the generated\nmesh in Drishti Renderer.",
					     dtypes,
					     0,
					     false,
					     &ok);
      if (!ok)
	return;
      
      if (option == "No Scaling") noScaling = true;
    }
  //----------------


  //----------------
  int depth, width, height;
  m_volume->gridSize(depth, width, height);
  
  int minDSlice, maxDSlice;
  int minWSlice, maxWSlice;
  int minHSlice, maxHSlice;
  m_imageWidget->getBox(minDSlice, maxDSlice,
			minWSlice, maxWSlice,
			minHSlice, maxHSlice);
  qint64 tdepth = maxDSlice-minDSlice+1;
  qint64 twidth = maxWSlice-minWSlice+1;
  qint64 theight = maxHSlice-minHSlice+1;
  
  QString pvlFilename = m_volume->fileName();
  QString tflnm = QFileDialog::getSaveFileName(0,
					       "Save mesh",
					       QFileInfo(pvlFilename).absolutePath(),
					       "Polygon Files (*.ply)",
					       0,
					       QFileDialog::DontUseNativeDialog);
  
  if (tflnm.isEmpty())
    return;

  if (!tflnm.endsWith(".ply"))
    tflnm += ".ply";

  if (saveFibers)
    {
      meshFibers(tflnm);
      return;
    }

  int holeSize = 0;
  holeSize = QInputDialog::getInt(0,
				"Close holes",
				"Close Holes of Size (0 means no closing)",
				0, 0, 100, 1);

  int dataspread = 0;
  dataspread = QInputDialog::getInt(0,
				"Smooth Data Before Meshing",
				"Apply data smoothing before meshing (0 means no smoothing)",
				1, 0, 3, 1);

  int spread = 0;
  spread = QInputDialog::getInt(0,
				"Smooth Mesh",
				"Apply mesh smoothing (0 means no smoothing)",
				1, 0, 5, 1);

  QProgressDialog progress("Meshing tagged region from volume data",
			   "",
			   0, 100,
			   0);
  progress.setMinimumDuration(0);

  uchar *meshingData = new uchar[tdepth*twidth*theight];
  memset(meshingData, 0, tdepth*twidth*theight);

  //----------------------------------
  uchar *curveMask = new uchar[tdepth*twidth*theight];
  memset(curveMask, 0, tdepth*twidth*theight);

  if (tag[0] != -2)
    updateCurveMask(curveMask, tag,
		    depth, width, height,
		    tdepth, twidth, theight,
		    minDSlice, minWSlice, minHSlice,
		    maxDSlice, maxWSlice, maxHSlice);
  //----------------------------------

  uchar *lut = Global::lut();

  int nbytes = width*height;
  uchar *raw = new uchar[width*height];
  uchar *mask = new uchar[width*height]; 
  for(int d=minDSlice; d<=maxDSlice; d++)
    {
      int slc = d-minDSlice;
      progress.setValue((int)(100*(float)slc/(float)tdepth));
      qApp->processEvents();

      uchar *slice = 0;
      if (tag[0] == -2 || colorType == 4) // using tag mask + transfer function to extract mesh
	{
	  slice = m_volume->getDepthSliceImage(d);
	  // we get value+grad from volume
	  // we need only value part
	  int i=0;
	  for(int w=minWSlice; w<=maxWSlice; w++)
	    for(int h=minHSlice; h<=maxHSlice; h++)
	      {
		slice[i] = slice[2*(w*height+h)];
		i++;
	      }
	}

      if (tag[0] == -2)
	memset(raw, 0, width*height);
      else
	{
	  memcpy(mask, m_volume->getMaskDepthSliceImage(d), nbytes);
            
	  if (tag[0] == -1)
	    {
	      for(int w=minWSlice; w<=maxWSlice; w++)
		for(int h=minHSlice; h<=maxHSlice; h++)
		  raw[w*height+h] = (mask[w*height+h] > 0 ? 0 : 255);
	      
	      // apply curve mask
	      for(int w=minWSlice; w<=maxWSlice; w++)
		for(int h=minHSlice; h<=maxHSlice; h++)
		  {
		    if (curveMask[slc*twidth*theight +
				  (w-minWSlice)*theight +
				  (h-minHSlice)] > 0)
		      raw[w*height+h] = 0;
		  }
	    }
	  else if (tag[0] == 0)
	    {
	      for(int w=minWSlice; w<=maxWSlice; w++)
		for(int h=minHSlice; h<=maxHSlice; h++)
		  raw[w*height+h] = (mask[w*height+h] > 0 ? 255 : 0);
	      
	      // apply curve mask
	      for(int w=minWSlice; w<=maxWSlice; w++)
		for(int h=minHSlice; h<=maxHSlice; h++)
		  {
		    if (curveMask[slc*twidth*theight +
				  (w-minWSlice)*theight +
				  (h-minHSlice)] > 0)
		      raw[w*height+h] = 255;
		  }
	    }
	  else
	    {
	      for(int w=minWSlice; w<=maxWSlice; w++)
		for(int h=minHSlice; h<=maxHSlice; h++)
		  raw[w*height+h] = (tag.contains(mask[w*height+h]) ? 0 : 255);
	      
	      // apply curve mask
	      for(int w=minWSlice; w<=maxWSlice; w++)
		for(int h=minHSlice; h<=maxHSlice; h++)
		  {
		    if (tag.contains(curveMask[slc*twidth*theight +
					       (w-minWSlice)*theight +
					       (h-minHSlice)]))
		      raw[w*height+h] = 0;
		  }
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
		if (raw[i] != 255 &&
		    lut[4*slice[i]+3] < 5)
		  raw[i] = 255;
		i++;
	      }
	}
      //-----------------------------

      memcpy(meshingData+slc*twidth*theight, raw, twidth*theight);
    }

  delete [] raw;
  delete [] mask;

  progress.setValue(100);  

//  if (spread > 0)
//    dilateAndSmooth(meshingData, tdepth, twidth, theight, spread+1);

  //----------------------------------
  if (holeSize != 0)
    processHoles(meshingData,
		tdepth, twidth, theight,
		holeSize);
  //----------------------------------

  //----------------------------------
  // add a border to make a watertight mesh when the isosurface
  // touches the border
  for(int i=0; i<tdepth; i++)
    for(int j=0;j<twidth; j++)
      for(int k=0;k<theight; k++)
	{
	  if (i==0 || i == tdepth-1 ||
	      j==0 || j == twidth-1 ||
	      k==0 || k == theight-1)
	    if (meshingData[i*twidth*theight+j*theight+k] < 255)
	      meshingData[i*twidth*theight+j*theight+k] = 255;
	}
  //----------------------------------

  //-----------------
  if (dataspread > 0)
    dilateAndSmooth(meshingData, tdepth, twidth, theight, dataspread+1);
  //-----------------

  MarchingCubes mc;
  mc.set_resolution(theight, twidth, tdepth);
  mc.set_ext_data(meshingData);
  mc.init_all();
  if (dataspread == 0)
    mc.run(32);
  else    
    mc.run(32 - 3*dataspread);

  processAndSaveMesh(colorType,
		     tflnm,
		     &mc,
		     curveMask,
		     minHSlice, minWSlice, minDSlice,
		     theight, twidth, tdepth, spread,
		     userColor, noScaling);
  
  mc.clean_all();

  delete [] meshingData;
  delete [] curveMask;

  QMessageBox::information(0, "Save", "-----Done-----");
}

void
DrishtiPaint::processAndSaveMesh(int colorType,
				 QString flnm,
				 MarchingCubes *mc,
				 uchar *tagdata,
				 int minHSlice, int minWSlice, int minDSlice,
				 int theight, int twidth, int tdepth,
				 int spread, Vec userColor,
				 bool noScaling)
{
  QList<Vec> V;
  QList<Vec> N;
  QList<Vec> C;
  QList<Vec> E;

  int ntriangles = mc->ntrigs();
  Triangle *triangles = mc->triangles();

  int nvertices = mc->nverts();
  Vertex *vertices = mc->vertices();

  for(int ni=0; ni<nvertices; ni++)
    {
      Vec v, n, c;
      v = Vec(vertices[ni].x, vertices[ni].y, vertices[ni].z);
      n = Vec(vertices[ni].nx, vertices[ni].ny, vertices[ni].nz);

      V << v;
      N << n;
      C << userColor;
    }

  for(int ni=0; ni<ntriangles; ni++)
    E << Vec(triangles[ni].v1, triangles[ni].v2, triangles[ni].v3);


  // for colorType 0 and 4 apply user defined color
  if (colorType != 0 && colorType != 4)
    colorMesh(C, V,
	      colorType, tagdata,
	      minHSlice, minWSlice, minDSlice,
	      theight, twidth, tdepth, spread);
	      
  if (spread > 0)
    smoothMesh(V, N, E, 5*spread);
    
  saveMesh(V, N, C, E, flnm, noScaling);
}

void
DrishtiPaint::colorMesh(QList<Vec>& C,
			QList<Vec> V,
			int colorType,
			uchar *tagdata,
			int minHSlice, int minWSlice, int minDSlice,
			int theight, int twidth, int tdepth,
			int spread)
{
  uchar *lut = Global::lut();

  QProgressDialog progress("Colouring mesh",
			   QString(),
			   0, 100,
			   0);
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
      int h = qBound(0, (int)V[ni].x, theight-1);
      int w = qBound(0, (int)V[ni].y, twidth-1);
      int d = qBound(0, (int)V[ni].z, tdepth-1);
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
      else if (colorType == 2 || colorType == 5) // apply only transfer function
	{
	  QList<uchar> val = m_volume->rawValue(d+minDSlice,
						w+minWSlice,
						h+minHSlice); 
	  r =  lut[4*val[0]+2];
	  g =  lut[4*val[0]+1];
	  b =  lut[4*val[0]+0];

//	  if (r == 0 && g == 0 && b == 0)
//	    {
//	      for(int sp=1; sp<=bsz+1; sp++)
//		{
//		  bool ok=false;
//		  for(int dd=qMax(d-sp, 0); dd<=qMin(tdepth-1,d+sp); dd++)
//		    for(int ww=qMax(w-sp, 0); ww<=qMin(twidth-1,w+sp); ww++)
//		      for(int hh=qMax(h-sp, 0); hh<=qMin(theight-1,h+sp); hh++)
//			{
//			  if (qAbs(dd-d) == sp ||
//			      qAbs(ww-w) == sp ||
//			      qAbs(hh-h) == sp)
//			    {
//			      val = m_volume->rawValue(dd+minDSlice,
//						       ww+minWSlice,
//						       hh+minHSlice);     
//			      r = lut[4*val[0]+2];
//			      g = lut[4*val[0]+1];
//			      b = lut[4*val[0]+0];
//			      if (r > 0 || g > 0 || b > 0)
//				{
//				  ok = true;
//				  break;
//				}
//			    }
//			}
//		  if (ok)
//		    break;
//		}
//	    }
	}
      else // merge tag and transfer function colors
	{
	  r = Global::tagColors()[4*tag+0];
	  g = Global::tagColors()[4*tag+1];
	  b = Global::tagColors()[4*tag+2];

	  QList<uchar> val = m_volume->rawValue(d+minDSlice,
						w+minWSlice,
						h+minHSlice);     
	  r = 0.5*r + 0.5*lut[4*val[0]+2];
	  g = 0.5*g + 0.5*lut[4*val[0]+1];
	  b = 0.5*b + 0.5*lut[4*val[0]+0];
	}

      if (r == 0 && g == 0 && b == 0)
	r = g = b = 255;

      C[ni] = Vec(r,g,b);
    }

  progress.setValue(100);
}

void
DrishtiPaint::saveMesh(QList<Vec> V,
		       QList<Vec> N,
		       QList<Vec> C,
		       QList<Vec> E,
		       QString flnm,
		       bool noScaling)
{
  int minDSlice, maxDSlice;
  int minWSlice, maxWSlice;
  int minHSlice, maxHSlice;
  m_imageWidget->getBox(minDSlice, maxDSlice,
			minWSlice, maxWSlice,
			minHSlice, maxHSlice);

  Vec voxelScaling = Global::voxelScaling();
  if (noScaling) voxelScaling = Vec(1,1,1);
  
  QProgressDialog progress("Saving mesh ...",
			   QString(),
			   0, 100,
			   0);
  progress.setMinimumDuration(0);

  QStringList ps;
  ps << "x";
  ps << "y";
  ps << "z";
  ps << "nx";
  ps << "ny";
  ps << "nz";
  ps << "red";
  ps << "green";
  ps << "blue";
  ps << "vertex_indices";
  ps << "vertex";
  ps << "face";

  QList<char *> plyStrings;
  for(int i=0; i<ps.count(); i++)
    {
      char *s;
      s = new char[ps[i].size()+1];
      strcpy(s, ps[i].toLatin1().data());
      plyStrings << s;
    }


  int ntriangles = E.count();
  int nvertices = V.count();

  typedef struct PlyFace
  {
    unsigned char nverts;    /* number of Vertex indices in list */
    int *verts;              /* Vertex index list */
  } PlyFace;

  typedef struct
  {
    real  x,  y,  z ;  /**< Vertex coordinates */
    real nx, ny, nz ;  /**< Vertex normal */
    uchar r, g, b;
  } myVertex ;

  PlyProperty vert_props[] = { /* list of property information for a vertex */
    {plyStrings[0], Float32, Float32, offsetof(myVertex,x), 0, 0, 0, 0},
    {plyStrings[1], Float32, Float32, offsetof(myVertex,y), 0, 0, 0, 0},
    {plyStrings[2], Float32, Float32, offsetof(myVertex,z), 0, 0, 0, 0},
    {plyStrings[3], Float32, Float32, offsetof(myVertex,nx), 0, 0, 0, 0},
    {plyStrings[4], Float32, Float32, offsetof(myVertex,ny), 0, 0, 0, 0},
    {plyStrings[5], Float32, Float32, offsetof(myVertex,nz), 0, 0, 0, 0},
    {plyStrings[6], Uint8, Uint8, offsetof(myVertex,r), 0, 0, 0, 0},
    {plyStrings[7], Uint8, Uint8, offsetof(myVertex,g), 0, 0, 0, 0},
    {plyStrings[8], Uint8, Uint8, offsetof(myVertex,b), 0, 0, 0, 0},
  };

  PlyProperty face_props[] = { /* list of property information for a face */
    {plyStrings[9], Int32, Int32, offsetof(PlyFace,verts),
     1, Uint8, Uint8, offsetof(PlyFace,nverts)},
  };

  PlyFile    *ply;
  FILE       *fp = fopen(flnm.toLatin1().data(),
			 bin ? "wb" : "w");

  PlyFace     face ;
  int         verts[3] ;
  char       *elem_names[]  = {plyStrings[10], plyStrings[11]};
  ply = write_ply (fp,
		   2,
		   elem_names,
		   bin? PLY_BINARY_LE : PLY_ASCII );

  /* describe what properties go into the PlyVertex elements */
  describe_element_ply ( ply, plyStrings[10], nvertices );
  describe_property_ply ( ply, &vert_props[0] );
  describe_property_ply ( ply, &vert_props[1] );
  describe_property_ply ( ply, &vert_props[2] );
  describe_property_ply ( ply, &vert_props[3] );
  describe_property_ply ( ply, &vert_props[4] );
  describe_property_ply ( ply, &vert_props[5] );
  describe_property_ply ( ply, &vert_props[6] );
  describe_property_ply ( ply, &vert_props[7] );
  describe_property_ply ( ply, &vert_props[8] );

  /* describe PlyFace properties (just list of PlyVertex indices) */
  describe_element_ply ( ply, plyStrings[11], ntriangles );
  describe_property_ply ( ply, &face_props[0] );

  header_complete_ply ( ply );


  /* set up and write the PlyVertex elements */
  put_element_setup_ply ( ply, plyStrings[10] );

  for(int ni=0; ni<nvertices; ni++)
    {
      if (ni%10000 == 0)
	{
	  progress.setValue((int)(100.0*(float)ni/(float)(nvertices)));
	  qApp->processEvents();
	}

      myVertex vertex;
      vertex.x = (minHSlice + V[ni].x)*voxelScaling.x;;
      vertex.y = (minWSlice + V[ni].y)*voxelScaling.y;;
      vertex.z = (minDSlice + V[ni].z)*voxelScaling.z;;
      vertex.nx = N[ni].x;
      vertex.ny = N[ni].y;
      vertex.nz = N[ni].z;
      vertex.r = C[ni].x;
      vertex.g = C[ni].y;
      vertex.b = C[ni].z;

      put_element_ply ( ply, ( void * ) &vertex );
    }

  /* set up and write the PlyFace elements */
  put_element_setup_ply ( ply, plyStrings[11] );
  face.nverts = 3 ;
  face.verts  = verts ;
  for(int ni=0; ni<ntriangles; ni++)
    {      
      face.verts[0] = E[ni].x;
      face.verts[1] = E[ni].y;
      face.verts[2] = E[ni].z;
      
      put_element_ply ( ply, ( void * ) &face );
    }

  close_ply ( ply );
  free_ply ( ply );

  for(int i=0; i<plyStrings.count(); i++)
    delete [] plyStrings[i];

  progress.setValue(100);
}

void
DrishtiPaint::smoothMesh(QList<Vec>& V,
			 QList<Vec>& N,
			 QList<Vec>& E,
			 int ntimes)
{
  QList<Vec> newV;
  newV = V;

  int nv = V.count();
  QProgressDialog progress("Mesh smoothing in progress ... ",
			   QString(),
			   0, 100,
			   0);
  progress.setMinimumDuration(0);

  //----------------------------
  // create incidence matrix
  progress.setLabelText("Generate incidence matrix ...");
  QMultiMap<int, int> imat;
  int ntri = E.count();
  for(int i=0; i<ntri; i++)
    {
      if (i%10000 == 0)
	{
	  progress.setValue((int)(100.0*(float)i/(float)(ntri)));
	  qApp->processEvents();
	}

      int a = E[i].x;
      int b = E[i].y;
      int c = E[i].z;

      imat.insert(a, b);
      imat.insert(b, a);
      imat.insert(a, c);
      imat.insert(c, a);
      imat.insert(b, c);
      imat.insert(c, b);
    }
  //----------------------------

  //----------------------------
  // smooth vertices
  progress.setLabelText("Process vertices ...");
  for(int nt=0; nt<ntimes; nt++)
    {
      progress.setValue((int)(100.0*(float)nt/(float)(ntimes)));
      qApp->processEvents();
	  
      for(int i=0; i<nv; i++)
	{
	  QList<int> idx = imat.values(i);
	  Vec v0 = V[i];
	  Vec v = Vec(0,0,0);
	  float sum = 0;
	  for(int j=0; j<idx.count(); j++)
	    {
	      Vec vj = V[idx[j]];
	      float ln = (v0-vj).norm();
	      if (ln > 0)
		{
		  sum += 1.0/ln;
		  v = v + vj/ln;
		}
	    }
	  if (sum > 0)
	    v0 = v0 + 0.5*(v/sum - v0);
	  newV[i] = v0;
	}
      
      for(int i=0; i<nv; i++)
	{
	  QList<int> idx = imat.values(i);
	  Vec v0 = newV[i];
	  Vec v = Vec(0,0,0);
	  float sum = 0;
	  for(int j=0; j<idx.count(); j++)
	    {
	      Vec vj = newV[idx[j]];
	      float ln = (v0-vj).norm();
	      if (ln > 0)
		{
		  sum += 1.0/ln;
		  v = v + vj/ln;
		}
	    }
	  if (sum > 0)
	    v0 = v0 - 0.5*(v/sum - v0);
	  V[i] = v0;
	}

      V = newV;
    }
  //----------------------------


  //----------------------------
  progress.setLabelText("Calculate normals ...");
  // now calculate normals
  for(int i=0; i<nv; i++)
    newV[i] = Vec(0,0,0);

  QVector<int> nvs;
  nvs.resize(nv);
  nvs.fill(0);

  for(int i=0; i<ntri; i++)
    {
      if (i%10000 == 0)
	{
	  progress.setValue((int)(100.0*(float)i/(float)(ntri)));
	  qApp->processEvents();
	}

      int a = E[i].x;
      int b = E[i].y;
      int c = E[i].z;

      Vec va = V[a];
      Vec vb = V[b];
      Vec vc = V[c];
      Vec v0 = (vb-va).unit();
      Vec v1 = (vc-va).unit();      
      Vec vn = v0^v1;
      
      newV[a] += vn;
      newV[b] += vn;
      newV[c] += vn;

      nvs[a]++;
      nvs[b]++;
      nvs[c]++;
    }

  for(int i=0; i<nv; i++)
      N[i] = newV[i]/nvs[i];
  //----------------------------


  progress.setValue(100);
}

void
DrishtiPaint::paint3DStart()
{
  m_prevSeed = Vec(-1,-1,-1);
  m_blockList.clear();
}

void
DrishtiPaint::paint3D(int d, int w, int h, int button, int otag)
{
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
  m_imageWidget->getBox(minDSlice, maxDSlice,
			minWSlice, maxWSlice,
			minHSlice, maxHSlice);

  uchar *volData = m_volume->memVolDataPtr();
  uchar *maskData = m_volume->memMaskDataPtr();


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
      for(qint64 dd=ds0; dd<=de0; dd++)
	for(qint64 ww=ws0; ww<=we0; ww++)
	  for(qint64 hh=hs0; hh<=he0; hh++)
	    {
	      Vec v0 = Vec(hh-h0,ww-w0,dd-d0);
	      Vec dv = viewD;
	      if (persp)
		dv = Vec(hh-camPos.x, ww-camPos.y, dd-camPos.z).unit();
	      
	      float pr = v0*viewR;
	      float pu = v0*viewU;
	      float pvd = v0*dv;
	      if (qAbs(pvd) <= vrad2 && qSqrt(pr*pr +pu*pu) <= vrad1)
		{
		  int val = volData[dd*m_width*m_height + ww*m_height + hh];
		  uchar mtag = maskData[dd*m_width*m_height + ww*m_height + hh];
		  bool opaque =  (lut[4*val+3]*Global::tagColors()[4*mtag+3] > 0);
		  if (opaque)
		    bitmask.setBit((dd-ds)*dm2 + (ww-ws)*dm + (hh-hs), true);	      
		}
	    }
    } // seeds

  int indices[] = {-1, 0, 0,
		    1, 0, 0,
		    0,-1, 0,
		    0, 1, 0,
		    0, 0,-1,
		    0, 0, 1};

  // tag only connected region

  QStack<Vec> stack;
  int minD,maxD, minW,maxW, minH,maxH;

  for(int i=0; i<seeds.count(); i++)
    {
      qint64 h0 = seeds[i].x;
      qint64 w0 = seeds[i].y;
      qint64 d0 = seeds[i].z;
      maskData[d0*m_width*m_height + w0*m_height + h0] = tag;
      bitmask.setBit((d0-ds)*dm2 + (w0-ws)*dm + (h0-hs), false);
      stack.push(Vec(d0, w0, h0));
    }

  minD = maxD = d;
  minW = maxW = w;
  minH = maxH = h;

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
	      maskData[d2*m_width*m_height + w2*m_height + h2] = tag;
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

  getSlice(m_currSlice);

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
DrishtiPaint::erodeConnected(int dr, int wr, int hr,
			     Vec bmin, Vec bmax, int tag)
{
  int m_depth, m_width, m_height;
  m_volume->gridSize(m_depth, m_width, m_height);

  if (dr < 0 || wr < 0 || hr < 0 ||
      dr > m_depth-1 ||
      wr > m_width-1 ||
      hr > m_height-1)
    {
      QMessageBox::information(0, "", QString("No visible region found at %1 %2 %3").\
			       arg(hr).arg(wr).arg(dr));
      return;
    }

  uchar *lut = Global::lut();
  uchar *volData = m_volume->memVolDataPtr();
  uchar *maskData = m_volume->memMaskDataPtr();

  {
    qint64 idx = (qint64)dr*m_width*m_height + (qint64)wr*m_height + (qint64)hr;
    int val = volData[idx];
    if (lut[4*val+3] == 0 || maskData[idx] != tag)
      {
	QMessageBox::information(0, "Erode",
				 QString("Cannot erode.\nYou are on voxel with tag %1, was expecting tag %2").arg(maskData[idx]).arg(tag));
	return;
      }
  }

    
  QProgressDialog progress("Updating voxel structure",
			   QString(),
			   0, 100,
			   0);
  progress.setMinimumDuration(0);


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

  MyBitArray cbitmask;
  cbitmask.resize(mx*my*mz);
  cbitmask.fill(false);

  int indices[] = {-1, 0, 0,
		   1, 0, 0,
		   0,-1, 0,
		   0, 1, 0,
		   0, 0,-1,
		   0, 0, 1};

  // find connected region before erosion
  int minD,maxD, minW,maxW, minH,maxH;

  QStack<Vec> stack;
  stack.push(Vec(dr,wr,hr));

  qint64 bidx = (dr-ds)*mx*my+(wr-ws)*mx+(hr-hs);
  bitmask.setBit(bidx, true);
  cbitmask.setBit(bidx, true);

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
	  if (!cbitmask.testBit(bidx))
	    {
	      cbitmask.setBit(bidx, true);
	      qint64 idx = d2*m_width*m_height + w2*m_height + h2;
	      int val = volData[idx];
	      if (lut[4*val+3] > 0 && maskData[idx] == tag)
		{
		  bitmask.setBit(bidx, true);
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
    }

  progress.setLabelText("Erode");
  qApp->processEvents();

  QList<Vec> edges;
  edges.clear();

  int nErode = viewerUi.dilateRad->value();

  // find inner boundary
  for(int d=ds; d<=de; d++)
    {
      progress.setValue(90*(d-ds)/(mz));
      if (d%10 == 0)
	qApp->processEvents();
      for(int w=ws; w<=we; w++)
	for(int h=hs; h<=he; h++)
	  {
	    qint64 bidx = (d-ds)*mx*my+(w-ws)*mx+(h-hs);
	    if (bitmask.testBit(bidx))
	      {
		bool inside = true;
		for(int i=0; i<6; i++)
		  {
		    int da = indices[3*i+0];
		    int wa = indices[3*i+1];
		    int ha = indices[3*i+2];
		    
		    qint64 d2 = qBound(ds, d+da, de);
		    qint64 w2 = qBound(ws, w+wa, we);
		    qint64 h2 = qBound(hs, h+ha, he);
		    
		    qint64 tidx = (d2-ds)*mx*my+(w2-ws)*mx+(h2-hs);
		    inside &= bitmask.testBit(tidx);
		  }
		if (!inside)
		  edges << Vec(d,w,h);
	      }
	  }
    }

  for(int ne=0; ne<nErode; ne++)
    {
      progress.setValue(90*(float)ne/(float)nErode);
      qApp->processEvents();

      // fill boundary
      for(int e=0; e<edges.count(); e++)
	{
	  qint64 d2 = edges[e].x;
	  qint64 w2 = edges[e].y;
	  qint64 h2 = edges[e].z;

	  qint64 idx = d2*m_width*m_height + w2*m_height + h2;
	  maskData[idx] = 0;

	  qint64 bidx = (d2-ds)*mx*my+(w2-ws)*mx+(h2-hs);
	  bitmask.setBit(bidx, false);
	}

      if (ne < nErode-1)
	{
	  QList<Vec> tedges;
	  tedges.clear();

	  cbitmask.fill(false);
	  // find next inner boundary to fill
	  for(int e=0; e<edges.count(); e++)
	    {
	      int dx = edges[e].x;
	      int wx = edges[e].y;
	      int hx = edges[e].z;
	      
	      for(int i=0; i<6; i++)
		{
		  int da = indices[3*i+0];
		  int wa = indices[3*i+1];
		  int ha = indices[3*i+2];
		  
		  qint64 d2 = qBound(ds, dx+da, de);
		  qint64 w2 = qBound(ws, wx+wa, we);
		  qint64 h2 = qBound(hs, hx+ha, he);
		  
		  qint64 bidx = (d2-ds)*mx*my+(w2-ws)*mx+(h2-hs);
		  if (bitmask.testBit(bidx) && !cbitmask.testBit(bidx))
		    {
		      cbitmask.setBit(bidx);
		      tedges << Vec(d2,w2,h2);	  
		    }
		}
	    }
	  edges = tedges;
	}
    }


  getSlice(m_currSlice);
  
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
DrishtiPaint::dilateConnected(int dr, int wr, int hr,
			      Vec bmin, Vec bmax, int tag)
{
  int m_depth, m_width, m_height;
  m_volume->gridSize(m_depth, m_width, m_height);

  if (dr < 0 || wr < 0 || hr < 0 ||
      dr > m_depth-1 ||
      wr > m_width-1 ||
      hr > m_height-1)
    {
      QMessageBox::information(0, "", QString("No visible region found at %1 %2 %3").\
			       arg(hr).arg(wr).arg(dr));
      return;
    }


  uchar *lut = Global::lut();
  uchar *volData = m_volume->memVolDataPtr();
  uchar *maskData = m_volume->memMaskDataPtr();

  {
    qint64 idx = (qint64)dr*m_width*m_height + (qint64)wr*m_height + (qint64)hr;
    int val = volData[idx];
    if (lut[4*val+3] == 0 || maskData[idx] != tag)
      {
	QMessageBox::information(0, "Dilate",
				 QString("Cannot dilate.\nYou are on voxel with tag %1, was expecting tag %2").arg(maskData[idx]).arg(tag));
	return;
      }
  }

  QProgressDialog progress("Updating voxel structure",
			   QString(),
			   0, 100,
			   0);
  progress.setMinimumDuration(0);

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

  MyBitArray cbitmask;
  cbitmask.resize(mx*my*mz);
  cbitmask.fill(false);

  QList<Vec> cPos =  m_viewer->clipPos();
  QList<Vec> cNorm = m_viewer->clipNorm();

  int indices[] = {-1, 0, 0,
		    1, 0, 0,
		    0,-1, 0,
		    0, 1, 0,
		    0, 0,-1,
		    0, 0, 1};

  // find connected region before dilation
  int minD,maxD, minW,maxW, minH,maxH;

  QStack<Vec> stack;
  stack.push(Vec(dr,wr,hr));

  qint64 bidx = (dr-ds)*mx*my+(wr-ws)*mx+(hr-hs);
  bitmask.setBit(bidx, true);
  cbitmask.setBit(bidx, true);

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
	  if (!cbitmask.testBit(bidx))
	    {
	      cbitmask.setBit(bidx, true);

	      bool clipped = false;
	      for(int i=0; i<cPos.count(); i++)
		{
		  Vec p = Vec(h2, w2, d2) - cPos[i];
		  if (cNorm[i]*p > 0)
		    {
		      clipped = true;
		      break;
		    }
		}
	      if (!clipped)
		{
		  qint64 idx = d2*m_width*m_height + w2*m_height + h2;
		  int val = volData[idx];
		  uchar mtag = maskData[idx];
		  bool opaque =  (lut[4*val+3]*Global::tagColors()[4*mtag+3] > 0);
		  opaque &= (mtag == tag);
		  if (opaque)
		    {
		      bitmask.setBit(bidx, true);
		      stack.push(Vec(d2,w2,h2));	      
		    }
		}
	    }
	}
    }

  progress.setLabelText("Dilate");
  qApp->processEvents();

  QList<Vec> edges;
  edges.clear();

  int nDilate = viewerUi.dilateRad->value()+1;

  // find  inner boundary
  for(int d=ds; d<=de; d++)
    {
      progress.setValue(90*(d-ds)/(mz));
      if (d%10 == 0)
	qApp->processEvents();
      for(int w=ws; w<=we; w++)
	for(int h=hs; h<=he; h++)
	  {
	    qint64 bidx = (d-ds)*mx*my+(w-ws)*mx+(h-hs);
	    if (bitmask.testBit(bidx))
	      {
		bool inside = true;
		for(int i=0; i<6; i++)
		  {
		    int da = indices[3*i+0];
		    int wa = indices[3*i+1];
		    int ha = indices[3*i+2];
		    
		    qint64 d2 = qBound(ds, d+da, de);
		    qint64 w2 = qBound(ws, w+wa, we);
		    qint64 h2 = qBound(hs, h+ha, he);
		    
		    qint64 tidx = (d2-ds)*mx*my+(w2-ws)*mx+(h2-hs);
		    inside &= bitmask.testBit(tidx);
		  }
		if (!inside)
		  edges << Vec(d,w,h);
	      }
	  }
    }

  for(int ne=0; ne<nDilate; ne++)
    {
      progress.setValue(90*(float)ne/(float)nDilate);
      qApp->processEvents();

      if (ne < nDilate-1)
	{
	  QList<Vec> tedges;
	  tedges.clear();

	  cbitmask.fill(false);
	  // find outer boundary to fill
	  for(int e=0; e<edges.count(); e++)
	    {
	      int dx = edges[e].x;
	      int wx = edges[e].y;
	      int hx = edges[e].z;
	      
	      for(int i=0; i<6; i++)
		{
		  int da = indices[3*i+0];
		  int wa = indices[3*i+1];
		  int ha = indices[3*i+2];
		  
		  qint64 d2 = qBound(ds, dx+da, de);
		  qint64 w2 = qBound(ws, wx+wa, we);
		  qint64 h2 = qBound(hs, hx+ha, he);
		  

		  bool clipped = false;
		  for(int i=0; i<cPos.count(); i++)
		    {
		      Vec p = Vec(h2, w2, d2) - cPos[i];
		      if (cNorm[i]*p > 0)
			{
			  clipped = true;
			  break;
			}
		    }
		  if (!clipped)
		    {
		      qint64 bidx = (d2-ds)*mx*my+(w2-ws)*mx+(h2-hs);
		      qint64 idx = d2*m_width*m_height + w2*m_height + h2;
		      int val = volData[idx];
		      if (lut[4*val+3] > 0 && // dilate only in connected region
			  maskData[idx] == 0 && // dilate only in zero mask region
			  !bitmask.testBit(bidx) &&
			  !cbitmask.testBit(bidx))
			{
			  cbitmask.setBit(bidx);
			  tedges << Vec(d2,w2,h2);	  
			  minD = qMin(minD, (int)d2);
			  maxD = qMax(maxD, (int)d2);
			  minW = qMin(minW, (int)w2);
			  maxW = qMax(maxW, (int)w2);
			  minH = qMin(minH, (int)h2);
			  maxH = qMax(maxH, (int)h2);
			}
		    } // not clipped
		}
	    }
	  // fill boundary
	  for(int e=0; e<tedges.count(); e++)
	    {
	      qint64 d2 = tedges[e].x;
	      qint64 w2 = tedges[e].y;
	      qint64 h2 = tedges[e].z;
	      
	      qint64 idx = d2*m_width*m_height + w2*m_height + h2;
	      maskData[idx] = tag;
	      
	      qint64 bidx = (d2-ds)*mx*my+(w2-ws)*mx+(h2-hs);
	      bitmask.setBit(bidx, true);
	    }
	  
	  edges = tedges;
	}
    }


  getSlice(m_currSlice);
  
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
DrishtiPaint::mergeTags(Vec bmin, Vec bmax, int tag1, int tag2, bool useTF)
{
  int m_depth, m_width, m_height;
  m_volume->gridSize(m_depth, m_width, m_height);

  int ds = bmin.z;
  int ws = bmin.y;
  int hs = bmin.x;

  int de = bmax.z;
  int we = bmax.y;
  int he = bmax.x;

  QProgressDialog progress("Updating voxel structure",
			   QString(),
			   0, 100,
			   0);
  progress.setMinimumDuration(0);

  int minD,maxD, minW,maxW, minH,maxH;
  minD = maxD = -1;
  minW = maxW = -1;
  minH = maxH = -1;

  uchar *maskData = m_volume->memMaskDataPtr();

  QList<Vec> cPos =  m_viewer->clipPos();
  QList<Vec> cNorm = m_viewer->clipNorm();

  if (useTF)
    {
      uchar *lut = Global::lut();
      uchar *volData = m_volume->memVolDataPtr();
      for(qint64 d=ds; d<=de; d++)
	{
	  progress.setValue(90*(d-ds)/((de-ds+1)));
	  if (d%10 == 0)
	    qApp->processEvents();
	  for(qint64 w=ws; w<=we; w++)
	    for(qint64 h=hs; h<=he; h++)
	      {
		bool clipped = false;
		for(int i=0; i<cPos.count(); i++)
		  {
		    Vec p = Vec(h, w, d) - cPos[i];
		    if (cNorm[i]*p > 0)
		      {
			clipped = true;
			break;
		      }
		  }
		
		if (!clipped)
		  {
		    qint64 idx = d*m_width*m_height + w*m_height + h;
		    if (tag2 == -1 || maskData[idx] == tag2)
		      {
			int val = volData[idx];
			int a =  lut[4*val+3];
			if (a > 0)
			  {
			    maskData[idx] = tag1;
			    if (minD > -1)
			      {
				minD = qMin(minD, (int)d);
				maxD = qMax(maxD, (int)d);
				minW = qMin(minW, (int)w);
				maxW = qMax(maxW, (int)w);
				minH = qMin(minH, (int)h);
				maxH = qMax(maxH, (int)h);
			      }
			    else
			      {
				minD = maxD = d;
				minW = maxW = w;
				minH = maxH = h;
			      }
			  }
		      }
		  }
	      }
	}
    }
  else
    {
      for(qint64 d=ds; d<=de; d++)
	{
	  progress.setValue(90*(d-ds)/((de-ds+1)));
	  for(qint64 w=ws; w<=we; w++)
	    for(qint64 h=hs; h<=he; h++)
	      {
		bool clipped = false;
		for(int i=0; i<cPos.count(); i++)
		  {
		    Vec p = Vec(h, w, d) - cPos[i];
		    if (cNorm[i]*p > 0)
		      {
			clipped = true;
			break;
		      }
		  }
		
		if (!clipped)
		  {
		    qint64 idx = d*m_width*m_height + w*m_height + h;
		    if (tag2 == -1 || maskData[idx] == tag2)
		      {
			maskData[idx] = tag1;
			if (minD > -1)
			  {
			    minD = qMin(minD, (int)d);
			    maxD = qMax(maxD, (int)d);
			    minW = qMin(minW, (int)w);
			    maxW = qMax(maxW, (int)w);
			    minH = qMin(minH, (int)h);
			    maxH = qMax(maxH, (int)h);
			  }
			else
			  {
			    minD = maxD = d;
			    minW = maxW = w;
			    minH = maxH = h;
			  }
		      }
		  }
	      }
	}
    }

  getSlice(m_currSlice);
  
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
DrishtiPaint::setVisible(Vec bmin, Vec bmax, int tag, bool visible)
{
  int m_depth, m_width, m_height;
  m_volume->gridSize(m_depth, m_width, m_height);

  int ds = bmin.z;
  int ws = bmin.y;
  int hs = bmin.x;

  int de = bmax.z;
  int we = bmax.y;
  int he = bmax.x;

  QProgressDialog progress("Updating voxel structure",
			   QString(),
			   0, 100,
			   0);
  progress.setMinimumDuration(0);

  int minD,maxD, minW,maxW, minH,maxH;
  minD = maxD = -1;
  minW = maxW = -1;
  minH = maxH = -1;

  QList<Vec> cPos =  m_viewer->clipPos();
  QList<Vec> cNorm = m_viewer->clipNorm();

  uchar *volData = m_volume->memVolDataPtr();
  uchar *maskData = m_volume->memMaskDataPtr();
  uchar *lut = Global::lut();

  for(qint64 d=ds; d<=de; d++)
    {
      progress.setValue(90*(d-ds)/((de-ds+1)));
      if (d%10 == 0)
	qApp->processEvents();
      for(qint64 w=ws; w<=we; w++)
	for(qint64 h=hs; h<=he; h++)
	  {
	    bool clipped = false;
	    for(int i=0; i<cPos.count(); i++)
	      {
		Vec p = Vec(h, w, d) - cPos[i];
		if (cNorm[i]*p > 0)
		  {
		    clipped = true;
		    break;
		  }
	      }
	    
	    if (!clipped)
	      {
		qint64 idx = d*m_width*m_height + w*m_height + h;
		int val = volData[idx];
		uchar mtag = maskData[idx];
		bool alpha =  (lut[4*val+3]*Global::tagColors()[4*mtag+3] > 0);

		if (alpha == visible && maskData[idx] != tag)
		  {
		    maskData[idx] = tag;
		    if (minD > -1)
		      {
			minD = qMin(minD, (int)d);
			maxD = qMax(maxD, (int)d);
			minW = qMin(minW, (int)w);
			maxW = qMax(maxW, (int)w);
			minH = qMin(minH, (int)h);
			maxH = qMax(maxH, (int)h);
		      }
		    else
		      {
			minD = maxD = d;
			minW = maxW = w;
			minH = maxH = h;
		      }
		  }
	      } // clipped
	  }
    }
  
  getSlice(m_currSlice);
  
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
			   0);
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
  uchar *maskData = m_volume->memMaskDataPtr();

  int indices[] = {-1, 0, 0,
		    1, 0, 0,
		    0,-1, 0,
		    0, 1, 0,
		    0, 0,-1,
		    0, 0, 1};

  qint64 dr,wr,hr;

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

	      Vec scr = m_viewer->camera()->projectedCoordinatesOf(Vec(h2,w2,d2));
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
  
  
  progress.setLabelText("Region growing");
  qApp->processEvents();

  QStack<Vec> stack;
  stack.push(Vec(dr,wr,hr));

  qint64 idx = ((qint64)dr)*m_width*m_height + (qint64)wr*m_height + hr;
  maskData[idx] = tag;

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

	      Vec scr = m_viewer->camera()->projectedCoordinatesOf(Vec(h2,w2,d2));
	      if (scr.x >= 0 && scr.x < spW &&
		  scr.y >= 0 && scr.y < spH)
		{
		  if (sketchPad[int(scr.x)+int(scr.y)*spW] > 0)
		    {
		      stack.push(Vec(d2,w2,h2));
		      
		      qint64 idx = d2*m_width*m_height + w2*m_height + h2;
		      int val = volData[idx];
		      bool visible = lut[4*val+3] > 0;
		      if (visible) // tag only visible region
			{
			  maskData[idx] = tag;	      
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

  getSlice(m_currSlice);
  
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
DrishtiPaint::on_actionSave_TF_triggered()
{
  QString tflnm = QFileDialog::getSaveFileName(0,
					       "Save Transfer Functions",
					       Global::previousDirectory(),
					       "Files (*.xml)",
					       0,
					       QFileDialog::DontUseNativeDialog);


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
  
  m_tfManager->save(tflnm.toLatin1().data());

}

void
DrishtiPaint::on_actionLoad_TF_triggered()
{
  QString tflnm = QFileDialog::getOpenFileName(0,
					       "Load Transfer Functions",
					       Global::previousDirectory(),
					       "Files (*.xml)",
					       0,
					       QFileDialog::DontUseNativeDialog);

  
  m_tfManager->load(tflnm.toLatin1().data());
}

void
DrishtiPaint::on_loadMask_triggered()
{
  QString flnm;
  flnm = QFileDialog::getOpenFileName(0,
				      "Load Mask File",
				      Global::previousDirectory(),
				      "MASK Files (*.mask)",
				      0,
				      QFileDialog::DontUseNativeDialog);

  
  if (flnm.isEmpty())
    return;


  uchar vt;
  int lrd, lrw, lrh;
  
  QFile mfile;
  mfile.setFileName(flnm);
  mfile.open(QFile::ReadOnly);

  mfile.read((char*)&vt, 1);
  mfile.read((char*)&lrd, 4);
  mfile.read((char*)&lrw, 4);
  mfile.read((char*)&lrh, 4);
  int m_depth, m_width, m_height;
  m_volume->gridSize(m_depth, m_width, m_height);

  float scld = (float)m_depth/lrd;
  float sclw = (float)m_width/lrw;
  float sclh = (float)m_height/lrh;

  QString mesg;
  mesg += QString("Volume Size : %1 %2 %3\n").\
                  arg(m_height).arg(m_width).arg(m_depth);
  mesg += QString("Input Mask Size : %1 %2 %3\n").\
                  arg(lrh).arg(lrw).arg(lrd);
  mesg += QString("Scaling applied : %1 %2 %3").\
		   arg(sclh).arg(sclw).arg(scld);
  QMessageBox::information(0, "", mesg);

  uchar *lmask;
  lmask = new uchar[(qint64)lrd*(qint64)lrw*(qint64)lrh];

  mfile.read((char*)lmask, (qint64)lrd*(qint64)lrw*(qint64)lrh);

  uchar *maskptr = m_volume->memMaskDataPtr();

  QProgressDialog progress("Updating voxel structure",
			   QString(),
			   0, 100,
			   0);
  progress.setMinimumDuration(0);

  for(qint64 d=0; d<lrd; d++)
    {
      progress.setValue((95.0*d)/lrd);
      for(qint64 w=0; w<lrw; w++)
      for(qint64 h=0; h<lrh; h++)
	{
	  if (lmask[d*lrw*lrh + w*lrh + h] > 0)
	    {
	      int ds = qMax(0, (int)(d*scld-scld/2));
	      int ws = qMax(0, (int)(w*sclw-sclw/2));
	      int hs = qMax(0, (int)(h*sclh-sclh/2));
	      int de = qMin(m_depth-1, (int)((d+1)*scld-scld/2));
	      int we = qMin(m_width-1, (int)((w+1)*sclw-sclw/2));
	      int he = qMin(m_height-1,(int)((h+1)*sclh-sclh/2));

	      uchar mv = lmask[d*lrw*lrh + w*lrh + h];
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

void
DrishtiPaint::shrinkwrapSlice(uchar *swvr, int mx, int my)
{
  MorphSlice ms;
  QList<QPolygonF> poly = ms.boundaryCurves(swvr, mx, my, true);
	
  memset(swvr, 0, my*mx);
  for (int npc=0; npc<poly.count(); npc++)
    {
      QImage pimg = QImage(mx, my, QImage::Format_RGB32);
      pimg.fill(0);
      QPainter p(&pimg);
      p.setPen(QPen(Qt::white, 1));
      p.setBrush(Qt::white);
      p.drawPolygon(poly[npc]);
      QRgb *rgb = (QRgb*)(pimg.bits());
      for(int i=0; i<my*mx; i++)
	swvr[i] = (swvr[i]>0 || qRed(rgb[i])>0 ? 255 : 0);  
    }  
}

void
DrishtiPaint::getConnectedRegion(int dr, int wr, int hr,
				 int ds, int ws, int hs,
				 int de, int we, int he,
				 int tag, bool zero,
				 MyBitArray& cbitmask)
{
  QProgressDialog progress("Identifying connected region",
			   QString(),
			   0, 100,
			   0);
  progress.setMinimumDuration(0);

  int m_depth, m_width, m_height;
  m_volume->gridSize(m_depth, m_width, m_height);

  qint64 mx = he-hs+1;
  qint64 my = we-ws+1;
  qint64 mz = de-ds+1;

  QList<Vec> cPos =  m_viewer->clipPos();
  QList<Vec> cNorm = m_viewer->clipNorm();

  uchar *lut = Global::lut();
  uchar *volData = m_volume->memVolDataPtr();
  uchar *maskData = m_volume->memMaskDataPtr();

  int indices[] = {-1, 0, 0,
		    1, 0, 0,
		    0,-1, 0,
		    0, 1, 0,
		    0, 0,-1,
		    0, 0, 1};

  //----------------------------  
  // identify visible region
  cbitmask.fill(false);

  progress.setLabelText("Identifying visible region");
  for(qint64 d2=ds; d2<=de; d2++)
  {
    progress.setLabelText(QString("Identifying visible region %1 of %2").arg(d2-ds).arg(mz));
    progress.setValue(90*(float)(d2-ds)/(float)mz);
    qApp->processEvents();
    for(qint64 w2=ws; w2<=we; w2++)
    for(qint64 h2=hs; h2<=he; h2++)
    {
      bool clipped = false;
      for(int i=0; i<cPos.count(); i++)
	{
	  Vec p = Vec(h2, w2, d2) - cPos[i];
	  if (cNorm[i]*p > 0)
	    {
	      clipped = true;
	      break;
	    }
	}
      
      qint64 idx = d2*m_width*m_height + w2*m_height + h2;
      int val = volData[idx];
      uchar mtag = maskData[idx];
      bool opaque =  (lut[4*val+3]*Global::tagColors()[4*mtag+3] > 0);      
      if (tag > -1)
	{
	  if (zero)
	    opaque &= (mtag == 0 || mtag == tag);
	  else
	    opaque &= (mtag == tag);
	}
      // grow only in zero or same tagged region
      // or if tag is 0 then grow in all visible regions
      if (!clipped && opaque)
	{
	  qint64 bidx = (d2-ds)*mx*my+(w2-ws)*mx+(h2-hs);
	  cbitmask.setBit(bidx, true);
	}  // visible voxel
    }
  }
  //----------------------------  

  MyBitArray bitmask;
  bitmask.resize(mx*my*mz);
  bitmask.fill(false);

  QList<Vec> edges;
  edges.clear();
  edges << Vec(dr,wr,hr);
  qint64 bidx = (dr-ds)*mx*my+(wr-ws)*mx+(hr-hs);
  bitmask.setBit(bidx, true);

  //------------------------------------------------------
  // dilate from seed
  bool done = false;
  int nd = 0;
  int pvnd = 0;
  while(!done)
    {
      nd = (nd + 1)%100;
      int pnd = 90*(float)nd/(float)100;
      progress.setValue(pnd);
      if (pnd != pvnd)
	qApp->processEvents();
      pvnd = pnd;

      QList<Vec> tedges;
      tedges.clear();

      progress.setLabelText(QString("Flooding %1").arg(edges.count()));
      qApp->processEvents();

            
      // find outer boundary to fill
      for(int e=0; e<edges.count(); e++)
	{
	  int dx = edges[e].x;
	  int wx = edges[e].y;
	  int hx = edges[e].z;
	  	  
	  for(int i=0; i<6; i++)
	    {
	      int da = indices[3*i+0];
	      int wa = indices[3*i+1];
	      int ha = indices[3*i+2];
	      
	      qint64 d2 = qBound(ds, dx+da, de);
	      qint64 w2 = qBound(ws, wx+wa, we);
	      qint64 h2 = qBound(hs, hx+ha, he);
	      
	      qint64 bidx = (d2-ds)*mx*my+(w2-ws)*mx+(h2-hs);
	      if (cbitmask.testBit(bidx) &&
		  !bitmask.testBit(bidx))
		{
		  bitmask.setBit(bidx, true);
		  tedges << Vec(d2,w2,h2);		  
		}
	    }
	}

      edges.clear();

      if (tedges.count() > 0)
	edges = tedges;
      else
	done = true;

      tedges.clear();
    }
  //------------------------------------------------------

  // copy bitmask into cbitmask
  for(qint64 i=0; i<mx*my*mz; i++)
    cbitmask.setBit(i, bitmask.testBit(i));
}

void
DrishtiPaint::getTransparentRegion(int ds, int ws, int hs,
				   int de, int we, int he,
				   MyBitArray& cbitmask)
{
  QProgressDialog progress("Identifying transparent region",
			   QString(),
			   0, 100,
			   0);
  progress.setMinimumDuration(0);

  int m_depth, m_width, m_height;
  m_volume->gridSize(m_depth, m_width, m_height);

  qint64 mx = he-hs+1;
  qint64 my = we-ws+1;
  qint64 mz = de-ds+1;

  QList<Vec> cPos =  m_viewer->clipPos();
  QList<Vec> cNorm = m_viewer->clipNorm();

  uchar *lut = Global::lut();
  uchar *volData = m_volume->memVolDataPtr();
  uchar *maskData = m_volume->memMaskDataPtr();

  for(qint64 d2=ds; d2<=de; d2++)
  {
    progress.setValue(90*(float)(d2-ds)/(float)mz);
    qApp->processEvents();
    for(qint64 w2=ws; w2<=we; w2++)
    for(qint64 h2=hs; h2<=he; h2++)
    {
      bool clipped = false;
      for(int i=0; i<cPos.count(); i++)
	{
	  Vec p = Vec(h2, w2, d2) - cPos[i];
	  if (cNorm[i]*p > 0)
	    {
	      clipped = true;
	      break;
	    }
	}
      
      qint64 idx = d2*m_width*m_height + w2*m_height + h2;
      int val = volData[idx];
      uchar mtag = maskData[idx];
      bool transparent =  (lut[4*val+3]*Global::tagColors()[4*mtag+3] == 0);
      if (clipped || transparent)
	{
	  qint64 bidx = (d2-ds)*mx*my+(w2-ws)*mx+(h2-hs);
	  cbitmask.setBit(bidx, true);
	}  // transparent voxel
    }
  }
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
  int m_depth, m_width, m_height;
  m_volume->gridSize(m_depth, m_width, m_height);

  //-------------------------
  int holeSize = 0;
  holeSize = QInputDialog::getInt(0,
				  "Fill Holes",
				  "Size of holes to fill",
				  0, 0, 100, 1);
  //-------------------------

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

  MyBitArray cbitmask;
  cbitmask.resize(mx*my*mz);
  cbitmask.fill(false);

  uchar *maskData = m_volume->memMaskDataPtr();

  int indices[] = {-1, 0, 0,
		    1, 0, 0,
		    0,-1, 0,
		    0, 1, 0,
		    0, 0,-1,
		    0, 0, 1};

  QList<Vec> cPos =  m_viewer->clipPos();
  QList<Vec> cNorm = m_viewer->clipNorm();

  //----------------------------  
  if (all) // identify transparent region  
    getTransparentRegion(ds, ws, hs, de, we, he, cbitmask);
  else // identify connected region
    {
      getConnectedRegion(dr, wr, hr,
			 ds, ws, hs,
			 de, we, he,
			 ctag, false,
			 cbitmask);
      // invert all values in cbitmask
      cbitmask.invert();
    }
  //----------------------------  


  //----------------------------
  // fill the holes before shrinkwrapping the region
  if (holeSize > 0)
    {
      MyBitArray o_bitmask;
      o_bitmask.resize(mx*my*mz);
      // make a copy of bitmask into o_bitmask
      for(qint64 i=0; i<mx*my*mz; i++)
	o_bitmask.setBit(i, cbitmask.testBit(i));
  
      // dilation
      dilateBitmask(holeSize, false, // dilate opaque (false) region
		    mx, my, mz,
		    cbitmask);

      // followed by erosion
      dilateBitmask(holeSize, true, // dilate transparent (true) region
		    mx, my, mz,
		    cbitmask);

      // merge the original back in after erosion
      for(qint64 i=0; i<mx*my*mz; i++)
	cbitmask.setBit(i, cbitmask.testBit(i) & o_bitmask.testBit(i));
    }
  //----------------------------  


  QProgressDialog progress("Shrinkwrap",
			   QString(),
			   0, 100,
			   0);
  progress.setMinimumDuration(0);

  int startd = de;
  int startw = we;
  int starth = he;
  int endd = ds;
  int endw = ws;
  int endh = hs;
  for(qint64 d2=ds; d2<=de; d2++)
  {
    progress.setValue(90*(float)(d2-ds)/(float)mz);
    qApp->processEvents();
    for(qint64 w2=ws; w2<=we; w2++)
    for(qint64 h2=hs; h2<=he; h2++)
    {
      qint64 bidx = (d2-ds)*mx*my+(w2-ws)*mx+(h2-hs);
      if (cbitmask.testBit(bidx) == false) // opaque voxels
	{
	  startd = qMin(startd, (int)d2);
	  startw = qMin(startw, (int)w2);
	  starth = qMin(starth, (int)h2);
	  endd = qMax(endd, (int)d2);
	  endw = qMax(endw, (int)w2);
	  endh = qMax(endh, (int)h2);
	}
    }
  }
//  QMessageBox::information(0, "", QString("%1 %2 %3\n%4 %5 %6").\
//			   arg(startd).arg(startw).arg(starth).\
//			   arg(endd).arg(endw).arg(endh));
  //----------------------------  

  //----------------------------  
  // set the non-transparent block
  for(qint64 d2=startd; d2<=endd; d2++)
  for(qint64 w2=startw; w2<=endw; w2++)
  for(qint64 h2=starth; h2<=endh; h2++)
    {
      qint64 bidx = (d2-ds)*mx*my+(w2-ws)*mx+(h2-hs);
      bitmask.setBit(bidx, true);
    }
  //----------------------------  

  
  QList<Vec> edges;
  edges.clear();

  //----------------------------
  // set all size faces
  //------------------------------------------------------
  { // handle depth slices
    uchar *swvr = new uchar[my*mx];
    for(qint64 d=startd; d<=endd; d=endd)
      {
	progress.setLabelText(QString("Z %1").arg(d));
	qApp->processEvents();
	
	memset(swvr, 0, my*mx);
	
	for(qint64 w=ws; w<=we; w++)
	  for(qint64 h=hs; h<=he; h++)
	    {
	      qint64 bidx = (d-ds)*mx*my+(w-ws)*mx+(h-hs);
	      if (!cbitmask.testBit(bidx))
		swvr[(w-ws)*mx + (h-hs)] = 255;
	    }

	shrinkwrapSlice(swvr, mx, my);
	
	for(qint64 w=ws; w<=we; w++)
	  for(qint64 h=hs; h<=he; h++)
	    {
	      if (swvr[(w-ws)*mx + (h-hs)] == 0)
		{
		  qint64 bidx = (d-ds)*mx*my+(w-ws)*mx+(h-hs);
		  bitmask.setBit(bidx, false);
		  edges << Vec(d,w,h);
		}
	    }
	if (d == endd)
	  break;
      }
    delete [] swvr;
  }
  //------------------------------------------------------
  { // handle width slices
    uchar *swvr = new uchar[mz*mx];
    for(qint64 w=startw; w<=endw; w=endw)
      {
	progress.setLabelText(QString("Y %1").arg(w));
	qApp->processEvents();
		
	memset(swvr, 0, mz*mx);
	
	for(qint64 d=ds; d<=de; d++)
	  for(qint64 h=hs; h<=he; h++)
	    {
	      qint64 bidx = (d-ds)*mx*my+(w-ws)*mx+(h-hs);
	      if (!cbitmask.testBit(bidx))
		swvr[(d-ds)*mx + (h-hs)] = 255;
	    }
	
	shrinkwrapSlice(swvr, mx, mz);
	
	for(qint64 d=ds; d<=de; d++)
	  for(qint64 h=hs; h<=he; h++)
	    {
	      if (swvr[(d-ds)*mx + (h-hs)] == 0)
		{
		  qint64 bidx = (d-ds)*mx*my+(w-ws)*mx+(h-hs);
		  bitmask.setBit(bidx, false);
		  edges << Vec(d,w,h);
		}
	    }
	if (w == endw)
	  break;
      }
    delete [] swvr;
  }
  //------------------------------------------------------

  //------------------------------------------------------
  { // handle height slices
    uchar *swvr = new uchar[mz*my];
    for(qint64 h=starth; h<=endh; h=endh)
      {
	progress.setLabelText(QString("X %1").arg(h));
	qApp->processEvents();
	
	memset(swvr, 0, mz*my);
	
	for(qint64 d=ds; d<=de; d++)
	  for(qint64 w=ws; w<=we; w++)
	    {
	      qint64 bidx = (d-ds)*mx*my+(w-ws)*mx+(h-hs);
	      if (!cbitmask.testBit(bidx))
		swvr[(d-ds)*my + (w-ws)] = 255;
	    }
	
	shrinkwrapSlice(swvr, my, mz);
		
	for(qint64 d=ds; d<=de; d++)
	  for(qint64 w=ws; w<=we; w++)
	    {
	      if (swvr[(d-ds)*my + (w-ws)] == 0)
		{
		  qint64 bidx = (d-ds)*mx*my+(w-ws)*mx+(h-hs);
		  bitmask.setBit(bidx, false);
		  edges << Vec(d,w,h);
		}
	    }
	if (h == endh)
	  break;
      }
    delete [] swvr;
  }
  //------------------------------------------------------

  //------------------------------------------------------
  // now dilate from boundary
  bool done = false;
  int nd = 0;
  int pvnd = 0;
  while(!done)
    {
      nd = (nd + 1)%100;
      int pnd = 90*(float)nd/(float)100;
      progress.setValue(pnd);
      if (pnd != pvnd)
	qApp->processEvents();
      pvnd = pnd;

      QList<Vec> tedges;
      tedges.clear();

      progress.setLabelText(QString("Boundary detection %1").arg(edges.count()));
      qApp->processEvents();

            
      // find outer boundary to fill
      for(int e=0; e<edges.count(); e++)
	{
	  int dx = edges[e].x;
	  int wx = edges[e].y;
	  int hx = edges[e].z;
	  	  
	  for(int i=0; i<6; i++)
	    {
	      int da = indices[3*i+0];
	      int wa = indices[3*i+1];
	      int ha = indices[3*i+2];
	      
	      qint64 d2 = qBound(ds, dx+da, de);
	      qint64 w2 = qBound(ws, wx+wa, we);
	      qint64 h2 = qBound(hs, hx+ha, he);
	      
	      qint64 bidx = (d2-ds)*mx*my+(w2-ws)*mx+(h2-hs);
	      if (cbitmask.testBit(bidx) &&
		  bitmask.testBit(bidx))
		{
		  bitmask.setBit(bidx, false);
		  tedges << Vec(d2,w2,h2);		  
		}
	    }
	}

      edges.clear();

      if (tedges.count() > 0)
	edges = tedges;
      else
	done = true;

      tedges.clear();
    }
  //------------------------------------------------------

  //------------------------------------------------------
  // if we want only shell remove interior
  if (shellOnly)
    {
      progress.setLabelText("Tag shell voxels");

      // copy bitmask into cbitmask
      for(qint64 i=0; i<mx*my*mz; i++)
	cbitmask.setBit(i, bitmask.testBit(i));

      bitmask.fill(false);
      
      for(qint64 d=ds; d<=de; d++)
	{
	  progress.setValue(90*(d-ds)/mz);
	  if (d%10 == 0) qApp->processEvents();
	
	  for(qint64 w=ws; w<=we; w++)
	  for(qint64 h=hs; h<=he; h++)
	    {
	      qint64 bidx = (d-ds)*mx*my+(w-ws)*mx + (h-hs);
	      if (cbitmask.testBit(bidx))
		{
		  qint64 d2s = qBound(ds, (int)d-1, de);
		  qint64 w2s = qBound(ws, (int)w-1, we);
		  qint64 h2s = qBound(hs, (int)h-1, he);
		  qint64 d2e = qBound(ds, (int)d+1, de);
		  qint64 w2e = qBound(ws, (int)w+1, we);
		  qint64 h2e = qBound(hs, (int)h+1, he);
		  
		  bool ok = true;
		  for(qint64 d2=d2s; d2<=d2e; d2++)
		  for(qint64 w2=w2s; w2<=w2e; w2++)
		  for(qint64 h2=h2s; h2<=h2e; h2++)
		    {
		      qint64 cidx = (d2-ds)*mx*my+(w2-ws)*mx+(h2-hs);
		      if (!cbitmask.testBit(cidx))
			{
			  ok = false;
			  break;
			}			  
		    }
		  
		  if (!ok) // boundary voxel
		    bitmask.setBit(bidx, true);
		}
	    }
	}
      
      MyBitArray dbitmask;
      dbitmask.resize(mx*my*mz);
      
      for(int nd=1; nd<shellThickness; nd++)
	{
	  dbitmask.fill(false);
	  progress.setLabelText(QString("shell no. %1").arg(nd));
	  for(qint64 d=ds; d<=de; d++)
	    {
	      progress.setValue(99*(d-ds)/mz);
	      if (d%10 == 0) qApp->processEvents();
	      
	      for(qint64 w=ws; w<=we; w++)
	      for(qint64 h=hs; h<=he; h++)
		{
		  qint64 bidx = (d-ds)*mx*my+(w-ws)*mx + (h-hs);
		  if (bitmask.testBit(bidx) && cbitmask.testBit(bidx))
		    {
		      cbitmask.setBit(bidx, false);
		      qint64 d2s = qBound(ds, (int)d-1, de);
		      qint64 w2s = qBound(ws, (int)w-1, we);
		      qint64 h2s = qBound(hs, (int)h-1, he);
		      qint64 d2e = qBound(ds, (int)d+1, de);
		      qint64 w2e = qBound(ws, (int)w+1, we);
		      qint64 h2e = qBound(hs, (int)h+1, he);
		      for(qint64 d2=d2s; d2<=d2e; d2++)
			for(qint64 w2=w2s; w2<=w2e; w2++)
			  for(qint64 h2=h2s; h2<=h2e; h2++)
			    {
			      qint64 cidx = (d2-ds)*mx*my+(w2-ws)*mx+(h2-hs);
			      if (cbitmask.testBit(cidx))
				dbitmask.setBit(cidx, true);
			    }
		    }
		}
	    }
	  // OR dbitmask into bitmask
	  for(qint64 i=0; i<mx*my*mz; i++)
	    bitmask.setBit(i, bitmask.testBit(i) || dbitmask.testBit(i));
	  
	} // shellThickness
    }
  //------------------------------------------------------

  //----------------------------  
  // now set the maskData
  for(qint64 d2=ds; d2<=de; d2++)
  for(qint64 w2=ws; w2<=we; w2++)
  for(qint64 h2=hs; h2<=he; h2++)
    {
      qint64 bidx = (d2-ds)*mx*my+(w2-ws)*mx+(h2-hs);
      if (bitmask.testBit(bidx))
	{
	  qint64 idx = d2*m_width*m_height + w2*m_height + h2;
	  maskData[idx] = tag;
	}
    }
  //----------------------------  

  getSlice(m_currSlice);
  
  progress.setLabelText("Update modified region on gpu");
  qApp->processEvents();
  m_viewer->uploadMask(ds,ws,hs, de,we,he);

  QList<int> dwh;  
  m_blockList.clear();  
  dwh << ds << ws << hs;
  m_blockList << dwh;
  dwh.clear();
  dwh << de << we << he;
  m_blockList << dwh;

  progress.setLabelText("Save modified region to mask file");
  qApp->processEvents();
  m_volume->saveMaskBlock(m_blockList);

  progress.setValue(100);
}

void
DrishtiPaint::resetTag(Vec bmin, Vec bmax, int tag)
{
  int m_depth, m_width, m_height;
  m_volume->gridSize(m_depth, m_width, m_height);

  int ds = bmin.z;
  int ws = bmin.y;
  int hs = bmin.x;

  int de = bmax.z;
  int we = bmax.y;
  int he = bmax.x;

  QProgressDialog progress("Updating voxel structure",
			   QString(),
			   0, 100,
			   0);
  progress.setMinimumDuration(0);

  int minD,maxD, minW,maxW, minH,maxH;
  minD = maxD = -1;
  minW = maxW = -1;
  minH = maxH = -1;

  QList<Vec> cPos =  m_viewer->clipPos();
  QList<Vec> cNorm = m_viewer->clipNorm();

  uchar *maskData = m_volume->memMaskDataPtr();
  for(qint64 d=ds; d<=de; d++)
    {
      progress.setValue(90*(d-ds)/((de-ds+1)));
      if (d%10 == 0)
	qApp->processEvents();
      for(qint64 w=ws; w<=we; w++)
	for(qint64 h=hs; h<=he; h++)
	  {
	    bool clipped = false;
	    for(int i=0; i<cPos.count(); i++)
	      {
		Vec p = Vec(h, w, d) - cPos[i];
		if (cNorm[i]*p > 0)
		  {
		    clipped = true;
		    break;
		  }
	      }
	    
	    if (!clipped)
	      {
		qint64 idx = d*m_width*m_height + w*m_height + h;
		maskData[idx] = tag;
		if (minD > -1)
		  {
		    minD = qMin(minD, (int)d);
		    maxD = qMax(maxD, (int)d);
		    minW = qMin(minW, (int)w);
		    maxW = qMax(maxW, (int)w);
		    minH = qMin(minH, (int)h);
		    maxH = qMax(maxH, (int)h);
		  }
		else
		  {
		    minD = maxD = d;
		    minW = maxW = w;
		    minH = maxH = h;
		  }
	      } // clipped
	  }
    }
  
  getSlice(m_currSlice);
  
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
DrishtiPaint::connectedRegion(int dr, int wr, int hr, Vec bmin, Vec bmax, int tag)
{
  int m_depth, m_width, m_height;
  m_volume->gridSize(m_depth, m_width, m_height);

  if (dr < 0 || wr < 0 || hr < 0 ||
      dr > m_depth-1 ||
      wr > m_width-1 ||
      hr > m_height-1)
    {
      QMessageBox::information(0, "", "No painted region found");
      return;
    }

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

  uchar *maskData = m_volume->memMaskDataPtr();

  getConnectedRegion(dr, wr, hr,
		     ds, ws, hs,
		     de, we, he,
		     tag, true,
		     bitmask);

  QProgressDialog progress("Updating voxel structure",
			   QString(),
			   0, 100,
			   0);
  progress.setMinimumDuration(0);

  //----------------------------  
  // set the maskData
  int minD,maxD, minW,maxW, minH,maxH;
  minD = maxD = dr;
  minW = maxW = wr;
  minH = maxH = hr;
  for(qint64 d2=ds; d2<=de; d2++)
  for(qint64 w2=ws; w2<=we; w2++)
  for(qint64 h2=hs; h2<=he; h2++)
    {
      qint64 bidx = (d2-ds)*mx*my+(w2-ws)*mx+(h2-hs);
      if (bitmask.testBit(bidx))
	{
	  qint64 idx = d2*m_width*m_height + w2*m_height + h2;
	  maskData[idx] = tag;
	  minD = qMin(minD, (int)d2);
	  maxD = qMax(maxD, (int)d2);
	  minW = qMin(minW, (int)w2);
	  maxW = qMax(maxW, (int)w2);
	  minH = qMin(minH, (int)h2);
	  maxH = qMax(maxH, (int)h2);
	}
    }
  //----------------------------  

  getSlice(m_currSlice);
  
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
DrishtiPaint::dilateBitmask(int nDilate, bool htype,
			    qint64 mx, qint64 my, qint64 mz,
			    MyBitArray &bitmask)
{
  QProgressDialog progress("Dilate bitmask",
			   QString(),
			   0, 100,
			   0);
  progress.setMinimumDuration(0);

  int indices[] = {-1, 0, 0,
		    1, 0, 0,
		    0,-1, 0,
		    0, 1, 0,
		    0, 0,-1,
		    0, 0, 1};

  QList<Vec> edges;
  edges.clear();

  // find  inner boundary
  for(qint64 d=0; d<mz; d++)
    {
      progress.setValue(90*(float)d/(float)mz);
      if (d%10 == 0)
	qApp->processEvents();
      for(qint64 w=0; w<my; w++)
	for(qint64 h=0; h<mz; h++)
	  {
	    qint64 bidx = d*mx*my+w*mx+h;
	    if (bitmask.testBit(bidx) == htype)
	      {
		bool inside = true;
		for(int i=0; i<6; i++)
		  {
		    int da = indices[3*i+0];
		    int wa = indices[3*i+1];
		    int ha = indices[3*i+2];
		    
		    qint64 d2 = qBound((qint64)0, d+da, mz-1);
		    qint64 w2 = qBound((qint64)0, w+wa, my-1);
		    qint64 h2 = qBound((qint64)0, h+ha, mx-1);
		    
		    qint64 tidx = d2*mx*my+w2*mx+h2;
		    inside &= (bitmask.testBit(tidx) == htype);
		  }
		if (!inside)
		  edges << Vec(d,w,h);
	      }
	  }
    }

  for(int ne=0; ne<nDilate; ne++)
    {
      progress.setValue(90*(float)ne/(float)nDilate);
      qApp->processEvents();

      QList<Vec> tedges;
      tedges.clear();
      
      // find outer boundary to fill
      for(int e=0; e<edges.count(); e++)
	{
	  int dx = edges[e].x;
	  int wx = edges[e].y;
	  int hx = edges[e].z;
	  
	  for(int i=0; i<6; i++)
	    {
	      int da = indices[3*i+0];
	      int wa = indices[3*i+1];
	      int ha = indices[3*i+2];
	      
	      qint64 d2 = qBound(0, dx+da, (int)mz-1);
	      qint64 w2 = qBound(0, wx+wa, (int)my-1);
	      qint64 h2 = qBound(0, hx+ha, (int)mx-1);
	      
	      qint64 bidx = d2*mx*my+w2*mx+h2;
	      if (bitmask.testBit(bidx) != htype)
		{
		  bitmask.setBit(bidx, htype);
		  tedges << Vec(d2,w2,h2);	  
		}
	    }
	}

      edges = tedges;	  
    }

  progress.setValue(100);
}

void
DrishtiPaint::getVolume(Vec bmin, Vec bmax, int tag)
{
  int m_depth, m_width, m_height;
  m_volume->gridSize(m_depth, m_width, m_height);

  int ds = bmin.z;
  int ws = bmin.y;
  int hs = bmin.x;

  int de = bmax.z;
  int we = bmax.y;
  int he = bmax.x;

  QProgressDialog progress("Updating voxel structure",
			   QString(),
			   0, 100,
			   0);
  progress.setMinimumDuration(0);

  QList<Vec> cPos =  m_viewer->clipPos();
  QList<Vec> cNorm = m_viewer->clipNorm();

  uchar *lut = Global::lut();
  uchar *volData = m_volume->memVolDataPtr();
  uchar *maskData = m_volume->memMaskDataPtr();

  qint64 nvoxels = 0;
  for(qint64 d=ds; d<=de; d++)
    {
      progress.setValue(90*(d-ds)/((de-ds+1)));
      if (d%10 == 0)
	qApp->processEvents();
      for(qint64 w=ws; w<=we; w++)
	for(qint64 h=hs; h<=he; h++)
	  {
	    bool clipped = false;
	    for(int i=0; i<cPos.count(); i++)
	      {
		Vec p = Vec(h, w, d) - cPos[i];
		if (cNorm[i]*p > 0)
		  {
		    clipped = true;
		    break;
		  }
	      }
	    
	    if (!clipped)
	      {
		qint64 idx = d*m_width*m_height + w*m_height + h;
		int val = volData[idx];
		uchar mtag = maskData[idx];
		bool opaque = (lut[4*val+3]*Global::tagColors()[4*mtag+3] > 0);      
		if (tag > -1)
		  opaque &= (mtag == tag);

		if (opaque)
		  nvoxels ++;
	      }
	  }
    }

  progress.setValue(100);

  VolumeInformation pvlInfo;
  pvlInfo = VolumeInformation::volumeInformation();
  Vec voxelSize = pvlInfo.voxelSize;
  float voxvol = nvoxels*voxelSize.x*voxelSize.y*voxelSize.z;

  QString mesg;
  mesg += QString("Visible Voxels : %1\n").arg(nvoxels);
  mesg += QString("Voxel Size : %1,%2,%3 %4\n").\
                  arg(voxelSize.x).arg(voxelSize.y).arg(voxelSize.z).
                  arg(pvlInfo.voxelUnitStringShort());
  mesg += QString("Volume : %1 %2^3\n").	\
                  arg(voxvol).\
                  arg(pvlInfo.voxelUnitStringShort());

  QMessageBox::information(0, "", mesg);
}

