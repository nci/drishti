#ifndef GILIGHTINFO_H
#define GILIGHTINFO_H


#include <QGLViewer/qglviewer.h>
using namespace qglviewer;

#include <fstream>
using namespace std;

#include "gilightobjectinfo.h"

class GiLightInfo
{
 public :
  GiLightInfo();
  ~GiLightInfo();

  void clear();

  GiLightInfo& operator=(const GiLightInfo&);

  static GiLightInfo interpolate(const GiLightInfo,
				 const GiLightInfo,
				 float);

  void load(fstream&);
  void save(fstream&);

  QList<GiLightObjectInfo> gloInfo;

  bool basicLight;
  bool applyClip;
  bool applyCrop;
  bool onlyAOLight;
  int lightLod;
  int lightDiffuse;

  Vec aoLightColor;
  int aoRad, aoTimes;
  float aoFrac, aoDensity1, aoDensity2;
  
  int opacityTF;

  int emisTF;
  float emisDecay;
  float emisBoost;
  int emisTimes;
};

#endif
