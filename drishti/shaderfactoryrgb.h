#ifndef SHADERFACTORYRGB_H
#define SHADERFACTORYRGB_H

#include <GL/glew.h>

class ShaderFactoryRGB
{
 public :
  static QString genDefaultShaderString(bool, bool);
  
  static QString genDefaultSliceShaderString(bool, bool,
					     QList<CropObject> crops,
					     bool, int, float, float, float);
  static QString genHighQualitySliceShaderString(bool, bool, bool, float,
						 QList<CropObject> crops,
						 bool, int, float, float, float);
  static QString genSliceShadowShaderString(float, float, float,
					    QList<CropObject> crops,
					    bool, int, float, float, float);

 private :
  static QString getColorOpacity(bool lowres=false);
  static QString genEmissiveColor(bool lowres=false);
  static QString getAlpha();
  static QString getNormal();
  static QString addLighting();
};

#endif
