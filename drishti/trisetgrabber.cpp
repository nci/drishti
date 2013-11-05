#include "global.h"
#include "staticfunctions.h"
#include "trisetgrabber.h"

TrisetGrabber::TrisetGrabber()
{
  m_lastX = m_lastY = -1;
  m_pressed = false;
  m_pointPressed = -1;
  m_moveAxis = MoveAll;
}

TrisetGrabber::~TrisetGrabber() { removeFromMouseGrabberPool(); }

int TrisetGrabber::pointPressed() { return m_pointPressed; }

void
TrisetGrabber::mousePosition(int& x, int& y)
{
  x = m_lastX;
  y = m_lastY;
}

int TrisetGrabber::moveAxis() { return m_moveAxis; }
void TrisetGrabber::setMoveAxis(int type) { m_moveAxis = type; }

void
TrisetGrabber::checkIfGrabsMouse(int x, int y,
				 const Camera* const camera)
{
  m_lastX = x;
  m_lastY = y;

  Vec pos = camera->projectedCoordinatesOf(centroid());
  QPoint hp0(pos.x, pos.y);
  QPoint hp1(x, y);
  if ((hp0-hp1).manhattanLength() < 100)
    setGrabsMouse(true);
  else
    {
      setGrabsMouse(false);
      m_pressed = false;
    }
}

void
TrisetGrabber::mousePressEvent(QMouseEvent* const event,
			       Camera* const camera)
{
  m_pressed = true;
  m_prevPos = event->pos();
}

void
TrisetGrabber::mouseMoveEvent(QMouseEvent* const event,
			       Camera* const camera)
{
  if (!m_pressed)
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
  
  Vec pos = position() + trans;
  setPosition(pos);

  m_prevPos = event->pos();
}

void
TrisetGrabber::mouseReleaseEvent(QMouseEvent* const event,
				 Camera* const camera)
{
  m_pressed = false;
}
