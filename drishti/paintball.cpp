#define  _USE_MATH_DEFINES
#include <math.h>
#include "staticfunctions.h"
#include "paintball.h"
#include "global.h"
#include <QtGlobal>

bool PaintBall::grabsMouse() { return m_frame.grabsMouse(); }

PaintBall::PaintBall()
{
  m_tag = 0;
  m_thickness = 1;

  m_show = false;
  m_size = Vec(5,5,5);

  m_constraints = new LocalConstraint();
  m_constraints->setTranslationConstraintType(AxisPlaneConstraint::FREE);
  m_constraints->setRotationConstraintType(AxisPlaneConstraint::FORBIDDEN);

  m_frame.setConstraint(m_constraints);
  m_frame.removeFromMouseGrabberPool();

  m_frame.setThreshold(15); // set grabsMouse radius to 20
  m_frame.setOnlyTranslate(true); // only translations allowed
  for(int i=0; i<6; i++)
    {
      m_bounds[i].setThreshold(10); // set grabsMouse radius to 20
      m_bounds[i].setOnlyTranslate(true); // only translations allowed
    }
  
  setSizeBounds();
}
PaintBall::~PaintBall()
{
  delete m_constraints;
}

void
PaintBall::setBounds(Vec dataMin, Vec dataMax)
{
  m_dataMin = dataMin;
  m_dataMax = dataMax;

  Vec voxelScaling = Global::voxelScaling();
  Vec bmin, bmax;
  bmin = VECPRODUCT(m_dataMin, voxelScaling);
  bmax = VECPRODUCT(m_dataMax, voxelScaling);

  Vec pos = StaticFunctions::clampVec(bmin, bmax,
				      m_frame.position());

  m_fpos = pos;
  m_frame.setPosition(pos);
  setSizeBounds();
}

void
PaintBall::setSizeBounds()
{
  LocalConstraint *lc;
  Vec vf = m_frame.position();
  Vec v;

  // --- X-
  v = Vec(-m_size.x, 0, 0) + vf;
  m_bounds[0].setPosition(v);
  m_bounds[0].setOrientation(Quaternion(Vec(0,1,0), -M_PI_2));
  lc = (LocalConstraint*)m_bounds[0].constraint();
  if (!lc)
    lc = new LocalConstraint();
  lc->setRotationConstraintType(AxisPlaneConstraint::FORBIDDEN);
  lc->setTranslationConstraintType(AxisPlaneConstraint::AXIS);
  lc->setTranslationConstraintDirection(Vec(0,0,1));
  m_bounds[0].setConstraint(lc);

  // --- X+
  v = Vec(m_size.x, 0, 0) + vf;
  m_bounds[1].setPosition(v);
  m_bounds[1].setOrientation(Quaternion(Vec(0,1,0), M_PI_2));
  lc = (LocalConstraint*)m_bounds[1].constraint();
  if (!lc)
    lc = new LocalConstraint();
  lc->setRotationConstraintType(AxisPlaneConstraint::FORBIDDEN);
  lc->setTranslationConstraintType(AxisPlaneConstraint::AXIS);
  lc->setTranslationConstraintDirection(Vec(0,0,1));
  m_bounds[1].setConstraint(lc);

  // --- Y-
  v = Vec(0, -m_size.y, 0) + vf;
  m_bounds[2].setPosition(v);
  m_bounds[2].setOrientation(Quaternion(Vec(1,0,0), M_PI_2));
  lc = (LocalConstraint*)m_bounds[2].constraint();
  if (!lc)
    lc = new LocalConstraint();
  lc->setRotationConstraintType(AxisPlaneConstraint::FORBIDDEN);
  lc->setTranslationConstraintType(AxisPlaneConstraint::AXIS);
  lc->setTranslationConstraintDirection(Vec(0,0,1));
  m_bounds[2].setConstraint(lc);

  // --- Y+
  v = Vec(0, m_size.y, 0) + vf;
  m_bounds[3].setPosition(v);
  m_bounds[3].setOrientation(Quaternion(Vec(1,0,0), -M_PI_2));
  lc = (LocalConstraint*)m_bounds[3].constraint();
  if (!lc)
    lc = new LocalConstraint();
  lc->setRotationConstraintType(AxisPlaneConstraint::FORBIDDEN);
  lc->setTranslationConstraintType(AxisPlaneConstraint::AXIS);
  lc->setTranslationConstraintDirection(Vec(0,0,1));
  m_bounds[3].setConstraint(lc);

  // --- Z-
  v = Vec(0, 0, -m_size.z) + vf;
  m_bounds[4].setPosition(v);
  m_bounds[4].setOrientation(Quaternion(Vec(1,0,0), M_PI));
  lc = (LocalConstraint*)m_bounds[4].constraint();
  if (!lc)
    lc = new LocalConstraint();
  lc->setRotationConstraintType(AxisPlaneConstraint::FORBIDDEN);
  lc->setTranslationConstraintType(AxisPlaneConstraint::AXIS);
  lc->setTranslationConstraintDirection(Vec(0,0,1));
  m_bounds[4].setConstraint(lc);

  // --- Z+
  v = Vec(0, 0, m_size.z) + vf;
  m_bounds[5].setPosition(v);
  lc = (LocalConstraint*)m_bounds[5].constraint();
  if (!lc)
    lc = new LocalConstraint();
  lc->setRotationConstraintType(AxisPlaneConstraint::FORBIDDEN);
  lc->setTranslationConstraintType(AxisPlaneConstraint::AXIS);
  lc->setTranslationConstraintDirection(Vec(0,0,1));
  m_bounds[5].setConstraint(lc);


  m_pb0 = Vec(m_bounds[1].position().x,
	      m_bounds[3].position().y,
	      m_bounds[5].position().z);

  m_pb1 = Vec(m_bounds[0].position().x,
	      m_bounds[2].position().y,
	      m_bounds[4].position().z);
}

