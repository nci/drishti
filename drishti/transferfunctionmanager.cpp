#include "global.h"
#include "transferfunctionmanager.h"
#include "propertyeditor.h"
#include <QDomDocument>

#include <QPushButton>
#include <QFileDialog>

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
  hbox->addStretch(4);

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
  m_tableWidget->setShowGrid(true);
  m_tableWidget->setAlternatingRowColors(false);
  m_tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
  m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);

  //m_tableWidget->verticalHeader()->setSectionsMovable(true);
//  m_tableWidget->setSortingEnabled(true);
//  m_tableWidget->verticalHeader()->setSortIndicatorShown(true);
//  m_tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
//  m_tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::Interactive);


  m_tableWidget->installEventFilter(this);

  QString str;
  str = QString("QTableWidget {"					\
		"selection-background-color: qlineargradient("		\
		"x1:0, y1:0, x2:0, y2:1, "				\
		"stop:0 darkgray,  stop:0.4 lightgray, "		\
		"stop:0.6 lightgray, stop:1 darkgray);"			\
		"}");
  m_tableWidget->setStyleSheet(str);

  modifyTableWidget();


  m_replaceTF = new QCheckBox("Replace existing transfer functions at keyframes");  
  m_morphTF = new QCheckBox("Morph transfer functions during keyframe animation");  

  QVBoxLayout *vbox = new QVBoxLayout();
  vbox->addWidget(m_buttonGroup);
  vbox->addWidget(m_replaceTF);
  vbox->addWidget(m_morphTF);
  vbox->addWidget(m_tableWidget);

  setLayout(vbox);

  connect(m_morphTF, SIGNAL(stateChanged(int)),
	  this, SLOT(morphChanged(int)));

  connect(m_replaceTF, SIGNAL(stateChanged(int)),
	  this, SLOT(replaceChanged(int)));

  connect(m_tableWidget->horizontalHeader(), SIGNAL(sectionClicked(int)),
	  this, SLOT(headerClicked(int)));

  connect(m_tableWidget, SIGNAL(cellClicked(int, int)),
	  this, SLOT(cellClicked(int, int)));

  connect(m_tableWidget, SIGNAL(cellChanged(int, int)),
	  this, SLOT(cellChanged(int, int)));
}

void
TransferFunctionManager::updateReplace(bool flag)
{
  Global::setReplaceTF(flag);
  if (flag)
    m_replaceTF->setCheckState(Qt::Checked);
  else
    m_replaceTF->setCheckState(Qt::Unchecked);
}

void
TransferFunctionManager::replaceChanged(int state)
{
  bool flag = false;
  if (state == Qt::Checked) flag = true;
  Global::setReplaceTF(flag);
}


void
TransferFunctionManager::updateMorph(bool flag)
{
  Global::setMorphTF(flag);
  if (flag)
    m_morphTF->setCheckState(Qt::Checked);
  else
    m_morphTF->setCheckState(Qt::Unchecked);
}

void
TransferFunctionManager::morphChanged(int state)
{
  bool flag = false;
  if (state == Qt::Checked) flag = true;
  Global::setMorphTF(flag);
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
				"Image Files (*.png *.tif *.bmp *.jpg)");

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
		        "Image Files (*.png *.tif *.bmp *.jpg)");
      
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
    item << QString("%1").arg(i+1);

  m_tableWidget->setHorizontalHeaderLabels(item);

  for (int i=0; i<maxSets; i++)
    m_tableWidget->setColumnWidth(i+1, 30);

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

  int cols = m_tableWidget->columnCount();
  int rows = m_tableWidget->rowCount();

  m_tableWidget->setRangeSelected(QTableWidgetSelectionRange(0, 0, rows-1, cols-1), false);
  m_tableWidget->setRangeSelected(QTableWidgetSelectionRange(row, 0, row, cols-1), true);

  emit changeTransferFunctionDisplay(row, on);
  //qApp->processEvents();
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
      //qApp->processEvents();
    }
}


