#include "global.h"
#include "pathgroupobject.h"
#include "captiondialog.h"
#include "staticfunctions.h"
#include "volumeinformation.h"

//------------------------------------------------------------------
PathGroupObjectUndo::PathGroupObjectUndo() { clear(); }
PathGroupObjectUndo::~PathGroupObjectUndo() { clear(); }

void
PathGroupObjectUndo::clear()
{
  m_pindex.clear();
  m_points.clear();
  m_pointRadX.clear();
  m_pointRadY.clear();
  m_pointAngle.clear();
  m_index = -1;
}

void
PathGroupObjectUndo::clearTop()
{
  if (m_index == m_points.count()-1)
    return;

  while(m_index < m_pindex.count()-1)
    m_pindex.removeLast();
  
  while(m_index < m_points.count()-1)
    m_points.removeLast();
  
  while(m_index < m_pointRadX.count()-1)
    m_pointRadX.removeLast();  

  while(m_index < m_pointRadY.count()-1)
    m_pointRadY.removeLast();
  
  while(m_index < m_pointAngle.count()-1)
    m_pointAngle.removeLast();
  
}

void
PathGroupObjectUndo::append(QList<int> pi, QList<Vec> p,
			    QList<float> px, QList<float> py, QList<float> pa)
{
  clearTop();
  m_pindex << pi;
  m_points << p;
  m_pointRadX << px;
  m_pointRadY << py;
  m_pointAngle << pa;
  m_index = m_points.count()-1;
}

void PathGroupObjectUndo::redo() { m_index = qMin(m_index+1, m_points.count()-1); }
void PathGroupObjectUndo::undo() { m_index = qMax(m_index-1, 0); }

QList<int>
PathGroupObjectUndo::index()
{
  QList<int> pi;

  if (m_index >= 0 && m_index < m_pindex.count())
    return m_pindex[m_index];

  return pi;
}

QList<Vec>
PathGroupObjectUndo::points()
{
  QList<Vec> p;

  if (m_index >= 0 && m_index < m_points.count())
    return m_points[m_index];

  return p;
}

QList<float>
PathGroupObjectUndo::pointRadX()
{
  QList<float> p;

  if (m_index >= 0 && m_index < m_pointRadX.count())
    return m_pointRadX[m_index];

  return p;
}

QList<float>
PathGroupObjectUndo::pointRadY()
{
  QList<float> p;

  if (m_index >= 0 && m_index < m_pointRadY.count())
    return m_pointRadY[m_index];

  return p;
}

QList<float>
PathGroupObjectUndo::pointAngle()
{
  QList<float> p;

  if (m_index >= 0 && m_index < m_pointAngle.count())
    return m_pointAngle[m_index];

  return p;
}
//------------------------------------------------------------------




PathGroupObject
PathGroupObject::get()
{
  PathGroupObject po;
  po = *this;
  return po;
}

void PathGroupObject::disableUndo(bool b)
{
  m_disableUndo = b;

  m_undo.clear();
  m_undo.append(m_index, m_points, m_pointRadX, m_pointRadY, m_pointAngle);
}

QFont PathGroupObject::captionFont() { return m_captionFont; }
QColor PathGroupObject::captionColor() { return m_captionColor; }
QColor PathGroupObject::captionHaloColor() { return m_captionHaloColor; }

bool PathGroupObject::captionGrabbed() { return m_captionGrabbed; }
void PathGroupObject::setCaptionGrabbed(bool cg) { m_captionGrabbed = cg; }

void PathGroupObject::setCaption(QFont cf,
				 QColor cc, QColor chc)
{
  bool doit = false;
  if (m_captionFont != cf) doit = true;
  if (m_captionColor != cc) doit = true;
  if (m_captionHaloColor != chc) doit = true;

  m_captionFont = cf;
  m_captionColor = cc;
  m_captionHaloColor = chc;

  if (doit)
    generateImages();
}

QList<Vec>
PathGroupObject::imageSizes()
{
  QList<Vec> isize;
  for(int ci=0; ci<m_pathIndex.count()-1; ci++)
    {
      isize << Vec(m_cImage[ci].width(),
		   m_cImage[ci].height(),
		   0);
    }
  return isize;
}

void
PathGroupObject::loadCaption()
{
  CaptionDialog cd(0,
		   "",
		   m_captionFont,
		   m_captionColor,
		   m_captionHaloColor);
  cd.hideText(true);
  cd.hideOpacity(true);
  cd.move(QCursor::pos());
  if (cd.exec() != QDialog::Accepted)
    return;

  setCaption(cd.font(),
	     cd.color(),
	     cd.haloColor());
}

void
PathGroupObject::set(const PathGroupObject &po)
{
  m_pointPressed = -1;

  m_updateFlag = true; // for recomputation

  if (m_spriteTexture)
    glDeleteTextures( 1, &m_spriteTexture );
  m_spriteTexture = 0;

  m_clip = po.m_clip;
  m_scaleType = po.m_scaleType;
  m_minScale = po.m_minScale;
  m_maxScale = po.m_maxScale;
  m_depthcue = po.m_depthcue;
  m_showPoints = po.m_showPoints;
  m_showLength = po.m_showLength;
  m_tube = po.m_tube;
  m_closed = po.m_closed;
  m_sections = po.m_sections;
  m_segments = po.m_segments;
  m_sparseness = po.m_sparseness;
  m_separation = po.m_separation;
  m_stops = po.m_stops;
  m_resampledStops = StaticFunctions::resampleGradientStops(m_stops);
  m_opacity = po.m_opacity;
  m_points = po.m_points;
  m_pointRadX = po.m_pointRadX;
  m_pointRadY = po.m_pointRadY;
  m_pointAngle = po.m_pointAngle;
  m_capType = po.m_capType;
  m_arrowDirection = po.m_arrowDirection;
  m_arrowForAll = po.m_arrowForAll;
  m_index = po.m_index;
  m_animate = po.m_animate;
  m_allowInterpolate = po.m_allowInterpolate;
  m_blendMode = po.m_blendMode;
  m_animateSpeed = po.m_animateSpeed;
  m_minUserPathLen = po.m_minUserPathLen;
  m_maxUserPathLen = po.m_maxUserPathLen;
  m_filterPathLen = po.m_filterPathLen;

  m_captionFont = po.m_captionFont;
  m_captionColor = po.m_captionColor;
  m_captionHaloColor = po.m_captionHaloColor;

  // we take m_sections to be multiple of 4
  m_sections = (m_sections/4)*4;

  for (int i=0; i<m_index.count()-1; i++)
    computeTangents(m_index[i], m_index[i+1]);

  m_undo.clear();
  m_undo.append(m_index, m_points, m_pointRadX, m_pointRadY, m_pointAngle);
}

PathGroupObject&
PathGroupObject::operator=(const PathGroupObject &po)
{
  m_pointPressed = -1;

  m_updateFlag = true; // for recomputation

  if (m_spriteTexture)
    glDeleteTextures( 1, &m_spriteTexture );
  m_spriteTexture = 0;

  m_clip = po.m_clip;
  m_scaleType = po.m_scaleType;
  m_minScale = po.m_minScale;
  m_maxScale = po.m_maxScale;
  m_depthcue = po.m_depthcue;
  m_showPoints = po.m_showPoints;
  m_showLength = po.m_showLength;
  m_tube = po.m_tube;
  m_closed = po.m_closed;
  m_sections = po.m_sections;
  m_segments = po.m_segments;
  m_sparseness = po.m_sparseness;
  m_separation = po.m_separation;
  m_stops = po.m_stops;
  m_resampledStops = StaticFunctions::resampleGradientStops(m_stops);
  m_opacity = po.m_opacity;
  m_points = po.m_points;
  m_pointRadX = po.m_pointRadX;
  m_pointRadY = po.m_pointRadY;
  m_pointAngle = po.m_pointAngle;
  m_capType = po.m_capType;
  m_arrowDirection = po.m_arrowDirection;
  m_arrowForAll = po.m_arrowForAll;
  m_index = po.m_index;
  m_animate = po.m_animate;
  m_allowInterpolate = po.m_allowInterpolate;
  m_blendMode = po.m_blendMode;
  m_animateSpeed = po.m_animateSpeed;
  m_minUserPathLen = po.m_minUserPathLen;
  m_maxUserPathLen = po.m_maxUserPathLen;
  m_filterPathLen = po.m_filterPathLen;

  m_captionFont = po.m_captionFont;
  m_captionColor = po.m_captionColor;
  m_captionHaloColor = po.m_captionHaloColor;

  // we take m_sections to be multiple of 4
  m_sections = (m_sections/4)*4;

  for (int i=0; i<m_index.count()-1; i++)
    computeTangents(m_index[i], m_index[i+1]);

  m_undo.clear();
  m_undo.append(m_index, m_points, m_pointRadX, m_pointRadY, m_pointAngle);

  return *this;
}

