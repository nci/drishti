#include "global.h"
#include "clipgrabber.h"
#include "staticfunctions.h"
#include "matrix.h"

ClipGrabber::ClipGrabber()
{
  m_lastX = m_lastY = -1;
  m_pressed = false;
  m_pointPressed = -1;
}

ClipGrabber::~ClipGrabber() { removeFromMouseGrabberPool(); }

int ClipGrabber::pointPressed() { return m_pointPressed; }

void
ClipGrabber::mousePosition(int& x, int& y)
{
  x = m_lastX;
  y = m_lastY;
}

bool
ClipGrabber::xActive(const Camera* const camera,
		     Vec pos, Vec pp, bool &flag)
{
  Vec tang = m_tang;
  Vec xaxis = m_xaxis;
  Vec yaxis = m_yaxis;
  tang = Matrix::rotateVec(m_xform, tang);
  xaxis = Matrix::rotateVec(m_xform, xaxis);
  yaxis = Matrix::rotateVec(m_xform, yaxis);

  float s1 = tscale1();
  float s2 = tscale2();
  Vec c0, ca, cb, c1;

  c0 = pos + s1*xaxis;
  c0 = camera->projectedCoordinatesOf(c0); c0 = Vec(c0.x, c0.y, 0);

  c1 = pos - s1*xaxis;
  c1 = camera->projectedCoordinatesOf(c1); c1 = Vec(c1.x, c1.y, 0);

  ca = pos - 0.2*s2*yaxis;
  ca = camera->projectedCoordinatesOf(ca); ca = Vec(ca.x, ca.y, 0);

  cb = pos + 0.2*s2*yaxis;
  cb = camera->projectedCoordinatesOf(cb); cb = Vec(cb.x, cb.y, 0);

  flag = StaticFunctions::inTriangle(ca, cb, c0, pp);
  return (flag || StaticFunctions::inTriangle(ca, cb, c1, pp));
}

bool
ClipGrabber::yActive(const Camera* const camera,
		     Vec pos, Vec pp, bool &flag)
{
  Vec tang = m_tang;
  Vec xaxis = m_xaxis;
  Vec yaxis = m_yaxis;
  tang = Matrix::rotateVec(m_xform, tang);
  xaxis = Matrix::rotateVec(m_xform, xaxis);
  yaxis = Matrix::rotateVec(m_xform, yaxis);

  float s1 = tscale1();
  float s2 = tscale2();
  Vec c0, ca, cb, c1;

  c0 = pos + s2*yaxis;
  c0 = camera->projectedCoordinatesOf(c0); c0 = Vec(c0.x, c0.y, 0);

  c1 = pos - s2*yaxis;
  c1 = camera->projectedCoordinatesOf(c1); c1 = Vec(c1.x, c1.y, 0);

  ca = pos - 0.2*s1*xaxis;
  ca = camera->projectedCoordinatesOf(ca); ca = Vec(ca.x, ca.y, 0);

  cb = pos + 0.2*s1*xaxis;
  cb = camera->projectedCoordinatesOf(cb); cb = Vec(cb.x, cb.y, 0);
  
  flag = StaticFunctions::inTriangle(ca, cb, c0, pp);
  return (flag || StaticFunctions::inTriangle(ca, cb, c1, pp));
}

bool
ClipGrabber::zActive(const Camera* const camera,
		     Vec pos, Vec pp)
{
  Vec tang = m_tang;
  Vec xaxis = m_xaxis;
  Vec yaxis = m_yaxis;
  tang = Matrix::rotateVec(m_xform, tang);
  xaxis = Matrix::rotateVec(m_xform, xaxis);
  yaxis = Matrix::rotateVec(m_xform, yaxis);

  float s1 = tscale1();
  float s2 = tscale2();
  float r = size();
  Vec c0, cax, cbx, cay, cby;

  c0 = pos + r*tang;
  c0 = camera->projectedCoordinatesOf(c0); c0 = Vec(c0.x, c0.y, 0);

  cax = pos - 0.2*s1*xaxis;
  cax = camera->projectedCoordinatesOf(cax); cax = Vec(cax.x, cax.y, 0);

  cbx = pos + 0.2*s1*xaxis;
  cbx = camera->projectedCoordinatesOf(cbx); cbx = Vec(cbx.x, cbx.y, 0);

  cay = pos - 0.2*s2*yaxis;
  cay = camera->projectedCoordinatesOf(cay); cay = Vec(cay.x, cay.y, 0);

  cby = pos + 0.2*s2*yaxis;
  cby = camera->projectedCoordinatesOf(cby); cby = Vec(cby.x, cby.y, 0);

  return (StaticFunctions::inTriangle(cax, cbx, c0, pp) ||
	  StaticFunctions::inTriangle(cay, cby, c0, pp));
}

