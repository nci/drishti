#include "pythonengine.h"

#include "pybridge.h"

#include "pywidget.h"
#include "global.h"
#include "staticfunctions.h"

#include <QSplitter>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QFileDialog>

#include <QtConcurrent/QtConcurrent>


//--------------------------------------------
//--------------------------------------------
PyWorker::PyWorker(QString script) : m_script(script) {}

void 
PyWorker::initScript()
{
  m_hasInit = false;
  m_hasDataAllocator = false;
  m_hasSliceProcessor = false;
  m_hasVolumeProcessor = false;

  py::gil_scoped_acquire gil;

  try {
    QString spath = QFileInfo(m_script).absolutePath();
    QString ps = "import sys\nsys.path.insert(0, r\"" + spath + "\")";
    py::exec(ps.toStdString());

    QString scriptName = QFileInfo(m_script).baseName();
    std::cout << "Importing module:" << scriptName.toStdString() << "\n";
    PaintVolMask::global_pyModule = py::module_::import(scriptName.toStdString().c_str());
    PaintVolMask::global_paint_vol_mask->scriptName = scriptName;

    if (py::hasattr(PaintVolMask::global_pyModule, "init"))
      m_hasInit = true;

    if (py::hasattr(PaintVolMask::global_pyModule, "set_paint_data"))
      m_hasDataAllocator = true;

    if (py::hasattr(PaintVolMask::global_pyModule, "process_slice"))
      m_hasSliceProcessor = true;

    if (py::hasattr(PaintVolMask::global_pyModule, "process_volume"))
      m_hasVolumeProcessor = true;

    std::cout << "** init " << (m_hasInit?"found":"not found") << "\n";
    std::cout << "** set_paint_data " << (m_hasDataAllocator?"found":"not found") << "\n";
    std::cout << "** process_slice " << (m_hasSliceProcessor?"found":"not found") << "\n";
    std::cout << "** process_volume " << (m_hasVolumeProcessor?"found":"not found") << "\n";

    //ps = "import "+scriptName+" as testmod\nprint('Available functions :', dir(testmod))";
    //py::exec(ps.toStdString());
    std::cout << "** Successfully imported module: " << scriptName.toStdString() << "\n";
    py::object py_paintvolmask = py::cast(PaintVolMask::global_paint_vol_mask);
      
    std::cout << "** Setting paint data in Python module... ";
    PaintVolMask::global_pyModule.attr("set_paint_data")(py_paintvolmask);
    
    if (m_hasInit)
      PaintVolMask::global_pyModule.attr("init")();

    std::cout << "done.\n";

    emit initDone("true");
  }
  catch (const std::exception& e) {
    std::cout << "*** Error ***\n";
    std::cout << "Failed to import module: " + QString(e.what()).toStdString();
    emit initDone(QString(e.what()));
  }
}

void
PyWorker::process_slice(int slice)
{
  py::gil_scoped_acquire gil;
  
  if (m_hasSliceProcessor)
    {
      PaintVolMask::global_pyModule.attr("process_slice")(slice);
      emit sliceProcessed();
    }
  else
    {
      std::cout << "** NO SLICE PROCESSOR FOUND\n";
    }
}

void
PyWorker::process_volume()
{
  py::gil_scoped_acquire gil;
  
  if (m_hasVolumeProcessor)
    {
      PaintVolMask::global_pyModule.attr("process_volume")();
      emit volumeProcessed();
    }
  else
    std::cout << "** NO VOLUME PROCESSOR FOUND\n";
}
//--------------------------------------------
//--------------------------------------------


PyWidget::PyWidget(QWidget *parent)
: QWidget(parent)
{
  QVBoxLayout *layout = new QVBoxLayout();

  m_menu = new PyWidgetMenu(parent);

  layout->addWidget(m_menu);
  setLayout(layout);

  connect(m_menu, SIGNAL(runCommand(QString)),
	  this, SLOT(runCommand(QString)));
  

  m_worker = 0;
  m_thread = 0;

  m_d = m_w = m_h = 0;
  m_volPtr = m_maskPtr = 0;
  
  resize(600, 500);

  show();

  loadScripts(); 
}

