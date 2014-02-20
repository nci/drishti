#include "global.h"
#include "staticfunctions.h"
#include "hitpoints.h"
#include "rawvolume.h"

#include <fstream>
using namespace std;

#include <QLineEdit>
#include <QInputDialog>
#include <QTextStream>

int HitPoints::bareCount() { return m_barePoints.count(); }
QList<Vec> HitPoints::barePoints() { return m_barePoints; }
void HitPoints::setBarePoints(QList<Vec> pts) { m_barePoints = pts; }
void HitPoints::removeBarePoints() { m_barePoints.clear(); }

void HitPoints::setPointSize(int sz) { m_pointSize = sz; }
int HitPoints::pointSize() { return m_pointSize; }

void HitPoints::setPointColor(Vec col) { m_pointColor = col; }
Vec HitPoints::pointColor() { return m_pointColor; }

int HitPoints::count() { return m_points.count(); }

void HitPoints::setMouseGrab(bool b)
{
  m_grab = b;
  if (!m_grab)
    removeFromMouseGrabberPool();
  else
    addInMouseGrabberPool();
}
void HitPoints::toggleMouseGrab()
{
  m_grab = !m_grab;
  if (!m_grab)
    removeFromMouseGrabberPool();
  else
    addInMouseGrabberPool();
}

int HitPoints::activeCount()
{
  return HitPointGrabber::activePool().count();
}

QList<Vec>
HitPoints::renewValues()
{
  QList<Vec> pts;
  pts.clear();

  if ((m_showRawValues || m_showTagValues) &&
      m_points.count() > m_rawValues.count())
    pts = points();

  return pts;
}

void
HitPoints::setRawTagValues(QList<QVariant> raw, QList<QVariant> tag)
{
  m_rawValues.clear();
  m_tagValues.clear();

  m_rawValues = raw;
  m_tagValues = tag;
}

bool
HitPoints::point(int i, Vec &pt)
{
  if (i < 0)
    {
      int idx = m_points.count() + i;
      if (idx >= 0)
	{
	  pt = m_points[idx]->point();
	  return true;
	}
    }
  else
    {
      if (i < m_points.count())
	{
	  pt = m_points[i]->point();
	  return true;
	}
    }

  return false;
}

bool
HitPoints::activePoint(int i, Vec &pt)
{
  QList<Vec> apoints = activePoints();

  if (i < 0)
    {
      int idx = apoints.count() + i;
      if (idx >= 0)
	{
	  pt = apoints[idx];
	  return true;
	}
    }
  else
    {
      if (i < apoints.count())
	{	  
	  pt = apoints[i];
	  return true;
	}
    }

  return false;
}

HitPoints::HitPoints()
{
  m_ignore = false;
  m_grab = true;
  m_pointColor = Vec(0.0f, 0.5f, 1.0f);
  m_pointSize = 20;
  m_showPoints = false;
  m_showRawValues = false;
  m_showTagValues = false;
  m_showCoordinates = false;
  m_points.clear();
  m_rawValues.clear();
  m_tagValues.clear();
  m_barePoints.clear();
}

HitPoints::~HitPoints()
{
  clear();  
  m_barePoints.clear();
}

void
HitPoints::addInMouseGrabberPool()
{
  for(int i=0; i<m_points.count(); i++)
    m_points[i]->addInMouseGrabberPool();
}
void
HitPoints::addInMouseGrabberPool(int i)
{
  if (i < m_points.count())
    m_points[i]->addInMouseGrabberPool();
}
void
HitPoints::removeFromMouseGrabberPool()
{
  for(int i=0; i<m_points.count(); i++)
    m_points[i]->removeFromMouseGrabberPool();
}
void
HitPoints::removeFromMouseGrabberPool(int i)
{
  if (i < m_points.count())
    m_points[i]->removeFromMouseGrabberPool();
}

void
HitPoints::removeActive()
{
  QList<Vec> pts;

  for(int i=0; i<m_points.count(); i++)
    {      
      if (m_points[i]->isInActivePool() == false)
	pts.append(m_points[i]->point());
    }

  setPoints(pts);
}

void
HitPoints::clear()
{
  for(int i=0; i<m_points.count(); i++)
    m_points[i]->disconnect();

  HitPointGrabber::clearActivePool();

  for(int i=0; i<m_points.count(); i++)
    m_points[i]->removeFromMouseGrabberPool();

  for(int i=0; i<m_points.count(); i++)
    delete m_points[i];

  m_points.clear();

  m_rawValues.clear();
  m_tagValues.clear();
}

