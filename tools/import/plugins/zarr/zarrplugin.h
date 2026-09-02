#ifndef ZARRPLUGIN_H
#define ZARRPLUGIN_H

#include <QObject>
#include <QStringList>
#include <QVector>
#include <QMap>
#include <memory>
#include "volinterface.h"

#include <libzarr/libzarr.hpp>
#include <libzarr/adapters/filesystem_store.hpp>

// Zarr (v2/v3) directory reader plugin.
//
// Reads a multiscale zarr directory, a group whose metadata describes
// arrays named by pyramid level "0", "1", ...).  On setFile() the available
// levels are listed and the user picks one; the chosen level's shape/type/
// chunk metadata is read through libzarr and its voxels are served on demand
// (per depth/width/height slice or single raw value), so the whole volume is
// never held in memory.
//
// All zarr v2/v3 decoding — compression (blosc/gzip/zstd), sharding,
// byte order and block layout — is handled by the libzarr library
// (https://github.com/kharchenkolab/libzarr); this plugin only maps slices
// onto libzarr reads.
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
  bool parseRoot();                  // open <dir> group; read attrs + levels
  bool parseLevel();                 // open <dir>/<level> array; read metadata
  bool readChunk(int kz, int ky, int kx, QByteArray& out) const;
                       // decompress one full (fill-padded) block
  void readSliceRegion(std::vector<uint64_t> origin, std::vector<uint64_t> shape,
                       uchar* slice) const;  // hyperslab via libzarr

  QString m_dir;                     // zarr directory
  QString m_level;                   // chosen pyramid level ("0","1",...)
  QString m_description;

  std::shared_ptr<zarr::FilesystemStore> m_store;  // store bound to m_dir
  std::shared_ptr<zarr::Group> m_root;             // root group
  std::shared_ptr<zarr::Array> m_array;            // chosen level's array

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

  std::vector<uint64_t> m_chunkShape;          // chunk shape of chosen level
  QList<QString> m_levels;                     // pyramid level paths ("0","1")

  bool m_haveRoot;                  // root group opened ok
};

#endif
