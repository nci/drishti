#include <GL/glew.h>
#include "colorbar.h"
#include "global.h"

ColorBars::ColorBars()
{
  m_colorbars.clear();
}

ColorBars::~ColorBars() { clear(); }

bool ColorBars::isValid() { return (m_colorbars.count() > 0); }

void
ColorBars::clear()
{
  for(int i=0; i<m_colorbars.count(); i++)
    m_colorbars[i]->removeFromMouseGrabberPool();

  for(int i=0; i<m_colorbars.count(); i++)
    delete m_colorbars[i];

  m_colorbars.clear();
}

QList<ColorBarObject>
ColorBars::colorbars()
{
  QList<ColorBarObject> caplist;
  for(int i=0; i<m_colorbars.count(); i++)
    caplist.append(m_colorbars[i]->colorbar());

  return caplist;
}


bool
ColorBars::grabsMouse()
{
  for(int i=0; i<m_colorbars.count(); i++)
    {
      if (m_colorbars[i]->grabsMouse())
	return true;
    }
  return false;
}

void
ColorBars::add(ColorBarObject cap)
{
  ColorBarGrabber *cmg = new ColorBarGrabber();
  cmg->setColorBar(cap);
  m_colorbars.append(cmg);
}

void
ColorBars::setColorBars(QList<ColorBarObject> caps)
{
  clear();

  for(int i=0; i<caps.count(); i++)
    add(caps[i]);
}

void
ColorBars::draw(QGLViewer *viewer)
{
  glDisable(GL_DEPTH_TEST);

  int screenWidth = viewer->size().width();
  int screenHeight = viewer->size().height();

  // splat the colorbar images
  viewer->startScreenCoordinatesSystem();

  for(int i=0; i<m_colorbars.count(); i++)
    {
      ColorBarGrabber *cmg = m_colorbars[i];
      QPointF pos = cmg->position();
      int wd = cmg->width();
      int ht = cmg->height();

      int x = pos.x() * screenWidth;
      int y = pos.y() * screenHeight;

      glTexCoord2f(-1.0, -1.0);

      glDisable(GL_BLEND);

      glDisable(GL_TEXTURE_2D);
      if (cmg->grabsMouse())
	{
	  glLineWidth(2);
	  glColor3f(1.0f, 0.4f, 0.2f);
	  glBegin(GL_LINE_STRIP);
	  glVertex2f(x-5, y+2);
	  glVertex2f(x+wd+5, y+2);
	  glVertex2f(x+wd+5, y-ht-2);
	  glVertex2f(x-5, y-ht-2);
	  glVertex2f(x-5, y+2);
	  glEnd();
	}

      Vec bg = Global::backgroundColor();
      glLineWidth(1);

      if (bg.x*0.35+bg.y*0.45+bg.z*0.2 < 0.6)
	glColor3f(1.0f, 1.0f, 1.0f);
      else
	glColor3f(0.0f, 0.0f, 0.0f);

      glBegin(GL_LINE_STRIP);
      glVertex2f(x-2, y+2);
      glVertex2f(x+wd+2, y+2);
      glVertex2f(x+wd+2, y-ht-2);
      glVertex2f(x-2, y-ht-2);
      glVertex2f(x-2, y+2);
      glEnd();

      glEnable(GL_BLEND);
      glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // blend on top

      glEnable(GL_TEXTURE_2D);

      float tfset = cmg->tfset();

      glColor3f(1.0f, 1.0f, 1.0f);

      if (cmg->type() == 0)
	{
	  int ed0 = ht/2;
	  int ed1 = ht;
	  if (cmg->onlyColor())
	    ed0 = ht;

	  glBegin(GL_QUADS);
	  glTexCoord2f(0.0, tfset); glVertex2f(x, y);
	  glTexCoord2f(1.0, tfset); glVertex2f(x+wd, y);
	  glTexCoord2f(1.0, tfset); glVertex2f(x+wd, y-ed0);
	  glTexCoord2f(0.0, tfset); glVertex2f(x, y-ed0);
	  
	  if (!cmg->onlyColor())
	    {
	      glMultiTexCoord2f(GL_TEXTURE1, 0.0, 0.0); 
	      glTexCoord2f(2.0, tfset); glVertex2f(x, y-ed0-2);
	      glMultiTexCoord2f(GL_TEXTURE1, 0.0, 0.0); 
	      glTexCoord2f(3.0, tfset); glVertex2f(x+wd, y-ed0-2);
	      glMultiTexCoord2f(GL_TEXTURE1, 1.0, 0.0); 
	      glTexCoord2f(3.0, tfset); glVertex2f(x+wd, y-ed1);
	      glMultiTexCoord2f(GL_TEXTURE1, 1.0, 0.0); 
	      glTexCoord2f(2.0, tfset); glVertex2f(x, y-ed1);
	    }

	  glEnd();
	}
      else
	{
	  int ed0 = wd/2;
	  int ed1 = wd;
	  if (cmg->onlyColor())
	      ed0 = wd;

	  glBegin(GL_QUADS);
	  glTexCoord2f(0.0, tfset); glVertex2f(x, y);
	  glTexCoord2f(0.0, tfset); glVertex2f(x+ed0, y);
	  glTexCoord2f(1.0, tfset); glVertex2f(x+ed0, y-ht);
	  glTexCoord2f(1.0, tfset); glVertex2f(x, y-ht);

	  if (!cmg->onlyColor())
	    {
	      glMultiTexCoord2f(GL_TEXTURE1, 0.0, 0.0); 
	      glTexCoord2f(2.0, tfset); glVertex2f(x+ed0+2, y);
	      glMultiTexCoord2f(GL_TEXTURE1, 1.0, 0.0); 
	      glTexCoord2f(2.0, tfset); glVertex2f(x+ed1, y);
	      glMultiTexCoord2f(GL_TEXTURE1, 1.0, 0.0); 
	      glTexCoord2f(3.0, tfset); glVertex2f(x+ed1, y-ht);
	      glMultiTexCoord2f(GL_TEXTURE1, 0.0, 0.0); 
	      glTexCoord2f(3.0, tfset); glVertex2f(x+ed0+2, y-ht);
	    }

	  glEnd();
	}
    }

  viewer->stopScreenCoordinatesSystem();

  glEnable(GL_DEPTH_TEST);
}

bool
ColorBars::keyPressEvent(QKeyEvent *event)
{
  for(int i=0; i<m_colorbars.count(); i++)
    {
      if (m_colorbars[i]->grabsMouse())
	{
	  if (event->key() == Qt::Key_Delete ||
	      event->key() == Qt::Key_Backspace ||
	      event->key() == Qt::Key_Backtab)
	    {
	      m_colorbars.removeAt(i);
	    }
	  else if (event->key() >= Qt::Key_0 && event->key() <= Qt::Key_9)
	    {
	      ColorBarObject* cmg = m_colorbars[i];
	      float tfset = (event->key()-Qt::Key_0);
	      cmg->setTFset(tfset);
	    }
	  else if (event->key() == Qt::Key_H)
	    {
	      ColorBarObject* cmg = m_colorbars[i];
	      cmg->setType(0);
	    }
	  else if (event->key() == Qt::Key_V)
	    {
	      ColorBarObject* cmg = m_colorbars[i];
	      cmg->setType(1);
	    }
	  else if (event->key() == Qt::Key_O)
	    {
	      ColorBarObject* cmg = m_colorbars[i];
	      cmg->setOnlyColor(!cmg->onlyColor());
	    }
	}
    }
  
  return true;
}
