#ifndef TIFFPLUGIN_H
#define TIFFPLUGIN_H

#include <atomic>

#include <QObject>
#include <QVector>
#include "volinterface.h"
#include "../../sourcefilesprovider.h"

class TiffPlugin : public QObject, VolInterface, SourceFilesProvider
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "drishti.import.Plugin.VolInterface/1.0")
  Q_INTERFACES(VolInterface SourceFilesProvider)

 public :
  QStringList registerPlugin();

  void init();
  void clear();

  void setValue(QString, float) {};

  bool setFile(QStringList);
  QStringList sourceFiles() const;
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
  void getWidthSlice(int, uchar*);
  void getHeightSlice(int, uchar*);

  QVariant rawValue(int, int, int);

  //void saveTrimmed(QString, int, int, int, int, int, int);

  void generateHistogram();
  void set4DVolume(bool);
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

  int m_bytesPerVoxel;

  quint64 m_sliceBytes;

  QList<QString> m_imageList;
  QVector<quint32> m_directoryList;
  QVector<quint64> m_scanlineBytes;

  std::atomic_bool m_cancelRequested;
  std::atomic_int m_progressValue;
  std::atomic_bool m_previewReadActive{false};
  QString m_lastError;
  bool m_lastOperationCanceled;

  struct StatisticsResult
  {
    bool success;
    bool canceled;
    QString error;
    float minimum;
    float maximum;
    QVector<quint64> histogram;

    StatisticsResult();
  };

  bool setImageFiles(const QStringList&, QString*);
  bool generateStatistics(QString*);
  StatisticsResult calculateStatistics();

  bool loadTiffImage(int, uchar*, quint64, QString*,
                     const std::atomic_bool* = 0) const;
  bool loadTiffRow(int, int, QByteArray*, QString*,
                   const std::atomic_bool* = 0) const;
};

#endif
