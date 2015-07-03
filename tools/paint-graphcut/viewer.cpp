#include "viewer.h"
#include "global.h"
#include <QInputDialog>
#include <QProgressDialog>

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

  m_fibers = 0;

  m_depth = 0;
  m_width = 0;
  m_height = 0;

  m_currSlice = 0;
  m_currSliceType = 0;

  m_maskPtr = 0;
  m_volPtr = 0;
  m_pointSkip = 5;
  m_pointSize = 5;

  m_voxChoice = 0;
  m_voxels.clear();

  m_minDSlice = 0;
  m_maxDSlice = 0;
  m_minWSlice = 0;
  m_maxWSlice = 0;
  m_minHSlice = 0;
  m_maxHSlice = 0;  

  m_paintedTags.clear();
  m_paintedTags << -1;

  m_curveTags.clear();
  m_curveTags << -1;

  m_fiberTags.clear();
  m_fiberTags << -1;

  m_showBox = true;
}

void
Viewer::updateCurrSlice(int cst, int cs)
{
  m_currSliceType = cst;
  m_currSlice = cs;
  update();
}

void
Viewer::setPaintedTags(QList<int> t)
{
  m_paintedTags = t;
  update();
}

void
Viewer::setCurveTags(QList<int> t)
{
  m_curveTags = t;
  update();
}

void
Viewer::setFiberTags(QList<int> t)
{
  m_fiberTags = t;
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

  setSceneCenter(Vec((m_maxHSlice+m_minHSlice),
		     (m_maxWSlice+m_minWSlice),
		     (m_maxDSlice+m_minDSlice))/2);		 
}

void
Viewer::setShowBox(bool b)
{
  m_showBox = b;
  update();  
}

