#include "global.h"
#include "shaderfactory.h"
#include "staticfunctions.h"
#include "dcolordialog.h"
#include "trisets.h"
#include "geoshaderfactory.h"
#include "propertyeditor.h"
#include "captiondialog.h"

#include <QFileDialog>

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
  m_depthTex[2] = 0;
  m_depthTex[3] = 0;
  m_rbo = 0;

  m_vertexScreenBuffer = 0;

  m_blur = 0;
  m_edges = 0;

  memset(&m_scrGeo[0], 0, 8*sizeof(float));

  m_solidTexName.clear();
  m_solidTexData.clear();
  m_solidTex = 0;
}

Trisets::~Trisets()
{
  clear();
  delete [] m_cpos;
  delete [] m_cnormal;

  if (m_depthBuffer) glDeleteFramebuffers(1, &m_depthBuffer);
  if (m_depthTex[0]) glDeleteTextures(4, m_depthTex);
  if (m_rbo) glDeleteRenderbuffers(1, &m_rbo);

  m_depthBuffer = 0;
  m_depthTex[0] = 0;
  m_depthTex[1] = 0;
  m_depthTex[2] = 0;
  m_depthTex[3] = 0;
  m_rbo = 0;

  if (m_solidTexName.count() > 0)
    {
      glDeleteTextures(m_solidTexName.size(), m_solidTex);
      delete [] m_solidTex;
      m_solidTex = 0;
      m_solidTexName.clear();
      foreach(uchar* td, m_solidTexData)
	{
	  delete [] td;
	}
      m_solidTexData.clear();
    }
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

  loadSolidTextures();
}

