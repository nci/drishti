#ifndef PATHSHADERFACTORY_H
#define PATHSHADERFACTORY_H

class PathShaderFactory
{
 public :
  static QString applyPathCrop();
  static QString applyPathBlend(int nvols = 1);

  static bool cropPresent();
  static bool blendPresent();

};

#endif
