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

  m_depthBuffer = 0;
  m_depthTex[0] = 0;
  m_depthTex[1] = 0;
  m_rbo = 0;

  m_vertexScreenBuffer = 0;

  m_blur = 0;
  m_edges = 0;
}

Trisets::~Trisets()
{
  clear();
  delete [] m_cpos;
  delete [] m_cnormal;

  if (m_depthBuffer) glDeleteFramebuffers(1, &m_depthBuffer);
  if (m_depthTex[0]) glDeleteTextures(2, m_depthTex);
  if (m_rbo) glDeleteRenderbuffers(1, &m_rbo);

  m_depthBuffer = 0;
  m_depthTex[0] = 0;
  m_depthTex[1] = 0;
  m_rbo = 0;
}

void
Trisets::setShapeEnhancements(float blur, float edges)
{
  m_blur = qMax(0, (int)(2*(blur-1)));
  m_edges = edges;
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
Trisets::setLighting(QVector4D l)
{
  for (int i=0; i<m_trisets.count(); i++)
    m_trisets[i]->setLighting(l);
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
Trisets::checkMouseHover(QGLViewer *viewer)
{
  // using checkMouseHover instead of checkIfGrabsMouse
  
  if (m_trisets.count() == 0)
    return;
  
    {
      QPoint scr = viewer->mapFromGlobal(QCursor::pos());
      int x = scr.x();
      int y = scr.y();
      int gt = -1;
      float dmin = 10000;
      for(int i=0; i<m_trisets.count(); i++)
	{
	  if (m_trisets[i]->grabsMouse())
	    {
	      if (!m_trisets[i]->mousePressed())
		{
		  dmin = m_trisets[i]->checkForMouseHover(x,y, viewer->camera());
		  if (dmin > 0)
		    {
		      gt = i;
		      break;
		    }
		}
	      else
		{
		  return;
		}
	    }
	}
      if (gt == -1)
	{
	  for(int i=0; i<m_trisets.count(); i++)
	    {
	      if (m_trisets[i]->isInMouseGrabberPool())
		{
		  float d = m_trisets[i]->checkForMouseHover(x,y, viewer->camera());
		  if (d > 0)
		    {
		      dmin = d;
		      gt = i;
		      break;
		    }
		}
	    }
	}
      
      for(int i=0; i<m_trisets.count();i++)
	m_trisets[i]->setMouseGrab(false);

      if (gt > -1)
	{
	  int pgt = gt;
		
	  for(int i=0; i<m_trisets.count();i++)
	    {
	      if (m_trisets[i]->isInMouseGrabberPool())
		{
		  float d = m_trisets[i]->checkForMouseHover(x,y, viewer->camera());
		  if (d > 0 && d < dmin)
		    {
		      dmin = d;
		      gt = i;
		    }
		}
	    }

	  m_trisets[gt]->setMouseGrab(true);
	}
    }
}

void
Trisets::predraw(QGLViewer *viewer,
		 double *Xform,
		 Vec pn,
		 bool shadows, int shadowWidth, int shadowHeight)
{  
  Vec smin, smax;
  allEnclosingBox(smin, smax);
  viewer->setSceneBoundingBox(smin, smax);

  for(int i=0; i<m_trisets.count();i++)
    m_trisets[i]->predraw(viewer,
			  m_trisets[i]->grabsMouse(),
			  Xform,
			  pn,
			  shadows, shadowWidth, shadowHeight);
}

void
Trisets::postdraw(QGLViewer *viewer)
{
  for(int i=0; i<m_trisets.count();i++)
    {
      int x,y;
      QPoint scr = viewer->mapFromGlobal(QCursor::pos());
      x = scr.x();
      y = scr.y();
      m_trisets[i]->postdraw(viewer,
			     x, y,
			     m_trisets[i]->grabsMouse(),
			     i);
    }
}

void
Trisets::draw(QGLViewer *viewer,
	      Vec lightVec,
	      float pnear, float pfar, Vec step,
	      bool applyShadows, Vec ep,
	      QList<Vec> cpos, QList<Vec> cnormal)
{
  if (m_trisets.count() == 0)
    return;
  

  int wd = viewer->camera()->screenWidth();
  int ht = viewer->camera()->screenHeight();

  if (!m_depthBuffer)
    createFBO(wd, ht);


  
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

  GLint drawFboId = 0, readFboId = 0;
  glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFboId);
  glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readFboId);

  if (applyShadows)
    {
      glBindFramebuffer(GL_FRAMEBUFFER, m_depthBuffer);
      glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
			     GL_COLOR_ATTACHMENT0,
			     GL_TEXTURE_RECTANGLE,
			     m_depthTex[0],
			     0);
      glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
			     GL_COLOR_ATTACHMENT1,
			     GL_TEXTURE_RECTANGLE,
			     m_depthTex[1],
			     0);
      GLenum buffers[2] = { GL_COLOR_ATTACHMENT0_EXT,
			    GL_COLOR_ATTACHMENT1_EXT };
      glDrawBuffersARB(2, buffers);
      
      glClearDepth(1);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      glDisable(GL_BLEND);
    }

  
  glUseProgram(ShaderFactory::meshShader());

  GLint *meshShaderParm = ShaderFactory::meshShaderParm();        

  GLfloat mv[16];
  viewer->camera()->getModelViewMatrix(mv);
  glUniformMatrix4fv(meshShaderParm[4], 1, GL_FALSE, mv);

  float sceneRadius = viewer->sceneRadius();
  glUniform1f(meshShaderParm[12], sceneRadius);

  Vec vd = viewer->camera()->viewDirection();
  glUniform3f(meshShaderParm[1], vd.x, vd.y, vd.z); // view direction

  Camera shadowCamera;
  shadowCamera = *(viewer->camera());  
  Vec vR = shadowCamera.rightVector();
  Vec vU = shadowCamera.upVector();
  shadowCamera.setPosition(shadowCamera.position() + vd*sceneRadius*0.1);
  shadowCamera.lookAt(viewer->camera()->sceneCenter());
  shadowCamera.getModelViewMatrix(mv);
  glUniformMatrix4fv(meshShaderParm[16], 1, GL_FALSE, mv);


  
  for(int i=0; i<m_trisets.count();i++)
    {
      glUseProgram(ShaderFactory::meshShader());
      
      Vec extras = Vec(0,0,0);
      if (m_trisets[i]->grabsMouse())
	extras.x = 1;
      glUniform3f(meshShaderParm[2], extras.x, extras.y, extras.z);

  
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
    }

  // --- draw shadows
  if (applyShadows)
  {
    glBindFramebuffer(GL_FRAMEBUFFER, drawFboId);
    
    glUseProgram(ShaderFactory::meshShadowShader());
    GLint *shadowParm = ShaderFactory::meshShadowShaderParm();        

    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_RECTANGLE);
    glBindTexture(GL_TEXTURE_RECTANGLE, m_depthTex[0]); // colors
    
    glActiveTexture(GL_TEXTURE1);
    glEnable(GL_TEXTURE_RECTANGLE);
    glBindTexture(GL_TEXTURE_RECTANGLE, m_depthTex[1]); // depth
    
    
    QMatrix4x4 mvp;
    mvp.setToIdentity();
    mvp.ortho(0.0, wd, 0.0, ht, 0.0, 1.0);
    
    glUniformMatrix4fv(shadowParm[0], 1, GL_FALSE, mvp.data());
    
    glUniform1i(shadowParm[1], 0); // colors
    glUniform1i(shadowParm[2], 1); // depthTex1
    glUniform1f(shadowParm[3], m_blur); // soft shadows
    glUniform1f(shadowParm[4], m_edges); // edge enhancement
    glUniform1f(shadowParm[5], Global::gamma()); // edge enhancement
    float roughness = 0.9-m_trisets[0]->roughness()*0.1;
    glUniform1f(shadowParm[6], roughness); // specularity
        
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexScreenBuffer);
    glVertexAttribPointer(0,  // attribute 0
			  2,  // size
			  GL_FLOAT, // type
			  GL_FALSE, // normalized
			  0, // stride
			  (void*)0 ); // array buffer offset
    glDrawArrays(GL_QUADS, 0, 8);
    
    glDisableVertexAttribArray(0);
    
    glActiveTexture(GL_TEXTURE1);
    glDisable(GL_TEXTURE_RECTANGLE);
    
    glActiveTexture(GL_TEXTURE0);
    glDisable(GL_TEXTURE_RECTANGLE);

    glUseProgram(0);
    glUseProgramObjectARB(0);

    glEnable(GL_BLEND);
  }
  //--------------------------------------------
  //--------------------------------------------

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
	      keys << "command";
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
	      
	      QString cmd = propertyEditor.getCommandString();
	      if (!cmd.isEmpty())
		processCommand(i, cmd);	
  
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

  if (list[0] == "resetposition")
    {
      for (int i=0; i<m_trisets.count(); i++)
	{
	  m_trisets[i]->setPosition(Vec(0,0,0));
	}
      return;
    }
  
  if (list[0] == "scale")
    {
      float x,y,z;
      x=y=z=1;
      
      if (list.count() == 2)
	x = y = z = list[1].toFloat(&ok);
      else if (list.count() == 4)
	{
	  x = list[1].toFloat(&ok);
	  y = list[2].toFloat(&ok);
	  z = list[3].toFloat(&ok);
	}
      Vec scale(x,y,z);
      for (int i=0; i<m_trisets.count(); i++)
	m_trisets[i]->setScale(scale);
      return;
    }
  
  if (list[0].contains("explode"))
    {
      Vec damp = Vec(1,1,1);
      if (list[0] == "explodex") damp = Vec(1, 0.1, 0.1);
      if (list[0] == "explodey") damp = Vec(0.1, 1, 0.1);
      if (list[0] == "explodez") damp = Vec(0.1, 0.1, 1);
      if (list[0] == "explodexy") damp = Vec(1, 1, 0.5);
      if (list[0] == "explodexz") damp = Vec(0.5, 1, 1);
      if (list[0] == "explodeyz") damp = Vec(0.5, 1, 1);
	
      float rad = 1;
      if (list.count() > 1)
	rad = list[1].toFloat(&ok);

      Vec centroid = Vec(0,0,0);
      for (int i=0; i<m_trisets.count(); i++)
	centroid += m_trisets[i]->centroid();

      centroid /= m_trisets.count();
      
      for (int i=0; i<m_trisets.count(); i++)
	{
	  Vec dr = m_trisets[i]->centroid() - centroid;
	  dr = VECPRODUCT(dr, damp);
	  dr *= rad;
	  m_trisets[i]->setPosition(dr);
	}
      return;
    }

  if (list[0] == "setcolor" || list[0] == "color")
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

