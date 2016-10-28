#ifndef TRANSFERFUNCTIONCONTAINER_H
#define TRANSFERFUNCTIONCONTAINER_H

#include <QtCore>
#include "splinetransferfunction.h"

class TransferFunctionContainer : public QObject
{
  Q_OBJECT

 public :
  TransferFunctionContainer();
  ~TransferFunctionContainer();
 
  int count();
  int maxSets();
  SplineTransferFunction* transferFunctionPtr(int);
  QImage composite(int col=0);
  void setCheckState(int, int, bool);
  bool getCheckState(int, int);
  QImage colorMapImage(int);

 public slots :  
  void switch1D();
  void clearContainer();
  void fromDomElement(QDomElement);
  void fromSplineInformation(SplineInformation);
  void addSplineTF();
  void removeSplineTF(int);

 private :
  int m_maxSets;
  QVector<SplineTransferFunction *> m_splineTF;  
};

#endif
