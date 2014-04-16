#include "global.h"
#include "imagecaptiongrabber.h"
#include "staticfunctions.h"

ImageCaptionObject
ImageCaptionGrabber::imageCaption()
{
  ImageCaptionObject co;
  co.set(position(), imageFile(),
	 width(), height());
  return co;
}

ImageCaptionGrabber::ImageCaptionGrabber() { m_pressed = false; }
ImageCaptionGrabber::~ImageCaptionGrabber() {}

void
ImageCaptionGrabber::checkIfGrabsMouse(int x, int y,
				   const Camera* const camera)
{
  if (m_pressed)
    {
      // mouse button pressed so keep grabbing
      setGrabsMouse(true);
      return;
    }
  Vec voxelSize = Global::voxelScaling();
  Vec pos = VECPRODUCT(position(), voxelSize);
  pos = camera->projectedCoordinatesOf(pos);
  QPoint hp(pos.x, pos.y);
  if ((hp-QPoint(x,y)).manhattanLength() < 50)
    setGrabsMouse(true);
  else
    setGrabsMouse(false);
}

void
ImageCaptionGrabber::mousePressEvent(QMouseEvent* const event,
				     Camera* const camera)
{
  m_pressed = true;
  if (!active()) emit activated();
  setActive(!active());
}

void
ImageCaptionGrabber::mouseMoveEvent(QMouseEvent* const event,
				Camera* const camera)
{ 
  // do not move
}

void
ImageCaptionGrabber::mouseReleaseEvent(QMouseEvent* const event,
				       Camera* const camera)
{
  m_pressed = false;
}
