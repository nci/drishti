
#include "pywidget.h"
#include "global.h"
#include "staticfunctions.h"

#include <QSplitter>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QFileDialog>

//#pragma push_macro("slots")
//#undef slots
//#include "Python.h"
//#pragma pop_macro("slots")
//
//#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
//#include "numpy/arrayobject.h"

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

  m_process->start("cmd.exe");

  resize(600, 500);
  
  //initPy();

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
 
  m_process->close();

  //Py_Finalize();

  emit pyWidgetClosed();  

  event->accept();
}


PyWidget::~PyWidget()
{ 
}

//int
//PyWidget::initPy()
//{
//  Py_Initialize();
//
//  import_array(); // used by numpy
//}
//
//
//int
//PyWidget::runPythonCode(QString moduleName, QString funcName)
//{
//  const int ND = 3;
//  npy_intp dims[3] {m_d, m_w, m_h};
//      
//  // Convert it to a NumPy array.
//  PyObject *volArray = PyArray_SimpleNewFromData(ND,
//						 dims,
//						 NPY_INT8,
//						 reinterpret_cast<void*>(m_volPtr));
//
//  PyObject *maskArray = PyArray_SimpleNewFromData(ND,
//						  dims,
//						  NPY_INT8,
//						  reinterpret_cast<void*>(m_maskPtr));
//  if (!volArray || !maskArray)
//    {
//      QMessageBox::information(0,"", "Some problem with PyArray point creation");
//      return -1;
//    }
//  
//
//  //PyArrayObject *vol_arr = reinterpret_cast<PyArrayObject*>(volArray);
//  //PyArrayObject *mask_arr = reinterpret_cast<PyArrayObject*>(maskArray);
//
//  // import moduleName.funcName
//  const char *module_name = moduleName.toLatin1().data();
//  PyObject *pName = PyUnicode_FromString(module_name);
//  if (!pName)
//    {
//      QMessageBox::information(0, "", moduleName+" not found");
//      Py_DECREF(volArray);
//      Py_DECREF(maskArray);
//      return -1;
//    }
//
//  PyObject *pModule = PyImport_Import(pName);
//  Py_DECREF(pName);
//  if (!pModule)
//    {
//      QMessageBox::information(0, "", "failed to import "+moduleName);
//      Py_DECREF(volArray);
//      Py_DECREF(maskArray);
//      return -1;
//    }
//
//  const char *func_name = funcName.toLatin1().data();
//  PyObject *pFunc = PyObject_GetAttrString(pModule, func_name);
//  if (!pFunc)
//    {
//      QMessageBox::information(0, "", funcName+" not found");
//      Py_DECREF(pModule);
//      return -1;
//    }
//
//  if (!PyCallable_Check(pFunc))
//    {
//      QMessageBox::information(0, "", moduleName + "." + funcName + " is not callable.");
//      Py_DECREF(pFunc);
//      return -1;
//  }
//
//  PyObject *pReturn = PyObject_CallFunctionObjArgs(pFunc, volArray, maskArray, NULL);
//  if (!pReturn)
//    {
//      QMessageBox::information(0, "", "Call to " + funcName +" failed");
//      Py_DECREF(pFunc);
//      return -1;
//    }
//
//  QMessageBox::information(0, "", "done");
//
//  return 0;
//}

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
  
  
//  if (sl[0] == QString("run") && sl.count() == 3)
//    {
//      QString scriptdir = Global::scriptFolder();
//      QString script = scriptdir + QDir::separator() + sl[1];
//      runPythonCode(script, sl[2]);
//      return;
//    }
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

      //m_lineEdit->clear();
      return;
    }  
  

  m_process->write(command.toUtf8() + "\n");
  //m_lineEdit->clear();
}

void
PyWidget::runCommand(QString cmd)
{
  m_process->write(cmd.toUtf8() + "\n");
}
