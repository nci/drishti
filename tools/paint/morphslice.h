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

  void setAllPaths(QMap< int, QList<QPolygonF> >);
  QMap< int, QList<QPolygonF> > computeIntermediates(int, int);

  QList<QPolygonF> boundaryCurves(uchar*, int, int, bool shrinkwrap=false);

 private :
  int m_nX, m_nY;
  uchar *m_startSlice;
  uchar *m_endSlice;
  QVBoxLayout* m_layout;

  QList<float*> m_slicesT;
  QList<float*> m_slicesV;
  
  void clearSlices();
  void showSliceImage(uchar*, int, int);
  void showCurves(QList<QPolygonF>);


  QMap< int, QList<QPolygonF> > mergeSlices(int);

  void distanceTransform(float*, float*, int);
  void distanceTransform(float*, uchar*, int, int, bool);

  void computeTangent(QList<int>);
  QMap< int, QList<QPolygonF> > splineInterpolateSlices(int, int);
  
};

#endif // MORPHSLICE_H
