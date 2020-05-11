#include "global.h"
#include "shaderfactory.h"
#include "staticfunctions.h"
#include "dcolordialog.h"
#include "trisets.h"
#include "geoshaderfactory.h"
#include "propertyeditor.h"

#include <QDomDocument>


Trisets::Trisets()
{
  m_trisets.clear();
  m_geoDefaultShader = 0;
  m_geoHighQualityShader = 0;
  m_geoShadowShader = 0;
  m_cpos = new float[100];
  m_cnormal = new float[100];
}

Trisets::~Trisets()
{
  clear();
  delete [] m_cpos;
  delete [] m_cnormal;
}


QString
Trisets::filename(int i)
{
  return m_trisets[i]->filename();
}

void
Trisets::show()
{
  for (int i=0; i<m_trisets.count(); i++)
    {
      m_trisets[i]->setShow(true);
      //m_trisets[i]->addInMouseGrabberPool();
    }
}

void
Trisets::hide()
{
  for (int i=0; i<m_trisets.count(); i++)
    {
      m_trisets[i]->setShow(false);
      //m_trisets[i]->removeFromMouseGrabberPool();
    }
}

bool
Trisets::show(int i)
{
  if (i >= 0 && i < m_trisets.count())
    return m_trisets[i]->show();
  else
    return false;
}

void
Trisets::setShow(int i, bool flag)
{
  if (i >= 0 && i < m_trisets.count())
    {
      m_trisets[i]->setShow(flag);
//      if (flag)
//	m_trisets[i]->addInMouseGrabberPool();
//      else
//	m_trisets[i]->removeFromMouseGrabberPool();
    }
}

bool
Trisets::clip(int i)
{
  if (i >= 0 && i < m_trisets.count())
    return m_trisets[i]->clip();
  else
    return false;
}

void
Trisets::setClip(int i, bool flag)
{
  if (i >= 0 && i < m_trisets.count())
    {
      m_trisets[i]->setClip(flag);
    }
}

void
Trisets::setLighting(Vec ads)
{
  for (int i=0; i<m_trisets.count(); i++)
    {
      m_trisets[i]->setAmbient(ads.x);
      m_trisets[i]->setDiffuse(ads.y);
      m_trisets[i]->setSpecular(ads.z);
    }
}

void
Trisets::clear()
{
  for (int i=0; i<m_trisets.count(); i++)
    delete m_trisets[i];

  m_trisets.clear();
}

void
Trisets::allGridSize(int &nx, int &ny, int &nz)
{
  nx = 0;
  ny = 0;
  nz = 0;

  if (m_trisets.count() == 0)
    return;

  m_trisets[0]->gridSize(nx, ny, nz);

  for(int i=1; i<m_trisets.count();i++)
    {
      int mx, my, mz;
      m_trisets[i]->gridSize(mx, my, mz);
      nx = qMax(nx, mx);
      ny = qMax(ny, my);
      nz = qMax(nz, mz);
    }
}

void
Trisets::allEnclosingBox(Vec& boxMin,
			 Vec& boxMax)
{
  if (m_trisets.count() == 0)
    return;

//  boxMin = Vec(0,0,0);
//  boxMax = Vec(0,0,0);

  m_trisets[0]->enclosingBox(boxMin, boxMax);

  for(int i=1; i<m_trisets.count();i++)
    {
      Vec bmin, bmax;
      m_trisets[i]->enclosingBox(bmin, bmax);
      boxMin = StaticFunctions::minVec(boxMin, bmin);
      boxMax = StaticFunctions::maxVec(boxMax, bmax);
    }
}

