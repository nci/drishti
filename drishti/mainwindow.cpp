#include "global.h"
#include "mainwindow.h"
#include "mainwindowui.h"
#include "geometryobjects.h"
#include "lighthandler.h"
#include "dialogs.h"
#include "staticfunctions.h"
#include "rawvolume.h"
#include "saveimageseqdialog.h"
#include "savemoviedialog.h"
#include "fileslistdialog.h"
#include "load2volumes.h"
#include "load3volumes.h"
#include "load4volumes.h"
#include "profileviewer.h"
#include "propertyeditor.h"
#include "enums.h"
#include "pluginthread.h"
#include "prunehandler.h"
#include "xmlheaderfunctions.h"
#include "cropshaderfactory.h"

#include <QDockWidget>
#include <QFileDialog>
#include <QInputDialog>

//-------------------------------------------------------------------------------
// -- turn off OpenGL rendering when menus are triggered --
//-------------------------------------------------------------------------------
void MainWindow::on_menuFile_aboutToShow()
{
  Global::disableViewerUpdate();
  MainWindowUI::changeDrishtiIcon(false);
}
void MainWindow::on_menuView_aboutToShow()
{
  Global::disableViewerUpdate();
  MainWindowUI::changeDrishtiIcon(false);
}
void MainWindow::on_menuImage_Size_aboutToShow()
{
  Global::disableViewerUpdate();
  MainWindowUI::changeDrishtiIcon(false);
}
void MainWindow::on_menuToggle_aboutToShow()
{
  Global::disableViewerUpdate();
  MainWindowUI::changeDrishtiIcon(false);
}
void MainWindow::on_menuHelp_aboutToShow()
{
  Global::disableViewerUpdate();
  MainWindowUI::changeDrishtiIcon(false);
}

void MainWindow::on_menuFile_aboutToHide()
{
  Global::enableViewerUpdate();
  MainWindowUI::changeDrishtiIcon(true);
}
void MainWindow::on_menuView_aboutToHide()
{
  Global::enableViewerUpdate();
  MainWindowUI::changeDrishtiIcon(true);
}
void MainWindow::on_menuImage_Size_aboutToHide()
{
  Global::enableViewerUpdate();
  MainWindowUI::changeDrishtiIcon(true);
}
void MainWindow::on_menuToggle_aboutToHide()
{
  Global::enableViewerUpdate();
  MainWindowUI::changeDrishtiIcon(true);
}
void MainWindow::on_menuHelp_aboutToHide()
{
  Global::enableViewerUpdate();
  MainWindowUI::changeDrishtiIcon(true);
}
//-------------------------------------------------------------------------------

void
MainWindow::createHiresLowresWindows()
{
  //---
  if (m_Lowres) delete m_Lowres;
  m_Lowres = new DrawLowresVolume(m_Viewer, m_Volume);
  connect(m_keyFrame, SIGNAL(updateVolumeBounds(Vec, Vec)),
	  m_Lowres, SLOT(setSubvolumeBounds(Vec, Vec)));
  //---

  //---
  if (m_Hires)
    {
      m_Hires->disconnect();
      delete m_Hires;
    }
  m_Hires = new DrawHiresVolume(m_Viewer, m_Volume);
  m_Hires->setBricks(m_bricks);
  #include "connecthires.h"
  //---

  m_Viewer->setHiresVolume(m_Hires);
  m_Viewer->setLowresVolume(m_Lowres);

  m_Lowres->lower();
  m_Hires->lower();

  //reset field of view if it were changed earlier
  m_Viewer->setFieldOfView((float)(M_PI/4.0));

  qApp->processEvents();
}

MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent)
{
  ui.setupUi(this);

  Global::setStatusBar(ui.statusBar, ui.actionStatusBar);

  ui.statusBar->setEnabled(true);
  ui.statusBar->setSizeGripEnabled(true);
  ui.statusBar->addWidget(Global::progressBar());


  MainWindowUI::setMainWindowUI(&ui);

  setWindowIcon(QPixmap(":/images/drishti_32.png"));

  m_saveRotationAnimation = 0;
  m_savePathAnimation = 0;
  m_pathAnimationVd.clear();
  m_pathAnimationUp.clear();
  m_pathAnimationPoints.clear();
  m_pathAnimationSaxis.clear();
  m_pathAnimationTaxis.clear();
  m_pathAnimationTang.clear();


  m_Viewer = new Viewer();
  setCentralWidget(m_Viewer);

  Global::setBatchMode(false);
  Global::setEmptySpaceSkip(true);
  Global::setImageQuality(Global::_NormalQuality);

#ifndef Q_OS_WIN32
  ui.actionSave_Movie->setEnabled(false);
#endif

  ui.actionStatusBar->setChecked(true);
  ui.actionBoundingBox->setChecked(true);
  ui.actionAxes->setChecked(false);
  ui.actionVisibility->setChecked(true);
  ui.actionNormal->setChecked(true);
  ui.actionLow->setChecked(false);
  ui.actionVeryLow->setChecked(false);
  ui.actionBottom_Text->setChecked(Global::bottomText());
  ui.actionDepthcue->setChecked(Global::depthcue());
  ui.actionUse_dragvolume->setChecked(Global::useDragVolume());
  ui.actionUse_dragvolumeforshadows->setChecked(Global::useDragVolumeforShadows());
  ui.actionUse_stillvolume->setChecked(Global::useStillVolume());
  ui.actionEmptySpaceSkip->setChecked(Global::emptySpaceSkip());

  StaticFunctions::initQColorDialog();

  initTagColors();

  setTextureMemory();
  setAcceptDrops(true);

  m_Volume = new Volume();
  m_Viewer->setVolume(m_Volume);
  
  m_bricks = new Bricks();

  m_Hires = 0;
  m_Lowres = 0;

  m_tfContainer = new TransferFunctionContainer(this);
  m_tfManager = new TransferFunctionManager(this);
  m_tfManager->registerContainer(m_tfContainer);
  m_tfEditor = new TransferFunctionEditorWidget(this);
  m_tfEditor->setTransferFunction(NULL);    

  m_tfManager->setDisabled(true);

  m_keyFrame = new KeyFrame();
  m_Viewer->setKeyFrame(m_keyFrame);

  //----------------------------------------------------------
  m_dockTF = new QDockWidget("Transfer Function Editor", this);
  m_dockTF->setAllowedAreas(Qt::LeftDockWidgetArea | 
			    Qt::RightDockWidgetArea);
  QSplitter *splitter = new QSplitter(Qt::Vertical, m_dockTF);
  splitter->addWidget(m_tfManager);
  splitter->addWidget(m_tfEditor);
  m_dockTF->setWidget(splitter);
  //----------------------------------------------------------

  //----------------------------------------------------------
  m_lightingWidget = new LightingWidget();
  QDockWidget *dock2 = new QDockWidget("Shader Widget", this);
  dock2->setAllowedAreas(Qt::LeftDockWidgetArea | 
			 Qt::RightDockWidgetArea);
  dock2->setWidget(m_lightingWidget);
  dock2->hide();
  //----------------------------------------------------------

  //----------------------------------------------------------
  m_bricksWidget = new BricksWidget(NULL, m_bricks);
  QDockWidget *dock3 = new QDockWidget("Bricks Editor", this);
  dock3->setAllowedAreas(Qt::LeftDockWidgetArea | 
			 Qt::RightDockWidgetArea);
  dock3->setWidget(m_bricksWidget);
  dock3->hide();
  //----------------------------------------------------------

  //----------------------------------------------------------
  m_volInfoWidget = new VolumeInformationWidget();
  QDockWidget *dock4 = new QDockWidget("Volume Information", this);
  dock4->setAllowedAreas(Qt::LeftDockWidgetArea | 
			 Qt::RightDockWidgetArea);
  dock4->setWidget(m_volInfoWidget);
  dock4->hide();
  //----------------------------------------------------------

  //----------------------------------------------------------
  m_preferencesWidget = new PreferencesWidget();
  m_preferencesWidget->updateStereoSettings(
                 m_Viewer->camera()->focusDistance(),
                 m_Viewer->camera()->IODistance(),
                 m_Viewer->camera()->physicalScreenWidth());
  QDockWidget *dock5 = new QDockWidget("Preferences", this);
  dock5->setAllowedAreas(Qt::LeftDockWidgetArea | 
			 Qt::RightDockWidgetArea);
  dock5->setWidget(m_preferencesWidget);
  dock5->hide();
  //----------------------------------------------------------

  //----------------------------------------------------------
  m_dockKeyframe = new QDockWidget("KeyFrame Editor", this);
  m_dockKeyframe->setAllowedAreas(Qt::BottomDockWidgetArea | 
        			  Qt::TopDockWidgetArea);
  m_keyFrameEditor = new KeyFrameEditor();
  m_dockKeyframe->setWidget(m_keyFrameEditor);
  m_dockKeyframe->hide();
  //----------------------------------------------------------

//  //----------------------------------------------------------
//  m_dockGallery = new QDockWidget("Gallery", this);
//  m_dockGallery->setAllowedAreas(Qt::AllDockWidgetAreas);
  m_gallery = new ViewsEditor();
//  m_dockGallery->setWidget(m_gallery);
//  m_dockGallery->hide();
//  //----------------------------------------------------------


  addDockWidget(Qt::RightDockWidgetArea, m_dockTF);
  addDockWidget(Qt::RightDockWidgetArea, dock2);
  addDockWidget(Qt::RightDockWidgetArea, dock3);
  addDockWidget(Qt::LeftDockWidgetArea, dock4);
  addDockWidget(Qt::LeftDockWidgetArea, dock5);
  addDockWidget(Qt::BottomDockWidgetArea,m_dockKeyframe);
//  addDockWidget(Qt::BottomDockWidgetArea,m_dockGallery);

  QString tstr = QString("Drishti v") +
                 Global::DrishtiVersion() +
                 " - Volume Exploration and Presentation Tool";
  if (QGLFormat::defaultFormat().stereo())
    tstr = "(Stereo)"+tstr;

  setWindowTitle(tstr);


  ui.menuView->addAction(m_dockTF->toggleViewAction());
  ui.menuView->addAction(dock2->toggleViewAction());
  ui.menuView->addAction(dock3->toggleViewAction());
  ui.menuView->addAction(dock4->toggleViewAction());
  ui.menuView->addAction(m_dockKeyframe->toggleViewAction());
//  ui.menuView->addAction(m_dockGallery->toggleViewAction());
  ui.menuView->addSeparator();
  ui.menuView->addAction(dock5->toggleViewAction());

  createHiresLowresWindows();

  QTimer::singleShot(2000, this, SLOT(GlewInit()));


  #include "connectbricks.h"
  #include "connectbrickswidget.h"
  #include "connectclipplanes.h"
  #include "connectkeyframe.h"
  #include "connectkeyframeeditor.h"
  #include "connectlightingwidget.h"
  #include "connectpreferences.h"
  #include "connectshowmessage.h"
  #include "connecttfeditor.h"
  #include "connecttfmanager.h"
  #include "connectviewer.h"
  //#include "connectviewseditor.h"
  #include "connectvolinfowidget.h"
  #include "connectgeometryobjects.h"


  initializeRecentFiles();

  loadSettings();
  
}

void
MainWindow::registerPlugins()
{
  m_pluginList.clear();
  m_pluginDll.clear();

  // look in @executable_path/renderplugins
  QString plugindir = qApp->applicationDirPath() + QDir::separator() + "renderplugins";

  QPair<QMenu*, QString> pair;
  QStack< QPair<QMenu*, QString> > stack;

  QStringList filters;
#if defined(Q_OS_WIN32)
  filters << "*.dll";
#endif
#ifdef Q_OS_MACX
  // look in drishti.app/renderplugins
  QString sep = QDir::separator();
  plugindir = qApp->applicationDirPath()+sep+".."+sep+".."+sep+"renderplugins";
  filters << "*.dylib";
#endif
#if defined(Q_OS_LINUX)
  filters << "*.so";
#endif

  QString cdir = plugindir;
  QMenu *menu = ui.menuPlugins;
  pair.first = menu;
  pair.second = cdir;
  stack.push(pair);

  bool ignore = true; // ignore very first entry
  QMenu *cmenu=0;

  while(!stack.isEmpty())
    {
      pair = stack.pop();
      
      menu = pair.first;
      cdir = pair.second; 

      QDir dir(cdir);
      dir.setNameFilters(filters);
      dir.setFilter(QDir::Files |
		    QDir::AllDirs |
		    QDir::NoSymLinks |
		    QDir::NoDotAndDotDot);

      if (!ignore)
	{
	  cmenu = new QMenu(QFileInfo(cdir).fileName());
	  menu->addMenu(cmenu);
	  menu = cmenu;
	}
      ignore = false;

      QFileInfoList list = dir.entryInfoList();
      if (list.size() > 0)
	{
	  for (int i=0; i<list.size(); i++)
	    {
	      QString flnm = list.at(i).absoluteFilePath();
	      if (list.at(i).isDir())
		{
		  pair.first = menu;
		  pair.second = flnm;
		  stack.push(pair);
		}
	      else
		{
		  QString pluginflnm = list.at(i).absoluteFilePath();
		  QPluginLoader pluginLoader(pluginflnm);
		  QObject *plugin = pluginLoader.instance();
		  if (plugin)
		    {
		      RenderPluginInterface *rpi = qobject_cast<RenderPluginInterface *>(plugin);
		      if (rpi)
			{
			  QStringList rs = rpi->registerPlugin();
			  
			  m_pluginList << rs;
			  m_pluginDll << pluginflnm;			    
			  
			  QAction *action = new QAction(this);
			  action->setText(m_pluginList[m_pluginList.count()-1][0]);
			  action->setData(m_pluginList.count()-1);
			  action->setVisible(true);      
			  connect(action, SIGNAL(triggered()),
				  this, SLOT(loadPlugin()));
			  
			  if (cmenu)
			    cmenu->addAction(action);
			  else
			    ui.menuPlugins->addAction(action);
			}
		    }
		  else
		    {
		      QMessageBox::information(0, "Error", QString("Cannot load %1").arg(pluginflnm));
		    }
		}
	      
	    }
	}
    }
}

void
MainWindow::loadPlugin()
{
  if (!m_Volume->valid() ||
      Global::volumeType() == Global::DummyVolume)
    {
      QMessageBox::information(0, "Error", "No volume to work on !");
      return;
    }

  QAction *action = qobject_cast<QAction *>(sender());
  int idx = action->data().toInt();

  QPluginLoader pluginLoader(m_pluginDll[idx]);
  QObject *plugin = pluginLoader.instance();

  if (!plugin)
    {
      QMessageBox::information(0, "Error", "Cannot load plugin");
      return;
    }

//  QMessageBox::information(0, "",
//			   QString("Load : %1\n %2\n %3").	\
//			   arg(idx).arg(m_pluginList[idx][0]).arg(m_pluginDll[idx]));
  

  RenderPluginInterface *pluginInterface = qobject_cast<RenderPluginInterface *>(plugin);
  if (!pluginInterface)
    {
      QMessageBox::information(0, "Error", "Cannot load plugin interface");
      return;
    }

  QList<Vec> clipPos;
  QList<Vec> clipNormal;
  m_Hires->getClipForMask(clipPos, clipNormal);

  Vec dataMin, dataMax;
  Vec vscale;
  if (m_Hires->raised())
    Global::bounds(dataMin, dataMax);
  else
    m_Lowres->subvolumeBounds(dataMin, dataMax);
  vscale = VolumeInformation::volumeInformation().voxelSize;

  QImage lutImage = QImage(Global::lutSize()*256, 256, QImage::Format_ARGB32);
  uchar *bits = lutImage.bits();
  memcpy(bits, m_Viewer->lookupTable(), Global::lutSize()*256*256*4);

  Vec subvolumeSize, dragTextureInfo;
  int subsamplinglevel = 1;
  int px,py,pz,plod;
  uchar *prune = 0;
  if (m_Hires->raised())
    {
      subsamplinglevel = m_Volume->getSubvolumeSubsamplingLevel();
      subvolumeSize = m_Volume->getSubvolumeSize();
      dragTextureInfo = m_Volume->getDragTextureInfo();
    }
  else
    {
      int nrows, ncols;
      int bpv = 1;
      if (m_Volume->pvlVoxelType(0) > 0) bpv = 2;
      int tms = Global::textureMemorySize(); // in Mb
      subsamplinglevel = StaticFunctions::getSubsamplingLevel(tms, bpv,
							      dataMin, dataMax);
      QList<Vec> slabinfo = Global::getSlabs(subsamplinglevel,
					     dataMin, dataMax,
					     nrows, ncols);
      
      if (slabinfo.count() > 1)
	dragTextureInfo = slabinfo[0];
      else
	dragTextureInfo = Vec(ncols, nrows, subsamplinglevel);
      
      subvolumeSize = dataMax-dataMin+Vec(1,1,1);
    }

  plod = dragTextureInfo.z;
  px = subvolumeSize.x/plod;
  py = subvolumeSize.y/plod;
  pz = subvolumeSize.z/plod;
  prune = new uchar[3*px*py*pz];

  if (m_Hires->raised())
    {
      PruneHandler::getRaw(prune,
			   -1, // get channels 0,1,2
			   dragTextureInfo,
			   subvolumeSize);
    }
  else
    memset(prune, 255, 3*px*py*pz);

  QVector<uchar> pruneData(3*px*py*pz);
  memcpy(pruneData.data(), prune, 3*px*py*pz);
  delete [] prune;

  QVector<uchar> tagData(1024);
  memcpy(tagData.data(), Global::tagColors(), 1024);


  pluginInterface->init();

  pluginInterface->setPvlFileManager(m_Volume->pvlFileManager(0));
  pluginInterface->setLodFileManager(m_Volume->lodFileManager(0));
  pluginInterface->setClipInfo(clipPos, clipNormal);
  pluginInterface->setCropInfo(GeometryObjects::crops()->crops());
  pluginInterface->setPathInfo(GeometryObjects::paths()->paths());
  pluginInterface->setLookupTable(Global::lutSize(), lutImage);  
  pluginInterface->setSamplingLevel(subsamplinglevel);  
  pluginInterface->setDataLimits(dataMin, dataMax);
  pluginInterface->setVoxelScaling(vscale);
  pluginInterface->setPreviousDirectory(Global::previousDirectory());
  pluginInterface->setPruneData(plod, px, py, pz, pruneData);
  pluginInterface->setTagColors(tagData);

  pluginInterface->start();
}

void
MainWindow::initTagColors()
{
  uchar *colors;
  colors = new uchar[1024];

  qsrand(1);

  for(int i=0; i<256; i++)
    {
      float r,g,b,a;
      if (i > 0 && i < 254)
	{
	  r = (float)qrand()/(float)RAND_MAX;
	  g = (float)qrand()/(float)RAND_MAX;
	  b = (float)qrand()/(float)RAND_MAX;
	  a = 0.5f;
	}
      else
	{
	  r = g = b = a = 0;
	  if (i == 254)
	    {
	      r = 0; g = 0.1f; b = 0; a = 0.1f;
	    }
	}
      colors[4*i+0] = 255*r;
      colors[4*i+1] = 255*g;
      colors[4*i+2] = 255*b;
      colors[4*i+3] = 255*a;
    }
  
  Global::setTagColors(colors);
  delete [] colors;
}

