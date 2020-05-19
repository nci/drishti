#include "global.h"
#include "staticfunctions.h"
#include "trisetgrabber.h"
#include "matrix.h"

TrisetGrabber::TrisetGrabber()
{
  m_pressed = false;
  m_moveAxis = MoveAll;
}

TrisetGrabber::~TrisetGrabber() { removeFromMouseGrabberPool(); }

bool TrisetGrabber::mousePressed() { return m_pressed; }

int TrisetGrabber::moveAxis() { return m_moveAxis; }
void TrisetGrabber::setMoveAxis(int type) { m_moveAxis = type; }

void
TrisetGrabber::setMouseGrab(bool f)
{
  setGrabsMouse(f);
  if (!f) m_pressed = false;
}

float
TrisetGrabber::checkForMouseHover(int x, int y,
				  const Camera* const camera)
{
  Vec pos = camera->projectedCoordinatesOf(centroid()+position());
  QPoint hp0(pos.x, pos.y);
  QPoint hp1(x, y);
  if ((hp0-hp1).manhattanLength() < 100)
    {
      Vec cpos = camera->position();
      Vec V = camera->viewDirection();
      float d = (pos-cpos)*V;
      return d;
    }

  return -1;
}

void
TrisetGrabber::checkIfGrabsMouse(int x, int y,
				 const Camera* const camera)
{
////  // don't want to grab mouse
////  setGrabsMouse(false);
//
//  Vec pos = camera->projectedCoordinatesOf(centroid()+position());
//  QPoint hp0(pos.x, pos.y);
//  QPoint hp1(x, y);
//  if ((hp0-hp1).manhattanLength() < 100)
//    setGrabsMouse(true);
//  else
//    {
//      setGrabsMouse(false);
//      m_pressed = false;
//    }
}

void
TrisetGrabber::mousePressEvent(QMouseEvent* const event,
			       Camera* const camera)
{
  m_pressed = true;
  m_prevPos = event->pos();
}

float
TrisetGrabber::projectOnBall(float x, float y)
{
  // If you change the size value, change angle computation in deformedBallQuaternion().
  const qreal size       = 1.0;
  const qreal size2      = size*size;
  const qreal size_limit = size2*0.5;
  
  const qreal d = x*x + y*y;
  return d < size_limit ? sqrt(size2 - d) : size_limit/sqrt(d);
}

void
TrisetGrabber::mouseMoveEvent(QMouseEvent* const event,
			       Camera* const camera)
{
  if (!m_pressed)
    return;
  

  QPoint delta = event->pos() - m_prevPos;

  if (event->buttons() == Qt::LeftButton &&
      event->modifiers() == Qt::NoModifier)
    {
      Vec trans(delta.x(), -delta.y(), 0.0f);
      
      // Scale to fit the screen mouse displacement
      trans *= 2.0 * tan(camera->fieldOfView()/2.0) *
	fabs((camera->frame()->coordinatesOf(Vec(0,0,0))).z) /
	camera->screenHeight();
      // Transform to world coordinate system.
      trans = camera->frame()->orientation().rotate(trans);
      
      if (m_moveAxis == MoveX)
	trans = Vec(trans.x,0,0);
      else if (m_moveAxis == MoveY)
	trans = Vec(0,trans.y,0);
      else if (m_moveAxis == MoveZ)
	trans = Vec(0,0,trans.z);

      Vec pos = position() + trans;
      setPosition(pos);
    }
  else if (event->buttons() == Qt::LeftButton &&
	   event->modifiers() & Qt::ControlModifier)
    {
      if (moveAxis() == MoveAll)
	{
	  Vec trans = camera->projectedCoordinatesOf(centroid()+position());

	  int x = event->x();
	  int y = event->y();
	  qreal px = (m_prevPos.x()  - trans.x)/ camera->screenWidth();
	  qreal py = (trans.y - m_prevPos.y()) / camera->screenHeight();
	  qreal dx = (x - trans.x)	       / camera->screenWidth();
	  qreal dy = (trans.y - y)	       / camera->screenHeight();
	  
	  const Vec p1(px, py, projectOnBall(px, py));
	  const Vec p2(dx, dy, projectOnBall(dx, dy));
	  Vec axis = cross(p2,p1);
	  qreal angle = 5.0 * asin(sqrt(axis.squaredNorm() / p1.squaredNorm() / p2.squaredNorm()));

	  if (event->modifiers() &= Qt::ShiftModifier)
	    {
	      if (axis*camera->viewDirection() < 0.0)
		angle = -angle;
	      axis = camera->viewDirection();
	    }

	  axis = Matrix::rotateVec(m_localXform, axis);

	  Quaternion rot = Quaternion(axis, angle);

	  rotate(rot);
	}
      else
	{
	  Vec axis;
	  if (moveAxis() < MoveY) axis = Vec(1,0,0);
	  else if (moveAxis() < MoveZ) axis = Vec(0,1,0);
	  else axis = Vec(0,0,1);
	  
	  Vec voxelScaling = Global::voxelScaling();
	  Vec pos = VECPRODUCT(position(), voxelScaling);
	  pos = Matrix::xformVec(m_localXform, pos);
	  
	  float r = 1.0;
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
    }
  
  m_prevPos = event->pos();
}

void
TrisetGrabber::mouseReleaseEvent(QMouseEvent* const event,
				 Camera* const camera)
{
  m_pressed = false;
}
