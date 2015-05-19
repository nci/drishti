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
  QVector<QPoint> seeds;
  QVector<int> seedpos;
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
  void setPolygonAt(int,
		    QVector<QPoint>,
		    QVector<QPoint>,
		    QVector<int>,
		    bool, int, int, bool);
  void joinPolygonAt(int, QVector<QPoint>);
  void setCurveAt(int, Curve);

  void smooth(int, int, int, int);
  void push(int, int, int, int);
  
  void resetMoveCurve();
  void setMoveCurve(int, int, int);
  void moveCurve(int, int, int);

  QList<int> polygonLevels();

  int showPolygonInfo(int, int, int);
  bool selectPolygon(int, int, int, bool);
  void setClosed(int, int, int, bool);
  void setThickness(int, int, int, int);
  void setTag(int, int, int, int);
  void flipPolygon(int, int, int);

  int copyCurve(int, int, int);
  Curve getCopyCurve() { return m_copyCurve; };
  void pasteCurve(int);

  int getActiveCurve(int, int, int);
  int getActiveMorphedCurve(int, int, int);

  QList< QMap<int, Curve> >* getPointerToMorphedCurves();
  void addMorphBlock(QMap<int, Curve>);

  QList<QPoint> xpoints(int);
  QList<QPoint> ypoints(int);

  void startAddingCurves();
  void endAddingCurves();

  QMultiMap<int, Curve*>* multiMapCurves() { return &m_cg; }
  QList< QMap<int, Curve> >* listMapCurves() { return &m_mcg; }

 private :
  QMultiMap<int, Curve*> m_cg;
  QList< QMap<int, Curve> > m_mcg;  

  bool m_addingCurves;
  QMap<int, Curve> m_tmcg;

  bool m_pointsDirtyBit;
  QMultiMap<int, QPoint> m_xpoints;
  QMultiMap<int, QPoint> m_ypoints;

  QVector<QPoint> subsample(QVector<QPoint>, float, bool);
  QVector<QPoint> smooth(QVector<QPoint>, QPoint, int, bool);
  QVector<QPoint> push(QVector<QPoint>, QPoint, int, bool);

  Curve m_copyCurve;
  int m_moveCurve;

  void alignClosedCurves(QMap<int, QVector<QPoint> >&);
  void alignOpenCurves(QMap<int, QVector<QPoint> >&);
  void clearMorphedCurves();
  float pathLength(Curve*);
  float area(Curve*);

  void generateXYpoints();
};

#endif
