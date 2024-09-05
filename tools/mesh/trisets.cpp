#include "global.h"
#include "shaderfactory.h"
#include "computeshaderfactory.h"
#include "staticfunctions.h"
#include "dcolordialog.h"
#include "trisets.h"
#include "propertyeditor.h"
#include "captiondialog.h"
#include "matrix.h"
#include "mainwindowui.h"

#include <QFileDialog>
#include <QInputDialog>
#include "popupslider.h"


Trisets::Trisets()
{
  m_trisets.clear();
  m_nclip = 0;
  m_cpos = new float[100];
  m_cnormal = new float[100];

  m_active = -1;
  m_multiActive.clear();
  
  m_depthBuffer = 0;
  for(int i=0; i<6; i++)
    m_depthTex[i] = 0;
  m_rbo = 0;

  
  m_vertexScreenBuffer = 0;

  m_blur = 0;
  m_edges = 0;
  m_shadowIntensity = 0;
  m_valleyIntensity = 0;
  m_peakIntensity = 0;
  
  memset(&m_scrGeo[0], 0, 8*sizeof(float));

  m_solidTexName.clear();
  m_solidTexData.clear();
  m_solidTex = 0;
  
  m_lightDir = Vec(0.0,0.0,1);
  m_lightDir.normalize();

  m_grab = false;

  m_clipPartial = true;
  
  m_renderingClearView = false;
}

Trisets::~Trisets()
{
  clear();
  delete [] m_cpos;
  delete [] m_cnormal;

  if (m_depthBuffer) glDeleteFramebuffers(1, &m_depthBuffer);
  if (m_depthTex[0]) glDeleteTextures(6, m_depthTex);
  if (m_rbo) glDeleteRenderbuffers(1, &m_rbo);
  
  m_depthBuffer = 0;
  for(int i=0; i<6; i++)
    m_depthTex[i] = 0;
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
Trisets::setGrab(bool flag)
{
  m_grab = flag;

  removeFromMouseGrabberPool();
  
  if (m_grab && m_active > -1)
    addInMouseGrabberPool(m_active);


//  if (m_grab)
//    {
//      addInMouseGrabberPool();
//      for (int i=0; i<m_trisets.count(); i++)
//  	m_trisets[i]->setGrabMode(m_grab);
//    }
//  else
//    {
//      removeFromMouseGrabberPool();
//      for (int i=0; i<m_trisets.count(); i++)
//  	m_trisets[i]->setGrabMode(m_grab);
//  
//      if (m_active > -1)
//  	addInMouseGrabberPool(m_active);
//    }

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
  return;
  
//  for (int i=0; i<m_trisets.count(); i++)
//    {
//      if (m_trisets[i]->mousePressed())
//	{
//	  emit meshGrabbed(i);
//	  return;
//	}
//    }
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

  //emit updateGL();
}

void
Trisets::setActive(int idx, bool flag)
{
  if (flag)
    {
      m_active = idx;
      removeFromMouseGrabberPool();
      sendParametersToMenu();
      if (m_grab)
	{
	  if (m_active > -1)
	    addInMouseGrabberPool(m_active);
	  sendParametersToMenu();
	  m_prevActivePos = m_trisets[m_active]->position();
	}
    }
  else
    {
      m_active = -1;
      if (!m_grab)
	removeFromMouseGrabberPool(idx);
    }

  //emit updateGL();
}

bool
Trisets::addHitPoint(Vec v)
{
  if (m_multiActive.count() > 1)
    {
      QMessageBox::information(0, "", "Cannot add hitpoint when multiple surfaces are selected");
      return true;
    }

  if (m_active == -1)
    return false;
  else
    m_trisets[m_active]->addHitPoint(v);

  return true;
}

void
Trisets::drawHitPoints()
{
  for (int i=0; i<m_trisets.count(); i++)
    m_trisets[i]->drawHitPoints();
}


void
Trisets::setLightDirection(Vec ldir)
{
  m_lightDir = ldir;
}

void
Trisets::setShapeEnhancements(float blur, float edges,
			      float si, float vi, float pi)
{
  m_blur = blur;
  m_edges = edges;
  m_shadowIntensity = si;
  m_valleyIntensity = vi;
  m_peakIntensity = pi;
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
    }
}

