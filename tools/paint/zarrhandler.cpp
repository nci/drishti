#include <QtGui>
#include <QFileInfo>
#include <QDir>
#include <vector>
#include <stdexcept>
#include "zarrhandler.h"

#include <QProgressDialog>
#include <QMessageBox>
#include <QInputDialog>

using namespace std;

enum VoxelType {
  _UChar,
  _Char,
  _UShort,
  _Short,
  _Int,
  _Float
};
enum VoxelUnit {
  Nounit = 0,
  _Angstrom,
  _Nanometer,
  _Micron,
  _Millimeter,
  _Centimeter,
  _Meter
};


void
ZarrHandler::init()
{
  m_dir.clear();
  m_level.clear();
  m_description.clear();

  m_store.reset();
  m_root.reset();
  m_array.reset();

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

  m_chunkShape.clear();
  m_levels.clear();

  m_haveRoot = false;
}

void
ZarrHandler::clear()
{
  init();
}


void
ZarrHandler::gridSize(int& d, int& w, int& h)
{
  d = m_depth;
  w = m_width;
  h = m_height;
}

void
ZarrHandler::voxelSize(float& vx, float& vy, float& vz)
{
  vx = m_voxelSizeX;
  vy = m_voxelSizeY;
  vz = m_voxelSizeZ;
}

QString ZarrHandler::description() { return m_description; }
int ZarrHandler::voxelUnit() { return m_voxelUnit; }
int ZarrHandler::voxelType() { return m_voxelType; }
int ZarrHandler::headerBytes() { return m_headerBytes; }

QList<uint> ZarrHandler::histogram() { return m_histogram; }

void
ZarrHandler::setMinMax(float rmin, float rmax)
{
  m_rawMin = rmin;
  m_rawMax = rmax;

  if (m_voxelType == _UChar || m_voxelType == _UShort)
    return;
  generateHistogram();
}

float ZarrHandler::rawMin() { return m_rawMin; }
float ZarrHandler::rawMax() { return m_rawMax; }

// ---------------------------------------------------------------------
// setFile(QStringList) -> the first entry is the zarr directory (or a file
// inside it, whose directory is then used).
bool
ZarrHandler::setFile(QString filename, QString level)
{
  QFileInfo f(filename);
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

  m_level = level;
  
//  if (m_levels.size() == 1)
//    {
//      m_level = m_levels[0];
//    }
//  else
//    {
//      // override for headless testing / scripts
//      QByteArray forced = qgetenv("ZARR_FORCE_LEVEL");
//      QString tf = QString::fromLocal8Bit(forced).trimmed();
//      if (forced.size() && m_levels.contains(tf))
//        {
//          m_level = tf;
//        }
//      else
//        {
//          bool ok;
//          QString lv = QInputDialog::getItem(0,
//                                             "Choose a pyramid level",
//                                             "Levels",
//                                             m_levels,
//                                             0,
//                                             false,
//                                             &ok);
//          m_level = ok ? lv : m_levels[0];
//        }
//    }

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

  //generateHistogram();

  return true;
}