void
TransferFunctionManager::refreshManager(int tfno)
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
      m_tableWidget->setRowHeight(rows, 40);

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
  
  QList<bool> on;

  //if (rows > 0)
  if (tfno >= 0 && tfno < rows)
    {
      m_tableWidget->setRangeSelected(QTableWidgetSelectionRange(0, 0, rows-1, cols-1), false);
      m_tableWidget->setRangeSelected(QTableWidgetSelectionRange(tfno, 0, rows-1, cols-1), true);
      m_tableWidget->setCurrentCell(tfno,0);
      for (int i=0; i<m_tfContainer->maxSets(); i++)
	{
	  QTableWidgetItem *wi;
	  wi = m_tableWidget->item(tfno, i+1);
	  bool flag = wi->checkState() == Qt::Checked;
	  on.append(flag);
	}

      emit changeTransferFunctionDisplay(tfno, on);      
    }

  if (rows == 0)
    {
      emit changeTransferFunctionDisplay(-1, on);
    }
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
TransferFunctionManager::loadDefaultTF()
{
  SplineTransferFunction splineTF;
  SplineInformation si = splineTF.getSpline();

  QPolygonF poly;

  poly << QPointF(0.5, 1.0)
       << QPointF(0.5, 0.2);
  si.setPoints(poly);
  poly.clear();

  poly << QPointF(0.45, 0.45)
       << QPointF(0.45, 0.45);
  si.setNormalWidths(poly);

  int cols = m_tableWidget->columnCount();
  QList<bool> on;
  for(int i=0; i<cols; i++) on << false;  

  QGradientStops stops;
  if (Global::volumeType() == Global::RGBVolume ||
      Global::volumeType() == Global::RGBAVolume)
    {
      poly.clear();
      poly << QPointF(0.5, 1.0)
	   << QPointF(0.5, 0.0);
      si.setPoints(poly);

      poly.clear();      
      poly << QPointF(0.5, 0.5)
	   << QPointF(0.5, 0.5);
      si.setNormalWidths(poly);


      stops.clear();
      stops << QGradientStop(0, QColor(255, 255, 255, 0))
	    << QGradientStop(0.5, QColor(255, 255, 255, 200))
	    << QGradientStop(1, QColor(255, 255, 255, 250));
      si.setGradientStops(stops);

      for(int i=0; i<cols; i++) on[i] = false;  
      on[0] = true;
      si.setOn(on);
      si.setName("red");
      m_tfContainer->fromSplineInformation(si);  

      for(int i=0; i<cols; i++) on[i] = false;  
      on[1] = true;
      si.setOn(on);
      si.setName("green");
      m_tfContainer->fromSplineInformation(si);  

      for(int i=0; i<cols; i++) on[i] = false;  
      on[2] = true;
      si.setOn(on);
      si.setName("blue");
      m_tfContainer->fromSplineInformation(si);  

      if (Global::volumeType() == Global::RGBAVolume)
	{
	  for(int i=0; i<cols; i++) on[i] = false;  
	  on[3] = true;
	  si.setOn(on);
	  si.setName("alpha");
	  m_tfContainer->fromSplineInformation(si);
	}
    }
  else if (Global::volumeType() != Global::DummyVolume)
    {
      if (Global::volumeType() >= Global::SingleVolume)
	{
	  for(int i=0; i<cols; i++) on[i] = false;  
	  on[0] = true;
	  si.setOn(on);
	  si.setName("TF0");
	  stops.clear();
	  stops << QGradientStop(0, QColor(147, 80, 0, 0))
		<< QGradientStop(0.5, QColor(255, 241, 170, 128))
		<< QGradientStop(1, QColor(255, 255, 255, 0));
	  si.setGradientStops(stops);
	  m_tfContainer->fromSplineInformation(si);  
	}

      if (Global::volumeType() >= Global::DoubleVolume)
	{
	  for(int i=0; i<cols; i++) on[i] = false;  
	  on[1] = true;
	  si.setOn(on);
	  si.setName("TF1");
	  stops.clear();
	  stops << QGradientStop(0, QColor(255, 255, 255, 0))
		<< QGradientStop(0.5, QColor(255, 83, 0, 128))
		<< QGradientStop(1, QColor(147, 45, 0, 0));
	  si.setGradientStops(stops);
	  m_tfContainer->fromSplineInformation(si);  
	}
      
      if (Global::volumeType() >= Global::TripleVolume)
	{
	  for(int i=0; i<cols; i++) on[i] = false;  
	  on[2] = true;
	  si.setOn(on);
	  si.setName("TF2");
	  stops.clear();
	  stops << QGradientStop(0, QColor(255, 255, 255, 0))
		<< QGradientStop(0.5, QColor(25, 255, 50, 128))
		<< QGradientStop(1, QColor(10, 100, 0, 0));
	  si.setGradientStops(stops);
	  m_tfContainer->fromSplineInformation(si);  
	}

      if (Global::volumeType() == Global::QuadVolume)
	{
	  for(int i=0; i<cols; i++) on[i] = false;  
	  on[3] = true;
	  si.setOn(on);
	  si.setName("TF3");
	  stops.clear();
	  stops << QGradientStop(0, QColor(255, 255, 255, 0))
		<< QGradientStop(0.5, QColor(25, 50, 255, 128))
		<< QGradientStop(1, QColor(0, 10, 100, 0));
	  si.setGradientStops(stops);
	  m_tfContainer->fromSplineInformation(si);  
	}
    }

  refreshManager(0);
}

