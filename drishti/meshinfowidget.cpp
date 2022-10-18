#include "meshinfowidget.h"

#include <QGuiApplication>
#include <QMessageBox>
#include <QHeaderView>
#include <QMenu>
#include <QAction>
#include <QInputDialog>
#include <QItemEditorFactory>
#include <QItemEditorCreatorBase>
#include <QItemDelegate>

#include "dcolordialog.h"
#include "propertyeditor.h"
#include "opacityeditor.h"

MeshInfoWidget::MeshInfoWidget(QWidget *parent) :
  QWidget(parent)
{
  ui.setupUi(this);
  resize(100, 100);

  //ui.Command->hide();

  m_updatingTable = true;

  //--- set background color ---
  QPalette pal = QPalette();
  pal.setColor(QPalette::Window, QColor("gainsboro")); 
  setAutoFillBackground(true);
  setPalette(pal);
  //----------------------------
  
  m_matcapDialog = new ImgListDialog();
  m_matcapDialog->hide();

  
  m_meshList = new QTableWidget();
  m_meshList->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_meshList->setShowGrid(false);
  m_meshList->setColumnCount(6);
  m_meshList->setFrameStyle(QFrame::NoFrame);

  
  QItemEditorCreatorBase *blendCreator = new QStandardItemEditorCreator<OpacityEditor>();
  QItemEditorFactory *blendFactory = new QItemEditorFactory;
  blendFactory->registerEditor(QVariant::Double, blendCreator);
  QItemDelegate *blendDelegate = new QItemDelegate();
  blendDelegate->setItemEditorFactory(blendFactory);
  m_meshList->setItemDelegateForColumn(5, blendDelegate); // material blend

  
  QStringList hlabels;
  hlabels << tr("Mesh Name");
  hlabels << tr("V  ");
  hlabels << tr("X");
  hlabels << tr("C");
  hlabels << tr("Mat");
  hlabels << tr("Blend");
  m_meshList->setHorizontalHeaderLabels(hlabels);  
  m_meshList->setColumnWidth(1, 20);
  m_meshList->setColumnWidth(2, 20);
  m_meshList->setColumnWidth(3, 20);
  m_meshList->setColumnWidth(4, 50);
  m_meshList->setColumnWidth(5, 50);
  
  
  QString ss;
  ss += "QTableWidget::item:selected{border-style: solid; ";
  ss += "border-width : 2px; ";
  ss += "border-radius : 5px; ";
  ss += "border-color: black; ";
  ss += "background-color: lightGray;";
  ss += "selection-color: black;";
  ss += "}";
  m_meshList->setStyleSheet(ss);
  
  
  QVBoxLayout *vbox = new QVBoxLayout();
  vbox->setContentsMargins(0,0,0,0);
  vbox->addWidget(m_meshList);
  ui.MeshList->setLayout(vbox);
  
  m_prevRow = -1;

  connect(m_meshList, SIGNAL(itemChanged(QTableWidgetItem*)),
	  this, SLOT(itemChanged(QTableWidgetItem*)));

  connect(m_meshList, SIGNAL(cellClicked(int, int)),
	  this, SLOT(cellClicked(int, int)));

  connect(m_meshList->horizontalHeader(), SIGNAL(sectionClicked(int)),
	  this, SLOT(sectionClicked(int)));

  connect(m_meshList->horizontalHeader(), SIGNAL(sectionDoubleClicked(int)),
	  this, SLOT(sectionDoubleClicked(int)));  

  
  m_meshList->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_meshList, SIGNAL(customContextMenuRequested(const QPoint&)),
	  this, SLOT(meshListContextMenu(const QPoint&)));

  
  connect(ui.transparency, SIGNAL(valueChanged(int)),
	  this, SIGNAL(transparencyChanged(int)));
  connect(ui.reveal, SIGNAL(valueChanged(int)),
	  this, SIGNAL(revealChanged(int)));
  connect(ui.outline, SIGNAL(valueChanged(int)),
	  this, SIGNAL(outlineChanged(int)));
  connect(ui.glow, SIGNAL(valueChanged(int)),
	  this, SIGNAL(glowChanged(int)));
  connect(ui.darken, SIGNAL(valueChanged(int)),
	  this, SIGNAL(darkenChanged(int)));
  connect(ui.position, SIGNAL(editingFinished()),
	  this, SLOT(positionChanged()));
  connect(ui.scale, SIGNAL(editingFinished()),
	  this, SLOT(scaleChanged()));

  ui.MeshParamBox->hide();

  m_totVertices = 0;
  m_totTriangles = 0;


