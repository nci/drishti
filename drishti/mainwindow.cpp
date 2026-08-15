#include "global.h"
#include "mainwindow.h"
#include "mainwindowui.h"
#include "geometryobjects.h"
#include "lighthandler.h"
#include "dialogs.h"
#include "staticfunctions.h"
#include "rawvolume.h"
#include "../../common/src/widgets/saveimageseqdialog.h"
#include "../../common/src/widgets/savemoviedialog.h"
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
#include "projectsavejournal.h"

#include <QDockWidget>
#include <QFileDialog>
#include <QFile>
#include <QDir>
#include <QCoreApplication>
#include <QInputDialog>
#include <QSaveFile>
#include <QDateTime>
#include <QTextStream>
#include <new>

namespace
{
void appendRuntimeDiagnostic(const QString &message)
{
  const QString path = QDir(QCoreApplication::applicationDirPath())
    .filePath(QStringLiteral("drishti-runtime.log"));
  QFile log(path);
  if (log.open(QIODevice::Append | QIODevice::Text))
    {
      QTextStream stream(&log);
      stream << QDateTime::currentDateTime().toString(Qt::ISODate)
             << " " << message << "\n";
    }
  qWarning().noquote() << message;
}
}

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
  // The renderer objects are long-lived and point at the stable Volume
  // facade.  Recreating them before a candidate load would discard the old
  // scene even when the candidate later fails, so only create them when the
  // window has not been initialized yet.
  if (m_Lowres && m_Hires)
    return;

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

  qApp->setFont(QFont("MS Reference Sans Serif", 12));
  
  
  Global::setStatusBar(ui.statusBar, ui.actionStatusBar);

  ui.statusBar->setEnabled(true);
  ui.statusBar->setSizeGripEnabled(true);
  ui.statusBar->addPermanentWidget(Global::progressBar());


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
  m_rendererServicesStarted = false;
  setCentralWidget(m_Viewer);

  
  
  Global::setBatchMode(false);
  Global::setEmptySpaceSkip(true);
  Global::setImageQuality(Global::_NormalQuality);

#ifndef Q_OS_WIN32
  ui.actionSave_Movie->setEnabled(false);
#endif

  ui.actionInterruptRendering->setChecked(Global::allowInterruption());
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
  m_deferVolumeCommit = false;
  m_projectRollbackPreferencesValid = false;
  m_projectRollbackLowresValid = false;
  m_projectRollbackDockTFVisible = false;
  m_projectRollbackEmptySpaceSkipEnabled = true;
  m_projectRollbackEmptySpaceSkipChecked = true;
  m_projectRollbackValid = false;
  m_projectRollbackHires = false;
  m_projectRollbackTFEnabled = false;
  m_projectRollbackUse1D = false;
  m_projectRollbackEmptySpaceSkip = false;
  m_projectRollbackGamma = 1.0f;
  m_projectRollbackLutSize = 8;
  memset(m_projectRollbackVolumeNumbers, 0,
         sizeof(m_projectRollbackVolumeNumbers));
  m_pendingKeyFrameCandidate = 0;
  m_pendingKeyFrameValid = false;
  m_pendingLowresStateValid = false;

  m_tfContainer = new TransferFunctionContainer(this);
  m_tfManager = new TransferFunctionManager(this);
  m_tfManager->registerContainer(m_tfContainer);
  m_tfEditor = new TransferFunctionEditorWidget(this);
  m_tfEditor->setTransferFunction(NULL);    

  m_tfManager->setDisabled(true);

  m_keyFrame = new KeyFrame();
  m_Viewer->setKeyFrame(m_keyFrame);

  //----------------------------------------------------------
  m_dockTF = new QDockWidget(QWidget::tr("Transfer Function Editor"), this);
  m_dockTF->setAllowedAreas(Qt::LeftDockWidgetArea | 
			    Qt::RightDockWidgetArea);
  QSplitter *splitter = new QSplitter(Qt::Vertical, m_dockTF);
  splitter->addWidget(m_tfManager);
  splitter->addWidget(m_tfEditor);
  m_dockTF->setWidget(splitter);
  m_dockTF->hide();
  //----------------------------------------------------------

  //----------------------------------------------------------
  m_lightingWidget = new LightingWidget();
  QDockWidget *dock2 = new QDockWidget(QWidget::tr("Shader Widget"), this);
  dock2->setAllowedAreas(Qt::LeftDockWidgetArea | 
			 Qt::RightDockWidgetArea);
  dock2->setWidget(m_lightingWidget);
  dock2->hide();
  //----------------------------------------------------------

  //----------------------------------------------------------
  m_bricksWidget = new BricksWidget(NULL, m_bricks);
  QDockWidget *dock3 = new QDockWidget(QWidget::tr("Bricks Editor"), this);
  dock3->setAllowedAreas(Qt::LeftDockWidgetArea | 
			 Qt::RightDockWidgetArea);
  dock3->setWidget(m_bricksWidget);
  dock3->hide();
  //----------------------------------------------------------

  //----------------------------------------------------------
  m_volInfoWidget = new VolumeInformationWidget();
  QDockWidget *dock4 = new QDockWidget(QWidget::tr("Volume Information"), this);
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
  QDockWidget *dock5 = new QDockWidget(QWidget::tr("Preferences"), this);
  dock5->setAllowedAreas(Qt::LeftDockWidgetArea | 
			 Qt::RightDockWidgetArea);
  dock5->setWidget(m_preferencesWidget);
  dock5->hide();
  //----------------------------------------------------------

  //----------------------------------------------------------
  m_dockKeyframe = new QDockWidget(QWidget::tr("KeyFrame Editor"), this);
  m_dockKeyframe->setAllowedAreas(Qt::BottomDockWidgetArea | 
        			  Qt::TopDockWidgetArea);
  m_keyFrameEditor = new KeyFrameEditor();
  m_dockKeyframe->setWidget(m_keyFrameEditor);
  m_dockKeyframe->hide();
  //----------------------------------------------------------

  

  addDockWidget(Qt::RightDockWidgetArea, m_dockTF);
  addDockWidget(Qt::RightDockWidgetArea, dock2);
  addDockWidget(Qt::RightDockWidgetArea, dock3);
  addDockWidget(Qt::LeftDockWidgetArea, dock4);
  addDockWidget(Qt::LeftDockWidgetArea, dock5);
  addDockWidget(Qt::BottomDockWidgetArea,m_dockKeyframe);


  QString tstr = QString("Drishti v") +
                 Global::DrishtiVersion() +
                 QWidget::tr(" - Volume Exploration and Presentation Tool");
  if (QGLFormat::defaultFormat().stereo())
    tstr = QWidget::tr("(Stereo)")+tstr;

  setWindowTitle(tstr);


  ui.menuView->addAction(m_dockTF->toggleViewAction());
  ui.menuView->addAction(dock2->toggleViewAction());
  ui.menuView->addAction(dock3->toggleViewAction());
  ui.menuView->addAction(dock4->toggleViewAction());
  ui.menuView->addAction(m_dockKeyframe->toggleViewAction());
  ui.menuView->addSeparator();
  ui.menuView->addAction(dock5->toggleViewAction());

  registerMenuViewerFunctions();

  createHiresLowresWindows();

  m_Viewer->setBricksWidget(m_bricksWidget);

  
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
  #include "connectvolinfowidget.h"
  #include "connectgeometryobjects.h"


  initializeRecentFiles();

  loadSettings();

  GeometryObjects::trisets()->setHitPoints(GeometryObjects::hitpoints());
			   
  QTimer::singleShot(1000, this, SLOT(GlewInit()));
}

void
MainWindow::addDockFrame(QString name, QFrame *menu)
{
  //----------------------------------------------------------
  QDockWidget *dockW = new QDockWidget(name, this);
  dockW->setAllowedAreas(Qt::LeftDockWidgetArea | 
			 Qt::RightDockWidgetArea);
  dockW->setWidget(menu);
  dockW->hide();
  //----------------------------------------------------------
  addDockWidget(Qt::RightDockWidgetArea, dockW);

  ui.menuView->addSeparator();
  ui.menuView->addAction(dockW->toggleViewAction());

  //emit dockAdded(dockW);
}

void
MainWindow::show16BitEditor(bool b)
{
  if (b)
    {
      m_tfEditor->show16BitEditor(true);
      return;
    }
  
  if (m_Volume->pvlVoxelType(0) == 0)
    m_tfEditor->show16BitEditor(false);
  else
    m_tfEditor->show16BitEditor(true);
}

void
MainWindow::registerMenuViewerFunctions()
{
  QMap<QString, QMap<QString, MenuViewerFncPtr> > menuFnc;

  menuFnc = m_Viewer->registerMenuFunctions();

  QStringList fnames = menuFnc.keys();

  QMenu *menu=0;

  for(int i=0; i<fnames.count(); i++)
    {
      if (!fnames[i].isEmpty())
	{
	  menu = new QMenu(fnames[i]);
	  ui.menuFunctions->addMenu(menu);
	}
      else
	menu = ui.menuFunctions;

      QMap<QString, MenuViewerFncPtr> m1 = menuFnc[fnames[i]];
      QStringList fnm = m1.keys();
      for(int j=0; j<fnm.count(); j++)
	{
	  QAction *action = new QAction(this);
	  action->setText(fnm[j]);
	  action->setData(fnm[j]);
	  action->setVisible(true);      
	  connect(action, SIGNAL(triggered()),
		  this, SLOT(menuViewerFunction()));
	  menu->addAction(action);
	  
	  m_menuViewerFunctions[fnm[j]] = m1[fnm[j]];
	}

    }
}

