#ifndef CROPSHADERFACTORY_H
#define CROPSHADERFACTORY_H

#include <QtGui>
#include "cropobject.h"

class CropShaderFactory
{
 public :
  static QString generateCropping(QList<CropObject>);

 private :
  static QString applyTubeCropping(bool, bool, bool,
				   Vec, Vec, float,
				   Vec, Vec,
				   float, float, float, float,
				   int, int,
				   int);
  static QString applyEllipsoidCropping(bool, bool, bool,
					Vec, float,
					Vec, Vec, Vec,
					float, float,
					int, int,
					int);
  static QString applyBoxCropping(bool, bool, bool,
				  Vec, float,
				  Vec, Vec, Vec,
				  float, float,
				  float, float,
				  int, int,
				  int);
};

#endif