void
Trisets::hide()
{
  for (int i=0; i<m_trisets.count(); i++)
    {
      m_trisets[i]->setShow(false);
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

bool
Trisets::clearView(int i)
{
  if (i >= 0 && i < m_trisets.count())
    return m_trisets[i]->clearView();
  else
    return false;
}

void
Trisets::setClearView(int i, bool flag)
{
  if (i >= 0 && i < m_trisets.count())
    {
      m_trisets[i]->setClearView(flag);
      emit updateGL();
    }
}

void
Trisets::setClearView(QList<bool> flag)
{
  for (int i=0; i<flag.count(); i++)
    {
      m_trisets[i]->setClearView(flag[i]);
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

  m_active = -1;
  m_multiActive.clear();

  m_nclip = 0;
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

//  m_trisets[0]->enclosingBox(boxMin, boxMax);
  m_trisets[0]->tenclosingBox(boxMin, boxMax);
  if ((boxMax-boxMin).squaredNorm() < 0.00000001f)
    m_trisets[0]->enclosingBox(boxMin, boxMax);
  
  for(int i=1; i<m_trisets.count();i++)
    {
      Vec bmin, bmax;
      //m_trisets[i]->enclosingBox(bmin, bmax);
      m_trisets[i]->tenclosingBox(bmin, bmax);
      if ((bmax-bmin).squaredNorm() < 0.00000001f)
	m_trisets[i]->enclosingBox(bmin, bmax);
      boxMin = StaticFunctions::minVec(boxMin, bmin);
      boxMax = StaticFunctions::maxVec(boxMax, bmax);
    }
}

bool
Trisets::isInMouseGrabberPool(int i)
{
  if (i >= 0 && i < m_trisets.count())
    return m_trisets[i]->isInMouseGrabberPool();
  else
    return false;
}
void
Trisets::addInMouseGrabberPool(int i)
{
  if (i >= 0 && i < m_trisets.count())
    {
      m_trisets[i]->addInMouseGrabberPool();
      //m_trisets[i]->setGrabMode(true);
    }
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
  if (i >= 0 && i < m_trisets.count())
    m_trisets[i]->removeFromMouseGrabberPool();
  else
    removeFromMouseGrabberPool();
}

void
Trisets::removeFromMouseGrabberPool()
{
  for(int i=0; i<m_trisets.count(); i++)
    {
      m_trisets[i]->removeFromMouseGrabberPool();
      m_trisets[i]->setGrabMode(false);
    }
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
    }
  else
    delete tg;

  loadMatCapTextures();
}

void
Trisets::checkHitPointsHover(QGLViewer *viewer)
{
  QPoint scr = viewer->mapFromGlobal(QCursor::pos());

  // reset hovered hit point
  bool prevHovered = false;
  for(int i=0; i<m_trisets.count(); i++)
    {
      if (m_trisets[i]->hitPointHovered() > -1)
	prevHovered = true;
      
      m_trisets[i]->setHitPointHovered(-1);
    }

  for(int i=0; i<m_trisets.count(); i++)
    {
      Vec tcen = m_trisets[i]->centroid();
      Vec tpos = m_trisets[i]->position();
      Quaternion tq = m_trisets[i]->rotation();
      QList<Vec> hpts;
      hpts = m_trisets[i]->hitPoints();
      for(int p=0; p<hpts.count(); p++)
	{
	  Vec pos = hpts[p];
	  pos = pos - tcen;
	  pos = tq.inverseRotate(pos) + tcen + tpos;
	  pos = viewer->camera()->projectedCoordinatesOf(pos);
	  QPoint hp(pos.x, pos.y);
	  if ((hp-scr).manhattanLength() < 10)
	    {
	      m_trisets[i]->setHitPointHovered(p);
	      emit updateGL();
	      return;
	    }
	}
    }

  // just so that the previously hovered point is not shown still hovered
  if (prevHovered)
    emit updateGL();  
}

void
Trisets::checkMouseHover(QGLViewer *viewer)
{
  // using checkMouseHover instead of checkIfGrabsMouse
  
  if (m_trisets.count() == 0)
    return;

  // check for hitpoint hover
  checkHitPointsHover(viewer);

  
  QPoint scr = viewer->mapFromGlobal(QCursor::pos());
  int x = scr.x();
  int y = scr.y();
  int gt = -1;
  float gmin = 1000000;
//  for(int i=0; i<m_trisets.count(); i++)
//    {
//      if (m_trisets[i]->grabsMouse())
//	{
//	  if (!m_trisets[i]->mousePressed())
//	    {
//	      Vec d = m_trisets[i]->checkForMouseHover(x,y, viewer->camera());
//	      if (d.x > -1 && d.y > 0)
//		{
//		  gt = i;
//		  gmin = d.x*d.y;
//		  break;
//		}
//	    }
//	  else
//	    return;
//	}
//    }
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
Trisets::predraw(double *Xform)
{
  if (m_trisets.count() == 0)
    return;
  
  for(int i=0; i<m_trisets.count();i++)
    m_trisets[i]->predraw(i == m_active,
			  Xform);
}

void
Trisets::postdraw(QGLViewer *viewer)
{
  if (m_trisets.count() == 0)
    return;
  
  // ----
  // just resetting model view matrices for post draw
  viewer->startScreenCoordinatesSystem();
  viewer->stopScreenCoordinatesSystem();
  // ----


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
      int actv = i==m_active ? 1 : 0;
//      if (m_multiActive.count() > 1)
//	actv += m_multiActive.contains(i) ? 1 : 0;

      m_trisets[i]->postdraw(viewer,
			     x, y,
			     m_grab,
			     m_trisets[i]->grabsMouse(),
			     actv,
			     i+1,
			     m_trisets[i]->moveAxis());
    }
}

void
Trisets::render(GLdouble *MVP, Vec viewDir,
		int scrW, int scrH, int nclip,
		bool outline,
		bool fromShadowCamera)
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

  bool pcv = false;
  for(int i=0; i<m_trisets.count();i++)
    {      
      if (m_trisets[i]->clearView())
	{
	  pcv = true;
	  break;
	}
    }

  GLint *meshShaderParm = ShaderFactory::meshShaderParm();        
  
  if (!m_renderingClearView && !pcv)
    {
      glUniform1i(meshShaderParm[32], 0); // processClearView
    }
  
  for(int i=0; i<m_trisets.count();i++)
    {
//      if (fromShadowCamera && m_trisets[i]->clearView())
//	continue;
      
      if (m_renderingClearView && !m_trisets[i]->clearView())
	continue;

      if (m_trisets[i]->clipped())
	continue;
      
      
      bool draw = false;
      if (!outline)
	{
	  if (m_trisets[i]->reveal() >= 0.0 // non transparent reveal
	      && m_trisets[i]->outline() < 0.05) // not outline
	    draw = true;
	}
      else
	{
	  if (m_trisets[i]->outline() >= 0.05) // outline
	    draw = true;
	}

      if (draw)
	  {

	  Vec extras = Vec(0,0,0);
	  if (i == m_active || m_trisets[i]->grabsMouse())
	    extras.x = 0.2;
      
	  extras.y = 1.0-m_trisets[i]->reveal();
	  
	  extras.z = m_trisets[i]->glow();
	  
	  float darken = 0;
	  if (glowOn && m_trisets[i]->glow() < 0.01)
	    darken = m_trisets[i]->dark();
	  
	  glUseProgram(ShaderFactory::meshShader());
	  GLint *meshShaderParm = ShaderFactory::meshShaderParm();        
	  
	  if (!m_renderingClearView)
	    {
	      if (m_trisets[i]->clearView())
		glUniform1i(meshShaderParm[32], 0); // processClearView
	      else
		{
		  if (pcv)
		    glUniform1i(meshShaderParm[32], 1);
		}
	    }
      
	  glUniform4f(meshShaderParm[2], extras.x, extras.y, extras.z, darken);
	  
	  glUniform1f(meshShaderParm[17], i+1);
	  
	  int matId = m_trisets[i]->material();
	  glUniform1i(meshShaderParm[18], matId);
	  if (matId > 0)
	    {
	      float matMix = m_trisets[i]->materialMix();
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

	  
	  m_trisets[i]->draw(MVP, viewDir,
			     i == m_active);
	  
	  
	  
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
	      GLdouble *MVP, GLfloat *MV, Vec viewDir,
	      GLdouble *shadowMVP, Vec shadowCam, Vec shadowViewDir,
	      int scrW, int scrH,
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
//  {
//    for(int i=0; i<m_trisets.count();i++)
//      {
//	Vec bmin, bmax;
//	m_trisets[i]->enclosingBox(bmin, bmax);      
//	trisetExtent = qMax(trisetExtent, (float)((bmax-bmin).norm()));
//      }
//  }
  {
//    //Vec bmin, bmax;
//    //allEnclosingBox(bmin, bmax);  
//    //trisetExtent = (bmax - bmin).norm()*2.0;
    trisetExtent = (viewer->camera()->position()-viewer->camera()->sceneCenter()).norm();
  }
  //--------------------------

      
  //--------------------------
  for(int i=0; i<m_trisets.count();i++)
    m_trisets[i]->setClipped(false);
  
  m_nclip = 0;
  if (m_clipPartial == false && cpos.count() > 0)
    {
      //identify clipped trisets - if entire triset clipping enabled
      for(int i=0; i<m_trisets.count();i++)
	{
	  if (m_trisets[i]->clip())
	    {
	      bool clipped = false;
	      Vec bmin, bmax, bcen;
	      m_trisets[i]->tenclosingBox(bmin, bmax);			    
	      bcen = (bmax+bmin)*0.5;
	      for (int c=0; c<cpos.count(); c++)
		{
		  if ((bcen - cpos[c])*cnormal[c] > 0)
		    {
		      m_trisets[i]->setClipped(true);
		      break;
		    }
		}
	    }
	}
    }
  //--------------------------

  
  //--------------------------
  if (m_clipPartial == true)
    {
      m_nclip = cpos.count();
      if (m_nclip > 0)
	{
	  for(int c=0; c<m_nclip; c++)
	    {
	      m_cpos[3*c+0] = cpos[c].x;
	      m_cpos[3*c+1] = cpos[c].y;
	      m_cpos[3*c+2] = cpos[c].z;
	    }
	  for(int c=0; c<m_nclip; c++)
	    {
	      m_cnormal[3*c+0] = cnormal[c].x;
	      m_cnormal[3*c+1] = cnormal[c].y;
	      m_cnormal[3*c+2] = cnormal[c].z;
	    }
	}
    }
  //--------------------------



  

  float sceneRadius = trisetExtent;
  Vec rightVec = viewer->camera()->rightVector();
  Vec upVec = viewer->camera()->upVector();
  
  for(int im=0; im<16; im++) m_mvpShadow[im] = shadowMVP[im];


  
  //--------------------------
  if (!m_depthBuffer)
    createFBO(scrW, scrH);

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
  glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
			 GL_COLOR_ATTACHMENT2,
			 GL_TEXTURE_RECTANGLE,
			 m_depthTex[2],
			 0);
  glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
			 GL_COLOR_ATTACHMENT3,
			 GL_TEXTURE_RECTANGLE,
			 m_depthTex[3],
			 0);
  glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
			 GL_COLOR_ATTACHMENT4,
			 GL_TEXTURE_RECTANGLE,
			 m_depthTex[4],
			 0);
  glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
			 GL_COLOR_ATTACHMENT5,
			 GL_TEXTURE_RECTANGLE,
			 m_depthTex[5],
			 0);
  GLenum buffers[6] = { GL_COLOR_ATTACHMENT0,
			GL_COLOR_ATTACHMENT1,
			GL_COLOR_ATTACHMENT2,
			GL_COLOR_ATTACHMENT3,
			GL_COLOR_ATTACHMENT4,
			GL_COLOR_ATTACHMENT5};
  glDrawBuffers(4, buffers);

  glClearDepth(1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  //--------------------------



  glUseProgram(ShaderFactory::meshShader());

  GLint *meshShaderParm = ShaderFactory::meshShaderParm();        


  glUniform3f(meshShaderParm[1], viewDir.x, viewDir.y, viewDir.z); // view direction
  glUniform3f(meshShaderParm[29], rightVec.x, rightVec.y, rightVec.z);
  glUniform3f(meshShaderParm[30], upVec.x, upVec.y, upVec.z);

  glUniform3f(meshShaderParm[28], shadowViewDir.x, shadowViewDir.y, shadowViewDir.z); // light direction

  glUniform1f(meshShaderParm[12], sceneRadius);

  glUniform3f(meshShaderParm[16], shadowCam.x, shadowCam.y, shadowCam.z);

  glUniform1i(meshShaderParm[25], Global::hideBlack());

  //================================================
  //================================================
  int trisetsForShadows = 0;
  for(int i=0; i<m_trisets.count();i++)
    {      
      if (m_trisets[i]->reveal() >= 0.0 // non transparent reveal
	  && m_trisets[i]->outline() < 0.05) // not outline
	trisetsForShadows++;
    }

  if (trisetsForShadows > 0)
    {
      //--------------------------------------------
      // first render from shadowCamera
      glUseProgram(ShaderFactory::meshShadowShader());
      glUniform1i(meshShaderParm[22], true); // shadowRender  
      
      renderFromShadowCamera(shadowMVP, shadowViewDir,
			     scrW, scrH, m_nclip);
      //--------------------------------------------
      

      //--------------------------------------------
      // process clear view
      {
	glUseProgram(ShaderFactory::meshShadowShader());
	glUniform1i(meshShaderParm[22], false); // shadowRender  
	
	renderFromCameraClearView(MVP, viewDir,
				  scrW, scrH, m_nclip);


	dilateClearViewBuffer(viewer, scrW, scrH);
      }
      //--------------------------------------------

      
      //--------------------------------------------
      // now render from actual camera
      {
	glUseProgram(ShaderFactory::meshShadowShader());
	glUniform1i(meshShaderParm[22], false); // shadowRender  
	
	glActiveTexture(GL_TEXTURE3);
	glEnable(GL_TEXTURE_RECTANGLE);
	glBindTexture(GL_TEXTURE_RECTANGLE, m_depthTex[3]); // shadow depthTex

	
	glActiveTexture(GL_TEXTURE6);
	glEnable(GL_TEXTURE_RECTANGLE);
	glBindTexture(GL_TEXTURE_RECTANGLE, m_depthTex[5]); // clear view depthTex
	
      
	renderFromCamera(MVP, viewDir,
			 scrW, scrH, m_nclip);
	
	glActiveTexture(GL_TEXTURE3);
	glDisable(GL_TEXTURE_RECTANGLE);

	glActiveTexture(GL_TEXTURE6);
	glDisable(GL_TEXTURE_RECTANGLE);
      }
      //--------------------------------------------
      
      
      //--------------------------------------------
      // render shadows
      renderShadows(drawFboId, scrW, scrH, shadowCam, sceneRadius, shadowViewDir);
      //--------------------------------------------

    }
  //================================================
  //================================================

  
//  //--------------------------------------------
//  // render bounding shadow box if any
//  if (Global::shadowBox())
//    renderShadowBox(drawFboId, scrW, scrH, shadowCam, sceneRadius, shadowViewDir);
//  //--------------------------------------------

  
  //--------------------------------------------
  // draw transparent surfaces if any
  renderTransparent(drawFboId,
		    MVP, MV, viewDir, shadowViewDir, scrW, scrH,
		    m_nclip, sceneRadius);
  //--------------------------------------------

  
  //--------------------------------------------
  // draw surface outlines if any
  renderOutline(drawFboId,
		  MVP, viewDir, scrW, scrH,
		  m_nclip, sceneRadius);
  //--------------------------------------------

  //--------------------------------------------
  // draw outline for grabbed surface if any
  if (m_grab || m_multiActive.count() > 0)
    renderGrabbedOutline(drawFboId,
			 MVP, viewDir, scrW, scrH);
  //--------------------------------------------


  //--------------------------------------------
  // render bounding shadow box if any
  if (Global::shadowBox())
    renderShadowBox(drawFboId, scrW, scrH, shadowCam, sceneRadius, shadowViewDir);
  //--------------------------------------------

  
  glActiveTexture(GL_TEXTURE0);
  glDisable(GL_TEXTURE_2D);
}

void
Trisets::dilateClearViewBuffer(QGLViewer *viewer,
			       int scrW, int scrH)
{
  int size = 30;
  
  glBindFramebuffer(GL_FRAMEBUFFER, m_depthBuffer);
  glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
			 GL_COLOR_ATTACHMENT0,
			 GL_TEXTURE_RECTANGLE,
			 m_depthTex[4],
			 0);
  GLenum buffers[1] = { GL_COLOR_ATTACHMENT0_EXT };
  glDrawBuffersARB(1, buffers);
  glClearDepth(1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  
  glActiveTexture(GL_TEXTURE6);
  glEnable(GL_TEXTURE_RECTANGLE);
  glBindTexture(GL_TEXTURE_RECTANGLE, m_depthTex[5]); // shadow depthTex
  
  glUseProgramObjectARB(ShaderFactory::dilateShader());
  glUniform1iARB(ShaderFactory::dilateShaderParm()[0], 6); // dilate image
  glUniform1iARB(ShaderFactory::dilateShaderParm()[1], size); // radius
  glUniform2fARB(ShaderFactory::dilateShaderParm()[2], 0, 1); // direction
  
  
  viewer->startScreenCoordinatesSystem();

  StaticFunctions::drawQuad(0, 0, scrW, scrH, 1);
  

  glBindFramebuffer(GL_FRAMEBUFFER, m_depthBuffer);
  glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
			 GL_COLOR_ATTACHMENT0,
			 GL_TEXTURE_RECTANGLE,
			 m_depthTex[5],
			 0);
  glDrawBuffersARB(1, buffers);
  glClearDepth(1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
  glActiveTexture(GL_TEXTURE6);
  glEnable(GL_TEXTURE_RECTANGLE);
  glBindTexture(GL_TEXTURE_RECTANGLE, m_depthTex[4]); // shadow depthTex
  
  glUseProgramObjectARB(ShaderFactory::dilateShader());
  glUniform1iARB(ShaderFactory::dilateShaderParm()[0], 6); // dilate image
  glUniform1iARB(ShaderFactory::dilateShaderParm()[1], size); // radius
  glUniform2fARB(ShaderFactory::dilateShaderParm()[2], 1, 0); // direction
  

  StaticFunctions::drawQuad(0, 0, scrW, scrH, 1);
  

  //----------------------------
  //----------------------------
  // blur depth buffer
  
  glBindFramebuffer(GL_FRAMEBUFFER, m_depthBuffer);
  glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
			 GL_COLOR_ATTACHMENT0,
			 GL_TEXTURE_RECTANGLE,
			 m_depthTex[4],
			 0);
  glDrawBuffersARB(1, buffers);
  glClearDepth(1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
  glActiveTexture(GL_TEXTURE6);
  glEnable(GL_TEXTURE_RECTANGLE);
  glBindTexture(GL_TEXTURE_RECTANGLE, m_depthTex[5]); // shadow depthTex
  
  glUseProgramObjectARB(ShaderFactory::blurShader());
  glUniform1iARB(ShaderFactory::blurShaderParm()[0], 6); // copy image from imageBuffer into frameBuffer


  StaticFunctions::drawQuad(0, 0, scrW, scrH, 1);


  glBindFramebuffer(GL_FRAMEBUFFER, m_depthBuffer);
  glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
			 GL_COLOR_ATTACHMENT0,
			 GL_TEXTURE_RECTANGLE,
			 m_depthTex[5],
			 0);
  glDrawBuffersARB(1, buffers);
  glClearDepth(1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
  glActiveTexture(GL_TEXTURE6);
  glEnable(GL_TEXTURE_RECTANGLE);
  glBindTexture(GL_TEXTURE_RECTANGLE, m_depthTex[4]); // shadow depthTex
  
  glUseProgramObjectARB(ShaderFactory::blurShader());
  glUniform1iARB(ShaderFactory::blurShaderParm()[0], 6); // copy image from imageBuffer into frameBuffer


  StaticFunctions::drawQuad(0, 0, scrW, scrH, 1);
  //----------------------------
  //----------------------------


  
  glUseProgramObjectARB(0);  
  
      
  glActiveTexture(GL_TEXTURE6);
  glDisable(GL_TEXTURE_RECTANGLE);

  viewer->stopScreenCoordinatesSystem();
}


void
Trisets::renderFromCameraClearView(GLdouble *MVP, Vec vd,
				   int scrW, int scrH, int nclip)
{
  glBindFramebuffer(GL_FRAMEBUFFER, m_depthBuffer);
  glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
			 GL_COLOR_ATTACHMENT0,
			 GL_TEXTURE_RECTANGLE,
			 m_depthTex[4],
			 0);
  glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
			 GL_COLOR_ATTACHMENT1,
			 GL_TEXTURE_RECTANGLE,
			 m_depthTex[5],
			 0);
  GLenum buffers[2] = { GL_COLOR_ATTACHMENT0_EXT,
			GL_COLOR_ATTACHMENT1_EXT };
  glDrawBuffersARB(2, buffers);
  
  glClearDepth(1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  
  glDisable(GL_BLEND);

  m_renderingClearView = true;
  glUseProgram(ShaderFactory::meshShader());
  GLint *mShP = ShaderFactory::meshShaderParm();        
  glUniform1i(mShP[31], 1); // renderingClearView
  glUniform1i(mShP[32], 0); // processClearView
  
  render(MVP, vd, scrW, scrH, nclip, false, false);

  m_renderingClearView = false;
  glUseProgram(ShaderFactory::meshShader());
  mShP = ShaderFactory::meshShaderParm();        
  glUniform1i(mShP[31], 0); // renderingClearView
}


