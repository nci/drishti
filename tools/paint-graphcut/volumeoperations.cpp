#include <GL/glew.h>
#include "volumeoperations.h"
#include "volumeinformation.h"
#include "global.h"
#include "mybitarray.h"

uchar* VolumeOperations::m_volData = 0;
uchar* VolumeOperations::m_maskData = 0;

void VolumeOperations::setVolData(uchar *v) { m_volData = v; }
void VolumeOperations::setMaskData(uchar *v) { m_maskData = v; }


int VolumeOperations::m_depth = 0;
int VolumeOperations::m_width = 0;
int VolumeOperations::m_height = 0;
void VolumeOperations::setGridSize(int d, int w, int h)
{
  m_depth = d;
  m_width = w;
  m_height = h;
}

void VolumeOperations::getVolume(Vec bmin, Vec bmax, int tag,
				 QList<Vec> cPos, QList<Vec> cNorm)
{
  QProgressDialog progress("Calculating Volume",
			   QString(),
			   0, 100,
			   0);
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
	    bool clipped = false;
	    for(int i=0; i<cPos.count(); i++)
	      {
		Vec p = Vec(h, w, d) - cPos[i];
		if (cNorm[i]*p > 0)
		  {
		    clipped = true;
		    break;
		  }
	      }
	    
	    if (!clipped)
	      {
		qint64 idx = d*m_width*m_height + w*m_height + h;
		int val = m_volData[idx];
		uchar mtag = m_maskData[idx];
		bool opaque = (lut[4*val+3]*tagColors[4*mtag+3] > 0);      
		if (tag > -1)
		  opaque &= (mtag == tag);

		if (opaque)
		  nvoxels ++;
	      }
	  }
    }

  progress.setValue(100);

  VolumeInformation pvlInfo;
  pvlInfo = VolumeInformation::volumeInformation();
  Vec voxelSize = pvlInfo.voxelSize;
  float voxvol = nvoxels*voxelSize.x*voxelSize.y*voxelSize.z;

  QString mesg;
  mesg += QString("Visible Voxels : %1\n").arg(nvoxels);
  mesg += QString("Voxel Size : %1, %2, %3 %4\n").\
                  arg(voxelSize.x).arg(voxelSize.y).arg(voxelSize.z).
                  arg(pvlInfo.voxelUnitStringShort());
  mesg += QString("Volume : %1 %2^3\n").	\
                  arg(voxvol).\
                  arg(pvlInfo.voxelUnitStringShort());

  QMessageBox::information(0, "", mesg);
}

void
VolumeOperations::connectedRegion(int dr, int wr, int hr,
				  Vec bmin, Vec bmax,
				  int tag, int ctag,
				  QList<Vec> cPos, QList<Vec> cNorm,
				  int& minD, int& maxD,
				  int& minW, int& maxW,
				  int& minH, int& maxH)
{
  minD = maxD = minW = maxW = minH = maxH = -1;

  if (dr < 0 || wr < 0 || hr < 0 ||
      dr > m_depth-1 ||
      wr > m_width-1 ||
      hr > m_height-1)
    {
      QMessageBox::information(0, "", "No painted region found");
      return;
    }

  int ds = bmin.z;
  int ws = bmin.y;
  int hs = bmin.x;

  int de = bmax.z;
  int we = bmax.y;
  int he = bmax.x;

  qint64 mx = he-hs+1;
  qint64 my = we-ws+1;
  qint64 mz = de-ds+1;

  MyBitArray bitmask;
  bitmask.resize(mx*my*mz);
  bitmask.fill(false);

  getConnectedRegion(dr, wr, hr,
		     ds, ws, hs,
		     de, we, he,
		     ctag, true,
		     cPos, cNorm,
		     bitmask);

  QProgressDialog progress("Updating voxel structure",
			   QString(),
			   0, 100,
			   0);
  progress.setMinimumDuration(0);

  //----------------------------  
  // set the maskData
  minD = maxD = dr;
  minW = maxW = wr;
  minH = maxH = hr;
  for(qint64 d2=ds; d2<=de; d2++)
  for(qint64 w2=ws; w2<=we; w2++)
  for(qint64 h2=hs; h2<=he; h2++)
    {
      qint64 bidx = (d2-ds)*mx*my+(w2-ws)*mx+(h2-hs);
      if (bitmask.testBit(bidx))
	{
	  qint64 idx = d2*m_width*m_height + w2*m_height + h2;
	  m_maskData[idx] = tag;
	  minD = qMin(minD, (int)d2);
	  maxD = qMax(maxD, (int)d2);
	  minW = qMin(minW, (int)w2);
	  maxW = qMax(maxW, (int)w2);
	  minH = qMin(minH, (int)h2);
	  maxH = qMax(maxH, (int)h2);
	}
    }
  //----------------------------  
  
  minD = qMax(minD-1, 0);
  minW = qMax(minW-1, 0);
  minH = qMax(minH-1, 0);
  maxD = qMin(maxD+1, m_depth);
  maxW = qMin(maxW+1, m_width);
  maxH = qMin(maxH+1, m_height);

  progress.setValue(100);
}

