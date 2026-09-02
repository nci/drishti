#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include <libzarr/libzarr.hpp>
#include <libzarr/adapters/filesystem_store.hpp>

#include "savepvldialog.h"
#include "zarrwriter.h"

#include <QMessageBox>
#include <QProgressDialog>

using byte = std::uint8_t;

// ---------------------------------------------------------------------
struct Compressor {
    std::string cname = "lz4";   // "" = none, else a blosc compressor name
    int clevel = 1;
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
static const std::int64_t CHUNK_Z = 16;
static const std::int64_t CHUNK_Y = 256;
static const std::int64_t CHUNK_X = 256;
static const std::int64_t RAM_BYTES = 8LL << 30;   // 8 GB ram_budget

struct Dims
{
    std::int64_t z = 0, y = 0, x = 0;         // (Z, Y, X)
    std::int64_t plane() const { return y * x; }
};

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

  std::vector<zarr::CodecSpec> codecs = make_codecs(comp);

  const auto make_spec = [&](const Dims& d, const Dims& c) {
      zarr::ArraySpec spec;
      spec.format = zarr::ZarrFormat::v3;
      spec.shape = { (std::uint64_t)d.z, (std::uint64_t)d.y, (std::uint64_t)d.x };
      spec.chunks = { (std::uint64_t)c.z, (std::uint64_t)c.y, (std::uint64_t)c.x };
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

  std::int64_t dataMin = 255, dataMax = 0;
  std::vector<byte> buf;
  for (std::int64_t b = 0; b < nblk; ++b)
    {
      progress.setValue((int)(100*(float)b/(float)nblk));
      qApp->processEvents();

      const std::int64_t z0 = b * batch;
      const std::int64_t z1 = std::min(Z, z0 + batch);
      const std::int64_t nz = z1 - z0;
      buf.assign(std::size_t(nz * planeBytes), 0);
      for (std::int64_t z = z0; z < z1; z++)
	volData->getDepthSlice(int(z), (uchar*)buf.data() + (z-z0)*planeBytes);
      for (const byte v : buf)
	{
	  if (v < dataMin) dataMin = v;
	  if (v > dataMax) dataMax = v;
	}
      // full-size (fill-padded) chunks are assembled by libzarr, including
      // read-modify-write of z-chunks split across block boundaries.
      arr0.write_region({ (std::uint64_t)z0, 0, 0 },
                        { (std::uint64_t)nz, (std::uint64_t)Y,
                          (std::uint64_t)X },
                        buf.data(), std::size_t(nz) * std::size_t(planeBytes));
      if ((b + 1) % std::max<std::int64_t>(1, nblk / 10) == 0 || b == nblk - 1)
	std::cout << "  block " << (b + 1) << "/" << nblk
		  << " (z " << z0 << ":" << z1 << ")\n";
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
                  << "x" << cur.x << " (serial, " << mode << ")\n";

        zarr::Array curArr =
          root.create_array(std::to_string(lv), make_spec(cur, cc));

        const std::int64_t nkz = (cur.z + cc.z - 1) / cc.z;
        std::vector<byte> obuf;
        for (std::int64_t kz = 0; kz < nkz; ++kz) {
            const std::int64_t zo0 = kz * cc.z;
            const std::int64_t zo1 = std::min(cur.z, zo0 + cc.z);
            const std::int64_t nzo = zo1 - zo0;
            obuf.assign(std::size_t(nzo * cur.plane()), 0);
            byte* oplane = obuf.data();
            for (std::int64_t o = 0; o < nzo; ++o) {
                const std::int64_t i = 2 * (zo0 + o);
                if (mode == "nearest") {
                    std::vector<byte> p0(std::size_t(prev.plane()));
                    prevArr.read_region({ (std::uint64_t)i, 0, 0 },
                                        { 1, (std::uint64_t)prev.y,
                                          (std::uint64_t)prev.x },
                                        p0.data(), p0.size());
                    byte* dst = oplane + std::size_t(o * cur.plane());
                    for (std::int64_t y2 = 0; y2 < cur.y; ++y2)
                        for (std::int64_t x2 = 0; x2 < cur.x; ++x2)
                            dst[std::size_t(y2 * cur.x + x2)] =
                                p0[std::size_t((2 * y2) * prev.x + 2 * x2)];
                } else {
                    std::vector<byte> p0(std::size_t(prev.plane()));
                    std::vector<byte> p1(std::size_t(prev.plane()));
                    prevArr.read_region({ (std::uint64_t)i, 0, 0 },
                                        { 1, (std::uint64_t)prev.y,
                                          (std::uint64_t)prev.x },
                                        p0.data(), p0.size());
                    prevArr.read_region({ (std::uint64_t)(i + 1), 0, 0 },
                                        { 1, (std::uint64_t)prev.y,
                                          (std::uint64_t)prev.x },
                                        p1.data(), p1.size());
                    byte* dst = oplane + std::size_t(o * cur.plane());
                    for (std::int64_t y2 = 0; y2 < cur.y; ++y2) {
                        const std::int64_t r0 = (2 * y2) * prev.x;
                        const std::int64_t r1 = (2 * y2 + 1) * prev.x;
                        for (std::int64_t x2 = 0; x2 < cur.x; ++x2) {
                            const std::int64_t c0 = 2 * x2;
                            dst[std::size_t(y2 * cur.x + x2)] = byte(
                                (double(p0[std::size_t(r0 + c0)]) +
                                 double(p0[std::size_t(r0 + c0 + 1)]) +
                                 double(p0[std::size_t(r1 + c0)]) +
                                 double(p0[std::size_t(r1 + c0 + 1)]) +
                                 double(p1[std::size_t(r0 + c0)]) +
                                 double(p1[std::size_t(r0 + c0 + 1)]) +
                                 double(p1[std::size_t(r1 + c0)]) +
                                 double(p1[std::size_t(r1 + c0 + 1)])) * 0.125);
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