void
Viewer::keyPressEvent(QKeyEvent *event)
{
  if (event->key() == Qt::Key_Escape)
    return;

  if (event->key() == Qt::Key_A)
    {  
      toggleAxisIsDrawn();
      update();
      return;
    }

  if (event->key() == Qt::Key_P)
    {
      camera()->setType(Camera::PERSPECTIVE);
      update();
    }

  if (event->key() == Qt::Key_O)
    {
      camera()->setType(Camera::ORTHOGRAPHIC);
      update();
    }

  if (event->key() != Qt::Key_H)
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

  m_maxDSlice = d-1;
  m_maxWSlice = w-1;
  m_maxHSlice = h-1;

  setSceneBoundingBox(Vec(0,0,0), Vec(m_height, m_width, m_depth));
  setSceneCenter(Vec((m_maxHSlice+m_minHSlice),
		     (m_maxWSlice+m_minWSlice),
		     (m_maxDSlice+m_minDSlice))/2);		 
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
Viewer::drawCurrentSlice(Vec subvolmin,
			 Vec subvolmax)
{
  uchar *lut = Global::lut();

  if (m_currSliceType == 0)
    {
      glBegin(GL_QUADS);  
      glVertex3f(subvolmin.x, subvolmin.y, m_currSlice);
      glVertex3f(subvolmax.x, subvolmin.y, m_currSlice);
      glVertex3f(subvolmax.x, subvolmax.y, m_currSlice);
      glVertex3f(subvolmin.x, subvolmax.y, m_currSlice);
      glEnd();  


      if (m_volPtr && m_pointSkip > 0)
	{
	  int d=m_currSlice;
	  for(int w=m_minWSlice; w<m_maxWSlice; w+=m_pointSkip)
	    for(int h=m_minHSlice; h<m_maxHSlice; h+=m_pointSkip)
	      {
		uchar v = m_volPtr[d*m_width*m_height + w*m_height + h];
		if (lut[4*v+3] > 10)
		  {
		    glBegin(GL_POINTS);
		    float r = lut[4*v+2]*1.0/255.0;
		    float g = lut[4*v+1]*1.0/255.0;
		    float b = lut[4*v+0]*1.0/255.0;
		    glColor3f(r,g,b);
		    glVertex3f(h, w, d);
		    glEnd();
		  }
	      }
	}
      
    }

  if (m_currSliceType == 1)
    {
      glBegin(GL_QUADS);  
      glVertex3f(subvolmin.x, m_currSlice, subvolmin.z);
      glVertex3f(subvolmax.x, m_currSlice, subvolmin.z);
      glVertex3f(subvolmax.x, m_currSlice, subvolmax.z);
      glVertex3f(subvolmin.x, m_currSlice, subvolmax.z);  
      glEnd();    

      if (m_volPtr && m_pointSkip > 0)
	{
	  int w=m_currSlice;
	  for(int d=m_minDSlice; d<m_maxDSlice; d+=m_pointSkip)
	    for(int h=m_minHSlice; h<m_maxHSlice; h+=m_pointSkip)
	      {
		uchar v = m_volPtr[d*m_width*m_height + w*m_height + h];
		if (lut[4*v+3] > 10)
		  {
		    glBegin(GL_POINTS);
		    float r = lut[4*v+2]*1.0/255.0;
		    float g = lut[4*v+1]*1.0/255.0;
		    float b = lut[4*v+0]*1.0/255.0;
		    glColor3f(r,g,b);
		    glVertex3f(h, w, d);
		    glEnd();
		  }
	      }
	}
    }

  if (m_currSliceType == 2)
    {
      glBegin(GL_QUADS);  
      glVertex3f(m_currSlice, subvolmin.y, subvolmin.z);
      glVertex3f(m_currSlice, subvolmax.y, subvolmin.z);
      glVertex3f(m_currSlice, subvolmax.y, subvolmax.z);
      glVertex3f(m_currSlice, subvolmin.y, subvolmax.z);  
      glEnd();    

      if (m_volPtr && m_pointSkip > 0)
	{
	  int h=m_currSlice;
	  for(int d=m_minDSlice; d<m_maxDSlice; d+=m_pointSkip)
	    for(int w=m_minWSlice; w<m_maxWSlice; w+=m_pointSkip)
	      {
		uchar v = m_volPtr[d*m_width*m_height + w*m_height + h];
		if (lut[4*v+3] > 10)
		  {
		    glBegin(GL_POINTS);
		    float r = lut[4*v+2]*1.0/255.0;
		    float g = lut[4*v+1]*1.0/255.0;
		    float b = lut[4*v+0]*1.0/255.0;
		    glColor3f(r,g,b);
		    glVertex3f(h, w, d);
		    glEnd();
		  }
	      }
	}
    }
}

void
Viewer::drawBox()
{

  //setAxisIsDrawn();
  
  glColor3d(0.5,0.5,0.5);

  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  
  glLineWidth(1);
  drawEnclosingCube(Vec(0,0,0),
		    Vec(m_height, m_width, m_depth));
  
  glLineWidth(2);
  glColor3d(0.8,0.8,0.8);
  drawEnclosingCube(Vec(m_minHSlice, m_minWSlice, m_minDSlice),
		    Vec(m_maxHSlice, m_maxWSlice, m_maxDSlice));
  
  
  glLineWidth(3);
  glColor3d(1.0,0.85,0.7);
  drawCurrentSlice(Vec(0,0,0),
		   Vec(m_height, m_width, m_depth));
  
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  glLineWidth(1);
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
Viewer::setFibers(QList<Fiber*> *fb)
{
  m_fibers = fb;
}


void
Viewer::draw()
{
  glDisable(GL_LIGHTING);

  if (m_showBox)
    drawBox();

  drawMMDCurve();
  drawMMWCurve();
  drawMMHCurve();

  drawLMDCurve();
  drawLMWCurve();
  drawLMHCurve();

  glEnable(GL_LIGHTING);
  drawFibers();
  glDisable(GL_LIGHTING);

  if (m_pointSkip > 0 && m_maskPtr)
    drawVolMask();
}

void
Viewer::drawFibers()
{
  if (!m_fibers) return;

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  bool noneselected = true;
  for(int i=0; i<m_fibers->count(); i++)
    {
      Fiber *fb = m_fibers->at(i);
      if (fb->selected)
	{
	  noneselected = false;
	  break;
	}
    }
 
  for(int i=0; i<m_fibers->count(); i++)
    {
      Fiber *fb = m_fibers->at(i);
      int tag = fb->tag;
      if (m_fiberTags.count() == 0 ||
	  m_fiberTags[0] == -1 ||
	  m_fiberTags.contains(tag))
	{
	  float r = Global::tagColors()[4*tag+0]*1.0/255.0;
	  float g = Global::tagColors()[4*tag+1]*1.0/255.0;
	  float b = Global::tagColors()[4*tag+2]*1.0/255.0;
	  glColor3f(r,g,b);

	  if (noneselected ||
	      (!noneselected && fb->selected))
	    {
	      glEnable(GL_LIGHTING);
	      QList<Vec> tube = fb->tube();
	      glBegin(GL_TRIANGLE_STRIP);
	      for(int t=0; t<tube.count()/2; t++)
		{
		  glNormal3fv(tube[2*t+0]);	      
		  glVertex3fv(tube[2*t+1]);
		}
	      glEnd();
	    }
	  else
	    {
	      glDisable(GL_LIGHTING);
	      glLineWidth(1);
	      glBegin(GL_LINE_STRIP);
	      for(int j=0; j<fb->smoothSeeds.count(); j++)
		glVertex3fv(fb->smoothSeeds[j]);
	      glEnd();
	    }
	}
    }

  glLineWidth(1);
}

void
Viewer::updateVoxels()
{
  m_voxels.clear();
  
  if (!m_volPtr || !m_maskPtr || m_pointSkip == 0)
    {
      if (!m_volPtr || !m_maskPtr)
	QMessageBox::information(0, "", "Data not loaded into memory, therefore cannot show the voxels");
      else if (m_pointSkip == 0)
	QMessageBox::information(0, "", "Step size is set to 0, therefore will not show the voxels");
      return;
    }

  if (m_voxChoice == 0)
    {
      updateVoxelsWithTF();
      return;
    }
  
  QProgressDialog progress("Updating voxel structure",
			   QString(),
			   0, 100,
			   0);
  progress.setMinimumDuration(0);

  bool takeall = (m_paintedTags.count() == 0 ||
		  m_paintedTags[0] == -1);

  //----------------------------------
  // get the edges first  
  int d,w,h;
  d=m_minDSlice;
  for(int w=m_minWSlice; w<m_maxWSlice; w+=m_pointSkip)
    for(int h=m_minHSlice; h<m_maxHSlice; h+=m_pointSkip)
      {
	uchar tag = m_maskPtr[d*m_width*m_height + w*m_height + h];
	if (m_paintedTags.contains(tag) ||
	    (tag > 0 && takeall))
	  {
	    uchar vol = m_volPtr[d*m_width*m_height + w*m_height + h];
	    m_voxels << d << w << h << tag << vol;
	  }
      }
  w=m_minWSlice;
  for(int d=m_minDSlice; d<m_maxDSlice; d+=m_pointSkip)
    for(int h=m_minHSlice; h<m_maxHSlice; h+=m_pointSkip)
      {
	uchar tag = m_maskPtr[d*m_width*m_height + w*m_height + h];
	if (m_paintedTags.contains(tag) ||
	    (tag > 0 && takeall))
	  {
	    uchar vol = m_volPtr[d*m_width*m_height + w*m_height + h];
	    m_voxels << d << w << h << tag << vol;
	  }
      }
  h=m_minHSlice;
  for(int d=m_minDSlice; d<m_maxDSlice; d+=m_pointSkip)
    for(int w=m_minWSlice; w<m_maxWSlice; w+=m_pointSkip)
      {
	uchar tag = m_maskPtr[d*m_width*m_height + w*m_height + h];
	if (m_paintedTags.contains(tag) ||
	    (tag > 0 && takeall))
	  {
	    uchar vol = m_volPtr[d*m_width*m_height + w*m_height + h];
	    m_voxels << d << w << h << tag << vol;
	  }
      }
  d=m_maxDSlice;
  for(int w=m_minWSlice; w<m_maxWSlice; w+=m_pointSkip)
    for(int h=m_minHSlice; h<m_maxHSlice; h+=m_pointSkip)
      {
	uchar tag = m_maskPtr[d*m_width*m_height + w*m_height + h];
	if (m_paintedTags.contains(tag) ||
	    (tag > 0 && takeall))
	  {
	    uchar vol = m_volPtr[d*m_width*m_height + w*m_height + h];
	    m_voxels << d << w << h << tag << vol;
	  }
      }
  w=m_maxWSlice;
  for(int d=m_minDSlice; d<m_maxDSlice; d+=m_pointSkip)
    for(int h=m_minHSlice; h<m_maxHSlice; h+=m_pointSkip)
      {
	uchar tag = m_maskPtr[d*m_width*m_height + w*m_height + h];
	if (m_paintedTags.contains(tag) ||
	    (tag > 0 && takeall))
	  {
	    uchar vol = m_volPtr[d*m_width*m_height + w*m_height + h];
	    m_voxels << d << w << h << tag << vol;
	  }
      }
  h=m_maxHSlice;
  for(int d=m_minDSlice; d<m_maxDSlice; d+=m_pointSkip)
    for(int w=m_minWSlice; w<m_maxWSlice; w+=m_pointSkip)
      {
	uchar tag = m_maskPtr[d*m_width*m_height + w*m_height + h];
	if (m_paintedTags.contains(tag) ||
	    (tag > 0 && takeall))
	  {
	    uchar vol = m_volPtr[d*m_width*m_height + w*m_height + h];
	    m_voxels << d << w << h << tag << vol;
	  }
      }
  //----------------------------------


  for(d=m_minDSlice+1; d<m_maxDSlice-1; d+=m_pointSkip)
    {
      progress.setValue(100*(float)(d-m_minDSlice)/(m_maxDSlice-m_minDSlice));
      qApp->processEvents();
      for(w=m_minWSlice+1; w<m_maxWSlice-1; w+=m_pointSkip)
	{
	  for(h=m_minHSlice+1; h<m_maxHSlice-1; h+=m_pointSkip)
	    {
	      uchar tag = m_maskPtr[d*m_width*m_height + w*m_height + h];
	      if (m_paintedTags.contains(tag) ||
		  (tag > 0 && takeall))
		{
		  bool ok = false;
		  for(int dd=-m_pointSkip; dd<=m_pointSkip; dd++)
		    for(int ww=-m_pointSkip; ww<=m_pointSkip; ww++)
		      for(int hh=-m_pointSkip; hh<=m_pointSkip; hh++)
			{
			  int d1 = qBound(m_minDSlice, d+dd, m_maxDSlice);
			  int w1 = qBound(m_minWSlice, w+ww, m_maxWSlice);
			  int h1 = qBound(m_minHSlice, h+hh, m_maxHSlice);
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
  progress.setValue(100);
}

void
Viewer::drawVolMask()
{
  if (!m_volPtr || !m_maskPtr || m_pointSkip == 0)
    return;

  if (m_voxChoice == 0)
    {
      drawVol();
      return;
    }

  glEnable(GL_ALPHA_TEST);
  glAlphaFunc(GL_GREATER, 0.5);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_POINT_SMOOTH);

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
      r = r*0.3 + 0.7*v;
      g = g*0.3 + 0.7*v;
      b = b*0.3 + 0.7*v;
      glColor3f(r,g,b);
      glVertex3f(h, w, d);
      glEnd();
    }

  glDisable(GL_POINT_SMOOTH);
  glBlendFunc(GL_NONE, GL_NONE);
  glDisable(GL_BLEND);
}

void
Viewer::updateVoxelsWithTF()
{
  m_voxels.clear();
  
  if (!m_volPtr || !m_maskPtr || m_pointSkip == 0)
    return;

  uchar *lut = Global::lut();

  QProgressDialog progress("Updating voxel structure",
			   QString(),
			   0, 100,
			   0);
  progress.setMinimumDuration(0);

  //----------------------------------
  // get the edges first
  int d,w,h;
  d=m_minDSlice;
  for(int w=m_minWSlice; w<m_maxWSlice; w+=m_pointSkip)
    for(int h=m_minHSlice; h<m_maxHSlice; h+=m_pointSkip)
      {
	uchar vol = m_volPtr[d*m_width*m_height + w*m_height + h];
	if (lut[4*vol+3] > 10)
	  m_voxels << d << w << h << vol;
      }
  w=m_minWSlice;
  for(int d=m_minDSlice; d<m_maxDSlice; d+=m_pointSkip)
    for(int h=m_minHSlice; h<m_maxHSlice; h+=m_pointSkip)
      {
	uchar vol = m_volPtr[d*m_width*m_height + w*m_height + h];
	if (lut[4*vol+3] > 10)
	  m_voxels << d << w << h << vol;
      }
  h=m_minHSlice;
  for(int d=m_minDSlice; d<m_maxDSlice; d+=m_pointSkip)
    for(int w=m_minWSlice; w<m_maxWSlice; w+=m_pointSkip)
      {
	uchar vol = m_volPtr[d*m_width*m_height + w*m_height + h];
	if (lut[4*vol+3] > 10)
	  m_voxels << d << w << h << vol;
      }
  d=m_maxDSlice;
  for(int w=m_minWSlice; w<m_maxWSlice; w+=m_pointSkip)
    for(int h=m_minHSlice; h<m_maxHSlice; h+=m_pointSkip)
      {
	uchar vol = m_volPtr[d*m_width*m_height + w*m_height + h];
	if (lut[4*vol+3] > 10)
	  m_voxels << d << w << h << vol;
      }
  w=m_maxWSlice;
  for(int d=m_minDSlice; d<m_maxDSlice; d+=m_pointSkip)
    for(int h=m_minHSlice; h<m_maxHSlice; h+=m_pointSkip)
      {
	uchar vol = m_volPtr[d*m_width*m_height + w*m_height + h];
	if (lut[4*vol+3] > 10)
	  m_voxels << d << w << h << vol;
      }
  h=m_maxHSlice;
  for(int d=m_minDSlice; d<m_maxDSlice; d+=m_pointSkip)
    for(int w=m_minWSlice; w<m_maxWSlice; w+=m_pointSkip)
      {
	uchar vol = m_volPtr[d*m_width*m_height + w*m_height + h];
	if (lut[4*vol+3] > 10)
	  m_voxels << d << w << h << vol;
      }
  //----------------------------------


  //----------------------------------
  // now for the interior  
  for(d=m_minDSlice+1; d<m_maxDSlice-1; d+=m_pointSkip)
    {
      progress.setValue(100*(float)(d-m_minDSlice)/(m_maxDSlice-m_minDSlice));
      qApp->processEvents();
      for(w=m_minWSlice+1; w<m_maxWSlice-1; w+=m_pointSkip)
	{
	  for(h=m_minHSlice+1; h<m_maxHSlice-1; h+=m_pointSkip)
	    {
	      uchar vol = m_volPtr[d*m_width*m_height + w*m_height + h];
	      if (lut[4*vol+3] > 10)
		{
		  bool ok = false;
		  for(int dd=-m_pointSkip; dd<=m_pointSkip; dd++)
		    for(int ww=-m_pointSkip; ww<=m_pointSkip; ww++)
		      for(int hh=-m_pointSkip; hh<=m_pointSkip; hh++)
			{
			  int d1 = qBound(m_minDSlice, d+dd, m_maxDSlice);
			  int w1 = qBound(m_minWSlice, w+ww, m_maxWSlice);
			  int h1 = qBound(m_minHSlice, h+hh, m_maxHSlice);
			  
			  uchar v = m_volPtr[d1*m_width*m_height + w1*m_height + h1];
			  if (lut[4*v+3] < 10)
			    {
			      ok = true;
			      break;
			    }
			}

		  if (ok)
		    m_voxels << d << w << h << vol;
		}
	    }
	}
    }
  
  progress.setValue(100);
}

void
Viewer::drawVol()
{
  if (!m_volPtr || !m_maskPtr || m_pointSkip == 0)
    return;

  uchar *lut = Global::lut();

  glEnable(GL_ALPHA_TEST);
  glAlphaFunc(GL_GREATER, 0.5);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_POINT_SMOOTH);

  glPointSize(m_pointSize);
  int nv = m_voxels.count()/4;
  for(int i=0; i<nv; i++)
    {
      int d = m_voxels[4*i+0];
      int w = m_voxels[4*i+1];
      int h = m_voxels[4*i+2];
      int v = m_voxels[4*i+3];

      glBegin(GL_POINTS);
      float r = lut[4*v+2]*1.0/255.0;
      float g = lut[4*v+1]*1.0/255.0;
      float b = lut[4*v+0]*1.0/255.0;
      glColor3f(r,g,b);
      glVertex3f(h, w, d);
      glEnd();
    }

  glDisable(GL_POINT_SMOOTH);
  glBlendFunc(GL_NONE, GL_NONE);
  glDisable(GL_BLEND);
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
	  if (m_curveTags.count() == 0 ||
	      m_curveTags[0] == -1 ||
	      m_curveTags.contains(tag))
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
  glLineWidth(1);
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
	  if (m_curveTags.count() == 0 ||
	      m_curveTags[0] == -1 ||
	      m_curveTags.contains(tag))
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
  glLineWidth(1);
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
	  if (m_curveTags.count() == 0 ||
	      m_curveTags[0] == -1 ||
	      m_curveTags.contains(tag))
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
  glLineWidth(1);
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
	  if (m_curveTags.count() == 0 ||
	      m_curveTags[0] == -1 ||
	      m_curveTags.contains(tag))
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
  glLineWidth(1);
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
	  if (m_curveTags.count() == 0 ||
	      m_curveTags[0] == -1 ||
	      m_curveTags.contains(tag))
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
  glLineWidth(1);
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
	  if (m_curveTags.count() == 0 ||
	      m_curveTags[0] == -1 ||
	      m_curveTags.contains(tag))
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
  glLineWidth(1);
}
