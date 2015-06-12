#include "fiber.h"
#include <QMessageBox>

Fiber::Fiber()
{
  tag = 0;
  thickness = 1;
  selected = false;
  seeds.clear();
  trace.clear();
  m_dPoints.clear();
  m_wPoints.clear();
  m_hPoints.clear();
  m_dSeeds.clear();
  m_wSeeds.clear();
  m_hSeeds.clear();
}

Fiber::~Fiber()
{
  tag = 0;
  thickness = 1;
  selected = false;
  seeds.clear();
  trace.clear();
  m_dPoints.clear();
  m_wPoints.clear();
  m_hPoints.clear();
  m_dSeeds.clear();
  m_wSeeds.clear();
  m_hSeeds.clear();
}

Fiber&
Fiber::operator=(const Fiber& f)
{
  tag = f.tag;
  thickness = f.thickness;
  selected = f.selected;
  seeds = f.seeds;
  trace = f.trace;
  m_dPoints = f.m_dPoints;
  m_wPoints = f.m_wPoints;
  m_hPoints = f.m_hPoints;
  m_dSeeds = f.m_dSeeds;
  m_wSeeds = f.m_wSeeds;
  m_hSeeds = f.m_hSeeds;

  return *this;
}

void
Fiber::addPoint(int d, int w, int h)
{
  Vec v = Vec(h,w,d);

  if (seeds.count() < 2)
    seeds << v;
  else
    {
      bool addAtEnd = true;
      for (int i=0; i<seeds.count()-1; i++)
	{
	  Vec v0 = seeds[i];
	  Vec v1 = seeds[i+1];
	  Vec p = v1-v0;
	  Vec pu = p.unit();
	  Vec pv = v-v0;
	  float proj = pu*pv;
	  if (proj < 0)
	    {
	      seeds.insert(i, v);
	      addAtEnd = false;
	      break;
	    }
	  else
	    {
	      if (proj < p.norm())
		{
		  seeds.insert(i+1, v);
		  addAtEnd = false;
		  break;
		}
	    }
	}
      if (addAtEnd)
	seeds << v;
    }
  
  updateTrace();
}

void
Fiber::removePoint(int d, int w, int h)
{
  for(int i=0; i<seeds.count(); i++)
    {
      if ((seeds[i]-Vec(h, w, d)).squaredNorm() < 10)
	{
	  seeds.removeAt(i);
	  updateTrace();
	  break;
	}
    }
}

bool
Fiber::contains(int d, int w, int h)
{
  for(int i=0; i<seeds.count(); i++)
    if ((seeds[i]-Vec(h, w, d)).squaredNorm() < 10)
      return true;

  return false;
}

void
Fiber::updateTrace()
{
  int ptend = seeds.count();

  trace.clear();
  m_dPoints.clear();
  m_wPoints.clear();
  m_hPoints.clear();

  m_dSeeds.clear();
  m_wSeeds.clear();
  m_hSeeds.clear();

  if (ptend == 0)
    return;

  if (ptend == 1)
    {
      trace << seeds[0];
      m_dPoints.insert((int)seeds[0].z, QPointF(seeds[0].x, seeds[0].y));
      m_wPoints.insert((int)seeds[0].y, QPointF(seeds[0].x, seeds[0].z));
      m_hPoints.insert((int)seeds[0].x, QPointF(seeds[0].y, seeds[0].z));      

      m_dSeeds.insert((int)seeds[0].z, QPointF(seeds[0].x, seeds[0].y));
      m_wSeeds.insert((int)seeds[0].y, QPointF(seeds[0].x, seeds[0].z));
      m_hSeeds.insert((int)seeds[0].x, QPointF(seeds[0].y, seeds[0].z));      
      return;
    }



  for(int ptn=0; ptn<ptend; ptn++)
    {
      Vec p = seeds[ptn];
      m_dSeeds.insert((int)p.z, QPointF(p.x, p.y));
      m_wSeeds.insert((int)p.y, QPointF(p.x, p.z));
      m_hSeeds.insert((int)p.x, QPointF(p.y, p.z));
    }

  // linear
  for(int ptn=0; ptn<(ptend-1); ptn++)
    {
      int nxt = (ptn+1)%ptend;
      trace += line3d(seeds[ptn], seeds[nxt]);
    }
  for(int i=0; i<trace.count(); i++)
    {
      Vec p = trace[i];
      m_dPoints.insert((int)p.z, QPointF(p.x, p.y));
      m_wPoints.insert((int)p.y, QPointF(p.x, p.z));
      m_hPoints.insert((int)p.x, QPointF(p.y, p.z));
    }

//  // spline
//  QVector<Vec> tv;
//  tv.resize(ptend);
//  for(int i=0; i<ptend; i++)
//    {
//      int nxt = (i+1)%ptend;
//      int prv = i-1;
//      if (prv<0) prv = ptend-1;
//      tv[i] = (seeds[nxt]-seeds[prv])/2;
//    }
//
//  for(int ptn=0; ptn<(ptend-1); ptn++)
//    {
//      int nxt = (ptn+1)%ptend;
//      Vec diff = seeds[nxt]-seeds[ptn];
//      int len = diff.norm();
//      if (len > 1)
//	{
//	  for(int i=0; i<len; i++)
//	    {
//	      float frc = i/(float)len;
//	      Vec a1 = 3*diff - 2*tv[ptn] - tv[nxt];
//	      Vec a2 = -2*diff + tv[ptn] + tv[nxt];
//	      Vec p = seeds[ptn] + frc*(tv[ptn] + frc*(a1 + frc*a2));
//	      trace << p;
//
//	      m_dPoints.insert((int)p.z, QPointF(p.x, p.y));
//	      m_wPoints.insert((int)p.y, QPointF(p.x, p.z));
//	      m_hPoints.insert((int)p.x, QPointF(p.y, p.z));
//	    }
//	}
//    }
}