bool
Trisets::isInMouseGrabberPool(int i)
{
  if (i < m_trisets.count())
    return m_trisets[i]->isInMouseGrabberPool();
  else
    return false;
}
void
Trisets::addInMouseGrabberPool(int i)
{
  if (i < m_trisets.count())
    m_trisets[i]->addInMouseGrabberPool();
}
void
Trisets::addInMouseGrabberPool()
{
  for(int i=0; i<m_trisets.count(); i++)
    m_trisets[i]->addInMouseGrabberPool();
}
void
Trisets::removeFromMouseGrabberPool(int i)
{
  if (i < m_trisets.count())
    m_trisets[i]->removeFromMouseGrabberPool();
}

void
Trisets::removeFromMouseGrabberPool()
{
  for(int i=0; i<m_trisets.count(); i++)
    m_trisets[i]->removeFromMouseGrabberPool();
}


bool
Trisets::grabsMouse()
{
 for(int i=0; i<m_trisets.count(); i++)
   {
     if (m_trisets[i]->grabsMouse())
	return true;
   }
 return false;
}

void
Trisets::addTriset(QString flnm)
{
  TrisetGrabber *tg = new TrisetGrabber();
  if (tg->load(flnm))
    m_trisets.append(tg);
  else
    delete tg;
}

void
Trisets::addMesh(QString flnm)
{
  TrisetGrabber *tg = new TrisetGrabber();
  if (tg->load(flnm))
    m_trisets.append(tg);
  else
    delete tg;
}

void
Trisets::postdraw(QGLViewer *viewer)
{
  for(int i=0; i<m_trisets.count();i++)
    {
      int x,y;
      m_trisets[i]->mousePosition(x,y);
      m_trisets[i]->postdraw(viewer,
			     x, y,
			     m_trisets[i]->grabsMouse(),
			     i);
    }
}

void
Trisets::predraw(QGLViewer *viewer,
		 double *Xform,
		 Vec pn,
		 bool shadows, int shadowWidth, int shadowHeight)
{
  for(int i=0; i<m_trisets.count();i++)
    m_trisets[i]->predraw(viewer,
			  Xform,
			  pn,
			  shadows, shadowWidth, shadowHeight);
}

