#include <QtGui>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <vector>
#include "common.h"
#include "zarrplugin.h"

#include "blosc.h"

using namespace std;

QStringList
ZarrPlugin::registerPlugin()
{
  QStringList regString;
  regString << "directory";
  regString << "Zarr Directory";
  //regString << "files";
  //regString << "Zarr Files";

  return regString;
}

void
ZarrPlugin::init()
{
  m_dir.clear();
  m_level.clear();
  m_description.clear();

  m_depth = m_width = m_height = 0;
  m_voxelType = _UChar;
  m_voxelUnit = _Micron;
  m_voxelSizeX = m_voxelSizeY = m_voxelSizeZ = 1;
  m_headerBytes = 0;
  m_bytesPerVoxel = 1;

  m_rawMin = m_rawMax = 0;
  m_haveDataMinMax = false;
  m_histogram.clear();
  m_levelScales.clear();

  m_chunkZ = m_chunkY = m_chunkX = 0;
  m_levels.clear();

  m_sharded = false;
  m_innerZ = m_innerY = m_innerX = 0;
  m_indexCrc = false;
  m_indexLocation = 0;

  m_haveRoot = false;
}

void
ZarrPlugin::clear()
{
  init();
}

void
ZarrPlugin::replaceFile(QString flnm)
{
  m_dir = flnm;
}

void
ZarrPlugin::gridSize(int& d, int& w, int& h)
{
  d = m_depth;
  w = m_width;
  h = m_height;
}

void
ZarrPlugin::voxelSize(float& vx, float& vy, float& vz)
{
  vx = m_voxelSizeX;
  vy = m_voxelSizeY;
  vz = m_voxelSizeZ;
}

QString ZarrPlugin::description() { return m_description; }
int ZarrPlugin::voxelUnit() { return m_voxelUnit; }
int ZarrPlugin::voxelType() { return m_voxelType; }
int ZarrPlugin::headerBytes() { return m_headerBytes; }

QList<uint> ZarrPlugin::histogram() { return m_histogram; }

void
ZarrPlugin::setMinMax(float rmin, float rmax)
{
  m_rawMin = rmin;
  m_rawMax = rmax;

  if (m_voxelType == _UChar || m_voxelType == _UShort)
    return;
  generateHistogram();
}

float ZarrPlugin::rawMin() { return m_rawMin; }
float ZarrPlugin::rawMax() { return m_rawMax; }

// ---------------------------------------------------------------------
bool
ZarrPlugin::setFile(QStringList files)
{
  if (files.size() == 0)
    return false;

  QFileInfo f(files[0]);
  if (f.isDir())
    m_dir = f.absoluteFilePath();
  else
    m_dir = f.absolutePath();

  if (!parseRoot())
    {
      QMessageBox::information(0, "Error",
                               QString("%1 is not a valid Zarr directory").
                               arg(m_dir));
      return false;
    }

  if (m_levels.size() == 0)
    {
      QMessageBox::information(0, "Error",
                               "No pyramid levels found in Zarr directory");
      return false;
    }

  if (m_levels.size() == 1)
    {
      m_level = m_levels[0];
    }
  else
    {
      // override for headless testing / scripts
      QByteArray forced = qgetenv("ZARR_FORCE_LEVEL");
      QString tf = QString::fromLocal8Bit(forced).trimmed();
      if (forced.size() && m_levels.contains(tf))
        {
          m_level = tf;
        }
      else
        {
          bool ok;
          QString lv = QInputDialog::getItem(0,
                                             "Choose a pyramid level",
                                             "Levels",
                                             m_levels,
                                             0,
                                             false,
                                             &ok);
          m_level = ok ? lv : m_levels[0];
        }
    }

  if (!parseLevel())
    {
      QMessageBox::information(0, "Error",
                               QString("%1 level %2 has no valid array "
                                       "metadata").arg(m_dir).arg(m_level));
      return false;
    }

  // apply the chosen level's scale transform to the base voxel size
  // (pyramid levels are scaled down from level 0).
  QVector<float> sc = m_levelScales.value(m_level);
  if (sc.size() >= 3)
    {
      if (sc[0] > 0) m_voxelSizeX *= sc[0];
      if (sc[1] > 0) m_voxelSizeY *= sc[1];
      if (sc[2] > 0) m_voxelSizeZ *= sc[2];
    }

  generateHistogram();

  return true;
}

