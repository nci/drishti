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
  QPointF m_xlate;
  uchar *m_startSlice;
  uchar *m_endSlice;
  uchar *m_overlapSlice;
  QVBoxLayout* m_layout;

  void clearSlices();
  void showSliceImage(uchar*, int, int);
  void showCurves(QList<QPolygonF>);


  QMap< int, QList<QPolygonF> > mergeSlices(int);
  QList<uchar*> dilateOverlap(uchar*, uchar*);
  uchar* getMedianSlice(QList<uchar*>, QList<uchar*>);

};

#endif // MORPHSLICE_H
