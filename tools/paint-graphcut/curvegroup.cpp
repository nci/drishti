#include "curvegroup.h"
#include "morphcurve.h"
#include "global.h"
#include "morphslice.h"

Curve::Curve()
{
  type = 0;
  tag = 0;
  thickness = 1;
  closed = true;
  selected = false;
  pts.clear();
  seedpos.clear();
  seeds.clear();
}
Curve::~Curve()
{
  type = 0;
  tag = 0;
  thickness = 1;
  closed = true;
  selected = false;
  pts.clear();
  seedpos.clear();
  seeds.clear();
}
Curve&
Curve::operator=(const Curve& c)
{
  type = c.type;
  tag = c.tag;
  thickness = c.thickness;
  closed = c.closed;
  selected = c.selected;
  pts = c.pts;
  seedpos = c.seedpos;
  seeds = c.seeds;
  return *this;
}

CurveGroup::CurveGroup()
{
  m_lambda = 0.5;
  m_seglen = 2;
  reset();
}

CurveGroup::~CurveGroup()
{
  reset();
}

void
CurveGroup::clearMorphedCurves()
{
  m_mcg.clear();
}

void
CurveGroup::reset()
{
  m_moveCurve = -1;

  QList<Curve*> c = m_cg.values();
  for(int i=0; i<c.count(); i++)
    delete c[i];
  m_cg.clear();
  
  m_mcg.clear();

  m_pointsDirtyBit = false;
  m_xpoints.clear();
  m_ypoints.clear();

  m_addingCurves = false;

  m_selectedPtCoord = QPointF(-1,-1);
}

QList<int>
CurveGroup::polygonLevels()
{
  return m_cg.uniqueKeys();
}

void
CurveGroup::removePolygonAt(int key, int v0, int v1)
{
  int ic = getActiveCurve(key, v0, v1);
  if (ic >= 0)
    {
      QList<Curve*> curves = m_cg.values(key);
      delete curves[ic];
      m_cg.remove(key);
      for(int j=0; j<curves.count(); j++)
	{
	  if (j != ic)
	    m_cg.insert(key, curves[j]);
	}      
    }

  m_pointsDirtyBit = true;

  // remove related morphed curve
  int mc = getActiveMorphedCurve(key, v0, v1);
  if (mc < 0)
    return;
  m_mcg.removeAt(mc);
}

float
CurveGroup::pathLength(int slcType, Curve *c)
{
  Vec vs = Global::voxelScaling();
  float vsx, vsy;
  if (slcType == 0) // DSlice
    {
      vsx = vs.x;
      vsy = vs.y;
    }
  else if (slcType == 1) // WSlice
    {
      vsx = vs.x;
      vsy = vs.z;
    }
  else if (slcType == 2) // HSlice
    {
      vsx = vs.y;
      vsy = vs.z;
    }

  // find total path length  
  int xcount = c->pts.count(); 
  double plen = 0;
  for (int i=1; i<xcount; i++)
    {
      QPointF v = c->pts[i]-c->pts[i-1];
      v = QPointF(v.x()*vsx, v.y()*vsy);
      plen += qSqrt(QPointF::dotProduct(v,v));
    }
  if (c->closed) // for closed curve
    {
      QPointF v = c->pts[0]-c->pts[xcount-1];
      v = QPointF(v.x()*vsx, v.y()*vsy);
      plen += qSqrt(QPointF::dotProduct(v,v));
    }
  return plen;
}

float
CurveGroup::area(int slcType, Curve *c)
{
  if (!c->closed)
    return 0;

  // find total path length  
  int xcount = c->pts.count(); 
  double area = 0;
  for (int i=0; i<xcount; i++)
    {
      int j = (i+1)%xcount;
      area += ((c->pts[j].x()-c->pts[i].x())*
	       (c->pts[j].y()+c->pts[i].y()));
    }

  Vec vs = Global::voxelScaling();
  float vsx, vsy;
  if (slcType == 0) // DSlice
    {
      vsx = vs.x;
      vsy = vs.y;
    }
  else if (slcType == 1) // WSlice
    {
      vsx = vs.x;
      vsy = vs.z;
    }
  else if (slcType == 2) // HSlice
    {
      vsx = vs.y;
      vsy = vs.z;
    }

  area *= vsx*vsy;

  return qAbs(0.5*area);
}

int
CurveGroup::showPolygonInfo(int slcType,
			    int key, int v0, int v1)
{
  int ic = getActiveCurve(key, v0, v1);
  if (ic < 0)
    return -1;

  QList<Curve*> curves = m_cg.values(key);
  QString str;
  str += "Curve Information\n";

  str += QString("Tag : %1\n").arg(curves[ic]->tag);

  if (curves[ic]->closed)
    str += QString("Closed curve\n");
  else
    str += QString("Open curve\n");

  str += QString("Width : %1\n").arg(curves[ic]->thickness);

  str += QString("Length : %1%2\n"). \
    arg(pathLength(slcType, curves[ic]), 0, 'f', 2). \
    arg(Global::voxelUnit());

  if (curves[ic]->closed)
    str += QString("Area : %1 %2 squared\n"). \
      arg(area(slcType, curves[ic]), 0, 'f', 4). \
      arg(Global::voxelUnit());
  
  
  QMessageBox::information(0, "Curve information", str);
  return ic;
}

void
CurveGroup::deselectAll()
{
  QList<Curve*> curves = m_cg.values();
  for(int ic=0; ic<curves.count(); ic++)
    curves[ic]->selected = false;
}

