#define  _USE_MATH_DEFINES
#include <math.h>

#include "global.h"
#include "staticfunctions.h"
#include "boundingbox.h"


BoundingBox::BoundingBox()
{
  boxColor = Vec(0.9f, 0.9f, 0.9f);
  defaultColor = Vec(0.9f, 0.9f, 0.7f);
  selectColor  = Vec(1.0f, 0.3f, 0.1f);

  setBounds(Vec(0,0,0), Vec(1,1,1));

  for(int i=0; i<6; i++)
    {
      m_bounds[i].setThreshold(20); // set grabsMouse radius to 20
      m_bounds[i].setOnlyTranslate(true); // only translations allowed
    }
  

  connect(&m_bounds[0], SIGNAL(manipulated()), this, SLOT(update()));
  connect(&m_bounds[1], SIGNAL(manipulated()), this, SLOT(update()));
  connect(&m_bounds[2], SIGNAL(manipulated()), this, SLOT(update()));
  connect(&m_bounds[3], SIGNAL(manipulated()), this, SLOT(update()));
  connect(&m_bounds[4], SIGNAL(manipulated()), this, SLOT(update()));
  connect(&m_bounds[5], SIGNAL(manipulated()), this, SLOT(update()));
}

BoundingBox::~BoundingBox()
{
  m_bounds[0].disconnect();
  m_bounds[1].disconnect();
  m_bounds[2].disconnect();
  m_bounds[3].disconnect();
  m_bounds[4].disconnect();
  m_bounds[5].disconnect();
  deactivateBounds();
}

void
BoundingBox::setPositions(Vec bmin, Vec bmax)
{
  Vec midPt = (m_dataMax+m_dataMin)/2;
  m_bounds[0].setPosition(Vec(bmin.x, midPt.y, midPt.z));
  m_bounds[1].setPosition(Vec(bmax.x, midPt.y, midPt.z));
  m_bounds[2].setPosition(Vec(midPt.x, bmin.y, midPt.z));
  m_bounds[3].setPosition(Vec(midPt.x, bmax.y, midPt.z));
  m_bounds[4].setPosition(Vec(midPt.x, midPt.y, bmin.z));
  m_bounds[5].setPosition(Vec(midPt.x, midPt.y, bmax.z));

  updateMidPoints();
}

void
BoundingBox::activateBounds()
{
  for(int i=0; i<6; i++)
    m_bounds[i].addInMouseGrabberPool();
}

void
BoundingBox::deactivateBounds()
{
  for(int i=0; i<6; i++)
    m_bounds[i].removeFromMouseGrabberPool();
}

void
BoundingBox::setBounds(Vec dmin, Vec dmax)
{
  m_dataMin = dmin;
  m_dataMax = dmax;

  Vec midPt = (m_dataMax+m_dataMin)/2;

  LocalConstraint *lc;
 
  // --- X-
  m_bounds[0].setPosition(Vec(m_dataMin.x, midPt.y, midPt.z));
  m_bounds[0].setOrientation(Quaternion(Vec(0,1,0), -M_PI_2));
  lc = (LocalConstraint*)m_bounds[0].constraint();
  if (!lc)
    lc = new LocalConstraint();
  lc->setRotationConstraintType(AxisPlaneConstraint::FORBIDDEN);
  lc->setTranslationConstraintType(AxisPlaneConstraint::AXIS);
  lc->setTranslationConstraintDirection(Vec(0,0,1));
  m_bounds[0].setConstraint(lc);

  // --- X+
  m_bounds[1].setPosition(Vec(m_dataMax.x, midPt.y, midPt.z));
  m_bounds[1].setOrientation(Quaternion(Vec(0,1,0), M_PI_2));
  lc = (LocalConstraint*)m_bounds[1].constraint();
  if (!lc)
    lc = new LocalConstraint();
  lc->setRotationConstraintType(AxisPlaneConstraint::FORBIDDEN);
  lc->setTranslationConstraintType(AxisPlaneConstraint::AXIS);
  lc->setTranslationConstraintDirection(Vec(0,0,1));
  m_bounds[1].setConstraint(lc);

  // --- Y-
  m_bounds[2].setPosition(Vec(midPt.x, m_dataMin.y, midPt.z));
  m_bounds[2].setOrientation(Quaternion(Vec(1,0,0), M_PI_2));
  lc = (LocalConstraint*)m_bounds[2].constraint();
  if (!lc)
    lc = new LocalConstraint();
  lc->setRotationConstraintType(AxisPlaneConstraint::FORBIDDEN);
  lc->setTranslationConstraintType(AxisPlaneConstraint::AXIS);
  lc->setTranslationConstraintDirection(Vec(0,0,1));
  m_bounds[2].setConstraint(lc);

  // --- Y+
  m_bounds[3].setPosition(Vec(midPt.x, m_dataMax.y, midPt.z));
  m_bounds[3].setOrientation(Quaternion(Vec(1,0,0), -M_PI_2));
  lc = (LocalConstraint*)m_bounds[3].constraint();
  if (!lc)
    lc = new LocalConstraint();
  lc->setRotationConstraintType(AxisPlaneConstraint::FORBIDDEN);
  lc->setTranslationConstraintType(AxisPlaneConstraint::AXIS);
  lc->setTranslationConstraintDirection(Vec(0,0,1));
  m_bounds[3].setConstraint(lc);

  // --- Z-
  m_bounds[4].setPosition(Vec(midPt.x, midPt.y, m_dataMin.z));
  m_bounds[4].setOrientation(Quaternion(Vec(1,0,0), M_PI));
  lc = (LocalConstraint*)m_bounds[4].constraint();
  if (!lc)
    lc = new LocalConstraint();
  lc->setRotationConstraintType(AxisPlaneConstraint::FORBIDDEN);
  lc->setTranslationConstraintType(AxisPlaneConstraint::AXIS);
  lc->setTranslationConstraintDirection(Vec(0,0,1));
  m_bounds[4].setConstraint(lc);

  // --- Z+
  m_bounds[5].setPosition(Vec(midPt.x, midPt.y, m_dataMax.z));
  lc = (LocalConstraint*)m_bounds[5].constraint();
  if (!lc)
    lc = new LocalConstraint();
  lc->setRotationConstraintType(AxisPlaneConstraint::FORBIDDEN);
  lc->setTranslationConstraintType(AxisPlaneConstraint::AXIS);
  lc->setTranslationConstraintDirection(Vec(0,0,1));
  m_bounds[5].setConstraint(lc);
}