// ---------------------------------------------------------------------
// Read <dir>/zarr.json and harvest the available pyramid levels
// (multiscales[0].datasets[].path) plus the drishti attributes.
bool
ZarrPlugin::parseRoot()
{
  m_haveRoot = false;
  m_levels.clear();

  QFile f(m_dir + "/zarr.json");
  if (!f.open(QIODevice::ReadOnly))
    return false;
  QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
  f.close();
  if (!doc.isObject())
    return false;

  QJsonObject root = doc.object();
  QJsonObject attr = root.value("attributes").toObject();

  // optional drishti/mango attributes (description / data_min_max /
  // voxel_unit / voxel_size_xyz).  Prefer "drishti", fall back to "mango"
  // (toZarrStreaming / toZarrStreaming.cpp writes "mango").
  QJsonObject meta = attr.value("drishti").toObject();
  if (meta.isEmpty())
    meta = attr.value("mango").toObject();
  if (meta.contains("description"))
    m_description = meta.value("description").toString();
  QJsonValue voxelUnit = meta.value("voxel_unit");
  if (voxelUnit.isString())
    {
      QString vu = voxelUnit.toString().toLower();
      if (vu == "mm" || vu == "millimeter") m_voxelUnit = _Millimeter;
      else if (vu == "micron" || vu == "um" || vu == "mu") m_voxelUnit = _Micron;
      else if (vu == "angstrom") m_voxelUnit = _Angstrom;
      else if (vu == "nanometer") m_voxelUnit = _Nanometer;
      else if (vu == "centimeter") m_voxelUnit = _Centimeter;
      else if (vu == "meter") m_voxelUnit = _Meter;
    }

  // voxel_size_xyz : [vx, vy, vz]
  QJsonArray vsz = meta.value("voxel_size_xyz").toArray();
  if (vsz.size() >= 3)
    {
      m_voxelSizeX = (float)vsz[0].toDouble();
      m_voxelSizeY = (float)vsz[1].toDouble();
      m_voxelSizeZ = (float)vsz[2].toDouble();
    }

  // data_min_max : [min, max]
  QJsonArray dmm = meta.value("data_min_max").toArray();
  if (dmm.size() >= 2)
    {
      m_rawMin = (float)dmm[0].toDouble();
      m_rawMax = (float)dmm[1].toDouble();
      m_haveDataMinMax = true;
    }

  // harvest level paths from multiscales[0].datasets[].path and the
  // per-level scale transform.
  QJsonArray multiscales = attr.value("multiscales").toArray();
  if (multiscales.size() > 0)
    {
      QJsonArray datasets = multiscales[0].toObject()
                            .value("datasets").toArray();
      for (int i = 0; i < datasets.size(); ++i)
        {
          QJsonObject d = datasets[i].toObject();
          QString p = d.value("path").toString();
          if (p.isEmpty())
            continue;
          m_levels.append(p);
          QVector<float> sc(3, 1.0f);
          QJsonArray ct = d.value("coordinateTransformations").toArray();
          if (ct.size() > 0)
            {
              QJsonArray scale =
                ct[0].toObject().value("scale").toArray();
              if (scale.size() >= 3)
                {
                  sc[0] = (float)scale[0].toDouble();
                  sc[1] = (float)scale[1].toDouble();
                  sc[2] = (float)scale[2].toDouble();
                }
            }
          m_levelScales[p] = sc;
        }
    }

  m_haveRoot = true;
  return true;
}

