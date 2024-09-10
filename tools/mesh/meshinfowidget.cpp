#include "meshinfowidget.h"
#include "global.h"

#include <QGuiApplication>
#include <QMessageBox>
#include <QHeaderView>
#include <QMenu>
#include <QAction>
#include <QInputDialog>
#include <QItemEditorFactory>
#include <QItemEditorCreatorBase>
#include <QItemDelegate>
#include <QInputDialog>

#include "dcolordialog.h"
#include "propertyeditor.h"
#include "opacityeditor.h"

MeshInfoWidget::MeshInfoWidget(QWidget *parent) :
  QWidget(parent)
{
  ui.setupUi(this);
  resize(100, 100);


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
  m_meshList->horizontalHeader()->setFont(QFont("MS Reference Sans Serif", 14));
  m_meshList->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_meshList->setShowGrid(false);
  m_meshList->setColumnCount(7);
  m_meshList->setFrameStyle(QFrame::NoFrame);
  m_meshList->setIconSize(QSize(25,25));
  m_meshList->horizontalHeader()->setMinimumSectionSize(40);

  //m_meshList->setAlternatingRowColors(true);
  
  
  QStringList hlabels;
  hlabels << tr("Name");
  hlabels << tr("");     // 1 Visibility
  hlabels << tr("");     // 2 Clip
  hlabels << tr("");     // 3 ClearView
  hlabels << tr("C");    // 4 Color
  hlabels << tr("M");  // 5 Material Id
  hlabels << tr("B");// 6 Blend
  m_meshList->setHorizontalHeaderLabels(hlabels);  
  m_meshList->setColumnWidth(1, 20);
  m_meshList->setColumnWidth(2, 20);
  m_meshList->setColumnWidth(3, 20);
  m_meshList->setColumnWidth(4, 20);
  m_meshList->setColumnWidth(5, 50);
  m_meshList->setColumnWidth(6, 50);

  
  
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
  
  connect(m_meshList, SIGNAL(cellClicked(int, int)),
	  this, SLOT(cellClicked(int, int)));

  connect(m_meshList->horizontalHeader(), SIGNAL(sectionClicked(int)),
	  this, SLOT(sectionClicked(int)));

  connect(m_meshList->horizontalHeader(), SIGNAL(sectionDoubleClicked(int)),
	  this, SLOT(sectionDoubleClicked(int)));

  m_meshList->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_meshList, SIGNAL(customContextMenuRequested(const QPoint&)),
	  this, SLOT(meshListContextMenu(const QPoint&)));
			     

  connect(ui.linemode, SIGNAL(toggled(bool)),
	  this, SIGNAL(lineModeChanged(bool)));
  connect(ui.linewidth, SIGNAL(valueChanged(int)),
	  this, SIGNAL(lineWidthChanged(int)));
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
  connect(ui.rotation, SIGNAL(editingFinished()),
	  this, SLOT(rotationChanged()));
  connect(ui.scale, SIGNAL(editingFinished()),
	  this, SLOT(scaleChanged()));

  ui.MeshParamBox->setFont(QFont("MS Reference Sans Serif", 12));
  ui.MeshParamBox->hide();

  m_totVertices = 0;
  m_totTriangles = 0;


  m_propIcons << QIcon(); // empty icon
  m_propIcons << QIcon(":/images/eye.png"); // visibility icon
  m_propIcons << QIcon(":/images/clipTool.png"); // clip icon
  m_propIcons << QIcon(":/images/clearview.png"); // clear view icon

  m_meshList->horizontalHeaderItem(1)->setIcon(m_propIcons[1]);
  m_meshList->horizontalHeaderItem(2)->setIcon(m_propIcons[2]);
  m_meshList->horizontalHeaderItem(3)->setIcon(m_propIcons[3]);
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

QList<int>
MeshInfoWidget::getSelectedMeshes()
{
  QList<int> selIdx;
  for(int i=0; i<m_meshList->rowCount(); i++)
    {
      QTableWidgetItem *wi = m_meshList->item(i, 0);
      if (wi->isSelected())
	selIdx << i;
    }
  return selIdx;
}

