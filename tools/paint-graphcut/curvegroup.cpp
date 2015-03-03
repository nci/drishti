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
  m_moveCurve = -1;

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

float
CurveGroup::pathLength(Curve *c)
{
  // find total path length  
  int xcount = c->pts.count(); 
  double plen = 0;
  for (int i=1; i<xcount; i++)
    {
      QPoint v = c->pts[i]-c->pts[i-1];
      plen += qSqrt(QPoint::dotProduct(v,v));
    }
  if (c->closed) // for closed curve
    {
      QPoint v = c->pts[0]-c->pts[xcount-1];
      plen += qSqrt(QPoint::dotProduct(v,v));
    }
  return plen;
}

float
CurveGroup::area(Curve *c)
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
  return qAbs(0.5*area);
}

void
CurveGroup::showPolygonInfo(int key, int v0, int v1)
{
  int ic = getActiveCurve(key, v0, v1);
  if (ic < 0)
    return;

  QList<Curve*> curves = m_cg.values(key);
  QString str;
  str += "Curve Information\n";
  str += QString("Tag : %1\n").arg(curves[ic]->tag);
  str += QString("Closed : %1\n").arg(curves[ic]->closed);
  str += QString("Width : %1\n").arg(curves[ic]->thickness);
  str += QString("Length : %1\n").arg(pathLength(curves[ic]));
  if (curves[ic]->closed)
    str += QString("Area : %1\n").arg(area(curves[ic]));

  
  QMessageBox::information(0, "Curve information", str);
}