void
MainWindow::setTextureMemory()
{
  QString homePath = QDir::homePath();
  QFileInfo settingsFile(homePath, ".drishti.xml");

  if (settingsFile.exists())
    return;

  bool ok;
  QStringList texlist;
  texlist << "128 Mb";
  texlist << "256 Mb";
  texlist << "512 Mb";
  texlist << "768 Mb";
  texlist << "1 Gb";
  texlist << "1.5 Gb";
  texlist << "2.0 Gb";
  texlist << "4.0 Gb";
  texlist << "6.0 Gb";
  texlist << "8.0 Gb";
  texlist << "10.0 Gb";
  texlist << "12.0 Gb";
  texlist << "16.0 Gb";
  QString texstr = QInputDialog::getItem(0,
					 "Texture Memory",
					 "Texture Memory Size",
					 texlist, 0, false,
					 &ok);
  int texmem = 128;
  if (ok && !texstr.isEmpty())
    {
      QStringList lst = texstr.split(" ");
      if (lst[0] == "128") texmem = 128;
      if (lst[0] == "256") texmem = 256;
      if (lst[0] == "512") texmem = 512;
      if (lst[0] == "768") texmem = 768;
      if (lst[0] == "1") texmem = 1024;
      if (lst[0] == "1.5") texmem = 1536;
      if (lst[0] == "2.0") texmem = 2*1024;
      if (lst[0] == "4.0") texmem = 4*1024;
      if (lst[0] == "6.0") texmem = 6*1024;
      if (lst[0] == "8.0") texmem = 8*1024;
      if (lst[0] == "10.0") texmem = 10*1024;
      if (lst[0] == "12.0") texmem = 12*1024;
      if (lst[0] == "16.0") texmem = 16*1024;
    }
  Global::setTextureMemorySize(texmem);
  Global::calculate3dTextureSize();
}


void
MainWindow::GlewInit()
{
  m_Viewer->GlewInit();

  registerPlugins();

  // query 3d texture size limits
  GLint textureSize;
  glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &textureSize);
  Global::setTextureSizeLimit(textureSize);
  m_preferencesWidget->updateTextureMemory();

  loadProjectRunKeyframesAndExit();

  // load program 
  QStringList arguments = qApp->arguments();
  if (arguments.count() >= 2)
    {
      int i = 1;
      if (arguments[i] == "-stereo") i++;

      if (StaticFunctions::checkExtension(arguments[i], ".pvl.nc"))
	{
	  QStringList flnms;
	  for(int a=i; a<arguments.count(); a++)
	    flnms << arguments[a];
	  loadSingleVolume(flnms);
	}
      else if (StaticFunctions::checkExtension(arguments[i], ".xml"))
	{
	  Global::addRecentFile(arguments[i]);
	  updateRecentFileAction();
	  createHiresLowresWindows();
	  loadProject(arguments[i].toLatin1().data());
	}
    }
}

bool
MainWindow::fromFile(QString flnm, BatchJob &bj)
{
  QFileInfo fi(flnm);
  if (!fi.exists())
    {
      QMessageBox::information(0, "",
			       QString("Cannot find %1").arg(flnm));
      return false;
    }
  
  QFile infile(flnm);
  if (!infile.open(QFile::ReadOnly))
    {
      QMessageBox::information(0, "",
			       QString("Cannot open for reading %1").arg(flnm));
      return false;
    }

  QStringList arguments;
  QTextStream stream(&infile);      
  QString line;
  do {
    line = stream.readLine().trimmed();
    QStringList tokens = line.split("=");    
    if (tokens.count() == 2)
      {
	QString t0 = tokens[0].trimmed();
	QString t1 = tokens[1].trimmed();
	if (t0[0] != '#')
	  arguments.append(t0+"="+t1);
      }
    else if (tokens.count() == 1)
      {
	QString t0 = tokens[0].trimmed();
	if (t0[0] != '#')
	  arguments.append(t0);
      }
  } while(!line.isNull());

  if (arguments.count() > 0)
    {
      QDir::setCurrent(fi.absolutePath());
      return(fromStringList(arguments, bj));
    }

  return false;
};

bool
MainWindow::fromStringList(QStringList arguments,
			   BatchJob &bj)
{
  for(int i=0; i<arguments.count(); i++)
    {
      QString arg = arguments[i];
      if (arg.contains("project="))
	{
	  QStringList tokens = arg.split("=");
	  QString projfile = tokens[1];
	  QFileInfo fi(projfile);
	  if (fi.exists())
	    {
	      bj.startProject = true;
	      bj.projectFilename = projfile;
	    }
	  else
	    {
	      QMessageBox::information(0, "", QString("Cannot find %1").arg(projfile));
	      return false;
	    }
	}
      else if (arg.contains("renderframes="))
	{
	  bj.renderFrames = true;
	  QStringList tokens = arg.split("=");
	  QString frames = tokens[1];
	  tokens = frames.split(",");
	  if (tokens.count() > 0) bj.startFrame = tokens[0].toInt();
	  if (tokens.count() > 1) bj.endFrame = tokens[1].toInt();
	  if (tokens.count() > 2) bj.stepFrame = tokens[2].toInt();
	}
      else if (arg.contains("image="))
	{
	  bj.image = true;
	  QStringList tokens = arg.split("=");
	  bj.imageFilename = tokens[1];
	}
      else if (arg.contains("movie="))
	{
	  bj.movie = true;
	  QStringList tokens = arg.split("=");
	  bj.movieFilename = tokens[1];
	}
      else if (arg.contains("framerate="))
	{
	  QStringList tokens = arg.split("=");
	  bj.frameRate = tokens[1].toInt();
	}
      else if (arg.contains("imagemode="))
	{
	  QStringList tokens = arg.split("=");
	  if (tokens[1]=="stereo")
	    bj.imageMode = Enums::StereoImageMode;
	  else if (tokens[1]=="cubic")
	    bj.imageMode = Enums::CubicImageMode;
	  else if (tokens[1]=="redcyan")
	    bj.imageMode = Enums::RedCyanImageMode;
	  else if (tokens[1]=="redblue")
	    bj.imageMode = Enums::RedBlueImageMode;	  
	  else if (tokens[1]=="crosseye")
	    bj.imageMode = Enums::CrosseyeImageMode;	  
	  else if (tokens[1]=="3dtv")
	    bj.imageMode = Enums::ImageMode3DTV;	  
	}
      else if (arg.contains("nobackgroundrender"))
	{
	  bj.backgroundrender = false;
	}
      else if (arg.contains("shading"))
	{
	  bj.shading = true;
	}
      else if (arg.contains("depthcue"))
	{
	  bj.depthcue = true;
	}
      else if (arg.contains("skipemptyspace"))
	{
	  bj.skipEmptySpace = true;
	}
      else if (arg.contains("dragonlyforshadows"))
	{
	  bj.dragonlyforshadows = true;
	}
      else if (arg.contains("dragonly"))
	{
	  bj.dragonly = true;
	}
      else if (arg.contains("imagesize="))
	{
	  bj.imagesize = true;
	  QStringList tokens = arg.split("=");
	  QString imgsz = tokens[1];
	  tokens = imgsz.split(",");
	  if (tokens.count() > 0) bj.imgWidth = tokens[0].toInt();
	  if (tokens.count() > 1) bj.imgHeight = tokens[1].toInt();
	}
      else if (arg.contains("stepsize="))
	{
	  QStringList tokens = arg.split("=");
	  if (tokens.count() > 0) bj.stepSize = tokens[1].toFloat();
	}      
    }
  return true;
}

void
MainWindow::loadProjectRunKeyframesAndExit()
{
  BatchJob bj;

  QStringList arguments = qApp->arguments();
  bool ok = false;
  QStringList isFile = arguments.filter("file=");
  if (isFile.count() > 0)
    {
      QString arg = isFile[0];
      QStringList tokens = arg.split("=");
      ok = fromFile(tokens[1], bj);
    }
  else
    {
      ok = fromStringList(arguments, bj);
    }

  if (!ok)
    qApp->quit();


  if (bj.startProject)
    {
      m_dockTF->hide();
      qApp->processEvents();
      ui.statusBar->hide();
      qApp->processEvents();

      Global::setBatchMode(true);
      Global::setEmptySpaceSkip(bj.skipEmptySpace);
      Global::setLoadDragOnly(bj.dragonly);
      Global::setUseDragVolume(bj.dragonly);
      Global::setUseDragVolumeforShadows(bj.dragonlyforshadows);
      Global::setStepsizeStill(bj.stepSize);
      Global::setDepthcue(bj.depthcue);

      loadProject(bj.projectFilename.toLatin1().data());

      if (!bj.backgroundrender)
	m_Viewer->setUseFBO(false);

      if (bj.shading)
	m_Hires->setRenderQuality(Enums::RenderHighQuality);

      if (bj.imageMode == Enums::CubicImageMode)
	{
	  bj.imgWidth = qMax(bj.imgWidth, bj.imgHeight);
	  bj.imgHeight = bj.imgWidth;
	}

      if (bj.renderFrames &&
	  bj.startFrame > 0 &&
	  bj.endFrame > 0 &&
	  bj.stepFrame > 0)
	{
	  bj.startFrame = qMax(1, bj.startFrame);
	  bj.endFrame = qMin(bj.endFrame, m_keyFrameEditor->endFrame());
	  bj.stepFrame = qMax(1, bj.stepFrame);
	}
      else
	{
	  bj.startFrame = m_keyFrameEditor->startFrame();
	  bj.endFrame = m_keyFrameEditor->endFrame();
	  bj.stepFrame = 1;
	}

      m_Viewer->setImageSize(bj.imgWidth, bj.imgHeight);

      if (bj.imageMode == Enums::RedCyanImageMode)
	{
	  MainWindowUI::mainWindowUI()->actionRedCyan->setChecked(true);
	  m_Viewer->setImageMode(Enums::MonoImageMode);
	}
      else if (bj.imageMode == Enums::RedBlueImageMode)
	{
	  MainWindowUI::mainWindowUI()->actionRedBlue->setChecked(true);
	  m_Viewer->setImageMode(Enums::MonoImageMode);
	}
      else if (bj.imageMode == Enums::CrosseyeImageMode)
	{
	  MainWindowUI::mainWindowUI()->actionCrosseye->setChecked(true);
	  m_Viewer->setImageMode(Enums::MonoImageMode);
	}
      else if (bj.imageMode == Enums::ImageMode3DTV)
	{
	  MainWindowUI::mainWindowUI()->actionFor3DTV->setChecked(true);
	  m_Viewer->setImageMode(Enums::MonoImageMode);
	}
      else
	m_Viewer->setImageMode(bj.imageMode);


      if (!m_Viewer->drawToFBO())
	{
	  if (statusBar()->isVisible())
	    resize(bj.imgWidth,
		   bj.imgHeight + (menuBar()->size().height() +
				   statusBar()->size().height()));
	  else
	    resize(bj.imgWidth,
		   bj.imgHeight  + menuBar()->size().height());
	}


      Global::setBottomText(false);
      qApp->processEvents();

      if (bj.renderFrames)
	{
	  m_keyFrame->playFrameNumber(bj.startFrame);

	  if (bj.movie)
	    {
	      if (m_Viewer->startMovie(bj.movieFilename,
				       bj.frameRate, 100,
				       false) == false)
		return;
	      m_Viewer->setSaveSnapshots(false);
	      m_Viewer->setSaveMovie(true);
	    }
	  else if (bj.image)
	    {
	      m_Viewer->setImageFileName(bj.imageFilename);
	      m_Viewer->setSaveSnapshots(true);
	    }
	  
	  m_Viewer->dummydraw();
	  
	  connect(this, SIGNAL(playKeyFrames(int,int,int)),
		  m_keyFrameEditor, SLOT(playKeyFrames(int,int,int)));
	  emit playKeyFrames(bj.startFrame,
			     bj.endFrame,
			     bj.stepFrame);
	  qApp->processEvents();
	  disconnect(this, SIGNAL(playKeyFrames(int,int,int)),
		     m_keyFrameEditor, SLOT(playKeyFrames(int,int,int)));
	}
      else
	{
	  m_Viewer->setCurrentFrame(-1);
	  m_Viewer->setImageFileName(bj.imageFilename);
	  m_Viewer->setSaveSnapshots(true);
	  m_Viewer->dummydraw();
	  m_Viewer->updateGL();
	  m_Viewer->endPlay();
	  qApp->quit();
	}
    }
}

void
MainWindow::closeEvent(QCloseEvent *event)
{
  if (Global::batchMode()) // don't ask this extra stuff
    return;

  Global::removeBackgroundTexture();
  Global::removeSpriteTexture();
  Global::removeSphereTexture();
  Global::removeCylinderTexture();

  saveSettings();

  if (m_Volume->valid() &&
      Global::volumeType() != Global::DummyVolume)
    {
      int ok = QMessageBox::question(0, "Exit Drishti",
				     QString("Would you like to save project before quitting ?"),
				     QMessageBox::Yes | QMessageBox::No);
      if (ok == QMessageBox::Yes)
	on_actionSave_Project_triggered();
    }

  m_Volume->clearVolumes();
  
  LightHandler::reset();

  GeometryObjects::imageCaptions()->clear();
  GeometryObjects::captions()->clear();
  GeometryObjects::hitpoints()->clear();
  GeometryObjects::landmarks()->clear();
  GeometryObjects::paths()->clear();
  GeometryObjects::grids()->clear();
  GeometryObjects::crops()->clear();
  GeometryObjects::pathgroups()->clear();
  GeometryObjects::trisets()->clear();
  GeometryObjects::networks()->clear();
}

void
MainWindow::on_actionHiresMode_triggered()
{
  if (m_Volume->valid())
    m_Viewer->switchDrawVolume();
}

void
MainWindow::on_actionNormal_triggered()
{
  ui.actionNormal->setChecked(true);
  ui.actionLow->setChecked(false);
  ui.actionVeryLow->setChecked(false);
  Global::setImageQuality(Global::_NormalQuality);
  m_Viewer->createImageBuffers();
  m_Viewer->update();
}

void
MainWindow::on_actionLow_triggered()
{
  ui.actionNormal->setChecked(false);
  ui.actionLow->setChecked(true);
  ui.actionVeryLow->setChecked(false);
  Global::setImageQuality(Global::_LowQuality);
  m_Viewer->createImageBuffers();
  m_Viewer->update();
}

void
MainWindow::on_actionVeryLow_triggered()
{
  ui.actionNormal->setChecked(false);
  ui.actionLow->setChecked(false);
  ui.actionVeryLow->setChecked(true);
  Global::setImageQuality(Global::_VeryLowQuality);
  m_Viewer->createImageBuffers();
  m_Viewer->update();
}

void
MainWindow::on_actionBottom_Text_triggered()
{
  Global::setBottomText(!Global::bottomText());
  m_Viewer->update();
}

void
MainWindow::on_actionDepthcue_triggered()
{
  Global::setDepthcue(!Global::depthcue());
  m_Viewer->update();
}

void
MainWindow::on_actionEmptySpaceSkip_triggered()
{
  Global::setEmptySpaceSkip(!Global::emptySpaceSkip());
  m_Hires->createDefaultShader();
  m_Viewer->update();
}

void
MainWindow::on_actionUse_stillvolume_triggered()
{
  Global::setUseStillVolume(!Global::useStillVolume());
  m_Viewer->update();
}

void
MainWindow::on_actionUse_dragvolume_triggered()
{
  Global::setUseDragVolume(!Global::useDragVolume());
  m_Viewer->update();
}

void
MainWindow::on_actionUse_dragvolumeforshadows_triggered()
{
  Global::setUseDragVolumeforShadows(!Global::useDragVolumeforShadows());
  m_Viewer->update();
}

void
MainWindow::on_actionAbout_triggered()
{
  QString mesg;
  mesg = QString("Drishti v")+Global::DrishtiVersion()+"\n\n";
  mesg += "Drishti is developed by\n";
  mesg += "Ajay Limaye\n";
  mesg += "Australian National University,\n";
  mesg += "Canberra,\n";
  mesg += "Australia\n\n";
  mesg += "Contact :\nAjay.Limaye@anu.edu.au\n\n";
  mesg += "Website :\n https://github.com/AjayLimaye/drishti\n\n";
  mesg += "Drishti User Group :\nhttps://groups.google.com/group/drishti-user-group\n\n";
  mesg += "YouTube :\nhttps://www.youtube.com/user/900acl/videos?sort=dd&flow=list&page=1&view=1\n";
  
  QMessageBox::information(0, "Drishti", mesg);
}

void
MainWindow::initializeRecentFiles()
{
  for(int i=0; i<m_recentFileActions.count(); i++)
    delete m_recentFileActions[i];
  m_recentFileActions.clear();

  for (int i=0; i<Global::maxRecentFiles(); i++)
    {
      m_recentFileActions.append(new QAction(this));
      m_recentFileActions[i]->setVisible(false);
      connect(m_recentFileActions[i], SIGNAL(triggered()),
	      this, SLOT(openRecentFile()));
      ui.menuFile->insertAction(ui.actionExit,
				m_recentFileActions[i]);
    }

  ui.menuFile->insertSeparator(ui.actionExit);
}

void
MainWindow::updateRecentFileAction()
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
MainWindow::on_actionSave_Image_triggered()
{
  if (m_Lowres->raised())
    {
      QMessageBox::critical(0, "Error", "Cannot save image in Lowres mode. Press F2 to switch to Hires mode");
      return;
    }
  
  m_Viewer->grabScreenShot();
}

void
MainWindow::on_actionSave_ImageSequence_triggered()
{
  if (m_Lowres->raised())
    {
      QMessageBox::critical(0, "Error", "Cannot save image sequence in Lowres mode. Press F2 to switch to Hires mode");
      return;
    }

  QSize imgSize = StaticFunctions::getImageSize(m_Viewer->size().width(),
						m_Viewer->size().height());

  m_Viewer->setImageSize(imgSize.width(), imgSize.height());

  SaveImageSeqDialog saveImg(0,
			     Global::previousDirectory(),
			     m_keyFrameEditor->startFrame(),
			     m_keyFrameEditor->endFrame(),
			     1);

  saveImg.move(QCursor::pos());

  if (saveImg.exec() == QDialog::Accepted)
    {
      QString flnm = saveImg.fileName();
      int imgMode = saveImg.imageMode();

      if (imgMode == Enums::CubicImageMode)
	{
	  if (imgSize.width() != imgSize.height())
	    {
	      QMessageBox::critical(0, "Error",
				    QString("Current image size is %1x%2.\nSquare image size required for cubic images.").\
				    arg(imgSize.width()).\
				    arg(imgSize.height()));
	      return;
	    }
	}

      QFileInfo f(flnm);
      Global::setPreviousDirectory(f.absolutePath());

      m_Viewer->setImageMode(imgMode);
      m_Viewer->setImageFileName(flnm);
      m_Viewer->setSaveSnapshots(true);
      
      m_Viewer->dummydraw();

      connect(this, SIGNAL(playKeyFrames(int,int,int)),
	      m_keyFrameEditor, SLOT(playKeyFrames(int,int,int)));
      emit playKeyFrames(saveImg.startFrame(),
			 saveImg.endFrame(),
			 saveImg.stepFrame());
      qApp->processEvents();
      disconnect(this, SIGNAL(playKeyFrames(int,int,int)),
		 m_keyFrameEditor, SLOT(playKeyFrames(int,int,int)));
    }
}

void
MainWindow::on_actionExit_triggered()
{
  close();
}


