#include <GL/glew.h>
#include "volumemeasure.h"
#include "volumeoperations.h"
#include "volumeinformation.h"
#include "global.h"
#include "mybitarray.h"
#include "morphslice.h"
#include "vdbvolume.h"
#include "geometryobjects.h"
#include "staticfunctions.h"
#include "binarydistancetransform.h"

#include <QtConcurrentMap>
#include <QFileDialog>



uchar* VolumeMeasure::m_volData = 0;
ushort* VolumeMeasure::m_volDataUS = 0;

ushort* VolumeMeasure::m_maskDataUS = 0;


void VolumeMeasure::setVolData(uchar *v)
{
  m_volData = v;
  m_volDataUS = 0;
  if (Global::bytesPerVoxel() == 2)
    m_volDataUS = (ushort*) m_volData;
}
void VolumeMeasure::setMaskData(uchar *v)
{
  m_maskDataUS = (ushort*)v;
}


int VolumeMeasure::m_depth = 0;
int VolumeMeasure::m_width = 0;
int VolumeMeasure::m_height = 0;
void VolumeMeasure::setGridSize(int d, int w, int h)
{
  m_depth = d;
  m_width = w;
  m_height = h;
}


QList<int>
VolumeMeasure::getLabels(int ds, int ws, int hs,
			 int de, int we, int he,
			 int tag)
{
  QList<int> ut;

  uchar *lut = Global::lut();
  uchar *tagColors = Global::tagColors();

  for(qint64 d=ds; d<=de; d++)
    for(qint64 w=ws; w<=we; w++)
      for(qint64 h=hs; h<=he; h++)
	{
	  qint64 idx = ((qint64)d)*m_width*m_height + ((qint64)w)*m_height + h;
	  int val = m_volData[idx];
	  if (m_volDataUS) val = m_volDataUS[idx];

	  int mtag = m_maskDataUS[idx];

	  bool opaque = (lut[4*val+3]*tagColors[4*mtag+3] > 0);      

	  if (opaque && !ut.contains(mtag))
	    {
	      if (tag == -1 || tag == mtag)
		ut << mtag;
	    }
	}

  return ut;
}


QMap<int, float>
VolumeMeasure::volume(Vec bmin, Vec bmax, int tag)
{
  QMap<int, float> vdbVolume;

  VolumeInformation pvlInfo;
  pvlInfo = VolumeInformation::volumeInformation();
  Vec voxelSize = pvlInfo.voxelSize;

  QProgressDialog progress("Calculating Volume",
			   QString(),
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);

  int ds = qMax(0, qFloor(bmin.z));
  int ws = qMax(0, qFloor(bmin.y));
  int hs = qMax(0, qFloor(bmin.x));

  int de = qMin(qCeil(bmax.z), m_depth-1);
  int we = qMin(qCeil(bmax.y), m_width-1);
  int he = qMin(qCeil(bmax.x), m_height-1);

  qint64 mx = he-hs+1;
  qint64 my = we-ws+1;
  qint64 mz = de-ds+1;


  QList<int> ut = getLabels(ds,ws,hs,de,we,he,tag);

  if (ut.count() == 0)
    {
      QMessageBox::information(0, "Error", QString("No voxel found with label %1\nComputing total visible volume").arg(tag));
    }

  // compute total visible volume as well for finding percentages
  ut << -1;
  
  qSort(ut);

    
  MyBitArray bitmask;
  bitmask.resize(mx*my*mz);

  for(int i=0; i<ut.count(); i++)
    {
      progress.setValue(100*(float)i/(float)ut.count());
      qApp->processEvents();
      
      bitmask.fill(false);
      
      VolumeOperations::getVisibleRegion(ds, ws, hs,
					 de, we, he,
					 ut[i], false,  // no tag zero checking
					 0, 0, 1,
					 bitmask,
					 false);
      
      VdbVolume vdb;
      vdb.setVoxelSize(voxelSize.x, voxelSize.y, voxelSize.z);
      openvdb::FloatGrid::Accessor accessor = vdb.getAccessor();
      openvdb::Coord ijk;
      int &d = ijk[0];
      int &w = ijk[1];
      int &h = ijk[2];
      for(d=0; d<mz; d++)
	for(w=0; w<my; w++)
	  for(h=0; h<mx; h++)
	    {
	      qint64 bidx = ((qint64)d)*mx*my+w*mx+h;
	      if (bitmask.testBit(bidx))
		accessor.setValue(ijk, 255);
	    }

      vdb.convertToLevelSet(128);
      vdbVolume[ut[i]] = vdb.volume();
    }

  progress.setValue(100);

  return vdbVolume;
}