PathGroupObject
PathGroupObject::interpolate(PathGroupObject po1,
			     PathGroupObject po2,
			     float frc)
{
  PathGroupObject po;
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
      po.replacePoints(pt);
    }

  po.m_scaleType = po1.m_scaleType;
  po.m_minScale = (1-frc)*po1.m_minScale + frc*po2.m_minScale;
  po.m_maxScale = (1-frc)*po1.m_maxScale + frc*po2.m_maxScale;
  po.m_sections = (1-frc)*po1.m_sections + frc*po2.m_sections;
  po.m_segments = (1-frc)*po1.m_segments + frc*po2.m_segments;
  po.m_sparseness = (1-frc)*po1.m_sparseness + frc*po2.m_sparseness;
  po.m_separation = (1-frc)*po1.m_separation + frc*po2.m_separation;
  po.m_opacity = (1-frc)*po1.m_opacity + frc*po2.m_opacity;
  po.m_animateSpeed = (1-frc)*po1.m_animateSpeed + frc*po2.m_animateSpeed;
  po.m_minUserPathLen = (1-frc)*po1.m_minUserPathLen + frc*po2.m_minUserPathLen;
  po.m_maxUserPathLen = (1-frc)*po1.m_maxUserPathLen + frc*po2.m_maxUserPathLen;

  return po;
}
QList<PathGroupObject>
PathGroupObject::interpolate(QList<PathGroupObject> po1,
			     QList<PathGroupObject> po2,
			     float frc)
{
  QList<PathGroupObject> po;
  for(int i=0; i<po1.count(); i++)
    {
      PathGroupObject pi;
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

bool PathGroupObject::scaleType() { return m_scaleType; }
void PathGroupObject::setScaleType(bool s) { m_scaleType = s; }

float PathGroupObject::minScale() { return m_minScale; }
void PathGroupObject::setMinScale(float s) { m_minScale = s; }

float PathGroupObject::maxScale() { return m_maxScale; }
void PathGroupObject::setMaxScale(float s) { m_maxScale = s; }

bool PathGroupObject::clip() { return m_clip; }
void PathGroupObject::setClip(bool c) { m_clip = c; }

bool PathGroupObject::allowEditing() { return m_allowEditing; }
void PathGroupObject::setAllowEditing(bool a) { m_allowEditing = a; }

bool PathGroupObject::blendMode() { return m_blendMode; }
void PathGroupObject::setBlendMode(bool a) { m_blendMode = a; }

bool PathGroupObject::allowInterpolate() { return m_allowInterpolate; }
void PathGroupObject::setAllowInterpolate(bool a) { m_allowInterpolate = a; }

int PathGroupObject::m_animationstep = 0;
PathGroupObject::PathGroupObject()
{
  m_disableUndo = false;

  m_pointPressed = -1;
  m_spriteTexture = 0;
  m_allowEditing = false;
  m_allowInterpolate = false;
  m_blendMode = false;
  m_clip = false;
  m_scaleType = true; // absolute pathlength
  m_minScale = 1;
  m_maxScale = 1;
  m_depthcue = false;
  m_showPoints = false;
  m_showLength = false;
  m_updateFlag = false;
  m_tube = false;
  m_closed = false;
  m_sections = 4;
  m_segments = 1;
  m_sparseness = 1;
  m_separation = 0;
  m_animate = false;
  m_animateSpeed = 25;
  m_pointColor = Vec(0.0f, 0.5f, 1.0f);
  m_stops << QGradientStop(0.0, QColor(250,230,200,255))
	  << QGradientStop(1.0, QColor(200,100,50,255));
  m_resampledStops = StaticFunctions::resampleGradientStops(m_stops);
  m_opacity = 1;
  m_points.clear();
  m_pointRadX.clear();
  m_pointRadY.clear();
  m_pointAngle.clear();
  m_capType = ROUND;
  m_arrowDirection = true;
  m_arrowForAll = false;
  m_index.clear();
  m_valid.clear();
  m_validPoint.clear();

  m_length = 0;
  m_tgP.clear();
  m_path.clear();
  m_pathIndex.clear();
  m_radX.clear();
  m_radY.clear();
  m_angle.clear();

  m_captionFont = QFont("Helvetica", 12);
  m_captionColor = Qt::white;
  m_captionHaloColor = Qt::black;

  m_displayList = 0;

  m_indexGrabbed = -1;

  m_minUserPathLen = m_maxUserPathLen = -1;
  m_filterPathLen = false;

  m_undo.clear();
}

PathGroupObject::~PathGroupObject()
{
  if (m_spriteTexture)
    glDeleteTextures( 1, &m_spriteTexture );
  m_spriteTexture = 0;
    
  m_points.clear();
  m_pointRadX.clear();
  m_pointRadY.clear();
  m_pointAngle.clear();
  m_tgP.clear();
  m_path.clear();
  m_pathIndex.clear();
  m_radX.clear();
  m_radY.clear();
  m_angle.clear();
  m_index.clear();

  m_valid.clear();
  m_validPoint.clear();

  if (m_displayList > 0)
    {
      glDeleteLists(m_displayList, 1);
      m_displayList = 0;
    }

  m_indexGrabbed = -1;

  m_undo.clear();
}
void PathGroupObject::setGrabsIndex(int ci) { m_indexGrabbed = ci; }

int PathGroupObject::sparseness() { return m_sparseness; }
void PathGroupObject::setSparseness(int sp) { m_sparseness = qMax(1, sp); }

int PathGroupObject::separation() { return m_separation; }
void PathGroupObject::setSeparation(int sp) { m_separation = qMax(0, sp); }

bool PathGroupObject::animate() { return m_animate; }
void PathGroupObject::setAnimate(bool a) { m_animate = a; }

int PathGroupObject::animateSpeed() { return m_animateSpeed; }
void PathGroupObject::setAnimateSpeed(int a) { m_animateSpeed = a; }

bool PathGroupObject::filterPathLen() { return m_filterPathLen; }
void PathGroupObject::setFilterPathLen(bool pl) { m_filterPathLen = pl; }

void PathGroupObject::pathlenMinmax(float &lmin, float &lmax)
{
  lmin = m_minPathLen;
  lmax = m_maxPathLen;
}
void PathGroupObject::userPathlenMinmax(float &lmin, float &lmax)
{
  lmin = m_minUserPathLen;
  lmax = m_maxUserPathLen;
}
void PathGroupObject::setUserPathlenMinmax(float lmin, float lmax)
{
  m_minUserPathLen = lmin;
  m_maxUserPathLen = lmax;
}
bool PathGroupObject::depthcue() { return m_depthcue; }
bool PathGroupObject::showPoints() { return m_showPoints; }
bool PathGroupObject::showLength() { return m_showLength; }
bool PathGroupObject::arrowDirection() { return m_arrowDirection; }
bool PathGroupObject::arrowForAll() { return m_arrowForAll; }
int PathGroupObject::capType() { return m_capType; }
bool PathGroupObject::tube() { return m_tube; }
bool PathGroupObject::closed() { return m_closed; }
QGradientStops PathGroupObject::stops() { return m_stops; }
float PathGroupObject::opacity() { return m_opacity; }
QList<Vec> PathGroupObject::points() { return m_points; }
Vec PathGroupObject::getPoint(int i)
{
  if (i < m_points.count())
    return m_points[i];
  else
    return Vec(0,0,0);
}
float PathGroupObject::getRadX(int i)
{
  if (i < m_points.count())
    return m_pointRadX[i];
  else
    return 0;
}
float PathGroupObject::getRadY(int i)
{
  if (i < m_points.count())
    return m_pointRadY[i];
  else
    return 0;
}
float PathGroupObject::getAngle(int i)
{
  if (i < m_points.count())
    return m_pointAngle[i];
  else
    return 0;
}
QVector<bool> PathGroupObject::valid() { return m_valid; }
QList<Vec> PathGroupObject::pathPoints()
{
  if (m_updateFlag)
    computePathLength();
  return m_path;
}
QList<int> PathGroupObject::pathPointIndices()
{
  if (m_updateFlag)
    computePathLength();
  return m_pathIndex;
}
QList<float> PathGroupObject::radX() { return m_pointRadX; }
QList<float> PathGroupObject::radY() { return m_pointRadY; }
QList<float> PathGroupObject::angle() { return m_pointAngle; }
int PathGroupObject::segments() { return m_segments; }
int PathGroupObject::sections() { return m_sections; }
float PathGroupObject::length()
{
  if (m_updateFlag)
    computePathLength();
  return m_length;
}

void
PathGroupObject::setStops(QGradientStops stops)
{
  m_stops = stops;
  m_resampledStops = StaticFunctions::resampleGradientStops(stops);
}
void PathGroupObject::setDepthcue(bool flag) { m_depthcue = flag; }
void PathGroupObject::setShowPoints(bool flag) { m_showPoints = flag; }
void PathGroupObject::setShowLength(bool flag) { m_showLength = flag; }
void PathGroupObject::setArrowDirection(bool flag)
{
  m_arrowDirection = flag;
  m_updateFlag = true;
}
void PathGroupObject::setArrowForAll(bool flag) { m_arrowForAll = flag; }
void PathGroupObject::setCapType(int ct) {m_capType = ct; }
void PathGroupObject::setTube(bool flag)
{
  m_tube = flag;
}
void PathGroupObject::setClosed(bool flag)
{
  m_closed = flag;
  m_updateFlag = true;
}

void PathGroupObject::setOpacity(float op)
{
  m_opacity = op;
}
void PathGroupObject::setPoint(int i, Vec pt)
{
  if (i < m_points.count())
    {
      m_points[i] = pt;

      for (int ci=0; ci<m_index.count()-1; ci++)
	computeTangents(m_index[ci], m_index[ci+1]);

      m_updateFlag = true;
    }
}
void PathGroupObject::setRadX(int i, float v, bool sameForAll)
{
  if (i < m_points.count())
    {      
      if (sameForAll)
	{
	  for(int j=0; j<m_points.count(); j++)
	    m_pointRadX[j] = v;	    
	  for(int j=0; j<m_radX.count(); j++)
	    m_radX[j] = v;   
	}
      else
	{
	  m_pointRadX[i] = v;
	  m_updateFlag = true;
	}

      m_updateFlag = true;
    }
  if (!m_disableUndo)
    m_undo.append(m_index, m_points, m_pointRadX, m_pointRadY, m_pointAngle);
}
void PathGroupObject::setRadY(int i, float v, bool sameForAll)
{
  if (i < m_points.count())
    {
      if (sameForAll)
	{
	  for(int j=0; j<m_points.count(); j++)
	    m_pointRadY[j] = v;   
	  for(int j=0; j<m_radY.count(); j++)
	    m_radY[j] = v;   
	}
      else
	{
	  m_pointRadY[i] = v;
	  m_updateFlag = true;
	}
    }
  if (!m_disableUndo)
    m_undo.append(m_index, m_points, m_pointRadX, m_pointRadY, m_pointAngle);
}
void PathGroupObject::setAngle(int i, float v, bool sameForAll)
{
  if (i < m_points.count())
    {

      if (sameForAll)
	{
	  for(int j=0; j<m_points.count(); j++)
	    m_pointAngle[j] = v;
	}
      else
	m_pointAngle[i] = v;

      m_updateFlag = true;
    }
  if (!m_disableUndo)
    m_undo.append(m_index, m_points, m_pointRadX, m_pointRadY, m_pointAngle);
}
void PathGroupObject::setSameForAll(bool flag)
{
  if (flag)
    {
      float x = m_pointRadX[0];
      float y = m_pointRadY[0];
      float a = m_pointAngle[0];
      for(int j=0; j<m_points.count(); j++)
	{
	  m_pointRadX[j] = x;
	  m_pointRadY[j] = y;
	  m_pointAngle[j] = a;
	}
    }
}
void PathGroupObject::normalize()
{
  for(int i=0; i<m_points.count(); i++)
    {
      Vec pt = m_points[i];
      pt = Vec((int)pt.x, (int)pt.y, (int)pt.z);
      m_points[i] = pt;
    }
  m_updateFlag = true;
  if (!m_disableUndo)
    m_undo.append(m_index, m_points, m_pointRadX, m_pointRadY, m_pointAngle);
}
void PathGroupObject::replacePoints(QList<Vec> pts)
{
  if (pts.count() == m_points.count())
    {
      m_points.clear();
      m_points.append(pts);
    }
}
void PathGroupObject::setPoints(QList<Vec> pts)
{
  m_pointPressed = -1;

  if (pts.count() < 2)
    {
      QMessageBox::information(0, QString("%1 points").arg(pts.count()),
			       "Number of points must be greater than 1");
      return;
    }

  m_points = pts;
  
  m_index.clear();
  m_index.append(0);
  m_index.append(m_points.count());
  computeTangents(0, m_points.count());

  m_pointRadX.clear();
  m_pointRadY.clear();
  m_pointAngle.clear();
  for(int i=0; i<pts.count(); i++)
    {
      m_pointRadX.append(1);
      m_pointRadY.append(1);
      m_pointAngle.append(0);
    }


  m_updateFlag = true;
  m_undo.append(m_index, m_points, m_pointRadX, m_pointRadY, m_pointAngle);
}
void PathGroupObject::setVectorPoints(QList<Vec> pts)
{
  m_pointPressed = -1;

  if (pts.count() < 2)
    {
      QMessageBox::information(0, QString("%1 points").arg(pts.count()),
			       "Number of points must be greater than 1");
      return;
    }

  m_capType = ARROW;
  m_showLength = false;
  m_showPoints = false;

  m_points = pts;
  
  m_index.clear();
  m_index.append(0);
  int nv = pts.count()/2;
  for(int i=0; i<nv; i++)
    m_index.append(2*(i+1));

  for (int i=0; i<m_index.count()-1; i++)
    computeTangents(m_index[i], m_index[i+1]);

  m_pointRadX.clear();
  m_pointRadY.clear();
  m_pointAngle.clear();
  for(int i=0; i<pts.count(); i++)
    {
      m_pointRadX.append(1);
      m_pointRadY.append(1);
      m_pointAngle.append(0);
    }

  m_updateFlag = true;
  m_undo.append(m_index, m_points, m_pointRadX, m_pointRadY, m_pointAngle);
}
void PathGroupObject::addPoints(QList<Vec> pts)
{
  m_pointPressed = -1;

  if (pts.count() < 2)
    {
      QMessageBox::information(0, QString("%1 points").arg(pts.count()),
			       "Number of points must be greater than 1");
      return;
    }

  if (m_index.count() == 0)
    {
      m_points = pts;

      m_index.append(0);
      m_index.append(m_points.count());
      computeTangents(0, m_points.count());

      m_pointRadX.clear();
      m_pointRadY.clear();
      m_pointAngle.clear();
    }
  else
    {
      m_points += pts;

      computeTangents(m_index[m_index.count()-1],
		      m_points.count());

      m_index.append(m_points.count());
    }

  for(int i=0; i<pts.count(); i++)
    {
      m_pointRadX.append(1);
      m_pointRadY.append(1);
      m_pointAngle.append(0);
    }

  m_updateFlag = true;
  if (!m_disableUndo)
    m_undo.append(m_index, m_points, m_pointRadX, m_pointRadY, m_pointAngle);
}
void PathGroupObject::setRadX(QList<float> rad)
{
  m_pointRadX = rad;  
  m_updateFlag = true;
  if (!m_disableUndo)
    m_undo.append(m_index, m_points, m_pointRadX, m_pointRadY, m_pointAngle);
}
void PathGroupObject::setRadY(QList<float> rad)
{
  m_pointRadY = rad;  
  m_updateFlag = true;
  if (!m_disableUndo)
    m_undo.append(m_index, m_points, m_pointRadX, m_pointRadY, m_pointAngle);
}
void PathGroupObject::setAngle(QList<float> rad)
{
  m_pointAngle = rad;  
  m_updateFlag = true;
  if (!m_disableUndo)
    m_undo.append(m_index, m_points, m_pointRadX, m_pointRadY, m_pointAngle);
}
void PathGroupObject::setSegments(int seg)
{
  m_segments = qMax(1, seg);
  m_updateFlag = true;
}
void PathGroupObject::setSections(int sec)
{
  m_sections = qMax(4, sec);
  // we take m_sections to be multiple of 4
  m_sections = (m_sections/4)*4;
}

void PathGroupObject::setPointPressed(int p) { m_pointPressed = p; }
int PathGroupObject::getPointPressed() { return m_pointPressed; }

void
PathGroupObject::computePathLength()
{
  if (m_arrowDirection == false)
    {
      QList<int> idx = m_index;
      QList<Vec> points = m_points;
      QList<float> radx = m_pointRadX;
      QList<float> rady = m_pointRadY;
      QList<float> angle = m_pointAngle;
      QVector<Vec> vtgP = m_tgP;

      m_index.clear();
      m_points.clear();
      m_pointRadX.clear();
      m_pointRadY.clear();
      m_pointAngle.clear();
      m_tgP.clear();
   
      int npts = points.count();
      for(int ci=0; ci<npts; ci++)
	m_points << points[npts-1-ci];
      for(int ci=0; ci<npts; ci++)
	m_pointRadX << radx[npts-1-ci];
      for(int ci=0; ci<npts; ci++)
	m_pointRadY << rady[npts-1-ci];
      for(int ci=0; ci<npts; ci++)
	m_pointAngle << angle[npts-1-ci];
      for(int ci=0; ci<npts; ci++)
	m_tgP << -vtgP[npts-1-ci];

      npts = idx.count();
      int lastidx = idx[npts-1];
      for(int ci=0; ci<npts; ci++)
	m_index << lastidx - idx[npts-1-ci];

      computePath();

      m_index = idx;
      m_points = points;
      m_pointRadX = radx;
      m_pointRadY = rady;
      m_pointAngle = angle;
      m_tgP = vtgP;
    }
  else
    {
      computePath();
    }

  generateImages();

  if (m_displayList > 0)
    {
      glDeleteLists(m_displayList, 1);
      m_displayList = 0;
    }
}

float
PathGroupObject::computeLength(QList<Vec> points)
{
  float length = 0;
  for(int i=1; i<points.count(); i++)
    length += (points[i]-points[i-1]).norm();
  return length;
}

void
PathGroupObject::computeTangents(int sidx, int eidx)
{
//  if (m_segments == 1)
//    return;

  if (m_points.count() > eidx)
    return;

  int nkf = m_points.count();
  if (nkf < 2)
    return;

  if (m_tgP.count() < m_points.count())
    {
      QVector<Vec> vtgP = m_tgP;
      m_tgP.resize(m_points.count());
      for(int i=0; i<vtgP.count(); i++)
	m_tgP[i] = vtgP[i];
    }

  for(int kf=sidx; kf<eidx; kf++)
    {
      Vec prevP, nextP;

      if (kf == sidx)
	{	  
	  if (!m_closed)
	    prevP = m_points[kf];
	  else
	    prevP = m_points[nkf-1];
	}
      else
	prevP = m_points[kf-1];

      if (kf == eidx-1)
	{
	  if (!m_closed)
	    nextP = m_points[kf];
	  else
	    nextP = m_points[0];
	}
      else
	nextP = m_points[kf+1];
      
      Vec tgP = 0.5*(nextP - prevP);
      m_tgP[kf] = tgP;
    } 
}

Vec
PathGroupObject::interpolate(int kf1, int kf2, float frc)
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

float
PathGroupObject::computeGrabbedPathLength()
{
  if (m_indexGrabbed == -1)
    return 0;

  Vec voxelScaling = Global::voxelScaling();
  Vec voxelSize = VolumeInformation::volumeInformation().voxelSize;

  if (m_points.count() < 2)
    return 0;

  int ci = m_indexGrabbed;
  int sidx = m_index[ci];
  int eidx = m_index[ci+1];

  // -- collect path points for length computation
  QList<Vec> lengthPath;
  lengthPath.clear();

  if (m_segments == 1)
    {
      for(int i=sidx; i<eidx; i++)
	{
	  Vec v = VECPRODUCT(m_points[i], voxelSize);
	  lengthPath.append(v);
	}
    }
  else
    {
      // for number of segments > 1 apply spline-based interpolation
      for(int i=sidx+1; i<eidx; i++)
	{
	  for(int j=0; j<m_segments; j++)
	    {
	      float frc = (float)j/(float)m_segments;
	      Vec pos = interpolate(i-1, i, frc);
	      Vec pv = VECPRODUCT(pos, voxelSize);
	      lengthPath.append(pv);
	    }
	}
      if (!m_closed)
	{
	  Vec pos = VECPRODUCT(m_points[eidx-1],
			   voxelSize);
	  lengthPath.append(pos);
	}
    }

  return computeLength(lengthPath);	      
}

void
PathGroupObject::computePath()
{
  Vec voxelScaling = Global::voxelScaling();
  Vec voxelSize = VolumeInformation::volumeInformation().voxelSize;

  m_length = 0;

  m_path.clear();
  m_pathIndex.clear();
  m_radX.clear();
  m_radY.clear();
  m_angle.clear();

  if (m_points.count() < 2)
    return;

  Vec prevPt;
  if (m_segments == 1)
    {
      m_pathIndex.append(0);
      for(int ci=0; ci<m_index.count()-1; ci++)
	{
	  int sidx = m_index[ci];
	  int eidx = m_index[ci+1];

	  // -- collect path points for length computation
	  QList<Vec> lengthPath;
	  lengthPath.clear();
	  
	  for(int i=sidx; i<eidx; i++)
	    {
	      Vec v = VECPRODUCT(m_points[i], voxelScaling);
	      m_path.append(v);
	      m_radX.append(m_pointRadX[i]);
	      m_radY.append(m_pointRadY[i]);
	      float o = DEG2RAD(m_pointAngle[i]);
	      m_angle.append(o);
	      
	      // for length calculation
	      v = VECPRODUCT(m_points[i], voxelSize);
	      lengthPath.append(v);
	    }
	  if (m_closed)
	    {
	      Vec v = VECPRODUCT(m_points[sidx], voxelScaling);
	      m_path.append(v);
	      m_radX.append(m_pointRadX[sidx]);
	      m_radY.append(m_pointRadY[sidx]);
	      float o = DEG2RAD(m_pointAngle[sidx]);
	      m_angle.append(o);
	      
	      // for length calculation
	      v = VECPRODUCT(m_points[sidx], voxelSize);
	      lengthPath.append(v);
	    }

	  m_pathIndex.append(m_path.count());

	  float plen = computeLength(lengthPath);
	  m_pathLength.append(plen);

	  m_length += plen;
	}

      //---------------
      // compute min and max path lengths
      m_minPathLen = m_pathLength[0];
      m_maxPathLen = m_pathLength[0];
      for(int ci=0; ci<m_pathLength.count(); ci++)
	{
	  m_minPathLen = qMin(m_minPathLen, m_pathLength[ci]);
	  m_maxPathLen = qMax(m_maxPathLen, m_pathLength[ci]);
	}
      if (m_minUserPathLen < 0)
	{
	  m_minUserPathLen = m_minPathLen;
	  m_maxUserPathLen = m_maxPathLen;
	}
      //---------------

      return;
    }


  // for number of segments > 1 apply spline-based interpolation
  m_pathIndex.append(0);
  for(int ci=0; ci<m_index.count()-1; ci++)
    {
      int sidx = m_index[ci];
      int eidx = m_index[ci+1];
      
      // -- collect path points for length computation
      QList<Vec> lengthPath;
      lengthPath.clear();
      
      for(int i=sidx+1; i<eidx; i++)
	{
	  float radX0 = m_pointRadX[i-1];
	  float radY0 = m_pointRadY[i-1];
	  float angle0 = DEG2RAD(m_pointAngle[i-1]);
	  float radX1 = m_pointRadX[i];
	  float radY1 = m_pointRadY[i];
	  float angle1 = DEG2RAD(m_pointAngle[i]);
	  for(int j=0; j<m_segments; j++)
	    {
	      float frc = (float)j/(float)m_segments;
	      Vec pos = interpolate(i-1, i, frc);
	      
	      Vec pv = VECPRODUCT(pos, voxelScaling);
	      m_path.append(pv);
	      
	      float v;
	      v = radX0 + frc*(radX1-radX0);
	      m_radX.append(v);
	      v = radY0 + frc*(radY1-radY0);
	      m_radY.append(v);
	      v = angle0 + frc*(angle1-angle0);
	      m_angle.append(v);
	      
	      
	      // for length calculation
	      pv = VECPRODUCT(pos, voxelSize);
	      lengthPath.append(pv);
	    }
	}

      if (!m_closed)
	{
	  Vec pos = VECPRODUCT(m_points[eidx-1],
			       voxelScaling);
	  m_path.append(pos);
	  
	  m_radX.append(m_pointRadX[eidx-1]);
	  m_radY.append(m_pointRadY[eidx-1]);
	  float o = DEG2RAD(m_pointAngle[eidx-1]);
	  m_angle.append(o);
	  
	  // for length calculation
	  pos = VECPRODUCT(m_points[eidx-1],
			   voxelSize);
	  lengthPath.append(pos);
	}
      else
	{
	  // for closed path
	  float radX0 = m_pointRadX[eidx-1];
	  float radY0 = m_pointRadY[eidx-1];
	  float angle0 = DEG2RAD(m_pointAngle[eidx-1]);
	  float radX1 = m_pointRadX[sidx];
	  float radY1 = m_pointRadY[sidx];
	  float angle1 = DEG2RAD(m_pointAngle[sidx]);
	  for(int j=0; j<m_segments; j++)
	    {
	      float frc = (float)j/(float)m_segments;
	      Vec pos = interpolate(eidx-1, sidx, frc);
	      
	      Vec pv = VECPRODUCT(pos, voxelScaling);
	      m_path.append(pv);
	      
	      float v;
	      v = radX0 + frc*(radX1-radX0);
	      m_radX.append(v);
	      v = radY0 + frc*(radY1-radY0);
	      m_radY.append(v);
	      v = angle0 + frc*(angle1-angle0);
	      m_angle.append(v);
	      
	      // for length calculation
	      pv = VECPRODUCT(pos, voxelSize);
	      lengthPath.append(pv);
	    }
	  Vec pos = VECPRODUCT(m_points[sidx], voxelScaling);
	  m_path.append(pos);
	  
	  m_radX.append(m_pointRadX[sidx]);
	  m_radY.append(m_pointRadY[sidx]);
	  float o = DEG2RAD(m_pointAngle[sidx]);
	  m_angle.append(o);
	  
	  // for length calculation
	  pos = VECPRODUCT(m_points[sidx], voxelSize);
	  lengthPath.append(pos);
	}
      
      m_pathIndex.append(m_path.count());

      float plen = computeLength(lengthPath);
      m_pathLength.append(plen);

      m_length += plen;
    }

  //---------------
  // compute min and max path lengths
  m_minPathLen = m_pathLength[0];
  m_maxPathLen = m_pathLength[0];
  for(int ci=0; ci<m_pathLength.count(); ci++)
    {
      m_minPathLen = qMin(m_minPathLen, m_pathLength[ci]);
      m_maxPathLen = qMax(m_maxPathLen, m_pathLength[ci]);
    }
  if (m_minUserPathLen < 0)
    {
      m_minUserPathLen = m_minPathLen;
      m_maxUserPathLen = m_maxPathLen;
    }
  //---------------
}

void
PathGroupObject::draw(QGLViewer *viewer,
		      bool active,
		      bool backToFront,
		      Vec lightPosition)
{
  if (m_tube)
    drawTube(viewer, active, lightPosition);
  else
    {
      if (m_capType != ARROW && m_showPoints)
	drawPoints();


      if (m_capType == ARROW && m_showPoints)
	{
	  glEnable(GL_POINT_SPRITE);
	  glActiveTexture(GL_TEXTURE0);
	  glEnable(GL_TEXTURE_2D);
	  glBindTexture(GL_TEXTURE_2D, m_spriteTexture);
	  glTexEnvf( GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE );
	  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	  glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
	  glEnable(GL_POINT_SMOOTH);
	}

      glCallList(m_displayList);

      if (m_capType == ARROW && m_showPoints)
	{
	  glDisable(GL_POINT_SPRITE);
	  glActiveTexture(GL_TEXTURE0);
	  glDisable(GL_TEXTURE_2D);
	  glDisable(GL_POINT_SMOOTH);
	}

    }
}

void
PathGroupObject::drawPoints()
{
  Vec col = m_opacity*m_pointColor;
  glColor4f(col.x*0.5,
	    col.y*0.5,
	    col.z*0.5,
	    m_opacity*0.5);

  glEnable(GL_POINT_SPRITE);
  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, Global::spriteTexture());
  glTexEnvf( GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE );
  glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
  glEnable(GL_POINT_SMOOTH);

  Vec voxelScaling = Global::voxelScaling();
  glPointSize(10);
  glBegin(GL_POINTS);
  for(int i=0; i<m_points.count();i++)
    {
      if (m_validPoint[i])
	{
	  Vec pt = VECPRODUCT(m_points[i], voxelScaling);
	  glVertex3dv(pt);
	}
    }
  glEnd();
	  
	  
  if (m_pointPressed > -1)
    {
      glColor3f(1,0,0);
      Vec voxelScaling = Global::voxelScaling();
      glPointSize(15);
      glBegin(GL_POINTS);
      Vec pt = VECPRODUCT(m_points[m_pointPressed], voxelScaling);
      glVertex3dv(pt);
      glEnd();
    }

  glPointSize(1);  

  glDisable(GL_POINT_SPRITE);
  glActiveTexture(GL_TEXTURE0);
  glDisable(GL_TEXTURE_2D);
  
  glDisable(GL_POINT_SMOOTH);
}

void
PathGroupObject::drawTube(QGLViewer *viewer,
			  bool active,
			  Vec lightPosition)
{
  float pos[4];
  float diff[4] = { 1.0, 1.0, 1.0, 1.0 };
  float spec[4] = { 1.0, 1.0, 1.0, 1.0 };
  float shine = 128;

  glEnable(GL_LIGHTING);
  glShadeModel(GL_SMOOTH);
  glEnable(GL_LIGHT0);

  glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
  glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);

  if (shine < 1)
    {
      spec[0] = spec[1] = spec[2] = 0;
    }

  glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, spec);
  glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, &shine);

  glLightfv(GL_LIGHT0, GL_DIFFUSE, diff);

  glEnable(GL_COLOR_MATERIAL);
  glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

  pos[0] = lightPosition.x;
  pos[1] = lightPosition.y;
  pos[2] = lightPosition.z;
  pos[3] = 0;
  glLightfv(GL_LIGHT0, GL_POSITION, pos);

  glColor4f(m_pointColor.x*m_opacity,
	    m_pointColor.y*m_opacity,
	    m_pointColor.z*m_opacity,
	    m_opacity);

  // emissive when active
  if (active)
    {
      float emiss[] = { 0.5, 0.0, 0.0, 1.0 };
      glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emiss);
    }
  else
    {
      float emiss[] = { 0.0, 0.0, 0.0, 1.0 };
      glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emiss);
    }

  glCallList(m_displayList);

  { // reset emissivity
    float emiss[] = { 0.0, 0.0, 0.0, 1.0 };
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emiss);
  }

  glDisable(GL_LIGHTING);
}

