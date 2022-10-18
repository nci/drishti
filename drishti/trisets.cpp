#include "global.h"
#include "shaderfactory.h"
#include "staticfunctions.h"
#include "dcolordialog.h"
#include "trisets.h"
#include "propertyeditor.h"
#include "captiondialog.h"
//#include "lighthandler.h"

#include <QFileDialog>
#include <QInputDialog>


Trisets::Trisets()
{
  m_trisets.clear();
  m_cpos = new float[100];
  m_cnormal = new float[100];

  m_active = -1;
  m_multiActive.clear();

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
  //m_solidTexData.clear();
  m_solidTex = 0;
  
  m_lightDir = Vec(0.1,0.1,1);
  m_lightDir.normalize();

  m_rotationMode = false;
  m_grab = false;
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
//      foreach(uchar* td, m_solidTexData)
//	{
//	  delete [] td;
//	}
//      m_solidTexData.clear();
    }
}

void
Trisets::setLightDirection(Vec ldir)
{
  m_lightDir = ldir;
}

void
Trisets::setShapeEnhancements(float blur, float edges)
{
  //m_blur = qMax(0, (int)(2*(blur-1)));
  //m_blur = qMax(0, (int)(blur-1));
  m_blur = blur;
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
      emit updateGL();
    }
}

void
Trisets::setShow(QList<bool> flag)
{
  for (int i=0; i<flag.count(); i++)
    {
      m_trisets[i]->setShow(flag[i]);
    }
  emit updateGL();
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
      emit updateGL();
    }
}

void
Trisets::setClip(QList<bool> flag)
{
  for (int i=0; i<flag.count(); i++)
    {
      m_trisets[i]->setClip(flag[i]);
    }
  emit updateGL();
}

void
Trisets::posChanged()
{
  if (m_active < 0)
    return;

  Vec deltaPos = m_trisets[m_active]->position() - m_prevActivePos;
  for (int i=0; i<m_multiActive.count(); i++)
    {
      if (m_multiActive[i] != m_active)
	{
	  Vec tpos = m_trisets[m_multiActive[i]]->position() + deltaPos;
	  m_trisets[m_multiActive[i]]->setPosition(tpos);
	}
    }
  m_prevActivePos = m_trisets[m_active]->position();
}

void
Trisets::meshGrabbed()
{
  for (int i=0; i<m_trisets.count(); i++)
    {
      if (m_trisets[i]->mousePressed())
	{
	  emit meshGrabbed(i);
	  return;
	}
    }
}

void
Trisets::refreshGrab()
{
  setGrab(m_grab);
}

void
Trisets::setGrab(bool flag)
{
  m_grab = flag;
  if (m_grab)
    {
      addInMouseGrabberPool();
      for (int i=0; i<m_trisets.count(); i++)
	{
	  m_trisets[i]->setGrabMode(m_grab);
	}
    }
  else
    {
      removeFromMouseGrabberPool();
      for (int i=0; i<m_trisets.count(); i++)
	{
	  m_trisets[i]->setGrabMode(m_grab);
	}

      if (m_active > -1)
	addInMouseGrabberPool(m_active);	
    }

  emit updateGL();
}

void
Trisets::multiSelection(QList<int> indices)
{
  m_multiActive = indices;

  if (!m_grab)
    {
      removeFromMouseGrabberPool();
      if (m_active > -1)
	addInMouseGrabberPool(m_active);
//for (int i=0; i<m_multiActive.count(); i++)
//	addInMouseGrabberPool(m_multiActive[i]);
    }

  emit updateGL();
}

void
Trisets::setActive(int idx, bool flag)
{
  if (flag)
    {
      m_active = idx;
      addInMouseGrabberPool(idx);
      sendParametersToMenu();
      m_prevActivePos = m_trisets[m_active]->position();
    }
  else
    {
      m_active = -1;
      if (!m_grab)
	removeFromMouseGrabberPool(idx);
    }

  emit updateGL();
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
    {
      m_trisets.append(tg);

      connect(tg, SIGNAL(updateParam()),
	      this, SLOT(sendParametersToMenu()));
      connect(tg, SIGNAL(meshGrabbed()),
	      this, SLOT(meshGrabbed()));
      connect(tg, SIGNAL(posChanged()),
	      this, SLOT(posChanged()));
      tg->setRotationMode(m_rotationMode);
    }
  else
    delete tg;
}

