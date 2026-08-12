#ifndef NRRDPLUGIN_H
#define NRRDPLUGIN_H

#include <QObject>

#include <memory>

#include "volinterface.h"

class NrrdPlugin : public QObject, VolInterface
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "drishti.import.Plugin.VolInterface/1.0")
  Q_INTERFACES(VolInterface)

 public:
  NrrdPlugin();
  ~NrrdPlugin() override;

  QStringList registerPlugin();

  void init();
  void clear();

  void setValue(QString, float) {}
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
  QVariant rawValue(int, int, int);

  void generateHistogram();

  Q_INVOKABLE QString lastError() const;
  Q_INVOKABLE bool wasCanceled() const;

 private:
  QStringList m_fileName;
  bool m_4dvol;
  int m_depth;
  int m_width;
  int m_height;
  int m_voxelUnit;
  int m_voxelType;
  int m_headerBytes;
  float m_voxelSizeX;
  float m_voxelSizeY;
  float m_voxelSizeZ;
  QString m_description;

  float m_rawMin;
  float m_rawMax;
  QList<uint> m_histogram;

  int m_skipBytes;
  int m_bytesPerVoxel;

  std::shared_ptr<uchar> m_entireVolume;
  QString m_lastError;
  bool m_lastOperationCanceled;
};

#endif
