#include "viewer.h"
#include "global.h"
#include <QInputDialog>

void
Viewer::init()
{
  m_Dcg = 0;
  m_Wcg = 0;
  m_Hcg = 0;
  m_Dmcg = 0;
  m_Wmcg = 0;
  m_Hmcg = 0;

  m_depth = 0;
  m_width = 0;
  m_height = 0;

  m_maskPtr = 0;
  m_volPtr = 0;
  m_pointSkip = 10;
  m_pointSize = 2;
}

void Viewer::setMaskDataPtr(uchar *ptr) { m_maskPtr = ptr; }
void Viewer::setVolDataPtr(uchar *ptr) { m_volPtr = ptr; }

void
Viewer::keyPressEvent(QKeyEvent *event)
{
  if (event->key() == Qt::Key_Escape)
    return;

  if (event->key() == Qt::Key_Space)
    {
      m_pointSkip = QInputDialog::getInt(this,
					 "Point Interval",
					 "Interval for display of points for painted mask",
					 m_pointSkip, 0, 100);
      return;
    }

  if (event->key() == Qt::Key_P)
    {
      m_pointSize = QInputDialog::getInt(this,
					 "Point Size",
					 "Point size for painted mask",
					 m_pointSize, 1, 20);
      return;
    }

  QGLViewer::keyPressEvent(event);
}

void
Viewer::setGridSize(int d, int w, int h)
{
  m_depth = d;
  m_width = w;
  m_height = h;

  setSceneBoundingBox(Vec(0,0,0), Vec(m_height, m_width, m_depth));
  showEntireScene();
}

void
Viewer::drawBox()
{
  setAxisIsDrawn();
  
  glColor3d(0.5,0.5,0.5);
  
  glBegin(GL_LINES);
   glVertex3f(0, 0, 0);
   glVertex3f(0, 0, m_depth);

   glVertex3f(0, 0, m_depth);
   glVertex3f(0, m_width, m_depth);

   glVertex3f(0, m_width, m_depth);
   glVertex3f(0, m_width, 0);

   glVertex3f(0, m_width, 0);
   glVertex3f(0, 0, 0);


   glVertex3f(m_height, 0, 0);
   glVertex3f(m_height, 0, m_depth);

   glVertex3f(m_height, 0, m_depth);
   glVertex3f(m_height, m_width, m_depth);

   glVertex3f(m_height, m_width, m_depth);
   glVertex3f(m_height, m_width, 0);

   glVertex3f(m_height, m_width, 0);
   glVertex3f(m_height, 0, 0);

   glVertex3f(0, 0, 0);
   glVertex3f(m_height, 0, 0);

   glVertex3f(0, 0, m_depth);
   glVertex3f(m_height, 0, m_depth);

   glVertex3f(0, m_width, m_depth);
   glVertex3f(m_height, m_width, m_depth);

   glVertex3f(0, m_width, 0);
   glVertex3f(m_height, m_width, 0);

   glEnd();
}

void
Viewer::setMultiMapCurves(int type, QMultiMap<int, Curve*> *cg)
{
  if (type == 0) m_Dcg = cg;
  if (type == 1) m_Wcg = cg;
  if (type == 2) m_Hcg = cg;
}

void
Viewer::setListMapCurves(int type, QList< QMap<int, Curve> > *cg)
{
  if (type == 0) m_Dmcg = cg;
  if (type == 1) m_Wmcg = cg;
  if (type == 2) m_Hmcg = cg;
}

void
Viewer::draw()
{
  glDisable(GL_LIGHTING);

  drawBox();

  drawMMDCurve();
  drawMMWCurve();
  drawMMHCurve();

  drawLMDCurve();
  drawLMWCurve();
  drawLMHCurve();

  if (m_pointSkip > 0 && m_maskPtr)
    drawVolMask();
}

