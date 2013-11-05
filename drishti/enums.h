#ifndef ENUMS_H
#define ENUMS_H

#define DEG2RAD(angle) angle*3.1415926535897931/180.0
#define RAD2DEG(angle) angle*180.0/3.1415926535897931
#define VECPRODUCT(a, b) Vec(a.x*b.x, a.y*b.y, a.z*b.z)
#define VECDIVIDE(a, b) Vec(a.x/b.x, a.y/b.y, a.z/b.z)

class Enums
{
 public :

  enum KeyFrameInterpolationType
    {
      KFIT_Linear = 0,
      KFIT_SmoothStep,
      KFIT_EaseIn,
      KFIT_EaseOut,
      KFIT_None
    };

    enum RenderQuality {
      RenderDefault,
      RenderHighQuality
    };
    
    enum ImageQuality {
      DragImage,
      StillImage
    };
    
    enum ImageMode {
      MonoImageMode = 0,
      StereoImageMode,
      CubicImageMode,
      RedCyanImageMode,
      RedBlueImageMode,
      CrosseyeImageMode,
      ImageMode3DTV
    };

    enum CropType {
      _SphereType = 0,
      _BoxType,
      _TubeType
    };

    enum RemoveLayerType {
      _NoLayerType = 0,
      _RemoveTopLayerType,
      _RemoveCoreLayerType,
      _RemoveMiddleLayerType,
      _KeepMiddleLayerType
    };
};

#endif
