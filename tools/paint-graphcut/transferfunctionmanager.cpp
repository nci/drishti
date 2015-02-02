#include "transferfunctionmanager.h"
#include "global.h"
#include <QDomDocument>

#include <QPushButton>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>

TransferFunctionManager::TransferFunctionManager(QWidget *parent) :
  QFrame(parent)
{
  setFrameStyle(QFrame::StyledPanel);
  setFrameShadow(QFrame::Plain);

  m_tfContainer = NULL;

  m_buttonGroup = new QGroupBox();

  QPushButton *newTF = new QPushButton("New");
  QPushButton *refreshTF = new QPushButton("Refresh");
  QPushButton *removeTF = new QPushButton("Remove");

  QHBoxLayout *hbox = new QHBoxLayout();
  hbox->addWidget(newTF);
  hbox->addWidget(refreshTF);
  hbox->addWidget(removeTF);
  hbox->addStretch(1);

  m_buttonGroup->setLayout(hbox);


  connect(newTF, SIGNAL(clicked()),
	  this, SLOT(addNewTransferFunction()));

  connect(refreshTF, SIGNAL(clicked()),
	  this, SLOT(refreshTransferFunction()));

  connect(removeTF, SIGNAL(clicked()),
	  this, SLOT(removeTransferFunction()));


  m_tableWidget = new QTableWidget();
  m_tableWidget->horizontalHeader()->setSectionsMovable(true);
  m_tableWidget->horizontalHeader()->setSectionsClickable(true);
  m_tableWidget->verticalHeader()->hide();
  m_tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
  m_tableWidget->setShowGrid(false);
  m_tableWidget->setAlternatingRowColors(true);
  m_tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
  m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
  QString str;
  str = QString("QTableWidget {"					\
		"selection-background-color: qlineargradient("		\
		"x1:0, y1:0, x2:0, y2:1, "				\
		"stop:0 darkgray,  stop:0.4 lightgray, "		\
		"stop:0.6 lightgray, stop:1 darkgray);"			\
		"}");
  m_tableWidget->setStyleSheet(str);

  modifyTableWidget();


  QVBoxLayout *vbox = new QVBoxLayout();
  vbox->addWidget(m_buttonGroup);
  vbox->addWidget(m_tableWidget);

  setLayout(vbox);

  connect(m_tableWidget->horizontalHeader(), SIGNAL(sectionClicked(int)),
	  this, SLOT(headerClicked(int)));

  connect(m_tableWidget, SIGNAL(cellClicked(int, int)),
	  this, SLOT(cellClicked(int, int)));

  connect(m_tableWidget, SIGNAL(cellChanged(int, int)),
	  this, SLOT(cellChanged(int, int)));
}

void
TransferFunctionManager::registerContainer(TransferFunctionContainer *tfContainer)
{
  m_tfContainer = tfContainer;
  modifyTableWidget();
}

void
TransferFunctionManager::headerClicked(int index)
{
  if (index == 0)
    {
      // save currently selected transfer function into an image file
      QList<QTableWidgetSelectionRange> sel = m_tableWidget->selectedRanges();
      for(int i=0; i<sel.count(); i++)
	{
	  for(int row=sel[i].topRow(); row<=sel[i].bottomRow(); row++)
	    {
	      QString imgFile = QFileDialog::getSaveFileName(0,
			        QString("Save transfer function %1 to").arg(row),
			        Global::previousDirectory()+"/tf.png",
			        "Image Files (*.png *.tif *.bmp *.jpg)",
			        0,
			        QFileDialog::DontUseNativeDialog);

	      if (imgFile.isEmpty())
		return;

	      QFileInfo f(imgFile);
	      Global::setPreviousDirectory(f.absolutePath());
	      
	      QImage img = m_tfContainer->colorMapImage(row);
	      if (! img.isNull())
		img.save(imgFile);
	    }
	}      
    }
  else
    {
      QString imgFile = QFileDialog::getSaveFileName(0,
		        QString("Save composite for Set %1 to").arg(index-1),
			Global::previousDirectory()+"/set.png",
			"Image Files (*.png *.tif *.bmp *.jpg)",
			0,
			QFileDialog::DontUseNativeDialog);
      
      if (imgFile.isEmpty())
	return;
      
      QFileInfo f(imgFile);
      Global::setPreviousDirectory(f.absolutePath());

      QImage img = m_tfContainer->composite(index-1);
      if (! img.isNull())
	img.save(imgFile);
    }
}

