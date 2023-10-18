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

  QList<QPolygonF> boundaryCurves(uchar*, int, int, bool shrinkwrap=false);

 private :
  int m_nX, m_nY;
  uchar *m_startSlice;
  uchar *m_endSlice;
  QVBoxLayout* m_layout;

  void clearSlices();
  void showSliceImage(uchar*, int, int);
  void showCurves(QList<QPolygonF>);


  QMap< int, QList<QPolygonF> > mergeSlices(int);

  void distanceTransform(float*, float*, int);
  void distanceTransform(float*, uchar*, int, int, bool);
  
};

#endif // MORPHSLICE_H
