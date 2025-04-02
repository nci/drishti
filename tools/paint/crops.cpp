#include "global.h"
#include "crops.h"
#include "staticfunctions.h"
#include "propertyeditor.h"

#include <QFileDialog>

int Crops::count() { return m_crops.count(); }

bool
Crops::checkCrop(Vec po)
{
  bool cropped = false;
  for(int i=0; i<m_crops.count(); i++)
    {
      if (m_crops[i]->cropType() < CropObject::Tear_Tear)
	{
	  // take union
	  cropped |= (!m_crops[i]->checkCropped(po));
	}
    }
  return cropped;
}

void
Crops::show()
{
  for(int i=0; i<m_crops.count(); i++)
    {
      m_crops[i]->setShow(true);
      m_crops[i]->addInMouseGrabberPool();
    }
}

void
Crops::hide()
{
  for(int i=0; i<m_crops.count(); i++)
    {
      m_crops[i]->setShow(false);
      m_crops[i]->removeFromMouseGrabberPool();
    }
}

bool
Crops::show(int i)
{
  if (i >= 0 && i < m_crops.count())
    return m_crops[i]->show();
  else
    return false;
}

void
Crops::setShow(int i, bool flag)
{
  if (i >= 0 && i < m_crops.count())
    {
      m_crops[i]->setShow(flag);
      if (flag)
	m_crops[i]->addInMouseGrabberPool();
      else
	m_crops[i]->removeFromMouseGrabberPool();
    }
}

bool
Crops::tearPresent()
{
  for(int i=0; i<m_crops.count(); i++)
    if (m_crops[i]->cropType() >= CropObject::Tear_Tear &&
	m_crops[i]->cropType() < CropObject::View_Tear)
      return true;

  return false;
}

bool
Crops::viewPresent()
{
  for(int i=0; i<m_crops.count(); i++)
    if (m_crops[i]->cropType() >= CropObject::View_Tear &&
	m_crops[i]->cropType() < CropObject::Glow_Ball)
      return true;

  return false;
}

bool
Crops::glowPresent()
{
  for(int i=0; i<m_crops.count(); i++)
    if (m_crops[i]->cropType() >= CropObject::Glow_Ball)
      return true;

  return false;
}

bool
Crops::cropPresent()
{
  for(int i=0; i<m_crops.count(); i++)
    if (m_crops[i]->cropType() < CropObject::Tear_Tear)
      return true;

  return false;
}


Crops::Crops()
{
  m_sameForAll = true;
  m_crops.clear();
}

Crops::~Crops()
{
  clear();
}
void 
Crops::clear()
{
  for(int i=0; i<m_crops.count(); i++)
    m_crops[i]->disconnect();

  for(int i=0; i<m_crops.count(); i++)
    m_crops[i]->removeFromMouseGrabberPool();

  for(int i=0; i<m_crops.count(); i++)
    delete m_crops[i];

  m_crops.clear();
}

bool
Crops::isInMouseGrabberPool(int i)
{
  if (i < m_crops.count())
    return m_crops[i]->isInMouseGrabberPool();
  else
    return false;
}
void
Crops::addInMouseGrabberPool(int i)
{
  if (i < m_crops.count())
    m_crops[i]->addInMouseGrabberPool();
}
void
Crops::addInMouseGrabberPool()
{
  for(int i=0; i<m_crops.count(); i++)
    m_crops[i]->addInMouseGrabberPool();
}
void
Crops::removeFromMouseGrabberPool(int i)
{
  if (i < m_crops.count())
    m_crops[i]->removeFromMouseGrabberPool();
}

void
Crops::removeFromMouseGrabberPool()
{
  for(int i=0; i<m_crops.count(); i++)
    m_crops[i]->removeFromMouseGrabberPool();
}


bool
Crops::grabsMouse()
{
  for(int i=0; i<m_crops.count(); i++)
    {
      if (m_crops[i]->grabsMouse())
	return true;
    }
  return false;
}


void
Crops::addCrop(QList<Vec> pts)
{
  CropGrabber *pg = new CropGrabber();
  pg->setPoints(pts);
  pg->setCropType(CropObject::Crop_Box);
  m_crops.append(pg);

  makeCropConnections();
}

void
Crops::addDisplace(QList<Vec> pts)
{
  CropGrabber *pg = new CropGrabber();
  pg->setPoints(pts);
  pg->setCropType(CropObject::Displace_Displace);
  m_crops.append(pg);

  makeCropConnections();
}