void
TransferFunctionManager::modifyTableWidget()
{
  // clear all information from the table
  while(m_tableWidget->rowCount() > 0)
    {
      m_tableWidget->removeRow(0);
    }

  int maxSets = 1;

  QStringList item;
  item.clear();

  if (m_tfContainer)
    maxSets = m_tfContainer->maxSets();

  m_tableWidget->setColumnCount(maxSets+1);
  item << "Name ";
  for (int i=0; i<maxSets; i++)
    item << QString("%1").arg(i);

  m_tableWidget->setHorizontalHeaderLabels(item);

  for (int i=0; i<maxSets; i++)
    m_tableWidget->setColumnWidth(i+1, 20);

  m_tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
}

void
TransferFunctionManager::clearManager()
{  
  if (!m_tfContainer) return;
  
  m_tfContainer->clearContainer();

  modifyTableWidget();
}

void
TransferFunctionManager::cellClicked(int row, int col)
{
  if (!m_tfContainer) return;

  QList<bool> on;
  for (int i=0; i<m_tfContainer->maxSets(); i++)
    {
      QTableWidgetItem *wi;
      wi = m_tableWidget->item(row, i+1);
      bool flag = wi->checkState() == Qt::Checked;
      on.append(flag);
    }
  emit changeTransferFunctionDisplay(row, on);
  qApp->processEvents();
}

void
TransferFunctionManager::cellChanged(int row, int col)
{
  if (!m_tfContainer) return;

  if (col == 0)
    {
      QTableWidgetItem *wi;
      wi = m_tableWidget->item(row, col);
      SplineTransferFunction *tf = m_tfContainer->transferFunctionPtr(row);
      tf->setName(wi->text());
    }
  else
    {
      QTableWidgetItem *wi;
      wi = m_tableWidget->item(row, col);
      m_tfContainer->setCheckState(row, col-1, wi->checkState());

      bool flag = wi->checkState() == Qt::Checked;
      emit checkStateChanged(row, col, flag);
      qApp->processEvents();
    }
}


void
TransferFunctionManager::refreshManager()
{
  if (!m_tfContainer) return;

  //------------------------------------------------
  //temporarily diconnect the connections
  m_tableWidget->disconnect(this);
  //------------------------------------------------

  modifyTableWidget();

  int cols = m_tableWidget->columnCount();
  int rows;


  for(rows=0; rows<m_tfContainer->count(); rows++)
    {
      m_tableWidget->insertRow(rows);

      QTableWidgetItem *wi;
      
      SplineTransferFunction *tf = m_tfContainer->transferFunctionPtr(rows);
      wi = new QTableWidgetItem(tf->name());
      wi->setFlags(wi->flags() & ~Qt::ItemIsUserCheckable);
      m_tableWidget->setItem(rows, 0, wi);
      
      for (int i=1; i<cols; i++)
	{
	  wi = new QTableWidgetItem("");
	  if (tf->on(i-1))
	    wi->setCheckState(Qt::Checked);
	  else
	    wi->setCheckState(Qt::Unchecked);
	  wi->setFlags(wi->flags() & ~Qt::ItemIsEditable);
	  m_tableWidget->setItem(rows, i, wi);
	}      
    }

  //------------------------------------------------
  //rewire the connections
  connect(m_tableWidget, SIGNAL(cellClicked(int, int)),
	  this, SLOT(cellClicked(int, int)));

  connect(m_tableWidget, SIGNAL(cellChanged(int, int)),
	  this, SLOT(cellChanged(int, int)));
  //------------------------------------------------

  rows = m_tableWidget->rowCount();

  for(int it=0; it<rows; it++)
    m_tableWidget->setRowHeight(it, 20);      
  
  if (rows > 0)
    {
      m_tableWidget->setCurrentCell(0,0);
      QList<bool> on;
      for (int i=0; i<m_tfContainer->maxSets(); i++)
	{
	  QTableWidgetItem *wi;
	  wi = m_tableWidget->item(0, i+1);
	  bool flag = wi->checkState() == Qt::Checked;
	  on.append(flag);
	}

      emit changeTransferFunctionDisplay(0, on);
    }

  qApp->processEvents();
}


