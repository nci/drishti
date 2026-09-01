#ifndef ZARRPLUGIN_H
#define ZARRPLUGIN_H

#include <QObject>
#include <QStringList>
#include <QVector>
#include <QMap>
#include "volinterface.h"

// Zarr (v3) directory reader plugin.
//
// Reads a multiscale zarr v3 directory (as written by toZarrStreaming /
// toZarrStreaming.cpp / zarrwriter.cpp, i.e. a group whose metadata describes
// arrays named by pyramid level "0", "1", ...).  On setFile() the available
// levels are listed and the user picks one; the chosen level's shape/type/
// chunk metadata is parsed and its voxels are served on demand (per
// depth/width/height slice or single raw value), decompressing per-chunk as
// needed, so the whole volume is never held in memory.
//
// Supports unsigned 8-bit (uint8) and unsigned 16-bit (uint16) voxels.
class ZarrPlugin : public QObject, public VolInterface
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "drishti.import.Plugin.VolInterface/1.0")
  Q_INTERFACES(VolInterface)

 public :
  QStringList registerPlugin();
  void init();
  void clear();

  void setValue(QString, float) {}
  void setValue(QString, QString) {}

  bool setFile(QStringList);
  void replaceFile(QString);

  void set4DVolume(bool) {}

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

  void generateHistogram();

  void getDepthSlice(int, uchar*);
  void getWidthSlice(int, uchar*);
  void getHeightSlice(int, uchar*);

  QVariant rawValue(int, int, int);

 private :
  bool parseRoot();                  // parse <dir>/zarr.json group metadata
  bool parseLevel();                 // parse <dir>/<level>/zarr.json array meta
  bool readChunk(qint64 kz, qint64 ky, qint64 kx, QByteArray& out) const;
                                   // decompress one full (padded) chunk block

  QString m_dir;                     // zarr directory
  QString m_level;                   // chosen pyramid level ("0","1",...)
  QString m_description;

  float m_voxelSizeX, m_voxelSizeY, m_voxelSizeZ;
  int m_depth, m_width, m_height;    // (Z, Y, X) of the chosen level
  int m_voxelType;                   // _UChar or _UShort
  int m_voxelUnit;
  int m_headerBytes;
  int m_bytesPerVoxel;               // 1 (uint8) or 2 (uint16)

  float m_rawMin, m_rawMax;
  bool m_haveDataMinMax;             // data_min_max present in the metadata
  QList<uint> m_histogram;

  QMap<QString, QVector<float> > m_levelScales;  // level -> scale transform

  qint64 m_chunkZ, m_chunkY, m_chunkX;  // chunk shape of the chosen level
  QList<QString> m_levels;              // pyramid level paths (e.g. "0","1")

  // zarr v3 sharding (sharding_indexed codec) parameters.  When enabled the
  // chunk files are shard containers: inner chunks packed concentively
  // followed/preceded by an index of (offset,nbytes) pairs.
  bool m_sharded;
  qint64 m_innerZ, m_innerY, m_innerX;  // inner (shard) chunk shape
  bool m_indexCrc;                      // index has a trailing crc32c
  int  m_indexLocation;                 // 0 = end, 1 = start

  bool m_haveRoot;                  // root zarr.json parsed ok
};

#endif
