#include "drishtipaint.h"
#include "global.h"
#include "staticfunctions.h"

#include <QDockWidget>
#include <QFileDialog>
#include <QInputDialog>
#include <QScrollArea>


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


  m_tagColorEditor = new TagColorEditor();

  m_viewer = new Viewer();

  //------------------------------
  // viewer menu
  QFrame *viewerMenu = new QFrame();
  viewerUi.setupUi(viewerMenu);
  connect(viewerUi.update, SIGNAL(clicked()),
	  m_viewer, SLOT(updateVoxels()));
  connect(viewerUi.interval, SIGNAL(sliderReleased()),
	  m_viewer, SLOT(updateVoxels()));
  connect(viewerUi.interval, SIGNAL(valueChanged(int)),
	  m_viewer, SLOT(setVoxelInterval(int)));
  connect(viewerUi.ptsize, SIGNAL(valueChanged(int)),
	  m_viewer, SLOT(setPointSize(int)));
  connect(viewerUi.voxchoice, SIGNAL(currentIndexChanged(int)),
	  m_viewer, SLOT(setVoxelChoice(int)));
  connect(viewerUi.box, SIGNAL(clicked(bool)),
	  m_viewer, SLOT(setShowBox(bool)));
  connect(viewerUi.snapshot, SIGNAL(clicked()),
	  m_viewer, SLOT(saveImage()));
  connect(viewerUi.paintedtags, SIGNAL(editingFinished()),
	  this, SLOT(paintedtag_editingFinished()));
  connect(viewerUi.curvetags, SIGNAL(editingFinished()),
	  this, SLOT(curvetag_editingFinished()));
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

  addDockWidget(Qt::RightDockWidgetArea, dock1);
  addDockWidget(Qt::RightDockWidgetArea, dockV);
  addDockWidget(Qt::LeftDockWidgetArea, dock2);


  m_bitmapThread = new BitmapThread();

  m_volume = new Volume();
  m_volume->setBitmapThread(m_bitmapThread);

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
  
  on_actionCurves_triggered();
  ui.lwsettingpanel->setVisible(false);
  ui.closed->setChecked(true);

  Global::setBoxSize(5);
  Global::setSpread(10);
  Global::setLambda(10);
  Global::setSmooth(1);
  Global::setPrevErode(5);

  ui.tag->setValue(Global::tag());
  ui.boxSize->setValue(Global::boxSize());
  ui.pointsize->setValue(7);
  ui.lambda->setValue(Global::lambda());
  ui.radius->setValue(Global::spread());
  ui.smooth->setValue(Global::smooth());
  ui.preverode->setValue(Global::prevErode());


  //------------------------
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

  connect(m_imageWidget, SIGNAL(getSlice(int)),
	  this, SLOT(getSlice(int)));  

  connect(m_imageWidget, SIGNAL(getRawValue(int, int, int)),
	  this, SLOT(getRawValue(int, int, int)));

  connect(m_imageWidget, SIGNAL(applyMaskOperation(int, int, int)),
	  this, SLOT(applyMaskOperation(int, int, int)));

  connect(m_tagColorEditor, SIGNAL(tagColorChanged()),
	  m_imageWidget, SLOT(updateTagColors()));

  connect(m_tagColorEditor, SIGNAL(tagSelected(int)),
	  this, SLOT(tagSelected(int)));

  connect(m_slider, SIGNAL(valueChanged(int)),
	  m_imageWidget, SLOT(sliceChanged(int)));

  connect(m_slider, SIGNAL(userRangeChanged(int, int)),
	  m_imageWidget, SLOT(userRangeChanged(int, int)));

  connect(m_imageWidget, SIGNAL(fillVolume(int, int,
					   int, int,
					   int, int,
					   QList<int>,
					   bool)),
	  this, SLOT(fillVolume(int, int,
				int, int,
				int, int,
				QList<int>,
				bool)));

  connect(m_imageWidget, SIGNAL(tagDSlice(int, QImage)),
	  this, SLOT(tagDSlice(int, QImage)));

  connect(m_imageWidget, SIGNAL(tagWSlice(int, QImage)),
	  this, SLOT(tagWSlice(int, QImage)));

  connect(m_imageWidget, SIGNAL(tagHSlice(int, QImage)),
	  this, SLOT(tagHSlice(int, QImage)));

  connect(m_imageWidget, SIGNAL(tagDSlice(int, uchar*)),
	  this, SLOT(tagDSlice(int, uchar*)));

  connect(m_imageWidget, SIGNAL(tagWSlice(int, uchar*)),
	  this, SLOT(tagWSlice(int, uchar*)));

  connect(m_imageWidget, SIGNAL(tagHSlice(int, uchar*)),
	  this, SLOT(tagHSlice(int, uchar*)));

  connect(m_imageWidget, SIGNAL(tagAllVisible(int, int,
					      int, int,
					      int, int)),
	  this, SLOT(tagAllVisible(int, int,
				   int, int,
				   int, int)));

  connect(m_imageWidget, SIGNAL(dilate()),
	  this, SLOT(dilate()));
  connect(m_imageWidget, SIGNAL(dilate(int, int,
				       int, int,
				       int, int)),
	  this, SLOT(dilate(int, int,
			    int, int,
			    int, int)));

  connect(m_imageWidget, SIGNAL(erode()),
	  this, SLOT(erode()));
  connect(m_imageWidget, SIGNAL(erode(int, int,
				      int, int,
				      int, int)),
	  this, SLOT(erode(int, int,
			   int, int,
			   int, int)));

  connect(m_imageWidget, SIGNAL(polygonLevels(QList<int>)),
	  m_slider, SLOT(polygonLevels(QList<int>)));
  

  connect(m_imageWidget, SIGNAL(updateViewerBox(int, int, int, int, int, int)),
	  m_viewer, SLOT(updateViewerBox(int, int, int, int, int, int)));
  //------------------------


  loadSettings();
  m_imageWidget->updateTagColors();
}