//  connect(ui.rotationMode, SIGNAL(clicked(bool)),
//	  this, SIGNAL(rotationMode(bool)));
//
  connect(ui.grabMode, SIGNAL(clicked(bool)),
	  this, SIGNAL(grabMesh(bool)));
  connect(ui.grabMode, SIGNAL(clicked(bool)),
	  this, SLOT(changeSelectionMode(bool)));
//	  
//  ui.rotationMode->setIcon(QIcon(QString::fromUtf8(":/images/rot.png")));
//  ui.grabMode->setIcon(QIcon(QString::fromUtf8(":/images/grab.png")));
}

void
MeshInfoWidget::changeSelectionMode(bool flag)
{
  if (flag)
    m_meshList->setSelectionMode(QAbstractItemView::MultiSelection);
  else
    m_meshList->setSelectionMode(QAbstractItemView::ExtendedSelection);
}

MeshInfoWidget::~MeshInfoWidget()
{
  m_meshList->setRowCount(0);
  m_prevRow = -1;
}

void
MeshInfoWidget::setActive(int idx)
{  
  if (idx > -1 && idx < m_meshList->rowCount())
    {
      if (QGuiApplication::keyboardModifiers() & Qt::ControlModifier)
	m_meshList->setSelectionMode(QAbstractItemView::MultiSelection);
      else
	m_meshList->setSelectionMode(QAbstractItemView::ExtendedSelection);

      m_meshList->setCurrentCell(idx, 0);
      
      //---------------
      QList<int> selIdx;
	for(int i=0; i<m_meshList->rowCount(); i++)
	  {
	    QTableWidgetItem *wi = m_meshList->item(i, 0);
	    QColor bgcol = wi->background().color();
	    if (wi->isSelected())
	      selIdx << i;
	  }
	emit multiSelection(selIdx);
      //---------------

      
      QTableWidgetItem *wi = m_meshList->item(idx, 0);
      QColor bgcol = wi->background().color();
      if (wi->isSelected())
	emit setActive(idx, true);
      else
	{
	  if (selIdx.count() > 0)
	    emit setActive(selIdx[0], true);
	  else
	    emit setActive(idx, false);
	}
	
      ui.MeshParamBox->show();
      m_prevRow = idx;
    }
}

