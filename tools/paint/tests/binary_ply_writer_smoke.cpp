#include "../../../common/src/mesh/binaryplywriter.h"

#include <QCoreApplication>
#include <QDataStream>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include <cmath>
#include <limits>

namespace
{
bool
fail(const QString& message)
{
  qCritical().noquote() << message;
  return false;
}

bool
verifyPly(const QString& filename)
{
  QFile file(filename);
  if (!file.open(QFile::ReadOnly))
    return fail("Cannot reopen the generated PLY file.");

  QByteArray bytes = file.readAll();
  const QByteArray terminator("end_header\n");
  const int headerEnd = bytes.indexOf(terminator);
  if (headerEnd < 0)
    return fail("Generated PLY header is incomplete.");
  const int dataOffset = headerEnd+terminator.size();
  if (!bytes.startsWith("ply\nformat binary_little_endian 1.0\n") ||
      !bytes.contains("element vertex 3\n") ||
      !bytes.contains("element face 1\n"))
    return fail("Generated PLY header has the wrong contract.");

  const qint64 expectedPayload = 3*(6*4+3)+1+3*4;
  if (bytes.size()-dataOffset != expectedPayload)
    return fail("Generated PLY payload has the wrong size.");

  file.seek(dataOffset);
  QDataStream stream(&file);
  stream.setByteOrder(QDataStream::LittleEndian);
  stream.setFloatingPointPrecision(QDataStream::SinglePrecision);

  float x = 0;
  float y = 0;
  float z = 0;
  float nx = 0;
  float ny = 0;
  float nz = 0;
  quint8 red = 0;
  quint8 green = 0;
  quint8 blue = 0;
  stream >> x >> y >> z >> nx >> ny >> nz >> red >> green >> blue;
  if (std::fabs(x-11.0f) > 0.0001f ||
      std::fabs(y-22.0f) > 0.0001f ||
      std::fabs(z-33.0f) > 0.0001f ||
      nx != 1.0f || ny != 0.0f || nz != 0.0f ||
      red != 255 || green != 0 || blue != 128)
    return fail("Generated PLY vertex values are incorrect.");

  file.seek(file.size()-13);
  quint8 count = 0;
  qint32 a = -1;
  qint32 b = -1;
  qint32 c = -1;
  stream.device()->seek(file.size()-13);
  stream >> count >> a >> b >> c;
  if (count != 3 || a != 0 || b != 1 || c != 2)
    return fail("Generated PLY face values are incorrect.");
  return true;
}
}

int
main(int argc, char **argv)
{
  QCoreApplication app(argc, argv);
  Q_UNUSED(app);

  QTemporaryDir temporary;
  if (!temporary.isValid())
    return fail("Cannot create the PLY smoke-test directory.") ? 0 : 1;

  const QString directory = temporary.path()+"/PLY 中文 path";
  if (!QDir().mkpath(directory))
    return fail("Cannot create the Unicode PLY test path.") ? 0 : 1;
  const QString filename = directory+"/mesh 输出.ply";

  QVector<float> vertices{
    1.0f, 2.0f, 3.0f,
    4.0f, 5.0f, 6.0f,
    7.0f, 8.0f, 9.0f
  };
  QVector<float> normals{
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 1.0f
  };
  QVector<float> colors{
    std::numeric_limits<float>::max(),
    std::numeric_limits<float>::lowest(),
    0.5f,
    0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 1.0f
  };
  QVector<uint> triangles{0, 1, 2};
  QString error;
  const auto vertexTransform = [](float x, float y, float z)
  {
    return QVector3D(x+10.0f, y+20.0f, z+30.0f);
  };
  const auto normalTransform = [](float x, float y, float z)
  {
    return QVector3D(x, y, z);
  };
  if (!BinaryPlyWriter::save(filename, vertices, normals, colors, triangles,
                             vertexTransform, normalTransform, &error))
    return fail("Cannot write the valid PLY file: "+error) ? 0 : 1;
  if (!verifyPly(filename))
    return 1;

  QFile sentinel(filename);
  if (!sentinel.open(QFile::WriteOnly | QFile::Truncate) ||
      sentinel.write("preserve-me") != 11)
    return fail("Cannot prepare the PLY preservation test.") ? 0 : 1;
  sentinel.close();

  QVector<uint> invalidTriangles{0, 1, 99};
  if (BinaryPlyWriter::save(filename, vertices, normals, colors,
                            invalidTriangles, vertexTransform,
                            normalTransform, &error))
    return fail("Invalid PLY indices were accepted.") ? 0 : 1;
  if (!sentinel.open(QFile::ReadOnly) || sentinel.readAll() != "preserve-me")
    return fail("Invalid PLY input replaced the existing file.") ? 0 : 1;
  sentinel.close();

  const auto invalidTransform = [](float, float, float)
  {
    return QVector3D(std::numeric_limits<float>::quiet_NaN(), 0.0f, 0.0f);
  };
  if (BinaryPlyWriter::save(filename, vertices, normals, colors, triangles,
                            invalidTransform, normalTransform, &error))
    return fail("Non-finite PLY coordinates were accepted.") ? 0 : 1;
  if (!sentinel.open(QFile::ReadOnly) || sentinel.readAll() != "preserve-me")
    return fail("A failed PLY write replaced the existing file.") ? 0 : 1;

  qInfo() << "Binary PLY writer smoke passed";
  return 0;
}
