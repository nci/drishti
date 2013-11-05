#ifndef NETWORKINFORMATION_H
#define NETWORKINFORMATION_H

#include <QGLViewer/qglviewer.h>
using namespace qglviewer;

#include <fstream>
using namespace std;

class NetworkInformation
{
 public :
  NetworkInformation();
  NetworkInformation& operator=(const NetworkInformation&);

  static NetworkInformation interpolate(const NetworkInformation,
					const NetworkInformation,
					float);

  static QList<NetworkInformation> interpolate(const QList<NetworkInformation>,
					       const QList<NetworkInformation>,
					       float);

  void clear();

  void save(fstream&);
  void load(fstream&);
  
  QString filename;
  float Vopacity, Eopacity;
  int Vatt, Eatt;
  float userVmin, userVmax;
  float userEmin, userEmax;
  QGradientStops Vstops;
  QGradientStops Estops;
  float scalee, scalev;
};

#endif
