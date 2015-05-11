#include "viewer.h"
#include "global.h"

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
	  glBegin(GL_LINE_STRIP);
	  for(int k=0; k<curves[j]->pts.count(); k++)
	    glVertex3f(curves[j]->pts[k].x(),
		       curves[j]->pts[k].y(),
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
	  glBegin(GL_LINE_STRIP);
	  for(int k=0; k<curves[j]->pts.count(); k++)
	    glVertex3f(curves[j]->pts[k].x(),
		       cgkeys[i],
		       curves[j]->pts[k].y());
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
	  glBegin(GL_LINE_STRIP);
	  for(int k=0; k<curves[j]->pts.count(); k++)
	    glVertex3f(cgkeys[i],
		       curves[j]->pts[k].x(),
		       curves[j]->pts[k].y());
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
      int key = mcg.keys()[0];
      Curve c = mcg.value(key);
      int tag = c.tag;
      float r = Global::tagColors()[4*tag+0]*1.0/255.0;
      float g = Global::tagColors()[4*tag+1]*1.0/255.0;
      float b = Global::tagColors()[4*tag+2]*1.0/255.0;
      glColor3f(r,g,b);
      glBegin(GL_LINE_STRIP);
      for(int k=0; k<c.pts.count(); k++)
	glVertex3f(c.pts[k].x(),
		   c.pts[k].y(),
		   key);
      glEnd();
    }
}

void
Viewer::drawLMWCurve()
{
}

void
Viewer::drawLMHCurve()
{
}
