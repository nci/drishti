#include "global.h"
#include "gilightobject.h"
#include "staticfunctions.h"
#include "volumeinformation.h"

//------------------------------------------------------------------
GiLightObjectUndo::GiLightObjectUndo() { clear(); }
GiLightObjectUndo::~GiLightObjectUndo() { clear(); }

void
GiLightObjectUndo::clear()
{
  m_points.clear();
  m_index = -1;
}

void
GiLightObjectUndo::clearTop()
{
  if (m_index == m_points.count()-1)
    return;

  while(m_index < m_points.count()-1)
    m_points.removeLast();
}

void
GiLightObjectUndo::append(QList<Vec> p)
{
  clearTop();
  m_points << p;
  m_index = m_points.count()-1;
}

void GiLightObjectUndo::redo() { m_index = qMin(m_index+1, m_points.count()-1); }
void GiLightObjectUndo::undo() { m_index = qMax(m_index-1, 0); }

QList<Vec>
GiLightObjectUndo::points()
{
  QList<Vec> p;

  if (m_index >= 0 && m_index < m_points.count())
    return m_points[m_index];

  return p;
}
//------------------------------------------------------------------

void GiLightObject::setLightChanged(bool b) { m_lightChanged = b; }
bool GiLightObject::lightChanged() { return m_lightChanged; }

GiLightObject
GiLightObject::get()
{
  GiLightObject po;
  po = *this;
  return po;
}

void
GiLightObject::set(const GiLightObject &po)
{
  m_pointPressed = -1;

  m_updateFlag = true; // for recomputation
  m_lightChanged = true; // for light recomputation

  m_showPointNumbers = po.m_showPointNumbers;
  m_showPoints = po.m_showPoints;
  m_segments = po.m_segments;
  m_color = po.m_color;
  m_opacity = po.m_opacity;
  m_lod = po.m_lod;
  m_smooth = po.m_smooth;
  m_points = po.m_points;
  m_allowInterpolate = po.m_allowInterpolate;
  m_doShadows = po.m_doShadows;
  m_lightType = po.m_lightType;
  m_rad = po.m_rad;
  m_decay = po.m_decay;
  m_angle = po.m_angle;
  m_show = po.m_show;

  computeTangents();

  m_undo.clear();
  m_undo.append(m_points);
}

GiLightObject&
GiLightObject::operator=(const GiLightObject &po)
{
  //m_pointPressed = -1;
  m_pointPressed = po.m_pointPressed;

  m_updateFlag = true; // for recomputation
  m_lightChanged = true; // for light recomputation

  m_showPointNumbers = po.m_showPointNumbers;
  m_showPoints = po.m_showPoints;
  m_segments = po.m_segments;
  m_color = po.m_color;
  m_opacity = po.m_opacity;
  m_lod = po.m_lod;
  m_smooth = po.m_smooth;
  m_points = po.m_points;
  m_allowInterpolate = po.m_allowInterpolate;
  m_doShadows = po.m_doShadows;
  m_lightType = po.m_lightType;
  m_rad = po.m_rad;
  m_decay = po.m_decay;
  m_angle = po.m_angle;
  m_show = po.m_show;

  computeTangents();

  m_undo.clear();
  m_undo.append(m_points);

  return *this;
}

GiLightObject::GiLightObject()
{
  m_undo.clear();

  m_pointPressed = -1;

  m_doShadows = true;
  m_allowInterpolate = false;
  m_showPointNumbers = false;
  m_showPoints = true;
  m_updateFlag = false;
  m_lightChanged = false; // for light recomputation
  m_lightType = 0; // point light
  m_rad = 5;
  m_decay = 1.0;
  m_angle = 60.0;
  m_show = true;

  m_segments = 1;
  m_color = Vec(1,1,1);
  m_opacity = 1;
  m_lod = 1;
  m_smooth = 1;
  m_points.clear();
  
  m_length = 0;
  m_tgP.clear();
  m_xaxis.clear();
  m_yaxis.clear();
  m_path.clear();
  m_pathT.clear();
  m_pathX.clear();
  m_pathY.clear();
}

