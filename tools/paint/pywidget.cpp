#include "pywidget.h"
#include <QVBoxLayout>

PyWidget::PyWidget(QWidget *parent)
: QWidget(parent)
{
  m_plainTextEdit = new QPlainTextEdit(this);
  m_plainTextEdit->setReadOnly(true);

  m_lineEdit = new QLineEdit(this);
  m_lineEdit->setClearButtonEnabled(true);

  QVBoxLayout *layout = new QVBoxLayout;
  layout->addWidget(m_plainTextEdit);
  layout->addWidget(m_lineEdit);
  setLayout(layout);
  show();
  
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
}

PyWidget::~PyWidget()
{
    m_process->close();
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
  m_process->write(m_lineEdit->text().toLatin1() + "\n");
  m_lineEdit->clear();
}
