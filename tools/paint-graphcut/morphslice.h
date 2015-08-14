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
  QPointF m_xlate;
  uchar *m_startSlice;
  uchar *m_endSlice;
  uchar *m_overlapSlice;

  void clearSlices();
  void showSliceImage(QVBoxLayout*, uchar*, int, int);
  void showCurves(QVBoxLayout*, QList<QPolygonF>);


  QMap< int, QList<QPolygonF> > mergeSlices(int);
  QList<uchar*> dilateOverlap(uchar*, uchar*);
  uchar* getMedianSlice(QList<uchar*>, QList<uchar*>);
  QList<QPolygonF> boundaryCurves(QVBoxLayout*, uchar*);

};

#endif // MORPHSLICE_H
