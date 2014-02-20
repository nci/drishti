#include "global.h"
#include "gilights.h"
#include "staticfunctions.h"
#include "propertyeditor.h"

#include <QFileDialog>

int GiLights::count() { return m_giLights.count(); }

GiLights::GiLights()
{
  m_sameForAll = true;
  m_giLights.clear();
}

GiLights::~GiLights()
{
  clear();
}
void 
GiLights::clear()
{
  for(int i=0; i<m_giLights.count(); i++)
    m_giLights[i]->disconnect();

  for(int i=0; i<m_giLights.count(); i++)
    m_giLights[i]->removeFromMouseGrabberPool();

  for(int i=0; i<m_giLights.count(); i++)
    delete m_giLights[i];

  m_giLights.clear();
}

bool
GiLights::isInMouseGrabberPool(int i)
{
  if (i < m_giLights.count())
    return m_giLights[i]->isInMouseGrabberPool();
  else
    return false;
}
void
GiLights::addInMouseGrabberPool(int i)
{
  if (i < m_giLights.count())
    m_giLights[i]->addInMouseGrabberPool();
}
void
GiLights::addInMouseGrabberPool()
{
  for(int i=0; i<m_giLights.count(); i++)
    m_giLights[i]->addInMouseGrabberPool();
}
void
GiLights::removeFromMouseGrabberPool(int i)
{
  if (i < m_giLights.count())
    m_giLights[i]->removeFromMouseGrabberPool();
}

void
GiLights::removeFromMouseGrabberPool()
{
  for(int i=0; i<m_giLights.count(); i++)
    m_giLights[i]->removeFromMouseGrabberPool();
}


bool
GiLights::grabsMouse()
{
  for(int i=0; i<m_giLights.count(); i++)
    {
      if (m_giLights[i]->grabsMouse())
	return true;
    }
  return false;
}


void
GiLights::addGiPointLight(QList<Vec> pts)
{
  GiLightGrabber *pg = new GiLightGrabber();
  pg->setPoints(pts);
  m_giLights.append(pg);

  makeGiLightConnections();
}
void
GiLights::addGiDirectionLight(QList<Vec> pts)
{
  GiLightGrabber *pg = new GiLightGrabber();
  pg->setPoints(pts);
  pg->setLightType(1);
  m_giLights.append(pg);

  makeGiLightConnections();
}

void
GiLights::addGiLight(GiLightObject po)
{
  GiLightGrabber *pg = new GiLightGrabber();
  pg->set(po);
  m_giLights.append(pg);

  makeGiLightConnections();
}

void
GiLights::addGiLight(QString flnm)
{
  QFile fpath(flnm);
  fpath.open(QFile::ReadOnly);
  QTextStream fd(&fpath);

  QString line = fd.readLine();
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
	    {
	      GiLightGrabber *pg = new GiLightGrabber();
	      pg->setPoints(pts);
	      m_giLights.append(pg);
	    }	  
	}
      line = fd.readLine();
    }

  makeGiLightConnections();
}

QList<GiLightGrabber*> GiLights::giLightsPtr() { return m_giLights; }

QList<GiLightObject>
GiLights::giLights()
{
  QList<GiLightObject> po;
  for(int i=0; i<m_giLights.count(); i++)
    po.append(m_giLights[i]->get());

  return po;
}

void
GiLights::setGiLights(QList<GiLightObject> po)
{
  clear();

  for(int i=0; i<po.count(); i++)
    {
      GiLightGrabber *pg = new GiLightGrabber();
      pg->set(po[i]);
      m_giLights.append(pg);
    }

  makeGiLightConnections();
}

void
GiLights::updateScaling()
{
  for(int i=0; i<m_giLights.count();i++)
    m_giLights[i]->computePathLength();
}

void
GiLights::draw(QGLViewer *viewer, bool backToFront)
{
  for(int i=0; i<m_giLights.count();i++)
    m_giLights[i]->draw(viewer,
			m_giLights[i]->grabsMouse(),
			backToFront);
}

void
GiLights::postdraw(QGLViewer *viewer)
{
  for(int i=0; i<m_giLights.count();i++)
    {
      int x,y;
      m_giLights[i]->mousePosition(x,y);
      m_giLights[i]->postdraw(viewer,
			   x, y,
			   m_giLights[i]->grabsMouse());
    }
}