QVector<QPointF>
Fiber::xyPoints(int type, int slc)
{
  QVector<QPointF> xypoints;
  if (type == 0) xypoints = QVector<QPointF>::fromList(m_dPoints.values(slc));
  if (type == 1) xypoints = QVector<QPointF>::fromList(m_wPoints.values(slc));
  if (type == 2) xypoints = QVector<QPointF>::fromList(m_hPoints.values(slc));
  return xypoints;
}

QVector<QPointF>
Fiber::xySeeds(int type, int slc)
{
  QVector<QPointF> xypoints;
  if (type == 0) xypoints = QVector<QPointF>::fromList(m_dSeeds.values(slc));
  if (type == 1) xypoints = QVector<QPointF>::fromList(m_wSeeds.values(slc));
  if (type == 2) xypoints = QVector<QPointF>::fromList(m_hSeeds.values(slc));
  return xypoints;
}


QList<Vec>
Fiber::line3d(Vec v0, Vec v1)
{
  QList<Vec> line;

  int gx0, gy0, gz0, gx1, gy1, gz1;
  gx0 = v0.x;
  gy0 = v0.y;
  gz0 = v0.z;
  gx1 = v1.x;
  gy1 = v1.y;
  gz1 = v1.z;

  float vx = gx1 - gx0;
  float vy = gy1 - gy0;
  float vz = gz1 - gz0;
  
  int sx = (gx1>gx0) ? 1 : (gx1<gx0) ? -1 : 0;
  int sy = (gy1>gy0) ? 1 : (gy1<gy0) ? -1 : 0;
  int sz = (gz1>gz0) ? 1 : (gz1<gz0) ? -1 : 0;
 
  int gx = gx0;
  int gy = gy0;
  int gz = gz0;

  //Planes for each axis that we will next cross
  int gxp = gx0 + ((gx1>gx0) ? 1 : 0);
  int gyp = gy0 + ((gy1>gy0) ? 1 : 0);
  int gzp = gz0 + ((gz1>gz0) ? 1 : 0);
  
  do {
    line << Vec(gx, gy, gz);

    if (gx == gx1 &&
	gy == gy1 &&
	gz == gz1)
      break;
    
    //Which plane do we cross first?
    float xr=0,yr=0,zr=0;
    if (qAbs(vx) > 0) xr = qAbs((gxp - gx0)/vx);
    if (qAbs(vy) > 0) yr = qAbs((gyp - gy0)/vy);
    if (qAbs(vz) > 0) zr = qAbs((gzp - gz0)/vz);

    if (sx != 0 &&
	(sy == 0 || xr < yr) &&
	(sz == 0 || xr < zr))
      {
      gx += sx;
      gxp += sx;
    }
    else if (sy != 0 && 
	     (sz == 0 || yr < zr))
      {
	gy += sy;
	gyp += sy;
      }
    else if (sz != 0)
      {
	gz += sz;
	gzp += sz;
      }
    
    if (qAbs(gx-gx0) > qAbs(vx) ||
	qAbs(gy-gy0) > qAbs(vy) ||
	qAbs(gz-gz0) > qAbs(vz))
      break;
    
  } while (true);
  
  return line;
}
