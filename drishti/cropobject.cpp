#include "global.h"
#include "cropobject.h"
#include "staticfunctions.h"
#include "captiondialog.h"
#include "volumeinformation.h"

//------------------------------------------------------------------
CropObjectUndo::CropObjectUndo() { clear(); }
CropObjectUndo::~CropObjectUndo() { clear(); }

void
CropObjectUndo::clear()
{
  m_points.clear();
  m_pointRadX.clear();
  m_pointRadY.clear();
  m_index = -1;
}

void
CropObjectUndo::clearTop()
{
  if (m_index == m_points.count()-1)
    return;

  while(m_index < m_points.count()-1)
    m_points.removeLast();
  
  while(m_index < m_pointRadX.count()-1)
    m_pointRadX.removeLast();  

  while(m_index < m_pointRadY.count()-1)
    m_pointRadY.removeLast();
}

void
CropObjectUndo::append(QList<Vec> p, QList<float> px, QList<float> py)
{
  clearTop();

  m_points << p;
  m_pointRadX << px;
  m_pointRadY << py;

  m_index = m_points.count()-1;
}

void CropObjectUndo::redo() { m_index = qMin(m_index+1, m_points.count()-1); }
void CropObjectUndo::undo() { m_index = qMax(m_index-1, 0); }

QList<Vec>
CropObjectUndo::points()
{
  QList<Vec> p;

  if (m_index >= 0 && m_index < m_points.count())
    return m_points[m_index];

  return p;
}

QList<float>
CropObjectUndo::pointRadX()
{
  QList<float> p;

  if (m_index >= 0 && m_index < m_pointRadX.count())
    return m_pointRadX[m_index];

  return p;
}

QList<float>
CropObjectUndo::pointRadY()
{
  QList<float> p;

  if (m_index >= 0 && m_index < m_pointRadY.count())
    return m_pointRadY[m_index];

  return p;
}
//------------------------------------------------------------------


bool
operator==(const CropObject& a,
	   const CropObject& b)
{
  return !(a!=b);
}

bool
operator!=(const CropObject& a,
	   const CropObject& b)
{
  if (a.m_cropType != b.m_cropType)
    return true;

  if (a.m_tfset != b.m_tfset)
    return true;

  if (a.m_clearView != b.m_clearView)
    return true;

  if (qAbs(a.m_magnify-b.m_magnify) > 0.01)
    return true;

  if (a.m_unionBlend != b.m_unionBlend)
    return true;

  if (qAbs(a.m_viewMix-b.m_viewMix) > 0.001)
    return true;

  if (a.m_cropType >= CropObject::Glow_Ball)
    {
      if (qAbs(a.m_opacity-b.m_opacity) > 0.001)
	return true;
      if ((a.m_color-b.m_color).squaredNorm() > 0.01)
	return true;
    }

  if (a.m_keepInside != b.m_keepInside)
    return true;

  if (a.m_keepEnds != b.m_keepEnds)
    return true;

  if (a.m_halfSection != b.m_halfSection)
    return true;

  for (int i=0; i<2; i++)
    if (a.m_points[i] != b.m_points[i])
      return true;

  for (int i=0; i<2; i++)
    if (a.m_lift[i] != b.m_lift[i])
      return true;

  for (int i=0; i<2; i++)
    if (qAbs(a.m_pointRadX[i]-b.m_pointRadX[i]) > 0.001)
      return true;

  for (int i=0; i<2; i++)
    if (qAbs(a.m_pointRadY[i]-b.m_pointRadY[i]) > 0.001)
      return true;

  if (qAbs(a.m_pointAngle-b.m_pointAngle) > 0.001)
      return true;

  if ((a.m_dtranslate-b.m_dtranslate).squaredNorm() > 0.01)
    return true;
  if ((a.m_dpivot-b.m_dpivot).squaredNorm() > 0.01)
    return true;
  if ((a.m_drotaxis-b.m_drotaxis).squaredNorm() > 0.01)
    return true;
  if (qAbs(a.m_drotangle-b.m_drotangle) > 0.001)
    return true;

  if (a.m_hatch != b.m_hatch)
    return true;
  else
    {
      if (a.m_hatchGrid != b.m_hatchGrid)
	return true;
      else if (a.m_hxn != b.m_hxn ||
	       a.m_hyn != b.m_hyn ||
	       a.m_hzn != b.m_hzn ||
	       a.m_hxd != b.m_hxd ||
	       a.m_hyd != b.m_hyd ||
	       a.m_hzd != b.m_hzd)
	return true;
    }

  return false;
}

QList<CropObject>
CropObject::interpolate(const QList<CropObject> crops1,
			const QList<CropObject> crops2,
			float frc)
{
  int i;
  QList <CropObject> crops;

  for(i=0; i<qMin(crops1.size(), crops2.size()); i++)
    crops.append(interpolate(crops1[i], crops2[i], frc));

  if (crops1.size() > crops2.size())
    {
      for(i=crops2.size(); i<crops1.size(); i++)
	crops.append(crops1[i]);
    }

  return crops;
}

CropObject
CropObject::interpolate(const CropObject crop1,
			const CropObject crop2,
			float frc)
{
  CropObject crop;

  if ( (crop1.m_cropType != crop2.m_cropType) ||
       (crop1.m_keepInside != crop2.m_keepInside) ||
       (crop1.m_keepEnds != crop2.m_keepEnds) ||
       (crop1.m_halfSection != crop2.m_halfSection) )
    {
      if (frc <= 0.5)
	crop = crop1;
      else
	crop = crop2;
      return crop;
    }

  if (crop1 == crop2)
    return crop1;

  crop = crop1;
  crop.m_tfset = (frc <= 0.5 ? crop1.m_tfset : crop2.m_tfset);
  crop.m_viewMix = (1-frc)*crop1.m_viewMix + frc*crop2.m_viewMix;
  crop.m_magnify = (1-frc)*crop1.m_magnify + frc*crop2.m_magnify;
  crop.m_color = (1-frc)*crop1.m_color + frc*crop2.m_color;
  crop.m_opacity = (1-frc)*crop1.m_opacity + frc*crop2.m_opacity;
  crop.m_pointAngle = (1-frc)*crop1.m_pointAngle + frc*crop2.m_pointAngle;
  crop.m_pointRadX[0] = (1-frc)*crop1.m_pointRadX[0] + frc*crop2.m_pointRadX[0];
  crop.m_pointRadX[1] = (1-frc)*crop1.m_pointRadX[1] + frc*crop2.m_pointRadX[1];
  crop.m_pointRadY[0] = (1-frc)*crop1.m_pointRadY[0] + frc*crop2.m_pointRadY[0];
  crop.m_pointRadY[1] = (1-frc)*crop1.m_pointRadY[1] + frc*crop2.m_pointRadY[1];
  crop.m_points[0] = (1-frc)*crop1.m_points[0] + frc*crop2.m_points[0];
  crop.m_points[1] = (1-frc)*crop1.m_points[1] + frc*crop2.m_points[1];
  crop.m_lift[0] = (1-frc)*crop1.m_lift[0] + frc*crop2.m_lift[0];
  crop.m_lift[1] = (1-frc)*crop1.m_lift[1] + frc*crop2.m_lift[1];

  crop.m_dtranslate = (1-frc)*crop1.m_dtranslate + frc*crop2.m_dtranslate;
  crop.m_dpivot = (1-frc)*crop1.m_dpivot + frc*crop2.m_dpivot;

  crop.m_hxn = (1-frc)*crop1.m_hxn + frc*crop2.m_hxn;
  crop.m_hyn = (1-frc)*crop1.m_hyn + frc*crop2.m_hyn;
  crop.m_hzn = (1-frc)*crop1.m_hzn + frc*crop2.m_hzn;
  crop.m_hxd = (1-frc)*crop1.m_hxd + frc*crop2.m_hxd;
  crop.m_hyd = (1-frc)*crop1.m_hyd + frc*crop2.m_hyd;
  crop.m_hzd = (1-frc)*crop1.m_hzd + frc*crop2.m_hzd;

  if ((crop1.m_drotaxis-crop2.m_drotaxis).squaredNorm() < 0.001)
    { // same axis
      crop.m_drotaxis = crop1.m_drotaxis;
      crop.m_drotangle = crop1.m_drotangle + frc*(crop2.m_drotangle-
						  crop1.m_drotangle);
    }
  else
    {
      if (qAbs(crop1.m_drotangle) < 0.001 &&
	  qAbs(crop2.m_drotangle) < 0.001)
	{ // axis change for 0 angle
	  if (frc < 0.5)
	    {
	      crop.m_drotaxis = crop1.m_drotaxis;
	      crop.m_drotangle = crop1.m_drotangle;
	    }
	  else
	    {
	      crop.m_drotaxis = crop2.m_drotaxis;
	      crop.m_drotangle = crop2.m_drotangle;
	    }
	}
      else
	{
	  Quaternion q, q1, q2;
	  q1 = Quaternion(crop1.m_drotaxis, DEG2RAD(crop1.m_drotangle));
	  q2 = Quaternion(crop2.m_drotaxis, DEG2RAD(crop2.m_drotangle));
	  q = Quaternion::slerp(q1, q2, frc);
	  crop.m_drotaxis = q.axis();
	  crop.m_drotangle = RAD2DEG(q.angle());
	}
    }

  return crop;
}


