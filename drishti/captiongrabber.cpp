#include "captiongrabber.h"

CaptionGrabber::CaptionGrabber() { m_dragging = false; }
CaptionGrabber::~CaptionGrabber() { removeFromMouseGrabberPool(); }

void
CaptionGrabber::checkIfGrabsMouse(int x, int y,
				  const Camera* const camera)
{
  QPointF pos = position();
  int cx = pos.x() * camera->screenWidth();
  int cy = pos.y() * camera->screenHeight();
  int wd = width();
  int ht = height();

  Quaternion q(Vec(0, 0, 1), -angle()*3.14159265/180.0);
  Vec pt = q.rotate(Vec(x-(cx+wd/2), y-(cy-ht/2), 0));

  if (m_dragging ||
      pt.x >= -wd/2 && pt.x <= wd/2 &&
      pt.y >= -ht/2 && pt.y <= ht/2)
    setGrabsMouse(true);
  else
    setGrabsMouse(false);
}

void
CaptionGrabber::mousePressEvent(QMouseEvent* const event,
				Camera* const camera)
{
  m_dragging = true;
  m_prevx = event->x();
  m_prevy = event->y();
  m_prevpos = position();
}
void
CaptionGrabber::mouseReleaseEvent(QMouseEvent* const event,
				  Camera* const camera)
{
  m_dragging = false;
}
void
CaptionGrabber::mouseMoveEvent(QMouseEvent* const event,
				Camera* const camera)
{
  if (m_dragging)
    {
      float dx = (float)(event->x()-m_prevx)/camera->screenWidth();
      float dy = (float)(event->y()-m_prevy)/camera->screenHeight();      

      setPosition(m_prevpos+QPointF(dx,dy));
    }
}

CaptionObject
CaptionGrabber::caption()
{
  CaptionObject co;
  co.set(position(),
	 text(),
	 font(),
	 color(),
	 haloColor(),
	 angle());
  return co;
}
