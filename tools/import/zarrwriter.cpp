#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <deque>
#include <thread>
#include <mutex>
#include <condition_variable>

#include <libzarr/libzarr.hpp>
#include <libzarr/adapters/filesystem_store.hpp>

#include "savepvldialog.h"
#include "zarrwriter.h"

#include <QMessageBox>
#include <QProgressDialog>
#include <QInputDialog>
#include <QStringList>

using byte = std::uint8_t;

// ---------------------------------------------------------------------
struct Compressor {
  //std::string cname = "lz4";   // "" = none, else a blosc compressor name
    std::string cname = "blosclz";   // "" = none, else a blosc compressor name
    int clevel = 9;
};

// TOZARR_CNAME (zstd|lz4|none),
// TOZARR_CLEVEL; default lz4 / level 1
static Compressor get_compressor()
{
    Compressor c;
    const char* e = std::getenv("TOZARR_CNAME");
    std::string s = e ? e : "lz4";
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char ch) { return char(std::tolower(ch)); });
    if (s.empty() || s == "none") {
        c.cname.clear();
        return c;
    }
    c.cname = s;
    const char* cl = std::getenv("TOZARR_CLEVEL");
    if (cl) {
        try {
            c.clevel = std::stoi(cl);
        } catch (...) {
            c.clevel = 1;
        }
    }
    return c;
}

// blosc codec for libzarr (v3-style named shuffle; typesize/blocksize use
// the dtype-driven defaults, matching the previous metadata).
static std::vector<zarr::CodecSpec> make_codecs(const Compressor& comp)
{
    std::vector<zarr::CodecSpec> codecs;
    if (!comp.cname.empty())
        codecs.push_back(zarr::codec::blosc(comp.cname, comp.clevel, "shuffle"));
    return codecs;
}

// ---------------------------------------------------------------------
static const std::int64_t CHUNK_Z = 64;
static const std::int64_t CHUNK_Y = 64;
static const std::int64_t CHUNK_X = 64;
static const std::int64_t RAM_BYTES = 8LL << 30;   // 8 GB ram_budget
static const std::int64_t SHARD_Z = 3;
static const std::int64_t SHARD_Y = 3;
static const std::int64_t SHARD_X = 3;

struct Dims
{
    std::int64_t z = 0, y = 0, x = 0;         // (Z, Y, X)
    std::int64_t plane() const { return y * x; }
};

// ---------------------------------------------------------------------
// v3 sharding: the shard (outer chunk) shape, whose extents must each be a
// multiple of the inner chunk extent. The number of inner chunks per shard
// along each axis is taken from SHARD_Z / SHARD_Y / SHARD_X.
static Dims make_shard(const Dims& c)
{
    Dims s{ c.z, c.y, c.x };
    s.z = c.z * SHARD_Z;
    s.y = c.y * SHARD_Y;
    s.x = c.x * SHARD_X;
    return s;
}

// ---------------------------------------------------------------------
// Root group user attributes (drishti + multiscales), consumed by the
// drishtiimport zarr reader plugin.
static zarr::json make_group_attributes(std::int64_t nLevels,
                                        std::int64_t dataMin,
                                        std::int64_t dataMax,
                                        const std::string& voxelUnit,
					float vx, float vy, float vz,
                                        const std::string& desc)
{
    zarr::json datasets = zarr::json::array();
    for (std::int64_t lv = 0; lv <= nLevels; ++lv) {
        const std::int64_t s = 1LL << lv;
        datasets.push_back({
            {"path", std::to_string(lv)},
            {"coordinateTransformations",
             zarr::json::array({{{"type", "scale"},
                                 {"scale", zarr::json::array({double(s), double(s), double(s)})}}})}
        });
    }
    return zarr::json{
        {"drishti",
         {{"description", desc},
          {"data_min_max", zarr::json::array({double(dataMin), double(dataMax)})},
          {"voxel_size_xyz", zarr::json::array({vx, vy, vz})},
          {"voxel_unit", voxelUnit}}},
        {"multiscales", zarr::json::array({{{"datasets", datasets}}})}
    };
}