void
TransferFunctionManager::refreshTransferFunction()
{
  if (!m_tfContainer) return;

  QList<QTableWidgetSelectionRange> sel = m_tableWidget->selectedRanges();
  QList<bool> on;

  if (sel.count() > 0)
    {
      for (int i=0; i<m_tfContainer->maxSets(); i++)
	{
	  QTableWidgetItem *wi;
	  wi = m_tableWidget->item(sel[0].topRow(), i+1);
	  bool flag = wi->checkState() == Qt::Checked;
	  on.append(flag);
	}
      emit changeTransferFunctionDisplay(sel[0].topRow(), on);
    }
  else
    emit changeTransferFunctionDisplay(-1, on);
}

void
TransferFunctionManager::addNewTransferFunction()
{
  if (!m_tfContainer) return;

  //------------------------------------------------
  //temporarily diconnect the connections
  m_tableWidget->disconnect(this);
  //------------------------------------------------

  int rows = m_tableWidget->rowCount();
  int cols = m_tableWidget->columnCount();

  m_tableWidget->insertRow(rows);

  QTableWidgetItem *qtwi;
  qtwi = new QTableWidgetItem(QString("TF %1").arg(rows));
  qtwi->setFlags(qtwi->flags() & ~Qt::ItemIsUserCheckable);
  m_tableWidget->setItem(rows, 0, qtwi);

  if (cols > 1)
    {
      QTableWidgetItem *wi;
      wi = new QTableWidgetItem("");
      wi->setCheckState(Qt::Checked);
      wi->setFlags(wi->flags() & ~Qt::ItemIsEditable);
      m_tableWidget->setItem(rows, 1, wi);
    }
  for (int i=2; i<cols; i++)
    {
      QTableWidgetItem *wi;
      wi = new QTableWidgetItem("");
      wi->setCheckState(Qt::Unchecked);
      wi->setFlags(wi->flags() & ~Qt::ItemIsEditable);
      m_tableWidget->setItem(rows, i, wi);
    }


  for(int it=0; it<m_tableWidget->rowCount(); it++)
    m_tableWidget->setRowHeight(it, 20);


  //------------------------------------------------
  // add new TransferFunction
  m_tfContainer->addSplineTF();
  SplineTransferFunction *tf = m_tfContainer->transferFunctionPtr(rows);
  tf->setName(qtwi->text());
  //------------------------------------------------

  //------------------------------------------------
  //rewire the connections
  connect(m_tableWidget, SIGNAL(cellClicked(int, int)),
	  this, SLOT(cellClicked(int, int)));

  connect(m_tableWidget, SIGNAL(cellChanged(int, int)),
	  this, SLOT(cellChanged(int, int)));
  //------------------------------------------------

  m_tableWidget->setRangeSelected(QTableWidgetSelectionRange(0, 0, rows, cols-1), false);
  m_tableWidget->setRangeSelected(QTableWidgetSelectionRange(rows, 0, rows, cols-1), true);

  QList<bool> on;
  for (int i=0; i<m_tfContainer->maxSets(); i++)
    {
      QTableWidgetItem *wi;
      wi = m_tableWidget->item(rows, i+1);
      bool flag = wi->checkState() == Qt::Checked;
      on.append(flag);
    }
  emit changeTransferFunctionDisplay(rows, on);
  qApp->processEvents();
}

