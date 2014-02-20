#ifndef SHADERFACTORY_H
#define SHADERFACTORY_H

#include <GL/glew.h>
#include "cropobject.h"

class ShaderFactory
{
 public :
  static bool loadShader(GLhandleARB&, QString);

  static QString genDefaultShaderString(bool, bool, int);

  static QString genBlurShaderString(bool, int, float);
  static QString genRectBlurShaderString(int);
  static QString genCopyShaderString();
  static QString genBackplaneShaderString1(float);
  static QString genBackplaneShaderString2(float);

  static QString genPassThruShaderString();

  static int loadShaderFromFile(GLhandleARB, const char*);


  static QString genDefaultSliceShaderString(bool, bool, bool,
					     QList<CropObject>,
					     bool, int, float, float, float);
  static QString genHighQualitySliceShaderString(bool, bool, bool, bool,
						 QList<CropObject>,
						 bool, int, float, float, float);
  static QString genSliceShadowShaderString(bool, float, float, float, float,
					    QList<CropObject>,
					    bool, int, float, float, float);

  static QString genTextureCoordinate();

  static QString genLutShaderString(bool);

 private :
  static QString getNormal();
  static QString addLighting();
  static QString tagVolume();
  static QString blendVolume();
  static QString genPeelShader(bool, int, float, float, float, bool);
  static QString genVgx();
};

#endif
