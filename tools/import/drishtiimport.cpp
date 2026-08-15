#include <pybind11/pybind11.h>

#include "global.h"
#include "staticfunctions.h"
#include "drishtiimport.h"
#include "remapwidget.h"
#include "fileslistdialog.h"
#include "raw2pvl.h"
#include "tiffinputrouting.h"

#include <QFile>
#include <QTextStream>
#include <QDomDocument>
#include <QTreeView>
#include <QTableWidget>


DrishtiImport::DrishtiImport(QWidget *parent) :
  QMainWindow(parent)
{
  ui.setupUi(this);
  
  resize(1280, 1024);
  qApp->setFont(QFont("MS Reference Sans Serif", 12));
  

  setWindowIcon(QPixmap(":/images/drishti_import_32.png"));
  setWindowTitle(QString("DrishtiImport v") + QString(DRISHTI_VERSION));

  setAcceptDrops(true);

  m_remapWidget = new RemapWidget();
  setCentralWidget(m_remapWidget);

  StaticFunctions::initQColorDialog();

  loadSettings();

  registerPlugins();

  Global::setStatusBar(statusBar());
}

void
DrishtiImport::registerPlugins()
{
  m_pluginFileTypes.clear();
  m_pluginFileDLib.clear();
  m_pluginDirTypes.clear();
  m_pluginDirDLib.clear();

  QString plugindir = qApp->applicationDirPath() + QDir::separator() + "importplugins";
  QStringList filters;

#if defined(Q_OS_WIN32)
  filters << "*.dll";
#endif
#ifdef Q_OS_MACX
  // look in drishti.app/importplugins
  QString sep = QDir::separator();
  plugindir = qApp->applicationDirPath()+sep+".."+sep+".."+sep+"importplugins";
  filters << "*.dylib";
#endif
#if defined(Q_OS_LINUX)
  filters << "*.so";
#endif

  QDir dir(plugindir);
  dir.setFilter(QDir::Files);

  dir.setNameFilters(filters);
  QFileInfoList list = dir.entryInfoList();

  if (list.size() == 0)
    {
      QMessageBox::information(0, "Error", QString("No plugins found in %1").arg(plugindir));
      close();
    }

  m_scriptsPlugin = "";
    
  for (int i=0; i<list.size(); i++)
    {
      QString pluginflnm = list.at(i).absoluteFilePath();

      QPluginLoader pluginLoader(pluginflnm);
      QObject *plugin = pluginLoader.instance();
      if (plugin)
	{
	  VolInterface *vi = qobject_cast<VolInterface *>(plugin);
	  if (vi)
	    {
	      QStringList rs = vi->registerPlugin();

	      int idx = rs.indexOf("script");
	      if (idx >= 0)
		m_scriptsPlugin = pluginflnm;
	      
	      
	      idx = rs.indexOf("file");
	      if (idx == -1) idx = rs.indexOf("files");
	      if (idx >= 0)
		{
		  if (rs.size() > idx+1)
		    {
		      m_pluginFileTypes << rs[idx+1];
		      m_pluginFileDLib << pluginflnm;
		    }
		  else
		    QMessageBox::information(0, "Error",
		      QString("Received illegal files register string [%1] for plugin [%2]").\
					     arg(rs.join(" ")).arg(pluginflnm));

		}

	      idx = rs.indexOf("dir");
	      if (idx == -1) idx = rs.indexOf("directory");
	      if (idx >= 0)
		{
		  QPair<QString, QStringList> ft;
		  if (rs.size() > idx+1)
		    {
		      m_pluginDirTypes << rs[idx+1];
		      m_pluginDirDLib << pluginflnm;
		    }
		  else
		    QMessageBox::information(0, "Error",
		    QString("Received illegal directory register string [%1] for plugin [%2]").\
					     arg(rs.join(" ")).arg(pluginflnm));

		}
	    }
	}
      else
	{
	  QMessageBox::information(0, "Error", QString("Cannot load %1").arg(pluginflnm));
	}
    }

  //---------------------
  // load external scripts if any
  registerExternalScripts();
  //---------------------

  
  QMenu *loadDirMenu;
  QMenu *loadFileMenu;

  if (m_pluginDirTypes.size() > 0)
    loadDirMenu = ui.menuLoad->addMenu("Directory");

  if (m_pluginFileTypes.size() > 0)
    loadFileMenu = ui.menuLoad->addMenu("Files");
  
  for (int i=0; i<m_pluginDirTypes.size(); i++)
    {
      QAction *action = new QAction(this);
      action->setText(m_pluginDirTypes[i]);
      action->setData(m_pluginDirTypes[i]);
      action->setVisible(true);      
      connect(action, SIGNAL(triggered()),
	      this, SLOT(loadDirectory()));
      loadDirMenu->addAction(action);
    }

  for (int i=0; i<m_pluginFileTypes.size(); i++)
    {
      QAction *action = new QAction(this);
      action->setText(m_pluginFileTypes[i]);
      action->setData(m_pluginFileTypes[i]);
      action->setVisible(true);      
      connect(action, SIGNAL(triggered()),
	      this, SLOT(loadFiles()));
      loadFileMenu->addAction(action);
    }
}