bool
GiLights::keyPressEvent(QKeyEvent *event)
{
  for(int i=0; i<m_giLights.count(); i++)
    {
      if (m_giLights[i]->grabsMouse())
	{
	  if (event->key() == Qt::Key_G)
	    {
	      m_giLights[i]->removeFromMouseGrabberPool();
	      return true;
	    }
	  else if (event->key() == Qt::Key_V)
	    {
	      setShow(i, false);
	      return true;
	    }
	  else if (event->key() == Qt::Key_P)
	    {
	      bool b = m_giLights[i]->showPoints();
	      m_giLights[i]->setShowPoints(!b);    
	      return true;
	    }
	  else if (event->key() == Qt::Key_N)
	    {
	      bool b = m_giLights[i]->showPointNumbers();
	      m_giLights[i]->setShowPointNumbers(!b);    
	      return true;
	    }
	  else if (event->key() == Qt::Key_X)
	    {
	      m_giLights[i]->setMoveAxis(GiLightGrabber::MoveX);
	      return true;
	    }
	  else if (event->key() == Qt::Key_Y)
	    {
	      if (event->modifiers() & Qt::ControlModifier ||
		  event->modifiers() & Qt::MetaModifier)
		m_giLights[i]->redo();
	      else
		m_giLights[i]->setMoveAxis(GiLightGrabber::MoveY);
	      return true;
	    }
	  else if (event->key() == Qt::Key_Z)
	    {
	      if (event->modifiers() & Qt::ControlModifier ||
		  event->modifiers() & Qt::MetaModifier)
		m_giLights[i]->undo();
	      else
		m_giLights[i]->setMoveAxis(GiLightGrabber::MoveZ);
	      return true;
	    }
	  else if (event->key() == Qt::Key_W)
	    {
	      m_giLights[i]->setMoveAxis(GiLightGrabber::MoveAll);
	      return true;
	    }
	  else if (event->key() == Qt::Key_Delete ||
		   event->key() == Qt::Key_Backspace ||
		   event->key() == Qt::Key_Backtab)
	    {
	      m_giLights[i]->removeFromMouseGrabberPool();
	      m_giLights.removeAt(i);
	      return true;
	    }
	  
	  if (event->key() == Qt::Key_Space)
	    return openPropertyEditor(i);
	}
    }
  
  return true;
}

bool
GiLights::openPropertyEditor(int i)
{
  PropertyEditor propertyEditor;
  QMap<QString, QVariantList> plist;
  
  QVariantList vlist;
  
  vlist.clear();
  vlist << QVariant("combobox");
  vlist << QVariant(m_giLights[i]->lightType());
  vlist << QVariant("point");
  vlist << QVariant("direction");
  plist["light type"] = vlist;

  vlist.clear();
  vlist << QVariant("color");
  Vec pcolor = m_giLights[i]->color();
  QColor dcolor = QColor::fromRgbF(pcolor.x,
				   pcolor.y,
				   pcolor.z);
  vlist << dcolor;
  plist["color"] = vlist;
  
//  vlist.clear();
//  vlist << QVariant("double");
//  vlist << QVariant(m_giLights[i]->opacity());
//  vlist << QVariant(0.0);
//  vlist << QVariant(2.0);
//  vlist << QVariant(0.1); // singlestep
//  vlist << QVariant(1); // decimals
//  plist["intensity"] = vlist;
  
  vlist.clear();
  vlist << QVariant("int");
  vlist << QVariant(m_giLights[i]->lod());
  vlist << QVariant(1);
  vlist << QVariant(10);
  plist["light buffer size"] = vlist;
  
  vlist.clear();
  vlist << QVariant("int");
  vlist << QVariant(m_giLights[i]->smooth());
  vlist << QVariant(0);
  vlist << QVariant(10);
  plist["light buffer smoothing"] = vlist;
  
  vlist.clear();
  vlist << QVariant("double");
  vlist << QVariant(m_giLights[i]->angle());
  vlist << QVariant(10.0);
  vlist << QVariant(80.0);
  vlist << QVariant(10.0);// singlestep
  vlist << QVariant(1); // decimals
  plist["collection angle"] = vlist;
      
  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(m_giLights[i]->allowInterpolate());
  plist["interpolate"] = vlist;
  
  if (m_giLights[i]->lightType() == 0) // point light
    {
      vlist.clear();
      vlist << QVariant("int");
      vlist << QVariant(m_giLights[i]->rad());
      vlist << QVariant(1);
      vlist << QVariant(50);
      plist["size"] = vlist;

      vlist.clear();
      vlist << QVariant("double");
      vlist << QVariant(m_giLights[i]->decay());
      vlist << QVariant(0.1);
      vlist << QVariant(1.0);
      vlist << QVariant(0.1);// singlestep
      vlist << QVariant(3); // decimals
      plist["falloff"] = vlist;

      vlist.clear();
      vlist << QVariant("int");
      vlist << QVariant(m_giLights[i]->segments());
      vlist << QVariant(1);
      vlist << QVariant(100);
      plist["smoothness"] = vlist;

      vlist.clear();
      vlist << QVariant("checkbox");
      vlist << QVariant(m_giLights[i]->doShadows());
      plist["do shadows"] = vlist;

    }

  vlist.clear();
  plist["command"] = vlist;
  
  
  vlist.clear();
  QFile helpFile(":/gilights.help");
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
  keys << "light type";
  keys << "light buffer size";
  keys << "light buffer smoothing";
  keys << "color";
  keys << "collection angle";
  keys << "gap";
  if (m_giLights[i]->lightType() == 0) // point light
    {
      keys << "size";
      keys << "falloff";
      keys << "smoothness";
      keys << "gap";
      keys << "do shadows";
      keys << "gap";
    }
  keys << "interpolate";
  keys << "command";
  //keys << "commandhelp";
  
  
  propertyEditor.set("GI Light Parameters", plist, keys);
  propertyEditor.resize(400, 500);  
  
  
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
	      m_giLights[i]->setColor(pcolor);
	    }
	  //else if (keys[ik] == "intensity")
	  //  m_giLights[i]->setOpacity(pair.first.toDouble());
	  else if (keys[ik] == "light buffer size")
	    m_giLights[i]->setLod(pair.first.toInt());
	  else if (keys[ik] == "light buffer smoothing")
	    m_giLights[i]->setSmooth(pair.first.toInt());
	  else if (keys[ik] == "smoothness")
	    m_giLights[i]->setSegments(pair.first.toInt());
	  else if (keys[ik] == "interpolate")
	    m_giLights[i]->setAllowInterpolate(pair.first.toBool());
	  else if (keys[ik] == "do shadows")
	    m_giLights[i]->setDoShadows(pair.first.toBool());
	  else if (keys[ik] == "light type")
	    m_giLights[i]->setLightType(pair.first.toInt());
	  else if (keys[ik] == "size")
	    m_giLights[i]->setRad(pair.first.toInt());
	  else if (keys[ik] == "falloff")
	    m_giLights[i]->setDecay(pair.first.toFloat());
	  else if (keys[ik] == "collection angle")
	    m_giLights[i]->setAngle(pair.first.toFloat());
	}
    }
  
  QString cmd = propertyEditor.getCommandString();
  if (!cmd.isEmpty())
    processCommand(i, cmd);	
  
  emit updateGL();
  return true;
}