void
Trisets::renderFromCamera(GLdouble *MVP, Vec vd,
			  int scrW, int scrH, int nclip)
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
  
  m_renderingClearView = false;
  glUseProgram(ShaderFactory::meshShader());
  GLint *mShP = ShaderFactory::meshShaderParm();        
  glUniform1i(mShP[31], 0); // renderingClearView
  glUniform1i(mShP[32], 1); // processClearView
  glUniform1i(mShP[33], 6); // ClearViewDepthTex

  render(MVP, vd, scrW, scrH, nclip, false, false);
}

void
Trisets::renderFromShadowCamera(GLdouble *MVP, Vec vd,
				int scrW, int scrH, int nclip)
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
  
  
  m_renderingClearView = false;
  glUseProgram(ShaderFactory::meshShader());
  GLint *mShP = ShaderFactory::meshShaderParm();        
  glUniform1i(mShP[31], 0); // renderingClearView
  glUniform1i(mShP[32], 0); // processClearView


  render(MVP, vd, scrW, scrH, nclip, false, true);
}

void
Trisets::renderShadows(GLint drawFboId, int wd, int ht,
		       Vec shadowCam, float sceneRadius, Vec shadowViewDir)
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
  
  
  QMatrix4x4 mvp;
  mvp.setToIdentity();
  mvp.ortho(0.0, wd, 0.0, ht, 0.0, 1.0);
  
  glUniformMatrix4fv(shadowParm[0], 1, GL_FALSE, mvp.data());
  
  glUniform1i(shadowParm[1], 0); // colors
  glUniform1i(shadowParm[2], 1); // actual-to-shadow depth
  glUniform1f(shadowParm[3], m_blur); // soft shadows
  glUniform1f(shadowParm[4], m_edges); // edge enhancement
  glUniform1f(shadowParm[5], Global::gamma()); // edge enhancement
  float roughness = m_trisets[0]->roughness()*0.1;
  glUniform1f(shadowParm[6], roughness); // specularity
  glUniform1f(shadowParm[7], m_trisets[0]->specular()); // specularity

  glUniform1f(shadowParm[10], m_shadowIntensity); // edge enhancement
  glUniform1f(shadowParm[11], m_valleyIntensity); // edge enhancement
  glUniform1f(shadowParm[12], m_peakIntensity); // edge enhancement
  
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
  