void
Trisets::draw(QGLViewer *viewer,
	      Vec lightVec,
	      float pnear, float pfar, Vec step,
	      bool applyShadows, bool applyShadowShader, Vec eyepos,
	      QList<Vec> cpos, QList<Vec> cnormal,
	      bool geoblend)
{
  int nclip = cpos.count();
  if (nclip > 0)
    {
      for(int c=0; c<nclip; c++)
	{
	  m_cpos[3*c+0] = cpos[c].x;
	  m_cpos[3*c+1] = cpos[c].y;
	  m_cpos[3*c+2] = cpos[c].z;
	}
      for(int c=0; c<nclip; c++)
	{
	  m_cnormal[3*c+0] = cnormal[c].x;
	  m_cnormal[3*c+1] = cnormal[c].y;
	  m_cnormal[3*c+2] = cnormal[c].z;
	}
    }

  for(int i=0; i<m_trisets.count();i++)
    {
      glUseProgram(ShaderFactory::meshShader());
      GLint *meshShaderParm = ShaderFactory::meshShaderParm();  

      if (m_trisets[i]->clip())
	{
	  glUniform1iARB(meshShaderParm[9],  nclip);
	  glUniform3fvARB(meshShaderParm[10], nclip, m_cpos);
	  glUniform3fvARB(meshShaderParm[11], nclip, m_cnormal);
	}
      else
	{
	  glUniform1iARB(meshShaderParm[9],  0);
	}
      
      
      m_trisets[i]->draw(viewer,
			 m_trisets[i]->grabsMouse(),
			 lightVec,
			 pnear, pfar, step/2);

      glUseProgramObjectARB(0);

//      if ((m_trisets[i]->blendMode() && geoblend) ||
//	  (!m_trisets[i]->blendMode() && !geoblend))
//	{
//	  if (! m_trisets[i]->pointMode())
//	    {
//	      if (applyShadows &&
//		  m_trisets[i]->shadows())
//		{
//		  if (applyShadowShader)
//		    {
//		      glUseProgramObjectARB(m_geoShadowShader);
//		      glUniform1iARB(m_shadowParm[0], nclip);
//		      glUniform3fvARB(m_shadowParm[1], nclip, m_cpos);
//		      glUniform3fvARB(m_shadowParm[2], nclip, m_cnormal);
//		    }
//		  else
//		    {
//		      Vec cb = m_trisets[i]->cropBorderColor();
//
//		      glActiveTexture(GL_TEXTURE2);
//		      glEnable(GL_TEXTURE_RECTANGLE_ARB);
//
//		      glUseProgramObjectARB(m_geoHighQualityShader);
//		      glUniform3fARB(m_highqualityParm[0], eyepos.x, eyepos.y, eyepos.z);
//		      glUniform1iARB(m_highqualityParm[1], 2); // blurred shadowBuffer
//		      glUniform1iARB(m_highqualityParm[2], m_trisets[i]->screenDoor());
//		      glUniform3fARB(m_highqualityParm[3], cb.x, cb.y, cb.z);
//		      glUniform1iARB(m_highqualityParm[4], nclip);
//		      glUniform3fvARB(m_highqualityParm[5], nclip, m_cpos);
//		      glUniform3fvARB(m_highqualityParm[6], nclip, m_cnormal);
//
//		      glActiveTexture(GL_TEXTURE2);
//		      glDisable(GL_TEXTURE_RECTANGLE_ARB);
//		    }
//		}
//	      else
//		{
//		  Vec cb = m_trisets[i]->cropBorderColor();
//		  glUseProgramObjectARB(m_geoDefaultShader);
//		  glUniform3fARB(m_defaultParm[0], eyepos.x, eyepos.y, eyepos.z);
//		  glUniform1iARB(m_defaultParm[1], m_trisets[i]->screenDoor());
//		  glUniform3fARB(m_defaultParm[2], cb.x, cb.y, cb.z);
//		  glUniform1iARB(m_defaultParm[3], nclip);
//		  glUniform3fvARB(m_defaultParm[4], nclip, m_cpos);
//		  glUniform3fvARB(m_defaultParm[5], nclip, m_cnormal);
//		}
//	    }
//	  else
//	    glUseProgramObjectARB(0);
//
//	  m_trisets[i]->draw(viewer,
//			     m_trisets[i]->grabsMouse(),
//			     lightVec,
//			     pnear, pfar, step/2);
//	  
//	  glUseProgramObjectARB(0);
//	}
    }

}

