#include "brickswidget.h"
#include "global.h"
#include "geometryobjects.h"
#include "staticfunctions.h"
#include "commonqtclasses.h"
#include "propertyeditor.h"

BricksWidget::BricksWidget(QWidget *parent,
			   Bricks *bricks) :
  QWidget(parent)
{
  m_bricks = bricks;

  ui.setupUi(this);

  QRegExp rx("(\\-?\\d*\\.?\\d*\\s?){3,3}");
  QRegExpValidator *validator = new QRegExpValidator(rx,0);

  ui.m_translation->setValidator(validator);
  ui.m_pivot->setValidator(validator);
  ui.m_axis->setValidator(validator);
  ui.m_scalepivot->setValidator(validator);
  ui.m_scale->setValidator(validator);
  ui.m_brickMinEdit->setValidator(validator);
  ui.m_brickMaxEdit->setValidator(validator);

  ui.m_brickList->addItem("Brick 0");

  m_selected = -1;
  ui.m_brickList->setCurrentRow(m_selected);
  fillInformation(m_selected);
  m_bricks->setSelected(m_selected);

  ui.m_linkBrick->clear();
  ui.m_linkBrick->addItem("Brick 0");


  connect(ui.m_clipTable, SIGNAL(cellClicked(int, int)),
	  this, SLOT(cellClicked(int, int)));

}

void
BricksWidget::setTFSets(int ntfSets)
{
  ui.m_tfSet->clear();

  QStringList items;
  for(int i=0; i<ntfSets; i++)
      items << QString("Set %1").arg(i);
  items << "None";

  ui.m_tfSet->addItems(items);
}

void
BricksWidget::cellClicked(int row, int col)
{
  QTableWidgetItem *wi = ui.m_clipTable->item(row, col);
  BrickInformation binfo = m_bricks->brickInformation(m_selected);

  if (wi->checkState() == Qt::Checked)
    binfo.clippers[col] = true;
  else
    binfo.clippers[col] = false;

  m_bricks->setBrick(m_selected, binfo);
  emit updateGL();
}

void
BricksWidget::addClipper()
{
  m_bricks->addClipper();
  updateClipTable(m_selected);
}

void
BricksWidget::removeClipper(int ci)
{
  m_bricks->removeClipper(ci);
  updateClipTable(m_selected);
}

void
BricksWidget::updateClipTable(int bno)
{
  // clear all information from the table
  while(ui.m_clipTable->rowCount() > 0)
      ui.m_clipTable->removeRow(0);

  if (bno < 0)
    return;
  
  BrickInformation binfo = m_bricks->brickInformation(bno);
  QList<bool> clippers = binfo.clippers;

  if (clippers.size() <= 0)
    return;

  ui.m_clipTable->setColumnCount(clippers.size());
  ui.m_clipTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
  ui.m_clipTable->verticalHeader()->hide();

  QStringList item;
  for (int i=0; i<clippers.size(); i++)
    item << QString("%1").arg(i);
  ui.m_clipTable->setHorizontalHeaderLabels(item);

  for (int i=0; i<clippers.size(); i++)
    ui.m_clipTable->setColumnWidth(i, 25);

  ui.m_clipTable->insertRow(0);
  for(int ci=0;ci<clippers.size();ci++)
    {
      QTableWidgetItem *wi = new QTableWidgetItem("");
      if (clippers[ci])
	wi->setCheckState(Qt::Checked);
      else
	wi->setCheckState(Qt::Unchecked);

      wi->setFlags(wi->flags() & ~Qt::ItemIsEditable);
      ui.m_clipTable->setItem(0, ci, wi);
    }

  update();
}

void
BricksWidget::refresh()
{
  int nitems = m_bricks->bricks().size();

  ui.m_brickList->clear();
  for(int i=0; i<nitems; i++)
    ui.m_brickList->addItem(QString("Brick %1").arg(i));

  ui.m_linkBrick->clear();
  for(int i=0; i<nitems; i++)
    ui.m_linkBrick->addItem(QString("Brick %1").arg(i));

  m_selected = qBound(0, m_selected, nitems-1);

  m_bricks->setSelected(m_selected);
  ui.m_brickList->setCurrentRow(m_selected);
  fillInformation(m_selected);
}

