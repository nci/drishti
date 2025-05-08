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

#include <QtConcurrentMap>
#include <QMap>



uchar* VolumeMeasure::m_volData = 0;
ushort* VolumeMeasure::m_volDataUS = 0;
uchar* VolumeMeasure::m_maskData = 0;


void VolumeMeasure::setVolData(uchar *v)
{
  m_volData = v;
  m_volDataUS = 0;
  if (Global::bytesPerVoxel() == 2)
    m_volDataUS = (ushort*) m_volData;
}
void VolumeMeasure::setMaskData(uchar *v) { m_maskData = v; }


int VolumeMeasure::m_depth = 0;
int VolumeMeasure::m_width = 0;
int VolumeMeasure::m_height = 0;
void VolumeMeasure::setGridSize(int d, int w, int h)
{
  m_depth = d;
  m_width = w;
  m_height = h;
}

//void VolumeMeasure::getVolume(Vec bmin, Vec bmax, int tag)
//{
//  QProgressDialog progress("Calculating Volume",
//			   QString(),
//			   0, 100,
//			   0,
//			   Qt::WindowStaysOnTopHint);
//  progress.setMinimumDuration(0);
//
//  int ds = bmin.z;
//  int ws = bmin.y;
//  int hs = bmin.x;
//
//  int de = bmax.z;
//  int we = bmax.y;
//  int he = bmax.x;
//
//  uchar *lut = Global::lut();
//  uchar *tagColors = Global::tagColors();
//
//  QMap<int, int> labelMap; // contains (number of voxels) volume for each label
//  qint64 nvoxels = 0;
//  for(qint64 d=ds; d<=de; d++)
//    {
//      progress.setValue(90*(d-ds)/((de-ds+1)));
//      if (d%10 == 0)
//	qApp->processEvents();
//      for(qint64 w=ws; w<=we; w++)
//	for(qint64 h=hs; h<=he; h++)
//	  {
//	    bool clipped = VolumeOperations::checkClipped(Vec(h, w, d));
//	    
//	    if (!clipped)
//	      {
//		qint64 idx = d*m_width*m_height + w*m_height + h;
//		int val = m_volData[idx];
//		if (m_volDataUS) val = m_volDataUS[idx];
//		uchar mtag = m_maskData[idx];
//		bool opaque = (lut[4*val+3]*tagColors[4*mtag+3] > 0);      
//		if (tag > -1)
//		  opaque &= (mtag == tag);
//
//		if (opaque)
//		  {
//		    nvoxels ++;
//		    labelMap[mtag] = labelMap[mtag] + 1;
//		  }
//	      }
//	  }
//    }
//
//  progress.setValue(100);
//
//  VolumeInformation pvlInfo;
//  pvlInfo = VolumeInformation::volumeInformation();
//  Vec voxelSize = pvlInfo.voxelSize;
//  float voxvol = nvoxels*voxelSize.x*voxelSize.y*voxelSize.z;
//
//  QString mesg;
//  mesg += QString("Visible Voxels : %1\n").arg(nvoxels);
//  mesg += QString("Voxel Size : %1, %2, %3 %4\n").\
//                  arg(voxelSize.x).arg(voxelSize.y).arg(voxelSize.z).
//                  arg(pvlInfo.voxelUnitStringShort());
//  mesg += QString("Volume : %1 %2^3\n").	\
//                  arg(voxvol).\
//                  arg(pvlInfo.voxelUnitStringShort());
//
//  if (tag == -1)
//    {	       
//      mesg += "------------------------------\n";
//      mesg += " Label : Voxel Count : Volume \n";
//      mesg += "------------------------------\n";
//      QList<int> key = labelMap.keys();
//      QList<int> value = labelMap.values();
//      for(int i=0; i<key.count(); i++)
//	{
//	  float vol = value[i]*voxelSize.x*voxelSize.y*voxelSize.z;
//	  mesg += QString("  %1 : %2 : %3 %4^3\n").arg(key[i], 6).arg(value[i], 12).\
//	                                         arg(vol).arg(pvlInfo.voxelUnitStringShort());
//	}
//    }
//  
//  StaticFunctions::showMessage("Volume", mesg);
//}