void DrishtiPaint::on_actionHelp_triggered() { m_imageWidget->showHelp(); }

void DrishtiPaint::on_saveImage_triggered() { m_imageWidget->saveImage(); }

void
DrishtiPaint::on_actionCurves_triggered()
{
  ui.actionGraphCut->setChecked(false);
  ui.graphcutBox->hide();
  
  ui.actionCurves->setChecked(true);
  ui.curvesBox->show();
  m_imageWidget->setCurve(true);
  if (m_volume->isValid())
    {
      ui.livewire->setChecked(true);
      m_imageWidget->setLivewire(true);
    }
}

void
DrishtiPaint::on_actionGraphCut_triggered()
{
  ui.actionGraphCut->setChecked(true);
  ui.graphcutBox->show();
  
  ui.actionCurves->setChecked(false);
  ui.curvesBox->hide();
  m_imageWidget->setCurve(false);
  ui.livewire->setChecked(false);

  ui.modify->setChecked(false);
  ui.propagate->setChecked(false);
  m_imageWidget->freezeModifyUsingLivewire();
}

void
DrishtiPaint::tagDSlice(int currslice, QImage usermask)
{
  m_volume->tagDSlice(currslice, usermask.bits(), false);
  getMaskSlice(m_currSlice);
}
void
DrishtiPaint::tagWSlice(int currslice, QImage usermask)
{
  m_volume->tagWSlice(currslice, usermask.bits(), false);
  getMaskSlice(m_currSlice);
}
void
DrishtiPaint::tagHSlice(int currslice, QImage usermask)
{
  m_volume->tagHSlice(currslice, usermask.bits(), false);
  getMaskSlice(m_currSlice);
}


void
DrishtiPaint::tagDSlice(int currslice, uchar* tags)
{
  m_volume->tagDSlice(currslice, tags, true);
}
void
DrishtiPaint::tagWSlice(int currslice, uchar* tags)
{
  m_volume->tagWSlice(currslice, tags, true);
}
void
DrishtiPaint::tagHSlice(int currslice, uchar* tags)
{
  m_volume->tagHSlice(currslice, tags, true);
}