void HitPoints::resetActive()
{
  for(int i=0; i<m_points.count(); i++)
    m_points[i]->resetActive();

  HitPointGrabber::clearActivePool();
}

QList<Vec>
HitPoints::points()
{
  QList<Vec> pts;
  for(int i=0; i<m_points.count(); i++)
    pts.append(m_points[i]->point());

  return pts;
}

QList<Vec>
HitPoints::activePoints()
{
  QList<HitPointGrabber*> activePool = HitPointGrabber::activePool();
  QList<Vec> pts;
  for(int i=0; i<activePool.count(); i++)
    pts.append(activePool[i]->point());
  return pts;
}

bool
HitPoints::grabsMouse()
{
  for(int i=0; i<m_points.count(); i++)
    {
      if (m_points[i]->grabsMouse())
	return true;
    }
  return false;
}

void
HitPoints::savePoints(QString flnm)
{
  int npts = m_points.count();
  fstream fp(flnm.toLatin1().data(), ios::out);
  fp << npts << "\n";
  for(int i=0; i<npts; i++)
    {
      Vec pt = m_points[i]->point();
      fp << pt.x << " " << pt.y << " " << pt.z << "\n";
    }
  fp.close();

  QMessageBox::information(0, "Save points", "Saved points to "+flnm);
}


void
HitPoints::addBarePoints(QString flnm)
{
  QList<Vec> pts;

  QFile fpoints(flnm);
  fpoints.open(QFile::ReadOnly);
  QTextStream fd(&fpoints);
  while (! fd.atEnd())
    {
      QString line = fd.readLine();
      QStringList list = line.split(" ", QString::SkipEmptyParts);
      if (list.count() == 1)
	{
	  int npts = list[0].toInt();
	  for(int i=0; i<npts; i++)
	    {
	      if (fd.atEnd())
		break;
	      else
		{
		  QString line = fd.readLine();
		  QStringList list = line.split(" ", QString::SkipEmptyParts); 
		  if (list.count() == 3)
		    {
		      float x = list[0].toFloat();
		      float y = list[1].toFloat();
		      float z = list[2].toFloat();
		      pts.append(Vec(x,y,z));
		    }
		}
	    }
	}
    }

  if (pts.count() > 0)
    setBarePoints(pts);

  QMessageBox::information(0, "", QString("No. of bare points in the scene : %1").\
			   arg(m_barePoints.count()));
}


void
HitPoints::addPoints(QString flnm)
{
  QList<Vec> pts;

  QFile fpoints(flnm);
  fpoints.open(QFile::ReadOnly);
  QTextStream fd(&fpoints);
  while (! fd.atEnd())
    {
      QString line = fd.readLine();
      QStringList list = line.split(" ", QString::SkipEmptyParts);
      if (list.count() == 1)
	{
	  int npts = list[0].toInt();
	  for(int i=0; i<npts; i++)
	    {
	      if (fd.atEnd())
		break;
	      else
		{
		  QString line = fd.readLine();
		  QStringList list = line.split(" ", QString::SkipEmptyParts);
		  if (list.count() == 3)
		    {
		      float x = list[0].toFloat();
		      float y = list[1].toFloat();
		      float z = list[2].toFloat();
		      pts.append(Vec(x,y,z));
		    }
		}
	    }
	}
    }

  if (pts.count() > 0)
    {
      for(int i=0; i<pts.count(); i++)
	{
	  HitPointGrabber *hpg = new HitPointGrabber(pts[i]);
	  m_points.append(hpg);
	}

      m_rawValues.clear();
      m_tagValues.clear();
      
      updatePointDialog();
    }

  QMessageBox::information(0, "", QString("No. of points in the scene : %1").\
			   arg(m_points.count()));

  removeFromMouseGrabberPool();
  if (m_grab)
    addInMouseGrabberPool();
}

HitPointGrabber *dummyHpg = 0;
void HitPoints::ignore(bool i) { m_ignore = i; }

