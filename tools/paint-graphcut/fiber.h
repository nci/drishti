#ifndef FIBER_H
#define FIBER_H

#include <QGLViewer/vec.h>
using namespace qglviewer;

class Fiber
{
 public :
  Fiber();
  ~Fiber();

  Fiber& operator=(const Fiber&);

  QVector<Vec> seeds;
  QList<Vec> trace;
  int tag;
  int thickness;
  bool selected;

  void addPoint(int, int, int);
  void removePoint(int, int, int);
  bool containsSeed(int, int, int);
  bool contains(int, int, int);
  
  QVector<QPointF> xyPoints(int, int);
  QVector<QPointF> xySeeds(int, int);

 private :
  QMultiMap<int, QPointF> m_dPoints;
  QMultiMap<int, QPointF> m_wPoints;
  QMultiMap<int, QPointF> m_hPoints;

  QMultiMap<int, QPointF> m_dSeeds;
  QMultiMap<int, QPointF> m_wSeeds;
  QMultiMap<int, QPointF> m_hSeeds;

  void updateTrace();
  QList<Vec> line3d(Vec, Vec);
};

#endif