void
DrishtiPaint::fillVolume(int mind, int maxd,
			 int minw, int maxw,
			 int minh, int maxh,
			 QList<int> dwh,
			 bool slice)
{
  m_volume->fillVolume(mind, maxd,
		       minw, maxw,
		       minh, maxh,
		       dwh, slice);
  getSlice(m_currSlice);
}

void
DrishtiPaint::tagAllVisible(int mind, int maxd,
			    int minw, int maxw,
			    int minh, int maxh)
{
  m_volume->tagAllVisible(mind, maxd,
			  minw, maxw,
			  minh, maxh);
  getSlice(m_currSlice);
}

void
DrishtiPaint::dilate()
{
  m_volume->dilateVolume();
  getSlice(m_currSlice);  
}
void
DrishtiPaint::dilate(int mind, int maxd,
		     int minw, int maxw,
		     int minh, int maxh)
{
  m_volume->dilateVolume(mind, maxd,
			 minw, maxw,
			 minh, maxh);
  getSlice(m_currSlice);  
}

void
DrishtiPaint::erode()
{
  m_volume->erodeVolume();
  getSlice(m_currSlice);  
}
void
DrishtiPaint::erode(int mind, int maxd,
		    int minw, int maxw,
		    int minh, int maxh)
{
  m_volume->erodeVolume(mind, maxd,
			minw, maxw,
			minh, maxh);
  getSlice(m_currSlice);  
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
}