void
MainWindow::menuViewerFunction()
{
//  if (!m_Volume->valid() ||
//      Global::volumeType() == Global::DummyVolume)
//    {
//      QMessageBox::information(0, "Error", "No volume to work on !");
//      return;
//    }

  if (!m_Hires->raised())
    {
      QMessageBox::information(0, "Error", "Functions available only in hires mode.");
      return;
    }


  QAction *action = qobject_cast<QAction *>(sender());
  QString mvf = action->data().toString();

  (m_Viewer->*m_menuViewerFunctions[mvf])();
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
  QAction *action = qobject_cast<QAction *>(sender());
  int idx = action->data().toInt();

  if (m_pluginList[idx].length() == 1 ||
      m_pluginList[idx][1] != "NoVolume")
    {
      if (!m_Volume->valid() ||
	  Global::volumeType() == Global::DummyVolume)
	{
	  QMessageBox::information(0, "Error", "No volume to work on !");
	  return;
	}
    }

  runPlugin(idx, false);
}

void MainWindow::runPlugin(int idx, bool batchMode)
{
  QPluginLoader pluginLoader(m_pluginDll[idx]);
  QObject *plugin = pluginLoader.instance();

  if (!plugin)
    {
      QMessageBox::information(0, "Error", "Cannot load plugin");
      return;
    }
  

  RenderPluginInterface *pluginInterface = qobject_cast<RenderPluginInterface *>(plugin);
  if (!pluginInterface)
    {
      QMessageBox::information(0, "Error", "Cannot load plugin interface");
      return;
    }

//  QMessageBox::information(0, "",
//			   QString("Load : %1\n %2\n %3").	\
//			   arg(idx).arg(m_pluginList[idx][0]).arg(m_pluginDll[idx]));

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
      int tms = Global::textureMemorySize()-25*Global::actualDragVolSize(); // in Mb
      subsamplinglevel = StaticFunctions::getSubsamplingLevel(tms,
							      Global::maxArrayTextureLayers(),
							      bpv,
							      dataMin, dataMax);
      QList<Vec> slabinfo = Global::getSlabs(subsamplinglevel,
					     bpv,
					     dataMin, dataMax,
					     nrows, ncols);
      
      if (slabinfo.count() > 1)
	dragTextureInfo = slabinfo[0];
      else
	dragTextureInfo = Vec(ncols, nrows, subsamplinglevel);
      
      subvolumeSize = dataMax-dataMin+Vec(1,1,1);
    }
  

  plod = qMax(1, static_cast<int>(dragTextureInfo.z));
  px = qMax(1, static_cast<int>(subvolumeSize.x)/plod);
  py = qMax(1, static_cast<int>(subvolumeSize.y)/plod);
  pz = qMax(1, static_cast<int>(subvolumeSize.z)/plod);
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

  pluginInterface->setBatchMode(batchMode);
    
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
      if (i > 0)
	{
	  r = (float)qrand()/(float)RAND_MAX;
	  g = (float)qrand()/(float)RAND_MAX;
	  b = (float)qrand()/(float)RAND_MAX;
	  a = 0.5f;
	}
      else
	{
	  r = g = b = a = 1.0;
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
MainWindow::setTextureMemory(bool bruteforce)
{
  QString homePath = QDir::homePath();
  QFileInfo settingsFile(homePath, ".drishti.xml");

  if (!bruteforce && settingsFile.exists())
    return;

  bool ok;
  QStringList texlist;
  texlist << "128 Mb";
  texlist << "256 Mb";
  texlist << "512 Mb";
  QString texstr = QInputDialog::getItem(0,
					 QWidget::tr("Texture Memory"),
					 QWidget::tr("Texture Memory Size"),
					 texlist, 2, false,
					 &ok);
  int texmem = 512;
  if (ok && !texstr.isEmpty())
    {
      QStringList lst = texstr.split(" ");
      if (lst[0] == "128") texmem = 128;
      if (lst[0] == "256") texmem = 256;
      if (lst[0] == "512") texmem = 512;
    }
  Global::setTextureMemorySize(texmem);
  Global::calculate3dTextureSize();
}


void
MainWindow::GlewInit()
{
  if (m_rendererServicesStarted)
    return;

  m_Viewer->GlewInit();

  if (!m_Viewer->rendererReady())
    return;

  m_rendererServicesStarted = true;

  registerPlugins();

  GLint textureSize;
  // query 3d texture size limits
  //glGetIntegerv(GL_MAX_TEXTURE_SIZE, &m_max2DTexSize);
  //glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &m_max3DTexSize);
  glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &textureSize);
  
  //GLint textureSize;
  //glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &textureSize);
  Global::setMaxArrayTextureLayers(textureSize);
  m_preferencesWidget->updateTextureMemory();

  loadProjectRunKeyframesAndExit();

  // load program 
  QStringList arguments = qApp->arguments();
  if (arguments.count() > 1)
    {
      QStringList positional;
      for (int ai = 1; ai < arguments.count(); ++ai)
	{
	  const QString argument = arguments.at(ai);
	  if (argument.compare("-drishti", Qt::CaseInsensitive) == 0 ||
	      argument.compare("-stereo", Qt::CaseInsensitive) == 0)
	    continue;
	  positional << argument;
	}
      if (!positional.isEmpty())
	{
	  if (StaticFunctions::checkExtension(positional[0], ".pvl.nc"))
	    {
	      QStringList flnms;
	      flnms = positional;
	      loadSingleVolume(flnms);
	    }
	  else if (StaticFunctions::checkExtension(positional[0], ".xml"))
	    {
	      Global::addRecentFile(positional[0]);
	      updateRecentFileAction();
	      createHiresLowresWindows();
	      loadProject(positional[0].toUtf8().data());
	    }
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
  while (true) {
    line = stream.readLine().trimmed();
    if (line.isNull())
      break;
    if (line.isEmpty() || line.startsWith('#'))
      continue;
    const int equals = line.indexOf('=');
    if (equals > 0)
      {
	QString t0 = line.left(equals).trimmed();
	QString t1 = line.mid(equals+1).trimmed();
	if (t1.isEmpty())
	  return false;
	if (t0[0] != '#')
	  arguments.append(t0+"="+t1);
      }
    else
      {
	QString t0 = line.trimmed();
	arguments.append(t0);
      }
  }

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
      QString arg = arguments[i].trimmed();
      if (arg.isEmpty() || arg == qApp->applicationFilePath())
	continue;
      if (arg.compare("-drishti", Qt::CaseInsensitive) == 0 ||
          arg.compare("-stereo", Qt::CaseInsensitive) == 0)
	continue;

      const int equals = arg.indexOf('=');
      const QString key = equals >= 0 ? arg.left(equals).toLower() :
                          arg.toLower();
      const QString optionKey = key.startsWith('-') ? key.mid(1) : key;
      const QString value = equals >= 0 ? arg.mid(equals+1) : QString();
      if (arg.startsWith('-') && equals < 0 &&
          optionKey != "nobackgroundrender" && optionKey != "shading" &&
          optionKey != "depthcue" && optionKey != "skipemptyspace" &&
          optionKey != "dragonlyforshadows" && optionKey != "dragonly")
	return false;
      if (equals >= 0 && value.isEmpty())
	return false;

      if (optionKey == "project")
	{
	QString projfile = value;
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
	else if (optionKey == "renderframes")
	{
	if (equals < 0) return false;
	bj.renderFrames = true;
	QStringList tokens = value.split(",");
	if (tokens.count() != 3) return false;
	bool ok0=false, ok1=false, ok2=false;
	bj.startFrame = tokens[0].toInt(&ok0);
	bj.endFrame = tokens[1].toInt(&ok1);
	bj.stepFrame = tokens[2].toInt(&ok2);
	if (!ok0 || !ok1 || !ok2) return false;
	}
	else if (optionKey == "plugin")
	{
	if (equals < 0) return false;
	bj.plugin = true;
	bj.pluginName = value.trimmed();
	if (bj.pluginName.isEmpty()) return false;
	}
	else if (optionKey == "image")
	{
	if (equals < 0) return false;
	bj.image = true;
	bj.imageFilename = value;
	}
	else if (optionKey == "movie")
	{
	if (equals < 0) return false;
	bj.movie = true;
	bj.movieFilename = value;
	}
	else if (optionKey == "framerate")
	{
	if (equals < 0) return false;
	bool ok = false;
	bj.frameRate = value.toInt(&ok);
	if (!ok || bj.frameRate <= 0) return false;
	}
	else if (optionKey == "imagemode")
	{
	if (equals < 0) return false;
	if (value=="stereo")
	  bj.imageMode = Enums::StereoImageMode;
	else if (value=="cubic")
	  bj.imageMode = Enums::CubicImageMode;
	else if (value=="pano")
	  bj.imageMode = Enums::PanoImageMode;
	else if (value=="redcyan")
	  bj.imageMode = Enums::RedCyanImageMode;
	else if (value=="redblue")
	  bj.imageMode = Enums::RedBlueImageMode;
	else if (value=="crosseye")
	  bj.imageMode = Enums::CrosseyeImageMode;
	else if (value=="3dtv")
	  bj.imageMode = Enums::ImageMode3DTV;
	else return false;
	}
	else if (optionKey == "nobackgroundrender")
	{
	bj.backgroundrender = false;
	}
	else if (optionKey == "shading")
	{
	  bj.shading = true;
	}
	else if (optionKey == "depthcue")
	{
	  bj.depthcue = true;
	}
	else if (optionKey == "skipemptyspace")
	{
	  bj.skipEmptySpace = true;
	}
	else if (optionKey == "dragonlyforshadows")
	{
	  bj.dragonlyforshadows = true;
	}
	else if (optionKey == "dragonly")
	{
	  bj.dragonly = true;
	}
	else if (optionKey == "imagesize")
	{
	if (equals < 0) return false;
	bj.imagesize = true;
	QStringList tokens = value.split(",");
	if (tokens.count() != 2) return false;
	bool ok0=false, ok1=false;
	bj.imgWidth = tokens[0].toInt(&ok0);
	bj.imgHeight = tokens[1].toInt(&ok1);
	if (!ok0 || !ok1 || bj.imgWidth <= 0 || bj.imgHeight <= 0) return false;
	}
	else if (optionKey == "stepsize")
	{
	if (equals < 0) return false;
	bool ok = false;
	bj.stepSize = value.toFloat(&ok);
	if (!ok || bj.stepSize <= 0) return false;
	}
	else if (arg.startsWith('-'))
	return false;
	else if (!StaticFunctions::checkExtension(arg, ".pvl.nc") &&
	         !StaticFunctions::checkExtension(arg, ".xml"))
	return false;
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
      const int equals = arg.indexOf('=');
      if (equals > 0 && equals+1 < arg.size())
	ok = fromFile(arg.mid(equals+1), bj);
    }
  else
    {
      if (!arguments.isEmpty())
        arguments.removeFirst();
      ok = fromStringList(arguments, bj);
    }

  if (!ok)
    {
      qApp->quit();
      return;
    }


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

      loadProject(bj.projectFilename.toUtf8().data());

      if (!bj.backgroundrender)
	m_Viewer->setUseFBO(false);

      if (bj.shading)
	m_Hires->setRenderQuality(Enums::RenderHighQuality);

      if (bj.imageMode == Enums::CubicImageMode ||
	  bj.imageMode == Enums::PanoImageMode)
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
				       bj.frameRate) == false)
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
      else if (bj.plugin)
	{
	  for(int i=0; i<m_pluginList.count(); i++)
	    {
	      if (m_pluginList[i].contains(bj.pluginName))
		{
		  m_Viewer->switchDrawVolume();

		  runPlugin(i, true);
		}
	    }
	  exit(0);
	}
      else
	{
	  m_Viewer->setCurrentFrame(-1);
	  m_Viewer->setImageFileName(bj.imageFilename);
	  m_Viewer->setSaveSnapshots(true);
	  m_Viewer->dummydraw();
	  m_Viewer->updateGL();
	  m_Viewer->endPlay();
	  exit(0);
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
      int ok = QMessageBox::question(0, QWidget::tr("Exit Drishti"),
				     QString(QWidget::tr("Would you like to save project before quitting ?")),
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

  Global::disableViewerUpdate();

  QSize imgSize = StaticFunctions::getImageSize(m_Viewer->size().width(),
						m_Viewer->size().height());

  //m_Viewer->setImageSize(imgSize.width(), imgSize.height());

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

      if (imgMode == Enums::CubicImageMode ||
	  imgMode == Enums::PanoImageMode)
	{
	  if (imgSize.width() != imgSize.height())
	    {
	      QMessageBox::critical(0, "Error",
				    QString("Current image size is %1x%2.\nSquare image size required for cubic images.").\
				    arg(imgSize.width()).\
				    arg(imgSize.height()));
	      Global::enableViewerUpdate();
	      return;
	    }
	}

      Global::enableViewerUpdate();

      QFileInfo f(flnm);
      Global::setPreviousDirectory(f.absolutePath());

      m_Viewer->setImageMode(imgMode);
      m_Viewer->setImageFileName(flnm);
      m_Viewer->setSaveSnapshots(true);
      
      m_Viewer->setImageSize(imgSize.width(), imgSize.height());

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

  Global::disableViewerUpdate();

  QSize imgSize = StaticFunctions::getImageSize(m_Viewer->size().width(),
						m_Viewer->size().height());

  m_Viewer->setImageSize(imgSize.width(), imgSize.height());

  if (imgSize.width()%2 > 0 ||
      imgSize.height()%2 > 0)
    {
      emit showMessage(QString("Image dimensions must be even numbers. Current size is %1 x %2"). \
		       arg(imgSize.width()).
		       arg(imgSize.height()), true);
      Global::enableViewerUpdate();
      return;
    }

  SaveMovieDialog saveMovDiag(0,
			      Global::previousDirectory(),
			      m_keyFrameEditor->startFrame(),
			      m_keyFrameEditor->endFrame(),
			      1);

  int cdW = saveMovDiag.width();
  int cdH = saveMovDiag.height();
  saveMovDiag.move(QCursor::pos() - QPoint(cdW/2, cdH/2));
  
  if (saveMovDiag.exec() == QDialog::Accepted)
    {
      QString flnm = saveMovDiag.fileName();
      bool movieMode = saveMovDiag.movieMode();
      QFileInfo f(flnm);
      Global::setPreviousDirectory(f.absolutePath());

      if (movieMode)
	m_Viewer->setImageMode(Enums::MonoImageMode);
      else
	m_Viewer->setImageMode(Enums::StereoImageMode);

      Global::enableViewerUpdate();

      if (m_Viewer->startMovie(flnm, saveMovDiag.frameRate()) == false)
	return;

      m_Viewer->setSaveSnapshots(false);
      m_Viewer->setSaveMovie(true);

      m_Viewer->dummydraw();

      connect(this, SIGNAL(playKeyFrames(int,int,int)),
	      m_keyFrameEditor, SLOT(playKeyFrames(int,int,int)));
      emit playKeyFrames(saveMovDiag.startFrame(),
			 saveMovDiag.endFrame(),
			 saveMovDiag.stepFrame());
      qApp->processEvents();
      disconnect(this, SIGNAL(playKeyFrames(int,int,int)),
		 m_keyFrameEditor, SLOT(playKeyFrames(int,int,int)));
    }
}

void
MainWindow::on_actionImage_Size_triggered()
{
  int woff = size().width() - m_Viewer->size().width();
  int hoff = size().height() - m_Viewer->size().height();
  
  QSize imgSize = StaticFunctions::getImageSize(m_Viewer->size().width(),
						m_Viewer->size().height());
  int w = imgSize.width() + woff;
  int h = imgSize.height() + hoff;

  showNormal();

  resize(w, h);
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
	  if (on.count() > 2)
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
	  if (on.count() > 3)
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
	  if (on.count() > 3)
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
  bool loaded = false;
  if (VolumeInformation::checkRGB(flnm[0]))
    loaded = loadVolumeRGB(flnm[0].toUtf8().data());
  else
    loaded = loadVolumeList(flnm, false);

  if (!loaded)
    return;

  Global::resetCurrentProjectFile();
  Global::addRecentFile(flnm[0]);
  updateRecentFileAction();

  // reset
  m_bricks->reset();
  m_bricksWidget->refresh();

  LightHandler::reset();
  GeometryObjects::clear();

  m_keyFrame->clear();
  m_keyFrameEditor->clear();
  m_lightingWidget->setLightInfo(LightingInformation());
}

void
MainWindow::on_actionLoad_1_Volume_triggered()
{
  QStringList flnm;
  flnm = QFileDialog::getOpenFileNames(0,
				      "Load volume files",
				       Global::previousDirectory(),
				       "Files (*.pvl.nc)",
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

  if (!loadVolume2List(vol1, vol2, false))
    return;

  // reset
  m_bricks->reset();
  m_bricksWidget->refresh();

  LightHandler::reset();
  GeometryObjects::clear();

  m_keyFrame->clear();
  m_keyFrameEditor->clear();
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

  if (!loadVolume3List(vol1, vol2, vol3, false))
    return;

  // reset
  m_bricks->reset();
  m_bricksWidget->refresh();

  LightHandler::reset();
  GeometryObjects::clear();

  m_keyFrame->clear();
  m_keyFrameEditor->clear();
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

  if (!loadVolume4List(vol1, vol2, vol3, vol4, false))
    return;

  // reset
  m_bricks->reset();
  m_bricksWidget->refresh();

  LightHandler::reset();
  GeometryObjects::clear();

  m_keyFrame->clear();
  m_keyFrameEditor->clear();
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
  if (!m_Viewer->rendererReady())
    {
      QMessageBox::information(0, "Drishti", "Renderer is not ready to load data.");
      return;
    }

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
	  loadProject(filename.toUtf8().data());
	}
    }
}

void
MainWindow::dropEvent(QDropEvent *event)
{
  if (!m_Viewer->rendererReady())
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
		  loadProject(url.toLocalFile().toUtf8().data());
		}
	      else if (StaticFunctions::checkExtension(url.toLocalFile(), ".keyframes"))
		{
		  m_keyFrame->import(url.toLocalFile());
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
      return loadDummyVolume(nx, ny, nz);
	}
    }

  return false;
}