void
Trisets::addMesh(QString flnm)
{
  TrisetGrabber *tg = new TrisetGrabber();
  if (tg->load(flnm))
    {
      m_trisets.append(tg);

      connect(tg, SIGNAL(updateParam()),
	      this, SLOT(sendParametersToMenu()));
      connect(tg, SIGNAL(meshGrabbed()),
	      this, SLOT(meshGrabbed()));
      connect(tg, SIGNAL(posChanged()),
	      this, SLOT(posChanged()));
      tg->setRotationMode(m_rotationMode);
    }
  else
    delete tg;

  loadMatCapTextures();
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
  float gmin = 1000000;
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
		  gmin = d.x*d.y;
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
		  gmin = d.x*d.y;
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
		  if (d.x*d.y < gmin)
		    {
		      gt = i;
		      gmin = d.x*d.y;
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
  
  //------------------
  // show bounding box for multiple selections
  if (m_multiActive.count() > 1)
    {
      Vec boxMin = Vec(0,0,0);
      Vec boxMax = Vec(0,0,0);
      for (int i=0; i<m_multiActive.count(); i++)
	{
	  Vec bmin, bmax;
	  m_trisets[m_multiActive[i]]->tenclosingBox(bmin, bmax);
	  if (i == 0)
	    {
	      boxMin = bmin;
	      boxMax = bmax;
	    }
	  else
	    {
	      boxMin = StaticFunctions::minVec(boxMin, bmin);
	      boxMax = StaticFunctions::maxVec(boxMax, bmax);
	    }
	}
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      glColor3f(1,1,0);
      Vec bgcolor = Global::backgroundColor();
      float bgintensity = (0.3*bgcolor.x +
			   0.5*bgcolor.y +
			   0.2*bgcolor.z);
      if (bgintensity > 0.5)
	glColor3f(0.3, 0.2, 0.1);
      StaticFunctions::drawEnclosingCube(boxMin, boxMax, false);
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
  //------------------
  
  for(int i=0; i<m_trisets.count();i++)
    {
      int x,y;
      QPoint scr = viewer->mapFromGlobal(QCursor::pos());
      x = scr.x();
      y = scr.y();
      m_trisets[i]->postdraw(viewer,
			     x, y,
			     i == m_active,
			     m_trisets[i]->grabsMouse(),
			     i+1,
			     m_trisets[i]->moveAxis());
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
  
  int scrW = camera->screenWidth();
  int scrH = camera->screenHeight();
  
  for(int i=0; i<m_trisets.count();i++)
    {      
      if (m_trisets[i]->reveal() >= 0.0 // non transparent reveal
	  && m_trisets[i]->outline() < 0.01) // not outline
	{

	  Vec extras = Vec(0,0,0);
	  if (m_trisets[i]->grabsMouse())
	    extras.x = 1;
      
	  extras.y = 1.0-m_trisets[i]->reveal();
	  
	  extras.z = m_trisets[i]->glow();
	  
	  float darken = 0;
	  if (glowOn && m_trisets[i]->glow() < 0.01)
	    darken = m_trisets[i]->dark();
	  
	  glUseProgram(ShaderFactory::meshShader());
	  GLint *meshShaderParm = ShaderFactory::meshShaderParm();        

	  
	  glUniform4f(meshShaderParm[2], extras.x, extras.y, extras.z, darken);
	  
	  glUniform1f(meshShaderParm[17], i+1);
	  
	  int matId = m_trisets[i]->material();
	  float matMix = m_trisets[i]->materialMix();
	  glUniform1i(meshShaderParm[18], matId);
	  if (matId > 0)
	    {
	      glActiveTexture(GL_TEXTURE1);
	      glEnable(GL_TEXTURE_2D);
	      glBindTexture(GL_TEXTURE_2D, m_solidTex[matId-1]);
	      glUniform1i(meshShaderParm[19], 1); // matcapTex
	      glUniform1f(meshShaderParm[20], matMix); // matMix
	    }
	  
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
	  
	  
	  {
	    glUniform1i(meshShaderParm[21], 3); // depthTex
	    glUniform2f(meshShaderParm[23], scrW, scrH); // screenSize
	    glUniformMatrix4fv(meshShaderParm[24], 1, GL_FALSE, m_mvpShadow);
	  }

	  
	  m_trisets[i]->draw(camera,
			     m_trisets[i]->grabsMouse());
	  
	  
	  if (matId > 0)
	    {
	      glActiveTexture(GL_TEXTURE1);
	      glDisable(GL_TEXTURE_2D);
	    }
	  
	  
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
  Vec rightVec = viewer->camera()->rightVector();
  Vec upVec = viewer->camera()->upVector();
  

  glUseProgram(ShaderFactory::meshShader());

  GLint *meshShaderParm = ShaderFactory::meshShaderParm();        


  glUniform1f(meshShaderParm[12], sceneRadius);

  glUniform3f(meshShaderParm[1], vd.x, vd.y, vd.z); // view direction
  glUniform3f(meshShaderParm[25], rightVec.x, rightVec.y, rightVec.z);
  glUniform3f(meshShaderParm[26], upVec.x, upVec.y, upVec.z);


  //--------------------------------------------
  // first render from shadowCamera
  Camera::Type ctype = viewer->camera()->type();

  //---------------------
  // setting shadow camera
  Quaternion orot = viewer->camera()->orientation();
  Vec opos = viewer->camera()->position();
  float ofov = viewer->camera()->fieldOfView();
  {
    float distFromCam;
    Vec offset;
    {
      Vec center = viewer->camera()->sceneCenter();
      Vec viewDir = viewer->camera()->viewDirection();
      distFromCam = (center-opos)*viewDir;
      Vec pos = (opos + distFromCam*viewDir);
      offset = center - pos;
    }

    Quaternion rot = orot * Quaternion(Vec(1,0,0), -DEG2RAD(m_lightDir.y*90))
                          * Quaternion(Vec(0,1,0), DEG2RAD(m_lightDir.x*90));
    viewer->camera()->setOrientation(rot);
    // now reposition the camera so that it faces the scene
    Vec center = viewer->camera()->sceneCenter();
    Vec viewDir = viewer->camera()->viewDirection();
    // set shadow camera to be orthographic
    // meaning the light is directional
    viewer->camera()->setType(Camera::ORTHOGRAPHIC);
    viewer->camera()->setPosition(center - distFromCam*viewDir - offset);

//    if (m_hitpoints->count() > 0)
//      {
//	Vec hp = m_hitpoints->points()[0];
//	viewer->camera()->setPosition(hp);
//	viewer->camera()->lookAt(center);
//	viewer->camera()->setFOVToFitScene();
//      }
  }
  //---------------------


  {
    Vec shadowCam = viewer->camera()->position();
    glUniform3f(meshShaderParm[16], shadowCam.x, shadowCam.y, shadowCam.z);
  }

  
  //--------------------------------------------
  viewer->camera()->getModelViewProjectionMatrix(m_mvpShadow);

  glUseProgram(ShaderFactory::meshShadowShader());
  glUniform1i(meshShaderParm[22], true); // shadowRender  
  renderFromShadowCamera(viewer->camera(), nclip);
  //--------------------------------------------


  { // restore camera
    viewer->camera()->setOrientation(orot);
    viewer->camera()->setPosition(opos);
    viewer->camera()->setType(ctype);
    viewer->camera()->setFieldOfView(ofov);
  }
  
  //--------------------------------------------
  // now render from actual camera
  {
    glUseProgram(ShaderFactory::meshShadowShader());
    glUniform1i(meshShaderParm[22], false); // shadowRender  

    glActiveTexture(GL_TEXTURE3);
    glEnable(GL_TEXTURE_RECTANGLE);
    glBindTexture(GL_TEXTURE_RECTANGLE, m_depthTex[3]); // shadow depthTex

    renderFromCamera(viewer->camera(), nclip);
    //renderFromCamera(&shadowCamera, nclip);

    glActiveTexture(GL_TEXTURE3);
    glDisable(GL_TEXTURE_RECTANGLE);
  }
  //--------------------------------------------
  

//  { // restore camera
//    viewer->camera()->setOrientation(orot);
//    viewer->camera()->setPosition(opos);
//  }
  
  //--------------------------------------------
  // render shadows
  renderShadows(drawFboId, wd, ht);
  //--------------------------------------------

  
  //--------------------------------------------
  // draw transparent surfaces if any
  renderTransparent(drawFboId, viewer, nclip, sceneRadius);
  //--------------------------------------------

  
  //--------------------------------------------
  // draw transparent surfaces if any
  renderOutline(drawFboId, viewer, nclip, sceneRadius);
  //--------------------------------------------

  //--------------------------------------------
  // draw outline for grabbed surface if any
  if (m_grab || m_multiActive.count() > 0)
    renderGrabbedOutline(drawFboId, viewer);
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
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


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
    
  QMatrix4x4 mvp;
  mvp.setToIdentity();
  mvp.ortho(0.0, wd, 0.0, ht, 0.0, 1.0);
  
  glUniformMatrix4fv(shadowParm[0], 1, GL_FALSE, mvp.data());
  
  glUniform1i(shadowParm[1], 0); // colors
  glUniform1i(shadowParm[2], 1); // actual-to-shadow depth
  glUniform1f(shadowParm[3], m_blur); // soft shadows
  glUniform1f(shadowParm[4], m_edges); // edge enhancement
  glUniform1f(shadowParm[5], Global::gamma()); // gamma
  float roughness = 0.9-m_trisets[0]->roughness()*0.1;
  glUniform1f(shadowParm[6], roughness); // specularity
  glUniform1f(shadowParm[7], m_trisets[0]->specular()); // specularity
  glUniform1i(shadowParm[8], 2); // shadow colors
  
  
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
  plist["reveal transparency"] = vlist;
  
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
  vlist << QVariant(m_trisets[i]->outline());
  vlist << QVariant(0.0);
  vlist << QVariant(1.0);
  vlist << QVariant(0.1); // singlestep
  vlist << QVariant(1); // decimals
  plist["outline"] = vlist;
  
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

  
//  Vec pat = m_trisets[i]->pattern();
//  vlist.clear();
//  vlist << QVariant("string");
//  vlist << QVariant(QString("%1 %2 %3").arg(pat.x).arg(pat.y).arg(pat.z));
//  plist["pattern"] = vlist;
  

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
  keys << "reveal transparency";
  keys << "reveal";
  keys << "outline";
  keys << "glow";
  keys << "darken on glow";
  //keys << "pattern";
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
	  else if (keys[ik] == "reveal transparency")
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
	  else if (keys[ik] == "outline")
	    {
	      float r = 0.0;
	      r = pair.first.toString().toDouble();
	      m_trisets[i]->setOutline(r);
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
//	  else if (keys[ik] == "pattern")
//            {
//	      QString vstr = pair.first.toString();
//	      QStringList pat = vstr.split(" ");
//	      if (pat.count() > 0)
//                {
//                  int texId = pat[0].toDouble();
//                  if (texId < 0 || texId > m_solidTexName.count())
//		    {
//		      QMessageBox::information(0, "", QString("Pattern id %1 is outside range 0-%1").\
//					       arg(texId).\
//					       arg(m_solidTexName.count()));
//		      return true;
//		    }
//		  if (pat.count() == 1)
//		    m_trisets[i]->setPattern(Vec(pat[0].toDouble(), 10, 0.5));
//		  else if (pat.count() == 3)
//		    m_trisets[i]->setPattern(Vec(pat[0].toDouble(),
//						 pat[1].toDouble(),
//						 pat[2].toDouble()));
//                }
//	    }
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

  connect(tg, SIGNAL(updateParam()),
	  this, SLOT(sendParametersToMenu()));
  connect(tg, SIGNAL(meshGrabbed()),
	  this, SLOT(meshGrabbed()));
  connect(tg, SIGNAL(posChanged()),
	  this, SLOT(posChanged()));

//  Vec bmin, bmax;
//  tg->enclosingBox(bmin, bmax);
//  float rad = (bmax-bmin).norm();
//  Vec pos = tg->position();
//  pos.x = pos.x + bmax.x-bmin.x;
//  tg->setPosition(pos);
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

  
  if (m_trisets[idx]->labelGrabbed() > -1)
    {
      int cpid = m_trisets[idx]->labelGrabbed();
      
      if ((event->key() == Qt::Key_Delete ||
	   event->key() == Qt::Key_Backspace))
	{
	  m_trisets[idx]->setCaptionText(m_trisets[idx]->labelGrabbed(), "");
	  return true;
	}

      if (event->key() == Qt::Key_M)
	{ // label moving
	  m_trisets[idx]->setCaptionOffset(cpid, 50,50);
	  return true;
	}

      if (event->key() == Qt::Key_F)
	{ // label fixed
	  m_trisets[idx]->setCaptionOffset(cpid, 0.1, 0.1);
	  return true;
	}

      if (event->key() == Qt::Key_R)
	{ // set label position
	  QList<Vec> pts;
	  if (m_hitpoints->activeCount())
	    pts = m_hitpoints->activePoints();
	  else
	    pts = m_hitpoints->points();
	  
	  if (pts.count() > 0)
	    {
	      // take the lastest hitpoint
	      Vec p = pts[pts.count()-1];
	      
	      m_trisets[idx]->setCaptionPosition(cpid, p);
	      
	      pts.removeLast();	    
	      m_hitpoints->setPoints(pts);
	    }
	  return true;
	}

      if (event->key() == Qt::Key_Space)
	{
	  // Used for modifying existing label
	  CaptionDialog cd(0,
			   m_trisets[idx]->captionText(cpid),
			   m_trisets[idx]->captionFont(cpid),
			   m_trisets[idx]->captionColor(cpid),
			   Qt::transparent,
			   0);
	  cd.hideAngle(true);
	  int cdW = cd.width();
	  int cdH = cd.height();
	  cd.move(QCursor::pos() - QPoint(cdW/2, cdH/2));
	  if (cd.exec() == QDialog::Accepted && !cd.text().isEmpty())
	    {
	      m_trisets[idx]->setCaptionText(cpid, cd.text());
	      m_trisets[idx]->setCaptionFont(cpid, cd.font());
	      m_trisets[idx]->setCaptionColor(cpid, cd.color());
	    }

	  return true;
	}
    } // Label Grabbed
//  if (m_trisets[idx]->labelGrabbed() &&
//      (event->key() == Qt::Key_Delete ||
//       event->key() == Qt::Key_Backspace))
//    {
//      m_trisets[idx]->setCaptionText("");
//      return true;
//    }

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
//  else if (event->key() == Qt::Key_Delete ||
//	   event->key() == Qt::Key_Backspace ||
//	   event->key() == Qt::Key_Backtab)
//    {
//      m_trisets[idx]->setMouseGrab(false);
//      m_trisets[idx]->removeFromMouseGrabberPool();
//      m_trisets.removeAt(idx);
//      emit resetBoundingBox();
//    }
  else if (event->key() == Qt::Key_Space)
    handleDialog(idx);
  
  return true;
}

void
Trisets::removeMesh(int idx)
{
  if (m_active == idx)
    m_active = -1;

  disconnect(m_trisets[idx]);
  m_trisets[idx]->setMouseGrab(false);
  m_trisets[idx]->removeFromMouseGrabberPool();
  m_trisets[idx]->clear();
  delete m_trisets[idx];
  m_trisets.removeAt(idx);
  emit resetBoundingBox();

  emit updateMeshList(getMeshList());
  emit updateGL();
}

void
Trisets::removeMesh(QList<int> indices)
{
  int idx;
  foreach(idx, indices)
    {
      if (m_active == idx)
	m_active = -1;
      
      disconnect(m_trisets[idx]);
      m_trisets[idx]->setMouseGrab(false);
      m_trisets[idx]->removeFromMouseGrabberPool();
      m_trisets[idx]->clear();
    }
			   
  int i = m_trisets.count()-1;
  while(i>=0)
    {
      if (m_trisets[i]->vertexCount() == 0)
	{
	  delete m_trisets[i];
	  m_trisets.removeAt(i);
	}
      i--;
    }
			   
  emit resetBoundingBox();  
  emit updateMeshList(getMeshList());
  emit updateGL();
}

void
Trisets::saveMesh(int idx)
{
  m_trisets[idx]->save();
}

// Single Mesh Command
void
Trisets::processCommand(int idx, QString cmd)
{
  bool ok;
  cmd = cmd.toLower();
  QStringList list = cmd.split(" ", QString::SkipEmptyParts);


  //------------------
  // reorder groups
  if (list[0] == "movebottom")
    {
      QList<TrisetGrabber*> oldT = m_trisets;
      int pl = -1;
      for (int i=0; i<m_trisets.count(); i++)
	{
	  if (idx != i)
	    m_trisets[++pl] = m_trisets[i];
	}
      m_trisets[++pl] = oldT[idx];
      
	emit updateMeshList(getMeshList());
    }
  if (list[0] == "movetop")
    {
      QList<TrisetGrabber*> oldT = m_trisets;
      int pl = -1;
      m_trisets[++pl] = m_trisets[idx];
      for (int i=0; i<m_trisets.count(); i++)
	{
	  if (i != idx)
	    m_trisets[++pl] = oldT[i];
	}
      
	emit updateMeshList(getMeshList());
    }  
  //------------------

  
  //------------------
  if (list[0] == "bakecolors")
    {
      m_trisets[idx]->bakeColors();
	
      emit updateGL();
      return;
    }

  if (list[0] == "label")
    {
      // Used for adding a new label
      CaptionDialog cd(0,
		       "",
		       QFont(QFont("MS Reference Sans Serif", 16)),
		       Qt::white,
		       Qt::transparent,
		       0);
      cd.hideAngle(true);
      int cdW = cd.width();
      int cdH = cd.height();
      cd.move(QCursor::pos() - QPoint(cdW/2, cdH/2));
      if (cd.exec() == QDialog::Accepted && !cd.text().isEmpty())
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

      if (list.size() == 2)
	{
	  if (list[1] == "moving")
	    m_trisets[idx]->setCaptionOffset(50,50);
	  else if (list[1] == "fixed")
	    m_trisets[idx]->setCaptionOffset(0.1,0.1);
	}
	
      emit updateGL();
      return;
    }
  //------------------

  if (list[0].contains("mirror"))
    {
      if (list[0] == "mirrorx") m_trisets[idx]->mirror(0);
      if (list[0] == "mirrory") m_trisets[idx]->mirror(1);
      if (list[0] == "mirrorz") m_trisets[idx]->mirror(2);

      emit updateGL();
      return;
    }
  

  if (list[0] == "resetposition")
    {
      m_trisets[idx]->setPosition(Vec(0,0,0));

      sendParametersToMenu();
      emit updateGL();
      return;
    }
  
  if (list[0] == "resetrotation")
    {
      m_trisets[idx]->resetRotation();

      emit updateGL();
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


      emit updateGL();
      return;
    }

}

// Multiple Mesh Command
void
Trisets::processCommand(QList<int> indices, QString cmd)
{
  int idx;
  
  bool ok;
  cmd = cmd.toLower();
  QStringList list = cmd.split(" ", QString::SkipEmptyParts);
  
  //------------------
  // reorder groups
  if (list[0] == "movebottom")
    {
      QList<TrisetGrabber*> oldT = m_trisets;
      int pl = -1;
      for (int i=0; i<m_trisets.count(); i++)
	{
	  if (!indices.contains(i))
	    m_trisets[++pl] = m_trisets[i];
	}
      foreach(idx, indices)
	{
	  m_trisets[++pl] = oldT[idx];
	}
      
	emit updateMeshList(getMeshList());
    }
  if (list[0] == "movetop")
    {
      QList<TrisetGrabber*> oldT = m_trisets;
      int pl = -1;
      foreach(idx, indices)
	{
	  m_trisets[++pl] = m_trisets[idx];
	}
      for (int i=0; i<m_trisets.count(); i++)
	{
	  if (!indices.contains(i))
	    m_trisets[++pl] = oldT[i];
	}
      
	emit updateMeshList(getMeshList());
    }  
  //------------------

  
  //------------------
  if (list[0] == "colormap")
    {
      askGradientChoice(indices);
      return;
    }

  
  if (list[0] == "merge")
    {
      mergeSurfaces(indices);
      return;
    }
  

  if (list[0] == "bakecolors")
    {
      bakeColors(indices);
      return;
    }
  

  if (list[0] == "resetposition")
    {
      foreach(idx, indices)
	{
	  if (idx >=0 && idx < m_trisets.count())
	    m_trisets[idx]->setPosition(Vec(0,0,0));
	}
      sendParametersToMenu();
      emit updateGL();
      return;
    }
  
      
  if (list[0] == "resetrotation")
    {
      foreach(idx, indices)
	{
	  if (idx >=0 && idx < m_trisets.count())
	    m_trisets[idx]->resetRotation();
	}
      emit updateGL();
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
      
      foreach(idx, indices)
	{
	  if (idx >=0 && idx < m_trisets.count())
	    m_trisets[idx]->rotate(rot);
	}
      
      emit updateGL();
      return;
    } 

  if (list[0] == "darkenall")
    {
      float dark = 0.5;
      if (list.count() > 1)
	dark = qBound(0.0f, list[1].toFloat(&ok), 1.0f);
      
      foreach(idx, indices)
	{
	  if (idx >=0 && idx < m_trisets.count())
	    m_trisets[idx]->setDark(dark);
	}
      return;
    }
  
  if (list[0] == "glowall")
    {
      float glow = 0.0;
      if (list.count() > 1)
	glow = qBound(0.0f, list[1].toFloat(&ok), 5.0f);
      
      foreach(idx, indices)
	{
	  m_trisets[idx]->setGlow(glow);
	}
      return;
    }
  
  if (list[0] == "outlineall")
    {
      float outline = 0.0;
      if (list.count() > 1)
	outline = qBound(0.0f, list[1].toFloat(&ok), 1.0f);
      
      foreach(idx, indices)
	{
	  if (idx >=0 && idx < m_trisets.count())
	    m_trisets[idx]->setOutline(outline);
	}
      return;
    }
  
  if (list[0] == "revealall")
    {
      float reveal = 0.0;
      if (list.count() > 1)
	reveal = qBound(-1.0f, list[1].toFloat(&ok), 1.0f);
      
      foreach(idx, indices)
	{
	  if (idx >=0 && idx < m_trisets.count())
	    m_trisets[idx]->setReveal(reveal);
	}
      return;
    }
  
  if (list[0] == "transparencyall")
    {
      float op = 1.0;
      if (list.count() > 1)
	op = qBound(0.1f, list[1].toFloat(&ok), 1.0f);
      
      foreach(idx, indices)
	{
	  if (idx >=0 && idx < m_trisets.count())
	    m_trisets[idx]->setOpacity(op);
	}
      return;
    }
  
  if (list[0].contains("explode"))
    {
      Vec damp = Vec(1,1,1);
      if (list[0] == "explodex") damp = Vec(1, 0, 0);
      if (list[0] == "explodey") damp = Vec(0, 1, 0);
      if (list[0] == "explodez") damp = Vec(0, 0, 1);
      if (list[0] == "explodexy") damp = Vec(1, 1, 0);
      if (list[0] == "explodexz") damp = Vec(0, 1, 1);
      if (list[0] == "explodeyz") damp = Vec(0, 1, 1);
	
      float rad = 1;
      if (list.count() > 1)
	rad = list[1].toFloat(&ok);

      float ex,ey,ez;
      ex = ey = ez = 0;
      if (list.count() == 5)
	{
	  ex = list[2].toFloat(&ok);
	  ey = list[3].toFloat(&ok);
	  ez = list[4].toFloat(&ok);
	}      

      setExplode(indices, rad, damp, Vec(ex,ey,ez));
      return;
    }

}

// All Meshes Command
void
Trisets::processCommand(QString cmd)
{
  bool ok;
  cmd = cmd.toLower();
  QStringList list = cmd.split(" ", QString::SkipEmptyParts);

  
  if (list[0] == "colormap")
    {
      QList<int> indices;
      for (int i=0; i<m_trisets.count(); i++)
	indices << i;
      askGradientChoice(indices);
      return;
    }
        
  if (list[0] == "bakecolors")
    {
      QList<int> indices;
      for (int i=0; i<m_trisets.count(); i++)
	indices << i;
      bakeColors(indices);
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
  
  if (list[0] == "outlineall")
    {
      float outline = 0.0;
      if (list.count() > 1)
	outline = qBound(0.0f, list[1].toFloat(&ok), 1.0f);
      
      for (int i=0; i<m_trisets.count(); i++)
	{
	  m_trisets[i]->setOutline(outline);
	}
      return;
    }
  
  if (list[0] == "revealall")
    {
      float reveal = 0.0;
      if (list.count() > 1)
	reveal = qBound(-1.0f, list[1].toFloat(&ok), 1.0f);
      
      for (int i=0; i<m_trisets.count(); i++)
	{
	  m_trisets[i]->setReveal(reveal);
	}
      return;
    }
  
  if (list[0] == "outline")
    {
      float outline = 0.0;
      if (list.count() > 1)
	outline = qBound(0.0f, list[1].toFloat(&ok), 1.0f);
      
      for (int i=0; i<m_trisets.count(); i++)
	{
	  if (m_trisets[i]->outline() >= 0.1)
	    m_trisets[i]->setOutline(outline);
	}
      return;
    }
  
  if (list[0] == "transparencyall")
    {
      float op = 1.0;
      if (list.count() > 1)
	op = qBound(0.1f, list[1].toFloat(&ok), 1.0f);
      
      for (int i=0; i<m_trisets.count(); i++)
	{
	  m_trisets[i]->setOpacity(op);
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
      if (list[0] == "explodex") damp = Vec(1, 0, 0);
      if (list[0] == "explodey") damp = Vec(0, 1, 0);
      if (list[0] == "explodez") damp = Vec(0, 0, 1);
      if (list[0] == "explodexy") damp = Vec(1, 1, 0);
      if (list[0] == "explodexz") damp = Vec(0, 1, 1);
      if (list[0] == "explodeyz") damp = Vec(0, 1, 1);
	
      float rad = 1;
      if (list.count() > 1)
	rad = list[1].toFloat(&ok);

      float ex,ey,ez;
      ex = ey = ez = 0;
      if (list.count() == 5)
	{
	  ex = list[2].toFloat(&ok);
	  ey = list[3].toFloat(&ok);
	  ez = list[4].toFloat(&ok);
	}      

      setExplode(rad, damp, Vec(ex,ey,ez));
      return;
    }

  if (list[0] == "clearscene")
    {
      removeFromMouseGrabberPool();

      for (int i=0; i<m_trisets.count(); i++)
	{
	  disconnect(m_trisets[i]);
	  m_trisets[i]->clear();
	}
      
      while(m_trisets.count() > 0)
	{
	  delete m_trisets[0];
	  m_trisets.removeAt(0);
	}

      //emit removeAllKeyFrames();
      emit resetBoundingBox();      
      emit updateMeshList(getMeshList());
      //emit clearScene();
      emit updateGL();
      
      return;
    }
  
//  if (list[0] == "merge")
//    {
//      QList<int> indices;
//      for (int i=0; i<m_trisets.count(); i++)
//	indices << i;
//      mergeSurfaces(indices);
//      return;
//    }
  
}

QList<TrisetInformation>
Trisets::get()
{
  QList<TrisetInformation> tinfo;

  for(int i=0; i<m_trisets.count(); i++)
    tinfo.append(m_trisets[i]->get());

  return tinfo;
}


QStringList
Trisets::getMeshList()
{
  QStringList names;
  for(int i=0; i<m_trisets.count(); i++)
    {
      //names << QString("%1").arg(QFileInfo(m_trisets[i]->filename()).fileName());      
      QString nm;
      nm = QString("%1").arg(QFileInfo(m_trisets[i]->filename()).fileName());

      int show = (m_trisets[i]->show() ? 1 : 0);
      int clip = (m_trisets[i]->clip() ? 1 : 0);

      Vec pcolor = m_trisets[i]->color();
      QString color = QColor::fromRgbF(pcolor.x,
				       pcolor.y,
				       pcolor.z).name();

      int matId = m_trisets[i]->material();
      float matMix = m_trisets[i]->materialMix();
      
      names << QString("%1 %2 %3 %4 %5 %6 %7 %8 %9").\
	arg((int)nm.length(), 5, 10, QChar(' ')).\
	arg(nm).\
	arg(show).\
	arg(clip).\
	arg(color).\
	arg(matId).\
	arg(matMix).\
	arg(m_trisets[i]->vertexCount()).	\
	arg(m_trisets[i]->triangleCount());

    }

  return names;
}

void
Trisets::set(QList<TrisetInformation> tinfo)
{
  if (tinfo.count() == 0)
    {
      clear();
      return;
    }

  loadMatCapTextures();
    
  if (m_trisets.count() == 0)
    {
      for(int i=0; i<tinfo.count(); i++)
	{
	  TrisetGrabber *tgi = new TrisetGrabber();	
	  if (tgi->set(tinfo[i]))
	    {
	      m_trisets.append(tgi);
	    
	      connect(tgi, SIGNAL(updateParam()),
		      this, SLOT(sendParametersToMenu()));
	      connect(tgi, SIGNAL(meshGrabbed()),
		      this, SLOT(meshGrabbed()));
	      connect(tgi, SIGNAL(posChanged()),
		      this, SLOT(posChanged()));
	      tgi->setRotationMode(m_rotationMode);
	    }
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
	{
	  tgi = tg[present[i]];	    
	  connect(tgi, SIGNAL(updateParam()),
		  this, SLOT(sendParametersToMenu()));
	  connect(tgi, SIGNAL(meshGrabbed()),
		  this, SLOT(meshGrabbed()));
	  connect(tgi, SIGNAL(posChanged()),
		  this, SLOT(posChanged()));
	  tgi->setRotationMode(m_rotationMode);
	}	  
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

  emit updateMeshList(getMeshList());
}

void
Trisets::makeReadyForPainting()
{
  for (int i=0; i<m_trisets.count(); i++)
    m_trisets[i]->makeReadyForPainting();
}

void
Trisets::paint(Vec hitPt, float rad, Vec color, int blendType, float blendFraction, int blendOctave)
{
  GLuint paintShader = ShaderFactory::paintShader();
  GLint* paintShaderParm = ShaderFactory::paintShaderParm();

  glUseProgram(paintShader);

  glUniform3f(paintShaderParm[0], hitPt.x, hitPt.y, hitPt.z);
  glUniform1f(paintShaderParm[1], rad);
  glUniform3f(paintShaderParm[2], color.x, color.y, color.z);
  glUniform1i(paintShaderParm[3], blendType);
  glUniform1f(paintShaderParm[4], blendFraction);
  glUniform1i(paintShaderParm[5], blendOctave);

  Vec bmin, bmax;
  allEnclosingBox(bmin, bmax);
  float blen = qMax(bmax.x-bmin.x, qMax(bmax.y-bmin.y, bmax.z-bmin.z));
  glUniform3f(paintShaderParm[6], bmin.x, bmin.y, bmin.z);
  glUniform1f(paintShaderParm[7], blen);

  for (int i=0; i<m_trisets.count(); i++)
    m_trisets[i]->paint(hitPt);
    
  glUseProgram(0);
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
Trisets::loadMatCapTextures()
{  
  if (m_solidTexName.count() > 0)
    return;
    
  QString texdir = qApp->applicationDirPath() + QDir::separator()  + "assets" + QDir::separator() + "matcap";
  QDir dir(texdir);  
  QStringList filters;
  filters << "*.png";
  dir.setNameFilters(filters);
  dir.setFilter(QDir::Files |
		QDir::NoSymLinks |
		QDir::NoDotAndDotDot);


  QFileInfoList list = dir.entryInfoList();
  if (list.size() == 0)
    return;

  
  m_solidTex = new GLuint[list.size()];
  glGenTextures(list.size(), m_solidTex);
  
  QRandomGenerator::global()->bounded(0,list.size()-1);

  int texSize;
  for (int i=0; i<list.size(); i++)
    {
      QString flnm = list.at(i).absoluteFilePath();
      m_solidTexName << flnm;
      
      QImage img(flnm);
      img = img.rgbSwapped();
      texSize = img.width();
      int ht = img.height();
      int wd = img.width();
      int nbytes = img.byteCount();
      int rgb = nbytes/(wd*ht);

      GLuint fmt;
      if (rgb == 1) fmt = GL_LUMINANCE;
      else if (rgb == 2) fmt = GL_LUMINANCE_ALPHA;
      else if (rgb == 3) fmt = GL_RGB;
      else if (rgb == 4) fmt = GL_RGBA;

      glActiveTexture(GL_TEXTURE1);
      glEnable(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D, m_solidTex[i]);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER); 
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER); 
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexImage2D(GL_TEXTURE_2D,
		   0, // single resolution
		   rgb,
		   texSize, texSize,
		   0, // no border
		   fmt,
		   GL_UNSIGNED_BYTE,
		   img.bits());
      glDisable(GL_TEXTURE_2D);
    }

  emit matcapFiles(m_solidTexName);
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
Trisets::renderGrabbedOutline(GLint drawFboId, QGLViewer *viewer)
{
  bool opOn = false;
  for(int i=0; i<m_trisets.count();i++)
    {      
      //if (i == m_active || m_trisets[i]->grabsMouse())
      if (m_multiActive.count() > 0 || m_trisets[i]->grabsMouse())
	{
	  opOn = true;
	  break;
	}
    }

  if (!opOn)
    return;

  
  Vec bgcolor = Global::backgroundColor();
  float bgintensity = (0.3*bgcolor.x +
		       0.5*bgcolor.y +
		       0.2*bgcolor.z);
  
  
  glDisable(GL_BLEND);
  
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

  float otln = 0.1;
  if (bgintensity > 0.5)
    otln = 0.2;
  
  for(int i=0; i<m_trisets.count();i++)
    {      
      int actv = i==m_active ? 1 : 0;
      if (m_multiActive.count() > 1)
	actv += m_multiActive.contains(i) ? 1 : 0;
      
      //if (i == m_active || m_trisets[i]->grabsMouse())
      if (actv > 0  || m_trisets[i]->grabsMouse())
	{
	  glUseProgram(ShaderFactory::meshShader());
	  GLint *meshShaderParm = ShaderFactory::meshShaderParm();        
	  
	  glUniform1f(meshShaderParm[17], i+1);

	  int matId = m_trisets[i]->material();
	  float matMix = m_trisets[i]->materialMix();
	  glUniform1i(meshShaderParm[18], matId);
	  if (matId > 0)
	    {
	      glActiveTexture(GL_TEXTURE1);
	      glEnable(GL_TEXTURE_2D);
	      glBindTexture(GL_TEXTURE_2D, m_solidTex[matId-1]);
	      glUniform1i(meshShaderParm[19], 1); // matcapTex
	      glUniform1f(meshShaderParm[20], matMix); // matMix
	    }

	  // no patterns
	  glUniform1i(meshShaderParm[18], 0);	  
	  // force no clipping
	  glUniform1iARB(meshShaderParm[9],  0);
	  
	  //---
	  float ot = m_trisets[i]->outline();
	  float op = m_trisets[i]->opacity();

	  m_trisets[i]->setOutline(ot+otln);

	  if (ot < 0.05)
	    m_trisets[i]->setOpacity(0.0);
	  else
	    m_trisets[i]->setOpacity(0.7);
	  
//	  if (m_trisets[i]->grabsMouse())
//	    m_trisets[i]->setOutline(ot+0.5);
	    
	  
	  m_trisets[i]->draw(viewer->camera(),
			     m_trisets[i]->grabsMouse());

	  
	  if (matId > 0)
	    {
	      glActiveTexture(GL_TEXTURE1);
	      glDisable(GL_TEXTURE_2D);
	    }

	  m_trisets[i]->setOutline(ot);
	  m_trisets[i]->setOpacity(op);
	  //---	  
	  
	  glUseProgramObjectARB(0);
	}
    }
 


  glBindFramebuffer(GL_FRAMEBUFFER, drawFboId);

  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  //glBlendFunc(GL_ONE, GL_ZERO);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  
  Vec grabbedColor = Vec(1,0,0);

  

  glUseProgram(ShaderFactory::outlineShader());
  GLint *outlineParm = ShaderFactory::outlineShaderParm();        

  
  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_TEXTURE_RECTANGLE);
  glBindTexture(GL_TEXTURE_RECTANGLE, m_depthTex[0]); // colors
  
  glActiveTexture(GL_TEXTURE1);
  glEnable(GL_TEXTURE_RECTANGLE);
  glBindTexture(GL_TEXTURE_RECTANGLE, m_depthTex[1]); // depth
  
  
  int wd = viewer->camera()->screenWidth();
  int ht = viewer->camera()->screenHeight();
  QMatrix4x4 mvp;
  mvp.setToIdentity();
  mvp.ortho(0.0, wd, 0.0, ht, 0.0, 1.0);
  
  
  glUniformMatrix4fv(outlineParm[0], 1, GL_FALSE, mvp.data());
  
  glUniform1i(outlineParm[1], 0); // colors
  glUniform1i(outlineParm[2], 1); // actual-to-shadow depth

  //glUniform1f(outlineParm[5], Global::gamma()); // edge enhancement

  if (bgintensity > 0.5)
    {
      glUniform1f(outlineParm[3], 0); // do not change outline color
      glUniform1f(outlineParm[5], 0.25*bgintensity); // gamma
    }
  else
    {
      glUniform1f(outlineParm[3], 20); // change outline color
      glUniform1f(outlineParm[5], 0.25+0.5*bgintensity); // gamma
    }
 
  glUniform1f(outlineParm[4], 1.0); // edge enhancement
  glUniform1f(outlineParm[6], 0.0); // roughness
  glUniform1f(outlineParm[7], m_trisets[0]->specular()); // specularity
  glUniform1f(outlineParm[10], 0); // shadowIntensity
  glUniform1f(outlineParm[11], 0); // valleyIntensity
  glUniform1f(outlineParm[12], 0); // peakIntensity);
  glUniform3f(outlineParm[13], grabbedColor.x, grabbedColor.y, grabbedColor.z); // grabbed color

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

  glDisable(GL_BLEND);

  glEnable(GL_DEPTH_TEST);  

  glDepthMask(GL_TRUE);

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
      if (m_trisets[i]->reveal() < 0.0) // transparent reveal
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
  Vec rightVec = viewer->camera()->rightVector();
  Vec upVec = viewer->camera()->upVector();
  
  glUniform3f(oitShaderParm[1], vd.x, vd.y, vd.z); // view direction
  glUniform3f(oitShaderParm[20], rightVec.x, rightVec.y, rightVec.z);
  glUniform3f(oitShaderParm[21], upVec.x, upVec.y, upVec.z);

  GLfloat mv[16];
  viewer->camera()->getModelViewMatrix(mv);

  glUniformMatrix4fv(oitShaderParm[4], 1, GL_FALSE, mv);

  glUniform1f(oitShaderParm[12], sceneRadius);
  
  for(int i=0; i<m_trisets.count();i++)
    {
      if (m_trisets[i]->reveal() < 0.0) // transparent reveal
	{
	  Vec extras = Vec(0,0,0);
	  if (m_trisets[i]->grabsMouse())
	    extras.x = 1;
      
	  extras.y = m_trisets[i]->reveal()*m_trisets[i]->reveal();
	  
	  extras.z = m_trisets[i]->glow();

	  float darken = 1;
	  if (glowOn && m_trisets[i]->glow() < 0.01)
	    darken = 1.0-0.8*m_trisets[i]->dark();
	  
	  glUseProgram(ShaderFactory::oitShader());
	  GLint *oitShaderParm = ShaderFactory::oitShaderParm();        
	  
	  glUniform4f(oitShaderParm[2], extras.x, extras.y, extras.z, darken);
	  
	  glUniform1f(oitShaderParm[17], i+1);
	  
	  int matId = m_trisets[i]->material();
	  float matMix = m_trisets[i]->materialMix();
	  glUniform1i(oitShaderParm[18], matId);
	  if (matId > 0)
	    {
	      glActiveTexture(GL_TEXTURE1);
	      glEnable(GL_TEXTURE_2D);
	      glBindTexture(GL_TEXTURE_2D, m_solidTex[matId-1]);
	      glUniform1i(oitShaderParm[19], 1); // matcapTex
	      glUniform1f(oitShaderParm[22], matMix); // matMix
	    }
	  
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
	  
	  
	  if (matId > 0)
	    {
	      glActiveTexture(GL_TEXTURE1);
	      glDisable(GL_TEXTURE_2D);
	    }
	  
	  
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

void
Trisets::renderOutline(GLint drawFboId,
		       QGLViewer *viewer,
		       int nclip,
		       float sceneRadius)
{
  bool opOn = false;
  for(int i=0; i<m_trisets.count();i++)
    {      
      if (m_trisets[i]->outline() > 0.01) // outline
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
 
  
  glDisable(GL_BLEND);

  
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
  
  for(int i=0; i<m_trisets.count();i++)
    {      
      if (m_trisets[i]->outline() > 0.01) // outline
	{

	  Vec extras = Vec(0,0,0);
	  if (m_trisets[i]->grabsMouse())
	    extras.x = 1;
      
	  //extras.y = 1.0-m_trisets[i]->reveal();
	  extras.y = 1.0;
	  
	  extras.z = m_trisets[i]->glow();
	  
	  float darken = 0;
	  if (glowOn && m_trisets[i]->glow() < 0.01)
	    darken = m_trisets[i]->dark();
	  
	  glUseProgram(ShaderFactory::meshShader());
	  GLint *meshShaderParm = ShaderFactory::meshShaderParm();        
	  
	  glUniform4f(meshShaderParm[2], extras.x, extras.y, extras.z, darken);
	  
	  glUniform1f(meshShaderParm[17], i+1);
	  
	  int matId = m_trisets[i]->material();
	  float matMix = m_trisets[i]->materialMix();
	  glUniform1i(meshShaderParm[18], matId);
	  if (matId > 0)
	    {
	      glActiveTexture(GL_TEXTURE1);
	      glEnable(GL_TEXTURE_2D);
	      glBindTexture(GL_TEXTURE_2D, m_solidTex[matId-1]);
	      glUniform1i(meshShaderParm[19], 1); // matcapTex
	      glUniform1f(meshShaderParm[20], matMix); // matMix
	    }

	  
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
	  
	  
	  m_trisets[i]->draw(viewer->camera(),
			     m_trisets[i]->grabsMouse());
	  
	  
	  if (matId > 0)
	    {
	      glActiveTexture(GL_TEXTURE1);
	      glDisable(GL_TEXTURE_2D);
	    }
	  
	  
	  glUseProgramObjectARB(0);
	}
    }



  glBindFramebuffer(GL_FRAMEBUFFER, drawFboId);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


  glUseProgram(ShaderFactory::outlineShader());
  GLint *outlineParm = ShaderFactory::outlineShaderParm();        

  
  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_TEXTURE_RECTANGLE);
  glBindTexture(GL_TEXTURE_RECTANGLE, m_depthTex[0]); // colors
  
  glActiveTexture(GL_TEXTURE1);
  glEnable(GL_TEXTURE_RECTANGLE);
  glBindTexture(GL_TEXTURE_RECTANGLE, m_depthTex[1]); // depth
  
  
  int wd = viewer->camera()->screenWidth();
  int ht = viewer->camera()->screenHeight();
  QMatrix4x4 mvp;
  mvp.setToIdentity();
  mvp.ortho(0.0, wd, 0.0, ht, 0.0, 1.0);
  
  glUniformMatrix4fv(outlineParm[0], 1, GL_FALSE, mvp.data());
  
  glUniform1i(outlineParm[1], 0); // colors
  glUniform1i(outlineParm[2], 1); // actual-to-shadow depth
  glUniform1f(outlineParm[3], m_blur); // soft shadows
  glUniform1f(outlineParm[4], m_edges); // edge enhancement
  glUniform1f(outlineParm[5], Global::gamma()); // edge enhancement
  float roughness = m_trisets[0]->roughness()*0.1;
  glUniform1f(outlineParm[6], roughness); // specularity
  glUniform1f(outlineParm[7], m_trisets[0]->specular()); // specularity
  glUniform1f(outlineParm[10], 0); // shadowintensity
  glUniform1f(outlineParm[11], 0); // valleyintensity
  glUniform1f(outlineParm[12], 0); // peakintensity
  glUniform3f(outlineParm[13], 0,0,0); // grabbed color
  
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

  glDisable(GL_BLEND);

  glEnable(GL_DEPTH_TEST);  

  glDepthMask(GL_TRUE);

}

void
Trisets::askGradientChoice(QList<int> indices)
{
  QString homePath = QDir::homePath();
  QFileInfo sfi(homePath, ".drishtigradients.xml");
  QString stopsflnm = sfi.absoluteFilePath();
  if (!sfi.exists())
    StaticFunctions::copyGradientFile(stopsflnm);

  QDomDocument document;
  QFile f(stopsflnm);
  if (f.open(QIODevice::ReadOnly))
    {
      document.setContent(&f);
      f.close();
    }

  QStringList glist;
  glist << "random";
  glist << "random-hue-pastel";
  glist << "random-hue-vibrant";
  glist << "random-pastel";
  glist << "random-vibrant";

  QDomElement main = document.documentElement();
  QDomNodeList dlist = main.childNodes();
  for(int i=0; i<dlist.count(); i++)
    {
      if (dlist.at(i).nodeName() == "gradient")
	{
	  QDomNodeList cnode = dlist.at(i).childNodes();
	  for(int j=0; j<cnode.count(); j++)
	    {
	      QDomElement dnode = cnode.at(j).toElement();
	      if (dnode.nodeName() == "name")
		glist << dnode.text();
	    }
	}
    }

  bool ok;
  QString gstr = QInputDialog::getItem(0,
				       "Color Gradient",
				       "Color Gradient",
				       glist, 0, false,
				       &ok);
  if (!ok)
    return;

  //--------------------
  if (gstr == "random")
    {
      for(int i=0; i<indices.count(); i++)
	{
	  float h = (float)qrand()/(float)RAND_MAX;
	  float s = (float)qrand()/(float)RAND_MAX;
	  float v = (float)qrand()/(float)RAND_MAX;

	  // don't want too low saturation or dark values
	  s = qMax(0.1f, s);
	  v = qMax(0.5f, v);
	  
	  QColor c;
	  c.setHsvF(h, s, v);
	  float r = c.redF();
	  float g = c.greenF();
	  float b = c.blueF();
	  
	  m_trisets[indices[i]]->setColor(Vec(r,g,b));
	}

      emit updateMeshList(getMeshList());
    }
  if (gstr == "random-pastel")
    {
      for(int i=0; i<indices.count(); i++)
	{
	  float h = (float)qrand()/(float)RAND_MAX;
	  float s = (float)qrand()/(float)RAND_MAX;
	  float v = (float)qrand()/(float)RAND_MAX;

	  // we want low saturation and higher value
	  s = 0.1 + 0.2*s;
	  v = 0.7 + 0.3*v;
	  
	  QColor c;
	  c.setHsvF(h, s, v);
	  float r = c.redF();
	  float g = c.greenF();
	  float b = c.blueF();
	  
	  m_trisets[indices[i]]->setColor(Vec(r,g,b));
	}

      emit updateMeshList(getMeshList());
    }
  if (gstr == "random-vibrant")
    {
      for(int i=0; i<indices.count(); i++)
	{
	  float h = (float)qrand()/(float)RAND_MAX;
	  float s = (float)qrand()/(float)RAND_MAX;
	  float v = (float)qrand()/(float)RAND_MAX;

	  // want higher saturation and value
	  s = 0.5 + 0.5*s;
	  v = 0.5 + 0.5*v;
	  
	  QColor c;
	  c.setHsvF(h, s, v);
	  float r = c.redF();
	  float g = c.greenF();
	  float b = c.blueF();
	  
	  m_trisets[indices[i]]->setColor(Vec(r,g,b));
	}

      emit updateMeshList(getMeshList());
    }
  if (gstr.contains("random-hue"))
    {
      QStringList clist;
      clist << "white";
      clist << "red";
      clist << "orange";
      clist << "yellow";
      clist << "olive";
      clist << "green";
      clist << "teal";
      clist << "cyan";
      clist << "azure";
      clist << "blue";
      clist << "purple";
      clist << "magenta";
      clist << "scarlet";
      
      bool ok;
      QString cstr = QInputDialog::getItem(0,
					   "Select Hue",
					   "Hue",
					   clist, 0, false,
					   &ok);
      if (!ok)
	return;
      
      if (cstr == "white")
	{
	  for(int i=0; i<indices.count(); i++)
	    {
	      float w = max(0.3f, (float)qrand()/(float)RAND_MAX);
	      m_trisets[indices[i]]->setColor(Vec(w,w,w));
	    }
	  
	  emit updateMeshList(getMeshList());
	  return;
	}
      
      float hmid = 0;
      if (cstr == "orange") hmid = 30;
      if (cstr == "yellow") hmid = 60;
      if (cstr == "olive") hmid = 90;
      if (cstr == "green") hmid = 120;
      if (cstr == "teal") hmid = 150;
      if (cstr == "cyan") hmid = 180;
      if (cstr == "azure") hmid = 210;
      if (cstr == "blue") hmid = 240;
      if (cstr == "purple") hmid = 270;
      if (cstr == "magenta") hmid = 300;
      if (cstr == "scarlet") hmid = 330;

      hmid /= 360.0;

      float smin = 0.3;
      float sint = 0.2;
      float vmin = 0.6;
      float vint = 0.3;
      if (gstr.contains("vibrant"))
	{
	  smin = 0.7;
	  sint = 0.3;
	  vmin = 0.5;
	  vint = 0.4;
	}
      
      for(int i=0; i<indices.count(); i++)
	{
	  float h = (float)qrand()/(float)RAND_MAX;
	  float s = (float)qrand()/(float)RAND_MAX;
	  float v = (float)qrand()/(float)RAND_MAX;

	  s = smin + sint*s;
	  v = vmin + vint*v;

	  h = hmid+(h-0.5)*0.04;
	  if (h < 0) h = 1+h;
	  
	  QColor c;
	  c.setHsvF(h, s, v);
	  float r = c.redF();
	  float g = c.greenF();
	  float b = c.blueF();
	  
	  m_trisets[indices[i]]->setColor(Vec(r,g,b));
	}

      emit updateMeshList(getMeshList());
    }
  //--------------------

  int cno = -1;
  for(int i=0; i<dlist.count(); i++)
    {
      if (dlist.at(i).nodeName() == "gradient")
	{
	  QDomNodeList cnode = dlist.at(i).childNodes();
	  for(int j=0; j<cnode.count(); j++)
	    {
	      QDomElement dnode = cnode.at(j).toElement();
	      if (dnode.tagName() == "name" && dnode.text() == gstr)
		{
		  cno = i;
		  break;
		}
	    }
	}
    }
	
  if (cno < 0)
    return;
  
  QGradientStops stops;
  QDomNodeList cnode = dlist.at(cno).childNodes();
  for(int j=0; j<cnode.count(); j++)
    {
      QDomElement de = cnode.at(j).toElement();
      if (de.tagName() == "gradientstops")
	{
	  QString str = de.text();
	  QStringList strlist = str.split(" ", QString::SkipEmptyParts);
	  for(int j=0; j<strlist.count()/5; j++)
	    {
	      float pos, r,g,b,a;
	      pos = strlist[5*j].toFloat();
	      r = strlist[5*j+1].toInt();
	      g = strlist[5*j+2].toInt();
	      b = strlist[5*j+3].toInt();
	      a = strlist[5*j+4].toInt();
	      stops << QGradientStop(pos, QColor(r,g,b,a));
	    }
	}
    }

  
  QGradientStops gstops;
  gstops = StaticFunctions::resampleGradientStops(stops, indices.count());  
  for(int i=0; i<indices.count(); i++)
    {
      float pos = gstops[i].first;
      QColor color = gstops[i].second;
      Vec clr = Vec(color.redF(),
		    color.greenF(),
		    color.blueF());
      m_trisets[indices[i]]->setColor(clr);
    }

  emit updateMeshList(getMeshList());
}


void
Trisets::sendParametersToMenu()
{
  if (m_active == -1)
    return;
  
  QMap<QString, QVariantList> plist;
  
  QVariantList vlist;

  vlist.clear();
  vlist << QVariant(m_active);
  plist["idx"] = vlist;
  
  Vec pos = m_trisets[m_active]->position();
  vlist.clear();
  vlist << QVariant(QString("%1 %2 %3").arg(pos.x).arg(pos.y).arg(pos.z));
  plist["position"] = vlist;
  
  Vec scl = m_trisets[m_active]->scale();
  vlist.clear();
  vlist << QVariant(QString("%1 %2 %3").arg(scl.x).arg(scl.y).arg(scl.z));
  plist["scale"] = vlist;
  
  vlist.clear();
  vlist << QVariant(10*m_trisets[m_active]->opacity());
  plist["transparency"] = vlist;
  
  vlist.clear();
  vlist << QVariant(10*m_trisets[m_active]->reveal());
  plist["reveal"] = vlist;
  
  vlist.clear();
  vlist << QVariant(qFloor(10*m_trisets[m_active]->outline()));
  plist["outline"] = vlist;
  
  vlist.clear();
  vlist << QVariant(10*m_trisets[m_active]->glow());
  plist["glow"] = vlist;
  
  vlist.clear();
  vlist << QVariant(10*m_trisets[m_active]->dark());
  plist["darken"] = vlist;
  
  emit setParameters(plist);
}

void
Trisets::setExplode(float rad, Vec damp, Vec eCen)
{
  Vec centroid = Vec(0,0,0);
  for (int i=0; i<m_trisets.count(); i++)
    centroid += m_trisets[i]->centroid();

  centroid /= m_trisets.count();
      
  for (int i=0; i<m_trisets.count(); i++)
    {
      Vec dr = m_trisets[i]->centroid() - centroid;
      dr = VECPRODUCT(dr, damp);
      dr *= rad;
      dr += eCen;
      m_trisets[i]->setPosition(dr);
    }
}

void
Trisets::setExplode(QList<int> indices, float rad, Vec damp, Vec eCen)
{
  Vec centroid = Vec(0,0,0);
  for (int i=0; i<indices.count(); i++)
    {
      int idx = indices[i];
      centroid += m_trisets[idx]->centroid();
    }
  centroid /= indices.count();
      
  for (int i=0; i<m_trisets.count(); i++)
    {
      if (indices.contains(i))
	{
	  Vec dr = m_trisets[i]->centroid() - centroid;
	  dr = VECPRODUCT(dr, damp);
	  dr *= rad;
	  dr += eCen;
	  m_trisets[i]->setPosition(dr);
	}
    }
}

void
Trisets::mergeSurfaces(QList<int> indices)
{
  QMessageBox::information(0, "Drishti", "Not Yet Implemented");			   
}

void
Trisets::bakeColors(QList<int> indices)
{
  for (int i=0; i<indices.count(); i++)
    {
      int idx = indices[i];
      m_trisets[idx]->bakeColors();
    }
  emit updateGL();
}

void
Trisets::scaleChanged(QVector3D scl)
{
  for (int i=0; i<m_multiActive.count(); i++)
    m_trisets[m_multiActive[i]]->setScale(Vec(scl.x(),
					      scl.y(),
					      scl.z()));
  emit updateGL();
}

void
Trisets::positionChanged(QVector3D pos)
{
  for (int i=0; i<m_multiActive.count(); i++)
    m_trisets[m_multiActive[i]]->setPosition(Vec(pos.x(),
						 pos.y(),
						 pos.z()));
  
  emit updateGL();
}

void
Trisets::transparencyChanged(int v)
{
  for (int i=0; i<m_multiActive.count(); i++)
    m_trisets[m_multiActive[i]]->setOpacity(v*0.1);
  
  emit updateGL();
}

void
Trisets::revealChanged(int v)
{
  for (int i=0; i<m_multiActive.count(); i++)
    m_trisets[m_multiActive[i]]->setReveal(v*0.1);
  emit updateGL();
}

void
Trisets::outlineChanged(int v)
{
  for (int i=0; i<m_multiActive.count(); i++)
    m_trisets[m_multiActive[i]]->setOutline(v*0.1);
  emit updateGL();
}

void
Trisets::glowChanged(int v)
{
  for (int i=0; i<m_multiActive.count(); i++)
    m_trisets[m_multiActive[i]]->setGlow(v*0.1);
  emit updateGL();
}

void
Trisets::darkenChanged(int v)
{
  for (int i=0; i<m_multiActive.count(); i++)
    m_trisets[m_multiActive[i]]->setDark(v*0.1);
  emit updateGL();
}

void
Trisets::colorChanged(QList<int> indices, QColor color)
{
  float r = color.redF();
  float g = color.greenF();
  float b = color.blueF();
  Vec pcolor = Vec(r,g,b);
  for (int i=0; i<indices.count(); i++)
    {
      int idx = indices[i];
      m_trisets[idx]->setColor(pcolor);
    }
  emit updateGL();
}

void
Trisets::colorChanged(QColor color)
{
  float r = color.redF();
  float g = color.greenF();
  float b = color.blueF();
  Vec pcolor = Vec(r,g,b);
  bool ignoreBlack = false;
//  if (m_trisets[m_active]->haveBlack())
//    {
//      bool ok;
//      QStringList vlist;
//      vlist << "No";
//      vlist << "Yes";
//      QString vstr = QInputDialog::getItem(0,
//					   QWidget::tr("Deleted Vertices"),
//					   QWidget::tr("Restore Deleted Vertices"),
//					   vlist, 0, false,
//					   &ok);
//      if (ok && !vstr.isEmpty())
//	{
//	  if (vstr == "Yes")
//	    ignoreBlack = true;
//	}
//    }
//  m_trisets[m_active]->setColor(pcolor, ignoreBlack);
  m_trisets[m_active]->setColor(pcolor);
  emit updateGL();
}

void
Trisets::materialChanged(QList<int> indices, int matId )
{
  for (int i=0; i<indices.count(); i++)
    {
      int idx = indices[i];
      m_trisets[idx]->setMaterial(matId);
    }
  emit updateGL();
}

void
Trisets::materialChanged(int matId)
{
  if (m_active < 0) return;
  m_trisets[m_active]->setMaterial(matId);
  emit updateGL();
}

void
Trisets::materialMixChanged(QList<int> indices, float matMix )
{
  for (int i=0; i<indices.count(); i++)
    {
      int idx = indices[i];
      m_trisets[idx]->setMaterialMix(matMix);
    }
  emit updateGL();
}

void
Trisets::materialMixChanged(float matMix)
{
  if (m_active < 0) return;
  m_trisets[m_active]->setMaterialMix(matMix);
  emit updateGL();
}

void
Trisets::setRotationMode(bool flag)
{
  m_rotationMode = flag;
  for (int i=0; i<m_trisets.count(); i++)
    {
      m_trisets[i]->setRotationMode(m_rotationMode);
    }
}