void
DrishtiPaint::on_tag_valueChanged(int t)
{
  Global::setTag(t);
  m_imageWidget->processPrevSliceTags();
}
void DrishtiPaint::on_sliceLod_currentIndexChanged(int l) { m_imageWidget->setSliceLOD(l+1); }
void DrishtiPaint::on_selectprecision_currentIndexChanged(int l) { Global::setSelectionPrecision((l+1)*5); }
void DrishtiPaint::on_boxSize_valueChanged(int d) { Global::setBoxSize(d); }
void DrishtiPaint::on_lambda_valueChanged(int d) { Global::setLambda(d); }
void DrishtiPaint::on_smooth_valueChanged(int d) { Global::setSmooth(d); }
void DrishtiPaint::on_thickness_valueChanged(int d) { Global::setThickness(d); }
void DrishtiPaint::on_radius_valueChanged(int d) { Global::setSpread(d); m_imageWidget->update(); }
void DrishtiPaint::on_pointsize_valueChanged(int d) { m_imageWidget->setPointSize(d); }
void DrishtiPaint::on_closed_clicked(bool c) { Global::setClosed(c); }
void DrishtiPaint::on_lwsmooth_currentIndexChanged(int i){ m_imageWidget->setSmoothType(i); }
void DrishtiPaint::on_lwgrad_currentIndexChanged(int i){ m_imageWidget->setGradType(i); }
void DrishtiPaint::on_newcurve_clicked() { m_imageWidget->newCurve(true); }
void DrishtiPaint::on_morphcurves_clicked() { m_imageWidget->morphCurves(); }
void DrishtiPaint::on_deselect_clicked() { m_imageWidget->deselectAll(); }
void DrishtiPaint::on_deleteallcurves_clicked() { m_imageWidget->deleteAllCurves(); }
void DrishtiPaint::on_zoom0_clicked() { m_imageWidget->zoom0(); }
void DrishtiPaint::on_zoom9_clicked() { m_imageWidget->zoom9(); }
void DrishtiPaint::on_zoomup_clicked() { m_imageWidget->zoomUp(); }
void DrishtiPaint::on_zoomdown_clicked() { m_imageWidget->zoomDown(); }
void DrishtiPaint::on_segmentlength_valueChanged(int d) { m_imageWidget->setSegmentLength(d); }

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
DrishtiPaint::paintedtag_editingFinished()
{
  QString text = viewerUi.paintedtags->text();

  QPair<QString, QList<int> > tags;
  tags = getTags(text);

  viewerUi.paintedtags->setText(tags.first);
  
  m_viewer->setPaintedTags(tags.second);
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
DrishtiPaint::on_livewire_clicked(bool c)
{
  if (!c)
    {
      if (!m_imageWidget->seedMoveMode())
	m_imageWidget->freezeLivewire(false);
      else
	m_imageWidget->freezeModifyUsingLivewire();
      ui.modify->setChecked(false);
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
      ui.livewire->setChecked(true);
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

  m_volume->reset();
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

  m_volume->createBitmask();
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
	      StaticFunctions::checkURLs(urls, ".curves"))
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


  if (m_volume->setFile(flnm) == false)
    {
      QMessageBox::critical(0, "Error", "Cannot load "+flnm);
      return;
    }


  int d, w, h;
  m_volume->gridSize(d, w, h);
  m_imageWidget->setGridSize(d, w, h);
  m_viewer->setGridSize(d, w, h);

  m_viewer->setMultiMapCurves(0, m_imageWidget->multiMapCurvesD());
  m_viewer->setListMapCurves(0, m_imageWidget->listMapCurvesD());

  m_viewer->setMultiMapCurves(1, m_imageWidget->multiMapCurvesW());
  m_viewer->setListMapCurves(1, m_imageWidget->listMapCurvesW());

  m_viewer->setMultiMapCurves(2, m_imageWidget->multiMapCurvesH());
  m_viewer->setListMapCurves(2, m_imageWidget->listMapCurvesH());

  m_viewer->setFibers(m_imageWidget->fibers());
  
  m_viewer->setVolDataPtr(m_volume->memVolDataPtr());
  m_viewer->setMaskDataPtr(m_volume->memMaskDataPtr());

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

  on_actionCurves_triggered();
  ui.lwsettingpanel->setVisible(false);
  ui.closed->setChecked(true);
  ui.livewire->setChecked(true);
  m_imageWidget->setLivewire(true);

  ui.tagcurves->setText("-1");
  viewerUi.curvetags->setText("-1");
  viewerUi.paintedtags->setText("-1");

  QString curvesfile = m_pvlFile;
  curvesfile.replace(".pvl.nc", ".curves");
  m_imageWidget->loadCurves(curvesfile);
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
	      colors[4*i+3] = clr[3].toInt();
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
  if (event->key() == Qt::Key_M)
    {
      m_volume->saveIntermediateResults();
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
  int tdepth = maxDSlice-minDSlice+1;
  int twidth = maxWSlice-minWSlice+1;
  int theight = maxHSlice-minHSlice+1;
  
  //----------------
  QString mesg;
  if (smoothType == 0) mesg = "Smoothing";
  if (smoothType == 1) mesg = "Dilating";
  if (smoothType == 2) mesg = "Eroding";
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

	  if (smoothType == 0)
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
	      
	      if (smoothType == 0)
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
	      
	      if (smoothType == 0)
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
	  
	  if (smoothType == 0)
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
	  else
	    sliceSmooth(tag, spread,
			val[2*spread], raw,
			width, height,
			thresh);
	}		  
      else
	{
	  memcpy(val[2*spread], m_volume->getMaskDepthSliceImage(depth-1), nbytes);
	  
	  if (smoothType == 0)
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
	  else
	    sliceSmooth(tag, spread,
			val[2*spread], raw,
			width, height,
			thresh);
	}		  
      
      if (smoothType == 0)
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
DrishtiPaint::on_actionExtractTag_triggered()
{
  QStringList dtypes;
  QList<int> tag;
  bool saveImageData;

  bool ok;
  //----------------
  QString tagstr = QInputDialog::getText(0, "Save Mesh for Tag",
	    "Tag Numbers (tags should be separated by space.\n-1 for all tags;\nFor e.g. 1 2 5 will mesh tags 1, 2 and 5)",
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

  saveImageData = true;

  //----------------
  int extractType = 1; // extract using tag
  dtypes.clear();
  dtypes << "Tag Only"
	 << "Tag + Transfer Function";
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
  int tdepth = maxDSlice-minDSlice+1;
  int twidth = maxWSlice-minWSlice+1;
  int theight = maxHSlice-minHSlice+1;
  
  QString pvlFilename = m_volume->fileName();
  QString tflnm = QFileDialog::getSaveFileName(0,
					       "Save volume",
					       QFileInfo(pvlFilename).absolutePath(),
					       "Processes Files (*.pvl.nc)",
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
  uchar *curveMask = new uchar[tdepth*twidth*theight];
  memset(curveMask, 0, tdepth*twidth*theight);

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

  //----------------------------------


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
      // use tag mask + transfer function to generate mesh
      if (extractType == 2)
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
	  for(int i=0; i<twidth*theight; i++)
	    {
	      if (raw[i] != 255)
		slice[i] = outsideVal;
	    }
	  tFile.setSlice(slc, slice);
	}
      else
	tFile.setSlice(slc, raw);
    }

  delete [] raw;
  delete [] curveMask;

  progress.setValue(100);  
  QMessageBox::information(0, "Save", "-----Done-----");
}

void
DrishtiPaint::on_actionMeshTag_triggered()
{
  QStringList dtypes;
  QList<int> tag;

  bool ok;
  QString tagstr = QInputDialog::getText(0, "Save Mesh for Tag",
	    "Tag Numbers (tags should be separated by space.\n-1 for all tags;\nFor e.g. 1 2 5 will mesh tags 1, 2 and 5)",
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

  //----------------
  int colorType = 1; // apply tag colors 
  dtypes.clear();
  dtypes << "Tag Color"
	 << "Transfer Function"
	 << "Tag Color + Transfer Function"
	 << "Tag Mask + Transfer Function"
	 << "No Color";
  QString option = QInputDialog::getItem(0,
					 "Mesh Color",
					 "Color Mesh with",
					 dtypes,
					 0,
					 false,
					 &ok);
  if (!ok)
    return;
  
  if (option == "Tag Color") colorType = 1;
  else if (option == "Transfer Function") colorType = 2;
  else if (option == "Tag Color + Transfer Function") colorType = 3;
  else if (option == "Tag Mask + Transfer Function") colorType = 4;
  else colorType = 0;
  //----------------

  int depth, width, height;
  m_volume->gridSize(depth, width, height);
  
  int minDSlice, maxDSlice;
  int minWSlice, maxWSlice;
  int minHSlice, maxHSlice;
  m_imageWidget->getBox(minDSlice, maxDSlice,
			minWSlice, maxWSlice,
			minHSlice, maxHSlice);
  int tdepth = maxDSlice-minDSlice+1;
  int twidth = maxWSlice-minWSlice+1;
  int theight = maxHSlice-minHSlice+1;
  
  QString pvlFilename = m_volume->fileName();
  QString tflnm = QFileDialog::getSaveFileName(0,
					       "Save mesh",
					       QFileInfo(pvlFilename).absolutePath(),
					       "Processes Files (*.ply)",
					       0,
					       QFileDialog::DontUseNativeDialog);
  
  if (tflnm.isEmpty())
    return;

  if (!tflnm.endsWith(".ply"))
    tflnm += ".ply";

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
      if (colorType == 4) // using tag mask + transfer function to extract mesh
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
      //-----------------------------

      int i=0;
      for(int w=minWSlice; w<=maxWSlice; w++)
	for(int h=minHSlice; h<=maxHSlice; h++)
	  {
	    raw[i] = raw[w*height+h];
	    i++;
	  }

      //-----------------------------
      // use tag mask + transfer function to generate mesh
      if (colorType == 4)
	{
	  int sval = 0;
	  if (tag[0] == -1) sval = 255;
	  else if (tag[0] == 0) sval = 0;
	  else sval = 255;

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

      memcpy(meshingData+slc*twidth*theight, raw, twidth*theight);
    }

  delete [] raw;
  delete [] mask;

  progress.setValue(100);  

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
	    if (meshingData[i*twidth*theight+j*theight+k] == 0)
	      meshingData[i*twidth*theight+j*theight+k] = 255;
	}
  //----------------------------------


  MarchingCubes mc;
  mc.set_resolution(theight, twidth, tdepth);
  mc.set_ext_data(meshingData);
  mc.init_all();
  mc.run(64);

  if (colorType > 0)
    saveMesh(colorType,
	     tflnm,
	     &mc,
	     curveMask,
	     minHSlice, minWSlice, minDSlice,
	     theight, twidth, tdepth);
  else
    mc.writePLY(tflnm.toLatin1().data(), true);
  
  mc.clean_all();

  delete [] meshingData;
  delete [] curveMask;

  QMessageBox::information(0, "Save", "-----Done-----");
}

void
DrishtiPaint::saveMesh(int colorType,
		       QString flnm,
		       MarchingCubes *mc,
		       uchar *tagdata,
		       int minHSlice, int minWSlice, int minDSlice,
		       int theight, int twidth, int tdepth)
{
  Vec voxelScaling = StaticFunctions::getVoxelSizeFromHeader(m_pvlFile);

  QProgressDialog progress("Colouring and saving mesh ...",
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

  int ntriangles = mc->ntrigs();
  Triangle *triangles = mc->triangles();

  int nvertices = mc->nverts();
  Vertex *vertices = mc->vertices();


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

  uchar *lut = Global::lut();

  for(int ni=0; ni<nvertices; ni++)
    {
      if (ni%1000 == 0)
	{
	  progress.setValue((int)(100.0*(float)ni/(float)(nvertices)));
	  qApp->processEvents();
	}

      myVertex vertex;
      vertex.x = vertices[ni].x;
      vertex.y = vertices[ni].y;
      vertex.z = vertices[ni].z;
      vertex.nx = vertices[ni].nx;
      vertex.ny = vertices[ni].ny;
      vertex.nz = vertices[ni].nz;

      // now get color
      int h = vertex.x;
      int w = vertex.y;
      int d = vertex.z;
      int tag = 0;
      for(int dd=qMax(d-1, 0); dd<=qMin(tdepth-1,d+1); dd++)
	for(int ww=qMax(w-1, 0); ww<=qMin(twidth-1,w+1); ww++)
	  for(int hh=qMax(h-1, 0); hh<=qMin(theight-1,h+1); hh++)
	    tag = qMax(tag, (int)tagdata[dd*twidth*theight +
				    ww*theight + hh]);
 
      uchar r,g,b;

      if (colorType == 1) // apply tag colors
	{
	  r = Global::tagColors()[4*tag+0];
	  g = Global::tagColors()[4*tag+1];
	  b = Global::tagColors()[4*tag+2];
	}
      else if (colorType == 2 || colorType == 4) // apply transfer function
	{
	  QList<uchar> val = m_volume->rawValue(d+minDSlice,
						w+minWSlice,
						h+minHSlice);     
	  r =  lut[4*val[0]+2];
	  g =  lut[4*val[0]+1];
	  b =  lut[4*val[0]+0];
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

      vertex.r = r;
      vertex.g = g;
      vertex.b = b;

      vertex.x *= voxelScaling.x;
      vertex.y *= voxelScaling.y;
      vertex.z *= voxelScaling.z;
      
      put_element_ply ( ply, ( void * ) &vertex );
    }

  /* set up and write the PlyFace elements */
  put_element_setup_ply ( ply, plyStrings[11] );
  face.nverts = 3 ;
  face.verts  = verts ;
  for(int ni=0; ni<ntriangles; ni++)
    {      
      face.verts[0] = triangles[ni].v1;
      face.verts[1] = triangles[ni].v2;
      face.verts[2] = triangles[ni].v3;
      
      put_element_ply ( ply, ( void * ) &face );
    }

  close_ply ( ply );
  free_ply ( ply );

  for(int i=0; i<plyStrings.count(); i++)
    delete [] plyStrings[i];

  progress.setValue(100);
}