// ---------------------------------------------------------------------
// Open <dir> as a zarr group and harvest the available pyramid levels
// (multiscales[0].datasets[].path) plus the drishti attributes, from the
// group's metadata (libzarr exposes them on the Group/attributes).
bool
ZarrHandler::parseRoot()
{
  m_haveRoot = false;
  m_levels.clear();

  try
    {
      m_store = std::make_shared<zarr::FilesystemStore>(
                  m_dir.toStdString(), false);
      m_root = std::make_shared<zarr::Group>(
                  zarr::Group::open(m_store, "", zarr::OpenOptions{}));
    }
  catch (const std::exception&)
    {
      return false;
    }

  const zarr::json& attr = m_root->attributes();

  // optional drishti/mango attributes (description / data_min_max /
  // voxel_unit / voxel_size_xyz).  Prefer "drishti", fall back to "mango"
  // (toZarrStreaming / toZarrStreaming.cpp writes "mango").
  zarr::json meta = attr.contains("drishti")
                    ? attr.at("drishti") : zarr::json::object();
  if (meta.empty() && attr.contains("mango"))
    meta = attr.at("mango");

  if (meta.contains("description") && meta.at("description").is_string())
    m_description = QString::fromStdString(
                      meta.at("description").get<std::string>());

  if (meta.contains("voxel_unit") && meta.at("voxel_unit").is_string())
    {
      QString vu = QString::fromStdString(
                     meta.at("voxel_unit").get<std::string>()).toLower();
      if (vu == "mm" || vu == "millimeter") m_voxelUnit = _Millimeter;
      else if (vu == "micron" || vu == "um" || vu == "mu") m_voxelUnit = _Micron;
      else if (vu == "angstrom") m_voxelUnit = _Angstrom;
      else if (vu == "nanometer") m_voxelUnit = _Nanometer;
      else if (vu == "centimeter") m_voxelUnit = _Centimeter;
      else if (vu == "meter") m_voxelUnit = _Meter;
    }

  // voxel_size_xyz : [vx, vy, vz]
  if (meta.contains("voxel_size_xyz") && meta.at("voxel_size_xyz").is_array())
    {
      const zarr::json& vsz = meta.at("voxel_size_xyz");
      if (vsz.size() >= 3)
        {
          m_voxelSizeX = (float)vsz[0].get<double>();
          m_voxelSizeY = (float)vsz[1].get<double>();
          m_voxelSizeZ = (float)vsz[2].get<double>();
        }
    }

  // data_min_max : [min, max]
  if (meta.contains("data_min_max") && meta.at("data_min_max").is_array())
    {
      const zarr::json& dmm = meta.at("data_min_max");
      if (dmm.size() >= 2)
        {
          m_rawMin = (float)dmm[0].get<double>();
          m_rawMax = (float)dmm[1].get<double>();
          m_haveDataMinMax = true;
        }
    }

  // harvest level paths from multiscales[0].datasets[].path and the
  // per-level scale transform.
  if (attr.contains("multiscales") && attr.at("multiscales").is_array())
    {
      const zarr::json& multiscales = attr.at("multiscales");
      if (multiscales.size() > 0)
        {
          const zarr::json& datasets =
            multiscales[0].contains("datasets") && multiscales[0].is_object()
            ? multiscales[0].at("datasets") : zarr::json::array();
          if (datasets.is_array())
            {
              for (const zarr::json& d : datasets)
                {
                  if (!d.is_object() || !d.contains("path"))
                    continue;
                  std::string p = d.at("path").get<std::string>();
                  if (p.empty())
                    continue;
                  QString pq = QString::fromStdString(p);
                  m_levels.append(pq);
                  QVector<float> sc(3, 1.0f);
                  if (d.contains("coordinateTransformations") &&
                      d.at("coordinateTransformations").is_array())
                    {
                      const zarr::json& ct = d.at("coordinateTransformations");
                      if (ct.size() > 0 && ct[0].is_object() &&
                          ct[0].contains("scale") && ct[0].at("scale").is_array())
                        {
                          const zarr::json& scale = ct[0].at("scale");
                          if (scale.size() >= 3)
                            {
                              sc[0] = (float)scale[0].get<double>();
                              sc[1] = (float)scale[1].get<double>();
                              sc[2] = (float)scale[2].get<double>();
                            }
                        }
                    }
                  m_levelScales[pq] = sc;
                }
            }
        }
    }

  m_haveRoot = true;
  return true;
}