bool
MainWindow::loadDummyVolume(int nx, int ny, int nz)
{
  Global::setSaveImageType(Global::NoImage);

  captureVolumeLoadRollback();
  if (!m_Volume->loadDummyVolume(nx, ny, nz))
    {
      recoverFromFailedVolumeLoad("Dummy volume allocation failed");
      QMessageBox::warning(0, "Error",
                           "Cannot allocate the requested dummy volume.");
      return false;
    }

  if (!postLoadVolume())
    return false;

  Global::setVolumeNumber(0);

  m_tfEditor->setTransferFunction(NULL);
  m_tfManager->setEnabled(false);
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
  m_keyFrameEditor->setHiresMode(false);

  Global::setVolumeNumber(0);

  QList<int> vsizes;
  vsizes << 1;
  emit setVolumes(vsizes);
  emit refreshVolInfo(0, m_Volume->volInfo(0));
  return true;
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

bool
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
		  return loadVolume2List(files1, files, true);
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
		  return loadVolume3List(files1, files2,
				  files, true);
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
		  return loadVolume4List(files1, files2,
				  files3, files, true);
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
	    return false;
	}
    }

  if (!loadVolume(files) || !m_Volume->valid())
    return false;

  Global::setVolumeNumber(0);

  QList<QString> volfiles;
  for(int i=0; i<files.count(); i++)
    volfiles.append(files[i].toUtf8().data());

  QList<int> vsizes;
  vsizes << volfiles.size();
  emit setVolumes(vsizes);
  emit refreshVolInfo(0, m_Volume->volInfo(0));
  return true;
}

void
MainWindow::preLoadVolume()
{
  // Volume::loadVolume now builds and validates an isolated candidate before
  // this hook is reached. Keep this phase non-destructive: the scene is
  // cleared only after postLoadVolume has completed the replacement setup.
  Global::setGamma(1.0);
}

