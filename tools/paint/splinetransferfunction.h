#ifndef SPLINETRANSFERFUNCTION_H
#define SPLINETRANSFERFUNCTION_H

#include "splineinformation.h"

class SplineTransferFunction : public QObject
{
  Q_OBJECT

 public :

  enum NormalType {
    LeftNormal,
    RightNormal
  };

  SplineTransferFunction();
  ~SplineTransferFunction();

  QDomElement domElement(QDomDocument&);
  void fromDomElement(QDomElement);

  int size();
  QImage colorMapImage();

  QString name();
  void setName(QString);

  void setOn(int, bool);
  bool on(int);

  SplineInformation getSpline();
  void setSpline(SplineInformation);

  void setGradientStops(QGradientStops);
  QGradientStops gradientStops();
  QPointF pointAt(int);
  QPointF rightNormalAt(int);
  QPointF leftNormalAt(int);
  void removePointAt(int);
  void appendPoint(QPointF);
  void insertPointAt(int, QPointF);
  void movePointAt(int, QPointF);
  void moveAllPoints(QPointF);
  void rotateNormalAt(int, int, QPointF);
  void moveNormalAt(int, int, QPointF, bool);

  void switch1D();

  void set16BitPoint(float, float);
 private :
  QList<bool> m_on;
  QString m_name;
  QPolygonF m_points;
  QPolygonF m_normals;
  QPolygonF m_normalWidths;
  QPolygonF m_leftNormals;
  QPolygonF m_rightNormals;
  QVector<float> m_normalRotations;
  QGradientStops m_gradientStops;

  QImage m_colorMapImage;

  void rotateNormal(QPointF);
  void updateNormals();
  void updateColorMapImage();
  void updateColorMapImageFor16bit();
  void getPainterPathSegment(QPainterPath*, QPolygonF, int);
};

#endif