void
PaintBall::bound()
{
  Vec voxelScaling = Global::voxelScaling();
  Vec bmin, bmax;
  bmin = VECPRODUCT(m_dataMin, voxelScaling);
  bmax = VECPRODUCT(m_dataMax, voxelScaling);

  Vec pos = StaticFunctions::clampVec(bmin, bmax,
				      m_frame.position());

  if ((m_fpos-pos).squaredNorm() > 0.01)
    {
      m_fpos = pos;
      m_frame.setPosition(pos);
      setSizeBounds();
      return;
    }

  Vec pb0 = Vec(m_bounds[1].position().x,
		m_bounds[3].position().y,
		m_bounds[5].position().z);

  Vec pb1 = Vec(m_bounds[0].position().x,
		m_bounds[2].position().y,
		m_bounds[4].position().z);

  if ((pb0 - m_pb0).squaredNorm() > 0.01 ||
      (pb1 - m_pb1).squaredNorm() > 0.01)
    {
      pos = Vec(m_bounds[1].position().x+m_bounds[0].position().x,
		m_bounds[3].position().y+m_bounds[2].position().y,
		m_bounds[5].position().z+m_bounds[4].position().z);
      pos /= 2;
      m_frame.setPosition(pos);
      
      m_size = Vec(m_bounds[1].position().x-m_bounds[0].position().x,
		   m_bounds[3].position().y-m_bounds[2].position().y,
		   m_bounds[5].position().z-m_bounds[4].position().z);
      
      if (m_size.x < 0) m_size.x = -m_size.x;
      if (m_size.y < 0) m_size.y = -m_size.y;
      if (m_size.z < 0) m_size.z = -m_size.z;

      m_size = m_size/2;

      setSizeBounds();
    }
}

bool PaintBall::showPaintBall() { return m_show; }
void
PaintBall::setShowPaintBall(bool flag)
{
  m_show = flag;

  if (m_show)
    {
      m_frame.addInMouseGrabberPool();
      for(int i=0; i<6; i++)
	m_bounds[i].addInMouseGrabberPool();
    }
  else
    {
      m_frame.removeFromMouseGrabberPool();
      for(int i=0; i<6; i++)
	m_bounds[i].removeFromMouseGrabberPool();
    }

  emit updateGL();
}

void PaintBall::setPosition(Vec pos) { m_frame.setPosition(pos); }
Vec PaintBall::position()
{
  Vec pos = m_frame.position();
  Vec voxelScaling = Global::voxelScaling();
  pos = VECDIVIDE(pos, voxelScaling);
  return pos;
}

void PaintBall::setSize(Vec size)
{
  m_size = size;
  emit updateGL();
}
Vec PaintBall::size()
{
  Vec sz = m_size;
  Vec voxelScaling = Global::voxelScaling();
  sz = VECDIVIDE(sz, voxelScaling);
  return 2*sz;
}
  

