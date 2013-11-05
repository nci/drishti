#include "global.h"
#include "pathgroups.h"
#include "dcolordialog.h"
#include "staticfunctions.h"
#include "propertyeditor.h"

int PathGroups::count() { return m_paths.count(); }

PathGroups::PathGroups()
{
  m_sameForAll = true;
  m_paths.clear();
}

PathGroups::~PathGroups()
{
  clear();
}
void 
PathGroups::clear()
{
  for(int i=0; i<m_paths.count(); i++)
    m_paths[i]->disconnect();

  for(int i=0; i<m_paths.count(); i++)
    m_paths[i]->removeFromMouseGrabberPool();

  for(int i=0; i<m_paths.count(); i++)
    delete m_paths[i];

  m_paths.clear();
}

bool
PathGroups::isInMouseGrabberPool(int i)
{
  if (i < m_paths.count())
    return m_paths[i]->isInMouseGrabberPool();
  else
    return false;
}
void
PathGroups::addInMouseGrabberPool(int i)
{
  if (i < m_paths.count())
    m_paths[i]->addInMouseGrabberPool();
}
void
PathGroups::addInMouseGrabberPool()
{
  for(int i=0; i<m_paths.count(); i++)
    m_paths[i]->addInMouseGrabberPool();
}
void
PathGroups::removeFromMouseGrabberPool(int i)
{
  if (i < m_paths.count())
    m_paths[i]->removeFromMouseGrabberPool();
}

void
PathGroups::removeFromMouseGrabberPool()
{
  for(int i=0; i<m_paths.count(); i++)
    m_paths[i]->removeFromMouseGrabberPool();
}


bool
PathGroups::grabsMouse()
{
  for(int i=0; i<m_paths.count(); i++)
    {
      if (m_paths[i]->grabsMouse())
	return true;
    }
  return false;
}


void
PathGroups::addPath(QList<Vec> pts)
{
  PathGroupGrabber *pg = new PathGroupGrabber();
  pg->setPoints(pts);
  m_paths.append(pg);

  makePathConnections();
}

void
PathGroups::addPath(PathGroupObject po)
{
  PathGroupGrabber *pg = new PathGroupGrabber();
  pg->set(po);
  m_paths.append(pg);

  makePathConnections();
}

bool
PathGroups::checkForMultiplePaths(QString flnm)
{
  int npaths = 0;

  QFile fpath(flnm);
  fpath.open(QFile::ReadOnly);
  QTextStream fd(&fpath);

  QString line = fd.readLine();
  if (line.contains("#indexed", Qt::CaseInsensitive)) //
      return true;

  while (! fd.atEnd())
    {
      QStringList list = line.split(" ", QString::SkipEmptyParts);
      if (list.count() == 1)
	{	  
	  int npts = list[0].toInt();
	  if (npts > 0) npaths ++;

	  if (npaths > 1)
	    return true;

	  for(int i=0; i<npts; i++)
	    {
	      if (fd.atEnd())
		break;
	      else
		fd.readLine();
	    }
	}
      line = fd.readLine();
    }
  
  return (npaths > 1);
}

void
PathGroups::addIndexedPath(QString flnm)
{
  QFile fpath(flnm);
  fpath.open(QFile::ReadOnly);
  QTextStream fd(&fpath);

  QString line = fd.readLine(); // ignore first line

  PathGroupGrabber *pg = new PathGroupGrabber();

  QList<Vec> pts;
  // first read all points
  while (! fd.atEnd())
    {
      line = fd.readLine(); // ignore first line
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
		  line = fd.readLine();
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
	  break;
	}
    }

  // now read indices
  while (! fd.atEnd())
    {
      line = fd.readLine();
      QStringList list = line.split(" ", QString::SkipEmptyParts);
      if (list.count() > 1)
	{
	  QList<Vec> ipts;
	  for(int i=0; i<list.count(); i++)
	    {
	      int j = list[i].toInt();
	      if (j>=0 && j<pts.count())
		ipts.append(pts[j]);
	    }
	  if (ipts.count() > 0)
	    pg->addPoints(ipts);		  
	}
    }

  m_paths.append(pg);
  
  makePathConnections();
}

