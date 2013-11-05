#ifndef GLOWSHADERFACTORY_H
#define GLOWSHADERFACTORY_H

#include <QtGui>
#include "cropobject.h"

class GlowShaderFactory
{
 public :
  static QString generateGlow(QList<CropObject>,
			      int nvol = 1,
			      QString fragColor = QString("gl_FragColor.rgba"));

 private :
  static QString applyGlow(bool, bool, bool,
			   Vec, float,
			   Vec, Vec, Vec,
			   float, float,
			   float, float,
			   int, float,
			   int, int);
};

#endif
