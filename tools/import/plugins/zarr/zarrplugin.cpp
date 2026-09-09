#include <QtGui>
#include <QFileInfo>
#include <QDir>
#include <QThread>
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <atomic>
#include <mutex>
#include <thread>
#include <cmath>

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

#include "common.h"
#include "zarrplugin.h"

using namespace std;

QStringList
ZarrPlugin::registerPlugin()
{
  QStringList regString;
  regString << "directory";
  regString << "Zarr Directory";

  return regString;
}

void
ZarrPlugin::init()
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
          // build option labels including each level's dimensions
          QStringList labels;
          for (const QString& lv : m_levels)
            {
              QString dims = "? x ? x ?";
              try
                {
                  zarr::Array arr = m_root->open_array(lv.toStdString());
                  const zarr::ArrayMeta& meta = arr.meta();
                  const std::vector<std::uint64_t>& shape = meta.shape;
                  if (shape.size() >= 3)
                    dims = QString("%1 x %2 x %3")
                             .arg(shape[2]).arg(shape[1]).arg(shape[0]);
                }
              catch (const std::exception&)
                {
                }
              labels << QString("%1  (%2)").arg(lv).arg(dims);
            }

          bool ok;
          QString choice = QInputDialog::getItem(0,
                                                 "Choose a pyramid level",
                                                 "Levels",
                                                 labels,
                                                 0,
                                                 false,
                                                 &ok);
          if (!ok)
            choice = labels[0];

          // map the chosen label back to its level path
          int idx = labels.indexOf(choice);
          if (idx < 0)
            idx = 0;
          m_level = m_levels[idx];
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

  // float32/int32 need their value range before the histogram can be binned
  // (see generateHistogram); do that scan once here unless the store metadata
  // already gave us data_min_max.
  if ((m_voxelType == _Float || m_voxelType == _Int) && !m_haveDataMinMax)
    findFloatMinMax();

  generateHistogram();

  return true;
}