void
PathGroups::addPath(QString flnm)
{
  QFile fpath(flnm);
  fpath.open(QFile::ReadOnly);
  QTextStream fd(&fpath);

  QString line = fd.readLine();
  if (line.contains("#indexed", Qt::CaseInsensitive))
    {
      addIndexedPath(flnm);
      return;
    }


  PathGroupGrabber *pg = new PathGroupGrabber();

  while (! fd.atEnd())
    {
      QStringList list = line.split(" ", QString::SkipEmptyParts);
      if (list.count() == 1)
	{
	  int npts = list[0].toInt();
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
	    pg->addPoints(pts);		  
	}
      line = fd.readLine();
    }

  m_paths.append(pg);
  
  makePathConnections();
}

void
PathGroups::addVector(QString flnm)
{
  QFile fpath(flnm);
  fpath.open(QFile::ReadOnly);
  QTextStream fd(&fpath);

  QString line = fd.readLine();

  PathGroupGrabber *pg = new PathGroupGrabber();

  QList<Vec> pts;
  while (! fd.atEnd())
    {
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
		  if (list.count() > 3)
		    {
		      float x = list[0].toFloat();
		      float y = list[1].toFloat();
		      float z = list[2].toFloat();
		      pts.append(Vec(x,y,z));
		      x += list[3].toFloat();
		      if (list.count() > 4) y += list[4].toFloat();
		      if (list.count() > 5) z += list[5].toFloat();
		      pts.append(Vec(x,y,z));
		    }
		}
	    }
	}

      line = fd.readLine();
    }

  pg->setVectorPoints(pts);		  
  m_paths.append(pg);
  
  makePathConnections();
}

QList<PathGroupObject>
PathGroups::paths()
{
  QList<PathGroupObject> po;
  for(int i=0; i<m_paths.count(); i++)
    po.append(m_paths[i]->get());

  return po;
}

void
PathGroups::setPaths(QList<PathGroupObject> po)
{
  clear();

  for(int i=0; i<po.count(); i++)
    {
      PathGroupGrabber *pg = new PathGroupGrabber();
      pg->set(po[i]);
      m_paths.append(pg);
    }

  makePathConnections();
}

void
PathGroups::updateScaling()
{
  for(int i=0; i<m_paths.count();i++)
    m_paths[i]->computePathLength();
}

void
PathGroups::predraw(QGLViewer *viewer, bool backToFront,
		    QList<Vec> cpos,
		    QList<Vec> cnorm,
		    QList<CropObject> crops)
{
  for(int i=0; i<m_paths.count();i++)
    m_paths[i]->predraw(viewer,
			m_paths[i]->grabsMouse(),
			backToFront,
			cpos, cnorm, crops);
}

void
PathGroups::draw(QGLViewer *viewer,
		 bool backToFront,
		 Vec lightVec,
		 bool geoblend)
{
  for(int i=0; i<m_paths.count();i++)
    {
      if ((m_paths[i]->blendMode() && geoblend) ||
	  (!m_paths[i]->blendMode() && !geoblend))
	m_paths[i]->draw(viewer,
			 m_paths[i]->grabsMouse(),
			 backToFront,
			 lightVec);
    }
}

void
PathGroups::postdraw(QGLViewer *viewer)
{
  for(int i=0; i<m_paths.count();i++)
    {
      int x,y;
      m_paths[i]->mousePosition(x,y);
      m_paths[i]->postdraw(viewer,
			   x, y,
			   m_paths[i]->grabsMouse());
    }
}