bool
MainWindow::postLoadVolume()
{  
  if (Global::volumeType() != Global::DummyVolume)
    {
      m_dockTF->show();
      //m_dockKeyframe->toggleViewAction()->setEnabled(true);
    }

  
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

  m_preferencesWidget->setDOF(0, 0);

  m_tfEditor->setTransferFunction(NULL);
  m_tfManager->clearManager();

  m_Lowres->setCurrentVolume(0);
  m_Hires->setCurrentVolume(0);

  const bool deferSceneCommit = m_deferVolumeCommit;
  if (!m_Lowres->loadVolume(!deferSceneCommit))
    {
      appendRuntimeDiagnostic(QStringLiteral("postLoadVolume lowres failed: %1")
                              .arg(m_Lowres->lastError()));
      recoverFromFailedVolumeLoad("Low-resolution rendering resources could not be created");
      return false;
    }
  appendRuntimeDiagnostic(QStringLiteral("postLoadVolume lowres ready"));
  if (!m_Hires->loadVolume(!deferSceneCommit))
    {
      appendRuntimeDiagnostic(QStringLiteral("postLoadVolume highres failed: %1")
                              .arg(m_Hires->lastError()));
      recoverFromFailedVolumeLoad("High-resolution rendering resources could not be created");
      return false;
    }
  appendRuntimeDiagnostic(QStringLiteral("postLoadVolume highres ready"));

  if (Global::volumeType() != Global::DummyVolume)
    {
      VolumeInformation volInfo = VolumeInformation::volumeInformation(0);
      QPolygonF fmap = volInfo.mapping;
      m_tfEditor->setMapping(fmap);
      
      if (m_Volume->pvlVoxelType(0) == 0)
	{
	  m_tfEditor->show16BitEditor(false);
	  m_tfEditor->setHistogramImage(m_Lowres->histogramImage1D(),
					m_Lowres->histogramImage2D());
	  m_tfEditor->setHistogram2D(m_Lowres->histogram1D());
	}
      else
	{
	  appendRuntimeDiagnostic(QStringLiteral("postLoadVolume histogram 16bit begin"));
	  m_tfEditor->show16BitEditor(true);
	  m_tfEditor->setHistogram2D(m_Lowres->histogram2D());
	  appendRuntimeDiagnostic(QStringLiteral("postLoadVolume histogram 16bit done"));
	}
    }

  m_Lowres->raise();
  m_Hires->lower();

  m_tfManager->setEnabled(true);

  m_Viewer->resetLookupTable();
  appendRuntimeDiagnostic(QStringLiteral("postLoadVolume lookup table ready"));

 if (Global::volumeType() != Global::DummyVolume)
   m_tfManager->loadDefaultTF();
  appendRuntimeDiagnostic(QStringLiteral("postLoadVolume default TF ready"));

  Global::enableViewerUpdate();
  MainWindowUI::changeDrishtiIcon(true);
  appendRuntimeDiagnostic(QStringLiteral("postLoadVolume viewer update enabled"));

  QList<bool> rt = m_volInfoWidget->repeatType();
  appendRuntimeDiagnostic(QStringLiteral("postLoadVolume repeat type read"));
  m_Volume->setRepeatType(rt);
  appendRuntimeDiagnostic(QStringLiteral("postLoadVolume repeat type applied"));

  // use 1D transfer functions for 16 bit data sets
  appendRuntimeDiagnostic(QStringLiteral("postLoadVolume voxel type check begin"));
  const int voxelType = m_Volume->pvlVoxelType(0);
  appendRuntimeDiagnostic(QStringLiteral("postLoadVolume voxel type=%1 defer=%2")
                          .arg(voxelType).arg(deferSceneCommit));
  if (voxelType > 0)
    {
      Global::setUse1D(true);
      ui.actionSwitch_To1D->setChecked(Global::use1D());
      m_tfContainer->switch1D();
      appendRuntimeDiagnostic(QStringLiteral("postLoadVolume 1D transfer function ready"));
    }

  if (!deferSceneCommit)
    {
      appendRuntimeDiagnostic(QStringLiteral("postLoadVolume scene reset begin"));
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
      m_keyFrameEditor->setHiresMode(false);
      appendRuntimeDiagnostic(QStringLiteral("postLoadVolume scene reset done"));
    }
  if (!m_deferVolumeCommit)
    {
      m_Volume->commitPendingLoad();
      m_projectRollbackValid = false;
      m_projectRollbackPreferencesValid = false;
    }
  return true;
}

void
MainWindow::recoverFromFailedVolumeLoad(const QString &message)
{
  // Restore the complete previous workset, including preferences changed
  // while preparing a candidate project or rendering resource.
  rollbackProjectVolumeLoad();
  Global::hideProgressBar();
  MainWindowUI::mainWindowUI()->menubar->parentWidget()->
    setWindowTitle(QString("Drishti"));
  MainWindowUI::mainWindowUI()->statusBar->showMessage(message);
  Global::enableViewerUpdate();
  MainWindowUI::changeDrishtiIcon(true);
}

void
MainWindow::captureVolumeLoadRollback()
{
  if (m_deferVolumeCommit)
    return;
  m_projectRollbackLowresState = m_Lowres->captureState();
  m_projectRollbackLowresValid = true;
  m_projectRollbackDockTFVisible = m_dockTF->isVisible();
  QAction *emptySpaceSkipAction = MainWindowUI::mainWindowUI()->actionEmptySpaceSkip;
  m_projectRollbackEmptySpaceSkipEnabled = emptySpaceSkipAction->isEnabled();
  m_projectRollbackEmptySpaceSkipChecked = emptySpaceSkipAction->isChecked();
  m_projectRollbackCurrentProject = Global::currentProjectFile();
  m_projectRollbackPreviousDirectory = Global::previousDirectory();
  m_projectRollbackVolFiles1 = m_volFiles1;
  m_projectRollbackVolFiles2 = m_volFiles2;
  m_projectRollbackVolFiles3 = m_volFiles3;
  m_projectRollbackVolFiles4 = m_volFiles4;
  m_projectRollbackPreferences = m_preferencesWidget->captureState();
  m_projectRollbackPreferencesValid = true;
  m_projectRollbackValid = m_Volume->valid();
  if (!m_projectRollbackValid)
    return;

  m_Lowres->subvolumeBounds(m_projectRollbackBoundsMin,
                            m_projectRollbackBoundsMax);
  m_projectRollbackHires = m_Hires->raised();
  m_projectRollbackTFEnabled = m_tfManager->isEnabled();
  m_projectRollbackUse1D = Global::use1D();
  m_projectRollbackEmptySpaceSkip = Global::emptySpaceSkip();
  m_projectRollbackGamma = Global::gamma();
  m_projectRollbackLutSize = Global::lutSize();
  for (int i = 0; i < 4; ++i)
    m_projectRollbackVolumeNumbers[i] = Global::volumeNumber(i);
  m_projectRollbackTF.clear();
  for (int i = 0; i < m_tfContainer->count(); ++i)
    m_projectRollbackTF.append(
      m_tfContainer->transferFunctionPtr(i)->getSpline());
}

void
MainWindow::rollbackProjectVolumeLoad()
{
  m_deferVolumeCommit = false;
  if (m_pendingKeyFrameCandidate)
    {
      delete m_pendingKeyFrameCandidate;
      m_pendingKeyFrameCandidate = 0;
    }
  m_pendingKeyFrameValid = false;
  m_pendingLowresStateValid = false;
  if (m_projectRollbackPreferencesValid)
    {
      m_preferencesWidget->restoreState(m_projectRollbackPreferences);
      m_projectRollbackPreferencesValid = false;
    }
  m_dockTF->setVisible(m_projectRollbackDockTFVisible);
  QAction *emptySpaceSkipAction = MainWindowUI::mainWindowUI()->actionEmptySpaceSkip;
  emptySpaceSkipAction->setEnabled(m_projectRollbackEmptySpaceSkipEnabled);
  emptySpaceSkipAction->setChecked(m_projectRollbackEmptySpaceSkipChecked);
  Global::setCurrentProjectFile(m_projectRollbackCurrentProject);
  Global::setPreviousDirectory(m_projectRollbackPreviousDirectory);
  m_volFiles1 = m_projectRollbackVolFiles1;
  m_volFiles2 = m_projectRollbackVolFiles2;
  m_volFiles3 = m_projectRollbackVolFiles3;
  m_volFiles4 = m_projectRollbackVolFiles4;
  m_Volume->rollbackPendingLoad();
  if (m_projectRollbackLowresValid)
    {
      m_Lowres->applyState(m_projectRollbackLowresState);
      m_Lowres->setSubvolumeBounds(m_projectRollbackLowresState.subvolumeMin,
                                   m_projectRollbackLowresState.subvolumeMax);
      m_projectRollbackLowresValid = false;
    }
  if (m_projectRollbackValid)
    {
      Global::setVolumeNumber(m_projectRollbackVolumeNumbers[0], 0);
      Global::setVolumeNumber(m_projectRollbackVolumeNumbers[1], 1);
      Global::setVolumeNumber(m_projectRollbackVolumeNumbers[2], 2);
      Global::setVolumeNumber(m_projectRollbackVolumeNumbers[3], 3);
      Global::setLutSize(m_projectRollbackLutSize);
      Global::setUse1D(m_projectRollbackUse1D);
      Global::setEmptySpaceSkip(m_projectRollbackEmptySpaceSkip);
      Global::setGamma(m_projectRollbackGamma);
      m_tfManager->load(m_projectRollbackTF);
      m_tfManager->setEnabled(m_projectRollbackTFEnabled);
      if (m_Volume->valid())
        {
          m_Lowres->loadVolume(false);
          m_Hires->loadVolume(false);
          m_Hires->loadTextureMemory();
          m_Lowres->setSubvolumeBounds(m_projectRollbackBoundsMin,
                                       m_projectRollbackBoundsMax);
          if (m_projectRollbackHires)
            m_Viewer->switchToHires();
          else
            {
              m_Lowres->raise();
              m_Hires->lower();
            }
        }
      m_projectRollbackValid = false;
    }
  m_Hires->enableSubvolumeUpdates();
  Global::enableViewerUpdate();
  MainWindowUI::changeDrishtiIcon(true);
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

      loadVolumeRGB(files[0].toUtf8().data());
    }
}

