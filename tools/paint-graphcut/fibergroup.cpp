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
  Fiber *fb = new Fiber;
  fb->tag = Global::tag();
  fb->thickness = Global::thickness();
  fb->selected = false;

  m_fibers << fb;

  QMessageBox::information(0, "", QString("new fiber %1").arg(m_fibers.count()));
}

void
FiberGroup::addPoint(int d, int w, int h)
{
  if (m_fibers.count() == 0)
    return;

  Fiber *fb = m_fibers.last();
  fb->addPoint(d, w, h);
}


void
FiberGroup::removePoint(int d, int w, int h)
{
  if (m_fibers.count() == 0)
    return;

  for(int i=0; i<m_fibers.count(); i++)
    {
      Fiber *fb = m_fibers[i];
      if (fb->contains(d, w, h))
	{
	  fb->removePoint(d, w, h);
	  return;
	}
    }
}

QVector<QPointF>
FiberGroup::xyPoints(int type, int slc)
{
  QVector<QPointF> xypoints;
  for (int i=0; i<m_fibers.count(); i++)
    xypoints += m_fibers[i]->xyPoints(type, slc);
  
  return xypoints;
}

QVector<QPointF>
FiberGroup::xySeeds(int type, int slc)
{
  QVector<QPointF> xypoints;
  for (int i=0; i<m_fibers.count(); i++)
    xypoints += m_fibers[i]->xySeeds(type, slc);
  
  return xypoints;
}