void
ClipGrabber::checkIfGrabsMouse(int x, int y,
			       const Camera* const camera)
{
  if (m_pressed)
    {
      // mouse button pressed so keep grabbing
      setActive(true);
      setGrabsMouse(true);
      return;
    }

  int sz = 20;

  m_lastX = x;
  m_lastY = y;

  Vec voxelScaling = Global::voxelScaling();
  Vec pos = VECPRODUCT(position(), voxelScaling);
  pos = Matrix::xformVec(m_xform, pos);

  Vec pp = Vec(x, y, 0);
    
  bool flag;
  if (xActive(camera, pos, pp, flag) ||
      yActive(camera, pos, pp, flag) ||
      zActive(camera, pos, pp))
    {
      setActive(true);
      setGrabsMouse(true);
      return;
    }

  setActive(false);
  setGrabsMouse(false);
}

void
ClipGrabber::mousePressEvent(QMouseEvent* const event,
				 Camera* const camera)
{
  m_pointPressed = -1;
  m_pressed = true;
  m_prevPos = event->pos();

  Vec voxelScaling = Global::voxelScaling();
  Vec pp = Vec(m_prevPos.x(), m_prevPos.y(), 0);
  Vec pos = VECPRODUCT(position(), voxelScaling);
  pos = Matrix::xformVec(m_xform, pos);

  bool flag;

  if (xActive(camera, pos, pp, flag))
    {
      if (flag)
	setMoveAxis(MoveX0);
      else
	setMoveAxis(MoveX1);
      emit selectForEditing();
      return;
    }
  else if (yActive(camera, pos, pp, flag))
    {
      if (flag)
	setMoveAxis(MoveY0);
      else
	setMoveAxis(MoveY1);
      emit selectForEditing();
      return;
    }
  else if (zActive(camera, pos, pp))
    {
      setMoveAxis(MoveZ);
      emit selectForEditing();
      return;
    }
}