void
PathGroupObject::predraw(QGLViewer *viewer,
			 bool active,
			 bool backToFront,		      
			 QList<Vec> cpos,
			 QList<Vec> cnorm,
			 QList<CropObject> crops)
{
  if (m_updateFlag)
    {
      m_updateFlag = false;
      computePathLength();
    }

  m_valid.resize(m_pathIndex.count());
  m_validPoint.resize(m_points.count());

  //-----------------------------------------------------------
  m_valid.fill(false);
  m_validPoint.fill(false);

  // filter points on sparseness
  for(int i=0; i<m_points.count(); i+=m_sparseness)
    m_validPoint[i] = true;

  // filter paths on sparseness
  for (int ci=0; ci<m_pathIndex.count()-1; ci+=m_sparseness)
    m_valid[ci] = true;
  //-----------------------------------------------------------

  //-----------------------------------------------------------
  // filter based on user limits
  if (m_filterPathLen)
    {
      for (int ci=0; ci<m_pathIndex.count()-1; ci+=m_sparseness)
	if (m_valid[ci])
	  {
	    float pl = m_pathLength[ci];
	    if (pl < m_minUserPathLen || pl > m_maxUserPathLen)
	      m_valid[ci] = false;
	  }
    }
  //-----------------------------------------------------------

  //-----------------------------------------------------------
  if (m_clip)
    {
      Vec bmin, bmax;
      Global::bounds(bmin, bmax);

      Vec voxelScaling = Global::voxelScaling();

      // filter points on clip/crop
      for(int i=0; i<m_points.count();i+=m_sparseness)
	{
	  if (m_validPoint[i])
	    {
	      bool ok = true;
	      Vec pt = VECPRODUCT(m_points[i], voxelScaling);
	      if (pt.x >= bmin.x &&
		  pt.y >= bmin.y &&
		  pt.z >= bmin.z &&
		  pt.x <= bmax.x &&
		  pt.y <= bmax.y &&
		  pt.z <= bmax.z)
		{
		  for (int ci=0; ci<cpos.count(); ci++)
		    {
		      if ((pt-cpos[ci])*cnorm[ci] >= 0)
			{
			  ok = false;
			  break;
			}
		    }
		  if (ok)
		    {
		      for(int ci=0; ci<crops.count(); ci++)
			{
			  ok &= crops[ci].checkCropped(pt);
			  if (!ok) break;
			}
		    }
		}
	      else
		ok = false;
	      
	      m_validPoint[i] = ok;
	    }
	}

      // filter paths on clip/crop
      for (int ci=0; ci<m_pathIndex.count()-1; ci+=m_sparseness)
	{
	  if (m_valid[ci])
	    {
	      bool ok = true;
	      int sidx = m_pathIndex[ci];
	      int eidx = m_pathIndex[ci+1];
	      for(int i=sidx; i<eidx; i++)
		{
		  Vec pt = m_path[i];
		  if (pt.x >= bmin.x &&
		      pt.y >= bmin.y &&
		      pt.z >= bmin.z &&
		      pt.x <= bmax.x &&
		      pt.y <= bmax.y &&
		      pt.z <= bmax.z)
		    {
		      for (int ci=0; ci<cpos.count(); ci++)
			{
			  if ((pt-cpos[ci])*cnorm[ci] >= 0)
			    {
			      ok = false;
			      break;
			    }
			}
		      if (ok)
			{
			  for(int ci=0; ci<crops.count(); ci++)
			    {
			      ok &= crops[ci].checkCropped(pt);
			      if (!ok) break;
			    }
			}
		    }
		  else
		    ok = false;
		  
		  if (!ok) break;
		}
	      
	      m_valid[ci] = ok;
	    }
	}
    }
  //-----------------------------------------------------------

  //-----------------------------------------------------------
  if (m_separation > 0)
    {
      int wd = viewer->width();
      int ht = viewer->height();
      QBitArray vbit;
      vbit.resize(wd*ht);
      vbit.fill(false);

      // filter paths based on screen separation
      for (int ci=0; ci<m_pathIndex.count()-1; ci+=m_sparseness)
	{
	  if (m_valid[ci])
	    {
	      bool ok = true;
	      int sidx = m_pathIndex[ci];
	      int eidx = m_pathIndex[ci+1];

	      int sep = m_separation;
	      if (m_capType == ARROW && m_showPoints)
		sep *= qMax(1.0f, m_pathLength[ci]/2);
	      for(int i=sidx; i<eidx; i++)
		{
		  Vec pt = m_path[i];
		  Vec sc0 = viewer->camera()->projectedCoordinatesOf(pt);
		  int x0 = qMax(0,(int)(sc0.x-sep));
		  int x1 = qMin(wd-1,(int)(sc0.x+sep));
		  int y0 = qMax(0,(int)(sc0.y-sep));
		  int y1 = qMin(ht-1,(int)(sc0.y+sep));

		  for(int ax=x0; ax<=x1; ax++)
		    for(int ay=y0; ay<=y1; ay++)
		      {
			int aidx = ay*wd+ax;
			if (vbit.testBit(aidx))
			  {
			    ok = false;
			    break;
			  }
		      }

		  if (!ok)
		    break;
		}

	      m_valid[ci] = ok;

	      if (ok)
		{
		  for(int i=sidx; i<eidx; i++)
		    {
		      Vec pt = m_path[i];
		      Vec sc0 = viewer->camera()->projectedCoordinatesOf(pt);
		      int x0 = qMax(0,(int)(sc0.x-m_separation));
		      int x1 = qMin(wd-1,(int)(sc0.x+m_separation));
		      int y0 = qMax(0,(int)(sc0.y-m_separation));
		      int y1 = qMin(ht-1,(int)(sc0.y+m_separation));
		      for(int ax=x0; ax<=x1; ax++)
			for(int ay=y0; ay<=y1; ay++)
			  {
			    int aidx = ay*wd+ax;
			    vbit.setBit(aidx);
			  }
		    }
		}
	    }	  
	}
    }
  //-----------------------------------------------------------


  //-----------------------------------------------------------
  m_minDepth = 10;
  m_maxDepth = -10;  
  for (int ci=0; ci<m_pathIndex.count()-1; ci+=m_sparseness)
    if (m_valid[ci])
      {
	Vec pt = m_path[m_pathIndex[ci]];
	Vec sc0 = viewer->camera()->projectedCoordinatesOf(pt);
	m_minDepth = qMin(m_minDepth, (float)sc0.z);
	m_maxDepth = qMax(m_maxDepth, (float)sc0.z);
      }
  m_minDepth = qMax(0.0f, m_minDepth);
  m_maxDepth = qMin(1.0f, m_maxDepth);
  //-----------------------------------------------------------


  //-----------------------------------------------------------
  if (m_displayList == 0)
    glDeleteLists(m_displayList, 1); 

  generateSphereSprite();


  // generate the displaylist
  m_displayList = glGenLists(1);
  glNewList(m_displayList, GL_COMPILE);
  if (m_tube)
    generateTube(viewer, 1.0);
  else
    {      
      if (m_capType == ARROW && m_showPoints)
	drawSphereSprites(viewer, active, backToFront);
      else
	drawLines(viewer, active, backToFront);
    }
  glEndList();
  //-----------------------------------------------------------
}

