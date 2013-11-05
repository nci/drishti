#include "global.h"
#include "staticfunctions.h"
#include "networkgrabber.h"

NetworkGrabber::NetworkGrabber()
{
  m_lastX = m_lastY = -1;
  m_pressed = false;
  m_pointPressed = -1;
}

NetworkGrabber::~NetworkGrabber() { removeFromMouseGrabberPool(); }

int NetworkGrabber::pointPressed() { return m_pointPressed; }

void
NetworkGrabber::mousePosition(int& x, int& y)
{
  x = m_lastX;
  y = m_lastY;
}

void
NetworkGrabber::checkIfGrabsMouse(int x, int y,
				 const Camera* const camera)
{
  m_lastX = x;
  m_lastY = y;

//  if (m_pressed)
//    {
//      // mouse button pressed so keep grabbing
//      setGrabsMouse(true);
//      return;
//    }
  
  Vec pos = camera->projectedCoordinatesOf(centroid());
  QPoint hp0(pos.x, pos.y);
  QPoint hp1(x, y);
  if ((hp0-hp1).manhattanLength() < 50)
    setGrabsMouse(true);
  else
    setGrabsMouse(false);
}

void
NetworkGrabber::mousePressEvent(QMouseEvent* const event,
				Camera* const camera)
{
}

void
NetworkGrabber::mouseMoveEvent(QMouseEvent* const event,
			       Camera* const camera)
{
}

void
NetworkGrabber::mouseReleaseEvent(QMouseEvent* const event,
				  Camera* const camera)
{
}
