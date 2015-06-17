#ifndef FIBER_H
#define FIBER_H

#include <QGLViewer/vec.h>
using namespace qglviewer;

#include <QVector4D>

class Fiber
{
 public :
  Fiber();
  ~Fiber();

  Fiber& operator=(const Fiber&);

  QVector<Vec> seeds;
  QVector<Vec> smoothSeeds;
  QList<Vec> trace;
  int tag;
  int thickness;
  bool selected;

  void addPoint(int, int, int);
  void removePoint(int, int, int);
  bool containsSeed(int, int, int);
  bool contains(int, int, int);
  
  QVector<QVector4D> xyPoints(int, int);
  QVector<QVector4D> xySeeds(int, int);

  void updateTrace();

  int sections() { return m_sections; }
  QList<Vec> tube() { return m_tube; }

 private :
  int m_sections;
  QList<Vec> m_tube;
  QMultiMap<int, QVector4D> m_dPoints;
  QMultiMap<int, QVector4D> m_wPoints;
  QMultiMap<int, QVector4D> m_hPoints;
  QMultiMap<int, QVector4D> m_dSeeds;
  QMultiMap<int, QVector4D> m_wSeeds;
  QMultiMap<int, QVector4D> m_hSeeds;

  QList<Vec> line3d(Vec, Vec);


  void generateTube(float);
  void addRoundCaps(int, Vec, QList<Vec>, QList<Vec>);
  QList<Vec> getCrossSection(float, int, Vec, Vec);
  QList<Vec> getNormals(QList<Vec>, Vec);

};

#endif
