#include "meshtools.h"
#include <QDir>
#include <QDebug>
#include <QFileInfo>
#include <QList>
#include <QProgressDialog>
#include <QMultiMap>
#include <QApplication>
#include <QFile>
#include <QSaveFile>
#include <QTemporaryDir>
#include <QTextStream>
#include <QtEndian>
#include <QtMath>
#include <QMessageBox>
#include <QThread>

#include <cstring>
#include <limits>
#include <memory>
#include <new>
#include <stdexcept>

#include "gmsh.h_cwrap"

namespace
{
struct SlabMeshData
{
  QVector<QVector3D> vertices;
  QVector<QVector3D> normals;
  QVector<QVector3D> colors;
  QVector<int> triangles;
  QStringList artifacts;
};

bool writeAll(QIODevice& device, const char *data, qint64 bytes);

class BufferedDeviceWriter
{
public:
  explicit BufferedDeviceWriter(QIODevice& device)
    : m_device(device)
  {
    m_buffer.reserve(1024*1024);
  }

  bool append(const char *data, int bytes)
  {
    if (bytes < 0)
      return false;
    if (m_buffer.size()+bytes > 1024*1024 && !flush())
      return false;
    if (bytes > 1024*1024)
      return writeAll(m_device, data, bytes);
    m_buffer.append(data, bytes);
    return true;
  }