bool
Trisets::keyPressEvent(QKeyEvent *event)
{
  for(int i=0; i<m_trisets.count(); i++)
    {
      if (m_trisets[i]->grabsMouse())
	{
	  if (event->key() == Qt::Key_G)
	    {
	      m_trisets[i]->removeFromMouseGrabberPool();
	      return true;
	    }
	  else if (event->key() == Qt::Key_S)
	    {
	      m_trisets[i]->save();
	      return true;
	    }
	  else if (event->key() == Qt::Key_X)
	    {
	      m_trisets[i]->setMoveAxis(TrisetGrabber::MoveX);
	      return true;
	    }
	  else if (event->key() == Qt::Key_Y)
	    {
	      m_trisets[i]->setMoveAxis(TrisetGrabber::MoveY);
	      return true;
	    }
	  else if (event->key() == Qt::Key_Z)
	    {
	      m_trisets[i]->setMoveAxis(TrisetGrabber::MoveZ);
	      return true;
	    }
	  else if (event->key() == Qt::Key_W)
	    {
	      m_trisets[i]->setMoveAxis(TrisetGrabber::MoveAll);
	      return true;
	    }
	  else if (event->key() == Qt::Key_Delete ||
	      event->key() == Qt::Key_Backspace ||
	      event->key() == Qt::Key_Backtab)
	    {
	      m_trisets[i]->removeFromMouseGrabberPool();
	      m_trisets.removeAt(i);
	      return true;
	    }
	  if (event->key() == Qt::Key_Space)
	    {
	      PropertyEditor propertyEditor;
	      QMap<QString, QVariantList> plist;

	      QVariantList vlist;

	      Vec pos = m_trisets[i]->position();
	      vlist.clear();
	      vlist << QVariant("string");
	      vlist << QVariant(QString("%1 %2 %3").arg(pos.x).arg(pos.y).arg(pos.z));
	      plist["position"] = vlist;

	      pos = m_trisets[i]->scale();
	      vlist.clear();
	      vlist << QVariant("string");
	      vlist << QVariant(QString("%1 %2 %3").arg(pos.x).arg(pos.y).arg(pos.z));
	      plist["scale"] = vlist;

//	      vlist.clear();
//	      vlist << QVariant("checkbox");
//	      vlist << QVariant(m_trisets[i]->flipNormals());
//	      plist["flip normals"] = vlist;
//
//	      vlist.clear();
//	      vlist << QVariant("checkbox");
//	      vlist << QVariant(m_trisets[i]->screenDoor());
//	      plist["screendoor transparency"] = vlist;
//
//	      vlist.clear();
//	      vlist << QVariant("double");
//	      vlist << QVariant(m_trisets[i]->opacity());
//	      vlist << QVariant(0.0);
//	      vlist << QVariant(1.0);
//	      vlist << QVariant(0.1); // singlestep
//	      vlist << QVariant(1); // decimals
//	      plist["opacity"] = vlist;
//
//	      vlist.clear();
//	      vlist << QVariant("double");
//	      vlist << QVariant(m_trisets[i]->ambient());
//	      vlist << QVariant(0.0);
//	      vlist << QVariant(1.0);
//	      vlist << QVariant(0.1); // singlestep
//	      vlist << QVariant(1); // decimals
//	      plist["ambient"] = vlist;
//
//	      vlist.clear();
//	      vlist << QVariant("double");
//	      vlist << QVariant(m_trisets[i]->diffuse());
//	      vlist << QVariant(0.0);
//	      vlist << QVariant(1.0);
//	      vlist << QVariant(0.1); // singlestep
//	      vlist << QVariant(1); // decimals
//	      plist["diffuse"] = vlist;
//
//	      vlist.clear();
//	      vlist << QVariant("double");
//	      vlist << QVariant(m_trisets[i]->specular());
//	      vlist << QVariant(0.0);
//	      vlist << QVariant(1.0);
//	      vlist << QVariant(0.1); // singlestep
//	      vlist << QVariant(1); // decimals
//	      plist["specular"] = vlist;

	      vlist.clear();
	      vlist << QVariant("color");
	      Vec pcolor = m_trisets[i]->color();
	      QColor dcolor = QColor::fromRgbF(pcolor.x,
					       pcolor.y,
					       pcolor.z);
	      vlist << dcolor;
	      plist["color"] = vlist;


//	      vlist.clear();
//	      vlist << QVariant("checkbox");
//	      vlist << QVariant(m_trisets[i]->pointMode());
//	      plist["pointmode"] = vlist;
//
//	      vlist.clear();
//	      vlist << QVariant("int");
//	      vlist << QVariant(m_trisets[i]->pointsize());
//	      vlist << QVariant(1);
//	      vlist << QVariant(50);
//	      plist["pointsize"] = vlist;
//
//	      vlist.clear();
//	      vlist << QVariant("int");
//	      vlist << QVariant(m_trisets[i]->pointstep());
//	      vlist << QVariant(1);
//	      vlist << QVariant(100);
//	      plist["pointstep"] = vlist;
//
//	      vlist.clear();
//	      vlist << QVariant("checkbox");
//	      vlist << QVariant(m_trisets[i]->shadows());
//	      plist["shadows"] = vlist;
//
//	      vlist.clear();
//	      vlist << QVariant("checkbox");
//	      vlist << QVariant(m_trisets[i]->blendMode());
//	      plist["blend with volume"] = vlist;
//
//	      vlist.clear();
//	      vlist << QVariant("color");
//	      pcolor = m_trisets[i]->cropBorderColor();
//	      dcolor = QColor::fromRgbF(pcolor.x,
//					pcolor.y,
//					pcolor.z);
//	      vlist << dcolor;
//	      plist["crop border color"] = vlist;


//	      vlist.clear();
//	      QFile helpFile(":/trisets.help");
//	      if (helpFile.open(QFile::ReadOnly))
//		{
//		  QTextStream in(&helpFile);
//		  QString line = in.readLine();
//		  while (!line.isNull())
//		    {
//		      if (line == "#begin")
//			{
//			  QString keyword = in.readLine();
//			  QString helptext;
//			  line = in.readLine();
//			  while (!line.isNull())
//			    {
//			      helptext += line;
//			      helptext += "\n";
//			      line = in.readLine();
//			      if (line == "#end") break;
//			    }
//			  vlist << keyword << helptext;
//			}
//		      line = in.readLine();
//		    }
//		}
//	      plist["commandhelp"] = vlist;

	      vlist.clear();
	      QString mesg;
	      mesg = m_trisets[i]->filename();
	      mesg += "\n";
	      mesg += QString("Vertices : (%1)    Triangles : (%2)\n").	\
		arg(m_trisets[i]->vertexCount()).arg(m_trisets[i]->triangleCount());
	      vlist << mesg;
	      plist["message"] = vlist;


	      QStringList keys;
	      keys << "position";
	      keys << "scale";
	      //keys << "gap";
	      //keys << "flip normals";
	      //keys << "screendoor transparency";
	      //keys << "gap";
	      keys << "color";
	      //keys << "opacity";
	      //keys << "gap";
	      //keys << "ambient";
	      //keys << "diffuse";
	      //keys << "specular";
	      //keys << "gap";
	      //keys << "pointmode";
	      //keys << "pointstep";
	      //keys << "pointsize";
	      //keys << "gap";
	      //keys << "shadows";
	      //keys << "blend with volume";
	      //keys << "gap";
	      //keys << "crop border color";
	      //keys << "commandhelp";
	      keys << "message";
	      

	      propertyEditor.set("Triset Parameters", plist, keys);
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
		      if (keys[ik] == "position")
			{
			  QString vstr = pair.first.toString();
			  QStringList pos = vstr.split(" ");
			  if (pos.count() == 3)
			    m_trisets[i]->setPosition(Vec(pos[0].toDouble(),
							  pos[1].toDouble(),
							  pos[2].toDouble()));
			}
		      else if (keys[ik] == "scale")
			{
			  QString vstr = pair.first.toString();
			  QStringList scl = vstr.split(" ");
			  Vec scale = Vec(1,1,1);
			  if (scl.count() > 0) scale.x = scl[0].toDouble();
			  if (scl.count() > 1) scale.y = scl[1].toDouble();
			  if (scl.count() > 2) scale.z = scl[2].toDouble();
			  m_trisets[i]->setScale(scale);
			}
		      else if (keys[ik] == "color")
			{
			  QColor color = pair.first.value<QColor>();
			  float r = color.redF();
			  float g = color.greenF();
			  float b = color.blueF();
			  Vec pcolor = Vec(r,g,b);
			  m_trisets[i]->setColor(pcolor);
			}
		      else if (keys[ik] == "crop border color")
			{
			  QColor color = pair.first.value<QColor>();
			  float r = color.redF();
			  float g = color.greenF();
			  float b = color.blueF();
			  Vec pcolor = Vec(r,g,b);
			  m_trisets[i]->setCropBorderColor(pcolor);
			}
		      else if (keys[ik] == "flip normals")
			m_trisets[i]->setFlipNormals(pair.first.toBool());
		      else if (keys[ik] == "screendoor transparency")
			m_trisets[i]->setScreenDoor(pair.first.toBool());
		      else if (keys[ik] == "opacity")
			m_trisets[i]->setOpacity(pair.first.toDouble());
		      else if (keys[ik] == "ambient")
			m_trisets[i]->setAmbient(pair.first.toDouble());
		      else if (keys[ik] == "diffuse")
			m_trisets[i]->setDiffuse(pair.first.toDouble());
		      else if (keys[ik] == "specular")
			m_trisets[i]->setSpecular(pair.first.toDouble());
		      else if (keys[ik] == "pointmode")
			m_trisets[i]->setPointMode(pair.first.toBool());
		      else if (keys[ik] == "pointsize")
			m_trisets[i]->setPointSize(pair.first.toInt());
		      else if (keys[ik] == "pointstep")
			m_trisets[i]->setPointStep(pair.first.toInt());
		      else if (keys[ik] == "shadows")
			m_trisets[i]->setShadows(pair.first.toBool());
		      else if (keys[ik] == "blend with volume")
			m_trisets[i]->setBlendMode(pair.first.toBool());
		    }
		}
	      
	      return true;
	    }
	}
    }

  return false;
}