//  glActiveTexture(GL_TEXTURE3);
//  glDisable(GL_TEXTURE_RECTANGLE);
  
  glUseProgram(0);
  

  glEnable(GL_BLEND);
}

void
Trisets::renderShadowBox(GLint drawFboId, int wd, int ht,
			 Vec shadowCam, float sceneRadius, Vec shadowViewDir)
{
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glBindFramebuffer(GL_FRAMEBUFFER, drawFboId);

  //--------------------------------------
  // Draw shadow box
  //--------------------------------------
  glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);      
  glLineWidth(2);
  Vec bgcolor = Global::backgroundColor();
  float bgintensity = (0.3*bgcolor.x +
		       0.5*bgcolor.y +
		       0.2*bgcolor.z);
  float si = m_shadowIntensity;
  if (bgintensity < 0.5)
    si = si*0.5;
      
  Vec bmin, bmax, bcen, blen;
  allEnclosingBox(bmin, bmax);  
  bcen = (bmax + bmin)*0.5;
  blen = (bmax - bmin)*0.75;
  bmin = bcen - blen;
  bmax = bcen + blen;
  
  glColor4f(0.6,0.6,0.6, 0.6);
  StaticFunctions::drawEnclosingCube(bmin, bmax, false);
  
  glUseProgram(ShaderFactory::planeShadowShader());
  GLint *shadowParm = ShaderFactory::planeShadowShaderParm();        
  
  glActiveTexture(GL_TEXTURE3);
  glEnable(GL_TEXTURE_RECTANGLE);
  glBindTexture(GL_TEXTURE_RECTANGLE, m_depthTex[3]); // shadow depth
  
  glUniform1f(shadowParm[0], Global::gamma()); // edge enhancement
  glUniform1i(shadowParm[1], 3); // shadow depth
  glUniform1f(shadowParm[2], si); // edge enhancement
  glUniform3f(shadowParm[3], shadowCam.x, shadowCam.y, shadowCam.z);
  glUniform1f(shadowParm[4], sceneRadius);
  glUniformMatrix4fv(shadowParm[5], 1, GL_FALSE, m_mvpShadow);
  glUniform2f(shadowParm[6], wd, ht); // screenSize
  glUniform3f(shadowParm[7], shadowViewDir.x, shadowViewDir.y, shadowViewDir.z); // light direction
  
  
  glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);      
  if (bgintensity > 0.5)
    glColor4f(0.4,0.4,0.4, 0.5);
  else
    glColor4f(0.8,0.8,0.8, 0.5);
  
  glEnable(GL_POLYGON_OFFSET_FILL);
  glPolygonOffset(1.0, 1.0);
  
  glEnable(GL_CULL_FACE);
  glCullFace(GL_FRONT);
  StaticFunctions::drawEnclosingCube(bmin, bmax, false);
  glDisable(GL_CULL_FACE);
  
  glActiveTexture(GL_TEXTURE3);
  glDisable(GL_TEXTURE_RECTANGLE);
  
  glUseProgram(0);
}

