#include "global.h"
#include "staticfunctions.h"
#include "hitpoints.h"

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
  m_showCoordinates = false;
  m_points.clear();
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
HitPoints::removeLastPoint()
{
  int i = m_points.count()-1;
  m_points[i]->removeFromActivePool();
  m_points[i]->removeFromMouseGrabberPool();	      
  m_points.removeAt(i);
}

void
HitPoints::removeLastActivePoint()
{
  int ia = -1;
  for(int i=0; i<m_points.count(); i++)
    {      
      if (m_points[i]->isInActivePool())
	ia = i;
    }
  if (ia == -1)
    return;

  m_points[ia]->removeFromActivePool();
  m_points[ia]->removeFromMouseGrabberPool();	      
  m_points.removeAt(ia);
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
  QList<Vec> pts;
  for(int i=0; i<m_points.count(); i++)
    {      
      if (m_points[i]->isInActivePool())
	pts.append(m_points[i]->point());
    }
  return pts;

//  QList<HitPointGrabber*> activePool = HitPointGrabber::activePool();
//  QList<Vec> pts;
//  for(int i=0; i<activePool.count(); i++)
//    pts.append(activePool[i]->point());
//  return pts;
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


//void
//HitPoints::addPoints(QString flnm)
//{
//  QList<Vec> pts;
//
//  QFile fpoints(flnm);
//  fpoints.open(QFile::ReadOnly);
//  QTextStream fd(&fpoints);
//  while (! fd.atEnd())
//    {
//      QString line = fd.readLine();
//      QStringList list = line.split(" ", QString::SkipEmptyParts);
//      if (list.count() == 1)
//	{
//	  int npts = list[0].toInt();
//	  for(int i=0; i<npts; i++)
//	    {
//	      if (fd.atEnd())
//		break;
//	      else
//		{
//		  QString line = fd.readLine();
//		  QStringList list = line.split(" ", QString::SkipEmptyParts);
//		  if (list.count() == 3)
//		    {
//		      float x = list[0].toFloat();
//		      float y = list[1].toFloat();
//		      float z = list[2].toFloat();
//		      pts.append(Vec(x,y,z));
//		    }
//		}
//	    }
//	}
//    }
//
//  if (pts.count() > 0)
//    {
//      for(int i=0; i<pts.count(); i++)
//	{
//	  HitPointGrabber *hpg = new HitPointGrabber(pts[i]);
//	  m_points.append(hpg);
//	}
//    }
//
//  QMessageBox::information(0, "", QString("No. of points in the scene : %1").\
//			   arg(m_points.count()));
//
//  removeFromMouseGrabberPool();
//  if (m_grab)
//    addInMouseGrabberPool();
//}

void
HitPoints::addPoints(QString flnm)
{
  QList<Vec> pts;

  pts = readPointsFromFile(flnm);
  
  if (pts.count() > 0)
    {
      for(int i=0; i<pts.count(); i++)
	{
	  HitPointGrabber *hpg = new HitPointGrabber(pts[i]);
	  m_points.append(hpg);
	}
    }

  QMessageBox::information(0, "", QString("No. of points in the scene : %1").\
			   arg(m_points.count()));

  removeFromMouseGrabberPool();
  if (m_grab)
    addInMouseGrabberPool();
}

QList<Vec>
HitPoints::readPointsFromFile(QString flnm)
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

  return pts;
}

void HitPoints::ignore(bool i) { m_ignore = i; }

void
HitPoints::add(Vec opt)
{
  Vec voxelScaling = Global::voxelScaling();
  Vec pt = VECDIVIDE(opt, voxelScaling);


  HitPointGrabber *hpg = new HitPointGrabber(pt);
  m_points.append(hpg);


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
HitPoints::draw()
{
  if (m_barePoints.count() == 0 &&
      m_points.count() == 0)
    return;

  // offset to draw coplanar points
  glDepthRange(0.0, 0.99);

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
  glPointSize(qMax(1, m_pointSize-2));
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
  glPointSize(m_pointSize+5);
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
  glPointSize(m_pointSize+2);
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


  glDepthRange(0.0, 1.0);
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

	  StaticFunctions::renderText(x+12, y, str, font, Qt::black, Qt::cyan);
	}
    }
  viewer->stopScreenCoordinatesSystem();
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

	      return true;
	    }
	  else if (event->key() == Qt::Key_Space)
	    {
	      QString cmd;
	      bool ok;
	      QString mesg;
	      mesg = "deselectall, removepoints, addpoint, setpoint, normalize/se\n";
	      mesg += "mop [carve|restore|set] <rad> <decay>\n";
	      mesg += "mop paint <tag> <rad>\n\n\n";
	      mesg += "Keyboard Options :\n";
	      mesg += "c : toggle coordinate display\n";
	      mesg += "n : toggle point numbers\n";
	      mesg += "space bar : bring up this dialog\n";
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
  else if (list[0] == "showcoordinates" || list[0] == "coordinateson")
    m_showCoordinates = true;
  else if (list[0] == "hidecoordinates" || list[0] == "coordinatesoff")
    m_showCoordinates = false;
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