void
Trisets::resize(int wd, int ht)
{
  createFBO(wd, ht);
}

void
Trisets::createFBO(int wd, int ht)
{
  //------------------
  m_scrGeo[0] = 0;
  m_scrGeo[1] = 0;
  m_scrGeo[2] = wd;
  m_scrGeo[3] = 0;
  m_scrGeo[4] = wd;
  m_scrGeo[5] = ht;
  m_scrGeo[6] = 0;
  m_scrGeo[7] = ht;
  if (m_vertexScreenBuffer) glDeleteBuffers(1, &m_vertexScreenBuffer);
  glGenBuffers(1, &m_vertexScreenBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, m_vertexScreenBuffer);
  glBufferData(GL_ARRAY_BUFFER,
	       8*sizeof(float),
	       &m_scrGeo,
	       GL_STATIC_DRAW);
  //------------------


  if (m_depthBuffer) glDeleteFramebuffers(1, &m_depthBuffer);
  if (m_depthTex) glDeleteTextures(4, m_depthTex);
  if (m_rbo) glDeleteRenderbuffers(1, &m_rbo);

  glGenFramebuffers(1, &m_depthBuffer);
  glGenTextures(2, m_depthTex);
  glBindFramebuffer(GL_FRAMEBUFFER, m_depthBuffer);

  glGenRenderbuffers(1, &m_rbo);
  glBindRenderbuffer(GL_RENDERBUFFER, m_rbo);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, wd, ht);
  // attach the renderbuffer to depth attachment point
  glFramebufferRenderbuffer(GL_FRAMEBUFFER,      // 1. fbo target: GL_FRAMEBUFFER
			    GL_DEPTH_ATTACHMENT, // 2. attachment point
			    GL_RENDERBUFFER,     // 3. rbo target: GL_RENDERBUFFER
			    m_rbo);              // 4. rbo ID
  glBindRenderbuffer(GL_RENDERBUFFER, 0);

  for(int dt=0; dt<2; dt++)
    {
      glBindTexture(GL_TEXTURE_RECTANGLE, m_depthTex[dt]);
      glTexImage2D(GL_TEXTURE_RECTANGLE,
		   0,
		   GL_RGBA32F,
		   wd, ht,
		   0,
		   GL_RGBA,
		   GL_FLOAT,
		   0);
    }
  glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
  glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