bool
CurveGroup::selectPolygon(int key, int v0, int v1,
			  bool all, int minSlice, int maxSlice)
{
  if (!m_cg.contains(key))
    return false;

  QList<Curve*> curves = m_cg.values(key);

  int sel = false;

  QPointF cen = QPointF(v0, v1);
  for(int ic=0; ic<curves.count(); ic++)
    {
      QVector<QPointF> c = curves[ic]->pts;
      int npts = c.count();
      for(int i=0; i<npts; i++)
	{
	  int ml = (c[i] - cen).manhattanLength();
	  if (ml <= Global::selectionPrecision())
	    {
	      if (all)
		{
		  sel = !curves[ic]->selected;
		  i = npts+1;
		  ic = curves.count()+1;
		  break;
		}

	      curves[ic]->selected = !curves[ic]->selected;

	      // set others to false
	      for(int oc=0; oc<ic; oc++)
		curves[oc]->selected = false;
	      for(int oc=ic+1; oc<curves.count(); oc++)
		curves[oc]->selected = false;

	      return true;
	    }
	}
    }

//  if (all)
//    {
//      QList<Curve*> curves = m_cg.values();
//      for(int ic=0; ic<curves.count(); ic++)
//	curves[ic]->selected = sel;
//
//      return true;
//    }

  if (all)
    {
      for(int k=minSlice; k<=maxSlice; k++)
	{
	  QList<Curve*> curves = m_cg.values(k);
	  for(int ic=0; ic<curves.count(); ic++)
	    curves[ic]->selected = sel;
	}
      return true;
    }

  return false;
}

void
CurveGroup::setClosed(int key, int v0, int v1, bool closed)
{
  int ic = getActiveCurve(key, v0, v1);
  if (ic < 0)
    return;

  QList<Curve*> curves = m_cg.values(key);
  curves[ic]->closed = closed;

  m_pointsDirtyBit = true;
}

void
CurveGroup::setTag(int key, int v0, int v1, int t)
{
  int ic = getActiveCurve(key, v0, v1);
  if (ic < 0)
    return;

  QList<Curve*> curves = m_cg.values(key);
  curves[ic]->tag = t;

  m_pointsDirtyBit = true;
}

void
CurveGroup::setThickness(int key, int v0, int v1, int t)
{
  int ic = getActiveCurve(key, v0, v1);
  if (ic < 0)
    return;

  QList<Curve*> curves = m_cg.values(key);
  curves[ic]->thickness = t;
}

void
CurveGroup::flipPolygon(int key, int v0, int v1)
{
  int ic = getActiveCurve(key, v0, v1);
  if (ic < 0)
    return;

  QList<Curve*> curves = m_cg.values(key);
  QVector<QPointF> c = curves[ic]->pts;
  int npts = curves[ic]->pts.count();
  for(int i=0; i<npts/2; i++)
    {
      QPointF v = curves[ic]->pts[i];
      curves[ic]->pts[i] = curves[ic]->pts[npts-1-i];
      curves[ic]->pts[npts-1-i] = v;
    }

  QMessageBox::information(0, "", "Flipped curve");
}

void
CurveGroup::newPolygon(int key, int v0, int v1,
		       bool smooth, bool line)
{
  //-------------------
  // cull zero curves
  QList<Curve*> curves = m_cg.values(key);
  QList<Curve*> nc;
  for(int j=0; j<curves.count(); j++)
    {
      if (curves[j]->pts.count() > 0)
	nc << curves[j];
      else
	delete curves[j];
    }
  if (nc.count() < curves.count())
    {
      m_cg.remove(key);
      for(int j=0; j<nc.count(); j++)
	m_cg.insert(key, nc[j]);
    }
  //-------------------
  
  Curve *c = new Curve();

  if (smooth && !line) c->type = 2; // smooth polygon = 2
  if (!smooth && !line) c->type = 3; // polygon = 3
  if (smooth && line) c->type = 4; // smooth polyline = 4
  if (!smooth && line) c->type = 5; // smooth polyline = 5

  c->tag = Global::tag();
  c->thickness = Global::thickness();
  if (c->type < 4)
    c->closed = true;
  else
    c->closed = false;
  
  c->seeds << QPointF(v0, v1-100);
  c->seeds << QPointF(v0+100, v1);
  c->seeds << QPointF(v0-100, v1);
  
  generatePolygon(c);    
  
  m_cg.insert(key, c);
  
  m_pointsDirtyBit = true;
}

void
CurveGroup::newEllipse(int key, int v0, int v1)
{
  //-------------------
  // cull zero curves
  QList<Curve*> curves = m_cg.values(key);
  QList<Curve*> nc;
  for(int j=0; j<curves.count(); j++)
    {
      if (curves[j]->pts.count() > 0)
	nc << curves[j];
      else
	delete curves[j];
    }
  if (nc.count() < curves.count())
    {
      m_cg.remove(key);
      for(int j=0; j<nc.count(); j++)
	m_cg.insert(key, nc[j]);
    }
  //-------------------
  
  Curve *c = new Curve();
  c->type = 1; // ellipse=1
  c->tag = Global::tag();
  c->thickness = Global::thickness();
  c->closed = true;
  
  c->seeds << QPointF(v0+100, v1);
  c->seeds << QPointF(v0, v1+100);
  c->seeds << QPointF(v0-100, v1);
  
  generateEllipse(c);
  
  m_cg.insert(key, c);
  
  m_pointsDirtyBit = true;
}

void
CurveGroup::generateEllipse(Curve *c)
{  
  QPointF cen = (c->seeds[0] + c->seeds[2])/2;
  QPointF a = c->seeds[0] - cen;
  QPointF b = c->seeds[1] - cen;

  c->seeds.clear();

  c->pts.clear();
  c->seedpos.clear();
  for(int i=0; i<360; i++)
    {
      if (i%90 == 0 && c->seedpos.count() < 3)
	c->seedpos << c->pts.count();

      float ir = qDegreesToRadians((float)i);
      QPointF v = cen + a*cos(ir) + b*sin(ir);
      c->pts << v;
    }
  
  c->seeds.clear();
  c->seeds << c->pts[0];
  c->seeds << c->pts[90];
  c->seeds << c->pts[180];
}