void
Crops::addGlow(QList<Vec> pts)
{
  CropGrabber *pg = new CropGrabber();
  pg->setPoints(pts);
  pg->setOpacity(0.3);
  pg->setCropType(CropObject::Glow_Ball);
  m_crops.append(pg);

  makeCropConnections();
}

void
Crops::addView(QList<Vec> pts)
{
  CropGrabber *pg = new CropGrabber();
  pg->setPoints(pts);
  pg->setCropType(CropObject::View_Block);
  m_crops.append(pg);

  makeCropConnections();
}

void
Crops::addTear(QList<Vec> pts)
{
  CropGrabber *pg = new CropGrabber();
  pg->setPoints(pts);
  pg->setCropType(CropObject::Tear_Tear);
  m_crops.append(pg);

  makeCropConnections();
}

void
Crops::addCrop(CropObject po)
{
  CropGrabber *pg = new CropGrabber();
  pg->set(po);
  m_crops.append(pg);

  makeCropConnections();
}

void
Crops::addCrop(QString flnm)
{
  QList<Vec> pts;
  QFile fcrop(flnm);
  fcrop.open(QFile::ReadOnly);
  QTextStream fd(&fcrop);
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
	  break;
	}
    }

  if (pts.count() > 0)
    {
      CropGrabber *pg = new CropGrabber();
      pg->setPoints(pts);
      m_crops.append(pg);

      makeCropConnections();
    }
}

QList<CropObject>
Crops::crops()
{
  QList<CropObject> po;
  for(int i=0; i<m_crops.count(); i++)
    po.append(m_crops[i]->get());

  return po;
}

void
Crops::setCrops(QList<CropObject> po)
{
  clear();

  for(int i=0; i<po.count(); i++)
    {
      CropGrabber *pg = new CropGrabber();
      pg->set(po[i]);
      m_crops.append(pg);
    }

  makeCropConnections();
}

void
Crops::updateScaling()
{
  for(int i=0; i<m_crops.count();i++)
    m_crops[i]->computeCropLength();
}

bool
Crops::updated()
{
  for(int i=0; i<m_crops.count();i++)
    if (m_crops[i]->updated())
      return true;
  return false;
}

void
Crops::draw(QGLViewer *viewer, bool backToFront)
{
  for(int i=0; i<m_crops.count();i++)
    m_crops[i]->draw(viewer,
		     m_crops[i]->grabsMouse(),
		     backToFront);
}

void
Crops::postdraw(QGLViewer *viewer)
{
  for(int i=0; i<m_crops.count();i++)
    {
      int x,y;
      m_crops[i]->mousePosition(x,y);
      m_crops[i]->postdraw(viewer,
			   x, y,
			   m_crops[i]->grabsMouse());
    }
}

