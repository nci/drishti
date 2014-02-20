#include "global.h"
#include "grids.h"
#include "staticfunctions.h"
#include "propertyeditor.h"

#include <QFileDialog>

int Grids::count() { return m_grids.count(); }

Grids::Grids()
{
  m_sameForAll = true;
  m_grids.clear();
}

Grids::~Grids()
{
  clear();
}
void 
Grids::clear()
{
  for(int i=0; i<m_grids.count(); i++)
    m_grids[i]->disconnect();

  for(int i=0; i<m_grids.count(); i++)
    m_grids[i]->removeFromMouseGrabberPool();

  for(int i=0; i<m_grids.count(); i++)
    delete m_grids[i];

  m_grids.clear();
}

bool
Grids::isInMouseGrabberPool(int i)
{
  if (i < m_grids.count())
    return m_grids[i]->isInMouseGrabberPool();
  else
    return false;
}
void
Grids::addInMouseGrabberPool(int i)
{
  if (i < m_grids.count())
    m_grids[i]->addInMouseGrabberPool();
}
void
Grids::addInMouseGrabberPool()
{
  for(int i=0; i<m_grids.count(); i++)
    m_grids[i]->addInMouseGrabberPool();
}
void
Grids::removeFromMouseGrabberPool(int i)
{
  if (i < m_grids.count())
    m_grids[i]->removeFromMouseGrabberPool();
}

void
Grids::removeFromMouseGrabberPool()
{
  for(int i=0; i<m_grids.count(); i++)
    m_grids[i]->removeFromMouseGrabberPool();
}


bool
Grids::grabsMouse()
{
  for(int i=0; i<m_grids.count(); i++)
    {
      if (m_grids[i]->grabsMouse())
	return true;
    }
  return false;
}

void
Grids::setPoints(int idx, QList<Vec> pts)
{
  m_grids[idx]->setPoints(pts);
 
  emit updateGL();
}

void
Grids::addGrid(QList<Vec> pts, int cols, int rows)
{
  GridGrabber *pg = new GridGrabber();
  pg->setColRow(cols, rows);
  pg->setPoints(pts);
  m_grids.append(pg);

  makeGridConnections();
}

void
Grids::addGrid(GridObject po)
{
  GridGrabber *pg = new GridGrabber();
  pg->set(po);
  m_grids.append(pg);

  makeGridConnections();
}

void
Grids::addGrid(QString flnm)
{
  QFile fgrid(flnm);
  fgrid.open(QFile::ReadOnly);
  QTextStream fd(&fgrid);

  while (! fd.atEnd())
    {
      QString line = fd.readLine();
      QStringList list = line.split(" ", QString::SkipEmptyParts);
      if (list.count() == 2)
	{
	  int cols = list[0].toInt();
	  int rows = list[1].toInt();
	  int npts = cols*rows;
	  QList<Vec> pts;
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
	 
	  if (pts.count() > 0)
	    {
	      GridGrabber *pg = new GridGrabber();
	      pg->setColRow(cols, rows);
	      pg->setPoints(pts);
	      m_grids.append(pg);
	    }	  
	}
    }

  makeGridConnections();
}

QList<GridObject>
Grids::grids()
{
  QList<GridObject> po;
  for(int i=0; i<m_grids.count(); i++)
    po.append(m_grids[i]->get());

  return po;
}

void
Grids::setGrids(QList<GridObject> po)
{
  clear();

  for(int i=0; i<po.count(); i++)
    {
      GridGrabber *pg = new GridGrabber();
      pg->set(po[i]);
      m_grids.append(pg);
    }

  makeGridConnections();
}

void
Grids::updateScaling()
{
  for(int i=0; i<m_grids.count();i++)
    m_grids[i]->regrid();
}

void
Grids::draw(QGLViewer *viewer, bool backToFront, Vec lightVec)
{
  for(int i=0; i<m_grids.count();i++)
    m_grids[i]->draw(viewer,
		     m_grids[i]->grabsMouse(),
		     backToFront,
		     lightVec);
}