void
Trisets::checkMouseHover(QGLViewer *viewer)
{
  // using checkMouseHover instead of checkIfGrabsMouse
  
  if (m_trisets.count() == 0)
    return;
  
  QPoint scr = viewer->mapFromGlobal(QCursor::pos());
  int x = scr.x();
  int y = scr.y();
  int gt = -1;
  float dmin = 1000000;
  float cmin = 1000000;
  for(int i=0; i<m_trisets.count(); i++)
    {
      if (m_trisets[i]->grabsMouse())
	{
	  if (!m_trisets[i]->mousePressed())
	    {
	      Vec d = m_trisets[i]->checkForMouseHover(x,y, viewer->camera());
	      if (d.x > -1 && d.y > 0)
		{
		  gt = i;
		  cmin = d.x;
		  dmin = d.y;
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
	      Vec d = m_trisets[i]->checkForMouseHover(x,y, viewer->camera());
	      if (d.x > -1 && d.y > 0)
		{
		  gt = i;
		  cmin = d.x;
		  dmin = d.y;
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
	      Vec d = m_trisets[i]->checkForMouseHover(x,y, viewer->camera());
	      if (d.x > -1 && d.y > 0)
		{
		  if (d.y < dmin)
		    {
		      gt = i;
		      cmin = d.x;
		      dmin = d.y;
		    }
		}
	    }
	}
      
      m_trisets[gt]->setMouseGrab(true);
    }
}

void
Trisets::predraw(QGLViewer *viewer, double *Xform)
{
  if (m_trisets.count() == 0)
    return;
  
//  Vec smin, smax;
//  allEnclosingBox(smin, smax);
//  viewer->setSceneBoundingBox(smin, smax);

  for(int i=0; i<m_trisets.count();i++)
    m_trisets[i]->predraw(viewer,
			  m_trisets[i]->grabsMouse(),
			  Xform);
}

void
Trisets::postdraw(QGLViewer *viewer)
{
  if (m_trisets.count() == 0)
    return;
  
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
Trisets::render(Camera *camera, int nclip)
{
  bool glowOn = false;
  for(int i=0; i<m_trisets.count();i++)
    {      
      if (m_trisets[i]->glow() > 0.01)
	{
	  glowOn = true;
	  break;
	}
    }
  
  for(int i=0; i<m_trisets.count();i++)
    {      
      if (m_trisets[i]->opacity() > 0.9 && // opaque
	  m_trisets[i]->reveal() >= 0.0) // or reveal
	{

	  Vec extras = Vec(0,0,0);
	  if (m_trisets[i]->grabsMouse())
	    extras.x = 1;
      
	  extras.y = 1.0-m_trisets[i]->reveal();
	  
	  extras.z = m_trisets[i]->glow();
	  
	  float darken = 1;
	  if (glowOn && m_trisets[i]->glow() < 0.01)
	    darken = 1.0-0.7*m_trisets[i]->dark();
	  
	  glUseProgram(ShaderFactory::meshShader());
	  GLint *meshShaderParm = ShaderFactory::meshShaderParm();        
	  
	  glUniform4f(meshShaderParm[2], extras.x, extras.y, extras.z, darken);
	  
	  glUniform1f(meshShaderParm[17], i+1);
	  
	  Vec pat = m_trisets[i]->pattern();
	  if (pat.x > 0.5)
	    {
	      glUniform3f(meshShaderParm[18], pat.x, pat.y, pat.z);
	      int patId = qCeil(pat.x) - 1;
	      glActiveTexture(GL_TEXTURE1);
	      glEnable(GL_TEXTURE_3D);
	      glBindTexture(GL_TEXTURE_3D, m_solidTex[patId]);
	      glUniform1i(meshShaderParm[19], 1); // solidTex
	    }
	  else
	    glUniform3f(meshShaderParm[18], 0,0,0);
	  
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
	  
	  
	  m_trisets[i]->draw(camera,
			     m_trisets[i]->grabsMouse());
	  
	  
	  if (pat.x > 0.5)
	    glDisable(GL_TEXTURE_3D);
	  
	  
	  glUseProgramObjectARB(0);
	}
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

  
  
  
  GLint drawFboId = 0, readFboId = 0;
  glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFboId);
  glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readFboId);


  
  //--------------------------
  // find min and max limits
  float trisetExtent = 0.0;
  {
    for(int i=0; i<m_trisets.count();i++)
      {
	Vec bmin, bmax;
	m_trisets[i]->enclosingBox(bmin, bmax);      
	trisetExtent = qMax(trisetExtent, (float)((bmax-bmin).norm()));
      }
  }
  //--------------------------
  
  
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

  int wd = viewer->camera()->screenWidth();
  int ht = viewer->camera()->screenHeight();

  if (!m_depthBuffer)
    createFBO(wd, ht);


  float sceneRadius = trisetExtent;

  Vec vd = viewer->camera()->viewDirection();
  
//  if (!applyShadows)
//    {
//      glUseProgram(ShaderFactory::meshShader());
//      
//      GLint *meshShaderParm = ShaderFactory::meshShaderParm();        
//      
//      GLfloat mv[16];
//      viewer->camera()->getModelViewMatrix(mv);
//      //glUniformMatrix4fv(meshShaderParm[4], 1, GL_FALSE, mv);
//      glUniformMatrix4fv(meshShaderParm[16], 1, GL_FALSE, mv);
//      
//      glUniform1f(meshShaderParm[12], sceneRadius);
//      
//      glUniform3f(meshShaderParm[1], vd.x, vd.y, vd.z); // view direction
//
//
//      render(viewer->camera(), nclip);
//      
//      return;
//    }
  


  glUseProgram(ShaderFactory::meshShader());

  GLint *meshShaderParm = ShaderFactory::meshShaderParm();        


  glUniform1f(meshShaderParm[12], sceneRadius);

  glUniform3f(meshShaderParm[1], vd.x, vd.y, vd.z); // view direction


  //--------------------------------------------
  // first render from shadowCamera
  Camera shadowCamera;
  shadowCamera = *(viewer->camera());  
  Vec vR = shadowCamera.rightVector();
  Vec vU = shadowCamera.upVector();
  shadowCamera.setPosition(shadowCamera.position() + vd*viewer->camera()->sceneRadius()*0.02);
  //shadowCamera.setPosition(shadowCamera.position() + vd*sceneRadius*0.1);

  GLfloat mv[16];
  shadowCamera.getModelViewMatrix(mv);
  glUniformMatrix4fv(meshShaderParm[16], 1, GL_FALSE, mv);

  renderFromShadowCamera(&shadowCamera, nclip);
  //--------------------------------------------

  
  //--------------------------------------------
  // now render from actual camera
  renderFromCamera(viewer->camera(), nclip);
  //--------------------------------------------
  

  //--------------------------------------------
  // render shadows
  renderShadows(drawFboId, wd, ht);
  //--------------------------------------------

  
  //--------------------------------------------
  // draw transparent surfaces if any
  renderTransparent(drawFboId, viewer, nclip, sceneRadius);
  //--------------------------------------------
}

void
Trisets::renderFromCamera(Camera *camera, int nclip)
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
  
  render(camera, nclip);
}

void
Trisets::renderFromShadowCamera(Camera *camera, int nclip)
{
  glBindFramebuffer(GL_FRAMEBUFFER, m_depthBuffer);
  glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
			 GL_COLOR_ATTACHMENT0,
			 GL_TEXTURE_RECTANGLE,
			 m_depthTex[2],
			 0);
  glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
			 GL_COLOR_ATTACHMENT1,
			 GL_TEXTURE_RECTANGLE,
			 m_depthTex[3],
			 0);
  GLenum buffers[2] = { GL_COLOR_ATTACHMENT0_EXT,
			GL_COLOR_ATTACHMENT1_EXT };
  glDrawBuffersARB(2, buffers);
  
  glClearDepth(1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  
  glDisable(GL_BLEND);
  
  render(camera, nclip);
}

