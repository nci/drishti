#ifndef NRRDPLUGIN_H
#define NRRDPLUGIN_H

#include <QObject>
#include "volinterface.h"

class NrrdPlugin : public QObject, VolInterface
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "drishti.import.Plugin.VolInterface/1.0")
  Q_INTERFACES(VolInterface)

 public :
  QStringList registerPlugin();

  void init();
  void clear();

  void setValue(QString, float) {};

  void set4DVolume(bool);

  bool setFile(QStringList);
  void replaceFile(QString);

  void gridSize(int&, int&, int&);
  void voxelSize(float&, float&, float&);
  QString description();
  int voxelUnit();
  int voxelType();
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
 private :
  QStringList m_fileName;
  bool m_4dvol;
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

  int m_skipBytes;
  int m_bytesPerVoxel;

  void findMinMax();
  void findMinMaxandGenerateHistogram();

  template <class T> void readSlice(int[3], int[3], int, uchar*);


  uchar *m_entireVolume;
  template <class T> void readEntireVolume();
  
};

#endif