void
VolumeMeasure::getVolume(Vec bmin, Vec bmax, int tag)
{
  VolumeInformation pvlInfo;
  pvlInfo = VolumeInformation::volumeInformation();
  Vec voxelSize = pvlInfo.voxelSize;

  
  QMap<int, float> vdbVolume = volume(bmin, bmax, tag);
  QList<int> ut = vdbVolume.keys();

  if (ut.count() == 0)
    return;

  QString mesg;
  mesg += QString("Voxel Size : %1, %2, %3 %4\n").\
                                arg(voxelSize.x).arg(voxelSize.y).arg(voxelSize.z).\
                                arg(pvlInfo.voxelUnitStringShort());
  //if (tag == -1)
    {
      mesg += "------------------------------\n";
      mesg += QString("Total Visible Volume : %1%2^3\n").\
	                              arg(vdbVolume[-1]).\
	                              arg(pvlInfo.voxelUnitStringShort());
      mesg += "------------------------------\n";
      ut.removeAt(0);
    }
  if (ut.count() > 0)
    {
      mesg += "------------------------------\n";
      mesg += QString(" Label : Volume %1^3: % of total\n").\
	                              arg(pvlInfo.voxelUnitStringShort());
      mesg += "------------------------------\n";
      for(int i=0; i<ut.count(); i++)
	{
	  float p = 100.0*vdbVolume[ut[i]]/vdbVolume[-1];
	  mesg += QString("%1 : %2 : %3\n").arg(ut[i], 6).\
	                                    arg(vdbVolume[ut[i]]).\
	                                    arg(p, 0, 'f', 2);
	}
    }
  
  StaticFunctions::showMessage("Volume", mesg);

  StaticFunctions::saveMesgToFile(Global::previousDirectory(), mesg);
}



QMap<int, float>
VolumeMeasure::surfaceArea(Vec bmin, Vec bmax, int tag)
{
  QMap<int, float> vdbArea;
  
  VolumeInformation pvlInfo;
  pvlInfo = VolumeInformation::volumeInformation();
  Vec voxelSize = pvlInfo.voxelSize;

  QProgressDialog progress("Calculating Surface Area",
			   QString(),
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);

  int ds = qMax(0, qFloor(bmin.z));
  int ws = qMax(0, qFloor(bmin.y));
  int hs = qMax(0, qFloor(bmin.x));

  int de = qMin(qCeil(bmax.z), m_depth-1);
  int we = qMin(qCeil(bmax.y), m_width-1);
  int he = qMin(qCeil(bmax.x), m_height-1);

  qint64 mx = he-hs+1;
  qint64 my = we-ws+1;
  qint64 mz = de-ds+1;

  QList<int> ut = getLabels(ds,ws,hs,de,we,he,tag);

  if (tag == -1)
    ut << -1;

  if (ut.count() == 0)
    {
      QMessageBox::information(0, "Error", QString("No voxel found with label %1").arg(tag));
      return vdbArea;
    }
  
  qSort(ut);
  
  MyBitArray bitmask;
  bitmask.resize(mx*my*mz);

  for(int i=0; i<ut.count(); i++)
    {
      progress.setValue(100*(float)i/(float)ut.count());
      qApp->processEvents();
      
      bitmask.fill(false);
      
      VolumeOperations::getVisibleRegion(ds, ws, hs,
					 de, we, he,
					 ut[i], false,  // no tag zero checking
					 0, 0, 1,
					 bitmask,
					 false);  
      
      VdbVolume vdb;
      vdb.setVoxelSize(voxelSize.x, voxelSize.y, voxelSize.z);
      openvdb::FloatGrid::Accessor accessor = vdb.getAccessor();
      openvdb::Coord ijk;
      int &d = ijk[0];
      int &w = ijk[1];
      int &h = ijk[2];
      for(d=0; d<mz; d++)
	for(w=0; w<my; w++)
	  for(h=0; h<mx; h++)
	    {
	      qint64 bidx = ((qint64)d)*mx*my+w*mx+h;
	      if (bitmask.testBit(bidx))
		accessor.setValue(ijk, 255);
	    }

      vdb.convertToLevelSet(128);
      vdbArea[ut[i]] = vdb.area();
    }
  progress.setValue(100);

  return vdbArea;
}