void
BricksWidget::on_m_linkBrick_activated(int index)
{
  if (m_selected < 0 || m_selected >= ui.m_brickList->count())
    return;
  
  if (ui.m_linkBrick->currentIndex() != m_selected)
    {
      QList<int> links;
      for(int i=0; i<ui.m_brickList->count(); i++)
	{
	  BrickInformation binfo = m_bricks->brickInformation(i);
	  links.append(binfo.linkBrick);
	}
      links[m_selected] = index;

      // check for cycles
      bool done = false;
      int linkBrick = m_selected;
      int plinkBrick;
      while(!done)
	{
	  if (links[linkBrick] == linkBrick ||
	      links[linkBrick] == 0)
	    {
	      done = true;
	    }
	  else
	    {
	      plinkBrick = linkBrick;
	      linkBrick = links[linkBrick];

	      if (m_selected == linkBrick)
		{
		  emit showMessage(QString("Link to brick %1 would create cycle").\
				   arg(ui.m_linkBrick->currentIndex()) +
				   QString(" - not allowed\nReverting to brick 0"),
				   true);
		  ui.m_linkBrick->setCurrentIndex(0);
		  done = true;
		}
	    }
	}
    }

  updateBrickInformation();
}

void
BricksWidget::on_m_tfSet_activated(int index)
{
  if (m_selected < 0 || m_selected >= ui.m_brickList->count())
    return;
  updateBrickInformation();
}

void
BricksWidget::on_m_new_pressed()
{
  int nitems = ui.m_brickList->count();
  ui.m_brickList->clear();
  ui.m_linkBrick->clear();
  for(int i=0; i<nitems+1; i++)
    {
      ui.m_brickList->addItem(QString("Brick %1").arg(i));
      ui.m_linkBrick->addItem(QString("Brick %1").arg(i));
    }

  m_bricks->addBrick();

  m_selected = nitems;
  m_bricks->setSelected(m_selected);
  ui.m_brickList->setCurrentRow(m_selected);
  fillInformation(m_selected);

  refresh();

  emit showMessage(QString("Added a new brick %1. Hover and move brick faces to change size. ").arg(m_selected),
		   false);
  emit updateGL();
  qApp->processEvents();
}

void
BricksWidget::on_m_remove_pressed()
{
  if (m_selected < 0)
    {
      emit showMessage("Select brick for removal", true);
      return;
    }
    
  if (m_selected == 0)
    {
      emit showMessage("Cannot remove toplevel brick 0", true);
      return;
    }

  m_bricks->removeBrick(m_selected);

  int nitems = ui.m_brickList->count();
  ui.m_brickList->clear();
  ui.m_linkBrick->clear();
  for(int i=0; i<nitems-1; i++)
    {
      ui.m_brickList->addItem(QString("Brick %1").arg(i));
      ui.m_linkBrick->addItem(QString("Brick %1").arg(i));
    }

  int deletedBrick = m_selected;

  m_selected = 0;
  m_bricks->setSelected(m_selected);
  ui.m_brickList->setCurrentRow(m_selected);
  fillInformation(m_selected);

  refresh();

  emit showMessage(QString("Removed brick %1").arg(deletedBrick), false);
  emit updateGL();
  qApp->processEvents();
}

void
BricksWidget::on_m_selectAll_pressed()
{
  if (m_selected < 0 || m_selected >= ui.m_brickList->count())
    return;

  BrickInformation binfo = m_bricks->brickInformation(m_selected);

  for(int ci=0;ci<binfo.clippers.size();ci++)
    {
      QTableWidgetItem *wi;
      wi = ui.m_clipTable->item(0, ci);
      wi->setCheckState(Qt::Checked);

      binfo.clippers[ci] = true;
    }
  m_bricks->setBrick(m_selected, binfo);
  emit updateGL();
}
void
BricksWidget::on_m_deselectAll_pressed()
{
  if (m_selected < 0 || m_selected >= ui.m_brickList->count())
    return;

  BrickInformation binfo = m_bricks->brickInformation(m_selected);

  for(int ci=0;ci<binfo.clippers.size();ci++)
    {
      QTableWidgetItem *wi;
      wi = ui.m_clipTable->item(0, ci);
      wi->setCheckState(Qt::Unchecked);

      binfo.clippers[ci] = false;
    }
  m_bricks->setBrick(m_selected, binfo);
  emit updateGL();
}

