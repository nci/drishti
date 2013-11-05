#ifndef VIEWINFORMATION_H
#define VIEWINFORMATION_H

//#include "glewinitialisation.h"
#include <QGLViewer/qglviewer.h>
using namespace qglviewer;

#include "camerapathnode.h"
#include "clipinformation.h"
#include "lightinginformation.h"
#include "brickinformation.h"
#include "splinetransferfunction.h"
#include "captionobject.h"
#include "pathobject.h"
#include "pathgroupobject.h"
#include "pathobject.h"
#include "trisetinformation.h"
#include "networkinformation.h"

#include <fstream>
using namespace std;

class ViewInformation
{
 public :
  ViewInformation();
  ViewInformation(const ViewInformation&);
  ~ViewInformation();
  ViewInformation& operator=(const ViewInformation&);

  void load(fstream&);
  void save(fstream&);

  void setStepsizeStill(float);
  void setStepsizeDrag(float);
  void setRenderQuality(int);
  void setDrawBox(bool);
  void setDrawAxis(bool);
  void setBackgroundColor(Vec);
  void setBackgroundImageFile(QString);
  void setFocusDistance(float);
  void setVolumeNumber(int);
  void setVolumeNumber2(int);
  void setVolumeNumber3(int);
  void setVolumeNumber4(int);
  void setPosition(Vec);
  void setOrientation(Quaternion);
  void setLightInfo(LightingInformation);
  void setClipInfo(ClipInformation);
  void setBrickInfo(QList<BrickInformation>);
  void setSplineInfo(QList<SplineInformation>);
  void setVolumeBounds(Vec, Vec);
  void setImage(QImage);
  void setTick(int, int, QString, QString, QString);
  void setCaptions(QList<CaptionObject>);
  void setPoints(QList<Vec>);
  void setPaths(QList<PathObject>);
  void setPathGroups(QList<PathGroupObject>);
  void setTrisets(QList<TrisetInformation>);
  void setNetworks(QList<NetworkInformation>);
  void setTagColors(uchar*);


  float stepsizeStill();
  float stepsizeDrag();
  int renderQuality();
  bool drawBox();
  bool drawAxis();
  Vec backgroundColor();
  QString backgroundImageFile();
  float focusDistance();
  int volumeNumber();
  int volumeNumber2();
  int volumeNumber3();
  int volumeNumber4();
  Vec position();
  Quaternion orientation();
  LightingInformation lightInfo();
  ClipInformation clipInfo();
  QList<BrickInformation> brickInfo();
  QList<SplineInformation> splineInfo();
  void volumeBounds(Vec&, Vec&);
  QImage image();
  void getTick(int&, int&, QString&, QString&, QString&);
  QList<CaptionObject> captions();
  QList<Vec> points();
  QList<PathObject> paths();
  QList<PathGroupObject> pathgroups();
  QList<TrisetInformation> trisets();
  QList<NetworkInformation> networks();
  uchar* tagColors();


 private :
  int m_renderQuality;
  bool m_drawBox;
  bool m_drawAxis;
  Vec m_backgroundColor;
  QString m_backgroundImageFile;
  float m_stepsizeStill;
  float m_stepsizeDrag;
  float m_focusDistance;
  int m_volumeNumber;
  int m_volumeNumber2;
  int m_volumeNumber3;
  int m_volumeNumber4;
  Vec m_position;
  Quaternion m_rotation;
  LightingInformation m_lightInfo;
  ClipInformation m_clipInfo;
  QList<BrickInformation> m_brickInfo;
  Vec m_volMin, m_volMax;
  QImage m_image;
  QList<SplineInformation> m_splineInfo;
  int m_tickSize, m_tickStep;
  QString m_labelX, m_labelY, m_labelZ;
  QList<CaptionObject> m_captions;
  QList<Vec> m_points;
  QList<PathObject> m_paths;
  QList<PathGroupObject> m_pathgroups;
  QList<TrisetInformation> m_trisets;
  QList<NetworkInformation> m_networks;
  uchar *m_tagColors;
};

#endif
