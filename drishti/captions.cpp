#include <GL/glew.h>

#include "captions.h"
#include "captiondialog.h"
#include <QInputDialog>

#include <QGLViewer/qglviewer.h>
using namespace qglviewer;

Captions::Captions()
{
  glGenTextures(1, &m_imageTex);
  m_captions.clear();
}

Captions::~Captions()
{
  clear();

  glDeleteTextures(1, &m_imageTex);
  m_imageTex = 0;
}

void
Captions::clear()
{
  for(int i=0; i<m_captions.count(); i++)
    m_captions[i]->removeFromMouseGrabberPool();

  for(int i=0; i<m_captions.count(); i++)
    delete m_captions[i];

  m_captions.clear();
}

QList<CaptionObject>
Captions::captions()
{
  QList<CaptionObject> caplist;
  for(int i=0; i<m_captions.count(); i++)
    caplist.append(m_captions[i]->caption());

  return caplist;
}


bool
Captions::grabsMouse()
{
  for(int i=0; i<m_captions.count(); i++)
    {
      if (m_captions[i]->grabsMouse())
	return true;
    }
  return false;
}

void
Captions::add(CaptionObject cap)
{
  CaptionGrabber *cmg = new CaptionGrabber();
  cmg->setCaption(cap);
  m_captions.append(cmg);
}

void
Captions::setCaptions(QList<CaptionObject> caps)
{
  clear();

  for(int i=0; i<caps.count(); i++)
    add(caps[i]);
}

void
Captions::draw(QGLViewer *viewer)
{
  glDisable(GL_DEPTH_TEST);

  int screenWidth = viewer->size().width();
  int screenHeight = viewer->size().height();

  // splat the caption images
  viewer->startScreenCoordinatesSystem();

  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // blend on top

  glLineWidth(2);
  for(int i=0; i<m_captions.count(); i++)
    {
      CaptionGrabber *cmg = m_captions[i];
      QFontMetrics metric(cmg->font());
      QPointF pos = cmg->position();
      int wd = cmg->width();
      int ht = cmg->height();

      int x = pos.x() * screenWidth;
      int y = pos.y() * screenHeight - metric.descent();

      if (cmg->grabsMouse())
	{
	  glPushMatrix();
	  glTranslatef(x+wd/2, y-ht/2, 0);
	  glRotatef(cmg->angle(), 0, 0, 1);
	  glTranslatef(-x-wd/2, -y+ht/2, 0);

	  glColor3f(1.0f, 0.4f, 0.2f);
	  glBegin(GL_LINE_STRIP);
	  glVertex2f(x-5, y+2);
	  glVertex2f(x+wd+5, y+2);
	  glVertex2f(x+wd+5, y-ht-2);
	  glVertex2f(x-5, y-ht-2);
	  glVertex2f(x-5, y+2);
	  glEnd();

	  glPopMatrix();
	}

      QImage cimage = cmg->image();

      int px = x;
      int py = y;
      if (px < 0 || py > screenHeight)
	{
	  int wd = cimage.width();
	  int ht = cimage.height();
	  int sx = 0;
	  int sy = 0;
	  if (px < 0)
	    {
	      wd = cimage.width()+px;
	      sx = -px;
	      px = 0;
	    }
	  if (py > screenHeight)
	    {
	      ht = cimage.height()-(py-screenHeight);
	      sy = (py-screenHeight);
	      py = screenHeight;
	    }

	  cimage = cimage.copy(sx, sy, wd, ht);
	}

      cimage = cimage.scaled(cimage.width()*viewer->camera()->screenWidth()/
			     viewer->size().width(),
			     cimage.height()*viewer->camera()->screenHeight()/
			     viewer->size().height());


      glPushMatrix();
      glTranslatef(x+wd/2, y-ht/2, 0);
      glRotatef(cmg->angle(), 0, 0, 1);
      glTranslatef(-x-wd/2, -y+ht/2, 0);

      wd = cimage.width();
      ht = cimage.height();
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_imageTex);
      glTexParameterf(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
      glTexParameterf(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
      glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexImage2D(GL_TEXTURE_RECTANGLE_ARB,
		   0,
		   4,
		   wd,
		   ht,
		   0,
		   GL_RGBA,
		   GL_UNSIGNED_BYTE,
		   cimage.bits());
      glEnable(GL_TEXTURE_RECTANGLE_ARB);
      glColor3f(1,1,1);
      glBegin(GL_QUADS);
      glTexCoord2f(0, 0);      glVertex2f(x-5, y+2);
      glTexCoord2f(wd, 0);     glVertex2f(x+wd+5, y+2);
      glTexCoord2f(wd, ht);    glVertex2f(x+wd+5, y-ht-2);
      glTexCoord2f(0, ht);     glVertex2f(x-5, y-ht-2);
      glEnd();
      glDisable(GL_TEXTURE_RECTANGLE_ARB);

      glPopMatrix();


//      const uchar *bits = cimage.bits();
//
//      glRasterPos2i(px, py);
//      glDrawPixels(cimage.width(), cimage.height(),
//		   GL_RGBA,
//		   GL_UNSIGNED_BYTE,
//		   bits);
    }

  viewer->stopScreenCoordinatesSystem();
  
  glEnable(GL_DEPTH_TEST);
}

bool
Captions::keyPressEvent(QKeyEvent *event)
{
  for(int i=0; i<m_captions.count(); i++)
    {
      if (m_captions[i]->grabsMouse())
	{
	  if (event->key() == Qt::Key_Delete ||
	      event->key() == Qt::Key_Backspace ||
	      event->key() == Qt::Key_Backtab)
	    {
	      m_captions.removeAt(i);
	      return true;
	    }
	  else if (event->key() == Qt::Key_Space)
	    {
	      CaptionObject* cmg = m_captions[i];
	      CaptionDialog cd(0,
			       cmg->text(),
			       cmg->font(),
			       cmg->color(),
			       cmg->haloColor(),
			       cmg->angle());
	      cd.hideAngle(false);
	      cd.move(QCursor::pos());
	      if (cd.exec() == QDialog::Accepted)
		{
		  cmg->setText(cd.text());
		  cmg->setFont(cd.font());
		  cmg->setColor(cd.color());
		  cmg->setHaloColor(cd.haloColor());
		  cmg->setAngle(cd.angle());
		}
	    }
	  else if (event->key() == Qt::Key_C)
	    {
	      QPointF pos = m_captions[i]->position();
	      bool ok;
	      QString text;
	      text = QString("%1 %2").arg(pos.x()).arg(pos.y());

	      text = QInputDialog::getText(0,
					   "Coordinates",
					   "Coordinates",
					   QLineEdit::Normal,
					   text,
					   &ok);
	      if (ok && !text.isEmpty())
		{
		  float x,y;
		  x=y=0;
		  QStringList list = text.split(" ", QString::SkipEmptyParts);
		  if (list.count() > 0) x = qBound(0.0f, list[0].toFloat(), 1.0f);
		  if (list.count() > 1) y = qBound(0.0f, list[1].toFloat(), 1.0f);
		  m_captions[i]->setPosition(QPointF(x,y));
		}
	    }
	}
    }
  
  return true;
}