  bool flush()
  {
    if (m_buffer.isEmpty())
      return true;
    const bool ok = writeAll(m_device, m_buffer.constData(), m_buffer.size());
    m_buffer.clear();
    return ok;
  }

private:
  QIODevice& m_device;
  QByteArray m_buffer;
};

void storeFloatLittleEndian(char *destination, float value)
{
  quint32 bits = 0;
  static_assert(sizeof(bits) == sizeof(value), "unexpected float size");
  std::memcpy(&bits, &value, sizeof(bits));
  qToLittleEndian(bits, destination);
}

void storeIntLittleEndian(char *destination, qint32 value)
{
  qToLittleEndian(static_cast<quint32>(value), destination);
}

bool writeAll(QIODevice& device, const char *data, qint64 bytes)
{
  qint64 written = 0;
  while (written < bytes)
    {
      const qint64 count = device.write(data+written, bytes-written);
      if (count <= 0)
        return false;
      written += count;
    }
  return true;
}

bool readAll(QIODevice& device, char *data, qint64 bytes)
{
  qint64 read = 0;
  while (read < bytes)
    {
      const qint64 count = device.read(data+read, bytes-read);
      if (count <= 0)
        return false;
      read += count;
    }
  return true;
}

bool validMesh(const QVector<QVector3D>& vertices,
               const QVector<QVector3D>& normals,
               const QVector<QVector3D>& colors,
               const QVector<int>& triangles)
{
  if ((!normals.isEmpty() && normals.count() != vertices.count()) ||
      (!colors.isEmpty() && colors.count() != vertices.count()) ||
      triangles.count()%3 != 0)
    return false;

  for (int index : triangles)
    if (index < 0 || index >= vertices.count())
      return false;
  return true;
}

bool loadSlabMesh(const QString& baseName,
                  int slabCount,
                  int expectedVertices,
                  int expectedTriangles,
                  bool hasColors,
                  bool globalTriangleIndices,
                  SlabMeshData& mesh)
{
  if (slabCount <= 0 || expectedVertices < 0 || expectedTriangles < 0)
    return false;

  try
    {
      mesh.vertices.reserve(expectedVertices);
      mesh.normals.reserve(expectedVertices);
      if (hasColors)
        mesh.colors.reserve(expectedVertices);
      if (expectedTriangles > std::numeric_limits<int>::max()/3)
        return false;
      mesh.triangles.reserve(3*expectedTriangles);

      for (int slab=0; slab<slabCount; ++slab)
        {
          const QString vertexName =
            baseName + QString(".%1.vert").arg(slab);
          QFile vertexFile(vertexName);
          if (!vertexFile.open(QFile::ReadOnly))
            return false;

          qint32 vertexCount = 0;
          if (!readAll(vertexFile, reinterpret_cast<char*>(&vertexCount), 4) ||
              vertexCount < 0)
            return false;
          const qint64 recordBytes = hasColors ? 27 : 24;
          if (static_cast<qint64>(vertexCount)*recordBytes !=
                vertexFile.size()-vertexFile.pos() ||
              vertexCount > std::numeric_limits<int>::max()-mesh.vertices.count())
            return false;

          const int vertexOffset = mesh.vertices.count();
          for (int index=0; index<vertexCount; ++index)
            {
              float values[6];
              if (!readAll(vertexFile, reinterpret_cast<char*>(values), 24))
                return false;
              mesh.vertices << QVector3D(values[0], values[1], values[2]);
              mesh.normals << QVector3D(values[3], values[4], values[5]);
              if (hasColors)
                {
                  uchar color[3];
                  if (!readAll(vertexFile, reinterpret_cast<char*>(color), 3))
                    return false;
                  mesh.colors << QVector3D(color[0], color[1], color[2]);
                }
            }
          vertexFile.close();
          mesh.artifacts << vertexName;

          const QString triangleName =
            baseName + QString(".%1.tri").arg(slab);
          QFile triangleFile(triangleName);
          if (!triangleFile.open(QFile::ReadOnly))
            return false;

          qint32 triangleCount = 0;
          if (!readAll(triangleFile,
                       reinterpret_cast<char*>(&triangleCount), 4) ||
              triangleCount < 0 ||
               static_cast<qint64>(triangleCount)*12 !=
                 triangleFile.size()-triangleFile.pos() ||
              triangleCount >
                (std::numeric_limits<int>::max()-mesh.triangles.count())/3)
            return false;

          for (int index=0; index<triangleCount; ++index)
            {
              qint32 triangle[3];
              if (!readAll(triangleFile,
                           reinterpret_cast<char*>(triangle), 12))
                return false;
              for (int corner=0; corner<3; ++corner)
                {
                  qint64 vertex = triangle[corner];
                  if (!globalTriangleIndices)
                    vertex += vertexOffset;
                  if (vertex < 0 || vertex >= mesh.vertices.count())
                    return false;
                  mesh.triangles << static_cast<int>(vertex);
                }
            }
          triangleFile.close();
          mesh.artifacts << triangleName;
        }
    }
  catch (const std::bad_alloc&)
    {
      return false;
    }

  return mesh.vertices.count() == expectedVertices &&
    mesh.triangles.count()/3 == expectedTriangles &&
    validMesh(mesh.vertices, mesh.normals, mesh.colors, mesh.triangles);
}

void removeSlabArtifacts(const QStringList& artifacts)
{
  for (const QString& artifact : artifacts)
    if (!QFile::remove(artifact))
      qWarning() << "Cannot remove mesh slab artifact" << artifact;
}
}