void
Trisets::processCommand(int idx, QString cmd)
{
  bool ok;
  cmd = cmd.toLower();
  QStringList list = cmd.split(" ", QString::SkipEmptyParts);
  
  if (list[0] == "setopacity" || list[0] == "opacity")
    {
      if (list.size() == 2)
	{
	  float op = list[1].toFloat(&ok);
	  op = qBound(0.0f, op, 1.0f);
	  m_trisets[idx]->setOpacity(op);
	}
      else
	QMessageBox::critical(0, "Triset Command Error",
				 "value not specified");
    }
  else if (list[0] == "setcolor" || list[0] == "color")
    {
      Vec pcolor = m_trisets[idx]->color();
      QColor dcolor = QColor::fromRgbF(pcolor.x,
				       pcolor.y,
				       pcolor.z);
      QColor color = DColorDialog::getColor(dcolor);
      if (color.isValid())
	{
	  float r = color.redF();
	  float g = color.greenF();
	  float b = color.blueF();
	  pcolor = Vec(r,g,b);
	  m_trisets[idx]->setColor(pcolor);
	}
    }
  else if (list[0] == "setambient" || list[0] == "ambient")
    {
      if (list.size() == 2)
	{
	  float amb = list[1].toFloat(&ok);
	  amb = qBound(0.0f, amb, 1.0f);
	  m_trisets[idx]->setAmbient(amb);
	}
      else
	QMessageBox::critical(0, "Triset Command Error",
				 "value not specified");
    }
  else if (list[0] == "setdiffuse" || list[0] == "diffuse")
    {
      if (list.size() == 2)
	{
	  float diff = list[1].toFloat(&ok);
	  diff = qBound(0.0f, diff, 1.0f);
	  m_trisets[idx]->setDiffuse(diff);
	}
      else
	QMessageBox::critical(0, "Triset Command Error",
				 "value not specified");
    }
  else if (list[0] == "setspecular" || list[0] == "specular")
    {
      if (list.size() == 2)
	{
	  float shine = list[1].toFloat(&ok);
	  shine = qBound(0.0f, shine, 1.0f);
	  m_trisets[idx]->setSpecular(shine);
	}
      else
	QMessageBox::critical(0, "Triset Command Error",
				 "value not specified");
    }
  else if (list[0] == "setpointsize" || list[0] == "pointsize")
    {
      if (list.size() == 2)
	{
	  int ps = list[1].toInt(&ok);
	  ps = qBound(0, ps, 128);
	  m_trisets[idx]->setPointSize(ps);
	}
      else
	QMessageBox::critical(0, "Triset Command Error",
				 "value not specified");
    }
  else if (list[0] == "setpointstep" || list[0] == "pointstep")
    {
      if (list.size() == 2)
	{
	  int ps = list[1].toInt(&ok);
	  ps = qBound(0, ps, 1000);
	  m_trisets[idx]->setPointStep(ps);
	}
      else
	QMessageBox::critical(0, "Triset Command Error",
				 "value not specified");
    }


}