// ---------------------------------------------------------------------
// Open <dir> as a zarr group and harvest the available pyramid levels
// (multiscales[0].datasets[].path) plus the drishti attributes, from the
// group's metadata (libzarr exposes them on the Group/attributes).
bool
ZarrPlugin::parseRoot()
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
  // per-level scale transform.  The multiscales metadata may live at the
  // top level ("multiscales") or, for NGFF/OME datasets (e.g. the Zeiss
  // TIMA output), nested under "ome.multiscales".
  zarr::json multiscales = zarr::json::array();
  if (attr.contains("multiscales") && attr.at("multiscales").is_array())
    multiscales = attr.at("multiscales");
  else if (attr.contains("ome") && attr.at("ome").is_object())
    {
      const zarr::json& ome = attr.at("ome");
      if (ome.contains("multiscales") && ome.at("multiscales").is_array())
        multiscales = ome.at("multiscales");
    }
  if (multiscales.is_array() && multiscales.size() > 0)
    {
      if (multiscales[0].is_object())
        {
          const zarr::json& datasets =
            multiscales[0].contains("datasets")
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
ZarrPlugin::parseLevel()
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

  switch (meta.dtype.kind)
    {
    case zarr::DType::uint8:
      m_voxelType = _UChar;
      m_bytesPerVoxel = 1;
      break;
    case zarr::DType::int8:
      m_voxelType = _Char;
      m_bytesPerVoxel = 1;
      break;
    case zarr::DType::uint16:
      m_voxelType = _UShort;
      m_bytesPerVoxel = 2;
      break;
    case zarr::DType::int16:
      m_voxelType = _Short;
      m_bytesPerVoxel = 2;
      break;
    case zarr::DType::int32:
      m_voxelType = _Int;
      m_bytesPerVoxel = 4;
      break;
    case zarr::DType::float32:
      m_voxelType = _Float;
      m_bytesPerVoxel = 4;
      break;
    default:
      // other dtypes (float16/float64/{u,}int64/{u}int32/boolean) are not
      // representable in drishti's VoxelType set (no double / 4-byte uints).
      return false;
    }

  m_chunkShape = meta.chunk_shape;

  return true;
}

// ---------------------------------------------------------------------
// Decompress one full (fill-padded to chunk shape) block into out.
// All decoding is delegated to libzarr (compression, sharding, byte order).
bool
ZarrPlugin::readChunk(int kz, int ky, int kx, QByteArray& out) const
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
ZarrPlugin::readSliceRegion(vector<uint64_t> origin, vector<uint64_t> shape,
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
// Open a fresh, independent read handle for a worker thread.  libzarr
// documents its core as single-threaded ("implementations are not required
// to be thread-safe"), so concurrent read_region() calls must go through
// separate Store/Group/Array instances; the underlying files are opened
// read-only, which is safe to share across threads.  Throws zarr::error on
// failure.
std::shared_ptr<zarr::Array>
ZarrPlugin::openArrayReader() const
{
  auto store = std::make_shared<zarr::FilesystemStore>(
                 m_dir.toStdString(), false);
  auto root = std::make_shared<zarr::Group>(
                zarr::Group::open(store, "", zarr::OpenOptions{}));
  return std::make_shared<zarr::Array>(
           root->open_array(m_level.toStdString()));
}

// ---------------------------------------------------------------------
// Read zcount depth planes (Y, X), starting at depth z0, through a
// caller-supplied array handle into buffer, which must hold
// zcount*width*height*bytesPerVoxel bytes.  Reading a z-block instead of
// one plane at a time lets a sharded store decompress each on-disk 64^3
// sub-block once for all the planes it covers, instead of once per plane.
void
ZarrPlugin::readDepthBlock(zarr::Array& arr, int z0, int zcount,
                           uchar* buffer) const
{
  const size_t nbytes = (size_t)zcount * (size_t)m_width *
                        (size_t)m_height * (size_t)m_bytesPerVoxel;
  try
    {
      arr.read_region({ (uint64_t)z0, 0, 0 },
                      { (uint64_t)zcount, (uint64_t)m_width,
                        (uint64_t)m_height },
                      buffer, nbytes);
    }
  catch (const std::exception&)
    {
      memset(buffer, 0, nbytes);   // missing/corrupt data -> fill (zero)
    }
}

// ---------------------------------------------------------------------
// Scan the whole volume for its raw min/max.  Reads z-blocks of depth
// planes (Y,X) so every on-disk sharded sub-block is decompressed once per
// sweep instead of once per plane, and splits the block sweep across worker
// threads, each with its own libzarr read handle, so the decompression runs
// in parallel.  Used for float32/int32 voxels whose value range must be
// known before the histogram can be binned, mirroring RawPlugin::findMinMax.
void
ZarrPlugin::findFloatMinMax()
{
  QProgressDialog progress("Finding Min and Max",
                           QString(),
                           0, 100,
                           0);
  progress.setMinimumDuration(0);

  const size_t planeBytes = (size_t)m_width * (size_t)m_height *
                            (size_t)m_bytesPerVoxel;
  const size_t n = (size_t)m_width * (size_t)m_height;
  const bool isFloat = (m_voxelType == _Float);

  // z-planes per read: start from the innermost chunk z-extent (64^3 for
  // sharded stores) so each sub-block is decompressed once, and halve it
  // until a single read fits within the per-read memory budget.
  const qint64 planeBytes64 = (qint64)planeBytes;
  const qint64 maxBlockBytes = 512LL * 1024 * 1024;
  qint64 blockZ = m_chunkShape.size() > 0 ? (qint64)m_chunkShape[0] : 1;
  blockZ = qBound<qint64>(1, blockZ, m_depth);
  while (blockZ > 1 && blockZ * planeBytes64 > maxBlockBytes)
    blockZ /= 2;
  const qint64 nlayers = (m_depth + blockZ - 1) / blockZ;

  const int hw = QThread::idealThreadCount();
  int nthreads = qMin(24, qBound(1, hw, (int)nlayers));
  // keep the total buffer memory of live workers within a budget
  const qint64 maxTotalBytes = 4LL * 1024 * 1024 * 1024;
  while (nthreads > 1 &&
         (qint64)nthreads * blockZ * planeBytes64 > maxTotalBytes)
    --nthreads;

  std::vector<float> localMin((size_t)nthreads, 10000000.0f);
  std::vector<float> localMax((size_t)nthreads, -10000000.0f);
  std::atomic<qint64> next(0);
  std::atomic<qint64> done(0);

  std::vector<std::thread> pool;
  pool.reserve((size_t)nthreads);
  for (int t = 0; t < nthreads; ++t)
    {
      pool.emplace_back([&, t]()
        {
          std::shared_ptr<zarr::Array> arr;
          try { arr = openArrayReader(); }
          catch (const std::exception&) { return; }

          std::vector<uchar> buf((size_t)(blockZ * planeBytes64));
          float mn = 10000000.0f, mx = -10000000.0f;
          for (;;)
            {
              const qint64 layer = next.fetch_add(1);
              if (layer >= nlayers)
                break;

              const int z0 = (int)(layer * blockZ);
              const int zc = qMin((int)blockZ, m_depth - z0);
              readDepthBlock(*arr, z0, zc, buf.data());

              const size_t ne = (size_t)zc * n;
              if (isFloat)
                {
                  const float* p = (const float*)buf.data();
                  for (size_t i = 0; i < ne; ++i)
                    {
                      float v = p[i];
                      if (std::isnan(v)) v = 0;
                      if (v < mn) mn = v;
                      if (v > mx) mx = v;
                    }
                }
              else // _Int
                {
                  const int* p = (const int*)buf.data();
                  for (size_t i = 0; i < ne; ++i)
                    {
                      float v = (float)p[i];
                      if (v < mn) mn = v;
                      if (v > mx) mx = v;
                    }
                }
              done.fetch_add(zc, std::memory_order_relaxed);
            }
          localMin[(size_t)t] = mn;
          localMax[(size_t)t] = mx;
        });
    }

  // Poll for progress on the GUI thread while the workers run; no Qt calls
  // are made off the main thread.
  while (done.load(std::memory_order_relaxed) < m_depth)
    {
      progress.setValue((int)(100.0 *
        (double)done.load(std::memory_order_relaxed) / (double)m_depth));
      qApp->processEvents();
      QThread::msleep(20);
    }
  for (size_t t = 0; t < pool.size(); ++t)
    pool[t].join();

  float mn = 10000000.0f, mx = -10000000.0f;
  for (size_t t = 0; t < localMin.size(); ++t)
    {
      if (localMin[t] < mn) mn = localMin[t];
      if (localMax[t] > mx) mx = localMax[t];
    }
  m_rawMin = mn;
  m_rawMax = mx;

  progress.setValue(100);
  qApp->processEvents();
}

// ---------------------------------------------------------------------
// Build the histogram (and raw min/max).  Reads z-blocks of depth planes
// (Y,X) via libzarr's read_region so each on-disk sharded sub-block is
// decompressed once per sweep instead of once per plane, and splits the
// block sweep across worker threads, each with its own libzarr read handle
// and a private cache-friendly bucket array, then merges the per-thread
// buckets (the same parallel pattern as RawPlugin::generateHistogram).
void
ZarrPlugin::generateHistogram()
{
  if (m_depth <= 0 || m_width <= 0 || m_height <= 0)
    return;

  const size_t planeBytes = (size_t)m_width * (size_t)m_height *
                            (size_t)m_bytesPerVoxel;
  const size_t n = (size_t)m_width * (size_t)m_height;

  // z-planes per read: start from the innermost chunk z-extent (64^3 for
  // sharded stores) so each sub-block is decompressed once, and halve it
  // until a single read fits within the per-read memory budget.
  const qint64 planeBytes64 = (qint64)planeBytes;
  const qint64 maxBlockBytes = 512LL * 1024 * 1024;
  qint64 blockZ = m_chunkShape.size() > 0 ? (qint64)m_chunkShape[0] : 1;
  blockZ = qBound<qint64>(1, blockZ, m_depth);
  while (blockZ > 1 && blockZ * planeBytes64 > maxBlockBytes)
    blockZ /= 2;
  const qint64 nlayers = (m_depth + blockZ - 1) / blockZ;

  const int hw = QThread::idealThreadCount();
  int nthreads = qMin(24, qBound(1, hw, (int)nlayers));
  // keep the total buffer memory of live workers within a budget
  const qint64 maxTotalBytes = 4LL * 1024 * 1024 * 1024;
  while (nthreads > 1 &&
         (qint64)nthreads * blockZ * planeBytes64 > maxTotalBytes)
    --nthreads;

  // ---- float32 / int32: no natural bin index, so min/max are computed
  // first (findFloatMinMax, called from setFile) and the value range is
  // scaled into 64K bins (see RawPlugin::generateHistogram).
  if (m_voxelType == _Float || m_voxelType == _Int)
    {
      const qint64 binCount = 65536;
      const qint64 histMax = binCount - 1;
      const float omin = m_rawMin;
      const float range = m_rawMax - m_rawMin;
      const bool isFloat = (m_voxelType == _Float);

      QProgressDialog progress("Generating Histogram",
                               QString(),
                               0, 100,
                               0);
      progress.setMinimumDuration(0);

      m_histogram.clear();
      m_histogram.reserve((int)binCount);
      for (qint64 i = 0; i < binCount; ++i)
        m_histogram.append(0);

      std::vector<std::vector<qint64> > local(
        (size_t)nthreads, std::vector<qint64>((size_t)binCount, 0));
      std::atomic<qint64> next(0);
      std::atomic<qint64> done(0);

      std::vector<std::thread> pool;
      pool.reserve((size_t)nthreads);
      for (int t = 0; t < nthreads; ++t)
        {
          pool.emplace_back([&, t]()
            {
              std::shared_ptr<zarr::Array> arr;
              try { arr = openArrayReader(); }
              catch (const std::exception&) { return; }

              std::vector<uchar> buf((size_t)(blockZ * planeBytes64));
              qint64* h = local[(size_t)t].data();
              for (;;)
                {
                  const qint64 layer = next.fetch_add(1);
                  if (layer >= nlayers)
                    break;

                  const int z0 = (int)(layer * blockZ);
                  const int zc = qMin((int)blockZ, m_depth - z0);
                  readDepthBlock(*arr, z0, zc, buf.data());

                  const size_t ne = (size_t)zc * n;
                  if (isFloat)
                    {
                      const float* p = (const float*)buf.data();
                      for (size_t i = 0; i < ne; ++i)
                        {
                          float v = p[i];
                          if (std::isnan(v)) v = 0;
                          float fidx = (range > 0) ? (v - omin) / range : 0.0f;
                          fidx = qBound(0.0f, fidx, 1.0f);
                          h[(int)(fidx * histMax)]++;
                        }
                    }
                  else // _Int
                    {
                      const int* p = (const int*)buf.data();
                      for (size_t i = 0; i < ne; ++i)
                        {
                          float v = (float)p[i];
                          float fidx = (range > 0) ? (v - omin) / range : 0.0f;
                          fidx = qBound(0.0f, fidx, 1.0f);
                          h[(int)(fidx * histMax)]++;
                        }
                    }
                  done.fetch_add(zc, std::memory_order_relaxed);
                }
            });
        }

      while (done.load(std::memory_order_relaxed) < m_depth)
        {
          progress.setValue((int)(100.0 *
            (double)done.load(std::memory_order_relaxed) / (double)m_depth));
          qApp->processEvents();
          QThread::msleep(20);
        }
      for (size_t t = 0; t < pool.size(); ++t)
        pool[t].join();

      for (qint64 v = 0; v < binCount; ++v)
        {
          uint total = 0;
          for (int t = 0; t < nthreads; ++t)
            total += (uint)local[(size_t)t][(size_t)v];
          m_histogram[(int)v] = total;
        }

      progress.setValue(100);
      qApp->processEvents();
      return;
    }

  // ---- integer types: direct bin indexing ----
  const bool is16 = (m_voxelType == _UShort || m_voxelType == _Short);
  const qint64 bin = is16 ? 65536 : 256;
  const size_t binSize = (size_t)bin;
  const long indexShift = (m_voxelType == _Char) ? 128
                        : (m_voxelType == _Short) ? 32768
                        : 0;

  QProgressDialog progress("Scanning Zarr volume",
                           QString(),
                           0, 100,
                           0);
  progress.setMinimumDuration(0);

  m_histogram.clear();
  m_histogram.reserve((int)bin);
  for (qint64 i = 0; i < bin; ++i)
    m_histogram.append(0);

  std::vector<std::vector<qint64> > local(
    (size_t)nthreads, std::vector<qint64>(binSize, 0));
  std::atomic<qint64> next(0);
  std::atomic<qint64> done(0);

  std::vector<std::thread> pool;
  pool.reserve((size_t)nthreads);
  for (int t = 0; t < nthreads; ++t)
    {
      pool.emplace_back([&, t]()
        {
          std::shared_ptr<zarr::Array> arr;
          try { arr = openArrayReader(); }
          catch (const std::exception&) { return; }

          std::vector<uchar> buf((size_t)(blockZ * planeBytes64));
          qint64* h = local[(size_t)t].data();
          for (;;)
            {
              const qint64 layer = next.fetch_add(1);
              if (layer >= nlayers)
                break;

              const int z0 = (int)(layer * blockZ);
              const int zc = qMin((int)blockZ, m_depth - z0);
              readDepthBlock(*arr, z0, zc, buf.data());

              const size_t ne = (size_t)zc * n;
              if (m_voxelType == _UShort)
                {
                  const unsigned short* sp = (const unsigned short*)buf.data();
                  for (size_t i = 0; i < ne; ++i)
                    h[(size_t)sp[i]]++;
                }
              else if (m_voxelType == _Short)
                {
                  const short* sp = (const short*)buf.data();
                  for (size_t i = 0; i < ne; ++i)
                    h[(size_t)((long)sp[i] + 32768)]++;
                }
              else if (m_voxelType == _UChar)
                {
                  const unsigned char* cp = (const unsigned char*)buf.data();
                  for (size_t i = 0; i < ne; ++i)
                    h[(size_t)cp[i]]++;
                }
              else // _Char
                {
                  const signed char* cp = (const signed char*)buf.data();
                  for (size_t i = 0; i < ne; ++i)
                    h[(size_t)((long)cp[i] + 128)]++;
                }
              done.fetch_add(zc, std::memory_order_relaxed);
            }
        });
    }

  while (done.load(std::memory_order_relaxed) < m_depth)
    {
      progress.setValue((int)(100.0 *
        (double)done.load(std::memory_order_relaxed) / (double)m_depth));
      qApp->processEvents();
      QThread::msleep(20);
    }
  for (size_t t = 0; t < pool.size(); ++t)
    pool[t].join();

  // merge the per-thread buckets
  std::vector<qint64> hist(binSize, 0);
  for (int t = 0; t < nthreads; ++t)
    for (size_t v = 0; v < binSize; ++v)
      hist[v] += local[(size_t)t][v];

  int minv = -1, maxv = -1;
  for (size_t v = 0; v < binSize; ++v)
    {
      if (hist[v] > 0)
        {
          if (minv < 0)
            minv = (int)v;
          maxv = (int)v;
        }
    }

  for (qint64 v = 0; v < bin; ++v)
    m_histogram[(int)v] = (uint)hist[(size_t)v];

  if (minv < 0)
    {
      minv = 0;
      maxv = 0;
    }

  // prefer the range given by the store metadata (data_min_max) when
  // present.
  if (!m_haveDataMinMax)
    {
      m_rawMin = (float)(minv - indexShift);
      m_rawMax = (float)(maxv - indexShift);
    }

  progress.setValue(100);
  qApp->processEvents();
}

// ---------------------------------------------------------------------
// Depth slice: plane (Y, X) at depth slc.
void
ZarrPlugin::getDepthSlice(int slc, uchar* slice)
{
  readSliceRegion({ (uint64_t)slc, 0, 0 },
                  { 1, (uint64_t)m_width, (uint64_t)m_height },
                  slice);
}

// ---------------------------------------------------------------------
// Width slice: plane (Z, X) at width slc (along Y).
void
ZarrPlugin::getWidthSlice(int slc, uchar* slice)
{
  readSliceRegion({ 0, (uint64_t)slc, 0 },
                  { (uint64_t)m_depth, 1, (uint64_t)m_height },
                  slice);
}

// ---------------------------------------------------------------------
// Height slice: plane (Z, Y) at height slc (along X).
void
ZarrPlugin::getHeightSlice(int slc, uchar* slice)
{
  readSliceRegion({ 0, 0, (uint64_t)slc },
                  { (uint64_t)m_depth, (uint64_t)m_width, 1 },
                  slice);
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

  if (m_voxelType == _Float)
    {
      float f;
      memcpy(&f, tmp, 4);
      v = QVariant((double)f);
    }
  else if (m_voxelType == _Int)
    {
      int iv;
      memcpy(&iv, tmp, 4);
      v = QVariant((int)iv);
    }
  else if (m_voxelType == _UShort)
    v = QVariant((uint)((int)tmp[0] | ((int)tmp[1] << 8)));
  else if (m_voxelType == _Short)
    {
      short s;
      memcpy(&s, tmp, 2);
      v = QVariant((int)s);
    }
  else if (m_voxelType == _Char)
    v = QVariant((int)(signed char)tmp[0]);
  else
    v = QVariant((uint)tmp[0]);

  return v;
}