void
VolumeMeasure::getSurfaceArea(Vec bmin, Vec bmax, int tag)
{
  VolumeInformation pvlInfo;
  pvlInfo = VolumeInformation::volumeInformation();
  Vec voxelSize = pvlInfo.voxelSize;

  
  QMap<int, float> vdbArea = surfaceArea(bmin, bmax, tag);
  QList<int> ut = vdbArea.keys();

  if (ut.count() == 0)
    return;



  QString mesg;
  mesg += QString("Voxel Size : %1, %2, %3 %4\n").\
                                arg(voxelSize.x).arg(voxelSize.y).arg(voxelSize.z).\
                                arg(pvlInfo.voxelUnitStringShort());
  if (tag == -1)
    {
      mesg += "------------------------------\n";
      mesg += QString("Total Visible Surface Area : %1%2^2\n").\
	                              arg(vdbArea[-1]).\
	                              arg(pvlInfo.voxelUnitStringShort());
      mesg += "------------------------------\n";
      ut.removeAt(0);
    }
  if (ut.count() > 0)
    {
      mesg += "------------------------------\n";
      mesg += QString(" Label : Surface Area %1^2\n").arg(pvlInfo.voxelUnitStringShort());

      mesg += "------------------------------\n";
      for(int i=0; i<ut.count(); i++)
	{
	  mesg += QString("%1 : %2\n").arg(ut[i], 6).\
	                               arg(vdbArea[ut[i]]);
	}
    }
  
  StaticFunctions::showMessage("Surface Area", mesg);

  StaticFunctions::saveMesgToFile(Global::previousDirectory(), mesg);
}



float
VolumeMeasure::feretDiameter(int mx, int my, int mz, MyBitArray& bitarray)
{
  
  VolumeInformation pvlInfo;
  pvlInfo = VolumeInformation::volumeInformation();
  Vec voxelSize = pvlInfo.voxelSize;

  QList<Vec> svox = VolumeOperations::getSurfaceVoxels(mx, my, mz, bitarray);

  if (svox.count() == 0)
    return 0;

  // apply voxelSize
  for(int i=0; i<svox.count(); i++)
    svox[i] = VECPRODUCT(svox[i], voxelSize);


  float feret = 0;
  for(int i=0; i<svox.count()-1; i++)
    {
      Vec v0 = svox[i];
      for(int j=i+1; j<svox.count(); j++)
      {
	Vec v1 = svox[j];
	float len = (v1-v0).norm();
	feret = qMax(feret, len);
      }
    }

  return feret;
}

