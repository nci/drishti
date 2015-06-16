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
FiberGroup::addFiber(Fiber f)
{
  Fiber *fb = new Fiber;
  fb->tag = f.tag;
  fb->thickness = f.thickness;
  fb->seeds = f.seeds;
//  for(int i=0; i<f.seeds.count(); i++)
//    fb->seeds << f.seeds[i];
  fb->selected = false;
  fb->updateTrace();

  m_fibers << fb;
}

void
FiberGroup::newFiber()
{
  //-------------------
  // weed out empty fibers
  QList<Fiber*> tmp = m_fibers;
  m_fibers.clear();
  for(int i=0; i<tmp.count(); i++)
    if (tmp[i]->seeds.count() > 0)
      m_fibers << tmp[i];
  //-------------------
  
  for(int i=0; i<m_fibers.count(); i++)
    m_fibers[i]->selected = false;

  Fiber *fb = new Fiber;
  fb->tag = Global::tag();
  fb->thickness = Global::thickness();
  fb->selected = true;

  m_fibers << fb;
}

void
FiberGroup::endFiber()
{
  for(int f=0; f<m_fibers.count(); f++)
    m_fibers[f]->selected = false;
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

void
FiberGroup::setTag(int d, int w, int h)
{
  if (m_fibers.count() == 0)
    return;

  for(int i=0; i<m_fibers.count(); i++)
    {
      if (m_fibers[i]->contains(d, w, h))
	{
	  m_fibers[i]->tag = Global::tag();
	  m_fibers[i]->updateTrace();
	  return;
	}
    }
}

void
FiberGroup::setThickness(int d, int w, int h)
{
  if (m_fibers.count() == 0)
    return;

  for(int i=0; i<m_fibers.count(); i++)
    {
      if (m_fibers[i]->contains(d, w, h))
	{
	  m_fibers[i]->thickness = Global::thickness();
	  m_fibers[i]->updateTrace();
	  return;
	}
    }
}

QVector<QVector4D>
FiberGroup::xyPoints(int type, int slc)
{
  QVector<QVector4D> xypoints;
  for (int i=0; i<m_fibers.count(); i++)
    if (!m_fibers[i]->selected)
      xypoints += m_fibers[i]->xyPoints(type, slc);
  
  return xypoints;
}

QVector<QVector4D>
FiberGroup::xyPointsSelected(int type, int slc)
{
  QVector<QVector4D> xypoints;
  for (int i=0; i<m_fibers.count(); i++)
    if (m_fibers[i]->selected)
      xypoints += m_fibers[i]->xyPoints(type, slc);
  
  return xypoints;
}

QVector<QVector4D>
FiberGroup::xySeeds(int type, int slc)
{
  QVector<QVector4D> xypoints;
  for (int i=0; i<m_fibers.count(); i++)
    if (!m_fibers[i]->selected)
      xypoints += m_fibers[i]->xySeeds(type, slc);
  
  return xypoints;
}

QVector<QVector4D>
FiberGroup::xySeedsSelected(int type, int slc)
{
  QVector<QVector4D> xypoints;
  for (int i=0; i<m_fibers.count(); i++)
    if (m_fibers[i]->selected)
      xypoints += m_fibers[i]->xySeeds(type, slc);
  
  return xypoints;
}
