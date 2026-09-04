#ifndef ZARRHANDLER_H
#define ZARRHANDLER_H

#include <QObject>
#include <QStringList>
#include <QVector>
#include <QMap>
#include <memory>

// Qt's <QtGui> pulls in <windows.h> -> minwindef.h, which #defines the
// lowercase keyword-like macro `far` (empty, and `FAR` as `far`).  libzarr's
// zip.hpp uses `far` as a member name (PackEntry::far), so we must undo the
// Windows macros before pulling in libzarr, or zip.hpp fails to parse:
//   error C2059: syntax error: '='   (zip.hpp:115  "bool far = false;")
//   error C2039: 'far'/'e' is not a member of 'PackEntry'   (cascade)
// We also keep `FAR` as an empty macro because zlib's zconf.h (used by
// libzarr's gzip codec) typedefs `Byte FAR Bytef` etc. via `FAR`.
#ifdef far
#undef far
#endif
#ifdef FAR
#undef FAR
#endif
#ifndef FAR
#define FAR
#endif

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
class ZarrHandler
{
 public :
  void init();
  void clear();

  void setValue(QString, float) {}
  void setValue(QString, QString) {}

  bool setFile(QString, QString level="0");

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

  // store a slice (depth = (Y,X) plane, width = (Z,X) plane, height = (Z,Y)
  // plane) back into the currently open array.
  void setDepthSlice(int, uchar*);
  void setWidthSlice(int, uchar*);
  void setHeightSlice(int, uchar*);

  // read an axis-aligned block given inclusive start (d0,w0,h0) and inclusive
  // end (d1,w1,h1) indices; `data` must hold
  // (d1-d0+1)*(w1-w0+1)*(h1-h0+1)*bytesPerVoxel bytes (Z, Y, X order).
  void getRegion(int d0, int w0, int h0, int d1, int w1, int h1, uchar* data);

  QVariant rawValue(int, int, int);

 private :
  bool parseRoot();                  // open <dir> group; read attrs + levels
  bool parseLevel();                 // open <dir>/<level> array; read metadata
  bool readChunk(int kz, int ky, int kx, QByteArray& out) const;
                       // decompress one full (fill-padded) block
  void readSliceRegion(std::vector<uint64_t> origin, std::vector<uint64_t> shape,
                       uchar* slice) const;  // hyperslab via libzarr
  void writeSliceRegion(std::vector<uint64_t> origin, std::vector<uint64_t> shape,
                        const uchar* slice) const;  // store hyperslab via libzarr

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