void
HitPoints::add(Vec opt)
{
  Vec voxelScaling = Global::voxelScaling();
  Vec pt = VECDIVIDE(opt, voxelScaling);

//  if (m_ignore) // while carving we do not want to add these points
//    {
//      if (dummyHpg)
//	dummyHpg->setPoint(pt);
//      else
//	{
//	  HitPointGrabber *hpg = new HitPointGrabber(pt);
//	  dummyHpg = hpg;
//	}
//      return;
//    }

  HitPointGrabber *hpg = new HitPointGrabber(pt);
  m_points.append(hpg);

  m_rawValues.clear();
  m_tagValues.clear();

  updatePointDialog();

  removeFromMouseGrabberPool();
  if (m_grab)
    addInMouseGrabberPool();
}

void
HitPoints::setPoints(QList<Vec> pts)
{
  clear();

  for(int i=0; i<pts.count(); i++)
    {
      HitPointGrabber *hpg = new HitPointGrabber(pts[i]);
      m_points.append(hpg);
    }

  m_rawValues.clear();
  m_tagValues.clear();

  updatePointDialog();

  removeFromMouseGrabberPool();
  if (m_grab)
    addInMouseGrabberPool();
}

void
HitPoints::drawArrows(Vec pt, int direc)
{
  Vec voxelScaling = Global::voxelScaling();

  Vec v;
  int l = 10;

  glLineWidth(2.0);
  glBegin(GL_LINES);

  if (direc == HitPointGrabber::MoveX ||
      direc == HitPointGrabber::MoveAll)
    {
      glColor3f(1,0,0);
      v = pt - Vec(l*voxelScaling.x, 0, 0);
      glVertex3fv(v);
      v = pt + Vec(l*voxelScaling.x, 0, 0);
      glVertex3fv(v);      
    }
  if (direc == HitPointGrabber::MoveY ||
      direc == HitPointGrabber::MoveAll)
    {
      glColor3f(0,1,0);
      v = pt - Vec(0, l*voxelScaling.y, 0);
      glVertex3fv(v);
      v = pt + Vec(0, l*voxelScaling.y, 0);
      glVertex3fv(v);      
    }
  if (direc == HitPointGrabber::MoveZ ||
      direc == HitPointGrabber::MoveAll)
    {
      glColor3f(0,0,1);
      v = pt - Vec(0, 0, l*voxelScaling.z);
      glVertex3fv(v);
      v = pt + Vec(0, 0, l*voxelScaling.z);
      glVertex3fv(v);      
    }
  glEnd();
}

void
HitPoints::draw(QGLViewer *viewer, bool backToFront)
{
  glEnable(GL_POINT_SPRITE);
  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, Global::spriteTexture());
  glTexEnvf( GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE );
  glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
  glEnable(GL_POINT_SMOOTH);


  Vec voxelScaling = Global::voxelScaling();

  //--------------------
  // draw bare points
  glColor3fv(m_pointColor);
  glPointSize(qMax(1, m_pointSize-5));
  glBegin(GL_POINTS);
  for(int i=0; i<m_barePoints.count();i++)
    {
      Vec pt = VECPRODUCT(m_barePoints[i], voxelScaling);
      glVertex3fv(pt);
    }
  glEnd();
  //--------------------


  //--------------------
  // draw grabbed point
  glColor3f(0, 1, 0.5f);
  glPointSize(m_pointSize+10);
  glBegin(GL_POINTS);
  for(int i=0; i<m_points.count();i++)
    {
      if (m_points[i]->grabsMouse())
	{
	  Vec pt = m_points[i]->point();
	  pt = VECPRODUCT(pt, voxelScaling);
	  glVertex3fv(pt);
	}
    }
  glEnd();
  //--------------------

  //--------------------
  // draw active points
  glColor3f(1, 0, 0.5);
  glPointSize(m_pointSize+5);
  glBegin(GL_POINTS);
  for(int i=0; i<m_points.count();i++)
    {
      if (m_points[i]->active())
	{
	  Vec pt = m_points[i]->point();
	  pt = VECPRODUCT(pt, voxelScaling);
	  
	  glVertex3fv(pt);
	}
    }
  glEnd();
  //--------------------

  //--------------------
  // draw rest of the points
  glColor3fv(m_pointColor);
  glPointSize(m_pointSize);
  glBegin(GL_POINTS);
  for(int i=0; i<m_points.count();i++)
    {
      if (! m_points[i]->grabsMouse() &&
	  ! m_points[i]->active())
	{
	  Vec pt = m_points[i]->point();
	  pt = VECPRODUCT(pt, voxelScaling);

	  glVertex3fv(pt);
	}
    }
  glEnd();

