#ifndef CURVEGROUP_H
#define CURVEGROUP_H

#include <QWidget>
#include "commonqtclasses.h"
#include <QVector3D>
#include <QMessageBox>
#include <QMap>
#include <QMultiMap>

class Curve
{
 public :
  Curve();
  ~Curve();
  Curve& operator=(const Curve&);

  QVector<QPoint> pts;
  int tag;
  int thickness;
  bool closed;
  bool selected;
};

class CurveGroup
{
 public :
  CurveGroup();
  ~CurveGroup();

  void reset();
  void resetPolygonAt(int, int, int);
  void morphCurves();

  void newCurve(int);

  void addPoint(int, int, int);
  void removePoint(int, int, int);

  QVector<QPoint> getPolygonAt(int);
  QList<Curve*> getCurvesAt(int);

  void setPolygonAt(int, int*, int, int, int, bool);  

  void setPolygonAt(int, QVector<QPoint>);  

  void smooth(int, int, int, int);
  void push(int, int, int, int);

  QList<int> polygonLevels();

  bool selectPolygon(int, int, int);
  void setClosed(int, int, int, bool);
  void setThickness(int, int, int, int);
  void flipPolygon(int, int, int);

 private :
  QMultiMap<int, Curve*> m_cg;
  QMultiMap<int, Curve*> m_mcg;

  QVector<QPoint> subsample(QVector<QPoint>, float, bool);
  QVector<QPoint> smooth(QVector<QPoint>, int, int, int, bool);

  void alignAllCurves(QMap<int, QVector<QPoint> >&);
  void clearMorphedCurves();
  int getActiveCurve(int, int, int);
};

#endif
