#include "camerapathnode.h"

CameraPathNode::CameraPathNode(Vec pos, Quaternion rot)
{
  m_mf = new ManipulatedFrame();

  // disable camera manipulation by mouse
  m_mf->removeFromMouseGrabberPool();

  m_mf->setPosition(pos); 
  m_mf->setOrientation(rot);

  m_constraints = new LocalConstraint();
  m_constraints->setTranslationConstraintType(AxisPlaneConstraint::FREE);
  m_mf->setConstraint(m_constraints);

  m_markForDelete = false;

  connect(m_mf, SIGNAL(modified()),
	  this, SLOT(nodeModified()));
}

void
CameraPathNode::nodeModified()
{
  emit modified();
}

bool CameraPathNode::markedForDeletion() { return m_markForDelete; }

Vec CameraPathNode::position() { return m_mf->position(); }
void CameraPathNode::setPosition(Vec pos) { m_mf->setPosition(pos); }


Quaternion CameraPathNode::orientation() { return m_mf->orientation(); }
void CameraPathNode::setOrientation(Quaternion rot) { m_mf->setOrientation(rot); }


int
CameraPathNode::keyPressEvent(QKeyEvent *event, bool &lookFrom)
{
  lookFrom = false;
  if (m_mf->grabsMouse())
    {
      if (event->key() == Qt::Key_Space)
	{
	  lookFrom = true;
	  return 1;
	}
      else if (event->modifiers() & Qt::ShiftModifier)
	{ // set rotational constraints
	  if (event->key() == Qt::Key_X)
	    {
	      m_constraints->setRotationConstraintType(AxisPlaneConstraint::AXIS);
	      m_constraints->setRotationConstraintDirection(Vec(1,0,0));
	      m_mf->setConstraint(m_constraints);
	      return 1;
	    }
	  else if (event->key() == Qt::Key_Y)
	    {
	      m_constraints->setRotationConstraintType(AxisPlaneConstraint::AXIS);
	      m_constraints->setRotationConstraintDirection(Vec(0,1,0));
	      m_mf->setConstraint(m_constraints);
	      return 1;
	    }
	  else if (event->key() == Qt::Key_Z)
	    {
	      m_constraints->setRotationConstraintType(AxisPlaneConstraint::AXIS);
	      m_constraints->setRotationConstraintDirection(Vec(0,0,1));
	      m_mf->setConstraint(m_constraints);
	      return 1;
	    }
	  else if (event->key() == Qt::Key_W)
	    {
	      m_constraints->setRotationConstraintType(AxisPlaneConstraint::FREE);
	      m_mf->setConstraint(m_constraints);
	      return 1;
	    }	  
	}
      else if (event->modifiers() == Qt::NoModifier)
	{ // set translational constraints
	  if (event->key() == Qt::Key_Delete ||
	      event->key() == Qt::Key_Backspace ||
	      event->key() == Qt::Key_Backtab)
	    {
 	      m_markForDelete = true;
	    }
	  else if (event->key() == Qt::Key_X)
	    {
	      m_constraints->setTranslationConstraintType(AxisPlaneConstraint::AXIS);
	      m_constraints->setTranslationConstraintDirection(Vec(1,0,0));
	      m_mf->setConstraint(m_constraints);
	      return 1;
	    }
	  else if (event->key() == Qt::Key_Y)
	    {
	      m_constraints->setTranslationConstraintType(AxisPlaneConstraint::AXIS);
	      m_constraints->setTranslationConstraintDirection(Vec(0,1,0));
	      m_mf->setConstraint(m_constraints);
	      return 1;
	    }
	  else if (event->key() == Qt::Key_Z)
	    {
	      m_constraints->setTranslationConstraintType(AxisPlaneConstraint::AXIS);
	      m_constraints->setTranslationConstraintDirection(Vec(0,0,1));
	      m_mf->setConstraint(m_constraints);
	      return 1;
	    }
	  else if (event->key() == Qt::Key_W)
	    {
	      m_constraints->setTranslationConstraintType(AxisPlaneConstraint::FREE);
	      m_mf->setConstraint(m_constraints);
	      return 1;
	    }
	}
    }

  return 0;
}

