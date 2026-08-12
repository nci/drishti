#ifndef BINARYPLYWRITER_H
#define BINARYPLYWRITER_H

#include <QVector>
#include <QVector3D>
#include <QString>

#include <functional>

class BinaryPlyWriter
{
public:
  using Transform = std::function<QVector3D(float, float, float)>;

  static bool save(const QString& filename,
                   const QVector<float>& vertices,
                   const QVector<float>& normals,
                   const QVector<float>& colors,
                   const QVector<uint>& triangles,
                   const Transform& vertexTransform,
                   const Transform& normalTransform,
                   QString *error = nullptr);
};

#endif