void
CurveGroup::generatePolygon(Curve *c)
{
  c->pts.clear();
  c->seedpos.clear();

  QVector <QPointF> v;
  v.resize(c->seeds.count());
  for(int i=0; i<c->seeds.count(); i++)
    v[i] = c->seeds[i];


  if (c->type == 3 || c->type == 5) // non-smooth versions
    {
      int ptend = c->seeds.count();
      if (c->type == 5) ptend--;
      for(int ptn=0; ptn<ptend; ptn++)
	{
	  c->seedpos << c->pts.count();
	  int nxt = (ptn+1)%c->seeds.count();
	  QPointF diff = v[nxt]-v[ptn];
	  int len = qSqrt(QPointF::dotProduct(diff, diff))/5;
	  len = qMax(2, len);
	  for(int i=0; i<len; i++)
	    {
	      float frc = i/(float)len;
	      QPointF p = v[ptn] + frc*diff;
	      c->pts << QPointF(p.x(), p.y());
	    }
	}

      if (c->type == 5) // polyline
	c->seedpos << c->pts.count()-1;
      return;
    }


  QVector <QPointF> tv;
  tv.resize(c->seeds.count());
  if (c->type == 4) // smooth polyline
    {
      for(int i=0; i<c->seeds.count(); i++)
	{
	  int nxt = qMin(i+1, c->seeds.count()-1);
	  int prv = qMax(0, i-1);
	  tv[i] = (v[nxt]-v[prv])/2;
	}
    }
  else // smooth polygon
    {
      for(int i=0; i<c->seeds.count(); i++)
	{
	  int nxt = (i+1)%c->seeds.count();
	  int prv = i-1;
	  if (prv<0) prv = c->seeds.count()-1;
	  tv[i] = (v[nxt]-v[prv])/2;
	}
    }

  int ptend = c->seeds.count();
  if (c->type == 4) ptend--;
  for(int ptn=0; ptn<ptend; ptn++)
    {
      c->seedpos << c->pts.count();
      int nxt = (ptn+1)%c->seeds.count();
      QPointF diff = v[nxt]-v[ptn];
      int len = qSqrt(QPointF::dotProduct(diff, diff))/5;
      len = qMax(2, len);
      for(int i=0; i<len; i++)
	{
	  float frc = i/(float)len;
	  QPointF a1 = 3*diff - 2*tv[ptn] - tv[nxt];
	  QPointF a2 = -2*diff + tv[ptn] + tv[nxt];
	  QPointF p = v[ptn] + frc*(tv[ptn] + frc*(a1 + frc*a2));
	  c->pts << QPointF(p.x(), p.y());
	}
    }

  if (c->type == 4) // polyline
    c->seedpos << c->pts.count()-1;
}

void
CurveGroup::newCurve(int key, bool closed)
{
  //-------------------
  // cull zero curves
  QList<Curve*> curves = m_cg.values(key);
  QList<Curve*> nc;
  for(int j=0; j<curves.count(); j++)
    {
      if (curves[j]->pts.count() > 0)
	nc << curves[j];
      else
	delete curves[j];
    }
  if (nc.count() < curves.count())
    {
      m_cg.remove(key);
      for(int j=0; j<nc.count(); j++)
	m_cg.insert(key, nc[j]);
    }
  //-------------------

  Curve *c = new Curve();
  c->type = 0; // normal curve
  c->tag = Global::tag();
  c->thickness = Global::thickness();
  c->closed = closed;
  m_cg.insert(key, c);

  m_pointsDirtyBit = true;
}

void
CurveGroup::addPoint(int key, int v0, int v1)
{
  if (!m_cg.contains(key))
    {
      Curve *c = new Curve();
      c->tag = Global::tag();
      c->thickness = Global::thickness();
      c->closed = Global::closed();
      m_cg.insert(key, c);
    }
    
  Curve *c = m_cg.value(key);
  c->pts << QPointF(v0, v1);
    
  // remove related morphed curves
  int mc = getActiveMorphedCurve(key, c->pts[0].x(), c->pts[0].y());
  if (mc >= 0)
    m_mcg.removeAt(mc);

  m_pointsDirtyBit = true;
}

int
CurveGroup::getActiveCurve(int key, int v0, int v1, bool selected)
{
  if (!m_cg.contains(key))
    return -1;

  QList<Curve*> curves = m_cg.values(key);
  for(int ic=0; ic<curves.count(); ic++)
    {
      if (selected && curves[ic]->selected)
	return ic;

      QPointF cen = QPointF(v0, v1);
      int npts = curves[ic]->pts.count();
      for(int i=npts-1; i>=0; i--)
	{
	  QPointF v = curves[ic]->pts[i] - cen;
	  if (v.manhattanLength() < Global::selectionPrecision())
	    return ic;
	}
    }

  return -1;
}

int
CurveGroup::getActiveMorphedCurve(int key, int v0, int v1)
{
  for(int m=0; m<m_mcg.count(); m++)
    {
      if (m_mcg[m].contains(key))
	{
	  Curve c = m_mcg[m].value(key);
	  QPointF cen = QPointF(v0, v1);
	  int npts = c.pts.count();
	  for(int i=npts-1; i>=0; i--)
	    {
	      QPointF v = c.pts[i] - cen;
	      if (v.manhattanLength() < Global::selectionPrecision())
		return m;
	    }
	}
    }
  
  return -1;
}

void
CurveGroup::removePoint(int key, int v0, int v1)
{
  int ic = getActiveCurve(key, v0, v1);
  if (ic < 0)
    return;

  m_pointsDirtyBit = true;

  QList<Curve*> curves = m_cg.values(key);
  QPointF cen = QPointF(v0, v1);
  int npts = curves[ic]->pts.count();
  for(int i=npts-1; i>=0; i--)
    {
      QPointF v = curves[ic]->pts[i] - cen;
      if (v.manhattanLength() < Global::selectionPrecision())
	{
	  curves[ic]->pts.remove(i, curves[ic]->pts.count()-i);
	  if (curves[ic]->pts.count() == 0)
	    {
	      // remove related morphed curves
	      int mc = getActiveMorphedCurve(key,
					     curves[ic]->pts[0].x(),
					     curves[ic]->pts[0].y());
	      if (mc >= 0)
		m_mcg.removeAt(mc);
  
	      delete curves[ic];
	      curves.removeAt(ic);
	      m_cg.remove(key);
	      for(int j=0; j<curves.count(); j++)
		m_cg.insert(key, curves[j]);
	    }
	  return;
	}
    }
}

