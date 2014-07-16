#ifndef SHADERFACTORY2_H
#define SHADERFACTORY2_H

#include <GL/glew.h>

class ShaderFactory2
{
 public :
  static QString genDefaultSliceShaderString(bool, bool,
					     QList<CropObject>,
					     int,
					     bool, int, float, float, float,
					     int, bool, bool, int);

  static QString genPruneTexture(int);

 private :
  static QString tagVolume();
  static QString blendVolume(int);
  static QString getNormal(int);
  static QString addLighting(int);
  static QString mixColorOpacity(int, int, bool, bool);
  static QString genVgx(int);
};

#endif
