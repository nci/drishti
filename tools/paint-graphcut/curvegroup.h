#ifndef CURVEGROUP_H
#define CURVEGROUP_H

#include <QWidget>
#include "commonqtclasses.h"
#include <QVector3D>
#include <QMessageBox>
#include <QMap>

class CurveGroup
{
 public :
  CurveGroup();
  ~CurveGroup();

  void reset();
  void resetPolygonAt(int);
  void morphCurves();
  void addPoint(int, int, int);
  QVector<QPoint> getPolygonAt(int);

  void flipPolygonAt(int);
  void smooth(int, int, int, int);
  void push(int, int, int, int);

  QList<int> polygonLevels();

 private :
  QMap<int, QVector<QPoint> > m_cg;
  QMap<int, QVector<QPoint> > m_mcg;

  QVector<QPoint> subsample(QVector<QPoint>, float);
  QVector<QPoint> smooth(QVector<QPoint>, int, int, int, int);

  void alignAllCurves();
};

#endif