// ---------------------------------------------------------------------
// Read <dir>/<level>/zarr.json array metadata: shape, data_type, chunks.
bool
ZarrPlugin::parseLevel()
{
  QFile f(m_dir + "/" + m_level + "/zarr.json");
  if (!f.open(QIODevice::ReadOnly))
    return false;
  QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
  f.close();
  if (!doc.isObject())
    return false;

  QJsonObject arr = doc.object();

  QJsonArray shape = arr.value("shape").toArray();
  if (shape.size() < 3)
    return false;
  m_depth = shape[0].toInt();  // Z
  m_width = shape[1].toInt();  // Y
  m_height = shape[2].toInt(); // X

  // data_type (v3) or dtype (v2 numpy-style)
  QString dt;
  if (arr.contains("data_type") && arr.value("data_type").isString())
    dt = arr.value("data_type").toString();
  else if (arr.contains("dtype") && arr.value("dtype").isString())
    dt = arr.value("dtype").toString();

  bool isU16 = false;
  QString d = dt.toLower();
  if (dt == "uint16" || d == "u2" || d == "|u2" || d == "<u2" || d == ">u2"
      || d == "=u2")
    isU16 = true;

  m_voxelType = isU16 ? _UShort : _UChar;
  m_bytesPerVoxel = isU16 ? 2 : 1;

  // chunk_grid.configuration.chunk_shape
  QJsonObject grid = arr.value("chunk_grid").toObject();
  QJsonObject gconf = grid.value("configuration").toObject();
  QJsonArray chunk = gconf.value("chunk_shape").toArray();
  m_chunkZ = m_chunkY = m_chunkX = 1;
  if (chunk.size() >= 3)
    {
      m_chunkZ = chunk[0].toInt();
      m_chunkY = chunk[1].toInt();
      m_chunkX = chunk[2].toInt();
    }
  if (m_chunkZ < 1 || m_chunkY < 1 || m_chunkX < 1)
    { m_chunkZ = m_depth; m_chunkY = m_width; m_chunkX = m_height; }

  // detect zarr v3 sharding_indexed codec and record shard parameters
  m_sharded = false;
  m_innerZ = m_innerY = m_innerX = 0;
  m_indexCrc = false;
  m_indexLocation = 0;
  QJsonArray codecs = arr.value("codecs").toArray();
  for (int i = 0; i < codecs.size(); ++i)
    {
      QJsonObject c = codecs[i].toObject();
      if (c.value("name").toString() != "sharding_indexed")
        continue;
      QJsonObject cfg = c.value("configuration").toObject();
      m_sharded = true;
      QJsonArray shp = cfg.value("chunk_shape").toArray();
      m_innerZ = shp.size() > 0 ? shp[0].toInt() : 1;
      m_innerY = shp.size() > 1 ? shp[1].toInt() : 1;
      m_innerX = shp.size() > 2 ? shp[2].toInt() : 1;
      if (cfg.value("index_location").toString() == "start")
        m_indexLocation = 1;
      QJsonArray ic = cfg.value("index_codecs").toArray();
      for (int j = 0; j < ic.size(); ++j)
        if (ic[j].toObject().value("name").toString() == "crc32c")
          m_indexCrc = true;
      break;
    }

  return true;
}

