#include "../../../common/src/pvlmanifest.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QSaveFile>
#include <QTemporaryDir>

#include <cstring>
#include <iostream>

namespace
{
int fail(const QString& message)
{
  std::cerr << message.toStdString() << std::endl;
  return 1;
}

bool writeSlab(const QString& path, int slices, int width, int height,
               int voxelType = 0, int headerSize = 13)
{
  QFile file(path);
  if (!file.open(QFile::WriteOnly | QFile::Truncate))
    return false;
  const qint32 values[] = { slices, width, height };
  const char type = static_cast<char>(voxelType == 5 ? 8 : voxelType);
  QByteArray header(headerSize, '\0');
  header[0] = type;
  std::memcpy(header.data()+1, values, sizeof(values));
  if (file.write(header) != header.size() ||
      headerSize < 13)
    return false;
  const int bytesPerVoxel = (voxelType == 2 || voxelType == 3) ? 2 :
                            ((voxelType >= 4) ? 4 : 1);
  QByteArray pixels(slices*width*height*bytesPerVoxel, '\0');
  return file.write(pixels) == pixels.size() && file.flush();
}

bool writeHeader(const QString& path, const QString& names,
                 const QString& extra = QString(),
                 const QString& geometry = "3 2 2")
{
  QSaveFile file(path);
  if (!file.open(QIODevice::WriteOnly))
    return false;
  const QByteArray xml = QString(
    "<PvlDotNcFileHeader>"
    "<pvlvoxeltype>unsigned char</pvlvoxeltype>"
    "<gridsize>%3</gridsize><slabsize>2</slabsize>"
    "<pvlnames>%1</pvlnames>%2"
    "</PvlDotNcFileHeader>").arg(names).arg(extra).arg(geometry).toUtf8();
  return file.write(xml) == xml.size() && file.commit();
}

bool writeColorHeader(const QString& path, const QString& pvlVoxelType,
                      const QString& rawVoxelType = QString())
{
  QSaveFile file(path);
  if (!file.open(QIODevice::WriteOnly))
    return false;
  const QByteArray xml = QString(
    "<PvlDotNcFileHeader>"
    "<voxeltype>%1</voxeltype><pvlvoxeltype>%2</pvlvoxeltype>"
    "<gridsize>3 2 2</gridsize><slabsize>2</slabsize>"
    "</PvlDotNcFileHeader>").arg(rawVoxelType.isEmpty() ? pvlVoxelType : rawVoxelType)
      .arg(pvlVoxelType).toUtf8();
  return file.write(xml) == xml.size() && file.commit();
}

bool writeColorHeaderWithScalarNames(const QString& path)
{
  QSaveFile file(path);
  if (!file.open(QIODevice::WriteOnly))
    return false;
  const QByteArray xml =
    "<PvlDotNcFileHeader>"
    "<voxeltype>RGB</voxeltype><pvlvoxeltype>RGB</pvlvoxeltype>"
    "<gridsize>3 2 2</gridsize><slabsize>2</slabsize>"
    "<pvlnames><name>colour.pvl.nc.001</name></pvlnames>"
    "</PvlDotNcFileHeader>";
  return file.write(xml) == xml.size() && file.commit();
}

bool expectReject(const QString& header, const QString& needle)
{
  PvlManifest manifest;
  return !PvlManifestParser::parse(header, manifest, true) &&
         manifest.error.contains(needle);
}
}

