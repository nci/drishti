#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <io.h>
#include "blosc.h"
#include "savepvldialog.h"
#include "volumefilemanager.h"
#include "zarrwriter.h"

#include <QMessageBox>

namespace fs = std::filesystem;

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

// blosc frame (c-blosc v1) exactly as numcodecs/zarr BloscCodec produces
static std::vector<byte> compress_payload(const std::vector<byte>& in,
                                          const Compressor& comp)
{
    if (comp.cname.empty()) return in;
    blosc_set_compressor(comp.cname.c_str());
    blosc_set_blocksize(0);
    blosc_set_nthreads(4);
    std::vector<byte> out(in.size() + BLOSC_MAX_OVERHEAD);
    int n = blosc_compress(comp.clevel, 1 /*doshuffle=shuffle*/,
                           1 /*typesize*/, in.size(), in.data(), out.data(),
                           out.size());
    if (n == 0)  // incompressible data -> clevel 0 copy frame is always valid
        n = blosc_compress(0, 1, 1, in.size(), in.data(), out.data(),
                           out.size());
    if (n <= 0) throw std::runtime_error("blosc_compress failed");
    out.resize(std::size_t(n));
    return out;
}

static void decompress_payload(const byte* src, std::size_t n, byte* dst,
                               std::size_t dstSize)
{
    int r = blosc_decompress(src, dst, dstSize);
    if (r < 0 || std::size_t(r) != dstSize)
        throw std::runtime_error("blosc_decompress failed");
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

static std::uint32_t read_le32(std::istream& f)
{
    std::uint8_t b[4];
    f.read(reinterpret_cast<char*>(b), 4);
    return std::uint32_t(b[0]) | std::uint32_t(b[1]) << 8 |
           std::uint32_t(b[2]) << 16 | std::uint32_t(b[3]) << 24;
}

static void write_file(const fs::path& p, const void* data, std::size_t n)
{
    fs::create_directories(p.parent_path());
    std::ofstream out(p, std::ios::binary | std::ios::trunc);
    if (!out) throw std::runtime_error("cannot write " + p.string());
    out.write(reinterpret_cast<const char*>(data), std::streamsize(n));
    if (!out) throw std::runtime_error("short write " + p.string());
}

static void write_json_file(const fs::path& p, const std::string& content)
{
    write_file(p, content.data(), content.size());
}

// JSON string literal with escaping (description/voxel_unit are free text)
static std::string json_quote(const std::string& s) {
    std::string r = "\"";
    for (unsigned char c : s) {
        switch (c) {
            case '"':  r += "\\\""; break;
            case '\\': r += "\\\\"; break;
            case '\n': r += "\\n";  break;
            case '\r': r += "\\r";  break;
            case '\t': r += "\\t";  break;
            case '\b': r += "\\b";  break;
            case '\f': r += "\\f";  break;
            default:
                if (c < 0x20) {
                    char buf[16];
                    std::snprintf(buf, sizeof buf, "\\u%04x", int(c));
                    r += buf;
                } else {
                    r += char(c);
                }
        }
    }
    return r + "\"";
}

// re-format a single-line JSON document with indentation (mirrors
// python json.dumps(indent=N): ", " separators, "\n" + N spaces per level)
static std::string pretty_json(const std::string& s, int indentW = 4)
{
    std::string out;
    out.reserve(s.size() + 64);
    int indent = 0;
    bool inStr = false;
    for (std::size_t i = 0; i < s.size(); ++i) {
        const char c = s[i];
        if (inStr) {
            out += c;
            if (c == '\\' && i + 1 < s.size()) out += s[++i];  // keep escape
            else if (c == '"') inStr = false;
            continue;
        }
        if (c == '"') { inStr = true; out += c; continue; }
        if (c == '{' || c == '[') {
            out += c;
            indent += indentW;
            out += '\n';
            out.append(std::size_t(indent), ' ');
        } else if (c == '}' || c == ']') {
            indent -= indentW;
            out += '\n';
            out.append(std::size_t(indent), ' ');
            out += c;
        } else if (c == ',') {
            out += c;
            out += '\n';
            out.append(std::size_t(indent), ' ');
        } else if (c == ':') {
            out += ": ";
        } else {
            out += c;
        }
    }
    return out;
}

// zarr v3 array metadata (uint8, default "/" chunk keys)
static std::string array_meta_json(const Dims& d, const Dims& c,
                                   const Compressor& comp)
{
    std::string codecs;
    if (comp.cname.empty()) {
        codecs = "[{\"name\":\"bytes\"}]";
    } else {
        codecs = "[{\"name\":\"bytes\"},"
                 "{\"name\":\"blosc\",\"configuration\":{\"typesize\":1,"
                 "\"cname\":" + json_quote(comp.cname) + ",\"clevel\":" +
                 std::to_string(comp.clevel) +
                 ",\"shuffle\":\"shuffle\",\"blocksize\":0}}]";
    }
    return std::string(
        "{\"shape\":[" + std::to_string(d.z) + "," + std::to_string(d.y) + "," + std::to_string(d.x) + "],"
        "\"data_type\":\"uint8\","
        "\"chunk_grid\":{\"name\":\"regular\",\"configuration\":{\"chunk_shape\":["
        + std::to_string(c.z) + "," + std::to_string(c.y) + "," + std::to_string(c.x) + "]}},"
        "\"chunk_key_encoding\":{\"name\":\"default\",\"configuration\":{\"separator\":\"/\"}},"
        "\"fill_value\":0,\"codecs\":" + codecs + ","
        "\"attributes\":{},\"zarr_format\":3,\"node_type\":\"array\","
        "\"storage_transformers\":[]}");
}

static fs::path chunk_key(const fs::path& arr, std::int64_t kz,
                          std::int64_t ky, std::int64_t kx) {
    return arr / "c" / std::to_string(kz) / std::to_string(ky) /
           std::to_string(kx);
}

// ---------------------------------------------------------------------
// reads one z-plane (Y*X bytes) of the zarr array `arr` (shape d, chunks c)
static std::vector<byte> read_plane(const fs::path& arr,
                                    const Dims& d, const Dims& c,
                                    std::int64_t planeIndex,
                                    const Compressor& comp) {
    std::vector<byte> plane(std::size_t(d.plane()));
    const std::int64_t kz = planeIndex / c.z;
    const std::int64_t inz = planeIndex % c.z;   // z-offset inside its chunk
    const std::int64_t nky = (d.y + c.y - 1) / c.y;
    const std::int64_t nkx = (d.x + c.x - 1) / c.x;
    const std::size_t chunkBytes = std::size_t(c.z * c.y * c.x);
    std::vector<byte> chunk(chunkBytes);         // full, padded
    for (std::int64_t ky = 0; ky < nky; ++ky) {
        const std::int64_t y0 = ky * c.y;
        const std::int64_t rcy = std::min(c.y, d.y - y0);
        for (std::int64_t kx = 0; kx < nkx; ++kx) {
            const std::int64_t x0 = kx * c.x;
            const std::int64_t rcx = std::min(c.x, d.x - x0);
            const fs::path key = chunk_key(arr, kz, ky, kx);
            const std::size_t fsz = std::size_t(fs::file_size(key));
            std::ifstream in(key, std::ios::binary);
            if (!in) throw std::runtime_error("cannot open chunk");
            if (comp.cname.empty()) {
                if (fsz != chunkBytes) throw std::runtime_error("bad chunk size");
                in.read(reinterpret_cast<char*>(chunk.data()),
                        std::streamsize(chunkBytes));
            } else {
                std::vector<byte> payload(fsz);
                in.read(reinterpret_cast<char*>(payload.data()),
                        std::streamsize(fsz));
                decompress_payload(payload.data(), fsz, chunk.data(),
                                   chunkBytes);
            }
            const byte* zp = chunk.data() + std::size_t(inz * c.y * c.x);
            for (std::int64_t ry = 0; ry < rcy; ++ry)
                std::memcpy(&plane[std::size_t((y0 + ry) * d.x + x0)],
                            zp + std::size_t(ry * c.x), std::size_t(rcx));
        }
    }
    return plane;
}

// ---------------------------------------------------------------------
// writes nplanes planes starting at z0 into full-size (padded) v3 chunks,
// handling arbitrary plane spans that may cross z-chunk boundaries
static void write_slab(const fs::path& arr,
                       const Dims& d, const Dims& c,
                       std::int64_t z0, std::int64_t nplanes,
                       const std::vector<byte>& buf,
                       const Compressor& comp)
{
    const std::int64_t nky = (d.y + c.y - 1) / c.y;
    const std::int64_t nkx = (d.x + c.x - 1) / c.x;
    std::vector<byte> chunk(std::size_t(c.z * c.y * c.x), 0);  // fill pad
    for (std::int64_t kz = z0 / c.z; kz <= (z0 + nplanes - 1) / c.z; ++kz) {
        const std::int64_t gza = std::max(z0, kz * c.z);
        const std::int64_t gzb = std::min(z0 + nplanes, (kz + 1) * c.z);
        for (std::int64_t ky = 0; ky < nky; ++ky) {
            const std::int64_t y0 = ky * c.y;
            const std::int64_t rcy = std::min(c.y, d.y - y0);
            for (std::int64_t kx = 0; kx < nkx; ++kx) {
                const std::int64_t x0 = kx * c.x;
                const std::int64_t rcx = std::min(c.x, d.x - x0);
                std::fill(chunk.begin(), chunk.end(), 0);
                for (std::int64_t gz = gza; gz < gzb; ++gz) {
                    const std::int64_t zloc = gz - kz * c.z;
                    const byte* plane =
                        buf.data() + std::size_t((gz - z0) * d.plane());
                    for (std::int64_t ry = 0; ry < rcy; ++ry)
                        std::memcpy(&chunk[std::size_t(zloc * c.y * c.x +
                                                       ry * c.x)],
                                    plane + std::size_t((y0 + ry) * d.x + x0),
                                    std::size_t(rcx));
                }
                std::vector<byte> payload = compress_payload(chunk, comp);
                write_file(chunk_key(arr, kz, ky, kx),
                           payload.data(), payload.size());
            }
        }
    }
}

// ---------------------------------------------------------------------
static std::string zarr_json_meta(std::int64_t dataMin, std::int64_t dataMax,
                                  std::int64_t nLevels,
                                  const std::string& voxelUnit,
                                  const std::string& desc)
{
    std::string datasets;
    for (std::int64_t lv = 0; lv <= nLevels; ++lv) {
        if (lv) datasets += ",";
        const std::int64_t s = 1LL << lv;
        datasets += "{\"path\":\"" + std::to_string(lv) +
                    "\",\"coordinateTransformations\":[{\"type\":\"scale\","
                    "\"scale\":[" + std::to_string(s) + ".0," +
                    std::to_string(s) + ".0," + std::to_string(s) + ".0]}]}";
    }
    return
        "{\"attributes\":{"
        "\"drishti\":{\"description\":" + json_quote(desc) +
        ",\"data_min_max\":[" + std::to_string(dataMin) + ".0," +
        std::to_string(dataMax) + ".0" +
        "],\"voxel_size_xyz\":[1.0,1.0,1.0],\"voxel_unit\":" +
        json_quote(voxelUnit) + "}," +
        "\"multiscales\":[{\"datasets\":[" + datasets + "]}]},"
        "\"zarr_format\":3,\"node_type\":\"group\"}";
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


  fs::create_directories(outDir.toStdString());

  std::int64_t X,Y,Z;
  Z = dmax-dmin+1;
  Y = wmax-wmin+1;
  X = hmax-hmin+1;

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

  const fs::path arr0 = outDir.toStdString() + "/0";
  fs::create_directories(arr0);
  write_json_file(arr0 / "zarr.json",
		  pretty_json(array_meta_json(Dims{Z, Y, X}, cd0, comp), 2));

  std::int64_t dataMin = 255, dataMax = 0;
  std::vector<byte> buf;
  uchar* val = new uchar[planeBytes];
  for (std::int64_t b = 0; b < nblk; ++b)
    {
      const std::int64_t z0 = b * batch;
      const std::int64_t z1 = std::min(Z, z0 + batch);
      const std::int64_t nz = z1 - z0;
      buf.assign(std::size_t(nz * planeBytes), 0);
      for (int z=z0; z<z1; z++)
	volData->getDepthSlice(z, buf.data()+(z-z0)*planeBytes);
      for (const byte v : buf)
	{
	  if (v < dataMin) dataMin = v;
	  if (v > dataMax) dataMax = v;
	}
      write_slab(arr0, Dims{Z, Y, X}, cd0, z0, nz, buf, comp);
      if ((b + 1) % std::max<std::int64_t>(1, nblk / 10) == 0 || b == nblk - 1)
	std::cout << "  block " << (b + 1) << "/" << nblk
		  << " (z " << z0 << ":" << z1 << ")\n";
    }

    // ---- pyramid -----------------------------------------------------
    Dims prev = {Z, Y, X};
    Dims cprev = cd0;
    fs::path arrPrev = arr0;
    for (std::int64_t lv = 1; lv <= nLevels; ++lv) {
        const Dims cur{ (mode == "nearest" ? (prev.z + 1) / 2 : prev.z / 2),
                        (mode == "nearest" ? (prev.y + 1) / 2 : prev.y / 2),
                        (mode == "nearest" ? (prev.x + 1) / 2 : prev.x / 2) };
        const Dims cc{std::min(CHUNK_Z, cur.z), std::min(CHUNK_Y, cur.y),
                      std::min(CHUNK_X, cur.x)};
        std::cout << "  level " << lv << " -> " << cur.z << "x" << cur.y
                  << "x" << cur.x << " (serial, " << mode << ")\n";

        const fs::path arrCur = outDir.toStdString() + "/" + std::to_string(lv);
        fs::create_directories(arrCur);
        write_json_file(arrCur / "zarr.json",
                    pretty_json(array_meta_json(cur, cc, comp), 2));

        const std::int64_t nkz = (cur.z + cc.z - 1) / cc.z;
        std::vector<byte> obuf;
        for (std::int64_t kz = 0; kz < nkz; ++kz) {
            const std::int64_t zo0 = kz * cc.z;
            const std::int64_t zo1 = std::min(cur.z, zo0 + cc.z);
            const std::int64_t nzo = zo1 - zo0;
            obuf.assign(std::size_t(nzo * cur.plane()), 0);
            const byte* oplane = obuf.data();
            for (std::int64_t o = 0; o < nzo; ++o) {
                const std::int64_t i = 2 * (zo0 + o);
                if (mode == "nearest") {
                    const auto p0 = read_plane(arrPrev, prev, cprev, i, comp);
                    byte* dst = const_cast<byte*>(oplane) + std::size_t(o * cur.plane());
                    for (std::int64_t y2 = 0; y2 < cur.y; ++y2)
                        for (std::int64_t x2 = 0; x2 < cur.x; ++x2)
                            dst[std::size_t(y2 * cur.x + x2)] =
                                p0[std::size_t((2 * y2) * prev.x + 2 * x2)];
                } else {
                    const auto p0 = read_plane(arrPrev, prev, cprev, i, comp);
                    const auto p1 = read_plane(arrPrev, prev, cprev, i + 1, comp);
                    byte* dst = const_cast<byte*>(oplane) + std::size_t(o * cur.plane());
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
            write_slab(arrCur, cur, cc, zo0, nzo, obuf, comp);
        }
        prev = cur;
        cprev = cc;
        arrPrev = arrCur;   // next level's source is this level
    }

    // ---- zarr.json ---------------------------------------------------
    write_json_file(outDir.toStdString() + "/zarr.json",
                    pretty_json(zarr_json_meta(dataMin, dataMax,
					       nLevels,
					       voxelUnitString.toStdString(),
					       description.toStdString()), 4));

    std::cout << "min/max: " << dataMin << " " << dataMax << "\n";
    std::cout << "done " << outDir.toStdString() << "\n";

    QMessageBox::information(parent, "Zarr", "Saved to "+outDir);
}