GiLightObject::~GiLightObject()
{
  m_undo.clear();

  m_points.clear();
  m_tgP.clear();
  m_xaxis.clear();
  m_yaxis.clear();
  m_path.clear();
  m_pathT.clear();
  m_pathX.clear();
  m_pathY.clear();
}

void
GiLightObject::translate(bool moveX, bool indir)
{
  QList<Vec> delta;

  int npoints = m_points.count();
  for (int i=0; i<npoints; i++)
    {
      if (moveX)
	delta << m_pathX[i*m_segments];
      else
	delta << m_pathY[i*m_segments];
    }	  

  if (indir)
    {
      for(int i=0; i<npoints; i++)
	m_points[i] += delta[i];
    }
  else
    {
      for(int i=0; i<npoints; i++)
	m_points[i] -= delta[i];
    }

  computeTangents();
  m_updateFlag = true;
  m_lightChanged = true; // for light recomputation

  m_undo.append(m_points);
}

void
GiLightObject::translate(int idx, int moveType, float mag)
{
  int npoints = m_points.count();

  if (moveType == 0) // move along y-axis
    {
      if (idx<0 || idx>=npoints)
	{
	  for(int i=0; i<npoints; i++)
	    m_points[i] += mag*m_pathY[i*m_segments];
	}
      else
	m_points[idx] += mag*m_pathY[idx*m_segments];
    }
  else if (moveType == 1) // move along x-axis
    {
      if (idx<0 || idx>=npoints)
	{
	  for(int i=0; i<npoints; i++)
	    m_points[i] += mag*m_pathX[i*m_segments];
	}
      else
	m_points[idx] += mag*m_pathX[idx*m_segments];
    }
  else // move along tangent
    {
      if (idx<0 || idx>=npoints)
	{
	  for(int i=0; i<npoints; i++)
	    m_points[i] += mag*m_pathT[i*m_segments];
	}
      else
	m_points[idx] += mag*m_pathT[idx*m_segments];
    }

  computeTangents();
  m_updateFlag = true;
  m_lightChanged = true; // for light recomputation

  m_undo.append(m_points);
}

bool GiLightObject::doShadows() { return m_doShadows; }
void GiLightObject::setDoShadows(bool b)
{
  m_doShadows = b; 
  m_lightChanged = true; // for light recomputation
}

bool GiLightObject::allowInterpolate() { return m_allowInterpolate; }
void GiLightObject::setAllowInterpolate(bool a) { m_allowInterpolate = a; }

void GiLightObject::setLightType(int t)
{
  m_lightType = t;

  if (m_lightType == 1) // direction light
    { // keep two points only
      if (m_points.count() != 2)
	{
	  QList<Vec> pp;
	  pp << m_points[0];
	  if (m_points.count() > 2)
	    pp << m_points[1];
	  else
	    pp << m_points[0] + Vec(0,0,1);

	  m_segments = 1;
	  setPoints(pp);
	}
    }
}

bool GiLightObject::showPointNumbers() { return m_showPointNumbers; }
bool GiLightObject::showPoints() { return m_showPoints; }
Vec GiLightObject::color() { return m_color; }
float GiLightObject::opacity() { return m_opacity; }
int GiLightObject::lod() { return m_lod; }
int GiLightObject::smooth() { return m_smooth; }
QList<Vec> GiLightObject::points() { return m_points; }
QList<Vec> GiLightObject::saxis() { return m_xaxis; }
QList<Vec> GiLightObject::taxis() { return m_yaxis; }
Vec GiLightObject::getPoint(int i)
{
  if (i >= 0 && i < m_points.count())
    return m_points[i];
  else
    return Vec(0,0,0);
}
QList<Vec> GiLightObject::pathPoints()
{
  if (m_updateFlag)
    computePathLength();
  return m_path;
}
QList<Vec> GiLightObject::tangents()
{
  if (m_updateFlag)
    computePathLength();
  return m_tgP;
}
QList<Vec> GiLightObject::pathT()
{
  if (m_updateFlag)
    computePathLength();
  return m_pathT;
}
QList<Vec> GiLightObject::pathX()
{
  if (m_updateFlag)
    computePathLength();
  return m_pathX;
}
QList<Vec> GiLightObject::pathY()
{
  if (m_updateFlag)
    computePathLength();
  return m_pathY;
}
int GiLightObject::segments() { return m_segments; }

