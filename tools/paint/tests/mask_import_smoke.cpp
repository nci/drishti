#include "../maskimportutils.h"

#include "blosc.h"

#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>
#include <QTextStream>
#include <QVector>

#include <cstring>

namespace
{
int fail(const QString& message)
{
  QTextStream(stderr) << "FAILED: " << message << Qt::endl;
  return 1;
}

bool writeExact(QFile& output, const void *data, qint64 bytes)
{
  return output.write(static_cast<const char*>(data), bytes) == bytes;
}

bool writeRawMask(const QString& path,
                  quint8 voxelType,
                  qint32 depth,
                  qint32 width,
                  qint32 height,
                  const QByteArray& payload,
                  const QByteArray& trailing = QByteArray())
{
  QFile output(path);
  if (!output.open(QFile::WriteOnly))
    return false;
  return writeExact(output, &voxelType, 1) &&
         writeExact(output, &depth, 4) &&
         writeExact(output, &width, 4) &&
         writeExact(output, &height, 4) &&
         writeExact(output, payload.constData(), payload.size()) &&
         (trailing.isEmpty() ||
          writeExact(output, trailing.constData(), trailing.size()));
}

bool writeCompressedMask(const QString& path,
                         quint8 voxelType,
                         qint32 depth,
                         qint32 width,
                         qint32 height,
                         const QByteArray& payload)
{
  QByteArray compressed(payload.size()+BLOSC_MAX_OVERHEAD, '\0');
  const int compressedBytes = blosc_compress_ctx(
    3, BLOSC_SHUFFLE, voxelType < 2 ? 1 : 2,
    static_cast<size_t>(payload.size()), payload.constData(),
    compressed.data(), static_cast<size_t>(compressed.size()),
    "blosclz", 0, 1);
  if (compressedBytes <= 0)
    return false;
  compressed.resize(compressedBytes);

  QFile output(path);
  if (!output.open(QFile::WriteOnly))
    return false;
  const char magic[6] = {'d', 'p', 'm', '1', '0', '0'};
  const qint32 blockCount = 1;
  const qint32 blockBytes = payload.size();
  const qint32 storedBytes = compressed.size();
  return writeExact(output, magic, 6) &&
         writeExact(output, &voxelType, 1) &&
         writeExact(output, &depth, 4) &&
         writeExact(output, &width, 4) &&
         writeExact(output, &height, 4) &&
         writeExact(output, &blockCount, 4) &&
         writeExact(output, &blockBytes, 4) &&
         writeExact(output, &storedBytes, 4) &&
         writeExact(output, compressed.constData(), compressed.size());
}

QByteArray bytePayload(std::initializer_list<quint8> values)
{
  QByteArray payload;
  payload.reserve(static_cast<int>(values.size()));
  for (quint8 value : values)
    payload.append(static_cast<char>(value));
  return payload;
}

QByteArray shortPayload(std::initializer_list<quint16> values)
{
  QByteArray payload(static_cast<int>(values.size()*sizeof(quint16)), '\0');
  std::memcpy(payload.data(), values.begin(),
              values.size()*sizeof(quint16));
  return payload;
}
}

