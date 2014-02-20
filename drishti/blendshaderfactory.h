#ifndef BLENDSHADERFACTORY_H
#define BLENDSHADERFACTORY_H

#include "cropobject.h"

class BlendShaderFactory
{
 public :
  static QString generateBlend(QList<CropObject>, int nvol = 1);

 private :
  static QString applyBlend(Vec, float,
			    Vec, Vec, Vec,
			    float, float,
			    float, float,
			    int, float,
			    int, int);
};

#endif