void
MainWindow::on_actionSave_Movie_triggered()
{
  if (m_Lowres->raised())
    {
      emit showMessage("Cannot save movie in Lowres mode. Press F2 to switch to Hires mode", true);
      return;
    }

  QSize imgSize = StaticFunctions::getImageSize(m_Viewer->size().width(),
						m_Viewer->size().height());

  m_Viewer->setImageSize(imgSize.width(), imgSize.height());

#if defined(Q_OS_WIN32)
  if (imgSize.width()%16 > 0 ||
      imgSize.height()%16 > 0)
    {
      emit showMessage(QString("For wmv movies, the image size must be multiple of 16. Current size is %1 x %2"). \
		       arg(imgSize.width()).
		       arg(imgSize.height()), true);
      return;
    }
#endif

  SaveMovieDialog saveImg(0,
			  Global::previousDirectory(),
			  m_keyFrameEditor->startFrame(),
			  m_keyFrameEditor->endFrame(),
			  1);

  saveImg.move(QCursor::pos());

  if (saveImg.exec() == QDialog::Accepted)
    {
      QString flnm = saveImg.fileName();
      bool movieMode = saveImg.movieMode();
      QFileInfo f(flnm);
      Global::setPreviousDirectory(f.absolutePath());

      if (movieMode)
	m_Viewer->setImageMode(Enums::MonoImageMode);
      else
	m_Viewer->setImageMode(Enums::StereoImageMode);

      if (m_Viewer->startMovie(flnm, 25, 100, true) == false)
	return;

      m_Viewer->setSaveSnapshots(false);
      m_Viewer->setSaveMovie(true);

      m_Viewer->dummydraw();

      connect(this, SIGNAL(playKeyFrames(int,int,int)),
	      m_keyFrameEditor, SLOT(playKeyFrames(int,int,int)));
      emit playKeyFrames(saveImg.startFrame(),
			 saveImg.endFrame(),
			 saveImg.stepFrame());
      qApp->processEvents();
      disconnect(this, SIGNAL(playKeyFrames(int,int,int)),
		 m_keyFrameEditor, SLOT(playKeyFrames(int,int,int)));
    }
}

void
MainWindow::on_actionImage_Size_triggered()
{
  QSize imgSize = StaticFunctions::getImageSize(m_Viewer->size().width(),
						m_Viewer->size().height());
  int x = imgSize.width();
  int y = imgSize.height();
  if (statusBar()->isVisible())
    resize(x,
	   y + (menuBar()->size().height() +
		statusBar()->size().height()));
  else
    resize(x,
	   y + menuBar()->size().height());
}

void
MainWindow::checkStateChanged(int i, int j, bool flag)
{
  if (Global::volumeType() == Global::DoubleVolume)
    {
      if (j == 1 && flag)
	m_tfEditor->changeVol(0);
      else if (j == 2 && flag)
	m_tfEditor->changeVol(1);
    }
  else if (Global::volumeType() == Global::TripleVolume)
    {
      if (j == 1 && flag)
	m_tfEditor->changeVol(0);
      else if (j == 2 && flag)
	m_tfEditor->changeVol(1);
      else if (j == 3 && flag)
	m_tfEditor->changeVol(2);
    }
  else if (Global::volumeType() == Global::QuadVolume)
    {
      if (j == 1 && flag)
	m_tfEditor->changeVol(0);
      else if (j == 2 && flag)
	m_tfEditor->changeVol(1);
      else if (j == 3 && flag)
	m_tfEditor->changeVol(2);
      else if (j == 4 && flag)
	m_tfEditor->changeVol(3);
    }
  else if (Global::volumeType() == Global::RGBVolume ||
	   Global::volumeType() == Global::RGBAVolume)
    {
      if (j == 1 && flag)
	m_tfEditor->changeVol(0);
      else if (j == 2 && flag)
	m_tfEditor->changeVol(1);
      else if (j == 3 && flag)
	m_tfEditor->changeVol(2);
      else if (j == 4 && flag)
	m_tfEditor->changeVol(3);
    }


  updateComposite();
}

void
MainWindow::updateComposite()
{
  loadLookupTable();
}

void
MainWindow::applyTFUndo(bool flag)
{
  m_tfManager->applyUndo(flag);
}

void
MainWindow::transferFunctionUpdated()
{
  m_tfManager->transferFunctionUpdated();
}

void
MainWindow::changeTransferFunctionDisplay(int tfnum, QList<bool> on)
{
  if (tfnum >= 0)
    {
      SplineTransferFunction *sptr = m_tfContainer->transferFunctionPtr(tfnum);
      m_tfEditor->setTransferFunction(sptr);

      if (Global::volumeType() == Global::DoubleVolume)
	{
	  if (on.count() > 1)
	    {
	      if (on[0])
		m_tfEditor->changeVol(0);
	      else if (on[1])
		m_tfEditor->changeVol(1);
	    }
	}
      else if (Global::volumeType() == Global::TripleVolume)
	{
	  if (on.count() > 1)
	    {
	      if (on[0])
		m_tfEditor->changeVol(0);
	      else if (on[1])
		m_tfEditor->changeVol(1);
	      else if (on[2])
		m_tfEditor->changeVol(2);
	    }
	}
      else if (Global::volumeType() == Global::QuadVolume)
	{
	  if (on.count() > 1)
	    {
	      if (on[0])
		m_tfEditor->changeVol(0);
	      else if (on[1])
		m_tfEditor->changeVol(1);
	      else if (on[2])
		m_tfEditor->changeVol(2);
	      else if (on[3])
		m_tfEditor->changeVol(3);
	    }
	}
      else if (Global::volumeType() == Global::RGBVolume ||
	       Global::volumeType() == Global::RGBAVolume)
	{
	  if (on.count() > 1)
	    {
	      if (on[0])
		m_tfEditor->changeVol(0);
	      else if (on[1])
		m_tfEditor->changeVol(1);
	      else if (on[2])
		m_tfEditor->changeVol(2);
	      else if (on[3])
		m_tfEditor->changeVol(3);
	    }
	}
    }
  else
    m_tfEditor->setTransferFunction(NULL);    

  updateComposite();
}

void
MainWindow::loadLookupTable()
{
  QList<QImage> imgList;
  for(int i=0; i<Global::lutSize(); i++)
    imgList.append(m_tfContainer->composite(i));

  m_Viewer->loadLookupTable(imgList);
}

void
MainWindow::on_actionPoints_triggered()
{
  QString flnm;
  flnm = QFileDialog::getOpenFileName(0,
				      "Load points file",
				      Global::previousDirectory(),
				      "Files (*.points | *.point)",
				      0,
				      QFileDialog::DontUseNativeDialog);
  
  if (flnm.isEmpty())
    return;
  
  GeometryObjects::hitpoints()->addPoints(flnm);

  if (!haveGrid())
    {
      GeometryObjects::hitpoints()->clear();
      QMessageBox::information(0, "Points",
			       "Removing points data, invalid grid size");
      return;
    }

  QFileInfo f(flnm);
  Global::setPreviousDirectory(f.absolutePath());
}

void
MainWindow::on_actionGrids_triggered()
{
  QString flnm;
  flnm = QFileDialog::getOpenFileName(0,
				      "Load grids file",
				      Global::previousDirectory(),
				      "Files (*.grids | *.grid)",
				      0,
				      QFileDialog::DontUseNativeDialog);
  
  if (flnm.isEmpty())
    return;
  
  GeometryObjects::grids()->addGrid(flnm);
  
  if (!haveGrid())
    {
      GeometryObjects::grids()->clear();
      QMessageBox::information(0, "Grids",
			       "Removing grids data, invalid grid size");
      return;
    }

  QFileInfo f(flnm);
  Global::setPreviousDirectory(f.absolutePath());
}

void
MainWindow::on_actionPaths_triggered()
{
  QString flnm;
  flnm = QFileDialog::getOpenFileName(0,
				      "Load paths/vectors file",
				      Global::previousDirectory(),
				      "Files (*.paths | *.path | *.vec | *.fibers | *.fiber)",
				      0,
				      QFileDialog::DontUseNativeDialog);
  
  if (flnm.isEmpty())
    return;

  QFileInfo f(flnm);
  if (f.suffix() == "vec")
    GeometryObjects::pathgroups()->addVector(flnm);
  else if (f.suffix() == "fibers" ||
	   f.suffix() == "fiber")
    GeometryObjects::paths()->addFibers(flnm);
  else
    {
      QStringList items;
      items << "Yes" << "No";
      bool ok;
      QString item = QInputDialog::getItem(this,
					   "Load paths",
					   "Load as individual paths",
					   items,
					   0,
					   false,
					   &ok);
  
      if (!ok || item == "Yes")
	GeometryObjects::paths()->addPath(flnm);
      else
	GeometryObjects::pathgroups()->addPath(flnm);
    }

  if (!haveGrid())
    {
      GeometryObjects::paths()->clear();
      GeometryObjects::pathgroups()->clear();
      QMessageBox::information(0, "Paths",
			       "Removing paths data, invalid grid size");
      return;
    }

  Global::setPreviousDirectory(f.absolutePath());
}

void
MainWindow::on_actionNetwork_triggered()
{
  QString flnm;
  flnm = QFileDialog::getOpenFileName(0,
				      "Load Network File",
				      Global::previousDirectory(),
				      "Network Files (*porethroat.nc | *.nc | *.graphml | *.network)",
				      0,
				      QFileDialog::DontUseNativeDialog);
  
  if (flnm.isEmpty())
    return;

  GeometryObjects::networks()->addNetwork(flnm);

  if (Global::volumeType() == Global::DummyVolume)
    {
      int nx, ny, nz;
      GeometryObjects::networks()->allGridSize(nx, ny, nz);
      loadDummyVolume(nx, ny, nz);
    }

  QFileInfo f(flnm);
  Global::setPreviousDirectory(f.absolutePath());
}

void
MainWindow::on_actionTriset_triggered()
{
  QString flnm;
  flnm = QFileDialog::getOpenFileName(0,
				      "Load Triset/PLY File",
				      Global::previousDirectory(),
				      "Triset/PLY Files (*.triset | *.ply)",
				      0,
				      QFileDialog::DontUseNativeDialog);
  
  if (flnm.isEmpty())
    return;

  if (StaticFunctions::checkExtension(flnm, ".triset"))
    GeometryObjects::trisets()->addTriset(flnm);
  else
    GeometryObjects::trisets()->addPLY(flnm);

  if (Global::volumeType() == Global::DummyVolume)
    {
      int nx, ny, nz;
      GeometryObjects::trisets()->allGridSize(nx, ny, nz);
      loadDummyVolume(nx, ny, nz);
    }

  QFileInfo f(flnm);
  Global::setPreviousDirectory(f.absolutePath());
}

void
MainWindow::on_actionLandmarks_triggered()
{
  QString flnm;
  flnm = QFileDialog::getOpenFileName(0,
				      "Load Landmarks File",
				      Global::previousDirectory(),
				      "Landmarks Files (*.landmark | *.point | *.points)",
				      0,
				      QFileDialog::DontUseNativeDialog);
  
  if (flnm.isEmpty())
    return;

  GeometryObjects::landmarks()->loadLandmarks(flnm);

  if (Global::volumeType() == Global::DummyVolume)
    {
      int nx, ny, nz;
      GeometryObjects::trisets()->allGridSize(nx, ny, nz);
      loadDummyVolume(nx, ny, nz);
    }

  QFileInfo f(flnm);
  Global::setPreviousDirectory(f.absolutePath());
}

void
MainWindow::loadSingleVolume(QStringList flnm)
{
  Global::resetCurrentProjectFile();
  Global::addRecentFile(flnm[0]);
  updateRecentFileAction();

  createHiresLowresWindows();

  if (VolumeInformation::checkRGB(flnm[0]))
    loadVolumeRGB(flnm[0].toLatin1().data());
  else
    loadVolumeList(flnm, false);

  // reset
  m_bricks->reset();
  m_bricksWidget->refresh();

  LightHandler::reset();
  GeometryObjects::clear();

  m_keyFrame->clear();
  m_keyFrameEditor->clear();
  m_gallery->clear();
  m_lightingWidget->setLightInfo(LightingInformation());
}

void
MainWindow::on_actionLoad_1_Volume_triggered()
{
  QStringList flnm;
  flnm = QFileDialog::getOpenFileNames(0,
				      "Load volume files",
				       Global::previousDirectory(),
				       "NetCDF Files (*.pvl.nc)",
				       0,
				       QFileDialog::DontUseNativeDialog);

  if (flnm.isEmpty())
    return;

  loadSingleVolume(flnm);
}

void
MainWindow::on_actionLoad_2_Volumes_triggered()
{
  QList<QString> vol1;
  QList<QString> vol2;
  
  Load2Volumes load2volumes;  
  load2volumes.exec();
  if (load2volumes.result() == QDialog::Rejected)
    return;

  vol1 = load2volumes.volume1Files();
  vol2 = load2volumes.volume2Files();

  if (vol1.isEmpty())
    {
      QMessageBox::information(0, "Empty List",
			       "No volume listed for Volume 1");
      return;
    }
  if (vol2.isEmpty())
    {
      QMessageBox::information(0, "Empty List",
			       "No volume listed for Volume 2");
      return;
    }
  
  Global::resetCurrentProjectFile();

  createHiresLowresWindows();

  loadVolume2List(vol1, vol2, false);

  // reset
  m_bricks->reset();
  m_bricksWidget->refresh();

  LightHandler::reset();
  GeometryObjects::clear();

  m_keyFrame->clear();
  m_keyFrameEditor->clear();
  m_gallery->clear();
  m_lightingWidget->setLightInfo(LightingInformation());
}


void
MainWindow::on_actionLoad_3_Volumes_triggered()
{
  QList<QString> vol1;
  QList<QString> vol2;
  QList<QString> vol3;
  
  Load3Volumes load3volumes;  
  load3volumes.exec();
  if (load3volumes.result() == QDialog::Rejected)
    return;

  vol1 = load3volumes.volume1Files();
  vol2 = load3volumes.volume2Files();
  vol3 = load3volumes.volume3Files();

  if (vol1.isEmpty())
    {
      QMessageBox::information(0, "Empty List",
			       "No volume listed for Volume 1");
      return;
    }
  if (vol2.isEmpty())
    {
      QMessageBox::information(0, "Empty List",
			       "No volume listed for Volume 2");
      return;
    }
  if (vol3.isEmpty())
    {
      QMessageBox::information(0, "Empty List",
			       "No volume listed for Volume 3");
      return;
    }
  
  Global::resetCurrentProjectFile();

  createHiresLowresWindows();

  loadVolume3List(vol1, vol2, vol3, false);

  // reset
  m_bricks->reset();
  m_bricksWidget->refresh();

  LightHandler::reset();
  GeometryObjects::clear();

  m_keyFrame->clear();
  m_keyFrameEditor->clear();
  m_gallery->clear();
  m_lightingWidget->setLightInfo(LightingInformation());
}

void
MainWindow::on_actionLoad_4_Volumes_triggered()
{
  QList<QString> vol1;
  QList<QString> vol2;
  QList<QString> vol3;
  QList<QString> vol4;
  
  Load4Volumes load4volumes;  
  load4volumes.exec();
  if (load4volumes.result() == QDialog::Rejected)
    return;

  vol1 = load4volumes.volume1Files();
  vol2 = load4volumes.volume2Files();
  vol3 = load4volumes.volume3Files();
  vol4 = load4volumes.volume4Files();

  if (vol1.isEmpty())
    {
      QMessageBox::information(0, "Empty List",
			       "No volume listed for Volume 1");
      return;
    }
  if (vol2.isEmpty())
    {
      QMessageBox::information(0, "Empty List",
			       "No volume listed for Volume 2");
      return;
    }
  if (vol3.isEmpty())
    {
      QMessageBox::information(0, "Empty List",
			       "No volume listed for Volume 3");
      return;
    }
  if (vol4.isEmpty())
    {
      QMessageBox::information(0, "Empty List",
			       "No volume listed for Volume 4");
      return;
    }
  
  Global::resetCurrentProjectFile();

  createHiresLowresWindows();

  loadVolume4List(vol1, vol2, vol3, vol4, false);

  // reset
  m_bricks->reset();
  m_bricksWidget->refresh();

  LightHandler::reset();
  GeometryObjects::clear();

  m_keyFrame->clear();
  m_keyFrameEditor->clear();
  m_gallery->clear();
  m_lightingWidget->setLightInfo(LightingInformation());
}



void
MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
  if (event && event->mimeData())
    {
      const QMimeData *md = event->mimeData();
      if (md->hasUrls())
	{
	  QList<QUrl> urls = md->urls();
	  if (StaticFunctions::checkURLs(urls, ".pvl.nc"))
	    {
	      event->acceptProposedAction();
	    }
	  else if (StaticFunctions::checkURLs(urls, ".xml"))
	    {
	      event->acceptProposedAction();
	    }
	  else if (StaticFunctions::checkURLs(urls, ".keyframes"))
	    {
	      event->acceptProposedAction();
	    }
	  else if (StaticFunctions::checkURLs(urls, ".triset"))
	    {
	      event->acceptProposedAction();
	    }
	  else if (StaticFunctions::checkURLs(urls, ".ply"))
	    {
	      event->acceptProposedAction();
	    }
	  else if (StaticFunctions::checkURLs(urls, "porethroat.nc"))
	    {
	      event->acceptProposedAction();
	    }
	  else if (StaticFunctions::checkURLs(urls, "graphml"))
	    {
	      event->acceptProposedAction();
	    }
	  else if (StaticFunctions::checkURLs(urls, "network"))
	    {
	      event->acceptProposedAction();
	    }
	  else if (StaticFunctions::checkURLs(urls, ".points") ||
		   StaticFunctions::checkURLs(urls, ".point") ||
		   StaticFunctions::checkURLs(urls, ".landmark") ||
		   StaticFunctions::checkURLs(urls, ".landmarks") ||
		   StaticFunctions::checkURLs(urls, ".paths") ||
		   StaticFunctions::checkURLs(urls, ".path") ||
		   StaticFunctions::checkURLs(urls, ".fibers") ||
		   StaticFunctions::checkURLs(urls, ".fiber") ||
		   StaticFunctions::checkURLs(urls, ".vec") ||
		   StaticFunctions::checkURLs(urls, ".grids") ||
		   StaticFunctions::checkURLs(urls, ".grid"))
	    {
	      event->acceptProposedAction();
	    }
//	  else // ignore the drag
	}
    }
}

void
MainWindow::openRecentFile()
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

      if (StaticFunctions::checkExtension(filename, ".pvl.nc"))
	{
	  QStringList flist;
	  flist << filename;
	  loadSingleVolume(flist);
	}
      else if (StaticFunctions::checkExtension(filename, ".xml"))
	{
	  Global::addRecentFile(filename);
	  updateRecentFileAction();
	  createHiresLowresWindows();
	  loadProject(filename.toLatin1().data());
	}
    }
}

