#ifndef VOLUMEDATA_H
#define VOLUMEDATA_H

#include "scriptsplugin.h"
#include "volinterface.h"

class VolumeData : public QObject
{
  Q_OBJECT

 public :
  VolumeData();
  ~VolumeData();

  bool setFile(QStringList, QString);
  bool setFile(QStringList, QString, bool, bool);
  bool replaceFile(QString);

  void setVoxelInfo(int, float, float, float);

  void gridSize(int&, int&, int&);
  void voxelSize(float&, float&, float&);
  QString description();
  int voxelUnit();
  int voxelType();
  int headerBytes();
  int bytesPerVoxel();

  QList<uint> histogram();
  
  bool setMinMax(float, float);
  float rawMin();
  float rawMax();
   
  void setMap(QList<float>, QList<int>);

  int m_pvlMapMax;
  QList<float> rawMap();
  QList<int> pvlMap();

  bool getDepthSlice(int, uchar*);
  QString lastError() const;
  bool lastOperationCanceled() const;

  QImage getDepthSliceImage(int);
  QImage getWidthSliceImage(int);
  QImage getHeightSliceImage(int);

  QPair<QVariant,QVariant> rawValue(int, int, int);

  bool saveTrimmed(QString, int, int, int, int, int, int);

 private :
  VolInterface *m_volInterface;

  bool m_scriptsPluginActive;
  ScriptsPlugin m_scriptsPlugin;

  QStringList m_fileName;
  int m_depth, m_width, m_height;
  int m_voxelUnit;
  int m_voxelType;
  int m_headerBytes;
  float m_voxelSizeX;
  float m_voxelSizeY;
  float m_voxelSizeZ;
  QString m_description;
  
  float m_rawMin, m_rawMax;
  QList<uint> m_histogram;

  QList<float> m_rawMap;
  QList<int> m_pvlMap;
   
  unsigned char *m_image;

  int m_skipBytes;
  int m_bytesPerVoxel;
  QString m_lastError;
  bool m_lastOperationCanceled;
    
  void clear();
  bool loadPlugin(QString);

  void printVolumeInfo();
};

#endif
