#ifndef TXMPLUGIN_H
#define TXMPLUGIN_H

#include <QObject>
#include "volinterface.h"

#include "pole.h"

class TxmPlugin : public QObject, VolInterface
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "drishti.import.Plugin.VolInterface/1.0")
  Q_INTERFACES(VolInterface)

 public :
  ~TxmPlugin() override;

  QStringList registerPlugin();

  void init();
  void clear();

  void setValue(QString, float) {};

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
  Q_INVOKABLE QString lastError() const;
  Q_INVOKABLE bool wasCanceled() const;
  //void getWidthSlice(int, uchar*);
  //void getHeightSlice(int, uchar*);

  QVariant rawValue(int, int, int);

  //void saveTrimmed(QString, int, int, int, int, int, int);

  void generateHistogram();
  void set4DVolume(bool);
 private :
  POLE::Storage *m_storage = 0;

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

  QStringList m_imageData;

  int m_bytesPerVoxel;

  int m_dirCount;
  QString m_lastError;
  bool m_lastOperationCanceled;

  void findMinMaxandGenerateHistogram();
  void findMinMax();

  bool loadFile(const QString&, bool, bool);
  bool loadTxmImage(int, uchar*);
};

#endif