void
Viewer::drawVolMask()
{
  glPointSize(m_pointSize);
  int wst = 0;
  int hst = 0;
  for(int d=0; d<m_depth; d+=m_pointSkip)
    {
      for(int w=0; w<m_width; w+=m_pointSkip)
	{
	  for(int h=0; h<m_height; h+=m_pointSkip)
	    {
	      int tag = m_maskPtr[d*m_width*m_height + w*m_height + h];
	      if (tag > 0)
		{
		  float vol = (float)m_volPtr[d*m_width*m_height + w*m_height + h]/255.0;
		  glBegin(GL_POINTS);
		  float r = Global::tagColors()[4*tag+0]*1.0/255.0;
		  float g = Global::tagColors()[4*tag+1]*1.0/255.0;
		  float b = Global::tagColors()[4*tag+2]*1.0/255.0;
		  r = r*0.5 + 0.5*vol;
		  g = g*0.5 + 0.5*vol;
		  b = b*0.5 + 0.5*vol;
		  glColor3f(r,g,b);
		  glVertex3f(h, w, d);
		  glEnd();
		}
	    } // h
	  if (hst > 0)
	    hst = m_pointSkip/2;
	  else
	    hst = 0;
	} // w
      if (wst > 0)
	wst = m_pointSkip/2;
      else
	wst = 0;
    } // d
}


void
Viewer::drawMMDCurve()
{
  if (!m_Dcg) return;

  QList<int> cgkeys = m_Dcg->uniqueKeys();
  for(int i=0; i<cgkeys.count(); i++)
    {
      QList<Curve*> curves = m_Dcg->values(cgkeys[i]);
      for (int j=0; j<curves.count(); j++)
	{
	  int tag = curves[j]->tag;
	  float r = Global::tagColors()[4*tag+0]*1.0/255.0;
	  float g = Global::tagColors()[4*tag+1]*1.0/255.0;
	  float b = Global::tagColors()[4*tag+2]*1.0/255.0;
	  glColor3f(r,g,b);
	  glLineWidth(curves[j]->thickness);
	  glBegin(GL_LINE_STRIP);
	  for(int k=0; k<curves[j]->pts.count(); k++)
	    glVertex3f(curves[j]->pts[k].x(),
		       curves[j]->pts[k].y(),
		       cgkeys[i]);
	  if (curves[j]->closed)
	    glVertex3f(curves[j]->pts[0].x(),
		       curves[j]->pts[0].y(),
		       cgkeys[i]);
	  glEnd();
	}
    }
}

void
Viewer::drawMMWCurve()
{
  if (!m_Wcg) return;

  QList<int> cgkeys = m_Wcg->uniqueKeys();
  for(int i=0; i<cgkeys.count(); i++)
    {
      QList<Curve*> curves = m_Wcg->values(cgkeys[i]);
      for (int j=0; j<curves.count(); j++)
	{
	  int tag = curves[j]->tag;
	  float r = Global::tagColors()[4*tag+0]*1.0/255.0;
	  float g = Global::tagColors()[4*tag+1]*1.0/255.0;
	  float b = Global::tagColors()[4*tag+2]*1.0/255.0;
	  glColor3f(r,g,b);
	  glLineWidth(curves[j]->thickness);
	  glBegin(GL_LINE_STRIP);
	  for(int k=0; k<curves[j]->pts.count(); k++)
	    glVertex3f(curves[j]->pts[k].x(),
		       cgkeys[i],
		       curves[j]->pts[k].y());
	  if (curves[j]->closed)
	    glVertex3f(curves[j]->pts[0].x(),
		       cgkeys[i],
		       curves[j]->pts[0].y());
	  glEnd();
	}
    }
}