bool
MainWindow::loadVolumeRGB(char *flnm)
{  
  Global::setSaveImageType(Global::NoImage);

  if (QString(flnm).isEmpty())
    return false;
 
  captureVolumeLoadRollback();
  if (!m_Volume->loadVolumeRGB(flnm, false))
    {
      recoverFromFailedVolumeLoad("RGB/RGBA volume loading failed");
      QMessageBox::warning(0, "Error",
			   "Cannot load RGB/RGBA volume within the available CPU memory.");
      return false;
    }

  preLoadVolume();
  if (!postLoadVolume())
    return false;

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
  return true;
}


bool
MainWindow::loadVolume(QList<QString> flnm)
{  
  Global::setSaveImageType(Global::NoImage);

  if (flnm.count() == 0)
    return false;
 
  captureVolumeLoadRollback();
  if (!m_Volume->loadVolume(flnm, false))
    {
      recoverFromFailedVolumeLoad("Volume loading failed");
      QMessageBox::warning(0, "Error",
			   "Cannot load volume within the available CPU memory.");
      return false;
    }

  preLoadVolume();
  if (!postLoadVolume())
    return false;

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
  return true;
}

bool
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
	return false;
      
      FilesListDialog fld2(files2);
      fld2.exec();
      if (fld2.result() == QDialog::Rejected)
	return false;
    }

  if (!loadVolume2(files1, files2))
    {
      QMessageBox::information(0, "Error", "Cannot load volumes");
      return false;
    }

  Global::setVolumeNumber(0);
  Global::setVolumeNumber(0, 1);


  QList<QString> volfiles1;
  for(int i=0; i<files1.count(); i++)
    volfiles1.append(files1[i].toUtf8().data());

  QList<QString> volfiles2;
  for(int i=0; i<files2.count(); i++)
    volfiles2.append(files2[i].toUtf8().data());

  QList<int> vsizes;
  vsizes << volfiles1.size();
  vsizes << volfiles2.size();
  emit setVolumes(vsizes);
  emit refreshVolInfo(0, 0, m_Volume->volInfo(0));
  emit refreshVolInfo(1, 0, m_Volume->volInfo(0,1));
  return true;
}

bool
MainWindow::loadVolume2(QList<QString> flnm1,
			QList<QString> flnm2)
{  
  Global::setSaveImageType(Global::NoImage);

  if (flnm1.count() == 0 ||
      flnm2.count() == 0)
    return false;

  captureVolumeLoadRollback();
  if (!m_Volume->loadVolume(flnm1, flnm2, false))
    {
      recoverFromFailedVolumeLoad("Two-volume loading failed");
      return false;
    }

  preLoadVolume();
  if (!postLoadVolume())
    return false;

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

bool
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
	return false;
      
      FilesListDialog fld2(files2);
      fld2.exec();
      if (fld2.result() == QDialog::Rejected)
	return false;

      FilesListDialog fld3(files3);
      fld3.exec();
      if (fld3.result() == QDialog::Rejected)
	return false;
    }

  if (!loadVolume3(files1, files2, files3))
    return false;

  Global::setVolumeNumber(0);
  Global::setVolumeNumber(0, 1);
  Global::setVolumeNumber(0, 2);


  QList<QString> volfiles1;
  for(int i=0; i<files1.count(); i++)
    volfiles1.append(files1[i].toUtf8().data());

  QList<QString> volfiles2;
  for(int i=0; i<files2.count(); i++)
    volfiles2.append(files2[i].toUtf8().data());

  QList<QString> volfiles3;
  for(int i=0; i<files3.count(); i++)
    volfiles3.append(files3[i].toUtf8().data());

  QList<int> vsizes;
  vsizes << volfiles1.size();
  vsizes << volfiles2.size();
  vsizes << volfiles3.size();
  emit setVolumes(vsizes);
  emit refreshVolInfo(0, 0, m_Volume->volInfo(0));
  emit refreshVolInfo(1, 0, m_Volume->volInfo(0,1));
  emit refreshVolInfo(2, 0, m_Volume->volInfo(0,2));
  return true;

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


  captureVolumeLoadRollback();
  if (!m_Volume->loadVolume(flnm1, flnm2, flnm3, false))
    {
      recoverFromFailedVolumeLoad("Three-volume loading failed");
      return false;
    }

  preLoadVolume();
  if (!postLoadVolume())
    return false;

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


bool
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
	return false;
      
      FilesListDialog fld2(files2);
      fld2.exec();
      if (fld2.result() == QDialog::Rejected)
	return false;

      FilesListDialog fld3(files3);
      fld3.exec();
      if (fld3.result() == QDialog::Rejected)
	return false;

      FilesListDialog fld4(files4);
      fld4.exec();
      if (fld4.result() == QDialog::Rejected)
	return false;
    }

  if (!loadVolume4(files1, files2, files3, files4))
    return false;

  Global::setVolumeNumber(0);
  Global::setVolumeNumber(0, 1);
  Global::setVolumeNumber(0, 2);
  Global::setVolumeNumber(0, 3);


  QList<QString> volfiles1;
  for(int i=0; i<files1.count(); i++)
    volfiles1.append(files1[i].toUtf8().data());

  QList<QString> volfiles2;
  for(int i=0; i<files2.count(); i++)
    volfiles2.append(files2[i].toUtf8().data());

  QList<QString> volfiles3;
  for(int i=0; i<files3.count(); i++)
    volfiles3.append(files3[i].toUtf8().data());

  QList<QString> volfiles4;
  for(int i=0; i<files4.count(); i++)
    volfiles4.append(files4[i].toUtf8().data());


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
  return true;

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

  captureVolumeLoadRollback();
  if (!m_Volume->loadVolume(flnm1, flnm2, flnm3, flnm4, false))
    {
      recoverFromFailedVolumeLoad("Four-volume loading failed");
      return false;
    }

  preLoadVolume();
  if (!postLoadVolume())
    return false;

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
    QDomElement de0 = doc.createElement("maxdragvolsize");
    QDomText tn0;
    tn0 = doc.createTextNode(QString("%1").arg(Global::maxDragVolSize()));
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
    QDomElement de0 = doc.createElement("maxslabsize");
    QDomText tn0;
    tn0 = doc.createTextNode(QString("%1").arg(Global::maxSlabSize()));
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }
  
  {
    QDomElement de0 = doc.createElement("texturesizelimit");
    QDomText tn0;
    tn0 = doc.createTextNode(QString("%1").arg(Global::maxArrayTextureLayers()));
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

  {
    QDomElement de0 = doc.createElement("autospin");
    QDomText tn0;
    tn0 = doc.createTextNode(QString("%1").arg(m_Viewer->camera()->frame()->spinningSensitivity()));
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }

  {
    QDomElement de0 = doc.createElement("bgcolor");
    QDomText tn0;
    Vec bgcolor = Global::backgroundColor();
    tn0 = doc.createTextNode(QString("%1 %2 %3").arg(bgcolor.x).arg(bgcolor.y).arg(bgcolor.z));
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }

  QString homePath = QDir::homePath();
  QFileInfo settingsFile(homePath, ".drishti.xml");
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


  m_preferencesWidget->save(flnm.toUtf8().data());
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
      if (dlist.at(i).nodeName() == "texturememory")
	{
	  QString str = dlist.at(i).toElement().text();
	  Global::setTextureMemorySize(str.toInt());
	  Global::calculate3dTextureSize();
	}
      else if (dlist.at(i).nodeName() == "maxdragvolsize")
	{
	  QString str = dlist.at(i).toElement().text();
	  Global::setMaxDragVolSize(str.toInt());
	  m_preferencesWidget->setMaxDragVolSize(Global::maxDragVolSize());
	}
      else if (dlist.at(i).nodeName() == "texsizereducefraction")
	{
	  QString str = dlist.at(i).toElement().text();
	  Global::setTexSizeReduceFraction(str.toFloat());
	}
      else if (dlist.at(i).nodeName() == "maxslabsize")
	{
	  QString str = dlist.at(i).toElement().text();
	  Global::setMaxSlabSize(str.toFloat());
	}
      else if (dlist.at(i).nodeName() == "texturesizelimit")
	{
	  QString str = dlist.at(i).toElement().text();
	  Global::setMaxArrayTextureLayers(str.toInt());
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
      else if (dlist.at(i).nodeName() == "autospin")
	{
	  QString str = dlist.at(i).toElement().text();
	  m_Viewer->camera()->frame()->setSpinningSensitivity(str.toFloat());
	}
      else if (dlist.at(i).nodeName() == "bgcolor")
	{
	  QStringList str = dlist.at(i).toElement().text().split(" ", Qt::SkipEmptyParts);
	  if (str.length() == 3)
	    Global::setBackgroundColor(Vec(str[0].toFloat(),str[1].toFloat(),str[2].toFloat()));
	}
    }
  m_preferencesWidget->updateTextureMemory();
  m_preferencesWidget->load(flnm.toUtf8().data());
  updateRecentFileAction();

  // texture memory is not set by the user, so ask the user to set it.
  if (Global::textureMemorySize() < 256)
    setTextureMemory(true);
}

void
MainWindow::loadTransferFunctionsOnly(const char* flnm)
{
  m_tfManager->append(flnm);
}