void
Trisets::renderOutline(GLint drawFboId,
		       GLdouble *MVP, Vec viewDir, int scrW, int scrH,
		       int nclip, float sceneRadius)
{
  bool opOn = false;
  for(int i=0; i<m_trisets.count();i++)
    {      
      if (m_trisets[i]->outline() >= 0.05) // outline
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
  
  glActiveTexture(GL_TEXTURE3);
  glEnable(GL_TEXTURE_RECTANGLE);
  glBindTexture(GL_TEXTURE_RECTANGLE, m_depthTex[3]); // shadow depthTex
	
  render(MVP, viewDir, scrW, scrH, nclip, true, false);
  
  glActiveTexture(GL_TEXTURE3);
  glDisable(GL_TEXTURE_RECTANGLE);

  glBindFramebuffer(GL_FRAMEBUFFER, drawFboId);

  glEnable(GL_BLEND);
  //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);


  glUseProgram(ShaderFactory::outlineShader());
  GLint *outlineParm = ShaderFactory::outlineShaderParm();        

  
  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_TEXTURE_RECTANGLE);
  glBindTexture(GL_TEXTURE_RECTANGLE, m_depthTex[0]); // colors
  
  glActiveTexture(GL_TEXTURE1);
  glEnable(GL_TEXTURE_RECTANGLE);
  glBindTexture(GL_TEXTURE_RECTANGLE, m_depthTex[1]); // depth
  
  
  QMatrix4x4 mvp;
  mvp.setToIdentity();
  mvp.ortho(0.0, scrW, 0.0, scrH, 0.0, 1.0);
  
  glUniformMatrix4fv(outlineParm[0], 1, GL_FALSE, mvp.data());
    

  glUniform1i(outlineParm[1], 0); // colors
  glUniform1i(outlineParm[2], 1); // actual-to-shadow depth
  glUniform1f(outlineParm[3], m_blur); // soft shadows
  glUniform1f(outlineParm[4], m_edges); // edge enhancement
  glUniform1f(outlineParm[5], Global::gamma()); // edge enhancement
  float roughness = m_trisets[0]->roughness()*0.1;
  glUniform1f(outlineParm[6], roughness); // specularity
  glUniform1f(outlineParm[7], m_trisets[0]->specular()); // specularity
  glUniform1f(outlineParm[10], m_shadowIntensity); // edge enhancement
  glUniform1f(outlineParm[11], m_valleyIntensity); // edge enhancement
  glUniform1f(outlineParm[12], m_peakIntensity); // edge enhancement
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
Trisets::renderGrabbedOutline(GLint drawFboId,
			      GLdouble *MVP, Vec viewDir, int scrW, int scrH)
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

  float otln = 0.5;
//  if (bgintensity > 0.5)
//    otln = 0.2;
  
  for(int i=0; i<m_trisets.count();i++)
    {
      if (m_trisets[i]->clipped())
	continue;
      
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
	  glUniform1i(meshShaderParm[18], matId);
	  if (matId > 0)
	    {
	      float matMix = m_trisets[i]->materialMix();
	      glActiveTexture(GL_TEXTURE1);
	      glEnable(GL_TEXTURE_2D);
	      glBindTexture(GL_TEXTURE_2D, m_solidTex[matId-1]);
	      glUniform1i(meshShaderParm[19], 1); // matcapTex
	      glUniform1f(meshShaderParm[20], matMix); // matMix
	    }

	  // force no clipping
	  glUniform1iARB(meshShaderParm[9],  0);
	  
	  //---
	  float ot = m_trisets[i]->outline();
	  float op = m_trisets[i]->opacity();

	  m_trisets[i]->setOutline(ot+otln);

	  m_trisets[i]->setOpacity(0.7);

//	  if (ot < 0.05)
//	    m_trisets[i]->setOpacity(0.0);
//	  else
//	    m_trisets[i]->setOpacity(0.7);

//	  if (m_trisets[i]->grabsMouse())
//	      m_trisets[i]->setOutline(ot+0.5);	      
	    
	  
	  m_trisets[i]->draw(MVP, viewDir, true);

	  
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

  
  //Vec grabbedColor = Vec(bgintensity,1.0-bgintensity,1.0-bgintensity);  
  Vec grabbedColor = Vec(1,0,0);
  

  glUseProgram(ShaderFactory::outlineShader());
  GLint *outlineParm = ShaderFactory::outlineShaderParm();        

  
  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_TEXTURE_RECTANGLE);
  glBindTexture(GL_TEXTURE_RECTANGLE, m_depthTex[0]); // colors
  
  glActiveTexture(GL_TEXTURE1);
  glEnable(GL_TEXTURE_RECTANGLE);
  glBindTexture(GL_TEXTURE_RECTANGLE, m_depthTex[1]); // depth
  
  
  QMatrix4x4 mvp;
  mvp.setToIdentity();
  mvp.ortho(0.0, scrW, 0.0, scrH, 0.0, 1.0);
  
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
//  glUniform1f(outlineParm[6], -1.0); // roughness
  glUniform1f(outlineParm[6], 0.0); // roughness
  glUniform1f(outlineParm[7], m_trisets[0]->specular()); // specularity
//  glUniform1f(outlineParm[10], m_shadowIntensity); // edge enhancement
//  glUniform1f(outlineParm[11], m_valleyIntensity); // edge enhancement
//  glUniform1f(outlineParm[12], m_peakIntensity); // edge enhancement
  glUniform1f(outlineParm[10], 0); // edge enhancement
  glUniform1f(outlineParm[11], 0); // edge enhancement
  glUniform1f(outlineParm[12], 0); // edge enhancement
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
			   GLdouble *MVP, GLfloat *MV,
			   Vec viewDir, Vec lightDir, int scrW, int scrH,			   
			   int nclip, float sceneRadius)
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