CropObject
CropObject::get()
{
  CropObject po;
  po = *this;
  return po;
}

void
CropObject::set(const CropObject &po)
{
  m_pointPressed = -1;

  m_updateFlag = true; // for recomputation

  m_show = po.m_show;
  m_cropType = po.m_cropType;
  m_clearView = po.m_clearView;
  m_magnify = po.m_magnify;
  m_showPoints = po.m_showPoints;
  m_tube = po.m_tube;
  m_color = po.m_color;
  m_opacity = po.m_opacity;
  m_tfset = po.m_tfset;
  m_viewMix = po.m_viewMix;
  m_unionBlend = po.m_unionBlend;
  m_points = po.m_points;
  m_lift = po.m_lift;
  m_pointRadX = po.m_pointRadX;
  m_pointRadY = po.m_pointRadY;
  m_pointAngle = po.m_pointAngle;
  m_halfSection = po.m_halfSection;
  m_keepInside = po.m_keepInside;
  m_keepEnds = po.m_keepEnds;
  m_tang = po.m_tang;
  m_xaxis = po.m_xaxis;
  m_yaxis = po.m_yaxis;
  m_oxaxis = po.m_oxaxis;

  m_dtranslate = po.m_dtranslate;
  m_dpivot = po.m_dpivot;
  m_drotaxis = po.m_drotaxis;
  m_drotangle = po.m_drotangle;

  m_hatch = po.m_hatch;
  m_hatchGrid = po.m_hatchGrid;
  m_hxn = po.m_hxn;
  m_hyn = po.m_hyn;
  m_hzn = po.m_hzn;
  m_hxd = po.m_hxd;
  m_hyd = po.m_hyd;
  m_hzd = po.m_hzd;

  computeTangents();

  m_undo.clear();
  m_undo.append(m_points, m_pointRadX, m_pointRadY);
}

CropObject&
CropObject::operator=(const CropObject &po)
{
  m_pointPressed = -1;

  m_updateFlag = true; // for recomputation

  m_show = po.m_show;
  m_cropType = po.m_cropType;
  m_clearView = po.m_clearView;
  m_magnify = po.m_magnify;
  m_showPoints = po.m_showPoints;
  m_tube = po.m_tube;
  m_color = po.m_color;
  m_opacity = po.m_opacity;
  m_tfset = po.m_tfset;
  m_viewMix = po.m_viewMix;
  m_unionBlend = po.m_unionBlend;
  m_points = po.m_points;
  m_lift = po.m_lift;
  m_pointRadX = po.m_pointRadX;
  m_pointRadY = po.m_pointRadY;
  m_pointAngle = po.m_pointAngle;
  m_halfSection = po.m_halfSection;
  m_keepInside = po.m_keepInside;
  m_keepEnds = po.m_keepEnds;
  m_tang = po.m_tang;
  m_xaxis = po.m_xaxis;
  m_yaxis = po.m_yaxis;
  m_oxaxis = po.m_oxaxis;

  m_dtranslate = po.m_dtranslate;
  m_dpivot = po.m_dpivot;
  m_drotaxis = po.m_drotaxis;
  m_drotangle = po.m_drotangle;

  m_hatch = po.m_hatch;
  m_hatchGrid = po.m_hatchGrid;
  m_hxn = po.m_hxn;
  m_hyn = po.m_hyn;
  m_hzn = po.m_hzn;
  m_hxd = po.m_hxd;
  m_hyd = po.m_hyd;
  m_hzd = po.m_hzd;

  computeTangents();

  m_undo.clear();
  m_undo.append(m_points, m_pointRadX, m_pointRadY);

  return *this;
}

int CropObject::moveAxis() { return m_moveAxis; }
void CropObject::setMoveAxis(int type) { m_moveAxis = type; }

CropObject::CropObject()
{
  m_undo.clear();

  m_moveAxis = MoveAll;
  m_show = true;
  m_pointPressed = -1;

  m_cropType = Crop_Tube;
  m_clearView = false;
  m_magnify = 1.0;
  m_showPoints = true;
  m_updateFlag = false;
  m_tube = false;
  m_color = Vec(0.8,0.7,0.9);
  m_opacity = 1;
  m_tfset = 1;
  m_viewMix = 0.2;
  m_unionBlend = false;
  m_points.clear();
  m_lift.clear();
  m_pointRadX.clear();
  m_pointRadY.clear();
  m_pointAngle = 0.0;
  m_halfSection = false;
  m_keepInside = true;
  m_keepEnds = true;
  m_tang = Vec(0,0,1);
  m_xaxis = Vec(1,0,0);
  m_yaxis = Vec(0,1,0);
  m_oxaxis = Vec(1,0,0);

  m_dtranslate = Vec(0,0,0);
  m_dpivot = Vec(0, 0, 0);
  m_drotaxis = Vec(0, 0, 1);
  m_drotangle = 0;
  
  m_length = 0;
  m_tgP.clear();
  m_crop.clear();
  m_radX.clear();
  m_radY.clear();

  m_hatch = false;
  m_hatchGrid = true;
  m_hxn = 10;
  m_hyn = 10;
  m_hzn = 10;
  m_hxd = 1;
  m_hyd = 1;
  m_hzd = 1;
}

CropObject::~CropObject()
{
  m_undo.clear();

  m_points.clear();
  m_lift.clear();
  m_pointRadX.clear();
  m_pointRadY.clear();
  m_tgP.clear();
  m_crop.clear();
  m_radX.clear();
  m_radY.clear();
}

void
CropObject::flipPoints()
{
  Vec pt = m_points[0];
  m_points[0] = m_points[1];
  m_points[1] = pt;
  m_updateFlag = true;
  m_undo.append(m_points, m_pointRadX, m_pointRadY);
}

void
CropObject::translate(Vec trans)
{
  m_points[0] += trans;
  m_points[1] += trans;
  m_updateFlag = true;
}

void
CropObject::translate(bool moveX, bool indir)
{
  QList<Vec> delta;

  for (int i=0; i<2; i++) // assuming only two points
    {
      if (moveX)
	delta.append(m_xaxis*m_pointRadX[i]);
      else
	delta.append(m_yaxis*m_pointRadY[i]);
    }

  if (indir)
    {
      m_points[0] += delta[0];
      m_points[1] += delta[1];
    }
  else
    {
      m_points[0] -= delta[0];
      m_points[1] -= delta[1];
    }

  computeTangents();
  m_updateFlag = true;
}

bool CropObject::hatch() { return m_hatch; }
void CropObject::hatchParameters(bool& hatchGrid,
				 int& hxn, int& hxd,
				 int& hyn, int& hyd,
				 int& hzn, int& hzd)
{
  hatchGrid = m_hatchGrid;
  hxn = m_hxn;
  hyn = m_hyn;
  hzn = m_hzn;
  hxd = m_hxd;
  hyd = m_hyd;
  hzd = m_hzd;
}
void CropObject::setHatch(bool hatch) { m_hatch = hatch; }
void CropObject::setHatchParameters(bool hatchGrid,
				    int hxn, int hxd,
				    int hyn, int hyd,
				    int hzn, int hzd)
{
  m_hatchGrid = hatchGrid;
  m_hxn = hxn;
  m_hyn = hyn;
  m_hzn = hzn;
  m_hxd = hxd;
  m_hyd = hyd;
  m_hzd = hzd;
}

Vec CropObject::dtranslate() { return m_dtranslate; }
Vec CropObject::dpivot() { return m_dpivot; }
Vec CropObject::drotaxis() { return m_drotaxis; }
float CropObject::drotangle() { return m_drotangle; }

bool CropObject::keepInside() { return m_keepInside; }
bool CropObject::keepEnds() { return m_keepEnds; }
bool CropObject::clearView() { return m_clearView; }
float CropObject::magnify() { return m_magnify; }
bool CropObject::showPoints() { return m_showPoints; }
bool CropObject::halfSection() { return m_halfSection; }
int CropObject::cropType() { return m_cropType; }
bool CropObject::tube() { return m_tube; }
Vec CropObject::color() { return m_color; }
float CropObject::opacity() { return m_opacity; }
int CropObject::tfset() { return m_tfset; }
float CropObject::viewMix() { return m_viewMix; }
bool CropObject::unionBlend() { return m_unionBlend; }
QList<Vec> CropObject::points() { return m_points; }
Vec CropObject::getPoint(int i)
{
  if (i < m_points.count())
    return m_points[i];
  else
    return Vec(0,0,0);
}
int CropObject::getLift(int i)
{
  if (i < m_lift.count())
    return m_lift[i];
  else
    return 0;
}
float CropObject::getRadX(int i)
{
  if (i < m_points.count())
    return m_pointRadX[i];
  else
    return 0;
}
float CropObject::getRadY(int i)
{
  if (i < m_points.count())
    return m_pointRadY[i];
  else
    return 0;
}
float CropObject::getAngle() { return m_pointAngle; }
QList<int> CropObject::lift() { return m_lift; }
QList<float> CropObject::radX() { return m_pointRadX; }
QList<float> CropObject::radY() { return m_pointRadY; }
float CropObject::angle() { return m_pointAngle; }
float CropObject::length()
{
  if (m_updateFlag)
    computeCropLength();
  return m_length;
}