void
MeshInfoWidget::addMesh(QString meshName, bool show, bool clip, QString color, int matId, float matMix)
{
  {
    QTableWidgetItem *wi = m_meshList->item(m_prevRow, 0);
    if (!wi) wi = new QTableWidgetItem("");
    //wi->setFont(QFont("MS Reference Sans Serif", 12));
    wi->setText(meshName);
    wi->setFlags(wi->flags() & ~Qt::ItemIsUserCheckable & ~Qt::ItemIsEditable);
    m_meshList->setItem(m_prevRow, 0, wi);
  }
  
  {
    QTableWidgetItem *wi = m_meshList->item(m_prevRow, 1);
    if (!wi) wi = new QTableWidgetItem("");

    wi->setFlags(wi->flags() & ~Qt::ItemIsUserCheckable & ~Qt::ItemIsEditable & ~Qt::ItemIsSelectable);
    wi->setTextAlignment(Qt::AlignCenter);
    wi->setForeground(QBrush(Qt::white));
    //wi->setFont(QFont("Arial Rounded MT Bold", 14));
    if (show)
      {
	wi->setBackground(QBrush(Qt::darkGray));	
	wi->setText("X");
	//wi->setText(u8"\u2705");
      }
    else
      {
	wi->setBackground(QBrush(Qt::lightGray));
	wi->setText("");
      }

    m_meshList->setItem(m_prevRow, 1, wi);
  }

  {
    QTableWidgetItem *wi = m_meshList->item(m_prevRow, 2);
    if (!wi) wi = new QTableWidgetItem("");

    wi->setFlags(wi->flags() & ~Qt::ItemIsUserCheckable & ~Qt::ItemIsEditable & ~Qt::ItemIsSelectable);
    wi->setTextAlignment(Qt::AlignCenter);
    wi->setForeground(QBrush(Qt::white));
    //wi->setFont(QFont("Arial Rounded MT Bold", 14));
    if (clip)
      {
	wi->setBackground(QBrush(Qt::darkGray));	
	//wi->setText(u8"\u2705");
	wi->setText("X");
      }
    else
      {
	wi->setBackground(QBrush(Qt::lightGray));
	wi->setText("");
      }

    m_meshList->setItem(m_prevRow, 2, wi);
  }

  {
    QTableWidgetItem *wi = m_meshList->item(m_prevRow, 3);
    if (!wi) wi = new QTableWidgetItem("");

    wi->setFlags(wi->flags() & ~Qt::ItemIsUserCheckable & ~Qt::ItemIsEditable & ~Qt::ItemIsSelectable);
    m_meshList->setItem(m_prevRow, 3, wi);
    wi->setBackground(QBrush(QColor(color)));
    wi->setForeground(QBrush(QColor(color)));
  }

  {
    QTableWidgetItem *wi = m_meshList->item(m_prevRow, 4);
    if (!wi) wi = new QTableWidgetItem("");

    wi->setFlags(wi->flags() & ~Qt::ItemIsUserCheckable & ~Qt::ItemIsEditable & ~Qt::ItemIsSelectable);
    m_meshList->setItem(m_prevRow, 4, wi);
    wi->setText(QString("%1").arg(matId));
  }

  {
    QTableWidgetItem *wi = m_meshList->item(m_prevRow, 5);
    if (!wi) wi = new QTableWidgetItem("");
    wi->setData(Qt::DisplayRole, (double)matMix);
    m_meshList->setItem(m_prevRow, 5, wi);
  }
}

void
MeshInfoWidget::setMeshes(QStringList meshNames)
{
  m_updatingTable = true;
  
  m_meshNames.clear();
  m_vertCount.clear();
  m_triCount.clear();
  m_totVertices = 0;
  m_totTriangles = 0;
  
  QList<bool> show;
  QList<bool> clip;
  QStringList color;
  QList<int> material;
  QList<float> blend;
  foreach(QString mesh, meshNames)
    {
      QString lstr = mesh.left(5);
      int len = lstr.toInt();
 
      QString name = mesh.mid(6, len);
      m_meshNames << name;


      QString rem = mesh.mid(7+len, -1);
      QStringList words = rem.split(" ", QString::SkipEmptyParts);

      show << (words[0].toInt() > 0);
      clip << (words[1].toInt() > 0);
      color << words[2];
      material << words[3].toInt();
      blend << words[4].toFloat();
      m_vertCount << words[5].toInt();
      m_triCount << words[6].toInt();
      m_totVertices += words[5].toInt();
      m_totTriangles += words[6].toInt();;
    }

  
  m_meshList->setRowCount(meshNames.count());
  
  m_prevRow = -1;
  for(int i=0; i<m_meshNames.count(); i++)
    {
      m_prevRow++;
      addMesh(m_meshNames[i], show[i], clip[i], color[i], material[i], blend[i]);
    }

  m_meshList->resizeColumnToContents(0);
//  m_meshList->resizeColumnToContents(1);
//  m_meshList->resizeColumnToContents(2);

  ui.grabMode->setChecked(false);
  changeSelectionMode(false);
  emit grabMesh(false);

  m_prevRow = -1;
  m_meshList->setCurrentCell(-1,-1);
  emit setActive(-1, false);
  emit multiSelection(QList<int>());

  
  ui.MeshParamBox->hide();

  m_updatingTable = false;  
}