QVector<QPointF>
CurveGroup::getPolygonAt(int key)
{
  if (m_cg.contains(key))
    return m_cg.value(key)->pts; // return most recent
      
//  if (m_mcg.count() > 0)
//    {
//      for(int i=0, i<m_mcg.count(); i++)
//	{
//	  if (m_mcg[i].contains(key))
//	    return m_mcg[i].value(key)->pts; // return most recent
//	}
//    }

  QVector<QPointF> p;
  return p;
}

QList<Curve*>
CurveGroup::getCurvesAt(int key)
{
  if (m_cg.contains(key))
    return m_cg.values(key);

  QList<Curve*> c;
  return c;
}

QList<Curve>
CurveGroup::getMorphedCurvesAt(int key)
{      
  QList<Curve> c;
  if (m_mcg.count() > 0)
    {
      for(int i=0; i<m_mcg.count(); i++)
	{
	  if (m_mcg[i].contains(key))
	    c << m_mcg[i].value(key);
	}
    }

  return c;
}

void
CurveGroup::setCurveAt(int key, Curve c)
{
  Curve *cnew = new Curve();
  *cnew = c;

  m_cg.insert(key, cnew);
  m_pointsDirtyBit = true;
}

void
CurveGroup::startAddingCurves()
{
  m_addingCurves = true;
  m_tmcg.clear();
}
void
CurveGroup::endAddingCurves()
{
  m_addingCurves = false;
  if (m_tmcg.count() > 0)
    m_mcg << m_tmcg;
  m_tmcg.clear();
}

void
CurveGroup::setPolygonAt(int key, int *pts, int npts,
			 int tag, int thickness, bool closed, uchar type)
{
  QVector<QPointF> cp;
  for(int i=0; i<npts; i++)
    cp << QPointF(pts[2*i+0], pts[2*i+1]);

  Curve *c = new Curve();
  c->type = type;
  c->tag = tag;
  c->thickness = thickness;
  c->closed = closed;
  c->pts = cp;

  m_cg.insert(key, c);
  m_pointsDirtyBit = true;
}

void
CurveGroup::setPolygonAt(int key,
			 QVector<QPointF> pts,
			 QVector<int> seedpos,
			 bool closed, int tag, int thickness,
			 bool select, uchar type,
			 QVector<QPointF> seeds)
{
//  if (m_addingCurves)
//    {
//      Curve c;
//      c.tag = Global::tag();
//      c.thickness = Global::thickness();
//      c.closed = closed;
//      c.pts = pts;
//      c.seedpos = seedpos;
//      c.selected = true;
//      m_tmcg.insert(key, c);
//    }
//  else
    {
      Curve *c = new Curve();
      c->type = type;
      c->tag = tag;
      c->thickness = thickness;
      c->closed = closed;
      c->pts = pts;
      c->seedpos = seedpos;
      c->seeds = seeds;
      c->selected = select;
      m_cg.insert(key, c);
      m_pointsDirtyBit = true;
    }
}

QList< QMap<int, Curve> >* CurveGroup::getPointerToMorphedCurves() { return &m_mcg; };
void CurveGroup::addMorphBlock(QMap<int, Curve> mb) { m_mcg << mb; }

void
CurveGroup::morphCurves(int minS, int maxS)
{
  QMap<int, Curve> cg;
  QList<int> cgkeys = m_cg.uniqueKeys();
  for(int i=0; i<cgkeys.count(); i++)
    {
      if (cgkeys[i] >= minS &&
	  cgkeys[i] <= maxS)
	{
	  int sel = -1;
	  QList<Curve*> curves = m_cg.values(cgkeys[i]);
	  for(int j=0; j<curves.count(); j++)
	    {
	      if (curves[j]->selected)
		{
		  sel = j;
		  break;
		}
	    }

	  if (sel >= 0)
	    cg.insert(cgkeys[i], *curves[sel]);
	}
    }
  if (cg.count() <= 1)
    {
      QMessageBox::information(0, "", "Atleast two curves required.  Not enough curves selected to interpolate");
      return;
    }

  QList<int> keys = cg.keys();
  for (int ncg=0; ncg<keys.count()-1; ncg++)
    {
      QMap<int, QVector<QPointF> > gmcg;
      gmcg.insert(keys[ncg], cg[keys[ncg]].pts);
      gmcg.insert(keys[ncg+1], cg[keys[ncg+1]].pts);

      bool is_closed = cg[keys[ncg]].closed;
      int thick0 = cg[keys[ncg]].thickness;
      int thick1 = cg[keys[ncg+1]].thickness;
      int mtag = cg[keys[ncg]].tag;

      if (is_closed)
	alignClosedCurves(gmcg);
      else
	alignOpenCurves(gmcg);
      
      MorphCurve mc;
      mc.setPaths(gmcg);
      
      QList<Perimeter> all_perimeters = mc.getMorphedPaths(is_closed);
      int nperi = all_perimeters.count();
      QMap<int, Curve> morphedCurves;
      for (int i=1; i<nperi; i++)
	{
	  QVector<QPointF> a;
	  
	  Perimeter p = all_perimeters[i];
	  for (int j=0; j<p.length; j++)
	    a << QPointF(p.x[j],p.y[j]);

	  Curve c;
	  c.tag = mtag;
	  c.pts = a;
	  c.closed = is_closed;
	  c.thickness = thick0 + (thick1-thick0)*((float)i/(float)(nperi-1));

	  //morphedCurves.insert(qCeil(p.z), c);
	  morphedCurves.insert(qRound(p.z), c);
	}
      
      m_mcg << morphedCurves;
    }
  
  QMessageBox::information(0, "", "morphed intermediate curves");
}