//  Vec pc = m_pointColor*0.5;
//  glColor3fv(pc);
//  glPointSize(qMax(1, m_pointSize-5));
//  glBegin(GL_POINTS);
//  for(int i=0; i<m_points.count();i++)
//    {
//      if (! m_points[i]->grabsMouse() &&
//	  ! m_points[i]->active())
//	{
//	  Vec pt = m_points[i]->point();
//	  pt = VECPRODUCT(pt, voxelScaling);
//
//	  glVertex3fv(pt);
//	}
//    }
//  glEnd();
  //--------------------


  glPointSize(1);

  glDisable(GL_POINT_SPRITE);
  glActiveTexture(GL_TEXTURE0);
  glDisable(GL_TEXTURE_2D);
  
  glDisable(GL_POINT_SMOOTH);
}

void
HitPoints::postdraw(QGLViewer *viewer)
{
  if (m_points.count() == 0)
    return;

  Vec voxelScaling = Global::voxelScaling();

  viewer->startScreenCoordinatesSystem();

  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // blend on top

  for(int i=0; i<m_points.count(); i++)
    {
      if (m_showPoints ||
	  m_showRawValues ||
	  m_showTagValues ||
	  m_showCoordinates)
	{
	  Vec pt = m_points[i]->point();

	  Vec spt = VECPRODUCT(pt, voxelScaling);
	  Vec scr = viewer->camera()->projectedCoordinatesOf(spt);
	  int x = scr.x;
	  int y = scr.y;
	  
	  QString str;

	  if (i < m_points.count() &&
	      m_showPoints)
	    str = QString("%1").arg(i);

	  if (m_showRawValues)
	    {
	      QVariant qv = m_rawValues[i];

	      if (qv.type() == QVariant::UInt)
		str += QString(" r(%1)").arg(qv.toUInt());
	      else if (qv.type() == QVariant::Int)
		str += QString(" r(%1)").arg(qv.toInt());
	      else if (qv.type() == QVariant::Double)
		str += QString(" r(%1)").arg(qv.toDouble(), 0, 'f', Global::floatPrecision());
	      else if (qv.type() == QVariant::String)
		str += QString(" r(%1)").arg(qv.toString());
	    }
	  if (m_showTagValues)
	    {
	      QVariant qv = m_tagValues[i];

	      if (qv.type() == QVariant::UInt)
		str += QString(" t(%1)").arg(qv.toUInt());
	      else if (qv.type() == QVariant::Int)
		str += QString(" t(%1)").arg(qv.toInt());
	      else if (qv.type() == QVariant::String)
		str += QString(" t(%1)").arg(qv.toString());
	    }
	  if (m_showCoordinates)
	    {
	      str += QString(" c(%1 %2 %3)").\
		arg(pt.x, 0, 'f', Global::floatPrecision()).\
		arg(pt.y, 0, 'f', Global::floatPrecision()).\
		arg(pt.z, 0, 'f', Global::floatPrecision());

//	      // projection of the coordinate on screen
//	      Vec ptp = viewer->camera()->projectedCoordinatesOf(pt);
//	      str += QString(" proj(%1 %2)").\
//		arg(ptp.x).arg(ptp.y);
	    }

	  QFont font = QFont();
	  QFontMetrics metric(font);
	  int ht = metric.height();
	  int wd = metric.width(str);

	  //---------------------
	  x *= viewer->size().width()/viewer->camera()->screenWidth();
	  y *= viewer->size().height()/viewer->camera()->screenHeight();
	  //---------------------

	  y += ht/2;

	  glColor4f(0,0,0,0.8f);
	  glBegin(GL_QUADS);
	  glVertex2f(x+10, y+2);
	  glVertex2f(x+wd+15, y+2);
	  glVertex2f(x+wd+15, y-ht);
	  glVertex2f(x+10, y-ht);
	  glEnd();

	  glColor3f(1,1,1);
	  viewer->renderText(x+12, y-metric.descent(), str, font);
	}
    }
  viewer->stopScreenCoordinatesSystem();
}

void
HitPoints::updatePointDialog()
{
  makePointConnections();
  
  m_rawValues.clear();
  m_tagValues.clear();
}