void BoundingBox::bounds(Vec &bmin, Vec &bmax)
{
  int minX, maxX, minY, maxY, minZ, maxZ;
  minX = m_bounds[0].position().x;
  maxX = m_bounds[1].position().x;
  minY = m_bounds[2].position().y;
  maxY = m_bounds[3].position().y;
  minZ = m_bounds[4].position().z;
  maxZ = m_bounds[5].position().z;

  bmin = Vec(minX, minY, minZ);
  bmax = Vec(maxX, maxY, maxZ);
}

bool
BoundingBox::keyPressEvent(QKeyEvent *event)
{
  if (event->key() == Qt::Key_Up || 
      event->key() == Qt::Key_Down)
    {
      bool doUpdate = false;

      bool moveBox = false;
      moveBox = (event->modifiers() & Qt::ControlModifier);

      int shift = -1; // Down Arrow -- pullout
      if (event->key() == Qt::Key_Up) shift = 1; // Up Arrow -- pushin

      if (m_bounds[0].grabsMouse())
	{
	  Vec pos = m_bounds[0].position();
	  m_bounds[0].setPosition(pos.x+shift, pos.y, pos.z);
	  if (moveBox)
	    {
	      Vec pos = m_bounds[1].position();
	      m_bounds[1].setPosition(+shift, pos.y, pos.z);	      
	    }
	  doUpdate = true;
	}
      else if (m_bounds[1].grabsMouse())
	{
	  Vec pos = m_bounds[1].position();
	  m_bounds[1].setPosition(pos.x-shift, pos.y, pos.z);
	  if (moveBox)
	    {
	      Vec pos = m_bounds[0].position();
	      m_bounds[0].setPosition(pos.x-shift, pos.y, pos.z);
	    }
	  doUpdate = true;
	}
      else if (m_bounds[2].grabsMouse())
	{
	  Vec pos = m_bounds[2].position();
	  m_bounds[2].setPosition(pos.x, pos.y+shift, pos.z);
	  if (moveBox)
	    {
	      Vec pos = m_bounds[3].position();
	      m_bounds[3].setPosition(pos.x, pos.y+shift, pos.z);
	    }
	  doUpdate = true;
	}
      else if (m_bounds[3].grabsMouse())
	{
	  Vec pos = m_bounds[3].position();
	  m_bounds[3].setPosition(pos.x, pos.y-shift, pos.z);
	  if (moveBox)
	    {
	      Vec pos = m_bounds[2].position();
	      m_bounds[2].setPosition(pos.x, pos.y-shift, pos.z);
	    }
	  doUpdate = true;
	}
      else if (m_bounds[4].grabsMouse())
	{
	  Vec pos = m_bounds[4].position();
	  m_bounds[4].setPosition(pos.x, pos.y, pos.z+shift);
	  if (moveBox)
	    {
	      Vec pos = m_bounds[5].position();
	      m_bounds[5].setPosition(pos.x, pos.y, pos.z+shift);
	    }
	  doUpdate = true;
	}
      else if (m_bounds[5].grabsMouse())
	{
	  Vec pos = m_bounds[5].position();
	  m_bounds[5].setPosition(pos.x, pos.y, pos.z-shift);
	  if (moveBox)
	    {
	      Vec pos = m_bounds[4].position();
	      m_bounds[4].setPosition(pos.x, pos.y, pos.z-shift);
	    }
	  doUpdate = true;
	}

      if (doUpdate)
	{
	  update();
	  return true;
	}
    }

  return false;
}