void
MainWindow::dropEvent(QDropEvent *event)
{
  if (! GlewInit::initialised())
    {
      QMessageBox::information(0, "Drishti", "Not yet ready to start work!");
      return;
    }

  if (event && event->mimeData())
    {
      const QMimeData *data = event->mimeData();
      if (data->hasUrls())
	{
	  QUrl url = data->urls()[0];
	  QFileInfo info(url.toLocalFile());
	  if (info.exists() && info.isFile())
	    {
	      if (StaticFunctions::checkExtension(url.toLocalFile(), ".pvl.nc"))
		{
		  QStringList flist;
		  QList<QUrl> urls = data->urls();
		  for(int i=0; i<urls.count(); i++)
		    flist.append(urls[i].toLocalFile());
		  
		  loadSingleVolume(flist);
		}
	      else if (StaticFunctions::checkExtension(url.toLocalFile(), ".xml"))
		{
		  Global::addRecentFile(url.toLocalFile());
		  updateRecentFileAction();
		  createHiresLowresWindows();
		  loadProject(url.toLocalFile().toLatin1().data());
		}
	      else if (StaticFunctions::checkExtension(url.toLocalFile(), ".keyframes"))
		{
		  m_keyFrame->import(url.toLocalFile());
		}
	      else if (StaticFunctions::checkExtension(url.toLocalFile(), ".triset") ||
		       StaticFunctions::checkExtension(url.toLocalFile(), ".ply"))
		{
		  if (StaticFunctions::checkExtension(url.toLocalFile(), ".triset"))
		    GeometryObjects::trisets()->addTriset(url.toLocalFile());
		  else
		    GeometryObjects::trisets()->addPLY(url.toLocalFile());
		  if (Global::volumeType() == Global::DummyVolume)
		    {
		      int nx, ny, nz;
		      GeometryObjects::trisets()->allGridSize(nx, ny, nz);
		      loadDummyVolume(nx, ny, nz);
		    }

		  QFileInfo f(url.toLocalFile());
		  Global::setPreviousDirectory(f.absolutePath());
		}
	      else if (StaticFunctions::checkExtension(url.toLocalFile(), "porethroat.nc") ||
		       StaticFunctions::checkExtension(url.toLocalFile(), "graphml") ||
		       StaticFunctions::checkExtension(url.toLocalFile(), "network"))
		{
		  GeometryObjects::networks()->addNetwork(url.toLocalFile());
		  if (Global::volumeType() == Global::DummyVolume)
		    {
		      int nx, ny, nz;
		      GeometryObjects::networks()->allGridSize(nx, ny, nz);
		      loadDummyVolume(nx, ny, nz);
		    }

		  QFileInfo f(url.toLocalFile());
		  Global::setPreviousDirectory(f.absolutePath());
		}
	      else if (StaticFunctions::checkExtension(url.toLocalFile(), ".points") ||
		       StaticFunctions::checkExtension(url.toLocalFile(), ".point"))
		{
		  GeometryObjects::hitpoints()->addPoints(url.toLocalFile());

		  if (!haveGrid())
		    {
		      GeometryObjects::hitpoints()->clear();
		      QMessageBox::information(0, "Points",
					       "Removing points data, invalid grid size");
		      return;
		    }
		  
		  QFileInfo f(url.toLocalFile());		  
		  Global::setPreviousDirectory(f.absolutePath());
		}
	      else if (StaticFunctions::checkExtension(url.toLocalFile(), ".landmarks") ||
		       StaticFunctions::checkExtension(url.toLocalFile(), ".landmark"))
		{
		  GeometryObjects::landmarks()->loadLandmarks(url.toLocalFile());

		  if (!haveGrid())
		    {
		      GeometryObjects::landmarks()->clear();
		      QMessageBox::information(0, "Points",
					       "Removing landmark data, invalid grid size");
		      return;
		    }
		  
		  QFileInfo f(url.toLocalFile());		  
		  Global::setPreviousDirectory(f.absolutePath());
		}
	      else if (StaticFunctions::checkExtension(url.toLocalFile(), ".grids") ||
		       StaticFunctions::checkExtension(url.toLocalFile(), ".grid"))
		{
		  GeometryObjects::grids()->addGrid(url.toLocalFile());
		  if (!haveGrid())
		    {
		      GeometryObjects::grids()->clear();
		      QMessageBox::information(0, "Grids",
					       "Removing grid data, invalid grid size");
		      return;
		    }
		  QFileInfo f(url.toLocalFile());		  
		  Global::setPreviousDirectory(f.absolutePath());
		}
	      else if (StaticFunctions::checkExtension(url.toLocalFile(), ".fibers") ||
		       StaticFunctions::checkExtension(url.toLocalFile(), ".fiber"))
		{
		  GeometryObjects::paths()->addFibers(url.toLocalFile());
		  if (!haveGrid())
		    {
		      GeometryObjects::paths()->clear();
		      GeometryObjects::pathgroups()->clear();		      
		      QMessageBox::information(0, "Fibers",
					       "Removing fibers data, invalid grid size");
		      return;
		    }
		  
		  QFileInfo f(url.toLocalFile());		  
		  Global::setPreviousDirectory(f.absolutePath());
		}
	      else if (StaticFunctions::checkExtension(url.toLocalFile(), ".paths") ||
		       StaticFunctions::checkExtension(url.toLocalFile(), ".path"))
		{
		  QStringList items;
		  items << "Yes" << "No";
		  bool ok;
		  QString item = QInputDialog::getItem(this,
						       "Load paths",
						       "Load as individual paths",
						       items,
						       0,
						       false,
						       &ok);
		  if (!ok || item == "Yes")
		    GeometryObjects::paths()->addPath(url.toLocalFile());
		  else
		    GeometryObjects::pathgroups()->addPath(url.toLocalFile());
						       
		  if (!haveGrid())
		    {
		      GeometryObjects::paths()->clear();
		      GeometryObjects::pathgroups()->clear();		      
		      QMessageBox::information(0, "Paths",
					       "Removing paths data, invalid grid size");
		      return;
		    }
		  
		  QFileInfo f(url.toLocalFile());		  
		  Global::setPreviousDirectory(f.absolutePath());
		}
	      else if (StaticFunctions::checkExtension(url.toLocalFile(), ".vec"))
		{
		  GeometryObjects::pathgroups()->addVector(url.toLocalFile());
						       
		  if (!haveGrid())
		    {
		      GeometryObjects::paths()->clear();
		      GeometryObjects::pathgroups()->clear();		      
		      QMessageBox::information(0, "Paths",
					       "Removing paths data, invalid grid size");
		      return;
		    }
		  
		  QFileInfo f(url.toLocalFile());		  
		  Global::setPreviousDirectory(f.absolutePath());
		}
	    }
	}
    }
}

bool
MainWindow::haveGrid()
{
  if (Global::volumeType() != Global::DummyVolume)
    return true;

  // ask user for the grid size
  bool ok;
  QString text = QInputDialog::getText(0,
				       "Please enter grid size",
				       "Grid Size",
				       QLineEdit::Normal,
				       "0 0 0",
				       &ok);
  if (ok && !text.isEmpty())
    {
      int nx=0;
      int ny=0;
      int nz=0;
      
      QStringList gs = text.split(" ", QString::SkipEmptyParts);
      if (gs.count() > 0) nx = gs[0].toInt();
      if (gs.count() > 1) ny = gs[1].toInt();
      if (gs.count() > 2) nz = gs[2].toInt();
      if (nx > 0 && ny > 0 && nz > 0)
	{
	  loadDummyVolume(nx, ny, nz);
	  return true;
	}
    }

  return false;
}

void
MainWindow::loadDummyVolume(int nx, int ny, int nz)
{
  Global::setSaveImageType(Global::NoImage);

  m_keyFrame->clear();
  m_keyFrameEditor->clear();
  m_gallery->clear();

  m_keyFrameEditor->setHiresMode(false);
  m_gallery->setHiresMode(false);

  m_Volume->loadDummyVolume(nx, ny, nz);

  postLoadVolume();

  Global::setVolumeNumber(0);

  m_tfEditor->setTransferFunction(NULL);
  m_tfManager->setEnabled(false);

  Global::setVolumeNumber(0);

  QList<int> vsizes;
  vsizes << 1;
  emit setVolumes(vsizes);
  emit refreshVolInfo(0, m_Volume->volInfo(0));
}

void
MainWindow::loadVolumeFromUrls(QList<QUrl> urls)
{
  Global::setSaveImageType(Global::NoImage);

  if (urls.count() > 0)
    {
      QList<QString> files;
      for(int i=0; i<urls.count(); i++)
	files.append(urls[i].toLocalFile());

      bool ok;
      VolumeInformation pvlInfo1;
      ok = VolumeInformation::volInfo(files[0], pvlInfo1);
      if (ok)
	loadVolumeList(files, false);
      else
	QMessageBox::critical(0, "Loading Volume",
			      QString("Cannot load volume : ") +
			      files[0] +
			      QString(" is invalid"));
    }
}

void
MainWindow::loadVolumeList(QList<QString> files, bool flag)
{
  if (flag)
    {
      if (m_Volume->valid())
	{
	  if (Global::volumeType() == Global::SingleVolume)
	    {
	      QList<QString> files1 = m_Volume->volumeFiles();

	      if (VolumeInformation::checkForDoubleVolume(files1,
							  files))
		{
		  loadVolume2List(files1, files, true);
		  return;
		}
	    }
	  if (Global::volumeType() == Global::DoubleVolume)
	    {
	      QList<QString> files1 = m_Volume->volumeFiles();
	      QList<QString> files2 = m_Volume->volumeFiles(1);

	      if (VolumeInformation::checkForTripleVolume(files1,
							  files2,
							  files))
		{
		  loadVolume3List(files1, files2,
				  files, true);
		  return;
		}
	    }
	  if (Global::volumeType() == Global::TripleVolume)
	    {
	      QList<QString> files1 = m_Volume->volumeFiles();
	      QList<QString> files2 = m_Volume->volumeFiles(1);
	      QList<QString> files3 = m_Volume->volumeFiles(2);

	      if (VolumeInformation::checkForQuadVolume(files1,
							files2,
							files3,
							files))
		{
		  loadVolume4List(files1, files2,
				  files3, files, true);
		  return;
		}
	    }
	}
    }

  Global::setSaveImageType(Global::NoImage);

  if (flag)
    {
      if (files.count() > 1)
	{
	  FilesListDialog fld(files);
	  fld.exec();
	  if (fld.result() == QDialog::Rejected)
	    return;
	}
    }

  loadVolume(files);

  Global::setVolumeNumber(0);

  QList<QString> volfiles;
  for(int i=0; i<files.count(); i++)
    volfiles.append(files[i].toLatin1().data());

  QList<int> vsizes;
  vsizes << volfiles.size();
  emit setVolumes(vsizes);
  emit refreshVolInfo(0, m_Volume->volInfo(0));
}

void
MainWindow::preLoadVolume()
{
  RawVolume::reset();
  LightHandler::reset();

  GeometryObjects::imageCaptions()->clear();
  GeometryObjects::captions()->clear();
  GeometryObjects::hitpoints()->clear();
  GeometryObjects::landmarks()->clear();
  GeometryObjects::paths()->clear();
  GeometryObjects::grids()->clear();
  GeometryObjects::crops()->clear();
  GeometryObjects::pathgroups()->clear();
  GeometryObjects::trisets()->clear();
  GeometryObjects::networks()->clear();

  m_keyFrame->clear();
  m_keyFrameEditor->clear();
  m_gallery->clear();

  m_keyFrameEditor->setHiresMode(false);
  m_gallery->setHiresMode(false);
}

void
MainWindow::postLoadVolume()
{
  if (Global::volumeType() == Global::RGBVolume ||
      Global::volumeType() == Global::RGBAVolume)
    {
      Global::setEmptySpaceSkip(false);
      MainWindowUI::mainWindowUI()->actionEmptySpaceSkip->setChecked(Global::emptySpaceSkip());
      MainWindowUI::mainWindowUI()->actionEmptySpaceSkip->setDisabled(true);
    }
  else
    MainWindowUI::mainWindowUI()->actionEmptySpaceSkip->setEnabled(true);


  if (Global::volumeType() == Global::DummyVolume)
    Global::setLutSize(8);
  else if (Global::volumeType() == Global::SingleVolume)
    Global::setLutSize(8);
  else
    Global::setLutSize(16);

  m_bricksWidget->setTFSets(Global::lutSize());


  m_tfEditor->setTransferFunction(NULL);
  m_tfManager->clearManager();

  m_Lowres->setCurrentVolume(0);
  m_Hires->setCurrentVolume(0);

  m_Lowres->loadVolume();
  m_Hires->loadVolume();

  if (Global::volumeType() != Global::DummyVolume)
    {
      VolumeInformation volInfo = VolumeInformation::volumeInformation(0);
      QPolygonF fmap = volInfo.mapping;
      m_tfEditor->setMapping(fmap);
      
      if (m_Volume->pvlVoxelType(0) == 0)
	m_tfEditor->setHistogramImage(m_Lowres->histogramImage1D(),
				      m_Lowres->histogramImage2D());
      else
	m_tfEditor->setHistogram2D(m_Lowres->histogram2D());
    }

  m_Lowres->raise();
  m_Hires->lower();

  m_tfManager->setEnabled(true);

  m_Viewer->resetLookupTable();

 if (Global::volumeType() != Global::DummyVolume)
   m_tfManager->loadDefaultTF();

  Global::enableViewerUpdate();
  MainWindowUI::changeDrishtiIcon(true);

  QList<bool> rt = m_volInfoWidget->repeatType();
  m_Volume->setRepeatType(rt);

  // use 1D transfer functions for 16 bit data sets
  if (m_Volume->pvlVoxelType(0) > 0)
    {
      Global::setUse1D(true);
      ui.actionSwitch_To1D->setChecked(Global::use1D());
      m_tfContainer->switch1D();
    }
}

void
MainWindow::loadVolumeRGBFromUrls(QList<QUrl> urls)
{
  Global::setSaveImageType(Global::NoImage);

  if (urls.count() > 0)
    {
      QList<QString> files;
      for(int i=0; i<urls.count(); i++)
	files.append(urls[i].toLocalFile());

      loadVolumeRGB(files[0].toLatin1().data());
    }
}

void
MainWindow::loadVolumeRGB(char *flnm)
{  
  Global::setSaveImageType(Global::NoImage);

  if (QString(flnm).isEmpty())
    return;
 
  preLoadVolume();

  m_Volume->loadVolumeRGB(flnm, false);

  postLoadVolume();

  QFileInfo f(flnm);
  Global::setPreviousDirectory(f.absolutePath());

  Global::setVolumeNumber(0);

  QList<int> vsizes;
  vsizes << 1;
  emit setVolumes(vsizes);
  emit refreshVolInfo(0, m_Volume->volInfo(0));

  if (!Global::batchMode())
    {
      if (Global::volumeType() == Global::RGBVolume)
	emit showMessage("RGB Volume loaded", false);
      else
	emit showMessage("RGBA Volume loaded", false);
    }
}


void
MainWindow::loadVolume(QList<QString> flnm)
{  
  Global::setSaveImageType(Global::NoImage);

  if (flnm.count() == 0)
    return;
 
  preLoadVolume();

  m_Volume->loadVolume(flnm, false);

  postLoadVolume();

  QFileInfo f(flnm[0]);
  Global::setPreviousDirectory(f.absolutePath());

  Global::setVolumeNumber(0);

  QList<int> vsizes;
  vsizes << 1;
  emit setVolumes(vsizes);
  emit refreshVolInfo(0, m_Volume->volInfo(0));

  if (!Global::batchMode())
    {
      if (Global::use1D())
	emit showMessage("Volume loaded. Currently in 1D Transfer Function mode", false);
      else
	emit showMessage("Volume loaded", false);

    }
}

void
MainWindow::loadVolume2List(QList<QString> files1,
			    QList<QString> files2,
			    bool flag)
{
  Global::setSaveImageType(Global::NoImage);

  if (flag)
    {
      FilesListDialog fld1(files1);
      fld1.exec();
      if (fld1.result() == QDialog::Rejected)
	return;
      
      FilesListDialog fld2(files2);
      fld2.exec();
      if (fld2.result() == QDialog::Rejected)
	return;
    }

  if (!loadVolume2(files1, files2))
    return;

  Global::setVolumeNumber(0);
  Global::setVolumeNumber(0, 1);


  QList<QString> volfiles1;
  for(int i=0; i<files1.count(); i++)
    volfiles1.append(files1[i].toLatin1().data());

  QList<QString> volfiles2;
  for(int i=0; i<files2.count(); i++)
    volfiles2.append(files2[i].toLatin1().data());

  QList<int> vsizes;
  vsizes << volfiles1.size();
  vsizes << volfiles2.size();
  emit setVolumes(vsizes);
  emit refreshVolInfo(0, 0, m_Volume->volInfo(0));
  emit refreshVolInfo(1, 0, m_Volume->volInfo(0,1));
}

bool
MainWindow::loadVolume2(QList<QString> flnm1,
			QList<QString> flnm2)
{  
  Global::setSaveImageType(Global::NoImage);

  if (flnm1.count() == 0 ||
      flnm2.count() == 0)
    return false;

  preLoadVolume();

  if (!m_Volume->loadVolume(flnm1, flnm2, false))
    return false;

  postLoadVolume();

  QFileInfo f(flnm2[0]);
  Global::setPreviousDirectory(f.absolutePath());

  Global::setVolumeNumber(0);
  Global::setVolumeNumber(0, 1);

  QList<int> vsizes;
  vsizes << 1;
  vsizes << 1;
  emit setVolumes(vsizes);
  emit refreshVolInfo(0, 0, m_Volume->volInfo(0));
  emit refreshVolInfo(1, 0, m_Volume->volInfo(0, 1));

  if (!Global::batchMode())
    {
      if (Global::use1D())
	emit showMessage("Volumes loaded. Currently in 1D Transfer Function mode", false);
      else
	emit showMessage("Volumes loaded", false);
    }

  return true;
}

void
MainWindow::loadVolume3List(QList<QString> files1,
			    QList<QString> files2,
			    QList<QString> files3,
			    bool flag)
{
  Global::setSaveImageType(Global::NoImage);

  if (flag)
    {
      FilesListDialog fld1(files1);
      fld1.exec();
      if (fld1.result() == QDialog::Rejected)
	return;
      
      FilesListDialog fld2(files2);
      fld2.exec();
      if (fld2.result() == QDialog::Rejected)
	return;

      FilesListDialog fld3(files3);
      fld3.exec();
      if (fld3.result() == QDialog::Rejected)
	return;
    }

  if (!loadVolume3(files1, files2, files3))
    return;

  Global::setVolumeNumber(0);
  Global::setVolumeNumber(0, 1);
  Global::setVolumeNumber(0, 2);


  QList<QString> volfiles1;
  for(int i=0; i<files1.count(); i++)
    volfiles1.append(files1[i].toLatin1().data());

  QList<QString> volfiles2;
  for(int i=0; i<files2.count(); i++)
    volfiles2.append(files2[i].toLatin1().data());

  QList<QString> volfiles3;
  for(int i=0; i<files3.count(); i++)
    volfiles3.append(files3[i].toLatin1().data());

  QList<int> vsizes;
  vsizes << volfiles1.size();
  vsizes << volfiles2.size();
  vsizes << volfiles3.size();
  emit setVolumes(vsizes);
  emit refreshVolInfo(0, 0, m_Volume->volInfo(0));
  emit refreshVolInfo(1, 0, m_Volume->volInfo(0,1));
  emit refreshVolInfo(2, 0, m_Volume->volInfo(0,2));

//  // for 3 or 4 volumes always use 1D transfer functions
//  Global::setUse1D(true);
//  ui.actionSwitch_To1D->setChecked(Global::use1D());
}