void
PathGroupObject::generateSphereSprite()
{
  if (m_spriteTexture)
    return;

  int texsize = 64;
  int t2 = texsize/2;
  QRadialGradient rg(t2, t2, t2-1, t2, t2);
  rg.setColorAt(0.0, Qt::white);
  rg.setColorAt(1.0, Qt::black);

  QImage texImage(texsize, texsize, QImage::Format_ARGB32);
  texImage.fill(0);
  QPainter p(&texImage);
  p.setBrush(QBrush(rg));
  p.setPen(Qt::transparent);
  p.drawEllipse(0, 0, texsize, texsize);

  uchar *thetexture = new uchar[2*texsize*texsize];
  const uchar *bits = texImage.bits();
  for(int i=0; i<texsize*texsize; i++)
    {
      uchar lum = 255;
      float a = (float)bits[4*i+2]/255.0f;
      a = 1-a;
      
      if (a < 0.5)
	{
	  a = 0.7f;
	  lum = 50;
	}
      else if (a >= 1)
	{
	  a = 0;
	  lum = 0;
	}
      else
	{
	  lum *= 1-fabs(a-0.75f)/0.25f;
	  a = 0.9f;
	}
      
      a *= 255;
      
      thetexture[2*i] = lum;
      thetexture[2*i+1] = a;
    }
  
  glGenTextures(1, &m_spriteTexture);

  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, m_spriteTexture);      
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA,
	       texsize, texsize, 0,
	       GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE,
	       thetexture);
  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

  delete [] thetexture;
}