void
BoundingBox::updateMidPoints()
{
  Vec bmin, bmax;  
  Vec pos;
  double v;
  int minX, maxX, minY, maxY, minZ, maxZ;

  minX = m_bounds[0].position().x;
  maxX = m_bounds[1].position().x;
  minY = m_bounds[2].position().y;
  maxY = m_bounds[3].position().y;
  minZ = m_bounds[4].position().z;
  maxZ = m_bounds[5].position().z;

  bmin = Vec(minX, minY, minZ);
  bmax = Vec(maxX, maxY, maxZ);
  pos = (bmin+bmax)/2;

  v = qMin(bmin.x, bmax.x-2);;
  v = qMin(m_dataMax.x, qMax(m_dataMin.x, v));
  m_bounds[0].setPosition(v, pos.y, pos.z);

  v = qMax(bmax.x, bmin.x+2);;
  v = qMin(m_dataMax.x, qMax(m_dataMin.x, v));
  m_bounds[1].setPosition(v, pos.y, pos.z);

  v = qMin(bmin.y, bmax.y-2);;
  v = qMin(m_dataMax.y, qMax(m_dataMin.y, v));
  m_bounds[2].setPosition(pos.x, v, pos.z);

  v = qMax(bmax.y, bmin.y+2);;
  v = qMin(m_dataMax.y, qMax(m_dataMin.y, v));
  m_bounds[3].setPosition(pos.x, v, pos.z);

  v = qMin(bmin.z, bmax.z-2);;
  v = qMin(m_dataMax.z, qMax(m_dataMin.z, v));
  m_bounds[4].setPosition(pos.x, pos.y, v);

  v = qMax(bmax.z, bmin.z+2);;
  v = qMin(m_dataMax.z, qMax(m_dataMin.z, v));
  m_bounds[5].setPosition(pos.x, pos.y, v);
}

void
BoundingBox::update()
{
  updateMidPoints();
  
  m_emitUpdate = true;
}

void
BoundingBox::draw()
{
  if (m_emitUpdate)
    {
      bool ok = true;
      for (int i=0; i<6; i++)
	{      
	  if (m_bounds[i].grabsMouse())
	    {
	      ok = false;
	      break;
	    }
	}
      if (ok)
	{
	  emit updated();
	  m_emitUpdate = false;
	}
    }

  Vec bmin, bmax;  

  bounds(bmin, bmax);

  Vec lineColor = defaultColor;
  lineColor = Vec(0.8f, 0.8f, 0.6f);

  glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
  glColor4f(boxColor.x, boxColor.y, boxColor.z, 0.9f);
  StaticFunctions::drawEnclosingCube(bmin,
				     bmax);
  glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);

  for (int i=0; i<6; i++)
    {      
      if (m_bounds[i].grabsMouse())
	{
	  glLineWidth(2);
	  switch(i)
	    {
	    case 0 :
	      drawX(bmin.x, bmin, bmax, selectColor);
	      break;
	    case 1 :
	      drawX(bmax.x, bmin, bmax, selectColor);
	      break;	     
	    case 2 :
	      drawY(bmin.y, bmin, bmax, selectColor);
	      break;	      
	    case 3 :
	      drawY(bmax.y, bmin, bmax, selectColor);
	      break;	      
	    case 4 :
	      drawZ(bmin.z, bmin, bmax, selectColor);
	      break;	      
	    case 5 :
	      drawZ(bmax.z, bmin, bmax, selectColor);
	      break;
	    }
	}
      else
	{
	  glLineWidth(1);
	  switch(i)
	    {
	    case 0 :
	      drawX(bmin.x, bmin, bmax, lineColor);
	      break;
	    case 1 :
	      drawX(bmax.x, bmin, bmax, lineColor);
	      break;	     
	    case 2 :
	      drawY(bmin.y, bmin, bmax, lineColor);
	      break;	      
	    case 3 :
	      drawY(bmax.y, bmin, bmax, lineColor);
	      break;	      
	    case 4 :
	      drawZ(bmin.z, bmin, bmax, lineColor);
	      break;	      
	    case 5 :
	      drawZ(bmax.z, bmin, bmax, lineColor);
	      break;
	    }
	}
    }

  glLineWidth(1);
}

