#include "global.h"
#include "mainwindow.h"
#include "mainwindowui.h"
#include "geometryobjects.h"
#include "dialogs.h"
#include "staticfunctions.h"
#include "saveimageseqdialog.h"
#include "savemoviedialog.h"
#include "propertyeditor.h"
#include "enums.h"
#include "pluginthread.h"
#include "xmlheaderfunctions.h"
#include "dcolordialog.h"

#include <QDockWidget>
#include <QFileDialog>
#include <QVBoxLayout>
#include <QInputDialog>
//#include <QNetworkReply>
#include <QTabWidget>
#include <QUdpSocket>

//#include <qt_windows.h>
//#include <netlistmgr.h>
//#include <atlcomcli.h>

#include <QTranslator>


#pragma comment(lib, "ole32.lib")

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
  if (m_Hires)
    {
      m_Hires->disconnect();
      delete m_Hires;
    }
  m_Hires = new DrawHiresVolume(m_Viewer);
  m_Hires->setBricks(m_bricks);

  connect(m_keyFrame, SIGNAL(updateLightInfo(LightingInformation)),
	  m_Hires, SLOT(setLightInfo(LightingInformation)));
  //---

  m_Viewer->setHiresVolume(m_Hires);

  m_Hires->lower();

  //reset field of view if it were changed earlier
  m_Viewer->setFieldOfView((float)(M_PI/4.0));

  qApp->processEvents();
}

MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent)
{

  QTranslator translator;
  if (translator.load(qApp->applicationDirPath() + QDir::separator() + "chinese"))
    QCoreApplication::installTranslator(&translator);
//  else
//    QMessageBox::information(0, "", "No translation found");
  
  
  ui.setupUi(this);

  qApp->setFont(QFont("MS Reference Sans Serif", 12));


  //--------
  // Chinese Help
  if (QFile::exists(qApp->applicationDirPath() + QDir::separator() + "chinese.qm"))
    Global::setHelpLanguage(".cn");
  //--------
    
  
  Global::setStatusBar(ui.statusBar, ui.actionStatusBar);

  ui.statusBar->setEnabled(true);
  ui.statusBar->setSizeGripEnabled(true);
  ui.statusBar->addPermanentWidget(Global::progressBar());
  
  MainWindowUI::setMainWindowUI(&ui);

  ui.actionGrabMode->setIcon(QIcon(":/images/grab.png"));

  
  setWindowIcon(QPixmap(":/images/logo.png"));

  m_saveRotationAnimation = 0;
  m_savePathAnimation = 0;
  m_pathAnimationVd.clear();
  m_pathAnimationUp.clear();
  m_pathAnimationPoints.clear();
  m_pathAnimationSaxis.clear();
  m_pathAnimationTaxis.clear();
  m_pathAnimationTang.clear();


  //--------------
  m_Viewer = new Viewer();
  setCentralWidget(m_Viewer);
  //--------------

  Global::setBatchMode(false);

#ifndef Q_OS_WIN32
  ui.actionSave_Movie->setEnabled(false);
#endif

  ui.actionStatusBar->setChecked(true);
  ui.actionBoundingBox->setChecked(false);

  StaticFunctions::initQColorDialog();

  initTagColors();

  //setTextureMemory();
  setAcceptDrops(true);

  m_bricks = new Bricks();

  m_Hires = 0;

  m_keyFrame = new KeyFrame();
  m_Viewer->setKeyFrame(m_keyFrame);
    
  //----------------------------------------------------------
  m_lightingWidget = new LightingWidget();
  m_bricksWidget = new BricksWidget(NULL, m_bricks);
  m_globalWidget = new GlobalWidget();

    //--- set background color ---
  QPalette pal = QPalette();
  pal.setColor(QPalette::Window, QColor("ghostwhite")); 
  setAutoFillBackground(true);
  setPalette(pal);
  pal.setColor(QPalette::Window, QColor("gainsboro")); 
  m_lightingWidget->setAutoFillBackground(true);
  m_lightingWidget->setPalette(pal);
  m_bricksWidget->setAutoFillBackground(true);
  m_bricksWidget->setPalette(pal);
  m_globalWidget->setAutoFillBackground(true);
  m_globalWidget->setPalette(pal);
  //----------------------------
  

  
  QDockWidget *dock2 = new QDockWidget(QWidget::tr("Shading-Transform"), this);
  dock2->setAllowedAreas(Qt::LeftDockWidgetArea | 
			 Qt::RightDockWidgetArea);
  QVBoxLayout *vbox = new QVBoxLayout();
  vbox->setSpacing(0);
  QTabWidget *tabW = new QTabWidget();
  tabW->setStyleSheet("QWidget{ font-size: 12pt; }");
  tabW->addTab(m_lightingWidget, "Shading");
  tabW->addTab(m_bricksWidget, "Transform");
  tabW->addTab(m_globalWidget, "Parameters");
  vbox->addWidget(tabW);
  vbox->addStretch();
  QWidget *widget = new QWidget();
  widget->setLayout(vbox);
  dock2->setWidget(widget);
  //----------------------------------------------------------


  //----------------------------------------------------------
  QDockWidget *dock3 = new QDockWidget(QWidget::tr("Mesh Information"), this);
  dock3->setStyleSheet("QDockWidget { font-size: 12pt; }");
  dock3->setAllowedAreas(Qt::LeftDockWidgetArea);
  QVBoxLayout *vbox3 = new QVBoxLayout();
  m_meshInfoWidget = new MeshInfoWidget();
  vbox3->addWidget(m_meshInfoWidget);
  QWidget *widget3 = new QWidget();
  widget3->setLayout(vbox3);
  dock3->setWidget(widget3);
  //dock3->hide();
  //----------------------------------------------------------
  

  //----------------------------------------------------------
  m_dockKeyframe = new QDockWidget(QWidget::tr("KeyFrame Editor"), this);
  m_dockKeyframe->setStyleSheet("QDockWidget { font-size: 12pt; }");
  m_dockKeyframe->setAllowedAreas(Qt::BottomDockWidgetArea | 
        			  Qt::TopDockWidgetArea);
  m_keyFrameEditor = new KeyFrameEditor();
  m_dockKeyframe->setWidget(m_keyFrameEditor);
  //m_dockKeyframe->hide();
  //----------------------------------------------------------



  addDockWidget(Qt::RightDockWidgetArea, dock2);
  addDockWidget(Qt::LeftDockWidgetArea, dock3);
  addDockWidget(Qt::BottomDockWidgetArea,m_dockKeyframe);


  QString tstr = QString(tr("Drishti ")) +
                 Global::DrishtiVersion() +
                 QWidget::tr(" - Surface Mesh Presentation Tool");

  if (QGLFormat::defaultFormat().stereo())
    tstr = QWidget::tr("(Stereo)")+tstr;

  setWindowTitle(tstr);



  ui.menuView->addAction(dock2->toggleViewAction());
  ui.menuView->addAction(dock3->toggleViewAction());
  ui.menuView->addAction(m_dockKeyframe->toggleViewAction());



  registerMenuViewerFunctions();

  createHiresLowresWindows();


  QTimer::singleShot(2000, this, SLOT(GlewInit()));


  #include "connectbricks.h"
  #include "connectbrickswidget.h"
  #include "connectclipplanes.h"
  #include "connectkeyframe.h"
  #include "connectkeyframeeditor.h"
  #include "connectshowmessage.h"
  #include "connectviewer.h"
  #include "connectgeometryobjects.h"
  #include "connectlightingwidget.h"

  #include "connectmeshinfowidget.h"
  
  connect(m_globalWidget, SIGNAL(bgColor()),
	  this, SLOT(setBackgroundColor()));
  connect(m_globalWidget, SIGNAL(shadowBox(bool)),
	  this, SLOT(setShadowBox(bool)));
  connect(m_globalWidget, SIGNAL(newVoxelUnit()),
	  this, SLOT(updateVoxelUnit()));
  connect(m_globalWidget, SIGNAL(newVoxelSize()),
	  this, SLOT(updateVoxelSize()));


  connect(GeometryObjects::trisets(), SIGNAL(updateMeshList(QStringList)),
	  this, SLOT(updateMeshList(QStringList)));

  
  initializeRecentFiles();


  loadSettings();

  
  GeometryObjects::trisets()->setHitPoints(GeometryObjects::hitpoints());
  //GeometryObjects::trisets()->setClipPlanes(GeometryObjects::clipplanes());

  //-----------
  //-----------
  // currently disabling all plugins and toolbar
  delete ui.menuPlugins;

  createPaintMenu();
  
  
  showMaximized();


  //----------------------------------------------------------------------
  m_projectFileName.clear();
  QStringList arguments = qApp->arguments();
  QStringList isFile = arguments.filter("file=");
  bool ok = false;
  if (isFile.count() == 1)
    {
      QString arg = isFile[0];
      QStringList tokens = arg.split("=");
      QFileInfo fileInfo(tokens[1]);
      if (fileInfo.exists())
	{
	  m_projectFileName = tokens[1];
	  // for full screen frameless Prayog like experience
	  Global::setPrayogMode(true);
	  m_keyFrameEditor->setPrayogMode(true);
	  ui.menubar->hide();
	  ui.statusBar->hide();
	  setWindowFlags(Qt::FramelessWindowHint);
	  dock2->hide();
	  dock3->hide();
	  m_dockKeyframe->setTitleBarWidget(new QWidget());
	  //m_dockKeyframe->hide();
	}
    }
  //----------------------------------------------------------------------

}