void
TransferFunctionManager::removeTransferFunction()
{
  if (!m_tfContainer) return;

  //------------------------------------------------
  //temporarily diconnect the connections
  m_tableWidget->disconnect(this);
  //------------------------------------------------


  QList<QTableWidgetSelectionRange> sel = m_tableWidget->selectedRanges();

  for(int i=0; i<sel.count(); i++)
    {
      if (sel[i].leftColumn() == 0)
	{
	  for(int row=sel[i].topRow(); row<=sel[i].bottomRow(); row++)
	    {
	      QTableWidgetItem *wi;
	      wi = new QTableWidgetItem(QString("%~%*"));
	      m_tableWidget->setItem(row, 0, wi);	    
	    }
	}
    }
  
  bool done = false;
  while (!done)
    {
      done = true;
      for(int i=0; i<m_tableWidget->rowCount(); i++)
	{
	  QTableWidgetItem *wi;
	  wi = m_tableWidget->item(i, 0);
	  if (wi->text() == QString("%~%*"))
	    {
	      m_tableWidget->removeRow(i);
	      done = false;
	      
	      //------------------------------------------------
	      // remove TransferFunction
	      m_tfContainer->removeSplineTF(i);
	      //------------------------------------------------
	    }
	}
    }

  //------------------------------------------------
  //rewire the connections
  connect(m_tableWidget, SIGNAL(cellClicked(int, int)),
	  this, SLOT(cellClicked(int, int)));

  connect(m_tableWidget, SIGNAL(cellChanged(int, int)),
	  this, SLOT(cellChanged(int, int)));
  //------------------------------------------------

  int rows = m_tableWidget->rowCount();
  int cols = m_tableWidget->columnCount();

  QList<bool> on;

  if (rows > 0)
    {
      m_tableWidget->setRangeSelected(QTableWidgetSelectionRange(0, 0, rows, cols-1), false);
      m_tableWidget->setRangeSelected(QTableWidgetSelectionRange(0, 0, 0, cols-1), true);      

      for (int i=0; i<m_tfContainer->maxSets(); i++)
	{
	  QTableWidgetItem *wi;
	  wi = m_tableWidget->item(0, i+1);
	  bool flag = wi->checkState() == Qt::Checked;
	  on.append(flag);
	}
      emit changeTransferFunctionDisplay(0, on);
    }
  else
    emit changeTransferFunctionDisplay(-1, on);

  qApp->processEvents();
}

void
TransferFunctionManager::load(const char *flnm)
{
  m_tfContainer->clearContainer();

  QDomDocument document;
  QFile f(flnm);
  if (f.open(QIODevice::ReadOnly))
    {
      document.setContent(&f);
      f.close();
    }

  QDomElement main = document.documentElement();
  QDomNodeList dlist = main.childNodes();
  for(int i=0; i<dlist.count(); i++)
    {
      if (dlist.at(i).nodeName() == "transferfunction")
	m_tfContainer->fromDomElement(dlist.at(i).toElement());
    }

  refreshManager();
}

void
TransferFunctionManager::append(const char *flnm)
{
  //m_tfContainer->clearContainer();

  QDomDocument document;
  QFile f(flnm);
  if (f.open(QIODevice::ReadOnly))
    {
      document.setContent(&f);
      f.close();
    }

  QDomElement main = document.documentElement();
  QDomNodeList dlist = main.childNodes();
  for(int i=0; i<dlist.count(); i++)
    {
      if (dlist.at(i).nodeName() == "transferfunction")
	m_tfContainer->fromDomElement(dlist.at(i).toElement());
    }

  refreshManager();
}

void
TransferFunctionManager::load(QList<SplineInformation> splineInfo)
{
  m_tfContainer->clearContainer();

  for(int i=0; i<splineInfo.count(); i++)
    m_tfContainer->fromSplineInformation(splineInfo[i]);

  refreshManager();
}


void
TransferFunctionManager::save(const char *flnm)
{
  QDomDocument document;
  QFile f(flnm);
  if (f.open(QIODevice::ReadOnly))
    {
      document.setContent(&f);
      f.close();
    }
  QDomElement topElement = document.documentElement();
  for(int i=0; i<m_tfContainer->count(); i++)
    {
      QDomElement de = m_tfContainer->transferFunctionPtr(i)->domElement(document);
      topElement.appendChild(de);
    }

  QFile fout(flnm);
  if (fout.open(QIODevice::WriteOnly))
    {
      QTextStream out(&fout);
      document.save(out, 2);
      fout.close();
    }
}