void GiLightObject::setShowPointNumbers(bool flag) { m_showPointNumbers = flag; }
void GiLightObject::setShowPoints(bool flag) { m_showPoints = flag; }

void GiLightObject::setRad(int r)
{
  m_rad = r;
  m_lightChanged = true; // for light recomputation
}
void GiLightObject::setDecay(float d)
{
  m_decay = d;
  m_lightChanged = true; // for light recomputation
}
void GiLightObject::setAngle(float a)
{
  m_angle = a;
  m_lightChanged = true; // for light recomputation
}
void GiLightObject::setColor(Vec color)
{
  m_color = color;
  m_lightChanged = true; // for light recomputation
}
void GiLightObject::setOpacity(float op)
{
  m_opacity = op;
  m_lightChanged = true; // for light recomputation
}
void GiLightObject::setLod(int l)
{
  m_lod = l;
  m_lightChanged = true; // for light recomputation
}
void GiLightObject::setSmooth(int l)
{
  m_smooth = l;
  m_lightChanged = true; // for light recomputation
}

void GiLightObject::setPoint(int i, Vec pt)
{
  if (i >= 0 && i < m_points.count())
    {
      m_points[i] = pt;
      computeTangents();
    }
  else
    {
      for(int j=0; j<m_points.count(); j++)
	m_points[j] += pt;
    }
  m_updateFlag = true;
  m_lightChanged = true; // for light recomputation
}
void GiLightObject::normalize()
{
  for(int i=0; i<m_points.count(); i++)
    {
      Vec pt = m_points[i];
      pt = Vec((int)pt.x, (int)pt.y, (int)pt.z);
      m_points[i] = pt;
    }
  m_updateFlag = true;
  m_lightChanged = true; // for light recomputation
  m_undo.append(m_points);
}
void GiLightObject::replace(QList<Vec> pts)
{
  if (pts.count() == m_points.count())
    {
      m_points.clear();
      m_points.append(pts);

      m_updateFlag = true;
      m_lightChanged = true; // for light recomputation
      m_undo.append(m_points);
    }
}
void GiLightObject::setPoints(QList<Vec> pts)
{
  m_pointPressed = -1;

//  if (pts.count() < 2)
//    {
//      QMessageBox::information(0, QString("%1 points").arg(pts.count()),
//			       "Number of points must be greater than 1");
//      return;
//    }

  m_points = pts;

  computeTangents();

  m_updateFlag = true;
  m_lightChanged = true; // for light recomputation
  m_undo.append(m_points);
}
void GiLightObject::setSegments(int seg)
{
  m_segments = qMax(1, seg);
  m_updateFlag = true;
  m_lightChanged = true; // for light recomputation
}

void GiLightObject::setPointPressed(int p) { m_pointPressed = p; }
int GiLightObject::getPointPressed() { return m_pointPressed; }

void
GiLightObject::removePoint(int idx)
{
  if (m_points.count() <= 1 ||
      idx >= m_points.count())
    return;

  m_pointPressed = -1;

  m_points.removeAt(idx);

  computeTangents();

  m_updateFlag = true;
  m_lightChanged = true; // for light recomputation

  m_undo.append(m_points);
}

void
GiLightObject::addPoint(Vec pt)
{
  m_points.append(pt);

  m_pointPressed = -1;

  computeTangents();

  m_updateFlag = true;
  m_lightChanged = true; // for light recomputation

  m_undo.append(m_points);
}

void
GiLightObject::insertPointAfter(int idx)
{
  int npts = m_points.count();
  if (idx == npts-1)
    {
      Vec v = m_points[npts-1]-m_points[npts-2];
      v += m_points[npts-1];
      m_points.append(v);

      setPointPressed(npts-1);
    }
  else
    {
      Vec v = (m_points[idx]+m_points[idx+1])/2;
      m_points.insert(idx+1, v);
      
      setPointPressed(idx+1);
    }

  computeTangents();

  m_updateFlag = true;
  m_lightChanged = true; // for light recomputation

  m_undo.append(m_points);
}