void
CurveGroup::morphSlices(int minS, int maxS)
{
  QMultiMap<int, Curve> cg;
  QList<int> cgkeys = m_cg.uniqueKeys();
  for(int i=0; i<cgkeys.count(); i++)
    {
      if (cgkeys[i] >= minS &&
	  cgkeys[i] <= maxS)
	{
//	  int sel = -1;
	  QList<Curve*> curves = m_cg.values(cgkeys[i]);
	  for(int j=0; j<curves.count(); j++)
	    {
	      if (curves[j]->selected)
		{
		  cg.insert(cgkeys[i], *curves[j]);
//		  sel = j;
//		  break;
		}
	    }

//	  if (sel >= 0)
//	    cg.insert(cgkeys[i], *curves[sel]);
	}
    }
  if (cg.count() <= 1)
    {
      QMessageBox::information(0, "", "Atleast two curves required.  Not enough curves selected to interpolate");
      return;
    }

  QList<int> keys = cg.uniqueKeys();
  for (int ncg=0; ncg<keys.count()-1; ncg++)
    {
      int zmin = keys[ncg];

      QMap<int, QList<QPolygonF> > gmcg;

      bool is_closed;
      int thick0, thick1, mtag;

      {
	QList<QPolygonF> pf;
	QList<Curve> curves = cg.values(keys[ncg]);
	is_closed = curves[0].closed;
	thick0 = curves[0].thickness;
	mtag = curves[0].tag;
	for(int j=0; j<curves.count(); j++)
	  pf << QPolygonF(curves[j].pts);
	gmcg.insert(keys[ncg], pf);
      }
      {
	QList<QPolygonF> pf;
	QList<Curve> curves = cg.values(keys[ncg+1]);
	thick1 = curves[0].thickness;	
	for(int j=0; j<curves.count(); j++)
	  pf << QPolygonF(curves[j].pts);
	gmcg.insert(keys[ncg+1], pf);
      }
      
      //gmcg.insert(keys[ncg], cg[keys[ncg]].pts);
      //gmcg.insert(keys[ncg+1], cg[keys[ncg+1]].pts);

      //bool is_closed = cg[keys[ncg]].closed;
      //int thick0 = cg[keys[ncg]].thickness;
      //int thick1 = cg[keys[ncg+1]].thickness;
      //int mtag = cg[keys[ncg]].tag;
      
      MorphSlice mc;
      QMap< int, QList<QPolygonF> > allcurves = mc.setPaths(gmcg);

      QList<int> keys = allcurves.keys();
      int nperi = keys.count();
      
      for(int npc=0; npc<10; npc++)
	{
	  bool over = true;
	  QMap<int, Curve> morphedCurves;
	  for(int i=0; i<nperi; i++)
	    {
	      int zv = keys[i];
	      QList<QPolygonF> poly = allcurves[zv];
	      
	      if (poly.count() > npc)
		{
		  QVector<QPointF> a;	  
		  for (int j=0; j<poly[npc].count(); j++)
		    {
		      QPointF p = poly[npc][j];
		      a << QPointF(p.x(),p.y());
		    }
		  a = smooth(a, is_closed);
		  
		  Curve c;
		  c.tag = mtag;
		  c.pts = a;
		  c.closed = is_closed;
		  c.thickness = thick0 + (thick1-thick0)*((float)i/(float)(nperi-1));
		  
		  morphedCurves.insert(zmin+zv, c);

		  over = false;
		}
	    }

	  if (!over)
	    m_mcg << morphedCurves;
	  else
	    break;
	}
    }
  
  QMessageBox::information(0, "", "morphed intermediate slices");
}

int
CurveGroup::copyCurve(int key, int v0, int v1)
{
  if (!m_cg.contains(key))
    {
      //QMessageBox::information(0, "", QString("No curve to copy found at %1").arg(key));
      return -1;
    }

  int ic = getActiveCurve(key, v0, v1);
  if (ic >= 0)
    {
      QList<Curve*> curves = m_cg.values(key);
      m_copyCurve = *curves[ic];
    }

  return ic;
}
void
CurveGroup::pasteCurve(int key)
{
  if (m_copyCurve.pts.count() == 0)
    return;

  Curve *c = new Curve();
  *c = m_copyCurve;
  m_cg.insert(key, c);
  m_pointsDirtyBit = true;
}

void
CurveGroup::resetCopyCurve()
{
  m_copyCurve.pts.clear();
  m_copyCurve.seeds.clear();
  m_copyCurve.seedpos.clear();
}

void CurveGroup::resetMoveCurve() { m_moveCurve = -1; }
void
CurveGroup::setMoveCurve(int key, int v0, int v1)
{
  m_moveCurve = -1;
  if (!m_cg.contains(key))
    {
      QMessageBox::information(0, "", QString("No curve to move found at %1").arg(key));
      return;
    }

  int ic = getActiveCurve(key, v0, v1);
  m_moveCurve = ic;
}
void
CurveGroup::moveCurve(int key, int dv0, int dv1)
{
  if (m_moveCurve >= 0)
    {
      QList<Curve*> curves = m_cg.values(key);
      int npts = curves[m_moveCurve]->pts.count();
      for (int i=0; i<npts; i++)
	curves[m_moveCurve]->pts[i] += QPointF(dv0, dv1);
    }  
  m_pointsDirtyBit = true;
}

void
CurveGroup::smooth(int key, int v0, int v1,
		   bool all, int minSlice, int maxSlice)
{
  dilateErode(key, v0, v1, all, minSlice, maxSlice, 0.5);
  dilateErode(key, v0, v1, all, minSlice, maxSlice, -0.5);
}