void
PyWidget::closeEvent(QCloseEvent *)
{
  m_menu->close();
  delete m_menu;
 
  if (m_thread)
  {
    m_thread->quit();
    m_thread->wait();
    m_thread = 0;
  }
  if (m_worker)
  {
    delete m_worker;
    m_worker = 0;
  }

  PaintVolMask::global_paint_vol_mask->scriptActive = false;

  emit pyWidgetClosed();  
}


PyWidget::~PyWidget()
{ 
}

void
PyWidget::setFilename(QString volfile)
{
  QStringList pvlnames = StaticFunctions::getPvlNamesFromHeader(volfile);
  if (pvlnames.count() > 0)
    m_fileName = pvlnames[0];
  else
    m_fileName = volfile;
    

  m_maskName = m_fileName;
  m_maskName.chop(6);
  m_maskName += QString("mask.sc");

  m_fileName += ".001";

  m_menu->addRow("%DIR%", QFileInfo(m_fileName).absolutePath());
  m_menu->addRow("volume", "%DIR%/"+QFileInfo(m_fileName).fileName());
  m_menu->addRow("mask", "%DIR%/"+QFileInfo(m_maskName).fileName());
}

void
PyWidget::setSize(int d, int w, int h)
{
  m_d = d;
  m_w = w;
  m_h = h;
}

void
PyWidget::setVolPtr(uchar* v)
{
  m_volPtr = v;
}

void
PyWidget::setMaskPtr(uchar* m)
{
  m_maskPtr = m;
}

void
PyWidget::loadScripts()
{
  QString scriptdir = Global::scriptFolder();
  m_menu->loadScripts(scriptdir);
}

void
PyWidget::processSlice(int slice)
{
  if (!m_worker->hasSliceProcessor())
    QMessageBox::information(0, "Error", 
      "No Slice Processor found in script "+ 
      PaintVolMask::global_paint_vol_mask->scriptName);
  
  emit process_slice(slice);
}

void
PyWidget::processVolume()
{
  if (!m_worker->hasVolumeProcessor())
    QMessageBox::information(0, "Error", 
      "No Volume Processor found in script "+ 
      PaintVolMask::global_paint_vol_mask->scriptName);

  emit process_volume();
}

void
PyWidget::initDone(QString mesg) 
{
  if (mesg != "true")
  {
    QMessageBox::information(0, "Error", "Failed to import module: " + mesg);
    PaintVolMask::global_paint_vol_mask->scriptActive = false;
  }
  else
  {
    std::cout << "\n** Script initialization completed\n";
    PaintVolMask::global_paint_vol_mask->scriptActive = true;
  }
}

void
PyWidget::sliceProcessed() 
{
  std::cout << "\n** slice processed\n";
}

void
PyWidget::volumeProcessed() 
{
  std::cout << "\n** volume processed\n";
}

void
PyWidget::runCommand(QString script)
{
  PaintVolMask::global_paint_vol_mask->scriptActive = false;

  if (m_thread)
  {
    m_thread->quit();
    m_thread->wait();
    m_thread = 0;
  }

  if (m_worker)
  {
    delete m_worker;
    m_worker = 0;
  }

  m_thread = new MyThread;
  m_worker = new PyWorker(script);
  m_worker->moveToThread(m_thread);
  
  connect(this, &PyWidget::process_slice, m_worker, &PyWorker::process_slice);
  connect(this, &PyWidget::process_volume, m_worker, &PyWorker::process_volume);

  connect(m_worker, &PyWorker::sliceProcessed, this, &PyWidget::sliceProcessed);
  connect(m_worker, &PyWorker::volumeProcessed, this, &PyWidget::volumeProcessed);
  connect(m_worker, &PyWorker::initDone, this, &PyWidget::initDone);
  

  connect(m_thread, &QThread::started, m_worker, &PyWorker::initScript);
  connect(m_thread, &QThread::finished, m_thread, &QThread::deleteLater);

  m_thread->start();
  
  return;

//  //-----------------
//  // single thread
//  m_worker->initScript();
}
