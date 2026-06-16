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
  m_plainTextEdit->appendPlainText(m_fileName);
  m_plainTextEdit->appendPlainText(m_maskName+"\n");

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
  

PyWidget::PyWidget(QWidget *parent)
: QWidget(parent)
{
  m_menu = new PyWidgetMenu(parent);
  m_menu->show();

  connect(m_menu, SIGNAL(runCommand(QString)),
	  this, SLOT(runCommand(QString)));
  

  m_worker = 0;
  m_thread = 0;

  m_d = m_w = m_h = 0;
  m_volPtr = m_maskPtr = 0;
  
  m_plainTextEdit = new QPlainTextEdit(this);
  m_plainTextEdit->setReadOnly(true);
  m_plainTextEdit->setFont(QFont("MS Reference Sans Serif", 12));

  m_lineEdit = new QLineEdit(this);
  m_lineEdit->setClearButtonEnabled(true);
  m_lineEdit->setFont(QFont("MS Reference Sans Serif", 12));

  QVBoxLayout *layout = new QVBoxLayout;
  layout->addWidget(m_plainTextEdit);
  layout->addWidget(m_lineEdit);

  QWidget *wid = new QWidget();
  wid->setLayout(layout);
  
  QSplitter *splitter = new QSplitter(Qt::Horizontal, this);
  splitter->addWidget(m_menu);
  splitter->addWidget(wid);
  splitter->setStretchFactor(0, 1);
  splitter->setStretchFactor(1, 3);
  splitter->setStyleSheet("QSplitter::handle {image: url(:/images/sizegrip.png);}");

  
  QHBoxLayout *hlayout = new QHBoxLayout;
  hlayout->addWidget(splitter);
  
  setLayout(hlayout);
  show();

  QFont font;
  font.setPointSize(10);
  m_plainTextEdit->setFont(font);
  m_lineEdit->setFont(font);

  m_process = new QProcess();

  connect(m_process, SIGNAL(readyReadStandardOutput()),
	  this, SLOT(readOutput()));
  connect(m_process, SIGNAL(readyReadStandardError()),
	  this, SLOT(readError()));

  connect(m_lineEdit, SIGNAL(returnPressed()),
	  this, SLOT(processLine()));

  connect(m_process, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
	  this, &PyWidget::close);

#if defined(Q_OS_WIN32)
  m_process->start("cmd.exe");
#else
  m_process->start("/bin/bash");
#endif

  resize(600, 500);

  loadScripts(); 
}

void
PyWidget::loadScripts()
{
  QString scriptdir = Global::scriptFolder();
  m_menu->loadScripts(scriptdir);
}

void
PyWidget::closeEvent(QCloseEvent *event)
{
  m_menu->close();
  delete m_menu;
 
  if (m_thread)
  {
    m_thread->quit();
    m_thread->terminate();
    m_thread->wait();
    m_thread = 0;
  }

  m_process->close();

  emit pyWidgetClosed();  

  event->accept();
}


PyWidget::~PyWidget()
{ 
}


void
PyWidget::readOutput()
{
  m_plainTextEdit->appendPlainText(m_process->readAllStandardOutput());
}
void
PyWidget::readError()
{
  m_plainTextEdit->appendPlainText(m_process->readAllStandardError());
}
void
PyWidget::processLine()
{
  QString command = m_lineEdit->text();

  QStringList sl = command.split(" ", QString::SkipEmptyParts);
  if (sl.count() == 0)
    return;
  
  if (sl[0] == QString("exit"))
    {
      m_process->kill();
      return;
    }
  
  if (sl.count() >= 2 && StaticFunctions::checkExtension(sl[0], ".py"))
    {
      QString scriptdir = Global::scriptFolder();
      QString script = scriptdir + QDir::separator() + sl[0];

      QString volflnm = m_fileName;
      QStringList result = sl.filter("volume=");
      if (result.count() >= 1)
	      volflnm = QFileInfo(m_fileName).absolutePath() + QDir::separator() + result[0].split("=")[1];

      QString tagflnm;
      result = sl.filter("output=");
      if (result.count() >= 1)
	      tagflnm = QFileInfo(m_fileName).absolutePath() + QDir::separator() + result[0].split("=")[1];
      
      QString  boxflnm;
      result = sl.filter("boxlist=");
      if (result.count() >= 1)
	      boxflnm = QFileInfo(m_fileName).absolutePath() + QDir::separator() + result[0].split("=")[1];
      
      
      QString cmd = "python "+script+" volume="+volflnm+" mask="+m_maskName;

      if (!tagflnm.isEmpty())
	      cmd +=" output="+tagflnm;

      if (!boxflnm.isEmpty())
	      cmd += " boxlist="+boxflnm;
	
      if (sl.count() > 1)
	    {
	      for (int s=1; s<sl.count(); s++)
	        {
	          if (!sl[s].contains("volume=", Qt::CaseInsensitive) &&
	    	        !sl[s].contains("output=", Qt::CaseInsensitive) &&
	    	        !sl[s].contains("boxlist=",Qt::CaseInsensitive))
	    	      cmd += " "+sl[s];
	        }
	    }
      m_process->write(cmd.toUtf8() + "\n");

      return;
    }  
  

  m_process->write(command.toUtf8() + "\n");
}

