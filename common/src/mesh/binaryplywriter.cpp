#include "binaryplywriter.h"

#include <QDataStream>
#include <QFile>
#include <QFileDevice>
#include <QSaveFile>
#include <QtGlobal>

#include <limits>

namespace
{
bool
setError(QString *error, const QString& message)
{
  if (error)
    *error = message;
  return false;
}

bool
writeHeader(QSaveFile& output, int vertexCount, int triangleCount,
            QString *error)
{
  QByteArray header("ply\nformat binary_little_endian 1.0\n");
  header += "element vertex " + QByteArray::number(vertexCount) + "\n";
  header += "property float x\nproperty float y\nproperty float z\n";
  header += "property float nx\nproperty float ny\nproperty float nz\n";
  header += "property uchar red\nproperty uchar green\nproperty uchar blue\n";
  header += "element face " + QByteArray::number(triangleCount) + "\n";
  header += "property list uchar int vertex_indices\nend_header\n";

  if (output.write(header) != header.size())
    return setError(error,
      QString("Cannot write the PLY header to '%1': %2")
      .arg(output.fileName(), output.errorString()));
  return true;
}

bool
finiteVector(const QVector3D& value)
{
  return qIsFinite(value.x()) &&
         qIsFinite(value.y()) &&
         qIsFinite(value.z());
}

quint8
colorByte(float value)
{
  if (value <= 0.0f)
    return 0;
  if (value >= 1.0f)
    return 255;
  return static_cast<quint8>(qRound(value*255.0f));
}
}

bool
BinaryPlyWriter::save(const QString& filename,
                      const QVector<float>& vertices,
                      const QVector<float>& normals,
                      const QVector<float>& colors,
                      const QVector<uint>& triangles,
                      const Transform& vertexTransform,
                      const Transform& normalTransform,
                      QString *error)
{
  if (error)
    error->clear();

  const bool hasNormals = !normals.isEmpty();
  const bool hasColors = !colors.isEmpty();
  if (filename.isEmpty())
    return setError(error, "No PLY output filename was supplied.");
  if (vertices.size()%3 != 0 || triangles.size()%3 != 0 ||
      (hasNormals && normals.size() != vertices.size()) ||
      (hasColors && colors.size() != vertices.size()))
    return setError(error, "The mesh arrays are inconsistent.");
  if (!vertexTransform || (hasNormals && !normalTransform))
    return setError(error, "The mesh coordinate transform is unavailable.");

  const int vertexCount = vertices.size()/3;
  const int triangleCount = triangles.size()/3;
  for (uint index : triangles)
    if (index >= static_cast<uint>(vertexCount) ||
        index > static_cast<uint>(std::numeric_limits<qint32>::max()))
      return setError(error, "The mesh contains an invalid triangle index.");

  QSaveFile output(filename);
  if (!output.open(QFile::WriteOnly))
    return setError(error,
      QString("Cannot open PLY output '%1': %2")
      .arg(filename, output.errorString()));
  if (!writeHeader(output, vertexCount, triangleCount, error))
    {
      output.cancelWriting();
      return false;
    }

  QDataStream stream(&output);
  stream.setByteOrder(QDataStream::LittleEndian);
  stream.setFloatingPointPrecision(QDataStream::SinglePrecision);

  for(int i=0; i<vertexCount; ++i)
    {
      const QVector3D vertex = vertexTransform(vertices[3*i],
                                                vertices[3*i+1],
                                                vertices[3*i+2]);
      QVector3D normal;
      if (hasNormals)
        normal = normalTransform(normals[3*i], normals[3*i+1], normals[3*i+2]);
      if (!finiteVector(vertex) || (hasNormals && !finiteVector(normal)))
        {
          output.cancelWriting();
          return setError(error,
            QString("The mesh contains a non-finite transformed value at vertex %1.")
            .arg(i));
        }

      stream << vertex.x() << vertex.y() << vertex.z();
      stream << normal.x() << normal.y() << normal.z();
      if (hasColors)
        {
          if (!qIsFinite(colors[3*i]) ||
              !qIsFinite(colors[3*i+1]) ||
              !qIsFinite(colors[3*i+2]))
            {
              output.cancelWriting();
              return setError(error,
                QString("The mesh contains a non-finite color at vertex %1.")
                .arg(i));
            }
          stream << colorByte(colors[3*i])
                 << colorByte(colors[3*i+1])
                 << colorByte(colors[3*i+2]);
        }
      else
        stream << quint8(0) << quint8(0) << quint8(0);
    }

  for(int i=0; i<triangleCount; ++i)
    stream << quint8(3)
           << static_cast<qint32>(triangles[3*i])
           << static_cast<qint32>(triangles[3*i+1])
           << static_cast<qint32>(triangles[3*i+2]);

  if (stream.status() != QDataStream::Ok ||
      output.error() != QFileDevice::NoError)
    {
      const QString detail = output.errorString();
      output.cancelWriting();
      return setError(error,
        QString("The PLY file '%1' could not be written completely: %2")
        .arg(filename, detail));
    }
  if (!output.commit())
    return setError(error,
      QString("Cannot commit PLY output '%1': %2")
      .arg(filename, output.errorString()));
  return true;
}