void
Viewer::drawMMHCurve()
{
  if (!m_Hcg) return;

  QList<int> cgkeys = m_Hcg->uniqueKeys();
  for(int i=0; i<cgkeys.count(); i++)
    {
      QList<Curve*> curves = m_Hcg->values(cgkeys[i]);
      for (int j=0; j<curves.count(); j++)
	{
	  int tag = curves[j]->tag;
	  float r = Global::tagColors()[4*tag+0]*1.0/255.0;
	  float g = Global::tagColors()[4*tag+1]*1.0/255.0;
	  float b = Global::tagColors()[4*tag+2]*1.0/255.0;
	  glColor3f(r,g,b);
	  glLineWidth(curves[j]->thickness);
	  glBegin(GL_LINE_STRIP);
	  for(int k=0; k<curves[j]->pts.count(); k++)
	    glVertex3f(cgkeys[i],
		       curves[j]->pts[k].x(),
		       curves[j]->pts[k].y());
	  if (curves[j]->closed)
	    glVertex3f(cgkeys[i],
		       curves[j]->pts[0].x(),
		       curves[j]->pts[0].y());
	  glEnd();
	}
    }
}

void
Viewer::drawLMDCurve()
{
  if (!m_Dmcg) return;

  for(int i=0; i<m_Dmcg->count(); i++)
    {
      QMap<int, Curve> mcg = m_Dmcg->at(i);
      QList<int> cgkeys = mcg.keys();
      for(int j=0; j<cgkeys.count(); j++)
	{
	  Curve c = mcg.value(cgkeys[j]);

	  int tag = c.tag;
	  float r = Global::tagColors()[4*tag+0]*1.0/255.0;
	  float g = Global::tagColors()[4*tag+1]*1.0/255.0;
	  float b = Global::tagColors()[4*tag+2]*1.0/255.0;
	  glColor3f(r,g,b);
	  glLineWidth(c.thickness);
	  glBegin(GL_LINE_STRIP);
	  for(int k=0; k<c.pts.count(); k++)
	    glVertex3f(c.pts[k].x(),
		       c.pts[k].y(),
		       cgkeys[j]);
	  if (c.closed)
	    glVertex3f(c.pts[0].x(),
		       c.pts[0].y(),
		       cgkeys[j]);
	  glEnd();
	}
    }
}

void
Viewer::drawLMWCurve()
{
  if (!m_Wmcg) return;

  for(int i=0; i<m_Wmcg->count(); i++)
    {
      QMap<int, Curve> mcg = m_Wmcg->at(i);
      QList<int> cgkeys = mcg.keys();
      for(int j=0; j<cgkeys.count(); j++)
	{
	  Curve c = mcg.value(cgkeys[j]);

	  int tag = c.tag;
	  float r = Global::tagColors()[4*tag+0]*1.0/255.0;
	  float g = Global::tagColors()[4*tag+1]*1.0/255.0;
	  float b = Global::tagColors()[4*tag+2]*1.0/255.0;
	  glColor3f(r,g,b);
	  glLineWidth(c.thickness);
	  glBegin(GL_LINE_STRIP);
	  for(int k=0; k<c.pts.count(); k++)
	    glVertex3f(c.pts[k].x(),
		       cgkeys[j],
		       c.pts[k].y());
	  if (c.closed)
	    glVertex3f(c.pts[0].x(),
		       cgkeys[j],
		       c.pts[0].y());
	    
	  glEnd();
	}
    }
}

void
Viewer::drawLMHCurve()
{
  if (!m_Hmcg) return;

  for(int i=0; i<m_Hmcg->count(); i++)
    {
      QMap<int, Curve> mcg = m_Hmcg->at(i);
      QList<int> cgkeys = mcg.keys();
      for(int j=0; j<cgkeys.count(); j++)
	{
	  Curve c = mcg.value(cgkeys[j]);

	  int tag = c.tag;
	  float r = Global::tagColors()[4*tag+0]*1.0/255.0;
	  float g = Global::tagColors()[4*tag+1]*1.0/255.0;
	  float b = Global::tagColors()[4*tag+2]*1.0/255.0;
	  glColor3f(r,g,b);
	  glLineWidth(c.thickness);
	  glBegin(GL_LINE_STRIP);
	  for(int k=0; k<c.pts.count(); k++)
	    glVertex3f(cgkeys[j],
		       c.pts[k].x(),
		       c.pts[k].y());
	  if (c.closed)
	    glVertex3f(cgkeys[j],
		       c.pts[0].x(),
		       c.pts[0].y());
	    
	  glEnd();
	}
    }
}
