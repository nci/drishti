#ifndef TEARSHADERFACTORY_H
#define TEARSHADERFACTORY_H

#include "cropobject.h"

class TearShaderFactory
{
 public :
  static QString generateTear(QList<CropObject>);

 private :
  static QString applyTear(Vec, float,
			   Vec, Vec, Vec,
			   float, float,
			   float, float,
			   int,
			   int, int,
			   float);
  static QString applyDisplace(Vec, float,
			       Vec, Vec, Vec,
			       float, float,
			       float, float,
			       int,
			       Vec, Vec, Vec, float,
			       int, int);
};

#endif
