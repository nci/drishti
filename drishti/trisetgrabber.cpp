#include "global.h"
#include "staticfunctions.h"
#include "trisetgrabber.h"
#include "matrix.h"

TrisetGrabber::TrisetGrabber()
{
  m_pressed = false;
  m_moveAxis = MoveAll;

  m_labelSelected = -1;
}

TrisetGrabber::~TrisetGrabber() { removeFromMouseGrabberPool(); }

bool TrisetGrabber::mousePressed() { return m_pressed; }

int TrisetGrabber::moveAxis() { return m_moveAxis; }
void TrisetGrabber::setMoveAxis(int type) { m_moveAxis = type; }

void
TrisetGrabber::setMouseGrab(bool f)
{
  setGrabsMouse(f);
  if (!f) m_pressed = false;
}


bool
TrisetGrabber::checkLabel(int x, int y,
			  const Camera* const camera)
{
  QList<Vec> cpD = captionOffsets();
  QList<Vec> cpS = captionSizes();
  for(int i=0; i<cpS.count(); i++)
    cpS[i] /= 2;

  m_screenWidth = camera->screenWidth();
  m_screenHeight = camera->screenHeight();
  for(int i=0; i<cpD.count(); i++)
    {
      int rpx, rpy;
      if (qAbs(cpD[i].x) < 1.1 && qAbs(cpD[i].y) < 1.1)
	{      
	  rpx = qMax(0.0f, (float)cpD[i].x*m_screenWidth+(float)cpS[i].x );
	  rpy = qMin((float)m_screenHeight, (float)cpD[i].y*m_screenHeight-(float)cpS[i].y);
	}
      else
	{
	  Vec pos = camera->projectedCoordinatesOf(captionPosition(i)+centroid()+position());
	  int cx = pos.x;
	  int cy = pos.y;
	  
	  rpx = cx + cpD[i].x + cpS[i].x;
	  rpy = cy + cpD[i].y - cpS[i].y;
	}

      // choose the first label that meets this criterion
      if (qAbs(x-rpx)<cpS[i].x && qAbs(y-rpy)<cpS[i].y)
	{
	  m_labelSelected = i;
	  return true;
	}
    }  

  return false;
}

Vec
TrisetGrabber::checkForMouseHover(int x, int y,
				  const Camera* const camera)
{
  m_labelSelected = -1;
  if (checkLabel(x,y, camera))
    {
      return Vec(1,1,100);
    }
		     

  float chkd1 = 100;
  float chkd2 = 100;
  int oh = camera->screenHeight();
  Vec cpos = camera->position();
  Vec vd = camera->viewDirection()/camera->sceneRadius();
  float mindst = 1000;
  float dx = -1;
  float dy = -1;
  QPoint hp1(x, y);
  for(int p=0; p<m_samplePoints.count(); p++)
    {
      int chkd = (p == 0 ? chkd1 : chkd2);
      Vec pos = camera->projectedCoordinatesOf(position()+m_samplePoints[p]);
      QPoint hp0(pos.x, pos.y);
      int dst = (hp0-hp1).manhattanLength();
      if (dst < chkd && dst < mindst)
	{
	  mindst = dst;
	  dx = dst;
	  dy = (position()+m_samplePoints[p] - cpos)*vd;
	}
    }
  
  return Vec(dx, dy, 100);
}

//Vec
//TrisetGrabber::checkForMouseHover(int x, int y,
//				  const Camera* const camera)
//{
//  Vec pos = camera->projectedCoordinatesOf(centroid()+position());
//
//  Vec bmin, bmax;
//  enclosingBox(bmin, bmax);
//
//  Vec pmin = camera->projectedCoordinatesOf(bmin);
//  Vec pmax = camera->projectedCoordinatesOf(bmax);
//  float chkd = qMin(qAbs(pmin.x-pmax.x), qAbs(pmin.y-pmax.y));
//  chkd = qMin(chkd, 100.0f);
//    
//  QPoint hp0(pos.x, pos.y);
//  QPoint hp1(x, y);
//  int dst = (hp0-hp1).manhattanLength();
//  if (dst < chkd)
//    {
//      Vec cpos = camera->position();
//      float d = (centroid()+position()-cpos)*camera->viewDirection();
//      d /= camera->sceneRadius();
//      Vec V = camera->viewDirection();
//      return Vec(dst, d, chkd);
//    }
//
//  return Vec(-1, -1, chkd);
//}