void
MeshInfoWidget::sectionClicked(int col)
{
  if (col == 0 || col > 4)
    return;
  
  if (col >= 3) // colormap/material
    {
      QList<int> selIdx;
      for(int i=0; i<m_meshList->rowCount(); i++)
	{
	  QTableWidgetItem *wi = m_meshList->item(i, 0);
	  if (wi->isSelected())
	    {
	      selIdx << i;
	    }
	}


      //-------
      if (col == 3) // colormap
	{
	  if (selIdx.count() == 1)
	    {
	      int row = selIdx[0];
	      QTableWidgetItem *wi;
	      wi = m_meshList->item(row, col);
	      QColor pcolor = wi->background().color();
	      QColor color = DColorDialog::getColor(pcolor);
	      if (color.isValid())
		{
		  wi->setBackground(QBrush(color));
		  wi->setForeground(QBrush(color));
		  emit colorChanged(color);
		}
	    }
	  else if (selIdx.count() > 1)
	    emit processCommand(selIdx, "colormap");
	  else
	    emit processCommand("colormap");

	  return;
	}
      //-------

      //-------
      if (col == 4) // material
	{
	  if (selIdx.count() == 1)
	    {
	      int row = selIdx[0];
	      QTableWidgetItem *wi;
	      wi = m_meshList->item(row, col);
	      int matId = wi->text().toInt();	      
	      matId = m_matcapDialog->getImageId(matId);
	      wi->setText(QString("%1").arg(matId));
	      emit materialChanged(matId);
	    }
	  else
	    {
	      if (selIdx.count() == 0)
		{
		  for(int i=0; i<m_meshList->rowCount(); i++)
		    selIdx << i;
		}
	      int matId = m_matcapDialog->getImageId(0);
	      emit materialChanged(selIdx, matId);
	      for(int i=0; i<m_meshList->rowCount(); i++)
		m_meshList->item(i, 4)->setText(QString("%1").arg(matId));
	    }

	  return;
	}
      //-------
    }

  // 1/2 = visibility/clip
  QList<bool> CS;
  for(int i=0; i<m_meshList->rowCount(); i++)
    {	  
      QTableWidgetItem *wi;
      wi = m_meshList->item(i, col);
      QColor bgcol = wi->background().color();
      bool cs = true;
      if (bgcol.red() < 150)
	{
	  wi->setText("");
	  wi->setBackground(QBrush(Qt::lightGray));
	  cs = false;
	}
      else
	{
	  wi->setBackground(QBrush(Qt::darkGray));
	  //wi->setText(u8"\u2705");
	  wi->setText("X");
	  cs = true;
	}

      CS << cs;
//      if (col == 1)
//	emit setVisible(i, CS);
//      else
//	emit setClip(i, CS);
    }
  if (col == 1)
    emit setVisible(CS);
  else
    emit setClip(CS);
}

void
MeshInfoWidget::sectionDoubleClicked(int col)
{
  if (col == 0 || col >= 3)
    return;

  if (m_meshList->rowCount() <= 0)
    return;

  
  // 1/2 = visibility/clip
  bool cs0;
  QTableWidgetItem *wi;
  wi = m_meshList->item(0, col);
  QColor bgcol = wi->background().color();
  if (bgcol.red() < 150)
    cs0 = true;
  else
    cs0 = false;

  QList<bool> CS;
  for(int i=0; i<m_meshList->rowCount(); i++)
    {	  
      QTableWidgetItem *wi;
      wi = m_meshList->item(i, col);
      QColor bgcol = wi->background().color();
      CS << cs0;
      if (cs0)
	{
	  wi->setBackground(QBrush(Qt::darkGray));
	  //wi->setText(u8"\u2705");
	  wi->setText("X");
	}
      else
	{
	  wi->setText("");
	  wi->setBackground(QBrush(Qt::lightGray));
	}
      
//      if (col == 1)
//	emit setVisible(i, CS);
//      else
//	emit setClip(i, CS);
    }
  if (col == 1)
    emit setVisible(CS);
  else
    emit setClip(CS);
}