void
PathGroupObject::drawSphereSprites(QGLViewer *viewer,
				   bool active,
				   bool backToFront)
{
  glEnable(GL_BLEND);

  int stopsCount = m_resampledStops.count()-1;
  int nci = m_pathIndex.count()-1;
  float deno = 1;
  if (m_minUserPathLen < m_maxUserPathLen)
    deno = m_maxUserPathLen-m_minUserPathLen;

  float depthdeno = m_minDepth-m_maxDepth;
  if (qAbs(depthdeno) < 0.000001)
    depthdeno = 1;

  Vec voxelScaling = Global::voxelScaling();

  for (int ci=0; ci<nci; ci++)
    {
      if (m_valid[ci])
	{
	  int sidx = m_pathIndex[ci];
	  
	  //----------------------------------
	  // define color
	  float frc;
	  if (m_arrowDirection)
	    frc = (m_pathLength[ci]-m_minUserPathLen)/deno;
	  else
	    frc = (m_pathLength[nci-1-ci]-m_minUserPathLen)/deno;
	  frc = qBound(0.0f, frc, 1.0f);
	  QColor col = m_resampledStops[frc*stopsCount].second;
	  float r = m_opacity*col.red()/255.0;
	  float g = m_opacity*col.green()/255.0;
	  float b = m_opacity*col.blue()/255.0;
	  float a = m_opacity;
	  if (m_depthcue)
	    {
	      float scd = viewer->camera()->projectedCoordinatesOf(m_path[sidx]).z;
	      float darken = qMin(1.0f, 0.2f + (scd-m_maxDepth)/depthdeno);
	      r *= darken;
	      g *= darken;
	      b *= darken;
	    }
	  glColor4f(r,g,b,a);	     
	  //----------------------------------
	  
	  float psz;
	  if (m_scaleType) // absolute scale
	    psz = m_maxScale*m_pathLength[ci];
	  else // relative scale
	    psz = (1-frc)*m_minScale + frc*m_maxScale;
	  glPointSize(qMax(1.0f, psz));
	  glBegin(GL_POINTS);
	  Vec pt = VECPRODUCT(m_path[sidx], voxelScaling);
	  glVertex3dv(pt);
	  glEnd();
	}
    }
  
  glPointSize(1);  

  return;
}

void
PathGroupObject::drawLines(QGLViewer *viewer,
			   bool active,
			   bool backToFront)
{
  glEnable(GL_BLEND);

  int stopsCount = m_resampledStops.count()-1;
  int nci = m_pathIndex.count()-1;
  float deno = 1;
  if (m_minUserPathLen < m_maxUserPathLen)
    deno = m_maxUserPathLen-m_minUserPathLen;

  float depthdeno = m_minDepth-m_maxDepth;
  if (qAbs(depthdeno) < 0.000001)
    depthdeno = 1;

  glEnable(GL_LINE_SMOOTH);
  glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

  if (active)
    glLineWidth(3);
  else
    glLineWidth(1);


  //---------------------------------
  // draw arrows
  if (m_capType == ARROW)
    {
      m_animationstep++;
      if (m_animationstep > 1000000) // reset after some steps
	m_animationstep = 0;

      int halfStep = qMax(1, m_segments/2);
      Vec voxelScaling = Global::voxelScaling();
      glBegin(GL_TRIANGLES);
      for (int ci=0; ci<nci; ci++)
	{
	  if (m_valid[ci])
	    {
	      int sidx = m_pathIndex[ci];
	      int ni = (m_pathIndex[ci+1]-m_pathIndex[ci])/m_segments;
	      ni += ((m_pathIndex[ci+1]-m_pathIndex[ci])%m_segments > 0);

	      //----------------------------------
	      // define color
	      float frc;
	      if (m_arrowDirection)
		frc = (m_pathLength[ci]-m_minUserPathLen)/deno;
	      else
		frc = (m_pathLength[nci-1-ci]-m_minUserPathLen)/deno;
	      frc = qBound(0.0f, frc, 1.0f);
	      QColor col = m_resampledStops[frc*stopsCount].second;
	      float r = 0.8*m_opacity*col.red()/255.0;
	      float g = 0.8*m_opacity*col.green()/255.0;
	      float b = 0.8*m_opacity*col.blue()/255.0;
	      float a = 0.8*m_opacity;
	      if (m_depthcue)
		{
		  float scd = viewer->camera()->projectedCoordinatesOf(m_path[sidx]).z;
		  float darken = qMin(1.0f, 0.2f + (scd-m_maxDepth)/depthdeno);
		  r *= darken;
		  g *= darken;
		  b *= darken;
		}
	      glColor4f(r,g,b,a);	     
	      //----------------------------------
	      

	      for(int i=1; i<ni; i++)
		{
		  float a0, a1;
		  if (m_animate)
		    {
		      float aa=(m_animationstep+ci)%m_animateSpeed;
		      a0 = 0.3 + 0.7*(aa/(float)(m_animateSpeed));
		      a1 = a0 - 0.3;
		    }
		  else
		    {
		      a0 = 1;
		      a1 = 0.3;
		    }
 
		  Vec v0, v1, v2;		  
		  v0 = m_path[sidx + i*m_segments];
		  v1 = m_path[sidx + (i-1)*m_segments];
		  v2 = m_path[sidx + i*m_segments-halfStep];

		  Vec sc0, sc1, sc2;
		  sc0 = viewer->camera()->projectedCoordinatesOf(v0);
		  sc1 = viewer->camera()->projectedCoordinatesOf(v1);
		  sc2 = viewer->camera()->projectedCoordinatesOf(v2);
	  
		  Vec dv = (sc0-sc2).unit();
		  float rl = (sc0-sc1).norm();

		  //--------
		  if (m_scaleType) // absolute scale
		    rl = m_maxScale*rl;
		  else // relative scale
		    rl = (1-frc)*m_minScale + frc*m_maxScale;
		  //-------- 

		  v1 = sc1 + a1*rl*dv;

		  float px = (sc1.y-sc0.y);
		  float py = -(sc1.x-sc0.x);
		  float dlen = sqrt(px*px + py*py);
		  px/=dlen; py/=dlen;	  
		  if (m_animate)
		    {
		      px *= qBound(1, (int)(rl/3), 5);
		      py *= qBound(1, (int)(rl/3), 5);
		    }
		  else
		    {
		      px *= qBound(1, (int)(rl/3), 10);
		      py *= qBound(1, (int)(rl/3), 10);
		    }

		  v0 = viewer->camera()->unprojectedCoordinatesOf(sc1+ a0*rl*dv);
		  v2 = viewer->camera()->unprojectedCoordinatesOf(v1 + Vec(px, py, 0));
		  v1 = viewer->camera()->unprojectedCoordinatesOf(v1 - Vec(px, py, 0));

		  glVertex3dv(v0);
		  glVertex3dv(v2);
		  glVertex3dv(v1);
		  
		  if (!m_arrowForAll)
		    break;
		}
	    }
	}
      glEnd();
    }
  //---------------------------------

  for (int ci=0; ci<nci; ci++)
    {
      if (m_valid[ci])
	{
	  int sidx = m_pathIndex[ci];
	  int eidx = m_pathIndex[ci+1];

	  //----------------------------------
	  // define color
	  float frc;
	  if (m_arrowDirection)
	    frc = (m_pathLength[ci]-m_minUserPathLen)/deno;
	  else
	    frc = (m_pathLength[nci-1-ci]-m_minUserPathLen)/deno;
	  frc = qBound(0.0f, frc, 1.0f);
	  QColor col = m_resampledStops[frc*stopsCount].second;
	  float r = m_opacity*col.red()/255.0;
	  float g = m_opacity*col.green()/255.0;
	  float b = m_opacity*col.blue()/255.0;
	  if (m_depthcue)
	    {
	      float scd = viewer->camera()->projectedCoordinatesOf(m_path[sidx]).z;
	      float darken = qMin(1.0f, 0.2f + (scd-m_maxDepth)/depthdeno);
	      r *= darken;
	      g *= darken;
	      b *= darken;
	    }
	  glColor4f(r,g,b,m_opacity);
	  //----------------------------------

	  glBegin(GL_LINE_STRIP);
	  for(int i=sidx; i<eidx; i++)
	    {
	      Vec v0 = m_path[i];
	      if (i > sidx)
		{
		  Vec sc0, sc1;
		  sc0 = viewer->camera()->projectedCoordinatesOf(v0);
		  sc1 = viewer->camera()->projectedCoordinatesOf(m_path[i-1]);
		  Vec dv = (sc0-sc1);
		  float rl = dv.norm();
		  dv.normalize();

		  //--------
		  if (m_scaleType) // absolute scale
		    rl = m_maxScale*rl;
		  else // relative scale
		    rl = (1-frc)*m_minScale + frc*m_maxScale;
		  //-------- 

		  v0 = viewer->camera()->unprojectedCoordinatesOf(sc1+rl*dv);
		}

	      glVertex3dv(v0);
	    }
	  glEnd();
//
//	  glBegin(GL_LINE_STRIP);
//	  for(int i=sidx; i<eidx; i++)
//	    glVertex3dv(m_path[i]);
//	  glEnd();
	}
    }

  Vec col = m_opacity*m_pointColor;
  glColor4f(col.x*0.5,
	    col.y*0.5,
	    col.z*0.5,
	    m_opacity*0.5);


  glLineWidth(1);

  if (m_showPoints && m_pointPressed > -1)
    {
      int i = m_pointPressed;
      
      int npaths = m_points.count();
      QList<Vec> csec;
      Vec tang, xaxis, yaxis;
      Vec ptang, pxaxis, pyaxis;
      
      pxaxis = Vec(1,0,0);
      pyaxis = Vec(0,1,0);
      ptang = Vec(0,0,1);
      
      if (m_closed)
	{
	  if (i==0)
	    tang = m_points[1]-m_points[npaths-1];
	  else if (i == npaths-1)
	    tang = m_points[0]-m_points[npaths-2];
	  else
	    tang = m_points[i+1]-m_points[i-1];
	}
      else
	{
	  if (i== 0)
	    tang = m_points[i+1]-m_points[i];
	  else if (i== npaths-1)
	    tang = m_points[i]-m_points[i-1];
	  else
	    tang = m_points[i+1]-m_points[i-1];
	}
      
      if (tang.norm() > 0)
	tang.normalize();
      else
	tang = Vec(1,0,0); // should really scold the user

      if (m_closed && i==npaths-1)
	{ // restore settings of zeroeth point
	  pxaxis = Vec(1,0,0);
	  pyaxis = Vec(0,1,0);
	  ptang = Vec(0,0,1);
	}
      
      csec = getCrossSection(1.0,
			     m_pointAngle[i], m_pointRadX[i], m_pointRadY[i],
			     m_sections,
			     ptang, pxaxis, pyaxis,
			     tang, xaxis, yaxis);
      
      for(int j=0; j<csec.count(); j++)
	csec[j] += m_points[m_pointPressed];
      
      glBegin(GL_LINE_STRIP);
      for(int j=0; j<csec.count(); j++)
	glVertex3dv(csec[j]);
      glEnd();
    }
  
  glDisable(GL_LINE_SMOOTH);
}