void
MainWindow::loadProject(const char* flnm)
{
  if (!m_Viewer->rendererReady())
    {
      QMessageBox::information(0, "Drishti", "Not yet ready to start work!");
      return;
    }
  Global::disableViewerUpdate();
  MainWindowUI::changeDrishtiIcon(false);
  m_Hires->disableSubvolumeUpdates();
  captureVolumeLoadRollback();
  m_deferVolumeCommit = true;

  bool keyframesVisible = m_dockKeyframe->isVisible();

  QFileInfo f(flnm);
  int projectType = -1;
  QList<QString> candidateFiles1;
  QList<QString> candidateFiles2;
  QList<QString> candidateFiles3;
  QList<QString> candidateFiles4;
  QString keyframesFile(flnm);
  if (keyframesFile.contains(".dpxml", Qt::CaseInsensitive))
    keyframesFile.replace(".dpxml", ".keyframes", Qt::CaseInsensitive);
  else
    keyframesFile.replace(".xml", ".keyframes", Qt::CaseInsensitive);
  bool projectParsed = loadVolumeFromProject(flnm, projectType,
                                             candidateFiles1,
                                             candidateFiles2,
                                             candidateFiles3,
                                             candidateFiles4);

  if (projectParsed)
    {
      QFile keyframes(keyframesFile);
      if (keyframes.exists())
        {
          QByteArray header;
          bool keyframesValid = keyframes.open(QIODevice::ReadOnly) &&
            (header = keyframes.read(18)).size() >= 17 &&
            header.startsWith("Drishti Keyframes");
          if (keyframesValid)
            {
              const qint64 fileSize = keyframes.size();
              // The binary reader uses a 32-bit frame count. Bound it before
              // any allocation and require the stream terminator so a
              // truncated sidecar cannot partially replace the project.
              keyframesValid = fileSize >= 22;
              if (keyframesValid)
                {
                  bool foundKeyframes = false;
                  bool foundDone = false;
                  QByteArray tail;
                  while (!keyframes.atEnd() && keyframesValid)
                    {
                      const QByteArray chunk = keyframes.read(64*1024);
                      if (chunk.isEmpty() && keyframes.error() != QFile::NoError)
                        {
                          keyframesValid = false;
                          break;
                        }
                      const QByteArray window = tail + chunk;
                      foundKeyframes = foundKeyframes || window.contains("keyframes");
                      foundDone = foundDone || window.contains("done");
                      tail = window.right(16);
                    }
                  keyframesValid = keyframesValid && foundKeyframes && foundDone &&
                    fileSize <= 512LL*1024*1024;
                }
            }
          if (!keyframesValid)
            {
              if (keyframes.isOpen())
                keyframes.close();
              projectParsed = false;
            }
          else
            keyframes.close();
        }
    }

  if (!projectParsed || projectType < Global::SingleVolume ||
      projectType > Global::DummyVolume)
    {
      rollbackProjectVolumeLoad();
      m_deferVolumeCommit = false;
      QMessageBox::warning(0, "Project load failed",
                           "The project does not contain a supported volume definition.");
      return;
    }
  {
    QFileInfo projectInfo(flnm);
    const QString xmlPath = projectInfo.absoluteFilePath();
    QString keyframesPath = xmlPath;
    keyframesPath.replace(".dpxml", ".keyframes", Qt::CaseInsensitive);
    if (keyframesPath == xmlPath)
      keyframesPath.replace(".xml", ".keyframes", Qt::CaseInsensitive);
    QString recoveryError;
    if (!ProjectSaveJournal::recover(QStringList() << xmlPath << keyframesPath,
                                     &recoveryError))
      {
        QMessageBox::warning(0, "Project load failed",
                             recoveryError.isEmpty() ?
                             QString("An incomplete project save could not be recovered.") :
                             recoveryError);
        return;
      }
  }

  // Validate project sidecars before loading or committing the replacement
  // volume. Existing sidecars are parsed by legacy loaders later; malformed
  // XML must be rejected while the current workset is still untouched.
  QStringList sidecars;
  sidecars << QString(flnm);
  QString lowresFile(flnm);
  QString preferencesFile(flnm);
  QString tfFile(flnm);
  lowresFile.replace(".xml", ".lowres", Qt::CaseInsensitive);
  preferencesFile.replace(".xml", ".preferences", Qt::CaseInsensitive);
  tfFile.replace(".xml", ".tf", Qt::CaseInsensitive);
  sidecars << lowresFile << preferencesFile << tfFile;
  for (int i = 0; i < sidecars.count(); ++i)
    {
      QFile sidecar(sidecars.at(i));
      if (!sidecar.exists())
        continue;
      QDomDocument sidecarDocument;
      QString sidecarError;
      int sidecarLine = 0;
      int sidecarColumn = 0;
      if (!sidecar.open(QIODevice::ReadOnly) ||
          !sidecarDocument.setContent(&sidecar, &sidecarError,
                                      &sidecarLine, &sidecarColumn))
        {
          if (sidecar.isOpen())
            sidecar.close();
          rollbackProjectVolumeLoad();
          QMessageBox::warning(0, "Project load failed",
                               QString("Invalid project sidecar: %1")
                               .arg(sidecars.at(i)));
          return;
        }
      sidecar.close();
    }

  if (!m_Lowres->validate(flnm))
    {
      rollbackProjectVolumeLoad();
      QMessageBox::warning(0, "Project load failed",
                           "The low-resolution project resource is invalid.");
      return;
    }
  if (!m_preferencesWidget->validate(flnm))
    {
      rollbackProjectVolumeLoad();
      QMessageBox::warning(0, "Project load failed",
                           "The project preferences are invalid.");
      return;
    }
  if (!m_tfManager->validate(flnm))
    {
      rollbackProjectVolumeLoad();
      QMessageBox::warning(0, "Project load failed",
                           "The project transfer functions are invalid.");
      return;
    }

  // Parse the complete binary sidecar before replacing the active volume.
  // The later call commits the already validated format into the new project.
  if (QFileInfo(keyframesFile).exists() && !loadKeyFrames(flnm, false))
    {
      rollbackProjectVolumeLoad();
      QMessageBox::warning(0, "Project load failed",
                           "The project keyframes could not be parsed.");
      return;
    }

  bool volumeLoaded = true;
  if (projectType == Global::SingleVolume)
    volumeLoaded = m_Volume->loadVolume(candidateFiles1, false);
  else if (projectType == Global::DoubleVolume)
    volumeLoaded = m_Volume->loadVolume(candidateFiles1, candidateFiles2, false);
  else if (projectType == Global::TripleVolume)
    volumeLoaded = m_Volume->loadVolume(candidateFiles1, candidateFiles2,
                                         candidateFiles3, false);
  else if (projectType == Global::QuadVolume)
    volumeLoaded = m_Volume->loadVolume(candidateFiles1, candidateFiles2,
                                         candidateFiles3, candidateFiles4, false);
  else if (projectType == Global::RGBVolume ||
	   projectType == Global::RGBAVolume)
    volumeLoaded = !candidateFiles1.isEmpty() &&
      m_Volume->loadVolumeRGB(candidateFiles1[0].toUtf8().data(), false);

  bool projectVolumeMatches = (projectType == Global::DummyVolume);
  if (projectType == Global::SingleVolume ||
      projectType == Global::RGBVolume ||
      projectType == Global::RGBAVolume)
    projectVolumeMatches = m_Volume->valid() &&
      (projectType == Global::RGBVolume || projectType == Global::RGBAVolume ?
       m_Volume->volumeFiles(0).value(0) == candidateFiles1.value(0) :
       m_Volume->volumeFiles(0) == candidateFiles1);
  else if (projectType == Global::DoubleVolume)
    projectVolumeMatches = m_Volume->valid() &&
      m_Volume->volumeFiles(0) == candidateFiles1 &&
      m_Volume->volumeFiles(1) == candidateFiles2;
  else if (projectType == Global::TripleVolume)
    projectVolumeMatches = m_Volume->valid() &&
      m_Volume->volumeFiles(0) == candidateFiles1 &&
      m_Volume->volumeFiles(1) == candidateFiles2 &&
      m_Volume->volumeFiles(2) == candidateFiles3;
  else if (projectType == Global::QuadVolume)
    projectVolumeMatches = m_Volume->valid() &&
      m_Volume->volumeFiles(0) == candidateFiles1 &&
      m_Volume->volumeFiles(1) == candidateFiles2 &&
      m_Volume->volumeFiles(2) == candidateFiles3 &&
      m_Volume->volumeFiles(3) == candidateFiles4;

  if (!volumeLoaded || !projectVolumeMatches)
    {
      rollbackProjectVolumeLoad();
      QMessageBox::warning(0, "Project load failed",
                           "The project volume could not be loaded.");
      return;
    }

  // Prepare generic rendering resources against the detached candidate.  The
  // public menu wrappers are intentionally bypassed so a project load cannot
  // emit a premature "Volume loaded" result or mutate project metadata.
  if (projectType != Global::DummyVolume)
    {
      preLoadVolume();
      if (!postLoadVolume())
        {
          rollbackProjectVolumeLoad();
          QMessageBox::warning(0, "Project load failed",
                               "The rendering resources could not be created.");
          return;
        }
      appendRuntimeDiagnostic(QStringLiteral("project load postLoadVolume returned"));
    }

  if (!Global::batchMode())
    emit showMessage("Volume loaded. Loading Project ....", false);

  DrawLowresVolume::State candidateLowresState;
  appendRuntimeDiagnostic(QStringLiteral("project load read lowres state begin"));
  if (!m_Lowres->readState(flnm, candidateLowresState))
    {
      rollbackProjectVolumeLoad();
      QMessageBox::warning(0, "Project load failed",
                           "The low-resolution project resource is invalid.");
      return;
    }
  appendRuntimeDiagnostic(QStringLiteral("project load read lowres state done"));
  m_pendingLowresState = candidateLowresState;
  m_pendingLowresStateValid = true;
  if (projectType == Global::DummyVolume)
    {
      Vec bmin = candidateLowresState.subvolumeMin;
      Vec bmax = candidateLowresState.subvolumeMax;
      Vec vmax = candidateLowresState.dataMax;
      int nx = vmax.x;
      int ny = vmax.y;
      int nz = vmax.z;
      if (!m_Volume->loadDummyVolume(nx, ny, nz))
        {
          rollbackProjectVolumeLoad();
          QMessageBox::warning(0, "Project load failed",
                               "The dummy project volume could not be created.");
          return;
        }

      Vec sbmin, sbmax;
      sbmin = Vec(bmin.z, bmin.y, bmin.x);
      sbmax = Vec(bmax.z, bmax.y, bmax.x);
      candidateLowresState.subvolumeMin = sbmin;
      candidateLowresState.subvolumeMax = sbmax;
    }

  // The low-resolution sidecar is parsed completely before it is applied to
  // the active renderer.  Applying this state is non-fallible; all fallible
  // project operations have already completed above.
  appendRuntimeDiagnostic(QStringLiteral("project load apply lowres state begin"));
  m_Lowres->applyState(candidateLowresState);
  appendRuntimeDiagnostic(QStringLiteral("project load apply lowres state done"));
  m_pendingLowresStateValid = false;
  if (projectType == Global::DummyVolume)
    {
      preLoadVolume();
      if (!postLoadVolume())
        {
          rollbackProjectVolumeLoad();
          QMessageBox::warning(0, "Project load failed",
                               "The rendering resources could not be created.");
          return;
        }
    }

  // The sidecars are loaded against the detached volume while the previous
  // geometry/keyframes remain intact.  Commit the scene reset only after all
  // sidecars below have succeeded.


  appendRuntimeDiagnostic(QStringLiteral("project load preferences begin"));
  if (!m_preferencesWidget->load(flnm))
    {
      rollbackProjectVolumeLoad();
      QMessageBox::warning(0, "Project load failed",
                           "The project preferences are invalid.");
      return;
    }
  appendRuntimeDiagnostic(QStringLiteral("project load preferences done"));

  appendRuntimeDiagnostic(QStringLiteral("project load transfer function begin"));
  if (!m_tfManager->load(flnm))
    {
      rollbackProjectVolumeLoad();
      QMessageBox::warning(0, "Project load failed",
                           "The project transfer functions are invalid.");
      return;
    }
  appendRuntimeDiagnostic(QStringLiteral("project load transfer function done"));

  // Commit the validated keyframe candidate before the scene reset.  No
  // fallible project operation remains after this point, so geometry is
  // rebuilt from a known-good keyframe rather than being cleared first.
  appendRuntimeDiagnostic(QStringLiteral("project load keyframe commit begin"));
  if (QFileInfo(keyframesFile).exists() && !commitPendingKeyFrames())
    {
      rollbackProjectVolumeLoad();
      QMessageBox::warning(0, "Project load failed",
                           "The project keyframes could not be loaded.");
      return;
    }
  appendRuntimeDiagnostic(QStringLiteral("project load keyframe commit done"));
  if (!QFileInfo(keyframesFile).exists())
    {
      // A project without an optional keyframe sidecar must not inherit the
      // previous scene.  The candidate state is explicitly empty/default.
      m_keyFrame->clear();
      appendRuntimeDiagnostic(QStringLiteral("project load empty keyframe reset done"));
    }

  m_dockKeyframe->setVisible(false);

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

  // Commit project metadata and scene ownership only after every detached
  // resource has parsed successfully.
  m_volFiles1 = candidateFiles1;
  m_volFiles2 = candidateFiles2;
  m_volFiles3 = candidateFiles3;
  m_volFiles4 = candidateFiles4;
  Global::setCurrentProjectFile(QString(flnm));
  Global::setPreviousDirectory(f.absolutePath());
  m_bricks->reset();
  GeometryObjects::clipplanes()->reset();

  m_bricksWidget->refresh();

  m_Hires->enableSubvolumeUpdates();
  m_Viewer->switchToHires();
  m_keyFrameEditor->setHiresMode(true);
  if (Global::volumeType() != Global::DummyVolume)
    {
      if (m_Volume->pvlVoxelType(0) == 0)
	{
	  m_tfEditor->setHistogramImage(m_Hires->histogramImage1D(),
					m_Hires->histogramImage2D());
	  m_tfEditor->setHistogram2D(m_Hires->histogram1D());
	}
      else
	m_tfEditor->setHistogram2D(m_Hires->histogram2D());
    }

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

  if (QFileInfo(keyframesFile).exists() &&
      !m_keyFrame->commitRendererCandidate())
    {
      rollbackProjectVolumeLoad();
      QMessageBox::warning(0, "Project load failed",
                           "The project renderer candidate is invalid.");
      return;
    }
  
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

  // check overlapping keyframes
  m_keyFrame->checkKeyFrameNumbers();
  m_deferVolumeCommit = false;
  m_Volume->commitPendingLoad();
  m_pendingLowresStateValid = false;
  m_projectRollbackValid = false;
  m_projectRollbackPreferencesValid = false;
  m_projectRollbackLowresValid = false;
}