void
MeshInfoWidget::cellClicked(int row, int col)
{
  QList<int> selIdx;
  for(int i=0; i<m_meshList->rowCount(); i++)
    {
      QTableWidgetItem *wi = m_meshList->item(i, 0);
      if (wi->isSelected())
	{
	  selIdx << i;
	}
    }

//  if (selIdx.count() > 1)
//    {
//      emit setActive(-1, false);
//      ui.MeshParamBox->hide();
//      return;
//    }

  if (selIdx.count() > 1)
    {
      emit setActive(selIdx[0], true);
      ui.MeshParamBox->setStyleSheet("QWidget{background:beige;}");
    }
  else
    ui.MeshParamBox->setStyleSheet("QWidget{background:gainsboro;}");

  
  emit multiSelection(selIdx);

  
  //-----------
  if (selIdx.count() > 1 &&
      selIdx.contains(row) &&
      (col == 1 || col == 2))
    {      
      QTableWidgetItem *wi;
      wi = m_meshList->item(row, col);
      QColor bgcol = wi->background().color();
      bool CS = true;
      if (bgcol.red() < 150)
	{
	  wi->setText("");
	  wi->setBackground(QBrush(Qt::lightGray));
	  CS = false;
	}
      else
	{
	  //wi->setText(u8"\u2705");
	  wi->setText("X");
	  wi->setBackground(QBrush(Qt::darkGray));
	  CS = true;
	}

      for(int id=0; id<selIdx.count(); id++)
	{
	  int i = selIdx[id];
	  
	  QTableWidgetItem *wi;
	  wi = m_meshList->item(i, col);
	  
	  if (CS)
	    {
	      wi->setBackground(QBrush(Qt::darkGray));
	      //wi->setText(u8"\u2705");
	      wi->setText("X");
	    }
	  else
	    {
	      wi->setText("");
	      wi->setBackground(QBrush(Qt::lightGray));
	    }
	  
	  if (col == 1)
	    emit setVisible(i, CS);
	  else
	    emit setClip(i, CS);
	}

      return;
    }
    //-----------

    //-----------
    if (selIdx.count() > 1 &&
      selIdx.contains(row) &&
      col == 3)
    {
      QTableWidgetItem *wi;
      wi = m_meshList->item(row, col);
      QColor pcolor = wi->background().color();
      QColor color = DColorDialog::getColor(pcolor);
      if (color.isValid())
	{
	  for(int id=0; id<selIdx.count(); id++)
	    {
	      int i = selIdx[id];
	      wi = m_meshList->item(i, col);
	      wi->setBackground(QBrush(color));
	      wi->setForeground(QBrush(color));
	    }
	  
	  emit colorChanged(selIdx, color);
	}

      return;
    }
    //-----------
    
    //-----------
    if (selIdx.count() > 1 &&
      selIdx.contains(row) &&
      col == 4)
    {
      QTableWidgetItem *wi;
      wi = m_meshList->item(row, col);
      int matId = wi->text().toInt();	      
      matId = m_matcapDialog->getImageId(matId);
      wi->setText(QString("%1").arg(matId));
      emit materialChanged(matId);

      for(int id=0; id<selIdx.count(); id++)
	{
	  int i = selIdx[id];
	  wi = m_meshList->item(i, col);
	  wi->setText(QString("%1").arg(matId));
	}
      emit materialChanged(selIdx, matId);

      return;
    }
    //-----------

    //-----------
    if (selIdx.count() > 1 &&
      selIdx.contains(row) &&
      col == 5)
    {
      QTableWidgetItem *wi;
      wi = m_meshList->item(row, col);
      float matMix = wi->data(Qt::DisplayRole).toDouble();	      
      emit materialMixChanged(matMix);

      for(int id=0; id<selIdx.count(); id++)
	{
	  int i = selIdx[id];
	  wi = m_meshList->item(i, col);
	  wi->setData(Qt::DisplayRole, (double)matMix);
	}
      emit materialMixChanged(selIdx, matMix);

      return;
    }
    //-----------
    
    
    if (selIdx.count() > 1)
      {
	m_prevRow = row;
	return;
      }
    
    if (m_meshList->selectionMode() != QAbstractItemView::ExtendedSelection)
      {    
	if (col == 0)
	  {
	    if (selIdx.count() > 0)
	      {
		emit setActive(selIdx[0], true);
		ui.MeshParamBox->show();
	      }
	    else
	      {
		emit setActive(-1, false);
		m_meshList->setCurrentCell(-1,-1);
		ui.MeshParamBox->hide();
	      }
	    return;
	  }
	return;
      }
    
    
    //-----------
    if (col == 0)
      {      
	if (m_prevRow != -1)
	  emit setActive(m_prevRow, false);
	
	if (row == m_prevRow)
	  {
	    row = -1;
	    m_meshList->setCurrentCell(-1,-1);
	    ui.MeshParamBox->hide();
	  }
	else
	  {
	    emit setActive(row, true);
	    ui.MeshParamBox->show();
	  }
	m_prevRow = row;
      
      return;
      }
    //-----------
    
    //-----------
    // for single selection
    if (col == 1 || col == 2)
      {
	m_meshList->setCurrentCell(row, col);
	if (m_prevRow != -1)
	  emit setActive(m_prevRow, false);
	
	m_prevRow = row;
	emit setActive(row, true);
	ui.MeshParamBox->show();
	
	QTableWidgetItem *wi;
	wi = m_meshList->item(row, col);
	
	QColor bgcol = wi->background().color();
	bool flag = true;
	if (bgcol.red() < 150)
	  {
	    wi->setText("");
	    wi->setBackground(QBrush(Qt::lightGray));
	    flag = false;
	  }
	else
	  {
	    //wi->setText(u8"\u2705");
	    wi->setText("X");
	    wi->setBackground(QBrush(Qt::darkGray));
	    flag = true;
	  }
	
	if (col == 1)
	  emit setVisible(row, flag);
	else
	  emit setClip(row, flag);
	
	return;
      }
    //-----------
    
    
    //-----------
    if (col == 3) // color
      {
	m_meshList->setCurrentCell(row, col);
	if (m_prevRow != -1)
	  emit setActive(m_prevRow, false);
	
	m_prevRow = row;
	emit setActive(row, true);
	ui.MeshParamBox->show();
	
	QTableWidgetItem *wi;
	wi = m_meshList->item(row, col);
	QColor pcolor = wi->background().color();
	
	QColor color = DColorDialog::getColor(pcolor);
	if (color.isValid())
	  {
	    wi->setBackground(QBrush(color));
	    wi->setForeground(QBrush(color));
	    emit colorChanged(color);
	  }
	
	return;
      }
    //-----------

    //-----------
    if (col == 4) // material
      {
	m_meshList->setCurrentCell(row, col);
	if (m_prevRow != -1)
	  emit setActive(m_prevRow, false);
	
	m_prevRow = row;
	emit setActive(row, true);
	ui.MeshParamBox->show();
	
	QTableWidgetItem *wi;
	wi = m_meshList->item(row, col);
	int matId = wi->text().toInt();	      
	matId = m_matcapDialog->getImageId(matId);
	wi->setText(QString("%1").arg(matId));
	emit materialChanged(matId);
	
	return;
      }
    //-----------
    
    //-----------    
    if (col == 5) // material blend
      {
	m_meshList->setCurrentCell(row, col);
	if (m_prevRow != -1)
	  emit setActive(m_prevRow, false);
	
	m_prevRow = row;
	emit setActive(row, true);
	ui.MeshParamBox->show();
	
//	QTableWidgetItem *wi;
//	wi = m_meshList->item(row, col);
//	float matMix = wi->data(Qt::DisplayRole).toDouble();	      
//	emit materialMixChanged(matMix);
	
	return;
      }
    //-----------
}

