#ifndef RCSHADERFACTORY_H
#define RCSHADERFACTORY_H

#include <GL/glew.h>
#include "commonqtclasses.h"
#include "cropobject.h"

class RcShaderFactory
{
 public :
  static bool loadShader(GLhandleARB&, QString);

  static QString genRectBlurShaderString(int);

  static QString genFirstHitShader(bool,
				   QList<CropObject>,
				   bool);

  static QString genIsoRaycastShader(bool,
				     QList<CropObject>,
				     bool);

  static QString genRaycastShader(bool,
				  QList<CropObject>,
				  bool);

  static QString genXRayShader(bool,
			       QList<CropObject>,
			       bool);

  static QString genEdgeEnhanceShader(bool);

 private :
  static QString addLighting();
  static QString getGrad();
};

#endif