// ---------------------------------------------------------------------
void
ZarrWriter::saveZarr(QWidget* parent,
		     QString outDir,
		     VolumeData* volData,
		     int dmin, int dmax,
		     int wmin, int wmax,
		     int hmin, int hmax)
{
  //------------------------------------------------------
  // -- get saving parameters for processed file
  SavePvlDialog savePvlDialog(parent);
  float vx, vy, vz;
  volData->voxelSize(vx, vy, vz);
  QString desc = volData->description();
  savePvlDialog.setVoxelUnit(3);
  savePvlDialog.setVoxelSize(vx, vy, vz);
  savePvlDialog.setDescription(desc);
  savePvlDialog.exec();

  int spread = savePvlDialog.volumeFilter();
  bool dilateFilter = savePvlDialog.dilateFilter();
  int voxelUnit = savePvlDialog.voxelUnit();
  QString description = savePvlDialog.description();
  savePvlDialog.voxelSize(vx, vy, vz);
  //------------------------------------------------------

  QString voxelUnitString = "no unit";
  if (voxelUnit == 1) voxelUnitString = "angstrom";
  if (voxelUnit == 2) voxelUnitString = "nanometer";
  if (voxelUnit == 3) voxelUnitString = "micron";
  if (voxelUnit == 4) voxelUnitString = "millimeter";
  if (voxelUnit == 5) voxelUnitString = "centimeter";
  if (voxelUnit == 6) voxelUnitString = "meter";
  if (voxelUnit == 7) voxelUnitString = "kilometer";
  
  const Compressor comp = get_compressor(); // using lz4 compression at level 1
  std::cout << "compressor: " << (comp.cname.empty() ? "none" : comp.cname)
	    << " level " << (comp.cname.empty() ? -1 : comp.clevel) << "\n";  
  std::int64_t nLevels = 0; // save only the full resolution
  std::string mode = "nearest"; // do not smooth data at lower levels

  std::shared_ptr<zarr::FilesystemStore> store =
    std::make_shared<zarr::FilesystemStore>(outDir.toStdString(), true);
  zarr::Group root =
    zarr::Group::create(store, "", zarr::ZarrFormat::v3);

  std::int64_t X,Y,Z;
  Z = dmax-dmin+1;
  Y = wmax-wmin+1;
  X = hmax-hmin+1;

  std::cout << "sharding: " << SHARD_Z << "x" << SHARD_Y << "x" << SHARD_X
            << " inner-chunks/shard\n";

  // ---- ask for smoothing mode and the number of pyramid levels ---------
  // Halving rule used by the pyramid loop below (each subsampled level is
  // produced from the previous one).
  const auto next_dims = [](const Dims& p, const std::string& m) {
      return Dims{ m == "nearest" ? (p.z + 1) / 2 : p.z / 2,
                   m == "nearest" ? (p.y + 1) / 2 : p.y / 2,
                   m == "nearest" ? (p.x + 1) / 2 : p.x / 2 };
  };

  // smoothing mode: nearest (no averaging) vs average (2x2x2 mean)
  QStringList modes; modes << "nearest" << "average";
  {
    bool ok;
    QString m = QInputDialog::getItem(parent, "Pyramid smoothing",
                                      "Downsampling mode", modes,
                                      0, false, &ok);
    mode = (ok && m == "average") ? "average" : "nearest";
  }

  // maximum number of levels such that the smallest (last) level still has
  // each dimension > 128 voxels.
  std::int64_t maxLevels = 0;
  {
    Dims cur{ Z, Y, X };
    for (std::int64_t lv = 1; ; ++lv) {
        Dims n = next_dims(cur, mode);
        if (!(n.z > 128 && n.y > 128 && n.x > 128)) break;
        maxLevels = lv;
        cur = n;
    }
  }

  {
    bool ok;
    int levels = QInputDialog::getInt(parent, "Number of levels",
                                      QString("Number of pyramid levels "
                                              "to save (max %1; smallest "
                                              "level > 128^3):")
                                      .arg(maxLevels),
                                      (int)maxLevels, 0, (int)maxLevels, 1,
                                      &ok);
    nLevels = ok ? std::clamp<std::int64_t>(levels, 0, maxLevels) : maxLevels;
  }

  std::vector<zarr::CodecSpec> codecs = make_codecs(comp);

  const auto make_spec = [&](const Dims& d, const Dims& c) {
      const Dims sh = make_shard(c);
      zarr::ArraySpec spec;
      spec.format = zarr::ZarrFormat::v3;
      spec.shape = { (std::uint64_t)d.z, (std::uint64_t)d.y, (std::uint64_t)d.x };
      spec.chunks = { (std::uint64_t)c.z, (std::uint64_t)c.y, (std::uint64_t)c.x };
      spec.shards = { (std::uint64_t)sh.z, (std::uint64_t)sh.y, (std::uint64_t)sh.x };
      spec.dtype = zarr::DataType::of(zarr::DType::uint8);
      spec.codecs = codecs;
      return spec;
  };

  QProgressDialog progress("Saving ZARR",
			   "Cancel",
			   0, 100,
			   parent,
			   Qt::Dialog|Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);
  progress.resize(500, 100);
  progress.move(QCursor::pos());

  // ---- level 0 ------------------------------------------------------
  const std::int64_t planeBytes = Y * X;
  const Dims cd0{std::min(CHUNK_Z, Z), std::min(CHUNK_Y, Y),
		 std::min(CHUNK_X, X)};
  std::int64_t batch = (cd0.z > 1) ? cd0.z : 64;               // 16 | 64
  std::int64_t maxBatch = std::max<std::int64_t>(1, RAM_BYTES / planeBytes);
  batch = std::min({batch, maxBatch, Z});
  const std::int64_t nblk = (Z + batch - 1) / batch;
  std::cout << "level 0: " << Z << " planes in " << nblk << " blocks of "
	    << batch << " (ram_budget=8 GB)\n";

  zarr::Array arr0 =
    root.create_array("0", make_spec(Dims{Z, Y, X}, cd0));

  // data_min_max from the source volume metadata instead of re-scanning all
  // voxels (uint8 => range 0..255, clipped to the observed byte min/max).
  std::int64_t dataMin = std::int64_t(std::max(0.0f, volData->rawMin()));
  std::int64_t dataMax = std::int64_t(std::min(255.0f, volData->rawMax()));
  if (dataMax < dataMin) { std::int64_t t = dataMin; dataMin = dataMax; dataMax = t; }

  // Pipeline the gather (producer: getDepthSlice, main thread) with the
  // costly encode+write (consumer thread). A small fixed pool of buffers is
  // handed off; getDepthSlice is only ever called from the producer, so the
  // source volume is never accessed concurrently.
  struct Job { std::vector<byte> data; std::int64_t z0, nz; };
  const std::int64_t nBuf = std::min<std::int64_t>(3, nblk);
  std::deque<Job> jobs;
  std::mutex mu;
  std::condition_variable cv;
  bool writeDone = false;
  std::thread consumer;
  if (nblk > 1)
    consumer = std::thread([&]() {
      while (true) {
        Job j;
        {
          std::unique_lock<std::mutex> lk(mu);
          cv.wait(lk, [&]{ return !jobs.empty() || writeDone; });
          if (jobs.empty() && writeDone) break;
          j = std::move(jobs.front());
          jobs.pop_front();
        }
        cv.notify_all();
        arr0.write_region({ (std::uint64_t)j.z0, 0, 0 },
                          { (std::uint64_t)j.nz, (std::uint64_t)Y,
                            (std::uint64_t)X },
                          j.data.data(),
                          std::size_t(j.nz) * std::size_t(planeBytes));
      }
    });

  for (std::int64_t b = 0; b < nblk; ++b)
    {
      progress.setValue((int)(100*(float)b/(float)nblk));
      qApp->processEvents();

      const std::int64_t z0 = b * batch;
      const std::int64_t z1 = std::min(Z, z0 + batch);
      const std::int64_t nz = z1 - z0;

      Job j;
      j.z0 = z0; j.nz = nz;
      j.data.assign(std::size_t(nz * planeBytes), 0);
      for (std::int64_t z = z0; z < z1; z++)
	volData->getDepthSlice(int(z), (uchar*)j.data.data() + (z-z0)*planeBytes);

      if (nblk > 1) {
        // hand off to the consumer, blocking while in-flight jobs are full
        // so memory stays bounded (~nBuf buffers).
        {
          std::unique_lock<std::mutex> lk(mu);
          cv.wait(lk, [&]{ return (std::int64_t)jobs.size() < nBuf - 1; });
          jobs.push_back(std::move(j));
        }
        cv.notify_all();
      } else {
        arr0.write_region({ (std::uint64_t)z0, 0, 0 },
                          { (std::uint64_t)nz, (std::uint64_t)Y,
                            (std::uint64_t)X },
                          j.data.data(),
                          std::size_t(nz) * std::size_t(planeBytes));
      }

      if ((b + 1) % std::max<std::int64_t>(1, nblk / 10) == 0 || b == nblk - 1)
	std::cout << "  block " << (b + 1) << "/" << nblk
		  << " (z " << z0 << ":" << z1 << ")\n";
    }

  if (nblk > 1) {
    {
      std::lock_guard<std::mutex> lk(mu);
      writeDone = true;
    }
    cv.notify_all();
    consumer.join();
  }

    // ---- pyramid -----------------------------------------------------
    Dims prev = {Z, Y, X};
    zarr::Array prevArr = arr0;
    for (std::int64_t lv = 1; lv <= nLevels; ++lv) {
        progress.setValue((int)(100*(float)lv/(float)nLevels));
        qApp->processEvents();

        const Dims cur{ (mode == "nearest" ? (prev.z + 1) / 2 : prev.z / 2),
                        (mode == "nearest" ? (prev.y + 1) / 2 : prev.y / 2),
                        (mode == "nearest" ? (prev.x + 1) / 2 : prev.x / 2) };
        const Dims cc{std::min(CHUNK_Z, cur.z), std::min(CHUNK_Y, cur.y),
                      std::min(CHUNK_X, cur.x)};
        std::cout << "  level " << lv << " -> " << cur.z << "x" << cur.y
                  << "x" << cur.x << " (" << mode << ", cached subsample)\n";

        zarr::Array curArr =
          root.create_array(std::to_string(lv), make_spec(cur, cc));

        const std::int64_t nkz = (cur.z + cc.z - 1) / cc.z;
        // one cached read of the previous-level block covering each output
        // z-chunk, then subsample in memory (no per-plane read_region).
        const std::int64_t srcPlaneBytes = prev.plane();
        std::vector<byte> src;
        std::vector<byte> obuf;
        for (std::int64_t kz = 0; kz < nkz; ++kz) {
            const std::int64_t zo0 = kz * cc.z;
            const std::int64_t zo1 = std::min(cur.z, zo0 + cc.z);
            const std::int64_t nzo = zo1 - zo0;

            // source planes covering this output chunk (nearest uses even
            // source rows; average reads the 2x2x2 neighbourhood). Clamp to
            // prev.z: for odd prev dimensions, 2*zo1 can be one past the end
            // of the previous level's last plane.
            const std::int64_t s0 = 2 * zo0;
            const std::int64_t sN = std::min(prev.z, 2 * zo1);
            const std::int64_t nsrc = sN - s0;
            src.assign(std::size_t(nsrc) * std::size_t(srcPlaneBytes), 0);
            prevArr.read_region({ (std::uint64_t)s0, 0, 0 },
                                { (std::uint64_t)nsrc, (std::uint64_t)prev.y,
                                  (std::uint64_t)prev.x },
                                src.data(), src.size());

            obuf.assign(std::size_t(nzo * cur.plane()), 0);
            byte* oplane = obuf.data();
            for (std::int64_t o = 0; o < nzo; ++o) {
                const std::int64_t srow = 2 * o;      // index into src planes
                const byte* p0 = src.data() + std::size_t(srow) * srcPlaneBytes;
                byte* dst = oplane + std::size_t(o * cur.plane());
                if (mode == "nearest") {
                    for (std::int64_t y2 = 0; y2 < cur.y; ++y2)
                        for (std::int64_t x2 = 0; x2 < cur.x; ++x2)
                            dst[std::size_t(y2 * cur.x + x2)] =
                                p0[std::size_t((2 * y2) * prev.x + 2 * x2)];
                } else {
                    const byte* p1 = (srow + 1 < nsrc)
                        ? (src.data() + std::size_t(srow + 1) * srcPlaneBytes)
                        : p0;
                    for (std::int64_t y2 = 0; y2 < cur.y; ++y2) {
                        const std::int64_t r0 = (2 * y2) * prev.x;
                        const std::int64_t r1 = std::min(prev.y - 1, 2 * y2 + 1) * prev.x;
                        for (std::int64_t x2 = 0; x2 < cur.x; ++x2) {
                            const std::int64_t c0 = 2 * x2;
                            const std::int64_t c1 = std::min(prev.x - 1, 2 * x2 + 1);
                            dst[std::size_t(y2 * cur.x + x2)] = byte(
                                (double(p0[std::size_t(r0 + c0)]) +
                                 double(p0[std::size_t(r0 + c1)]) +
                                 double(p0[std::size_t(r1 + c0)]) +
                                 double(p0[std::size_t(r1 + c1)]) +
                                 double(p1[std::size_t(r0 + c0)]) +
                                 double(p1[std::size_t(r0 + c1)]) +
                                 double(p1[std::size_t(r1 + c0)]) +
                                 double(p1[std::size_t(r1 + c1)])) * 0.125);
                        }
                    }
                }
            }
            curArr.write_region({ (std::uint64_t)zo0, 0, 0 },
                                { (std::uint64_t)nzo, (std::uint64_t)cur.y,
                                  (std::uint64_t)cur.x },
                                obuf.data(), obuf.size());
        }
        prev = cur;
        prevArr = curArr;   // next level's source is this level
    }

    // ---- root zarr.json (drishti + multiscales attributes) -----------
    root.set_attributes(make_group_attributes(nLevels, dataMin, dataMax,
					      voxelUnitString.toStdString(),
					      vx, vy, vz,
					      description.toStdString()));

    std::cout << "min/max: " << dataMin << " " << dataMax << "\n";
    std::cout << "done " << outDir.toStdString() << "\n";

    QMessageBox::information(parent, "Zarr", "Saved to "+outDir);
}