void
MeshInfoWidget::itemChanged(QTableWidgetItem *item)
{
  if (m_updatingTable)
    return;
  
  int row = item->row();
  int colm = item->column();
  if (colm == 5)
    {
      QList<int> selIdx;
      for(int i=0; i<m_meshList->rowCount(); i++)
	{
	  QTableWidgetItem *wi = m_meshList->item(i, 0);
	  if (wi->isSelected())
	    selIdx << i;
	}
      
      float matMix = item->data(Qt::DisplayRole).toDouble();	      

      if (selIdx.count() > 1 &&
	  selIdx.contains(row))
	{
	  for(int id=0; id<selIdx.count(); id++)
	    {
	      int i = selIdx[id];
	      QTableWidgetItem *wi = m_meshList->item(i, 5);
	      wi->setData(Qt::DisplayRole, (double)matMix);
	    }
	  emit materialMixChanged(selIdx, matMix);
	}
      else
	{
	  emit materialMixChanged(matMix);
	}
    }
}



void
MeshInfoWidget::meshListContextMenu(const QPoint& pos)
{
  int row = m_meshList->currentRow();
  if (row < 0)
    return;

  QTableWidgetItem *wi = m_meshList->itemAt(pos);
  if (!wi)
    {
      if (m_prevRow != -1)
	emit setActive(m_prevRow, false);

      m_prevRow = -1;

      ui.MeshParamBox->hide();
      return;
    }

  
  if (m_prevRow != -1 && m_prevRow != row)
    emit setActive(m_prevRow, false);

  m_prevRow = row;
  emit setActive(row, true);
  ui.MeshParamBox->show();

  
  QMenu *mContextMenu = new QMenu(this);  

  QAction *delAction = new QAction("Delete", this);
  QAction *saveAction = new QAction("Save", this);
  //QAction *dupAction = new QAction("Duplicate", this);

  connect(delAction, SIGNAL(triggered()), this, SLOT(removeMesh()));
  connect(saveAction, SIGNAL(triggered()), this, SLOT(saveMesh()));
  //connect(dupAction, SIGNAL(triggered()), this, SLOT(duplicateMesh()));

  mContextMenu->addAction(delAction);
  mContextMenu->addSeparator();
  mContextMenu->addAction(saveAction);
  //mContextMenu->addSeparator();
  //mContextMenu->addAction(dupAction);

  mContextMenu->exec(m_meshList->mapToGlobal(pos));
}