void
PathGroupObject::generateTube(QGLViewer *viewer,
			      float scale)
{
  QList<Vec> csec1, norm1;
  Vec ptang, pxaxis, pyaxis;

  pxaxis = Vec(1,0,0);
  pyaxis = Vec(0,1,0);
  ptang = Vec(0,0,1);


  int stopsCount = m_resampledStops.count()-1;
  int nci = m_pathIndex.count()-1;
  float deno = 1;
//  if (m_minPathLen < m_maxPathLen)
//    deno = m_maxPathLen-m_minPathLen;
  if (m_minUserPathLen < m_maxUserPathLen)
    deno = m_maxUserPathLen-m_minUserPathLen;

  float depthdeno = m_minDepth-m_maxDepth;
  if (qAbs(depthdeno) < 0.000001)
    depthdeno = 1;

  for (int ci=0; ci<nci; ci++)
    {
      if (m_valid[ci])
	{
	  int sidx = m_pathIndex[ci];
	  int eidx = m_pathIndex[ci+1];

	  float frc;
	  if (m_arrowDirection)
	    frc = (m_pathLength[ci]-m_minUserPathLen)/deno;
	  else
	    frc = (m_pathLength[nci-1-ci]-m_minUserPathLen)/deno;
	  frc = qBound(0.0f, frc, 1.0f);
	  QColor col = m_resampledStops[frc*stopsCount].second;
	  float r = m_opacity*col.red()/255.0;
	  float g = m_opacity*col.green()/255.0;
	  float b = m_opacity*col.blue()/255.0;
	  if (m_depthcue)
	    {
	      float scd = viewer->camera()->projectedCoordinatesOf(m_path[sidx]).z;
	      float darken = qMin(1.0f, 0.2f + (scd-m_maxDepth)/depthdeno);
	      r *= darken;
	      g *= darken;
	      b *= darken;
	    }
	  glColor4f(r,g,b, m_opacity);

	  int nextArrowIdx=sidx;
	  Vec nextArrowHead;
	  float nextArrowHeight;

	  for(int i=sidx; i<eidx; i++)
	    {
	      QList<Vec> csec2, norm2;
	      Vec tang, xaxis, yaxis;
	      
	      if (m_closed)
		{
		  if (i==sidx || i == eidx-1)
		    // both points are actually the same
		    tang = m_path[sidx+1]-m_path[eidx-2];
		  else
		    tang = m_path[i+1]-m_path[i-1];
		}
	      else
		{
		  if (i== sidx)
		    tang = m_path[i+1]-m_path[i];
		  else if (i== eidx-1)
		    tang = m_path[i]-m_path[i-1];
		  else
		    tang = m_path[i+1]-m_path[i-1];
		}
	      
	      if (tang.norm() > 0)
		tang.normalize();
	      else
		tang = Vec(1,0,0); // should really scold the user
	      
	      if (m_closed && i==eidx-1)
		{ // restore settings of zeroeth point
		  pxaxis = Vec(1,0,0);
		  pyaxis = Vec(0,1,0);
		  ptang = Vec(0,0,1);
		}
	      
	      
	      //---------------------------------------------
	      csec2 = getCrossSection(scale,
				      m_angle[i], m_radX[i], m_radY[i],
				      m_sections,
				      ptang, pxaxis, pyaxis,
				      tang, xaxis, yaxis);
	      norm2 = getNormals(csec2, tang);
	      //---------------------------------------------
	      
	      
	      //---------------------------------------------
	      if (m_capType == FLAT)
		{
		  if (!m_closed && (i == sidx || i==eidx-1))
		    addFlatCaps(i, tang, csec2);
		}
	      //---------------------------------------------
	      
	      //---------------------------------------------
	      if (m_capType == ROUND)
		{
		  if (!m_closed && (i == sidx || i==eidx-1))
		    addRoundCaps(i, tang, csec2, norm2);
		}
	      //---------------------------------------------
	      
	      
	      
	      //---------------------------------------------
	      // generate the tubular mesh
	      if (i > sidx)
		{
		  //---------------------------------------
		  // extend based on scale
		  Vec v0 = m_path[i];
		  Vec sc0, sc1;
		  sc0 = viewer->camera()->projectedCoordinatesOf(v0);
		  sc1 = viewer->camera()->projectedCoordinatesOf(m_path[i-1]);
		  Vec dv = (sc0-sc1);
		  float rl = dv.norm();
		  dv.normalize();
		  //--------
		  if (m_scaleType) // absolute scale
		    rl = m_maxScale*rl;
		  else // relative scale
		    rl = (1-frc)*m_minScale + frc*m_maxScale;
		  //-------- 
		  v0 = viewer->camera()->unprojectedCoordinatesOf(sc1+rl*dv);
		  //---------------------------------------

		  float frc1 = 2;
		  float frc2 = 2;
		  if (m_capType == ARROW)
		    {
		      if (m_arrowForAll || i == eidx-1) nextArrowHead = v0;
		      frc1 = (m_path[i-1]-nextArrowHead).norm()/nextArrowHeight;
		      frc2 = (v0-nextArrowHead).norm()/nextArrowHeight;
		    }
		  if (frc1 >= 1 && frc2 < 1)
		    addArrowHead(i, scale,
				 nextArrowHead,
				 ptang, pxaxis, pyaxis,
				 frc1, frc2, v0,
				 csec1, norm1);
		  else if (frc2 >= 1)
		    {
		      glBegin(GL_TRIANGLE_STRIP);
		      for(int j=0; j<m_sections+1; j++)
			{
			  Vec vox1 = m_path[i-1] + csec1[j];
			  glNormal3dv(norm1[j]);
			  glVertex3dv(vox1);
			  
			  Vec vox2 = v0 + csec2[j];
			  glNormal3dv(norm2[j]);
			  glVertex3dv(vox2);
			}	      
		      glEnd();
		    }
		}
	      //---------------------------------------------
	      
	      ptang = tang;
	      pxaxis = xaxis;
	      pyaxis = yaxis;
	      csec1 = csec2;
	      norm1 = norm2;      
	      
	      if (m_capType == ARROW)
		{
		  // calculate nextArrowHead
		  if (!m_arrowForAll && i == sidx)
		    {
		      if (!m_closed)
			{
			  nextArrowIdx = eidx-1;
			  nextArrowHead = m_path[eidx-1];
			  nextArrowHeight = 2*qMax(m_radX[eidx-1],
						   m_radY[eidx-1]);
			}
		      else
			{
			  nextArrowIdx = eidx-2;
			  nextArrowHead = m_path[eidx-2];
			  nextArrowHeight = 2*qMax(m_radX[eidx-2],
						   m_radY[eidx-2]);
			}
		    }
		  else if (m_arrowForAll)
		    {
		      if ((i-sidx)%m_segments == 0)
			{
			  nextArrowIdx += m_segments;
			  if (nextArrowIdx > eidx-1)
			    nextArrowIdx = eidx-1;
			  nextArrowHead = m_path[nextArrowIdx];
			  nextArrowHeight = 2*qMax(m_radX[nextArrowIdx],
						   m_radY[nextArrowIdx]);
			}
		    }
		}
	      
	    }
	  
	  csec1.clear();
	  norm1.clear();
	}
    }
}

void
PathGroupObject::addFlatCaps(int i,
			     Vec tang,
			     QList<Vec> csec)
{
  Vec norm = -tang;
  int halfway = m_sections/2;
  glBegin(GL_TRIANGLE_STRIP);
  for(int j=0; j<=halfway; j++)
    {
      Vec vox2 = m_path[i] + csec[j];
      glNormal3dv(norm);
      glVertex3dv(vox2);

      if (j < halfway)
	{
	  vox2 = m_path[i] + csec[m_sections-j];
	  glNormal3dv(norm);
	  glVertex3dv(vox2);
	}
    }
  glEnd();
}

void
PathGroupObject::addRoundCaps(int i,
			      Vec tang,
			      QList<Vec> csec2,
			      QList<Vec> norm2)
{
  int npaths = m_path.count();
  int ksteps = 4;
  Vec ctang = -tang;
  if (i==0) ctang = tang;
  QList<Vec> csec = csec2;
  float rad = qMin(m_radX[i], m_radY[i]);
  glBegin(GL_TRIANGLE_STRIP);	  
  for(int k=0; k<ksteps-1; k++)
    {
      float ct1 = cos(1.57*(float)k/(float)ksteps);
      float ct2 = cos(1.57*(float)(k+1)/(float)ksteps);
      float st1 = sin(1.57*(float)k/(float)ksteps);
      float st2 = sin(1.57*(float)(k+1)/(float)ksteps);
      for(int j=0; j<m_sections+1; j++)
	{
	  Vec norm = csec2[j]*ct1 - ctang*rad*st1;
	  Vec vox2 = m_path[i] + norm;
	  if (k==0)
	    {
	      if (i==0)
		glNormal3dv(-norm2[j]);
	      else
		glNormal3dv(norm2[j]);
	    }
	  else
	    {  
	      norm.normalize();
	      if (i==npaths-1) norm=-norm;
	      glNormal3dv(norm);
	    }
	  glVertex3dv(vox2);
	  
	  norm = csec2[j]*ct2 - ctang*rad*st2;
	  vox2 = m_path[i] + norm;
	  norm.normalize();
	  if (i==npaths-1) norm=-norm;
	  glNormal3dv(norm);
	  glVertex3dv(vox2);
	}
    }
  glEnd();
  
  // add flat ends
  float ct2 = cos(1.57*(float)(ksteps-1)/(float)ksteps);
  float st2 = sin(1.57*(float)(ksteps-1)/(float)ksteps);
  int halfway = m_sections/2;
  glBegin(GL_TRIANGLE_STRIP);
  for(int j=0; j<=halfway; j++)
    {
      Vec norm = csec2[j]*ct2 - ctang*rad*st2;
      Vec vox2 = m_path[i] + norm;
      norm.normalize();
      if (i==npaths-1) norm=-norm;
      glNormal3dv(norm);
      glVertex3dv(vox2);
      
      if (j < halfway)
	{
	  norm = csec2[m_sections-j]*ct2 -
	                      ctang*rad*st2;
	  vox2 = m_path[i] + norm;
	  norm.normalize();
	  if (i==npaths-1) norm=-norm;
	  glNormal3dv(norm);
	  glVertex3dv(vox2);
	}
    }
  glEnd();
}