void
BoundingBox::drawX(float vx, Vec bmin, Vec bmax,
		   Vec color)
{
  glColor3f(color.x, color.y, color.z);

  glBegin(GL_LINE_STRIP);
  glVertex3f(vx, bmin.y, bmin.z);
  glVertex3f(vx, bmin.y, bmax.z);
  glVertex3f(vx, bmax.y, bmax.z);
  glVertex3f(vx, bmax.y, bmin.z);
  glVertex3f(vx, bmin.y, bmin.z);
  glEnd();

  glBegin(GL_LINES);
  glVertex3f(vx, bmin.y, bmin.z);
  glVertex3f(vx, bmax.y, bmax.z);  
  glVertex3f(vx, bmin.y, bmax.z);
  glVertex3f(vx, bmax.y, bmin.z);
  glEnd();

  Vec len, pt0, pt1;
  len = (bmax-bmin)/3;
  pt0 = bmin + len;
  pt1 = bmin + 2*len;
  glBegin(GL_LINE_STRIP);
  glVertex3f(vx, pt0.y, pt0.z);
  glVertex3f(vx, pt0.y, pt1.z);
  glVertex3f(vx, pt1.y, pt1.z);
  glVertex3f(vx, pt1.y, pt0.z);
  glVertex3f(vx, pt0.y, pt0.z);
  glEnd();
}

void
BoundingBox::drawY(float vy, Vec bmin, Vec bmax,
		   Vec color)
{
  glColor3f(color.x, color.y, color.z);

  glBegin(GL_LINE_STRIP);
  glVertex3f(bmin.x, vy, bmin.z);
  glVertex3f(bmin.x, vy, bmax.z);
  glVertex3f(bmax.x, vy, bmax.z);
  glVertex3f(bmax.x, vy, bmin.z);
  glVertex3f(bmin.x, vy, bmin.z);
  glEnd();

  glBegin(GL_LINES);
  glVertex3f(bmin.x, vy, bmin.z);
  glVertex3f(bmax.x, vy, bmax.z);  
  glVertex3f(bmin.x, vy, bmax.z);
  glVertex3f(bmax.x, vy, bmin.z);
  glEnd();

  Vec len, pt0, pt1;
  len = (bmax-bmin)/3;
  pt0 = bmin + len;
  pt1 = bmin + 2*len;
  glBegin(GL_LINE_STRIP);
  glVertex3f(pt0.x, vy, pt0.z);
  glVertex3f(pt0.x, vy, pt1.z);
  glVertex3f(pt1.x, vy, pt1.z);
  glVertex3f(pt1.x, vy, pt0.z);
  glVertex3f(pt0.x, vy, pt0.z);
  glEnd();
}

void
BoundingBox::drawZ(float vz, Vec bmin, Vec bmax,
		   Vec color)
{
  glColor3f(color.x, color.y, color.z);

  glBegin(GL_LINE_STRIP);
  glVertex3f(bmin.x, bmin.y, vz);
  glVertex3f(bmin.x, bmax.y, vz);
  glVertex3f(bmax.x, bmax.y, vz);
  glVertex3f(bmax.x, bmin.y, vz);
  glVertex3f(bmin.x, bmin.y, vz);
  glEnd();

  glBegin(GL_LINES);
  glVertex3f(bmin.x, bmin.y, vz);
  glVertex3f(bmax.x, bmax.y, vz);  
  glVertex3f(bmin.x, bmax.y, vz);
  glVertex3f(bmax.x, bmin.y, vz);
  glEnd();

  Vec len, pt0, pt1;
  len = (bmax-bmin)/3;
  pt0 = bmin + len;
  pt1 = bmin + 2*len;
  glBegin(GL_LINE_STRIP);
  glVertex3f(pt0.x, pt0.y, vz);
  glVertex3f(pt0.x, pt1.y, vz);
  glVertex3f(pt1.x, pt1.y, vz);
  glVertex3f(pt1.x, pt0.y, vz);
  glVertex3f(pt0.x, pt0.y, vz);
  glEnd();
}