void
MeshInfoWidget::selectMeshes(QList<int> indices)
{
  m_meshList->setSelectionMode(QAbstractItemView::MultiSelection);
  m_meshList->setRangeSelected(QTableWidgetSelectionRange(0, 0, m_meshList->rowCount(), 0), false);

  m_meshList->setCurrentCell(indices[0], 0);
  foreach(int idx, indices)
    {
      QTableWidgetItem *wi = m_meshList->item(idx, 0);
      wi->setSelected(true);
    }
  
  ui.MeshParamBox->show();
  if (indices.count() > 1)
    ui.MeshParamBox->setStyleSheet("QWidget{background:beige;}");
  else
    ui.MeshParamBox->setStyleSheet("QWidget{background:#f2f3f4;}");
}

void
MeshInfoWidget::selectMesh(int idx)
{
  if (idx > -1 && idx < m_meshList->rowCount())
    {
      if (QGuiApplication::keyboardModifiers() & Qt::ControlModifier)
	m_meshList->setSelectionMode(QAbstractItemView::MultiSelection);
      else
	m_meshList->setSelectionMode(QAbstractItemView::ExtendedSelection);
      
      {
	QTableWidgetItem *wi = m_meshList->item(idx, 0);
	bool sel = wi->isSelected();
	m_meshList->setCurrentCell(idx, 0);
	if (sel)
	  wi->setSelected(false);
	else
	  wi->setSelected(true);
      }

      QList<int> selIdx = getSelectedMeshes();
      emit multiSelection(selIdx);
      //---------------

      
      QTableWidgetItem *wi = m_meshList->item(idx, 0);
      if (wi->isSelected())
	emit setActive(idx, true);
      else
	{
	  if (selIdx.count() > 0)
	    emit setActive(selIdx[0], true);
	  else
	    emit setActive(idx, false);
	}

      if (selIdx.count() > 0)
	ui.MeshParamBox->show();
      else
	ui.MeshParamBox->hide();

      m_prevRow = idx;
    }
}