bool
MainWindow::saveProject(QString xmlflnm)
{

  QFileInfo f(xmlflnm);
  const QString xmlPath = f.absoluteFilePath();
  QString keyframesPath = xmlPath;
  keyframesPath.replace(".dpxml", ".keyframes", Qt::CaseInsensitive);
  if (keyframesPath == xmlPath)
    keyframesPath.replace(".xml", ".keyframes", Qt::CaseInsensitive);
  // Preferences, lowres and transfer functions are embedded as XML nodes;
  // the XML plus binary keyframe sidecar are the two physical commit targets.
  const QStringList projectTargets = QStringList() << xmlPath << keyframesPath;
  ProjectSaveJournalState transaction;
  QString transactionError;
  if (!ProjectSaveJournal::begin(projectTargets,
                                 transaction, &transactionError))
    {
      QMessageBox::warning(0, "Project not saved", transactionError);
      return false;
    }

  const QString stagedXmlPath = transaction.stages.at(0);
  int flnmlen = stagedXmlPath.length()+1;
  char *flnm = new char[flnmlen];
  memset(flnm, 0, flnmlen);
  memcpy(flnm, stagedXmlPath.toUtf8().data(), flnmlen);


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

  int dofblur;
  float dofnf;
  m_Hires->dof(dofblur, dofnf);

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
			  fop, bop,
			  dofblur, dofnf);


  bool saved = saveVolumeIntoProject(flnm, QString());
  saved = saved && m_Lowres->save(flnm);
  saved = saved && m_preferencesWidget->save(flnm);
  saved = saved && m_tfManager->save(flnm);
  saved = saved && saveKeyFrames(flnm);
  delete [] flnm;
  if (saved)
    {
      QString stagedKeyframes = stagedXmlPath;
      if (stagedXmlPath.contains(".dpxml", Qt::CaseInsensitive))
        stagedKeyframes.replace(".dpxml", ".keyframes", Qt::CaseInsensitive);
      else
        stagedKeyframes.replace(".xml", ".keyframes", Qt::CaseInsensitive);
      if (stagedKeyframes != transaction.stages.at(1) &&
          QFileInfo::exists(stagedKeyframes))
        saved = QFile::rename(stagedKeyframes, transaction.stages.at(1));
      else
        saved = QFileInfo::exists(transaction.stages.at(1));
    }
  if (!saved || !ProjectSaveJournal::commit(transaction, &transactionError))
    {
      if (saved)
        transactionError = "project transaction commit failed";
      ProjectSaveJournal::recover(projectTargets, 0);
      QMessageBox::warning(0, "Project not saved",
                           transactionError.isEmpty() ?
                           QString("Unable to write project files") :
                           transactionError);
      return false;
    }

  Global::setPreviousDirectory(f.absolutePath());
  Global::setCurrentProjectFile(xmlPath);


  emit showMessage("Project saved", false);
  return true;
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
  loadProject(flnm.toUtf8().data());
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

  loadTransferFunctionsOnly(flnm.toUtf8().data());
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
					"xml Files (*.xml)");
//					0,
//					QFileDialog::DontUseNativeDialog);


  if (flnm.isEmpty())
    return;

  if (!StaticFunctions::checkExtension(flnm, ".xml"))
    flnm += ".xml";
  
  saveProject(flnm);
}
void
MainWindow::on_actionSave_InformationForDrishtiPrayog_triggered()
{
  QString flnm;
  flnm = QFileDialog::getSaveFileName(0,
				      "Save Information for Drishti-Prayog",
				      Global::previousDirectory(),
				      "DrishtiPrayog Files (*.drishtiprayog)");
//				      0,
//				      QFileDialog::DontUseNativeDialog);

  if (flnm.isEmpty())
    return;

  if (!StaticFunctions::checkExtension(flnm, ".drishtiprayog"))
    flnm += ".drishtiprayog";

  if (!m_Hires->saveForDrishtiPrayog(flnm))
    {
      QMessageBox::warning(0, "Drishti-Prayog information not saved",
			   "A complete single-slab texture is not available.");
      return;
    }
  
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
				      "Drishti Project - xml Files (*.xml)");
//				      0,
//				      QFileDialog::DontUseNativeDialog);

  if (flnm.isEmpty())
    return;

  if (!StaticFunctions::checkExtension(flnm, ".xml"))
    flnm += ".xml";
      
  saveProject(flnm.toUtf8().data());
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

  QVector4D lighting = QVector4D(hl.ambient,
			       hl.diffuse,
			       hl.specular,
			       hl.specularCoefficient);
		
  GeometryObjects::trisets()->setLighting(lighting);

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

  if (vnum != Global::volumeNumber())
    PruneHandler::forceRegen();
	      
  Global::setVolumeNumber(vnum);
  emit refreshVolInfo(vnum, m_Volume->volInfo(vnum));

  VolumeInformation volInfo = VolumeInformation::volumeInformation(vnum);
  QPolygonF fmap = volInfo.mapping;
  m_tfEditor->setMapping(fmap); 
}