void
Trisets::setClipDistance0(float e0, float e1, float e2, float e3)
{
  if (m_trisets.count()==0) return;
  glUniform4fARB(m_defaultParm[8], e0, e1, e2, e3);
  glUniform4fARB(m_highqualityParm[8], e0, e1, e2, e3);
  glUniform4fARB(m_shadowParm[8], e0, e1, e2, e3);
}

void
Trisets::setClipDistance1(float e0, float e1, float e2, float e3)
{
  if (m_trisets.count()==0) return;
  glUniform4fARB(m_defaultParm[9], e0, e1, e2, e3);
  glUniform4fARB(m_highqualityParm[9], e0, e1, e2, e3);
  glUniform4fARB(m_shadowParm[9], e0, e1, e2, e3);
}

void
Trisets::createDefaultShader(QList<CropObject> crops)
{
  if (m_geoDefaultShader)
    glDeleteObjectARB(m_geoDefaultShader);

  QString shaderString;
  shaderString = GeoShaderFactory::genDefaultShaderString(crops);

  m_geoDefaultShader = glCreateProgramObjectARB();
  if (! GeoShaderFactory::loadShader(m_geoDefaultShader,
				     shaderString))
    exit(0);

  m_defaultParm[0] = glGetUniformLocationARB(m_geoDefaultShader, "eyepos");
  m_defaultParm[1] = glGetUniformLocationARB(m_geoDefaultShader, "screenDoor");
  m_defaultParm[2] = glGetUniformLocationARB(m_geoDefaultShader, "cropBorder");
  m_defaultParm[3] = glGetUniformLocationARB(m_geoDefaultShader, "nclip");
  m_defaultParm[4] = glGetUniformLocationARB(m_geoDefaultShader, "clipPos");
  m_defaultParm[5] = glGetUniformLocationARB(m_geoDefaultShader, "clipNormal");

  m_defaultParm[8] = glGetUniformLocationARB(m_geoDefaultShader, "ClipPlane0");
  m_defaultParm[9] = glGetUniformLocationARB(m_geoDefaultShader, "ClipPlane1");
}