void
GiLightObject::insertPointBefore(int idx)
{
  if (idx == 0)
    {
      Vec v = m_points[0]-m_points[1];
      v += m_points[0];

      m_points.insert(0, v);
      setPointPressed(0);
    }
  else
    {
      Vec v = (m_points[idx]+m_points[idx-1])/2;
      m_points.insert(idx, v);
  
      setPointPressed(idx);
    }

  computeTangents();

  m_updateFlag = true;
  m_lightChanged = true; // for light recomputation

  m_undo.append(m_points);
}

void
GiLightObject::computePathLength()
{
  computePath(m_points);
}

void
GiLightObject::computeLength(QList<Vec> points)
{
  m_length = 0;
  for(int i=1; i<points.count(); i++)
    m_length += (points[i]-points[i-1]).norm();
}

void
GiLightObject::computeTangents()
{
  m_xaxis.clear();
  m_yaxis.clear();
  m_tgP.clear();

  int nkf = m_points.count();
  if (nkf < 2)
    {
      m_xaxis << Vec(1,0,0);
      m_yaxis << Vec(0,1,0);
      m_tgP << Vec(0,0,1);
      return;
    }

  Vec pxaxis = Vec(1,0,0);
  Vec ptang = Vec(0,0,1);

  for(int kf=0; kf<nkf; kf++)
    {
      Vec prevP, nextP;

      if (kf == 0)
	{	  
	  prevP = m_points[kf];
	}
      else
	prevP = m_points[kf-1];

      if (kf == nkf-1)
	{
	  nextP = m_points[kf];
	}
      else
	nextP = m_points[kf+1];
      
      Vec tgP = 0.5*(nextP - prevP);
      m_tgP.append(tgP);

      //-------------------
      tgP.normalize();
      Vec xaxis, yaxis;
      Vec axis;
      float angle;      
      StaticFunctions::getRotationBetweenVectors(ptang, tgP, axis, angle);
      if (qAbs(angle) > 0.0 && qAbs(angle) < 3.1415)
	{
	  Quaternion q(axis, angle);	  
	  xaxis = q.rotate(pxaxis);
	}
      else
	xaxis = pxaxis;

      yaxis = tgP^xaxis;

      m_xaxis.append(xaxis);
      m_yaxis.append(yaxis);

      pxaxis = xaxis;
      ptang = tgP;
      //-------------------
    } 
}

Vec
GiLightObject::interpolate(int kf1, int kf2, float frc)
{
  Vec diff = m_points[kf2] - m_points[kf1];
  Vec pos = m_points[kf1];
  float len = diff.squaredNorm();
  if (len > 0.1)
    {
      Vec v1 = 3*diff - 2*m_tgP[kf1] - m_tgP[kf2];
      Vec v2 = -2*diff + m_tgP[kf1] + m_tgP[kf2];
      
      pos += frc*(m_tgP[kf1] + frc*(v1+frc*v2));
    }

  return pos;
}

void
GiLightObject::computePathVectors()
{
  // using Double Reflection Method
  m_pathT.clear();
  m_pathX.clear();
  m_pathY.clear();

  int npoints = m_path.count();
  if (npoints < 2)
    return;

  Vec t0 = m_path[1] - m_path[0];
  t0.normalize();
  Vec r0 = Vec(1,0,0)^t0;
  if (r0.norm() < 0.1)
    {
      r0 = Vec(0,1,0)^t0;      
      if (r0.norm() < 0.1)
	r0 = Vec(0,0,1)^t0;	  
    }
  r0.normalize();
  Vec s0 = t0^r0;
  s0. normalize();

  m_pathT << t0;
  m_pathX << r0;
  m_pathY << s0;
  for(int i=0; i<npoints-1; i++)
    {
      Vec t1, r1, s1;
      Vec t0L, r0L, s0L;
      Vec v1, v2;
      float c1, c2;

      v1 = m_path[i+1]-m_path[i];
      c1 = v1*v1;
      r0L = r0-(2.0/c1)*(v1*r0)*v1;
      t0L = t0-(2.0/c1)*(v1*t0)*v1;

      t1 = m_path[i+1]-m_path[i];

      if (i < npoints-2) t1 = m_path[i+2]-m_path[i];
	
      t1.normalize();
      v2 = t1 - t0L;
      c2 = v2*v2;
      r1 = r0L-(2.0/c2)*(v2*r0L)*v2;
      r1.normalize();
      s1 = t1^r1;
      s1.normalize();      

      m_pathT << t1;
      m_pathX << r1;
      m_pathY << s1;

      t0 = t1;
      r0 = r1;
      s0 = s1;
    }
}