void
PathGroupObject::addArrowHead(int i, float scale,
			      Vec nextArrowHead,
			      Vec ptang, Vec pxaxis, Vec pyaxis,
			      float frc1, float frc2, Vec v0,
			      QList<Vec> csec1,
			      QList<Vec> norm1)
{
  //---------------------------------------------
  Vec tangm, xaxism, yaxism;
  tangm = m_path[i]-m_path[i-1];
  if (tangm.norm() > 0)
    tangm.normalize();
  else
    tangm = Vec(1,0,0); // should really scold the user

  QList<Vec> csecm, normm;
  csecm = getCrossSection(scale,
			  m_angle[i], m_radX[i], m_radY[i],
			  m_sections,
			  ptang, pxaxis, pyaxis,
			  tangm, xaxism, yaxism);
  normm = getNormals(csecm, tangm);
  //---------------------------------------------
  
  float t = (1.0-frc2)/(frc1-frc2);
  Vec mid = v0 + t*(m_path[i-1]-v0);

  glBegin(GL_TRIANGLE_STRIP);
  for(int j=0; j<m_sections+1; j++)
    {
      Vec vox1 = m_path[i-1] + csec1[j];
      glNormal3dv(norm1[j]);
      glVertex3dv(vox1);
      
      Vec vox2 = mid + csecm[j];
      glNormal3dv(normm[j]);
      glVertex3dv(vox2);
    }
  glEnd();

  glBegin(GL_TRIANGLE_STRIP);
  for(int j=0; j<m_sections+1; j++)
    {
      Vec vox1 = mid + csecm[j];
      glNormal3dv(normm[j]);
      glVertex3dv(vox1);
      
      Vec vox2 = mid + 2*csecm[j];
      glNormal3dv(normm[j]);
      glVertex3dv(vox2);
    }
  glEnd();

  glBegin(GL_TRIANGLE_STRIP);
  for(int j=0; j<m_sections+1; j++)
    {
      Vec vox1 = mid + 2*csecm[j];
      glNormal3dv(normm[j]);
      glVertex3dv(vox1);
      
      Vec vox2 = nextArrowHead;
      glNormal3dv(tangm);
      glVertex3dv(vox2);
    }
  glEnd();
}

QList<Vec>
PathGroupObject::getCrossSection(float scale,
				 float offsetAngle, float a, float b,
				 int sections,
				 Vec ptang, Vec pxaxis, Vec pyaxis,
				 Vec tang, Vec &xaxis, Vec &yaxis)
{
  Vec axis;
  float angle;

  StaticFunctions::getRotationBetweenVectors(ptang, tang, axis, angle);
  Quaternion q(axis, angle);

  xaxis = q.rotate(pxaxis);
  yaxis = q.rotate(pyaxis);
      
  Vec voxelScaling = Global::voxelScaling();

  QList<Vec> csec;
  for(int j=0; j<sections; j++)
    {
      float t = (float)j/(float)sections;

      // change 't' to get a smoother cross-section
      if (j<0.25*sections && j>0)
	{
	  if (a/b > 1) t -= (1-b/a)/sections;
	  else if (b/a > 1) t += (1-a/b)/sections;
	}
      else if (j<0.5*sections && j>0.25*sections)
	{
	  if (b/a > 1) t -= (1-a/b)/sections;
	  else if (a/b > 1) t += (1-b/a)/sections;
	}
      else if (j<0.75*sections && j>0.5*sections)
	{
	  if (a/b > 1) t -= (1-b/a)/sections;
	  else if (b/a > 1) t += (1-a/b)/sections;
	}
      else if (j<sections && j>0.75*sections)
	{
	  if (b/a > 1) t -= (1-a/b)/sections;
	  else if (a/b > 1) t += (1-b/a)/sections;
	}

      float st = a*sin(offsetAngle + 6.2831853*t);
      float ct = b*cos(offsetAngle + 6.2831853*t);
      float r = (a*b)/sqrt(ct*ct + st*st);
      float x = r*cos(6.2831853*t)*scale;
      float y = r*sin(6.2831853*t)*scale;
      Vec v = x*xaxis + y*yaxis;
      v = VECPRODUCT(v, voxelScaling);
      csec.append(v);
    }
  csec.append(csec[0]);

  return csec;
}

QList<Vec>
PathGroupObject::getNormals(QList<Vec> csec, Vec tang)
{
  QList<Vec> norm;
  int sections = csec.count();
  for(int j=0; j<sections; j++)
    {
      Vec v;
      if (j==0 || j==sections-1)
	v = csec[1]-csec[sections-2];
      else
	v = csec[j+1]-csec[j-1];
      
      v.normalize();
      v = tang^v;

      norm.append(v);
    }

  return norm;
}

void
PathGroupObject::postdraw(QGLViewer *viewer,
			  int x, int y,
			  bool grabsMouse)
{
  if (!grabsMouse && !m_showPoints)
    return;

  if (!m_showLength)
    return;

  QList<Vec> pc;
  for(int ci=0; ci<m_pathIndex.count()-1; ci++)
    {
      int sidx = m_pathIndex[ci];
      int eidx = m_pathIndex[ci+1];
      Vec pc0 = viewer->camera()->projectedCoordinatesOf(m_path[sidx]);
      Vec pc1 = viewer->camera()->projectedCoordinatesOf(m_path[eidx-1]);
      pc << pc0;
      pc << pc1;
    }
  
  viewer->startScreenCoordinatesSystem();
  
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // blend on top
  
  Vec voxelScaling = Global::voxelScaling();

  if (m_arrowDirection)
    {  
      for(int ci=0; ci<m_pathIndex.count()-1; ci++)
	drawImage(viewer, ci, pc[2*ci], pc[2*ci+1]);
    }
  else
    {  
      int nci = m_pathIndex.count()-1;
      for(int ci=0; ci<nci; ci++)
	drawImage(viewer, nci-1-ci, pc[2*ci], pc[2*ci+1]);
    }
  
  viewer->stopScreenCoordinatesSystem();
}

void
PathGroupObject::drawImage(QGLViewer *viewer,
			   int ci,
			   Vec pp0, Vec pp1)
{
  QImage cimage = m_cImage[ci];

  int screenWidth = viewer->size().width();
  int screenHeight = viewer->size().height();

  float frcw = (float(viewer->camera()->screenWidth())/
		float(screenWidth));
  float frch = (float(viewer->camera()->screenHeight())/
		float(screenHeight));

  pp0.x /= frcw;
  pp0.y /= frch;
  pp1.x /= frcw;
  pp1.y /= frch;

  int px = pp0.x + 10;
  if (pp0.x < pp1.x)
    px = pp0.x - cimage.width() - 10;
  int py = pp0.y + cimage.height()/2;
      
  if (px < 0 || py > screenHeight)
    {
      int wd = cimage.width();
      int ht = cimage.height();
      int sx = 0;
      int sy = 0;
      if (px < 0)
	{
	  wd = cimage.width()+px;
	  sx = -px;
	  px = 0;
	}
      if (py > screenHeight)
	{
	  ht = cimage.height()-(py-screenHeight);
	  sy = (py-screenHeight);
	  py = screenHeight;
	}
      
      cimage = cimage.copy(sx, sy, wd, ht);
    }
  
  cimage = cimage.scaled(cimage.width()*frcw,
			 cimage.height()*frch);
  
  const uchar *bits = cimage.bits();
  
  glRasterPos2i(px, py);
  glDrawPixels(cimage.width(), cimage.height(),
	       GL_RGBA,
	       GL_UNSIGNED_BYTE,
	       bits);
}

void
PathGroupObject::generateImages()
{
  VolumeInformation pvlInfo = VolumeInformation::volumeInformation();
  
  m_cImage.clear();

  for(int ci=0; ci<m_pathIndex.count()-1; ci++)
    {
      m_indexGrabbed = ci;
      float len = m_pathLength[ci];
      QString str = QString("%1 %2").				\
	            arg(len, 0, 'f', Global::floatPrecision()).	\
	            arg(pvlInfo.voxelUnitStringShort()); 

      QFontMetrics metric(m_captionFont);
      int mde = metric.descent();
      int fht = metric.height();
      int fwd = metric.width(str)+2;

      //-------------------
      QImage bImage = QImage(fwd, fht, QImage::Format_ARGB32);
      bImage.fill(0);
      {
	QPainter bpainter(&bImage);
	// we have image as ARGB, but we want RGBA
	// so switch red and blue colors here itself
	QColor penColor(m_captionHaloColor.blue(),
			m_captionHaloColor.green(),
			m_captionHaloColor.red());
	// do not use alpha(),
	// opacity will be modulated using clip-plane's opacity parameter  
	bpainter.setPen(penColor);
	bpainter.setFont(m_captionFont);
	bpainter.drawText(1, fht-mde, str);

	uchar *dbits = new uchar[4*fht*fwd];
	uchar *bits = bImage.bits();
	for(int nt=0; nt < 4; nt++)
	  {
	    memcpy(dbits, bits, 4*fht*fwd);

	    for(int i=2; i<fht-2; i++)
	      for(int j=2; j<fwd-2; j++)
		{
		  for (int k=0; k<4; k++)
		    {
		      int sum = 0;
		      
		      for(int i0=-2; i0<=2; i0++)
			for(int j0=-2; j0<=2; j0++)
			  sum += dbits[4*((i+i0)*fwd+(j+j0)) + k];
		      
		      bits[4*(i*fwd+j) + k] = sum/25;
		    }
		}
	  }
	delete [] dbits;
      }
      //-------------------

      QImage cImage = QImage(fwd, fht, QImage::Format_ARGB32);
      cImage.fill(0);
      QPainter cpainter(&cImage);

      // first draw the halo image
      cpainter.drawImage(0, 0, bImage);

      // we have image as ARGB, but we want RGBA
      // so switch red and blue colors here itself
      QColor penColor(m_captionColor.blue(),
		      m_captionColor.green(),
		      m_captionColor.red());
      // do not use alpha(),
      // opacity will be modulated using clip-plane's opacity parameter  
      cpainter.setPen(penColor);
      cpainter.setFont(m_captionFont);
      cpainter.drawText(1, fht-mde, str);  

      m_cImage << cImage.mirrored();
    }
}

