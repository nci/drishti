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

  m_selected = 0;
  m_bricks->setSelected(m_selected);
  fillInformation(m_selected);
}

void
BricksWidget::refresh()
{
  m_bricks->setSelected(m_selected);
  fillInformation(m_selected);
}


void
BricksWidget::fillInformation(int bno)
{
  ui.m_translation->setEnabled(true);
  ui.m_pivot->setEnabled(true);
  ui.m_axis->setEnabled(true);
  ui.m_angle->setEnabled(true);

  BrickInformation binfo = m_bricks->brickInformation(bno);

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

  binfo.position = StaticFunctions::getVec(ui.m_translation->text());
  binfo.pivot = StaticFunctions::getVec(ui.m_pivot->text());
  binfo.axis = StaticFunctions::getVec(ui.m_axis->text());
  
  
 // remove the degree symbol at the end of angle value
  QString astr = ui.m_angle->text();
  astr.chop(1);
  if (astr.contains(","))
    {
      astr = astr.replace(',','.');
    }
  binfo.angle = astr.toFloat(&ok);

  Vec voxelScaling = Global::voxelScaling();
  binfo.brickMin = VECPRODUCT(binfo.brickMin, voxelScaling);
  binfo.brickMax = VECPRODUCT(binfo.brickMax, voxelScaling);

  m_bricks->setBrick(m_selected, binfo);

  emit updateGL();
}

void
BricksWidget::on_m_translation_editingFinished()
{
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

  // remove point from the pool
  if (GeometryObjects::hitpoints()->activeCount())
    GeometryObjects::hitpoints()->removeLastActivePoint();
  else
    GeometryObjects::hitpoints()->removeLastPoint();
    
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
    updateBrickInformation();
}
void
BricksWidget::on_m_axis_editingFinished()
{
  Vec axis = StaticFunctions::getVec(ui.m_axis->text());

  if (axis.squaredNorm() < 0.1)
    QMessageBox::information(0, "Error",
			     QString("Invalid Rotation Axis : %1 %2 %3").\
			     arg(axis.x).arg(axis.y).arg(axis.z));
  else
    updateBrickInformation();
}
void
BricksWidget::on_m_angle_editingFinished()
{
  updateBrickInformation();
}

void
BricksWidget::setBrickZeroRotation(int axisType, float angle)
{
  m_selected = 0;
  m_bricks->setSelected(m_selected);
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
  QFile helpFile(":/brick.help"+Global::helpLanguage());
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

  QStringList keys;
  keys << "commandhelp";
  
  propertyEditor.set("Bricks Editor Help", plist, keys);
  propertyEditor.exec();
}
