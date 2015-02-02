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

 private :
  QMap<int, QVector<QPoint> > m_cg;
  QMap<int, QVector<QPoint> > m_mcg;
  
};

#endif
