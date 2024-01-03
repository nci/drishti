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
      //bcen = (bmax+bmin)*0.5;
      bcen = centroid() + position();
      
      
      Vec va[6];
      Vec pcen = camera->projectedCoordinatesOf(bcen);
      Vec vX = camera->unprojectedCoordinatesOf(pcen + Vec(100,0,0));
      float rad = (vX - bcen).norm();

      Vec sX = axisX.unit();
      Vec sY = axisY.unit();
      Vec sZ = axisZ.unit();
      va[0] = camera->projectedCoordinatesOf(bcen + rad*sX);
      va[1] = camera->projectedCoordinatesOf(bcen + rad*sY);
      va[2] = camera->projectedCoordinatesOf(bcen + rad*sZ);
      Vec ax = va[0] - pcen;
      Vec ay = va[1] - pcen;
      Vec az = va[2] - pcen;
      va[3] = pcen + 0.55*ax + 0.55*ay;
      va[4] = pcen + 0.55*ax + 0.55*az;
      va[5] = pcen + 0.55*ay + 0.55*az;



      m_flipRotAxis = false;      
      

      // check for translations
      float dmin = 10000000;
      int daxis = -1;
      QVector2D h0 = QVector2D(pcen.x, pcen.y);
      // find whether pointer on x/y/z axes
      for(int i=0; i<3; i++)
	{
	  QVector2D h1 = QVector2D(va[i].x, va[i].y);
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
      // find whether pointer on xy/xz/yz faces
      for(int i=3; i<6; i++)
	{
	  QVector2D h1 = QVector2D(va[i].x, va[i].y);
	  float d1 = QVector2D(x,y).distanceToPoint(h1);
	  if (d1 < 20)
	    {
	      dmin = d1;
	      daxis = i;
	    }	  
	}

      m_interactive_axis = 0;
      
      m_moveAxis = MoveAll;
      if (daxis == 0) m_moveAxis = MoveX;
      if (daxis == 1) m_moveAxis = MoveY;
      if (daxis == 2) m_moveAxis = MoveZ;
      if (daxis == 3) m_moveAxis = MoveXY;
      if (daxis == 4) m_moveAxis = MoveXZ;
      if (daxis == 5) m_moveAxis = MoveYZ;

      m_rotationMode = false;	  
      m_interactive_mode = 1;
      if (m_moveAxis != MoveAll) m_interactive_axis = daxis+1;
      
      // check for rotations
      int ncheks = 72;
      if (daxis == -1)
	{
	  Vec vX = camera->unprojectedCoordinatesOf(pcen + Vec(100,0,0));
	  float rad = (vX - bcen).norm();

	  Vec v = rad*sY;
	  Quaternion q(sX, qDegreesToRadians(5.0));
	  for(int i=0; i<ncheks; i++)
	    {
	      v = q.rotate(v);
	      Vec u = camera->projectedCoordinatesOf(bcen + v);
	      float d = QVector2D(x,y).distanceToPoint(QVector2D(u.x, u.y));
	      if (d < 10)
		{
		  m_rotationMode = true;		  
		  m_moveAxis = MoveX;
		  daxis = 0;
		  m_interactive_mode = 2;
		  m_interactive_axis = 1;

		  m_flipRotAxis = (pcen.z < va[0].z);
		  break;
		}
	    }

	  if (m_rotationMode == false)
	    {
	      Vec v = rad*sZ;
	      Quaternion q(sY, qDegreesToRadians(5.0));
	      for(int i=0; i<ncheks; i++)
		{
		  v = q.rotate(v);
		  Vec u = camera->projectedCoordinatesOf(bcen + v);
		  float d = QVector2D(x,y).distanceToPoint(QVector2D(u.x, u.y));
		  if (d < 10)
		    {
		      m_rotationMode = true;		  
		      m_moveAxis = MoveY;
		      daxis = 1;
		      m_interactive_mode = 2;
		      m_interactive_axis = 2;

		      m_flipRotAxis = (pcen.z < va[1].z);
		      break;
		    }
		}
	    }

	  if (m_rotationMode == false)
	    {
	      Vec v = rad*sX;
	      Quaternion q(sZ, qDegreesToRadians(5.0));
	      for(int i=0; i<ncheks; i++)
		{
		  v = q.rotate(v);
		  Vec u = camera->projectedCoordinatesOf(bcen + v);
		  float d = QVector2D(x,y).distanceToPoint(QVector2D(u.x, u.y));
		  if (d < 10)
		    {
		      m_rotationMode = true;		  
		      m_moveAxis = MoveZ;
		      daxis = 2;
		      m_interactive_mode = 2;
		      m_interactive_axis = 3;

		      m_flipRotAxis = (pcen.z < va[2].z);
		      break;
		    }
		}
	    }
	}
      
      

      
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
      //if (QVector2D(x,y).distanceToPoint(QVector2D(c.x, c.y)) < 15)
      if (qAbs(x-c.x) < 15 && qAbs(y-c.y-7) < 25)
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

  
  
  if (event->buttons() == Qt::LeftButton)
    {
      if (!m_rotationMode)
	{
	  Vec trans(delta.x(), -delta.y(), 0.0f);
	  Vec bcen = centroid() + position();
	  
	  // Scale to fit the screen mouse displacement
//	  trans *= 2.0 * tan(camera->fieldOfView()/2.0) *
//	    fabs((camera->frame()->coordinatesOf(Vec(0,0,0))).z) /
//	    camera->screenHeight();
	  trans *= 2.0 * tan(camera->fieldOfView()/2.0) *
	    fabs((camera->frame()->coordinatesOf(bcen)).z) /
	    camera->screenHeight();
	  // Transform to world coordinate system.
	  trans = camera->frame()->orientation().rotate(trans);
	  trans = Matrix::rotateVec(m_localXform, trans);
	  
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
	      else if (m_moveAxis == MoveXY)
		trans = trans.x*axisX + trans.y*axisY;
	      else if (m_moveAxis == MoveXZ)
		trans = trans.x*axisX + trans.z*axisZ;
	      else if (m_moveAxis == MoveYZ)
		trans = trans.y*axisY + trans.z*axisZ;
	    }
	  
	  Vec pos = position() + trans;
	  setPosition(pos);
	  emit posChanged();
	}
      else 
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
	      Vec axisX, axisY, axisZ;
	      getAxes(axisX, axisY, axisZ);
	      Vec bcen = centroid() + position();
	      Vec p0 = camera->projectedCoordinatesOf(bcen);
	      Vec vX = camera->unprojectedCoordinatesOf(p0 + Vec(100,0,0));
	      float rad = (vX - bcen).norm();
	      Vec c0;
	      if (moveAxis() < MoveY)
		c0 = camera->projectedCoordinatesOf(bcen + rad*axisX.unit());
	      else if (moveAxis() < MoveZ)
		c0 = camera->projectedCoordinatesOf(bcen + rad*axisY.unit());
	      else
		c0 = camera->projectedCoordinatesOf(bcen + rad*axisZ.unit());	      

	      Vec projAxis = c0-p0;
	      Vec perpAxis = Vec(projAxis.y, -projAxis.x, 0);
	      perpAxis.normalize();
	      
	      Vec trans(delta.x(), delta.y(), 0.0f);
	      float angle = perpAxis * trans;
	      if (c0.z > p0.z)
		angle = -angle;
	      
	      QPoint ep = event->pos();
	      Vec ev = Vec(ep.x()-p0.x, ep.y()-p0.y, 0);
	      if (ev*projAxis >= 0)
		angle = -angle;
	      
	      Vec axis;
	      if (moveAxis() < MoveY) axis = Vec(1,0,0);
	      else if (moveAxis() < MoveZ) axis = Vec(0,1,0);
	      else axis = Vec(0,0,1);

	      rotate(axis, angle);
	    }
	}
    }
  
  m_prevPos = event->pos();
}

void
TrisetGrabber::mouseReleaseEvent(QMouseEvent* const event,
				 Camera* const camera)
{
  m_pressed = false;

  if (!m_grabMode)
    emit updateParam();
}
