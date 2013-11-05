#ifndef ITKBINARYTHINNINGPLUGIN_H
#define ITKBINARYTHINNINGPLUGIN_H

#include <QObject>
#include "plugininterface.h"

class BinaryThinning : public QObject, RenderPluginInterface
{
 Q_OBJECT
   Q_INTERFACES(RenderPluginInterface)

 public :

  QStringList registerPlugin();

  void setPvlFileManager(VolumeFileManager*);
  void setLodFileManager(VolumeFileManager*);
  void setClipInfo(QList<Vec>, QList<Vec>);
  void setCropInfo(QList<CropObject>);
  void setPathInfo(QList<PathObject>);
  void setLookupTable(int, QImage);
  void setSamplingLevel(int);
  void setDataLimits(Vec, Vec);
  void setVoxelScaling(Vec);
  void setPreviousDirectory(QString);
  void setPruneData(int, int, int, int, QVector<uchar>);
  void setTagColors(QVector<uchar> t) { m_tagColors = t; }

  void init();
  void start();

 private :
  VolumeFileManager *m_pvlFileManager;
  VolumeFileManager *m_lodFileManager;
  Vec m_dataMin, m_dataMax;
  Vec m_voxelScaling;
  QList<Vec> m_clipPos;
  QList<Vec> m_clipNormal;
  QList<CropObject> m_crops;
  QList<PathObject> m_paths;
  int m_lutSize;
  QImage m_lutImage;
  int m_samplingLevel;
  QString m_previousDirectory;
  int m_pruneLod, m_pruneX, m_pruneY, m_pruneZ;
  QVector<uchar> m_pruneData;
  QVector<uchar> m_tagColors;
};

#endif