void
MeshInfoWidget::addMesh(QString meshName, bool show,
			bool clip, bool clearView,
			QString color, int matId, float matMix)
{

  // 0 Name
  {
    QTableWidgetItem *wi = m_meshList->item(m_prevRow, 0);
    if (!wi)
      {
	wi = new QTableWidgetItem("");
	m_meshList->setItem(m_prevRow, 0, wi);
      }
    wi->setFont(QFont("MS Reference Sans Serif", 12));
    wi->setText(meshName);
    wi->setFlags(wi->flags() & ~Qt::ItemIsUserCheckable & ~Qt::ItemIsEditable);
  }

  // 1 Visibility
  {
    QTableWidgetItem *wi = m_meshList->item(m_prevRow, 1);
    if (!wi)
      {
	wi = new QTableWidgetItem("");
	m_meshList->setItem(m_prevRow, 1, wi);
      }

    wi->setFlags(wi->flags() & ~Qt::ItemIsUserCheckable & ~Qt::ItemIsEditable & ~Qt::ItemIsSelectable);
    wi->setTextAlignment(Qt::AlignCenter);
    wi->setForeground(QBrush(Qt::white));
    wi->setFont(QFont("Arial Rounded MT Bold", 14));
    if (show)
      {
	wi->setData(Qt::StatusTipRole, "-");
	wi->setIcon(m_propIcons[1]);
      }
    else
      {
	wi->setData(Qt::StatusTipRole, "");
	wi->setIcon(m_propIcons[0]);
      }

  }

  // 2 Clip
  {
    QTableWidgetItem *wi = m_meshList->item(m_prevRow, 2);
    if (!wi)
      {
	wi = new QTableWidgetItem("");
	m_meshList->setItem(m_prevRow, 2, wi);
      }

    wi->setFlags(wi->flags() & ~Qt::ItemIsUserCheckable & ~Qt::ItemIsEditable & ~Qt::ItemIsSelectable);
    if (clip)
      {
	wi->setData(Qt::StatusTipRole, "-");
	wi->setIcon(m_propIcons[2]);
      }
    else
      {
	wi->setData(Qt::StatusTipRole, "");
	wi->setIcon(m_propIcons[0]);
      }

  }

  // 3 ClearView
  {
    QTableWidgetItem *wi = m_meshList->item(m_prevRow, 3);
    if (!wi)
      {
	wi = new QTableWidgetItem("");
	m_meshList->setItem(m_prevRow, 3, wi);
      }

    wi->setFlags(wi->flags() & ~Qt::ItemIsUserCheckable & ~Qt::ItemIsEditable & ~Qt::ItemIsSelectable);
    wi->setTextAlignment(Qt::AlignCenter);
    wi->setForeground(QBrush(Qt::white));
    wi->setFont(QFont("Arial Rounded MT Bold", 14));
    if (clearView)
      {
	wi->setData(Qt::StatusTipRole, "-");
	wi->setIcon(m_propIcons[3]);
      }
    else
      {
	wi->setData(Qt::StatusTipRole, "");
	wi->setIcon(m_propIcons[0]);
      }

  }

  // 4 Color
  {
    QTableWidgetItem *wi = m_meshList->item(m_prevRow, 4);
    if (!wi)
      {
	wi = new QTableWidgetItem("");
	m_meshList->setItem(m_prevRow, 4, wi);
      }

    wi->setFlags(wi->flags() & ~Qt::ItemIsUserCheckable & ~Qt::ItemIsEditable & ~Qt::ItemIsSelectable);
    wi->setBackground(QBrush(QColor(color)));
    wi->setForeground(QBrush(QColor(color)));
  }

  // 5 Material Id
  {
    QTableWidgetItem *wi = m_meshList->item(m_prevRow, 5);
    if (!wi)
      {
	wi = new QTableWidgetItem("");
	m_meshList->setItem(m_prevRow, 5, wi);
      }

    wi->setFlags(wi->flags() & ~Qt::ItemIsUserCheckable & ~Qt::ItemIsEditable & ~Qt::ItemIsSelectable);
    wi->setTextAlignment(Qt::AlignCenter);
    wi->setText(QString("%1").arg(matId));
  }

  // 6 Blend
  {
    QTableWidgetItem *wi = m_meshList->item(m_prevRow, 6);
    if (!wi)
      {
	wi = new QTableWidgetItem("");
	m_meshList->setItem(m_prevRow, 6, wi);
      }
    wi->setText(QString("%1").arg(matMix));
    wi->setTextAlignment(Qt::AlignCenter);
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
  QList<bool> clearView;
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
      clearView << (words[2].toInt() > 0);
      color << words[3];
      material << words[4].toInt();
      blend << words[5].toFloat();
      
      m_vertCount << words[6].toInt();
      m_triCount << words[7].toInt();
      m_totVertices += words[6].toInt();
      m_totTriangles += words[7].toInt();;
    }

  
  m_meshList->setRowCount(meshNames.count());
  
  m_prevRow = -1;
  for(int i=0; i<m_meshNames.count(); i++)
    {
      m_prevRow++;
      addMesh(m_meshNames[i], show[i], clip[i], clearView[i], color[i], material[i], blend[i]);
    }

  m_meshList->resizeColumnToContents(0);
  m_meshList->resizeRowsToContents();

  //ui.grabMode->setChecked(false);
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
  if (col == 0)
    return;


  //--------------------------------------
  //--------------------------------------
  if (col >= 4) // colormap/material/blend
    {
      QList<int> selIdx = getSelectedMeshes();
      if (selIdx.count() == 0)
	{
	  for(int i=0; i<m_meshList->rowCount(); i++)
	    selIdx << i;
	}

	  

      if (col == 4) // colormap
	{
	  emit processCommand(selIdx, "colormap");
	  return;
	}
  
      //-------
      if (col == 5) // material
	{
	  int matId = m_matcapDialog->getImageId(selIdx[0]);
	  emit materialChanged(selIdx, matId);
	  for(int id=0; id<selIdx.count(); id++)
	    {
	      int i = selIdx[id];
	      m_meshList->item(i, col)->setText(QString("%1").arg(matId));
	    }
	      
	  return;
	}
      //-------

      //-------
      if (col == 6) // blend
	{
	  float matMix = getMatMix(0.5);
	  if (matMix < 0)
	    return;

	  emit materialMixChanged(selIdx, matMix);
	  for(int id=0; id<selIdx.count(); id++)
	    {
	      int i = selIdx[id];
	      QTableWidgetItem *wi;
	      wi = m_meshList->item(i, col);
	      wi->setText(QString("%1").arg(matMix));
	    }

	  return;
	}
      //-------
    }
  //--------------------------------------
  //--------------------------------------

  
  //--------------------------------------
  //--------------------------------------
  // 1/2/3 = visibility/clip/clearView
  QList<bool> CS;
  for(int i=0; i<m_meshList->rowCount(); i++)
    {	  
      QTableWidgetItem *wi;
      wi = m_meshList->item(i, col);
      bool cs = true;
      QVariant value = wi->data(Qt::StatusTipRole);
      if (value.toString() == "-")
	{
	  wi->setData(Qt::StatusTipRole, "");
	  wi->setIcon(m_propIcons[0]);	    
	  cs = false;
	}
      else
	{
	  wi->setData(Qt::StatusTipRole, "-");
	  wi->setIcon(m_propIcons[col]);
	  cs = true;
	}

      CS << cs;
    }
  if (col == 1)
    emit setVisible(CS);
  else if (col == 2)
    emit setClip(CS);
  else
    emit setClearView(CS);
  //--------------------------------------
  //--------------------------------------
}