void
Trisets::createHighQualityShader(bool shadows,
				 float shadowIntensity,
				 QList<CropObject> crops)
{
  if (m_geoHighQualityShader)
    glDeleteObjectARB(m_geoHighQualityShader);

  QString shaderString;
  shaderString = GeoShaderFactory::genHighQualityShaderString(shadows,
							      shadowIntensity,
							      crops);
  m_geoHighQualityShader = glCreateProgramObjectARB();
  if (! GeoShaderFactory::loadShader(m_geoHighQualityShader,
				     shaderString))
    exit(0);

  m_highqualityParm[0] = glGetUniformLocationARB(m_geoHighQualityShader, "eyepos");
  m_highqualityParm[1] = glGetUniformLocationARB(m_geoHighQualityShader, "shadowTex");
  m_highqualityParm[2] = glGetUniformLocationARB(m_geoHighQualityShader, "screenDoor");
  m_highqualityParm[3] = glGetUniformLocationARB(m_geoHighQualityShader, "cropBorder");
  m_highqualityParm[4] = glGetUniformLocationARB(m_geoHighQualityShader, "nclip");
  m_highqualityParm[5] = glGetUniformLocationARB(m_geoHighQualityShader, "clipPos");
  m_highqualityParm[6] = glGetUniformLocationARB(m_geoHighQualityShader, "clipNormal");

  m_highqualityParm[8] = glGetUniformLocationARB(m_geoHighQualityShader, "ClipPlane0");
  m_highqualityParm[9] = glGetUniformLocationARB(m_geoHighQualityShader, "ClipPlane1");
}