bool
Crops::keyPressEvent(QKeyEvent *event)
{
  for(int i=0; i<m_crops.count(); i++)
    {
      if (m_crops[i]->grabsMouse())
	{
	  if (event->key() == Qt::Key_Up || 
	      event->key() == Qt::Key_Down ||
	      event->key() == Qt::Key_Left || 
	      event->key() == Qt::Key_Right)
	    {
	      int shift = -1;
	      if (event->key() == Qt::Key_Up ||
		  event->key() == Qt::Key_Right)
		shift = 1;

	      if (event->modifiers() & Qt::ShiftModifier)
		shift *= 10;

	      if (event->key() == Qt::Key_Left || 
		  event->key() == Qt::Key_Right)
		{
		  if (m_crops[i]->moveAxis() < CropObject::MoveY0)
		    m_crops[i]->rotate(m_crops[i]->m_xaxis, shift);
		  else if (m_crops[i]->moveAxis() < CropObject::MoveZ)
		    m_crops[i]->rotate(m_crops[i]->m_yaxis, shift);
		  else
		    m_crops[i]->setAngle(m_crops[i]->getAngle() + shift);

		  m_crops[i]->updateUndo();
		}
	      else
		{
		  if (m_crops[i]->moveAxis() < CropObject::MoveY0)
		    {
		      QList<float> radx = m_crops[i]->radX();
		      float rad = shift*(radx[0]+radx[1])/10;
		      m_crops[i]->translate(rad*m_crops[i]->m_xaxis);
		    }
		  else if (m_crops[i]->moveAxis() < CropObject::MoveZ)
		    {
		      QList<float> rady = m_crops[i]->radY();
		      float rad = shift*(rady[0]+rady[1])/10;
		      m_crops[i]->translate(rad*m_crops[i]->m_yaxis);
		    }
		  else
		    {
		      QList<Vec> pts = m_crops[i]->points();
		      float rad = shift*(pts[0]-pts[1]).norm()/10;
		      m_crops[i]->translate(rad*m_crops[i]->m_tang);
		    }

		  m_crops[i]->updateUndo();
		}
	      return true;
	    }
	  else if (event->key() == Qt::Key_G)
	    {
	      m_crops[i]->removeFromMouseGrabberPool();
	      return true;
	    }
	  else if (event->key() == Qt::Key_V)
	    {
	      setShow(i, false);
	      return true;
	    }
	  else if (event->key() == Qt::Key_P)
	    {
	      bool b = m_crops[i]->showPoints();
	      m_crops[i]->setShowPoints(!b);    
	      return true;
	    }
	  else if (event->key() == Qt::Key_X)
	    {
	      m_crops[i]->setMoveAxis(CropGrabber::MoveX0);
	      return true;
	    }
	  else if (event->key() == Qt::Key_Y)
	    {
	      if (event->modifiers() & Qt::ControlModifier ||
		  event->modifiers() & Qt::MetaModifier)
		m_crops[i]->redo();
	      else
		m_crops[i]->setMoveAxis(CropGrabber::MoveY0);
	      return true;
	    }
	  else if (event->key() == Qt::Key_Z)
	    {
	      if (event->modifiers() & Qt::ControlModifier ||
		  event->modifiers() & Qt::MetaModifier)
		m_crops[i]->undo();
	      else
		m_crops[i]->setMoveAxis(CropGrabber::MoveZ);
	      return true;
	    }
	  else if (event->key() == Qt::Key_F)
	    {
	      m_crops[i]->flipPoints();
	      return true;
	    }
	  else if (event->key() == Qt::Key_S)
	    {
	      int idx = m_crops[i]->pointPressed();
	      if (idx > -1)
		{
		  float radx = m_crops[i]->getRadX(idx);
		  if (event->modifiers() & Qt::ShiftModifier)
		    radx--;
		  else
		    radx++;
		  radx = qMax(1.0f, radx);
		  m_crops[i]->setRadX(idx, radx, false);

		  m_crops[i]->updateUndo();
		  return true;
		}
	    }
	  else if (event->key() == Qt::Key_T)
	    {
	      int idx = m_crops[i]->pointPressed();
	      if (idx > -1)
		{
		  float rady = m_crops[i]->getRadY(idx);
		  if (event->modifiers() & Qt::ShiftModifier)
		    rady--;
		  else
		    rady++;
		  rady = qMax(1.0f, rady);
		  m_crops[i]->setRadY(idx, rady, false);

		  m_crops[i]->updateUndo();
		}
	      else // switch to tube mode
		{
		  bool b = m_crops[i]->tube();
		  m_crops[i]->setTube(!b);
		}

	      return true;
	    }
	  else if (event->key() == Qt::Key_A)
	    {
	      int idx = m_crops[i]->pointPressed();
	      if (idx > -1)
		{
		  float a = m_crops[i]->getAngle();
		  if (event->modifiers() & Qt::ShiftModifier)
		    a--;
		  else
		    a++;
		  m_crops[i]->setAngle(a);
		  return true;
		}
	    }
	  else if (event->key() == Qt::Key_L)
	    {
	      int idx = m_crops[i]->pointPressed();
	      if (idx > -1)
		{
		  int lift = m_crops[i]->getLift(idx);
		  if (event->modifiers() & Qt::ShiftModifier)
		    lift--;
		  else
		    lift++;
		  m_crops[i]->setLift(idx, lift, false);
		  return true;
		}
	    }
	  else if (event->key() == Qt::Key_Delete ||
		   event->key() == Qt::Key_Backspace ||
		   event->key() == Qt::Key_Backtab)
	    {
	      m_crops[i]->removeFromMouseGrabberPool();
	      m_crops.removeAt(i);
	      emit updateShaders();
	      return true;
	    }
	  else if (event->key() == Qt::Key_Space)
	    {
	      PropertyEditor propertyEditor;
	      QMap<QString, QVariantList> plist;

	      QVariantList vlist;

	      vlist.clear();
	      if (m_crops[i]->cropType() != CropObject::Displace_Displace)
		{
		  vlist << QVariant("combobox");
		  if (m_crops[i]->cropType() >= CropObject::Glow_Ball)
		    {
		      vlist << QVariant(m_crops[i]->cropType() - CropObject::Glow_Ball);
		      vlist << QVariant("Glow ball");
		      vlist << QVariant("Glow block");
		      vlist << QVariant("Glow tube");
		    }
		  else if (m_crops[i]->cropType() >= CropObject::View_Tear)
		    {
		      vlist << QVariant(m_crops[i]->cropType() - CropObject::View_Tear);
		      vlist << QVariant("Blend eye");
		      vlist << QVariant("Blend tube");
		      vlist << QVariant("Blend ball");
		      vlist << QVariant("Blend block");
		    }
		  else if (m_crops[i]->cropType() >= CropObject::Tear_Tear)
		    {
		      vlist << QVariant(m_crops[i]->cropType() - CropObject::Tear_Tear);
		      vlist << QVariant("Dissect eye");
		      vlist << QVariant("Dissect hole");
		      vlist << QVariant("Dissect wedge");
		      //vlist << QVariant("Dissect curl");
		    }
		  else
		    {
		      vlist << QVariant(m_crops[i]->cropType());
		      vlist << QVariant("Crop tube");
		      vlist << QVariant("Crop box");
		      vlist << QVariant("Crop ellipsoid");
		    }
		  plist["style"] = vlist;
		}
	      else
		{
		  QString str;
		  Vec tran = m_crops[i]->dtranslate();
		  Vec pivot = m_crops[i]->dpivot();
		  Vec axis = m_crops[i]->drotaxis();
		  float angle = m_crops[i]->drotangle();

		  vlist.clear();
		  vlist << QVariant("string");
		  str = QString("%1 %2 %3").arg(tran.x).arg(tran.y).arg(tran.z);
		  vlist << QVariant(str);
		  plist["translate"] = vlist;

		  vlist.clear();
		  vlist << QVariant("string");
		  str = QString("%1 %2 %3").arg(pivot.x).arg(pivot.y).arg(pivot.z);
		  vlist << QVariant(str);
		  plist["pivot"] = vlist;

		  vlist.clear();
		  vlist << QVariant("string");
		  str = QString("%1 %2 %3").arg(axis.x).arg(axis.y).arg(axis.z);
		  vlist << QVariant(str);
		  plist["axis"] = vlist;

		  vlist.clear();
		  vlist << QVariant("string");
		  str = QString("%1").arg(angle);
		  vlist << QVariant(str);
		  plist["angle"] = vlist;
		}

	      if (m_crops[i]->cropType() >= CropObject::Glow_Ball)
		{
		  vlist.clear();
		  vlist << QVariant("double");
		  vlist << QVariant(m_crops[i]->opacity());
		  vlist << QVariant(0.0);
		  vlist << QVariant(1.0);
		  vlist << QVariant(0.1); // singlestep
		  vlist << QVariant(1); // decimals
		  plist["opacity"] = vlist;

		  vlist.clear();
		  vlist << QVariant("color");
		  Vec pcolor = m_crops[i]->color();
		  QColor dcolor = QColor::fromRgbF(pcolor.x,
						   pcolor.y,
						   pcolor.z);
		  vlist << dcolor;
		  plist["color"] = vlist;

		  vlist.clear();
		  vlist << QVariant("double");
		  vlist << QVariant(m_crops[i]->viewMix());
		  vlist << QVariant(0.0);
		  vlist << QVariant(1.0);
		  vlist << QVariant(0.1); // singlestep
		  vlist << QVariant(1); // decimals
		  plist["mix"] = vlist;
		}
	      else if (m_crops[i]->cropType() >= CropObject::View_Tear)
		{
		  vlist.clear();
		  vlist << QVariant("checkbox");
		  vlist << QVariant(m_crops[i]->clearView());
		  plist["clearview"] = vlist;

		  vlist.clear();
		  vlist << QVariant("double");
		  vlist << QVariant(m_crops[i]->magnify());
		  vlist << QVariant(1.0);
		  vlist << QVariant(3.0);
		  vlist << QVariant(0.1); // singlestep
		  vlist << QVariant(1); // decimals
		  plist["magnify"] = vlist;

		  vlist.clear();
		  vlist << QVariant("int");
		  vlist << QVariant(m_crops[i]->tfset());
		  vlist << QVariant(0);
		  vlist << QVariant(15);
		  plist["tfset"] = vlist;

		  vlist.clear();
		  vlist << QVariant("double");
		  vlist << QVariant(m_crops[i]->viewMix());
		  vlist << QVariant(0.0);
		  vlist << QVariant(1.0);
		  vlist << QVariant(0.1); // singlestep
		  vlist << QVariant(1); // decimals
		  plist["mix"] = vlist;

		  vlist.clear();
		  vlist << QVariant("checkbox");
		  vlist << QVariant(m_crops[i]->unionBlend());
		  plist["union"] = vlist;
		}
	      else if (m_crops[i]->cropType() < CropObject::Tear_Tear)
		{
		  vlist.clear();
		  vlist << QVariant("checkbox");
		  vlist << QVariant(m_crops[i]->keepInside());
		  plist["keep inside"] = vlist;

		  vlist.clear();
		  vlist << QVariant("checkbox");
		  vlist << QVariant(m_crops[i]->keepEnds());
		  plist["keep ends"] = vlist;

		  vlist.clear();
		  vlist << QVariant("checkbox");
		  vlist << QVariant(m_crops[i]->halfSection());
		  plist["half section"] = vlist;

		  bool hatch;
		  hatch = m_crops[i]->hatch();
		  vlist.clear();
		  vlist << QVariant("checkbox");
		  vlist << QVariant(hatch);
		  plist["hatch"] = vlist;

		  bool grid;
		  int xn, xd, yn, yd, zn, zd;
		  m_crops[i]->hatchParameters(grid, xn,xd,yn,yd,zn,zd);
		  vlist.clear();
		  vlist << QVariant("string");
		  QString str;
		  if (grid)
		    str += QString("grid %1 %2 %3 %4 %5 %6").\
		      arg(xn).arg(xd).arg(yn).arg(yd).arg(zn).arg(zd);
		  else
		    str += QString("box %1 %2 %3 %4 %5 %6").\
		      arg(xn).arg(xd).arg(yn).arg(yd).arg(zn).arg(zd);
		  vlist << QVariant(str);
		  plist["hatch parameters"] = vlist;
		}

	      vlist.clear();
	      vlist << QVariant("combobox");
	      vlist << QVariant(0);
	      vlist << QVariant("");
	      vlist << QVariant("Crop");
	      vlist << QVariant("Blend");
	      vlist << QVariant("Dissect");
	      vlist << QVariant("Glow");
	      vlist << QVariant("Displace");
	      plist["morph into"] = vlist;

	      vlist.clear();
	      plist["command"] = vlist;


	      vlist.clear();
	      QFile helpFile(":/crops.help");
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
	      QString str;
	      QList<Vec> pts = m_crops[i]->points();
	      QList<float> radx = m_crops[i]->radX();
	      QList<float> rady = m_crops[i]->radY();
	      QList<int> lift = m_crops[i]->lift();
	      str += QString("pt0 : %1 %2 %3\n").arg(pts[0].x).arg(pts[0].y).arg(pts[0].z);
	      str += QString("pt1 : %1 %2 %3\n").arg(pts[1].x).arg(pts[1].y).arg(pts[1].z);
	      str += QString("rads : %1 %2\n").arg(radx[0]).arg(radx[1]);
	      str += QString("radt : %1 %2\n").arg(rady[0]).arg(rady[1]);
	      str += QString("angle : %1\n").arg(m_crops[i]->getAngle());
	      str += QString("lift : %1 %2\n").arg(lift[0]).arg(lift[1]);
	      vlist << str;
	      plist["message"] = vlist;


	      QStringList keys;
	      if (m_crops[i]->cropType() != CropObject::Displace_Displace)
		keys << "style";
	      else
		{
		  keys << "translate";
		  keys << "pivot";
		  keys << "axis";
		  keys << "angle";
		}
	      
	      if (m_crops[i]->cropType() >= CropObject::Glow_Ball)
		{
		  keys << "gap";
		  keys << "mix";
		  keys << "gap";
		  keys << "color";
		  keys << "opacity";
		}
	      else if (m_crops[i]->cropType() >= CropObject::View_Tear)
		{
		  keys << "gap";
		  keys << "clearview";
		  keys << "magnify";
		  keys << "tfset";
		  keys << "mix";
		  keys << "gap";
		  keys << "union";
		}
	      else if (m_crops[i]->cropType() < CropObject::Tear_Tear)
		{
		  keys << "gap";
		  keys << "keep inside";
		  keys << "keep ends";
		  keys << "half section";
		  keys << "hatch";
		  keys << "hatch parameters";
		}
	      
	      keys << "gap";
	      keys << "gap";
	      keys << "morph into";

	      keys << "command";
	      keys << "commandhelp";
	      keys << "message";
	      

	      if (m_crops[i]->cropType() != CropObject::Displace_Displace)
		{
		  if (m_crops[i]->cropType() >= CropObject::Glow_Ball)
		    propertyEditor.set("Glow Parameters", plist, keys);
		  else if (m_crops[i]->cropType() >= CropObject::View_Tear)
		    propertyEditor.set("Blend Parameters", plist, keys);
		  else if (m_crops[i]->cropType() >= CropObject::Tear_Tear)
		    propertyEditor.set("Dissect Parameters", plist, keys);
		  else 
		    propertyEditor.set("Crop Parameters", plist, keys);
		}
	      else 
		propertyEditor.set("Displace Parameters", plist, keys);

	      
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
		      if (keys[ik] == "style")
			{
			  if (m_crops[i]->cropType() >= CropObject::Glow_Ball)
			    m_crops[i]->setCropType(pair.first.toInt() +
						    CropObject::Glow_Ball);
			  else if (m_crops[i]->cropType() >= CropObject::View_Tear)
			    m_crops[i]->setCropType(pair.first.toInt() +
						    CropObject::View_Tear);
			  else if (m_crops[i]->cropType() >= CropObject::Tear_Tear)
			    m_crops[i]->setCropType(pair.first.toInt() +
						    CropObject::Tear_Tear);
			  else
			    m_crops[i]->setCropType(pair.first.toInt());
			}
		      else if (keys[ik] == "morph into")
			{
			  int id = pair.first.toInt();
			  if (id > 0)
			    {
			      if (id == 1)
				{
				  if(m_crops[i]->cropType() < CropObject::Crop_Tube ||
				     m_crops[i]->cropType() > CropObject::Crop_Ellipsoid)
				    m_crops[i]->setCropType(CropObject::Crop_Box);
				}
			      else if (id == 2)
				{
				  if (m_crops[i]->cropType() < CropObject::View_Tear ||
				      m_crops[i]->cropType() > CropObject::View_Block)
				    m_crops[i]->setCropType(CropObject::View_Block);
				}
			      else if (id == 3)
				{
				  if (m_crops[i]->cropType() < CropObject::Tear_Tear ||
				      m_crops[i]->cropType() > CropObject::Tear_Curl)
				    m_crops[i]->setCropType(CropObject::Tear_Tear);
				}
			      else if (id == 4)
				{
				  if (m_crops[i]->cropType() < CropObject::Glow_Ball ||
				      m_crops[i]->cropType() > CropObject::Glow_Tube)
				    m_crops[i]->setCropType(CropObject::Glow_Ball);
				}
			      else if (id == 5)
				{
				  if (m_crops[i]->cropType() != CropObject::Displace_Displace)
				    m_crops[i]->setCropType(CropObject::Displace_Displace);
				}
			    }
			}
		      else if (keys[ik] == "color")
			{
			  QColor color = pair.first.value<QColor>();
			  float r = color.redF();
			  float g = color.greenF();
			  float b = color.blueF();
			  Vec pcolor = Vec(r,g,b);
			  m_crops[i]->setColor(pcolor);
			}
		      else if (keys[ik] == "clearview")
			m_crops[i]->setClearView(pair.first.toBool());
		      else if (keys[ik] == "magnify")
			m_crops[i]->setMagnify(pair.first.toDouble());
		      else if (keys[ik] == "tfset")
			m_crops[i]->setTFset(pair.first.toInt());
		      else if (keys[ik] == "mix")
			m_crops[i]->setViewMix(pair.first.toDouble());
		      else if (keys[ik] == "union")
			m_crops[i]->setUnionBlend(pair.first.toDouble());
		      else if (keys[ik] == "opacity")
			m_crops[i]->setOpacity(pair.first.toDouble());
		      else if (keys[ik] == "half section")
			m_crops[i]->setHalfSection(pair.first.toBool());
		      else if (keys[ik] == "keep inside")
			m_crops[i]->setKeepInside(pair.first.toBool());
		      else if (keys[ik] == "keep ends")
			m_crops[i]->setKeepEnds(pair.first.toBool());
		      else if (keys[ik] == "translate")
			{
			  QString str = pair.first.toString();
			  Vec v = StaticFunctions::getVec(str);
			  m_crops[i]->setDtranslate(v);
			}
		      else if (keys[ik] == "pivot")
			{
			  QString str = pair.first.toString();
			  Vec v = StaticFunctions::getVec(str);
			  m_crops[i]->setDpivot(v);
			}
		      else if (keys[ik] == "axis")
			{
			  QString str = pair.first.toString();
			  Vec v = StaticFunctions::getVec(str);
			  m_crops[i]->setDrotaxis(v);
			}
		      else if (keys[ik] == "angle")
			m_crops[i]->setDrotangle(pair.first.toDouble());
		      else if (keys[ik] == "hatch")
			m_crops[i]->setHatch(pair.first.toBool());
		      else if (keys[ik] == "hatch parameters")
			{
			  QString str = pair.first.toString();
			  QStringList words = str.split(" ", QString::SkipEmptyParts);
			  bool hg = true;
			  int xn,xd,yn,yd,zn,zd;
			  xn=xd=yn=yd=zn=zd=0;
			  if (words.count() > 0) hg = (words[0] == "grid");
			  if (words.count() > 1) xn = words[1].toInt();
			  if (words.count() > 2) xd = words[2].toInt();
			  if (words.count() > 3) yn = words[3].toInt();
			  if (words.count() > 4) yd = words[4].toInt();
			  if (words.count() > 5) zn = words[5].toInt();
			  if (words.count() > 6) zd = words[6].toInt();
			  m_crops[i]->setHatchParameters(hg, xn,xd,yn,yd,zn,zd);
			}
		    }
		}

	      QString cmd = propertyEditor.getCommandString();
	      if (!cmd.isEmpty())
		processCommand(i, cmd);	
	      
 	      //emit updateGL();
	      return true;
	    }
	}
    }
  
  return false;
}