void
GiLights::makeGiLightConnections()
{
  for(int i=0; i<m_giLights.count(); i++)
    m_giLights[i]->disconnect();

  for(int i=0; i<m_giLights.count(); i++)
    {
      connect(m_giLights[i], SIGNAL(selectForEditing(int, int)),
	      this, SLOT(selectForEditing(int, int)));

      connect(m_giLights[i], SIGNAL(deselectForEditing()),
	      this, SLOT(deselectForEditing()));
    }
}
void
GiLights::deselectForEditing()
{
  for(int i=0; i<m_giLights.count(); i++)
    m_giLights[i]->setPointPressed(-1);

  emit updateLightBuffers();
}

void
GiLights::selectForEditing(int mouseButton,
			int p0)
{
  int idx = -1;
  for(int i=0; i<m_giLights.count(); i++)
    {
      if (m_giLights[i]->grabsMouse())
	{
	  idx = i;
	  break;
	}
    }
  if (idx == -1)
    return;

  if (mouseButton == Qt::RightButton)
    {
      if (m_giLights[idx]->pointPressed() == -1)
	  emit showMessage("No point selected for removal", true);
      else
	m_giLights[idx]->removePoint(m_giLights[idx]->pointPressed());
    }
  else if (mouseButton == Qt::LeftButton && p0 > -1)
    m_giLights[idx]->insertPointAfter(p0);
//  else
//    m_giLights[idx]->setPointPressed(m_giLights[idx]->pointPressed());
}