//  Vec rightVec = viewer->camera()->rightVector();
//  Vec upVec = viewer->camera()->upVector();
  
  glUniform3f(oitShaderParm[1], viewDir.x, viewDir.y, viewDir.z); // view direction
//  glUniform3f(oitShaderParm[20], rightVec.x, rightVec.y, rightVec.z);
//  glUniform3f(oitShaderParm[21], upVec.x, upVec.y, upVec.z);
  glUniform3f(oitShaderParm[28], lightDir.x, lightDir.y, lightDir.z); // light direction

  GLfloat mv[16];
  for(int im=0; im<16; im++) mv[im] = MV[im];
  
  glUniformMatrix4fv(oitShaderParm[4], 1, GL_FALSE, mv);

  glUniform1f(oitShaderParm[12], sceneRadius);
  glUniform1i(oitShaderParm[20], Global::hideBlack());
  

  for(int i=0; i<m_trisets.count();i++)
    {
      if (m_trisets[i]->reveal() < 0.0) // transparent reveal
	{
	  Vec extras = Vec(0,0,0);
	  if (i == m_active)
	    extras.x = 0.2;
      
	  extras.y = m_trisets[i]->reveal()*m_trisets[i]->reveal();
	  
	  extras.z = m_trisets[i]->glow();

	  float darken = 1;
	  if (glowOn && m_trisets[i]->glow() < 0.01)
	    darken = 1.0 - 0.8*m_trisets[i]->dark();
	  
	  glUseProgram(ShaderFactory::oitShader());
	  GLint *oitShaderParm = ShaderFactory::oitShaderParm();        
	  
	  glUniform4f(oitShaderParm[2], extras.x, extras.y, extras.z, darken);
	  
	  glUniform1f(oitShaderParm[17], i+1);
	  
	  int matId = m_trisets[i]->material();
	  glUniform1i(oitShaderParm[18], matId);
	  if (matId > 0)
	    {
	      float matMix = m_trisets[i]->materialMix();
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
	  

	  m_trisets[i]->drawOIT(MVP, viewDir,
				i == m_active);
	  
	  
	  if (matId > 0)
	    {
	      glActiveTexture(GL_TEXTURE1);
	      glDisable(GL_TEXTURE_2D);
	    }
	  	  
	  
	  glUseProgramObjectARB(0);
	}
    }

  //glBindFramebuffer(GL_FRAMEBUFFER, 0);

  glDisable(GL_BLEND);


  // ------------------
  // ------------------


  glBindFramebuffer(GL_FRAMEBUFFER, drawFboId);

  drawOITTextures(scrW, scrH);
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

  glClearDepth(1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  GLfloat c0[4] = {0,0,0,0};
  GLfloat c1[4] = {1,1,1,1};
  
  //glClear(GL_COLOR_BUFFER_BIT);
  glClearBufferfv(GL_COLOR, 0, c0);
  glClearBufferfv(GL_COLOR, 1, c1);  


  glEnable(GL_BLEND);
  glBlendEquation(GL_FUNC_ADD);
  glBlendFunci(0, GL_ONE, GL_ONE);
  glBlendFunci(1, GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
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
  float roughness = m_trisets[0]->roughness()*0.1;
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

//----------------------------
//----------------------------

void
Trisets::duplicateMesh(int i)
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
      return;
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
  
  emit updateMeshList(getMeshList());
  emit updateGL();
}


void
Trisets::removeHoveredHitPoint()
{
  for (int ma=0; ma<m_multiActive.count(); ma++)
    {
      m_trisets[m_multiActive[ma]]->removeHoveredHitPoint();
    }
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

  
  if (event->key() == Qt::Key_X)
    m_trisets[idx]->setMoveAxis(TrisetGrabber::MoveX);
  else if (event->key() == Qt::Key_Y)
    m_trisets[idx]->setMoveAxis(TrisetGrabber::MoveY);
  else if (event->key() == Qt::Key_Z)
    m_trisets[idx]->setMoveAxis(TrisetGrabber::MoveZ);
  else if (event->key() == Qt::Key_W)
    m_trisets[idx]->setMoveAxis(TrisetGrabber::MoveAll);
  
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
  m_trisets[idx]->save(m_nclip, m_cpos, m_cnormal);
}

// Single Mesh Command
void
Trisets::processCommand(int idx, QString cmd)
{
  bool ok;
  cmd = cmd.toLower();
  QStringList list = cmd.split(" ", QString::SkipEmptyParts);


  
  if (list[0] == "savepoints")
    {
      saveHitPoints(idx);
      return;
    }

  if (list[0] == "loadpoints")
    {
      loadHitPoints(idx);
      return;
    }

  if (list[0] == "clearpoints")
    {
      m_trisets[idx]->clearHitPoints();
      return;
    }

  if (list[0] == "smoothcolors")
    {
      smoothVertexColors(idx);
      return;
    }
  
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
      Vec bgcolor = Global::backgroundColor();
      float bgintensity = (0.3*bgcolor.x +
			   0.5*bgcolor.y +
			   0.2*bgcolor.z);
      QColor ccol = Qt::white;
      if (bgintensity > 0.5)
	ccol = Qt::black;
      
      CaptionDialog cd(0,
		       "",
		       QFont(QFont("MS Reference Sans Serif", 16)),
		       ccol,
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

  if (list[0] == "clearpoints") // remove all not attached points
    {
      m_hitpoints->clear();
      emit updateGL();
      return;
    }
  
  if (list[0] == "savepoints")
    {
      saveHitPoints();
      return;
    }
  
  if (list[0] == "loadpoints")
    {
      loadHitPoints();
      return;
    }
  
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

      setExplode(rad, damp, Vec(ex, ey, ez));
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

      emit removeAllKeyFrames();
      emit resetBoundingBox();      
      emit updateMeshList(getMeshList());
      emit clearScene();
      emit updateGL();
      
      return;
    }
  
  if (list[0] == "merge")
    {
      QList<int> indices;
      for (int i=0; i<m_trisets.count(); i++)
	indices << i;
      mergeSurfaces(indices);
      return;
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
      int clearView = (m_trisets[i]->clearView() ? 1 : 0);

      Vec pcolor = m_trisets[i]->color();
      QString color = QColor::fromRgbF(pcolor.x,
				       pcolor.y,
				       pcolor.z).name();
      
      int matId = m_trisets[i]->material();
      float matMix = m_trisets[i]->materialMix();
      
      names << QString("%1 %2 %3 %4 %5 %6 %7 %8 %9 %10").\
	arg((int)nm.length(), 5, 10, QChar(' ')).\
	arg(nm).\
	arg(show).\
	arg(clip).\
	arg(clearView).\
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
  m_active = -1;

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
	    }
	  else
	    delete tgi;
	}

      emit updateMeshList(getMeshList());
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

  // disconnect all connections
  for(int i=0; i<tg.count(); i++)
    tg[i]->disconnect();
  
  m_trisets.clear();
  for(int i=0; i<tinfo.count(); i++)
    {
      TrisetGrabber *tgi;

      if (present[i] >= 0)
	tgi = tg[present[i]];
      else
	tgi = new TrisetGrabber();

	
      if (tgi->set(tinfo[i]))
	{
	  m_trisets.append(tgi);
	  connect(tgi, SIGNAL(updateParam()),
		  this, SLOT(sendParametersToMenu()));
	  connect(tgi, SIGNAL(meshGrabbed()),
		  this, SLOT(meshGrabbed()));
	  connect(tgi, SIGNAL(posChanged()),
		  this, SLOT(posChanged()));
	}
      else
	delete tgi;
    }

  for(int i=0; i<tg.count(); i++)
    {
      if (! m_trisets.contains(tg[i]))
	{
	  disconnect(tg[i]);
	  delete tg[i];
	}
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

  if (!m_vertexScreenBuffer) glGenBuffers(1, &m_vertexScreenBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, m_vertexScreenBuffer);
  glBufferData(GL_ARRAY_BUFFER,
	       8*sizeof(float),
	       &m_scrGeo,
	       GL_STATIC_DRAW);
  //------------------


  if (m_depthBuffer) glDeleteFramebuffers(1, &m_depthBuffer);
  if (m_depthTex) glDeleteTextures(6, m_depthTex);
  if (m_rbo) glDeleteRenderbuffers(1, &m_rbo);

  glGenFramebuffers(1, &m_depthBuffer);
  glGenTextures(6, m_depthTex);
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

  for(int dt=0; dt<6; dt++)
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
  QFont font;
  font.setPointSize(12);
  QInputDialog* dialog = new QInputDialog(0, Qt::Popup);
  dialog->setFont(font);
  dialog->setWindowTitle("Color Gradient");
  dialog->setLabelText("Color Gradient");
  dialog->setComboBoxItems(glist);
  dialog->setTextValue(glist[0]);
  dialog->setComboBoxEditable(false);
  int cdW = dialog->width();
  int cdH = dialog->height();
  dialog->move(QCursor::pos()-QPoint(100,0));
  int ret = dialog->exec();
  if (!ret)
    return;

  QString gstr = dialog->textValue();

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
      
      QInputDialog* dialog = new QInputDialog(0, Qt::Popup);
      dialog->setFont(font);
      dialog->setWindowTitle("Select Hue");
      dialog->setLabelText("Hue");
      dialog->setComboBoxItems(clist);
      dialog->setTextValue(clist[0]);
      dialog->setComboBoxEditable(false);
      int cdW = dialog->width();
      int cdH = dialog->height();
      dialog->move(QCursor::pos()-QPoint(100,0));
      int ret = dialog->exec();
      if (!ret)
	return;

      QString cstr = dialog->textValue();

//      bool ok;
//      QString cstr = QInputDialog::getItem(0,
//					   "Select Hue",
//					   "Hue",
//					   clist, 0, false,
//					   &ok);
//      if (!ok)
//	return;
      
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
	      stops << QGradientStop(pos, QColor(r,g,b,255));
	    }
	}
    }


  
  QGradientStops gstops;
  gstops = StaticFunctions::resampleGradientStops(stops, m_trisets.count());

  uchar *colors = Global::tagColors();  
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
  if (m_active < 0)
    return;
  
  QMap<QString, QVariantList> plist;
  
  QVariantList vlist;

  vlist.clear();
  vlist << QVariant(m_active);
  plist["idx"] = vlist;

  vlist.clear();
  vlist << QVariant(m_trisets[m_active]->lineMode());
  plist["linemode"] = vlist;
  
  vlist.clear();
  vlist << QVariant(m_trisets[m_active]->lineWidth());
  plist["linewidth"] = vlist;
  
  Vec pos = m_trisets[m_active]->position();
  vlist.clear();
  vlist << QVariant(QString("%1 %2 %3").arg(pos.x).arg(pos.y).arg(pos.z));
  plist["position"] = vlist;
  
  Quaternion rot = m_trisets[m_active]->rotation();
  vlist.clear();
  float a = rot.angle();
  pos = rot.axis();
  a = RAD2DEG(a);
  vlist << QVariant(QString("%1 %2 %3 %4").arg(pos.x).arg(pos.y).arg(pos.z).arg(a));
  plist["rotation"] = vlist;
  
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
Trisets::lineModeChanged(bool b)
{
  for (int i=0; i<m_multiActive.count(); i++)
    m_trisets[m_multiActive[i]]->setLineMode(b);

  emit updateGL();
}

