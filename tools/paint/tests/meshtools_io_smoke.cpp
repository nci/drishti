#include "meshtools.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTextStream>
#include <QtEndian>
#include <QtMath>

#include <cstring>

namespace
{
int fail(const QString& message)
{
  QTextStream(stderr) << "FAILED: " << message << Qt::endl;
  return 1;
}

void checkpoint(const char *message)
{
  QTextStream(stdout) << "checkpoint: " << message << Qt::endl;
}

bool writeAll(QFile& file, const char *data, qint64 bytes)
{
  qint64 written = 0;
  while (written < bytes)
    {
      const qint64 count = file.write(data+written, bytes-written);
      if (count <= 0)
        return false;
      written += count;
    }
  return true;
}

bool writeFile(const QString& path, const QByteArray& data)
{
  QFile file(path);
  return file.open(QFile::WriteOnly) &&
         writeAll(file, data.constData(), data.size()) && file.flush();
}

QByteArray readFile(const QString& path)
{
  QFile file(path);
  if (!file.open(QFile::ReadOnly))
    return QByteArray();
  return file.readAll();
}

bool writeVertexSlab(const QString& path,
                     const QVector<QVector3D>& vertices)
{
  QFile file(path);
  if (!file.open(QFile::WriteOnly))
    return false;
  const qint32 count = vertices.count();
  if (!writeAll(file, reinterpret_cast<const char*>(&count), 4))
    return false;
  for (const QVector3D& vertex : vertices)
    {
      const float values[6] = {
        vertex.x(), vertex.y(), vertex.z(), 0.0f, 0.0f, 1.0f
      };
      if (!writeAll(file, reinterpret_cast<const char*>(values), 24))
        return false;
    }
  return file.flush();
}

bool writeTriangleSlab(const QString& path, const QVector<int>& triangles)
{
  if (triangles.count()%3 != 0)
    return false;
  QFile file(path);
  if (!file.open(QFile::WriteOnly))
    return false;
  const qint32 count = triangles.count()/3;
  return writeAll(file, reinterpret_cast<const char*>(&count), 4) &&
         writeAll(file, reinterpret_cast<const char*>(triangles.constData()),
                  triangles.count()*static_cast<qint64>(sizeof(int))) &&
         file.flush();
}

int headerEnd(const QByteArray& ply)
{
  const QByteArray marker("end_header\n");
  const int index = ply.indexOf(marker);
  return index < 0 ? -1 : index+marker.size();
}

float floatLittleEndian(const char *data)
{
  const quint32 bits = qFromLittleEndian<quint32>(
    reinterpret_cast<const uchar*>(data));
  float value = 0.0f;
  std::memcpy(&value, &bits, sizeof(value));
  return value;
}
}

