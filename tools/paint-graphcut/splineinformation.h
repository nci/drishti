#ifndef SPLINEINFORMATION_H
#define SPLINEINFORMATION_H

#include <QtGui>
#include <QDomDocument>

#include <fstream>
using namespace std;

class SplineInformation
{
 public :
  SplineInformation();
  SplineInformation& operator=(const SplineInformation&);

  void load(fstream&);
  void save(fstream&);

  static SplineInformation interpolate(SplineInformation&,
				       SplineInformation&,
				       float);

  static QList<SplineInformation> interpolate(QList<SplineInformation>,
					      QList<SplineInformation>,
					      float);

  static QGradientStops interpolateGradientStops(QGradientStops,
						 QGradientStops,
						 float);

  void setName(QString);
  void setOn(QList<bool>);
  void setPoints(QPolygonF);
  void setNormalWidths(QPolygonF);
  void setNormalRotations(QVector<float>);
  void setGradientStops(QGradientStops);

  QString name();
  QList<bool> on();
  QPolygonF points();
  QPolygonF normalWidths();
  QVector<float> normalRotations();
  QGradientStops gradientStops();
  
 private :
  QString m_name;
  QList<bool> m_on;
  QPolygonF m_points;
  QPolygonF m_normalWidths;
  QVector<float> m_normalRotations;
  QGradientStops m_gradientStops;  
};

#endif