// ---------------------------------------------------------------------
// Decompress one full (padded to chunk shape) block into out.
// Chunk file location: <dir>/<level>/c/<kz>/<ky>/<kx>
bool
ZarrPlugin::readChunk(qint64 kz, qint64 ky, qint64 kx, QByteArray& out) const
{
  QString key = m_dir + "/" + m_level + "/c/" +
                QString::number(kz) + "/" + QString::number(ky) + "/" +
                QString::number(kx);

  QFile f(key);
  if (!f.open(QIODevice::ReadOnly))
    return false;
  QByteArray payload = f.readAll();
  f.close();

  const qint64 blockBytes = m_chunkZ * m_chunkY * m_chunkX * m_bytesPerVoxel;

  // ---- non-sharded: the chunk file is a single blosc frame ----
  if (!m_sharded)
    {
      out.clear();
      out.resize((int)blockBytes);
      int n = blosc_decompress(payload.constData(), out.data(),
                               (size_t)blockBytes);
      // tolerate truncated last chunk (some writers store boundary chunks
      // at their real size rather than the full chunk shape)
      if (n < 0)
        {
          qint64 rem = (m_depth - kz * m_chunkZ) * m_chunkY * m_chunkX
                       * m_bytesPerVoxel;
          if (rem > 0 && rem < blockBytes)
            {
              out.resize((int)rem);
              n = blosc_decompress(payload.constData(), out.data(),
                                   (size_t)rem);
            }
        }
      return n >= 0;
    }

  // ---- sharded (zarr v3 sharding_indexed) ----
  // The chunk file is a shard container: inner chunks tiled in row-major
  // C order, followed (index_location == end) or preceded (== start) by an
  // index of (offset,nbytes) uint64 pairs per inner chunk (+ optional crc32c).
  const qint64 nkIz = (m_chunkZ + m_innerZ - 1) / m_innerZ;
  const qint64 nkIy = (m_chunkY + m_innerY - 1) / m_innerY;
  const qint64 nkIx = (m_chunkX + m_innerX - 1) / m_innerX;
  const qint64 nInner = nkIz * nkIy * nkIx;
  const qint64 innerBytes = m_innerZ * m_innerY * m_innerX * m_bytesPerVoxel;
  const qint64 idxBytes = nInner * 16 + (m_indexCrc ? 4 : 0);

  if ((qint64)payload.size() < idxBytes)
    return false;

  const qint64 dataStart = (m_indexLocation == 1) ? idxBytes : 0;
  std::vector<qint64> idx(nInner * 2);
  {
    const int ib = (int)((m_indexLocation == 1) ? 0
                        : payload.size() - (int)idxBytes);
    const unsigned char* q = (const unsigned char*)payload.constData() + ib;
    for (qint64 i = 0; i < nInner * 2; ++i)
      idx[i] = (qint64)((quint64)q[i*8] | ((quint64)q[i*8+1] << 8)
                        | ((quint64)q[i*8+2] << 16) | ((quint64)q[i*8+3] << 24)
                        | ((quint64)q[i*8+4] << 32) | ((quint64)q[i*8+5] << 40)
                        | ((quint64)q[i*8+6] << 48) | ((quint64)q[i*8+7] << 56));
  }

  out.clear();
  out.resize((int)blockBytes);
  memset(out.data(), 0, (size_t)blockBytes);  // fill_value == 0 padding

  const char* src0 = payload.constData() + dataStart;
  for (qint64 i = 0; i < nInner; ++i)
    {
      const qint64 off = idx[i*2];
      const qint64 nb = idx[i*2 + 1];
      if (off < 0 || nb <= 0)      // empty inner chunk -> fill value
        continue;
      if (off > (qint64)payload.size() || nb > (qint64)payload.size() - off)
        continue;
      // grid position (iz,iy,ix) of this inner chunk within the shard
      const qint64 iz = i / (nkIy * nkIx);
      const qint64 r  = i % (nkIy * nkIx);
      const qint64 iy = r / nkIx;
      const qint64 ix = r % nkIx;
      const qint64 dstOff =
        ((iz * m_innerZ) * m_chunkY + (iy * m_innerY)) * m_chunkX
        + (ix * m_innerX);
      const qint64 dstByte = dstOff * m_bytesPerVoxel;
      if (dstByte < 0 || dstByte + innerBytes > blockBytes)
        continue;

      // decode the whole inner chunk into a temporary buffer, then place it
      // row-by-row (an inner chunk only spans a contiguous X-run of the full
      // shard; its Y and Z extents are interleaved in the shard's C order).
      QByteArray tmp;
      tmp.resize((int)innerBytes);
      int n;
      if (nb == innerBytes)
        {
          // nbytes equals the raw size: could be a blosc frame or raw data
          n = blosc_decompress(src0 + off, tmp.data(), (size_t)innerBytes);
          if (n < 0)
            {
              n = (int)innerBytes;
              memcpy(tmp.data(), src0 + off, (size_t)innerBytes);
            }
        }
      else
        {
          n = blosc_decompress(src0 + off, tmp.data(), (size_t)innerBytes);
        }
      if (n < 0)
        continue;

      const char* src = tmp.constData();
      const qint64 rowBytes = m_innerX * m_bytesPerVoxel;
      for (qint64 lz = 0; lz < m_innerZ; ++lz)
        for (qint64 ly = 0; ly < m_innerY; ++ly)
          {
            const qint64 srcRow = (lz * m_innerY + ly) * m_innerX
                                  * m_bytesPerVoxel;
            const qint64 dstRow =
              (((iz * m_innerZ + lz) * m_chunkY + (iy * m_innerY + ly))
               * m_chunkX + ix * m_innerX) * m_bytesPerVoxel;
            if (dstRow < 0 || dstRow + rowBytes > blockBytes)
              continue;
            memcpy(out.data() + (int)dstRow, src + (int)srcRow,
                   (size_t)rowBytes);
          }
    }
  return true;
}