bool
MainWindow::loadVolume3(QList<QString> flnm1,
			QList<QString> flnm2,
			QList<QString> flnm3)
{  
  Global::setSaveImageType(Global::NoImage);

  if (flnm1.count() == 0 ||
      flnm2.count() == 0 ||
      flnm3.count() == 0)
    return false;

//  // for 3 or 4 volumes always use 1D transfer functions
//  Global::setUse1D(true);
//  ui.actionSwitch_To1D->setChecked(Global::use1D());


  preLoadVolume();
  
  if (!m_Volume->loadVolume(flnm1, flnm2, flnm3, false))
    return false;

  postLoadVolume();

  QFileInfo f(flnm3[0]);
  Global::setPreviousDirectory(f.absolutePath());

  Global::setVolumeNumber(0);
  Global::setVolumeNumber(0, 1);
  Global::setVolumeNumber(0, 2);

  QList<int> vsizes;
  vsizes << 1;
  vsizes << 1;
  vsizes << 1;
  emit setVolumes(vsizes);
  emit refreshVolInfo(0, 0, m_Volume->volInfo(0));
  emit refreshVolInfo(1, 0, m_Volume->volInfo(0, 1));
  emit refreshVolInfo(2, 0, m_Volume->volInfo(0, 2));

  if (!Global::batchMode())
    {
      if (Global::use1D())
	emit showMessage("3 Volumes loaded. Using only 1D Transfer Functions", false);
      else
	emit showMessage("3 Volumes loaded", false);
    }

  return true;
}


void
MainWindow::loadVolume4List(QList<QString> files1,
			    QList<QString> files2,
			    QList<QString> files3,
			    QList<QString> files4,
			    bool flag)
{
  Global::setSaveImageType(Global::NoImage);

  if (flag)
    {
      FilesListDialog fld1(files1);
      fld1.exec();
      if (fld1.result() == QDialog::Rejected)
	return;
      
      FilesListDialog fld2(files2);
      fld2.exec();
      if (fld2.result() == QDialog::Rejected)
	return;

      FilesListDialog fld3(files3);
      fld3.exec();
      if (fld3.result() == QDialog::Rejected)
	return;

      FilesListDialog fld4(files4);
      fld4.exec();
      if (fld4.result() == QDialog::Rejected)
	return;
    }

  if (!loadVolume4(files1, files2, files3, files4))
    return;

  Global::setVolumeNumber(0);
  Global::setVolumeNumber(0, 1);
  Global::setVolumeNumber(0, 2);
  Global::setVolumeNumber(0, 3);


  QList<QString> volfiles1;
  for(int i=0; i<files1.count(); i++)
    volfiles1.append(files1[i].toLatin1().data());

  QList<QString> volfiles2;
  for(int i=0; i<files2.count(); i++)
    volfiles2.append(files2[i].toLatin1().data());

  QList<QString> volfiles3;
  for(int i=0; i<files3.count(); i++)
    volfiles3.append(files3[i].toLatin1().data());

  QList<QString> volfiles4;
  for(int i=0; i<files4.count(); i++)
    volfiles4.append(files4[i].toLatin1().data());


  QList<int> vsizes;
  vsizes << volfiles1.size();
  vsizes << volfiles2.size();
  vsizes << volfiles3.size();
  vsizes << volfiles4.size();
  emit setVolumes(vsizes);
  emit refreshVolInfo(0, 0, m_Volume->volInfo(0));
  emit refreshVolInfo(1, 0, m_Volume->volInfo(0,1));
  emit refreshVolInfo(2, 0, m_Volume->volInfo(0,2));
  emit refreshVolInfo(3, 0, m_Volume->volInfo(0,3));

//  // for 3 or 4 volumes always use 1D transfer functions
//  Global::setUse1D(true);
//  ui.actionSwitch_To1D->setChecked(Global::use1D());
}

bool
MainWindow::loadVolume4(QList<QString> flnm1,
			QList<QString> flnm2,
			QList<QString> flnm3,
			QList<QString> flnm4)
{  
  Global::setSaveImageType(Global::NoImage);

  if (flnm1.count() == 0 ||
      flnm2.count() == 0 ||
      flnm3.count() == 0 ||
      flnm4.count() == 0)
    return false;

//  // for 3 or 4 volumes always use 1D transfer functions
//  Global::setUse1D(true);
//  ui.actionSwitch_To1D->setChecked(Global::use1D());

  preLoadVolume();

  if (!m_Volume->loadVolume(flnm1, flnm2, flnm3, flnm4, false))
    return false;

  postLoadVolume();

  QFileInfo f(flnm4[0]);
  Global::setPreviousDirectory(f.absolutePath());

  Global::setVolumeNumber(0);
  Global::setVolumeNumber(0, 1);
  Global::setVolumeNumber(0, 2);
  Global::setVolumeNumber(0, 3);


  QList<int> vsizes;
  vsizes << 1;
  vsizes << 1;
  vsizes << 1;
  vsizes << 1;
  emit setVolumes(vsizes);
  emit refreshVolInfo(0, 0, m_Volume->volInfo(0));
  emit refreshVolInfo(1, 0, m_Volume->volInfo(0, 1));
  emit refreshVolInfo(2, 0, m_Volume->volInfo(0, 2));
  emit refreshVolInfo(3, 0, m_Volume->volInfo(0, 3));

  if (!Global::batchMode())
    {
      if (Global::use1D())
	emit showMessage("4 Volumes loaded. Using only 1D Transfer Functions", false);
      else
	emit showMessage("4 Volumes loaded", false);
    }

  return true;
}


void
MainWindow::saveSettings()
{
  QString str;
  QDomDocument doc("Drishti_v1.0");

  QDomElement topElement = doc.createElement("DrishtiGlobalSettings");
  doc.appendChild(topElement);

  {
    QDomElement de0 = doc.createElement("texturememory");
    QDomText tn0;
    tn0 = doc.createTextNode(QString("%1").arg(Global::textureMemorySize()));
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }

  {
    QDomElement de0 = doc.createElement("texsizereducefraction");
    QDomText tn0;
    tn0 = doc.createTextNode(QString("%1").arg(Global::texSizeReduceFraction()));
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }
  
  {
    QDomElement de0 = doc.createElement("texturesizelimit");
    QDomText tn0;
    tn0 = doc.createTextNode(QString("%1").arg(Global::textureSizeLimit()));
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }

  {
    QDomElement de0 = doc.createElement("tempdirectory");
    QDomText tn0;
    tn0 = doc.createTextNode(Global::tempDir());
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
    QDomElement de0 = doc.createElement("replacetf");
    QDomText tn0;
    tn0 = doc.createTextNode(QString("%1").arg(Global::replaceTF()));
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }

  {
    QDomElement de0 = doc.createElement("floatprecision");
    QDomText tn0;
    tn0 = doc.createTextNode(QString("%1").arg(Global::floatPrecision()));
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }

  QString homePath = QDir::homePath();
  QFileInfo settingsFile(homePath, ".drishti.xml");
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


  m_preferencesWidget->save(flnm.toLatin1().data());
}

void
MainWindow::loadSettings()
{
  QString homePath = QDir::homePath();
  QFileInfo settingsFile(homePath, ".drishti.xml");

  if (! settingsFile.exists())
    return;

  QString flnm = settingsFile.absoluteFilePath();  


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
      if (dlist.at(i).nodeName() == "texturememory")
	{
	  QString str = dlist.at(i).toElement().text();
	  Global::setTextureMemorySize(str.toInt());
	  Global::calculate3dTextureSize();
	}
      else if (dlist.at(i).nodeName() == "texsizereducefraction")
	{
	  QString str = dlist.at(i).toElement().text();
	  Global::setTexSizeReduceFraction(str.toFloat());
	}
      else if (dlist.at(i).nodeName() == "texturesizelimit")
	{
	  QString str = dlist.at(i).toElement().text();
	  Global::setTextureSizeLimit(str.toInt());
	}
      else if (dlist.at(i).nodeName() == "tempdirectory")
	{
	  QString str = dlist.at(i).toElement().text();
	  Global::setTempDir(str);
	}
      else if (dlist.at(i).nodeName() == "previousdirectory")
	{
	  QString str = dlist.at(i).toElement().text();
	  Global::setPreviousDirectory(str);
	}
      else if (dlist.at(i).nodeName() == "recentfile")
	{
	  QString str = dlist.at(i).toElement().text();
	  Global::addRecentFile(str);
	}
      else if (dlist.at(i).nodeName() == "replacetf")
	{
	  QString str = dlist.at(i).toElement().text();
	  if (str == "true")
	    m_tfManager->updateReplace(true);
	  else
	    m_tfManager->updateReplace(false);
	}
      else if (dlist.at(i).nodeName() == "floatprecision")
	{
	  QString str = dlist.at(i).toElement().text();
	  Global::setFloatPrecision(str.toInt());
	}
    }
  m_preferencesWidget->updateTextureMemory();
  m_preferencesWidget->load(flnm.toLatin1().data());
  updateRecentFileAction();
}

void
MainWindow::loadTransferFunctionsOnly(const char* flnm)
{
  m_tfManager->append(flnm);
}

void
MainWindow::loadProject(const char* flnm)
{
  if (! GlewInit::initialised())
    {
      QMessageBox::information(0, "Drishti", "Not yet ready to start work!");
      return;
    }
  if (!Global::batchMode())
    {
      Global::setEmptySpaceSkip(true);
      MainWindowUI::mainWindowUI()->actionEmptySpaceSkip->setChecked(Global::emptySpaceSkip());
    }

  Global::disableViewerUpdate();
  MainWindowUI::changeDrishtiIcon(false);
  m_Hires->disableSubvolumeUpdates();

//  bool galleryVisible = m_dockGallery->isVisible();
  bool keyframesVisible = m_dockKeyframe->isVisible();

  Global::setCurrentProjectFile(QString(flnm));

  m_volFiles1.clear();
  m_volFiles2.clear();
  m_volFiles3.clear();
  m_volFiles4.clear();

  QFileInfo f(flnm);
  Global::setPreviousDirectory(f.absolutePath());

  int projectType = loadVolumeFromProject(flnm);

  if (projectType == Global::SingleVolume)
    loadVolumeList(m_volFiles1, false);
  else if (projectType == Global::DoubleVolume)
    loadVolume2List(m_volFiles1, m_volFiles2, false);
  else if (projectType == Global::TripleVolume)
    loadVolume3List(m_volFiles1, m_volFiles2, m_volFiles3, false);
  else if (projectType == Global::QuadVolume)
    loadVolume4List(m_volFiles1, m_volFiles2, m_volFiles3, m_volFiles4, false);
  else if (projectType == Global::RGBVolume ||
	   projectType == Global::RGBAVolume)
    loadVolumeRGB(m_volFiles1[0].toLatin1().data());


  m_bricks->reset();
  GeometryObjects::clipplanes()->reset();

  m_keyFrame->clear();
  m_keyFrameEditor->clear();
  m_gallery->clear();

  if (!Global::batchMode())
    emit showMessage("Volume loaded. Loading Project ....", false);

  m_Lowres->load(flnm);
  if (projectType == Global::DummyVolume)
    {
      Vec bmin, bmax;
      m_Lowres->subvolumeBounds(bmin, bmax);

      Vec vmax = m_Lowres->volumeMax();
      int nx = vmax.x;
      int ny = vmax.y;
      int nz = vmax.z;
      loadDummyVolume(nx, ny, nz);

      Vec sbmin, sbmax;
      sbmin = Vec(bmin.z, bmin.y, bmin.x);
      sbmax = Vec(bmax.z, bmax.y, bmax.x);
      m_Lowres->setSubvolumeBounds(sbmin, sbmax);
    }

  m_preferencesWidget->load(flnm);
  m_tfManager->load(flnm);

  
//  m_dockGallery->setVisible(false);
  m_dockKeyframe->setVisible(false);

  loadViewsAndKeyFrames(flnm);

  m_bricksWidget->refresh();

  m_Hires->enableSubvolumeUpdates();
  m_Viewer->switchToHires();
  m_keyFrameEditor->setHiresMode(true);
  m_gallery->setHiresMode(true);
  if (Global::volumeType() != Global::DummyVolume)
    {
      if (m_Volume->pvlVoxelType(0) == 0)
	m_tfEditor->setHistogramImage(m_Hires->histogramImage1D(),
				      m_Hires->histogramImage2D());
      else
	m_tfEditor->setHistogram2D(m_Hires->histogram2D());
    }

//  m_dockGallery->setVisible(galleryVisible);
  m_dockKeyframe->setVisible(keyframesVisible);

  m_Viewer->createImageBuffers();

  updateComposite();

  Global::enableViewerUpdate();
  MainWindowUI::changeDrishtiIcon(true);


  GeometryObjects::showGeometry = true;
  GeometryObjects::show();
  GeometryObjects::inPool = false;
  GeometryObjects::removeFromMouseGrabberPool();

  LightHandler::removeFromMouseGrabberPool();

  m_keyFrame->playSavedKeyFrame();
  
  m_Viewer->updateLookupTable();

  if (!Global::batchMode())
    {
      if (Global::use1D())
	emit showMessage("Project loaded. Currently in 1D Transfer Function mode", false);
      else
	emit showMessage("Project loaded", false);
    }

  // always keep image captions in mouse grabber pool
  GeometryObjects::imageCaptions()->addInMouseGrabberPool();
}

void
MainWindow::saveProject(QString xmlflnm, QString dtvfile)
{
  int flnmlen = xmlflnm.length()+1;
  char *flnm = new char[flnmlen];
  memset(flnm, 0, flnmlen);
  memcpy(flnm, xmlflnm.toLatin1().data(), flnmlen);

  Vec bmin, bmax;
  m_Lowres->subvolumeBounds(bmin, bmax);
  QImage image = m_Viewer->grabFrameBuffer();
  image = image.scaled(100, 100);
  int sz, st;
  QString xl, yl, zl;
  m_preferencesWidget->getTick(sz, st, xl, yl, zl);
  int mv;
  bool mc, mo, mt;
  m_Hires->getMix(mv, mc, mo, mt);

  float fop, bop;
  m_Hires->getOpMod(fop, bop);

  m_keyFrame->saveProject(m_Viewer->camera()->position(),
			  m_Viewer->camera()->orientation(),
			  m_Viewer->camera()->focusDistance(),
			  m_Viewer->camera()->IODistance(),
			  Global::volumeNumber(),
			  Global::volumeNumber(1),
			  Global::volumeNumber(2),
			  Global::volumeNumber(3),
			  m_Viewer->lookupTable(),
			  m_Hires->lightInfo(),
			  m_Hires->bricks(),
			  bmin, bmax,
			  image,
			  sz, st, xl, yl, zl,
			  mv, mc, mo, mt,
			  PruneHandler::getPruneBuffer(),
			  fop, bop);


  saveVolumeIntoProject(flnm, dtvfile);
  m_Lowres->save(flnm);
  m_preferencesWidget->save(flnm);
  m_tfManager->save(flnm);
  saveViewsAndKeyFrames(flnm);

  m_Hires->saveForDrishtiPrayog(dtvfile);

  QFileInfo f(flnm);
  Global::setPreviousDirectory(f.absolutePath());
  Global::setCurrentProjectFile(QString(flnm));

  emit showMessage("Project saved", false);
}

void
MainWindow::on_actionLoad_Project_triggered()
{
  QString flnm;
  flnm = QFileDialog::getOpenFileName(0,
				      "Load Project",
				      Global::previousDirectory(),
				      "xml Files (*.xml)",
				      0,
				      QFileDialog::DontUseNativeDialog);


  if (flnm.isEmpty())
    return;

  Global::addRecentFile(flnm);
  updateRecentFileAction();
  loadProject(flnm.toLatin1().data());
}

void
MainWindow::on_actionLoad_TFfromproject_triggered()
{
  QString flnm;
  flnm = QFileDialog::getOpenFileName(0,
				      "Load Project",
				      Global::previousDirectory(),
				      "xml Files (*.xml)",
				      0,
				      QFileDialog::DontUseNativeDialog);


  if (flnm.isEmpty())
    return;

  loadTransferFunctionsOnly(flnm.toLatin1().data());
}

void
MainWindow::on_actionImport_Keyframes_triggered()
{
  if (m_keyFrame->numberOfKeyFrames() == 0)
    {
      QMessageBox::information(0,
			       "Import KeyFrames",
			       "Need atleast one keyframe in the keyframe editor before import can take place");
      return;
    }

  QString flnm;
  flnm = QFileDialog::getOpenFileName(0,
				      "Import Keyframes",
				      Global::previousDirectory(),
				      "keyframes Files (*.keyframes)",
				      0,
				      QFileDialog::DontUseNativeDialog);


  if (flnm.isEmpty())
    return;

  m_keyFrame->import(flnm);
}

void
MainWindow::on_actionSave_Project_triggered()
{
  QString flnm;
  flnm = Global::currentProjectFile();
  
  if (flnm.isEmpty())
    flnm = QFileDialog::getSaveFileName(0,
					"Save Project",
					Global::previousDirectory(),
					"xml Files (*.xml)",
					0,
					QFileDialog::DontUseNativeDialog);


  if (flnm.isEmpty())
    return;

  if (!StaticFunctions::checkExtension(flnm, ".xml"))
    flnm += ".xml";
  
  saveProject(flnm, QString());
}
void
MainWindow::on_actionSave_InformationForDrishtiPrayog_triggered()
{
  QString flnm;
  flnm = QFileDialog::getSaveFileName(0,
				      "Save Information for Drishti-Prayog",
				      Global::previousDirectory(),
				      "DrishtiPrayog Files (*.drishtiprayog)",
				      0,
				      QFileDialog::DontUseNativeDialog);

  if (!StaticFunctions::checkExtension(flnm, ".drishtiprayog"))
    flnm += ".drishtiprayog";
      
  m_Hires->saveForDrishtiPrayog(flnm);
  
  QMessageBox::information(0, "Drishti-Prayog information saved",
			   "Drishti-Prayog information data saved to " + flnm);
}

void
MainWindow::on_actionSave_ProjectAs_triggered()
{
  QString flnm;
  flnm = QFileDialog::getSaveFileName(0,
				      "Save Project As",
				      Global::previousDirectory(),
				      "Drishti Project - xml Files (*.xml)",
				      0,
				      QFileDialog::DontUseNativeDialog);

  if (flnm.isEmpty())
    return;

  if (!StaticFunctions::checkExtension(flnm, ".xml"))
    flnm += ".xml";
      
  saveProject(flnm.toLatin1().data(), QString());
}

void
MainWindow::lightDirectionChanged(Vec dir)
{
  m_Hires->updateLightVector(dir);
  m_Viewer->update();
}

void
MainWindow::applyLighting(bool flag)
{
  m_Hires->applyLighting(flag);
  m_Viewer->update();
}
void
MainWindow::applyEmissive(bool flag)
{
  m_Hires->applyEmissive(flag);
  m_Viewer->update();
}
void
MainWindow::highlights(Highlights hl)
{
  m_Hires->updateHighlights(hl);
  m_Viewer->update();
}

void
MainWindow::applyShadow(bool flag)
{
  m_Hires->applyShadows(flag);
  m_Viewer->update();
}

void
MainWindow::applyBackplane(bool flag)
{
  m_Hires->applyBackplane(flag);
  m_Viewer->update();
}

void
MainWindow::lightDistanceOffset(float val)
{
  m_Hires->updateLightDistanceOffset(val);
  m_Viewer->update();
}

void
MainWindow::shadowBlur(float val)
{
  m_Hires->updateShadowBlur(val);
  m_Viewer->update();
}

