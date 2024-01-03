#include <GL/glew.h>
#include "scalebar.h"
#include "global.h"

ScaleBars::ScaleBars()
{
  m_scalebars.clear();
}

ScaleBars::~ScaleBars() { clear(); }

bool ScaleBars::isValid() { return (m_scalebars.count() > 0); }

void
ScaleBars::clear()
{
  for(int i=0; i<m_scalebars.count(); i++)
    m_scalebars[i]->removeFromMouseGrabberPool();

  for(int i=0; i<m_scalebars.count(); i++)
    delete m_scalebars[i];

  m_scalebars.clear();
}

QList<ScaleBarObject>
ScaleBars::scalebars()
{
  QList<ScaleBarObject> scalist;
  for(int i=0; i<m_scalebars.count(); i++)
    scalist.append(m_scalebars[i]->scalebar());

  return scalist;
}


bool
ScaleBars::grabsMouse()
{
  for(int i=0; i<m_scalebars.count(); i++)
    {
      if (m_scalebars[i]->grabsMouse())
	return true;
    }
  return false;
}

void
ScaleBars::add(ScaleBarObject cap)
{
  ScaleBarGrabber *cmg = new ScaleBarGrabber();
  cmg->setScaleBar(cap);
  m_scalebars.append(cmg);
}

void
ScaleBars::setScaleBars(QList<ScaleBarObject> caps)
{
  clear();

  for(int i=0; i<caps.count(); i++)
    add(caps[i]);
}

#define VECPRODUCT(a, b) Vec(a.x*b.x, a.y*b.y, a.z*b.z)

void
ScaleBars::draw(QGLViewer *viewer,
		ClipInformation clipInfo)
{
  Vec voxelScaling = Global::voxelScaling();

  glDepthMask(GL_FALSE); // disable writing to depth buffer
  glDisable(GL_DEPTH_TEST);

  int screenWidth = viewer->camera()->screenWidth();
  int screenHeight = viewer->camera()->screenHeight();
  float pgr0 = viewer->camera()->pixelGLRatio(viewer->camera()->sceneCenter());
  int viewWidth = viewer->size().width();
  int viewHeight = viewer->size().height();

  // get pixelGLRatios for all virtual cameras
  QList<float> clippgr;
  for(int ic=0; ic<clipInfo.size(); ic++)
    {
      QVector4D vp = clipInfo.viewport[ic];
      int vx = vp.x()*screenWidth;
      int vy = vp.y()*screenHeight;
      int vw = vp.z()*screenWidth;
      int vh = vp.w()*screenHeight;
      Camera clipCam;
      clipCam.setOrientation(clipInfo.rot[ic]);	  
      Vec cpos0 = VECPRODUCT(clipInfo.pos[ic], voxelScaling);
      clipCam.setSceneCenter(cpos0);
      Vec cpos = cpos0 -
	clipCam.viewDirection()*viewer->sceneRadius()*2*(1.0/clipInfo.viewportScale[ic]);
      clipCam.setPosition(cpos);
      clipCam.setScreenWidthAndHeight(vw, vh);
      clipCam.loadProjectionMatrix(true);
      clipCam.loadModelViewMatrix(true);
      clippgr << clipCam.pixelGLRatio(cpos0);
    }


  viewer->startScreenCoordinatesSystem();
  for(int i=0; i<m_scalebars.count(); i++)
    {
      float pgr = pgr0;
      QPointF pos = m_scalebars[i]->position();
      int px = pos.x()*screenWidth;
      int py = pos.y()*screenHeight;
      for(int ic=0; ic<clipInfo.size(); ic++)
	{
	  QVector4D vp = clipInfo.viewport[ic];
	  int vx = vp.x()*screenWidth;
	  int vy = vp.y()*screenHeight;
	  int vw = vp.z()*screenWidth;
	  int vh = vp.w()*screenHeight;
	  if (px >= vx && px <= (vx+vw) &&
	      py >= (screenHeight-vy-vh) && py <= (screenHeight-vy))
	    {
	      pgr = clippgr[ic];
	      break;
	    }
	}

      m_scalebars[i]->draw(pgr,
			   screenWidth, screenHeight,
			   viewWidth, viewHeight,
			   m_scalebars[i]->grabsMouse());

    }
  viewer->stopScreenCoordinatesSystem();

  
  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_TRUE); // enable writing to depth buffer
  glEnable(GL_BLEND);
}

bool
ScaleBars::keyPressEvent(QKeyEvent *event)
{
  for(int i=0; i<m_scalebars.count(); i++)
    {
      if (m_scalebars[i]->grabsMouse())
	{
	  if (event->key() == Qt::Key_Delete ||
	      event->key() == Qt::Key_Backspace ||
	      event->key() == Qt::Key_Backtab)
	    {
	      m_scalebars[i]->removeFromMouseGrabberPool();
	      m_scalebars.removeAt(i);
	    }
	  else if (event->key() == Qt::Key_H)
	    m_scalebars[i]->setType(true);
	  else if (event->key() == Qt::Key_V)
	    m_scalebars[i]->setType(false);
	  else if (event->key() == Qt::Key_D ||
		   event->key() == Qt::Key_R)
	    m_scalebars[i]->setTextpos(true);
	  else if (event->key() == Qt::Key_U ||
		   event->key() == Qt::Key_L)
	    m_scalebars[i]->setTextpos(false);
	}
    }
  
  return true;
}