int main(int argc, char **argv)
{
  QApplication application(argc, argv);
  checkpoint("application");
  QTemporaryDir temporary;
  if (!temporary.isValid())
    return fail("cannot create a temporary directory");

  QDir root(temporary.path());
  if (!root.mkdir(QString::fromUtf8("mesh \xE4\xB8\xAD\xE6\x96\x87 path")))
    return fail("cannot create the Unicode test directory");
  const QString directory = root.filePath(
    QString::fromUtf8("mesh \xE4\xB8\xAD\xE6\x96\x87 path"));

  QVector<QVector3D> vertices;
  vertices << QVector3D(0, 0, 0) << QVector3D(1, 0, 0)
           << QVector3D(0, 1, 0) << QVector3D(0, 0, 1);
  QVector<QVector3D> normals(vertices.count(), QVector3D(0, 0, 1));
  QVector<QVector3D> colors;
  colors << QVector3D(255, 0, 0) << QVector3D(0, 255, 0)
         << QVector3D(0, 0, 255) << QVector3D(255, 255, 255);
  QVector<int> triangles;
  triangles << 0 << 1 << 2 << 0 << 2 << 3;

  const QString objPath = QDir(directory).filePath("round trip.obj");
  const QString asciiPlyPath = QDir(directory).filePath("round trip ascii.ply");
  const QString binaryPlyPath = QDir(directory).filePath("round trip binary.ply");
  const QString stlPath = QDir(directory).filePath("round trip.stl");
  checkpoint("round-trip writers");
  if (!MeshTools::saveToOBJ(objPath, vertices, normals, colors, triangles,
                            false) ||
      !MeshTools::saveToPLY(asciiPlyPath, vertices, normals, colors, triangles,
                            false, false) ||
      !MeshTools::saveToPLY(binaryPlyPath, vertices, normals, colors, triangles,
                            false, true) ||
      !MeshTools::saveToSTL(stlPath, vertices, normals, triangles, false))
    return fail("a Unicode-path mesh writer failed");
  checkpoint("round-trip validation");

  const QByteArray obj = readFile(objPath);
  if (obj.count("\nv ") != vertices.count() ||
      obj.count("\nf ") != triangles.count()/3)
    return fail("OBJ output cannot be parsed back");

  const QByteArray asciiPly = readFile(asciiPlyPath);
  if (!asciiPly.startsWith("ply\nformat ascii 1.0\n") ||
      !asciiPly.contains("element vertex 4\n") ||
      !asciiPly.contains("element face 2\n"))
    return fail("ASCII PLY header is invalid");

  const QByteArray binaryPly = readFile(binaryPlyPath);
  const int binaryHeader = headerEnd(binaryPly);
  if (binaryHeader < 0 ||
      binaryPly.size() != binaryHeader+vertices.count()*27+2*13)
    return fail("binary PLY record size is invalid");
  const int firstFace = binaryHeader+vertices.count()*27;
  if (static_cast<uchar>(binaryPly[firstFace]) != 3 ||
      qFromLittleEndian<qint32>(reinterpret_cast<const uchar*>(
        binaryPly.constData()+firstFace+1)) != 2)
    return fail("binary PLY face data is invalid");

  const QByteArray stl = readFile(stlPath);
  if (stl.size() != 84+2*50 ||
      qFromLittleEndian<quint32>(reinterpret_cast<const uchar*>(
        stl.constData()+80)) != 2)
    return fail("binary STL output is invalid");

  const QString preserved = QDir(directory).filePath("preserve old.ply");
  const QByteArray sentinel("existing-output");
  if (!writeFile(preserved, sentinel))
    return fail("cannot prepare the transactional output test");
  QVector<int> invalidTriangles = triangles;
  invalidTriangles[0] = vertices.count();
  if (MeshTools::saveToPLY(preserved, vertices, normals, colors,
                           invalidTriangles, false, true) ||
      readFile(preserved) != sentinel)
    return fail("invalid mesh input replaced an existing output");

  checkpoint("mesh smoothing");
  QVector<QVector3D> isolatedVertices = vertices;
  QVector<QVector3D> generatedNormals;
  QVector<int> oneTriangle;
  oneTriangle << 0 << 1 << 2;
  MeshTools::smoothMesh(isolatedVertices, generatedNormals, oneTriangle, 0,
                        false);
  if (generatedNormals.count() != isolatedVertices.count() ||
      !qIsFinite(generatedNormals[3].x()) ||
      !qFuzzyIsNull(generatedNormals[3].lengthSquared()))
    return fail("mesh smoothing mishandled an isolated vertex");

  checkpoint("multi-slab STL");
  const QString slabBase = QDir(directory).filePath("two slabs.stl");
  QVector<QVector3D> firstVertices;
  firstVertices << QVector3D(0, 0, 0) << QVector3D(1, 0, 0)
                << QVector3D(0, 1, 0);
  QVector<QVector3D> secondVertices;
  secondVertices << QVector3D(10, 0, 0) << QVector3D(11, 0, 0)
                 << QVector3D(10, 1, 0);
  QVector<int> localTriangle;
  localTriangle << 0 << 1 << 2;
  if (!writeVertexSlab(slabBase+".0.vert", firstVertices) ||
      !writeTriangleSlab(slabBase+".0.tri", localTriangle) ||
      !writeVertexSlab(slabBase+".1.vert", secondVertices) ||
      !writeTriangleSlab(slabBase+".1.tri", localTriangle) ||
      !MeshTools::saveToSTL(slabBase, 2, 6, 2))
    return fail("multi-slab STL conversion failed");
  const QByteArray slabStl = readFile(slabBase);
  if (slabStl.size() != 84+2*50 ||
      floatLittleEndian(slabStl.constData()+84+50+12) < 9.0f)
    return fail("the second STL slab did not receive its vertex offset");
  for (int slab=0; slab<2; ++slab)
    if (QFileInfo::exists(slabBase+QString(".%1.vert").arg(slab)) ||
        QFileInfo::exists(slabBase+QString(".%1.tri").arg(slab)))
      return fail("successful slab conversion left temporary artifacts");

  checkpoint("corrupt slab rollback");
  const QString corruptBase = QDir(directory).filePath("corrupt slabs.stl");
  if (!writeFile(corruptBase, sentinel) ||
      !writeVertexSlab(corruptBase+".0.vert", firstVertices) ||
      !writeTriangleSlab(corruptBase+".0.tri", localTriangle))
    return fail("cannot prepare the corrupt-slab test");
  QFile truncated(corruptBase+".0.vert");
  if (!truncated.open(QFile::ReadWrite) ||
      !truncated.resize(qMax<qint64>(4, truncated.size()-1)))
    return fail("cannot truncate the test vertex slab");
  truncated.close();
  if (MeshTools::saveToSTL(corruptBase, 1, 3, 1) ||
      readFile(corruptBase) != sentinel ||
      !QFileInfo::exists(corruptBase+".0.vert") ||
      !QFileInfo::exists(corruptBase+".0.tri"))
    return fail("corrupt slab input was accepted or destroyed recovery data");

  checkpoint("blocked output");
  const QString blockedParent = QDir(directory).filePath("not a directory");
  if (!writeFile(blockedParent, sentinel) ||
      MeshTools::saveToOBJ(QDir(blockedParent).filePath("output.obj"),
                           vertices, normals, colors, triangles, false))
    return fail("an unwritable output path was reported as successful");

  QTextStream(stdout) << "MeshTools I/O smoke passed" << Qt::endl;
  return 0;
}