void
ClipGrabber::mouseMoveEvent(QMouseEvent* const event,
			    Camera* const camera)
{
  if (!m_pressed)
    return;
  
  QPoint delta = event->pos() - m_prevPos;

  Vec voxelScaling = Global::voxelScaling();

  Vec tang = m_tang;
  Vec xaxis = m_xaxis;
  Vec yaxis = m_yaxis;
  tang = Matrix::rotateVec(m_xform, tang);
  xaxis = Matrix::rotateVec(m_xform, xaxis);
  yaxis = Matrix::rotateVec(m_xform, yaxis);

  if (event->buttons() != Qt::LeftButton)
    {
      tang = VECDIVIDE(tang, voxelScaling);
      xaxis = VECDIVIDE(xaxis, voxelScaling);
      yaxis = VECDIVIDE(yaxis, voxelScaling);

      Vec trans(delta.x(), -delta.y(), 0.0f);
      
      // Scale to fit the screen mouse displacement
      trans *= 2.0 * tan(camera->fieldOfView()/2.0) *
	             fabs((camera->frame()->coordinatesOf(Vec(0,0,0))).z) /
	             camera->screenHeight();

      // Transform to world coordinate system.
      trans = camera->frame()->orientation().rotate(trans);

      Vec voxelScaling = Global::voxelScaling();
      trans = VECDIVIDE(trans, voxelScaling);

      if (event->modifiers() & Qt::ControlModifier ||
	  event->modifiers() & Qt::MetaModifier)
	{
	  if (moveAxis() < MoveY0)
	    {
	      float vx = trans*m_xaxis;
	      if (moveAxis() == MoveX0)
		setScale1(scale1() + 0.05*vx);
	      else
		setScale1(scale1() - 0.05*vx);
	    }
	  else if (moveAxis() < MoveZ)
	    {
	      float vy = trans*m_yaxis;
	      if (moveAxis() == MoveY0)
		setScale2(scale2() + 0.05*vy);
	      else
		setScale2(scale2() - 0.05*vy);
	    }
	}
      else
	{
	  if (moveAxis() < MoveY0)
	    {
	      float vx = trans*xaxis;
	      trans = vx*xaxis;
	    }
	  else if (moveAxis() < MoveZ)
	    {
	      float vy = trans*yaxis;
	      trans = vy*yaxis;
	    }
	  else if (moveAxis() == MoveZ)
	    {
	      float vz = trans*tang;
	      if (qAbs(vz) < 0.1)
		{
		  vz = trans.norm();
		  if (qAbs(delta.x()) > qAbs(delta.y()))
		    vz = (delta.x() >= 0 ? 1 : -1);
		  else
		    vz = (delta.y() >= 0 ? 1 : -1);
		}
	      
	      trans = vz*tang;
	    }
	  
	  translate(trans);
	}
    }
  else
    {
      if (moveAxis() == MoveZ)
	{
	  Vec axis;
	  axis = (delta.y()*camera->rightVector() +
		  delta.x()*camera->upVector());
	  rotate(axis, qMax(qAbs(delta.x()), qAbs(delta.y())));

	  //rotate(camera->rightVector(), delta.y());
	  //rotate(camera->upVector(), delta.x());
	}
      else
	{
	  Vec axis;
	  if (moveAxis() < MoveY0) axis = xaxis;
	  else if (moveAxis() < MoveZ) axis = yaxis;

	  Vec voxelScaling = Global::voxelScaling();
	  Vec pos = VECPRODUCT(position(), voxelScaling);
	  pos = Matrix::xformVec(m_xform, pos);
	  
	  float r = size();
	  Vec trans(delta.x(), delta.y(), 0.0f);
	  
	  Vec p0 = camera->projectedCoordinatesOf(pos); p0 = Vec(p0.x, p0.y, 0);
	  Vec c0 = pos + r*axis;
	  c0 = camera->projectedCoordinatesOf(c0); c0 = Vec(c0.x, c0.y, 0);
	  Vec perp = c0-p0;
	  perp = Vec(-perp.y, perp.x, 0);
	  perp.normalize();
	  
	  float angle = perp * trans;
	  rotate(axis, angle);
	}

//      if (moveAxis() == MoveZ)
//	{
//	  float ag;
//	  if (qAbs(delta.x()) > qAbs(delta.y()))
//	    ag = delta.x();
//	  else
//	    ag = delta.y();
//	  rotate(camera->viewDirection(), ag);
//	}
//      else

//      Vec axis;
//      if (moveAxis() < MoveY0) axis = xaxis;
//      else if (moveAxis() < MoveZ) axis = yaxis;
//      else if (moveAxis() == MoveZ) axis = tang;
//
//      Vec voxelScaling = Global::voxelScaling();
//      Vec pos = VECPRODUCT(position(), voxelScaling);
//      pos = Matrix::xformVec(m_xform, pos);
//
//      float r = size();
//      Vec trans(delta.x(), -delta.y(), 0.0f);
//
//      Vec p0 = camera->projectedCoordinatesOf(pos); p0 = Vec(p0.x, p0.y, 0);
//      Vec c0 = pos + r*axis;
//      c0 = camera->projectedCoordinatesOf(c0); c0 = Vec(c0.x, c0.y, 0);
//      Vec perp = c0-p0;
//      perp = Vec(-perp.y, perp.x, 0);
//      perp.normalize();
//
//      float angle = perp * trans;
//      rotate(axis, angle);
    }

  m_prevPos = event->pos();
}

void
ClipGrabber::mouseReleaseEvent(QMouseEvent* const event,
			       Camera* const camera)
{
  m_pressed = false;
  m_pointPressed = -1;

  setActive(false);
  emit deselectForEditing();
}

void
ClipGrabber::wheelEvent(QWheelEvent* const event,
			Camera* const camera)
{
  int mag = event->delta()/8.0f/15.0f;
  if (event->modifiers() & Qt::ShiftModifier)
    {
      int tk = thickness();
      tk = qBound(0, tk+mag, 100);
      setThickness(tk);
    }
  else
    {
      Vec tang = m_tang;
      tang = Matrix::rotateVec(m_xform, tang);
      translate(mag*tang);
    }
}