bool
PathGroups::keyPressEvent(QKeyEvent *event)
{
  for(int i=0; i<m_paths.count(); i++)
    {
      if (m_paths[i]->grabsMouse())
	{
	  if (event->key() == Qt::Key_G)
	    {
	      m_paths[i]->removeFromMouseGrabberPool();
	      return true;
	    }
	  if (event->key() == Qt::Key_C)
	    {
	      bool b = m_paths[i]->closed();
	      m_paths[i]->setClosed(!b);
	      return true;
	    }
	  else if (event->key() == Qt::Key_P)
	    {
	      bool b = m_paths[i]->showPoints();
	      m_paths[i]->setShowPoints(!b);    
	      return true;
	    }
	  else if (event->key() == Qt::Key_L)
	    {
	      bool b = m_paths[i]->showLength();
	      m_paths[i]->setShowLength(!b);    
	      return true;
	    }
	  else if (event->key() == Qt::Key_X)
	    {
	      m_paths[i]->setMoveAxis(PathGroupGrabber::MoveX);
	      return true;
	    }
	  else if (event->key() == Qt::Key_Y)
	    {
	      if (event->modifiers() & Qt::ControlModifier ||
		  event->modifiers() & Qt::MetaModifier)
		m_paths[i]->redo();
	      else
		m_paths[i]->setMoveAxis(PathGroupGrabber::MoveY);
	      return true;
	    }
	  else if (event->key() == Qt::Key_Z)
	    {
	      if (event->modifiers() & Qt::ControlModifier ||
		  event->modifiers() & Qt::MetaModifier)
		m_paths[i]->undo();
	      else
		m_paths[i]->setMoveAxis(PathGroupGrabber::MoveZ);
	      return true;
	    }
	  else if (event->key() == Qt::Key_W)
	    {
	      m_paths[i]->setMoveAxis(PathGroupGrabber::MoveAll);
	      return true;
	    }
	  else if (event->key() == Qt::Key_S)
	    {
	      int idx = m_paths[i]->pointPressed();
	      if (idx > -1)
		{
		  float radx = m_paths[i]->getRadX(idx);
		  if (event->modifiers() & Qt::ShiftModifier)
		    radx--;
		  else
		    radx++;
		  radx = qMax(1.0f, radx);
		  m_paths[i]->setRadX(idx, radx, m_sameForAll);
		  return true;
		}
	    }
	  else if (event->key() == Qt::Key_T)
	    {
	      int idx = m_paths[i]->pointPressed();
	      if (idx > -1)
		{
		  float rady = m_paths[i]->getRadY(idx);
		  if (event->modifiers() & Qt::ShiftModifier)
		    rady--;
		  else
		    rady++;
		  rady = qMax(1.0f, rady);
		  m_paths[i]->setRadY(idx, rady, m_sameForAll);
		}
	      else // switch to tube mode
		{
		  if (event->modifiers() & Qt::ShiftModifier)
		    {
		      m_paths[i]->loadCaption();
		    }
		  else
		    {
		      bool b = m_paths[i]->tube();
		      m_paths[i]->setTube(!b);
		    }
		}

	      return true;
	    }
	  else if (event->key() == Qt::Key_A)
	    {
	      int idx = m_paths[i]->pointPressed();
	      if (idx > -1)
		{
		  float a = m_paths[i]->getAngle(idx);
		  if (event->modifiers() & Qt::ShiftModifier)
		    a--;
		  else
		    a++;
		  m_paths[i]->setAngle(idx, a, m_sameForAll);
		  return true;
		}
	    }
	  else if (event->key() == Qt::Key_Delete ||
		   event->key() == Qt::Key_Backspace ||
		   event->key() == Qt::Key_Backtab)
	    {
	      m_paths[i]->removeFromMouseGrabberPool();
	      m_paths.removeAt(i);
	      return true;
	    }

	  if (event->key() == Qt::Key_Space)
	    {
	      PropertyEditor propertyEditor;
	      QMap<QString, QVariantList> plist;

	      QVariantList vlist;

	      vlist.clear();
	      vlist << QVariant("double");
	      vlist << QVariant(m_paths[i]->opacity());
	      vlist << QVariant(0.0);
	      vlist << QVariant(1.0);
	      vlist << QVariant(0.1); // singlestep
	      vlist << QVariant(1); // decimals
	      plist["opacity"] = vlist;

	      vlist.clear();
	      vlist << QVariant("colorgradient");
	      QGradientStops stops = m_paths[i]->stops();
	      for(int s=0; s<stops.size(); s++)
		{
		  float pos = stops[s].first;
		  QColor color = stops[s].second;
		  int r = color.red();
		  int g = color.green();
		  int b = color.blue();
		  int a = color.alpha();
		  vlist << QVariant(pos);
		  vlist << QVariant(r);
		  vlist << QVariant(g);
		  vlist << QVariant(b);
		  vlist << QVariant(a);
		}
	      plist["color"] = vlist;

	      vlist.clear();
	      vlist << QVariant("checkbox");
	      vlist << QVariant(m_paths[i]->filterPathLen());
	      plist["filter on length"] = vlist;

	      float vmin, vmax;
	      m_paths[i]->userPathlenMinmax(vmin, vmax);
	      vlist.clear();
	      vlist << QVariant("string");
	      vlist << QVariant(QString("%1 %2").arg(vmin).arg(vmax));
	      plist["length bounds"] = vlist;

	      vlist.clear();
	      vlist << QVariant("checkbox");
	      vlist << QVariant(m_paths[i]->scaleType());
	      plist["scale type"] = vlist;

	      vlist.clear();
	      vlist << QVariant("checkbox");
	      vlist << QVariant(m_paths[i]->depthcue());
	      plist["depthcue"] = vlist;

	      vlist.clear();
	      vlist << QVariant("int");
	      vlist << QVariant(m_paths[i]->segments());
	      vlist << QVariant(1);
	      vlist << QVariant(100);
	      plist["smoothness"] = vlist;

	      vlist.clear();
	      vlist << QVariant("int");
	      vlist << QVariant(m_paths[i]->sections());
	      vlist << QVariant(1);
	      vlist << QVariant(100);
	      plist["sections"] = vlist;

	      vlist.clear();
	      vlist << QVariant("int");
	      vlist << QVariant(m_paths[i]->sparseness());
	      vlist << QVariant(1);
	      vlist << QVariant(100);
	      plist["sparseness"] = vlist;

	      vlist.clear();
	      vlist << QVariant("int");
	      vlist << QVariant(m_paths[i]->separation());
	      vlist << QVariant(0);
	      vlist << QVariant(10);
	      plist["screen separation"] = vlist;

	      vlist.clear();
	      vlist << QVariant("combobox");
	      vlist << QVariant(m_paths[i]->capType());
	      vlist << QVariant("flat");
	      vlist << QVariant("round");
	      vlist << QVariant("arrow");
	      plist["cap style"] = vlist;

	      vlist.clear();
	      vlist << QVariant("combobox");
	      vlist << QVariant(m_paths[i]->arrowDirection());
	      vlist << QVariant("forward");
	      vlist << QVariant("backward");
	      plist["arrow direction"] = vlist;

	      vlist.clear();
	      vlist << QVariant("checkbox");
	      vlist << QVariant(m_paths[i]->animate());
	      plist["animate"] = vlist;

	      vlist.clear();
	      vlist << QVariant("checkbox");
	      vlist << QVariant(m_paths[i]->allowInterpolate());
	      plist["interpolate"] = vlist;

	      vlist.clear();
	      vlist << QVariant("int");
	      vlist << QVariant(m_paths[i]->animateSpeed());
	      vlist << QVariant(10);
	      vlist << QVariant(100);
	      plist["animation speed"] = vlist;

	      vlist.clear();
	      vlist << QVariant("checkbox");
	      vlist << QVariant(m_paths[i]->arrowForAll());
	      plist["arrows for all"] = vlist;

	      vlist.clear();
	      vlist << QVariant("checkbox");
	      vlist << QVariant(m_sameForAll);
	      plist["same for all"] = vlist;

	      vlist.clear();
	      vlist << QVariant("checkbox");
	      vlist << QVariant(m_paths[i]->clip());
	      plist["clip"] = vlist;

	      vlist.clear();
	      vlist << QVariant("checkbox");
	      vlist << QVariant(m_paths[i]->allowEditing());
	      plist["allow editing"] = vlist;

	      vlist.clear();
	      vlist << QVariant("checkbox");
	      vlist << QVariant(m_paths[i]->blendMode());
	      plist["blend with volume"] = vlist;

	      vlist.clear();
	      plist["command"] = vlist;

	      vlist.clear();
	      QFile helpFile(":/pathgroups.help");
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


	      vlist.clear();
	      QString mesg;
	      float pmin, pmax;
	      m_paths[i]->pathlenMinmax(pmin, pmax);
	      mesg = QString("min/max path lengths : %1 %2\n").arg(pmin).arg(pmax);
	      float mins,maxs;
	      mins = m_paths[i]->minScale();
	      maxs = m_paths[i]->maxScale();
	      mesg += QString("min/max scale : %1 %2\n").arg(mins).arg(maxs);
	      vlist << mesg;
	      plist["message"] = vlist;


	      QStringList keys;
	      keys << "color";
	      keys << "opacity";
	      keys << "depthcue";
	      keys << "gap";
	      keys << "filter on length";
	      keys << "length bounds";
	      keys << "scale type";
	      keys << "gap";
	      keys << "smoothness";
	      keys << "sections";
	      keys << "sparseness";
	      keys << "screen separation";
	      keys << "gap";
	      keys << "cap style";
	      keys << "arrow direction";
	      keys << "arrows for all";
	      keys << "gap";
	      keys << "animate";
	      keys << "interpolate";
	      keys << "animation speed";
	      keys << "gap";
	      keys << "same for all";
	      keys << "clip";
	      keys << "blend with volume";
	      keys << "allow editing";
	      keys << "command";
	      keys << "commandhelp";
	      keys << "message";
	      

	      propertyEditor.set("Path Group Parameters", plist, keys);

	      
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
			  QGradientStops stops = propertyEditor.getGradientStops(keys[ik]);
			  m_paths[i]->setStops(stops);
			}
		      else if (keys[ik] == "opacity")
			m_paths[i]->setOpacity(pair.first.toDouble());
		      else if (keys[ik] == "scale type")
			m_paths[i]->setScaleType(pair.first.toBool());
		      else if (keys[ik] == "sections")
			m_paths[i]->setSections(pair.first.toInt());
		      else if (keys[ik] == "smoothness")
			m_paths[i]->setSegments(pair.first.toInt());
		      else if (keys[ik] == "sparseness")
			m_paths[i]->setSparseness(pair.first.toInt());
		      else if (keys[ik] == "screen separation")
			m_paths[i]->setSeparation(pair.first.toInt());
		      else if (keys[ik] == "depthcue")
			m_paths[i]->setDepthcue(pair.first.toBool());
		      else if (keys[ik] == "animate")
			m_paths[i]->setAnimate(pair.first.toBool());
		      else if (keys[ik] == "interpolate")
			m_paths[i]->setAllowInterpolate(pair.first.toBool());
		      else if (keys[ik] == "animation speed")
			m_paths[i]->setAnimateSpeed(pair.first.toInt());
		      else if (keys[ik] == "cap style")
			m_paths[i]->setCapType(pair.first.toInt());
		      else if (keys[ik] == "arrow direction")
			m_paths[i]->setArrowDirection(pair.first.toInt());
		      else if (keys[ik] == "arrows for all")
			m_paths[i]->setArrowForAll(pair.first.toBool());
		      else if (keys[ik] == "same for all")
			{
			  m_sameForAll = pair.first.toBool();
			  m_paths[i]->setSameForAll(m_sameForAll);
			}
		      else if (keys[ik] == "clip")
			m_paths[i]->setClip(pair.first.toBool());
		      else if (keys[ik] == "allow editing")
			m_paths[i]->setAllowEditing(pair.first.toBool());
		      else if (keys[ik] == "blend with volume")
			m_paths[i]->setBlendMode(pair.first.toBool());
		      else if (keys[ik] == "filter on length")
			m_paths[i]->setFilterPathLen(pair.first.toBool());
		      else if (keys[ik] == "length bounds")
			{
			  QString vstr = pair.first.toString();
			  QStringList vl = vstr.split(" ", QString::SkipEmptyParts);
			  if (vl.count() == 2)
			    m_paths[i]->setUserPathlenMinmax(vl[0].toDouble(),
							     vl[1].toDouble());
			}
		    }
		}

	      QString cmd = propertyEditor.getCommandString();
	      if (!cmd.isEmpty())
		processCommand(i, cmd);	
	      	      
	      updateGL();
	    }
	}
    }
  
  return true;
}