//------------------------------------------
//------------------------------------------
void
MeshTools::smoothMesh(QVector<QVector3D>& V,
		      QVector<QVector3D>& N,
		      QVector<int>& T,
		      int ntimes,
		      bool showProgress)
{
  QVector<QVector3D> noColors;
  if (ntimes < 0 || !validMesh(V, N, noColors, T))
    {
      qWarning() << "Cannot smooth an inconsistent mesh";
      return;
    }

  if (N.isEmpty())
    N.resize(V.count());

  QProgressDialog progress("Mesh smoothing in progress ... ",
			   QString(),
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
  if (showProgress)
    progress.setMinimumDuration(0);
  else
    progress.close();
  
  
  QVector<QVector3D> newV;
  newV = V;
  
  int nv = V.count();
  
  //----------------------------
  // create incidence matrix
  QMultiMap<int, int> imat;
  int ntri = T.count()/3;
  for(int i=0; i<ntri; i++)
    {
      if (showProgress)
	{
	  if (i%10000 == 0)
	    {
	      progress.setValue((int)(100.0*(float)i/(float)(ntri)));
	      if (qApp) qApp->processEvents();
	    }
	}

      int a = T[3*i+0];
      int b = T[3*i+1];
      int c = T[3*i+2];

      imat.insert(a, b);
      imat.insert(b, a);
      imat.insert(a, c);
      imat.insert(c, a);
      imat.insert(b, c);
      imat.insert(c, b);
    }
  //----------------------------

  //----------------------------
  // smooth vertices
  if (showProgress)
    progress.setLabelText("   Smoothing vertices ...");
  for(int nt=0; nt<ntimes; nt++)
    {
      if (showProgress)
	{
	  progress.setValue((int)(100.0*(float)nt/(float)(ntimes)));
	  if (qApp) qApp->processEvents();
	}

      // deflation step
      for(int i=0; i<nv; i++)
	{
	  QList<int> idx = imat.values(i);
	  QVector3D v0 = V[i];
	  QVector3D v = QVector3D(0,0,0);
	  float sum = 0;
	  for(int j=0; j<idx.count(); j++)
	    {
	      QVector3D vj = V[idx[j]];
	      float ln = (v0-vj).length();
	      if (ln > 0)
		{
		  sum += 1.0/ln;
		  v = v + vj/ln;
		}
	    }
	  if (sum > 0)
	    v0 = v0 + 0.9*(v/sum - v0);
	  newV[i] = v0;
	}

      //inflation step
      for(int i=0; i<nv; i++)
	{
	  QList<int> idx = imat.values(i);
	  QVector3D v0 = newV[i];
	  QVector3D v = QVector3D(0,0,0);
	  float sum = 0;
	  for(int j=0; j<idx.count(); j++)
	    {
	      QVector3D vj = newV[idx[j]];
	      float ln = (v0-vj).length();
	      if (ln > 0)
		{
		  sum += 1.0/ln;
		  v = v + vj/ln;
		}
	    }
	  if (sum > 0)
	    v0 = v0 - 0.5*(v/sum - v0);

	  V[i] = v0;
	}
    }
  //----------------------------


  //----------------------------
  if (showProgress)
    progress.setLabelText("   Calculate normals ...");
  // now calculate normals
  for(int i=0; i<nv; i++)
    newV[i] = QVector3D(0,0,0);

  QVector<int> nvs;
  nvs.resize(nv);
  nvs.fill(0);

  for(int i=0; i<ntri; i++)
    {
      if (showProgress)
	{
	  if (i%10000 == 0)
	    {
	      progress.setValue((int)(100.0*(float)i/(float)(ntri)));
	      if (qApp) qApp->processEvents();
	    }
	}

      int a = T[3*i+0];
      int b = T[3*i+1];
      int c = T[3*i+2];

      QVector3D va = V[a];
      QVector3D vb = V[b];
      QVector3D vc = V[c];
      QVector3D v0 = (vb-va).normalized();
      QVector3D v1 = (vc-va).normalized();      
      QVector3D vn = QVector3D::crossProduct(v1,v0);
      
      newV[a] += vn;
      newV[b] += vn;
      newV[c] += vn;

      nvs[a]++;
      nvs[b]++;
      nvs[c]++;
    }

  for(int i=0; i<nv; i++)
    N[i] = nvs[i] > 0 ? newV[i]/nvs[i] : QVector3D();
  //----------------------------

  if (showProgress)
    progress.setValue(100);
}
//------------------------------------------
//------------------------------------------




//------------------------------------------
//------------------------------------------
bool
MeshTools::saveToOBJ(QString objflnm,
		     QVector<QVector3D> V,
		     QVector<QVector3D> N,
		     QVector<int> T,
		     bool showProgress)
{
  QVector<QVector3D> C;
  return saveToOBJ(objflnm, V, N, C, T, showProgress);
}

bool
MeshTools::saveToOBJ(QString flnm,
		     int nSlabs,
		     int nvertices, int ntriangles)
{
  QProgressDialog progress("Saving mesh ...",
			   QString(),
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);


  SlabMeshData mesh;
  if (!loadSlabMesh(flnm, nSlabs, nvertices, ntriangles,
                    true, true, mesh))
    {
      qWarning() << "Cannot read OBJ mesh slab files for" << flnm;
      return false;
    }
  progress.setValue(100);
  if (qApp)
    qApp->processEvents();

  if (!saveToOBJ(flnm, mesh.vertices, mesh.normals,
                 mesh.colors, mesh.triangles))
    return false;
  removeSlabArtifacts(mesh.artifacts);
  return true;
}

bool
MeshTools::saveToOBJ(QString objflnm,
		     QVector<QVector3D> V,
		     QVector<QVector3D> N,
		     QVector<QVector3D> C,
		     QVector<int> T,
		     bool showProgress)
{
  Q_UNUSED(showProgress);
  if (!validMesh(V, N, C, T))
    {
      qWarning() << "Invalid mesh passed to OBJ writer" << objflnm;
      return false;
    }

  QSaveFile fobj(objflnm);
  if (!fobj.open(QFile::WriteOnly | QFile::Text))
    {
      qWarning() << "Cannot open OBJ output" << objflnm << fobj.errorString();
      return false;
    }
  QTextStream out(&fobj);
  out.setLocale(QLocale::c());
  out << "#\n";
  out << "#  Wavefront OBJ generated by Drishti\n";
  out << "#\n";
  out << "#  https://github.com/nci/drishti\n";
  out << "#\n";
  out << QString("# %1 vertices\n").arg(V.count());
  if (N.count() > 0)
    out << QString("# %1 normals\n").arg(N.count());
  out << QString("# %1 triangles\n").arg(T.count()/3);

  out << "g\n";
  if (C.count() == 0)
    {
      for(int i=0; i<V.count(); i++)
	out << "v " << QString("%1 %2 %3\n").arg(V[i].x()).arg(V[i].y()).arg(V[i].z());
    }
  else
    {
      for(int i=0; i<V.count(); i++)
	out << "v " << QString("%1 %2 %3  %4 %5 %6\n").arg(V[i].x()).arg(V[i].y()).arg(V[i].z()).\
	                                               arg(C[i].x()/255.0).arg(C[i].y()/255.0).arg(C[i].z()/255.0);
    }

  if (N.count() > 0)
    {
      out << "g\n";
      for(int i=0; i<N.count(); i++)
	out << "vn "<< QString("%1 %2 %3\n").arg(N[i].x()).arg(N[i].y()).arg(N[i].z());
    }

  if (N.count() > 0) // with normal and no texcoord
    {      
      out << "g\n";
      for(int i=0; i<T.count()/3; i++)
	out << "f " << QString("%1//%1 %2//%2 %3//%3\n").arg(T[3*i+2]+1).arg(T[3*i+1]+1).arg(T[3*i+0]+1);
    }
  else // no normals and no texcoord
    {      
      out << "g\n";
      for(int i=0; i<T.count()/3; i++)
	out << "f " << QString("%1 %2 %3\n").arg(T[3*i+2]+1).arg(T[3*i+1]+1).arg(T[3*i+0]+1);
    }

  out.flush();
  if (out.status() != QTextStream::Ok ||
      fobj.error() != QFileDevice::NoError || !fobj.commit())
    {
      qWarning() << "Cannot complete OBJ output" << objflnm
                 << fobj.errorString();
      fobj.cancelWriting();
      return false;
    }
  return true;
}
//------------------------------------------
//------------------------------------------





//------------------------------------------
//------------------------------------------
bool
MeshTools::saveToPLY(QString flnm,
		     QVector<QVector3D> V,
		     QVector<QVector3D> N,
		     QVector<int> T,
		     bool showProgress)
{
  QVector<QVector3D> C;
  return saveToPLY(flnm, V, N, C, T, showProgress);
}
bool
MeshTools::saveToPLY(QString flnm,
		     int nSlabs,
		     int nvertices, int ntriangles,
		     bool bin)
{
  QProgressDialog progress("Saving mesh ...",
			   QString(),
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
  SlabMeshData mesh;
  if (!loadSlabMesh(flnm, nSlabs, nvertices, ntriangles,
                    true, true, mesh))
    {
      qWarning() << "Cannot read PLY mesh slab files for" << flnm;
      return false;
    }
  progress.setValue(100);
  if (qApp)
    qApp->processEvents();

  if (!saveToPLY(flnm, mesh.vertices, mesh.normals,
                 mesh.colors, mesh.triangles, true, bin))
    return false;
  removeSlabArtifacts(mesh.artifacts);
  return true;
}

bool
MeshTools::saveToPLY(QString flnm,
		     QVector<QVector3D> V,
		     QVector<QVector3D> N,
		     QVector<QVector3D> C,
		     QVector<int> T,
		     bool showProgress,
		     bool binary)
{
  QProgressDialog progress("Saving mesh ...",
			   QString(),
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
  if (showProgress)
    progress.setMinimumDuration(0);
  else
    progress.close();

  if (!validMesh(V, N, C, T))
    {
      qWarning() << "Invalid mesh passed to PLY writer" << flnm;
      return false;
    }

  const int nvertices = V.count();
  const int ntriangles = T.count()/3;
  QSaveFile output(flnm);
  if (!output.open(QFile::WriteOnly))
    {
      qWarning() << "Cannot open PLY output" << flnm << output.errorString();
      return false;
    }

  QByteArray header("ply\n");
  header += binary ? "format binary_little_endian 1.0\n" :
                     "format ascii 1.0\n";
  header += "element vertex " + QByteArray::number(nvertices) + "\n";
  header += "property float x\nproperty float y\nproperty float z\n";
  header += "property float nx\nproperty float ny\nproperty float nz\n";
  header += "property uchar red\nproperty uchar green\nproperty uchar blue\n";
  header += "element face " + QByteArray::number(ntriangles) + "\n";
  header += "property list uchar int vertex_indices\nend_header\n";
  if (!writeAll(output, header.constData(), header.size()))
    {
      qWarning() << "Cannot write PLY header" << flnm << output.errorString();
      output.cancelWriting();
      return false;
    }

  bool ok = true;
  if (binary)
    {
      BufferedDeviceWriter writer(output);
      for (int ni=0; ok && ni<nvertices; ++ni)
        {
          if (showProgress && ni%10000 == 0)
            {
              progress.setValue(nvertices > 0 ? 80*ni/nvertices : 80);
              if (qApp) qApp->processEvents();
            }

          const QVector3D normal = N.isEmpty() ? QVector3D() : N[ni];
          char record[27];
          storeFloatLittleEndian(record+0, V[ni].x());
          storeFloatLittleEndian(record+4, V[ni].y());
          storeFloatLittleEndian(record+8, V[ni].z());
          storeFloatLittleEndian(record+12, normal.x());
          storeFloatLittleEndian(record+16, normal.y());
          storeFloatLittleEndian(record+20, normal.z());
          record[24] = static_cast<char>(C.isEmpty() ? 200 :
                            qBound(0, qRound(C[ni].x()), 255));
          record[25] = static_cast<char>(C.isEmpty() ? 200 :
                            qBound(0, qRound(C[ni].y()), 255));
          record[26] = static_cast<char>(C.isEmpty() ? 200 :
                            qBound(0, qRound(C[ni].z()), 255));
          ok = writer.append(record, sizeof(record));
        }
      for (int ni=0; ok && ni<ntriangles; ++ni)
        {
          char record[13];
          record[0] = 3;
          storeIntLittleEndian(record+1, T[3*ni+2]);
          storeIntLittleEndian(record+5, T[3*ni+1]);
          storeIntLittleEndian(record+9, T[3*ni+0]);
          ok = writer.append(record, sizeof(record));
        }
      ok = ok && writer.flush();
    }
  else
    {
      QTextStream stream(&output);
      stream.setLocale(QLocale::c());
      stream.setRealNumberPrecision(9);
      for (int ni=0; ni<nvertices; ++ni)
        {
          if (showProgress && ni%10000 == 0)
            {
              progress.setValue(nvertices > 0 ? 80*ni/nvertices : 80);
              if (qApp) qApp->processEvents();
            }
          const QVector3D normal = N.isEmpty() ? QVector3D() : N[ni];
          const int red = C.isEmpty() ? 200 : qBound(0, qRound(C[ni].x()), 255);
          const int green = C.isEmpty() ? 200 : qBound(0, qRound(C[ni].y()), 255);
          const int blue = C.isEmpty() ? 200 : qBound(0, qRound(C[ni].z()), 255);
          stream << V[ni].x() << ' ' << V[ni].y() << ' ' << V[ni].z() << ' '
                 << normal.x() << ' ' << normal.y() << ' ' << normal.z() << ' '
                 << red << ' ' << green << ' ' << blue << '\n';
        }
      for (int ni=0; ni<ntriangles; ++ni)
        stream << "3 " << T[3*ni+2] << ' ' << T[3*ni+1] << ' '
               << T[3*ni+0] << '\n';
      stream.flush();
      ok = stream.status() == QTextStream::Ok;
    }

  if (!ok || output.error() != QFileDevice::NoError || !output.commit())
    {
      qWarning() << "Cannot complete PLY output" << flnm
                 << output.errorString();
      output.cancelWriting();
      return false;
    }
  if (showProgress)
    progress.setValue(100);
  return true;
}
//------------------------------------------
//------------------------------------------





//------------------------------------------
//------------------------------------------
bool
MeshTools::saveToSTL(QString flnm,
		     int nSlabs,
		     int nvertices, int ntriangles)
{
  QProgressDialog progress("Saving mesh ...",
			   QString(),
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
  SlabMeshData mesh;
  if (!loadSlabMesh(flnm, nSlabs, nvertices, ntriangles,
                    false, false, mesh))
    {
      qWarning() << "Cannot read STL mesh slab files for" << flnm;
      return false;
    }
  progress.setValue(100);
  if (qApp)
    qApp->processEvents();

  if (!saveToSTL(flnm, mesh.vertices, mesh.normals, mesh.triangles))
    return false;
  removeSlabArtifacts(mesh.artifacts);
  return true;
}

bool
MeshTools::saveToSTL(QString flnm,
		     QVector<QVector3D> V,
		     QVector<QVector3D> N,
		     QVector<int> T,
		     bool showProgress)
{
  QProgressDialog progress("Saving mesh ...",
			   QString(),
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
  if (showProgress)
    progress.setMinimumDuration(0);
  else
    progress.close();

  QVector<QVector3D> noColors;
  if (!validMesh(V, N, noColors, T))
    {
      qWarning() << "Invalid mesh passed to STL writer" << flnm;
      return false;
    }

  const int ntri = T.count()/3;
  QSaveFile output(flnm);
  if (!output.open(QFile::WriteOnly))
    {
      qWarning() << "Cannot open STL output" << flnm << output.errorString();
      return false;
    }

  char header[84] = {};
  const QByteArray title("Drishti generated STL file.");
  std::memcpy(header, title.constData(), title.size());
  storeIntLittleEndian(header+80, ntri);
  BufferedDeviceWriter writer(output);
  bool ok = writer.append(header, sizeof(header));

  for(int ni=0; ok && ni<ntri; ni++)
    {
      if (showProgress)
	{
	  if (ni%10000 == 0)
	    {
	      progress.setValue((int)(100.0*(float)ni/(float)(ntri)));
	      if (qApp) qApp->processEvents();
	    }
	}

      const int k = T[3*ni+0];
      const int j = T[3*ni+1];
      const int i = T[3*ni+2];
      QVector3D normal;
      if (!N.isEmpty())
        normal = -(N[i]+N[j]+N[k]);
      if (normal.lengthSquared() <= 0)
        normal = QVector3D::crossProduct(V[j]-V[i], V[k]-V[i]);
      if (normal.lengthSquared() > 0)
        normal.normalize();

      char record[50] = {};
      const float values[12] = {
        normal.x(), normal.y(), normal.z(),
        V[i].x(), V[i].y(), V[i].z(),
        V[j].x(), V[j].y(), V[j].z(),
        V[k].x(), V[k].y(), V[k].z()
      };
      for (int value=0; value<12; ++value)
        storeFloatLittleEndian(record+4*value, values[value]);
      ok = writer.append(record, sizeof(record));
    }

  ok = ok && writer.flush();
  if (!ok || output.error() != QFileDevice::NoError || !output.commit())
    {
      qWarning() << "Cannot complete STL output" << flnm
                 << output.errorString();
      output.cancelWriting();
      return false;
    }
  if (showProgress)
    progress.setValue(100);
  return true;
}
//------------------------------------------
//------------------------------------------





//------------------------------------------
//------------------------------------------
bool
MeshTools::saveToTetrahedralMesh(QString flnm,
				 int nSlabs,
				 int nvertices, int ntriangles)
{
  QProgressDialog progress("Saving tetrahedral mesh ...",
			   QString(),
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);

  SlabMeshData mesh;
  if (!loadSlabMesh(flnm, nSlabs, nvertices, ntriangles,
                    true, true, mesh))
    {
      qWarning() << "Cannot read tetrahedral mesh slab files for" << flnm;
      return false;
    }
  progress.setValue(100);
  if (qApp)
    qApp->processEvents();

  if (!saveToTetrahedralMesh(flnm, mesh.vertices, mesh.triangles))
    return false;
  removeSlabArtifacts(mesh.artifacts);
  return true;
}

bool
MeshTools::saveToTetrahedralMesh(QString flnm,
				 QVector<QVector3D> V,
				 QVector<int> T,
				 bool showProgress)
{
  QProgressDialog progress("Saving tetrahedral mesh ...",
			   QString(),
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);

  QVector<QVector3D> noNormals;
  QVector<QVector3D> noColors;
  if (!validMesh(V, noNormals, noColors, T))
    {
      qWarning() << "Invalid mesh passed to tetrahedral writer" << flnm;
      return false;
    }

  QTemporaryDir temporaryDirectory;
  if (!temporaryDirectory.isValid())
    {
      qWarning() << "Cannot create temporary directory for tetrahedral output"
                 << flnm << temporaryDirectory.errorString();
      return false;
    }

  const QString stl_flnm = temporaryDirectory.filePath("surface.stl");
  const QString gmsh_flnm = temporaryDirectory.filePath("volume.msh");
  bool gmshInitialized = false;

  try
      {
      gmsh::initialize();
      gmshInitialized = true;

      const int nThreads = qMax(1, (int)(QThread::idealThreadCount()));
      gmsh::option::setNumber("General.NumThreads", nThreads);
      gmsh::option::setNumber("Mesh.Algorithm3D", 10);
      gmsh::option::setNumber("Mesh.Optimize", 1);
//-----------------------------------------
// Save STL and load from it
      QVector<QVector3D> N;
      if (!saveToSTL(stl_flnm, V, N, T, false))
        throw std::runtime_error("cannot create temporary STL input");
  
      gmsh::model::add("tetrahedral_model");
	gmsh::merge(stl_flnm.toUtf8().constData());
	//QMessageBox::information(0, "", "merge "+stl_flnm);
	
	// Create a volume from all the surfaces
	gmsh::vectorpair s;
	gmsh::model::getEntities(s, 2);
	std::vector<int> sl;
	for(auto surf : s) sl.push_back(surf.second);
	int l = gmsh::model::geo::addSurfaceLoop(sl);
	gmsh::model::geo::addVolume({l});
//-----------------------------------------

	
////-----------------------------------------
//        gmsh::model::add("tetrahedral_model");
//
//	//--------------
//	// Add vertices to Gmsh
//	progress.setLabelText("Adding points");
//	std::vector<int> vertexTags;
//	for (int i = 0; i < V.count(); ++i)
//	  {
//	    int tag = gmsh::model::geo::addPoint(V[i].x(), V[i].y(), V[i].z());
//	    vertexTags.push_back(tag);
//	  }
//	//--------------
//
//	progress.setValue(20);
//	qApp->processEvents();
//	
//	//--------------
//	// Add triangles to Gmsh
//	progress.setLabelText("Adding triangles");
//	std::vector<int> curveLoops;
//	std::vector<int> surfaceTags;
//        
//	for (int i = 0; i < T.count()/3; ++i)
//	  {
//	    // Create lines for each edge of the triangle
//	    int line1 = gmsh::model::geo::addLine(vertexTags[T[3*i+0]], 
//						  vertexTags[T[3*i+2]]);
//	    int line2 = gmsh::model::geo::addLine(vertexTags[T[3*i+2]], 
//						  vertexTags[T[3*i+1]] );
//	    int line3 = gmsh::model::geo::addLine(vertexTags[T[3*i+1]], 
//						  vertexTags[T[3*i+0]] );
//	    
//	    // Create curve loop from the lines
//	    int curveLoop = gmsh::model::geo::addCurveLoop({line1, line2, line3});
//	    curveLoops.push_back(curveLoop);
//            
//	    // Create surface from the curve loop
//	    int surface = gmsh::model::geo::addPlaneSurface({curveLoop});
//	    surfaceTags.push_back(surface);
//	  }
//	//--------------
//
//	progress.setValue(40);
//	qApp->processEvents();
//		
//	progress.setLabelText("Create surface loop and volume");
//	//--------------
//	// Create surface loop from all surfaces
//	int surfaceLoop = gmsh::model::geo::addSurfaceLoop(surfaceTags);
//	//--------------
//        
//	//--------------
//	// Create volume from surface loop
//	gmsh::model::geo::addVolume({surfaceLoop});
//	//--------------
////-----------------------------------------

	
	progress.setValue(60);
	if (qApp) qApp->processEvents();
	
	progress.setLabelText("Sync");
	//--------------
	// Synchronize the geometry
        gmsh::model::geo::synchronize();
	//--------------

	progress.setValue(80);
	if (qApp) qApp->processEvents();

	progress.setLabelText("Creating tetrahedral mesh");
	//--------------
        // Generate 3D mesh (tetrahedral)
        gmsh::model::mesh::generate(3);
	//--------------

	
//	// Optimize the mesh
//	QMessageBox::information(0, "", "optimizing tetrahedral mesh");
//      gmsh::model::mesh::optimize("Netgen");


	progress.setValue(90);
	if (qApp) qApp->processEvents();

	progress.setLabelText("Saving tetrahedral mesh");
	//--------------
	// Save the mesh to a file
	gmsh::write(gmsh_flnm.toUtf8().constData());
	//--------------

	// Finalize Gmsh
	gmsh::finalize();
	gmshInitialized = false;
	progress.setValue(100);
      }
    catch (const std::exception &e)
      {
	if (gmshInitialized)
	  {
	    try { gmsh::finalize(); } catch (...) {}
	  }
	progress.setValue(100);
	qWarning() << "Tetrahedral mesh generation failed for" << flnm
	           << e.what();
	return false;
      }

  QFile generated(gmsh_flnm);
  if (!generated.open(QFile::ReadOnly) || generated.size() <= 0)
    {
      qWarning() << "Gmsh did not create a valid output" << gmsh_flnm
                 << generated.errorString();
      return false;
    }

  QSaveFile output(flnm);
  if (!output.open(QFile::WriteOnly))
    {
      qWarning() << "Cannot open tetrahedral output" << flnm
                 << output.errorString();
      return false;
    }
  QByteArray buffer(1024*1024, '\0');
  bool copied = true;
  while (!generated.atEnd())
    {
      const qint64 bytes = generated.read(buffer.data(), buffer.size());
      if (bytes <= 0)
        {
          copied = false;
          break;
        }
      if (!writeAll(output, buffer.constData(), bytes))
        {
          copied = false;
          break;
        }
    }
  copied = copied && generated.error() == QFileDevice::NoError &&
           output.error() == QFileDevice::NoError;
  if (!copied || !output.commit())
    {
      qWarning() << "Cannot install tetrahedral output" << flnm
                 << output.errorString();
      output.cancelWriting();
      return false;
    }

  return true;
}
//------------------------------------------
//------------------------------------------
