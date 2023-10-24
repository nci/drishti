#include "global.h"
#include "cropgrabber.h"
#include "staticfunctions.h"

CropGrabber::CropGrabber()
{
  m_lastX = m_lastY = -1;
  m_pressed = false;
  m_pointPressed = -1;
  m_moved = false;
}

CropGrabber::~CropGrabber() { removeFromMouseGrabberPool(); }

int CropGrabber::pointPressed() { return m_pointPressed; }

void
CropGrabber::mousePosition(int& x, int& y)
{
  x = m_lastX;
  y = m_lastY;
}

void
CropGrabber::checkIfGrabsMouse(int x, int y,
			       const Camera* const camera)
{
  if (m_pressed)
    {
      // mouse button pressed so keep grabbing
      setGrabsMouse(true);
      return;
    }

  int sz = 20;
  if (tube()) sz = 40;

  Vec voxelScaling = Vec(1,1,1);
  //Vec voxelScaling = Global::voxelScaling();
  QList<Vec> pts = points();

  m_lastX = x;
  m_lastY = y;
  Vec v = Vec(x, y, 0);
  Vec pvs = VECPRODUCT(pts[0], voxelScaling);
  Vec pos0 = camera->projectedCoordinatesOf(pvs);
  Vec p0 = Vec(pos0.x, pos0.y, 0);

  for(int i=1; i<pts.count(); i++)
    {
      pvs = VECPRODUCT(pts[i], voxelScaling);
      Vec pos1 = camera->projectedCoordinatesOf(pvs);
      Vec p1 = Vec(pos1.x, pos1.y, 0);
      Vec pvec = p1-p0;
      Vec pv = v-p0;
      pv.projectOnAxis(pvec);
      float pvdotpvec = pv*pvec.unit();
      if (pvdotpvec >= -sz && pvdotpvec <= pvec.norm()+sz)
	{
	  pv += p0;
	  if ((pv-v).norm() < sz)
	    {
	      setGrabsMouse(true);
	      return;
	    }
	}
      pos0 = pos1;
      p0 = p1;
    }

  {
    float radX0 = getRadX(0);
    float radX1 = getRadX(1);
    float radY0 = getRadY(0);
    float radY1 = getRadY(1);
    float rx = (radX0+radX1)*0.5f;
    float ry = (radY0+radY1)*0.5f;

    Vec p0 = VECPRODUCT(pts[0], voxelScaling);
    Vec p1 = VECPRODUCT(pts[1], voxelScaling);
    
    Vec ca = (0.4f*p0 + 0.6f*p1);
    Vec cb = (0.6f*p0 + 0.4f*p1);
    Vec cx0 = (0.5f*p0 + 0.5f*p1) + rx*m_xaxis;
    Vec cy0 = (0.5f*p0 + 0.5f*p1) + ry*m_yaxis;
    Vec cx1 = (0.5f*p0 + 0.5f*p1) - rx*m_xaxis;
    Vec cy1 = (0.5f*p0 + 0.5f*p1) - ry*m_yaxis;
    Vec cax0 = ca + 0.2f*rx*m_xaxis;
    Vec cay0 = ca + 0.2f*ry*m_yaxis;
    Vec cax1 = ca - 0.2f*rx*m_xaxis;
    Vec cay1 = ca - 0.2f*ry*m_yaxis;

    p1 = camera->projectedCoordinatesOf(p1);    p1 = Vec(p1.x, p1.y, 0);
    ca = camera->projectedCoordinatesOf(ca);    ca = Vec(ca.x, ca.y, 0);
    cb = camera->projectedCoordinatesOf(cb);    cb = Vec(cb.x, cb.y, 0);

    cx0 = camera->projectedCoordinatesOf(cx0);  cx0 = Vec(cx0.x, cx0.y, 0);
    cy0 = camera->projectedCoordinatesOf(cy0);  cy0 = Vec(cy0.x, cy0.y, 0);
    cx1 = camera->projectedCoordinatesOf(cx1);  cx1 = Vec(cx1.x, cx1.y, 0);
    cy1 = camera->projectedCoordinatesOf(cy1);  cy1 = Vec(cy1.x, cy1.y, 0);

    cax0 = camera->projectedCoordinatesOf(cax0); cax0= Vec(cax0.x,cax0.y,0);
    cay0 = camera->projectedCoordinatesOf(cay0); cay0= Vec(cay0.x,cay0.y,0);
    cax1 = camera->projectedCoordinatesOf(cax1); cax1= Vec(cax1.x,cax1.y,0);
    cay1 = camera->projectedCoordinatesOf(cay1); cay1= Vec(cay1.x,cay1.y,0);
    Vec pp = Vec(x, y, 0);
    
    if (StaticFunctions::inTriangle(ca,  cb, cx0, pp) ||
	StaticFunctions::inTriangle(ca,  cb, cy0, pp) ||
	StaticFunctions::inTriangle(ca,  cb, cx1, pp) ||
	StaticFunctions::inTriangle(ca,  cb, cy1, pp) ||
	StaticFunctions::inTriangle(cax0,p1,cax1, pp) ||
	StaticFunctions::inTriangle(cay0,p1,cay1, pp))
      {
	setGrabsMouse(true);
	return;
      }
    }

  setGrabsMouse(false);
}