void
MainWindow::clearScene()
{
  GeometryObjects::captions()->clear();
  GeometryObjects::hitpoints()->clear();
  GeometryObjects::paths()->clear();
}

void
MainWindow::setShadowBox(bool b)
{
  Global::setShadowBox(b);
  m_Viewer->updateGL();
  qApp->processEvents();
}
void
MainWindow::setBackgroundColor()
{
  Vec bg = Global::backgroundColor();
  QColor bgc = QColor(255*bg.x,
		      255*bg.y,
		      255*bg.z);

  QColor color = DColorDialog::getColor(bgc);
  if (color.isValid())
    {      
      bg = Vec(color.redF(),
	       color.greenF(),
	       color.blueF());
      Global::setBackgroundColor(bg);
    }
}

void
MainWindow::lightDirectionChanged(Vec ldir)
{
  m_Hires->updateLightVector(ldir);
  GeometryObjects::trisets()->setLightDirection(ldir);
  m_Viewer->updateGL();
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
  if (!m_Hires->raised())
    {
      QMessageBox::information(0, tr("Error"), tr("Available only after surface mesh is loaded."));
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

  // look in @executable_path/meshplugins
  QString plugindir = qApp->applicationDirPath() + QDir::separator() + "meshplugins";

  QPair<QMenu*, QString> pair;
  QStack< QPair<QMenu*, QString> > stack;

  QStringList filters;
#if defined(Q_OS_WIN32)
  filters << "*.dll";
#endif
#ifdef Q_OS_MACX
  // look in drishti.app/renderplugins
  QString sep = QDir::separator();
  plugindir = qApp->applicationDirPath()+sep+".."+sep+".."+sep+"meshplugins";
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
	    } // for i
	} // list > 0
    } // while stack not empty
}

void
MainWindow::loadPlugin()
{
  QAction *action = qobject_cast<QAction *>(sender());
  int idx = action->data().toInt();


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
  
  pluginInterface->init();

  pluginInterface->setPreviousDirectory(Global::previousDirectory());
    
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
MainWindow::setTextureMemory()
{
  QString homePath = QDir::homePath();
  QFileInfo settingsFile(homePath, ".drishtimesh.xml");

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
					 QWidget::tr("Texture Memory"),
					 QWidget::tr("Texture Memory Size"),
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

  //registerPlugins();

  registerScripts();
  

  GLint textureSize;
  // query 3d texture size limits
  //glGetIntegerv(GL_MAX_TEXTURE_SIZE, &m_max2DTexSize);
  //glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &m_max3DTexSize);
  glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &textureSize);

  //GLint textureSize;
  //glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &textureSize);
  Global::setTextureSizeLimit(textureSize);

  // load material capture textures
  GeometryObjects::trisets()->loadMatCapTextures();

  // load program 
  QStringList arguments = qApp->arguments();
  if (arguments.count() > 1)
    {
      int i = 1;
      if (arguments[i] == "-stereo") i++;
      if (i < arguments.count())
	{
	  QStringList meshformats;
	  meshformats << ".fbx";
	  meshformats << ".dae";
	  meshformats << ".gltf";
	  meshformats << ".glb";
	  meshformats << ".blend";
	  meshformats << ".3ds";
	  meshformats << ".obj";
	  meshformats << ".ply";
	  meshformats << ".stl";	  
	  if (StaticFunctions::checkExtension(arguments[i], ".dm"))
	    {
	      if (m_projectFileName.isEmpty())
		{
		  Global::addRecentFile(arguments[i]);
		  updateRecentFileAction();
		  createHiresLowresWindows();
		  loadProject(arguments[i].toLatin1().data());
		}
	    }
	  else if (StaticFunctions::checkExtension(arguments[i], meshformats))
	    {
	      GeometryObjects::trisets()->addMesh(arguments[i]);

	      if (Global::volumeType() == Global::DummyVolume)
		{
		  int nx, ny, nz;
		  GeometryObjects::trisets()->allGridSize(nx, ny, nz);
		  Global::setVisualizationMode(Global::Surfaces);  
		  loadDummyVolume(nx, ny, nz);
		}
	  
	      QFileInfo f(arguments[i]);
	      Global::setPreviousDirectory(f.absolutePath());
	      Global::addRecentFile(arguments[i]);
	      
	      ui.statusBar->showMessage("");
	      
	      updateMeshList(GeometryObjects::trisets()->getMeshList());
	      GeometryObjects::trisets()->removeFromMouseGrabberPool();
	    }	  
	}
    }

  ui.actionPaintMode->setChecked(false);
  m_Viewer->setPaintMode(false);
  
  // Prayog Mode
  if (!m_projectFileName.isEmpty())
    loadProject(m_projectFileName.toLatin1().data());
}