void
PathGroupObject::save(fstream& fout)
{
  char keyword[100];
  float f[3];

  memset(keyword, 0, 100);
  sprintf(keyword, "pathgroupobjectstart");
  fout.write((char*)keyword, strlen(keyword)+1);

  memset(keyword, 0, 100);
  sprintf(keyword, "allowinterpolate");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_allowInterpolate, sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "blendmode");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_blendMode, sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "clip");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_clip, sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "minscale");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_minScale, sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "maxscale");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_maxScale, sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "scaletype");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_scaleType, sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "filterpathlen");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_filterPathLen, sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "showpoints");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_showPoints, sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "showlength");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_showLength, sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "depthcue");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_depthcue, sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "tube");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_tube, sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "closed");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_closed, sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "captype");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_capType, sizeof(int));

  memset(keyword, 0, 100);
  sprintf(keyword, "arrowdirection");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_arrowDirection, sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "arrowforall");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_arrowForAll, sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "sections");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_sections, sizeof(int));

  memset(keyword, 0, 100);
  sprintf(keyword, "segments");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_segments, sizeof(int));

  memset(keyword, 0, 100);
  sprintf(keyword, "sparseness");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_sparseness, sizeof(int));

  memset(keyword, 0, 100);
  sprintf(keyword, "separation");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_separation, sizeof(int));

  memset(keyword, 0, 100);
  sprintf(keyword, "animate");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_animate, sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "animatespeed");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_animateSpeed, sizeof(int));

  memset(keyword, 0, 100);
  sprintf(keyword, "stops");
  fout.write((char*)keyword, strlen(keyword)+1);
  int nstops = m_stops.count();
  fout.write((char*)&nstops, sizeof(int));
  for(int ni=0; ni<nstops; ni++)
    {
      float st[5];
      qreal r,g,b,a;
      QColor col;
      st[0] = m_stops[ni].first;
      col = m_stops[ni].second;
      col.getRgbF(&r,&g,&b,&a);
      st[1] = r;
      st[2] = g;
      st[3] = b;
      st[4] = a;
      fout.write((char*)st, 5*sizeof(float));
    }

  memset(keyword, 0, 100);
  sprintf(keyword, "opacity");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_opacity, sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "userpathlenminmax");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_minUserPathLen, sizeof(float));
  fout.write((char*)&m_maxUserPathLen, sizeof(float));

  int nidx = m_index.count();
  memset(keyword, 0, 100);
  sprintf(keyword, "index");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&nidx, sizeof(int));
  for(int i=0; i<nidx; i++)
    fout.write((char*)&m_index[i], sizeof(int));

  int npts = m_points.count();
  memset(keyword, 0, 100);
  sprintf(keyword, "points");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&npts, sizeof(int));
  for(int i=0; i<npts; i++)
    {
      f[0] = m_points[i].x;
      f[1] = m_points[i].y;
      f[2] = m_points[i].z;
      fout.write((char*)&f, 3*sizeof(float));
    }

  memset(keyword, 0, 100);
  sprintf(keyword, "crosssection");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&npts, sizeof(int));
  for(int i=0; i<npts; i++)
    {
      f[0] = m_pointRadX[i];
      f[1] = m_pointRadY[i];
      f[2] = m_pointAngle[i];
      fout.write((char*)&f, 3*sizeof(float));
    }

  memset(keyword, 0, 100);
  sprintf(keyword, "captionfont");
  fout.write((char*)keyword, strlen(keyword)+1);
  QString fontStr = m_captionFont.toString();
  int len = fontStr.size()+1;
  fout.write((char*)&len, sizeof(int));
  if (len > 0)	
    fout.write((char*)fontStr.toAscii().data(), len*sizeof(char));
  
  memset(keyword, 0, 100);
  sprintf(keyword, "captioncolor");
  fout.write((char*)keyword, strlen(keyword)+1);
  unsigned char r = m_captionColor.red();
  unsigned char g = m_captionColor.green();
  unsigned char b = m_captionColor.blue();
  unsigned char a = m_captionColor.alpha();
  fout.write((char*)&r, sizeof(unsigned char));
  fout.write((char*)&g, sizeof(unsigned char));
  fout.write((char*)&b, sizeof(unsigned char));
  fout.write((char*)&a, sizeof(unsigned char));

  memset(keyword, 0, 100);
  sprintf(keyword, "captionhalocolor");
  fout.write((char*)keyword, strlen(keyword)+1);
  r = m_captionHaloColor.red();
  g = m_captionHaloColor.green();
  b = m_captionHaloColor.blue();
  a = m_captionHaloColor.alpha();
  fout.write((char*)&r, sizeof(unsigned char));
  fout.write((char*)&g, sizeof(unsigned char));
  fout.write((char*)&b, sizeof(unsigned char));
  fout.write((char*)&a, sizeof(unsigned char));

  memset(keyword, 0, 100);
  sprintf(keyword, "end");
  fout.write((char*)keyword, strlen(keyword)+1);
}

void
PathGroupObject::load(fstream &fin)
{  
  m_index.clear();
  m_points.clear();
  m_pointRadX.clear();
  m_pointRadY.clear();
  m_pointAngle.clear();
  m_tgP.clear();
  m_path.clear();
  m_radX.clear();
  m_radY.clear();
  m_angle.clear();
  m_captionFont = QFont("Helvetica", 12);
  m_captionColor = Qt::white;
  m_captionHaloColor = Qt::black;
  m_minScale = m_maxScale = 1;
  m_scaleType = true;

  if (m_spriteTexture)
    glDeleteTextures( 1, &m_spriteTexture );
  m_spriteTexture = 0;

  bool done = false;
  char keyword[100];
  float f[3];
  while (!done)
    {
      fin.getline(keyword, 100, 0);

      if (strcmp(keyword, "end") == 0)
	done = true;
      else if (strcmp(keyword, "allowinterpolate") == 0)
	fin.read((char*)&m_allowInterpolate, sizeof(bool));
      else if (strcmp(keyword, "blendmode") == 0)
	fin.read((char*)&m_blendMode, sizeof(bool));
      else if (strcmp(keyword, "clip") == 0)
	fin.read((char*)&m_clip, sizeof(bool));
      else if (strcmp(keyword, "minscale") == 0)
	fin.read((char*)&m_minScale, sizeof(float));
      else if (strcmp(keyword, "maxscale") == 0)
	fin.read((char*)&m_maxScale, sizeof(float));
      else if (strcmp(keyword, "scaletype") == 0)
	fin.read((char*)&m_scaleType, sizeof(bool));
      else if (strcmp(keyword, "filterpathlen") == 0)
	fin.read((char*)&m_filterPathLen, sizeof(bool));
      else if (strcmp(keyword, "showpoints") == 0)
	fin.read((char*)&m_showPoints, sizeof(bool));
      else if (strcmp(keyword, "showlength") == 0)
	fin.read((char*)&m_showLength, sizeof(bool));
      else if (strcmp(keyword, "depthcue") == 0)
	fin.read((char*)&m_depthcue, sizeof(bool));
      else if (strcmp(keyword, "tube") == 0)
	fin.read((char*)&m_tube, sizeof(bool));
      else if (strcmp(keyword, "closed") == 0)
	fin.read((char*)&m_closed, sizeof(bool));
      else if (strcmp(keyword, "captype") == 0)
	fin.read((char*)&m_capType, sizeof(int));
      else if (strcmp(keyword, "arrowdirection") == 0)
	fin.read((char*)&m_arrowDirection, sizeof(bool));
      else if (strcmp(keyword, "arrowforall") == 0)
	fin.read((char*)&m_arrowForAll, sizeof(bool));
      else if (strcmp(keyword, "sections") == 0)
	fin.read((char*)&m_sections, sizeof(int));
      else if (strcmp(keyword, "segments") == 0)
	fin.read((char*)&m_segments, sizeof(int));
      else if (strcmp(keyword, "sparseness") == 0)
	fin.read((char*)&m_sparseness, sizeof(int));
      else if (strcmp(keyword, "separation") == 0)
	fin.read((char*)&m_separation, sizeof(int));
      else if (strcmp(keyword, "animate") == 0)
	fin.read((char*)&m_animate, sizeof(bool));
      else if (strcmp(keyword, "animateSpeed") == 0)
	fin.read((char*)&m_animateSpeed, sizeof(int));
      else if (strcmp(keyword, "stops") == 0)
	{
	  m_stops.clear();
	  int nstops;
	  fin.read((char*)&nstops, sizeof(int));
	  for(int ni=0; ni<nstops; ni++)
	    {
	      float st[5];
	      QColor col;
	      fin.read((char*)st, 5*sizeof(float));
	      m_stops << QGradientStop(st[0],
				       QColor::fromRgbF(st[1],st[2],st[3],st[4]));
	    }
	}
      else if (strcmp(keyword, "opacity") == 0)
	fin.read((char*)&m_opacity, sizeof(float));
      else if (strcmp(keyword, "index") == 0)
	{
	  int nidx;
	  fin.read((char*)&nidx, sizeof(int));
	  for(int i=0; i<nidx; i++)
	    {
	      int id;
	      fin.read((char*)&id, sizeof(int));
	      m_index.append(id);
	    }
	}
      else if (strcmp(keyword, "userpathlenminmax") == 0)
	{
	  fin.read((char*)&m_minUserPathLen, sizeof(float));
	  fin.read((char*)&m_maxUserPathLen, sizeof(float));
	}
      else if (strcmp(keyword, "points") == 0)
	{
	  int npts;
	  fin.read((char*)&npts, sizeof(int));
	  for(int i=0; i<npts; i++)
	    {
	      fin.read((char*)&f, 3*sizeof(float));
	      m_points.append(Vec(f[0], f[1], f[2]));
	    }
	}
      else if (strcmp(keyword, "crosssection") == 0)
	{
	  int npts;
	  fin.read((char*)&npts, sizeof(int));
	  for(int i=0; i<npts; i++)
	    {
	      fin.read((char*)&f, 3*sizeof(float));
	      m_pointRadX.append(f[0]);
	      m_pointRadY.append(f[1]);
	      m_pointAngle.append(f[2]);
	    }
	}
      else if (strcmp(keyword, "captionfont") == 0)
	{
	  int len;
	  fin.read((char*)&len, sizeof(int));
	  if (len > 0)
	    {
	      char *str = new char[len];
	      fin.read((char*)str, len*sizeof(char));
	      QString fontStr = QString(str);
	      m_captionFont.fromString(fontStr); 
	      delete [] str;
	    }
	}
      else if (strcmp(keyword, "captioncolor") == 0)
	{
	  unsigned char r, g, b, a;
	  fin.read((char*)&r, sizeof(unsigned char));
	  fin.read((char*)&g, sizeof(unsigned char));
	  fin.read((char*)&b, sizeof(unsigned char));
	  fin.read((char*)&a, sizeof(unsigned char));
	  m_captionColor = QColor(r,g,b,a);
	}
      else if (strcmp(keyword, "captionhalocolor") == 0)
	{
	  unsigned char r, g, b, a;
	  fin.read((char*)&r, sizeof(unsigned char));
	  fin.read((char*)&g, sizeof(unsigned char));
	  fin.read((char*)&b, sizeof(unsigned char));
	  fin.read((char*)&a, sizeof(unsigned char));
	  m_captionHaloColor = QColor(r,g,b,a);
	}
    }

  m_undo.clear();
  m_undo.append(m_index, m_points, m_pointRadX, m_pointRadY, m_pointAngle);
}

void
PathGroupObject::undo()
{
  m_undo.undo();

  m_index = m_undo.index();
  m_points = m_undo.points();
  m_pointRadX = m_undo.pointRadX();
  m_pointRadY = m_undo.pointRadY();
  m_pointAngle = m_undo.pointAngle();

  for (int i=0; i<m_index.count()-1; i++)
    computeTangents(m_index[i], m_index[i+1]);

  m_updateFlag = true;
}

void
PathGroupObject::redo()
{
  m_undo.redo();

  m_index = m_undo.index();
  m_points = m_undo.points();
  m_pointRadX = m_undo.pointRadX();
  m_pointRadY = m_undo.pointRadY();
  m_pointAngle = m_undo.pointAngle();

  for (int i=0; i<m_index.count()-1; i++)
    computeTangents(m_index[i], m_index[i+1]);

  m_updateFlag = true;
}


void
PathGroupObject::updateUndo()
{
  if (!m_disableUndo)
    m_undo.append(m_index, m_points, m_pointRadX, m_pointRadY, m_pointAngle);
}
