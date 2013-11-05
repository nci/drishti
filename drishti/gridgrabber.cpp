#include "global.h"
#include "gridgrabber.h"
#include "staticfunctions.h"

GridGrabber::GridGrabber()
{
  m_lastX = m_lastY = -1;
  m_pressed = false;
  m_pointPressed = -1;
  m_moveAxis = MoveAll;
  m_moved = false;
}

GridGrabber::~GridGrabber() { removeFromMouseGrabberPool(); }

int GridGrabber::moveAxis() { return m_moveAxis; }
void GridGrabber::setMoveAxis(int type) { m_moveAxis = type; }

int GridGrabber::pointPressed() { return m_pointPressed; }

void
GridGrabber::mousePosition(int& x, int& y)
{
  x = m_lastX;
  y = m_lastY;
}

void
GridGrabber::checkIfGrabsMouse(int x, int y,
			       const Camera* const camera)
{
  if (m_pressed)
    {
      // mouse button pressed so keep grabbing
      setGrabsMouse(true);
      return;
    }

  int sz = 20;
  QList<Vec> pts = gridPoints();

  m_lastX = x;
  m_lastY = y;
  QPoint mpos(x, y);

  for(int i=0; i<pts.count(); i++)
    {
      Vec pos = camera->projectedCoordinatesOf(pts[i]);
      QPoint hp(pos.x, pos.y);
      if ((hp-mpos).manhattanLength() < 30)
	{
	  setGrabsMouse(true);
	  return;
	}
    }

  setGrabsMouse(false);
}

void
GridGrabber::mousePressEvent(QMouseEvent* const event,
				 Camera* const camera)
{
  m_pointPressed = -1;
  m_pressed = true;
  m_moved = false;
  m_prevPos = event->pos();

  if (event->button() == Qt::MidButton)
    {
      setPointPressed(m_pointPressed);
      return;
    }

  Vec voxelScaling = Global::voxelScaling();
  QList<Vec> pts = points();
  for(int i=0; i<pts.count(); i++)
    {
      Vec v = VECPRODUCT(pts[i], voxelScaling);
      Vec pos = camera->projectedCoordinatesOf(v);
      QPoint hp(pos.x, pos.y);
      if ((hp-m_prevPos).manhattanLength() < 20)
	{
	  m_pointPressed = i;
	  break;
	}
    }

  setPointPressed(m_pointPressed);

  emit selectForEditing(event->button(), -1);
}

void
GridGrabber::mouseMoveEvent(QMouseEvent* const event,
			    Camera* const camera)
{
  if (!m_pressed && getPointPressed() == -1)
    return;
  
  QPoint delta = event->pos() - m_prevPos;
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
  

  m_pointPressed = getPointPressed();
  if (m_pointPressed > -1)
    {
      Vec pt = getPoint(m_pointPressed);
      pt += trans;
      setPoint(m_pointPressed, pt);
    }
  else
    setPoint(m_pointPressed, trans);

  m_moved = true;

  m_prevPos = event->pos();
}

void
GridGrabber::mouseReleaseEvent(QMouseEvent* const event,
			       Camera* const camera)
{
  if (m_moved)
    updateUndo();

  m_moved = false;
  m_pressed = false;
  m_pointPressed = -1;

  emit deselectForEditing();
}