void
PyWidget::processSlice(int slice)
{
  emit process_slice(slice);

  //QFuture<void> future = QtConcurrent::run([slice] {
  //  QMessageBox::information(0, "", "process Slice");
  //  py::gil_scoped_acquire acquire;
  //  PaintVolMask::global_pyModule.attr("process_slice")(slice);
  //});
  //PaintVolMask::global_pyModule.attr("process_slice")(slice);
  //emit process_slice(slice);
}


//--------------------------------------------
//--------------------------------------------
PyWorker::PyWorker(QString script) : m_script(script) {}

void 
PyWorker::initScript()
{
  py::gil_scoped_acquire gil;

  try {
    QString spath = QFileInfo(m_script).absolutePath();
    QString ps = "import sys\nsys.path.insert(0, r\"" + spath + "\")";
    py::exec(ps.toStdString());

    QString scriptName = QFileInfo(m_script).baseName();
    py::print("Importing module:", scriptName.toStdString());
    PaintVolMask::global_pyModule = py::module_::import(scriptName.toStdString().c_str());
    ps = "import "+scriptName+" as testmod\nprint('Available functions :', dir(testmod))";
    py::exec(ps.toStdString());
    py::print("Successfully imported module: ", scriptName.toStdString());
    py::object py_paintvolmask = py::cast(PaintVolMask::global_paint_vol_mask);
      
    py::print("Setting paint data in Python module...");
    PaintVolMask::global_pyModule.attr("set_paint_data")(py_paintvolmask);
    PaintVolMask::global_pyModule.attr("init_sam")();
  }
  catch (const std::exception& e) {
    QMessageBox::information(0, "Error", "Failed to import module: " + QString(e.what()));
  }
}

void
PyWorker::process_slice(int slice)
{
  py::gil_scoped_acquire gil;
  
  PaintVolMask::global_pyModule.attr("process_slice")(slice);
}
//--------------------------------------------
//--------------------------------------------



void
PyWidget::runCommand(QString script)
{
  if (m_worker)
  {
    delete m_worker;
    m_worker = 0;
  }
  if (m_thread)
  {
    m_thread->quit();
    m_thread->terminate();
    m_thread->wait();
    m_thread = 0;
  }

  m_thread = new MyThread;
  m_worker = new PyWorker(script);
  m_worker->moveToThread(m_thread);
  
  connect(this, &PyWidget::process_slice, m_worker, &PyWorker::process_slice);
  
  connect(m_thread, &QThread::started, m_worker, &PyWorker::initScript);
  //connect(m_worker, &PyWorker::finished, m_thread, &QThread::quit);
  //connect(m_thread, &QThread::finished, m_thread, &QThread::deleteLater);

  m_thread->start();
  
  return;

  try {
    QString spath = QFileInfo(script).absolutePath();
    py::print("Script path:", spath.toStdString());
    QString ps = "import sys\nsys.path.insert(0, r\"" + spath + "\")";
    py::exec(ps.toStdString());
    QString scriptName = QFileInfo(script).baseName();
    py::print("Importing module:", scriptName.toStdString());
    PaintVolMask::global_pyModule = py::module_::import(scriptName.toStdString().c_str());
    ps = "import "+scriptName+" as testmod\nprint('Available functions :', dir(testmod))";
    py::exec(ps.toStdString());
    py::print("Successfully imported module: ", scriptName.toStdString());

    py::object py_paintvolmask = py::cast(PaintVolMask::global_paint_vol_mask);
    PaintVolMask::global_pyModule.attr("set_paint_data") (py_paintvolmask);
    PaintVolMask::global_pyModule.attr("init_sam")();
  }
  catch (const std::exception& e) {
    QMessageBox::information(0, "Error", "Failed to import module: " + QString(e.what()));
    return;
  }
  
  QMessageBox::information(0, "Info", "Successfully imported module: " + QFileInfo(script).baseName());

  //m_process->write(cmd.toUtf8() + "\n");
}
