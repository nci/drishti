#include <GL/glew.h>

#include "global.h"
#include "staticfunctions.h"
#include "imagecaptions.h"

#include <QGLViewer/qglviewer.h>
using namespace qglviewer;

#include <QFileDialog>

#ifdef Q_OS_OSX
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif


ImageCaptions::ImageCaptions()
{
  glGenTextures(1, &m_imageTex);
  m_imageCaptions.clear();
}

ImageCaptions::~ImageCaptions()
{
  clear();

  glDeleteTextures(1, &m_imageTex);
  m_imageTex = 0;
}

void
ImageCaptions::addInMouseGrabberPool()
{
  for(int i=0; i<m_imageCaptions.count(); i++)
    m_imageCaptions[i]->addInMouseGrabberPool();
}

void
ImageCaptions::clear()
{
  for(int i=0; i<m_imageCaptions.count(); i++)
    m_imageCaptions[i]->removeFromMouseGrabberPool();

  for(int i=0; i<m_imageCaptions.count(); i++)
    delete m_imageCaptions[i];

  m_imageCaptions.clear();
}

QList<ImageCaptionObject>
ImageCaptions::imageCaptions()
{
  QList<ImageCaptionObject> caplist;
  for(int i=0; i<m_imageCaptions.count(); i++)
    caplist.append(m_imageCaptions[i]->imageCaption());

  return caplist;
}


bool
ImageCaptions::grabsMouse()
{
  for(int i=0; i<m_imageCaptions.count(); i++)
    {
      if (m_imageCaptions[i]->grabsMouse())
	return true;
    }
  return false;
}

void
ImageCaptions::add(Vec pt)
{
  QString imgFile = QFileDialog::getOpenFileName(0,
                                 QString("Load image to map on the clip plane"),
                                 Global::previousDirectory(),
                                 "Image Files (*.png *.tif *.bmp *.jpg *.gif)");
	      
  if (imgFile.isEmpty()) return;
  QFileInfo f(imgFile);
  if (f.exists() == false) return;
  
  ImageCaptionGrabber *cmg = new ImageCaptionGrabber();
  cmg->set(pt, imgFile);
  m_imageCaptions.append(cmg);
}

void
ImageCaptions::add(ImageCaptionObject cap)
{
  ImageCaptionGrabber *cmg = new ImageCaptionGrabber();
  cmg->setImageCaption(cap);
  m_imageCaptions.append(cmg);
}

void
ImageCaptions::setImageCaptions(QList<ImageCaptionObject> caps)
{
  clear();

  for(int i=0; i<caps.count(); i++)
    add(caps[i]);
}

void
ImageCaptions::draw(QGLViewer *viewer, bool backToFront)
{
  int pointSize = 30;

  glEnable(GL_POINT_SPRITE);
  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, Global::infoSpriteTexture());
  glTexEnvf( GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE );
  glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
  glEnable(GL_POINT_SMOOTH);


  Vec voxelScaling = Global::voxelScaling();

  //--------------------
  // draw grabbed point
  glColor3f(0, 1, 0.3);
  glPointSize(pointSize+10);
  glBegin(GL_POINTS);
  for(int i=0; i<m_imageCaptions.count(); i++)
    {
      if (m_imageCaptions[i]->grabsMouse())
	{
	  Vec pt = m_imageCaptions[i]->position();
	  pt = VECPRODUCT(pt, voxelScaling);
	  glVertex3fv(pt);
	}
    }
  glEnd();
  //--------------------

  //--------------------
  // draw active points
  glColor3f(1, 1, 1);
  glPointSize(pointSize+20);
  glBegin(GL_POINTS);
  for(int i=0; i<m_imageCaptions.count(); i++)
    {
      if (m_imageCaptions[i]->active())
	{
	  Vec pt = m_imageCaptions[i]->position();
	  pt = VECPRODUCT(pt, voxelScaling);
	  
	  glVertex3fv(pt);
	}
    }
  glEnd();
  //--------------------

  //--------------------
  // draw rest of the points
  glColor3f(1,1,1);
  glPointSize(pointSize);
  glBegin(GL_POINTS);
  for(int i=0; i<m_imageCaptions.count(); i++)
    {
      if (! m_imageCaptions[i]->grabsMouse() &&
	  ! m_imageCaptions[i]->active())
	{
	  Vec pt = m_imageCaptions[i]->position();
	  pt = VECPRODUCT(pt, voxelScaling);

	  glVertex3fv(pt);
	}
    }
  glEnd();

  glPointSize(1);

  glDisable(GL_POINT_SPRITE);
  glActiveTexture(GL_TEXTURE0);
  glDisable(GL_TEXTURE_2D);
  
  glDisable(GL_POINT_SMOOTH);
}