void
MeshInfoWidget::removeMesh()
{
  QList<int> selIdx;
  for(int i=0; i<m_meshList->rowCount(); i++)
    {
      QTableWidgetItem *wi = m_meshList->item(i, 0);
      if (wi->isSelected())
      selIdx << i;
    }

  int row = -1;

  if (selIdx.count() == 1)
    row = m_meshList->currentRow();

  if (row == -1)
    {
      emit removeMesh(selIdx);
    }
  else    
      emit removeMesh(row);

//  int idx = m_meshList->currentRow();
//  if (idx > -1)
//    {
//      emit removeMesh(idx);
//    }
}

void
MeshInfoWidget::saveMesh()
{
  int idx = m_meshList->currentRow();
  if (idx > -1)
    {
      emit saveMesh(idx);
    }
}

void
MeshInfoWidget::duplicateMesh()
{
  int idx = m_meshList->currentRow();
  if (idx > -1)
    {
      emit duplicateMesh(idx);
    }
}

void
MeshInfoWidget::setParameters(QMap<QString, QVariantList> plist)
{  
  int idx = plist.value("idx")[0].toInt();
  ui.MeshParamBox->setTitle(QString("(%1) %2").	\
			    arg(idx+1).		\
			    arg(m_meshNames[idx]));
      
  QStringList pkeys = plist.keys();
  for(int i=0; i<pkeys.count(); i++)
    {
      QVariantList vlist;

      vlist = plist.value(pkeys[i]);
      
      if (pkeys[i] == "position")
	{
	  ui.position->setText(vlist[0].toString());	  
	}
      else if (pkeys[i] == "scale")
	{
	  ui.scale->setText(vlist[0].toString());	  
	}
      else if (pkeys[i] == "transparency")
	{
	  ui.transparency->setValue(vlist[0].toInt());
	}
      else if (pkeys[i] == "reveal")
	{
	  ui.reveal->setValue(vlist[0].toInt());
	}
      else if (pkeys[i] == "outline")
	{
	  ui.outline->setValue(vlist[0].toInt());
	}
      else if (pkeys[i] == "glow")
	{
	  ui.glow->setValue(vlist[0].toInt());
	}
      else if (pkeys[i] == "darken")
	{
	  ui.darken->setValue(vlist[0].toInt());
	}
    }
}