void
Trisets::renderShadows(GLint drawFboId, int wd, int ht)
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
  
  glActiveTexture(GL_TEXTURE2);
  glEnable(GL_TEXTURE_RECTANGLE);
  glBindTexture(GL_TEXTURE_RECTANGLE, m_depthTex[2]); // colors
  
  glActiveTexture(GL_TEXTURE3);
  glEnable(GL_TEXTURE_RECTANGLE);
  glBindTexture(GL_TEXTURE_RECTANGLE, m_depthTex[3]); // depth
  
  QMatrix4x4 mvp;
  mvp.setToIdentity();
  mvp.ortho(0.0, wd, 0.0, ht, 0.0, 1.0);
  
  glUniformMatrix4fv(shadowParm[0], 1, GL_FALSE, mvp.data());
  
  glUniform1i(shadowParm[1], 0); // colors
  glUniform1i(shadowParm[2], 1); // actual-to-shadow depth
  glUniform1f(shadowParm[3], m_blur); // soft shadows
  glUniform1f(shadowParm[4], m_edges); // edge enhancement
  glUniform1f(shadowParm[5], Global::gamma()); // edge enhancement
  float roughness = 0.9-m_trisets[0]->roughness()*0.1;
  glUniform1f(shadowParm[6], roughness); // specularity
  glUniform1f(shadowParm[7], m_trisets[0]->specular()); // specularity
  glUniform1i(shadowParm[8], 2); // shadow colors
  glUniform1i(shadowParm[9], 3); // shadow depth
  
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
  
  glActiveTexture(GL_TEXTURE2);
  glDisable(GL_TEXTURE_RECTANGLE);
  
  glActiveTexture(GL_TEXTURE3);
  glDisable(GL_TEXTURE_RECTANGLE);
  
  glUseProgram(0);
  
  glEnable(GL_BLEND);
}



