#ifndef MORPHSLICE_H
#define MORPHSLICE_H

#include <QtWidgets>
#include "commonqtclasses.h"
#include <QPointF>
#include <QMap>
#include <QVBoxLayout>

class MorphSlice
{
 public :
  MorphSlice();
  ~MorphSlice();

  QMap< int, QList<QPolygonF> > setPaths(QMap< int, QList<QPolygonF> >);

 private :
  int m_nX, m_nY;
  int *m_startSlice;
  int *m_endSlice;
  int *m_overlapSlice;
  QList<int*> m_o2E;
  QList<int*> m_o2S;

  void clearSlices();
  void showSliceImage(QVBoxLayout*, int*);
  void showCurves(QVBoxLayout*, QList<QPolygonF>);

  QList<int*> dilateOverlap(int*);

  QMap< int, QList<QPolygonF> > mergeSlices(QVBoxLayout*, int);
  void boundary(int*);
  QList<QPolygonF> boundaryCurves(int*);
};

#endif // MORPHSLICE_H
