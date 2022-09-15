#include "global.h"
#include "staticfunctions.h"
#include "trisetgrabber.h"
#include "matrix.h"

TrisetGrabber::TrisetGrabber()
{
  m_pressed = false;
  m_moveAxis = MoveAll;
  //m_allowMove = false;

  m_rotationMode = false;
  m_grabMode = false;

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

void
TrisetGrabber::setRotationMode(bool f)
{
  m_rotationMode = f;
}
void
TrisetGrabber::setGrabMode(bool f)
{
  m_grabMode = f;
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
		     
  if (!m_grabMode)
    {
      Vec bmin, bmax, bcen;
      Vec axisX, axisY, axisZ;
      tenclosingBox(bmin, bmax);			    
      getAxes(axisX, axisY, axisZ);
      bcen = (bmax+bmin)*0.5;
      
      Vec va[6];
      va[0] = camera->projectedCoordinatesOf(bcen + axisX);
      va[1] = camera->projectedCoordinatesOf(bcen - axisX);
      va[2] = camera->projectedCoordinatesOf(bcen + axisY);
      va[3] = camera->projectedCoordinatesOf(bcen - axisY);
      va[4] = camera->projectedCoordinatesOf(bcen + axisZ);
      va[5] = camera->projectedCoordinatesOf(bcen - axisZ);
      
      float dmin = 10000000;
      int daxis = -1;
      for(int i=0; i<3; i++)
	{
	  QVector2D h0 = QVector2D(va[2*i].x, va[2*i].y);
	  QVector2D h1 = QVector2D(va[2*i+1].x, va[2*i+1].y);
	  QVector2D hu = (h1-h0).normalized();
	  float hlen = h1.distanceToPoint(h0);
	  
	  float d = QVector2D(x,y).distanceToLine(h0, hu);
	  float d0 = QVector2D(x,y).distanceToPoint(h0);
	  float d1 = QVector2D(x,y).distanceToPoint(h1);
	  if (d < 10 &&
	      d < dmin &&
	      d0< hlen &&
	      d1< hlen)
	    {
	      dmin = d;
	      daxis = i;
	    }
	}
      
      m_moveAxis = MoveAll;
      if (daxis == 0) m_moveAxis = MoveX;
      if (daxis == 1) m_moveAxis = MoveY;
      if (daxis == 2) m_moveAxis = MoveZ;
      
      if (daxis == -1)
	{
	  Vec c = camera->projectedCoordinatesOf(bcen);
	  if (QVector2D(x,y).distanceToPoint(QVector2D(c.x, c.y)) < 50)
	    return Vec(1,1,100);
	  else
	    return Vec(0,0,100);
	}
      
      return Vec(1,1,100);
    }
  else
    {  
      Vec bmin, bmax, bcen;
      Vec axisX, axisY, axisZ;
      tenclosingBox(bmin, bmax);			    
      bcen = (bmax+bmin)*0.5;
      Vec c = camera->projectedCoordinatesOf(bcen);
      if (QVector2D(x,y).distanceToPoint(QVector2D(c.x, c.y)) < 15)
	return Vec(1,1,100);
      else
	return Vec(0,0,100);
    }
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

  if (m_grabMode)
    emit meshGrabbed();
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
  if (!m_pressed || m_grabMode)
    return;  

  QPoint delta = event->pos() - m_prevPos;


  if (m_labelSelected > -1)
    {
      moveLabel(delta);
      m_prevPos = event->pos();
      return;
    }

  if (event->buttons() == Qt::RightButton)
    {
      Vec trans(delta.x(), -delta.y(), 0.0f);
      
      // Scale to fit the screen mouse displacement
      trans *= 2.0 * tan(camera->fieldOfView()/2.0) *
	fabs((camera->frame()->coordinatesOf(Vec(0,0,0))).z) /
	camera->screenHeight();
      // Transform to world coordinate system.
      trans = camera->frame()->orientation().rotate(trans);
      
      Vec axisX, axisY, axisZ;
      if (m_moveAxis != MoveAll)
	{
	  getAxes(axisX, axisY, axisZ);
	  axisX.normalize();
	  axisY.normalize();
	  axisZ.normalize();
	  
	  if (m_moveAxis == MoveX)
	    trans = trans.x * axisX;
	  else if (m_moveAxis == MoveY)
	    trans = trans.y * axisY;
	  else if (m_moveAxis == MoveZ)
	    trans = trans.z * axisZ;
	}
      
      Vec pos = position() + trans;
      setPosition(pos);
      emit posChanged();
    }

  if (event->buttons() == Qt::LeftButton)
    {
      if (moveAxis() == MoveAll)
	{
	  //Vec trans = camera->projectedCoordinatesOf(centroid()+position());
	  Vec trans = camera->projectedCoordinatesOf(tcentroid());
	  
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

  //if (!m_grabMode)
    emit updateParam();
}