void
CameraPathNode::draw(float widgetSize)
{
  glPushMatrix();
  glMultMatrixd(m_mf->matrix());

  float scale = widgetSize;

  if (m_mf->grabsMouse())
    scale = widgetSize * 1.5;

  glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);

  glLineWidth(1);
  if (m_mf->grabsMouse())
    glColor3f(1, 1, 0);
  else
    glColor3f(0.6f, 1, 1);

  glBegin(GL_LINES);
  glVertex3f(0, 0, 0);
  glVertex3f(-scale, -scale, -scale);
  glVertex3f(0, 0, 0);
  glVertex3f(-scale,  scale, -scale);
  glVertex3f(0, 0, 0);
  glVertex3f(scale,   scale, -scale);
  glVertex3f(0, 0, 0);
  glVertex3f(scale,  -scale, -scale);
  glEnd();
  glBegin(GL_LINE_STRIP);
  glVertex3f(-scale, -scale, -scale);
  glVertex3f(-scale,  scale, -scale);
  glVertex3f(scale,   scale, -scale);
  glVertex3f(scale,  -scale, -scale);
  glVertex3f(-scale, -scale, -scale);
  glEnd();

  //glLineWidth(3);
  if (m_mf->grabsMouse())
    glColor3f(0.5, 1, 0.5f);
  else
    glColor3f(1, 1, 0.8f);

  glBegin(GL_LINES);
  glVertex3f(0, 0, 0);
  glVertex3f(-scale, -scale, -scale);
  glVertex3f(0, 0, 0);
  glVertex3f(-scale,  scale, -scale);
  glVertex3f(0, 0, 0);
  glVertex3f(scale,   scale, -scale);
  glVertex3f(0, 0, 0);
  glVertex3f(scale,  -scale, -scale);
  glEnd();
  glBegin(GL_LINE_STRIP);
  glVertex3f(-scale, -scale, -scale);
  glVertex3f(-scale,  scale, -scale);
  glVertex3f(scale,   scale, -scale);
  glVertex3f(scale,  -scale, -scale);
  glVertex3f(-scale, -scale, -scale);
  glEnd();

  // draw lookup vector
  float arrowscale = scale/3;
  glBegin(GL_QUADS);
  glVertex3f(-arrowscale/2, scale, -scale);
  glVertex3f(-arrowscale/2, scale+arrowscale/2, -scale);
  glVertex3f( arrowscale/2, scale+arrowscale/2, -scale);
  glVertex3f( arrowscale/2, scale, -scale);
  glEnd();
  glBegin(GL_TRIANGLES);
  glVertex3f(-arrowscale/2, scale+arrowscale/2, -scale);
  glVertex3f( arrowscale/2, scale+arrowscale/2, -scale);
  glVertex3f( 0,            scale+arrowscale, -scale);
  glEnd();


  glEnable(GL_LIGHTING);

  glColor3f(1.0f, 0.5f, 0.2f);
  if (m_constraints->translationConstraintType() == AxisPlaneConstraint::AXIS)
    {
      Vec dir = m_constraints->translationConstraintDirection();
      if (dir.norm() > 0)
	dir.normalize();

      if ((dir-Vec(0,0,1)).norm() < 0.001)
	{
	  dir *= scale;
	  QGLViewer::drawArrow(Vec(0,0,-scale),
			       dir*0.5f+Vec(0,0,-scale),
			       0.5f, 12);
	  QGLViewer::drawArrow(Vec(0,0,-scale),
			       -dir*0.5f+Vec(0,0,-scale),
			       0.5f, 12);
	}
      else
	{
	  dir *= scale;
	  QGLViewer::drawArrow(dir+Vec(0,0,-scale),
			       dir*1.3f+Vec(0,0,-scale),
			       0.5f, 12);
	  QGLViewer::drawArrow(-dir+Vec(0,0,-scale),
			       -dir*1.3f+Vec(0,0,-scale),
			       0.5f, 12);
	}
    }
  else if (m_constraints->translationConstraintType() == AxisPlaneConstraint::FREE)
    {
      Vec Xaxis, Yaxis, Zaxis;
      Xaxis = scale*Vec(1,0,0);
      Yaxis = scale*Vec(0,1,0);
      Zaxis = scale*Vec(0,0,1);
      QGLViewer::drawArrow( Xaxis+Vec(0,0,-scale),
			    Xaxis*1.3f+Vec(0,0,-scale),
			    0.5f, 12);
      QGLViewer::drawArrow(-Xaxis+Vec(0,0,-scale),
			   -Xaxis*1.3f+Vec(0,0,-scale),
			   0.5f, 12);
      QGLViewer::drawArrow( Yaxis+Vec(0,0,-scale),
			    Yaxis*1.3f+Vec(0,0,-scale),
			    0.5f, 12);
      QGLViewer::drawArrow(-Yaxis+Vec(0,0,-scale),
			   -Yaxis*1.3f+Vec(0,0,-scale),
			   0.5f, 12);
      QGLViewer::drawArrow(Vec(0,0,0)+Vec(0,0,-scale),
			   Zaxis*0.3f+Vec(0,0,-scale),
			   0.5f, 12);
      QGLViewer::drawArrow(Vec(0,0,0)+Vec(0,0,-scale),
			   -Zaxis*0.3f+Vec(0,0,-scale),
			   0.5f, 12);
    }
  glDisable(GL_LIGHTING);

  glColor3f(0.3f, 0.8f, 0.6f);
  if (m_constraints->rotationConstraintType() == AxisPlaneConstraint::AXIS)
    {
      Vec a1, a2;
      Vec dir = m_constraints->rotationConstraintDirection();
      if (dir.norm() > 0)
	dir.normalize();

      a1 = dir.orthogonalVec();
      a2 = dir^a1;

      dir *= scale;
      a1 *= 0.2f*scale;
      a2 *= 0.2f*scale;

      int nticks;
      float angle, astep;
      nticks = 50;
      astep = 6.28f/nticks;

      Vec vp[100];
      
      angle = 0.0f;
      for(int t=0; t<=nticks; t++)
	{
	  vp[t] = a1*cos(angle) + a2*sin(angle);
	  angle += astep;
	}
      
      Vec shift = Vec(0,0,-scale);
      shift += 0.2f*dir;

      //----------------------------
      glColor3f(1, 1, 1);
      glLineWidth(1);
      glBegin(GL_LINE_STRIP);
      for(int t=0; t<=nticks; t++)
	{
	  Vec v = vp[t] + shift;
	  glVertex3f(v.x, v.y, v.z);
	}
      glEnd();

      glLineWidth(3);
      glBegin(GL_LINE_STRIP);
      for(int t=0; t<=nticks; t++)
	{
	  float frc = (float)t/(float)nticks;
	  Vec v = vp[t] + shift;
	  glColor3f(1-frc, 0.5-0.5*frc, 1);
	  glVertex3f(v.x, v.y, v.z);
	}
      glEnd();

      // draw arrows
      glColor3f(0.9f, 0.8f, 0.6f);
      Vec v0, v1, v2;
      glBegin(GL_TRIANGLES);
      for(int t=0; t<nticks-4; t+=5)
	{
	  v0 = vp[t]+shift;
	  v1 = vp[t+2]+shift+0.02f*dir;
	  v2 = vp[t+2]+shift-0.02f*dir;
	  glVertex3f(v0.x, v0.y, v0.z);
	  glVertex3f(v1.x, v1.y, v1.z);
	  glVertex3f(v2.x, v2.y, v2.z);
	}
      glEnd();
      //----------------------------


      shift = Vec(0,0,-scale);
      shift -= 0.2f*dir;

      //----------------------------
      glColor3f(1, 1, 1);
      glLineWidth(1);
      glBegin(GL_LINE_STRIP);
      for(int t=0; t<=nticks; t++)
	{
	  Vec v = vp[t] + shift;
	  glVertex3f(v.x, v.y, v.z);
	}
      glEnd();

      glLineWidth(3);
      glBegin(GL_LINE_STRIP);
      for(int t=0; t<=nticks; t++)
	{
	  float frc = (float)t/(float)nticks;
	  Vec v = vp[t] + shift;
	  glColor3f(1-frc, 0.5-0.5*frc, 1);
	  glVertex3f(v.x, v.y, v.z);
	}
      glEnd();
 
      // draw arrows
      glColor3f(0.9f, 0.8f, 0.6f);
      glBegin(GL_TRIANGLES);
      for(int t=0; t<nticks-4; t+=5)
	{
	  v0 = vp[t+2]+shift;
	  v1 = vp[t]+shift+0.02f*dir;
	  v2 = vp[t]+shift-0.02f*dir;
	  glVertex3f(v0.x, v0.y, v0.z);
	  glVertex3f(v1.x, v1.y, v1.z);
	  glVertex3f(v2.x, v2.y, v2.z);
	}
      glEnd();
      //----------------------------

    }
  else if (m_constraints->rotationConstraintType() == AxisPlaneConstraint::FREE)
    {
      int nticks;
      float angle, astep;
      nticks = 50;
      astep = 6.28f/nticks;
      angle = 0.0f;

      glLineWidth(1);
      glColor3f(0.7f, 0.7f, 0.7f);

      glBegin(GL_LINE_STRIP);
      for(int t=0; t<=nticks; t++)
	{
	  float frc = (float)t/(float)nticks;
	  Vec v, v1, v2;
	  v = Vec(cos(angle),sin(angle), 0);
	  v *= 0.5f*scale;
	  angle += astep;
	  glVertex3f(v.x, v.y, v.z);
	}
      glEnd();

      glBegin(GL_LINE_STRIP);
      for(int t=0; t<=nticks; t++)
	{
	  float frc = (float)t/(float)nticks;
	  Vec v, v1, v2;
	  v = Vec(cos(angle),sin(angle), 0);
	  v *= 0.5f*scale;
	  angle += astep;
	  glVertex3f(v.x, v.z, v.y);
	}
      glEnd();

      glBegin(GL_LINE_STRIP);
      for(int t=0; t<=nticks; t++)
	{
	  float frc = (float)t/(float)nticks;
	  Vec v, v1, v2;
	  v = Vec(cos(angle),sin(angle), 0);
	  v *= 0.5f*scale;
	  angle += astep;
	  glVertex3f(v.z, v.x, v.y);
	}
      glEnd();
    }


  glLineWidth(1);
  glPopMatrix();
}

