#include "global.h"
#include "staticfunctions.h"
#include "dcolordialog.h"
#include "networks.h"
#include "geoshaderfactory.h"
#include "propertyeditor.h"

#include <QDomDocument>


Networks::Networks()
{
  m_networks.clear();
  m_geoSpriteShader = 0;
  m_geoShadowShader = 0;
}

Networks::~Networks()
{
  clear();
}


void
Networks::show()
{
  for (int i=0; i<m_networks.count(); i++)
    {
      m_networks[i]->setShow(true);
      m_networks[i]->addInMouseGrabberPool();
    }
}

void
Networks::hide()
{
  for (int i=0; i<m_networks.count(); i++)
    {
      m_networks[i]->setShow(false);
      m_networks[i]->removeFromMouseGrabberPool();
    }
}

bool
Networks::show(int i)
{
  if (i >= 0 && i < m_networks.count())
    return m_networks[i]->show();
  else
    return false;
}

void
Networks::setShow(int i, bool flag)
{
  if (i >= 0 && i < m_networks.count())
    {
      m_networks[i]->setShow(flag);
      if (flag)
	m_networks[i]->addInMouseGrabberPool();
      else
	m_networks[i]->removeFromMouseGrabberPool();
    }
}

void
Networks::clear()
{
  for (int i=0; i<m_networks.count(); i++)
    delete m_networks[i];

  m_networks.clear();
}

void
Networks::allGridSize(int &nx, int &ny, int &nz)
{
  nx = 0;
  ny = 0;
  nz = 0;

  if (m_networks.count() == 0)
    return;

  m_networks[0]->gridSize(nx, ny, nz);

  for(int i=1; i<m_networks.count();i++)
    {
      int mx, my, mz;
      m_networks[i]->gridSize(mx, my, mz);
      nx = qMax(nx, mx);
      ny = qMax(ny, my);
      nz = qMax(nz, mz);
    }
}

void
Networks::allEnclosingBox(Vec& boxMin,
			  Vec& boxMax)
{
  if (m_networks.count() == 0)
    return;

  boxMin = Vec(0,0,0);
  boxMax = Vec(0,0,0);

  m_networks[0]->enclosingBox(boxMin, boxMax);

  for(int i=1; i<m_networks.count();i++)
    {
      Vec bmin, bmax;
      m_networks[i]->enclosingBox(bmin, bmax);
      boxMin = StaticFunctions::minVec(boxMin, bmin);
      boxMax = StaticFunctions::maxVec(boxMax, bmax);
    }
}

bool
Networks::isInMouseGrabberPool(int i)
{
  if (i < m_networks.count())
    return m_networks[i]->isInMouseGrabberPool();
  else
    return false;
}
void
Networks::addInMouseGrabberPool(int i)
{
  if (i < m_networks.count())
    m_networks[i]->addInMouseGrabberPool();
}
void
Networks::addInMouseGrabberPool()
{
  for(int i=0; i<m_networks.count(); i++)
    m_networks[i]->addInMouseGrabberPool();
}
void
Networks::removeFromMouseGrabberPool(int i)
{
  if (i < m_networks.count())
    m_networks[i]->removeFromMouseGrabberPool();
}

void
Networks::removeFromMouseGrabberPool()
{
  for(int i=0; i<m_networks.count(); i++)
    m_networks[i]->removeFromMouseGrabberPool();
}


bool
Networks::grabsMouse()
{
  for(int i=0; i<m_networks.count(); i++)
    {
      if (m_networks[i]->grabsMouse())
	return true;
    }
  return false;
}

void
Networks::addNetwork(QString flnm)
{
  NetworkGrabber *tg = new NetworkGrabber();
  if (tg->load(flnm))
    m_networks.append(tg);
  else
    delete tg;
}

void
Networks::postdraw(QGLViewer *viewer)
{
  for(int i=0; i<m_networks.count();i++)
    {
      int x,y;
      m_networks[i]->mousePosition(x,y);
      m_networks[i]->postdraw(viewer,
			     x, y,
			     m_networks[i]->grabsMouse(),
			     i);
    }
}

