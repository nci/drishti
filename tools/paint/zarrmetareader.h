#ifndef ZARRMETAREADER_H
#define ZARRMETAREADER_H

#include <QString>
#include <QList>
#include <QStringList>
#include <QMap>
#include <QVector>

struct ZarrVolumeInfo
{
  bool valid = false;
  int voxelUnit = 0;
  int voxelType = 0;
  float voxelSizeX = 1.0f;
  float voxelSizeY = 1.0f;
  float voxelSizeZ = 1.0f;
  QString description;
  QString level = "0";
  QList<QString> m_levels;
  QMap<QString, QVector<float> > m_levelScales;  // level -> scale transform
  int depth = 0;
  int width = 0;
  int height = 0;
};

class ZarrMetaReader
{
 public:
  static ZarrVolumeInfo getInfo(const QString& zarrDir);
  static ZarrVolumeInfo zarrInfo;
};

#endif
