#include "fibergroup.h"
#include "global.h"


FiberGroup::FiberGroup()
{
  m_fibers.clear();
}

FiberGroup::~FiberGroup()
{
  m_fibers.clear();
}

void
FiberGroup::reset()
{
  m_fibers.clear();
}

void
FiberGroup::newFiber()
{
  for(int i=0; i<m_fibers.count(); i++)
    m_fibers[i]->selected = false;

  Fiber *fb = new Fiber;
  fb->tag = Global::tag();
  fb->thickness = Global::thickness();
  fb->selected = true;

  m_fibers << fb;

  QMessageBox::information(0, "", QString("new fiber %1").arg(m_fibers.count()));
}

void
FiberGroup::addPoint(int d, int w, int h)
{
  if (m_fibers.count() == 0)
    return;

  for(int i=0; i<m_fibers.count(); i++)
    {
      if (m_fibers[i]->selected)
	{
	  m_fibers[i]->addPoint(d, w, h);
	  return;
	}
    }
}

void
FiberGroup::removePoint(int d, int w, int h)
{
  if (m_fibers.count() == 0)
    return;

  for(int i=0; i<m_fibers.count(); i++)
    {
      Fiber *fb = m_fibers[i];
      if (fb->containsSeed(d, w, h))
	{
	  fb->removePoint(d, w, h);
	  return;
	}
    }
}

void
FiberGroup::selectFiber(int d, int w, int h)
{
  if (m_fibers.count() == 0)
    return;

  for(int i=0; i<m_fibers.count(); i++)
    {
      if (m_fibers[i]->contains(d, w, h))
	{
	  if (m_fibers[i]->selected)
	    m_fibers[i]->selected = false;
	  else
	    {
	      for(int f=0; f<m_fibers.count(); f++)
		m_fibers[f]->selected = false;

	      m_fibers[i]->selected = true;
	    }
	  return;
	}
    }

  for(int f=0; f<m_fibers.count(); f++)
    m_fibers[f]->selected = false;
}

QVector<QPointF>
FiberGroup::xyPoints(int type, int slc)
{
  QVector<QPointF> xypoints;
  for (int i=0; i<m_fibers.count(); i++)
    if (!m_fibers[i]->selected)
      xypoints += m_fibers[i]->xyPoints(type, slc);
  
  return xypoints;
}

QVector<QPointF>
FiberGroup::xyPointsSelected(int type, int slc)
{
  QVector<QPointF> xypoints;
  for (int i=0; i<m_fibers.count(); i++)
    if (m_fibers[i]->selected)
      xypoints += m_fibers[i]->xyPoints(type, slc);
  
  return xypoints;
}

QVector<QPointF>
FiberGroup::xySeeds(int type, int slc)
{
  QVector<QPointF> xypoints;
  for (int i=0; i<m_fibers.count(); i++)
    if (!m_fibers[i]->selected)
      xypoints += m_fibers[i]->xySeeds(type, slc);
  
  return xypoints;
}

QVector<QPointF>
FiberGroup::xySeedsSelected(int type, int slc)
{
  QVector<QPointF> xypoints;
  for (int i=0; i<m_fibers.count(); i++)
    if (m_fibers[i]->selected)
      xypoints += m_fibers[i]->xySeeds(type, slc);
  
  return xypoints;
}
