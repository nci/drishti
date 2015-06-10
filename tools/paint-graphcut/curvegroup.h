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

  QVector<QPointF> pts;
  QVector<int> seedpos;
  int tag;
  int thickness;
  bool closed;
  bool selected;
  uchar type;
  QVector<QPointF> seeds;
};

class CurveGroup
{
 public :
  CurveGroup();
  ~CurveGroup();

  bool curvesPresent() {return (m_cg.count()>0 ||
				m_mcg.count()>0); };

  void reset();
  void removePolygonAt(int, int, int);
  void morphCurves(int, int);

  void newShape(int, int, int);
  void newPolygon(int, int, int);
  void newEllipse(int, int, int);
  void newCurve(int, bool);

  void addPoint(int, int, int);
  void removePoint(int, int, int);

  QVector<QPointF> getPolygonAt(int);
  QList<Curve*> getCurvesAt(int);
  QList<Curve> getMorphedCurvesAt(int);

  void setPolygonAt(int, int*, int, int, int, bool, uchar type = 0);
  void setPolygonAt(int,
		    QVector<QPointF>,
		    QVector<int>,
		    bool, int, int, bool,
		    uchar,
		    QVector<QPointF> seeds);
  void joinPolygonAt(int, QVector<QPointF>);
  void setCurveAt(int, Curve);

  void smooth(int, int, int);
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

  void deselectAll();

  void resetCopyCurve();
  int copyCurve(int, int, int);
  Curve getCopyCurve() { return m_copyCurve; };
  void pasteCurve(int);

  int getActiveCurve(int, int, int);
  int getActiveMorphedCurve(int, int, int);

  QList< QMap<int, Curve> >* getPointerToMorphedCurves();
  void addMorphBlock(QMap<int, Curve>);

  QList<QPointF> xpoints(int);
  QList<QPointF> ypoints(int);

  void startAddingCurves();
  void endAddingCurves();

  QMultiMap<int, Curve*>* multiMapCurves() { return &m_cg; };
  QList< QMap<int, Curve> >* listMapCurves() { return &m_mcg; };

  void setLambda(float l) { m_lambda = l; };
  void setSegmentLength(int l) { m_seglen = l; };

 private :
  float m_lambda;
  int m_seglen;

  QMultiMap<int, Curve*> m_cg;
  QList< QMap<int, Curve> > m_mcg;  

  bool m_addingCurves;
  QMap<int, Curve> m_tmcg;

  bool m_pointsDirtyBit;
  QMultiMap<int, QPointF> m_xpoints;
  QMultiMap<int, QPointF> m_ypoints;

  QVector<QPointF> smooth(QVector<QPointF>, bool);
  void smoothCurveWithSeedPoints(Curve *);

  QVector<QPointF> push(QVector<QPointF>, QPointF, int, bool);

  QVector<QPointF> subsample(QVector<QPointF>, float, bool);

  Curve m_copyCurve;
  int m_moveCurve;

  void alignClosedCurves(QMap<int, QVector<QPointF> >&);
  void alignOpenCurves(QMap<int, QVector<QPointF> >&);
  void clearMorphedCurves();
  float pathLength(Curve*);
  float area(Curve*);

  void generateXYpoints();

  void generatePolygon(Curve*);
  void generateEllipse(Curve*);
};

#endif