void CropObject::setClearView(bool flag) { m_clearView = flag; }
void CropObject::setMagnify(float m) { m_magnify = m; }
void CropObject::setShowPoints(bool flag) { m_showPoints = flag; }
void CropObject::setKeepInside(bool flag)
{
  m_keepInside = flag;
  m_updateFlag = true;
}
void CropObject::setKeepEnds(bool flag)
{
  m_keepEnds = flag;
  m_updateFlag = true;
}
void CropObject::setCropType(int ct)
{
  m_cropType = ct;
  m_updateFlag = true;
}
void CropObject::setHalfSection(bool flag)
{
  m_halfSection = flag;
  m_updateFlag = true;
}
void CropObject::setTube(bool flag) { m_tube = flag; }
void CropObject::setColor(Vec color) { m_color = color; }
void CropObject::setOpacity(float op) { m_opacity = op; }
void CropObject::setTFset(int ts) { m_tfset = ts; }
void CropObject::setViewMix(float vm) { m_viewMix = vm; }
void CropObject::setUnionBlend(bool vm) { m_unionBlend = vm; }

void CropObject::setDtranslate(Vec v) { m_dtranslate = v; }
void CropObject::setDpivot(Vec v) { m_dpivot = v; }
void CropObject::setDrotaxis(Vec v) { m_drotaxis = v.unit(); }
void CropObject::setDrotangle(float v) { m_drotangle = v; }

void CropObject::setPoint(int i, Vec pt)
{
  if (i < m_points.count())
    {
      m_points[i] = pt;
      computeTangents();
      m_updateFlag = true;
    }
}
void CropObject::setLift(int i, int v, bool sameForAll)
{
  if (i < m_points.count())
    {      
      if (sameForAll)
	{
	  for(int j=0; j<m_lift.count(); j++)
	    m_lift[j] = v;	    
	}
      else
	m_lift[i] = v;

      m_updateFlag = true;
    }
}
void CropObject::setRadX(int i, float v, bool sameForAll)
{
  if (i < m_points.count())
    {      
      if (sameForAll)
	{
	  for(int j=0; j<m_points.count(); j++)
	    m_pointRadX[j] = v;	    
	}
      else
	m_pointRadX[i] = v;

      m_updateFlag = true;
    }
}
void CropObject::setRadY(int i, float v, bool sameForAll)
{
  if (i < m_points.count())
    {
      if (sameForAll)
	{
	  for(int j=0; j<m_points.count(); j++)
	    m_pointRadY[j] = v;	    
	}
      else
	m_pointRadY[i] = v;

      m_updateFlag = true;
    }
}
void CropObject::rotate(Vec axis, float angle)
{
  Quaternion q(axis, DEG2RAD(angle));
  Vec midpt = (m_points[0] + m_points[1])*0.5;
  Vec pt = m_points[0]-midpt;
  m_points[0] = q.rotate(pt) + midpt;
  pt = m_points[1]-midpt;
  m_points[1] = q.rotate(pt) + midpt;

  computeTangents();
  m_updateFlag = true;
}
void CropObject::setAngle(float v)
{
  m_pointAngle = v;
  m_updateFlag = true;
}
void CropObject::setSameForAll(bool flag)
{
  if (flag)
    {
      float x = m_pointRadX[0];
      float y = m_pointRadY[0];
      int l = m_lift[0];
      for(int j=0; j<m_points.count(); j++)
	{
	  m_pointRadX[j] = x;
	  m_pointRadY[j] = y;
	  m_lift[j] = l;
	}
    }
}
void CropObject::normalize()
{
  for(int i=0; i<m_points.count(); i++)
    {
      Vec pt = m_points[i];
      pt = Vec((int)pt.x, (int)pt.y, (int)pt.z);
      m_points[i] = pt;
    }
  m_updateFlag = true;
}
void CropObject::shiftPoints(float shift)
{
  Vec pt;

  pt = m_points[0]; pt -= shift*m_tang; m_points[0] = pt;
  pt = m_points[1]; pt += shift*m_tang; m_points[1] = pt;

  m_updateFlag = true;
}
void CropObject::setPoints(QList<Vec> pts)
{
  m_pointPressed = -1;

  if (pts.count() != 2)
    {
      QMessageBox::information(0, QString("%1 points").arg(pts.count()),
			       "Exactly 2 points must be specified");
      return;
    }

  m_points = pts;
  
  computeTangents();

  m_lift.clear();
  m_pointRadX.clear();
  m_pointRadY.clear();
  m_pointAngle = 0;
  for(int i=0; i<pts.count(); i++)
    {
      m_pointRadX.append(50);
      m_pointRadY.append(50);
      m_lift.append(0);
    }

  m_updateFlag = true;
  m_undo.append(m_points, m_pointRadX, m_pointRadY);
}
void CropObject::setLift(QList<int> lft)
{
  m_lift = lft;  
  m_updateFlag = true;
}
void CropObject::setRadX(QList<float> rad)
{
  m_pointRadX = rad;  
  m_updateFlag = true;
}
void CropObject::setRadY(QList<float> rad)
{
  m_pointRadY = rad;  
  m_updateFlag = true;
}

void CropObject::setPointPressed(int p) { m_pointPressed = p; }
int CropObject::getPointPressed() { return m_pointPressed; }

void
CropObject::computeCropLength()
{
  computeCrop(m_points);
  computeTangents();
}

void
CropObject::computeLength(QList<Vec> points)
{
  m_length = 0;
  for(int i=1; i<points.count(); i++)
    m_length += (points[i]-points[i-1]).norm();
}

void
CropObject::computeTangents()
{
  int nkf = m_points.count();
  if (nkf < 2)
    return;

  m_tgP.clear();
  for(int kf=0; kf<nkf; kf++)
    {
      Vec prevP, nextP;

      if (kf == 0)
	prevP = m_points[kf];
      else
	prevP = m_points[kf-1];

      if (kf == nkf-1)
	nextP = m_points[kf];
      else
	nextP = m_points[kf+1];
      
      Vec tgP = 0.5*(nextP - prevP);
      m_tgP.append(tgP);
    } 

  //----------------
  Vec tang = m_points[1]-m_points[0];
  if (tang.norm() > 0)
    tang.normalize();
  else
    tang = Vec(1,0,0); // should really scold the user

  Vec voxelScaling = Global::voxelScaling();
  tang = VECPRODUCT(tang, voxelScaling);
  tang.normalize();


  // calculate new vectors from old vectors
  Vec axis;
  float angle;
  StaticFunctions::getRotationBetweenVectors(m_tang,
					     tang,
					     axis, angle);
  m_tang = tang;

  if (qAbs(angle) > 0.0 && qAbs(angle) < 3.1415)
    {
      Quaternion q(axis, angle);
      m_oxaxis = q.rotate(m_oxaxis);
    }
  // apply offset rotation
  angle = DEG2RAD(m_pointAngle);
  Quaternion q = Quaternion(m_tang, angle);
  m_xaxis = q.rotate(m_oxaxis);
  m_yaxis = m_tang^m_xaxis;
  
  m_xaxis.normalize();
  m_yaxis.normalize();
}

Vec
CropObject::interpolate(int kf1, int kf2, float frc)
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
CropObject::computeCrop(QList<Vec> points)
{
  Vec voxelScaling = Global::voxelScaling();
  Vec voxelSize = VolumeInformation::volumeInformation().voxelSize;


  // -- collect crop points for length computation
  QList<Vec> lengthCrop;
  lengthCrop.clear();

  m_crop.clear();
  m_radX.clear();
  m_radY.clear();

  int npts = points.count();
  if (npts < 2)
    return;

  Vec prevPt;
  for(int i=0; i<npts; i++)
    {
      Vec v = VECPRODUCT(points[i], voxelScaling);
      m_crop.append(v);
      m_radX.append(m_pointRadX[i]);
      m_radY.append(m_pointRadY[i]);
      
      // for length calculation
      v = VECPRODUCT(points[i], voxelSize);
      lengthCrop.append(v);
    }
  
  computeLength(lengthCrop);
}

QList<Vec>
CropObject::getPointCrop()
{
  QList<Vec> crop;

  int npts = m_points.count();
  if (npts < 2)
    return crop;

  Vec prevPt;
  for(int i=0; i<npts; i++)
    crop.append(m_points[i]);
  
  return crop;
}

void
CropObject::draw(QGLViewer *viewer,
		 bool active,
		 bool backToFront)
{
  if (!m_show)
    return;

  if (m_updateFlag)
    {
      m_updateFlag = false;

      computeCropLength();
    }

  if (m_tube)
    drawTube(viewer, active);
  else
    drawLines(viewer, active, backToFront);
}

void
CropObject::drawTube(QGLViewer *viewer,
		     bool active)
{
  if (m_opacity < 0.1)
    return;

  glColor4f(m_color.x*m_opacity,
	    m_color.y*m_opacity,
	    m_color.z*m_opacity,
	    m_opacity);

  generateTube(1.0);
}