void
GiLightObject::computePath(QList<Vec> points)
{
  Vec voxelScaling = Global::voxelScaling();
  Vec voxelSize = VolumeInformation::volumeInformation().voxelSize;

  // -- collect path points for length computation
  QList<Vec> lengthPath;
  lengthPath.clear();

  m_path.clear();

  int npts = points.count();
  if (npts == 1)
    {
      m_path << VECPRODUCT(points[0], voxelScaling);
      m_pathT << Vec(0,0,1);
      m_pathX << Vec(0,1,0);
      m_pathY << Vec(1,0,0);
      return;
    }

  Vec prevPt;
  if (m_segments == 1)
    {
      for(int i=0; i<npts; i++)
	{
	  Vec v = VECPRODUCT(points[i], voxelScaling);
	  m_path.append(v);

	  // for length calculation
	  v = VECPRODUCT(points[i], voxelSize);
	  lengthPath.append(v);
	}

      computePathVectors();
  
      computeLength(lengthPath);
      return;
    }

  // for number of segments > 1 apply spline-based interpolation
  for(int i=1; i<npts; i++)
    {
      for(int j=0; j<m_segments; j++)
	{
	  float frc = (float)j/(float)m_segments;
	  Vec pos = interpolate(i-1, i, frc);

	  Vec pv = VECPRODUCT(pos, voxelScaling);
	  m_path.append(pv);

	  // for length calculation
	  pv = VECPRODUCT(pos, voxelSize);
	  lengthPath.append(pv);
	}
    }

  // last point
  Vec pos = VECPRODUCT(points[points.count()-1],
		       voxelScaling);
  m_path.append(pos);

  // for length calculation
  pos = VECPRODUCT(points[points.count()-1],
		   voxelSize);
  lengthPath.append(pos);
  
  computePathVectors();

  computeLength(lengthPath);
}

QList<Vec>
GiLightObject::getPointPath()
{
  QList<Vec> path;

  int npts = m_points.count();

  Vec prevPt;
  if (m_segments == 1 || npts == 1)
    {
      for(int i=0; i<npts; i++)
	path.append(m_points[i]);

      return path;
    }

  // for number of segments > 1 apply spline-based interpolation
  for(int i=1; i<npts; i++)
    {
      for(int j=0; j<m_segments; j++)
	{
	  float frc = (float)j/(float)m_segments;
	  Vec pos = interpolate(i-1, i, frc);
	  path.append(pos);
	}
    }
  // last point
  path.append(m_points[m_points.count()-1]);

  return path;
}

QList< QPair<Vec, Vec> >
GiLightObject::getPointAndNormalPath()
{
  computePath(m_points);

  QList< QPair<Vec, Vec> > pathNormal;

  if (m_path.count() == 0)
    return pathNormal;

  for (int i=0; i<m_path.count(); i++)
    pathNormal.append(qMakePair(m_path[i], m_pathX[i]));

  return pathNormal;
}

void
GiLightObject::draw(QGLViewer *viewer,
		    bool active,
		    bool backToFront)
{
  if (m_updateFlag)
    {
      m_updateFlag = false;

      computePathLength();
    }

  if (m_show)
    drawLines(viewer, active, backToFront);
}

