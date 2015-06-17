#include "fiber.h"
#include "global.h"
#include <QMessageBox>

Fiber::Fiber()
{
  tag = 0;
  thickness = 1;
  selected = false;
  seeds.clear();
  smoothSeeds.clear();
  trace.clear();
  m_dPoints.clear();
  m_wPoints.clear();
  m_hPoints.clear();
  m_dSeeds.clear();
  m_wSeeds.clear();
  m_hSeeds.clear();
  m_sections = 4;
  m_tube.clear();
}

Fiber::~Fiber()
{
  tag = 0;
  thickness = 1;
  selected = false;
  seeds.clear();
  smoothSeeds.clear();
  trace.clear();
  m_dPoints.clear();
  m_wPoints.clear();
  m_hPoints.clear();
  m_dSeeds.clear();
  m_wSeeds.clear();
  m_hSeeds.clear();
  m_sections = 4;
  m_tube.clear();
}

Fiber&
Fiber::operator=(const Fiber& f)
{
  tag = f.tag;
  thickness = f.thickness;
  selected = f.selected;
  seeds = f.seeds;
//  smoothSeeds = f.smoothSeeds;
//  trace = f.trace;
//  m_dPoints = f.m_dPoints;
//  m_wPoints = f.m_wPoints;
//  m_hPoints = f.m_hPoints;
//  m_dSeeds = f.m_dSeeds;
//  m_wSeeds = f.m_wSeeds;
//  m_hSeeds = f.m_hSeeds;

  updateTrace();

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
Fiber::containsSeed(int d, int w, int h)
{
  QList<QVector4D> dpts = m_dSeeds.values(d);

  for(int i=0; i<dpts.count(); i++)
    if ((dpts[i].toPointF()-QPointF(h, w)).manhattanLength() < 10)
      return true;

  return false;
}

bool
Fiber::contains(int d, int w, int h)
{
  QList<QVector4D> dpts = m_dPoints.values(d);

  for(int i=0; i<dpts.count(); i++)
    if ((dpts[i].toPointF()-QPointF(h, w)).manhattanLength() < 10)
	return true;

  return false;
}

void
Fiber::updateTrace()
{  
  m_tube.clear();

  int ptend = seeds.count();

  smoothSeeds.clear();
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
      m_dPoints.insert((int)seeds[0].z, QVector4D(seeds[0].x, seeds[0].y, tag, thickness));
      m_wPoints.insert((int)seeds[0].y, QVector4D(seeds[0].x, seeds[0].z, tag, thickness));
      m_hPoints.insert((int)seeds[0].x, QVector4D(seeds[0].y, seeds[0].z, tag, thickness));      
       m_dSeeds.insert((int)seeds[0].z, QVector4D(seeds[0].x, seeds[0].y, tag, thickness));
       m_wSeeds.insert((int)seeds[0].y, QVector4D(seeds[0].x, seeds[0].z, tag, thickness));
       m_hSeeds.insert((int)seeds[0].x, QVector4D(seeds[0].y, seeds[0].z, tag, thickness));      
      return;
    }



  for(int ptn=0; ptn<ptend; ptn++)
    {
      Vec p = seeds[ptn];
      m_dSeeds.insert((int)p.z, QVector4D(p.x, p.y, tag, thickness));
      m_wSeeds.insert((int)p.y, QVector4D(p.x, p.z, tag, thickness));
      m_hSeeds.insert((int)p.x, QVector4D(p.y, p.z, tag, thickness));
    }

//  // linear
//  for(int ptn=0; ptn<(ptend-1); ptn++)
//    {
//      int nxt = (ptn+1)%ptend;
//      trace += line3d(seeds[ptn], seeds[nxt]);
//    }
//  for(int i=0; i<trace.count(); i++)
//    {
//      Vec p = trace[i];
//      m_dPoints.insert((int)p.z, QVector4D(p.x, p.y, tag, thickness));
//      m_wPoints.insert((int)p.y, QVector4D(p.x, p.z, tag, thickness));
//      m_hPoints.insert((int)p.x, QVector4D(p.y, p.z, tag, thickness));
//    }

  // spline
  QVector<Vec> tv;
  tv.resize(ptend);
  for(int i=0; i<ptend; i++)
    {
      int nxt = qMin(ptend-1, (i+1));
      int prv = qMax(0, i-1);
      tv[i] = (seeds[nxt]-seeds[prv])/2;
    }

  for(int ptn=0; ptn<(ptend-1); ptn++)
    {
      int nxt = (ptn+1)%ptend;
      Vec diff = seeds[nxt]-seeds[ptn];
      int len = diff.norm()/5;
      if (len > 1)
	{
	  for(int i=0; i<len; i++)
	    {
	      float frc = (float)i/(float)len;
	      Vec a1 = 3*diff - 2*tv[ptn] - tv[nxt];
	      Vec a2 = -2*diff + tv[ptn] + tv[nxt];
	      Vec p = seeds[ptn] + frc*(tv[ptn] + frc*(a1 + frc*a2));
	      smoothSeeds << p;
	    }
	}
    }

  ptend = smoothSeeds.count();
  for(int ptn=0; ptn<(ptend-1); ptn++)
    {
      int nxt = (ptn+1)%ptend;
      trace += line3d(smoothSeeds[ptn], smoothSeeds[nxt]);
    }
  for(int i=0; i<trace.count(); i++)
    {
      Vec p = trace[i];
      m_dPoints.insert((int)p.z, QVector4D(p.x, p.y, tag, thickness));
      m_wPoints.insert((int)p.y, QVector4D(p.x, p.z, tag, thickness));
      m_hPoints.insert((int)p.x, QVector4D(p.y, p.z, tag, thickness));
    }

  generateTube(1);
}