void
CropObject::drawLines(QGLViewer *viewer,
		      bool active,
		      bool backToFront)
{
  Vec voxelScaling = Global::voxelScaling();

  glEnable(GL_BLEND);
  glEnable(GL_LINE_SMOOTH);
  glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

  if (backToFront)
    {
      if (active)
        glLineWidth(7);
      else
        glLineWidth(3);

      glColor3f(m_color.x, m_color.y, m_color.z);

      glBegin(GL_LINE_STRIP);
      for(int i=0; i<m_crop.count(); i++)
	glVertex3fv(m_crop[i]);
      glEnd();
    }

  if (m_showPoints)
    {
      glColor3f(m_color.x, m_color.y, m_color.z);


      glEnable(GL_POINT_SPRITE);
      glActiveTexture(GL_TEXTURE0);
      glEnable(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D, Global::spriteTexture());
      glTexEnvf( GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE );
      glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
      glEnable(GL_POINT_SMOOTH);

      Vec voxelScaling = Global::voxelScaling();
      glPointSize(20);
      glBegin(GL_POINTS);
      for(int i=0; i<m_points.count();i++)
	{
	  Vec pt = VECPRODUCT(m_points[i], voxelScaling);
	  glVertex3fv(pt);
	}
      glEnd();


      if (m_pointPressed > -1)
	{
	  glColor3f(1,0,0);
	  Vec voxelScaling = Global::voxelScaling();
	  glPointSize(25);
	  glBegin(GL_POINTS);
	  Vec pt = VECPRODUCT(m_points[m_pointPressed], voxelScaling);
	  glVertex3fv(pt);
	  glEnd();
	}
	

      glPointSize(1);  

      glDisable(GL_POINT_SPRITE);
      glActiveTexture(GL_TEXTURE0);
      glDisable(GL_TEXTURE_2D);
  
      glDisable(GL_POINT_SMOOTH);
    }

  if (active)
    glLineWidth(3);
  else
    glLineWidth(1);

  glColor3f(m_color.x, m_color.y, m_color.z);

  glBegin(GL_LINE_STRIP);
  for(int i=0; i<m_crop.count(); i++)
    glVertex3fv(m_crop[i]);
  glEnd();

  if (!backToFront)
    {
      if (active)
        glLineWidth(7);
      else
        glLineWidth(3);

      glColor3f(m_color.x, m_color.y, m_color.z);

      glBegin(GL_LINE_STRIP);
      for(int i=0; i<m_crop.count(); i++)
	glVertex3fv(m_crop[i]);
      glEnd();
    }

  glLineWidth(1);

  if (m_showPoints || m_pointPressed > -1)
    {
      Vec c0 = (m_crop[0]+m_crop[1])*0.5;
      float rx = (m_radX[0]+m_radX[1])*0.5;
      float ry = (m_radY[0]+m_radY[1])*0.5;

      Vec ca = (0.4*m_crop[0]+ 0.6*m_crop[1]);
      Vec cb = (0.6*m_crop[0]+ 0.4*m_crop[1]);

      glColor3f(0, 0.5, 1);
      if (active && m_moveAxis >= MoveZ) glBegin(GL_TRIANGLES);
      else glBegin(GL_LINE_STRIP);
      glVertex3fv(m_crop[1]);
      glVertex3fv(ca+0.2*rx*m_xaxis);
      glVertex3fv(ca-0.2*rx*m_xaxis);
      if (!(active && m_moveAxis >= MoveZ)) glVertex3fv(m_crop[1]);
      glVertex3fv(m_crop[1]);
      glVertex3fv(ca+0.2*ry*m_yaxis);
      glVertex3fv(ca-0.2*ry*m_yaxis);
      if (!(active && m_moveAxis >= MoveZ)) glVertex3fv(m_crop[1]);
      glEnd();


      glColor3f(1, 0.2, 0);
      glBegin(GL_LINES);
      glVertex3fv(c0-rx*m_xaxis);
      glVertex3fv(c0);
      glEnd();
      if (active &&
	  m_moveAxis >= MoveX0 &&
	  m_moveAxis <= MoveX1) glBegin(GL_TRIANGLES);
      else glBegin(GL_LINE_STRIP);
      glVertex3fv(ca);
      glVertex3fv(c0+rx*m_xaxis);
      glVertex3fv(cb);
      if (!(active &&
	    m_moveAxis >= MoveX0 &&
	    m_moveAxis <= MoveX1)) glVertex3fv(ca);
      glEnd();
      glBegin(GL_LINE_STRIP);
      glVertex3fv(ca);
      glVertex3fv(c0-rx*m_xaxis);
      glVertex3fv(cb);
      glVertex3fv(ca);
      glEnd();

	  
      glColor3f(0.2,1,0.3);
      glBegin(GL_LINES);
      glVertex3fv(c0-ry*m_yaxis);
      glVertex3fv(c0);
      glEnd();
      if (active &&
	  m_moveAxis >= MoveY0 &&
	  m_moveAxis <= MoveY1) glBegin(GL_TRIANGLES);
      else glBegin(GL_LINE_STRIP);
      glVertex3fv(ca);
      glVertex3fv(c0+ry*m_yaxis);
      glVertex3fv(cb);
      if (!(active &&
	    m_moveAxis >= MoveY0 &&
	    m_moveAxis <= MoveY1)) glVertex3fv(ca);
      glEnd();
      glBegin(GL_LINE_STRIP);
      glVertex3fv(ca);
      glVertex3fv(c0-ry*m_yaxis);
      glVertex3fv(cb);
      glVertex3fv(ca);
      glEnd();


      if (m_cropType == Displace_Displace)
	{
	  Vec pivot = m_points[0] - m_radX[0]*m_xaxis - m_radY[0]*m_yaxis +
		      m_dpivot.x*2*m_radX[0]*m_xaxis +
		      m_dpivot.y*2*m_radY[0]*m_yaxis +
	              m_dpivot.z*length()*m_tang;
	  glColor3f(0.3, 0.5, 1.0);
	  glPointSize(10);
	  glBegin(GL_POINTS);
	  glVertex3fv(pivot);
	  glEnd();

	  Vec rotaxis = m_drotaxis.x*m_xaxis +
			m_drotaxis.y*m_yaxis +
			m_drotaxis.z*m_tang;
	  rotaxis = rotaxis.unit();

	  glBegin(GL_LINES);
	  glVertex3fv(pivot-20*rotaxis);
	  glVertex3fv(pivot+20*rotaxis);
	  glEnd();
	}

      for (int i=0; i<2; i++)
	{
	  if (i == m_pointPressed || m_pointPressed == -1)
	    {
	      glColor3f(m_color.x, m_color.y, m_color.z);
	      QList<Vec> csec;
	      if (m_cropType != Crop_Box &&
		  m_cropType < Tear_Tear)
		csec = getCrossSection(1.0,
				       m_radX[i], m_radY[i],
				       m_tang, m_xaxis, m_yaxis);
	      else
		{
		  csec.append( m_radX[i]*m_xaxis+m_radY[i]*m_yaxis);
		  csec.append(-m_radX[i]*m_xaxis+m_radY[i]*m_yaxis);
		  csec.append(-m_radX[i]*m_xaxis-m_radY[i]*m_yaxis);
		  csec.append( m_radX[i]*m_xaxis-m_radY[i]*m_yaxis);
		  csec.append( m_radX[i]*m_xaxis+m_radY[i]*m_yaxis);
		}

	      for(int j=0; j<csec.count(); j++)
		csec[j] += m_crop[i];
	      
	      glBegin(GL_LINE_STRIP);
	      for(int j=0; j<csec.count(); j++)
		glVertex3fv(csec[j]);
	      glEnd();
	    }
	}
    }

  glDisable(GL_LINE_SMOOTH);
}