bool
HitPoints::keyPressEvent(QKeyEvent *event)
{
  for(int i=0; i<m_points.count(); i++)
    {
      if (m_points[i]->grabsMouse())
	{
	  if (event->key() == Qt::Key_N)
	    {
	      m_showPoints = !m_showPoints;
	    }
	  else if (event->key() == Qt::Key_R)
	    {
	      m_showRawValues = !m_showRawValues;
	    }
	  else if (event->key() == Qt::Key_T)
	    {
	      m_showTagValues = !m_showTagValues;
	    }
	  else if (event->key() == Qt::Key_C)
	    {
	      m_showCoordinates = !m_showCoordinates;
	    }
	  else if (event->key() == Qt::Key_X)
	    {
	      m_points[i]->setMoveAxis(HitPointGrabber::MoveX);
	      return true;
	    }
	  else if (event->key() == Qt::Key_Y)
	    {
	      m_points[i]->setMoveAxis(HitPointGrabber::MoveY);
	      return true;
	    }
	  else if (event->key() == Qt::Key_Z)
	    {
	      m_points[i]->setMoveAxis(HitPointGrabber::MoveZ);
	      return true;
	    }
	  else if (event->key() == Qt::Key_W)
	    {
	      m_points[i]->setMoveAxis(HitPointGrabber::MoveAll);
	      return true;
	    }
	  else if (event->key() == Qt::Key_Delete ||
		   event->key() == Qt::Key_Backspace ||
		   event->key() == Qt::Key_Backtab)
	    {
	      m_points[i]->removeFromActivePool();
	      m_points[i]->removeFromMouseGrabberPool();	      
	      m_points.removeAt(i);

	      updatePointDialog();
	      return true;
	    }
	  else if (event->key() == Qt::Key_Space)
	    {
	      QString cmd;
	      bool ok;
	      QString mesg;
	      mesg = "deselectall, removepoints, addpoint, setpoint, normalize/se\n";
	      mesg += "mop [carve|restore|set] <rad> <decay>\n";
	      mesg += "mop paint <tag> <rad>\n";
	      cmd = QInputDialog::getText(0,
					  "Point Commands",
					  mesg,
					  QLineEdit::Normal,
					  "",
					  &ok);
	      if (ok && !cmd.isEmpty())
		processCommand(i, cmd); 
	    }
	}
    }
  
  return true;
}

void
HitPoints::updatePoint()
{
  m_rawValues.clear();
  m_tagValues.clear();
}

void
HitPoints::makePointConnections()
{
  for(int i=0; i<m_points.count(); i++)
    {
      m_points[i]->disconnect();
    }
  for(int i=0; i<m_points.count(); i++)
    {
      connect(m_points[i], SIGNAL(updatePoint()),
	      this, SLOT(updatePoint()));
    }
}

