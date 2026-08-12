#ifndef MESHVERTEXBUFFER_H
#define MESHVERTEXBUFFER_H

#include <QVector>
#include <QtGlobal>

#include <limits>

namespace MeshVertexBuffer
{
  enum
  {
    PositionOffset = 0,
    NormalOffset = 3,
    ColorOffset = 6,
    TangentOffset = 9,
    BaseStride = 9,
    TangentStride = 12
  };

  struct Layout
  {
    Layout() : stride(BaseStride), hasTangents(false) {}

    int stride;
    bool hasTangents;
  };

  inline bool
  ensureRequiredAttributes(const QVector<float> &vertices,
			   QVector<float> &normals,
			   QVector<float> &colors,
			   float defaultRed,
			   float defaultGreen,
			   float defaultBlue)
  {
    const int componentCount = vertices.count();
    if (componentCount <= 0 || componentCount%3 != 0)
      return false;

    if (normals.isEmpty())
      {
	normals.fill(0.0f, componentCount);
	for (int z=2; z<componentCount; z+=3)
	  normals[z] = 1.0f;
      }
    else if (normals.count() != componentCount)
      return false;

    if (colors.isEmpty())
      {
	colors.resize(componentCount);
	for (int vertex=0; vertex<componentCount/3; ++vertex)
	  {
	    colors[3*vertex+0] = defaultRed;
	    colors[3*vertex+1] = defaultGreen;
	    colors[3*vertex+2] = defaultBlue;
	  }
      }
    else if (colors.count() != componentCount)
      return false;

    return true;
  }

  inline bool
  pack(const QVector<float> &vertices,
	 const QVector<float> &normals,
	 const QVector<float> &colors,
	 const QVector<float> &uv,
	 const QVector<float> *tangents,
	 QVector<float> &packed,
	 Layout &layout)
  {
    packed.clear();
    layout = Layout();

    const int componentCount = vertices.count();
    if (componentCount <= 0 || componentCount%3 != 0)
      return false;

    if (!normals.isEmpty() && normals.count() != componentCount)
      return false;

    if (colors.count() != componentCount ||
	(!uv.isEmpty() && uv.count() != componentCount))
      return false;

    const QVector<float> &colorOrUv = uv.isEmpty() ? colors : uv;

    layout.hasTangents = tangents && !tangents->isEmpty();
    if (layout.hasTangents && tangents->count() != componentCount)
      return false;

    layout.stride = layout.hasTangents ? TangentStride : BaseStride;
    const int vertexCount = componentCount/3;
    const qint64 packedCount = static_cast<qint64>(vertexCount)*layout.stride;
    if (packedCount > std::numeric_limits<int>::max())
      return false;

    packed.resize(static_cast<int>(packedCount));
    for (int vertex=0; vertex<vertexCount; ++vertex)
      {
	const int source = 3*vertex;
	const int destination = layout.stride*vertex;

	packed[destination+PositionOffset+0] = vertices[source+0];
	packed[destination+PositionOffset+1] = vertices[source+1];
	packed[destination+PositionOffset+2] = vertices[source+2];

	if (normals.isEmpty())
	  {
	    packed[destination+NormalOffset+0] = 0.0f;
	    packed[destination+NormalOffset+1] = 0.0f;
	    packed[destination+NormalOffset+2] = 1.0f;
	  }
	else
	  {
	    packed[destination+NormalOffset+0] = normals[source+0];
	    packed[destination+NormalOffset+1] = normals[source+1];
	    packed[destination+NormalOffset+2] = normals[source+2];
	  }

	packed[destination+ColorOffset+0] = colorOrUv[source+0];
	packed[destination+ColorOffset+1] = colorOrUv[source+1];
	packed[destination+ColorOffset+2] = colorOrUv[source+2];

	if (layout.hasTangents)
	  {
	    packed[destination+TangentOffset+0] = (*tangents)[source+0];
	    packed[destination+TangentOffset+1] = (*tangents)[source+1];
	    packed[destination+TangentOffset+2] = (*tangents)[source+2];
	  }
      }

    return true;
  }
}

#endif