void
Grids::postdraw(QGLViewer *viewer)
{
  for(int i=0; i<m_grids.count();i++)
    {
      int x,y;
      m_grids[i]->mousePosition(x,y);
      m_grids[i]->postdraw(viewer,
			   x, y,
			   m_grids[i]->grabsMouse());
    }
}

bool
Grids::keyPressEvent(QKeyEvent *event)
{
  for(int i=0; i<m_grids.count(); i++)
    {
      if (m_grids[i]->grabsMouse())
	{
	  if (event->key() == Qt::Key_P)
	    {
	      bool b = m_grids[i]->showPoints();
	      m_grids[i]->setShowPoints(!b);    
	      return true;
	    }
	  else if (event->key() == Qt::Key_N)
	    {
	      bool b = m_grids[i]->showPointNumbers();
	      m_grids[i]->setShowPointNumbers(!b);    
	      return true;
	    }
	  else if (event->key() == Qt::Key_X)
	    {
	      m_grids[i]->setMoveAxis(GridGrabber::MoveX);
	      return true;
	    }
	  else if (event->key() == Qt::Key_Y)
	    {
	      if (event->modifiers() & Qt::ControlModifier ||
		  event->modifiers() & Qt::MetaModifier)
		m_grids[i]->redo();
	      else
		m_grids[i]->setMoveAxis(GridGrabber::MoveY);
	      return true;
	    }
	  else if (event->key() == Qt::Key_Z)
	    {
	      if (event->modifiers() & Qt::ControlModifier ||
		  event->modifiers() & Qt::MetaModifier)
		m_grids[i]->undo();
	      else
		m_grids[i]->setMoveAxis(GridGrabber::MoveZ);
	      return true;
	    }
	  else if (event->key() == Qt::Key_W)
	    {
	      m_grids[i]->setMoveAxis(GridGrabber::MoveAll);
	      return true;
	    }
	  else if (event->key() == Qt::Key_Delete ||
		   event->key() == Qt::Key_Backspace ||
		   event->key() == Qt::Key_Backtab)
	    {
	      m_grids[i]->removeFromMouseGrabberPool();
	      m_grids.removeAt(i);
	      return true;
	    }
	  
	  if (event->key() == Qt::Key_Space)
	    {
	      PropertyEditor propertyEditor;
	      QMap<QString, QVariantList> plist;

	      QVariantList vlist;

	      vlist.clear();
	      vlist << QVariant("double");
	      vlist << QVariant(m_grids[i]->opacity());
	      vlist << QVariant(0.0);
	      vlist << QVariant(1.0);
	      vlist << QVariant(0.1); // singlestep
	      vlist << QVariant(1); // decimals
	      plist["opacity"] = vlist;

	      vlist.clear();
	      vlist << QVariant("color");
	      Vec pcolor = m_grids[i]->color();
	      QColor dcolor = QColor::fromRgbF(pcolor.x,
					       pcolor.y,
					       pcolor.z);
	      vlist << dcolor;
	      plist["color"] = vlist;

	      vlist.clear();
	      plist["command"] = vlist;


	      vlist.clear();
	      QFile helpFile(":/grids.help");
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
	      keys << "color";
	      keys << "opacity";
	      keys << "gap";
	      keys << "command";
	      keys << "commandhelp";
	      

	      propertyEditor.set("Grid Parameters", plist, keys);

	      
	      QMap<QString, QPair<QVariant, bool> > vmap;

	      if (propertyEditor.exec() == QDialog::Accepted)
		vmap = propertyEditor.get();
	      else
		return true;
	      
	      keys = vmap.keys();

	      for(int ik=0; ik<keys.count(); ik++)
		{
		  QPair<QVariant, bool> pair = vmap.value(keys[ik]);


		  if (pair.second)
		    {
		      if (keys[ik] == "color")
			{
			  QColor color = pair.first.value<QColor>();
			  float r = color.redF();
			  float g = color.greenF();
			  float b = color.blueF();
			  pcolor = Vec(r,g,b);
			  m_grids[i]->setColor(pcolor);
			}
		      else if (keys[ik] == "opacity")
			m_grids[i]->setOpacity(pair.first.toDouble());
		    }
		}

	      QString cmd = propertyEditor.getCommandString();
	      if (!cmd.isEmpty())
		processCommand(i, cmd);	
	      
 	      emit updateGL();
	    }
	}
    }
  
  return true;
}

