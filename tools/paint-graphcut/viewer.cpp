#include "viewer.h"
#include "global.h"
#include <QInputDialog>

Viewer::Viewer(QWidget *parent) :
  QGLViewer(parent)
{
  init();
  setMinimumSize(100, 100);
}

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
  m_pointSkip = 5;
  m_pointSize = 5;

  m_voxels.clear();

  m_minDSlice = 0;
  m_maxDSlice = 0;
  m_minWSlice = 0;
  m_maxWSlice = 0;
  m_minHSlice = 0;
  m_maxHSlice = 0;  

  m_showTags.clear();
  m_showTags << -1;
}

void
Viewer::showTags(QList<int> t)
{
  m_showTags = t;
  update();
}


void Viewer::setMaskDataPtr(uchar *ptr) { m_maskPtr = ptr; }
void Viewer::setVolDataPtr(uchar *ptr) { m_volPtr = ptr; }

void
Viewer::updateViewerBox(int minD, int maxD, int minW, int maxW, int minH, int maxH)
{
  m_minDSlice = minD;
  m_maxDSlice = maxD;

  m_minWSlice = minW;
  m_maxWSlice = maxW;

  m_minHSlice = minH;
  m_maxHSlice = maxH;
}

void
Viewer::keyPressEvent(QKeyEvent *event)
{
  if (event->key() == Qt::Key_Escape)
    return;

  if (event->key() == Qt::Key_U)
    {
      updateVoxels();
      return;
    }

  if (event->key() == Qt::Key_Space)
    {
      m_pointSkip = QInputDialog::getInt(this,
					 "Point Interval",
					 "Interval for display of points for painted mask",
					 m_pointSkip, 0, 100);
      updateVoxels();
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

  m_minDSlice = 0;
  m_minWSlice = 0;
  m_minHSlice = 0;

  m_maxDSlice = d;
  m_maxWSlice = w;
  m_maxHSlice = h;

  setSceneBoundingBox(Vec(0,0,0), Vec(m_height, m_width, m_depth));
  showEntireScene();
}

void
Viewer::drawEnclosingCube(Vec subvolmin,
			  Vec subvolmax)
{
  glBegin(GL_QUADS);  
  glVertex3f(subvolmin.x, subvolmin.y, subvolmin.z);
  glVertex3f(subvolmax.x, subvolmin.y, subvolmin.z);
  glVertex3f(subvolmax.x, subvolmax.y, subvolmin.z);
  glVertex3f(subvolmin.x, subvolmax.y, subvolmin.z);
  glEnd();
  
  // FRONT 
  glBegin(GL_QUADS);  
  glVertex3f(subvolmin.x, subvolmin.y, subvolmax.z);
  glVertex3f(subvolmax.x, subvolmin.y, subvolmax.z);
  glVertex3f(subvolmax.x, subvolmax.y, subvolmax.z);
  glVertex3f(subvolmin.x, subvolmax.y, subvolmax.z);
  glEnd();
  
  // TOP
  glBegin(GL_QUADS);  
  glVertex3f(subvolmin.x, subvolmax.y, subvolmin.z);
  glVertex3f(subvolmax.x, subvolmax.y, subvolmin.z);
  glVertex3f(subvolmax.x, subvolmax.y, subvolmax.z);
  glVertex3f(subvolmin.x, subvolmax.y, subvolmax.z);
  glEnd();
  
  // BOTTOM
  glBegin(GL_QUADS);  
  glVertex3f(subvolmin.x, subvolmin.y, subvolmin.z);
  glVertex3f(subvolmax.x, subvolmin.y, subvolmin.z);
  glVertex3f(subvolmax.x, subvolmin.y, subvolmax.z);
  glVertex3f(subvolmin.x, subvolmin.y, subvolmax.z);  
  glEnd();  
}

void
Viewer::drawBox()
{
  setAxisIsDrawn();
  
  glColor3d(0.5,0.5,0.5);

  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  
  drawEnclosingCube(Vec(0,0,0),
		    Vec(m_height, m_width, m_depth));

  glColor3d(0.8,0.8,0.8);
  drawEnclosingCube(Vec(m_minHSlice, m_minWSlice, m_minDSlice),
		    Vec(m_maxHSlice, m_maxWSlice, m_maxDSlice));

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

//  glBegin(GL_LINES);
//   glVertex3f(0, 0, 0);
//   glVertex3f(0, 0, m_depth);
//
//   glVertex3f(0, 0, m_depth);
//   glVertex3f(0, m_width, m_depth);
//
//   glVertex3f(0, m_width, m_depth);
//   glVertex3f(0, m_width, 0);
//
//   glVertex3f(0, m_width, 0);
//   glVertex3f(0, 0, 0);
//
//
//   glVertex3f(m_height, 0, 0);
//   glVertex3f(m_height, 0, m_depth);
//
//   glVertex3f(m_height, 0, m_depth);
//   glVertex3f(m_height, m_width, m_depth);
//
//   glVertex3f(m_height, m_width, m_depth);
//   glVertex3f(m_height, m_width, 0);
//
//   glVertex3f(m_height, m_width, 0);
//   glVertex3f(m_height, 0, 0);
//
//   glVertex3f(0, 0, 0);
//   glVertex3f(m_height, 0, 0);
//
//   glVertex3f(0, 0, m_depth);
//   glVertex3f(m_height, 0, m_depth);
//
//   glVertex3f(0, m_width, m_depth);
//   glVertex3f(m_height, m_width, m_depth);
//
//   glVertex3f(0, m_width, 0);
//   glVertex3f(m_height, m_width, 0);
//
//   glEnd();
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
Viewer::updateVoxels()
{
  m_voxels.clear();
  
  if (m_pointSkip == 0)
    return;

  for(int d=1; d<m_depth-1; d+=m_pointSkip)
    {
      for(int w=1; w<m_width-1; w+=m_pointSkip)
	{
	  for(int h=1; h<m_height-1; h+=m_pointSkip)
	    {
	      uchar tag = m_maskPtr[d*m_width*m_height + w*m_height + h];
	      if (tag > 0 &&
		  (m_showTags.count() == 0 ||
		   m_showTags[0] == -1 ||
		   m_showTags.contains(tag)))
		{
		  bool ok = false;
		  for(int dd=-m_pointSkip; dd<=m_pointSkip; dd++)
		    for(int ww=-m_pointSkip; ww<=m_pointSkip; ww++)
		      for(int hh=-m_pointSkip; hh<=m_pointSkip; hh++)
			{
			  int d1 = qBound(0, d+dd, m_depth);
			  int w1 = qBound(0, w+ww, m_width);
			  int h1 = qBound(0, h+hh, m_height);
			  if (m_maskPtr[d1*m_width*m_height + w1*m_height + h1] != tag)
			    {
			      ok = true;
			      break;
			    }
			}

		  if (ok)
		    {
		      uchar vol = m_volPtr[d*m_width*m_height + w*m_height + h];
		      m_voxels << d << w << h << tag << vol;
		    }

		}
	    }
	}
    }
}

void
Viewer::drawVolMask()
{
  if (m_pointSkip == 0)
    return;

  glPointSize(m_pointSize);
  int nv = m_voxels.count()/5;
  for(int i=0; i<nv; i++)
    {
      int d = m_voxels[5*i+0];
      int w = m_voxels[5*i+1];
      int h = m_voxels[5*i+2];
      int t = m_voxels[5*i+3];
      float v = (float)m_voxels[5*i+4]/255.0;

      glBegin(GL_POINTS);
      float r = Global::tagColors()[4*t+0]*1.0/255.0;
      float g = Global::tagColors()[4*t+1]*1.0/255.0;
      float b = Global::tagColors()[4*t+2]*1.0/255.0;
      r = r*0.5 + 0.5*v;
      g = g*0.5 + 0.5*v;
      b = b*0.5 + 0.5*v;
      glColor3f(r,g,b);
      glVertex3f(h, w, d);
      glEnd();
    }
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
	  if (m_showTags.count() == 0 ||
	      m_showTags[0] == -1 ||
	      m_showTags.contains(tag))
	    {
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
	  if (m_showTags.count() == 0 ||
	      m_showTags[0] == -1 ||
	      m_showTags.contains(tag))
	    {
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
	  if (m_showTags.count() == 0 ||
	      m_showTags[0] == -1 ||
	      m_showTags.contains(tag))
	    {
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
	  if (m_showTags.count() == 0 ||
	      m_showTags[0] == -1 ||
	      m_showTags.contains(tag))
	    {
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
	  if (m_showTags.count() == 0 ||
	      m_showTags[0] == -1 ||
	      m_showTags.contains(tag))
	    {
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
	  if (m_showTags.count() == 0 ||
	      m_showTags[0] == -1 ||
	      m_showTags.contains(tag))
	    {
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
}