void
MeshInfoWidget::positionChanged()
{
  QStringList v = ui.position->text().split(" ");
  QVector3D pos(1,1,1);
  float x = v[0].toDouble();
  if (v.count() > 0) pos = QVector3D(x,x,x);
  if (v.count() > 1) pos.setY(v[1].toDouble());
  if (v.count() > 2) pos.setZ(v[2].toDouble());

  ui.position->setText(QString("%1 %2 %3").
		   arg(pos.x()).
		   arg(pos.y()).
		   arg(pos.z()));

  emit positionChanged(pos);
}

void
MeshInfoWidget::scaleChanged()
{
  QStringList v = ui.scale->text().split(" ");
  QVector3D scale(1,1,1);
  float x = v[0].toDouble();
  if (v.count() > 0) scale = QVector3D(x,x,x);
  if (v.count() > 1) scale.setY(v[1].toDouble());
  if (v.count() > 2) scale.setZ(v[2].toDouble());

  ui.scale->setText(QString("%1 %2 %3").
		   arg(scale.x()).
		   arg(scale.y()).
		   arg(scale.z()));

  emit scaleChanged(scale);
}

void
MeshInfoWidget::on_Command_pressed()
{
  QList<int> selIdx;
  for(int i=0; i<m_meshList->rowCount(); i++)
    {
      QTableWidgetItem *wi = m_meshList->item(i, 0);
      if (wi->isSelected())
      selIdx << i;
    }
  
  int row = -1;

  if (selIdx.count() == 1)
    row = m_meshList->currentRow();

  PropertyEditor propertyEditor;

  //--- set background color ---
  QPalette pal = QPalette();
  pal.setColor(QPalette::Window, QColor("gainsboro")); 
  propertyEditor.setAutoFillBackground(true);
  propertyEditor.setPalette(pal);
  //----------------------------

  
  QMap<QString, QVariantList> plist;
  
  QVariantList vlist;

  //------------
  vlist.clear();
  QString mesg;
  if (row != -1)
    {
      mesg += "- " + m_meshNames[row] + " -";
      mesg += "\n";
      mesg += QString("Vertices : %1\nTriangles : %2\n").	\
	arg(m_vertCount[row]).arg(m_triCount[row]);
    }
  else
    {
      mesg += QString("Total Meshes : %1\n").arg(m_meshNames.count());		      
      mesg += QString("Total Vertices : %1\nTotal Triangles : %2\n").	\
	arg(m_totVertices).arg(m_totTriangles);
    }  
  vlist << mesg;
  plist["message"] = vlist;
  //------------
  

  //------------
  vlist.clear();
  QFile helpFile;

  if (row != -1)
    helpFile.setFileName(":/singlemeshcommand.help");
  else if (selIdx.count() > 1)
    helpFile.setFileName(":/multiplemeshcommand.help");
  else
    helpFile.setFileName(":/globalmeshcommand.help");
  
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
  //------------

  
  QStringList keys;
  keys << "message";
  keys << "commandhelp";
  keys << "command";

  QString pc;
  if (row != -1)
    pc = QString("Process Command - (%1) ").arg(row) + m_meshNames[row].toUpper();
  else if (selIdx.count() > 1)
    pc = QString("Process Command - Selected Meshes");
  else
    pc = "Process Command - All Meshes";
  
  //propertyEditor.set(pc, plist, keys, QStringList());
  propertyEditor.set(pc, plist, keys, false);
  
  if (propertyEditor.exec() != QDialog::Accepted)
    return;
  
  QString cmd = propertyEditor.getCommandString();
  if (!cmd.isEmpty())
    {
      if (row != -1)
	emit processCommand(row, cmd);
      else if (selIdx.count() > 1)
	emit processCommand(selIdx, cmd);
      else
	emit processCommand(cmd);
    }
  
}

void
MeshInfoWidget::matcapFiles(QStringList list)
{
  m_matcapDialog->setIcons(list);
}