void
VolumeMeasure::getFeretDiameter(Vec bmin, Vec bmax, int tag)
{
  int ds = qMax(0, qFloor(bmin.z));
  int ws = qMax(0, qFloor(bmin.y));
  int hs = qMax(0, qFloor(bmin.x));

  int de = qMin(qCeil(bmax.z), m_depth-1);
  int we = qMin(qCeil(bmax.y), m_width-1);
  int he = qMin(qCeil(bmax.x), m_height-1);

  qint64 mx = he-hs+1;
  qint64 my = we-ws+1;
  qint64 mz = de-ds+1;


  QProgressDialog progress("Calculating Feret Diameter",
			   QString(),
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);


  QList<int> ut = getLabels(ds,ws,hs,de,we,he,tag);

  if (ut.count() == 0)
    {
      QMessageBox::information(0, "Error", QString("No voxel found with label %1\nComputing total visible volume").arg(tag));
    }

  qSort(ut);

  
  QList<float> feret;

  
  MyBitArray bitmask;
  bitmask.resize(mx*my*mz);

  for(int i=0; i<ut.count(); i++)
    {
      progress.setValue(100*(float)i/(float)ut.count());
      qApp->processEvents();
      
      bitmask.fill(false);
      
      VolumeOperations::getVisibleRegion(ds, ws, hs,
					 de, we, he,
					 ut[i], false,  // no tag zero checking
					 0, 0, 1,
					 bitmask,
					 false);
      
      feret << feretDiameter(mx, my, mz, bitmask);
    }
  progress.setValue(100);

  
  VolumeInformation pvlInfo;
  pvlInfo = VolumeInformation::volumeInformation();
  Vec voxelSize = pvlInfo.voxelSize;

  QString mesg;
  mesg += QString("Voxel Size : %1, %2, %3 %4\n").\
                                arg(voxelSize.x).arg(voxelSize.y).arg(voxelSize.z).\
                                arg(pvlInfo.voxelUnitStringShort());

  mesg += "------------------------------\n";
  mesg += QString(" Label : Max Feret Diameter (%1)\n").arg(pvlInfo.voxelUnitStringShort());
  
  mesg += "------------------------------\n";
  for(int i=0; i<ut.count(); i++)
    {
      mesg += QString("%1 : %2\n").arg(ut[i], 6).arg(feret[i]);
    }
  
  StaticFunctions::showMessage("Max Feret Diameter", mesg);

  StaticFunctions::saveMesgToFile(Global::previousDirectory(), mesg);
}



void
VolumeMeasure::getSphericity(Vec bmin, Vec bmax, int tag)
{
  int ds = qMax(0, qFloor(bmin.z));
  int ws = qMax(0, qFloor(bmin.y));
  int hs = qMax(0, qFloor(bmin.x));

  int de = qMin(qCeil(bmax.z), m_depth-1);
  int we = qMin(qCeil(bmax.y), m_width-1);
  int he = qMin(qCeil(bmax.x), m_height-1);

  qint64 mx = he-hs+1;
  qint64 my = we-ws+1;
  qint64 mz = de-ds+1;

  
  QList<int> ut = getLabels(ds,ws,hs,de,we,he,tag);

  if (ut.count() == 0)
    {
      QMessageBox::information(0, "Error", QString("No voxel found with label %1\nComputing total visible volume").arg(tag));
    }

  qSort(ut);

  QMap<int, float> vdbVolume = volume(bmin, bmax, tag);
  QMap<int, float> vdbArea = surfaceArea(bmin, bmax, tag);
  QMap<int, float> sphericity;
  for(int i=0; i<ut.count(); i++)
    {
      float V = vdbVolume[ut[i]];
      float A = vdbArea[ut[i]];

      float S = qPow(6*qSqrt(M_PI)*V, 2.0/3.0)/A;

      sphericity[ut[i]] = S;
    }

  QString mesg;
  mesg += "--------------------\n";
  mesg += " Label : Sphericity \n";
  mesg += "--------------------\n";
  for(int i=0; i<ut.count(); i++)
    {
      mesg += QString("%1 : %2\n").arg(ut[i], 6).arg(sphericity[ut[i]]);
    }
  
  StaticFunctions::showMessage("Sphericity", mesg);

  StaticFunctions::saveMesgToFile(Global::previousDirectory(), mesg);
}



