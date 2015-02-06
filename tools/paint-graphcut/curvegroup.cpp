#include "curvegroup.h"
#include "morphcurve.h"

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
  if (!m_cg.contains(key))
    return;

  QVector<QPoint> c = m_cg.value(key);
  int npts = c.count();
  for(int i=0; i<npts/2; i++)
    {
      QPoint v = c[i];
      c[i] = c[npts-1-i];
      c[npts-1-i] = v;
    }

  m_cg.insert(key, c);

  if (m_mcg.count() > 0)
    morphCurves();
  else
    QMessageBox::information(0, "", "Flipped curve");
}

void
CurveGroup::addPoint(int key, int v0, int v1)
{
  QVector<QPoint> c = m_cg.value(key);
  c << QPoint(v0, v1);
  m_cg.insert(key, c);

  m_mcg.clear();
}

QVector<QPoint>
CurveGroup::getPolygonAt(int key)
{
  if (m_cg.contains(key))
    return m_cg[key];
      
  if (m_mcg.contains(key))
    return m_mcg[key];

  QVector<QPoint> p;
  return p;
}

void
CurveGroup::morphCurves()
{
  MorphCurve mc;
  mc.setPaths(m_cg);

  QList<Perimeter> all_perimeters = mc.getMorphedPaths();
  m_mcg.clear();

  for (int i=0; i<all_perimeters.count(); i++)
    {
      QVector<QPoint> a;

      Perimeter p = all_perimeters[i];
      for (int j=0; j<p.length; j++)
	a << QPoint(p.x[j],p.y[j]);

      m_mcg.insert(p.z, a);
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

  QVector<QPoint> w = smooth(m_cg.value(key), key, v0, v1, rad);
  // replace the existing polyline
  m_cg.insert(key, w);
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

  QVector<QPoint> c = m_cg.value(key);
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

////  QVector<QPoint> w;
////  w << newc[0];
////  // remove close points
////  for(int i=1; i<npts; i++)
////    {
////      QPoint v = newc[i] - newc[i-1];
////      if (v.manhattanLength() >= 1)
////	w << newc[i];
////    }

  QVector<QPoint> w;
  w = subsample(newc, 1.2);

  // replace the existing polyline
  m_cg.insert(key, w);
}

QVector<QPoint>
CurveGroup::smooth(QVector<QPoint> c, int key, int v0, int v1, int rad)
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
