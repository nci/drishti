#include "vdbvolume.h"
#include <openvdb/tools/GridTransformer.h>
#include <openvdb/tools/Filter.h>
#include <openvdb/tools/Morphology.h>
#include <openvdb/tools/VolumeToMesh.h>

#include <QMessageBox>
#include <QProgressDialog>
#include <QInputDialog>
#include <QFileDialog>
#include <QTextStream>
#include <QtMath>
#include <QApplication>

using namespace std;

VdbVolume::VdbVolume()
{
  openvdb::initialize();

  m_vdbGrid = openvdb::FloatGrid::create();  
}

VdbVolume::~VdbVolume()
{
  m_vdbGrid->clear();
}

void
VdbVolume::addSliceToVDB(unsigned char *data,
			 int x, int nY, int nZ,
			 int bType, int bValue)
{
  openvdb::FloatGrid::Accessor accessor = m_vdbGrid->getAccessor();

  openvdb::Coord ijk;
  int &d = ijk[0];
  int &w = ijk[1];
  int &h = ijk[2];

  d = x;
  
  if (bType == -1) // anything less than bValue is background
    {
      for (w=0; w<nY; w++)
	{
	  for (h=0; h<nZ; h++)
	    {
	      int value = data[w*nZ + h];
	      if (value > bValue)
		accessor.setValue(ijk, float(value));	  
	    }
	}
    }
  if (bType == 0) // bValue is background
    {
      for (w=0; w<nY; w++)
	{
	  for (h=0; h<nZ; h++)
	    {
	      int value = data[w*nZ + h];
	      if (value != bValue)
		accessor.setValue(ijk, float(value));	  
	    }
	}
    }
  if (bType == 1) // anything greater than bValue is background
    {
      for (w=0; w<nY; w++)
	{
	  for (h=0; h<nZ; h++)
	    {
	      int value = data[w*nZ + h];
	      if (value < bValue)
		accessor.setValue(ijk, float(value));	  
	    }
	}
    }
}

void
VdbVolume::generateVDB(unsigned char *data,
		       int nX, int nY, int nZ,
		       int bType, int bValue,
		       QProgressBar *progress)
{
  openvdb::FloatGrid::Accessor accessor = m_vdbGrid->getAccessor();

  openvdb::Coord ijk;
  int &d = ijk[0];
  int &w = ijk[1];
  int &h = ijk[2];
  
//  QProgressDialog progress("Generating VDB",
//			   "Cancel",
//			   0, 100,
//			   0,
//			   Qt::Dialog|Qt::WindowStaysOnTopHint);
//  progress.setMinimumDuration(0);
//  progress.resize(500, 100);
//  progress.move(QCursor::pos());

  if (bType == -1) // anything less than bValue is background
    {
      for (d=0; d<nX; d++)
	{
	  if (progress)
	    {
	      progress->setValue((int)(100.0*(float)d/(float)(nZ)));
	      qApp->processEvents();
	    }
	  for (w=0; w<nY; w++)
	    {
	      for (h=0; h<nZ; h++)
		{
		  int value = data[d*(long int)(nY*nZ) + (long int)(w*nZ) + h];
		  if (value > bValue)
		    accessor.setValue(ijk, float(value));
		}
	    }
	}
    }
  if (bType == 0) // bValue is background
    {
      for (d=0; d<nX; d++)
	{
	  if (progress)
	    {
	      progress->setValue((int)(100.0*(float)d/(float)(nZ)));
	      qApp->processEvents();
	    }
	  for (w=0; w<nY; w++)
	    {
	      for (h=0; h<nZ; h++)
		{
		  int value = data[d*(long int)(nY*nZ) + (long int)(w*nZ) + h];
		  if (value != bValue)
		    accessor.setValue(ijk, float(value));
		}
	    }
	}
    }
  if (bType == 1) // anything greater than bValue is background
    {
      for (d=0; d<nX; d++)
	{
	  if (progress)
	    {
	      progress->setValue((int)(100.0*(float)d/(float)(nZ)));
	      qApp->processEvents();
	    }
	  for (w=0; w<nY; w++)
	    {
	      for (h=0; h<nZ; h++)
		{
		  int value = data[d*(long int)(nY*nZ) + (long int)(w*nZ) + h];
		  if (value < bValue)
		    accessor.setValue(ijk, float(value));
		}
	    }
	}
    }

  
  if (progress)
    {
      progress->setValue(100);
      qApp->processEvents();
    }

  
  //QMessageBox::information(0, "Active Voxels", QString("Active voxels : %1").arg(m_vdbGrid->activeVoxelCount()));
}