void
Trisets::lineWidthChanged(int f)
{
  for (int i=0; i<m_multiActive.count(); i++)
    m_trisets[m_multiActive[i]]->setLineWidth(f);

  emit updateGL();
}

void
Trisets::transparencyChanged(int v)
{
  for (int i=0; i<m_multiActive.count(); i++)
    m_trisets[m_multiActive[i]]->setOpacity(v*0.1);
  
  //m_trisets[m_active]->setOpacity(v*0.1);
  emit updateGL();
}

void
Trisets::revealChanged(int v)
{
  for (int i=0; i<m_multiActive.count(); i++)
    m_trisets[m_multiActive[i]]->setReveal(v*0.1);
  //m_trisets[m_active]->setReveal(v*0.1);
  emit updateGL();
}

void
Trisets::outlineChanged(int v)
{
  for (int i=0; i<m_multiActive.count(); i++)
    m_trisets[m_multiActive[i]]->setOutline(v*0.1);
  //m_trisets[m_active]->setOutline(v*0.1);
  emit updateGL();
}

void
Trisets::glowChanged(int v)
{
  for (int i=0; i<m_multiActive.count(); i++)
    m_trisets[m_multiActive[i]]->setGlow(v*0.1);
  //  m_trisets[m_active]->setGlow(v*0.1);
  emit updateGL();
}