void
TrisetGrabber::checkIfGrabsMouse(int x, int y,
				 const Camera* const camera)
{
////  // don't want to grab mouse
////  setGrabsMouse(false);
//
//  Vec pos = camera->projectedCoordinatesOf(centroid()+position());
//  QPoint hp0(pos.x, pos.y);
//  QPoint hp1(x, y);
//  if ((hp0-hp1).manhattanLength() < 100)
//    setGrabsMouse(true);
//  else
//    {
//      setGrabsMouse(false);
//      m_pressed = false;
//    }
}

void
TrisetGrabber::mousePressEvent(QMouseEvent* const event,
			       Camera* const camera)
{
  m_pressed = true;
  m_prevPos = event->pos();
}

void
TrisetGrabber::moveLabel(QPoint delta)
{
  QList<Vec> cpD = captionOffsets();
  QList<Vec> cpS = captionSizes();
  float rpx = cpD[m_labelSelected].x;
  float rpy = cpD[m_labelSelected].y;
  if (qAbs(rpx) < 1.1 && qAbs(rpy) < 1.1)
    {      
      rpx += (float)delta.x()/m_screenWidth;
      rpy += (float)delta.y()/m_screenHeight;
      rpx = qBound(0.0f, rpx, 1.0f-(float)cpS[m_labelSelected].x/m_screenWidth);
      rpy = qBound((float)cpS[m_labelSelected].y/m_screenHeight, rpy, 1.0f);
    }
  else
    {
      rpx += delta.x();
      rpy += delta.y();      
    }

  setCaptionOffset(m_labelSelected, rpx, rpy);
}

void
TrisetGrabber::mouseMoveEvent(QMouseEvent* const event,
			       Camera* const camera)
{
  if (!m_pressed)
    return;
  

  QPoint delta = event->pos() - m_prevPos;

  if (m_labelSelected > -1)
    {
      moveLabel(delta);
      m_prevPos = event->pos();
      return;
    }
  
  if (event->buttons() == Qt::LeftButton &&
      event->modifiers() == Qt::NoModifier)
    {
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
    }
  else if (event->buttons() == Qt::LeftButton &&
	   event->modifiers() & Qt::ControlModifier)
    {
      if (moveAxis() == MoveAll)
	{
	  Vec trans = camera->projectedCoordinatesOf(centroid()+position());

	  Quaternion q = StaticFunctions::deformedBallQuaternion(event->x(), event->y(),
								 trans.x, trans.y,
								 m_prevPos.x(), m_prevPos.y(),
								 camera);
	  Vec axis;
	  qreal angle;
	  q.getAxisAngle(axis, angle);
	  
	  if (event->modifiers() &= Qt::ShiftModifier)
	    {
	      if (axis*camera->viewDirection() < 0.0)
		angle = -angle;
	      axis = camera->viewDirection();
	    }

	  axis = Matrix::rotateVec(m_localXform, axis);

	  Quaternion rot = Quaternion(axis, angle);

	  rotate(rot);
	}
      else
	{
	  Vec axis;
	  if (moveAxis() < MoveY) axis = Vec(1,0,0);
	  else if (moveAxis() < MoveZ) axis = Vec(0,1,0);
	  else axis = Vec(0,0,1);
	  
	  Vec voxelScaling = Global::voxelScaling();
	  Vec pos = VECPRODUCT(position(), voxelScaling);
	  pos = Matrix::xformVec(m_localXform, pos);
	  
	  float r = 1.0;
	  Vec trans(delta.x(), delta.y(), 0.0f);
	  
	  Vec p0 = camera->projectedCoordinatesOf(pos); p0 = Vec(p0.x, p0.y, 0);
	  Vec c0 = pos + r*axis;
	  c0 = camera->projectedCoordinatesOf(c0); c0 = Vec(c0.x, c0.y, 0);
	  Vec perp = c0-p0;
	  perp = Vec(-perp.y, perp.x, 0);
	  perp.normalize();
	  
	  float angle = perp * trans;
	  rotate(axis, angle);
	}
    }
  
  m_prevPos = event->pos();
}

void
TrisetGrabber::mouseReleaseEvent(QMouseEvent* const event,
				 Camera* const camera)
{
  m_pressed = false;
}