void
ImageCaptions::postdraw(QGLViewer *viewer)
{
  Vec voxelScaling = Global::voxelScaling();

  glDisable(GL_DEPTH_TEST);

  int screenWidth = viewer->size().width();
  int screenHeight = viewer->size().height();

  // splat the caption images
  viewer->startScreenCoordinatesSystem();

  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // blend on top

  glLineWidth(2);
  for(int i=0; i<m_imageCaptions.count(); i++)
    {
      ImageCaptionGrabber *cmg = m_imageCaptions[i];
      if (m_imageCaptions[i]->active())
	{
	  Vec pt = cmg->position();

	  Vec spt = VECPRODUCT(pt, voxelScaling);
	  Vec scr = viewer->camera()->projectedCoordinatesOf(spt);
	  int x = scr.x;
	  int y = scr.y;

	  //---------------------
	  //x *= viewer->size().width()/viewer->camera()->screenWidth();
	  //y *= viewer->size().height()/viewer->camera()->screenHeight();
	  //---------------------
	  
	  int wd = cmg->width();
	  int ht = cmg->height();
	  
	  QImage cimage = cmg->image();

	  int xpos = x+20;
	  if (xpos + cimage.width() > screenWidth)
	    {
	      xpos -= cimage.width();
	      xpos -= 40;
	    }
	  int ypos = y+cimage.height()/2;
	  ypos = qMin(ypos, screenHeight);
	  if (ypos - cimage.height() < 0)
	    ypos = cimage.height();
//	  const uchar *bits = cimage.bits();	  
//	  glRasterPos2i(xpos, ypos);
//	  glDrawPixels(cimage.width(), cimage.height(),
//		       GL_BGRA,
//		       GL_UNSIGNED_BYTE,
//		       bits);
	  
	  int px = xpos;
	  int py = ypos;
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
		       GL_BGRA,
		       GL_UNSIGNED_BYTE,
		       cimage.bits());
	  glEnable(GL_TEXTURE_RECTANGLE_ARB);
	  glColor3f(1,1,1);
	  glBegin(GL_QUADS);
	  glTexCoord2f(0, 0);      glVertex2f(xpos, ypos);
	  glTexCoord2f(wd, 0);     glVertex2f(xpos+wd, ypos);
	  glTexCoord2f(wd, ht);    glVertex2f(xpos+wd, ypos-ht);
	  glTexCoord2f(0, ht);     glVertex2f(xpos, ypos-ht);
	  glEnd();
	  glDisable(GL_TEXTURE_RECTANGLE_ARB);
	}
    }
  viewer->stopScreenCoordinatesSystem();
  
  glEnable(GL_DEPTH_TEST);
}

bool
ImageCaptions::keyPressEvent(QKeyEvent *event)
{
  for(int i=0; i<m_imageCaptions.count(); i++)
    {
      if (m_imageCaptions[i]->grabsMouse())
	{
	  if (event->key() == Qt::Key_Delete ||
	      event->key() == Qt::Key_Backspace ||
	      event->key() == Qt::Key_Backtab)
	    {
	      m_imageCaptions.removeAt(i);
	      return true;
	    }
	  if (event->key() == Qt::Key_Space)
	    {
	      ImageCaptionObject* cmg = m_imageCaptions[i];
	      QString imgFile = QFileDialog::getOpenFileName(0,
                                 QString("Load image to map on the clip plane"),
                                 Global::previousDirectory(),
                                 "Image Files (*.png *.tif *.bmp *.jpg *.gif)");
	      
	      if (imgFile.isEmpty())
		return false;
	      
	      QFileInfo f(imgFile);
	      if (f.exists() == false)
		return false;

	      cmg->setImageFileName(imgFile);
	    }
	}
    }
  
  return true;
}
