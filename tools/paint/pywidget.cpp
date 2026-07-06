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

#include <QtConcurrent>
#include <QFuture>
#include <QFutureWatcher>

//--------------------------------------------
//--------------------------------------------

//--------------------------------------------
//--------------------------------------------


PyWidget::PyWidget(QWidget *parent)
: QWidget(parent)
{
  QVBoxLayout *layout = new QVBoxLayout();

  m_menu = new PyWidgetMenu(parent);

  layout->addWidget(m_menu);
  setLayout(layout);

  connect(m_menu, SIGNAL(runCommand(QString, QHash<QString, QVariant>)),
	          this, SLOT(runCommand(QString, QHash<QString, QVariant>)));
  

  m_worker = 0;
  m_thread = 0;

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

  emit pyWidgetClosed();  
}

void
PyWidget::init(uchar *vol, ushort *mask, uchar *lut, int depth, int width, int height)
{
  m_volume = vol;
  m_mask = mask;
  m_lut = lut;
  m_depth = depth;
  m_width = width;
  m_height = height;
}

void
PyWidget::setMask(ushort *mask)
{
  if (m_worker)
    m_worker->setMask(mask);
}

PyWidget::~PyWidget()
{ 
  if (m_worker)
  {
    delete m_worker;
    m_worker = 0;
  }
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
}

void
PyWidget::loadScripts()
{
  QString scriptdir = Global::scriptFolder();
  m_menu->loadScripts(scriptdir);
}

void
PyWidget::processSlice(uchar* image, ushort* mask, int width, int height, int tag)
{
  if (!m_worker->hasSliceProcessor())
  {
    QMessageBox::information(0, "Error", 
      "No Slice Processor found in script "+ 
      m_worker->scriptName());
      return;
  }

  populateArguments();

  //emit process_slice(image, mask, width, height, tag);
  m_worker->process_slice(image, mask, width, height, tag);
}
  
void
PyWidget::processVolume()
{
  if (m_worker == 0)
  {
    QMessageBox::information(0, "Error", "No active script detected");
    return;
  }

  if (!m_worker->hasVolumeProcessor())
  {
    QMessageBox::information(0, "Error", 
              "No Volume Processor found in script "+ 
              m_worker->scriptName());
    return;
  }

  populateArguments();

  //emit process_volume();
  m_worker->process_volume();
}

void
PyWidget::initDone(QString mesg) 
{
  if (mesg != "true")
    QMessageBox::information(0, "Error", "Failed to import module: " + mesg);
  else
    std::cout << "\n** Script initialization completed\n";
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
PyWidget::populateArguments()
{
  m_menu->genArgumentsFromTable();
  m_worker->populateArguments(m_menu->getArguments());
}

void
PyWidget::runCommand(QString script, QHash<QString, QVariant> arguments)
{
  if (m_worker)
  {
    delete m_worker;
    m_worker = 0;
  }
  m_worker = new PyWorker(script);

  m_worker->init(m_volume, m_mask, m_lut, m_depth, m_width, m_height);

  populateArguments();

  connect(this, &PyWidget::initScript, m_worker, &PyWorker::initScript);
  connect(this, &PyWidget::process_slice, m_worker, &PyWorker::process_slice);
  connect(this, &PyWidget::process_volume, m_worker, &PyWorker::process_volume);

  connect(m_worker, &PyWorker::sliceProcessed, this, &PyWidget::sliceProcessed);
  connect(m_worker, &PyWorker::volumeProcessed, this, &PyWidget::volumeProcessed);
  connect(m_worker, &PyWorker::sliceProcessingDone, this, &PyWidget::sliceProcessingDone);
  connect(m_worker, &PyWorker::volumeProcessingDone, this, &PyWidget::volumeProcessingDone);
  connect(m_worker, &PyWorker::initDone, this, &PyWidget::initDone);

  emit initScript();
  //-----------------

  
  //if (m_thread)
  //{
  //  m_thread->quit();
  //  m_thread->wait();
  //  m_thread = 0;
  //}
  //
  //if (m_worker)
  //{
  //  delete m_worker;
  //  m_worker = 0;
  //}
  //
  //m_thread = new MyThread;
  //m_worker = new PyWorker(script);
  //m_worker->moveToThread(m_thread);
  //
  //connect(this, &PyWidget::initScript, m_worker, &PyWorker::initScript);
  //connect(this, &PyWidget::process_slice, m_worker, &PyWorker::process_slice);
  //connect(this, &PyWidget::process_volume, m_worker, &PyWorker::process_volume);
  //
  //connect(m_worker, &PyWorker::sliceProcessed, this, &PyWidget::sliceProcessed);
  //connect(m_worker, &PyWorker::volumeProcessed, this, &PyWidget::volumeProcessed);
  //connect(m_worker, &PyWorker::initDone, this, &PyWidget::initDone);
  //
  //connect(m_thread, &QThread::finished, m_thread, &QThread::deleteLater);
  //
  //m_thread->start();
  //
  //emit initScript();
  //
  //return;
}