void
MeshInfoWidget::sectionDoubleClicked(int col)
{
  if (col == 0 || col >= 4)
    return;

  if (m_meshList->rowCount() <= 0)
    return;

  
  // 1/2/3 = visibility/clip/clearView
  bool cs0;
  QTableWidgetItem *wi;
  wi = m_meshList->item(0, col);
  QVariant value = wi->data(Qt::StatusTipRole);
  if (value.toString() == "")
    cs0 = true;
  else
    cs0 = false;

  
  QList<bool> CS;
  for(int i=0; i<m_meshList->rowCount(); i++)
    {	  
      QTableWidgetItem *wi;
      wi = m_meshList->item(i, col);
      CS << cs0;
      if (cs0)
	{
	  wi->setData(Qt::StatusTipRole, "-");
	  wi->setIcon(m_propIcons[col]);
	}
      else
	{
	  wi->setData(Qt::StatusTipRole, "");
	  wi->setIcon(m_propIcons[0]);
	}
    }
  if (col == 1)
    emit setVisible(CS);
  else if (col == 2)
    emit setClip(CS);
  else
    emit setClearView(CS);
}

void
MeshInfoWidget::clearSelection()
{
  m_meshList->clearSelection();
}


void
MeshInfoWidget::cellClicked(int row, int col)
{
  QList<int> selIdx = getSelectedMeshes();

  if (selIdx.count() > 1)
    {
      emit setActive(selIdx[0], true);
      ui.MeshParamBox->setStyleSheet("QWidget{background:beige;}");
    }
  else
    ui.MeshParamBox->setStyleSheet("QWidget{background:#f2f3f4;}");

  
  emit multiSelection(selIdx);

  
  //-----------
  // Visibility/Clip/ClearView
  if (selIdx.count() > 1 &&
      selIdx.contains(row) &&
      (col > 0 && col < 4))
    {      
      QTableWidgetItem *wi;
      wi = m_meshList->item(row, col);
      bool CS = true;
      QVariant value = wi->data(Qt::StatusTipRole);
      if (value.toString() == "-")
	{
	  wi->setData(Qt::StatusTipRole, "");
	  wi->setIcon(m_propIcons[0]);	    
	  CS = false;
	}
      else
	{
	  wi->setData(Qt::StatusTipRole, "-");
	  wi->setIcon(m_propIcons[col]);
	  CS = true;
	}

      
      QList<bool> vflag;
      vflag.reserve(m_meshList->rowCount());
      for(int i=0; i<m_meshList->rowCount(); i++)
	{
	  QTableWidgetItem *wi;
	  vflag.append((m_meshList->item(i, col)->data(Qt::StatusTipRole)).toString() == "-");
	}
      
      for(int id=0; id<selIdx.count(); id++)
	{
	  int i = selIdx[id];
	  
	  QTableWidgetItem *wi;
	  wi = m_meshList->item(i, col);
	  
	  if (CS)
	    {
	      wi->setData(Qt::StatusTipRole, "-");
	      wi->setIcon(m_propIcons[col]);
	    }
	  else
	    {
	      wi->setData(Qt::StatusTipRole, "");
	      wi->setIcon(m_propIcons[0]);
	    }
 	  
	  vflag[i] = CS;
	}

      if (col == 1)
	emit setVisible(vflag);
      else if (col == 2)
	emit setClip(vflag);
      else
	emit setClearView(vflag);

      emit updateGL();
      
      return;
    }
  //-----------

  //-----------
  // Color
  if (selIdx.count() > 1 &&
      selIdx.contains(row) &&
      col == 4)
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

      emit updateGL();

      return;
    }
  //-----------

  //-----------
  // Material Id
  if (selIdx.count() > 1 &&
      selIdx.contains(row) &&
      col == 5)
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
  // Blend
  if (selIdx.count() > 1 &&
      selIdx.contains(row) &&
      col == 6)
    {
      QTableWidgetItem *wi;
      wi = m_meshList->item(row, col);
      float matMix = wi->text().toFloat();
      matMix = getMatMix(matMix);
      if (matMix < 0)
	return;
      
      emit materialMixChanged(matMix);

      for(int id=0; id<selIdx.count(); id++)
	{
	  int i = selIdx[id];
	  wi = m_meshList->item(i, col);
	  wi->setText(QString("%1").arg(matMix));
	}
      emit materialMixChanged(selIdx, matMix);

      return;
    }
  //-----------
    
    
    
  //-----------
  if (selIdx.count() > 1)
      {
	m_prevRow = row;

	emit updateGL();
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
	  emit updateGL();
	  return;
	}
      
      emit updateGL();
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

      emit updateGL();
      return;
    }
  //-----------

  //-----------
  // for single selection
  // Visibility/Clip/ClearView
  if (col > 0 && col < 4)
    {
      m_meshList->setCurrentCell(row, col);
      if (m_prevRow != -1)
	emit setActive(m_prevRow, false);

      m_prevRow = row;
      emit setActive(row, true);
      ui.MeshParamBox->show();

      QTableWidgetItem *wi;
      wi = m_meshList->item(row, col);

      bool flag = true;
      QVariant value = wi->data(Qt::StatusTipRole);
      if (value.toString() == "-")
	{
	  wi->setData(Qt::StatusTipRole, "");
	  wi->setIcon(m_propIcons[0]);	    
	  flag = false;
	}
      else
	{
	  wi->setData(Qt::StatusTipRole, "-");
	  wi->setIcon(m_propIcons[col]);
	  flag = true;
	}

      if (col == 1)
	emit setVisible(row, flag);
      else if (col == 2)
	emit setClip(row, flag);
      else
	emit setClearView(row, flag);
      
      
      emit updateGL();
      return;
    }
  //-----------


  //-----------
  if (col == 4) // color
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

      emit updateGL();
      return;
    }
  //-----------

  //-----------
  if (col == 5) // material
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
      

      emit updateGL();
      return;
    }
  //-----------
  
  //-----------    
  if (col == 6) // material blend
    {      
      m_meshList->setCurrentCell(row, col);
      if (m_prevRow != -1)
	emit setActive(m_prevRow, false);
      
      m_prevRow = row;
      emit setActive(row, true);
      ui.MeshParamBox->show();

      QTableWidgetItem *wi;
      wi = m_meshList->item(row, col);
      float matMix = wi->text().toFloat();
      matMix = getMatMix(matMix);
      if (matMix < 0)
	return;

      emit materialMixChanged(selIdx, matMix);
      for(int id=0; id<selIdx.count(); id++)
	{
	  int i = selIdx[id];
	  QTableWidgetItem *wi;
	  wi = m_meshList->item(i, col);
	  wi->setText(QString("%1").arg(matMix));
	}
      

      emit updateGL();
      return;
    }
  //-----------
  
  emit updateGL();
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

  QFont font;
  font.setPointSize(14);
  mContextMenu->setFont(font);

  QAction *delAction = new QAction("Delete", this);
  QAction *saveAction = new QAction("Save", this);
  QAction *dupAction = new QAction("Duplicate", this);

  connect(delAction, SIGNAL(triggered()), this, SLOT(removeMesh()));
  connect(saveAction, SIGNAL(triggered()), this, SLOT(saveMesh()));
  connect(dupAction, SIGNAL(triggered()), this, SLOT(duplicateMesh()));

  mContextMenu->addAction(delAction);
  mContextMenu->addSeparator();
  mContextMenu->addAction(saveAction);
  mContextMenu->addSeparator();
  mContextMenu->addAction(dupAction);

  mContextMenu->exec(m_meshList->mapToGlobal(pos));
}