void
GiLightObject::drawLines(QGLViewer *viewer,
		      bool active,
		      bool backToFront)
{
  glEnable(GL_BLEND);
//  glEnable(GL_LINE_SMOOTH);
//  glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

  //Vec col = m_opacity*m_color;
  Vec col = m_color;
  if (backToFront)
    {
      if (active)
	glLineWidth(7);
      else
	glLineWidth(3);
	
      glColor4f(col.x*0.5,
		col.y*0.5,
		col.z*0.5,
		0.5);
		//m_opacity*0.5);
      
      glBegin(GL_LINE_STRIP);
      for(int i=0; i<m_path.count(); i++)
	glVertex3fv(m_path[i]);
      glEnd();
    }

  if (m_showPoints)
    {
      glColor3f(m_color.x,
		m_color.y,
		m_color.z);


      glEnable(GL_POINT_SPRITE);
      glActiveTexture(GL_TEXTURE0);
      glEnable(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D, Global::lightTexture());
      glTexEnvf(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE );
      glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
      glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
      glEnable(GL_POINT_SMOOTH);

      Vec voxelScaling = Global::voxelScaling();
      glPointSize(50);
      if (m_lightType == 1)
	{ // draw only first point
	  glBegin(GL_POINTS);
	  Vec pt = VECPRODUCT(m_points[0], voxelScaling);
	  glVertex3fv(pt);
	  glEnd();
	}
      else
	{
	  glBegin(GL_POINTS);
	  for(int i=0; i<m_points.count();i++)
	    {
	      Vec pt = VECPRODUCT(m_points[i], voxelScaling);
	      glVertex3fv(pt);
	    }
	  glEnd();
	}


      if (m_pointPressed > -1)
	{
	  glColor3f(1,0,0);
	  Vec voxelScaling = Global::voxelScaling();
	  glPointSize(75);
	  glBegin(GL_POINTS);
	  Vec pt = VECPRODUCT(m_points[m_pointPressed], voxelScaling);
	  glVertex3fv(pt);
	  glEnd();
	}

      // "inverse ambient light"
      if (m_rad == 0)
	{
	  glColor3f(1,1,1);	  

	  glEnable(GL_POINT_SPRITE);
	  glActiveTexture(GL_TEXTURE0);
	  glEnable(GL_TEXTURE_2D);
	  glBindTexture(GL_TEXTURE_2D, Global::hollowSpriteTexture());
	  glTexEnvf(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE );
	  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	  glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
	  glEnable(GL_POINT_SMOOTH);
	  Vec voxelScaling = Global::voxelScaling();
	  glPointSize(50);
	  glBegin(GL_POINTS);
	  Vec pt = VECPRODUCT(m_points[0], voxelScaling);
	  glVertex3fv(pt);
	  glEnd();
	}

      glPointSize(1);  

      glDisable(GL_POINT_SPRITE);
      glDisable(GL_TEXTURE_2D);
  
      glDisable(GL_POINT_SMOOTH);
    }

  //glColor4f(col.x, col.y, col.z, qMin(1.0f,m_opacity));
  glColor4f(col.x, col.y, col.z, 1.0f);

  if (m_path.count() > 1)
    {
      if (active)
	glLineWidth(3);
      else
	glLineWidth(1);

      glBegin(GL_LINE_STRIP);
      for(int i=0; i<m_path.count(); i++)
	glVertex3fv(m_path[i]);
      glEnd();

      if (!backToFront)
	{
	  if (active)
	    glLineWidth(7);
	  else
	    glLineWidth(3);
	  
	  glColor4f(col.x*0.5,
		    col.y*0.5,
		    col.z*0.5,
		    0.5);
		    //m_opacity*0.5);
	  
	  glBegin(GL_LINE_STRIP);
	  for(int i=0; i<m_path.count(); i++)
	    glVertex3fv(m_path[i]);
	  glEnd();
	}
      glLineWidth(1);
    }

  //  glDisable(GL_LINE_SMOOTH);
}