int main(int argc, char **argv)
{
  QCoreApplication application(argc, argv);
  QTemporaryDir temporary;
  if (!temporary.isValid())
    return fail("cannot create fixture directory");

  const QString header = QDir(temporary.path()).filePath("volume header.pvl.nc");
  if (!writeSlab(header+".001", 2, 2, 2) ||
      !writeSlab(header+".002", 1, 2, 2) ||
      !writeHeader(header, "<name>volume header.pvl.nc.001</name>"
                          "<name>volume header.pvl.nc.002</name>",
                   "<sourceorder><file>slice 10.tif</file>"
                   "<file>slice 2.tif</file></sourceorder>"))
    return fail("cannot create PVL fixture");

  PvlManifest manifest;
  if (!PvlManifestParser::parse(header, manifest, true) ||
      manifest.pvlNames.count() != 2 ||
      manifest.pvlNames.at(0) != QFileInfo(header+".001").absoluteFilePath() ||
      manifest.sourceOrder != (QStringList() << "slice 10.tif" << "slice 2.tif"))
    return fail("structured PVL manifest did not validate");

  const QString unicodeHeader = QDir(temporary.path()).filePath("体积 header.pvl.nc");
  if (!writeSlab(unicodeHeader+".001", 2, 2, 2) ||
      !writeSlab(unicodeHeader+".002", 1, 2, 2) ||
      !writeHeader(unicodeHeader, "<name>体积 header.pvl.nc.001</name>"
                   "<name>体积 header.pvl.nc.002</name>"))
    return fail("cannot create Unicode PVL fixture");
  if (!PvlManifestParser::parse(unicodeHeader, manifest, true) ||
      !manifest.pvlNames.at(0).contains("体积 header"))
    return fail("Unicode PVL path was not preserved");

  const QString fallback = QDir(temporary.path()).filePath("fallback.pvl.nc");
  if (!writeSlab(fallback+".001", 2, 2, 2) ||
      !writeSlab(fallback+".002", 1, 2, 2) ||
      !writeHeader(fallback, QString()))
    return fail("cannot create fallback fixture");
  if (!PvlManifestParser::parse(fallback, manifest, true) ||
      manifest.pvlNames.count() != 2)
    return fail("PVL fallback slab derivation failed");

  if (!writeHeader(fallback, "<name>fallback.pvl.nc.001</name>"))
    return fail("cannot create malformed manifest fixture");
  if (PvlManifestParser::parse(fallback, manifest, false) ||
      !manifest.error.contains("expected 2"))
    return fail("short PVL manifest was accepted");

  if (!writeHeader(fallback, "<name>fallback.pvl.nc.001</name>"
                   "<name>fallback.pvl.nc.001</name>") ||
      PvlManifestParser::parse(fallback, manifest, false) ||
      !manifest.error.contains("duplicate"))
    return fail("duplicate PVL slab was accepted");

  if (!writeHeader(fallback, "<name>fallback.pvl.nc.001</name>"
                   "<name>fallback.pvl.nc.002</name>",
                   "<gridsize><nested>3 2 2</nested></gridsize>") ||
      PvlManifestParser::parse(fallback, manifest, false))
    return fail("nested gridsize was accepted");

  if (!writeHeader(fallback, "<name>fallback.pvl.nc.001</name>"
                   "<name>fallback.pvl.nc.002</name>",
                   "<unknown>value</unknown>") ||
      PvlManifestParser::parse(fallback, manifest, false) ||
      !manifest.error.contains("unknown"))
    return fail("unknown PVL field was accepted");

  if (!writeHeader(fallback, "<name>fallback.pvl.nc.001</name>"
                   " fallback.pvl.nc.002") ||
      PvlManifestParser::parse(fallback, manifest, false) ||
      !manifest.error.contains("mix structured"))
    return fail("mixed structured and legacy PVL names were accepted");

  if (!writeHeader(fallback, "<name><nested>fallback.pvl.nc.001</nested></name>"
                   "<name>fallback.pvl.nc.002</name>") ||
      PvlManifestParser::parse(fallback, manifest, false) ||
      !manifest.error.contains("text only"))
    return fail("nested PVL name content was accepted");

  const QString longName(512, QChar('x'));
  if (!writeHeader(fallback, QString("<name>%1</name>"
                                    "<name>fallback.pvl.nc.002</name>")
                   .arg(longName)) ||
      !PvlManifestParser::parse(fallback, manifest, false) ||
      manifest.pvlNames.at(0).size() < 512)
    return fail("long structured slab name was not preserved");

  const QString rawHeader = QDir(temporary.path()).filePath("raw source.pvl.nc");
  if (!writeSlab(rawHeader+".001", 2, 2, 2) ||
      !writeSlab(rawHeader+".002", 1, 2, 2) ||
      !writeSlab(QDir(temporary.path()).filePath("raw data.001"), 2, 2, 2, 2) ||
      !writeSlab(QDir(temporary.path()).filePath("raw data.002"), 1, 2, 2, 2) ||
      !writeHeader(rawHeader,
                   "<name>raw source.pvl.nc.001</name>"
                   "<name>raw source.pvl.nc.002</name>",
                   "<voxeltype>unsigned short</voxeltype>"
                   "<rawheadersize>13</rawheadersize>"
                   "<rawnames><name>raw data.001</name>"
                   "<name>raw data.002</name></rawnames>"))
    return fail("cannot create RAW manifest fixture");
  if (!PvlManifestParser::parse(rawHeader, manifest, true) ||
      manifest.rawNames.count() != 2 || manifest.rawVoxelType != 2)
    return fail("RAW manifest did not validate");

  if (!writeSlab(QDir(temporary.path()).filePath("raw data.001"),
                 2, 3, 2, 2) ||
      PvlManifestParser::parse(rawHeader, manifest, true) ||
      !manifest.error.contains("RAW slab"))
    return fail("RAW geometry mismatch was accepted");
  if (!writeSlab(QDir(temporary.path()).filePath("raw data.001"),
                 2, 2, 2, 0) ||
      PvlManifestParser::parse(rawHeader, manifest, true) ||
      !manifest.error.contains("RAW slab"))
    return fail("RAW voxel type mismatch was accepted");
  if (!writeSlab(QDir(temporary.path()).filePath("raw data.001"),
                 2, 2, 2, 2))
    return fail("cannot restore RAW slab fixture");

  QFile::remove(QDir(temporary.path()).filePath("raw data.002"));
  if (PvlManifestParser::parse(rawHeader, manifest, true) ||
      !manifest.error.contains("RAW slab"))
    return fail("missing RAW slab was accepted");

  const QString truncated = QDir(temporary.path()).filePath("truncated.pvl.nc");
  if (!writeSlab(truncated+".001", 2, 2, 2) ||
      !writeSlab(truncated+".002", 1, 2, 2) ||
      !writeHeader(truncated, "<name>truncated.pvl.nc.001</name>"
                   "<name>truncated.pvl.nc.002</name>"))
    return fail("cannot create truncated fixture");
  QFile broken(truncated+".002");
  if (!broken.open(QIODevice::ReadWrite) || !broken.resize(13+3))
    return fail("truncated PVL slab was accepted");
  broken.close();
  if (!expectReject(truncated, "PVL slab"))
    return fail("truncated PVL slab was accepted");

  if (!writeSlab(truncated+".002", 1, 2, 2, 0) ||
      !broken.open(QIODevice::ReadWrite))
    return fail("cannot restore binary-header fixture");
  const bool wroteWrongType = broken.seek(0) &&
                              broken.putChar(static_cast<char>(2));
  broken.close();
  if (!wroteWrongType || !expectReject(truncated, "PVL slab"))
    return fail("wrong binary voxel type was accepted");

  if (!writeHeader(truncated, "<name>truncated.pvl.nc.001</name>"
                   "<name>truncated.pvl.nc.002</name>", QString(), "4 2 2") ||
      !expectReject(truncated, "PVL slab"))
    return fail("inconsistent PVL geometry was accepted");

  if (!writeHeader(truncated, "<name>truncated.pvl.nc.001</name>"
                   "<name>truncated.pvl.nc.002</name>"
                   "<name>truncated.pvl.nc.003</name>") ||
      PvlManifestParser::parse(truncated, manifest, false) ||
      !manifest.error.contains("expected 2"))
    return fail("extra PVL slab name was accepted");

  if (!writeHeader(truncated, "<name>truncated.pvl.nc.001</name>"
                   "<name>truncated.pvl.nc.002</name>",
                   "<rawmap>0 255</rawmap><pvlmap>0</pvlmap>") ||
      PvlManifestParser::parse(truncated, manifest, false) ||
      !manifest.error.contains("equal lengths"))
    return fail("mapping lists with different lengths were accepted");

  if (!writeHeader(truncated, "<name>truncated.pvl.nc.001</name>"
                   "<name>truncated.pvl.nc.002</name>",
                   "<voxeltype>not-a-voxel-type</voxeltype>") ||
      PvlManifestParser::parse(truncated, manifest, false) ||
      !manifest.error.contains("unsupported PVL voxel type"))
    return fail("unsupported PVL voxel type was accepted");

  const QString colorHeader = QDir(temporary.path()).filePath("colour.pvl.nc");
  const QStringList colorBases = QStringList()
    << colorHeader.left(colorHeader.size()-6) + "red"
    << colorHeader.left(colorHeader.size()-6) + "green"
    << colorHeader.left(colorHeader.size()-6) + "blue";
  for (int i = 0; i < colorBases.count(); ++i)
    if (!writeSlab(colorBases.at(i)+".001", 2, 2, 2) ||
        !writeSlab(colorBases.at(i)+".002", 1, 2, 2))
      return fail("cannot create RGB channel fixture");
  if (!writeColorHeader(colorHeader, "RGB"))
    return fail("cannot create RGB manifest fixture");
  if (!PvlManifestParser::parse(colorHeader, manifest, true) ||
      !manifest.isColor || manifest.channelNames.count() != 3)
    return fail("RGB channel manifest did not validate");

  if (!writeColorHeader(colorHeader, "RGBA", "RGB") ||
      PvlManifestParser::parse(colorHeader, manifest, false) ||
      !manifest.error.contains("RGB/RGBA"))
    return fail("RGB/RGBA voxel type mismatch was accepted");

  if (!writeColorHeaderWithScalarNames(colorHeader))
    return fail("cannot create invalid RGB scalar-list fixture");
  if (PvlManifestParser::parse(colorHeader, manifest, false) ||
      !manifest.error.contains("channelnames"))
    return fail("RGB scalar PVL names were accepted");

  std::cout << "PVL manifest smoke passed" << std::endl;
  return 0;
}