void
MeshInfoWidget::removeMesh()
{
  QList<int> selIdx = getSelectedMeshes();

  int row = -1;

  if (selIdx.count() == 1)
    row = m_meshList->currentRow();

  if (row == -1)
    {
      emit removeMesh(selIdx);
    }
  else    
      emit removeMesh(row);
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
      
      if (pkeys[i] == "linemode")
	{	  
	  ui.linemode->setChecked(vlist[0].toBool());
	}
      else if (pkeys[i] == "linewidth")
	{	  
	  ui.linewidth->setValue(vlist[0].toInt());
	}
      else if (pkeys[i] == "position")
	{	  
	  ui.position->setText(vlist[0].toString());
	}
      else if (pkeys[i] == "rotation")
	{	  
	  ui.rotation->setText(vlist[0].toString());
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
  float x = v[0].toFloat();
  if (v.count() > 0) pos = QVector3D(x,x,x);
  if (v.count() > 1) pos.setY(v[1].toFloat());
  if (v.count() > 2) pos.setZ(v[2].toFloat());

  ui.position->setText(QString("%1 %2 %3").
		   arg(pos.x()).
		   arg(pos.y()).
		   arg(pos.z()));

  emit positionChanged(pos);
}

void
MeshInfoWidget::rotationChanged()
{
  QStringList v = ui.rotation->text().split(" ");
  if (v.count() != 4)
    {
      QMessageBox::information(0, "Rotation", "Need to specify a total of 4 components for axis and angle");
      return;
    }
  float x=0,y=0,z=0,a=0;
  if (v.count() > 0) x = v[0].toFloat();
  if (v.count() > 1) y = v[1].toFloat();
  if (v.count() > 2) z = v[2].toFloat();
  if (v.count() > 3) a = v[3].toFloat();

  ui.rotation->setText(QString("%1 %2 %3 %4").
		       arg(x).
		       arg(y).
		       arg(z).
		       arg(a));

  QVector4D rot(x,y,z,a);
  emit rotationChanged(rot);
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
  QList<int> selIdx = getSelectedMeshes();  
  
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
    helpFile.setFileName(":/singlemeshcommand.help"+Global::helpLanguage());
  else if (selIdx.count() > 1)
    helpFile.setFileName(":/multiplemeshcommand.help"+Global::helpLanguage());
  else
    helpFile.setFileName(":/globalmeshcommand.help"+Global::helpLanguage());
  
  if (helpFile.open(QFile::ReadOnly))
    {
      QTextStream in(&helpFile);
      in.setCodec("Utf-8");
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
  
  propertyEditor.set(pc, plist, keys, QStringList());
  
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

float
MeshInfoWidget::getMatMix(float mm)
{
  QFont font;
  font.setPointSize(12);
  QInputDialog* dialog = new QInputDialog(0, Qt::Popup);
  dialog->setFont(font);
  dialog->setWindowTitle("Blend Material");
  dialog->setLabelText("Blend");
  dialog->setDoubleDecimals(1);
  dialog->setDoubleRange(0.0, 1.0);
  dialog->setDoubleValue(0.5);
  dialog->setDoubleStep(0.1);	  
  dialog->setDoubleValue(mm);
  int cdW = dialog->width();
  int cdH = dialog->height();
  dialog->move(QCursor::pos()-QPoint(100,0));
  int ret = dialog->exec();
  if (ret)
    {
      float matMix = dialog->doubleValue();
      return matMix;
    }

  return -1;
}