void
GiLights::processCommand(int idx, QString cmd)
{
  bool ok;
  QString origCmd = cmd;
  cmd = cmd.toLower();
  QStringList list = cmd.split(" ", QString::SkipEmptyParts);
  
  int li = 0;
  while (li < list.count())
    {
      if (list[li] == "planar")
	{
	  if (list.count()-li > 3)
	    {
	      int v0 = list[li+1].toInt(&ok);
	      int v1 = list[li+2].toInt(&ok);
	      int v2 = list[li+3].toInt(&ok);
	      m_giLights[idx]->makePlanar(v0,v1,v2);
	      li += 3;
	    }
	  else
	    m_giLights[idx]->makePlanar();
	}
      else if (list[li] == "circle")
	{
	  m_giLights[idx]->makeCircle();
	}
      else if (list[li] == "normalize" ||
	  list[li] == "normalise")
	{
	  m_giLights[idx]->normalize();
	}
      else if (list[li] == "moves")
	{
	  if (list.size()-li > 1)
	    {
	      if (list[li+1] == "-")
		m_giLights[idx]->translate(true, false);
	      else
		m_giLights[idx]->translate(true, true);

	      li++;
	    }
	  else
	    m_giLights[idx]->translate(true, true);
	}
      else if (list[li] == "movet")
	{
	  if (list.size()-li > 1)
	    {
	      if (list[li+1] == "-")
		m_giLights[idx]->translate(false, false);
	      else
		m_giLights[idx]->translate(false, true);

	      li++;
	    }
	  else
	    m_giLights[idx]->translate(false, true);
	}
      else if (list[li] == "save")
	{
	  QString flnm;
	  flnm = QFileDialog::getSaveFileName(0,
					      "Save point lights to text file",
					      Global::previousDirectory(),
					      "Files (*.*)");
	  
	  if (flnm.isEmpty())
	    return;

	  QList<Vec> pts = m_giLights[idx]->points();

	  QFile fpath(flnm);
	  fpath.open(QFile::WriteOnly | QFile::Text);
	  QTextStream fd(&fpath);
	  fd << pts.count() << "\n";
	  for(int pi=0; pi < pts.count(); pi++)
	    fd << pts[pi].x << " " << pts[pi].y << " " << pts[pi].z << "\n";
	  
	  fd.flush();
	}
      else
	QMessageBox::information(0, "GI Light Command Error",
				 QString("Cannot understand the command : ") +
				 cmd);

      li++;
    }
}

GiLightGrabber*
GiLights::checkIfGrabsMouse(int x, int y, Camera *cam)
{
  for(int i=0; i<m_giLights.count(); i++)
    {
      if (m_giLights[i]->isInMouseGrabberPool())
	{
	  m_giLights[i]->checkIfGrabsMouse(x, y, cam);
	  if (m_giLights[i]->grabsMouse())
	    return m_giLights[i];
	}
    }
  return NULL;
}
void
GiLights::mousePressEvent(QMouseEvent *e, Camera *c)
{
  for(int i=0; i<m_giLights.count(); i++)
    m_giLights[i]->mousePressEvent(e,c);
}
void
GiLights::mouseMoveEvent(QMouseEvent *e, Camera *c)
{
  for(int i=0; i<m_giLights.count(); i++)
    m_giLights[i]->mouseMoveEvent(e,c);
}
void
GiLights::mouseReleaseEvent(QMouseEvent *e, Camera *c)
{
  for(int i=0; i<m_giLights.count(); i++)
    m_giLights[i]->mouseReleaseEvent(e,c);
}
void
GiLights::show()
{
  for(int i=0; i<m_giLights.count(); i++)
    {
      m_giLights[i]->setShow(true);
      m_giLights[i]->addInMouseGrabberPool();
    }
}
void
GiLights::hide()
{
  for(int i=0; i<m_giLights.count(); i++)
    {
      m_giLights[i]->setShow(false);
      m_giLights[i]->removeFromMouseGrabberPool();
    }
}
bool
GiLights::show(int i)
{
  if (i >= 0 && i < m_giLights.count())
    return m_giLights[i]->show();
  else
    return false;
}

void
GiLights::setShow(int i, bool flag)
{
  if (i == -1)
    {
      for(int l=0; l<m_giLights.count(); l++)
	{
	  m_giLights[l]->setShow(flag);
	  if (flag)
	    m_giLights[l]->addInMouseGrabberPool();
	  else
	    m_giLights[l]->removeFromMouseGrabberPool();
	}
    }
  else if (i >= 0 && i < m_giLights.count())
    {
      m_giLights[i]->setShow(flag);
      if (flag)
	m_giLights[i]->addInMouseGrabberPool();
      else
	m_giLights[i]->removeFromMouseGrabberPool();
    }
}

bool
GiLights::setGiLightObjectInfo(QList<GiLightObjectInfo> gloInfo)
{
  bool lightsChanged = false;
  if (gloInfo.count() != m_giLights.count()) lightsChanged = true;

  for(int i=0; i<qMin(gloInfo.count(), m_giLights.count()); i++)
    {
      if (m_giLights[i]->checkIfNotEqual(gloInfo[i]))
	{
	  lightsChanged = true;
	  break;
	}
    }

  clear();
  for(int i=0; i<gloInfo.count(); i++)
    {
      GiLightGrabber *pg = new GiLightGrabber();
      pg->setGiLightObjectInfo(gloInfo[i]);
      m_giLights.append(pg);
    }

  return lightsChanged;  
}

QList<GiLightObjectInfo>
GiLights::giLightObjectInfo()
{
  QList<GiLightObjectInfo> glo;
  for(int i=0; i<m_giLights.count(); i++)
    glo << m_giLights[i]->giLightObjectInfo();

  return glo;
}
