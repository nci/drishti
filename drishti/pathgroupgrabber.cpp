#include "global.h"
#include "pathgroupgrabber.h"
#include "staticfunctions.h"

PathGroupGrabber::PathGroupGrabber()
{
  m_lastX = m_lastY = -1;
  m_pressed = false;
  m_pointPressed = -1;
  m_moveAxis = MoveAll;
}

PathGroupGrabber::~PathGroupGrabber() { removeFromMouseGrabberPool(); }

int PathGroupGrabber::moveAxis() { return m_moveAxis; }
void PathGroupGrabber::setMoveAxis(int type) { m_moveAxis = type; }

int PathGroupGrabber::pointPressed() { return m_pointPressed; }

void
PathGroupGrabber::mousePosition(int& x, int& y)
{
  x = m_lastX;
  y = m_lastY;
}

void
PathGroupGrabber::checkIfGrabsMouse(int x, int y,
				    const Camera* const camera)
{
  if (m_pressed)
    {
      // mouse button pressed so keep grabbing
      setGrabsMouse(true);
      return;
    }

  setGrabsIndex(-1);

  int sz = 20;
  if (tube()) sz = 30;

  QList<Vec> pts = pathPoints();
  QList<int> pathIndex = pathPointIndices();
  QVector<bool> validPath = valid();

  //--------
  {
    setCaptionGrabbed(false);
    QList<Vec> isize;
    isize = imageSizes();
    for(int ci=0; ci<pathIndex.count()-1; ci++)
    {
      if (validPath[ci])
	{
	  int sidx = pathIndex[ci];
	  int eidx = pathIndex[ci+1];
	  Vec pp0 = camera->projectedCoordinatesOf(pts[sidx]);
	  Vec pp1 = camera->projectedCoordinatesOf(pts[eidx-1]);
	  int twd = isize[ci].x;
	  int tht = isize[ci].y;
	  int px = pp0.x + 10;
	  if (pp0.x < pp1.x)
	    px = pp0.x - twd - 10;
	  if (x > px && x < px+twd &&
	      y > pp0.y-tht/2 && y < pp0.y+tht/2)
	    {
	      setCaptionGrabbed(true);
	      setGrabsMouse(true);
	      return;
	    }
	}
    }
  }
  //--------

  m_lastX = x;
  m_lastY = y;
  for(int ci=0; ci<pathIndex.count()-1; ci++)
    {
      if (validPath[ci])
	{
	  int sidx = pathIndex[ci];
	  int eidx = pathIndex[ci+1];
	  for(int i=sidx+1; i<eidx; i++)
	    {
	      Vec pos0 = camera->projectedCoordinatesOf(pts[i-1]);
	      Vec pos1 = camera->projectedCoordinatesOf(pts[i]);
	      int p0x = pos0.x;
	      int p0y = pos0.y;
	      int p1x = pos1.x;
	      int p1y = pos1.y;
	      int pvx = p1x-p0x;
	      int pvy = p1y-p0y;
	      int pmx = x-p0x;
	      int pmy = y-p0y;
	      int t = pvx*pmx + pvy*pmy;
	      int pd =  pvx*pvx+pvy*pvy;
	      if (t > 0 && t < pd)
		{
		  int sqd = (pmx*pmx + pmy*pmy) - (float)(t*t)/pd;
		  if (sqd < sz)
		    {
		      setGrabsMouse(true);
		      setGrabsIndex(ci);
		      return;
		    }
		}
	    }
	}
    }
  setGrabsMouse(false);
}

void
PathGroupGrabber::mousePressEvent(QMouseEvent* const event,
				  Camera* const camera)
{
  m_pointPressed = -1;
  m_pressed = true;
  m_prevPos = event->pos();

  Vec voxelScaling = Global::voxelScaling();
  int nseg = segments();
  QList<Vec> pts = points();
  for(int i=0; i<pts.count(); i++)
    {
      Vec v = VECPRODUCT((pts[i]), voxelScaling);
      Vec pos = camera->projectedCoordinatesOf(v);
      QPoint hp(pos.x, pos.y);
      if ((hp-m_prevPos).manhattanLength() < 20)
	{
	  m_pointPressed = i;
	  break;
	}
    }

  int pt0 = -1;
  if (m_pointPressed == -1 && allowEditing())
    {
      QList<Vec> pts = pathPoints();  
      for(int i=1; i<pts.count(); i++)
	{
	  Vec pos0 = camera->projectedCoordinatesOf(pts[i-1]);
	  Vec pos1 = camera->projectedCoordinatesOf(pts[i]);
	  int p0x = pos0.x;
	  int p0y = pos0.y;
	  int p1x = pos1.x;
	  int p1y = pos1.y;
	  int pvx = p1x-p0x;
	  int pvy = p1y-p0y;
	  int pmx = m_prevPos.x()-p0x;
	  int pmy = m_prevPos.y()-p0y;
	  int t = pvx*pmx + pvy*pmy;
	  int pd =  pvx*pvx+pvy*pvy;
	  if (t > 0 && t < pd)
	    {
	      int sqd = (pmx*pmx + pmy*pmy) - (float)(t*t)/pd;
	      if (sqd < 10)
		{
		  pt0 = i;
		  break;
		}
	    }
	}
      if (pt0 > -1)
	{
	  if (pts.count() > 2 && nseg > 1)
	    {
	      pt0 /= nseg;
	      pt0 += 1;
	    }
	  if (arrowDirection() == false)
	    pt0 = points().count()-1-pt0;
	  else
	    pt0 = pt0 - 1;
	}
    }

  emit selectForEditing(event->button(), pt0);
}

void
PathGroupGrabber::mouseMoveEvent(QMouseEvent* const event,
				 Camera* const camera)
{
  if (!m_pressed ||
      getPointPressed() == -1 ||
      !allowEditing())
    return;
  
  m_pointPressed = getPointPressed();

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

  Vec pt = getPoint(m_pointPressed);
  pt += trans;
  setPoint(m_pointPressed, pt);

  m_prevPos = event->pos();
}

void
PathGroupGrabber::mouseReleaseEvent(QMouseEvent* const event,
				    Camera* const camera)
{
  updateUndo();

  m_pressed = false;
  m_pointPressed = -1;

  emit deselectForEditing();
}