void
CurveGroup::dilateErode(int key, int v0, int v1,
			bool all, int minSlice, int maxSlice,
			float lambda)
{
  if (all)
    {
      for(int k=minSlice; k<=maxSlice; k++)
	{
	  QList<Curve*> curves = m_cg.values(k);
	  for(int ic=0; ic<curves.count(); ic++)
	    {
	      // fix seedpos
	      if (curves[ic]->seedpos.count() > 0)
		dilateErodeCurveWithSeedPoints(curves[ic], lambda);
	      else
		// replace pts with the smooth version
		curves[ic]->pts = dilateErode(curves[ic]->pts,
					      curves[ic]->closed,
					      lambda);
	    }
	}

      for(int m=0; m<m_mcg.count(); m++)
	{
	  QList<int> keys = m_mcg[m].keys();	  
	  for(int k=0; k<keys.count(); k++)
	    {
	      if (k >= minSlice && k<=maxSlice)
		{
		  Curve c = m_mcg[m].value(keys[k]);
		  QVector<QPointF> w;
		  w = dilateErode(c.pts, c.closed, lambda);
		  c.pts = w; // replace pts with the smooth version
		  m_mcg[m].insert(keys[k], c);
		}
	    }
	}

      m_pointsDirtyBit = true;
      return;
    }


  if (m_cg.contains(key))
    {
      int ic = getActiveCurve(key, v0, v1);
      if (ic >= 0)
	{
	  QList<Curve*> curves = m_cg.values(key);
	  // fix seedpos
	  if (curves[ic]->seedpos.count() > 0)
	    dilateErodeCurveWithSeedPoints(curves[ic], lambda);
	  else
	    curves[ic]->pts = dilateErode(curves[ic]->pts,
					  curves[ic]->closed, lambda);
	  
	  m_pointsDirtyBit = true;
	}
    }
  int mc = getActiveMorphedCurve(key, v0, v1);
  if (mc >= 0)
    {
      Curve c = m_mcg[mc].value(key);
      QVector<QPointF> w;
      w = dilateErode(c.pts, c.closed, lambda);
      c.pts = w; // replace pts with the smooth version
      m_mcg[mc].insert(key, c);
    }
}

void
CurveGroup::smoothCurveWithSeedPoints(Curve *c)
{
  QVector<float> seedfrc;
  int scount = c->seedpos.count(); 

  int onpts = c->pts.count();
  seedfrc << 0;
  for(int j=1; j<scount; j++)
    seedfrc << (float)c->seedpos[j]/(float)onpts;
  
  // replace pts with the smooth version
  c->pts = smooth(c->pts, c->closed);

  int npts = c->pts.count();
  for(int j=1; j<scount; j++)
    c->seedpos[j] = seedfrc[j]*npts;

  if (!c->closed) // for open curve
    c->seedpos[scount-1] = c->pts.count()-1;
}

void
CurveGroup::startPush(int key, int v0, int v1, int rad)
{
  m_selectedPtCoord = QPointF(-1,-1);

  QPointF cen = QPointF(v0, v1);

  if (m_cg.contains(key))
    {
      int ic = getActiveCurve(key, v0, v1);
      if (ic == -1)
	ic = getActiveCurve(key, v0, v1, true);
      if (ic >= 0)
	{
	  QList<Curve*> curves = m_cg.values(key);

	  for(int i=0; i<curves.count(); i++)	    
	    curves[i]->selected = false;
	  curves[ic]->selected = true;

	  // find nearest pt
	  int npts = curves[ic]->pts.count();
	  float minlen = 2*rad;
	  int jc = -1;
	  for(int i=0; i<npts; i++)
	    {
	      QPointF v = curves[ic]->pts[i] - QPointF(v0,v1);
	      float len = qSqrt(QPointF::dotProduct(v,v));
	      if (len <= minlen)
		{
		  minlen = len;
		  jc = i;
		}
	    }
	  if (jc>=0)
	    m_selectedPtCoord = curves[ic]->pts[jc];
	}
    }
}

void
CurveGroup::push(int key, int v0, int v1, int rad)
{
  QPointF cen = QPointF(v0, v1);

  if (m_cg.contains(key))
    {
      int ic = getActiveCurve(key, v0, v1);
      if (ic == -1)
	ic = getActiveCurve(key, v0, v1, true);
      if (ic >= 0)
	{
	  QList<Curve*> curves = m_cg.values(key);

	  for(int i=0; i<curves.count(); i++)	    
	    curves[i]->selected = false;
	  curves[ic]->selected = true;

	  // replace pts with the pushed version
	  curves[ic]->pts = push(curves[ic]->pts,
				 cen,
				 rad,
				 curves[ic]->closed);
	  m_pointsDirtyBit = true;
	}
    }
  int mc = getActiveMorphedCurve(key, v0, v1);
  if (mc >= 0)
    {
      Curve c = m_mcg[mc].value(key);
      QVector<QPointF> w;
      w = push(c.pts,
	       cen,
	       rad,
	       c.closed);
      c.pts = w; // replace pts with the pushed version
      m_mcg[mc].insert(key, c);
    }
}

QVector<QPointF>
CurveGroup::push(QVector<QPointF> c, QPointF cen, int rad, bool closed)
{
  QVector<QPointF> newc;
  newc = c;
  int npts = c.count();

  // find nearest pt
  float minlen = 2*rad;
  int ic = -1;
  for(int i=0; i<npts; i++)
    {
      QPointF v = newc[i] - m_selectedPtCoord;
      float len = qSqrt(QPointF::dotProduct(v,v));
      if (len <= minlen)
	{
	  minlen = len;
	  ic = i;
	}
    }
  
  if (ic == -1) return c;

  QPointF v = cen - newc[ic];
  minlen = qSqrt(QPointF::dotProduct(v,v));
  if (minlen < 1) return c;
  QPointF dv = v/minlen;

  int mini = ic;
  int maxi = ic;
  int jend = -npts;
  if (!closed) jend = 0;
  for(int j=ic-1; j>ic-npts; j--)
    {
      int i = j;
      if (i < 0) i += npts;

      QPointF v = newc[i] - cen;
      float len = qSqrt(QPointF::dotProduct(v,v));
      if (len < 2*rad)
	mini = j;
      else
	break;
    }
  jend = 2*npts;
  if (!closed) jend = npts;
  for(int j=ic+1; j<ic+npts; j++)
    {
      int i = j%npts;      
      QPointF v = newc[i] - cen;
      float len = qSqrt(QPointF::dotProduct(v,v));
      if (len < 2*rad)
	maxi = j;
      else
	break;
    }

  QEasingCurve easing(QEasingCurve::OutCubic);
  for(int j=mini; j<=maxi; j++)
    {
      int i = j;
      if (i < 0) i += npts;
      else i = i%npts;
      
      QPointF v = cen - c[i];
      float len = qSqrt(QPointF::dotProduct(v,v));
      float frc = 1.0 - len/(2*rad);
      frc = easing.valueForProgress(frc);
      newc[i] += minlen*frc*dv;
    }
    
  m_selectedPtCoord = cen;

  QVector<QPointF> w;
  w = subsample(newc, 1.2, closed);
  
  return w;
}