void
GiLightObject::postdrawPointNumbers(QGLViewer *viewer)
{
  Vec voxelScaling = Global::voxelScaling();
  for(int i=0; i<m_points.count();i++)
    {
      Vec pt = VECPRODUCT(m_points[i], voxelScaling);
      Vec scr = viewer->camera()->projectedCoordinatesOf(pt);
      int x = scr.x;
      int y = scr.y;
      
      //---------------------
      x *= viewer->size().width()/viewer->camera()->screenWidth();
      y *= viewer->size().height()/viewer->camera()->screenHeight();
      //---------------------
      
      QString str = QString("%1").arg(i);
      QFont font = QFont();
      QFontMetrics metric(font);
      int ht = metric.height();
      int wd = metric.width(str);
      y += ht/2;
      
      StaticFunctions::renderText(x+2, y, str, font, Qt::black, Qt::green);
    }
}

void
GiLightObject::postdrawGrab(QGLViewer *viewer,
			    int x, int y)
{
  QString str;
  if (m_lightType == 0)
    str = "point light grabbed";
  else 
    str = "direction light grabbed";
  QFont font = QFont();
  QFontMetrics metric(font);
  int ht = metric.height();
  int wd = metric.width(str);
  x += 10;
  
  StaticFunctions::renderText(x+2, y, str, font, Qt::black, Qt::green);
}


void
GiLightObject::postdraw(QGLViewer *viewer,
		     int x, int y,
		     bool grabsMouse,
		     float scale)
{
  if (!m_show)
    return;

  if (!grabsMouse &&
      !m_showPointNumbers)
    return;

//  glEnable(GL_LINE_SMOOTH);
//  glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

  glDisable(GL_DEPTH_TEST);
  viewer->startScreenCoordinatesSystem();

  if (grabsMouse)
    postdrawGrab(viewer, x, y);
  
  if (m_showPointNumbers)
    postdrawPointNumbers(viewer);
  
  viewer->stopScreenCoordinatesSystem();
  glEnable(GL_DEPTH_TEST);
}

void
GiLightObject::undo()
{
  m_undo.undo();

  m_points = m_undo.points();

  computeTangents();
  m_updateFlag = true;
  m_lightChanged = true; // for light recomputation
}

void
GiLightObject::redo()
{
  m_undo.redo();

  m_points = m_undo.points();

  computeTangents();
  m_updateFlag = true;
  m_lightChanged = true; // for light recomputation
}

void
GiLightObject::updateUndo()
{
  m_undo.append(m_points);
}

bool
operator==(const GiLightObject& a,
	   const GiLightObject& b)
{
  return !(a!=b);
}

bool
operator!=(const GiLightObject& a,
	   const GiLightObject& b)
{
  if(a.m_lightType != b.m_lightType)
    return true;

  if(a.m_rad != b.m_rad)
    return true;

  if(a.m_lod != b.m_lod)
    return true;

  if(a.m_smooth != b.m_smooth)
    return true;

  if(a.m_decay != b.m_decay)
    return true;

  if(a.m_angle != b.m_angle)
    return true;

  if(a.m_points.count() != b.m_points.count())
    return true;

  for (int i=0; i<a.m_points.count(); i++)
    if (a.m_points[i] != b.m_points[i])
      return true;

  return false;
}

GiLightObject
GiLightObject::interpolate(GiLightObject po1,
			   GiLightObject po2,
			   float frc)
{
  GiLightObject po;
  po = po1;

  QList<Vec> pt1 = po1.points();
  QList<Vec> pt2 = po2.points();
  if (pt1.count() == pt2.count())
    {
      QList<Vec> pt;
      for(int i=0; i<pt1.count(); i++)
	{
	  Vec v;
	  v = (1-frc)*pt1[i] + frc*pt2[i];
	  pt << v;
	}

      po.replace(pt);
    }

  po.m_segments = (1-frc)*po1.m_segments + frc*po2.m_segments;
  po.m_opacity = (1-frc)*po1.m_opacity + frc*po2.m_opacity;
  po.m_color = (1-frc)*po1.m_color + frc*po2.m_color;

  return po;
}
QList<GiLightObject>
GiLightObject::interpolate(QList<GiLightObject> po1,
			   QList<GiLightObject> po2,
			   float frc)
{
  QList<GiLightObject> po;
  for(int i=0; i<po1.count(); i++)
    {
      GiLightObject pi;
      if (i < po2.count() && po1[i].allowInterpolate())
	{
	  pi = interpolate(po1[i], po2[i], frc);
	  po.append(pi);
	}
      else
	{
	  pi = po1[i];
	  po.append(pi);
	}
    }

  return po;
}