// ---------------------------------------------------------------------
// Open <dir>/<level> as a zarr array and read its shape/type/chunks from
// the normalized metadata exposed by libzarr.
bool
ZarrHandler::parseLevel()
{
  try
    {
      m_array = std::make_shared<zarr::Array>(
                  m_root->open_array(m_level.toStdString()));
    }
  catch (const std::exception&)
    {
      return false;
    }

  const zarr::ArrayMeta& meta = m_array->meta();

  const vector<uint64_t>& shape = meta.shape;
  if (shape.size() < 3)
    return false;
  m_depth = (int)shape[0];   // Z
  m_width = (int)shape[1];   // Y
  m_height = (int)shape[2];  // X

  bool isU16 = (meta.dtype.kind == zarr::DType::uint16);
  m_voxelType = isU16 ? _UShort : _UChar;
  m_bytesPerVoxel = isU16 ? 2 : 1;

  m_chunkShape = meta.chunk_shape;

  return true;
}

// ---------------------------------------------------------------------
// Decompress one full (fill-padded to chunk shape) block into out.
// All decoding is delegated to libzarr (compression, sharding, byte order).
bool
ZarrHandler::readChunk(int kz, int ky, int kx, QByteArray& out) const
{
  try
    {
      zarr::Bytes bytes = m_array->read_chunk(
          { (uint64_t)kz, (uint64_t)ky, (uint64_t)kx });
      out = QByteArray((const char*)bytes.data(), (int)bytes.size());
      return true;
    }
  catch (const std::exception&)
    {
      return false;
    }
}

// ---------------------------------------------------------------------
// Read an axis-aligned hyperslab (a full plane or a single voxel) into
// `slice`, which must hold product(shape)*bytesPerVoxel bytes, C order.
void
ZarrHandler::readSliceRegion(vector<uint64_t> origin, vector<uint64_t> shape,
                             uchar* slice) const
{
  size_t n = 1;
  for (size_t i = 0; i < shape.size(); ++i)
    n *= (size_t)shape[i];
  const size_t nbytes = n * (size_t)m_bytesPerVoxel;
  if (nbytes == 0)
    return;
  try
    {
      m_array->read_region(origin, shape, slice, nbytes);
    }
  catch (const std::exception&)
    {
      memset(slice, 0, nbytes);   // missing/corrupt data -> fill (zero)
    }
}

// ---------------------------------------------------------------------
// Store an axis-aligned hyperslab into the array. `slice` must hold
// product(shape)*bytesPerVoxel bytes, C order. libzarr read-modify-writes
// any chunk that the slab only partially covers.
void
ZarrHandler::writeSliceRegion(vector<uint64_t> origin, vector<uint64_t> shape,
                              const uchar* slice) const
{
  size_t n = 1;
  for (size_t i = 0; i < shape.size(); ++i)
    n *= (size_t)shape[i];
  const size_t nbytes = n * (size_t)m_bytesPerVoxel;
  if (nbytes == 0)
    return;
  try
    {
      m_array->write_region(origin, shape, slice, nbytes);
    }
  catch (const std::exception&)
    {
    }
}

// ---------------------------------------------------------------------
void
ZarrHandler::generateHistogram()
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

  const qint64 chunkZ = m_chunkShape.size() > 0 ? (qint64)m_chunkShape[0] : 1;
  const qint64 chunkY = m_chunkShape.size() > 1 ? (qint64)m_chunkShape[1] : 1;
  const qint64 chunkX = m_chunkShape.size() > 2 ? (qint64)m_chunkShape[2] : 1;

  const qint64 nkz = (m_depth  + chunkZ - 1) / chunkZ;
  const qint64 nky = (m_width  + chunkY - 1) / chunkY;
  const qint64 nkx = (m_height + chunkX - 1) / chunkX;
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
              if (!readChunk((int)kz, (int)ky, (int)kx, chunk))
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
// Read an axis-aligned block: inclusive start (d0,w0,h0) and inclusive end
// (d1,w1,h1) indices, stored into `data` as a contiguous Z, Y, X cube.
void
ZarrHandler::getRegion(int d0, int w0, int h0, int d1, int w1, int h1,
                       uchar* data)
{
  if (!data)
    return;

  // clamp to the array bounds
  if (d0 < 0) d0 = 0;
  if (w0 < 0) w0 = 0;
  if (h0 < 0) h0 = 0;
  if (d1 >= m_depth)  d1 = m_depth  - 1;
  if (w1 >= m_width)  w1 = m_width  - 1;
  if (h1 >= m_height) h1 = m_height - 1;

  if (d1 < d0 || w1 < w0 || h1 < h0)
    {
      memset(data, 0, (size_t)m_bytesPerVoxel);
      return;
    }

  vector<uint64_t> origin = { (uint64_t)d0, (uint64_t)w0, (uint64_t)h0 };
  vector<uint64_t> shape  = { (uint64_t)(d1 - d0 + 1),
                              (uint64_t)(w1 - w0 + 1),
                              (uint64_t)(h1 - h0 + 1) };
  readSliceRegion(origin, shape, data);
}