bool
Trisets::handleDialog(int i)
{
  PropertyEditor propertyEditor;
  QMap<QString, QVariantList> plist;
  
  QVariantList vlist;
  
  Vec pos = m_trisets[i]->position();
  vlist.clear();
  vlist << QVariant("string");
  vlist << QVariant(QString("%1 %2 %3").arg(pos.x).arg(pos.y).arg(pos.z));
  plist["position"] = vlist;
  
  Vec scl = m_trisets[i]->scale();
  vlist.clear();
  vlist << QVariant("string");
  vlist << QVariant(QString("%1 %2 %3").arg(scl.x).arg(scl.y).arg(scl.z));
  plist["scale"] = vlist;
  
  vlist.clear();
  vlist << QVariant("double");
  vlist << QVariant(m_trisets[i]->opacity());
  vlist << QVariant(0.1);
  vlist << QVariant(1.0);
  vlist << QVariant(0.1); // singlestep
  vlist << QVariant(1); // decimals
  plist["opacity"] = vlist;
  
  vlist.clear();
  vlist << QVariant("double");
  vlist << QVariant(m_trisets[i]->reveal());
  vlist << QVariant(-1.0);
  vlist << QVariant(1.0);
  vlist << QVariant(0.1); // singlestep
  vlist << QVariant(1); // decimals
  plist["reveal"] = vlist;
  
  vlist.clear();
  vlist << QVariant("double");
  vlist << QVariant(m_trisets[i]->glow());
  vlist << QVariant(0.0);
  vlist << QVariant(5.0);
  vlist << QVariant(0.1); // singlestep
  vlist << QVariant(1); // decimals
  plist["glow"] = vlist;
  
  vlist.clear();
  vlist << QVariant("double");
  vlist << QVariant(m_trisets[i]->dark());
  vlist << QVariant(0.0);
  vlist << QVariant(1.0);
  vlist << QVariant(0.1); // singlestep
  vlist << QVariant(1); // decimals
  plist["darken on glow"] = vlist;
  
  vlist.clear();
  vlist << QVariant("color");
  Vec pcolor = m_trisets[i]->color();
  QColor dcolor = QColor::fromRgbF(pcolor.x,
				   pcolor.y,
				   pcolor.z);
  vlist << dcolor;
  plist["color"] = vlist;
    

  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(m_trisets[i]->clip());
  plist["clip"] = vlist;

  
  Vec pat = m_trisets[i]->pattern();
  vlist.clear();
  vlist << QVariant("string");
  vlist << QVariant(QString("%1 %2 %3").arg(pat.x).arg(pat.y).arg(pat.z));
  plist["pattern"] = vlist;
  

  vlist.clear();
  QString mesg;
  mesg = m_trisets[i]->filename();
  mesg += "\n";
  mesg += QString("Vertices : (%1)    Triangles : (%2)\n").		\
    arg(m_trisets[i]->vertexCount()).arg(m_trisets[i]->triangleCount());
  vlist << mesg;
  plist["message"] = vlist;


	      
  vlist.clear();
  QFile helpFile(":/trisets.help");
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
  keys << "position";
  keys << "scale";
  keys << "clip";
  keys << "color";
  keys << "opacity";
  keys << "reveal";
  keys << "glow";
  keys << "darken on glow";
  keys << "pattern";
  keys << "commandhelp";
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
	      if (pos.count() == 1)
		m_trisets[i]->setPosition(Vec(0, 0, 0));
	      else if (pos.count() == 3)
		m_trisets[i]->setPosition(Vec(pos[0].toDouble(),
					      pos[1].toDouble(),
					      pos[2].toDouble()));
	    }
	  else if (keys[ik] == "scale")
	    {
	      QString vstr = pair.first.toString();
	      QStringList scl = vstr.split(" ");
	      Vec scale = Vec(1,1,1);
	      if (scl.count() > 0) scale.x = scale.y = scale.z = scl[0].toDouble();
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
	  else if (keys[ik] == "opacity")
	    {
	      float r = 0.0;
	      r = pair.first.toString().toDouble();
	      m_trisets[i]->setOpacity(r);
	    }
	  else if (keys[ik] == "reveal")
	    {
	      float r = 0.0;
	      r = pair.first.toString().toDouble();
	      m_trisets[i]->setReveal(r);
	    }
	  else if (keys[ik] == "glow")
	    {
	      float r = 0.0;
	      r = pair.first.toString().toDouble();
	      m_trisets[i]->setGlow(r);
	    }
	  else if (keys[ik] == "darken on glow")
            {
	      float r = 0.0;
	      r = pair.first.toString().toDouble();
	      m_trisets[i]->setDark(r);
	    }
	  else if (keys[ik] == "clip")
	    m_trisets[i]->setClip(pair.first.toBool());
	  else if (keys[ik] == "pattern")
            {
	      QString vstr = pair.first.toString();
	      QStringList pat = vstr.split(" ");
	      if (pat.count() > 0)
                {
                  int texId = pat[0].toDouble();
                  if (texId < 0 || texId > m_solidTexData.count()-1)
		    {
		      QMessageBox::information(0, "", QString("Pattern id %1 is outside range 0-%1").\
					       arg(texId).\
					       arg(m_solidTexData.count()));
		      return true;
		    }
		  if (pat.count() == 1)
		    m_trisets[i]->setPattern(Vec(pat[0].toDouble(), 10, 0.5));
		  else if (pat.count() == 3)
		    m_trisets[i]->setPattern(Vec(pat[0].toDouble(),
						 pat[1].toDouble(),
						 pat[2].toDouble()));
                }
	    }
	}
    }
  
  QString cmd = propertyEditor.getCommandString();
  if (!cmd.isEmpty())
    processCommand(i, cmd);	
  
  return true;
}