void
CropObject::generateTube(float scale)
{
  Vec cv;
  QList<Vec> csec1, csec2;
  if (m_cropType == Crop_Ellipsoid)
    {
      csec1 = getCrossSection(scale,
			      (m_radX[0]+m_radX[1])*0.5,
			      (m_radY[0]+m_radY[1])*0.5,
			      m_tang, m_xaxis, m_yaxis);
    }
  else if (m_cropType == Crop_Box ||
	   m_cropType >= Tear_Tear)
    {
      if (m_halfSection)
	{
	  csec1.append(m_radX[0]*m_xaxis);
	  csec1.append(m_radX[0]*m_xaxis+m_radY[0]*m_yaxis);
	  csec1.append(-m_radX[0]*m_xaxis+m_radY[0]*m_yaxis);
	  csec1.append(-m_radX[0]*m_xaxis);

	  csec2.append( m_radX[1]*m_xaxis);
	  csec2.append( m_radX[1]*m_xaxis+m_radY[1]*m_yaxis);
	  csec2.append(-m_radX[1]*m_xaxis+m_radY[1]*m_yaxis);
	  csec2.append(-m_radX[1]*m_xaxis);
	}
      else
	{
	  csec1.append( m_radX[0]*m_xaxis+m_radY[0]*m_yaxis);
	  csec1.append(-m_radX[0]*m_xaxis+m_radY[0]*m_yaxis);
	  csec1.append(-m_radX[0]*m_xaxis-m_radY[0]*m_yaxis);
	  csec1.append( m_radX[0]*m_xaxis-m_radY[0]*m_yaxis);
	  csec1.append( m_radX[0]*m_xaxis+m_radY[0]*m_yaxis);

	  csec2.append( m_radX[1]*m_xaxis+m_radY[1]*m_yaxis);
	  csec2.append(-m_radX[1]*m_xaxis+m_radY[1]*m_yaxis);
	  csec2.append(-m_radX[1]*m_xaxis-m_radY[1]*m_yaxis);
	  csec2.append( m_radX[1]*m_xaxis-m_radY[1]*m_yaxis);
	  csec2.append( m_radX[1]*m_xaxis+m_radY[1]*m_yaxis);
	}
    }
  else // Crop_Tube
    {
      csec1 = getCrossSection(scale,
			      m_radX[0], m_radY[0],
			      m_tang, m_xaxis, m_yaxis);
      
      csec2 = getCrossSection(scale,
			      m_radX[1], m_radY[1],
			      m_tang, m_xaxis, m_yaxis);
    }

  cv = m_crop[0];
  if (m_cropType == Crop_Ellipsoid)
    cv = (m_crop[0]+m_crop[1])*0.5;
  glBegin(GL_LINE_STRIP);
  for(int j=0; j<csec1.count(); j++)
    glVertex3fv(cv + csec1[j]);
  if (m_halfSection && m_cropType != Crop_Ellipsoid)
    glVertex3fv(cv + csec1[0]);
  glEnd();


  if (m_cropType != Crop_Ellipsoid)
    {
      cv = m_crop[1];
      glBegin(GL_LINE_STRIP);
      for(int j=0; j<csec2.count(); j++)
	glVertex3fv(cv + csec2[j]);
      if (m_halfSection && m_cropType != Crop_Ellipsoid)
	glVertex3fv(cv + csec2[0]);
      glEnd();
    }

  if (m_cropType == Crop_Ellipsoid)
    {
      Vec cv = (m_crop[0]+m_crop[1])*0.5;
      float rd0, rd1, rd2;
      rd0 = (m_points[0]-m_points[1]).norm()*0.5;
      rd1 = (m_radX[0]+m_radX[1])*0.5;
      rd2 = (m_radY[0]+m_radY[1])*0.5;

      int nseg = 8;
      for (int seg=0; seg<nseg; seg++)
	{
	  float angle = (3.14159*seg)/(nseg-1);
	  Quaternion q = Quaternion(m_tang, angle);
	  Vec xaxis = q.rotate(m_xaxis);
	  Vec yaxis = q.rotate(m_yaxis);

	  float st = rd1*sin(angle);
	  float ct = rd2*cos(angle);
	  float rd = (rd1*rd2)/sqrt(ct*ct + st*st);
	  
	  csec2 = getCrossSection(scale, rd0, rd,
				  yaxis, m_tang, xaxis);
	  
	  glBegin(GL_LINE_STRIP);
	  for(int j=0; j<csec2.count(); j++)
	    glVertex3fv(cv + csec2[j]);
	  glEnd();
	}
    }

  if (m_cropType == Crop_Box ||
      m_cropType >= Tear_Tear)
    {
      QList<Vec> csec3;

	{
	  for(int j=0; j<csec2.count(); j++)
	    csec3.append(0.5*(csec1[j]+csec2[j]));
	}
      
      cv = (m_crop[0]+m_crop[1])*0.5;
      glBegin(GL_LINE_STRIP);
      for(int j=0; j<csec3.count(); j++)
	glVertex3fv(cv + csec3[j]);
      glEnd();

      csec3.clear();
    }

  if (m_cropType != Crop_Ellipsoid)
    {
      glBegin(GL_LINES);
      for(int j=0; j<csec1.count(); j++)
	{
	  glVertex3fv(m_crop[0] + csec1[j]);
	  glVertex3fv(m_crop[1] + csec2[j]);
	}
      glEnd();
    }

  csec1.clear();
  csec2.clear();
}


QList<Vec>
CropObject::getCrossSection(float scale,
			    float a, float b,
			    Vec tang, Vec xaxis, Vec yaxis,
			    int sections)
{
  //Vec voxelScaling = Global::voxelScaling();
  //int sections = 20;
  if (m_cropType == Crop_Box ||
      m_cropType >= Tear_Tear)
    sections = 4;
  int jend = sections;
  if (m_halfSection)
    jend = sections/2+1;

  QList<Vec> csec;
  for(int j=0; j<jend; j++)
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

      float st = a*sin(6.2831853*t);
      float ct = b*cos(6.2831853*t);
      float r = (a*b)/sqrt(ct*ct + st*st);
      float x = r*cos(6.2831853*t)*scale;
      float y = r*sin(6.2831853*t)*scale;
      Vec v = x*xaxis + y*yaxis;
      //v = VECPRODUCT(v, voxelScaling);
      csec.append(v);
    }

  if (!m_halfSection)
    csec.append(csec[0]);

  return csec;
}

QList<Vec>
CropObject::getNormals(QList<Vec> csec, Vec tang)
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
CropObject::postdraw(QGLViewer *viewer,
		     int x, int y,
		     bool grabsMouse)
{
  if (!m_show || !grabsMouse)
    return;

  glEnable(GL_LINE_SMOOTH);
  glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // blend on top

  glDisable(GL_DEPTH_TEST);

  viewer->startScreenCoordinatesSystem();

  Vec voxelScaling = Global::voxelScaling();

  if (grabsMouse)
    {
      VolumeInformation pvlInfo = VolumeInformation::volumeInformation();
      QString str = "crop";

      if (m_cropType < Tear_Tear)
	str = "crop";
      else if (m_cropType < Displace_Displace)
	str = "dissect";
      else if (m_cropType < View_Tear)
	str = "displace";
      else if (m_cropType < Glow_Ball)
	str = "blend";
      else
	str = "glow";

      if (m_pointPressed > -1)
	{
	  if (m_cropType < Tear_Tear)
	    str = QString("crop (pt %1)").arg(m_pointPressed);
	  else if (m_cropType < Displace_Displace)
	    str = QString("dissect (pt %1)").arg(m_pointPressed);
	  else if (m_cropType < View_Tear)
	    str = QString("displace (pt %1)").arg(m_pointPressed);
	  else if (m_cropType < Glow_Ball)
	    str = QString("blend (pt %1)").arg(m_pointPressed);
	  else 
	    str = QString("glow (pt %1)").arg(m_pointPressed);
	}

      QFont font = QFont();
      QFontMetrics metric(font);
      int ht = metric.height();
      int wd = metric.width(str);
      x += 10;

      StaticFunctions::renderText(x+2, y, str, font, Qt::black, Qt::green);
    }

  glEnable(GL_DEPTH_TEST);

  viewer->stopScreenCoordinatesSystem();
}

void
CropObject::save(fstream& fout)
{
  char keyword[100];
  float f[3];

  memset(keyword, 0, 100);
  sprintf(keyword, "cropobjectstart");
  fout.write((char*)keyword, strlen(keyword)+1);

  memset(keyword, 0, 100);
  sprintf(keyword, "show");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_show, sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "croptype");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_cropType, sizeof(int));

  memset(keyword, 0, 100);
  sprintf(keyword, "clearview");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_clearView, sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "magnify");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_magnify, sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "showpoints");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_showPoints, sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "tube");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_tube, sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "halfsection");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_halfSection, sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "keepinside");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_keepInside, sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "keepends");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_keepEnds, sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "color");
  fout.write((char*)keyword, strlen(keyword)+1);
  f[0] = m_color.x;
  f[1] = m_color.y;
  f[2] = m_color.z;
  fout.write((char*)&f, 3*sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "opacity");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_opacity, sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "tfset");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_tfset, sizeof(int));

  memset(keyword, 0, 100);
  sprintf(keyword, "viewmix");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_viewMix, sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "unionblend");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_unionBlend, sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "dtranslate");
  fout.write((char*)keyword, strlen(keyword)+1);
  f[0] = m_dtranslate.x;
  f[1] = m_dtranslate.y;
  f[2] = m_dtranslate.z;
  fout.write((char*)&f, 3*sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "dpivot");
  fout.write((char*)keyword, strlen(keyword)+1);
  f[0] = m_dpivot.x;
  f[1] = m_dpivot.y;
  f[2] = m_dpivot.z;
  fout.write((char*)&f, 3*sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "drotaxis");
  fout.write((char*)keyword, strlen(keyword)+1);
  f[0] = m_drotaxis.x;
  f[1] = m_drotaxis.y;
  f[2] = m_drotaxis.z;
  fout.write((char*)&f, 3*sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "drotangle");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_drotangle, sizeof(float));

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
  sprintf(keyword, "tang");
  fout.write((char*)keyword, strlen(keyword)+1);
  f[0] = m_tang.x;
  f[1] = m_tang.y;
  f[2] = m_tang.z;
  fout.write((char*)&f, 3*sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "oxaxis");
  fout.write((char*)keyword, strlen(keyword)+1);
  f[0] = m_oxaxis.x;
  f[1] = m_oxaxis.y;
  f[2] = m_oxaxis.z;
  fout.write((char*)&f, 3*sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "crosssection");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&npts, sizeof(int));
  for(int i=0; i<npts; i++)
    {
      f[0] = m_pointRadX[i];
      f[1] = m_pointRadY[i];
      f[2] = m_pointAngle;
      fout.write((char*)&f, 3*sizeof(float));
    }

  int nlfts = m_lift.count();
  memset(keyword, 0, 100);
  sprintf(keyword, "lift");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&nlfts, sizeof(int));
  for(int i=0; i<nlfts; i++)
    fout.write((char*)&m_lift[i], sizeof(int));


  memset(keyword, 0, 100);
  sprintf(keyword, "hatch");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_hatch, sizeof(bool));
  fout.write((char*)&m_hatchGrid, sizeof(bool));
  fout.write((char*)&m_hxn, sizeof(int));
  fout.write((char*)&m_hxd, sizeof(int));
  fout.write((char*)&m_hyn, sizeof(int));
  fout.write((char*)&m_hyd, sizeof(int));
  fout.write((char*)&m_hzn, sizeof(int));
  fout.write((char*)&m_hzd, sizeof(int));

  memset(keyword, 0, 100);
  sprintf(keyword, "cropobjectend");
  fout.write((char*)keyword, strlen(keyword)+1);
}

