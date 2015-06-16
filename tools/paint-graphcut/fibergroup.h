#ifndef FIBERGROUP_H
#define FIBERGROUP_H

#include "fiber.h"

class FiberGroup
{
 public :
  FiberGroup();
  ~FiberGroup();


  bool fibersPresent() { return m_fibers.count() > 0; };
  QList<Fiber*>* fibers() { return &m_fibers; };

  void reset();

  void newFiber();
  void endFiber();
  void addPoint(int, int, int);
  void removePoint(int, int, int);
  void selectFiber(int, int, int);
  void setTag(int, int, int);
  void setThickness(int, int, int);

  QVector<QVector4D> xyPoints(int, int);
  QVector<QVector4D> xySeeds(int, int);
  QVector<QVector4D> xyPointsSelected(int, int);
  QVector<QVector4D> xySeedsSelected(int, int);

 private :
  QList<Fiber*> m_fibers;
};

#endif