bool
Trisets::duplicate(int i)
{
  TrisetInformation ti = m_trisets[i]->get();
  
  QString flnm = ti.filename;
  QDir direc(Global::previousDirectory());
  QString oldflnm = direc.absoluteFilePath(QString(flnm));
  
  QString ext = QFileInfo(flnm).completeSuffix();
  flnm = QFileDialog::getSaveFileName(0,
				      "Save duplicate mesh to file",
				      Global::previousDirectory(),
				      "*."+ext);
  if (flnm.size() == 0)
    {
      QMessageBox::information(0, "", "Mesh not duplicated.");
      return false;
    }
  
  QString newflnm = direc.absoluteFilePath(QString(flnm));
  
  QFile::copy(oldflnm, newflnm);
  ti.filename = flnm;
  
  TrisetGrabber *tg = new TrisetGrabber();
  tg->set(ti);
  
  Vec bmin, bmax;
  tg->enclosingBox(bmin, bmax);
  float rad = (bmax-bmin).norm();
  Vec pos = tg->position();
  pos.x = pos.x + bmax.x-bmin.x;
  tg->setPosition(pos);
  m_trisets.append(tg);
  
  return true;
}

bool
Trisets::keyPressEvent(QKeyEvent *event)
{
  int idx = -1;
  for(int i=0; i<m_trisets.count(); i++)
    {
      if (m_trisets[i]->grabsMouse())
	{
	  idx = i;
	  break;
	}
    }

  if (idx == -1)
    return false;

  
  if (event->modifiers() & Qt::ControlModifier &&
      event->key() == Qt::Key_D)
    duplicate(idx);
  else if (event->key() == Qt::Key_G)
    m_trisets[idx]->removeFromMouseGrabberPool();
  else if (event->key() == Qt::Key_S)
    m_trisets[idx]->save();
  else if (event->key() == Qt::Key_X)
    m_trisets[idx]->setMoveAxis(TrisetGrabber::MoveX);
  else if (event->key() == Qt::Key_Y)
    m_trisets[idx]->setMoveAxis(TrisetGrabber::MoveY);
  else if (event->key() == Qt::Key_Z)
    m_trisets[idx]->setMoveAxis(TrisetGrabber::MoveZ);
  else if (event->key() == Qt::Key_W)
    m_trisets[idx]->setMoveAxis(TrisetGrabber::MoveAll);
  else if (event->key() == Qt::Key_Delete ||
	   event->key() == Qt::Key_Backspace ||
	   event->key() == Qt::Key_Backtab)
    {
      m_trisets[idx]->setMouseGrab(false);
      m_trisets[idx]->removeFromMouseGrabberPool();
      m_trisets.removeAt(idx);
    }
  else if (event->key() == Qt::Key_Space)
    handleDialog(idx);
  
  return true;
}

