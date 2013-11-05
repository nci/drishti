#include "scalebargrabber.h"

ScaleBarGrabber::ScaleBarGrabber() { m_dragging = false; }
ScaleBarGrabber::~ScaleBarGrabber() { removeFromMouseGrabberPool(); }

void
ScaleBarGrabber::checkIfGrabsMouse(int x, int y,
				  const Camera* const camera)
{
  QPointF pos = position();
  int cx = pos.x() * camera->screenWidth();
  int cy = pos.y() * camera->screenHeight();

  int cxs, cys, cxe, cye;
  if (type())//horizontal
    {
      cxs = cx-50;
      cys = cy-10;
      cxe = cx+50;
      cye = cy+50;
    }
  else
    {
      cxs = cx-10;
      cys = cy-50;
      cxe = cx+50;
      cye = cy+50;
    }

  if (m_dragging ||
      x >= cxs && x <= cxe &&
      y >= cys && y <= cye)
    setGrabsMouse(true);
  else
    setGrabsMouse(false);
}

void
ScaleBarGrabber::mousePressEvent(QMouseEvent* const event,
				Camera* const camera)
{
  m_dragging = true;
  m_prevx = event->x();
  m_prevy = event->y();
  m_prevpos = position();
}
void
ScaleBarGrabber::mouseReleaseEvent(QMouseEvent* const event,
				  Camera* const camera)
{
  m_dragging = false;
}
void
ScaleBarGrabber::mouseMoveEvent(QMouseEvent* const event,
				Camera* const camera)
{
  if (m_dragging)
    {

      float dx = event->x()-m_prevx;
      float dy = event->y()-m_prevy;

      dx /= camera->screenWidth();
      dy /= camera->screenHeight();      
      setPosition(m_prevpos+QPointF(dx,dy));
    }
}

ScaleBarObject
ScaleBarGrabber::scalebar()
{
  ScaleBarObject so;
  so.set(position(), voxels(), type(), textpos());
  return so;
}