void
CropObject::load(fstream &fin)
{
  m_show = true;
  m_clearView = false;
  m_magnify = 1.0;
  m_points.clear();
  m_lift.clear();
  m_pointRadX.clear();
  m_pointRadY.clear();
  m_pointAngle = 0.0;
  m_tgP.clear();
  m_crop.clear();
  m_radX.clear();
  m_radY.clear();
  m_tfset = 1;
  m_viewMix = 0.2;
  m_unionBlend = false;
  m_tang = Vec(0,0,1);
  m_xaxis = Vec(1,0,0);
  m_yaxis = Vec(0,1,0);
  m_oxaxis = Vec(1,0,0);

  m_dtranslate = Vec(0,0,0);
  m_dpivot = Vec(0, 0, 0);
  m_drotaxis = Vec(0, 0, 1);
  m_drotangle = 0;
  
  m_hatch = false;
  m_hatchGrid = true;
  m_hxn = 10;
  m_hyn = 10;
  m_hzn = 10;
  m_hxd = 1;
  m_hyd = 1;
  m_hzd = 1;

  bool done = false;
  char keyword[100];
  float f[3];
  while (!done)
    {
      fin.getline(keyword, 100, 0);

      if (strcmp(keyword, "cropobjectend") == 0)
	done = true;
      else if (strcmp(keyword, "show") == 0)
	fin.read((char*)&m_show, sizeof(bool));
      else if (strcmp(keyword, "croptype") == 0)
	fin.read((char*)&m_cropType, sizeof(int));
      else if (strcmp(keyword, "clearview") == 0)
	fin.read((char*)&m_clearView, sizeof(bool));
      else if (strcmp(keyword, "magnify") == 0)
	fin.read((char*)&m_magnify, sizeof(float));
      else if (strcmp(keyword, "showpoints") == 0)
	fin.read((char*)&m_showPoints, sizeof(bool));
      else if (strcmp(keyword, "tube") == 0)
	fin.read((char*)&m_tube, sizeof(bool));
      else if (strcmp(keyword, "halfsection") == 0)
	fin.read((char*)&m_halfSection, sizeof(bool));
      else if (strcmp(keyword, "keepinside") == 0)
	fin.read((char*)&m_keepInside, sizeof(bool));
      else if (strcmp(keyword, "keepends") == 0)
	fin.read((char*)&m_keepEnds, sizeof(bool));
      else if (strcmp(keyword, "color") == 0)
	{
	  fin.read((char*)&f, 3*sizeof(float));
	  m_color = Vec(f[0], f[1], f[2]);
	}
      else if (strcmp(keyword, "opacity") == 0)
	fin.read((char*)&m_opacity, sizeof(float));
      else if (strcmp(keyword, "tfset") == 0)
	fin.read((char*)&m_tfset, sizeof(int));
      else if (strcmp(keyword, "viewmix") == 0)
	fin.read((char*)&m_viewMix, sizeof(float));
      else if (strcmp(keyword, "unionblend") == 0)
	fin.read((char*)&m_unionBlend, sizeof(bool));
      else if (strcmp(keyword, "dtranslate") == 0)
	{
	  fin.read((char*)&f, 3*sizeof(float));
	  m_dtranslate = Vec(f[0], f[1], f[2]);
	}
      else if (strcmp(keyword, "dpivot") == 0)
	{
	  fin.read((char*)&f, 3*sizeof(float));
	  m_dpivot = Vec(f[0], f[1], f[2]);
	}
      else if (strcmp(keyword, "drotaxis") == 0)
	{
	  fin.read((char*)&f, 3*sizeof(float));
	  m_drotaxis = Vec(f[0], f[1], f[2]);
	}
      else if (strcmp(keyword, "drotangle") == 0)
	fin.read((char*)&m_drotangle, sizeof(float));
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
      else if (strcmp(keyword, "tang") == 0)
	{
	  fin.read((char*)&f, 3*sizeof(float));
	  m_tang = Vec(f[0], f[1], f[2]);
	}
      else if (strcmp(keyword, "oxaxis") == 0)
	{
	  fin.read((char*)&f, 3*sizeof(float));
	  m_oxaxis = Vec(f[0], f[1], f[2]);
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
	      m_pointAngle = f[2];
	    }
	}
      else if (strcmp(keyword, "lift") == 0)
	{
	  int npts;
	  fin.read((char*)&npts, sizeof(int));
	  for(int i=0; i<npts; i++)
	    {
	      int l;
	      fin.read((char*)&l, sizeof(int));
	      m_lift.append(l);
	    }
	}
      else if (strcmp(keyword, "hatch") == 0)
	{
	  fin.read((char*)&m_hatch, sizeof(bool));
	  fin.read((char*)&m_hatchGrid, sizeof(bool));
	  fin.read((char*)&m_hxn, sizeof(int));
	  fin.read((char*)&m_hxd, sizeof(int));
	  fin.read((char*)&m_hyn, sizeof(int));
	  fin.read((char*)&m_hyd, sizeof(int));
	  fin.read((char*)&m_hzn, sizeof(int));
	  fin.read((char*)&m_hzd, sizeof(int));
	}
    }

  if (m_lift.count() != m_points.count())
    {
      m_lift.clear();
      for(int i=0; i<m_points.count(); i++)
	m_lift.append(0);
    }
}

float
CropObject::checkBlend(Vec v)
{
  int viewType = cropType();

  QList<Vec> pts;
  QList<float> radX;
  QList<float> radY;
  QList<int> lift;

  float srad1 = m_pointRadX[0];
  float srad2 = m_pointRadX[1];
  float trad1 = m_pointRadY[0];
  float trad2 = m_pointRadY[1];
  float lift1 = m_lift[0];
  float lift2 = m_lift[1];

  pts = m_points;
  radX = m_pointRadX;
  radY = m_pointRadY;
  
  Vec pvec = m_tang;
  Vec saxis = m_xaxis;
  Vec taxis = m_yaxis;

  Vec p0 = (pts[0]+pts[1])*0.5;;
  float plen = (pts[1]-pts[0]).norm()*0.5;

  float vMix = 0.0;
  bool sradEqual = (qAbs(srad1-srad2) < 0.001f);
  bool tradEqual = (qAbs(trad1-trad2) < 0.001f);

  Vec v0;
  float pvlen=0;
  if (!sradEqual || !tradEqual)
    {
      Vec w0 = p0-plen*pvec; // we are given center p0 - now get bottom point
      v0 = v-w0;
      pvlen = pvec*v0;
      pvlen /= (2.0*plen);
    }

  v0 = v-p0;

  float sr, tr;

  if (sradEqual)
    sr = srad1;
  else
    sr = (1.0f-pvlen)*srad1 + pvlen*srad2;
  
  if (tradEqual)
    tr = trad1;
  else
    tr = (1.0-pvlen)*trad1 + pvlen*trad2;


  float z = v0*pvec;
  float c = z/plen;
  float c2 = c*c;

  if (lift1 != 0 || lift2 != 0)
    {
      pvlen = 0.5*(c + 1.0);
      Vec scplen = (1.0-c2)*saxis;
      v0 = v0 - ((1-pvlen)*lift1*scplen + pvlen*lift2*scplen);
    }

  float x = v0*saxis;
  float s = x/sr;

  float y = v0*taxis;
  float t = y/tr;

  float s2 = s*s;
  float t2 = t*t;

  float frc;
  if (c2 <= 1.0 && s2<=1.0 && t2<=1.0)
  {
    if (viewType == CropObject::View_Tear)
      {
	c = StaticFunctions::smoothstep(0.0, 1.0, qAbs(c));
	c = 0.5*(1.0-c);
	c *= (s+1.0);
	frc = qAbs(t);
	frc = (c - frc)/c;
	vMix = StaticFunctions::smoothstep(0.0, viewMix(), frc);
      }
    else if (viewType == CropObject::View_Tube)
      {
	frc = qMin((t2+s2), 1.0f);
	if (vMix > 0.0)
	  vMix = 1.0-StaticFunctions::smoothstep(1.0-viewMix(), 1.0, frc);
	else
	  vMix = 1.0-StaticFunctions::smoothstep(0.95, 1.0, frc);
      }
    else if (viewType == CropObject::View_Ball)
      {
	frc = qMin((c2+t2+s2), 1.0f);
	if (vMix > 0.0)
	  vMix = 1.0-StaticFunctions::smoothstep(1.0-viewMix(), 1.0, frc);
      else
	vMix = 1.0-StaticFunctions::smoothstep(0.95, 1.0, frc);
      }
    else if (viewType == CropObject::View_Block)
      {
	frc = qMax(c2,qMax(t2,s2));
	if (vMix > 0.0)
	  vMix = (1.0-StaticFunctions::smoothstep(1.0-viewMix(), 1.0, frc));
	else
	  vMix = 1.0-StaticFunctions::smoothstep(0.95, 1.0, frc);
      }
  }

  return vMix;
}