void
TransferFunctionManager::addNewTransferFunction()
{
  m_tfContainer->addSplineTF();
  int rows = m_tableWidget->rowCount();
  SplineTransferFunction *tf = m_tfContainer->transferFunctionPtr(rows);
  tf->setName(QString("TF %1").arg(rows));

  refreshManager(rows);
}

void
TransferFunctionManager::removeTransferFunction()
{
  if (!m_tfContainer) return;

  QList<QTableWidgetSelectionRange> sel = m_tableWidget->selectedRanges();
  if (sel.count() > 0)
    {
      m_tfContainer->removeSplineTF(sel[0].topRow());
      refreshManager(0);
    }
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

  refreshManager(0);
}

void
TransferFunctionManager::append(const char *flnm)
{
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

  refreshManager(0);
}

void
TransferFunctionManager::load(QList<SplineInformation> splineInfo)
{
  m_tfContainer->clearContainer();

  for(int i=0; i<splineInfo.count(); i++)
    m_tfContainer->fromSplineInformation(splineInfo[i]);

  refreshManager(0);
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

void
TransferFunctionManager::applyUndo(bool flag)
{
  m_tfContainer->applyUndo(m_tableWidget->currentRow(), flag);
  refreshTransferFunction();
}

void
TransferFunctionManager::transferFunctionUpdated()
{
  m_tfContainer->transferFunctionUpdated(m_tableWidget->currentRow());
}

void
TransferFunctionManager::keyPressEvent(QKeyEvent *event)
{
  if (event->modifiers() & Qt::ControlModifier ||
      event->modifiers() & Qt::MetaModifier)
    {
      if (event->key() == Qt::Key_H)
	showHelp();
      else if (event->key() == Qt::Key_D)
	{
	  QList<QTableWidgetSelectionRange> sel = m_tableWidget->selectedRanges();
	  if (sel.count() > 0)
	    {
	      int rows = m_tableWidget->rowCount();
	      int dupTF = sel[0].topRow();
	      QMessageBox::information(0, "", QString("Duplicating %1").arg(dupTF));
	      m_tfContainer->duplicate(dupTF);
	      refreshManager(rows);
	    }
	}
    }
}

bool
TransferFunctionManager::eventFilter(QObject *obj, QEvent *event)
{
  if (obj == m_tableWidget)
    {
      if (event->type() == QEvent::KeyPress)
	{
	  QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
	  if ( (keyEvent->modifiers() & Qt::ControlModifier ||
		keyEvent->modifiers() & Qt::MetaModifier) &&
	       (keyEvent->key() == Qt::Key_D ||
		keyEvent->key() == Qt::Key_H) )
	    {
	      keyPressEvent(keyEvent);
	      return true;
	    }
	}
      return false;
    }
  
  return TransferFunctionManager::eventFilter(obj, event);
}

void
TransferFunctionManager::showHelp()
{
  PropertyEditor propertyEditor;
  QMap<QString, QVariantList> plist;
  QVariantList vlist;

  vlist.clear();
  QFile helpFile(":/transferfunctioneditor.help");
  if (helpFile.open(QFile::ReadOnly))
    {
      QTextStream in(&helpFile);
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
  
  propertyEditor.set("Transfer Function Manager Help", plist, keys);
  propertyEditor.exec();
}