void
Crops::makeCropConnections()
{
  for(int i=0; i<m_crops.count(); i++)
    m_crops[i]->disconnect();

  for(int i=0; i<m_crops.count(); i++)
    {
      connect(m_crops[i], SIGNAL(selectForEditing(int, int)),
	      this, SLOT(selectForEditing(int, int)));

      connect(m_crops[i], SIGNAL(deselectForEditing()),
	      this, SLOT(deselectForEditing()));
    }
}
void
Crops::deselectForEditing()
{
  for(int i=0; i<m_crops.count(); i++)
    m_crops[i]->setPointPressed(-1);
}

void
Crops::selectForEditing(int mouseButton,
			int p0)
{
  int idx = -1;
  for(int i=0; i<m_crops.count(); i++)
    {
      if (m_crops[i]->grabsMouse())
	{
	  idx = i;
	  break;
	}
    }
  if (idx == -1)
    return;

  m_crops[idx]->setPointPressed(m_crops[idx]->pointPressed());
}

void
Crops::processCommand(int idx, QString cmd)
{
  bool ok;
  cmd = cmd.toLower();
  QStringList list = cmd.split(" ", QString::SkipEmptyParts);
  
  int li = 0;
  while (li < list.count())
    {
      if (list[li] == "mop")
	{
	  if (list.count()-li > 1)
	    {
	      if (list[li+1] == "crop")
		{
		  emit mopCrop(idx);
		  return;
		}
	    }
	  QMessageBox::information(0, "Crop Command Error",
				   "Incorrect option for mop");
	}
      else if (list[li] == "normalize" ||
	  list[li] == "normalise")
	{
	  m_crops[idx]->normalize();

	  m_crops[idx]->updateUndo();
	}
      else if (list[li] == "point")
	{
	  if (list.count()-li > 4)
	    {
	      int pidx = qBound(0, list[li+1].toInt(&ok), 1);
	      float x = list[li+2].toFloat(&ok);
	      float y = list[li+3].toFloat(&ok);
	      float z = list[li+4].toFloat(&ok);
	      m_crops[idx]->setPoint(pidx, Vec(x,y,z));
	      li+=4;
	    }
	  else
	    QMessageBox::information(0, "Crop Command Error",
				     "Point not specified correctly.\nfor e.g. point 0 10.5 5.2 2.4");

	  m_crops[idx]->updateUndo();
	}
      else if (list[li] == "setradius" ||
	       list[li] == "radius")
	{
	  if (list.count()-li > 2)
	    {
	      int pidx = qBound(0, list[li+1].toInt(&ok), 1);
	      float rad = list[li+2].toFloat(&ok);
	      m_crops[idx]->setRadX(pidx, rad, false);
	      m_crops[idx]->setRadY(pidx, rad, false);
	      li+=2;
	    }
	  else if (list.count()-li > 1)
	    {
	      float rad = list[li+1].toFloat(&ok);
	      m_crops[idx]->setRadX(0, rad, true);
	      m_crops[idx]->setRadY(0, rad, true);
	      li++;
	    }
	  else
	    QMessageBox::information(0, "Crop Command Error",
				     "No radius given");

	  m_crops[idx]->updateUndo();
	}
      else if (list[li] == "setlift" ||
	       list[li] == "lift")
	{
	  if (list.count()-li > 2)
	    {
	      int pidx = qBound(0, list[li+1].toInt(&ok), 1);
	      int lift = list[li+2].toInt(&ok);
	      m_crops[idx]->setLift(pidx, lift, false);
	      li+=2;
	    }
	  else if (list.count()-li > 1)
	    {
	      int lift = list[li+1].toInt(&ok);
	      m_crops[idx]->setLift(0, lift, true);
	      m_crops[idx]->setLift(1, lift, true);
	      li++;
	    }
	  else
	    QMessageBox::information(0, "Crop Command Error",
				     "No radius given");
	}
      else if (list[li] == "setrads" || list[li] == "rads")
	{
	  if (list.count()-li > 2)
	    {
	      int pidx = qBound(0, list[li+1].toInt(&ok), 1);
	      float radx = list[li+2].toFloat(&ok);
	      m_crops[idx]->setRadX(pidx, radx, false);
	      li+=2;
	    }
	  else if (list.count()-li > 1)
	    {
	      float radx = list[li+1].toFloat(&ok);
	      m_crops[idx]->setRadX(idx, radx, true);
	      li++;
	    }
	  else
	    QMessageBox::information(0, "Crop Command Error",
				     "No S radius given");

	  m_crops[idx]->updateUndo();
	}
      else if (list[li] == "setradt" || list[li] == "radt")
	{
	  if (list.count()-li > 2)
	    {
	      int pidx = qBound(0, list[li+1].toInt(&ok), 1);
	      float rady = list[li+2].toFloat(&ok);
	      m_crops[idx]->setRadY(pidx, rady, false);
	      li+=2;
	    }
	  else if (list.count()-li > 1)
	    {
	      float rady = list[li+1].toFloat(&ok);
	      m_crops[idx]->setRadY(idx, rady, true);
	      li++;
	    }
	  else
	    QMessageBox::information(0, "Crop Command Error",
				     "No T radius given");

	  m_crops[idx]->updateUndo();
	}
      else if (list[li] == "setangle" || list[li] == "angle")
	{
	  if (list.count()-li > 1)
	    {
	      float a = list[li+1].toFloat(&ok);
	      m_crops[idx]->setAngle(a);
	      li++;
	    }
	  else
	    QMessageBox::information(0, "Crop Command Error",
				     "No angle given");
	}
      else if (list[li] == "moves")
	{
	  if (list.size()-li > 1)
	    {
	      if (list[li+1] == "-")
		m_crops[idx]->translate(true, false);
	      else
		m_crops[idx]->translate(true, true);

	      li++;
	    }
	  else
	    m_crops[idx]->translate(true, true);

	  m_crops[idx]->updateUndo();
	}
      else if (list[li] == "movet")
	{
	  if (list.size()-li > 1)
	    {
	      if (list[li+1] == "-")
		m_crops[idx]->translate(false, false);
	      else
		m_crops[idx]->translate(false, true);

	      li++;
	    }
	  else
	    m_crops[idx]->translate(false, true);

	  m_crops[idx]->updateUndo();
	}
      else if (list[li] == "save")
	{
	  QString flnm;
	  flnm = QFileDialog::getSaveFileName(0,
					      "Save points to text file",
					      Global::previousDirectory(),
					      "Files (*.*)");
	  
	  if (flnm.isEmpty())
	    return;

	  QList<Vec> pts = m_crops[idx]->points();

	  QFile fcrop(flnm);
	  fcrop.open(QFile::WriteOnly | QFile::Text);
	  QTextStream fd(&fcrop);
	  fd << pts.count() << "\n";
	  for(int pi=0; pi < pts.count(); pi++)
	    fd << pts[pi].x << " " << pts[pi].y << " " << pts[pi].z << "\n";
	  
	  fd.flush();
	}
      else
	QMessageBox::information(0, "Crop Command Error",
				 QString("Cannot understand the command : ") +
				 cmd);

      li++;
    }
}
