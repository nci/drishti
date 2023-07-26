#ifndef HDF4PLUGIN_H
#define HDF4PLUGIN_H

#include <QObject>
#include "volinterface.h"

class HDF4Plugin : public QObject, VolInterface
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "drishti.import.Plugin.VolInterface/1.0")
  Q_INTERFACES(VolInterface)

 public :
  QStringList registerPlugin();

  void init();
  void clear();

  void setValue(QString, float) {};

  bool setFile(QStringList);
  void replaceFile(QString);

  void gridSize(int&, int&, int&);
  void voxelSize(float&, float&, float&);
  QString description();
  int voxelType();
  int voxelUnit();
  int headerBytes();

  QList<uint> histogram();
  
  void setMinMax(float, float);
  float rawMin();
  float rawMax();
   
  void getDepthSlice(int, uchar*);
  //void getWidthSlice(int, uchar*);
  //void getHeightSlice(int, uchar*);

  QVariant rawValue(int, int, int);

  //void saveTrimmed(QString, int, int, int, int, int, int);

  void generateHistogram();
  void set4DVolume(bool);
 private :
  QStringList m_fileName;
  bool m_4dvol;
  int m_depth, m_width, m_height;
  int m_voxelType;
  int m_voxelUnit;
  int m_headerBytes;
  float m_voxelSizeX;
  float m_voxelSizeY;
  float m_voxelSizeZ;
  QString m_description;
  
  float m_rawMin, m_rawMax;
  QList<uint> m_histogram;

  int m_skipBytes;
  int m_bytesPerVoxel;

  int m_Index;
  QList<QString> m_imageList;

  QList<QString> listAllVariables();
  void findMinMax();
  void findMinMaxandGenerateHistogram();

  bool setImageFiles(QStringList);
};

#endif
