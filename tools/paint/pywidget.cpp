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

  m_plugin = 0;
  m_menu = new PyWidgetMenu(parent);

  layout->addWidget(m_menu);
  setLayout(layout);

  connect(m_menu, SIGNAL(runCommand(QString, QHash<QString, QVariant>)),
	          this, SLOT(runCommand(QString, QHash<QString, QVariant>)));
  


  resize(600, 500);

  show();

  loadScripts(); 
}

void
PyWidget::closeEvent(QCloseEvent *)
{
  m_menu->close();
  delete m_menu;

  if (m_plugin)
  {
    m_plugin->clear();
    delete m_plugin;
    m_plugin = 0;
  }

  emit pyWidgetClosed();  
}

void PyWidget::setPyVersion(QString flnm) { m_pyversionflnm = flnm; }

void
PyWidget::init(uchar *vol, ushort *mask, 
              uchar *lut, uchar *tag,
              int depth, int width, int height)
{
  m_volume = vol;
  m_mask = mask;
  m_lut = lut;
  m_tag = tag;
  m_depth = depth;
  m_width = width;
  m_height = height;
}

void
PyWidget::setMask(ushort *mask)
{
  if (m_plugin)
    m_plugin->setMask(mask);
}

PyWidget::~PyWidget()
{ 
  if (m_plugin)
  {
    delete m_plugin;
    m_plugin = 0;
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

bool
PyWidget::processSlice(uchar* image, ushort* mask, 
                      int width, int height, int tag,
                      bool applyRecursive)
{
  if (m_plugin == 0)
  {
    QMessageBox::information(0, "Error", "No active script detected");
    return false;
  }

  if (!m_plugin->hasSliceProcessor())
  {
    QMessageBox::information(0, "Error", 
              QString("No Slice Processor found in script [%1]").\
              arg(m_plugin->scriptName()));
      return false;
  }

  populateArguments();
  
  bool result = m_plugin->process_slice(image, mask, width, height, tag);
  
  // sliceProcessingDone reloads the same slice hence avoiding it for recursive setup
  if (result && !applyRecursive) 
    {
      std::cout << "\n** slice processed\n";
      emit sliceProcessingDone();
    }

  return result;
}
  
bool
PyWidget::processVolume()
{
  if (m_plugin == 0)
  {
    QMessageBox::information(0, "Error", "No active script detected");
    return false;
  }

  if (!m_plugin->hasVolumeProcessor())
  {
    QMessageBox::information(0, "Error", 
              QString("No Volume Processor found in script [%1]").\
              arg(m_plugin->scriptName()));
    return false;
  }

  populateArguments();

  bool result = m_plugin->process_volume();

  if (result)
    {  
      std::cout << "\n** volume processed\n";
      emit sliceProcessingDone();
      emit volumeProcessingDone();
    }
  
  return result;
}

void
PyWidget::initDone(QString mesg) 
{
  if (mesg != "true")
    {
      QMessageBox::information(0, "Error", "Failed to import module: " + mesg);
      Global::setScriptActive(false);
    }
  else
    {  
      std::cout << "\n** Script initialization completed : " 
                << (m_plugin->scriptName()).toStdString()
                << "\n";
      Global::setScriptName(m_plugin->scriptName());
      Global::setScriptActive(true);
    }
}

void
PyWidget::populateArguments()
{
  m_menu->genArgumentsFromTable();
  m_plugin->populateArguments(m_menu->getArguments());
}

void
PyWidget::runCommand(QString script, QHash<QString, QVariant> arguments)
{
  if (m_plugin)
  {
    m_plugin->clear();
    delete m_plugin;
    m_plugin = 0;
  }

  m_plugin = new PyPlugin();
  
  if (m_plugin->init(m_pyversionflnm, script, 
                    m_volume, m_mask, m_lut, m_tag,
                    m_depth, m_width, m_height))
    Global::setPythonInstalled(true);
  else
  {
    Global::setPythonInstalled(false);
    delete m_plugin;
    m_plugin = 0;
    return;
  }

  populateArguments();

  connect(this, &PyWidget::initScript, m_plugin, &PyPlugin::initScript);
  connect(this, &PyWidget::process_slice, m_plugin, &PyPlugin::process_slice);
  connect(this, &PyWidget::process_volume, m_plugin, &PyPlugin::process_volume);

  connect(m_plugin, &PyPlugin::sliceProcessingDone, this, &PyWidget::sliceProcessingDone);
  connect(m_plugin, &PyPlugin::volumeProcessingDone, this, &PyWidget::volumeProcessingDone);
  connect(m_plugin, &PyPlugin::initDone, this, &PyWidget::initDone);

  emit initScript();
}