void
Grids::makeGridConnections()
{
  for(int i=0; i<m_grids.count(); i++)
    m_grids[i]->disconnect();

  for(int i=0; i<m_grids.count(); i++)
    {
      connect(m_grids[i], SIGNAL(selectForEditing(int, int)),
	      this, SLOT(selectForEditing(int, int)));

      connect(m_grids[i], SIGNAL(deselectForEditing()),
	      this, SLOT(deselectForEditing()));
    }
}
void
Grids::deselectForEditing()
{
  for(int i=0; i<m_grids.count(); i++)
    m_grids[i]->setPointPressed(-1);
}

void
Grids::selectForEditing(int mouseButton,
			int p0)
{
  int idx = -1;
  for(int i=0; i<m_grids.count(); i++)
    {
      if (m_grids[i]->grabsMouse())
	{
	  idx = i;
	  break;
	}
    }
  if (idx == -1)
    return;
}

void
Grids::processCommand(int idx, QString cmd)
{
  bool ok;
  QString origCmd = cmd;
  cmd = cmd.toLower();
  QStringList list = cmd.split(" ", QString::SkipEmptyParts);
  
  if (list[0] == "normalize" ||
      list[0] == "normalise")
    {
      m_grids[idx]->normalize();
    }
  else if (list[0] == "insertrow")
    {
      if (list.count() == 2)
	{
	  int r = list[1].toInt();
	  m_grids[idx]->insertRow(r);
	}
      else
	QMessageBox::information(0, "Error", "Row number not specified");
    }	   
  else if (list[0] == "removerow")
    {
      if (list.count() == 2)
	{
	  int r = list[1].toInt();
	  m_grids[idx]->removeRow(r);
	}
      else
	QMessageBox::information(0, "Error", "Row number not specified");
    }	   
  else if (list[0] == "insertcol")
    {
      if (list.count() == 2)
	{
	  int r = list[1].toInt();
	  m_grids[idx]->insertCol(r);
	}
      else
	QMessageBox::information(0, "Error", "Row number not specified");
    }	   
  else if (list[0] == "removecol")
    {
      if (list.count() == 2)
	{
	  int r = list[1].toInt();
	  m_grids[idx]->removeCol(r);
	}
      else
	QMessageBox::information(0, "Error", "Row number not specified");
    }	   
  else if (list[0] == "sticktosurface")
    {
      int radius = 1;
      if (list.size() > 1)
	{
	  radius = list[1].toInt(&ok);
	  radius = qMin(radius, 200);
	}
      QList< QPair<Vec, Vec> > gridPNs = m_grids[idx]->getPointsAndNormals();
      emit gridStickToSurface(idx, radius, gridPNs);
    }
  else if (list[0] == "save")
    {
      QString flnm;
      flnm = QFileDialog::getSaveFileName(0,
					  "Save grid points to text file",
					  Global::previousDirectory(),
					  "Files (*.*)");
      
      if (flnm.isEmpty())
	return;
      
      QList<Vec> pts = m_grids[idx]->points();
      
      QFile fgrid(flnm);
      fgrid.open(QFile::WriteOnly | QFile::Text);
      QTextStream fd(&fgrid);
      fd << m_grids[idx]->columns()
	 << "   "
	 <<  m_grids[idx]->rows()
	 << "\n";
      for(int pi=0; pi < pts.count(); pi++)
	fd << pts[pi].x << " " << pts[pi].y << " " << pts[pi].z << "\n";
      
      fd.flush();
    }
  else
    QMessageBox::information(0, "Grid Command Error",
			     QString("Cannot understand the command : ") +
			     cmd);
  
}