void
PaintBall::draw()
{
  if (m_show == false)
    return;

  bound();

  Vec dataSize = m_dataMax-m_dataMin+Vec(1,1,1);
  float scale = 0.1*qMax(dataSize.x,
			 qMax(dataSize.y, dataSize.z));
  
  glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
  glEnable(GL_LINE_SMOOTH);

  glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
  glEnable(GL_POINT_SMOOTH);

  glPushMatrix();
  glMultMatrixd(m_frame.matrix());

  Vec linecolor = Vec(0.9f,0.8f,0.5f);
  if (m_frame.grabsMouse())
    linecolor = Vec(0,0.8f,0);

  Vec sz = m_size;

  glColor4f(linecolor.x,
	    linecolor.y,
	    linecolor.z,
	    0.9f);
  glLineWidth(1);
  glBegin(GL_LINES);
  glVertex3f(sz.x, 0, 0);
  glVertex3f(-sz.x,0, 0);
  glVertex3f(0, sz.y, 0);
  glVertex3f(0,-sz.y, 0);
  glVertex3f(0, 0, sz.z);
  glVertex3f(0, 0,-sz.z);
  glEnd();

  //------------------------------
  int nst;  
  
  glBegin(GL_LINE_STRIP);
  nst = 101*qMax<float>((float)(sz.x/dataSize.x), (float)(sz.y/dataSize.y));
  if (nst < 5) nst = 5;
  for(int i=0; i<nst; i++)
    {
      float t = (float)i/(float)(nst-1);
      float x = sz.x * cos(6.2831853*t);
      float y = sz.y * sin(6.2831853*t);
      glVertex3f(x, y, 0);
    }
  glEnd();

  glBegin(GL_LINE_STRIP);
  nst = 101*qMax<float>((float)(sz.z/dataSize.z), (float)(sz.y/dataSize.y));
  if (nst < 5) nst = 5;
  for(int i=0; i<nst; i++)
    {
      float t = (float)i/(float)(nst-1);
      float y = sz.y * cos(6.2831853*t);
      float z = sz.z * sin(6.2831853*t);
      glVertex3f(0, y, z);
    }
  glEnd();

  glBegin(GL_LINE_STRIP);
  nst = 101*qMax<float>((float)(sz.x/dataSize.x), (float)(sz.z/dataSize.z));
  if (nst < 5) nst = 5;
  for(int i=0; i<nst; i++)
    {
      float t = (float)i/(float)(nst-1);
      float x = sz.x * cos(6.2831853*t);
      float z = sz.z * sin(6.2831853*t);
      glVertex3f(x, 0, z);
    }
  glEnd();

  
  //------------------------------
  glPointSize(11);
  glBegin(GL_POINTS);
  // draw center point
  if (m_frame.grabsMouse())
    glColor4f(0,0.9f,0, 0.9f);
  else
    glColor4f(0.9f,0.8f,0.5f, 0.9f);
  glVertex3fv(Vec(0,0,0));
  // now draw the face points
  for(int i=0; i<6; i++)
    {      
      Vec v = m_bounds[i].position() - m_frame.position();
      if (m_bounds[i].grabsMouse())
	glColor4f(0.9f,0,0, 0.9f);
      else
	glColor4f(0.9f,0.8f,0.5f, 0.9f);

      glVertex3fv(v);
    }
  glEnd();  
  //------------------------------


  glEnable(GL_LIGHTING);

  //glColor3fv(linecolor);
  if (m_constraints && m_constraints->translationConstraintType() == AxisPlaneConstraint::AXIS)
    {
      Vec dir = m_constraints->translationConstraintDirection();
      if (dir.norm() > 0)
	dir.normalize();
      Vec pos = VECPRODUCT(sz, dir);
      pos += dir;
      dir *= scale;
      QGLViewer::drawArrow(pos,   pos+dir, 0.5f, 7);
      QGLViewer::drawArrow(-pos, -pos-dir, 0.5f, 7);
    }
  else if (m_constraints && m_constraints->translationConstraintType() == AxisPlaneConstraint::FREE)
    {
      for(int i=0; i<3; i++)
	{
	  Vec pos, dir;
	  dir = Vec((i==0),(i==1),(i==2));
	  pos = VECPRODUCT(sz, dir);
	  pos += dir;
	  dir *= scale;
	  QGLViewer::drawArrow(pos,   pos+dir, 0.5f, 7);
	  QGLViewer::drawArrow(-pos, -pos-dir, 0.5f, 7);
	}
    }
  glDisable(GL_LIGHTING);

  glPopMatrix();
}

