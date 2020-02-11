#include "global.h"
#include "staticfunctions.h"
#include "drishtiimport.h"
#include "remapwidget.h"
#include "fileslistdialog.h"
#include "raw2pvl.h"

#include <QFile>
#include <QTextStream>
#include <QDomDocument>
#include <QTreeView>


DrishtiImport::DrishtiImport(QWidget *parent) :
  QMainWindow(parent)
{
  ui.setupUi(this);

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

	      int idx = rs.indexOf("file");
	      if (idx == -1) idx = rs.indexOf("files");
	      if (idx >= 0)
		{
		  if (rs.size() >= idx+1)
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
		  if (rs.size() >= idx+1)
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
DrishtiImport::loadFiles()
{
  QAction *action = qobject_cast<QAction *>(sender());
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
  if (flnms.count() > 1)
    {
      FilesListDialog fld(flnms);
      fld.exec();
      if (fld.result() == QDialog::Rejected)
	return;
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

      QString option = QInputDialog::getItem(0,
					     "Select File Type",
					     "File Type",
					     ftypes,
					     0,
					     false);
  
      idx = ftypes.indexOf(option);

      if (idx == -1)
	{
	  QMessageBox::information(0, "Error",
				   QString("No plugin found for %1").arg(option));
	  return;
	}
    }

  if (idx >= 0)
    m_remapWidget->setFile(flnms, m_pluginFileDLib[idx]);

  if (ui.action8_bit->isChecked())
    m_remapWidget->setPvlMapMax(255);
  else
    m_remapWidget->setPvlMapMax(65535);
}

void
DrishtiImport::loadDirectory()
{
  QAction *action = qobject_cast<QAction *>(sender());
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

      QString option = QInputDialog::getItem(0,
					     "Select Directory Type",
					     "Directory Type",
					     dtypes,
					     0,
					     false);  
      idx = dtypes.indexOf(option);
      
      if (idx == -1)
	{
	  QMessageBox::information(0, "Error",
				   QString("No plugin found for %1").arg(option));
	  return;
	}
    }

  if (idx >= 0)
    m_remapWidget->setFile(flnms, m_pluginDirDLib[idx]);

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
  QFile f(flnm.toLatin1().data());
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

  QString option = QInputDialog::getItem(0,
					 "Select File Type",
					 "File Type",
					 ftypes,
					 0,
					 false);
  
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

  QString option = QInputDialog::getItem(0,
					 "Select File Type",
					 "File Type",
					 ftypes,
					 0,
					 false);
  
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
DrishtiImport::on_actionSave_Isosurface_As_triggered()
{
  m_remapWidget->saveIsosurfaceAs();
}

void
DrishtiImport::on_actionSave_Images_triggered()
{
  m_remapWidget->saveImages();
}

void
DrishtiImport::on_actionExit_triggered()
{
  saveSettings();
  close();
}

void
DrishtiImport::closeEvent(QCloseEvent *)
{
  on_actionExit_triggered();
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

      QString option = QInputDialog::getItem(0,
					     "Select Directory Type",
					     "Directory Type",
					     dtypes,
					     0,
					     false);  
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
      QProgressDialog progress("Processing ",
			       "Cancel",
			       0, 100,
			       0);
      progress.setMinimumDuration(0);


      for (int i=0; i<flnms.count(); i++)
	{
	  QStringList dnames;
	  dnames << flnms[i];

	  progress.setLabelText(flnms[i]);
	  qApp->processEvents();

	  m_remapWidget->setFile(dnames, m_pluginDirDLib[idx]);

	  progress.setValue(30);
	  qApp->processEvents();

	  if (ui.action8_bit->isChecked())
	    m_remapWidget->setPvlMapMax(255);
	  else
	    m_remapWidget->setPvlMapMax(65535);

	  progress.setValue(50);
	  qApp->processEvents();

	  m_remapWidget->saveTrimmed(0,0,0, 0,0,0);
	}

      progress.setValue(100);
      qApp->processEvents();

      QMessageBox::information(0, "", "Converted all to raw");
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
  l->setSelectionMode(QAbstractItemView::ExtendedSelection);
  QTreeView *t = w.findChild<QTreeView*>("treeView");
  t->setSelectionMode(QAbstractItemView::ExtendedSelection);
  
  if (w.exec() != QDialog::Accepted)
    return;

  flnms = w.selectedFiles();
    
  if (flnms.size() == 0)
    return;

  
  QProgressDialog progress("Processing ",
			       "Cancel",
			   0, 100,
			   0);
  progress.setMinimumDuration(0);
  qApp->processEvents();

  int idx = m_pluginDirTypes.indexOf("DICOM Image Directory");

  QStringList rawFiles;

  //-------------------
  // convert Dicom to raw
  for (int i=0; i<flnms.count(); i++)
    {
      QStringList dnames;
      dnames << flnms[i];
      
      QFileInfo fraw(flnms[i]);
      rawFiles << QFileInfo(fraw.absolutePath(),
			    fraw.baseName() + ".raw").absoluteFilePath();
  

      progress.setLabelText(flnms[i]);
      qApp->processEvents();
      
      m_remapWidget->setFile(dnames, m_pluginDirDLib[idx]);
      
      progress.setValue(30);
      qApp->processEvents();
      
      if (ui.action8_bit->isChecked())
	m_remapWidget->setPvlMapMax(255);
      else
	m_remapWidget->setPvlMapMax(65535);
      
      progress.setValue(50);
      qApp->processEvents();
      
      m_remapWidget->saveTrimmed(0,0,0, 0,0,0);
    }
  
  progress.setValue(100);
  qApp->processEvents();
  //-------------------


  //-------------------
  // process the saved raw files  
  idx = m_pluginFileTypes.indexOf("RAW Files");

  m_remapWidget->mergeVolumes(m_pluginFileTypes[idx],
			      m_pluginFileDLib[idx],
			      rawFiles);

  m_remapWidget->saveAs();
  //---------------------

  
  //---------------------
  // remove temporary raw files
  foreach(QString flnm, rawFiles)
    {
      QFile::remove(flnm);
    }
  //---------------------
}