// ---------------------------------------------------------------------
void
ZarrPlugin::generateHistogram()
{
  if (m_depth <= 0 || m_width <= 0 || m_height <= 0)
    return;

  const bool ushort = (m_voxelType == _UShort);
  const qint64 bin = ushort ? 65536 : 256;

  QProgressDialog progress("Scanning Zarr volume",
                           QString(),
                           0, 100,
                           0);
  progress.setMinimumDuration(0);

  // preallocate the full-range histogram, then increment in place
  // (mirrors the nc4 plugin's _UChar/_UShort handling).
  m_histogram.clear();
  m_histogram.reserve((int)bin);
  for (qint64 i = 0; i < bin; ++i)
    m_histogram.append(0);

  int minv = 10000000, maxv = -10000000;

  const qint64 nkz = (m_depth  + m_chunkZ - 1) / m_chunkZ;
  const qint64 nky = (m_width  + m_chunkY - 1) / m_chunkY;
  const qint64 nkx = (m_height + m_chunkX - 1) / m_chunkX;
  const qint64 total = nkz * nky * nkx;

  QByteArray chunk;
  qint64 idx = 0;
  for (qint64 kz = 0; kz < nkz; ++kz)
    {
      for (qint64 ky = 0; ky < nky; ++ky)
        {
          for (qint64 kx = 0; kx < nkx; ++kx, ++idx)
            {
              progress.setValue((int)(100.0 * (double)idx / (double)total));
              qApp->processEvents();

              chunk.clear();
              if (!readChunk(kz, ky, kx, chunk))
                continue;

              const unsigned char* p = (const unsigned char*)chunk.constData();
              const qint64 nbytes = (qint64)chunk.size();
              if (ushort)
                {
                  const qint64 n = nbytes / m_bytesPerVoxel;
                  for (qint64 i = 0; i < n; ++i)
                    {
                      int v = (int)p[(size_t)i * 2]
                              | ((int)p[(size_t)i * 2 + 1] << 8);
                      if ((size_t)v >= (size_t)m_histogram.size())
                        continue;
                      m_histogram[v]++;
                      if (v < minv) minv = v;
                      if (v > maxv) maxv = v;
                    }
                }
              else
                {
                  for (qint64 i = 0; i < nbytes; ++i)
                    {
                      int v = p[i];
                      m_histogram[v]++;
                      if (v < minv) minv = v;
                      if (v > maxv) maxv = v;
                    }
                }
            }
        }
    }

  if (minv > maxv) { minv = 0; maxv = 0; }

  // prefer the range given by the store metadata (data_min_max) when
  // present, since a scan-based min includes fabric padding zeros.
  if (!m_haveDataMinMax)
    {
      m_rawMin = minv;
      m_rawMax = maxv;
    }

  progress.setValue(100);
  qApp->processEvents();
}

// ---------------------------------------------------------------------
// Depth slice: plane (Y, X) at depth slc.  Only chunks intersecting the
// plane are read, each chunk decompressed once.
void
ZarrPlugin::getDepthSlice(int slc, uchar* slice)
{
  const int w = m_width, h = m_height;   // (Y, X)
  const int bpv = m_bytesPerVoxel;
  const qint64 kz = slc / m_chunkZ;
  const qint64 inz = slc % m_chunkZ;
  const qint64 nky = (m_width  + m_chunkY - 1) / m_chunkY;
  const qint64 nkx = (m_height + m_chunkX - 1) / m_chunkX;

  memset(slice, 0, (size_t)w * h * bpv);

  QByteArray chunk;
  for (qint64 ky = 0; ky < nky; ++ky)
    {
      const qint64 y0 = ky * m_chunkY;
      const qint64 ry = qMin((qint64)m_chunkY, (qint64)w - y0);
      for (qint64 kx = 0; kx < nkx; ++kx)
        {
          const qint64 x0 = kx * m_chunkX;
          const qint64 rx = qMin((qint64)m_chunkX, (qint64)h - x0);
          chunk.clear();
          if (!readChunk(kz, ky, kx, chunk))
            continue;
          const unsigned char* p = (const unsigned char*)chunk.constData()
                                  + (size_t)(inz * m_chunkY * m_chunkX) * bpv;
          for (qint64 y = 0; y < ry; ++y)
            memcpy(slice + (size_t)((y0 + y) * h + x0) * bpv,
                   p + (size_t)(y * m_chunkX) * bpv,
                   (size_t)rx * bpv);
        }
    }
}