// ---------------------------------------------------------------------
// Depth slice: plane (Y, X) at depth slc.
void
ZarrHandler::getDepthSlice(int slc, uchar* slice)
{
  readSliceRegion({ (uint64_t)slc, 0, 0 },
                  { 1, (uint64_t)m_width, (uint64_t)m_height },
                  slice);
}

// ---------------------------------------------------------------------
// Width slice: plane (Z, X) at width slc (along Y).
void
ZarrHandler::getWidthSlice(int slc, uchar* slice)
{
  readSliceRegion({ 0, (uint64_t)slc, 0 },
                  { (uint64_t)m_depth, 1, (uint64_t)m_height },
                  slice);
}

// ---------------------------------------------------------------------
// Height slice: plane (Z, Y) at height slc (along X).
void
ZarrHandler::getHeightSlice(int slc, uchar* slice)
{
  readSliceRegion({ 0, 0, (uint64_t)slc },
                  { (uint64_t)m_depth, (uint64_t)m_width, 1 },
                  slice);
}

// ---------------------------------------------------------------------
// Store a depth slice: plane (Y, X) at depth slc back into the array.
void
ZarrHandler::setDepthSlice(int slc, uchar* slice)
{
  writeSliceRegion({ (uint64_t)slc, 0, 0 },
                   { 1, (uint64_t)m_width, (uint64_t)m_height },
                   slice);
}

// ---------------------------------------------------------------------
// Store a width slice: plane (Z, X) at width slc back into the array.
void
ZarrHandler::setWidthSlice(int slc, uchar* slice)
{
  writeSliceRegion({ 0, (uint64_t)slc, 0 },
                   { (uint64_t)m_depth, 1, (uint64_t)m_height },
                   slice);
}

// ---------------------------------------------------------------------
// Store a height slice: plane (Z, Y) at height slc back into the array.
void
ZarrHandler::setHeightSlice(int slc, uchar* slice)
{
  writeSliceRegion({ 0, 0, (uint64_t)slc },
                   { (uint64_t)m_depth, (uint64_t)m_width, 1 },
                   slice);
}

// ---------------------------------------------------------------------
QVariant
ZarrHandler::rawValue(int d, int w, int h)
{
  QVariant v;

  if (d < 0 || d >= m_depth ||
      w < 0 || w >= m_width ||
      h < 0 || h >= m_height)
    {
      v = QVariant(QString("OutOfBounds"));
      return v;
    }

  unsigned char tmp[8];
  const int bpv = m_bytesPerVoxel;
  try
    {
      vector<uint64_t> origin = { (uint64_t)d, (uint64_t)w, (uint64_t)h };
      vector<uint64_t> shape = { 1, 1, 1 };
      m_array->read_region(origin, shape, tmp, (size_t)bpv);
    }
  catch (const std::exception&)
    {
      v = QVariant(QString("ReadFailed"));
      return v;
    }

  if (m_voxelType == _UShort)
    v = QVariant((uint)((int)tmp[0] | ((int)tmp[1] << 8)));
  else
    v = QVariant((uint)tmp[0]);

  return v;
}