void
CropGrabber::mousePressEvent(QMouseEvent* const event,
				 Camera* const camera)
{
  m_pointPressed = -1;
  m_pressed = true;
  m_prevPos = event->pos();
  m_moved = false;

  Vec voxelScaling = Vec(1,1,1);
  //Vec voxelScaling = Global::voxelScaling();
  QList<Vec> pts = points();
  for(int i=0; i<pts.count(); i++)
    {
      Vec v = VECPRODUCT(pts[i], voxelScaling);
      Vec pos = camera->projectedCoordinatesOf(v);
      QPoint hp(pos.x, pos.y);
      if ((hp-m_prevPos).manhattanLength() < 15)
	{
	  m_pointPressed = i;
	  setMoveAxis(MoveAll);
	  break;
	}
    }

  if (m_pointPressed == -1)
    {
      float radX0 = getRadX(0);
      float radX1 = getRadX(1);
      float radY0 = getRadY(0);
      float radY1 = getRadY(1);
      float rx = (radX0+radX1)*0.5f;
      float ry = (radY0+radY1)*0.5f;

      Vec p0 = VECPRODUCT(pts[0], voxelScaling);
      Vec p1 = VECPRODUCT(pts[1], voxelScaling);

      Vec ca = (0.4f*p0 + 0.6f*p1);
      Vec cb = (0.6f*p0 + 0.4f*p1);
      Vec cx0 = (0.5f*p0 + 0.5f*p1) + rx*m_xaxis;
      Vec cy0 = (0.5f*p0 + 0.5f*p1) + ry*m_yaxis;
      Vec cx1 = (0.5f*p0 + 0.5f*p1) - rx*m_xaxis;
      Vec cy1 = (0.5f*p0 + 0.5f*p1) - ry*m_yaxis;
      Vec cax0 = ca + 0.2f*rx*m_xaxis;
      Vec cay0 = ca + 0.2f*ry*m_yaxis;
      Vec cax1 = ca - 0.2f*rx*m_xaxis;
      Vec cay1 = ca - 0.2f*ry*m_yaxis;
    
      p1 = camera->projectedCoordinatesOf(p1);    p1 = Vec(p1.x, p1.y, 0);
      ca = camera->projectedCoordinatesOf(ca);    ca = Vec(ca.x, ca.y, 0);
      cb = camera->projectedCoordinatesOf(cb);    cb = Vec(cb.x, cb.y, 0);

      cx0 = camera->projectedCoordinatesOf(cx0);  cx0 = Vec(cx0.x, cx0.y, 0);
      cy0 = camera->projectedCoordinatesOf(cy0);  cy0 = Vec(cy0.x, cy0.y, 0);
      cx1 = camera->projectedCoordinatesOf(cx1);  cx1 = Vec(cx1.x, cx1.y, 0);
      cy1 = camera->projectedCoordinatesOf(cy1);  cy1 = Vec(cy1.x, cy1.y, 0);

      cax0 = camera->projectedCoordinatesOf(cax0); cax0= Vec(cax0.x,cax0.y,0);
      cay0 = camera->projectedCoordinatesOf(cay0); cay0= Vec(cay0.x,cay0.y,0);
      cax1 = camera->projectedCoordinatesOf(cax1); cax1= Vec(cax1.x,cax1.y,0);
      cay1 = camera->projectedCoordinatesOf(cay1); cay1= Vec(cay1.x,cay1.y,0);
      Vec pp = Vec(m_prevPos.x(), m_prevPos.y(), 0);
  
  
      if (StaticFunctions::inTriangle(ca, cb, cx0, pp))
	setMoveAxis(MoveX0);
      else if (StaticFunctions::inTriangle(ca, cb, cx1, pp))
	setMoveAxis(MoveX1);
      else if (StaticFunctions::inTriangle(ca, cb, cy0, pp))
	setMoveAxis(MoveY0);
      else if (StaticFunctions::inTriangle(ca, cb, cy1, pp))
	setMoveAxis(MoveY1);
      else if (StaticFunctions::inTriangle(cax0,p1,cax1, pp) ||
	       StaticFunctions::inTriangle(cay0,p1,cay1, pp))
	setMoveAxis(MoveZ);
      else
	setMoveAxis(MoveAll);
    }

  emit selectForEditing(event->button(), m_pointPressed);
}