bool
CropObject::hatched(float c, float s, float t,
		    bool hatch, bool hatchGrid,
		    int xn, int xd, int yn, int yd, int zn, int zd)
{
  float myfeather;
  if (!hatch)
    return false;
  else
    {
      if (hatchGrid)
	myfeather = 1.0;
      else
	myfeather = 0.0;
      int oz = floor(0.5*(c+1.0)*100.0);
      int oy = floor(0.5*(t+1.0)*100.0);
      int ox = floor(0.5*(s+1.0)*100.0);
      float hfe = 1.0;
      if (xn > 0 && xd > 0)
	hfe = hfe * StaticFunctions::smoothstep(xd-1, xd+1, ox%xn);
      if (yn > 0 && yd > 0)
	hfe = hfe * StaticFunctions::smoothstep(yd-1, yd+1, oy%yn);
      if (zn > 0 && zd > 0) 
	hfe = hfe * StaticFunctions::smoothstep(zd-1, zd+1, oz%zn);
      if (hatchGrid) 
	myfeather = hfe; 
      else 
	myfeather = 1.0 - hfe; 
    }

  if (myfeather < 1.0)
    return true;
  else
    return false;
}

bool
CropObject::checkCropped(Vec v)
{
  QList<Vec> pts;
  QList<float> radX;
  QList<float> radY;
  QList<int> lift;

  float srad1 = m_pointRadX[0];
  float srad2 = m_pointRadX[1];
  float trad1 = m_pointRadY[0];
  float trad2 = m_pointRadY[1];
  float lift1 = m_lift[0];
  float lift2 = m_lift[1];

  pts = m_points;
  radX = m_pointRadX;
  radY = m_pointRadY;
  
  Vec pvec = m_tang;
  Vec saxis = m_xaxis;
  Vec taxis = m_yaxis;

  if (m_cropType == Crop_Tube)
    {
      Vec p0 = pts[0];
      float plen = (pts[1]-pts[0]).norm();
  
      Vec v0 = v - p0;
      float pvlen = pvec*v0;

      float c = 2*(pvlen/plen - 0.5);
      float c2 = c*c;

      Vec pv = v - (p0 + pvlen*pvec);
      pvlen /= plen;

      if (lift1 != 0 || lift2 != 0)
	{
	  Vec sc2 = (1.0-c2)*saxis;
	  pv -= sc2*((1.0-pvlen)*lift1 + pvlen*lift2);
	}

      float s = pv*saxis;
      float t = pv*taxis;

      float sr = s/((1.0-pvlen)*srad1 + pvlen*srad2);
      float tr = t/((1.0-pvlen)*trad1 + pvlen*trad2);

      pvlen = sr*sr + tr*tr;

      s = sr; t = tr;

      bool ends = (pvlen < 0.0 || pvlen > plen);
      if (!m_keepEnds && ends)
	return hatched(c, s, t, m_hatch, m_hatchGrid,
		       m_hxn,m_hxd, m_hyn,m_hyd, m_hzn,m_hzd);

      
      if (m_keepEnds)
	{
	  if (m_keepInside)
	    {
	      if (m_halfSection)
		{
		  if (c2<1.0 && t>0.0 && pvlen>1.0)
		    return hatched(c, s, t, m_hatch, m_hatchGrid,
				   m_hxn,m_hxd, m_hyn,m_hyd, m_hzn,m_hzd);
		}
	      else
		{
		  if (c2<1.0 && pvlen>1.0)
		    return hatched(c, s, t, m_hatch, m_hatchGrid,
				   m_hxn,m_hxd, m_hyn,m_hyd, m_hzn,m_hzd);
		}
	    }
	  else
	    {
	      if (m_halfSection)
		{
		  if (c2<1.0 && (t<0.0 || pvlen<1.0))
		    return hatched(c, s, t, m_hatch, m_hatchGrid,
				   m_hxn,m_hxd, m_hyn,m_hyd, m_hzn,m_hzd);
		}
	      else
		{
		  if (c2<1.0 && pvlen<1.0)
		    return hatched(c, s, t, m_hatch, m_hatchGrid,
				   m_hxn,m_hxd, m_hyn,m_hyd, m_hzn,m_hzd);
		}
	    }
	}
      else // do not keep ends
	{
	  if (m_keepInside)
	    {
	      if (m_halfSection)
		{
		  if (t>0.0 && pvlen>1.0)
		    return hatched(c, s, t, m_hatch, m_hatchGrid,
				   m_hxn,m_hxd, m_hyn,m_hyd, m_hzn,m_hzd);
		}
	      else
		{
		  if (pvlen>1.0)
		    return hatched(c, s, t, m_hatch, m_hatchGrid,
				   m_hxn,m_hxd, m_hyn,m_hyd, m_hzn,m_hzd);
		}
	    }
	  else
	    {
	      if (m_halfSection)
		{
		  if (t<0.0 || (t>0.0 && pvlen<1.0))
		    return hatched(c, s, t, m_hatch, m_hatchGrid,
				   m_hxn,m_hxd, m_hyn,m_hyd, m_hzn,m_hzd);
		}
	      else
		{
		  if (pvlen<1.0)
		    return hatched(c, s, t, m_hatch, m_hatchGrid,
				   m_hxn,m_hxd, m_hyn,m_hyd, m_hzn,m_hzd);
		}
	    }
	}      
    }
  else if (m_cropType == Crop_Box)
    {
      Vec p0 = (pts[0] + pts[1])/2;
      float plen = (pts[1]-pts[0]).norm()/2;

      Vec w0 = p0 - plen*pvec;
      Vec v0 = v - w0;
      float pvlen = (pvec*v0)/(2*plen);

      v0 = v - p0;

      float sr = (1.0-pvlen)*srad1 + pvlen*srad2;
      float tr = (1.0-pvlen)*trad1 + pvlen*trad2;

      float c = v0*pvec/plen;

      if (lift1 != 0 || lift2 != 0)
	{
	  float cplen = c;
	  pvlen = 0.5*(cplen + 1.0);
	  Vec scplen = (1.0 - cplen*cplen)*saxis;
	  v0 -= scplen*((1.0-pvlen)*lift1 + pvlen*lift2);
	}

      float s = (v0*saxis)/sr;
      float t = (v0*taxis)/tr;
      float c2 = c*c;
      float s2 = s*s;
      float t2 = t*t;

      if (!m_keepEnds)
	{
	  if (c2 > 1.0)
	    return hatched(c, s, t, m_hatch, m_hatchGrid,
			   m_hxn,m_hxd, m_hyn,m_hyd, m_hzn,m_hzd);

	  if (m_keepInside)
	    {
	      if (m_halfSection)
		{
		  if (t>0.0 && (s2>1.0 || t2>1.0))
		    return hatched(c, s, t, m_hatch, m_hatchGrid,
				   m_hxn,m_hxd, m_hyn,m_hyd, m_hzn,m_hzd);
		}
	      else
		{
		  if (s2>1.0 || t2>1.0)
		    return hatched(c, s, t, m_hatch, m_hatchGrid,
				   m_hxn,m_hxd, m_hyn,m_hyd, m_hzn,m_hzd);
		}
	    }
	  else
	    {
	      if (m_halfSection)
		{
		  if (t<0.0 || (s2<1.0 && t2<1.0))
		    return hatched(c, s, t, m_hatch, m_hatchGrid,
				   m_hxn,m_hxd, m_hyn,m_hyd, m_hzn,m_hzd);
		}
	      else
		{
		  if (s2<1.0 && t2<1.0)
		    return hatched(c, s, t, m_hatch, m_hatchGrid,
				   m_hxn,m_hxd, m_hyn,m_hyd, m_hzn,m_hzd);
		}
	    }
	}
      else
	{
	  if (m_keepInside)
	    {
	      if (m_halfSection)
		{
		  if (c2<=1.0 && (t>0.0 && (s2>1.0 || t2>1.0)))
		    return hatched(c, s, t, m_hatch, m_hatchGrid,
				   m_hxn,m_hxd, m_hyn,m_hyd, m_hzn,m_hzd);
		}
	      else
		{
		  if (c2<=1.0 && (s2>1.0 || t2>1.0))
		    return hatched(c, s, t, m_hatch, m_hatchGrid,
				   m_hxn,m_hxd, m_hyn,m_hyd, m_hzn,m_hzd);
		}
	    }
	  else
	    {
	      if (m_halfSection)
		{
		  if (c2<=1.0 && (t<0.0 || (s2<1.0 && t2<1.0)))
		    return hatched(c, s, t, m_hatch, m_hatchGrid,
				   m_hxn,m_hxd, m_hyn,m_hyd, m_hzn,m_hzd);
		}
	      else
		{
		  if (c2<=1.0 && (s2<1.0 && t2<1.0))
		    return hatched(c, s, t, m_hatch, m_hatchGrid,
				   m_hxn,m_hxd, m_hyn,m_hyd, m_hzn,m_hzd);
		}
	    }
	}
    }
  else if (m_cropType == Crop_Ellipsoid)
    {
      Vec p0 = (pts[0] + pts[1])/2;
      float plen = (pts[1]-pts[0]).norm()/2;

      float srad = (srad1+srad2)/2;
      float trad = (trad1+trad2)/2;

      Vec v0 = v - p0;
      float c = (v0*pvec)/plen;
      float c2 = c*c;

      float pvlen;
      if (lift1 != 0 || lift2 != 0)
	{
	  pvlen = 0.5*(c + 1.0);
	  Vec scplen = (1.0 - c2)*saxis;
	  v0 -= scplen*((1.0-pvlen)*lift1 + pvlen*lift2);
	}

      float a = (v0*saxis)/srad;
      float b = (v0*taxis)/trad;
      float a2 = a*a;
      float b2 = b*b;

      pvlen = a2 + b2 + c2;

      if (!m_keepEnds)
	{
	  if (c2 > 1.0)
	    return hatched(c, a, b, m_hatch, m_hatchGrid,
			   m_hxn,m_hxd, m_hyn,m_hyd, m_hzn,m_hzd);

	  if (m_keepInside)
	    {
	      if (m_halfSection)
		{
		  if (b>0.0 && pvlen>1.0)
		    return hatched(c, a, b, m_hatch, m_hatchGrid,
				   m_hxn,m_hxd, m_hyn,m_hyd, m_hzn,m_hzd);
		}
	      else
		{
		  if (pvlen > 1.0)
		    return hatched(c, a, b, m_hatch, m_hatchGrid,
				   m_hxn,m_hxd, m_hyn,m_hyd, m_hzn,m_hzd);
		}
	    }
	  else
	    {
	      if (m_halfSection)
		{
		  if (b<0.0 || pvlen<1.0)
		    return hatched(c, a, b, m_hatch, m_hatchGrid,
				   m_hxn,m_hxd, m_hyn,m_hyd, m_hzn,m_hzd);
		}
	      else
		{
		  if (pvlen < 1.0)
		    return hatched(c, a, b, m_hatch, m_hatchGrid,
				   m_hxn,m_hxd, m_hyn,m_hyd, m_hzn,m_hzd);
		}
	    }
	}
      else
	{
	  if (m_keepInside)
	    {
	      if (m_halfSection)
		{
		  if (b>0.0 && pvlen>1.0)
		    return hatched(c, a, b, m_hatch, m_hatchGrid,
				   m_hxn,m_hxd, m_hyn,m_hyd, m_hzn,m_hzd);
		}
	      else
		{
		  if (c2 < 1.0 && pvlen > 1.0)
		    return hatched(c, a, b, m_hatch, m_hatchGrid,
				   m_hxn,m_hxd, m_hyn,m_hyd, m_hzn,m_hzd);
		}
	    }
	  else
	    {
	      if (m_halfSection)
		{
		  if (b<0.0 || pvlen<1.0)
		    return hatched(c, a, b, m_hatch, m_hatchGrid,
				   m_hxn,m_hxd, m_hyn,m_hyd, m_hzn,m_hzd);
		}
	      else
		{
		  if (pvlen < 1.0)
		    return hatched(c, a, b, m_hatch, m_hatchGrid,
				   m_hxn,m_hxd, m_hyn,m_hyd, m_hzn,m_hzd);
		}
	    }
	}

    }

  return true;
}

