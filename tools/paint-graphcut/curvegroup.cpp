#include "curvegroup.h"
#include "morphcurve.h"

Curve::Curve()
{
  tag = 0;
  pts.clear();
}
Curve::~Curve()
{
  tag = 0;
  pts.clear();
}
Curve&
Curve::operator=(const Curve& c)
{
  tag = c.tag;
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
CurveGroup::reset()
{
  m_cg.clear();
  m_mcg.clear();
}

QList<int>
CurveGroup::polygonLevels()
{
  return m_cg.keys();
}

void
CurveGroup::resetPolygonAt(int key)
{
  m_cg.remove(key);
  m_mcg.clear();
}

void
CurveGroup::flipPolygonAt(int key)
{
//  if (!m_cg.contains(key))
//    return;
//  
//  QVector<QPoint> c = m_cg.value(key);
//  int npts = c.count();
//  for(int i=0; i<npts/2; i++)
//    {
//      QPoint v = c[i];
//      c[i] = c[npts-1-i];
//      c[npts-1-i] = v;
//    }
//
//  m_cg.insert(key, c);
//
//  if (m_mcg.count() > 0)
//    morphCurves();
//  else
//    QMessageBox::information(0, "", "Flipped curve");
}

void
CurveGroup::newCurve(int key, int tag)
{
  Curve c;
  c.tag = tag;
  m_cg.insert(key, c);
}

void
CurveGroup::addPoint(int key, int v0, int v1)
{
  Curve c = m_cg.value(key);
  c.pts << QPoint(v0, v1);
  m_cg.replace(key, c);
  
  //QVector<QPoint> c = m_cg.value(key);
  //c << QPoint(v0, v1);
  //m_cg.insert(key, c);

  m_mcg.clear();
}

QVector<QPoint>
CurveGroup::getPolygonAt(int key)
{
  if (m_cg.contains(key))
    return m_cg.value(key).pts; // return most recent
      
  if (m_mcg.contains(key))
    return m_mcg.value(key).pts; // return most recent

  QVector<QPoint> p;
  return p;
}

QList<Curve>
CurveGroup::getCurvesAt(int key)
{
  if (m_cg.contains(key))
    return m_cg.values(key);
      
  if (m_mcg.contains(key))
    return m_mcg.values(key);

  QList<Curve> c;
  return c;
}

void
CurveGroup::setPolygonAt(int key, int *pts, int npts, int tag)
{
  QVector<QPoint> cp;
  for(int i=0; i<npts; i++)
    cp << QPoint(pts[2*i+0], pts[2*i+1]);

  Curve c;
  c.tag = tag;
  c.pts = cp;

  m_cg.insert(key, c);
}

void
CurveGroup::setPolygonAt(int key, QVector<QPoint> pts, int tag)
{
  Curve c;
  c.tag = tag;
  c.pts = pts;

  m_cg.insert(key, c);
}

void
CurveGroup::morphCurves()
{
  alignAllCurves();

  QMap<int, QVector<QPoint> > cg;
  QList<int> cgkeys = m_cg.keys();
  for(int i=0; i<cgkeys.count(); i++)
    cg.insert(cgkeys[i], m_cg.value(cgkeys[i]).pts);

  MorphCurve mc;
  //mc.setPaths(m_cg);
  mc.setPaths(cg);

  QList<Perimeter> all_perimeters = mc.getMorphedPaths();
  m_mcg.clear();

  for (int i=0; i<all_perimeters.count(); i++)
    {
      QVector<QPoint> a;

      Perimeter p = all_perimeters[i];
      for (int j=0; j<p.length; j++)
	a << QPoint(p.x[j],p.y[j]);

      Curve c;
      c.tag = 1;
      c.pts = a;

      m_mcg.insert(p.z, c);
    }

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

  QList<Curve> curve = m_cg.values(key);
  m_cg.remove(key);
  for(int ic=0; ic<curve.count(); ic++)
    {
      QVector<QPoint> w = smooth(curve[ic].pts, v0, v1, rad);
      curve[ic].pts = w; // replace pts with the smooth version
      m_cg.insert(key, curve[ic]);
    }

//  QVector<QPoint> w = smooth(m_cg.value(key), v0, v1, rad);
//  // replace the existing polyline
//  m_cg.insert(key, w);
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

  QList<Curve> curve = m_cg.values(key);
  m_cg.remove(key);
  for(int ic=0; ic<curve.count(); ic++)
    {
      //QVector<QPoint> c = m_cg.value(key);
      QVector<QPoint> c = curve[ic].pts;
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
      w = subsample(newc, 1.2);

      curve[ic].pts = w;
      m_cg.insert(key, curve[ic]);

      // replace the existing polyline
      //m_cg.insert(key, w);
    }

}

QVector<QPoint>
CurveGroup::smooth(QVector<QPoint> c, int v0, int v1, int rad)
{
  QPoint cen = QPoint(v0, v1);
  QVector<QPoint> newc;
  newc = c;
  int npts = c.count();
  int sz = 5;
  for(int i=0; i<npts; i++)
    {
      QPoint v = c[i] - cen;
      if (v.manhattanLength() <= rad)
	{
	  v = QPoint(0,0);
	  for(int j=-sz; j<=sz; j++)
	    {
	      int idx = i+j;
	      if (idx < 0) idx = npts + idx;
	      else if (idx > npts-1) idx = idx - npts;
	      v += c[idx];
	    }
	  v /= (2*sz+1);
	  newc[i] = v;
	}
    }
  QVector<QPoint> w;
  w = subsample(newc, 1.2);

  return w;
}

QVector<QPoint>
CurveGroup::subsample(QVector<QPoint> cp, float delta)
{
  // find total path length  
  int xcount = cp.count(); 
  double plen = 0;
  for (int i=1; i<xcount; i++)
    {
      QPoint v = cp[i]-cp[i-1];
      plen += qSqrt(QPoint::dotProduct(v,v));
    }
  // because curve is closed
  QPoint v = cp[0]-cp[xcount-1];
  plen += qSqrt(QPoint::dotProduct(v,v));
  
  int npcount = plen/delta;
  delta = plen/npcount;

  QVector<QPoint> c;
  c << cp[0];

  double clen = 0;
  double pclen = 0;
  int j = c.count();
  for (int i=1; i<xcount+1; i++)
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
  return c;
}

void
CurveGroup::alignAllCurves()
{
//  if (m_cg.count() == 0)
//    return;
//
//  QList<int> keys = m_cg.keys();
//  QPoint cp;
//  cp = m_cg[keys[0]][0];
//
//  for(int i=1; i<keys.count(); i++)
//    {
//      QVector<QPoint> c = m_cg.value(keys[i]);
//      int npts = c.count();
//      int dst = 10000000;
//      int j0 = 0;
//      for(int j=0; j<npts; j++)
//	{
//	  int ml = (cp-c[j]).manhattanLength();
//	  if (ml < dst)
//	    {
//	      dst = ml;
//	      j0 = j;
//	    }
//	}
//
//      QVector<QPoint> nc;
//      nc.resize(npts);
//      int k = 0;
//      for(int j=j0; j<npts; j++)
//	nc[k++] = c[j];
//      for(int j=0; j<j0; j++)
//	nc[k++] = c[j];
//
//      m_cg.insert(keys[i], nc);
//    }
}