QVector<QPointF>
CurveGroup::smooth(QVector<QPointF> c, bool closed)
{
  QVector<QPointF> newc;
  newc = c;

  // taubin smoothing
  int npts = c.count();
  int ist = 0;
  int ied = npts;
  int p2 = 2;
  if (!closed)
    {
      ist = 1;
      ied = npts-1;
    }    

  int nxt2, prv2;
  for(int i=ist; i<ied; i++)
    {
      if (closed)
	{
	  nxt2 = (i+2)%npts;
	  prv2 = (npts+(i-2))%npts;
	}
      else
	{
	  nxt2 = qMin(npts-1, (i+2)%npts);
	  prv2 = qMax(0, (npts+(i-2))%npts);
	}
      int nxt = (i+1)%npts;
      int prv = (npts+(i-1))%npts;
      
      QPointF vp = (c[nxt]+c[prv]+c[nxt2]+c[prv2])/4;      
      newc[i] += m_lambda*(vp-c[i]);
    }

  for(int i=ist; i<ied; i++)
    {
      if (closed)
	{
	  nxt2 = (i+2)%npts;
	  prv2 = (npts+(i-2))%npts;
	}
      else
	{
	  nxt2 = qMin(npts-1, (i+2)%npts);
	  prv2 = qMax(0, (npts+(i-2))%npts);
	}
      int nxt = (i+1)%npts;
      int prv = (npts+(i-1))%npts;
      
      QPointF vp = (newc[nxt]+newc[prv]+newc[nxt2]+newc[prv2])/4;      
      c[i] = newc[i] - m_lambda*(vp-newc[i]);
    }

  QVector<QPointF> w;
  w = subsample(c, m_seglen, closed);
  return w;
}

QVector<QPointF>
CurveGroup::subsample(QVector<QPointF> cp, float delta, bool closed)
{
  // find total path length  
  int xcount = cp.count(); 
  double plen = 0;
  for (int i=1; i<xcount; i++)
    {
      QPointF v = cp[i]-cp[i-1];
      plen += qSqrt(QPointF::dotProduct(v,v));
    }

  if (closed) // for closed curve
    {
      QPointF v = cp[0]-cp[xcount-1];
      plen += qSqrt(QPointF::dotProduct(v,v));
    }

  int npcount = plen/delta;
  delta = plen/npcount;

  QVector<QPointF> c;
  c << cp[0];

  double clen = 0;
  double pclen = 0;
  int j = c.count();
  int iend = xcount;
  if (closed) iend = xcount+1;
  for (int i=1; i<iend; i++)
    {
      QPointF a, b;
      if (i < xcount)
	{
	  b = cp[i];
	  a = cp[i-1];
	}
      else
	{
	  b = cp[0];
	  a = cp[xcount-1];
	}

      QPointF dv = b-a;
      clen += qSqrt(QPointF::dotProduct(dv, dv));

      while (j*delta <= clen)
	{
	  double frc = (j*delta - pclen)/(clen-pclen);
	  c << (a + frc*dv);

	  j = c.count();
	}
      
      pclen = clen;
    }

  if (!closed)
    {
      if ((c[c.count()-1]-cp[xcount-1]).manhattanLength() < delta)
	c.removeLast();
      c << cp[xcount-1];
    }

  return c;
}

void
CurveGroup::alignClosedCurves(QMap<int, QVector<QPointF> >&cg)
{
  if (cg.count() == 0)
    return;

  QList<int> keys = cg.keys();
  QVector<QPointF> cp = cg.value(keys[0]);
  int ncpts = cp.count();
  
  // get winding for the polygon
  int c0w=0;
  for(int k=0; k<ncpts; k++)
    {
      int k1 = (k+1)%ncpts;
      c0w += (cp[k].x()*cp[k1].y());
      c0w -= (cp[k1].x()*cp[k].y());
    }  
  c0w = (c0w > 0 ? 1 : -1);

  for(int i=1; i<keys.count(); i++)
    {
      QVector<QPointF> c = cg.value(keys[i]);
      int npts = c.count();
      int dst = 10000000;
      int j0 = 0;
      for(int j=0; j<npts; j++)
	{
	  int ml = (cp[0]-c[j]).manhattanLength();
	  if (ml < dst)
	    {
	      dst = ml;
	      j0 = j;
	    }
	}

      QVector<QPointF> nc;
      nc.resize(npts);
      int k = 0;
      for(int j=j0; j<npts; j++)
	nc[k++] = c[j];
      for(int j=0; j<j0; j++)
	nc[k++] = c[j];


      // using shoelace formula to determine the winding of polygons
      // if opposite then flip one polygon
      int c1w=0;
      for(int k=0; k<npts; k++)
	{
	  int k1 = (k+1)%npts;
	  c1w += (nc[k].x()*nc[k1].y());
	  c1w -= (nc[k1].x()*nc[k].y());
	}  
      c1w = (c1w > 0 ? 1 : -1);

      if (c0w*c1w < 0) // opposite signs then flip curve
	{
	  for(int j=0; j<npts/2; j++)
	    {
	      QPointF v = nc[j];
	      nc[j] = nc[npts-1-j];
	      nc[npts-1-j] = v;
	    }
	}

      cg.insert(keys[i], nc);
    }
}