void
GiLightObject::makePlanar()
{
  if (m_points.count() < 3)
    return;
  makePlanar(0,1,2);
}

void
GiLightObject::makePlanar(int v0, int v1, int v2)
{
  int npoints = m_points.count();

  if (npoints < 3)
    return;
    
  Vec cen = m_points[0];
  for(int i=1; i<npoints; i++)
    cen += m_points[i];
  cen /= npoints;

  Vec normal;
  normal = (m_points[v0]-m_points[v1]).unit()^(m_points[v2]-m_points[v1]).unit();
  normal.normalize();
  // now shift all points into the plane
  for(int i=0; i<npoints; i++)
    {
      Vec v0 = m_points[i]-cen;
      float dv = normal*v0;
      m_points[i] -= dv*normal;
    }

  computeTangents();

  m_updateFlag = true;
  m_lightChanged = true; // for light recomputation
  m_undo.append(m_points);
}

void
GiLightObject::makeCircle()
{
  int npoints = m_points.count();

  if (npoints < 3)
    return;
    
  Vec cen = m_points[0];
  for(int i=1; i<npoints; i++)
    cen += m_points[i];
  cen /= npoints;

  float avgD = 0;
  for(int i=1; i<npoints; i++)
    avgD += (m_points[i]-cen).norm();
  avgD /= npoints;

  // now shift all points into the plane
  for(int i=0; i<npoints; i++)
    {
      Vec v0 = (m_points[i]-cen);
      if (v0.squaredNorm() > 0.0)
	{
	  v0.normalize();
	  m_points[i] = cen + avgD*v0;
	}
    }

  computeTangents();

  m_updateFlag = true;
  m_lightChanged = true; // for light recomputation
  m_undo.append(m_points);
}

bool
GiLightObject::checkIfNotEqual(GiLightObjectInfo glo)
{
  if (glo.points.count() != m_points.count()) return true;
  if (glo.segments != m_segments) return true;
  if (glo.lod != m_lod) return true;
  if (glo.doShadows != m_doShadows) return true;
  if (glo.smooth != m_smooth) return true;
  if ((glo.color-m_color).squaredNorm() > 0.0001) return true;
  if (fabs(glo.opacity-m_opacity) > 0.001) return true;
  if (glo.lightType != m_lightType) return true;
  if (glo.rad != m_rad) return true;
  if (fabs(glo.decay-m_decay) > 0.001) return true;
  if (fabs(glo.angle-m_angle) > 0.001) return true;

  for(int i=0; i<m_points.count(); i++)
    if ((glo.points[i]-m_points[i]).squaredNorm() > 0.001)
      return true;

  return false;
}

void
GiLightObject::setGiLightObjectInfo(GiLightObjectInfo glo)
{
  m_pointPressed = -1;

  m_points.clear();
  m_points = glo.points;
  m_allowInterpolate = glo.allowInterpolation;
  m_doShadows = glo.doShadows;
  m_show = glo.show;
  m_lightType = glo.lightType;
  m_rad = glo.rad;
  m_decay = glo.decay;
  m_angle = glo.angle;
  m_color = glo.color;
  m_opacity = glo.opacity;
  m_segments = glo.segments;
  m_lod = glo.lod;
  m_smooth = glo.smooth;

  m_pointPressed = -1;

  computeTangents();

  m_undo.clear();
  m_undo.append(m_points);

  m_updateFlag = true;
  m_lightChanged = true;
}

GiLightObjectInfo
GiLightObject::giLightObjectInfo()
{
  GiLightObjectInfo glo;
  glo.points = m_points;
  glo.allowInterpolation = m_allowInterpolate;
  glo.doShadows = m_doShadows;
  glo.show = m_show;
  glo.lightType = m_lightType;
  glo.rad = m_rad;
  glo.decay = m_decay;
  glo.angle = m_angle;
  glo.color = m_color;
  glo.opacity = m_opacity;
  glo.lod = m_lod;
  glo.smooth = m_smooth;
  glo.segments = m_segments;

  return glo;
}