void
Networks::predraw(QGLViewer *viewer,
		  double *Xform,
		  Vec pn,
		  QList<Vec> clipPos,
		  QList<Vec> clipNormal,
		  QList<CropObject> crops,
		  Vec lightVector)
{
  for(int i=0; i<m_networks.count();i++)
    m_networks[i]->predraw(viewer,
			   Xform,
			   pn,
			   clipPos, clipNormal,
			   crops,
			   lightVector);
}

void
Networks::draw(QGLViewer *viewer,
	      float pnear, float pfar,
	      bool backlit)
{
  for(int i=0; i<m_networks.count();i++)
    {
      m_networks[i]->draw(viewer,
			  m_networks[i]->grabsMouse(),
			  pnear, pfar,
			  backlit);

      glUseProgramObjectARB(0);
    }

}

bool
Networks::keyPressEvent(QKeyEvent *event)
{
  for(int i=0; i<m_networks.count(); i++)
    {
      if (m_networks[i]->grabsMouse())
	{
	  if (event->key() == Qt::Key_G)
	    {
	      m_networks[i]->removeFromMouseGrabberPool();
	      return true;
	    }
	  if (event->key() == Qt::Key_Delete ||
	      event->key() == Qt::Key_Backspace ||
	      event->key() == Qt::Key_Backtab)
	    {
	      m_networks[i]->removeFromMouseGrabberPool();
	      m_networks.removeAt(i);
	      return true;
	    }
	  if (event->key() == Qt::Key_Space)
	    {
	      PropertyEditor propertyEditor;
	      QMap<QString, QVariantList> plist;

	      QVariantList vlist;

	      vlist.clear();
	      vlist << QVariant("double");
	      vlist << QVariant(m_networks[i]->vopacity());
	      vlist << QVariant(0.0);
	      vlist << QVariant(1.0);
	      vlist << QVariant(0.1); // singlestep
	      vlist << QVariant(1); // decimals
	      plist["vertex opacity"] = vlist;

	      vlist.clear();
	      vlist << QVariant("colorgradient");
	      QGradientStops vstops = m_networks[i]->vstops();
	      for(int s=0; s<vstops.size(); s++)
		{
		  float pos = vstops[s].first;
		  QColor color = vstops[s].second;
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
	      plist["vertex color gradient"] = vlist;


	      vlist.clear();
	      vlist << QVariant("combobox");
	      vlist << QVariant(m_networks[i]->vertexAttribute());
	      for(int va=0; va<m_networks[i]->vertexAttributeCount(); va++)
		{
		  QString vstr;
		  float vmin, vmax;
		  vstr = m_networks[i]->vertexAttributeString(va);
		  m_networks[i]->vMinmax(va, vmin, vmax);
		  vlist << QVariant(QString("%1 : %2 : %3 %4").\
				    arg(va).arg(vstr).arg(vmin).arg(vmax));
		}
	      plist["vertex attribute list"] = vlist;


	      int iva = m_networks[i]->vertexAttribute();
	      float vmin, vmax;
	      m_networks[i]->userVminmax(iva, vmin, vmax);
	      vlist.clear();
	      vlist << QVariant("string");
	      vlist << QVariant(QString("%1 %2").arg(vmin).arg(vmax));
	      plist["vertex attribute bounds"] = vlist;





	      vlist.clear();
	      vlist << QVariant("double");
	      vlist << QVariant(m_networks[i]->eopacity());
	      vlist << QVariant(0.0);
	      vlist << QVariant(1.0);
	      vlist << QVariant(0.1); // singlestep
	      vlist << QVariant(1); // decimals
	      plist["edge opacity"] = vlist;

	      vlist.clear();
	      vlist << QVariant("combobox");
	      vlist << QVariant(m_networks[i]->edgeAttribute());
	      for(int ea=0; ea<m_networks[i]->edgeAttributeCount(); ea++)
		{
		  QString estr;
		  float emin, emax;
		  estr = m_networks[i]->edgeAttributeString(ea);
		  m_networks[i]->eMinmax(ea, emin, emax);
		  vlist << QVariant(QString("%1 : %2 : %3 %4").\
				    arg(ea).arg(estr).arg(emin).arg(emax));
		}
	      plist["edge attribute list"] = vlist;


	      int nea = m_networks[i]->edgeAttribute();
	      float emin, emax;
	      m_networks[i]->userEminmax(nea, emin, emax);
	      vlist.clear();
	      vlist << QVariant("string");
	      vlist << QVariant(QString("%1 %2").arg(emin).arg(emax));
	      plist["edge attribute bounds"] = vlist;

	      vlist.clear();
	      vlist << QVariant("colorgradient");
	      QGradientStops estops = m_networks[i]->estops();
	      for(int s=0; s<estops.size(); s++)
		{
		  float pos = estops[s].first;
		  QColor color = estops[s].second;
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
	      plist["edge color gradient"] = vlist;

	      vlist.clear();
	      QString mesg;
	      mesg += QString("scalev : %1\n").arg(m_networks[i]->scaleV());
	      mesg += QString("scalee : %1\n").arg(m_networks[i]->scaleE());
	      vlist << mesg;
	      plist["message"] = vlist;

	      vlist.clear();
	      plist["command"] = vlist;

	      QStringList keys;
	      keys << "vertex color gradient";
	      keys << "vertex opacity";
	      keys << "vertex attribute list";
	      keys << "vertex attribute bounds";
	      keys << "gap";
	      keys << "edge color gradient";
	      keys << "edge opacity";
	      keys << "edge attribute list";
	      keys << "edge attribute bounds";
	      keys << "command";
	      keys << "message";
	      

	      propertyEditor.set("Network Parameters", plist, keys);
	      QMap<QString, QPair<QVariant, bool> > vmap;

	      if (propertyEditor.exec() == QDialog::Accepted)
		{
		  QString cmd = propertyEditor.getCommandString();
		  if (!cmd.isEmpty())
		    return processCommand(i, cmd);

		  vmap = propertyEditor.get();
		}
	      else
		return true;


	      for(int ik=0; ik<keys.count(); ik++)
		{
		  QPair<QVariant, bool> pair = vmap.value(keys[ik]);

		  if (pair.second)
		    {
		      if (keys[ik] == "vertex color gradient")
			{
			  QGradientStops stops = propertyEditor.getGradientStops(keys[ik]);
			  m_networks[i]->setVstops(stops);
			}
		      else if (keys[ik] == "vertex opacity")
			m_networks[i]->setVOpacity(pair.first.toDouble());
		      else if (keys[ik] == "vertex attribute list")
			{
			  QString vstr = pair.first.toString();
			  QStringList vl = vstr.split(":");
			  m_networks[i]->setVertexAttribute(vl[0].toInt());
			}
		      else if (keys[ik] == "vertex attribute bounds")
			{
			  QString vstr = pair.first.toString();
			  QStringList vl = vstr.split(" ");
			  if (vl.count() == 2)
			    m_networks[i]->setUserVminmax(vl[0].toDouble(),
							  vl[1].toDouble());
			}
		      else if (keys[ik] == "edge color gradient")
			{
			  QGradientStops stops = propertyEditor.getGradientStops(keys[ik]);
			  m_networks[i]->setEstops(stops);
			}
		      else if (keys[ik] == "edge opacity")
			m_networks[i]->setEOpacity(pair.first.toDouble());
		      else if (keys[ik] == "edge attribute list")
			{
			  QString estr = pair.first.toString();
			  QStringList el = estr.split(":");
			  m_networks[i]->setEdgeAttribute(el[0].toInt());
			}
		      else if (keys[ik] == "edge attribute bounds")
			{
			  QString estr = pair.first.toString();
			  QStringList el = estr.split(" ");
			  if (el.count() == 2)
			    m_networks[i]->setUserEminmax(el[0].toDouble(),
							  el[1].toDouble());
			}
		    }
		}
	      
	      return true;
	    }
	}
    }

  return false;
}

void
Networks::createSpriteShader()
{
  if (m_geoSpriteShader)
    glDeleteObjectARB(m_geoSpriteShader);

  QString shaderString;

  shaderString = GeoShaderFactory::genSpriteShaderString();
  m_geoSpriteShader = glCreateProgramObjectARB();
  if (! GeoShaderFactory::loadShader(m_geoSpriteShader,
				     shaderString))
    exit(0);

  m_spriteParm[0] = glGetUniformLocationARB(m_geoSpriteShader, "spriteTex");
}

void
Networks::createShadowShader(Vec attenuation)
{
  if (m_geoShadowShader)
    glDeleteObjectARB(m_geoShadowShader);

  float r = attenuation.x;
  float g = attenuation.y;
  float b = attenuation.z;

  QString shaderString;

  shaderString = GeoShaderFactory::genSpriteShadowShaderString(r,g,b);
  m_geoShadowShader = glCreateProgramObjectARB();
  if (! GeoShaderFactory::loadShader(m_geoShadowShader,
				     shaderString))
    exit(0);

  m_shadowParm[0] = glGetUniformLocationARB(m_geoShadowShader, "spriteTex");
}

void
Networks::save(const char *flnm)
{
  QDomDocument doc;
  QFile f(flnm);
  if (f.open(QIODevice::ReadOnly))
    {
      doc.setContent(&f);
      f.close();
    }

  QDomElement topElement = doc.documentElement();

  for(int i=0; i<m_networks.count(); i++)
    {
      QDomElement de = m_networks[i]->domElement(doc);
      topElement.appendChild(de);
    }

  QFile fout(flnm);
  if (fout.open(QIODevice::WriteOnly))
    {
      QTextStream out(&fout);
      doc.save(out, 2);
      fout.close();
    }
}

void
Networks::load(const char *flnm)
{
  QDomDocument document;
  QFile f(flnm);
  if (f.open(QIODevice::ReadOnly))
    {
      document.setContent(&f);
      f.close();
    }

  QDomElement main = document.documentElement();
  QDomNodeList dlist = main.childNodes();
  for(int i=0; i<dlist.count(); i++)
    {
      if (dlist.at(i).nodeName() == "network")
	{
	  QDomElement de = dlist.at(i).toElement();

	  NetworkGrabber *tg = new NetworkGrabber();
	  if (tg->fromDomElement(de))	    
	    m_networks.append(tg);
	  else
	    delete tg;
	}
    }

}

QList<NetworkInformation>
Networks::get()
{
  QList<NetworkInformation> tinfo;

  for(int i=0; i<m_networks.count(); i++)
    tinfo.append(m_networks[i]->get());

  return tinfo;
}

void
Networks::set(QList<NetworkInformation> tinfo)
{
  if (tinfo.count() == 0)
    {
      clear();
      return;
    }
    
  if (m_networks.count() == 0)
    {
      for(int i=0; i<tinfo.count(); i++)
	{
	  NetworkGrabber *tgi = new NetworkGrabber();	
	  if (tgi->set(tinfo[i]))
	    m_networks.append(tgi);
	  else
	    delete tgi;
	}

      return;
    }


  QVector<int> present;
  present.resize(tinfo.count());
  for(int i=0; i<tinfo.count(); i++)
    {
      present[i] = -1;
      for(int j=0; j<m_networks.count(); j++)
	{
	  if (tinfo[i].filename == m_networks[j]->filename())
	    {
	      present[i] = j;
	      break;
	    }
	}
    }

  QList<NetworkGrabber*> tg;
  tg = m_networks;
  
  m_networks.clear();
  for(int i=0; i<tinfo.count(); i++)
    {
      NetworkGrabber *tgi;

      if (present[i] >= 0)
	tgi = tg[present[i]];
      else
	tgi = new NetworkGrabber();
	
      if (tgi->set(tinfo[i]))
	m_networks.append(tgi);
      else
	delete tgi;
    }

  for(int i=0; i<tg.count(); i++)
    {
      if (! m_networks.contains(tg[i]))
	delete tg[i];
    }

  tg.clear();
}

bool
Networks::processCommand(int id, QString cmd)
{
  bool ok;
  cmd = cmd.toLower();
  QStringList list = cmd.split(" ", QString::SkipEmptyParts);
  
  if (list[0] == "scale")
    {
      float scl = 1.0;
      if (list.count() > 1)
	scl = list[1].toFloat();
      m_networks[id]->setScale(scl);
      return true;
    }
  else if (list[0] == "scalev")
    {
      float scl = 1.0;
      if (list.count() > 1)
	scl = list[1].toFloat();
      m_networks[id]->setScaleV(scl);
      return true;
    }
  else if (list[0] == "scalee")
    {
      float scl = 1.0;
      if (list.count() > 1)
	scl = list[1].toFloat();
      m_networks[id]->setScaleE(scl);
      return true;
    }
  return false;
}