void
CurveGroup::alignOpenCurves(QMap<int, QVector<QPointF> >&cg)
{
  if (cg.count() == 0)
    return;

  QList<int> keys = cg.keys();
  QVector<QPointF> cp = cg.value(keys[0]);
  int ncpts = cp.count();

  // get winding for the polygon
  int c0w=0;
  for(int k=0; k<ncpts; k++)
    {
      int k1 = (k+1)%ncpts;
      c0w += (cp[k].x()*cp[k1].y());
      c0w -= (cp[k1].x()*cp[k].y());
    }  
  c0w = (c0w > 0 ? 1 : -1);

  for(int i=1; i<keys.count(); i++)
    {
      QVector<QPointF> c = cg.value(keys[i]);
      int npts = c.count();

      // using shoelace formula to determine the winding of polygons
      // if opposite then flip one polygon
      int c1w=0;
      for(int k=0; k<npts; k++)
	{
	  int k1 = (k+1)%npts;
	  c1w += (c[k].x()*c[k1].y());
	  c1w -= (c[k1].x()*c[k].y());
	}  
      c1w = (c1w > 0 ? 1 : -1);

      if (c0w*c1w < 0) // opposite signs then flip curve
	{
	  for(int j=0; j<npts/2; j++)
	    {
	      QPointF v = c[j];
	      c[j] = c[npts-1-j];
	      c[npts-1-j] = v;
	    }
	}

      cg.insert(keys[i], c);
    }
}

void
CurveGroup::joinPolygonAt(int key, QVector<QPointF> pts)
{
  int npts = pts.count();

  int mc = getActiveMorphedCurve(key, pts[0].x(), pts[0].y());
  if (mc >= 0)
    {
      Curve c = m_mcg[mc].value(key);
      QVector<QPointF> w = c.pts;
      int ncpts = w.count();
      int start = -1;
      QPointF startPt = pts[0];
      QPointF endPt = pts[npts-1];
      for(int j=0; j<ncpts; j++)
	{
	  int ml = (startPt-w[j]).manhattanLength();
	  if (ml < 3)
	    {
	      start = j;
	      break;
	    }
	}
      int end = -1;
      for(int j=0; j<ncpts; j++)
	{
	  int ml = (endPt-w[j]).manhattanLength();
	  if (ml < Global::selectionPrecision())
	    {
	      end = j;
	      break;
	    }
	}

      if (start <0 || end < 0)
	return;
      
      // insert pts into the curve
      QVector<QPointF> newc;
      newc = pts;
      int jend = ncpts;
      if (start > end)
	jend = start;
      for(int j=end; j<jend; j++)
	newc << w[j];
      if (start < end)
	{
	  for(int j=0; j<start; j++)
	    newc << w[j];
	}      

      w = subsample(newc, 1.2, c.closed);
      c.pts = w; // replace pts with the pushed version
      m_mcg[mc].insert(key, c);
    }
}

QList<QPointF>
CurveGroup::xpoints(int key)
{
  if (m_pointsDirtyBit)
    generateXYpoints();
  return m_xpoints.values(key);
}
QList<QPointF>
CurveGroup::ypoints(int key)
{
  if (m_pointsDirtyBit)
    generateXYpoints();
  return m_ypoints.values(key);
}

void
CurveGroup::generateXYpoints()
{
  m_xpoints.clear();
  m_ypoints.clear();

  QList<int> keys = m_cg.keys();
  for(int k=0; k<keys.count(); k++)
    {
      int z = keys[k];
      QList<Curve*> c = m_cg.values(z);
      for(int ic=0; ic<c.count(); ic++)
	{
	  int xcount = c[ic]->pts.count(); 
	  for (int i=0; i<xcount; i++)
	    {
	      int x = c[ic]->pts[i].x();
	      int y = c[ic]->pts[i].y();
	      // Z, X, Y
	      // d, h, w
	      // w, h, d
	      // h, w, d
	      m_xpoints.insert(x, QPointF(z, y));
	      m_ypoints.insert(y, QPointF(z, x));
	    }
	}
    }

  m_pointsDirtyBit = false;
}

QVector<QPointF>
CurveGroup::dilateErode(QVector<QPointF> c, bool closed, float lambda)
{
  QVector<QPointF> newc;
  newc = c;

  // taubin smoothing
  int npts = c.count();
  int ist = 0;
  int ied = npts;
  int p2 = 2;
  if (!closed)
    {
      ist = 1;
      ied = npts-1;
    }    

  for(int i=ist; i<ied; i++)
    {
      int nxt = (i+1)%npts;
      int prv = (npts+(i-1))%npts;
      QPointF vp = (c[nxt]-c[prv]);
      double vpl = qSqrt(vp.x()*vp.x() + vp.y()*vp.y());
      if (vpl > 0.1)
	{
	  vp /= vpl;
	  newc[i] = c[i] + lambda*QPointF(-vp.y(), vp.x());
	}
    }

  QVector<QPointF> w;
  w = subsample(newc, m_seglen, closed);
  return w;
}

void
CurveGroup::dilateErodeCurveWithSeedPoints(Curve *c, float lambda)
{
  QVector<float> seedfrc;
  int scount = c->seedpos.count();   

  int onpts = c->pts.count();
  seedfrc << 0;
  for(int j=1; j<scount; j++)
    seedfrc << (float)c->seedpos[j]/(float)onpts;
    
  // replace pts with the dilated/eroded version
  c->pts = dilateErode(c->pts, c->closed, lambda);

  int npts = c->pts.count();
  for(int j=1; j<scount; j++)
    c->seedpos[j] = qRound(seedfrc[j]*npts);
  
  if (!c->closed) // for open curve
    c->seedpos[scount-1] = c->pts.count()-1;
}