void
VdbVolume::mean(int width, int iterations)
{
  openvdb::tools::Filter<openvdb::FloatGrid> filter(*m_vdbGrid);
  filter.mean(width, iterations);
}


void
VdbVolume::gaussian(int width, int iterations)
{
  openvdb::tools::Filter<openvdb::FloatGrid> filter(*m_vdbGrid);
  filter.gaussian(width, iterations);
}


void
VdbVolume::dilate(int iter)
{
  openvdb::tools::dilateActiveValues(m_vdbGrid->tree(),
				     iter, // iterations
				     openvdb::tools::NN_FACE, // NearestNeighbors
				     openvdb::tools::IGNORE_TILES);
}

void
VdbVolume::generateMesh(float isovalue, float adaptivity,
			QVector<QVector3D> &V, QVector<QVector3D> &VN, QVector<int> &T)
{
  // construct surface mesh
  vector<openvdb::Vec3s> points;
  vector<openvdb::Vec3I> triangles;
  vector<openvdb::Vec4I> quads;
  bool relaxDisorientedTriangles = true;
  

  openvdb::tools::volumeToMesh(*m_vdbGrid,
			       points,
			       triangles,
			       quads,
			       (double)isovalue,
			       (double)adaptivity,
			       relaxDisorientedTriangles);

  V.clear();
  VN.clear();
  T.clear();
  
  for (int i=0; i<points.size(); i++)
    {
      openvdb::Vec3s p = points[i];
      V << QVector3D(p[2], p[1], p[0]);
    }
  for (int i=0; i<triangles.size(); i++)
    {
      openvdb::Vec3I t = triangles[i];
      
      // check for zero area faces
      QVector3D p1 = V[t[0]];
      QVector3D p2 = V[t[1]];
      QVector3D p3 = V[t[2]];
      float p21L = p2.distanceToPoint(p1);
      float p31L = p3.distanceToPoint(p1);
      float p32L = p3.distanceToPoint(p2);

      if (p21L*p31L*p32L > 0) T << t[0] << t[2] << t[1];
    }
  for (int i=0; i<quads.size(); i++)
    {
      openvdb::Vec4I q = quads[i];

      // split into two triangles by the shortest diagonal across the quad.
      openvdb::Vec3s p;
      p = points[q[0]]; QVector3D v0(p.x(), p.y(), p.z());
      p = points[q[1]]; QVector3D v1(p.x(), p.y(), p.z());
      p = points[q[2]]; QVector3D v2(p.x(), p.y(), p.z());
      p = points[q[3]]; QVector3D v3(p.x(), p.y(), p.z());
      
      float d1 = v0.distanceToPoint(v2);
      float d2 = v1.distanceToPoint(v3);

      // check for zero area faces
      float p10L = v1.distanceToPoint(v0);
      float p30L = v3.distanceToPoint(v0);
      float p21L = v1.distanceToPoint(v2);
      float p32L = v3.distanceToPoint(v2);

      if (d1 > d2)
	{
	  if (p10L*p30L > 0) T << q[0] << q[3] << q[1];
	  if (p21L*p32L > 0) T << q[1] << q[3] << q[2];
	}
      else
	{
	  if (p10L*p21L > 0) T << q[0] << q[2] << q[1];
	  if (p30L*p32L > 0) T << q[0] << q[3] << q[2];
	}
    }


  // generate vertex normals
  VN.resize(V.count());  
  for(int i=0; i<VN.count(); i++)
    VN[i] = QVector3D(0,0,0);
  for (int i=0; i<T.count()/3; i++)
    {
      int a = T[3*i+0];
      int b = T[3*i+1];
      int c = T[3*i+2];

      QVector3D p1 = V[a];
      QVector3D p2 = V[b];
      QVector3D p3 = V[c];

      QVector3D p21 = p2-p1;
      QVector3D p31 = p3-p1;
      QVector3D p32 = p3-p2;

      QVector3D faceNormal = QVector3D::crossProduct(p31, p21);
      
      float p21L = p21.length();
      float p31L = p31.length();
      float p32L = p32.length();
	  
      float angleA = qAcos(QVector3D::dotProduct(p21 , p31)/(p21L*p31L));
      float angleB = qAcos(QVector3D::dotProduct(p32 ,-p21)/(p32L*p21L));
      float angleC = qAcos(QVector3D::dotProduct(-p31,-p32)/(p31L*p32L));

      // angle weighted normal averaging
      VN[a] += angleA * faceNormal;
      VN[b] += angleB * faceNormal;
      VN[c] += angleC * faceNormal;
    }

  for (int i=0; i<VN.count(); i++)
    VN[i].normalize();
}