void
PathGroups::makePathConnections()
{
  for(int i=0; i<m_paths.count(); i++)
    m_paths[i]->disconnect();

  for(int i=0; i<m_paths.count(); i++)
    {
      connect(m_paths[i], SIGNAL(selectForEditing(int, int)),
	      this, SLOT(selectForEditing(int, int)));

      connect(m_paths[i], SIGNAL(deselectForEditing()),
	      this, SLOT(deselectForEditing()));
    }
}
void
PathGroups::deselectForEditing()
{
  for(int i=0; i<m_paths.count(); i++)
    m_paths[i]->setPointPressed(-1);
}

void
PathGroups::selectForEditing(int mouseButton,
			     int p0)
{
  int idx = -1;
  for(int i=0; i<m_paths.count(); i++)
    {
      if (m_paths[i]->grabsMouse())
	{
	  idx = i;
	  break;
	}
    }
  if (idx == -1)
    return;

  m_paths[idx]->setPointPressed(m_paths[idx]->pointPressed());
}

void
PathGroups::processCommand(int idx, QString cmd)
{
  bool ok;
  cmd = cmd.toLower();
  QStringList list = cmd.split(" ", QString::SkipEmptyParts);
  
  int li = 0;
  while (li < list.count())
    {
      if (list[li] == "disableundo")
	m_paths[idx]->disableUndo(true);
      else if (list[li] == "enableundo")
	m_paths[idx]->disableUndo(false);
      else if (list[li] == "normalize" ||
	  list[li] == "normalise")
	{
	  m_paths[idx]->normalize();
	}
      else if (list[li] == "setscale" || list[li] == "scale")
	{
	  if (list.count()-li > 1)
	    {
	      if (list.count()-li > 2)
		{
		  float minscl = list[li+1].toFloat(&ok);
		  li++;
		  float maxscl = list[li+1].toFloat(&ok);
		  li++;
		  m_paths[idx]->setMinScale(minscl);
		  m_paths[idx]->setMaxScale(maxscl);
		}
	      else
		{
		  float scl = list[li+1].toFloat(&ok);
		  li++;
		  m_paths[idx]->setMinScale(scl);
		  m_paths[idx]->setMaxScale(scl);
		}
	    }
	  else
	    QMessageBox::information(0, "PathGroup Command Error",
				     "No scale given");
	}
      else if (list[li] == "setradius" || list[li] == "radius")
	{
	  if (list.count()-li > 1)
	    {
	      float rad = list[li+1].toFloat(&ok);
	      m_paths[idx]->setRadX(0, rad, true);
	      m_paths[idx]->setRadY(0, rad, true);
	      li++;
	    }
	  else
	    QMessageBox::information(0, "PathGroup Command Error",
				     "No radius given");
	}
      else if (list[li] == "setrads" || list[li] == "rads")
	{
	  if (list.count()-li > 1)
	    {
	      float radx = list[li+1].toFloat(&ok);
	      m_paths[idx]->setRadX(0, radx, true);
	      li++;
	    }
	  else
	    QMessageBox::information(0, "PathGroup Command Error",
				     "No S radius given");
	}
      else if (list[li] == "setradt" || list[li] == "radt")
	{
	  if (list.count()-li > 1)
	    {
	      float rady = list[li+1].toFloat(&ok);
	      m_paths[idx]->setRadY(0, rady, true);
	      li++;
	    }
	  else
	    QMessageBox::information(0, "PathGroup Command Error",
				     "No T radius given");
	}
      else if (list[li] == "setangle" || list[li] == "angle")
	{
	  if (list.count()-li > 1)
	    {
	      float a = list[li+1].toFloat(&ok);
	      m_paths[idx]->setAngle(0, a, true);
	      li++;
	    }
	  else
	    QMessageBox::information(0, "PathGroup Command Error",
				     "No angle given");
	}
      else
	QMessageBox::information(0, "PathGroup Command Error",
				 QString("Cannot understand the command : ") +
				 cmd);

      li++;
    }
}