float
CropObject::checkTear(Vec v, Vec &newV)
{
  newV = v;

  int viewType = cropType();

  QList<Vec> pts;
  QList<float> radX;
  QList<float> radY;
  QList<int> lift;

  float srad1 = m_pointRadX[0];
  float srad2 = m_pointRadX[1];
  float trad1 = m_pointRadY[0];
  float trad2 = m_pointRadY[1];
  float lift1 = m_lift[0];
  float lift2 = m_lift[1];

  pts = m_points;
  radX = m_pointRadX;
  radY = m_pointRadY;
  
  Vec pvec = m_tang;
  Vec saxis = m_xaxis;
  Vec taxis = m_yaxis;

  Vec p0 = (pts[0]+pts[1])*0.5;;
  float plen = (pts[1]-pts[0]).norm()*0.5;

  float vMix = 0.0;
  bool sradEqual = (qAbs(srad1-srad2) < 0.001f);
  bool tradEqual = (qAbs(trad1-trad2) < 0.001f);

  Vec v0;
  float pvlen=0;
  if (!sradEqual || !tradEqual)
    {
      Vec w0 = p0-plen*pvec; // we are given center p0 - now get bottom point
      v0 = v-w0;
      pvlen = pvec*v0;
      pvlen /= (2.0*plen);
    }

  v0 = v-p0;

  float sr, tr;

  if (sradEqual)
    sr = srad1;
  else
    sr = (1.0f-pvlen)*srad1 + pvlen*srad2;
  
  if (tradEqual)
    tr = trad1;
  else
    tr = (1.0-pvlen)*trad1 + pvlen*trad2;

  float z = v0*pvec;
  float c = z/plen;
  float c2 = c*c;

  if (lift1 != 0 || lift2 != 0)
    {
      pvlen = 0.5*(c + 1.0);
      Vec scplen = (1.0-c2)*saxis;
      v0 = v0 - ((1-pvlen)*lift1*scplen + pvlen*lift2*scplen);
    }

  float x = v0*saxis;
  float s = x/sr;

  float y = v0*taxis;
  float t = y/tr;

  float s2 = s*s;
  float t2 = t*t;

  float frc;

  if (c2 <= 1.0 && s2<=1.0 && t2<=1.0)
  {
    if (viewType == CropObject::Tear_Tear)
      {
	c = StaticFunctions::smoothstep(0.0, 0.9, qAbs(c));
	c = 0.5*(1.0-c);
	c *= (s+1.0);
	frc = qAbs(t);
	if (frc < c) return 1;
	frc = (frc - c)/(1.0 - c);
	float st = 0.0f;
	if (t > 0) st = frc*tr;
	else if (t < 0) st = -frc*tr;
	newV = p0 + z*pvec + x*saxis + st*taxis;
	vMix = 1.0-StaticFunctions::smoothstep(0.0, 0.05, frc);
      }
    else if (viewType == CropObject::Tear_Wedge)
      {
	c = 0.5*(1+c);
	c = StaticFunctions::smoothstep(0.4, 1.0, c);
	c = 0.5*(1.0-c); 
	c *= (s+1.0);
	frc = qAbs(t);
	if (frc < c) return 1;
	frc = (frc - c)/(1.0 - c);
	float st = 0.0f;
	if (t > 0) st = frc*tr;
	else if (t < 0) st = -frc*tr;
	newV = p0 + z*pvec + x*saxis + st*taxis;
	vMix = 1.0-StaticFunctions::smoothstep(0.0, 0.05, frc);
      }
    else if (viewType == CropObject::Tear_Hole)
      {
	s = qMax(0.0f, s);
	s = s*s;
	c = 0.4 + 0.4*s;
	frc = qMin((c2+t2), 1.0f);
	Vec apt0 = p0 + x*saxis;
	if (frc < c) return 1;
	frc = (frc - c)/(1.0 - c);
	newV = (1-frc)*apt0 + frc*v;
	vMix = 1.0-StaticFunctions::smoothstep(0.0, 0.05, frc);
      }
    else if (viewType == CropObject::Tear_Curl)
      {
	Vec oc = p0 + z*pvec + (sr/2)*saxis - tr*taxis;

	Vec vdir = v-oc;
	float vlen = vdir.norm();
	vdir.normalize();

	float theta = acos(-vdir*saxis);

	frc = theta/3.14159;

	float r0 = 0.5*sr;
	float r1 = 1.5*sr;
	r0 = (1-frc)*r0 + frc*0.1*r0;
	r1 = (1-frc)*r1 + frc*0.1*r1;

	if (vlen < r0 || vlen > r1)
	{
	  vMix = 1.0 - StaticFunctions::smoothstep(0.9, 1.0, c2);
	  vMix = qMin(vMix, 1.0f-StaticFunctions::smoothstep(0.9, 1.0, s2));
	  vMix = qMin(vMix, 1.0f-StaticFunctions::smoothstep(0.9, 1.0, t2));
	}
	else
	{
	  s = (vlen-r0)/(r1-r0);
	  vMix = 1.0 - StaticFunctions::smoothstep(0.0, 0.1, s);
	  vMix = qMax(vMix, StaticFunctions::smoothstep(0.7, 1.0, s));
	  s = 2.0*s;
	  newV = p0 + z*pvec - (-tr+2.0*frc*tr)*taxis - s*sr*saxis;
	}
      }
  }

  return vMix;
}


void
CropObject::undo()
{
  m_undo.undo();

  m_points = m_undo.points();
  m_pointRadX = m_undo.pointRadX();
  m_pointRadY = m_undo.pointRadY();

  computeTangents();
  m_updateFlag = true;
}

void
CropObject::redo()
{
  m_undo.redo();

  m_points = m_undo.points();
  m_pointRadX = m_undo.pointRadX();
  m_pointRadY = m_undo.pointRadY();

  computeTangents();
  m_updateFlag = true;
}

void
CropObject::updateUndo()
{
  m_undo.append(m_points, m_pointRadX, m_pointRadY);
}