void VolumeMeasure::getVolume(Vec bmin, Vec bmax, int tag)
{
  VolumeInformation pvlInfo;
  pvlInfo = VolumeInformation::volumeInformation();
  Vec voxelSize = pvlInfo.voxelSize;

  
  
  QProgressDialog progress("Calculating Volume",
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

  qint64 mx = he-hs+1;
  qint64 my = we-ws+1;
  qint64 mz = de-ds+1;


  uchar *lut = Global::lut();
  uchar *tagColors = Global::tagColors();

  QList<int> ut;
  for(qint64 d=ds; d<=de; d++)
    for(qint64 w=ws; w<=we; w++)
      for(qint64 h=hs; h<=he; h++)
	{
	  qint64 idx = ((qint64)d)*m_width*m_height + ((qint64)w)*m_height + h;
	  int val = m_volData[idx];
	  if (m_volDataUS) val = m_volDataUS[idx];
	  uchar mtag = m_maskData[idx];
	  bool opaque = (lut[4*val+3]*tagColors[4*mtag+3] > 0);      

	  if (opaque && !ut.contains(mtag))
	    {
	      if (tag == -1 || tag == mtag)
		ut << mtag;
	    }
	}

  // compute total visible volume as well for finding percentages
  ut << -1;

  if (ut.count() == 0)
    {
      QMessageBox::information(0, "Error", QString("No voxel found with label %1").arg(tag));
      return;
    }
  
  qSort(ut);

    
  MyBitArray bitmask;
  bitmask.resize(mx*my*mz);
  QMap<int, float> vdbVolume;
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
      mesg += " Label : Volume : % of total\n";
      mesg += "------------------------------\n";
      for(int i=0; i<ut.count(); i++)
	{
	  float p = 100.0*vdbVolume[ut[i]]/vdbVolume[-1];
	  mesg += QString("  %1 : %2%3^3 : %4\n").arg(ut[i], 6).\
	                                     arg(vdbVolume[ut[i]]).\
	                                     arg(pvlInfo.voxelUnitStringShort()).\
	                                     arg(p, 0, 'f', 2);
	}
    }
  
  StaticFunctions::showMessage("Volume", mesg);
}


void VolumeMeasure::getSurfaceArea(Vec bmin, Vec bmax, int tag)
{
  VolumeInformation pvlInfo;
  pvlInfo = VolumeInformation::volumeInformation();
  Vec voxelSize = pvlInfo.voxelSize;

  
  
  QProgressDialog progress("Calculating Volume",
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

  qint64 mx = he-hs+1;
  qint64 my = we-ws+1;
  qint64 mz = de-ds+1;

  uchar *lut = Global::lut();
  uchar *tagColors = Global::tagColors();

  QList<int> ut;
  for(qint64 d=ds; d<=de; d++)
    for(qint64 w=ws; w<=we; w++)
      for(qint64 h=hs; h<=he; h++)
	{
	  qint64 idx = ((qint64)d)*m_width*m_height + ((qint64)w)*m_height + h;
	  int val = m_volData[idx];
	  if (m_volDataUS) val = m_volDataUS[idx];
	  uchar mtag = m_maskData[idx];
	  bool opaque = (lut[4*val+3]*tagColors[4*mtag+3] > 0);      

	  if (opaque && !ut.contains(mtag))
	    {
	      if (tag == -1 || tag == mtag)
		ut << mtag;
	    }
	}

  if (tag == -1)
    ut << -1;

  if (ut.count() == 0)
    {
      QMessageBox::information(0, "Error", QString("No voxel found with label %1").arg(tag));
      return;
    }
  
  qSort(ut);
  
  MyBitArray bitmask;
  bitmask.resize(mx*my*mz);
  QMap<int, float> vdbVolume;
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
      vdbVolume[ut[i]] = vdb.area();
    }
  progress.setValue(100);


  QString mesg;
  mesg += QString("Voxel Size : %1, %2, %3 %4\n").\
                                arg(voxelSize.x).arg(voxelSize.y).arg(voxelSize.z).\
                                arg(pvlInfo.voxelUnitStringShort());
  if (tag == -1)
    {
      mesg += "------------------------------\n";
      mesg += QString("Total Visible Surface Area : %1%2^2\n").\
	                              arg(vdbVolume[-1]).\
	                              arg(pvlInfo.voxelUnitStringShort());
      mesg += "------------------------------\n";
      ut.removeAt(0);
    }
  if (ut.count() > 0)
    {
      mesg += "------------------------------\n";
      mesg += " Label : Surface Area\n";
      mesg += "------------------------------\n";
      for(int i=0; i<ut.count(); i++)
	{
	  float p = 100.0*vdbVolume[ut[i]]/vdbVolume[-1];
	  mesg += QString("  %1 : %2%3^2\n").arg(ut[i], 6).\
	                                     arg(vdbVolume[ut[i]]).\
	                                     arg(pvlInfo.voxelUnitStringShort());
	}
    }
  
  StaticFunctions::showMessage("Surface Area", mesg);
}