void
CropGrabber::mouseMoveEvent(QMouseEvent* const event,
			    Camera* const camera)
{
  //if (!m_pressed || getPointPressed() == -1)
  if (!m_pressed)
    return;
  
  QPoint delta = event->pos() - m_prevPos;

  if (event->buttons() != Qt::LeftButton)
    {
      Vec trans(delta.x(), -delta.y(), 0.0f);
      
      // Scale to fit the screen mouse displacement
      trans *= 2.0 * tan(camera->fieldOfView()/2.0) *
	             fabs((camera->frame()->coordinatesOf(Vec(0,0,0))).z) /
	             camera->screenHeight();
      // Transform to world coordinate system.
      trans = camera->frame()->orientation().rotate(trans);

      Vec voxelScaling = Vec(1,1,1);
      //Vec voxelScaling = Global::voxelScaling();
      trans = VECDIVIDE(trans, voxelScaling);

      if (event->modifiers() & Qt::ControlModifier ||
	  event->modifiers() & Qt::MetaModifier)
	{
	  if (moveAxis() < MoveY0)
	    {
	      float vx = trans*m_xaxis;
	      if (moveAxis() == MoveX1) vx = -vx;
	      if (getPointPressed() != -1)
		{
		  int ip = getPointPressed();
		  setRadX(ip, qMax(1.0f, getRadX(ip)+vx), false);
		  m_moved = true;
		}
	      else
		{
		  setRadX(0, qMax(1.0f, getRadX(0)+vx), false);
		  setRadX(1, qMax(1.0f, getRadX(1)+vx), false);
		  m_moved = true;
		}
	    }
	  else if (moveAxis() < MoveZ)
	    {
	      float vy = trans*m_yaxis;
	      if (moveAxis() == MoveY1) vy = -vy;
	      if (getPointPressed() != -1)
		{
		  int ip = getPointPressed();
		  setRadY(ip, qMax(1.0f, getRadY(ip)+vy), false);
		  m_moved = true;
		}
	      else
		{
		  setRadY(0, qMax(1.0f, getRadY(0)+vy), false);
		  setRadY(1, qMax(1.0f, getRadY(1)+vy), false);
		  m_moved = true;
		}
	    }
	  else if (moveAxis() == MoveZ)
	    {
	      float vz = trans*m_tang;
	      shiftPoints(vz);
	      m_moved = true;
	    }
	  else
	    {
	      float vx = trans*m_xaxis;
	      float vy = trans*m_yaxis;
	      if (getPointPressed() != -1)
		{
		  int ip = getPointPressed();
		  if (fabs(vx) > fabs(vy))
		    setRadX(ip, qMax(1.0f, getRadX(ip)+vx), false);
		  else
		    setRadY(ip, qMax(1.0f, getRadY(ip)+vy), false);
		  m_moved = true;
		}
	    }
	}
      else
	{
	  if (moveAxis() < MoveY0)
	    {
	      float vx = trans*m_xaxis;
	      trans = vx*m_xaxis;
	    }
	  else if (moveAxis() < MoveZ)
	    {
	      float vy = trans*m_yaxis;
	      trans = vy*m_yaxis;
	    }
	  else if (moveAxis() == MoveZ)
	    {
	      float vz = trans*m_tang;
	      trans = vz*m_tang;
	    }

	  if (getPointPressed() != -1)
	    {
	      m_pointPressed = getPointPressed();
	      Vec pt = getPoint(m_pointPressed);
	      pt += trans;
	      setPoint(m_pointPressed, pt);
	    }
	  else
	    translate(trans);
	  m_moved = true;
	}
    }
  else
    {
      Vec axis;
      if (moveAxis() < MoveY0) axis = m_xaxis;
      else if (moveAxis() < MoveZ) axis = m_yaxis;
      else if (moveAxis() == MoveZ) axis = m_tang;
      
      QList<Vec> pts = points();
      Vec voxelScaling = Vec(1,1,1);
      //Vec voxelScaling = Global::voxelScaling();
      Vec p0 = VECPRODUCT(pts[0], voxelScaling);
      Vec p1 = VECPRODUCT(pts[1], voxelScaling);
      Vec ca = (p0+p1)*0.5f;

      Vec trans(delta.x(), -delta.y(), 0.0f);

      ca = camera->projectedCoordinatesOf(ca); ca = Vec(ca.x, ca.y, 0);
      Vec c0 = ca + 1000*axis;
      c0 = camera->projectedCoordinatesOf(c0); c0 = Vec(c0.x, c0.y, 0);
      Vec perp = c0-ca;
      perp.normalize();
      perp = Vec(perp.y, -perp.x, 0);

      float angle = perp * trans;
      if (moveAxis() != MoveZ)
	rotate(axis, angle);
      else
	setAngle(getAngle() + angle);

      m_moved = true;
    }

  m_prevPos = event->pos();
}

void
CropGrabber::mouseReleaseEvent(QMouseEvent* const event,
			       Camera* const camera)
{
  if (m_moved)
    updateUndo();

  m_moved = false;
  m_pressed = false;
  m_pointPressed = -1;

  emit deselectForEditing();
}
