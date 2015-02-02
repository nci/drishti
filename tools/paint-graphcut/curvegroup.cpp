#include "curvegroup.h"
#include "morphcurve.h"

CurveGroup::CurveGroup()
{
  m_cg.clear();
}

CurveGroup::~CurveGroup()
{
  m_cg.clear();
}

void
CurveGroup::reset()
{
  m_cg.clear();
  m_mcg.clear();
}

void
CurveGroup::resetPolygonAt(int key)
{
  m_cg.remove(key);
}

void
CurveGroup::addPoint(int key, int v0, int v1)
{
  QVector<QPoint> c = m_cg.value(key);
  c << QPoint(v0, v1);
  m_cg.insert(key, c);
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
}
