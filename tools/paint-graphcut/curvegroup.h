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
  void removePolygonAt(int, int, int);
  void morphCurves();

  void newCurve(int, bool);

  void addPoint(int, int, int);
  void removePoint(int, int, int);

  QVector<QPoint> getPolygonAt(int);
  QList<Curve*> getCurvesAt(int);
  QList<Curve> getMorphedCurvesAt(int);

  void setPolygonAt(int, int*, int, int, int, bool);  

  void setPolygonAt(int, QVector<QPoint>, bool);  

  void smooth(int, int, int, int);
  void push(int, int, int, int);
  
  void resetMoveCurve();
  void setMoveCurve(int, int, int);
  void moveCurve(int, int, int);

  QList<int> polygonLevels();

  void showPolygonInfo(int, int, int);
  bool selectPolygon(int, int, int, bool);
  void setClosed(int, int, int, bool);
  void setThickness(int, int, int, int);
  void setTag(int, int, int, int);
  void flipPolygon(int, int, int);

  void copyCurve(int, int, int);
  void pasteCurve(int);

 private :
  QMultiMap<int, Curve*> m_cg;
  QList< QMap<int, Curve> > m_mcg;

  QVector<QPoint> subsample(QVector<QPoint>, float, bool);
  QVector<QPoint> smooth(QVector<QPoint>, int, int, int, bool);

  Curve m_copyCurve;
  int m_moveCurve;

  void alignAllCurves(QMap<int, QVector<QPoint> >&);
  void clearMorphedCurves();
  int getActiveCurve(int, int, int, int mdst=5);
  int getActiveMorphedCurve(int, int, int);
  float pathLength(Curve*);
  float area(Curve*);
};

#endif