void
MainWindow::shadowScale(float val)
{
  m_Hires->updateShadowScale(val);
  m_Viewer->update();
}

void
MainWindow::shadowFOV(float val)
{
  m_Hires->updateShadowFOV(val);
  m_Viewer->update();
}

void
MainWindow::shadowIntensity(float val)
{
  m_Hires->updateShadowIntensity(val);
  m_Viewer->update();
}

void
MainWindow::applyColoredShadow(bool flag)
{
  m_Hires->applyColoredShadows(flag);
  m_Viewer->update();
}

void
MainWindow::shadowColorAttenuation(float r, float g, float b)
{
  m_Hires->updateShadowColorAttenuation(r,g,b);
  m_Viewer->update();
}

void
MainWindow::backplaneShadowScale(float val)
{
  m_Hires->updateBackplaneShadowScale(val);
  m_Viewer->update();
}

void
MainWindow::backplaneIntensity(float val)
{
  m_Hires->updateBackplaneIntensity(val);
  m_Viewer->update();
}

void
MainWindow::peel(bool flag)
{
  m_Hires->peel(flag);
  m_Viewer->update();
}

void
MainWindow::peelInfo(int etype, float emin, float emax, float emix)
{
  m_Hires->peelInfo(etype, emin, emax, emix);
  m_Viewer->update();
}

void
MainWindow::updateVolInfo(int vnum)
{
  Global::setVolumeNumber(vnum);
  emit refreshVolInfo(vnum, m_Volume->volInfo(vnum));

  VolumeInformation volInfo = VolumeInformation::volumeInformation(vnum);
  QPolygonF fmap = volInfo.mapping;
  m_tfEditor->setMapping(fmap); 
}

void
MainWindow::updateVolInfo(int vol, int vnum)
{
  Global::setVolumeNumber(vnum, vol);
  emit refreshVolInfo(vol, vnum, m_Volume->volInfo(vnum, vol));

  VolumeInformation volInfo = VolumeInformation::volumeInformation(vnum);
  QPolygonF fmap = volInfo.mapping;
  m_tfEditor->setMapping(fmap);
}

void
MainWindow::setRepeatType(int volnum, bool rtype)
{
  m_Volume->setRepeatType(volnum, rtype);
  if (m_Hires->raised())
    {
      m_Hires->updateSubvolume();
      m_Viewer->update();
    }
}

void
MainWindow::setVolumeNumber(int vnum)
{
  Global::setVolumeNumber(vnum);
  if (m_Hires->raised())
    m_Hires->updateSubvolume();
  m_Viewer->update();
  updateVolInfo(vnum);
}

void
MainWindow::setVolumeNumber(int vol, int vnum)
{
  Global::setVolumeNumber(vnum, vol);
  if (m_Hires->raised())
    m_Hires->updateSubvolume();
  m_Viewer->update();
  updateVolInfo(vol, vnum);
}

void
MainWindow::updateScaling()
{
  LightHandler::giLights()->updateScaling();

  GeometryObjects::paths()->updateScaling();
  GeometryObjects::grids()->updateScaling();
  GeometryObjects::pathgroups()->updateScaling();
  m_Lowres->updateScaling();
  m_Hires->updateScaling();
  m_Viewer->updateScaling();
}

void
MainWindow::addRotationAnimation(int axis, float angle, int frames)
{
  m_saveRotationAnimation = 1;
  m_sraAxis = axis;
  m_sraAngle = angle;
  m_sraFrames = frames;

  m_bricksWidget->setBrickZeroRotation(axis, 0);
  m_keyFrameEditor->setKeyFrame();
}

void
MainWindow::moveToKeyframe(int frm)
{
  m_keyFrameEditor->moveTo(frm);
}

void
MainWindow::setKeyFrame(Vec pos, Quaternion rot,
			int fno, float focus, float es,
			unsigned char *lut,
			QImage image)
{
  QList<SplineInformation> splineInfo;
  for(int i=0; i<m_tfContainer->count(); i++)
    {
      SplineTransferFunction *sptr = m_tfContainer->transferFunctionPtr(i);
      splineInfo.append(sptr->getSpline());
    }

  Vec bmin, bmax;
  m_Lowres->subvolumeBounds(bmin, bmax);

  int sz, st;
  QString xl, yl, zl;
  m_preferencesWidget->getTick(sz, st, xl, yl, zl);

  int mixvol;
  bool mixColor, mixOpacity;
  bool mixTag;
  m_Hires->getMix(mixvol, mixColor, mixOpacity, mixTag);

  float fop, bop;
  m_Hires->getOpMod(fop, bop);

  m_keyFrame->setKeyFrame(pos, rot,
			  focus, es,
			  fno,
			  lut,
			  m_Hires->lightInfo(),
			  m_Hires->bricks(),
			  bmin, bmax,
			  image,
			  splineInfo,
			  sz, st, xl, yl, zl,
			  mixvol, mixColor, mixOpacity, mixTag,
			  fop, bop);

  if (m_savePathAnimation > 0)
    {
      m_savePathAnimation ++;
      if (m_savePathAnimation <= m_pathAnimationPoints.count())
	{
	  int currentFrame = m_keyFrameEditor->currentFrame();
	  m_keyFrameEditor->moveTo(currentFrame+10);
	  m_Viewer->camera()->setPosition(m_pathAnimationPoints[m_savePathAnimation-1]);
	  m_Viewer->camera()->setViewDirection(m_pathAnimationVd[m_savePathAnimation-1]);
	  m_Viewer->camera()->setUpVector(m_pathAnimationUp[m_savePathAnimation-1]);	  
	  m_keyFrameEditor->setKeyFrame();	  
	}
    }
  else if (m_saveRotationAnimation == 1)
    {
      m_saveRotationAnimation = 2;
      int currentFrame = m_keyFrameEditor->currentFrame();
      m_keyFrameEditor->moveTo(currentFrame+m_sraFrames);
      m_bricksWidget->setBrickZeroRotation(m_sraAxis, m_sraAngle);
      m_keyFrameEditor->setKeyFrame();
    }
  else if (m_saveRotationAnimation == 2)
    {
      m_saveRotationAnimation = 0;
      QMessageBox::information(0, "Save Rotation Animation",
	       "Rotation animation frames created in Keyframe Editor."); 
    }
}

void
MainWindow::setView(Vec pos, Quaternion rot,
		    QImage image, float focus)
{
  QList<SplineInformation> splineInfo;
  for(int i=0; i<m_tfContainer->count(); i++)
    {
      SplineTransferFunction *sptr = m_tfContainer->transferFunctionPtr(i);
      splineInfo.append(sptr->getSpline());
    }

  Vec bmin, bmax;
  m_Lowres->subvolumeBounds(bmin, bmax);

  int sz, st;
  QString xl, yl, zl;
  m_preferencesWidget->getTick(sz, st, xl, yl, zl);

  emit addView(Global::stepsizeStill(),
	       Global::stepsizeDrag(),
	       Global::drawBox(), Global::drawAxis(),
	       Global::backgroundColor(),
	       pos, rot,
	       focus,
	       image,
	       m_Hires->renderQuality(),
	       m_Hires->lightInfo(),
	       m_Hires->bricks(),
	       bmin, bmax,
	       splineInfo,
	       sz, st, xl, yl, zl);
}

void
MainWindow::updateTransferFunctionManager(QList<SplineInformation> splineInfo)
{
  m_tfManager->load(splineInfo);
}
void MainWindow::updateMorph(bool flag) { m_tfManager->updateMorph(flag); }

void
MainWindow::updateFocus(float focusDistance, float es)
{
  m_preferencesWidget->updateStereoSettings(focusDistance, es,
					    m_Viewer->camera()->physicalScreenWidth());
}

// called from viewseditor
void
MainWindow::updateParameters(float stepStill, float stepDrag,
			     int renderQuality,
			     bool drawBox, bool drawAxis,
			     Vec bgColor,
			     QString bgImage,
			     int sz, int st,
			     QString xl, QString yl, QString zl)
{
  Global::setStepsizeStill(stepStill);
  Global::setStepsizeDrag(stepDrag);
  m_preferencesWidget->setRenderQualityValues(stepStill, stepDrag);

  m_preferencesWidget->setTick(sz, st, xl, yl, zl);

  Global::setBackgroundColor(bgColor);

  //----------------
  // bgimage file is assumed to be relative to .pvl.nc file
  // get the absolute path
  VolumeInformation pvlInfo = VolumeInformation::volumeInformation();
  QFileInfo fileInfo(pvlInfo.pvlFile);
  Global::setBackgroundImageFile(bgImage, fileInfo.absolutePath());
  //----------------

  Global::setDrawBox(drawBox);
  Global::setDrawAxis(drawAxis);
  m_Hires->setRenderQuality(renderQuality);

  ui.actionAxes->setChecked(Global::drawAxis());
  ui.actionBoundingBox->setChecked(Global::drawBox());

  // remove all geometry from mousegrab pool
  GeometryObjects::removeFromMouseGrabberPool();
  LightHandler::removeFromMouseGrabberPool();

  // always keep image captions in mouse grabber pool
  GeometryObjects::imageCaptions()->addInMouseGrabberPool();
}

// called from keyframeeditor
void
MainWindow::updateParameters(bool drawBox, bool drawAxis,
			     Vec bgColor,
			     QString bgImage,
			     int sz, int st,
			     QString xl, QString yl, QString zl,
			     int mv, bool mc, bool mo, float iv, bool mt,
			     bool pruneblend,
			     float fop, float bop)
{
  m_preferencesWidget->setTick(sz, st, xl, yl, zl);
  Global::setBackgroundColor(bgColor);

  //----------------
  // bgimage file is assumed to be relative to .pvl.nc file
  // get the absolute path
  VolumeInformation pvlInfo = VolumeInformation::volumeInformation();
  QFileInfo fileInfo(pvlInfo.pvlFile);
  Global::setBackgroundImageFile(bgImage, fileInfo.absolutePath());
  //----------------

  Global::setDrawBox(drawBox);
  Global::setDrawAxis(drawAxis);

  m_Hires->setMix(mv, mc, mo, iv);
  m_Hires->setMixTag(mt);

  m_Hires->setOpMod(fop, bop);

  ui.actionAxes->setChecked(Global::drawAxis());
  ui.actionBoundingBox->setChecked(Global::drawBox());

  // remove all geometry from mousegrab pool
  GeometryObjects::removeFromMouseGrabberPool();
  LightHandler::removeFromMouseGrabberPool();

  if (pruneblend != PruneHandler::blend())
    {
      PruneHandler::setBlend(pruneblend);
      m_Hires->createDefaultShader();
    }

  // always keep image captions in mouse grabber pool
  GeometryObjects::imageCaptions()->addInMouseGrabberPool();
}

void
MainWindow::loadViewsAndKeyFrames(const char* flnm)
{
  QString sflnm(flnm);
  sflnm.replace(QString(".xml"), QString(".keyframes"));

  QFileInfo fileInfo(sflnm);
  if (! fileInfo.exists())
    return;

  fstream fin(sflnm.toLatin1().data(), ios::binary|ios::in);

  char keyword[100];
  fin.getline(keyword, 100, 0);
  if (strcmp(keyword, "Drishti Keyframes") != 0)
    {
      QMessageBox::information(0, "Load Keyframes",
			       QString("Invalid .keyframes file : ")+sflnm);
      return;
    }

  while (!fin.eof())
    {
      fin.getline(keyword, 100, 0);
      if (strcmp(keyword, "views") == 0)
	m_gallery->load(fin);
      else if (strcmp(keyword, "keyframes") == 0)
	m_keyFrame->load(fin);
    }
  fin.close();
}

void
MainWindow::saveViewsAndKeyFrames(const char* flnm)
{
  QString sflnm(flnm);
  if (sflnm.contains(".dpxml", Qt::CaseInsensitive))
    sflnm.replace(QString(".dpxml"), QString(".keyframes"));
  else
    sflnm.replace(QString(".xml"), QString(".keyframes"));

  fstream fout(sflnm.toLatin1().data(), ios::binary|ios::out);

  QString keyword;
  keyword = "Drishti Keyframes";
  fout.write((char*)(keyword.toLatin1().data()), keyword.length()+1);  

  m_gallery->save(fout);
  m_keyFrame->save(fout);

  fout.close();
}

void
MainWindow::saveVolumeIntoProject(const char *flnm, QString dtvfile)
{
  QString str;

  QDomDocument doc("Drishti_v1.0");

  QDomElement topElement = doc.createElement("Drishti");
  doc.appendChild(topElement);

  {
    QDomElement de0 = doc.createElement("texsizereducefraction");
    QDomText tn0;
    tn0 = doc.createTextNode(QString("%1").arg(Global::texSizeReduceFraction()));
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }

  {
    QDomElement de0 = doc.createElement("floatprecision");
    QDomText tn0;
    tn0 = doc.createTextNode(QString("%1").arg(Global::floatPrecision()));
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }

  {
    QDomElement de0 = doc.createElement("fieldofview");
    QDomText tn0;
    tn0 = doc.createTextNode(QString("%1").arg(m_Viewer->camera()->fieldOfView()));
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }

  {
    QDomElement de0 = doc.createElement("drawbox");
    QDomText tn0;
    if (Global::drawBox())
      tn0 = doc.createTextNode("true");
    else
      tn0 = doc.createTextNode("false");
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }

  {
    QDomElement de0 = doc.createElement("drawaxis");
    QDomText tn0;
    if (Global::drawAxis())
      tn0 = doc.createTextNode("true");
    else
      tn0 = doc.createTextNode("false");
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }

  {
    QDomElement de0 = doc.createElement("using1d");
    QDomText tn0;
    if (Global::use1D())
      tn0 = doc.createTextNode("true");
    else
      tn0 = doc.createTextNode("false");
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }

  {
    QDomElement de0 = doc.createElement("volrepeattype");
    QList<bool> rt = m_volInfoWidget->repeatType();

    QString rtstr;
    for(int i=0; i<rt.count(); i++)
      rtstr += (rt[i] ? "true " : "false ");

    QDomText tn0;
    tn0 = doc.createTextNode(rtstr);
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }

  {
    QDomElement de0 = doc.createElement("volumetype");
    QDomText tn0;
    if (Global::volumeType() == Global::SingleVolume)
      tn0 = doc.createTextNode("single");
    else if (Global::volumeType() == Global::DoubleVolume)
      tn0 = doc.createTextNode("double");
    else if (Global::volumeType() == Global::TripleVolume)
      tn0 = doc.createTextNode("triple");
    else if (Global::volumeType() == Global::QuadVolume)
      tn0 = doc.createTextNode("quad");
    else if (Global::volumeType() == Global::RGBVolume)
      tn0 = doc.createTextNode("rgb");
    else if (Global::volumeType() == Global::RGBAVolume)
      tn0 = doc.createTextNode("rgba");
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }

  {
    // for saving volume file names with relative path
    QFileInfo fileInfo(flnm);
    QDir direc = fileInfo.absoluteDir();

    QDomElement dev = doc.createElement("volumefiles");
    if (dtvfile.isEmpty())
      {
	QList<QString> files = m_Volume->volumeFiles();
	for(int i=0; i<files.count(); i++)
	  {
	    QDomElement de0 = doc.createElement("name");
	    // saving volume filenames relative to the project file path
	    QString relFile = direc.relativeFilePath(files[i]);
	    QDomText tn0 = doc.createTextNode(relFile);
	    de0.appendChild(tn0);
	    dev.appendChild(de0);
	  }
      }
    else
      {
	QDomElement de0 = doc.createElement("name");
	// saving volume filenames relative to the project file path
	QString relFile = direc.relativeFilePath(dtvfile);
	QDomText tn0 = doc.createTextNode(relFile);
	de0.appendChild(tn0);
	dev.appendChild(de0);
      }
    topElement.appendChild(dev);

    if (Global::volumeType() == Global::DoubleVolume ||
	Global::volumeType() == Global::TripleVolume ||
	Global::volumeType() == Global::QuadVolume)
      {
	QDomElement dev = doc.createElement("volumefiles2");
	QList<QString> files = m_Volume->volumeFiles(1);
	for(int i=0; i<files.count(); i++)
	  {
	    QDomElement de0 = doc.createElement("name");
	    // saving volume filenames relative to the project file path
	    QString relFile = direc.relativeFilePath(files[i]);
	    QDomText tn0 = doc.createTextNode(relFile);
	    de0.appendChild(tn0);
	    dev.appendChild(de0);
	  }
	topElement.appendChild(dev);
      }

    if (Global::volumeType() == Global::TripleVolume ||
	Global::volumeType() == Global::QuadVolume)
      {
	QDomElement dev = doc.createElement("volumefiles3");
	QList<QString> files = m_Volume->volumeFiles(2);
	for(int i=0; i<files.count(); i++)
	  {
	    QDomElement de0 = doc.createElement("name");
	    // saving volume filenames relative to the project file path
	    QString relFile = direc.relativeFilePath(files[i]);
	    QDomText tn0 = doc.createTextNode(relFile);
	    de0.appendChild(tn0);
	    dev.appendChild(de0);
	  }
	topElement.appendChild(dev);
      }

    if (Global::volumeType() == Global::QuadVolume)
      {
	QDomElement dev = doc.createElement("volumefiles4");
	QList<QString> files = m_Volume->volumeFiles(3);
	for(int i=0; i<files.count(); i++)
	  {
	    QDomElement de0 = doc.createElement("name");
	    // saving volume filenames relative to the project file path
	    QString relFile = direc.relativeFilePath(files[i]);
	    QDomText tn0 = doc.createTextNode(relFile);
	    de0.appendChild(tn0);
	    dev.appendChild(de0);
	  }
	topElement.appendChild(dev);
      }
  }

  QFile f(flnm);
  if (f.open(QIODevice::WriteOnly))
    {
      QTextStream out(&f);
      doc.save(out, 2);
      f.close();
    }
}