void
Trisets::processCommand(int idx, QString cmd)
{
  bool ok;
  cmd = cmd.toLower();
  QStringList list = cmd.split(" ", QString::SkipEmptyParts);

  //------------------
  if (list[0] == "caption")
    {
      CaptionDialog cd(0,
		       m_trisets[idx]->captionText(),
		       m_trisets[idx]->captionFont(),
		       m_trisets[idx]->captionColor(),
		       Qt::black,
		       0);
      cd.hideAngle(true);
      cd.move(QCursor::pos());
      if (cd.exec() == QDialog::Accepted)
	{
	  m_trisets[idx]->setCaptionText(cd.text());
	  m_trisets[idx]->setCaptionFont(cd.font());
	  m_trisets[idx]->setCaptionColor(cd.color());
	}

      // set caption position
      QList<Vec> pts;
      if (m_hitpoints->activeCount())
	pts = m_hitpoints->activePoints();
      else
	pts = m_hitpoints->points();
      
      if (pts.count() > 0)
	{
	  // take the lastest hitpoint
	  Vec p = pts[pts.count()-1];
	  
	  m_trisets[idx]->setCaptionPosition(p);
	  
	  pts.removeLast();	    
	  m_hitpoints->setPoints(pts);
	}

      // set caption offsets
      if (list.size() == 3)
	{
	  int x = 100;
	  int y = -100;
	  x = list[1].toInt(&ok);
	  y = list[2].toInt(&ok);
	  m_trisets[idx]->setCaptionOffset(x,y);
	}

      return;
    }
  //------------------

  if (list[0].contains("mirror"))
    {
      if (list[0] == "mirrorx") m_trisets[idx]->mirror(0);
      if (list[0] == "mirrory") m_trisets[idx]->mirror(1);
      if (list[0] == "mirrorz") m_trisets[idx]->mirror(2);
      return;
    }
  
  if (list[0] == "save")
    {
      m_trisets[idx]->save();
      return;
    }

  if (list[0] == "resetposition")
    {
      m_trisets[idx]->setPosition(Vec(0,0,0));
      return;
    }
  
  if (list[0] == "tagcolors")
    {
      uchar *tc = Global::tagColors();
      for (int t=0; t<m_trisets.count(); t++)
	{
	  int i = t+1; // don't want to use tag color 0
	  Vec color(tc[4*i+0]/255.0, tc[4*i+1]/255.0, tc[4*i+2]/255.0);
	  m_trisets[t]->setColor(color);
	}
      return;
    }
  
  if (list[0] == "resetrotation")
    {
      m_trisets[idx]->resetRotation();
      return;
    }

  if (list[0] == "resetpositions")
    {
      for (int i=0; i<m_trisets.count(); i++)
	{
	  m_trisets[i]->setPosition(Vec(0,0,0));
	}
      return;
    }

  if (list[0] == "rotate" ||
      list[0] == "rotatex" ||
      list[0] == "rotatey" ||
      list[0] == "rotatez")
    {
      Quaternion rot;
      float x=0,y=0,z=0,a=0;
      if (list[0] == "rotate")
	{
	  if (list.size() > 1) x = list[1].toFloat(&ok);
	  if (list.size() > 2) y = list[2].toFloat(&ok);
	  if (list.size() > 3) z = list[3].toFloat(&ok);
	  if (list.size() > 4) a = list[4].toFloat(&ok);
	  rot = Quaternion(Vec(x,y,z), DEG2RAD(a));
	}
      else
	{
	  float a=0;
	  if (list.size() > 1) a = list[1].toFloat(&ok);
	  if (list[0] == "rotatex")
	    rot = Quaternion(Vec(1,0,0), DEG2RAD(a));
	  else if (list[0] == "rotatey")
	    rot = Quaternion(Vec(0,1,0), DEG2RAD(a));
	  else if (list[0] == "rotatez")
	    rot = Quaternion(Vec(0,0,1), DEG2RAD(a));
	}
      
      m_trisets[idx]->rotate(rot);

      return;
    }
  
  if (list[0] == "darkenall")
    {
      float dark = 0.5;
      if (list.count() > 1)
	dark = qBound(0.0f, list[1].toFloat(&ok), 1.0f);
      
      for (int i=0; i<m_trisets.count(); i++)
	{
	  m_trisets[i]->setDark(dark);
	}
      return;
    }
  
  if (list[0] == "glowall")
    {
      float glow = 0.0;
      if (list.count() > 1)
	glow = qBound(0.0f, list[1].toFloat(&ok), 5.0f);
      
      for (int i=0; i<m_trisets.count(); i++)
	{
	  m_trisets[i]->setGlow(glow);
	}
      return;
    }
  
  if (list[0] == "clipall")
    {
      bool clip = true;
      if (list.count() > 1)
	clip = list[1].toFloat(&ok) > 0.1;
      
      for (int i=0; i<m_trisets.count(); i++)
	{
	  m_trisets[i]->setClip(clip);
	}
      return;
    }

  if (list[0] == "activescale")
    {
      float scl = 1.0;
      if (list.count() > 1)
	scl = list[1].toFloat(&ok);
      
      for (int i=0; i<m_trisets.count(); i++)
	{
	  m_trisets[i]->setActiveScale(scl);
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

//  if (list[0] == "setcolor" || list[0] == "color")
//    {
//      Vec pcolor = m_trisets[idx]->color();
//      QColor dcolor = QColor::fromRgbF(pcolor.x,
//				       pcolor.y,
//				       pcolor.z);
//      QColor color = DColorDialog::getColor(dcolor);
//      if (color.isValid())
//	{
//	  float r = color.redF();
//	  float g = color.greenF();
//	  float b = color.blueF();
//	  pcolor = Vec(r,g,b);
//	  m_trisets[idx]->setColor(pcolor);
//	}
//    }
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

  loadSolidTextures();
    
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
  //if (m_vertexScreenBuffer) glDeleteBuffers(1, &m_vertexScreenBuffer);
  if (!m_vertexScreenBuffer) glGenBuffers(1, &m_vertexScreenBuffer);
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
  glGenTextures(4, m_depthTex);
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

  for(int dt=0; dt<4; dt++)
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

void
Trisets::loadSolidTextures()
{  
  if (m_solidTexName.count() > 0)
    return;

  
  //QString texdir = qApp->applicationDirPath() + QDir::separator() + "textures";
  QString texdir = qApp->applicationDirPath() + QDir::separator()  + "assets" + QDir::separator() + "textures";
  QDir dir(texdir);
  QStringList filters;
  filters << "*.tex";
  dir.setNameFilters(filters);
  dir.setFilter(QDir::Files |
		QDir::NoSymLinks |
		QDir::NoDotAndDotDot);


  QFileInfoList list = dir.entryInfoList();
  if (list.size() == 0)
    return;

  int texSize;
  QString flnms;
  for (int i=0; i<list.size(); i++)
    {
      QString flnm = list.at(i).absoluteFilePath();
      m_solidTexName << flnm;

      QFile fd(flnm);
      fd.open(QFile::ReadOnly);

      int vs;
      fd.read((char*)&vs, 4);

      texSize = vs; // assuming all textures are of same size
      
      uchar *td = new uchar[vs*vs*vs*3];
      fd.read((char*)td, vs*vs*vs*3);
      fd.close();

      m_solidTexData << td;
      
      flnms += flnm;
      flnms += "\n";
    }
  
  m_solidTex = new GLuint[m_solidTexData.count()];
  glGenTextures(m_solidTexData.count(), m_solidTex);
  
  for (int i=0; i<m_solidTexData.count(); i++)
    {
      glActiveTexture(GL_TEXTURE1);
      glEnable(GL_TEXTURE_3D);
      glBindTexture(GL_TEXTURE_3D, m_solidTex[i]);
      glTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER); 
      glTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER); 
      glTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexImage3D(GL_TEXTURE_3D,
		   0, // single resolution
		   GL_RGB,
		   texSize, texSize, texSize,
		   0, // no border
		   GL_RGB,
		   GL_UNSIGNED_BYTE,
		   m_solidTexData[i]);
      glDisable(GL_TEXTURE_3D);
    }
}


void
Trisets::bindOITTextures()
{
  // don't write to depth buffer
  // we want all non occluded transparent fragments
  glDepthMask(GL_FALSE);

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
  GLenum buffers[2] = { GL_COLOR_ATTACHMENT0,
			GL_COLOR_ATTACHMENT1 };
  glDrawBuffers(2, buffers);

  GLfloat c0[4] = {0,0,0,0};
  GLfloat c1[4] = {1,1,1,1};
  
  glClear(GL_COLOR_BUFFER_BIT);
  glClearBufferfv(GL_COLOR, 0, c0);
  glClearBufferfv(GL_COLOR, 1, c1);  

  glEnable(GL_BLEND);
  glBlendEquation(GL_FUNC_ADD);
  glBlendFunci(0, GL_ONE, GL_ONE);
  glBlendFunci(1, GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
}

void
Trisets::renderTransparent(GLint drawFboId,
			   QGLViewer *viewer,
			   int nclip,
			   float sceneRadius)
{
  bool opOn = false;
  for(int i=0; i<m_trisets.count();i++)
    {      
      if (m_trisets[i]->opacity() < 1.0 || // transparent
	  m_trisets[i]->reveal() < 0.0) // or transparent reveal
	{
	  opOn = true;
	  break;
	}
    }

  if (!opOn)
    return;

  
  bool glowOn = false;
  for(int i=0; i<m_trisets.count();i++)
    {      
      if (m_trisets[i]->glow() > 0.01)
	{
	  glowOn = true;
	  break;
	}
    }
  

  
  bindOITTextures();
  
  
  glUseProgram(ShaderFactory::oitShader());
  
  GLint *oitShaderParm = ShaderFactory::oitShaderParm();        
  
  Vec vd = viewer->camera()->viewDirection();
  
  glUniform3f(oitShaderParm[1], vd.x, vd.y, vd.z); // view direction

  GLfloat mv[16];
  viewer->camera()->getModelViewMatrix(mv);

  glUniformMatrix4fv(oitShaderParm[4], 1, GL_FALSE, mv);

  glUniform1f(oitShaderParm[12], sceneRadius);
  
  for(int i=0; i<m_trisets.count();i++)
    {
      if (m_trisets[i]->opacity() < 1.0 || // transparent
	  m_trisets[i]->reveal() < 0.0) // or transparent reveal
	{
	  glUniform1f(oitShaderParm[16], m_trisets[i]->opacity()); // opacity

	  Vec extras = Vec(0,0,0);
	  if (m_trisets[i]->grabsMouse())
	    extras.x = 1;
      
	  extras.y = m_trisets[i]->reveal();
	  
	  extras.z = m_trisets[i]->glow();

	  float darken = 1;
	  if (glowOn && m_trisets[i]->glow() < 0.01)
	    darken = 1.0-0.7*m_trisets[i]->dark();
	  
	  glUseProgram(ShaderFactory::oitShader());
	  GLint *oitShaderParm = ShaderFactory::oitShaderParm();        
	  
	  glUniform4f(oitShaderParm[2], extras.x, extras.y, extras.z, darken);
	  
	  glUniform1f(oitShaderParm[17], i+1);
	  
	  Vec pat = m_trisets[i]->pattern();
	  if (pat.x > 0.5)
	    {
	      glUniform3f(oitShaderParm[18], pat.x, pat.y, pat.z);
	      int patId = qCeil(pat.x) - 1;
	      glActiveTexture(GL_TEXTURE1);
	      glEnable(GL_TEXTURE_3D);
	      glBindTexture(GL_TEXTURE_3D, m_solidTex[patId]);
	      glUniform1i(oitShaderParm[19], 1); // solidTex
	    }
	  else
	    glUniform3f(oitShaderParm[18], 0,0,0);
	  
	  if (m_trisets[i]->clip())
	    {
	      glUniform1iARB(oitShaderParm[9],  nclip);
	      glUniform3fvARB(oitShaderParm[10], nclip, m_cpos);
	      glUniform3fvARB(oitShaderParm[11], nclip, m_cnormal);
	    }
	  else
	    {
	      glUniform1iARB(oitShaderParm[9],  0);
	    }
	  

	  m_trisets[i]->drawOIT(viewer->camera(),
				m_trisets[i]->grabsMouse());
	  
	  
	  if (pat.x > 0.5)
	    glDisable(GL_TEXTURE_3D);
	  
	  
	  glUseProgramObjectARB(0);
	}
    }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  glDisable(GL_BLEND);


  // ------------------
  // ------------------


  int wd = viewer->camera()->screenWidth();
  int ht = viewer->camera()->screenHeight();

  glBindFramebuffer(GL_FRAMEBUFFER, drawFboId);

  drawOITTextures(wd, ht);
}


void
Trisets::drawOITTextures(int wd, int ht)
{       
  glDisable(GL_DEPTH_TEST);
  
  glUseProgram(ShaderFactory::oitFinalShader());
  GLint *oitFinalParm = ShaderFactory::oitFinalShaderParm();        

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_TEXTURE_RECTANGLE);
  glBindTexture(GL_TEXTURE_RECTANGLE, m_depthTex[0]);

  glActiveTexture(GL_TEXTURE1);
  glEnable(GL_TEXTURE_RECTANGLE);
  glBindTexture(GL_TEXTURE_RECTANGLE, m_depthTex[1]);
  
  QMatrix4x4 mvp;
  mvp.setToIdentity();
  mvp.ortho(0.0, wd, 0.0, ht, 0.0, 1.0);

  glUniformMatrix4fv(oitFinalParm[0], 1, GL_FALSE, mvp.data());

  glUniform1i(oitFinalParm[1], 0); // accumulation tex
  glUniform1i(oitFinalParm[2], 1); // revealage tex
  float roughness = 0.9-m_trisets[0]->roughness()*0.1;
  glUniform1f(oitFinalParm[3], roughness); // specularity
  glUniform1f(oitFinalParm[4], m_trisets[0]->specular()); // specularity

  
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

  glActiveTexture(GL_TEXTURE0);
  glDisable(GL_TEXTURE_RECTANGLE);

  glActiveTexture(GL_TEXTURE1);
  glDisable(GL_TEXTURE_RECTANGLE);
  

  glUseProgram(0);

  
  glDisable(GL_BLEND);

  glEnable(GL_DEPTH_TEST);  

  glDepthMask(GL_TRUE);
}

