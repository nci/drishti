#ifndef KEYFRAME_H
#define KEYFRAME_H

#include "keyframeinformation.h"

class KeyFrame : public QObject
{
  Q_OBJECT

 public :
  KeyFrame();
  ~KeyFrame();

  void clear();

  void load(fstream&);
  void import(QString);

  void save(fstream&);

  void draw(float);

  KeyFrameInformation interpolate(int);

  KeyFrameInformation keyFrameInfo(int);

  int searchCaption(QStringList);

  void saveProject(Vec, Quaternion,
		   float, float,
		   int, int, int, int,
		   unsigned char*,
		   LightingInformation,
		   QList<BrickInformation>,
		   Vec, Vec,
		   QImage,
		   int, int, QString, QString, QString,
		   int, bool, bool, bool,
		   QByteArray);

  void setKeyFrame(Vec, Quaternion,
		   float, float,
		   int, unsigned char*,
		   LightingInformation,
		   QList<BrickInformation>,
		   Vec, Vec,
		   QImage,
		   QList<SplineInformation>,
		   int, int, QString, QString, QString,
		   int, bool, bool, bool);

  void interpolateAt(int, float,
		     Vec&, Quaternion&,
		     KeyFrameInformation&,
		     float&);

  int numberOfKeyFrames();

 public slots :
  void playFrameNumber(int);
  void updateKeyFrameInterpolator();
  void removeKeyFrame(int);
  void removeKeyFrames(int, int);
  void setKeyFrameNumber(int, int);
  void setKeyFrameNumbers(QList<int>);
  void reorder(QList<int>);
  void copyFrame(int);
  void pasteFrame(int);
  void pasteFrameOnTop(int);
  void pasteFrameOnTop(int, int);
  void editFrameInterpolation(int);
  void replaceKeyFrameImage(int, QImage);
  void playSavedKeyFrame();

 signals :
  void updateParameters(bool, bool, Vec, QString,
			int, int, QString, QString, QString,
			int, bool, bool, float, bool, bool);
  void loadKeyframes(QList<int>, QList<QImage>);
  void updateVolInfo(int);
  void updateVolInfo(int, int);
  void updateFocus(float, float);
  void updateLookFrom(Vec, Quaternion, float, float);
  void updateTagColors();
  void updateLookupTable(unsigned char*);
  void updateLightInfo(LightingInformation);
  void updateBrickInfo(QList<BrickInformation>);
  void updateVolumeBounds(Vec, Vec);
  void updateVolumeBounds(int, Vec, Vec);
  void updateVolumeBounds(int, int, Vec, Vec);
  void updateVolumeBounds(int, int, int, Vec, Vec);
  void updateVolumeBounds(int, int, int, int, Vec, Vec);
  void updateGL();
  void setImage(int, QImage);
  void currentFrameChanged(int);
  void updateTransferFunctionManager(QList<SplineInformation>);
  void updateMorph(bool);
  void replaceKeyFrameImage(int);
  void addKeyFrameNumbers(QList<int>);

 private :
  QList<CameraPathNode*> m_cameraList;
  QList<KeyFrameInformation*> m_keyFrameInfo;

  KeyFrameInformation m_savedKeyFrame;
  KeyFrameInformation m_copyKeyFrame;

  QList<Vec> m_tgP;
  QList<Quaternion> m_tgQ;
  void computeTangents();
  Vec interpolatePosition(int, int, float);
  Quaternion interpolateOrientation(int, int, float);

  void updateCameraPath();

  QMap<QString, QPair<QVariant, bool> > copyProperties(QString);
};

#endif