void
VolumeMeasure::getEquivalentSphericalDiameter(Vec bmin, Vec bmax, int tag)
{
  int ds = qMax(0, qFloor(bmin.z));
  int ws = qMax(0, qFloor(bmin.y));
  int hs = qMax(0, qFloor(bmin.x));

  int de = qMin(qCeil(bmax.z), m_depth-1);
  int we = qMin(qCeil(bmax.y), m_width-1);
  int he = qMin(qCeil(bmax.x), m_height-1);

  qint64 mx = he-hs+1;
  qint64 my = we-ws+1;
  qint64 mz = de-ds+1;

  
  QList<int> ut = getLabels(ds,ws,hs,de,we,he,tag);

  if (ut.count() == 0)
    {
      QMessageBox::information(0, "Error", QString("No voxel found with label %1\nComputing total visible volume").arg(tag));
    }

  qSort(ut);

  QMap<int, float> vdbVolume = volume(bmin, bmax, tag);
  QMap<int, float> sphereDiameter;
  for(int i=0; i<ut.count(); i++)
    {
      float V = vdbVolume[ut[i]];

      float D = 2*qPow(3*V/4*M_PI, 1.0/3.0);

      sphereDiameter[ut[i]] = D;
    }
  
  VolumeInformation pvlInfo;
  pvlInfo = VolumeInformation::volumeInformation();
  QString mesg;
  mesg += "------------------------------\n";
  mesg += QString(" Label : Sphere Diameter (%1)\n").	\
		  arg(pvlInfo.voxelUnitStringShort());
  mesg += "------------------------------\n";
  for(int i=0; i<ut.count(); i++)
    {
      mesg += QString("%1 : %2\n").arg(ut[i], 6).arg(sphereDiameter[ut[i]]);
    }
  
  StaticFunctions::showMessage("Equivalent Spherical Diamater", mesg);

  StaticFunctions::saveMesgToFile(Global::previousDirectory(), mesg);
}



void
VolumeMeasure::getDistanceToSurface(Vec bmin, Vec bmax, int tag)
{
  QProgressDialog progress("Distance Transform",
			   QString(),
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);  
  qApp->processEvents();

  
  uchar *lut = Global::lut();

  int ds = qMax(0, qFloor(bmin.z));
  int ws = qMax(0, qFloor(bmin.y));
  int hs = qMax(0, qFloor(bmin.x));

  int de = qMin(qCeil(bmax.z), m_depth-1);
  int we = qMin(qCeil(bmax.y), m_width-1);
  int he = qMin(qCeil(bmax.x), m_height-1);

  qint64 mx = he-hs+1;
  qint64 my = we-ws+1;
  qint64 mz = de-ds+1;


  //---------------
  QList<int> ut = getLabels(ds,ws,hs,de,we,he,tag);

  ut.removeAll(0); // remove zero label
  
  if (ut.count() == 0)
    {
      QMessageBox::information(0, "Error", "No voxel found with non-zero label");
      return;
    }

  qSort(ut);
  //---------------
  
  
  //---------------
  // get entire visible region includes all non-zero visible labels
  MyBitArray visibleMask;
  visibleMask.resize(mx*my*mz);
  visibleMask.fill(false);

  VolumeOperations::getVisibleRegion(ds, ws, hs,
				     de, we, he,
				     -1, false,
				     0, 0, 1,
				     visibleMask);
  //---------------

  
  
  //---------------
  // find distance tranform for entire visible region
  progress.setValue(50);
  qApp->processEvents();

  // generate squared distance transform
  float *dt = BinaryDistanceTransform::binaryEDTsq(visibleMask,
						   mx, my, mz,
						   true);
  //---------------

  
  progress.setValue(75);
  qApp->processEvents();

  
  //---------------
  // find distance to surface for all labels
  QMap<int, float> distToSurface;
  for(int u=0; u<ut.count(); u++)
    distToSurface[ut[u]] = 10000000;
      
  qint64 idx = 0;
  for(qint64 d=0; d<mz; d++)
    {
      progress.setValue(100*(float)d/(float)mz);
      qApp->processEvents();
      
      for(qint64 w=0; w<my; w++)
      for(qint64 h=0; h<mx; h++)
	{
	  qint64 bidx = ((qint64)(d+ds))*m_width*m_height+((qint64)(w+ws))*m_height+(h+hs);
	  
	  int lbl = m_maskDataUS[bidx];
	  if (ut.contains(lbl))
	    distToSurface[lbl] = qMin(distToSurface[lbl], (float)qFloor(sqrt(dt[idx])));
	  
	  idx++;
	}
    }
  //---------------

  
  delete [] dt;


  progress.setValue(100);


  VolumeInformation pvlInfo;
  pvlInfo = VolumeInformation::volumeInformation();
  Vec voxelSize = pvlInfo.voxelSize;

  QString mesg;
  mesg += QString("Voxel Size : %1, %2, %3 %4\n").\
                                arg(voxelSize.x).arg(voxelSize.y).arg(voxelSize.z).\
                                arg(pvlInfo.voxelUnitStringShort());

  mesg += "------------------------------\n";
  mesg += QString(" Label : Dist To Surface (%1)\n").		\
	                        arg(pvlInfo.voxelUnitStringShort());
  mesg += "------------------------------\n";
  for(int i=0; i<ut.count(); i++)
    {
      mesg += QString("%1 : %2\n").arg(ut[i], 6).\
	                           arg(distToSurface[ut[i]]*voxelSize.x);
    }

  
  StaticFunctions::showMessage("Distance To Surface", mesg);

  StaticFunctions::saveMesgToFile(Global::previousDirectory(), mesg);  
}



