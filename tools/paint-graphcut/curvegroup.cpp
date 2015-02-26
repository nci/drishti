#include "curvegroup.h"
#include "morphcurve.h"
#include "global.h"

Curve::Curve()
{
  tag = 0;
  thickness = 1;
  closed = true;
  selected = false;
  pts.clear();
}
Curve::~Curve()
{
  tag = 0;
  thickness = 1;
  closed = true;
  selected = false;
  pts.clear();
}
Curve&
Curve::operator=(const Curve& c)
{
  tag = c.tag;
  thickness = c.thickness;
  closed = c.closed;
  selected = c.selected;
  pts = c.pts;
  return *this;
}

CurveGroup::CurveGroup()
{
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
  QList<Curve*> c = m_cg.values();
  for(int i=0; i<c.count(); i++)
    delete c[i];
  m_cg.clear();
  
  m_mcg.clear();
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
      curves.removeAt(ic);
      m_cg.remove(key);
      for(int j=0; j<curves.count(); j++)
	m_cg.insert(key, curves[j]);
    }

  // remove related morphed curve
  int mc = getActiveMorphedCurve(key, v0, v1);
  if (mc < 0)
    return;
  m_mcg.removeAt(mc);
}

bool
CurveGroup::selectPolygon(int key, int v0, int v1)
{
  if (!m_cg.contains(key))
    return false;
  
  QList<Curve*> curves = m_cg.values(key);

  QPoint cen = QPoint(v0, v1);
  for(int ic=0; ic<curves.count(); ic++)
    {
      QVector<QPoint> c = curves[ic]->pts;
      int npts = c.count();
      for(int i=0; i<npts; i++)
	{
	  int ml = (c[i] - cen).manhattanLength();
	  if (ml <= 5)
	    {
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
  QVector<QPoint> c = curves[ic]->pts;
  int npts = curves[ic]->pts.count();
  for(int i=0; i<npts/2; i++)
    {
      QPoint v = curves[ic]->pts[i];
      curves[ic]->pts[i] = curves[ic]->pts[npts-1-i];
      curves[ic]->pts[npts-1-i] = v;
    }
//  if (m_mcg.count() > 0)
//    morphCurves();
//  else
    QMessageBox::information(0, "", "Flipped curve");
}

void
CurveGroup::newCurve(int key)
{
  Curve *c = new Curve();
  c->tag = Global::tag();
  c->thickness = Global::smooth();
  m_cg.insert(key, c);
}

void
CurveGroup::addPoint(int key, int v0, int v1)
{
  if (!m_cg.contains(key))
    {
      Curve *c = new Curve();
      c->tag = Global::tag();
      m_cg.insert(key, c);
    }
    
  Curve *c = m_cg.value(key);
  c->pts << QPoint(v0, v1);
    
  // remove related morphed curves
  int mc = getActiveMorphedCurve(key, c->pts[0].x(), c->pts[0].y());
  if (mc >= 0)
    m_mcg.removeAt(mc);
}

int
CurveGroup::getActiveCurve(int key, int v0, int v1)
{
  if (!m_cg.contains(key))
    return -1;

  QList<Curve*> curves = m_cg.values(key);
  for(int ic=0; ic<curves.count(); ic++)
    {
      QPoint cen = QPoint(v0, v1);
      int npts = curves[ic]->pts.count();
      for(int i=npts-1; i>=0; i--)
	{
	  QPoint v = curves[ic]->pts[i] - cen;
	  if (v.manhattanLength() < 5)
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
	  QPoint cen = QPoint(v0, v1);
	  int npts = c.pts.count();
	  for(int i=npts-1; i>=0; i--)
	    {
	      QPoint v = c.pts[i] - cen;
	      if (v.manhattanLength() < 5)
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

  QList<Curve*> curves = m_cg.values(key);
  QPoint cen = QPoint(v0, v1);
  int npts = curves[ic]->pts.count();
  for(int i=npts-1; i>=0; i--)
    {
      QPoint v = curves[ic]->pts[i] - cen;
      if (v.manhattanLength() < 3)
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

QVector<QPoint>
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

  QVector<QPoint> p;
  return p;
}

QList<Curve*>
CurveGroup::getCurvesAt(int key)
{
  if (m_cg.contains(key))
    return m_cg.values(key);

  QList<Curve*> c;
  return c;
      
//  if (m_mcg.contains(key))
//    return m_mcg.values(key);
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
CurveGroup::setPolygonAt(int key, int *pts, int npts,
			 int tag, int thickness, bool closed)
{
  QVector<QPoint> cp;
  for(int i=0; i<npts; i++)
    cp << QPoint(pts[2*i+0], pts[2*i+1]);

  Curve *c = new Curve();
  c->tag = tag;
  c->thickness = thickness;
  c->closed = closed;
  c->pts = cp;

  m_cg.insert(key, c);
}

void
CurveGroup::setPolygonAt(int key, QVector<QPoint> pts)
{
  Curve *c = new Curve();
  c->tag = Global::tag();
  c->thickness = Global::smooth();
  c->pts = pts;

  m_cg.insert(key, c);
}

void
CurveGroup::morphCurves()
{
  QMap<int, QVector<QPoint> > cg;
  QList<int> cgkeys = m_cg.uniqueKeys();
  int first = -1;
  int mtag = 1;
  //QList<int> thick;
  for(int i=0; i<cgkeys.count(); i++)
    {
      int sel = -1;
      QList<Curve*> curves = m_cg.values(cgkeys[i]);
      for(int j=0; j<curves.count(); j++)
	{
	  if (curves[j]->selected)
	    {
	      mtag = curves[j]->tag;
	      sel = j;
	      break;
	    }
	}

      if (sel >= 0)
	{
	  cg.insert(cgkeys[i], curves[sel]->pts);
	  //thick << curves[cgkeys[i]]->thickness;
	  if (first == -1)
	    first = i;
	}
      else if (first > 0 && i > first)
	break;
    }
  if (cg.count() <= 1)
    {
      QMessageBox::information(0, "", "Not enough curves selected to morph");
      return;
    }

  alignAllCurves(cg);

  MorphCurve mc;
  mc.setPaths(cg);

  QList<Perimeter> all_perimeters = mc.getMorphedPaths();

  QMap<int, Curve> morphedCurves;
  for (int i=0; i<all_perimeters.count(); i++)
    {
      QVector<QPoint> a;

      Perimeter p = all_perimeters[i];
      for (int j=0; j<p.length; j++)
	a << QPoint(p.x[j],p.y[j]);

      Curve c;
      c.tag = mtag;
      c.pts = a;

      morphedCurves.insert(p.z, c);
    }

  m_mcg << morphedCurves;

  QMessageBox::information(0, "", "morphed intermediate curves");
}

void
CurveGroup::smooth(int key, int v0, int v1, int rad)
{
  if (!m_cg.contains(key))
    {
      QMessageBox::information(0, "", QString("No curve to smooth found at %1").arg(key));
      return;
    }

  QList<Curve*> curve = m_cg.values(key);
  for(int ic=0; ic<curve.count(); ic++)
    {
      QVector<QPoint> w = smooth(curve[ic]->pts, v0, v1, rad, curve[ic]->closed);
      curve[ic]->pts = w; // replace pts with the smooth version
    }
}

void
CurveGroup::push(int key, int v0, int v1, int rad)
{
  if (!m_cg.contains(key))
    {
      QMessageBox::information(0, "", QString("No curve to push found at %1").arg(key));
      return;
    }

  QPoint cen = QPoint(v0, v1);

  QList<Curve*> curve = m_cg.values(key);
  for(int ic=0; ic<curve.count(); ic++)
    {
      QVector<QPoint> c = curve[ic]->pts;
      QVector<QPoint> newc;
      newc = c;
      int npts = c.count();
      for(int i=0; i<npts; i++)
	{
	  QPoint v = newc[i] - cen;
	  float len = qSqrt(QPoint::dotProduct(v,v));
	  if (len <= rad)
	    {
	      v /= qMax(0.0f, len);	      
	      for(int j=-rad; j<=rad; j++)
		{
		  int idx = i+j;
		  if (idx < 0) idx = npts + idx;
		  else if (idx > npts-1) idx = idx - npts;
		  
		  QPoint v0 = newc[idx] - cen;
		  int v0len = qSqrt(QPoint::dotProduct(v0,v0));
		  if (v0len <= rad)
		    {
		      float frc = (float)qAbs(qAbs(j)-rad)/(float)rad;
		      newc[idx] = newc[idx] + frc*(rad-len)*v;
		    }
		}
	    }
	}

      QVector<QPoint> w;
      w = subsample(newc, 1.2, curve[ic]->closed);

      curve[ic]->pts = w;
    }

}

QVector<QPoint>
CurveGroup::smooth(QVector<QPoint> c, int v0, int v1, int rad, bool closed)
{
  QPoint cen = QPoint(v0, v1);
  QVector<QPoint> newc;
  newc = c;
  int npts = c.count();
  int sz = 5;
  for(int i=1; i<npts-1; i++)
    {
      QPoint v = c[i] - cen;
      if (v.manhattanLength() <= rad)
	{
	  v = QPoint(0,0);
	  for(int j=-sz; j<=sz; j++)
	    {
	      int idx = i+j;
	      if (closed)
		{
		  if (idx < 0) idx = npts + idx;
		  else if (idx > npts-1) idx = idx - npts;
		}
	      else
		idx = qBound(0, idx, npts-1);
	      v += c[idx];
	    }
	  v /= (2*sz+1);
	  newc[i] = v;
	}
    }
  QVector<QPoint> w;
  w = subsample(newc, 1.2, closed);

  return w;
}

QVector<QPoint>
CurveGroup::subsample(QVector<QPoint> cp, float delta, bool closed)
{
  // find total path length  
  int xcount = cp.count(); 
  double plen = 0;
  for (int i=1; i<xcount; i++)
    {
      QPoint v = cp[i]-cp[i-1];
      plen += qSqrt(QPoint::dotProduct(v,v));
    }

  if (closed) // for closed curve
    {
      QPoint v = cp[0]-cp[xcount-1];
      plen += qSqrt(QPoint::dotProduct(v,v));
    }

  int npcount = plen/delta;
  delta = plen/npcount;

  QVector<QPoint> c;
  c << cp[0];

  double clen = 0;
  double pclen = 0;
  int j = c.count();
  int iend = xcount;
  if (closed) iend = xcount+1;
  for (int i=1; i<iend; i++)
    {
      QPoint a, b;
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

      QPoint dv = b-a;
      clen += qSqrt(QPoint::dotProduct(dv, dv));

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
CurveGroup::alignAllCurves(QMap<int, QVector<QPoint> >&cg)
{
  if (cg.count() == 0)
    return;

  QList<int> keys = cg.keys();
  QPoint cp;
  cp = cg[keys[0]][0];

  for(int i=1; i<keys.count(); i++)
    {
      QVector<QPoint> c = cg.value(keys[i]);
      int npts = c.count();
      int dst = 10000000;
      int j0 = 0;
      for(int j=0; j<npts; j++)
	{
	  int ml = (cp-c[j]).manhattanLength();
	  if (ml < dst)
	    {
	      dst = ml;
	      j0 = j;
	    }
	}

      QVector<QPoint> nc;
      nc.resize(npts);
      int k = 0;
      for(int j=j0; j<npts; j++)
	nc[k++] = c[j];
      for(int j=0; j<j0; j++)
	nc[k++] = c[j];

      cg.insert(keys[i], nc);
    }
}