bool
CurveGroup::selectPolygon(int key, int v0, int v1, bool all)
{
  if (!m_cg.contains(key))
    return false;

  QList<Curve*> curves = m_cg.values(key);

  int sel = false;

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

  if (all)
    {
      QList<Curve*> curves = m_cg.values();
      for(int ic=0; ic<curves.count(); ic++)
	curves[ic]->selected = sel;

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
}

void
CurveGroup::setTag(int key, int v0, int v1, int t)
{
  int ic = getActiveCurve(key, v0, v1);
  if (ic < 0)
    return;

  QList<Curve*> curves = m_cg.values(key);
  curves[ic]->tag = t;
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
CurveGroup::newCurve(int key, bool closed)
{
  Curve *c = new Curve();
  c->tag = Global::tag();
  c->thickness = Global::thickness();
  c->closed = closed;
  m_cg.insert(key, c);
}

void
CurveGroup::addPoint(int key, int v0, int v1)
{
  if (!m_cg.contains(key))
    {
      Curve *c = new Curve();
      c->tag = Global::tag();
      c->closed = Global::closed();
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
CurveGroup::getActiveCurve(int key, int v0, int v1, int mdst)
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
	  if (v.manhattanLength() < mdst)
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
CurveGroup::setPolygonAt(int key, QVector<QPoint> pts, bool closed)
{
  Curve *c = new Curve();
  c->tag = Global::tag();
  c->thickness = Global::thickness();
  c->closed = closed;
  c->pts = pts;

  m_cg.insert(key, c);
}

void
CurveGroup::morphCurves()
{
  //QMap<int, QVector<QPoint> > cg;
  QMap<int, Curve> cg;
  QList<int> cgkeys = m_cg.uniqueKeys();
  int first = -1;
  int mtag = 1;
  //bool is_closed_curve = true;
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
	  //cg.insert(cgkeys[i], curves[sel]->pts);
	  cg.insert(cgkeys[i], *curves[sel]);
	  if (first == -1)
	    {
	      first = i;
	      //is_closed_curve = curves[sel]->closed;
	    }
	}
      else if (first > 0 && i > first)
	break;
    }
  if (cg.count() <= 1)
    {
      QMessageBox::information(0, "", "Not enough curves selected to morph");
      return;
    }

  QList<int> keys = cg.keys();
  for (int ncg=0; ncg<keys.count()-1; ncg++)
    {
      QMap<int, QVector<QPoint> > gmcg;
      gmcg.insert(keys[ncg], cg[keys[ncg]].pts);
      gmcg.insert(keys[ncg+1], cg[keys[ncg+1]].pts);

      bool is_closed = cg[keys[ncg]].closed;
      int thick0 = cg[keys[ncg]].thickness;
      int thick1 = cg[keys[ncg+1]].thickness;

      if (is_closed)
	alignAllCurves(gmcg);
      
      MorphCurve mc;
      mc.setPaths(gmcg);
      
      QList<Perimeter> all_perimeters = mc.getMorphedPaths(is_closed);
      int nperi = all_perimeters.count();
      QMap<int, Curve> morphedCurves;
      for (int i=0; i<nperi; i++)
	{
	  QVector<QPoint> a;
	  
	  Perimeter p = all_perimeters[i];
	  for (int j=0; j<p.length; j++)
	    a << QPoint(p.x[j],p.y[j]);
	  
	  Curve c;
	  c.tag = mtag;
	  c.pts = a;
	  c.closed = is_closed;
	  c.thickness = thick0 + (thick1-thick0)*((float)i/(float)(nperi-1));

	  morphedCurves.insert(p.z, c);
	}
      
      m_mcg << morphedCurves;
    }
  
  QMessageBox::information(0, "", "morphed intermediate curves");
}

void
CurveGroup::copyCurve(int key, int v0, int v1)
{
  if (!m_cg.contains(key))
    {
      QMessageBox::information(0, "", QString("No curve to copy found at %1").arg(key));
      return;
    }

  int ic = getActiveCurve(key, v0, v1);
  if (ic >= 0)
    {
      QList<Curve*> curves = m_cg.values(key);
      m_copyCurve = *curves[ic];
    }
}
void
CurveGroup::pasteCurve(int key)
{
  Curve *c = new Curve();
  *c = m_copyCurve;
  m_cg.insert(key, c);
  QMessageBox::information(0, "", QString("%1 : %2").arg(key).arg(m_cg.values(key).count()));
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
	curves[m_moveCurve]->pts[i] += QPoint(dv0, dv1);
    }  
}

void
CurveGroup::smooth(int key, int v0, int v1, int rad)
{
  if (!m_cg.contains(key))
    {
      QMessageBox::information(0, "", QString("No curve to smooth found at %1").arg(key));
      return;
    }

  int ic = getActiveCurve(key, v0, v1);
  if (ic >= 0)
    {
      QList<Curve*> curves = m_cg.values(key);
      QVector<QPoint> w;
      //w = subsample(curves[ic]->pts, 1.2, curves[ic]->closed);
      w = smooth(curves[ic]->pts, v0, v1, rad, curves[ic]->closed);
      curves[ic]->pts = w; // replace pts with the smooth version
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

  QMultiMap<int, int> inRad;
  for(int i=1; i<npts-1; i++)
    {
      QPoint v = c[i] - cen;
      if (v.manhattanLength() <= rad)
	inRad.insert(v.manhattanLength(), i);
    }

  QList<int> keys = inRad.keys();
  for(int k=0; k<keys.count(); k++)
    {
      QList<int> ipts = inRad.values(keys[k]);
      for(int p=0; p<ipts.count(); p++)
	{
	  int i = ipts[p];
	  QPoint v = c[i];
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
	      v += newc[idx];
	    }
	  v /= (2*sz+2);
	  newc[i] = v;
	}
    }

//  for(int i=1; i<npts-1; i++)
//    {
//      QPoint v = c[i] - cen;
//      if (v.manhattanLength() <= rad)
//	{
//	  //v = QPoint(0,0);
//	  v = c[i];
//	  for(int j=-sz; j<=sz; j++)
//	    {
//	      int idx = i+j;
//	      if (closed)
//		{
//		  if (idx < 0) idx = npts + idx;
//		  else if (idx > npts-1) idx = idx - npts;
//		}
//	      else
//		idx = qBound(0, idx, npts-1);
//	      v += c[idx];
//	    }
//	  v /= (2*sz+2);
//	  newc[i] = v;
//	}
//    }

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
