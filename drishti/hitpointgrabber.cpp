#include "global.h"
#include "hitpointgrabber.h"
#include "staticfunctions.h"

QList<HitPointGrabber*> HitPointGrabber::activeHitPointPool_;

const QList<HitPointGrabber*>
HitPointGrabber::activePool() { return activeHitPointPool_; }

void HitPointGrabber::setPoint(Vec pt) { m_point = pt; m_moveAxis = MoveAll; }
Vec HitPointGrabber::point() { return m_point; }

bool HitPointGrabber::active() { return m_active; }
void HitPointGrabber::resetActive()
{
  m_pressed = false;
  m_active = false;
  removeFromActivePool();
  m_moveAxis = MoveAll;
}

int HitPointGrabber::moveAxis() { return m_moveAxis; }
void HitPointGrabber::setMoveAxis(int type) { m_moveAxis = type; }

HitPointGrabber::HitPointGrabber(Vec pt)
{  
  m_pressed = false;
  m_point = pt;
  m_active = false;
  m_moveAxis = MoveAll;
}
HitPointGrabber::~HitPointGrabber()
{
  removeFromActivePool();
  removeFromMouseGrabberPool();
}
void
HitPointGrabber::checkIfGrabsMouse(int x, int y,
				   const Camera* const camera)
{
  if (m_pressed)
    {
      // mouse button pressed so keep grabbing
      setGrabsMouse(true);
      return;
    }
  Vec voxelSize = Global::voxelScaling();
  Vec pos = VECPRODUCT(m_point, voxelSize);
  pos = camera->projectedCoordinatesOf(pos);
  QPoint hp(pos.x, pos.y);
  if ((hp-QPoint(x,y)).manhattanLength() < 10)
    setGrabsMouse(true);
  else
    setGrabsMouse(false);
}

void
HitPointGrabber::mousePressEvent(QMouseEvent* const event,
				 Camera* const camera)
{
  m_pressed = true;
  m_prevPos = event->pos();
  m_active = !m_active;

  if (m_active)
    addToActivePool();
  else
    removeFromActivePool();
}

void
HitPointGrabber::mouseMoveEvent(QMouseEvent* const event,
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
  
  Vec voxelScaling = Global::voxelScaling();
  trans = VECDIVIDE(trans, voxelScaling);

  m_point += trans;

  m_prevPos = event->pos();

  emit updatePoint();
}

void
HitPointGrabber::mouseReleaseEvent(QMouseEvent* const event,
				   Camera* const camera)
{
  m_pressed = false;
}

void
HitPointGrabber::addToActivePool()
{
  if (!isInActivePool())
    HitPointGrabber::activeHitPointPool_.append(this);
}

void
HitPointGrabber::removeFromActivePool()
{
  if (isInActivePool())
    HitPointGrabber::activeHitPointPool_.removeAll(const_cast<HitPointGrabber*>(this));
}

void
HitPointGrabber::clearActivePool()
{  
  HitPointGrabber::activeHitPointPool_.clear();
}

bool
HitPointGrabber::isInActivePool()
{
  return HitPointGrabber::activeHitPointPool_.contains(const_cast<HitPointGrabber*>(this));
}