// ---------------------------------------------------------------------
// Width slice: plane (Z, X) at width slc (along Y).
void
ZarrPlugin::getWidthSlice(int slc, uchar* slice)
{
  const int d = m_depth, h = m_height;   // (Z, X)
  const int bpv = m_bytesPerVoxel;
  const qint64 ky = slc / m_chunkY;
  const qint64 iny = slc % m_chunkY;
  const qint64 nkz = (m_depth  + m_chunkZ - 1) / m_chunkZ;
  const qint64 nkx = (m_height + m_chunkX - 1) / m_chunkX;

  memset(slice, 0, (size_t)d * h * bpv);

  QByteArray chunk;
  for (qint64 kz = 0; kz < nkz; ++kz)
    {
      const qint64 z0 = kz * m_chunkZ;
      const qint64 rz = qMin((qint64)m_chunkZ, (qint64)d - z0);
      for (qint64 kx = 0; kx < nkx; ++kx)
        {
          const qint64 x0 = kx * m_chunkX;
          const qint64 rx = qMin((qint64)m_chunkX, (qint64)h - x0);
          chunk.clear();
          if (!readChunk(kz, ky, kx, chunk))
            continue;
          const unsigned char* p = (const unsigned char*)chunk.constData();
          for (qint64 z = 0; z < rz; ++z)
            {
              const size_t rowoff = (size_t)((z * m_chunkY + iny) * m_chunkX)
                                    * bpv;
              memcpy(slice + (size_t)((z0 + z) * h + x0) * bpv,
                     p + rowoff,
                     (size_t)rx * bpv);
            }
        }
    }
}

// ---------------------------------------------------------------------
// Height slice: plane (Z, Y) at height slc (along X).  The source column is
// strided by m_chunkX (a column within each z-plane of the chunk), so each
// voxel is copied individually.
void
ZarrPlugin::getHeightSlice(int slc, uchar* slice)
{
  const int d = m_depth, w = m_width;   // (Z, Y)
  const int bpv = m_bytesPerVoxel;
  const qint64 kx = slc / m_chunkX;
  const qint64 inx = slc % m_chunkX;
  const qint64 nkz = (m_depth  + m_chunkZ - 1) / m_chunkZ;
  const qint64 nky = (m_width  + m_chunkY - 1) / m_chunkY;

  memset(slice, 0, (size_t)d * w * bpv);

  QByteArray chunk;
  for (qint64 kz = 0; kz < nkz; ++kz)
    {
      const qint64 z0 = kz * m_chunkZ;
      const qint64 rz = qMin((qint64)m_chunkZ, (qint64)d - z0);
      for (qint64 ky = 0; ky < nky; ++ky)
        {
          const qint64 y0 = ky * m_chunkY;
          const qint64 ry = qMin((qint64)m_chunkY, (qint64)w - y0);
          chunk.clear();
          if (!readChunk(kz, ky, kx, chunk))
            continue;
          const unsigned char* p = (const unsigned char*)chunk.constData();
          for (qint64 z = 0; z < rz; ++z)
            {
              for (qint64 y = 0; y < ry; ++y)
                {
                  const size_t off = (size_t)((z * m_chunkY + y)
                                              * m_chunkX + inx) * bpv;
                  memcpy(slice + (size_t)((z0 + z) * w + (y0 + y)) * bpv,
                         p + off,
                         (size_t)bpv);
                }
            }
        }
    }
}

// ---------------------------------------------------------------------
QVariant
ZarrPlugin::rawValue(int d, int w, int h)
{
  QVariant v;

  if (d < 0 || d >= m_depth ||
      w < 0 || w >= m_width ||
      h < 0 || h >= m_height)
    {
      v = QVariant(QString("OutOfBounds"));
      return v;
    }

  const int bpv = m_bytesPerVoxel;
  const qint64 kz = d / m_chunkZ;
  const qint64 inz = d % m_chunkZ;
  const qint64 ky = w / m_chunkY;
  const qint64 iny = w % m_chunkY;
  const qint64 kx = h / m_chunkX;
  const qint64 inx = h % m_chunkX;

  QByteArray chunk;
  if (!readChunk(kz, ky, kx, chunk))
    {
      v = QVariant(QString("ReadFailed"));
      return v;
    }

  const unsigned char* p = (const unsigned char*)chunk.constData();
  const size_t off = (size_t)((inz * m_chunkY + iny) * m_chunkX + inx) * bpv;

  if (m_voxelType == _UShort)
    v = QVariant((uint)((int)p[off] | ((int)p[off + 1] << 8)));
  else
    v = QVariant((uint)p[off]);

  return v;
}