void
HitPoints::processCommand(int idx, QString cmd)
{
  bool ok;
  cmd = cmd.toLower();
  QStringList list = cmd.split(" ", QString::SkipEmptyParts);
  
  if (list[0] == "deselectall")
    {
      resetActive();
    }
  else if (list[0] == "removepoints")
    {
      if (list[1] == "all")
	{
	  int npts = m_points.count();
	  // clear all points
	  for(int ip=0; ip<npts; ip++)
	    {
	      m_points[0]->removeFromActivePool();
	      m_points[0]->removeFromMouseGrabberPool();	      
	      m_points.removeAt(0);
	    }
	  updatePointDialog();
	}
      else if (list[1] == "selected")
	{
	  int npts = m_points.count();
	  // clear all points
	  int ia = 0;
	  for(int ip=0; ip<npts; ip++)
	    {
	      if (m_points[ia]->isInActivePool())
		{
		  m_points[ia]->removeFromActivePool();
		  m_points[ia]->removeFromMouseGrabberPool();	      
		  m_points.removeAt(ia);
		}
	      else
		ia++;
	    }
	  updatePointDialog();
	}
    }
  else if (list[0] == "addpoint")
    {
      if (list.size() != 4)
	{
	  QMessageBox::information(0, "Point Command Error",
				   QString("needs 3 coordinates : only %1 given").arg(list.size()-1));
	  return;
	}

      Vec pos;
      float x=0,y=0,z=0;
      if (list.size() > 1) x = list[1].toFloat(&ok);
      if (list.size() > 2) y = list[2].toFloat(&ok);
      if (list.size() > 3) z = list[3].toFloat(&ok);
      pos = Vec(x,y,z);
      add(pos);
    }
  else if (list[0] == "normalize" ||
	   list[0] == "normalise")
    {
      for(int i=0; i<m_points.count(); i++)
	{
	  Vec pt = m_points[i]->point();
	  pt = Vec((int)pt.x, (int)pt.y, (int)pt.z);
	  m_points[i]->setPoint(pt);
	}
    }
  else if (list[0] == "setpoint")
    {
      if (list.size() != 4)
	{
	  QMessageBox::information(0, "Point Command Error",
				   QString("needs 3 coordinates : only %1 given").arg(list.size()-1));
	  return;
	}

      Vec pos;
      float x=0,y=0,z=0;
      if (list.size() > 1) x = list[1].toFloat(&ok);
      if (list.size() > 2) y = list[2].toFloat(&ok);
      if (list.size() > 3) z = list[3].toFloat(&ok);
      m_points[idx]->setPoint(Vec(x,y,z));
    }
  else if (list[0] == "showpointnumbers" || list[0] == "pointnumberson")
    m_showPoints = true;
  else if (list[0] == "hidepointnumbers" || list[0] == "pointnumbersoff")
    m_showPoints = false;
  else if (list[0] == "showrawvalues" || list[0] == "rawvalueson")
    m_showRawValues = true;
  else if (list[0] == "hiderawvalues" || list[0] == "rawvaluesoff")
    m_showRawValues = false;
  else if (list[0] == "showtagvalues" || list[0] == "tagvalueson")
    m_showTagValues = true;
  else if (list[0] == "hidetagvalues" || list[0] == "tagvaluesoff")
    m_showTagValues = false;
  else if (list[0] == "showcoordinates" || list[0] == "coordinateson")
    m_showCoordinates = true;
  else if (list[0] == "hidecoordinates" || list[0] == "coordinatesoff")
    m_showCoordinates = false;
  else if (list[0] == "mop")
    {
      if (list.size() > 1)
	{
	  int docarve = 1;
	  if (list[1] == "paint")
	    docarve = 0;
	  else if (list[1] == "carve")
	    docarve = 1;
	  else if (list[1] == "restore")
	    docarve = 2;
	  else if (list[1] == "set")
	    docarve = 3;
	  else
	    {
	      QMessageBox::information(0, "Error",
				       "Second parameter has to be paint/carve/restore/set.");
	      return;
	    }
	  float rad = 0;
	  float decay = 0;
	  int tag = -1;
	  if (docarve == 0)
	    {
	      if (list.size() <= 3)
		{
		  QMessageBox::information(0, "Error",
					   "paint needs tag and radius\n mop paint tag rad");
		  return;
		}
	      tag = list[2].toInt(&ok);
	      tag = qBound(0, tag, 255);
	      rad = list[3].toFloat(&ok);
	      rad = qBound(0.0f, rad, 200.0f);
	    }
	  else
	    {
	      if (list.size() > 2)
		{
		  rad = list[2].toFloat(&ok);
		  rad = qBound(0.0f, rad, 200.0f);
		}
	      if (list.size() > 3)
		{
		  decay = list[3].toFloat(&ok);
		  decay = qBound(0.0f, decay, rad);
		}
	    }
	  
	  QList<Vec> pts = points();
	  emit sculpt(docarve, pts, rad, decay, tag);

	  // clear all points
	  for(int ip=0; ip<pts.count(); ip++)
	    {
	      m_points[0]->removeFromActivePool();
	      m_points[0]->removeFromMouseGrabberPool();	      
	      m_points.removeAt(0);
	    }
	  updatePointDialog();
	}
      else
	QMessageBox::information(0, "Error",
				 "No operation specified for mop.");
    }
  else
    QMessageBox::information(0, "Point Command Error",
			     QString("Cannot understand the command : ") +
			     cmd);
}

HitPointGrabber*
HitPoints::checkIfGrabsMouse(int x, int y, Camera *cam)
{
  for(int i=0; i<m_points.count(); i++)
    {
      m_points[i]->checkIfGrabsMouse(x, y, cam);
      if (m_points[i]->grabsMouse())
	return m_points[i];
    }
  return NULL;
}
void
HitPoints::mousePressEvent(QMouseEvent *e, Camera *c)
{
  for(int i=0; i<m_points.count(); i++)
    m_points[i]->mousePressEvent(e,c);
}
void
HitPoints::mouseMoveEvent(QMouseEvent *e, Camera *c)
{
  for(int i=0; i<m_points.count(); i++)
    m_points[i]->mouseMoveEvent(e,c);
}
void
HitPoints::mouseReleaseEvent(QMouseEvent *e, Camera *c)
{
  for(int i=0; i<m_points.count(); i++)
    m_points[i]->mouseReleaseEvent(e,c);
}