void
MainWindow::closeEvent(QCloseEvent *event)
{
  if (Global::batchMode()) // don't ask this extra stuff
    return;


  //---------------
  if (m_scriptProcess && m_scriptProcess->processId() != 0)
    {
      //QMessageBox::information(0, "", "kill script");

      QByteArray Data;
      QString mesg = "exit";
      Data.append(mesg);
      qint64 res = QUdpSocket().writeDatagram(Data, QHostAddress::LocalHost, 7770);

      delete m_scriptProcess;
      m_scriptProcess = 0;

      //QMessageBox::information(0, "", "script killed");
    }
  //---------------
  
  
  Global::removeBackgroundTexture();
  Global::removeSpriteTexture();
  Global::removeSphereTexture();
  Global::removeCylinderTexture();

  
  saveSettings();

  
  GeometryObjects::captions()->clear();
  GeometryObjects::hitpoints()->clear();
  GeometryObjects::paths()->clear();
  GeometryObjects::trisets()->clear();
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

  QFont font;
  font.setPointSize(12);

  QMessageBox mesgBox(QMessageBox::Information,
		      "DrishtiMesh",
		      mesg,
		      QMessageBox::Ok);
  mesgBox.setFont(font);
  mesgBox.button(QMessageBox::Ok)->setFont(font);
  mesgBox.exec();
}

