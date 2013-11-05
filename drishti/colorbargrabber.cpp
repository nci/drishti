#include "colorbargrabber.h"

ColorBarGrabber::ColorBarGrabber() { m_dragging = false; }
ColorBarGrabber::~ColorBarGrabber() { removeFromMouseGrabberPool(); }

void
ColorBarGrabber::checkIfGrabsMouse(int x, int y,
				  const Camera* const camera)
{
  QPointF pos = position();
  int cx = pos.x() * camera->screenWidth();
  int cy = pos.y() * camera->screenHeight();

  if (m_dragging ||
      x >= cx && x <= cx+width() &&
      y <= cy && y >= cy-height())
    setGrabsMouse(true);
  else
    setGrabsMouse(false);
}

void
ColorBarGrabber::mousePressEvent(QMouseEvent* const event,
				Camera* const camera)
{
  m_dragging = true;
  m_prevx = event->x();
  m_prevy = event->y();
  m_prevpos = position();
}
void
ColorBarGrabber::mouseReleaseEvent(QMouseEvent* const event,
				  Camera* const camera)
{
  m_dragging = false;
}
void
ColorBarGrabber::mouseMoveEvent(QMouseEvent* const event,
				Camera* const camera)
{
  if (m_dragging)
    {

      float dx = event->x()-m_prevx;
      float dy = event->y()-m_prevy;

      if (event->buttons() != Qt::LeftButton) // scale
	{
	  if (fabs(dx) > fabs(dy))
	    scale(dx*0.1, 0.0);
	  else
	    scale(0.0, -dy*0.1);
	}
      else
	{
	  dx /= camera->screenWidth();
	  dy /= camera->screenHeight();      
	  setPosition(m_prevpos+QPointF(dx,dy));
	}
    }
}

ColorBarObject
ColorBarGrabber::colorbar()
{
  ColorBarObject co;
  co.set(position(), type(), tfset(), width(), height(), onlyColor());
  return co;
}