void
BricksWidget::fillInformation(int bno)
{
  if (bno < 0)
    {
      ui.m_linkBrick->setDisabled(true);
      ui.m_tfSet->setDisabled(true);
      ui.m_translation->setDisabled(true);
      ui.m_pivot->setDisabled(true);
      ui.m_axis->setDisabled(true);
      ui.m_angle->setDisabled(true);
      ui.m_clipTable->setDisabled(true);
      ui.m_scalepivot->setDisabled(true);
      ui.m_scale->setDisabled(true);
      ui.m_brickMinEdit->setDisabled(true);
      ui.m_brickMaxEdit->setDisabled(true);

      return;
    }
  else
    {
      if (bno == 0)
	{
	  ui.m_linkBrick->setDisabled(true);
	  ui.m_brickMinEdit->setEnabled(false);
	  ui.m_brickMaxEdit->setEnabled(false);
	}
      else
	{
	  ui.m_linkBrick->setEnabled(true);
	  ui.m_brickMinEdit->setEnabled(true);
	  ui.m_brickMaxEdit->setEnabled(true);
	}

      ui.m_tfSet->setEnabled(true);
      ui.m_translation->setEnabled(true);
      ui.m_pivot->setEnabled(true);
      ui.m_axis->setEnabled(true);
      ui.m_angle->setEnabled(true);
      ui.m_clipTable->setEnabled(true);
      ui.m_scalepivot->setEnabled(true);
      ui.m_scale->setEnabled(true);
    }

  BrickInformation binfo = m_bricks->brickInformation(bno);

  Vec dataMin, dataMax, dataSize;
  Global::bounds(dataMin, dataMax);
  dataSize = dataMax - dataMin;
  Vec bmin, bmax, bsz;
  bmin = dataMin + VECPRODUCT(binfo.brickMin,dataSize);
  bmax = dataMin + VECPRODUCT(binfo.brickMax,dataSize);
  bsz = bmax-bmin+Vec(1,1,1);

  ui.m_brickMinEdit->setText(QString("%1 %2 %3").\
                              arg((int)bmin.x).   \
                              arg((int)bmin.y).   \
                              arg((int)bmin.z));
  ui.m_brickMaxEdit->setText(QString("%1 %2 %3").\
                              arg((int)bmax.x).   \
                              arg((int)bmax.y).   \
                              arg((int)bmax.z));
  ui.m_brickSizeEdit->setText(QString("%1 %2 %3").\
                               arg((int)bsz.x).    \
                               arg((int)bsz.y).    \
                               arg((int)bsz.z));

  ui.m_tfSet->setCurrentIndex(binfo.tfSet);
  
  ui.m_linkBrick->setCurrentIndex(binfo.linkBrick);

  ui.m_translation->setText(QString("%1 %2 %3").	\
			    arg(binfo.position.x).	\
			    arg(binfo.position.y).	\
			    arg(binfo.position.z));

  ui.m_pivot->setText(QString("%1 %2 %3").		\
		      arg(binfo.pivot.x).		\
		      arg(binfo.pivot.y).		\
		      arg(binfo.pivot.z));
  
  ui.m_axis->setText(QString("%1 %2 %3").		\
		     arg(binfo.axis.x).			\
		     arg(binfo.axis.y).			\
		     arg(binfo.axis.z));

  ui.m_angle->setValue(binfo.angle);
  ui.m_scalepivot->setText(QString("%1 %2 %3"). \
			   arg(binfo.scalepivot.x). \
			   arg(binfo.scalepivot.y). \
			   arg(binfo.scalepivot.z));
  ui.m_scale->setText(QString("%1 %2 %3"). \
		      arg(binfo.scale.x).		\
		      arg(binfo.scale.y).		\
		      arg(binfo.scale.z));


  

  updateClipTable(bno);
}

void
BricksWidget::on_m_brickList_itemClicked(QListWidgetItem *item)
{
  m_selected = ui.m_brickList->row(item);
  m_bricks->setSelected(m_selected);
  fillInformation(m_selected);
  emit updateGL();
}

void
BricksWidget::on_m_showAxis_stateChanged(int state)
{
  if (state == Qt::Checked)
    m_bricks->setShowAxis(true);
  else
    m_bricks->setShowAxis(false);
  emit updateGL();
}

