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
  fb->pts << Vec(h, w, d);
}


void
FiberGroup::removePoint(int d, int w, int h)
{
  if (m_fibers.count() == 0)
    return;

  for(int i=0; i<m_fibers.count(); i++)
    {
      Fiber *fb = m_fibers[i];
      bool ok = false;
      for(int j=0; j<fb->pts.count(); j++)
	{
	  if ((fb->pts[j]-Vec(h, w, d)).squaredNorm() < 5)
	    {
	      fb->pts.removeAt(j);
	      ok = true;
	      break;
	    }
	}
      if (ok)
	break;
    }
}