void
Trisets::darkenChanged(int v)
{
  for (int i=0; i<m_multiActive.count(); i++)
    m_trisets[m_multiActive[i]]->setDark(v*0.1);
  //m_trisets[m_active]->setDark(v*0.1);
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
  if (m_trisets[m_active]->haveBlack())
    {
      bool ok;
      QStringList vlist;
      vlist << "No";
      vlist << "Yes";
      QString vstr = QInputDialog::getItem(0,
					   QWidget::tr("Deleted Vertices"),
					   QWidget::tr("Restore Deleted Vertices"),
					   vlist, 0, false,
					   &ok);
      if (ok && !vstr.isEmpty())
	{
	  if (vstr == "Yes")
	    ignoreBlack = true;
	}
    }
  m_trisets[m_active]->setColor(pcolor, ignoreBlack);
  emit updateGL();
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
Trisets::scaleChanged(QVector3D scl)
{
  for (int i=0; i<m_multiActive.count(); i++)
    m_trisets[m_multiActive[i]]->setScale(Vec(scl.x(),
					      scl.y(),
					      scl.z()));
  emit updateScaling();
  //emit updateGL();
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
Trisets::rotationChanged(QVector4D rot)
{
  Vec axis = Vec(rot.x(), rot.y(), rot.z());
  double angle = DEG2RAD(rot.w());
  Quaternion q(axis, angle);
  for (int i=0; i<m_multiActive.count(); i++)
    m_trisets[m_multiActive[i]]->setRotation(q);
  
  emit updateGL();
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
Trisets::paint(Vec hitPt, float rad, Vec color,
	       int blendType, float blendFraction,
	       int blendOctave, int roughnessType)
{
  glUseProgram(ComputeShaderFactory::paintShader());

  GLint* paintShaderParm = ComputeShaderFactory::paintShaderParm();
  
  //glUniform3f(paintShaderParm[0], hitPt.x, hitPt.y, hitPt.z);
  glUniform1f(paintShaderParm[1], rad);
  glUniform3f(paintShaderParm[2], color.x, color.y, color.z);
  glUniform1i(paintShaderParm[3], blendType);
  glUniform1f(paintShaderParm[4], blendFraction);
  glUniform1i(paintShaderParm[5], blendOctave);

  glUniform1i(paintShaderParm[8], roughnessType);

  for (int i=0; i<m_trisets.count(); i++)
    {
      Vec pos = m_trisets[i]->position();
      Vec hp = hitPt - pos;
      glUniform3f(paintShaderParm[0], hp.x, hp.y, hp.z);

      Vec bmin, bmax;
      m_trisets[i]->enclosingBox(bmin, bmax);

      float blen = qMax(bmax.x-bmin.x, qMax(bmax.y-bmin.y, bmax.z-bmin.z));
      glUniform3f(paintShaderParm[6], bmin.x, bmin.y, bmin.z);
      glUniform1f(paintShaderParm[7], blen);
      
      m_trisets[i]->paint(hp);
    }
    
  glUseProgram(0);
}

void
Trisets::mergeSurfaces(QList<int> indices)
{
  QVector<float> vertices;
  QVector<float> normals;
  QVector<float> colors;
  QVector<uint> triangles;
  
  uint nvert = 0;
  for (int i=0; i<indices.count(); i++)
    {
      Global::progressBar()->setValue((int)(100.0*(float)(i)/(float)(indices.count())));
      qApp->processEvents();
      int idx = indices[i];
      vertices += m_trisets[idx]->vertices();
      normals += m_trisets[idx]->normals();
      colors += m_trisets[idx]->colors();

      QVector<uint> tri = m_trisets[idx]->triangles();
      for(int t=0; t<tri.count(); t++)
	{
	  triangles << nvert + tri[t];
	}
      
      nvert = vertices.count()/3;
    }

  MainWindowUI::mainWindowUI()->statusBar->showMessage("Saving merged surfaces");
  
  double s[16];
  Matrix::identity(s);
  StaticFunctions::savePLY(vertices,
			   normals,
			   colors,
			   triangles,
			   &s[0],
			   Global::previousDirectory());
  
  
  MainWindowUI::mainWindowUI()->statusBar->showMessage("");  
}

void
Trisets::saveHitPoints()
{
  QString flnm;
  flnm = QFileDialog::getSaveFileName(0,
				      "Save non-attached points to text file",
				      Global::previousDirectory(),
				      "Files (*.txt)");
  
  if (flnm.isEmpty())
    return;
  
  m_hitpoints->savePoints(flnm);
}

void
Trisets::loadHitPoints()
{
  QString flnm;
      flnm = QFileDialog::getOpenFileName(0,
					  "Load non-attached points from text file",
					  Global::previousDirectory(),
					  "Files (*.txt)");  
  if (flnm.isEmpty())
    return;
  
  m_hitpoints->addPoints(flnm);

  emit updateGL();
}

void
Trisets::saveHitPoints(int idx)
{
  QString flnm;
  flnm = QFileDialog::getSaveFileName(0,
				      "Save attached points to text file",
				      Global::previousDirectory(),
				      "Files (*.txt)");
  
  if (flnm.isEmpty())
    return;

  QList<Vec> hpts;
  hpts = m_trisets[idx]->hitPoints();
  
  int npts = hpts.count();
  fstream fp(flnm.toLatin1().data(), ios::out);
  fp << npts << "\n";
  for(int i=0; i<npts; i++)
    {
      Vec pt = hpts[i];
      fp << pt.x << " " << pt.y << " " << pt.z << "\n";
    }
  fp.close();

  QMessageBox::information(0, "Save attached points", "Saved attached points to "+flnm);
}

void
Trisets::loadHitPoints(int idx)
{
  QString flnm;
      flnm = QFileDialog::getOpenFileName(0,
					  "Load attached points from text file",
					  Global::previousDirectory(),
					  "Files (*.txt)");  
  if (flnm.isEmpty())
    return;
  
  QList<Vec> pts = m_hitpoints->readPointsFromFile(flnm);
  for (int i=0; i<pts.count(); i++)
    m_trisets[idx]->addHitPoint(pts[i]);


  QMessageBox::information(0, "", "Loaded attached points");
  
  emit updateGL();
}

void
Trisets::selectionWindow(Camera *camera, QRect selectionWindow)
{
  emit clearSelection();
  m_active = -1;
  m_multiActive.clear();

  QList<int> mg;
  for (int i=0; i<m_trisets.count(); i++)
    {
      Vec pt = camera->projectedCoordinatesOf(m_trisets[i]->tcentroid());
      if (selectionWindow.contains(pt.x, pt.y))
	mg << i;
    }

  if (mg.count() > 0)
    {
      emit meshesGrabbed(mg);
      multiSelection(mg);
      setActive(mg[0], true);
    }
}

void
Trisets::pointProbe(Vec v, float threshold)
{
  setGrab(true);
  float dmin = 100000000;
  int idx = -1;
  for (int i=0; i<m_trisets.count(); i++)
    {
      float d = (m_trisets[i]->tcentroid()-v).norm(); 
      if (d < dmin)
	{
	  dmin = d;
	  idx = i;
	}
    }
  if (dmin < threshold)
    {
      m_trisets[idx]->setMouseGrab(true);
      m_probeStartPos = m_trisets[idx]->position();
      emit probeMeshName(QFileInfo(m_trisets[idx]->filename()).fileName());
    }
}

void
Trisets::probeMeshMove(Vec pos)
{
  for (int i=0; i<m_trisets.count(); i++)
    {
      if (m_trisets[i]->grabsMouse())
	{
	  m_trisets[i]->setPosition(m_probeStartPos + pos);
	  break;
	}
    }  
}

void
Trisets::resetProbe()
{
  setGrab(false);
  for (int i=0; i<m_trisets.count(); i++)
    {
      m_trisets[i]->setMouseGrab(false);
    }
}

void
Trisets::smoothVertexColors(int idx)
{ 
  m_active = idx;
  m_trisets[idx]->copyToOrigVcolor();

  if (m_dialog)
    {
      m_dialog->close();
      delete m_dialog;
      m_dialog = 0;
    }

  m_dialog = new QDialog();
  m_dialog->setWindowFlag(Qt::WindowStaysOnTopHint,true);
  
  QFont font;
  font.setPointSize(14);
  m_dialog->setFont(font);

  bool deleteOnClose = m_dialog->testAttribute(Qt::WA_DeleteOnClose);
  m_dialog->setAttribute(Qt::WA_DeleteOnClose, false);
  m_dialog->setWindowTitle("Vertex Color Smoothing");
  m_dialog->move(QCursor::pos()-QPoint(100,0));
  m_dialog->resize(300, 70);
  
  QHBoxLayout *hlayout = new QHBoxLayout();
  m_dialog->setLayout(hlayout);
  
  PopUpSlider *slider = new PopUpSlider();
  slider->setRange(0, 50);
  slider->setValue(0);
  slider->setText("Smoothing Iterations");
  hlayout->addWidget(slider);
  
  connect(slider, SIGNAL(valueChanged(int)),
	  this, SLOT(applyVertexColorSmoothing(int)));
  connect(m_dialog, SIGNAL(done(int)),
	  this, SLOT(deleteDialog(int)));
	  
  m_dialog->show();
  m_dialog->raise();
  m_dialog->activateWindow();
}

void
Trisets::deleteDialog(int r)
{
  m_dialog->close();
  delete m_dialog;
  m_dialog = 0;
}

void
Trisets::applyVertexColorSmoothing(int niter)
{
  if (m_active >= 0 && m_active < m_trisets.count())
    {
      m_trisets[m_active]->smoothVertexColors(niter);
      emit updateGL();
    }
}
