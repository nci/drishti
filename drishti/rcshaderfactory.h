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
				   QList<CropObject>);

  static QString genIsoRaycastShader(bool,
				     QList<CropObject>);

  static QString genRaycastShader(bool,
				  QList<CropObject>);

  static QString genXRayShader(bool,
			       QList<CropObject>);

  static QString genEdgeEnhanceShader();

 private :
  static QString addLighting();
  static QString getGrad();
};

#endif