void
BricksWidget::updateBrickInformation()
{
  bool ok;
  BrickInformation binfo = m_bricks->brickInformation(m_selected);

  if (m_selected == 0)
    binfo.linkBrick = 0;
  else
    binfo.linkBrick = ui.m_linkBrick->currentIndex();

  binfo.tfSet = ui.m_tfSet->currentIndex();
  binfo.position = StaticFunctions::getVec(ui.m_translation->text());
  binfo.pivot = StaticFunctions::getVec(ui.m_pivot->text());
  binfo.axis = StaticFunctions::getVec(ui.m_axis->text());

 // remove the degree symbol at the end of angle value
  QString astr = ui.m_angle->text();
  astr.chop(1);
  binfo.angle = astr.toFloat(&ok);
  binfo.scalepivot = StaticFunctions::getVec(ui.m_scalepivot->text());
  binfo.scale = StaticFunctions::getVec(ui.m_scale->text());

  m_bricks->setBrick(m_selected, binfo);

  emit updateGL();
}

void
BricksWidget::on_m_brickMinEdit_editingFinished()
{
  if (m_selected < 0 || m_selected >= ui.m_brickList->count())
    return;
  Vec dataMin, dataMax;
  Global::bounds(dataMin, dataMax);
  Vec dataSize = dataMax - dataMin;
  Vec max = StaticFunctions::getVec(ui.m_brickMaxEdit->text());
  Vec min = StaticFunctions::getVec(ui.m_brickMinEdit->text());
  if (min.x<dataMin.x) min.x=dataMin.x; else if (min.x>dataMax.x) min.x=dataMax.x;
  if (min.y<dataMin.y) min.y=dataMin.y; else if (min.y>dataMax.y) min.y=dataMax.y;
  if (min.z<dataMin.z) min.z=dataMin.z; else if (min.z>dataMax.z) min.z=dataMax.x;
  max = Vec( qMax(min.x+1,max.x), qMax(min.y+1,max.y), qMax(min.z+1,max.z));
  BrickInformation binfo = m_bricks->brickInformation(m_selected);
  min = min-dataMin;
  max = max-dataMin;
  binfo.brickMin = VECDIVIDE(min, dataSize);
  binfo.brickMax = VECDIVIDE(max, dataSize);
  m_bricks->setBrick(m_selected, binfo);
  updateBrickInformation();
}

void BricksWidget::on_m_brickMaxEdit_editingFinished()
{
  if (m_selected < 0 || m_selected >= ui.m_brickList->count())
    return;
  Vec dataMin, dataMax;
  Global::bounds(dataMin, dataMax);
  Vec dataSize = dataMax - dataMin;
  Vec max = StaticFunctions::getVec(ui.m_brickMaxEdit->text());
  Vec min = StaticFunctions::getVec(ui.m_brickMinEdit->text());
  if (max.x<dataMin.x) max.x=dataMin.x; else if (max.x>dataMax.x) max.x=dataMax.x;
  if (max.y<dataMin.y) max.y=dataMin.y; else if (max.y>dataMax.y) max.y=dataMax.y;
  if (max.z<dataMin.z) max.z=dataMin.z; else if (max.z>dataMax.z) max.z=dataMax.x;
  min =  Vec(qMin(min.x,max.x-1), qMin(min.y,max.y-1), qMin(min.z,max.z-1));
  BrickInformation binfo = m_bricks->brickInformation(m_selected);
  min = min-dataMin;
  max = max-dataMin;
  binfo.brickMin = VECDIVIDE(min, dataSize);
  binfo.brickMax = VECDIVIDE(max, dataSize);
  m_bricks->setBrick(m_selected, binfo);
  updateBrickInformation();
}

void
BricksWidget::on_m_translation_editingFinished()
{
  if (m_selected < 0 || m_selected >= ui.m_brickList->count())
    return;
  updateBrickInformation();
}

bool
BricksWidget::getHitpoint(Vec& hitpt)
{
  QList<Vec> pts;
  if (GeometryObjects::hitpoints()->activeCount())
    pts = GeometryObjects::hitpoints()->activePoints();
  else
    pts = GeometryObjects::hitpoints()->points();
  
  if (pts.count() == 0)
    {
      emit showMessage("No active hitpoint found. Use shift key and left mouse button to add a point on the volume. Click a point to make it active", true);
      return false;
    }

  Vec dataMin, dataMax, dataSize;
  Global::bounds(dataMin, dataMax);
  dataSize = dataMax-dataMin;
  BrickInformation binfo = m_bricks->brickInformation(m_selected);
  Vec bmin = dataMin + VECPRODUCT(binfo.brickMin, dataSize);
  Vec bmax = dataMin + VECPRODUCT(binfo.brickMax, dataSize);

  // take the last active hitpoint
  Vec p = pts[pts.count()-1];
  p -= bmin;
  Vec d = bmax-bmin;
  hitpt = VECDIVIDE(p, d);
  return true;
}

