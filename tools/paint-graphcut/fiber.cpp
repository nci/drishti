#include "fiber.h"

Fiber::Fiber()
{
  tag = 0;
  thickness = 1;
  selected = false;
  pts.clear();
}

Fiber::~Fiber()
{
  tag = 0;
  thickness = 1;
  selected = false;
  pts.clear();
}

Fiber&
Fiber::operator=(const Fiber& f)
{
  tag = f.tag;
  thickness = f.thickness;
  selected = f.selected;
  pts = f.pts;

  return *this;
}