bool
PaintBall::keyPressEvent(QKeyEvent *event)
{
  if (m_frame.grabsMouse() == false)
    return false;

  if (event->key() == Qt::Key_Space)
    {      
      QString cmd;
      bool ok;
      QString mesg;
      mesg = "tag, thickness\n\n";
      mesg += QString("Current tag value = %1\nCurrent surface thickness value = %2").arg(m_tag).arg(m_thickness),
      cmd = QInputDialog::getText(0,
				  "PaintBall",
				  mesg,
				  QLineEdit::Normal,
				  "",
				  &ok);
      if (ok && !cmd.isEmpty())
	processCommand(cmd);
      return true;
    }

  if (event->key() == Qt::Key_T)
    {
      // tag all
      if (event->modifiers() & Qt::ShiftModifier)
	emit tagVolume(m_tag, false);
      else
	emit tagVolume(m_tag, true);
      return true;
    }
  else if (event->key() == Qt::Key_F)
    {
      // tag connected region
      if (event->modifiers() & Qt::ShiftModifier)
	emit fillVolume(m_tag, false);
      else
	emit fillVolume(m_tag, true);
      return true;
    }
  else if (event->key() == Qt::Key_D)
    {
      // tag connected region
      if (event->modifiers() & Qt::ShiftModifier)
	emit dilateVolume(m_tag, m_thickness, false);
      else
	emit dilateVolume(m_tag, m_thickness, true);
      return true;
    }
  else if (event->key() == Qt::Key_E)
    {
      // tag connected region
      if (event->modifiers() & Qt::ShiftModifier)
	emit erodeVolume(m_tag, m_thickness, false);
      else
	emit erodeVolume(m_tag, m_thickness, true);
      return true;
    }
  else if (event->key() == Qt::Key_S)
    {
      // tag surface
      if (event->modifiers() & Qt::ShiftModifier)
	emit tagSurface(m_tag, m_thickness, false, false);
      else
	emit tagSurface(m_tag, m_thickness, true, false);
      return true;
    }
  else if (event->key() == Qt::Key_C)
    {
      // tag connected surface
      if (event->modifiers() & Qt::ShiftModifier)
	emit tagSurface(m_tag, m_thickness, false, true);
      else
	emit tagSurface(m_tag, m_thickness, true, true);
      return true;
    }
  else if (event->key() == Qt::Key_X)
    {
      m_constraints->setTranslationConstraintType(AxisPlaneConstraint::AXIS);
      m_constraints->setTranslationConstraintDirection(Vec(1,0,0));
      m_frame.setConstraint(m_constraints);
      return true;
    }
  else if (event->key() == Qt::Key_Y)
    {
      m_constraints->setTranslationConstraintType(AxisPlaneConstraint::AXIS);
      m_constraints->setTranslationConstraintDirection(Vec(0,1,0));
      m_frame.setConstraint(m_constraints);
      return true;
    }
  else if (event->key() == Qt::Key_Z)
    {
      m_constraints->setTranslationConstraintType(AxisPlaneConstraint::AXIS);
      m_constraints->setTranslationConstraintDirection(Vec(0,0,1));
      m_frame.setConstraint(m_constraints);
      return true;
    }
  else if (event->key() == Qt::Key_W)
    {
      m_constraints->setTranslationConstraintType(AxisPlaneConstraint::FREE);
      m_frame.setConstraint(m_constraints);
      return true;
    }

  return false;
}

void
PaintBall::processCommand(QString cmd)
{
  bool ok;
  cmd = cmd.toLower();
  QStringList list = cmd.split(" ", QString::SkipEmptyParts);
  
  if (list[0] == "tag")
    {
      if (list.size() > 1) m_tag = list[1].toInt(&ok);
    }
  else if (list[0] == "thickness")
    {
      int t;
      if (list.size() > 1) t = list[1].toInt(&ok);
      m_thickness = qMax(1, t);
    }
  else
    QMessageBox::information(0, "Error",
			     QString("Cannot understand the command : ") +
			     cmd);

}