void
Trisets::createShadowShader(Vec attenuation, QList<CropObject> crops)
{
  if (m_geoShadowShader)
    glDeleteObjectARB(m_geoShadowShader);

  float r = attenuation.x;
  float g = attenuation.y;
  float b = attenuation.z;

  QString shaderString;
  shaderString = GeoShaderFactory::genShadowShaderString(r,g,b, crops);

  m_geoShadowShader = glCreateProgramObjectARB();
  if (! GeoShaderFactory::loadShader(m_geoShadowShader,
				     shaderString))
    exit(0);

  m_shadowParm[0] = glGetUniformLocationARB(m_geoShadowShader, "nclip");
  m_shadowParm[1] = glGetUniformLocationARB(m_geoShadowShader, "clipPos");
  m_shadowParm[2] = glGetUniformLocationARB(m_geoShadowShader, "clipNormal");

  m_shadowParm[8] = glGetUniformLocationARB(m_geoShadowShader, "ClipPlane0");
  m_shadowParm[9] = glGetUniformLocationARB(m_geoShadowShader, "ClipPlane1");
}

void
Trisets::save(const char *flnm)
{
  QDomDocument doc;
  QFile f(flnm);
  if (f.open(QIODevice::ReadOnly))
    {
      doc.setContent(&f);
      f.close();
    }

  QDomElement topElement = doc.documentElement();

  for(int i=0; i<m_trisets.count(); i++)
    {
      QDomElement de = m_trisets[i]->domElement(doc);
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
Trisets::load(const char *flnm)
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
      if (dlist.at(i).nodeName() == "triset")
	{
	  QDomElement de = dlist.at(i).toElement();

	  TrisetGrabber *tg = new TrisetGrabber();
	  if (tg->fromDomElement(de))	    
	    m_trisets.append(tg);
	  else
	    delete tg;
	}
    }

}

QList<TrisetInformation>
Trisets::get()
{
  QList<TrisetInformation> tinfo;

  for(int i=0; i<m_trisets.count(); i++)
    tinfo.append(m_trisets[i]->get());

  return tinfo;
}

void
Trisets::set(QList<TrisetInformation> tinfo)
{
  if (tinfo.count() == 0)
    {
      clear();
      return;
    }
    
  if (m_trisets.count() == 0)
    {
      for(int i=0; i<tinfo.count(); i++)
	{
	  TrisetGrabber *tgi = new TrisetGrabber();	
	  if (tgi->set(tinfo[i]))
	    m_trisets.append(tgi);
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
      for(int j=0; j<m_trisets.count(); j++)
	{
	  if (tinfo[i].filename == m_trisets[j]->filename())
	    {
	      present[i] = j;
	      break;
	    }
	}
    }

  QList<TrisetGrabber*> tg;
  tg = m_trisets;
  
  m_trisets.clear();
  for(int i=0; i<tinfo.count(); i++)
    {
      TrisetGrabber *tgi;

      if (present[i] >= 0)
	tgi = tg[present[i]];
      else
	tgi = new TrisetGrabber();
	
      if (tgi->set(tinfo[i]))
	m_trisets.append(tgi);
      else
	delete tgi;
    }

  for(int i=0; i<tg.count(); i++)
    {
      if (! m_trisets.contains(tg[i]))
	delete tg[i];
    }

  tg.clear();
}

void
Trisets::makeReadyForPainting(QGLViewer *viewer)
{
  // handle only first one
  if (m_trisets.count() > 0)
    m_trisets[0]->makeReadyForPainting(viewer);
}

void
Trisets::releaseFromPainting()
{
  // handle only first one
  if (m_trisets.count() > 0)
    m_trisets[0]->releaseFromPainting();
}

void
Trisets::paint(QGLViewer *viewer,
	       QBitArray doodleMask,
	       float *doodleDepth,
	       Vec tcolor, float tmix)
{
  // handle only first one
  if (m_trisets.count() > 0)
    m_trisets[0]->paint(viewer, doodleMask, doodleDepth, tcolor, tmix);
}