void
VolumeOperations::getConnectedRegion(int dr, int wr, int hr,
				     int ds, int ws, int hs,
				     int de, int we, int he,
				     int tag, bool zero,
				     QList<Vec> cPos, QList<Vec> cNorm,
				     MyBitArray& cbitmask)
{
  QProgressDialog progress("Identifying connected region",
			   QString(),
			   0, 100,
			   0);
  progress.setMinimumDuration(0);

  qint64 mx = he-hs+1;
  qint64 my = we-ws+1;
  qint64 mz = de-ds+1;

  uchar *lut = Global::lut();

  int indices[] = {-1, 0, 0,
		    1, 0, 0,
		    0,-1, 0,
		    0, 1, 0,
		    0, 0,-1,
		    0, 0, 1};

  //----------------------------  
  // identify visible region
  cbitmask.fill(false);

  progress.setLabelText("Identifying visible region");
  for(qint64 d2=ds; d2<=de; d2++)
  {
    progress.setLabelText(QString("Identifying visible region %1 of %2").arg(d2-ds).arg(mz));
    progress.setValue(90*(float)(d2-ds)/(float)mz);
    qApp->processEvents();
    for(qint64 w2=ws; w2<=we; w2++)
    for(qint64 h2=hs; h2<=he; h2++)
    {
      bool clipped = false;
      for(int i=0; i<cPos.count(); i++)
	{
	  Vec p = Vec(h2, w2, d2) - cPos[i];
	  if (cNorm[i]*p > 0)
	    {
	      clipped = true;
	      break;
	    }
	}
      
      qint64 idx = d2*m_width*m_height + w2*m_height + h2;
      int val = m_volData[idx];
      uchar mtag = m_maskData[idx];
      bool opaque =  (lut[4*val+3]*Global::tagColors()[4*mtag+3] > 0);      
      if (tag > -1)
	{
	  if (zero)
	    opaque &= (mtag == 0 || mtag == tag);
	  else
	    opaque &= (mtag == tag);
	}
      // grow only in zero or same tagged region
      // or if tag is 0 then grow in all visible regions
      if (!clipped && opaque)
	{
	  qint64 bidx = (d2-ds)*mx*my+(w2-ws)*mx+(h2-hs);
	  cbitmask.setBit(bidx, true);
	}  // visible voxel
    }
  }
  //----------------------------  

  MyBitArray bitmask;
  bitmask.resize(mx*my*mz);
  bitmask.fill(false);

  QList<Vec> edges;
  edges.clear();
  edges << Vec(dr,wr,hr);
  qint64 bidx = (dr-ds)*mx*my+(wr-ws)*mx+(hr-hs);
  bitmask.setBit(bidx, true);

  //------------------------------------------------------
  // dilate from seed
  bool done = false;
  int nd = 0;
  int pvnd = 0;
  while(!done)
    {
      nd = (nd + 1)%100;
      int pnd = 90*(float)nd/(float)100;
      progress.setValue(pnd);
      if (pnd != pvnd)
	qApp->processEvents();
      pvnd = pnd;

      QList<Vec> tedges;
      tedges.clear();

      progress.setLabelText(QString("Flooding %1").arg(edges.count()));
      qApp->processEvents();

            
      // find outer boundary to fill
      for(int e=0; e<edges.count(); e++)
	{
	  int dx = edges[e].x;
	  int wx = edges[e].y;
	  int hx = edges[e].z;
	  	  
	  for(int i=0; i<6; i++)
	    {
	      int da = indices[3*i+0];
	      int wa = indices[3*i+1];
	      int ha = indices[3*i+2];
	      
	      qint64 d2 = qBound(ds, dx+da, de);
	      qint64 w2 = qBound(ws, wx+wa, we);
	      qint64 h2 = qBound(hs, hx+ha, he);
	      
	      qint64 bidx = (d2-ds)*mx*my+(w2-ws)*mx+(h2-hs);
	      if (cbitmask.testBit(bidx) &&
		  !bitmask.testBit(bidx))
		{
		  bitmask.setBit(bidx, true);
		  tedges << Vec(d2,w2,h2);		  
		}
	    }
	}

      edges.clear();

      if (tedges.count() > 0)
	edges = tedges;
      else
	done = true;

      tedges.clear();
    }
  //------------------------------------------------------

  // copy bitmask into cbitmask
  for(qint64 i=0; i<mx*my*mz; i++)
    cbitmask.setBit(i, bitmask.testBit(i));
}

