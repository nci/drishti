#ifndef ITKSEGMENTATION_H
#define ITKSEGMENTATION_H

#include <QObject>
#include "mopplugininterface.h"

class ITKSegmentation : public QObject, MopPluginInterface
{
  Q_OBJECT
  Q_INTERFACES(MopPluginInterface)  

 public :
  QStringList registerPlugin();

  void setData(int, int, int, uchar*, QList<Vec>);

  void init();
  bool start();

 private :
  int m_nX, m_nY, m_nZ;
  uchar* m_data;  
  QList<Vec> m_points;

  QProgressDialog *m_progress;
  int m_prog;

  void next();

  bool regionGrowing(int, int, int, uchar*, QList<Vec>);

  void connectedComponent(int, int, int, uchar*);

  void connectedThreshold(int, int, int, uchar*, QList<Vec>, int,
         		 int, int);

  void neighborhoodConnected(int, int, int, uchar*, QList<Vec>, int,
         		    int, int, int);

  void confidenceConnected(int, int, int, uchar*, QList<Vec>, int,
         		  int, float, int);

  void watershed(int, int, int, uchar*, QList<Vec>);

  void binaryThinning(int, int, int, uchar*);
};

#endif