void
DrishtiImport::registerExternalScripts()
{
  QStringList scripts;

  const QString scriptDir =
    QDir(QCoreApplication::applicationDirPath())
      .filePath(QStringLiteral("assets/scripts/import"));
  
  QDir topDir(scriptDir);
  if (!topDir.exists())
    {
      qWarning().noquote() << "Optional import scripts not found under"
                           << QDir::toNativeSeparators(scriptDir);
      return;
    }

  topDir.setFilter(QDir::Dirs | QDir::Readable | QDir::NoDotAndDotDot);
  topDir.setSorting(QDir::Name);

  QFileInfoList scriptD = topDir.entryInfoList();
  for (int i=0; i<scriptD.count(); i++)
    {
      QString jsonfile = scriptD[i].fileName();
      jsonfile += ".json";
      QDir pdir(scriptD[i].absoluteFilePath());
      if (pdir.exists(jsonfile))
	{
	  jsonfile = scriptD[i].absoluteFilePath() + QDir::separator() + jsonfile;
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

		  QString filesDesc, dirDesc;
		  
		  QString skrpt;
		  for (auto key : keys)
		    {
		      QString value = jsonObj.take(key).toString();
		      if (!value.isEmpty())
			{
			  if (key == "executable")
			    skrpt = scriptD[i].fileName();
			  if (key == "script")
			    skrpt = scriptD[i].fileName();

			  if (key == "filetype") filesDesc = value;
			  if (key == "dirtype") dirDesc = value;			  
			}
		    }
		  if (!skrpt.isEmpty())
		    {
		      if (!filesDesc.isEmpty())
			{
			  m_pluginFileTypes << filesDesc;
			  m_pluginFileDLib << "script : "+m_scriptsPlugin+" : "+jsonfile;
			}
		      if (!dirDesc.isEmpty())
			{
			  m_pluginDirTypes << dirDesc;
			  m_pluginDirDLib << "script : "+m_scriptsPlugin+" : "+jsonfile;
			}
			
		      scripts << skrpt;
		    }
		}
	    }
	}
    }

  if (scripts.count() == 0)
    qWarning().noquote() << "No valid import scripts found under"
                         << QDir::toNativeSeparators(scriptDir);
}


void
DrishtiImport::loadFiles()
{
  QAction *action = qobject_cast<QAction *>(sender());
  if (!action)
    return;

  QString plugin = action->data().toString();
  int idx = m_pluginFileTypes.indexOf(plugin);

  QStringList flnms;
#ifndef Q_OS_MACX
  flnms = QFileDialog::getOpenFileNames(0,
					"Load files",
					Global::previousDirectory(),
					QString("%1 (*)").arg(plugin),
					0,
					QFileDialog::DontUseNativeDialog);
#else
  flnms = QFileDialog::getOpenFileNames(0,
					"Load files",
					Global::previousDirectory(),
					QString("%1 (*)").arg(plugin),
					0);
#endif
  
  if (flnms.size() == 0)
    return;

  loadFiles(flnms, idx);
}