QVector<QVector4D>
Fiber::xyPoints(int type, int slc)
{
  QVector<QVector4D> xypoints;
  if (type == 0) xypoints = QVector<QVector4D>::fromList(m_dPoints.values(slc));
  if (type == 1) xypoints = QVector<QVector4D>::fromList(m_wPoints.values(slc));
  if (type == 2) xypoints = QVector<QVector4D>::fromList(m_hPoints.values(slc));
  return xypoints;
}

QVector<QVector4D>
Fiber::xySeeds(int type, int slc)
{
  QVector<QVector4D> xypoints;
  if (type == 0) xypoints = QVector<QVector4D>::fromList(m_dSeeds.values(slc));
  if (type == 1) xypoints = QVector<QVector4D>::fromList(m_wSeeds.values(slc));
  if (type == 2) xypoints = QVector<QVector4D>::fromList(m_hSeeds.values(slc));
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

void
Fiber::generateTube(float scale)
{  
  m_sections = 20;
  m_tube.clear();

  int nsmoothSeeds = smoothSeeds.count();

  if (nsmoothSeeds < 2)
    return;
  
  // Frenet frame - using Double Reflection Method
  QList<Vec> pathT;
  QList<Vec> pathX;
  QList<Vec> pathY;

  Vec t0 = smoothSeeds[1] - smoothSeeds[0];
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

  pathT << t0;
  pathX << r0;
  pathY << s0;
  for(int i=0; i<nsmoothSeeds-1; i++)
    {
      Vec t1, r1, s1;
      Vec t0L, r0L, s0L;
      Vec v1, v2;
      float c1, c2;

      v1 = smoothSeeds[i+1]-smoothSeeds[i];
      c1 = v1*v1;
      r0L = r0-(2.0/c1)*(v1*r0)*v1;
      t0L = t0-(2.0/c1)*(v1*t0)*v1;

      t1 = smoothSeeds[i+1]-smoothSeeds[i];

      if (i < nsmoothSeeds-2)
	t1 = smoothSeeds[i+2]-smoothSeeds[i];
	
      t1.normalize();
      v2 = t1 - t0L;
      c2 = v2*v2;
      r1 = r0L-(2.0/c2)*(v2*r0L)*v2;
      r1.normalize();
      s1 = t1^r1;
      s1.normalize();      

      pathT << t1;
      pathX << r1;
      pathY << s1;

      t0 = t1;
      r0 = r1;
      s0 = s1;
    }

  QList<Vec> csec1, norm1;
  for(int i=0; i<nsmoothSeeds; i++)
    {
      QList<Vec> csec2, norm2;

      csec2 = getCrossSection(scale*thickness*0.5, m_sections,
			      pathX[i], pathY[i]);
      norm2 = getNormals(csec2, pathT[i]);

      if (i == 0 || i==nsmoothSeeds-1)
	addRoundCaps(i, pathT[i], csec2, norm2);

      if (i > 0)
	{
	  for(int j=0; j<csec1.count(); j++)
	    {
	      Vec vox1 = smoothSeeds[i-1] + csec1[j];
	      m_tube << norm1[j];
	      m_tube << vox1;

	      Vec vox2 = smoothSeeds[i] + csec2[j];
	      m_tube << norm2[j];
	      m_tube << vox2;
	    }	      
	}

      csec1 = csec2;
      norm1 = norm2;      
    }

  csec1.clear();
  norm1.clear();
}

void
Fiber::addRoundCaps(int i,
		    Vec tang,
		    QList<Vec> csec2,
		    QList<Vec> norm2)
{
  int sections = csec2.count()-1;
  int ksteps = 4;
  Vec ctang = -tang;
  if (i==0) ctang = tang;
  QList<Vec> csec = csec2;
  float rad = thickness/2;
  for(int k=0; k<ksteps-1; k++)
    {
      float ct1 = cos(1.57*(float)k/(float)ksteps);
      float ct2 = cos(1.57*(float)(k+1)/(float)ksteps);
      float st1 = sin(1.57*(float)k/(float)ksteps);
      float st2 = sin(1.57*(float)(k+1)/(float)ksteps);
      for(int j=0; j<csec2.count(); j++)
	{
	  Vec norm = csec2[j]*ct1 - ctang*rad*st1;
	  Vec vox2 = smoothSeeds[i] + norm;
	  if (k==0)
	    m_tube << norm2[j];
	  else
	    {  
	      norm.normalize();
	      m_tube << norm;
	    }

	  m_tube << vox2;
	  
	  norm = csec2[j]*ct2 - ctang*rad*st2;
	  vox2 = smoothSeeds[i] + norm;
	  norm.normalize();
	  m_tube << norm;
	  m_tube << vox2;
	}
    }

  
  // add flat ends
  float ct2 = cos(1.57*(float)(ksteps-1)/(float)ksteps);
  float st2 = sin(1.57*(float)(ksteps-1)/(float)ksteps);
  int halfway = sections/2;
  for(int j=0; j<=halfway; j++)
    {
      Vec norm = csec2[j]*ct2 - ctang*rad*st2;
      Vec vox2 = smoothSeeds[i] + norm;
      norm.normalize();
      m_tube << norm;
      m_tube << vox2;
      
      if (j < halfway)
	{
	  norm = csec2[sections-j]*ct2 -
	    ctang*rad*st2;
	  vox2 = smoothSeeds[i] + norm;
	  norm.normalize();
	  m_tube << norm;
	  m_tube << vox2;
	}
    }
}
QList<Vec>
Fiber::getCrossSection(float r, int sections,
		       Vec xaxis, Vec yaxis)
{
  QList<Vec> csec;
  for(int j=0; j<sections; j++)
    {
      float t = 360*(float)j/(float)sections;
      float x = r*cos(qDegreesToRadians(t));
      float y = r*sin(qDegreesToRadians(t));
      Vec v = x*xaxis + y*yaxis;
      csec.append(v);
    }

  csec.append(csec[0]);

  return csec;
}

QList<Vec>
Fiber::getNormals(QList<Vec> csec, Vec tang)
{
  QList<Vec> norm;
  int sections = csec.count();
  for(int j=0; j<sections; j++)
    {
      int nxt = (j+1)%sections;
      int prv = (j-1);
      if (prv < 0) prv = sections-1;

      Vec v = csec[prv]-csec[nxt];

      v.normalize();
      v = tang^v;

      norm.append(v);
    }

  return norm;
}