QMap<int, int>
VolumeMeasure::voxelCount(Vec bmin, Vec bmax, int tag)
{
  QMap<int, int> voxels; // contains (number of voxels) volume for each label

  QProgressDialog progress("Calculating Voxel Count",
			   QString(),
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);

  int ds = bmin.z;
  int ws = bmin.y;
  int hs = bmin.x;

  int de = bmax.z;
  int we = bmax.y;
  int he = bmax.x;

  uchar *lut = Global::lut();
  uchar *tagColors = Global::tagColors();

  qint64 nvoxels = 0;
  for(qint64 d=ds; d<=de; d++)
    {
      progress.setValue(90*(d-ds)/((de-ds+1)));
      if (d%10 == 0)
	qApp->processEvents();
      for(qint64 w=ws; w<=we; w++)
	for(qint64 h=hs; h<=he; h++)
	  {
	    bool clipped = VolumeOperations::checkClipped(Vec(h, w, d));
	    
	    if (!clipped)
	      {
		qint64 idx = d*m_width*m_height + w*m_height + h;
		int val = m_volData[idx];
		if (m_volDataUS) val = m_volDataUS[idx];
		
		int mtag = m_maskDataUS[idx];

		bool opaque = (lut[4*val+3]*tagColors[4*mtag+3] > 0);      

		if (tag > -1)
		  opaque &= (mtag == tag);

		if (opaque)
		  {
		    nvoxels ++;
		    voxels[mtag] = voxels[mtag] + 1;
		  }
	      }
	  }
    }

  // total visible voxels
  voxels[-1] = nvoxels;
  
  progress.setValue(100);

  return voxels;
}

void
VolumeMeasure::getVoxelCount(Vec bmin, Vec bmax, int tag)
{
  int ds = bmin.z;
  int ws = bmin.y;
  int hs = bmin.x;

  int de = bmax.z;
  int we = bmax.y;
  int he = bmax.x;

  QList<int> ut = getLabels(ds,ws,hs,de,we,he,tag);

  QMap<int, int> voxels = voxelCount(bmin, bmax, tag);

  VolumeInformation pvlInfo;
  pvlInfo = VolumeInformation::volumeInformation();
  Vec voxelSize = pvlInfo.voxelSize;

  QString mesg;
  mesg += QString("Visible Voxels : %1\n").arg(voxels[-1]);
  mesg += QString("Voxel Size : %1, %2, %3 %4\n").\
                  arg(voxelSize.x).arg(voxelSize.y).arg(voxelSize.z).
                  arg(pvlInfo.voxelUnitStringShort());

  if (ut.count() > 0)
    {	       
      mesg += "------------------------------\n";
      mesg += " Label : Voxel Count \n";
      mesg += "------------------------------\n";
      for(int i=0; i<ut.count(); i++)
	{
	  float vol = voxels[ut[i]]*voxelSize.x*voxelSize.y*voxelSize.z;
	  mesg += QString("  %1 : %2\n").arg(ut[i], 6).arg(voxels[ut[i]], 12);
	}
    }
  
  StaticFunctions::showMessage("Voxel Count", mesg);
  
  StaticFunctions::saveMesgToFile(Global::previousDirectory(), mesg);
}
//------
//------