void
BricksWidget::on_m_pivotFromHitpoint_pressed()
{
  Vec p;
  if (getHitpoint(p) == false)
    return;

  ui.m_pivot->setText(QString("%1 %2 %3"). \
		      arg(p.x).arg(p.y).arg(p.z));

  update();
  updateBrickInformation();
}

void
BricksWidget::on_m_axisFromHitpoint_pressed()
{
  Vec p1;
  if (getHitpoint(p1) == false)
    return;

  Vec p0 = StaticFunctions::getVec(ui.m_pivot->text());

  //----- get the original coordinates back to find the rotation axis
  Vec dataMin, dataMax, dataSize;
  Global::bounds(dataMin, dataMax);
  dataSize = dataMax-dataMin;
  BrickInformation binfo = m_bricks->brickInformation(m_selected);
  Vec bmin = dataMin + VECPRODUCT(binfo.brickMin, dataSize);
  Vec bmax = dataMin + VECPRODUCT(binfo.brickMax, dataSize);
  //-----
  Vec d = bmax-bmin;
  p0 = bmin + VECPRODUCT(p0,d);
  p1 = bmin + VECPRODUCT(p1,d);
  //-----

  Vec axis = (p1-p0).unit();

  ui.m_axis->setText(QString("%1 %2 %3"). \
		     arg(axis.x).arg(axis.y).arg(axis.z));

  update();
  updateBrickInformation();
}


void
BricksWidget::on_m_pivot_editingFinished()
{
  if (m_selected < 0 || m_selected >= ui.m_brickList->count())
    return;     
  updateBrickInformation();
}
void
BricksWidget::on_m_axis_editingFinished()
{
  if (m_selected < 0 || m_selected >= ui.m_brickList->count())
    return;     
  updateBrickInformation();
}
void
BricksWidget::on_m_angle_editingFinished()
{
  if (m_selected < 0 || m_selected >= ui.m_brickList->count())
    return;     
  updateBrickInformation();
}

void
BricksWidget::on_m_scalepivotFromHitpoint_pressed()
{
  Vec p;
  if (getHitpoint(p) == false)
    return;

  ui.m_scalepivot->setText(QString("%1 %2 %3"). \
			   arg(p.x).arg(p.y).arg(p.z));

  update();
  updateBrickInformation();
}
void
BricksWidget::on_m_scalepivot_editingFinished()
{
  if (m_selected < 0 || m_selected >= ui.m_brickList->count())
    return;     
  updateBrickInformation();
}
void
BricksWidget::on_m_scale_editingFinished()
{
  if (m_selected < 0 || m_selected >= ui.m_brickList->count())
    return;     
  updateBrickInformation();
}

void
BricksWidget::setBrickZeroRotation(int axisType, float angle)
{
  m_selected = 0;
  m_bricks->setSelected(m_selected);
  ui.m_brickList->setCurrentRow(m_selected);
  fillInformation(m_selected);

  Vec axis;
  axis = Vec(axisType==0,
	     axisType==1,
	     axisType==2);
  ui.m_axis->setText(QString("%1 %2 %3").		\
		     arg(axis.x).			\
		     arg(axis.y).			\
		     arg(axis.z));
  ui.m_angle->setValue(angle);

  BrickInformation binfo = m_bricks->brickInformation(m_selected);
  binfo.axis = axis;
  binfo.angle = angle;
  m_bricks->setBrick(m_selected, binfo);
  emit updateGL();
}

void
BricksWidget::keyPressEvent(QKeyEvent *event)
{
  if (event->key() == Qt::Key_H &&
      (event->modifiers() & Qt::ControlModifier ||
       event->modifiers() & Qt::MetaModifier) )
    showHelp();
}

void
BricksWidget::showHelp()
{
  PropertyEditor propertyEditor;
  QMap<QString, QVariantList> plist;
  QVariantList vlist;

  vlist.clear();
  QFile helpFile(":/brick.help");
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
  
  propertyEditor.set("Bricks Editor Help", plist, keys);
  propertyEditor.exec();
}