void
DrishtiImport::loadFiles(QStringList flnms,
			 int pluginidx)
{
  if (flnms.isEmpty())
    return;

  if (flnms.count() > 1)
    {
      FilesListDialog fld(flnms);
      fld.exec();
      if (fld.result() == QDialog::Rejected)
	return;
      flnms = fld.files();
    }

  QFileInfo f(flnms[0]);
  Global::setPreviousDirectory(f.absolutePath());

  int idx = pluginidx;

  if (idx == -1)
    {
      QStringList ftypes;
      for(int i=0; i<m_pluginFileTypes.size(); i++)
	{
	  ftypes << QString("%1 : %2").		\
	    arg(i+1).arg(m_pluginFileTypes[i]);
	}

      bool accepted = false;
      QString option = QInputDialog::getItem(0,
					     "Select File Type",
					     "File Type",
					     ftypes,
					     0,
					     false, &accepted);
      if (!accepted)
	return;
  
      idx = ftypes.indexOf(option);

      if (idx == -1)
	{
	  QMessageBox::information(0, "Error",
				   QString("No plugin found for %1").arg(option));
	  return;
	}
    }

  if (idx >= 0)
    {
      if (idx >= m_pluginFileDLib.size())
	{
	  QMessageBox::information(0, "Error", "The selected file plugin is unavailable");
	  return;
	}

      const int routedIndex = TiffInputRouting::routedPluginIndex(
	idx, m_pluginFileTypes, m_pluginFileDLib, flnms, false);
      if (routedIndex != idx)
	{
	  qInfo().noquote()
	    << "Routing an all-TIFF file selection through the native TIFF importer.";
	  idx = routedIndex;
	}

      if (!m_remapWidget->setFile(flnms, m_pluginFileDLib[idx]))
	return;
    }

  if (ui.action8_bit->isChecked())
    m_remapWidget->setPvlMapMax(255);
  else
    m_remapWidget->setPvlMapMax(65535);
}

void
DrishtiImport::loadDirectory()
{
  QAction *action = qobject_cast<QAction *>(sender());
  if (!action)
    return;

  QString plugin = action->data().toString();
  int idx = m_pluginDirTypes.indexOf(plugin);

  if (action)
    {
      QString dirname;
      dirname = QFileDialog::getExistingDirectory(0,
						  "Directory",
						  Global::previousDirectory(),
						  QFileDialog::ShowDirsOnly |
						  QFileDialog::DontResolveSymlinks |
						  QFileDialog::DontUseNativeDialog);
  
      if (dirname.size() == 0)
	return;

      loadDirectory(dirname, idx);
    }
}

void
DrishtiImport::loadDirectory(QString dirname, int pluginidx)
{
  if (dirname.isEmpty())
    return;

  QFileInfo f(dirname);
  Global::setPreviousDirectory(f.absolutePath());

  QStringList flnms;
  flnms << dirname;

  int idx = pluginidx;

  if (idx == -1)
    {
      QStringList dtypes;
      for(int i=0; i<m_pluginDirTypes.size(); i++)
	{
	  dtypes << QString("%1 : %2").		\
	    arg(i+1).arg(m_pluginDirTypes[i]);
	}

      bool accepted = false;
      QString option = QInputDialog::getItem(0,
					     "Select Directory Type",
					     "Directory Type",
					     dtypes,
					     0,
					     false, &accepted);
      if (!accepted)
	return;
      idx = dtypes.indexOf(option);
      
      if (idx == -1)
	{
	  QMessageBox::information(0, "Error",
				   QString("No plugin found for %1").arg(option));
	  return;
	}
    }

  if (idx >= 0)
    {
      if (idx >= m_pluginDirDLib.size())
	{
	  QMessageBox::information(0, "Error", "The selected directory plugin is unavailable");
	  return;
	}

      const int routedIndex = TiffInputRouting::routedPluginIndex(
	idx, m_pluginDirTypes, m_pluginDirDLib, flnms, true);
      if (routedIndex != idx)
	{
	  qInfo().noquote()
	    << "Routing an all-TIFF directory through the native TIFF importer.";
	  idx = routedIndex;
	}

      if (!m_remapWidget->setFile(flnms, m_pluginDirDLib[idx]))
	return;
    }

  if (ui.action8_bit->isChecked())
    m_remapWidget->setPvlMapMax(255);
  else
    m_remapWidget->setPvlMapMax(65535);
}

void
DrishtiImport::on_saveLimits_triggered()
{
  m_remapWidget->saveLimits();
}

void
DrishtiImport::on_loadLimits_triggered()
{
  m_remapWidget->loadLimits();
}

void
DrishtiImport::on_saveImage_triggered()
{
  m_remapWidget->saveImage();
}