int
MainWindow::loadVolumeFromProject(const char *flnm)
{
  int volType = Global::DummyVolume;

  m_volFiles1.clear();
  m_volFiles2.clear();
  m_volFiles3.clear();
  m_volFiles4.clear();

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
      if (dlist.at(i).nodeName() == "texsizereducefraction")
	{
	  QString str = dlist.at(i).toElement().text();
	  float rlod = str.toFloat();
	  Global::setTexSizeReduceFraction(rlod);
	}
      else if (dlist.at(i).nodeName() == "floatprecision")
	{
	  QString str = dlist.at(i).toElement().text();
	  Global::setFloatPrecision(str.toInt());
	}
      else if (dlist.at(i).nodeName() == "fieldofview")
	{
	  QString str = dlist.at(i).toElement().text();
	  m_Viewer->setFieldOfView(str.toFloat());
	}
      else if (dlist.at(i).nodeName() == "drawbox")
	{
	  QString str = dlist.at(i).toElement().text();
	  if (str == "true")
	    Global::setDrawBox(true);
	  else
	    Global::setDrawBox(false);
	}
      else if (dlist.at(i).nodeName() == "drawaxis")
	{
	  QString str = dlist.at(i).toElement().text();
	  if (str == "true")
	    Global::setDrawAxis(true);
	  else
	    Global::setDrawAxis(false);
	}
      else if (dlist.at(i).nodeName() == "using1d")
	{
	  QString str = dlist.at(i).toElement().text();
	  if (str == "true")
	    Global::setUse1D(true);
	  else
	    Global::setUse1D(false);
	}
      else if (dlist.at(i).nodeName() == "volrepeattype")
	{
	  QString str = dlist.at(i).toElement().text();
	  QStringList words = str.split(" ", QString::SkipEmptyParts);
	  QList<bool> rt;
	  for(int i=0; i<words.count(); i++)
	    {
	      if (words[i] == "true")
		rt << true;
	      else
		rt << false;
	    }
	  m_volInfoWidget->setRepeatType(rt);
	}
      else if (dlist.at(i).nodeName() == "volumetype")
	{
	  QString str = dlist.at(i).toElement().text();
	  if (str == "single")
	    volType = Global::SingleVolume;
	  else if (str == "double")
	    volType = Global::DoubleVolume;
	  else if (str == "triple")
	    volType = Global::TripleVolume;
	  else if (str == "quad")
	    volType = Global::QuadVolume;
	  else if (str == "rgb")
	    volType = Global::RGBVolume;
	  else if (str == "rgba")
	    volType = Global::RGBAVolume;
	  else
	    {
	      showMessage(QString("VolumeType : %1 ??").arg(str), true);
	      return volType;
	    }
	}
      else if (dlist.at(i).nodeName() == "volumefiles")
	{
	  // for loading volume file names with relative path
	  QFileInfo fileInfo(flnm);
	  QDir direc = fileInfo.absoluteDir();

	  QDomNodeList vlist = dlist.at(i).childNodes();
	  for(int vi=0; vi<vlist.count(); vi++)
	    {
	      QString str = vlist.at(vi).toElement().text();
	      QString vfile = direc.absoluteFilePath(str);
	      m_volFiles1.append(vfile);
	    }
	}
      else if (dlist.at(i).nodeName() == "volumefiles2")
	{
	  // for loading volume file names with relative path
	  QFileInfo fileInfo(flnm);
	  QDir direc = fileInfo.absoluteDir();

	  QDomNodeList vlist = dlist.at(i).childNodes();
	  for(int vi=0; vi<vlist.count(); vi++)
	    {
	      QString str = vlist.at(vi).toElement().text();
	      QString vfile = direc.absoluteFilePath(str);
	      m_volFiles2.append(vfile);
	    }
	}
      else if (dlist.at(i).nodeName() == "volumefiles3")
	{
	  // for loading volume file names with relative path
	  QFileInfo fileInfo(flnm);
	  QDir direc = fileInfo.absoluteDir();

	  QDomNodeList vlist = dlist.at(i).childNodes();
	  for(int vi=0; vi<vlist.count(); vi++)
	    {
	      QString str = vlist.at(vi).toElement().text();
	      QString vfile = direc.absoluteFilePath(str);
	      m_volFiles3.append(vfile);
	    }
	}
      else if (dlist.at(i).nodeName() == "volumefiles4")
	{
	  // for loading volume file names with relative path
	  QFileInfo fileInfo(flnm);
	  QDir direc = fileInfo.absoluteDir();

	  QDomNodeList vlist = dlist.at(i).childNodes();
	  for(int vi=0; vi<vlist.count(); vi++)
	    {
	      QString str = vlist.at(vi).toElement().text();
	      QString vfile = direc.absoluteFilePath(str);
	      m_volFiles4.append(vfile);
	    }
	}
    }

//  // for 3 or 4 volumes always use 1D transfer functions
//  if (Global::volumeType() == Global::TripleVolume ||
//      Global::volumeType() == Global::QuadVolume)
//    Global::setUse1D(true);
//
//  ui.actionSwitch_To1D->setChecked(Global::use1D());

  return volType;
}

void
MainWindow::changeHistogram(int volnum)
{
  if (Global::volumeType() == Global::DummyVolume)
    return;

  m_Lowres->setCurrentVolume(volnum);
  m_Hires->setCurrentVolume(volnum);

  m_Lowres->generateHistogramImage();
  m_Hires->generateHistogramImage();

  QList<QString> files = m_Volume->volumeFiles(volnum);
  int vfn = Global::volumeNumber(volnum);
  vfn = m_Volume->timestepNumber(volnum, vfn);

  VolumeInformation volInfo;

  if (Global::volumeType() != Global::RGBVolume &&
      Global::volumeType() != Global::RGBAVolume)
    VolumeInformation::volInfo(files[vfn], volInfo);

  QPolygonF fmap = volInfo.mapping;
  m_tfEditor->setMapping(fmap);

  if (m_Lowres->raised())
    {
      if (m_Volume->pvlVoxelType(0) == 0)
	emit histogramUpdated(m_Lowres->histogramImage1D(),
			      m_Lowres->histogramImage2D());
      else
	emit histogramUpdated(m_Lowres->histogram2D());
    }
  else
    {
      if (m_Volume->pvlVoxelType(0) == 0)
	emit histogramUpdated(m_Hires->histogramImage1D(),
			      m_Hires->histogramImage2D());
      else
	emit histogramUpdated(m_Hires->histogram2D());
    }

}

void
MainWindow::resetFlipImage()
{
  Global::setFlipImageX(false);
  Global::setFlipImageY(false);
  Global::setFlipImageZ(false);
  ui.actionFlip_ImageX->setChecked(Global::flipImageX());
  ui.actionFlip_ImageY->setChecked(Global::flipImageY());
  ui.actionFlip_ImageZ->setChecked(Global::flipImageZ());
  m_Hires->collectBrickInformation(true);
}
void
MainWindow::on_actionFlip_ImageX_triggered()
{
  bool fi = Global::flipImageX();
  Global::setFlipImageX(!fi);
  ui.actionFlip_ImageX->setChecked(Global::flipImageX());
  m_Hires->collectBrickInformation(true);
  m_Viewer->updateGL();
}
void
MainWindow::on_actionFlip_ImageY_triggered()
{
  bool fi = Global::flipImageY();
  Global::setFlipImageY(!fi);
  ui.actionFlip_ImageY->setChecked(Global::flipImageY());
  m_Hires->collectBrickInformation(true);
  m_Viewer->updateGL();
}
void
MainWindow::on_actionFlip_ImageZ_triggered()
{
  bool fi = Global::flipImageZ();
  Global::setFlipImageZ(!fi);
  ui.actionFlip_ImageZ->setChecked(Global::flipImageZ());
  m_Hires->collectBrickInformation(true);
  m_Viewer->updateGL();
}

void
MainWindow::on_actionEnable_Mask_triggered()
{
  if (Global::volumeType() != Global::RGBVolume &&
      Global::volumeType() != Global::RGBAVolume)
    {
      bool hr = m_Hires->raised();
      if (hr) m_Viewer->switchDrawVolume();

      Global::setUseMask(!Global::useMask());

      m_Hires->createShaders();

      if (hr) m_Viewer->switchDrawVolume();
    }
  else
    {
      Global::setUseMask(false);
      QMessageBox::information(0,
			       "Error",
			       "Use of masks allowed only for SingleVolumes");
    }

  ui.actionEnable_Mask->setChecked(Global::useMask());
}

void
MainWindow::on_actionSwitch_To1D_triggered()
{
  Global::setUse1D(!Global::use1D());
  ui.actionSwitch_To1D->setChecked(Global::use1D());
  m_tfContainer->switch1D();

  if (m_Volume->valid())
    {
      m_Lowres->createShaders();
      m_Hires->createShaders();
    }

  if (Global::use1D())
    emit showMessage("Switching to 1D Transfer Functions", false);
  else
    emit showMessage("Switching to 2D Transfer Functions", false);
}

void
MainWindow::on_actionAxes_triggered()
{
  Global::setDrawAxis(ui.actionAxes->isChecked());
  m_Viewer->update();
}
void
MainWindow::switchAxis()
{
  Global::setDrawAxis(!Global::drawAxis());
  ui.actionAxes->setChecked(Global::drawAxis());
}

void
MainWindow::searchCaption(QStringList str)
{
  int fno = m_keyFrame->searchCaption(str);
  if (fno >= 0)
    m_keyFrameEditor->moveTo(fno);
  else
    QMessageBox::information(0, "Not Found",
			     QString("Search term [%1] not found").arg(str.join(" ")));
}

void
MainWindow::on_actionBoundingBox_triggered()
{
  Global::setDrawBox(ui.actionBoundingBox->isChecked());
  m_Viewer->update();
}
void
MainWindow::switchBB()
{
  Global::setDrawBox(!Global::drawBox());
  ui.actionBoundingBox->setChecked(Global::drawBox());
}


void
MainWindow::quitDrishti()
{
  close();
}

void
MainWindow::mopClip(Vec pos, Vec normal)
{
  Vec dmin = m_Hires->volumeMin();
  PruneHandler::clip(pos, normal, dmin);
}

void
MainWindow::mopCrop(int cidx)
{
  QString shaderString;
  QList<CropObject> crops;
  crops << (GeometryObjects::crops()->crops())[cidx];
  shaderString = CropShaderFactory::generateCropping(crops);
  Vec dmin = m_Hires->volumeMin();
  PruneHandler::crop(shaderString, dmin);
}

void
MainWindow::addImageCaption(Vec pt)
{
  GeometryObjects::imageCaptions()->add(pt);
}


void
MainWindow::sculpt(int docarve, QList<Vec> ppos, float rad, float decay, int tag)
{
  Vec dmin = m_Hires->volumeMin();
  Vec voxelScaling = Global::voxelScaling();
  QList<Vec> pos;
  for (int i=0; i<ppos.count(); i++)
    {
      Vec pt = VECDIVIDE(ppos[i], voxelScaling);
      pos << pt;
    }
  PruneHandler::sculpt(docarve, dmin, pos, rad, decay, tag);
}

void
MainWindow::fillPathPatch(QList<Vec> ppos, int thick, int val)
{
  Vec dmin = m_Hires->volumeMin();
  Vec voxelScaling = Global::voxelScaling();
  QList<Vec> pos;
  for (int i=0; i<ppos.count(); i++)
    {
      Vec pt = VECDIVIDE(ppos[i], voxelScaling);
      pos << pt;
    }
  PruneHandler::fillPathPatch(dmin, pos, thick, val, false);
}

void
MainWindow::paintPathPatch(QList<Vec> ppos, int thick, int tag)
{
  Vec dmin = m_Hires->volumeMin();
  Vec voxelScaling = Global::voxelScaling();
  QList<Vec> pos;
  for (int i=0; i<ppos.count(); i++)
    {
      Vec pt = VECDIVIDE(ppos[i], voxelScaling);
      pos << pt;
    }
  PruneHandler::fillPathPatch(dmin, pos, thick, tag, true);
}

void
MainWindow::addToCameraPath(QList<Vec> points,
			    QList<Vec> tang,
			    QList<Vec> saxis,
			    QList<Vec> taxis)
{
  m_savePathAnimation = 1;
  m_pathAnimationVd.clear();
  m_pathAnimationUp.clear();
  m_pathAnimationPoints = points;
  m_pathAnimationTang = tang;
  m_pathAnimationSaxis = saxis;
  m_pathAnimationTaxis = taxis;
  m_pathAnimationPrevSaxis = Vec(1,0,0);
  m_pathAnimationPrevTaxis = Vec(0,1,0);
  m_pathAnimationPrevTang = Vec(0,0,1);

  m_Viewer->camera()->setPosition(m_pathAnimationPoints[m_savePathAnimation-1]);

  int vd = 0;
  {
    bool ok;
    QStringList texlist;
    texlist << "path direction";
    texlist << "red axis";
    texlist << "-ve red axis";
    texlist << "green axis";
    texlist << "-ve green axis";
    QString texstr = QInputDialog::getItem(0,
					   "View Direction",
					   "Viewing Direction",
					   texlist, 0, false,
					   &ok);
    if (ok && !texstr.isEmpty())
      vd = texlist.indexOf(texstr);
  }

  int up = 0;
  {
    bool ok;
    QStringList texlist;
    if (vd == 0)
      {
	texlist << "red axis";
	texlist << "-ve red axis";
	texlist << "green axis";
	texlist << "-ve green axis";
      }
    else if (vd == 1 || vd == 2)
      {
	texlist << "green axis";
	texlist << "-ve green axis";
	texlist << "path direction";
	texlist << "-ve path direction";
      }
    else if (vd == 3 || vd == 4)
      {
	texlist << "red axis";
	texlist << "-ve red axis";
	texlist << "path direction";
	texlist << "-ve path direction";
      }
    QString texstr = QInputDialog::getItem(0,
					   "Up Direction",
					   "Upward Direction",
					   texlist, 0, false,
					   &ok);
    if (ok && !texstr.isEmpty())
      up = texlist.indexOf(texstr);
  }

  if (vd == 0)
    {
      m_pathAnimationVd = m_pathAnimationTang;      
      if (up == 0 || up == 1)
	m_pathAnimationUp = m_pathAnimationSaxis;
      else if (up == 2 || up == 3)
	m_pathAnimationUp = m_pathAnimationTaxis;
    }
  else if (vd == 1)
    {
      m_pathAnimationVd = m_pathAnimationTaxis;
      if (up == 0 || up == 1)
	m_pathAnimationUp = m_pathAnimationSaxis;
      else if (up == 2 || up == 3)
	m_pathAnimationUp = m_pathAnimationTang;
    }
  else if (vd == 2)
    {
      m_pathAnimationVd = m_pathAnimationTaxis;
      if (up == 0 || up == 1)
	m_pathAnimationUp = m_pathAnimationSaxis;
      else if (up == 2 || up == 3)
	m_pathAnimationUp = m_pathAnimationTang;
    }
  else if (vd == 3)
    {
      m_pathAnimationVd = m_pathAnimationSaxis;
      if (up == 0 || up == 1)
	m_pathAnimationUp = m_pathAnimationTaxis;
      else if (up == 2 || up == 3)
	m_pathAnimationUp = m_pathAnimationTang;
    }
  else if (vd == 4)
    {
      m_pathAnimationVd = m_pathAnimationSaxis;
      if (up == 0 || up == 1)
	m_pathAnimationUp = m_pathAnimationTaxis;
      else if (up == 2 || up == 3)
	m_pathAnimationUp = m_pathAnimationTang;
    }

  if (vd == 1 || vd == 4)
    {
      for(int i=0; i<m_pathAnimationVd.count(); i++)
	m_pathAnimationVd[i] = -m_pathAnimationVd[i]; 
    }
  if (up == 1 || up == 3)
    {
      for(int i=0; i<m_pathAnimationVd.count(); i++)
	m_pathAnimationUp[i] = -m_pathAnimationUp[i]; 
    }

  m_Viewer->camera()->setViewDirection(m_pathAnimationVd[m_savePathAnimation-1]);
  m_Viewer->camera()->setUpVector(m_pathAnimationUp[m_savePathAnimation-1]);

  m_keyFrameEditor->setKeyFrame();
}


void
MainWindow::reorientCameraUsingClipPlane(int cp)
{
  ClipInformation clipInfo = GeometryObjects::clipplanes()->clipInfo();

  m_Viewer->camera()->setOrientation(clipInfo.rot[cp]);

  Vec cpos = clipInfo.pos[cp] -
    m_Viewer->camera()->viewDirection()*m_Viewer->sceneRadius()*2;

  m_Viewer->camera()->setPosition(cpos);
}

void
MainWindow::saveSliceImage(int cp, int step)
{
  ClipInformation clipInfo = GeometryObjects::clipplanes()->clipInfo();

  Vec pos = clipInfo.pos[cp];
  Vec tang  = clipInfo.rot[cp].rotate(Vec(0,0,1));
  Vec xaxis = clipInfo.rot[cp].rotate(Vec(1,0,0));
  Vec yaxis = clipInfo.rot[cp].rotate(Vec(0,1,0));
  float scalex = clipInfo.scale1[cp];
  float scaley = clipInfo.scale2[cp];

  if (scalex > 0 || scaley > 0)
    {
      scalex = scaley = 100;
    }
  else
    {
      scalex = qAbs(scalex);
      scaley = qAbs(scaley);
    }

  m_Volume->saveSliceImage(pos, tang, xaxis, yaxis, scalex, scaley, step);
}

void
MainWindow::saveVolume()
{
  QList<Vec> clipPos;
  QList<Vec> clipNormal;
  m_Hires->getClipForMask(clipPos, clipNormal);

  m_Volume->saveVolume(m_Viewer->lookupTable(),
		       clipPos, clipNormal,
		       GeometryObjects::crops()->crops(),
		       GeometryObjects::paths()->paths());
}
void
MainWindow::maskRawVolume()
{
  QList<Vec> clipPos;
  QList<Vec> clipNormal;
  m_Hires->getClipForMask(clipPos, clipNormal);

  if (Global::volumeType() == Global::RGBVolume ||
      Global::volumeType() == Global::RGBAVolume)
    {
      m_Volume->maskRawVolume(m_Viewer->lookupTable(),
			      clipPos, clipNormal,
			      GeometryObjects::crops()->crops(),
			      GeometryObjects::paths()->paths());
      return;
    }

  Vec bmin = m_Hires->volumeMin();
  Vec bmax = m_Hires->volumeMax();
  QBitArray bitmask = m_Volume->getBitmask(m_Viewer->lookupTable(),
					   clipPos, clipNormal,
					   GeometryObjects::crops()->crops(),
					   GeometryObjects::paths()->paths());

  RawVolume::maskRawVolume((int)bmin.x, (int)bmax.x,
			   (int)bmin.y, (int)bmax.y,
			   (int)bmin.z, (int)bmax.z,
			   bitmask);
}

void
MainWindow::countIsolatedRegions()
{
  QList<Vec> clipPos;
  QList<Vec> clipNormal;
  m_Hires->getClipForMask(clipPos, clipNormal);

  m_Volume->countIsolatedRegions(m_Viewer->lookupTable(),
				 clipPos, clipNormal,
				 GeometryObjects::crops()->crops(),
				 GeometryObjects::paths()->paths());
}

void
MainWindow::getSurfaceArea()
{
  QList<Vec> clipPos;
  QList<Vec> clipNormal;
  m_Hires->getClipForMask(clipPos, clipNormal);

  m_Volume->getSurfaceArea(m_Viewer->lookupTable(),
			   clipPos, clipNormal,
			   GeometryObjects::crops()->crops(),
			   GeometryObjects::paths()->paths());
}

void MainWindow::getSurfaceArea(unsigned char tag) { }

void
MainWindow::on_actionLinear_TextureInterpolation_triggered()
{
  bool ti = Global::interpolationType(Global::TextureInterpolation);
  Global::setInterpolationType(Global::TextureInterpolation, !ti); // nearest
  ui.actionLinear_TextureInterpolation->setChecked(Global::interpolationType(Global::TextureInterpolation));

  if (m_Volume->valid())
    m_Lowres->switchInterpolation();
}
void
MainWindow::on_actionSpline_PositionInterpolation_triggered()
{
  bool cpi = Global::interpolationType(Global::CameraPositionInterpolation);
  Global::setInterpolationType(Global::CameraPositionInterpolation, !cpi);
  ui.actionSpline_PositionInterpolation->setChecked(Global::interpolationType(Global::CameraPositionInterpolation));
}

void
MainWindow::extractPath(int pathIdx, bool fullThickness, int subsample, int tagvalue)
{
  m_Hires->resliceUsingPath(pathIdx, fullThickness, subsample, tagvalue);
}