void
VolumeMeasure::allMeasures(Vec bmin, Vec bmax, int tag,
			   QMap<QString, bool> flags)
{
  if (flags.count() < 6)
    {
      QMessageBox::information(0, "Error", "No Measures to measure");
      return;
    }

  bool voxelsFlag = flags["Voxel Count"];
  bool volFlag = flags["Volume"];
  bool areaFlag = flags["Surface Area"];
  bool feretFlag = flags["Max Feret Distance"];
  bool sphericityFlag = flags["Sphericity"];
  bool diameterFlag = flags["Equivalent Sphere Diameter"];

  
  bool vdbFlag = volFlag || areaFlag || sphericityFlag || diameterFlag;
  
  
  QMap<int, float> vdbVolume;
  QMap<int, float> vdbArea;
  QMap<int, float> feret;
  QMap<int, float> sphericity;
  QMap<int, float> sphereDiameter;
  
  QMap<int, int> voxels = voxelCount(bmin, bmax, tag);

  VolumeInformation pvlInfo;
  pvlInfo = VolumeInformation::volumeInformation();
  Vec voxelSize = pvlInfo.voxelSize;

  QProgressDialog progress("Calculating Measures",
			   QString(),
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);

  int ds = qMax(0, qFloor(bmin.z));
  int ws = qMax(0, qFloor(bmin.y));
  int hs = qMax(0, qFloor(bmin.x));

  int de = qMin(qCeil(bmax.z), m_depth-1);
  int we = qMin(qCeil(bmax.y), m_width-1);
  int he = qMin(qCeil(bmax.x), m_height-1);

  qint64 mx = he-hs+1;
  qint64 my = we-ws+1;
  qint64 mz = de-ds+1;


  QList<int> ut = getLabels(ds,ws,hs,de,we,he,tag);

//  if (ut.count() == 0)
//    {
//      QMessageBox::information(0, "Error", QString("No voxel found with label %1\nComputing total visible volume").arg(tag));
//    }

  // compute total visible volume as well for finding percentages
  ut << -1;
  
  qSort(ut);

    
  MyBitArray bitmask;
  bitmask.resize(mx*my*mz);

  for(int i=0; i<ut.count(); i++)
    {
      progress.setLabelText(QString("Calculating Measures %1 : %2").arg(i).arg(ut[i]));
      progress.setValue(100*(float)i/(float)ut.count());
      qApp->processEvents();
      
      bitmask.fill(false);
      
      VolumeOperations::getVisibleRegion(ds, ws, hs,
					 de, we, he,
					 ut[i], false,  // no tag zero checking
					 0, 0, 1,
					 bitmask,
					 false);

      if (feretFlag && ut[i] != -1)
	feret[ut[i]] = feretDiameter(mx, my, mz, bitmask);

      VdbVolume vdb;
      if (vdbFlag)
	{
	  vdb.setVoxelSize(voxelSize.x, voxelSize.y, voxelSize.z);
	  openvdb::FloatGrid::Accessor accessor = vdb.getAccessor();
	  openvdb::Coord ijk;
	  int &d = ijk[0];
	  int &w = ijk[1];
	  int &h = ijk[2];
	  for(d=0; d<mz; d++)
	    for(w=0; w<my; w++)
	      for(h=0; h<mx; h++)
		{
		  qint64 bidx = ((qint64)d)*mx*my+w*mx+h;
		  if (bitmask.testBit(bidx))
		    accessor.setValue(ijk, 255);
		}
	  
	  vdb.convertToLevelSet(128);
	}
      if (volFlag || sphericityFlag || diameterFlag)
	vdbVolume[ut[i]] = vdb.volume();

      if (areaFlag || sphericityFlag)
	vdbArea[ut[i]] = vdb.area();
    }

  if (sphericityFlag)
    {
      for(int i=0; i<ut.count(); i++)
	{
	  float V = vdbVolume[ut[i]];
	  float D = 2*qPow(3*V/4*M_PI, 1.0/3.0);
	  sphereDiameter[ut[i]] = D;
	}
    }

  if (diameterFlag)
    {
      for(int i=0; i<ut.count(); i++)
	{
	  float V = vdbVolume[ut[i]];
	  float A = vdbArea[ut[i]];
	  
	  float S = qPow(6*qSqrt(M_PI)*V, 2.0/3.0)/A;
	  
	  sphericity[ut[i]] = S;
	}
    }

  progress.setValue(100);


  
  QString mesg;
  mesg += QString("Voxel Size : %1, %2, %3 %4\n").\
                                arg(voxelSize.x).arg(voxelSize.y).arg(voxelSize.z).\
                                arg(pvlInfo.voxelUnitStringShort());
  if (voxelsFlag)
    mesg += QString("Visible Voxels : %1\n").arg(voxels[-1]);

  if (volFlag)
    {
      mesg += QString("Total Visible Volume : %1%2^3\n").\
	                              arg(vdbVolume[-1]).\
	                              arg(pvlInfo.voxelUnitStringShort());
      ut.removeAt(0);
    }
  
  if (areaFlag && tag == -1)
    {
      mesg += QString("Total Visible Surface Area : %1%2^2\n").	\
	                              arg(vdbArea[-1]).\
	                              arg(pvlInfo.voxelUnitStringShort());
    }

  mesg += "------------------------------\n";
  mesg += " Label : ";

  if (voxelsFlag)
    mesg += "Voxels : ";

  if (volFlag)
    mesg += QString("Volume %1^3: % of total Volume : ").\
	                 arg(pvlInfo.voxelUnitStringShort());
  if (areaFlag)
      mesg += QString("SurfaceArea %1^2 : ").arg(pvlInfo.voxelUnitStringShort());
    
  if (sphericityFlag)
    mesg += "Sphericity : ";
    
  if (diameterFlag)
    mesg += "EquiSphereDiameter : ";
    
  if (feretFlag)
    mesg += "MaxFeret : ";

  mesg += "\n------------------------------\n";
  
  if (ut.count() > 0)
    {
      for(int i=0; i<ut.count(); i++)
	{
	  mesg += QString("%1 : ").arg(ut[i], 6); // label

	  if (voxelsFlag)
	    mesg += QString("%1 : ").arg(voxels[ut[i]]);

	    
	  float p = 100.0*vdbVolume[ut[i]]/vdbVolume[-1];
	  if (volFlag)
	    mesg += QString("%1 : %2 : ").arg(vdbVolume[ut[i]]).\
	                                  arg(p, 0, 'f', 2);
	  if (areaFlag)
	    mesg += QString("%1 : ").arg(vdbArea[ut[i]]);

	  if (sphericityFlag)
	    mesg += QString("%1 : ").arg(sphericity[ut[i]]);

	  if (diameterFlag)
	    mesg += QString("%1 : ").arg(sphereDiameter[ut[i]]);

	  if (feretFlag)
	    mesg += QString("%1 : ").arg(feret[ut[i]]);

	  mesg += "\n";
	}
    }
  
  StaticFunctions::showMessage("Volume Measures", mesg);

  StaticFunctions::saveMesgToFile(Global::previousDirectory(), mesg);
}