int main(int argc, char **argv)
{
  QCoreApplication application(argc, argv);
  QTemporaryDir directory;
  if (!directory.isValid())
    return fail("Cannot create temporary mask directory");

  const QString raw8 = directory.filePath("labels8.mask");
  const QByteArray raw8Payload = bytePayload({0, 1, 2, 0, 3, 4, 0, 5});
  if (!writeRawMask(raw8, 0, 2, 2, 2, raw8Payload))
    return fail("Cannot write 8-bit raw mask fixture");

  ImportedPaintMask mask;
  QString error;
  if (!loadImportedPaintMask(raw8, mask, error))
    return fail("8-bit raw mask was rejected: "+error);
  if (mask.depth != 2 || mask.width != 2 || mask.height != 2 ||
      mask.voxelCount != 8 || mask.labels[0] != 0 ||
      mask.labels[1] != 1 || mask.labels[7] != 5)
    return fail("8-bit raw mask values or dimensions changed");

  QVector<quint16> equalTarget(8, 99);
  if (!overlayImportedPaintMask(
        mask, true, 2, 2, 2, 2,
        reinterpret_cast<uchar*>(equalTarget.data()),
        static_cast<std::uint64_t>(equalTarget.size()*sizeof(quint16)),
        error))
    return fail("Equal-size mask overlay failed: "+error);
  const QVector<quint16> equalExpected =
    QVector<quint16>({99, 1, 2, 99, 3, 4, 99, 5});
  if (equalTarget != equalExpected)
    return fail("Equal-size overlay skipped an edge or replaced zero labels");

  QVector<quint16> flipped(8, 0);
  if (!overlayImportedPaintMask(
        mask, false, 2, 2, 2, 2,
        reinterpret_cast<uchar*>(flipped.data()),
        static_cast<std::uint64_t>(flipped.size()*sizeof(quint16)),
        error) ||
      flipped[0] != 3 || flipped[1] != 4 ||
      flipped[4] != 0 || flipped[5] != 1)
    return fail("Slice-zero depth reversal is incorrect");

  const QString oneVoxel = directory.filePath("one.mask");
  if (!writeRawMask(oneVoxel, 0, 1, 1, 1, bytePayload({7})) ||
      !loadImportedPaintMask(oneVoxel, mask, error))
    return fail("Cannot load one-voxel resize fixture: "+error);
  QVector<quint16> scaled(3*2*5, 0);
  if (!overlayImportedPaintMask(
        mask, true, 3, 2, 5, 2,
        reinterpret_cast<uchar*>(scaled.data()),
        static_cast<std::uint64_t>(scaled.size()*sizeof(quint16)),
        error))
    return fail("Scaled mask overlay failed: "+error);
  for (quint16 value : scaled)
    if (value != 7)
      return fail("Center-point nearest-neighbor mapping omitted a target edge");

  const QString raw16 = directory.filePath("labels16.mask");
  if (!writeRawMask(raw16, 2, 1, 1, 2, shortPayload({300, 12})) ||
      !loadImportedPaintMask(raw16, mask, error) ||
      mask.labels[0] != 300 || mask.labels[1] != 12)
    return fail("16-bit raw labels were not preserved: "+error);
  QVector<quint8> byteTarget(2, 77);
  if (overlayImportedPaintMask(
        mask, true, 1, 1, 2, 1, byteTarget.data(), byteTarget.size(), error) ||
      byteTarget[0] != 77 || byteTarget[1] != 77)
    return fail("16-bit labels partially modified an 8-bit destination");

  const QString compressed = directory.filePath("labels.mask.sc");
  if (!writeCompressedMask(compressed, 2, 1, 1, 2,
                           shortPayload({511, 65535})) ||
      !loadImportedPaintMask(compressed, mask, error) ||
      mask.labels[0] != 511 || mask.labels[1] != 65535)
    return fail("Valid compressed mask was not decoded: "+error);

  const qint32 savedDepth = mask.depth;
  const quint16 savedValue = mask.labels[0];
  const QString shortRaw = directory.filePath("short.mask");
  if (!writeRawMask(shortRaw, 0, 2, 2, 2, bytePayload({1, 2, 3})) ||
      loadImportedPaintMask(shortRaw, mask, error) ||
      mask.depth != savedDepth || mask.labels[0] != savedValue)
    return fail("Rejected short raw mask damaged the active imported mask");

  const QString trailingRaw = directory.filePath("trailing.mask");
  if (!writeRawMask(trailingRaw, 0, 1, 1, 1, bytePayload({1}),
                    QByteArray("x", 1)) ||
      loadImportedPaintMask(trailingRaw, mask, error))
    return fail("Raw mask with trailing data was accepted");

  QFile corrupt(compressed);
  if (!corrupt.open(QFile::ReadWrite) || !corrupt.seek(0) ||
      corrupt.write("bad100", 6) != 6)
    return fail("Cannot corrupt compressed-mask signature fixture");
  corrupt.close();
  if (loadImportedPaintMask(compressed, mask, error) ||
      !error.contains("signature", Qt::CaseInsensitive))
    return fail("Compressed mask with a bad signature was accepted");

  QTextStream(stdout)
    << "Paint mask import smoke passed: raw8/raw16/compressed, "
       "transactional rejection, flip and nearest-neighbor scaling"
    << Qt::endl;
  return 0;
}
