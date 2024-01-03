#ifndef KEYFRAMEINFORMATION_H
#define KEYFRAMEINFORMATION_H

#include <GL/glew.h>
#include <QObject>
#include <QGLViewer/qglviewer.h>
using namespace qglviewer;

#include <fstream>
using namespace std;

#include "camerapathnode.h"
#include "clipinformation.h"
#include "lightinginformation.h"
#include "brickinformation.h"
#include "captionobject.h"
#include "scalebarobject.h"
#include "pathobject.h"
#include "trisetinformation.h"

class KeyFrameInformation
{
 public :
  KeyFrameInformation();
  KeyFrameInformation(const KeyFrameInformation&);
  ~KeyFrameInformation();

  KeyFrameInformation& operator=(const KeyFrameInformation&);

  void clear();

  void load(fstream&);
  void save(fstream&);

  void setTitle(QString);
  void setDrawBox(bool);
  void setDrawAxis(bool);
  void setShadowBox(bool);
  void setBackgroundColor(Vec);
  void setBackgroundImageFile(QString);
  void setFrameNumber(int);
  void setFocusDistance(float, float);
  void setVolumeInterpolation(float);
  void setVolumeNumber(int);
  void setVolumeNumber2(int);
  void setVolumeNumber3(int);
  void setVolumeNumber4(int);
  void setPosition(Vec);
  void setOrientation(Quaternion);
  void setLut(uchar*);
  void setLightInfo(LightingInformation);
  void setClipInfo(ClipInformation);
  void setBrickInfo(QList<BrickInformation>);
  void setVolumeBounds(Vec, Vec);
  void setImage(QImage);
  void setTick(int, int, QString, QString, QString);
  void setMix(int, bool, bool, bool);
  void setCaptions(QList<CaptionObject>);
  void setScaleBars(QList<ScaleBarObject>);
  void setPoints(QList<Vec>, QList<Vec>, int, Vec);
  void setPaths(QList<PathObject>);
  void setTrisets(QList<TrisetInformation>);
  void setTrisetsColors(QList<TrisetInformation>);
  void setTrisetsLabels(QList<TrisetInformation>);
  void setTagColors(uchar*);
  void setPruneBuffer(QByteArray);
  void setPruneBlend(bool);
  void setOpMod(float, float);
  void setDOF(int, float);
  void setGamma(float);
  
  QString title();
  bool hasCaption(QStringList);
  bool drawBox();
  bool drawAxis();
  bool shadowBox();
  Vec backgroundColor();
  QString backgroundImageFile();
  int frameNumber();
  Vec position();
  Quaternion orientation();
  LightingInformation lightInfo();
  ClipInformation clipInfo();
  QList<BrickInformation> brickInfo();
  QImage image();
  QList<CaptionObject> captions();
  QList<ScaleBarObject> scalebars();
  QList<Vec> points();
  QList<Vec> barepoints();
  int pointSize();
  Vec pointColor();
  QList<PathObject> paths();
  QList<TrisetInformation> trisets();
  float gamma();
  

  // -- keyframe interpolation parameters
  void setInterpBGColor(int);
  void setInterpCaptions(int);
  void setInterpCameraPosition(int);
  void setInterpCameraOrientation(int);
  void setInterpBrickInfo(int);
  void setInterpClipInfo(int);
  void setInterpLightInfo(int);

  int interpBGColor();
  int interpCaptions();
  int interpCameraPosition();
  int interpCameraOrientation();
  int interpBrickInfo();
  int interpClipInfo();
  int interpLightInfo();

 private :
  QString m_title;
  int m_frameNumber;
  Vec m_position;
  Quaternion m_rotation;
  LightingInformation m_lightInfo;
  ClipInformation m_clipInfo;
  QList<BrickInformation> m_brickInfo;
  QImage m_image;
  bool m_shadowBox;
  Vec m_backgroundColor;
  QString m_backgroundImageFile;
  bool m_drawBox, m_drawAxis;
  QList<CaptionObject> m_captions;
  QList<ScaleBarObject> m_scalebars;
  QList<Vec> m_points;
  QList<Vec> m_barepoints;
  int m_pointSize;
  Vec m_pointColor;
  QList<PathObject> m_paths;
  QList<TrisetInformation> m_trisets;
  float m_gamma;
  
  //-- keyframe interpolation parameters
  int m_interpBGColor;
  int m_interpCaptions;
  int m_interpCameraPosition;
  int m_interpCameraOrientation;
  int m_interpBrickInfo;
  int m_interpClipInfo;
  int m_interpLightInfo;
};

#endif