void
DrishtiImport::loadSettings()
{
  QString homePath = QDir::homePath();
  QFileInfo settingsFile(homePath, ".drishti.import");
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
  for(uint i=0; i<dlist.count(); i++)
    {
      if (dlist.at(i).nodeName() == "previousdirectory")
	{
	  QString str = dlist.at(i).toElement().text();
	  Global::setPreviousDirectory(str);
	}
    }
}

void
DrishtiImport::saveSettings()
{
  QString str;
  QDomDocument doc("Drishti_Import_v1.0");

  QDomElement topElement = doc.createElement("DrishtiImportSettings");
  doc.appendChild(topElement);

  {
    QDomElement de0 = doc.createElement("previousdirectory");
    QDomText tn0;
    tn0 = doc.createTextNode(Global::previousDirectory());
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }

  QString homePath = QDir::homePath();
  QFileInfo settingsFile(homePath, ".drishti.import");
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
DrishtiImport::on_action8_bit_triggered()
{
  if (ui.action16_bit->isChecked())
    ui.action16_bit->setChecked(false);
  else if (! ui.action8_bit->isChecked())
    ui.action16_bit->setChecked(true);

  if (ui.action8_bit->isChecked())
    m_remapWidget->setPvlMapMax(255);
  else
    m_remapWidget->setPvlMapMax(65535);
}

void
DrishtiImport::on_action16_bit_triggered()
{
  if (ui.action8_bit->isChecked())
    ui.action8_bit->setChecked(false);
  else if (! ui.action16_bit->isChecked())
    ui.action8_bit->setChecked(true);

  if (ui.action8_bit->isChecked())
    m_remapWidget->setPvlMapMax(255);
  else
    m_remapWidget->setPvlMapMax(65535);
}

void
DrishtiImport::on_actionMergeVolumes_triggered()
{
  QStringList ftypes;
//  for(int i=0; i<m_pluginDirTypes.size(); i++)
//    {
//      ftypes << QString("%1 : %2").		\
//	arg(i+1).arg(m_pluginDirTypes[i]);
//    }
  for(int i=0; i<m_pluginFileTypes.size(); i++)
    {
      ftypes << QString("%1 : %2").		\
	arg(i+1).arg(m_pluginFileTypes[i]);
    }

  bool accepted = false;
  QString option = QInputDialog::getItem(0,
					 "Select File Type",
					 "File Type",
					 ftypes,
					 0,
					 false, &accepted);
  if (!accepted)
    return;
  
  int idx = ftypes.indexOf(option);
  
  if (idx == -1)
    {
      QMessageBox::information(0, "Error", "No file type selected");
      return;
    }

  m_remapWidget->handleMergeVolumes(m_pluginFileTypes[idx],
				    m_pluginFileDLib[idx]);

//  if (idx >= m_pluginDirTypes.size())
//    {
//      idx -= m_pluginDirTypes.size();
//      m_remapWidget->handleTimeSeries(m_pluginFileTypes[idx],
//				      m_pluginFileDLib[idx]);
//    }
//  else
//    {
//      m_remapWidget->handleTimeSeries(m_pluginDirTypes[idx],
//				      m_pluginDirDLib[idx]);
//    }
//  
}

void
DrishtiImport::on_actionTimeSeries_triggered()
{
  QStringList ftypes;
  for(int i=0; i<m_pluginFileTypes.size(); i++)
    {
      ftypes << QString("%1 : %2").		\
	arg(i+1).arg(m_pluginFileTypes[i]);
    }

  bool accepted = false;
  QString option = QInputDialog::getItem(0,
					 "Select File Type",
					 "File Type",
					 ftypes,
					 0,
					 false, &accepted);
  if (!accepted)
    return;
  
  int idx = ftypes.indexOf(option);
  
  if (idx == -1)
    {
      QMessageBox::information(0, "Error", "No file type selected");
      return;
    }

  m_remapWidget->handleTimeSeries(m_pluginFileTypes[idx],
				  m_pluginFileDLib[idx]);
}

void
DrishtiImport::on_actionSave_As_triggered()
{
  m_remapWidget->saveAs();
}

void
DrishtiImport::on_actionBatch_Process_triggered()
{
  m_remapWidget->batchProcess();
}


void
DrishtiImport::on_actionSave_Images_triggered()
{
  m_remapWidget->saveImages();
}

void
DrishtiImport::on_actionExit_triggered()
{
  close();
}

void
DrishtiImport::closeEvent(QCloseEvent *event)
{
  saveSettings();
  QMainWindow::closeEvent(event);
}

void
DrishtiImport::dragEnterEvent(QDragEnterEvent *event)
{
  if (event && event->mimeData())
    {
      const QMimeData *data = event->mimeData();
      if (data->hasUrls())
	  event->acceptProposedAction();
    }
}

void
DrishtiImport::dropEvent(QDropEvent *event)
{
  if (event && event->mimeData())
    {
      const QMimeData *data = event->mimeData();
      if (data->hasUrls())
	{
	  QUrl url = data->urls()[0];
	  QFileInfo info(url.toLocalFile());

	  // handle directories
	  if (info.isDir())
	    {
	      if (data->urls().count() == 1)
		{
		  loadDirectory(url.toLocalFile(), -1);
		}
	      else
		{
		  QStringList flnms;
		  for(uint i=0; i<data->urls().count(); i++)
		    flnms << (data->urls())[i].toLocalFile();
		  convertDirectories(flnms, -1);
		}
	    }

	  // handle files
	  if (info.exists() && info.isFile())
	    {
	      QStringList flnms;
	      for(uint i=0; i<data->urls().count(); i++)
		flnms << (data->urls())[i].toLocalFile();

	      loadFiles(flnms, -1);
	    }
	}
    }
}


void
DrishtiImport::convertDirectories(QStringList flnms, int pluginidx)
{
  if (flnms.count() > 1)
    {
      FilesListDialog fld(flnms);
      fld.exec();
      if (fld.result() == QDialog::Rejected)
	return;
      flnms = fld.files();
    }

  QFileInfo f(flnms[0]);
  Global::setPreviousDirectory(f.absolutePath());

  int idx = pluginidx;

  if (idx == -1)
    {
      QStringList dtypes;
      for(int i=0; i<m_pluginDirTypes.size(); i++)
	{
	  dtypes << QString("%1 : %2").		\
	    arg(i+1).arg(m_pluginDirTypes[i]);
	}

      bool accepted = false;
      QString option = QInputDialog::getItem(0,
					     "Select Directory Type",
					     "Directory Type",
					     dtypes,
					     0,
					     false, &accepted);
      if (!accepted)
	return;
      idx = dtypes.indexOf(option);
      
      if (idx == -1)
	{
	  QMessageBox::information(0, "Error",
				   QString("No plugin found for %1").arg(option));
	  return;
	}
    }

  if (idx >= 0)
    {
      if (idx >= m_pluginDirDLib.size())
	{
	  QMessageBox::information(0, "Error", "The selected directory plugin is unavailable");
	  return;
	}
      QProgressDialog progress("Processing ",
			       "Cancel",
			       0, 100,
			       this,
			       Qt::Dialog|Qt::WindowStaysOnTopHint);

      progress.setMinimumDuration(0);


      for (int i=0; i<flnms.count(); i++)
	{
	  if (progress.wasCanceled())
	    {
	      progress.setValue(100);  
	      QMessageBox::information(0, "Save", "-----Aborted-----");
	      break;
	    }
      
	  QStringList dnames;
	  dnames << flnms[i];

	  progress.setLabelText(flnms[i]);
	  qApp->processEvents();

	  if (!m_remapWidget->setFile(dnames, m_pluginDirDLib[idx]))
	    {
	      progress.setValue(100);
	      return;
	    }

	  progress.setValue(30);
	  qApp->processEvents();

	  if (ui.action8_bit->isChecked())
	    m_remapWidget->setPvlMapMax(255);
	  else
	    m_remapWidget->setPvlMapMax(65535);

	  progress.setValue(50);
	  qApp->processEvents();

	  if (!m_remapWidget->saveQuickRaw())
	    {
	      progress.setValue(100);
	      return;
	    }
	}

      progress.setValue(100);
      qApp->processEvents();

      //QMessageBox::information(0, "", "Converted all to raw");
    }
}

void
DrishtiImport::on_actionMimics_triggered()
{
  QStringList flnms;

  QFileDialog w;
  w.setDirectory(Global::previousDirectory());
  w.setFileMode(QFileDialog::DirectoryOnly);
  w.setOption(QFileDialog::DontUseNativeDialog, true);
  QListView *l = w.findChild<QListView*>("listView");
  if (l)
    l->setSelectionMode(QAbstractItemView::ExtendedSelection);
  QTreeView *t = w.findChild<QTreeView*>("treeView");
  if (t)
    t->setSelectionMode(QAbstractItemView::ExtendedSelection);
  
  if (w.exec() != QDialog::Accepted)
    return;

  flnms = w.selectedFiles();
    
  if (flnms.size() == 0)
    return;

  
  QProgressDialog progress("Processing ",
			       "Cancel",
			   0, 100,
			   this,
			   Qt::Dialog|Qt::WindowStaysOnTopHint);

  progress.setMinimumDuration(0);
  qApp->processEvents();

  int idx = m_pluginDirTypes.indexOf("DICOM Image Directory");
  if (idx < 0 || idx >= m_pluginDirDLib.size())
    {
      QMessageBox::information(0, "Error", "The DICOM directory plugin is unavailable");
      return;
    }

  QStringList rawFiles;
  const auto removeTemporaryRawFiles = [&rawFiles]()
    {
      for (const QString &fileName : rawFiles)
	QFile::remove(fileName);
      rawFiles.clear();
    };

  float vx, vy, vz;
  
  //-------------------
  // convert Dicom to raw
  for (int i=0; i<flnms.count(); i++)
    {
      if (progress.wasCanceled())
	{
	  progress.setValue(100);  
	  QMessageBox::information(0, "Save", "-----Aborted-----");
	  removeTemporaryRawFiles();
	  return;
	}
	  
      QStringList dnames;
      dnames << flnms[i];
      
      QFileInfo fraw(flnms[i]);
      const QString rawFile = QFileInfo(fraw.absolutePath(),
					fraw.baseName() + ".raw").absoluteFilePath();
      if (QFileInfo::exists(rawFile))
	{
	  removeTemporaryRawFiles();
	  QMessageBox::critical(0, "Mimics Conversion",
	    QString("Refusing to overwrite an existing RAW file: %1").arg(rawFile));
	  return;
	}
  

      progress.setLabelText(flnms[i]);
      qApp->processEvents();
      
      if (!m_remapWidget->setFile(dnames, m_pluginDirDLib[idx]))
	{
	  progress.setValue(100);
	  removeTemporaryRawFiles();
	  return;
	}

      if (i == 0)
	{ // use saved voxel information while merging volumes
	  m_remapWidget->saveVoxelInfo();
	}
      
      progress.setValue(30);
      qApp->processEvents();
      
      if (ui.action8_bit->isChecked())
	m_remapWidget->setPvlMapMax(255);
      else
	m_remapWidget->setPvlMapMax(65535);
      
      progress.setValue(50);
      qApp->processEvents();
      
	  if (!m_remapWidget->saveQuickRaw())
	{
	  progress.setValue(100);
	  removeTemporaryRawFiles();
	  return;
	}
      QFileInfo generatedRaw(rawFile);
      if (!generatedRaw.exists() || !generatedRaw.isFile() ||
	  generatedRaw.size() <= 13)
	{
	  removeTemporaryRawFiles();
	  QMessageBox::critical(0, "Mimics Conversion",
	    QString("RAW conversion did not produce a complete output: %1")
	      .arg(rawFile));
	  return;
	}
      rawFiles << rawFile;
  }
  
  progress.setValue(100);
  qApp->processEvents();
  //-------------------

  
  //-------------------
  // process the saved raw files  
  idx = m_pluginFileTypes.indexOf("RAW Files");
  if (idx < 0 || idx >= m_pluginFileDLib.size())
    {
      removeTemporaryRawFiles();
      QMessageBox::information(0, "Error", "The RAW file plugin is unavailable");
      return;
    }

  if (!m_remapWidget->mergeVolumes(m_pluginFileTypes[idx],
				   m_pluginFileDLib[idx],
				   rawFiles))
    {
      removeTemporaryRawFiles();
      return;
    }

  if (!m_remapWidget->saveAs())
    {
      QMessageBox::warning(
	0, "Mimics Conversion",
	"The merged output was not completed. Temporary RAW volumes were kept "
	"so the Save operation can be retried:\n\n" + rawFiles.join("\n"));
      return;
    }
  //---------------------

  
  //---------------------
  // remove temporary raw files
  //foreach(QString flnm, rawFiles)
  removeTemporaryRawFiles();
  //---------------------
}

void
DrishtiImport::on_actionExport_Mesh_triggered()
{
  m_remapWidget->saveIsosurfaceAs();
}