void
MainWindow::on_actionDocumentation_triggered()
{
  PropertyEditor propertyEditor;
  QMap<QString, QVariantList> plist;
  QVariantList vlist;

  vlist.clear();
  QFile helpFile(":/general.help"+Global::helpLanguage());
  if (helpFile.open(QFile::ReadOnly))
    {
      QTextStream in(&helpFile);
      in.setCodec("Utf-8");
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
  keys << "commandhelp";
  
  propertyEditor.set("General Help", plist, keys);
  propertyEditor.exec();
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
MainWindow::on_actionGrabMode_triggered(bool b)
{
  GeometryObjects::inPool = b;
  ui.actionGrabMode->setChecked(b);
  GeometryObjects::trisets()->setGrab(b);
  m_meshInfoWidget->changeSelectionMode(b);
}

void
MainWindow::on_actionSave_Image_triggered()
{
  m_Viewer->grabScreenShot();
}

void
MainWindow::on_actionSave_ImageSequence_triggered()
{
  QSize imgSize = StaticFunctions::getImageSize(m_Viewer->size().width(),
						m_Viewer->size().height());

  //m_Viewer->setImageSize(imgSize.width(), imgSize.height());

  SaveImageSeqDialog saveImg(0,
			     Global::previousDirectory(),
			     m_keyFrameEditor->startFrame(),
			     m_keyFrameEditor->endFrame(),
			     1);

  int cdW = saveImg.width();
  int cdH = saveImg.height();
  saveImg.move(QCursor::pos() - QPoint(cdW/2, cdH/2));

  if (saveImg.exec() == QDialog::Accepted)
    {
      QString flnm = saveImg.fileName();
      int imgMode = saveImg.imageMode();

      if (imgMode == Enums::CubicImageMode ||
	  imgMode == Enums::PanoImageMode)
	{
	  if (imgSize.width() != imgSize.height())
	    {
	      QMessageBox::critical(0, tr("Error"),
				    QString(tr("Current image size is %1x%2.\nSquare image size required for cubic images.")). \
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
  QSize imgSize = StaticFunctions::getImageSize(m_Viewer->size().width(),
						m_Viewer->size().height());

  m_Viewer->setImageSize(imgSize.width(), imgSize.height());

#if defined(Q_OS_WIN32)
  if (imgSize.width()%16 > 0)
//    imgSize.height()%16 > 0)
    {
      emit showMessage(QString(tr("For wmv movies, the image width must be multiple of 16. Current size is %1 x %2")). \
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

  int cdW = saveImg.width();
  int cdH = saveImg.height();
  saveImg.move(QCursor::pos() - QPoint(cdW/2, cdH/2));

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
MainWindow::on_actionTriset_triggered()
{
  QStringList meshformats;
  meshformats << "fbx";
  meshformats << "dae";
  meshformats << "gltf";
  meshformats << "glb";
  meshformats << "blend";
  meshformats << "3ds";
  meshformats << "obj";
  meshformats << "ply";
  meshformats << "stl";

  QString formats = tr("Surface Mesh ( ");
  foreach(QString fmt, meshformats)
    formats += QString("*.%1 ").arg(fmt);
  formats += ")";
  
  QStringList flnms;
  flnms = QFileDialog::getOpenFileNames(0,
					tr("Load Surface File"),
					Global::previousDirectory(),
					formats,
					0);
					//QFileDialog::DontUseNativeDialog);
  
  if (flnms.isEmpty())
    return;

  
  ui.statusBar->showMessage(tr("Loading ... "));


  foreach (QString flnm, flnms)
    GeometryObjects::trisets()->addMesh(flnm);
  
  if (Global::volumeType() == Global::DummyVolume)
    {
      int nx, ny, nz;
      GeometryObjects::trisets()->allGridSize(nx, ny, nz);
      Global::setVisualizationMode(Global::Surfaces);  
      loadDummyVolume(nx, ny, nz);
    }

  QFileInfo f(flnms[0]);
  Global::setPreviousDirectory(f.absolutePath());

  ui.statusBar->showMessage("");

  updateMeshList(GeometryObjects::trisets()->getMeshList());
  GeometryObjects::trisets()->removeFromMouseGrabberPool();

  
  ui.actionPaintMode->setChecked(false);
  m_Viewer->setPaintMode(false);

  
  Global::addRecentFile(flnms[0]);
  updateRecentFileAction();
}

void
MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
  QStringList meshformats;
  meshformats << ".fbx";
  meshformats << ".dae";
  meshformats << ".gltf";
  meshformats << ".glb";
  meshformats << ".blend";
  meshformats << ".3ds";
  meshformats << ".obj";
  meshformats << ".ply";
  meshformats << ".stl";

  if (event && event->mimeData())
    {
      const QMimeData *md = event->mimeData();
      if (md->hasUrls())
	{
	  QList<QUrl> urls = md->urls();
	  if (StaticFunctions::checkURLs(urls, ".dm"))
	    {
	      event->acceptProposedAction();
	    }
	  else if (StaticFunctions::checkURLs(urls, meshformats))
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
  QStringList meshformats;
  meshformats << ".fbx";
  meshformats << ".dae";
  meshformats << ".gltf";
  meshformats << ".glb";
  meshformats << ".blend";
  meshformats << ".3ds";
  meshformats << ".obj";
  meshformats << ".ply";
  meshformats << ".stl";

  QAction *action = qobject_cast<QAction *>(sender());
  if (action)
    {
      QString filename = action->data().toString();
      QFileInfo fileInfo(filename);
      if (! fileInfo.exists())
	{
	  QMessageBox::information(0, tr("Error"),
				   QString(tr("Cannot locate ")) +
				   filename +
				   QString(tr(" for loading")));
	  return;
	}

      if (StaticFunctions::checkExtension(filename, ".dm"))
	{
	  Global::addRecentFile(filename);
	  updateRecentFileAction();
	  createHiresLowresWindows();
	  loadProject(filename.toLatin1().data());
	}
      else if (StaticFunctions::checkExtension(filename, meshformats))
	{
	  ui.statusBar->showMessage(tr("Loading ... "));

	  GeometryObjects::trisets()->addMesh(filename);
	  
	  if (Global::volumeType() == Global::DummyVolume)
	    {
	      int nx, ny, nz;
	      GeometryObjects::trisets()->allGridSize(nx, ny, nz);
	      Global::setVisualizationMode(Global::Surfaces);  
	      loadDummyVolume(nx, ny, nz);
	    }
	  
	  QFileInfo f(filename);
	  Global::setPreviousDirectory(f.absolutePath());
	  Global::addRecentFile(filename);
	  
	  ui.statusBar->showMessage("");

	  updateMeshList(GeometryObjects::trisets()->getMeshList());
	  GeometryObjects::trisets()->removeFromMouseGrabberPool();
	}
    }

  ui.actionPaintMode->setChecked(false);
  m_Viewer->setPaintMode(false);

}

void
MainWindow::dropEvent(QDropEvent *event)
{
  if (! GlewInit::initialised())
    {
      QMessageBox::information(0, "DrishtiMesh", tr("Not yet ready to start work!"));
      return;
    }

  QStringList meshformats;
  meshformats << ".fbx";
  meshformats << ".dae";
  meshformats << ".gltf";
  meshformats << ".glb";
  meshformats << ".blend";
  meshformats << ".3ds";
  meshformats << ".obj";
  meshformats << ".ply";
  meshformats << ".stl";

  if (event && event->mimeData())
    {
      const QMimeData *data = event->mimeData();
      if (data->hasUrls())
	{
	  QUrl url = data->urls()[0];
	  QFileInfo info(url.toLocalFile());
	  if (info.exists() && info.isFile())
	    {
	      if (StaticFunctions::checkExtension(url.toLocalFile(), ".dm"))
		{
		  Global::addRecentFile(url.toLocalFile());
		  updateRecentFileAction();
		  createHiresLowresWindows();
		  loadProject(url.toLocalFile().toLatin1().data());
		}
	      else if (StaticFunctions::checkExtension(url.toLocalFile(), meshformats))
		{
		  ui.statusBar->showMessage(tr("Loading ... "));

		  QList<QUrl> urls = data->urls();
		  for(int i=0; i<urls.count(); i++)
		    GeometryObjects::trisets()->addMesh(urls[i].toLocalFile());

		  if (Global::volumeType() == Global::DummyVolume)
		    {
		      int nx, ny, nz;
		      GeometryObjects::trisets()->allGridSize(nx, ny, nz);
		      Global::setVisualizationMode(Global::Surfaces);  
		      loadDummyVolume(nx, ny, nz);
		    }

		  QFileInfo f(url.toLocalFile());
		  Global::setPreviousDirectory(f.absolutePath());
		  Global::addRecentFile(url.toLocalFile());

		  ui.statusBar->showMessage("");

		  updateMeshList(GeometryObjects::trisets()->getMeshList());
		  GeometryObjects::trisets()->removeFromMouseGrabberPool();
		}
	    }
	}
    }

  ui.actionPaintMode->setChecked(false);
  m_Viewer->setPaintMode(false);

}

void
MainWindow::loadDummyVolume(int nx, int ny, int nz)
{
  Global::setSaveImageType(Global::NoImage);

  m_keyFrame->clear();
  m_keyFrameEditor->clear();

  m_Hires->loadVolume();
  m_Hires->raise();


  m_Viewer->showFullScene();
  
  Global::enableViewerUpdate();
  MainWindowUI::changeDrishtiIcon(true);
  
  if (ui.actionPaintMode->isChecked())
    {
      ui.actionPaintMode->setChecked(false);
      m_paintMenuWidget->hide();
    }
}

void
MainWindow::loadSurfaceMesh(QString meshFile)
{
  ui.statusBar->showMessage(tr("Loading ... "));
  
  GeometryObjects::trisets()->addMesh(meshFile);
  
  if (Global::volumeType() == Global::DummyVolume)
    {
      int nx, ny, nz;
      GeometryObjects::trisets()->allGridSize(nx, ny, nz);
      Global::setVisualizationMode(Global::Surfaces);  
      loadDummyVolume(nx, ny, nz);
    }
  
  QFileInfo f(meshFile);
  Global::setPreviousDirectory(f.absolutePath());
  Global::addRecentFile(meshFile);
  
  ui.statusBar->showMessage("");
  
  updateMeshList(GeometryObjects::trisets()->getMeshList());
  GeometryObjects::trisets()->removeFromMouseGrabberPool();
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
    QDomElement de0 = doc.createElement("floatprecision");
    QDomText tn0;
    tn0 = doc.createTextNode(QString("%1").arg(Global::floatPrecision()));
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }

  {
    QDomElement de0 = doc.createElement("bgcolor");
    Vec bc = Global::backgroundColor();
    str = QString("%1 %2 %3").arg(bc.x).arg(bc.y).arg(bc.z);
    QDomText tn0 = doc.createTextNode(str);
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }

  QString homePath = QDir::homePath();
  QFileInfo settingsFile(homePath, ".drishtimesh.xml");
  QString flnm = settingsFile.absoluteFilePath();  

  QFile f(flnm.toLatin1().data());
  if (f.open(QIODevice::WriteOnly))
    {
      QTextStream out(&f);
      doc.save(out, 2);
      f.close();
    }
  else
    QMessageBox::information(0, tr("Cannot save "), flnm.toLatin1().data());
}

void
MainWindow::loadSettings()
{
  QString homePath = QDir::homePath();
  QFileInfo settingsFile(homePath, ".drishtimesh.xml");

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
      else if (dlist.at(i).nodeName() == "floatprecision")
	{
	  QString str = dlist.at(i).toElement().text();
	  Global::setFloatPrecision(str.toInt());
	}
      else if (dlist.at(i).nodeName() == "bgcolor")
	{
	  QString str = dlist.at(i).toElement().text();
	  QStringList xyz = str.split(" ");
	  float x,y,z;
	  if (xyz.size() > 0) x = xyz[0].toFloat();
	  if (xyz.size() > 1) y = xyz[1].toFloat();
	  if (xyz.size() > 2) z = xyz[2].toFloat();
	  Global::setBackgroundColor(Vec(x,y,z));
	}
    }
  updateRecentFileAction();
}

void
MainWindow::loadProject(const char* flnm)
{
  if (! GlewInit::initialised())
    {
      QMessageBox::information(0, "DrishtiMesh", tr("Not yet ready to start work!"));
      return;
    }

  Global::disableViewerUpdate();
  MainWindowUI::changeDrishtiIcon(false);

  bool keyframesVisible = m_dockKeyframe->isVisible();

  Global::setCurrentProjectFile(QString(flnm));

  QFileInfo f(flnm);
  Global::setPreviousDirectory(f.absolutePath());


  m_bricks->reset();
  GeometryObjects::clipplanes()->reset();

  m_keyFrame->clear();
  m_keyFrameEditor->clear();

  if (!Global::batchMode())
    emit showMessage(tr("Loading Project ...."), false);

  Global::setVisualizationMode(Global::Surfaces);  
  loadDummyVolume(1, 1, 1);


  m_dockKeyframe->setVisible(false);

  loadKeyFrames(flnm);
  if (!m_projectFileName.isEmpty())
    {
      Global::setPrayogMode(true);
      m_keyFrameEditor->setPrayogMode(true);
    }

  m_bricksWidget->refresh();

  m_Viewer->switchToHires();

  m_dockKeyframe->setVisible(keyframesVisible);

  m_Viewer->createImageBuffers();

  Global::enableViewerUpdate();
  MainWindowUI::changeDrishtiIcon(true);


  GeometryObjects::showGeometry = true;
  GeometryObjects::show();
  GeometryObjects::inPool = false;
  GeometryObjects::removeFromMouseGrabberPool();

  m_keyFrame->playSavedKeyFrame();
  

  if (!Global::batchMode())
    {
      emit showMessage(tr("Project loaded"), false);
    }

//  // check overlapping keyframes
//  m_keyFrame->checkKeyFrameNumbers();
//
//  {
//    m_keyFrameEditor->setPrayogMode(true);
//    ui.menubar->hide();
//    ui.toolBar->hide();
//    ui.statusBar->hide();
//    //setWindowFlags(Qt::FramelessWindowHint);
//    m_dockKeyframe->setTitleBarWidget(new QWidget());
//  }
}

void
MainWindow::saveProject(QString dmflnm)
{
  
  QFileInfo f(dmflnm);
  Global::setPreviousDirectory(f.absolutePath());
  Global::setCurrentProjectFile(QString(dmflnm));

  

  int flnmlen = dmflnm.length()+1;
  char *flnm = new char[flnmlen];
  memset(flnm, 0, flnmlen);
  memcpy(flnm, dmflnm.toLatin1().data(), flnmlen);


  QImage image = m_Viewer->grabFrameBuffer();
  image = image.scaled(100, 100);


  m_keyFrame->saveProject(m_Viewer->camera()->position(),
			  m_Viewer->camera()->orientation(),
			  m_Hires->lightInfo(),
			  m_Hires->bricks(),
			  image);


  //saveVolumeIntoProject(flnm, QString());


  saveKeyFrames(flnm);

  emit showMessage(tr("Saved Project ...."), false);
}

void
MainWindow::on_actionLoad_Project_triggered()
{
  QString flnm;
  flnm = QFileDialog::getOpenFileName(0,
				      tr("Load Project"),
				      Global::previousDirectory(),
				      "dm Files (*.dm)",
				      0);
                                      //QFileDialog::DontUseNativeDialog);


  if (flnm.isEmpty())
    return;

  Global::addRecentFile(flnm);
  updateRecentFileAction();
  loadProject(flnm.toLatin1().data());
}

void
MainWindow::on_actionSave_Project_triggered()
{
  QString flnm;
  flnm = Global::currentProjectFile();
  
  if (flnm.isEmpty())
    flnm = QFileDialog::getSaveFileName(0,
					tr("Save Project"),
					Global::previousDirectory(),
					"dm Files (*.dm)");
//					0,
//					QFileDialog::DontUseNativeDialog);


  if (flnm.isEmpty())
    return;

  if (!StaticFunctions::checkExtension(flnm, ".dm"))
    flnm += ".dm";
  
  saveProject(flnm);
}

void
MainWindow::on_actionSave_ProjectAs_triggered()
{
  QString flnm;
  flnm = QFileDialog::getSaveFileName(0,
				      tr("Save Project As"),
				      Global::previousDirectory(),
				      "Drishti Mesh Project - dm Files (*.dm)");

  if (flnm.isEmpty())
    return;

  if (!StaticFunctions::checkExtension(flnm, ".dm"))
    flnm += ".dm";
      
  saveProject(flnm.toLatin1().data());
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

  m_Viewer->updateGL();
}

void
MainWindow::softness(float val)
{
  m_Hires->updateSoftness(val);
  m_Viewer->updateGL();
}
void
MainWindow::edges(float val)
{
  m_Hires->updateEdges(val);
  m_Viewer->updateGL();
}
void
MainWindow::shadowIntensity(float val)
{
  m_Hires->updateShadowIntensity(val);
  m_Viewer->updateGL();
}
void
MainWindow::valleyIntensity(float val)
{
  m_Hires->updateValleyIntensity(val);
  m_Viewer->updateGL();
}
void
MainWindow::peakIntensity(float val)
{
  m_Hires->updatePeakIntensity(val);
  m_Viewer->updateGL();
}

void
MainWindow::updateScaling()
{
  GeometryObjects::paths()->updateScaling();
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
			int fno, QImage image)
{
  m_keyFrame->setKeyFrame(pos, rot,
			  fno,
			  m_Hires->lightInfo(),
			  m_Hires->bricks(),
			  image);

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
      QMessageBox::information(0, tr("Save Rotation Animation"),
	           tr("Rotation animation frames created in Keyframe Editor.")); 
    }
}


// called from keyframeeditor
void
MainWindow::updateParameters(bool drawBox, bool drawAxis,
			     bool shadowBox, Vec bgColor,
			     QString bgImage)
{
  Global::setShadowBox(shadowBox);
  m_globalWidget->setShadowBox(shadowBox);
  
  Global::setBackgroundColor(bgColor);

  
  if (m_Viewer->paintMode())
    GeometryObjects::trisets()->makeReadyForPainting();


  Global::setDrawBox(drawBox);
  ui.actionBoundingBox->setChecked(Global::drawBox());


  // remove all geometry from mousegrab pool
  GeometryObjects::removeFromMouseGrabberPool();

  //updateMeshList(GeometryObjects::trisets()->getMeshList());
}

void
MainWindow::loadKeyFrames(const char* flnm)
{
  QString sflnm(flnm);
  //sflnm.replace(QString(".dm"), QString(".keyframes"));


  QFileInfo fileInfo(sflnm);
  if (! fileInfo.exists())
    {
      QMessageBox::information(0, tr("Error loading Drishti file"), QString(tr("%1 not found")).arg(sflnm));
      return;
    }

  fstream fin(sflnm.toLatin1().data(), ios::binary|ios::in);

  char keyword[100];
  fin.getline(keyword, 100, 0);
  if (strcmp(keyword, "Drishti Keyframes") != 0 &&
      strcmp(keyword, "DrishtiMesh Keyframes") != 0)
    {
      QMessageBox::information(0, tr("Load Keyframes"),
			       QString(tr("Invalid .dm file : "))+sflnm);
      return;
    }

  while (!fin.eof())
    {
      fin.getline(keyword, 100, 0);
      if (strcmp(keyword, "keyframes") == 0)
	m_keyFrame->load(fin);
    }
  fin.close();
}

void
MainWindow::saveKeyFrames(const char* flnm)
{
  QString sflnm(flnm);

  fstream fout(sflnm.toLatin1().data(), ios::binary|ios::out);

  QString keyword;
  keyword = "DrishtiMesh Keyframes";
  fout.write((char*)(keyword.toLatin1().data()), keyword.length()+1);  

  m_keyFrame->save(fout);

  fout.close();
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
  on_actionExit_triggered();
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
MainWindow::on_actionSpline_PositionInterpolation_triggered()
{
  bool cpi = Global::interpolationType(Global::CameraPositionInterpolation);
  Global::setInterpolationType(Global::CameraPositionInterpolation, !cpi);
  ui.actionSpline_PositionInterpolation->setChecked(Global::interpolationType(Global::CameraPositionInterpolation));
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
MainWindow::on_actionPerspective_triggered()
{
  m_Viewer->camera()->setType(Camera::PERSPECTIVE);
  ui.actionOrthographic->setChecked(false);
  m_Viewer->updateGL();
}

void
MainWindow::on_actionOrthographic_triggered()
{
  m_Viewer->camera()->setType(Camera::ORTHOGRAPHIC);
  ui.actionPerspective->setChecked(false);
  m_Viewer->updateGL();
}

void
MainWindow::setMeshVisible(int idx, bool flag)
{
  GeometryObjects::trisets()->setShow(idx, flag);  
  m_Viewer->updateGL();
}

void
MainWindow::setMeshActive(int idx, bool flag)
{
  GeometryObjects::trisets()->setActive(idx, flag);

  if (!flag)
    GeometryObjects::trisets()->removeFromMouseGrabberPool(idx);
  else
    GeometryObjects::trisets()->addInMouseGrabberPool(idx);

  m_Viewer->updateGL();
}

void
MainWindow::updateMeshList(QStringList names)
{
  m_meshInfoWidget->setMeshes(names);

  if (GeometryObjects::trisets()->count() == 0)
    {
      if (ui.actionPaintMode->isChecked())
	{
	  ui.actionPaintMode->setChecked(false);
	  m_paintMenuWidget->hide();
	}
    }
}

void
MainWindow::updateMeshList()
{
  m_meshInfoWidget->setMeshes(GeometryObjects::trisets()->getMeshList());
}


void
MainWindow::on_actionClipPartial_triggered()
{
  GeometryObjects::trisets()->setClipPartial(ui.actionClipPartial->isChecked());
  m_Viewer->updateGL();
}


void
MainWindow::on_actionHideBlack_triggered()
{
  Global::setHideBlack(ui.actionHideBlack->isChecked());
  m_Viewer->updateGL();
}

void
MainWindow::on_actionPaintMode_triggered()
{
  m_Viewer->setPaintMode(ui.actionPaintMode->isChecked());

  if (ui.actionPaintMode->isChecked())
    {
      m_paintMenuWidget->show();
      m_paintMenuWidget->move(QCursor::pos());
    }
  else
    m_paintMenuWidget->hide();

}

void
MainWindow::createPaintMenu()
{
  m_paintMenuWidget = new QWidget(this);

  QVBoxLayout *vbox = new QVBoxLayout();
  vbox->setSpacing(0);
  vbox->setContentsMargins(0,0,0,0);
  
  QHBoxLayout *hbox = new QHBoxLayout();
  hbox->setSpacing(0);

  {
    QFrame *wid = new QFrame(m_paintMenuWidget);  
    wid->setFrameStyle(QFrame::Box);
    wid->setMinimumSize(250,250);
    wid->setMaximumSize(250,250);
    QVBoxLayout *wbox = new QVBoxLayout();
    wbox->setSpacing(0);
    wid->setLayout(wbox);

    Vec pc = m_Viewer->paintColor();
    QColor pcol = QColor(255*pc.x,
			 255*pc.y,
			 255*pc.z);
    
    DColorWheel* colorWheel = new DColorWheel(wid, 5);
    colorWheel->setColorGridSize(5);
    colorWheel->setMinimumSize(250,250);
    //colorWheel->setGeometry(0, 0, 250, 250);
    colorWheel->setColor(pcol);
    connect(colorWheel, SIGNAL(colorChanged(QColor)),
	    this, SLOT(setPaintColor(QColor)));
    vbox->addWidget(wid, 0, Qt::AlignCenter);
  }
  
  {
    float r = m_Viewer->paintRadius();
    QInputDialog *dlg = new QInputDialog(this);
    dlg->setInputMode(QInputDialog::DoubleInput);
    dlg->setLabelText("Radius");
    dlg->setDoubleValue(r);
    dlg->setDoubleRange(0.1, 50.0);
    dlg->setDoubleDecimals(1);
    dlg->setFont(QFont("MS Reference Sans Serif", 12));
    dlg->setOptions(QInputDialog::NoButtons);
    dlg->setWindowFlags(Qt::Tool);
    connect(dlg, SIGNAL(doubleValueChanged(double)),
	    m_Viewer, SLOT(setPaintRadius(double)));
    hbox->addWidget(dlg);
  }

  
  {
    float r = m_Viewer->paintAlpha();
    QInputDialog *dlg = new QInputDialog(this);
    dlg->setInputMode(QInputDialog::DoubleInput);
    dlg->setLabelText("Opacity");
    dlg->setDoubleValue(r);
    dlg->setDoubleRange(0.01, 1.0);
    dlg->setDoubleDecimals(2);
    dlg->setDoubleStep(0.01);
    dlg->setFont(QFont("MS Reference Sans Serif", 12));
    dlg->setOptions(QInputDialog::NoButtons);
    dlg->setWindowFlags(Qt::Tool);
    connect(dlg, SIGNAL(doubleValueChanged(double)),
	    m_Viewer, SLOT(setPaintAlpha(double)));
    hbox->addWidget(dlg);
  }

  {
    QInputDialog *dlg = new QInputDialog(this);
    dlg->setLabelText("Pattern");
    QStringList items;
    items << "Ridge" << "Marble" << "FBM";
    // Fractal Brownian Motion
    dlg->setComboBoxItems(items);
    dlg->setTextValue(items[0]);
    dlg->setComboBoxEditable(false);
    dlg->setFont(QFont("MS Reference Sans Serif", 12));
    dlg->setOptions(QInputDialog::NoButtons);
    dlg->setWindowFlags(Qt::Tool);
    connect(dlg, SIGNAL(textValueChanged(QString)),
	    m_Viewer, SLOT(setPaintRoughness(QString)));
    hbox->addWidget(dlg);
  }

  {
    int r = m_Viewer->paintOctave();
    QInputDialog *dlg = new QInputDialog(this);
    dlg->setInputMode(QInputDialog::IntInput);
    dlg->setLabelText("Frequency");
    dlg->setIntValue(r);
    dlg->setIntRange(1, 10);
    dlg->setIntStep(1);
    dlg->setFont(QFont("MS Reference Sans Serif", 12));
    dlg->setOptions(QInputDialog::NoButtons);
    dlg->setWindowFlags(Qt::Tool);
    connect(dlg, SIGNAL(intValueChanged(int)),
	    m_Viewer, SLOT(setPaintOctave(int)));
    hbox->addWidget(dlg);
  }

  vbox->addLayout(hbox);
  
  m_paintMenuWidget->setLayout(vbox);
  m_paintMenuWidget->setWindowFlags(Qt::Tool | Qt::CustomizeWindowHint | Qt::WindowTitleHint);
  m_paintMenuWidget->setWindowTitle("Paint Menu");
  m_paintMenuWidget->setMaximumSize(270, 400);
  m_paintMenuWidget->adjustSize();
  m_paintMenuWidget->hide();
}

void
MainWindow::setPaintColor(QColor col)
{
  m_Viewer->setPaintColor(Vec(col.redF(),
			      col.greenF(),
			      col.blueF()));
}

void
MainWindow::on_actionSplitBinarySTL_triggered()
{
  QString flnm;
  flnm = QFileDialog::getOpenFileName(0,
				      tr("Load Binary STL file to split"),
				      Global::previousDirectory(),
				      "stl Files (*.stl)",
				      0);
				      //QFileDialog::DontUseNativeDialog);
  
  
  if (flnm.isEmpty())
    return;

  StaticFunctions::splitBinarySTL(flnm);
}

void
MainWindow::registerScripts()
{
  m_scriptProcess = 0;
  
  m_scriptsTitle.clear();
  m_scriptsList.clear();
  m_jsonFileList.clear();

  // look in @executable_path/renderplugins
  QString scriptdir = qApp->applicationDirPath() +
                      QDir::separator() + "assets" +
                      QDir::separator() + "scripts" +
                      QDir::separator() + "mesh";


  
  m_scriptDir = scriptdir;

  QStringList filters;
  filters << "*.json";
  
  QPair<QMenu*, QString> pair;
  QStack< QPair<QMenu*, QString> > stack;
  QString cdir = scriptdir;
  QMenu *menu = ui.menuScripts;
  pair.first = menu;
  pair.second = cdir;
  stack.push(pair);


  QStringList menuList;
  QFont font;
  font.setPointSize(12);
  
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
		  QString jsonfile = list.at(i).absoluteFilePath();
		  QFile fl(jsonfile);
		  if (fl.open(QIODevice::ReadOnly))
		    {
		      QByteArray bytes = fl.readAll();
		      fl.close();

		      QJsonParseError jsonError;
		      QJsonDocument document = QJsonDocument::fromJson( bytes, &jsonError );
		      if (jsonError.error != QJsonParseError::NoError )
			{
			  QMessageBox::information(0, "Error",
						   QString("fromJson failed: %1"). \
						   arg(jsonError.errorString()));
			}
		      else if (document.isObject() )
			{
			  QJsonObject jsonObj = document.object(); 
			  QStringList keys = jsonObj.keys();
			  
			  QString skrpt;
			  QString title;
			  QString menuh;
			  for (auto key : keys)
			    {
			      QString value = jsonObj.take(key).toString();
			      if (!value.isEmpty())
				{
				  if (key == "executable")
				    skrpt = QFileInfo(jsonfile).completeBaseName();
				  if (key == "script")
				    skrpt = QFileInfo(jsonfile).completeBaseName();
				  if (key == "description")
				    title = value;
				  if (key == "menu_hierarchy")
				    menuh = value;
				}
			    }
			  
			  if (!skrpt.isEmpty())
			    {
			      m_scriptsList << skrpt;
			      m_jsonFileList << jsonfile;
			      if (!title.isEmpty())
				m_scriptsTitle << title;
			      else
				m_scriptsTitle << skrpt;

			      

			      QAction *actionShowPackages = new QAction(this);
			      actionShowPackages->setFont(font);
			      actionShowPackages->setText("Packages Required");
			      actionShowPackages->setData(m_scriptsList.count()-1);
			      actionShowPackages->setVisible(true);      
			      connect(actionShowPackages, SIGNAL(triggered()),
				      this, SLOT(showRequiredPackages()));

			      QAction *action = new QAction(this);
			      action->setFont(font);
			      action->setText("Execute Script");
			      //action->setText(title);
			      action->setData(m_scriptsList.count()-1);
			      action->setVisible(true);      
			      connect(action, SIGNAL(triggered()),
				      this, SLOT(loadScript()));
			  
			      if (!menuh.isEmpty())
				{
				  QStringList menus = menuh.split("->");
				  QMenu *cmenu = ui.menuScripts;
				  foreach(QString m, menus)
				    {
				      QMenu *submenu = cmenu->findChild<QMenu*>(m);
				      if (!submenu)
					{
					  submenu = new QMenu(m, cmenu);
					  submenu->setFont(font);
					  submenu->setObjectName(m);
					  cmenu->addMenu(submenu);
					}
				      cmenu = submenu;
				    }
				  //cmenu->addAction(actionShowPackages);
				  //cmenu->addAction(action);

				  
				  QMenu *submenu = new QMenu(title, cmenu);
				  submenu->setFont(font);
				  submenu->setObjectName(title);
				  submenu->addAction(actionShowPackages);			      
				  submenu->addAction(action);
				  cmenu->addMenu(submenu);
				}
			      else
				{
				  QMenu *cmenu = ui.menuScripts;
				  
				  QMenu *submenu = new QMenu(title, cmenu);
				  submenu->setFont(font);
				  submenu->setObjectName(title);
				  submenu->addAction(actionShowPackages);			      
				  submenu->addAction(action);
				  cmenu->addMenu(submenu);

				  //ui.menuScripts->addAction(actionShowPackages);			      
				  //ui.menuScripts->addAction(action);
				}
			    }
			} // valid json
		    } // read json file
		} // not a directory
	    } // loop over all directory entries
	}
    } // while loop
}

void
MainWindow::showRequiredPackages()
{
  QAction *action = qobject_cast<QAction *>(sender());
  int idx = action->data().toInt();

  QStringList packages;


  QString jsonfile = m_jsonFileList[idx];
  QFile fl(jsonfile);
  if (fl.open(QIODevice::ReadOnly))
    {
      QByteArray bytes = fl.readAll();
      fl.close();
      
      QJsonParseError jsonError;
      QJsonDocument document = QJsonDocument::fromJson( bytes, &jsonError );
      if (jsonError.error != QJsonParseError::NoError )
	{
	  QMessageBox::information(0, "Error",
				   QString("fromJson failed: %1").	\
				   arg(jsonError.errorString()));
	  return;
	}

      if (document.isObject() )
	{
	  QJsonObject jsonObj = document.object(); 
	  QStringList keys = jsonObj.keys();
	  for (auto key : keys)
	    {
	      auto value = jsonObj.take(key);

	      if (key == "packages")
		packages = value.toVariant().toStringList();
	    }
	}
    } 

  if (packages.count() > 0)
    {
      QString pkgs = packages.join("\n");
      QFont font;
      font.setPointSize(14);

      QMessageBox mesgBox(QMessageBox::Information,
			  "Packages required to run the script",
			  pkgs,
			  QMessageBox::Ok);
      mesgBox.setFont(font);
      mesgBox.button(QMessageBox::Ok)->setFont(font);
      mesgBox.exec();
    }
  else
    QMessageBox::information(0, "Packages required to run the script",
			     "No package information found in the json file.");
}


void
MainWindow::loadScript()
{
  QAction *action = qobject_cast<QAction *>(sender());
  int idx = action->data().toInt();
  
  runScript(idx, false);
}

void
MainWindow::runScript(int idx, bool batchMode)
{
  QString script;
  QString executable;
  QString interpreter;


  QString jsonfile = m_jsonFileList[idx];
  QFile fl(jsonfile);
  if (fl.open(QIODevice::ReadOnly))
    {
      QByteArray bytes = fl.readAll();
      fl.close();
      
      QJsonParseError jsonError;
      QJsonDocument document = QJsonDocument::fromJson( bytes, &jsonError );
      if (jsonError.error != QJsonParseError::NoError )
	{
	  QMessageBox::information(0, "Error",
				   QString("fromJson failed: %1").	\
				   arg(jsonError.errorString()));
	  return;
	}

      if (document.isObject() )
	{
	  QJsonObject jsonObj = document.object(); 
	  QStringList keys = jsonObj.keys();
	  for (auto key : keys)
	    {
	      auto value = jsonObj.take(key);

//	      if (key == "arguments")
//		{
//		  QJsonObject obj = value.toObject();
//		  QStringList keys = obj.keys();
//		  for (auto key : keys)
//		    {
//		      auto value = obj.take(key);
//		      if (value.isString())
//			addRow(key, value.toString());
//		      else
//			addRow(key, QString("%1").arg(value.toDouble()));
//		    }
//		}
	      if (key == "executable")
		executable = value.toString();		
	      if (key == "interpreter")
		interpreter = value.toString();
	      if (key == "script")
		script = m_scriptDir + QDir::separator() +
		         m_scriptsList[idx] + QDir::separator() +
		         value.toString();  
	    }	  
	}
    }

  //-----------------------
  QList<int> indices;
  if (GeometryObjects::trisets()->count() == 1)
    {
      indices << 0;
    }
  else
    {
      indices = m_meshInfoWidget->getSelectedMeshes();
      if (indices.count() == 0)
	{
	  QMessageBox::information(0, "Error", "No mesh selected");
	  return;
	}
    }
  
  
  QStringList selected;
  for(int i=0; i<indices.count(); i++)
    {
      selected << QString("\"%1\"").arg(GeometryObjects::trisets()->filename(indices[i]));
    }

  QString meshes = selected.join(" ");
  //-----------------------

  
  QString cmd;
  
  if (!executable.isEmpty())
    cmd = QString("\"%1\"").arg(executable)+" "+meshes;
  else if (!interpreter.isEmpty() &&
	   !script.isEmpty())
    cmd = interpreter+" "+script+" "+meshes;
  

  
  if (m_scriptProcess == 0)
    m_scriptProcess = new QProcess(this);
    
  if (m_scriptProcess->processId() != 0)
    {
      QByteArray Data;
      QString mesg = "exit";
      Data.append(mesg);
      qint64 res = QUdpSocket().writeDatagram(Data, QHostAddress::LocalHost, 7770);

      delete m_scriptProcess;

      m_scriptProcess = new QProcess();
    }

  QStringList args;
  args << cmd.toLatin1();
  
  //----------
#if defined(Q_OS_WIN32)
  m_scriptProcess->start("cmd.exe ");
#else
  m_scriptProcess->start("/bin/bash ");
#endif  
  
  QMessageBox::information(0, "CMD", cmd);
  
  m_scriptProcess->write(cmd.toLatin1() + "\n");
  qApp->processEvents();
}


void
MainWindow::updateVoxelUnit()
{
  m_Viewer->updateGL();
  qApp->processEvents();
}

void
MainWindow::updateVoxelSize()
{
  GeometryObjects::paths()->updateVoxelSize();
  m_Viewer->updateGL();
  qApp->processEvents();
}