void
MainWindow::extractClip(int clipIdx, int subsample, int tagvalue)
{
  ClipInformation clipInfo = GeometryObjects::clipplanes()->clipInfo();

  Vec cpos = clipInfo.pos[clipIdx];
  Quaternion rot = clipInfo.rot[clipIdx];
  int thickness = clipInfo.thickness[clipIdx];
  QVector4D vp = clipInfo.viewport[clipIdx];
  float viewportScale = clipInfo.viewportScale[clipIdx];
  int tfSet = clipInfo.tfSet[clipIdx];

  m_Hires->resliceUsingClipPlane(cpos, rot, thickness,
				 vp, viewportScale, tfSet,
				 subsample, tagvalue);
}

void
MainWindow::viewProfile(int segments,
			int radius,
			QList<Vec> pathPoints)
{
  VoxelizedPath voxPath = StaticFunctions::voxelizePath(pathPoints);
  QMap<QString, QList<QVariant> > valueMap;
  QList<QVariant> rawValues;
  QList<QVariant> rawMin;
  QList<QVariant> rawMax;

  valueMap = RawVolume::rawValues(radius, voxPath.voxels);

  if (valueMap["raw"].count() < 2)
    {
      //use preprocessed file instead
      valueMap = m_Volume->rawValues(radius, voxPath.voxels);
    }

  rawValues = valueMap["raw"];
  rawMin = valueMap["rawMin"];
  rawMax = valueMap["rawMax"];

  QList<float> values;
  QList<float> valMin;
  QList<float> valMax;

  for(int i=0; i<rawValues.count(); i++)
    {
      QVariant qv = rawValues[i];
      if (qv.type() == QVariant::UInt)
	values << qv.toUInt();
      else if (qv.type() == QVariant::Int)
	values << qv.toInt();
      else if (qv.type() == QVariant::Double)
	values << qv.toDouble();
      else
	values << 0;
    }


  //------
  //------
  QString pflnm;
  pflnm = QFileDialog::getSaveFileName(0,
				       "Save profile to text file ?",
				       Global::previousDirectory(),
				       "Files (*.txt)",
				       0,
				       QFileDialog::DontUseNativeDialog);
  
  
  if (! pflnm.isEmpty())
    {  
      if (!StaticFunctions::checkExtension(pflnm, ".txt"))
	pflnm += ".txt";
 
      QFile pfile(pflnm);
      if (!pfile.open(QIODevice::WriteOnly | QIODevice::Text))
	{
	  QMessageBox::information(0, "Error",
				   QString("Cannot open %1 for writing").arg(pflnm));
	}
      else
	{
	  QTextStream out(&pfile);
	  out << "Index   X    Y    Z    Value\n";
	  for(int i=0; i<values.count(); i++)
	    {
	      out << i << "     ";
	      out << voxPath.voxels[i].x << "     ";
	      out << voxPath.voxels[i].y << "     ";
	      out << voxPath.voxels[i].z << "     ";
	      out << values[i];
	      out << "\n";
	    }
	}
    }
  //------
  //------




  if (radius > 0) // we need min and max bounds
    {
      for(int i=0; i<rawMin.count(); i++)
	{
	  QVariant qv = rawMin[i];
	  if (qv.type() == QVariant::UInt)
	    valMin << qv.toUInt();
	  else if (qv.type() == QVariant::Int)
	    valMin << qv.toInt();
	  else if (qv.type() == QVariant::Double)
	    valMin << qv.toDouble();
	  else
	    valMin << 0;
	}

      for(int i=0; i<rawMax.count(); i++)
	{
	  QVariant qv = rawMax[i];
	  if (qv.type() == QVariant::UInt)
	    valMax << qv.toUInt();
	  else if (qv.type() == QVariant::Int)
	    valMax << qv.toInt();
	  else if (qv.type() == QVariant::Double)
	    valMax << qv.toDouble();
	  else
	    valMax << 0;
	}
    }

  float vmin, vmax;
  if (radius == 0) // we need min and max bounds
    {
      vmin = vmax = values[0];
      for(int i=1; i<values.count(); i++)
	{
	  vmin = qMin(vmin, values[i]);
	  vmax = qMax(vmax, values[i]);
	}
    }
  else
    {
      vmin = valMin[0];
      for(int i=1; i<valMin.count(); i++)
	vmin = qMin(vmin, valMin[i]);

      vmax = valMax[0];
      for(int i=1; i<valMax.count(); i++)
	vmax = qMax(vmax, valMax[i]);
    }

  QList<uint> index;
  for(int i=0; i<voxPath.index.size(); i++)
    {
      if (i%segments == 0)
	index.append(voxPath.index[i]);
    }
  index.append(voxPath.index[voxPath.index.size()-1]);

  ProfileViewer *profileViewer = new ProfileViewer();
  if (radius == 0)
    profileViewer->setGraphValues(vmin, vmax,
				  index,
				  values);
  else
    profileViewer->setGraphValues(vmin, vmax,
				  index,
				  values, valMin, valMax);

  profileViewer->generateScene();
  profileViewer->show();
}

void
MainWindow::viewThicknessProfile(int searchType,
				 int segments,
				 QList< QPair<Vec, Vec> > pathPoints)
{
  VoxelizedPath voxPath = StaticFunctions::voxelizePath(pathPoints);
  QList<float> thickness;
  thickness = m_Volume->getThicknessProfile(searchType,
					    m_Viewer->lookupTable(),
					    voxPath.voxels,
					    voxPath.normals);

  QString pflnm;
  pflnm = QFileDialog::getSaveFileName(0,
				       "Save thickness profile to text file ?",
				       Global::previousDirectory(),
				       "Files (*.txt)",
				       0,
				       QFileDialog::DontUseNativeDialog);

  //------
  if (! pflnm.isEmpty())
    {  
      if (!StaticFunctions::checkExtension(pflnm, ".txt"))
	pflnm += ".txt";
  
      QFile pfile(pflnm);
      if (!pfile.open(QIODevice::WriteOnly | QIODevice::Text))
	{
	  QMessageBox::information(0, "Error",
				   QString("Cannot open %1 for writing").arg(pflnm));
	}
      else
	{
	  QTextStream out(&pfile);
	  out << "Index    Value\n";
	  for(int i=0; i<thickness.count(); i++)
	    {
	      out << i << "     ";
	      out << thickness[i];
	      out << "\n";
	    }
	}
    }
  //------

  float vmin, vmax;
  vmin = vmax = thickness[0];
  for(int i=1; i<thickness.count(); i++)
    {
      vmin = qMin(vmin, thickness[i]);
      vmax = qMax(vmax, thickness[i]);
    }

  QList<uint> index;
  for(int i=0; i<voxPath.index.size(); i++)
    {
      if (i%segments == 0)
	index.append(voxPath.index[i]);
    }
  index.append(voxPath.index[voxPath.index.size()-1]);

  ProfileViewer *profileViewer = new ProfileViewer();
  profileViewer->setGraphValues(vmin, vmax,
				index,
				thickness);

  profileViewer->generateScene();
  profileViewer->show();
}

void
MainWindow::gridStickToSurface(int gidx, int rad,
			       QList< QPair<Vec, Vec> > pn)
{
  QList<Vec> pts;
  pts = m_Volume->stickToSurface(m_Viewer->lookupTable(),
				 rad,
				 pn);
  
  if (pts.count() > 0)    
    GeometryObjects::grids()->setPoints(gidx, pts);
}

void
MainWindow::on_actionStatusBar_triggered()
{
  if (ui.actionStatusBar->isChecked())
    ui.statusBar->show();
  else
    ui.statusBar->hide();
}

void
MainWindow::on_actionShadowRender_triggered()
{
  if (ui.actionShadowRender->isChecked())
    m_Hires->setRenderQuality(Enums::RenderHighQuality);
  else
    m_Hires->setRenderQuality(Enums::RenderDefault);
}

void
MainWindow::on_actionPerspective_triggered()
{
  m_Viewer->camera()->setType(Camera::PERSPECTIVE);
  ui.actionOrthographic->setChecked(false);
}

void
MainWindow::on_actionOrthographic_triggered()
{
  m_Viewer->camera()->setType(Camera::ORTHOGRAPHIC);
  ui.actionPerspective->setChecked(false);
}

void
MainWindow::on_actionRedBlue_triggered()
{
  MainWindowUI::mainWindowUI()->actionFor3DTV->setChecked(false);
  MainWindowUI::mainWindowUI()->actionCrosseye->setChecked(false);
  MainWindowUI::mainWindowUI()->actionRedCyan->setChecked(false);

  m_Viewer->updateGL();
}

void
MainWindow::on_actionRedCyan_triggered()
{
  MainWindowUI::mainWindowUI()->actionFor3DTV->setChecked(false);
  MainWindowUI::mainWindowUI()->actionCrosseye->setChecked(false);
  MainWindowUI::mainWindowUI()->actionRedBlue->setChecked(false);

  m_Viewer->updateGL();
}

void
MainWindow::on_actionCrosseye_triggered()
{
  MainWindowUI::mainWindowUI()->actionFor3DTV->setChecked(false);
  MainWindowUI::mainWindowUI()->actionRedBlue->setChecked(false);
  MainWindowUI::mainWindowUI()->actionRedCyan->setChecked(false);

  m_Viewer->updateGL();
}

void
MainWindow::on_actionFor3DTV_triggered()
{
  MainWindowUI::mainWindowUI()->actionCrosseye->setChecked(false);
  MainWindowUI::mainWindowUI()->actionRedBlue->setChecked(false);
  MainWindowUI::mainWindowUI()->actionRedCyan->setChecked(false);

  m_Viewer->updateGL();
}

void
MainWindow::on_actionMIP_triggered()
{
  m_Viewer->updateGL();
}

void
MainWindow::on_actionVisibility_triggered()
{
  PropertyEditor propertyEditor;
  QMap<QString, QVariantList> plist;

  QVariantList vlist;

  QStringList keys;

  for(int i=0; i<LightHandler::giLights()->count(); i++)
    {
      bool flag = LightHandler::giLights()->show(i);
      QString name = QString("light %1").arg(i);
      vlist.clear();
      vlist << QVariant("checkbox");
      vlist << QVariant(flag);
      plist[name] = vlist;

      keys << name;
    }
  if (LightHandler::giLights()->count())
    keys << "gap";

  for(int i=0; i<GeometryObjects::networks()->count(); i++)
    {
      bool flag = GeometryObjects::networks()->show(i);
      QString name = QString("network %1").arg(i);
      vlist.clear();
      vlist << QVariant("checkbox");
      vlist << QVariant(flag);
      plist[name] = vlist;

      keys << name;
    }
  if (GeometryObjects::networks()->count())
    keys << "gap";

  for(int i=0; i<GeometryObjects::clipplanes()->count(); i++)
    {
      bool flag = GeometryObjects::clipplanes()->show(i);
      QString name = QString("clipplane %1").arg(i);
      vlist.clear();
      vlist << QVariant("checkbox");
      vlist << QVariant(flag);
      plist[name] = vlist;

      keys << name;
    }
  if (GeometryObjects::clipplanes()->count())
    keys << "gap";

  QList<CropObject> co = GeometryObjects::crops()->crops();
  for(int i=0; i<co.count(); i++)
    {
      bool flag = GeometryObjects::crops()->show(i);
      QString name;
      if (co[i].cropType() < CropObject::Tear_Tear)
	name = QString("crop %1").arg(i);
      else if (co[i].cropType() < CropObject::Displace_Displace)
	name = QString("dissect %1").arg(i);
      else if (co[i].cropType() < CropObject::View_Tear)
	name = QString("displace %1").arg(i);
      else if (co[i].cropType() < CropObject::Glow_Ball)
	name = QString("blend %1").arg(i);
      else
	name = QString("glow %1").arg(i);

      vlist.clear();
      vlist << QVariant("checkbox");
      vlist << QVariant(flag);
      plist[name] = vlist;

      keys << name;
    }


  propertyEditor.set("Visibility Settings", plist, keys, false);
  propertyEditor.resize(200, 200);

  QMap<QString, QPair<QVariant, bool> > vmap;

  if (propertyEditor.exec() == QDialog::Accepted)
    vmap = propertyEditor.get();
  else
    return;

  keys = vmap.keys();

  for(int ik=0; ik<keys.count(); ik++)
    {
      QPair<QVariant, bool> pair = vmap.value(keys[ik]);
      
      QStringList slist = keys[ik].split(" ");
      QString name = slist[0];
      int idx = slist[1].toInt();
      if (pair.second)
	{	  
	  if (name == "light")
	    LightHandler::giLights()->setShow(idx, pair.first.toBool());
	  else if (name == "network")
	    GeometryObjects::networks()->setShow(idx, pair.first.toBool());
	  else if (name == "clipplane")
	    GeometryObjects::clipplanes()->setShow(idx, pair.first.toBool());
	  else if (name == "crop" ||
		   name == "dissect" ||
		   name == "displace" ||
		   name == "blend" ||
		   name == "glow")
	    GeometryObjects::crops()->setShow(idx, pair.first.toBool());
	}
    }
}

void
MainWindow::on_actionMouse_Grab_triggered()
{
  PropertyEditor propertyEditor;
  QMap<QString, QVariantList> plist;

  QVariantList vlist;

  QStringList keys;

  for(int i=0; i<LightHandler::giLights()->count(); i++)
    {
      bool flag = LightHandler::giLights()->isInMouseGrabberPool(i);
      QString name = QString("light %1").arg(i);
      vlist.clear();
      vlist << QVariant("checkbox");
      vlist << QVariant(flag);
      plist[name] = vlist;

      keys << name;
    }
  if (LightHandler::giLights()->count())
    keys << "gap";

  for(int i=0; i<GeometryObjects::trisets()->count(); i++)
    {
      bool flag = GeometryObjects::trisets()->isInMouseGrabberPool(i);
      QString name = QString("triset %1").arg(i);
      vlist.clear();
      vlist << QVariant("checkbox");
      vlist << QVariant(flag);
      plist[name] = vlist;

      keys << name;
    }
  if (GeometryObjects::trisets()->count())
    keys << "gap";

  for(int i=0; i<GeometryObjects::networks()->count(); i++)
    {
      bool flag = GeometryObjects::networks()->isInMouseGrabberPool(i);
      QString name = QString("network %1").arg(i);
      vlist.clear();
      vlist << QVariant("checkbox");
      vlist << QVariant(flag);
      plist[name] = vlist;

      keys << name;
    }
  if (GeometryObjects::networks()->count())
    keys << "gap";

  for(int i=0; i<GeometryObjects::paths()->count(); i++)
    {
      bool flag = GeometryObjects::paths()->isInMouseGrabberPool(i);
      QString name = QString("path %1").arg(i);
      vlist.clear();
      vlist << QVariant("checkbox");
      vlist << QVariant(flag);
      plist[name] = vlist;

      keys << name;
    }

  if (GeometryObjects::paths()->count())
    keys << "gap";

  for(int i=0; i<GeometryObjects::pathgroups()->count(); i++)
    {
      bool flag = GeometryObjects::pathgroups()->isInMouseGrabberPool(i);
      QString name = QString("pathgroup %1").arg(i);
      vlist.clear();
      vlist << QVariant("checkbox");
      vlist << QVariant(flag);
      plist[name] = vlist;

      keys << name;
    }

  if (GeometryObjects::pathgroups()->count())
    keys << "gap";

  for(int i=0; i<GeometryObjects::clipplanes()->count(); i++)
    {
      bool flag = GeometryObjects::clipplanes()->isInMouseGrabberPool(i);
      QString name = QString("clipplane %1").arg(i);
      vlist.clear();
      vlist << QVariant("checkbox");
      vlist << QVariant(flag);
      plist[name] = vlist;

      keys << name;
    }
  if (GeometryObjects::clipplanes()->count())
    keys << "gap";


  for(int i=0; i<GeometryObjects::grids()->count(); i++)
    {
      bool flag = GeometryObjects::grids()->isInMouseGrabberPool(i);
      QString name = QString("grid %1").arg(i);
      vlist.clear();
      vlist << QVariant("checkbox");
      vlist << QVariant(flag);
      plist[name] = vlist;

      keys << name;
    }

  if (GeometryObjects::grids()->count())
    keys << "gap";


  QList<CropObject> co = GeometryObjects::crops()->crops();
  for(int i=0; i<co.count(); i++)
    {
      bool flag = GeometryObjects::crops()->isInMouseGrabberPool(i);
      QString name;
      if (co[i].cropType() < CropObject::Tear_Tear)
	name = QString("crop %1").arg(i);
      else if (co[i].cropType() < CropObject::Displace_Displace)
	name = QString("dissect %1").arg(i);
      else if (co[i].cropType() < CropObject::View_Tear)
	name = QString("displace %1").arg(i);
      else if (co[i].cropType() < CropObject::Glow_Ball)
	name = QString("blend %1").arg(i);
      else
	name = QString("glow %1").arg(i);

      vlist.clear();
      vlist << QVariant("checkbox");
      vlist << QVariant(flag);
      plist[name] = vlist;

      keys << name;
    }



  propertyEditor.set("Mouse Grabbers", plist, keys, false);
  propertyEditor.resize(200, 200);

  QMap<QString, QPair<QVariant, bool> > vmap;

  if (propertyEditor.exec() == QDialog::Accepted)
    vmap = propertyEditor.get();
  else
    return;

  keys = vmap.keys();

  for(int ik=0; ik<keys.count(); ik++)
    {
      QPair<QVariant, bool> pair = vmap.value(keys[ik]);
      
      QStringList slist = keys[ik].split(" ");
      QString name = slist[0];
      int idx = slist[1].toInt();
      if (pair.second)
	{	  
	  if (name == "light")
	    {
	      if (pair.first.toBool())
		LightHandler::giLights()->addInMouseGrabberPool(idx);
	      else
		LightHandler::giLights()->removeFromMouseGrabberPool(idx);
	    }
	  else if (name == "triset")
	    {
	      if (pair.first.toBool())
		GeometryObjects::trisets()->addInMouseGrabberPool(idx);
	      else
		GeometryObjects::trisets()->removeFromMouseGrabberPool(idx);
	    }
	  else if (name == "network")
	    {
	      if (pair.first.toBool())
		GeometryObjects::networks()->addInMouseGrabberPool(idx);
	      else
		GeometryObjects::networks()->removeFromMouseGrabberPool(idx);
	    }
	  else if (name == "path")
	    {
	      if (pair.first.toBool())
		GeometryObjects::paths()->addInMouseGrabberPool(idx);
	      else
		GeometryObjects::paths()->removeFromMouseGrabberPool(idx);
	    }
	  else if (name == "grid")
	    {
	      if (pair.first.toBool())
		GeometryObjects::grids()->addInMouseGrabberPool(idx);
	      else
		GeometryObjects::grids()->removeFromMouseGrabberPool(idx);
	    }
	  else if (name == "crop" ||
		   name == "dissect" ||
		   name == "displace" ||
		   name == "blend" ||
		   name == "glow")
	    {
	      if (pair.first.toBool())
		GeometryObjects::crops()->addInMouseGrabberPool(idx);
	      else
		GeometryObjects::crops()->removeFromMouseGrabberPool(idx);
	    }
	  else if (name == "pathgroup")
	    {
	      if (pair.first.toBool())
		GeometryObjects::pathgroups()->addInMouseGrabberPool(idx);
	      else
		GeometryObjects::pathgroups()->removeFromMouseGrabberPool(idx);
	    }
	  else if (name == "clipplane")
	    {
	      if (pair.first.toBool())
		GeometryObjects::clipplanes()->addInMouseGrabberPool(idx);
	      else
		GeometryObjects::clipplanes()->removeFromMouseGrabberPool(idx);
	    }
	}
    }
}