void
MainWindow::updateVolInfo(int vol, int vnum)
{
  if (vnum != Global::volumeNumber(vol))
    PruneHandler::forceRegen();
	      
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
  if (vnum != Global::volumeNumber())
    PruneHandler::forceRegen();
	      
  Global::setVolumeNumber(vnum);
  if (m_Hires->raised())
    m_Hires->updateSubvolume();
  m_Viewer->update();
  updateVolInfo(vnum);
}

void
MainWindow::setVolumeNumber(int vol, int vnum)
{
  if (vnum != Global::volumeNumber(vol))
    PruneHandler::forceRegen();
	      
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

  int dofblur;
  float dofnf;
  m_Hires->dof(dofblur, dofnf);

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
			  fop, bop,
			  dofblur, dofnf);

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

// called from keyframeeditor
void
MainWindow::updateParameters(bool drawBox, bool drawAxis,
			     Vec bgColor,
			     QString bgImage,
			     int sz, int st,
			     QString xl, QString yl, QString zl,
			     int mv, bool mc, bool mo, float iv, bool mt,
			     bool pruneblend,
			     float fop, float bop,
			     int dofblur, float dofnf,
			     bool splinePos)
{
  ui.actionSpline_PositionInterpolation->setChecked(splinePos);
  if (splinePos)
    Global::setInterpolationType(Global::CameraPositionInterpolation, 1);
  else
    Global::setInterpolationType(Global::CameraPositionInterpolation, 0);
  
  
  m_preferencesWidget->setTick(sz, st, xl, yl, zl);
  Global::setBackgroundColor(bgColor);

  m_preferencesWidget->setDOF(dofblur, dofnf);

  m_Hires->setDOF(dofblur, dofnf);

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

bool
MainWindow::loadKeyFrames(const char* flnm, bool commit)
{
  QString sflnm(flnm);
  sflnm.replace(QString(".xml"), QString(".keyframes"));


  QFileInfo fileInfo(sflnm);
  if (! fileInfo.exists())
    {
      QMessageBox::information(0, "Error loading keyframes file", QString("%1 not found").arg(sflnm));
      return false;
    }

  fstream fin(sflnm.toUtf8().data(), ios::binary|ios::in);

  char keyword[100];
  fin.getline(keyword, 100, 0);
  if (strcmp(keyword, "Drishti Keyframes") != 0)
    {
      QMessageBox::information(0, "Load Keyframes",
			       QString("Invalid .keyframes file : ")+sflnm);
      return false;
    }

  KeyFrame candidate;
  bool foundKeyframes = false;
  while (!fin.eof())
    {
      fin.getline(keyword, 100, 0);
      if (strcmp(keyword, "keyframes") == 0)
	{
	  foundKeyframes = true;
	  if (!candidate.load(fin))
	    {
	      fin.close();
	      return false;
	    }
	}
    }
  fin.close();
  if (!foundKeyframes)
    return false;
  if (!candidate.validateRendererCandidate())
    return false;
  if (commit)
    {
      m_keyFrame->swapState(candidate);
      m_keyFrameEditor->clear();
    }
  else
    {
      if (m_pendingKeyFrameCandidate)
        delete m_pendingKeyFrameCandidate;
      m_pendingKeyFrameCandidate = new (std::nothrow) KeyFrame;
      if (!m_pendingKeyFrameCandidate)
        return false;
      m_pendingKeyFrameCandidate->swapState(candidate);
      m_pendingKeyFrameValid = true;
    }
  return true;
}

bool
MainWindow::commitPendingKeyFrames()
{
  if (!m_pendingKeyFrameValid || !m_pendingKeyFrameCandidate)
    return false;
  m_keyFrame->swapState(*m_pendingKeyFrameCandidate);
  delete m_pendingKeyFrameCandidate;
  m_pendingKeyFrameCandidate = 0;
  m_pendingKeyFrameValid = false;
  m_keyFrameEditor->clear();
  return true;
}

bool
MainWindow::saveKeyFrames(const char* flnm)
{
  QString sflnm(flnm);
  if (sflnm.contains(".dpxml", Qt::CaseInsensitive))
    sflnm.replace(QString(".dpxml"), QString(".keyframes"));
  else
    sflnm.replace(QString(".xml"), QString(".keyframes"));

  fstream fout(sflnm.toUtf8().data(), ios::binary|ios::out);
  if (!fout.good())
    return false;

  QString keyword;
  keyword = "Drishti Keyframes";
  fout.write((char*)(keyword.toUtf8().data()), keyword.length()+1);  

  m_keyFrame->save(fout);

  fout.close();
  return fout.good();
}

bool
MainWindow::saveVolumeIntoProject(const char *flnm, QString dtvfile)
{
  QString str;

  QDomDocument doc("Drishti_v1.0");

  QDomElement topElement = doc.createElement("Drishti");
  doc.appendChild(topElement);

  {
    QDomElement de0 = doc.createElement("maxdragvolsize");
    QDomText tn0;
    tn0 = doc.createTextNode(QString("%1").arg(Global::maxDragVolSize()));
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

  QSaveFile f(flnm);
  f.setDirectWriteFallback(false);
  if (!f.open(QIODevice::WriteOnly))
    return false;
  QTextStream out(&f);
  doc.save(out, 2);
  if (out.status() != QTextStream::Ok || !f.commit())
    {
      f.cancelWriting();
      return false;
    }
  return true;
}

bool
MainWindow::loadVolumeFromProject(const char *flnm,
                                  int& volType,
                                  QList<QString>& files1,
                                  QList<QString>& files2,
                                  QList<QString>& files3,
                                  QList<QString>& files4)
{
  volType = -1;
  files1.clear();
  files2.clear();
  files3.clear();
  files4.clear();

  QDomDocument document;
  QFile f(flnm);
  QString parseError;
  int parseLine = 0;
  int parseColumn = 0;
  if (!f.open(QIODevice::ReadOnly) ||
      !document.setContent(&f, &parseError, &parseLine, &parseColumn))
    {
      if (f.isOpen())
        f.close();
      return false;
    }
  f.close();

  QDomElement main = document.documentElement();
  if (main.isNull() || main.tagName() != "Drishti")
    return false;
  QDomNodeList dlist = main.childNodes();
  int volumeTypeElements = 0;
  for(int i=0; i<dlist.count(); i++)
    {
      if (!dlist.at(i).isElement())
        continue;
      if (dlist.at(i).nodeName() == "volumetype")
	{
	  ++volumeTypeElements;
	  if (volumeTypeElements > 1)
	    return false;
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
	  else if (str == "dummy")
	    volType = Global::DummyVolume;
	  else
	    {
	      return false;
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
	      if (!vlist.at(vi).isElement())
	        continue;
	      QString str = vlist.at(vi).toElement().text();
	      if (str.trimmed().isEmpty())
	        return false;
	      QString vfile = direc.absoluteFilePath(str);
	      files1.append(vfile);
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
	      if (!vlist.at(vi).isElement())
	        continue;
	      QString str = vlist.at(vi).toElement().text();
	      if (str.trimmed().isEmpty())
	        return false;
	      QString vfile = direc.absoluteFilePath(str);
	      files2.append(vfile);
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
	      if (!vlist.at(vi).isElement())
	        continue;
	      QString str = vlist.at(vi).toElement().text();
	      if (str.trimmed().isEmpty())
	        return false;
	      QString vfile = direc.absoluteFilePath(str);
	      files3.append(vfile);
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
	      if (!vlist.at(vi).isElement())
	        continue;
	      QString str = vlist.at(vi).toElement().text();
	      if (str.trimmed().isEmpty())
	        return false;
	      QString vfile = direc.absoluteFilePath(str);
	      files4.append(vfile);
	    }
	}
    }

//  // for 3 or 4 volumes always use 1D transfer functions
//  if (Global::volumeType() == Global::TripleVolume ||
//      Global::volumeType() == Global::QuadVolume)
//    Global::setUse1D(true);
//
//  ui.actionSwitch_To1D->setChecked(Global::use1D());

  if (volumeTypeElements != 1)
    return false;

  const int expectedGroups =
    volType == Global::SingleVolume || volType == Global::RGBVolume ||
    volType == Global::RGBAVolume ? 1 :
    volType == Global::DoubleVolume ? 2 :
    volType == Global::TripleVolume ? 3 :
    volType == Global::QuadVolume ? 4 : 0;
  if (volType == Global::DummyVolume)
    return files1.isEmpty() && files2.isEmpty() &&
      files3.isEmpty() && files4.isEmpty();

  if (expectedGroups == 0 || files1.isEmpty() ||
      (expectedGroups > 1 && files2.isEmpty()) ||
      (expectedGroups > 2 && files3.isEmpty()) ||
      (expectedGroups > 3 && files4.isEmpty()))
    return false;

  return true;
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
	{
	  emit histogramUpdated(m_Lowres->histogramImage1D(),
				m_Lowres->histogramImage2D());
	  emit histogramUpdated(m_Lowres->histogram1D());
	}
      else
	emit histogramUpdated(m_Lowres->histogram2D());
    }
  else
    {
      if (m_Volume->pvlVoxelType(0) == 0)
	{
	  emit histogramUpdated(m_Hires->histogramImage1D(),
				m_Hires->histogramImage2D());
	  emit histogramUpdated(m_Hires->histogram1D());
	}
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
				       "Files (*.txt)");
//				       0,
//				       QFileDialog::DontUseNativeDialog);
  
  
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
				       "Files (*.txt)");
//				       0,
//				       QFileDialog::DontUseNativeDialog);

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
    {
      m_Hires->setRenderQuality(Enums::RenderHighQuality);
      m_Hires->applyShadows(true);
    }
  else
    {
      m_Hires->setRenderQuality(Enums::RenderDefault);
      m_Hires->applyShadows(false);
    }
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
MainWindow::on_actionInterruptRendering_triggered()
{
  Global::setAllowInterruption(!Global::allowInterruption());  
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

