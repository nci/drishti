#include <GL/glew.h>
#include "volumeoperations.h"
#include "volumeinformation.h"
#include "global.h"
#include "mybitarray.h"
#include "morphslice.h"

uchar* VolumeOperations::m_volData = 0;
ushort* VolumeOperations::m_volDataUS = 0;
uchar* VolumeOperations::m_maskData = 0;

void VolumeOperations::setVolData(uchar *v)
{
  m_volData = v;
  m_volDataUS = 0;
  if (Global::bytesPerVoxel() == 2)
    m_volDataUS = (ushort*) m_volData;
}
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

QList<Vec> VolumeOperations::m_cPos;
QList<Vec> VolumeOperations::m_cNorm;
void VolumeOperations::setClip(QList<Vec> cpos, QList<Vec> cnorm)
{
  m_cPos = cpos;
  m_cNorm = cnorm;
}

void VolumeOperations::getVolume(Vec bmin, Vec bmax, int tag)
{
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
	    for(int i=0; i<m_cPos.count(); i++)
	      {
		Vec p = Vec(h, w, d) - m_cPos[i];
		if (m_cNorm[i]*p > 0)
		  {
		    clipped = true;
		    break;
		  }
	      }
	    
	    if (!clipped)
	      {
		qint64 idx = d*m_width*m_height + w*m_height + h;
		int val = m_volData[idx];
		if (m_volDataUS) val = m_volDataUS[idx];
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
VolumeOperations::resetTag(Vec bmin, Vec bmax, int tag,
			   int& minD, int& maxD,
			   int& minW, int& maxW,
			   int& minH, int& maxH)
{
  int ds = bmin.z;
  int ws = bmin.y;
  int hs = bmin.x;

  int de = bmax.z;
  int we = bmax.y;
  int he = bmax.x;

  QProgressDialog progress("Updating voxel structure",
			   QString(),
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);

  minD = maxD = -1;
  minW = maxW = -1;
  minH = maxH = -1;

  for(qint64 d=ds; d<=de; d++)
    {
      progress.setValue(90*(d-ds)/((de-ds+1)));
      if (d%10 == 0)
	qApp->processEvents();
      for(qint64 w=ws; w<=we; w++)
	for(qint64 h=hs; h<=he; h++)
	  {
	    bool clipped = false;
	    for(int i=0; i<m_cPos.count(); i++)
	      {
		Vec p = Vec(h, w, d) - m_cPos[i];
		if (m_cNorm[i]*p > 0)
		  {
		    clipped = true;
		    break;
		  }
	      }
	    
	    if (!clipped)
	      {
		qint64 idx = d*m_width*m_height + w*m_height + h;
		m_maskData[idx] = tag;
		if (minD > -1)
		  {
		    minD = qMin(minD, (int)d);
		    maxD = qMax(maxD, (int)d);
		    minW = qMin(minW, (int)w);
		    maxW = qMax(maxW, (int)w);
		    minH = qMin(minH, (int)h);
		    maxH = qMax(maxH, (int)h);
		  }
		else
		  {
		    minD = maxD = d;
		    minW = maxW = w;
		    minH = maxH = h;
		  }
	      } // clipped
	  }
    }
  
  minD = qMax(minD-1, 0);
  minW = qMax(minW-1, 0);
  minH = qMax(minH-1, 0);
  maxD = qMin(maxD+1, m_depth);
  maxW = qMin(maxW+1, m_width);
  maxH = qMin(maxH+1, m_height);
}

void
VolumeOperations::hatchConnectedRegion(int dr, int wr, int hr,
				       Vec bmin, Vec bmax,
				       int tag, int ctag,
				       int thickness, int interval,
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
		     bitmask,
		     0, 0.0, 1.0);

  QProgressDialog progress("Updating voxel structure",
			   QString(),
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
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
      bool okd = ((d2-ds)%interval <= thickness);
      bool okw = ((w2-ws)%interval <= thickness);
      bool okh = ((h2-hs)%interval <= thickness);
      bool ok = (okd && (okw || okh));
      ok |= (okw && (okd || okh));
      ok |= (okh && (okd || okw));
      if (ok)
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
VolumeOperations::connectedRegion(int dr, int wr, int hr,
				  Vec bmin, Vec bmax,
				  int tag, int ctag,
				  int& minD, int& maxD,
				  int& minW, int& maxW,
				  int& minH, int& maxH,
				  int gradType, float minGrad, float maxGrad)
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
		     bitmask,
		     gradType, minGrad, maxGrad);

  QProgressDialog progress("Updating voxel structure",
			   QString(),
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
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
				     MyBitArray& cbitmask,
				     int gradType, float minGrad, float maxGrad)
{  
  QProgressDialog progress("Identifying connected region",
			   QString(),
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
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
      for(int i=0; i<m_cPos.count(); i++)
	{
	  Vec p = Vec(h2, w2, d2) - m_cPos[i];
	  if (m_cNorm[i]*p > 0)
	    {
	      clipped = true;
	      break;
	    }
	}
      
      qint64 idx = d2*m_width*m_height + w2*m_height + h2;
      int val = m_volData[idx];
      if (m_volDataUS) val = m_volDataUS[idx];
      uchar mtag = m_maskData[idx];
      bool opaque =  (lut[4*val+3]*Global::tagColors()[4*mtag+3] > 0);      

      //-------
      if (opaque)
      {
	float gradMag;
	if (gradType == 0)
	  {
	    float gx,gy,gz;
	    qint64 d3 = qBound(0, (int)d2+1, m_depth-1);
	    qint64 d4 = qBound(0, (int)d2-1, m_depth-1);
	    qint64 w3 = qBound(0, (int)w2+1, m_width-1);
	    qint64 w4 = qBound(0, (int)w2-1, m_width-1);
	    qint64 h3 = qBound(0, (int)h2+1, m_height-1);
	    qint64 h4 = qBound(0, (int)h2-1, m_height-1);
	    if (!m_volDataUS)
	      {
		gz = (m_volData[d3*m_width*m_height + w2*m_height + h2] -
		      m_volData[d4*m_width*m_height + w2*m_height + h2]);
		gy = (m_volData[d2*m_width*m_height + w3*m_height + h2] -
		      m_volData[d2*m_width*m_height + w4*m_height + h2]);
		gx = (m_volData[d2*m_width*m_height + w2*m_height + h3] -
		      m_volData[d2*m_width*m_height + w2*m_height + h4]);
		gx/=255.0;
		gy/=255.0;
		gz/=255.0;
	      }
	    else
	      {
		gz = (m_volDataUS[d3*m_width*m_height + w2*m_height + h2] -
		      m_volDataUS[d4*m_width*m_height + w2*m_height + h2]);
		gy = (m_volDataUS[d2*m_width*m_height + w3*m_height + h2] -
		      m_volDataUS[d2*m_width*m_height + w4*m_height + h2]);
		gx = (m_volDataUS[d2*m_width*m_height + w2*m_height + h3] -
		      m_volDataUS[d2*m_width*m_height + w2*m_height + h4]);
		gx/=65535.0;
		gy/=65535.0;
		gz/=65535.0;
	      }

	    Vec dv = Vec(gx, gy, gz); // surface gradient
	    gradMag = dv.norm();
	  } // gradType == 0
	
	if (gradType > 0)
	  {
	    int sz = 1;
	    float divisor = 10.0;
	    if (gradType == 2)
	      {
		sz = 2;
		divisor = 70.0;
	      }
	    if (!m_volDataUS)
	      {	      
		float sum = 0;
		float vval = m_volData[d2*m_width*m_height + w2*m_height + h2];
		for(int a=d2-sz; a<=d2+sz; a++)
		for(int b=w2-sz; b<=w2+sz; b++)
		for(int c=h2-sz; c<=h2+sz; c++)
		  {
		    qint64 a0 = qBound(0, a, m_depth-1);
		    qint64 b0 = qBound(0, b, m_width-1);
		    qint64 c0 = qBound(0, c, m_height-1);
		    sum += m_volData[a0*m_width*m_height + b0*m_height + c0];
		  }
		
		sum = (sum-vval)/divisor;
		gradMag = fabs(sum-vval)/255.0;
	      }
	    else
	      {
		float sum = 0;
		float vval = m_volDataUS[d2*m_width*m_height + w2*m_height + h2];
		for(int a=d2-sz; a<=d2+sz; a++)
		for(int b=w2-sz; b<=w2+sz; b++)
		for(int c=h2-sz; c<=h2+sz; c++)
		  {
		    qint64 a0 = qBound(0, a, m_depth-1);
		    qint64 b0 = qBound(0, b, m_width-1);
		    qint64 c0 = qBound(0, c, m_height-1);
		    sum += m_volDataUS[a0*m_width*m_height + b0*m_height + c0];
		  }
		
		sum = (sum-vval)/divisor;
		gradMag = fabs(sum-vval)/65535.0;
	      }
	  } // gradType > 0
	
	gradMag = qBound(0.0f, gradMag, 1.0f);	

	if (gradMag < minGrad || gradMag > maxGrad)
	  opaque = false;
      }
      //-------
      
      
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
  cbitmask = bitmask;
}

void
VolumeOperations::shrinkwrapSlice(uchar *swvr, int mx, int my)
{
  memset(swvr, 0, my*mx);

  MorphSlice ms;
  QList<QPolygonF> poly = ms.boundaryCurves(swvr, mx, my, true);

  QImage pimg = QImage(mx, my, QImage::Format_RGB32);
  pimg.fill(0);
  QPainter p(&pimg);
  p.setPen(QPen(Qt::white, 1));
  p.setBrush(Qt::white);

  for (int npc=0; npc<poly.count(); npc++)
    p.drawPolygon(poly[npc]);

  QRgb *rgb = (QRgb*)(pimg.bits());
  for(int i=0; i<my*mx; i++)
    swvr[i] = (swvr[i]>0 || qRed(rgb[i])>0 ? 255 : 0);  
}

void
VolumeOperations::getTransparentRegion(int ds, int ws, int hs,
				       int de, int we, int he,
				       MyBitArray& cbitmask,
				       int gradType, float minGrad, float maxGrad)
{
  QProgressDialog progress("Identifying transparent region",
			   QString(),
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);

  qint64 mx = he-hs+1;
  qint64 my = we-ws+1;
  qint64 mz = de-ds+1;

  uchar *lut = Global::lut();

  for(qint64 d2=ds; d2<=de; d2++)
  {
    progress.setValue(90*(float)(d2-ds)/(float)mz);
    qApp->processEvents();
    for(qint64 w2=ws; w2<=we; w2++)
    for(qint64 h2=hs; h2<=he; h2++)
    {
      bool clipped = false;
      for(int i=0; i<m_cPos.count(); i++)
	{
	  Vec p = Vec(h2, w2, d2) - m_cPos[i];
	  if (m_cNorm[i]*p > 0)
	    {
	      clipped = true;
	      break;
	    }
	}
      
      qint64 idx = d2*m_width*m_height + w2*m_height + h2;
      int val = m_volData[idx];
      if (m_volDataUS) val = m_volDataUS[idx];
      uchar mtag = m_maskData[idx];
      bool transparent =  (lut[4*val+3]*Global::tagColors()[4*mtag+3] == 0);

      //-------
      if (!transparent)
      {
	float gradMag;
	if (gradType == 0)
	  {
	    float gx,gy,gz;
	    qint64 d3 = qBound(0, (int)d2+1, m_depth-1);
	    qint64 d4 = qBound(0, (int)d2-1, m_depth-1);
	    qint64 w3 = qBound(0, (int)w2+1, m_width-1);
	    qint64 w4 = qBound(0, (int)w2-1, m_width-1);
	    qint64 h3 = qBound(0, (int)h2+1, m_height-1);
	    qint64 h4 = qBound(0, (int)h2-1, m_height-1);
	    if (!m_volDataUS)
	      {
		gz = (m_volData[d3*m_width*m_height + w2*m_height + h2] -
		      m_volData[d4*m_width*m_height + w2*m_height + h2]);
		gy = (m_volData[d2*m_width*m_height + w3*m_height + h2] -
		      m_volData[d2*m_width*m_height + w4*m_height + h2]);
		gx = (m_volData[d2*m_width*m_height + w2*m_height + h3] -
		      m_volData[d2*m_width*m_height + w2*m_height + h4]);
		gx/=255.0;
		gy/=255.0;
		gz/=255.0;
	      }
	    else
	      {
		gz = (m_volDataUS[d3*m_width*m_height + w2*m_height + h2] -
		      m_volDataUS[d4*m_width*m_height + w2*m_height + h2]);
		gy = (m_volDataUS[d2*m_width*m_height + w3*m_height + h2] -
		      m_volDataUS[d2*m_width*m_height + w4*m_height + h2]);
		gx = (m_volDataUS[d2*m_width*m_height + w2*m_height + h3] -
		      m_volDataUS[d2*m_width*m_height + w2*m_height + h4]);
		gx/=65535.0;
		gy/=65535.0;
		gz/=65535.0;
	      }
	    
	    Vec dv = Vec(gx, gy, gz); // surface gradient
	    gradMag = dv.norm();
	  } // gradType == 0

	if (gradType > 0)
	  {
	    int sz = 1;
	    float divisor = 10.0;
	    if (gradType == 2)
	      {
		sz = 2;
		divisor = 70.0;
	      }
	    if (!m_volDataUS)
	      {	      
		float sum = 0;
		float vval = m_volData[d2*m_width*m_height + w2*m_height + h2];
		for(int a=d2-sz; a<=d2+sz; a++)
		for(int b=w2-sz; b<=w2+sz; b++)
		for(int c=h2-sz; c<=h2+sz; c++)
		  {
		    qint64 a0 = qBound(0, a, m_depth-1);
		    qint64 b0 = qBound(0, b, m_width-1);
		    qint64 c0 = qBound(0, c, m_height-1);
		    sum += m_volData[a0*m_width*m_height + b0*m_height + c0];
		  }
		
		sum = (sum-vval)/divisor;
		gradMag = fabs(sum-vval)/255.0;
	      }
	    else
	      {
		float sum = 0;
		float vval = m_volDataUS[d2*m_width*m_height + w2*m_height + h2];
		for(int a=d2-sz; a<=d2+sz; a++)
		for(int b=w2-sz; b<=w2+sz; b++)
		for(int c=h2-sz; c<=h2+sz; c++)
		  {
		    qint64 a0 = qBound(0, a, m_depth-1);
		    qint64 b0 = qBound(0, b, m_width-1);
		    qint64 c0 = qBound(0, c, m_height-1);
		    sum += m_volDataUS[a0*m_width*m_height + b0*m_height + c0];
		  }
		
		sum = (sum-vval)/divisor;
		gradMag = fabs(sum-vval)/65535.0;
	      }
	  } // gradType > 0
	
	  
	gradMag = qBound(0.0f, gradMag, 1.0f);	

	if (gradMag < minGrad || gradMag > maxGrad)
	  transparent = true;
      }
      //-------

      if (clipped || transparent)
	{
	  qint64 bidx = (d2-ds)*mx*my+(w2-ws)*mx+(h2-hs);
	  cbitmask.setBit(bidx, true);
	}  // transparent voxel
    }
  }
  progress.setValue(100);
}


void
VolumeOperations::shrinkwrap(Vec bmin, Vec bmax, int tag,
			     bool shellOnly, int shellThickness,
			     bool all,
			     int dr, int wr, int hr, int ctag,
			     int& minD, int& maxD,
			     int& minW, int& maxW,
			     int& minH, int& maxH,
			     int gradType, float minGrad, float maxGrad)
{
  //-------------------------
  int holeSize = 0;
  holeSize = QInputDialog::getInt(0,
				  "Fill Holes",
				  "Size of holes to fill",
				  0, 0, 100, 1);
  //-------------------------

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

  MyBitArray cbitmask;
  cbitmask.resize(mx*my*mz);
  cbitmask.fill(false);


  int indices[] = {-1, 0, 0,
		    1, 0, 0,
		    0,-1, 0,
		    0, 1, 0,
		    0, 0,-1,
		    0, 0, 1};
  
  //----------------------------  
  if (all) // identify transparent region  
    getTransparentRegion(ds, ws, hs,
			 de, we, he,
			 cbitmask,
			 gradType, minGrad, maxGrad);
  else // identify connected region
    {
      getConnectedRegion(dr, wr, hr,
			 ds, ws, hs,
			 de, we, he,
			 ctag, false,
			 cbitmask,
			 gradType, minGrad, maxGrad);
      // invert all values in cbitmask
      cbitmask.invert();
    }
  //----------------------------  


  //----------------------------
  // fill the holes before shrinkwrapping the region
  if (holeSize > 0)
    {
      MyBitArray o_bitmask;
      o_bitmask.resize(mx*my*mz);
      // make a copy of bitmask into o_bitmask
      o_bitmask = cbitmask;
  
      // dilation
      dilateBitmask(holeSize, false, // dilate opaque (false) region
		    mx, my, mz,
		    cbitmask);

      // followed by erosion
      dilateBitmask(holeSize, true, // dilate transparent (true) region
		    mx, my, mz,
		    cbitmask);

      // merge the original back in after erosion
      for(qint64 i=0; i<mx*my*mz; i++)
	cbitmask.setBit(i, cbitmask.testBit(i) & o_bitmask.testBit(i));
    }
  //----------------------------  


  QProgressDialog progress("Shrinkwrap",
			   QString(),
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);

  int startd = de;
  int startw = we;
  int starth = he;
  int endd = ds;
  int endw = ws;
  int endh = hs;
  for(qint64 d2=ds; d2<=de; d2++)
  {
    progress.setValue(90*(float)(d2-ds)/(float)mz);
    qApp->processEvents();
    for(qint64 w2=ws; w2<=we; w2++)
    for(qint64 h2=hs; h2<=he; h2++)
    {
      qint64 bidx = (d2-ds)*mx*my+(w2-ws)*mx+(h2-hs);
      if (cbitmask.testBit(bidx) == false) // opaque voxels
	{
	  startd = qMin(startd, (int)d2);
	  startw = qMin(startw, (int)w2);
	  starth = qMin(starth, (int)h2);
	  endd = qMax(endd, (int)d2);
	  endw = qMax(endw, (int)w2);
	  endh = qMax(endh, (int)h2);
	}
    }
  }
//  QMessageBox::information(0, "", QString("%1 %2 %3\n%4 %5 %6").\
//			   arg(startd).arg(startw).arg(starth).\
//			   arg(endd).arg(endw).arg(endh));
  //----------------------------  

  //----------------------------  
  // set the non-transparent block
  for(qint64 d2=startd; d2<=endd; d2++)
  for(qint64 w2=startw; w2<=endw; w2++)
  for(qint64 h2=starth; h2<=endh; h2++)
    {
      qint64 bidx = (d2-ds)*mx*my+(w2-ws)*mx+(h2-hs);
      bitmask.setBit(bidx, true);
    }
  //----------------------------  

  
  QList<Vec> edges;
  edges.clear();

  //----------------------------
  // set all size faces
  //------------------------------------------------------
  { // handle depth slices
    uchar *swvr = new uchar[my*mx];
    for(qint64 d=startd; d<=endd; d=endd)
      {
	progress.setLabelText(QString("Z %1").arg(d));
	qApp->processEvents();
	
	memset(swvr, 0, my*mx);
	
	for(qint64 w=ws; w<=we; w++)
	  for(qint64 h=hs; h<=he; h++)
	    {
	      qint64 bidx = (d-ds)*mx*my+(w-ws)*mx+(h-hs);
	      if (!cbitmask.testBit(bidx))
		swvr[(w-ws)*mx + (h-hs)] = 255;
	    }

	shrinkwrapSlice(swvr, mx, my);
	
	for(qint64 w=ws; w<=we; w++)
	  for(qint64 h=hs; h<=he; h++)
	    {
	      if (swvr[(w-ws)*mx + (h-hs)] == 0)
		{
		  qint64 bidx = (d-ds)*mx*my+(w-ws)*mx+(h-hs);
		  bitmask.setBit(bidx, false);
		  edges << Vec(d,w,h);
		}
	    }
	if (d == endd)
	  break;
      }
    delete [] swvr;
  }
  //------------------------------------------------------
  { // handle width slices
    uchar *swvr = new uchar[mz*mx];
    for(qint64 w=startw; w<=endw; w=endw)
      {
	progress.setLabelText(QString("Y %1").arg(w));
	qApp->processEvents();
		
	memset(swvr, 0, mz*mx);
	
	for(qint64 d=ds; d<=de; d++)
	  for(qint64 h=hs; h<=he; h++)
	    {
	      qint64 bidx = (d-ds)*mx*my+(w-ws)*mx+(h-hs);
	      if (!cbitmask.testBit(bidx))
		swvr[(d-ds)*mx + (h-hs)] = 255;
	    }
	
	shrinkwrapSlice(swvr, mx, mz);
	
	for(qint64 d=ds; d<=de; d++)
	  for(qint64 h=hs; h<=he; h++)
	    {
	      if (swvr[(d-ds)*mx + (h-hs)] == 0)
		{
		  qint64 bidx = (d-ds)*mx*my+(w-ws)*mx+(h-hs);
		  bitmask.setBit(bidx, false);
		  edges << Vec(d,w,h);
		}
	    }
	if (w == endw)
	  break;
      }
    delete [] swvr;
  }
  //------------------------------------------------------

  //------------------------------------------------------
  { // handle height slices
    uchar *swvr = new uchar[mz*my];
    for(qint64 h=starth; h<=endh; h=endh)
      {
	progress.setLabelText(QString("X %1").arg(h));
	qApp->processEvents();
	
	memset(swvr, 0, mz*my);
	
	for(qint64 d=ds; d<=de; d++)
	  for(qint64 w=ws; w<=we; w++)
	    {
	      qint64 bidx = (d-ds)*mx*my+(w-ws)*mx+(h-hs);
	      if (!cbitmask.testBit(bidx))
		swvr[(d-ds)*my + (w-ws)] = 255;
	    }
	
	shrinkwrapSlice(swvr, my, mz);
		
	for(qint64 d=ds; d<=de; d++)
	  for(qint64 w=ws; w<=we; w++)
	    {
	      if (swvr[(d-ds)*my + (w-ws)] == 0)
		{
		  qint64 bidx = (d-ds)*mx*my+(w-ws)*mx+(h-hs);
		  bitmask.setBit(bidx, false);
		  edges << Vec(d,w,h);
		}
	    }
	if (h == endh)
	  break;
      }
    delete [] swvr;
  }
  //------------------------------------------------------

  //------------------------------------------------------
  // now dilate from boundary
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

      progress.setLabelText(QString("Boundary detection %1").arg(edges.count()));
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
		  bitmask.testBit(bidx))
		{
		  bitmask.setBit(bidx, false);
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

  //------------------------------------------------------
  // if we want only shell remove interior
  if (shellOnly)
    {
      progress.setLabelText("Tag shell voxels");

      // copy bitmask into cbitmask
      cbitmask = bitmask;

      bitmask.fill(false);
      
      for(qint64 d=ds; d<=de; d++)
	{
	  progress.setValue(90*(d-ds)/mz);
	  if (d%10 == 0) qApp->processEvents();
	
	  for(qint64 w=ws; w<=we; w++)
	  for(qint64 h=hs; h<=he; h++)
	    {
	      qint64 bidx = (d-ds)*mx*my+(w-ws)*mx + (h-hs);
	      if (cbitmask.testBit(bidx))
		{
		  qint64 d2s = qBound(ds, (int)d-1, de);
		  qint64 w2s = qBound(ws, (int)w-1, we);
		  qint64 h2s = qBound(hs, (int)h-1, he);
		  qint64 d2e = qBound(ds, (int)d+1, de);
		  qint64 w2e = qBound(ws, (int)w+1, we);
		  qint64 h2e = qBound(hs, (int)h+1, he);
		  
		  bool ok = true;
		  for(qint64 d2=d2s; d2<=d2e; d2++)
		  for(qint64 w2=w2s; w2<=w2e; w2++)
		  for(qint64 h2=h2s; h2<=h2e; h2++)
		    {
		      qint64 cidx = (d2-ds)*mx*my+(w2-ws)*mx+(h2-hs);
		      if (!cbitmask.testBit(cidx))
			{
			  ok = false;
			  break;
			}			  
		    }
		  
		  if (!ok) // boundary voxel
		    bitmask.setBit(bidx, true);
		}
	    }
	}
      
      MyBitArray dbitmask;
      dbitmask.resize(mx*my*mz);
      
      for(int nd=1; nd<shellThickness; nd++)
	{
	  dbitmask.fill(false);
	  progress.setLabelText(QString("shell no. %1").arg(nd));
	  for(qint64 d=ds; d<=de; d++)
	    {
	      progress.setValue(99*(d-ds)/mz);
	      if (d%10 == 0) qApp->processEvents();
	      
	      for(qint64 w=ws; w<=we; w++)
	      for(qint64 h=hs; h<=he; h++)
		{
		  qint64 bidx = (d-ds)*mx*my+(w-ws)*mx + (h-hs);
		  if (bitmask.testBit(bidx) && cbitmask.testBit(bidx))
		    {
		      cbitmask.setBit(bidx, false);
		      qint64 d2s = qBound(ds, (int)d-1, de);
		      qint64 w2s = qBound(ws, (int)w-1, we);
		      qint64 h2s = qBound(hs, (int)h-1, he);
		      qint64 d2e = qBound(ds, (int)d+1, de);
		      qint64 w2e = qBound(ws, (int)w+1, we);
		      qint64 h2e = qBound(hs, (int)h+1, he);
		      for(qint64 d2=d2s; d2<=d2e; d2++)
			for(qint64 w2=w2s; w2<=w2e; w2++)
			  for(qint64 h2=h2s; h2<=h2e; h2++)
			    {
			      qint64 cidx = (d2-ds)*mx*my+(w2-ws)*mx+(h2-hs);
			      if (cbitmask.testBit(cidx))
				dbitmask.setBit(cidx, true);
			    }
		    }
		}
	    }
	  // OR dbitmask into bitmask
	  for(qint64 i=0; i<mx*my*mz; i++)
	    bitmask.setBit(i, bitmask.testBit(i) || dbitmask.testBit(i));
	  
	} // shellThickness
    }
  //------------------------------------------------------

  //----------------------------  
  // now set the maskData
  for(qint64 d2=ds; d2<=de; d2++)
  for(qint64 w2=ws; w2<=we; w2++)
  for(qint64 h2=hs; h2<=he; h2++)
    {
      qint64 bidx = (d2-ds)*mx*my+(w2-ws)*mx+(h2-hs);
      if (bitmask.testBit(bidx))
	{
	  qint64 idx = d2*m_width*m_height + w2*m_height + h2;
	  m_maskData[idx] = tag;
	}
    }
  //----------------------------  

  minD = ds;  maxD = de;
  minW = ws;  maxW = we;
  minH = hs;  maxH = he;
}

void
VolumeOperations::dilateBitmask(int nDilate, bool htype,
				qint64 mx, qint64 my, qint64 mz,
				MyBitArray &bitmask)
{
  QProgressDialog progress("Dilate bitmask",
			   QString(),
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);

  int indices[] = {-1, 0, 0,
		    1, 0, 0,
		    0,-1, 0,
		    0, 1, 0,
		    0, 0,-1,
		    0, 0, 1};
    
  // we use multiple QLists because there is a limit on the size of a single QList
  // and we might exceed that limit for complex volumes.
  QList<QList<Vec> > edges;
  edges.clear();

  qint64 MAXEDGES = 100000000;
  int ce = 0;
  QList<Vec> ege;
  edges << ege;

  // find inner boundary
  for(qint64 d=0; d<mz; d++)
    {
      progress.setValue(90*(float)d/(float)mz);
      if (d%10 == 0)
	{
	  progress.setLabelText(QString("Finding boundary - %1").arg(edges[ce].count()));
	  qApp->processEvents();
	}
      for(qint64 w=0; w<my; w++)
	for(qint64 h=0; h<mx; h++)
	  {
	    qint64 bidx = d*mx*my+w*mx+h;
	    if (bitmask.testBit(bidx) == htype)
	      {
		bool inside = true;
		for(int i=0; i<6; i++)
		  {
		    int da = indices[3*i+0];
		    int wa = indices[3*i+1];
		    int ha = indices[3*i+2];
		    
		    qint64 d2 = qBound((qint64)0, d+da, mz-1);
		    qint64 w2 = qBound((qint64)0, w+wa, my-1);
		    qint64 h2 = qBound((qint64)0, h+ha, mx-1);
		    
		    qint64 tidx = d2*mx*my+w2*mx+h2;
		    inside &= (bitmask.testBit(tidx) == htype);
		  }
		if (!inside)
		  {
		    if (edges[ce].count() >= MAXEDGES)
		      {
			QList<Vec> ege;
			edges << ege;
			ce++;
		      }
		    edges[ce] << Vec(d,w,h);
		  }
	      }
	  }
    }
  

//--------------------------
  // create convolution mask
  int crad = 2*nDilate;
  MyBitArray cm;
  cm.resize(crad*crad*crad);
  cm.fill(!htype);
  Vec cen = Vec(nDilate,nDilate,nDilate);
  for(int i=0; i<crad; i++)
    for(int j=0; j<crad; j++)
      for(int k=0; k<crad; k++)
	{
	  if ((Vec(i,j,k)-cen).norm() < nDilate)
	    cm.setBit(i*crad*crad+j*crad+k, htype);
	}

  for(int tce=0; tce<edges.count(); tce++)
    {
      for(int e=0; e<edges[tce].count(); e++)
	{
	  if (e%10000 == 0)
	    {
	      progress.setValue(90*(float)e/(float)edges[tce].count());
	      progress.setLabelText(QString("Dilating boundary %1").arg(e));
	      qApp->processEvents();
	    }
	  int dx = edges[tce][e].x;
	  int wx = edges[tce][e].y;
	  int hx = edges[tce][e].z;
	  
	  for(int i=0; i<crad; i++)
	    for(int j=0; j<crad; j++)
	      for(int k=0; k<crad; k++)
		{
		  qint64 d2 = qBound(0, dx+i-nDilate, (int)mz-1);
		  qint64 w2 = qBound(0, wx+j-nDilate, (int)my-1);
		  qint64 h2 = qBound(0, hx+k-nDilate, (int)mx-1);
		  if (cm.testBit(i*crad*crad+j*crad+k) == htype)
		    bitmask.setBit(d2*mx*my+w2*mx+h2, htype);		  
		}	  
	}
    }
//--------------------------

//  for(int ne=0; ne<nDilate; ne++)
//    {
//      progress.setValue(90*(float)ne/(float)nDilate);
//      qApp->processEvents();
//
//      QList<QList<Vec> > tedges;
//      tedges.clear();
//
//      QList<Vec> ege;
//      tedges << ege;
//
//      int ce = 0;
//      // find outer boundary to fill
//      for(int tce=0; tce<edges.count(); tce++)
//	{
//	  progress.setLabelText(QString("Dilating boundary %1").arg(edges[tce].count()));
//	  for(int e=0; e<edges[tce].count(); e++)
//	    {
//	      int dx = edges[tce][e].x;
//	      int wx = edges[tce][e].y;
//	      int hx = edges[tce][e].z;
//	      
//	      for(int i=0; i<6; i++)
//		{
//		  int da = indices[3*i+0];
//		  int wa = indices[3*i+1];
//		  int ha = indices[3*i+2];
//		  
//		  qint64 d2 = qBound(0, dx+da, (int)mz-1);
//		  qint64 w2 = qBound(0, wx+wa, (int)my-1);
//		  qint64 h2 = qBound(0, hx+ha, (int)mx-1);
//		  
//		  qint64 bidx = d2*mx*my+w2*mx+h2;
//		  if (bitmask.testBit(bidx) != htype)
//		    {
//		      bitmask.setBit(bidx, htype);
//		      if (tedges[ce].count() >= MAXEDGES)
//			{
//			  QList<Vec> ege;
//			  tedges << ege;
//			  ce++;
//			}
//		      tedges[ce] << Vec(d2,w2,h2);
//		    }
//		}
//	    }
//	}
//
//      for(int tce=0; tce<edges.count(); tce++)
//	edges[tce].clear();
//      edges.clear();
//
//      for(int tce=0; tce<tedges.count(); tce++)
//	edges << tedges[tce];
//    }
//
//  for(int tce=0; tce<edges.count(); tce++)
//    edges[tce].clear();
//  edges.clear();

  progress.setValue(100);
}

void
VolumeOperations::setVisible(Vec bmin, Vec bmax,
			     int tag, bool visible,
			     int& minD, int& maxD,
			     int& minW, int& maxW,
			     int& minH, int& maxH,
			     int gradType, float minGrad, float maxGrad)
{
  int ds = bmin.z;
  int ws = bmin.y;
  int hs = bmin.x;

  int de = bmax.z;
  int we = bmax.y;
  int he = bmax.x;

  QProgressDialog progress("Updating voxel structure",
			   QString(),
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);

  minD = maxD = -1;
  minW = maxW = -1;
  minH = maxH = -1;

  uchar *lut = Global::lut();

  for(qint64 d=ds; d<=de; d++)
    {
      progress.setValue(90*(d-ds)/((de-ds+1)));
      if (d%10 == 0)
	qApp->processEvents();
      for(qint64 w=ws; w<=we; w++)
	for(qint64 h=hs; h<=he; h++)
	  {
	    bool clipped = false;
	    for(int i=0; i<m_cPos.count(); i++)
	      {
		Vec p = Vec(h, w, d) - m_cPos[i];
		if (m_cNorm[i]*p > 0)
		  {
		    clipped = true;
		    break;
		  }
	      }
	    
	    if (!clipped)
	      {
		qint64 idx = d*m_width*m_height + w*m_height + h;
		int val = m_volData[idx];
		if (m_volDataUS) val = m_volDataUS[idx];
		uchar mtag = m_maskData[idx];
		bool alpha =  (lut[4*val+3]*Global::tagColors()[4*mtag+3] > 0);

		//-------
		if (alpha)
		{
		  float gradMag;
		  if (gradType == 0)
		    {
		      float gx,gy,gz;
		      qint64 d3 = qBound(0, (int)d+1, m_depth-1);
		      qint64 d4 = qBound(0, (int)d-1, m_depth-1);
		      qint64 w3 = qBound(0, (int)w+1, m_width-1);
		      qint64 w4 = qBound(0, (int)w-1, m_width-1);
		      qint64 h3 = qBound(0, (int)h+1, m_height-1);
		      qint64 h4 = qBound(0, (int)h-1, m_height-1);
		      if (!m_volDataUS)
			{
			  gz = (m_volData[d3*m_width*m_height + w*m_height + h] -
				m_volData[d4*m_width*m_height + w*m_height + h]);
			  gy = (m_volData[d*m_width*m_height + w3*m_height + h] -
				m_volData[d*m_width*m_height + w4*m_height + h]);
			  gx = (m_volData[d*m_width*m_height + w*m_height + h3] -
				m_volData[d*m_width*m_height + w*m_height + h4]);
			  gx/=255.0;
			  gy/=255.0;
			  gz/=255.0;
			}
		      else
			{
			  gz = (m_volDataUS[d3*m_width*m_height + w*m_height + h] -
				m_volDataUS[d4*m_width*m_height + w*m_height + h]);
			  gy = (m_volDataUS[d*m_width*m_height + w3*m_height + h] -
				m_volDataUS[d*m_width*m_height + w4*m_height + h]);
			  gx = (m_volDataUS[d*m_width*m_height + w*m_height + h3] -
				m_volDataUS[d*m_width*m_height + w*m_height + h4]);
			  gx/=65535.0;
			  gy/=65535.0;
			  gz/=65535.0;
			}
		  
		      Vec dv = Vec(gx, gy, gz); // surface gradient
		      gradMag = dv.norm();
		    } // gradType == 0

		  if (gradType > 0)
		    {
		      int sz = 1;
		      float divisor = 10.0;
		      if (gradType == 2)
			{
			  sz = 2;
			  divisor = 70.0;
			}
		      if (!m_volDataUS)
			{	      
			  float sum = 0;
			  float vval = m_volData[d*m_width*m_height + w*m_height + h];
			  for(int a=d-sz; a<=d+sz; a++)
			  for(int b=w-sz; b<=w+sz; b++)
			  for(int c=h-sz; c<=h+sz; c++)
			    {
			      qint64 a0 = qBound(0, a, m_depth-1);
			      qint64 b0 = qBound(0, b, m_width-1);
			      qint64 c0 = qBound(0, c, m_height-1);
			      sum += m_volData[a0*m_width*m_height + b0*m_height + c0];
			    }
			  
			  sum = (sum-vval)/divisor;
			  gradMag = fabs(sum-vval)/255.0;
			}
		      else
			{
			  float sum = 0;
			  float vval = m_volDataUS[d*m_width*m_height + w*m_height + h];
			  for(int a=d-sz; a<=d+sz; a++)
			  for(int b=w-sz; b<=w+sz; b++)
			  for(int c=h-sz; c<=h+sz; c++)
			    {
			      qint64 a0 = qBound(0, a, m_depth-1);
			      qint64 b0 = qBound(0, b, m_width-1);
			      qint64 c0 = qBound(0, c, m_height-1);
			      sum += m_volDataUS[a0*m_width*m_height + b0*m_height + c0];
			    }
			  
			  sum = (sum-vval)/divisor;
			  gradMag = fabs(sum-vval)/65535.0;
			}
		    } // gradType > 0
		  
		  
		  gradMag = qBound(0.0f, gradMag, 1.0f);	
		  
		  if (gradMag < minGrad || gradMag > maxGrad)
		    alpha = false;
		}
		//-------

		if (alpha == visible && m_maskData[idx] != tag)
		  {
		    m_maskData[idx] = tag;
		    if (minD > -1)
		      {
			minD = qMin(minD, (int)d);
			maxD = qMax(maxD, (int)d);
			minW = qMin(minW, (int)w);
			maxW = qMax(maxW, (int)w);
			minH = qMin(minH, (int)h);
			maxH = qMax(maxH, (int)h);
		      }
		    else
		      {
			minD = maxD = d;
			minW = maxW = w;
			minH = maxH = h;
		      }
		  }
	      } // clipped
	  }
    }
}

void
VolumeOperations::stepTags(Vec bmin, Vec bmax,
			   int tagStep, int tagVal,
			   int& minD, int& maxD,
			   int& minW, int& maxW,
			   int& minH, int& maxH)
  
{    
  int ds = bmin.z;
  int ws = bmin.y;
  int hs = bmin.x;

  int de = bmax.z;
  int we = bmax.y;
  int he = bmax.x;

  minD = ds;
  maxD = de;
  minW = ws;
  maxW = we;
  minH = hs;
  maxH = he;
  
  QProgressDialog progress("Updating voxel structure",
			   QString(),
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);

  for(qint64 d=ds; d<=de; d++)
    {
      progress.setValue(90*(d-ds)/((de-ds+1)));
      if (d%10 == 0)
	qApp->processEvents();
      for(qint64 w=ws; w<=we; w++)
	for(qint64 h=hs; h<=he; h++)
	  {
	    bool clipped = false;
//	    for(int i=0; i<m_cPos.count(); i++)
//	      {
//		Vec p = Vec(h, w, d) - m_cPos[i];
//		if (m_cNorm[i]*p > 0)
//		  {
//		    clipped = true;
//		    break;
//		  }
//	      }
	    
	    if (!clipped)
	      {
		qint64 idx = d*m_width*m_height + w*m_height + h;
		if (m_maskData[idx] < tagStep)
		  m_maskData[idx] = 0;
		else
		  m_maskData[idx] = tagVal;
	      }
	  }
    }
}

void
VolumeOperations::mergeTags(Vec bmin, Vec bmax,
			    int tag1, int tag2, bool useTF,
			    int& minD, int& maxD,
			    int& minW, int& maxW,
			    int& minH, int& maxH)

{
  int ds = bmin.z;
  int ws = bmin.y;
  int hs = bmin.x;

  int de = bmax.z;
  int we = bmax.y;
  int he = bmax.x;

  minD = maxD = -1;
  minW = maxW = -1;
  minH = maxH = -1;

  QProgressDialog progress("Updating voxel structure",
			   QString(),
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);

  if (useTF)
    {
      uchar *lut = Global::lut();
      for(qint64 d=ds; d<=de; d++)
	{
	  progress.setValue(90*(d-ds)/((de-ds+1)));
	  if (d%10 == 0)
	    qApp->processEvents();
	  for(qint64 w=ws; w<=we; w++)
	    for(qint64 h=hs; h<=he; h++)
	      {
		bool clipped = false;
		for(int i=0; i<m_cPos.count(); i++)
		  {
		    Vec p = Vec(h, w, d) - m_cPos[i];
		    if (m_cNorm[i]*p > 0)
		      {
			clipped = true;
			break;
		      }
		  }
		
		if (!clipped)
		  {
		    qint64 idx = d*m_width*m_height + w*m_height + h;
		    if (tag2 == -1 || m_maskData[idx] == tag2)
		      {
			int val = m_volData[idx];
			if (m_volDataUS) val = m_volDataUS[idx];
			int a =  lut[4*val+3];
			if (a > 0)
			  {
			    m_maskData[idx] = tag1;
			    if (minD > -1)
			      {
				minD = qMin(minD, (int)d);
				maxD = qMax(maxD, (int)d);
				minW = qMin(minW, (int)w);
				maxW = qMax(maxW, (int)w);
				minH = qMin(minH, (int)h);
				maxH = qMax(maxH, (int)h);
			      }
			    else
			      {
				minD = maxD = d;
				minW = maxW = w;
				minH = maxH = h;
			      }
			  }
		      }
		  }
	      }
	}
    }
  else
    {
      for(qint64 d=ds; d<=de; d++)
	{
	  progress.setValue(90*(d-ds)/((de-ds+1)));
	  if (d%10 == 0)
	    qApp->processEvents();
	  for(qint64 w=ws; w<=we; w++)
	    for(qint64 h=hs; h<=he; h++)
	      {
		bool clipped = false;
		for(int i=0; i<m_cPos.count(); i++)
		  {
		    Vec p = Vec(h, w, d) - m_cPos[i];
		    if (m_cNorm[i]*p > 0)
		      {
			clipped = true;
			break;
		      }
		  }
		
		if (!clipped)
		  {
		    qint64 idx = d*m_width*m_height + w*m_height + h;
		    if (tag2 == -1 || m_maskData[idx] == tag2)
		      {
			m_maskData[idx] = tag1;
			if (minD > -1)
			  {
			    minD = qMin(minD, (int)d);
			    maxD = qMax(maxD, (int)d);
			    minW = qMin(minW, (int)w);
			    maxW = qMax(maxW, (int)w);
			    minH = qMin(minH, (int)h);
			    maxH = qMax(maxH, (int)h);
			  }
			else
			  {
			    minD = maxD = d;
			    minW = maxW = w;
			    minH = maxH = h;
			  }
		      }
		  }
	      }
	}
    }
}

void
VolumeOperations::dilateConnected(int dr, int wr, int hr,
				  Vec bmin, Vec bmax, int tag,
				  int nDilate,
				  int& minD, int& maxD,
				  int& minW, int& maxW,
				  int& minH, int& maxH,
				  bool allVisible,
				  int gradType, float minGrad, float maxGrad)
{
  if (dr < 0 || wr < 0 || hr < 0 ||
      dr > m_depth-1 ||
      wr > m_width-1 ||
      hr > m_height-1)
    {
      QMessageBox::information(0, "", QString("No visible region found at %1 %2 %3").\
			       arg(hr).arg(wr).arg(dr));
      return;
    }


  uchar *lut = Global::lut();

  {
    qint64 idx = (qint64)dr*m_width*m_height + (qint64)wr*m_height + (qint64)hr;
    int val = m_volData[idx];
    if (m_volDataUS) val = m_volDataUS[idx];
    if (lut[4*val+3] == 0 || m_maskData[idx] != tag)
      {
	QMessageBox::information(0, "Dilate",
				 QString("Cannot dilate.\nYou are on voxel with tag %1, was expecting tag %2").arg(m_maskData[idx]).arg(tag));
	return;
      }
  }

  QProgressDialog progress("Updating voxel structure",
			   QString(),
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);

  int ds = qMax(0, (int)bmin.z);
  int ws = qMax(0, (int)bmin.y);
  int hs = qMax(0, (int)bmin.x);

  int de = qMin((int)bmax.z, m_depth-1);
  int we = qMin((int)bmax.y, m_width-1);
  int he = qMin((int)bmax.x, m_height-1);

  qint64 mx = he-hs+1;
  qint64 my = we-ws+1;
  qint64 mz = de-ds+1;

  MyBitArray bitmask;
  bitmask.resize(mx*my*mz);
  bitmask.fill(false);

  MyBitArray cbitmask;
  cbitmask.resize(mx*my*mz);
  cbitmask.fill(false);

  int indices[] = {-1, 0, 0,
		    1, 0, 0,
		    0,-1, 0,
		    0, 1, 0,
		    0, 0,-1,
		    0, 0, 1};

  // find connected region before dilation

  QQueue<Vec> que;
  que.enqueue(Vec(dr,wr,hr));

  qint64 bidx = (dr-ds)*mx*my+(wr-ws)*mx+(hr-hs);
  bitmask.setBit(bidx, true);
  cbitmask.setBit(bidx, true);

  minD = maxD = dr;
  minW = maxW = wr;
  minH = maxH = hr;

  int pgstep = 10*m_width*m_height;
  int prevpgv = 0;
  int ip=0;
  while(!que.isEmpty())
    {
      ip = (ip+1)%pgstep;
      int pgval = 99*(float)ip/(float)pgstep;
      progress.setValue(pgval);
      if (pgval != prevpgv)
	{
	  progress.setLabelText(QString("Updating voxel structure %1").arg(que.count()));
	  qApp->processEvents();
	}
      prevpgv = pgval;
      
      Vec dwh = que.dequeue();
      int dx = qBound(ds, qCeil(dwh.x), de);
      int wx = qBound(ws, qCeil(dwh.y), we);
      int hx = qBound(hs, qCeil(dwh.z), he);

      for(int i=0; i<6; i++)
	{
	  int da = indices[3*i+0];
	  int wa = indices[3*i+1];
	  int ha = indices[3*i+2];

	  qint64 d2 = qBound(ds, dx+da, de);
	  qint64 w2 = qBound(ws, wx+wa, we);
	  qint64 h2 = qBound(hs, hx+ha, he);

	  qint64 bidx = (d2-ds)*mx*my+(w2-ws)*mx+(h2-hs);
	  if (!cbitmask.testBit(bidx))
	    {
	      cbitmask.setBit(bidx, true);

	      bool clipped = false;
	      for(int i=0; i<m_cPos.count(); i++)
		{
		  Vec p = Vec(h2, w2, d2) - m_cPos[i];
		  if (m_cNorm[i]*p > 0)
		    {
		      clipped = true;
		      break;
		    }
		}
	      if (!clipped)
		{
		  qint64 idx = d2*m_width*m_height + w2*m_height + h2;
		  int val = m_volData[idx];
		  if (m_volDataUS) val = m_volDataUS[idx];
		  uchar mtag = m_maskData[idx];
		  bool opaque =  (lut[4*val+3]*Global::tagColors()[4*mtag+3] > 0);
		  opaque &= mtag == tag;

		  //-------
		  if (opaque)
		  {
		    float gradMag;
		    if (gradType == 0)
		      {
			float gx,gy,gz;
			qint64 d3 = qBound(0, (int)d2+1, m_depth-1);
			qint64 d4 = qBound(0, (int)d2-1, m_depth-1);
			qint64 w3 = qBound(0, (int)w2+1, m_width-1);
			qint64 w4 = qBound(0, (int)w2-1, m_width-1);
			qint64 h3 = qBound(0, (int)h2+1, m_height-1);
			qint64 h4 = qBound(0, (int)h2-1, m_height-1);
			if (!m_volDataUS)
			  {
			    gz = (m_volData[d3*m_width*m_height + w2*m_height + h2] -
				  m_volData[d4*m_width*m_height + w2*m_height + h2]);
			    gy = (m_volData[d2*m_width*m_height + w3*m_height + h2] -
				  m_volData[d2*m_width*m_height + w4*m_height + h2]);
			    gx = (m_volData[d2*m_width*m_height + w2*m_height + h3] -
				  m_volData[d2*m_width*m_height + w2*m_height + h4]);
			    gx/=255.0;
			    gy/=255.0;
			    gz/=255.0;
			  }
			else
			  {
			    gz = (m_volDataUS[d3*m_width*m_height + w2*m_height + h2] -
				  m_volDataUS[d4*m_width*m_height + w2*m_height + h2]);
			    gy = (m_volDataUS[d2*m_width*m_height + w3*m_height + h2] -
				  m_volDataUS[d2*m_width*m_height + w4*m_height + h2]);
			    gx = (m_volDataUS[d2*m_width*m_height + w2*m_height + h3] -
				  m_volDataUS[d2*m_width*m_height + w2*m_height + h4]);
			    gx/=65535.0;
			    gy/=65535.0;
			    gz/=65535.0;
			  }
			
			Vec dv = Vec(gx, gy, gz); // surface gradient
			gradMag = dv.norm();
		      }

		    if (gradType > 0)
		      {
			int sz = 1;
			float divisor = 10.0;
			if (gradType == 2)
			  {
			    sz = 2;
			    divisor = 70.0;
			  }
			if (!m_volDataUS)
			  {	      
			    float sum = 0;
			    float vval = m_volData[d2*m_width*m_height + w2*m_height + h2];
			    for(int a=d2-sz; a<=d2+sz; a++)
			    for(int b=w2-sz; b<=w2+sz; b++)
			    for(int c=h2-sz; c<=h2+sz; c++)
			      {
				qint64 a0 = qBound(0, a, m_depth-1);
				qint64 b0 = qBound(0, b, m_width-1);
				qint64 c0 = qBound(0, c, m_height-1);
				sum += m_volData[a0*m_width*m_height + b0*m_height + c0];
			      }
			    
			    sum = (sum-vval)/divisor;
			    gradMag = fabs(sum-vval)/255.0;
			  }
			else
			  {
			    float sum = 0;
			    float vval = m_volDataUS[d2*m_width*m_height + w2*m_height + h2];
			    for(int a=d2-sz; a<=d2+sz; a++)
			    for(int b=w2-sz; b<=w2+sz; b++)
			    for(int c=h2-sz; c<=h2+sz; c++)
			      {
				qint64 a0 = qBound(0, a, m_depth-1);
				qint64 b0 = qBound(0, b, m_width-1);
				qint64 c0 = qBound(0, c, m_height-1);
				sum += m_volDataUS[a0*m_width*m_height + b0*m_height + c0];
			      }
			    
			    sum = (sum-vval)/divisor;
			    gradMag = fabs(sum-vval)/65535.0;
			  }
		      } // gradType > 0
		    
		    gradMag = qBound(0.0f, gradMag, 1.0f);	
		    
		    if (gradMag < minGrad || gradMag > maxGrad)
		      opaque = false;
		  }
		  //-------

	      if (opaque)
		    {
		      bitmask.setBit(bidx, true);
		      que.enqueue(Vec(d2,w2,h2)); 
		    }
		}
	    }
	}
    }

  progress.setLabelText("Dilate");
  qApp->processEvents();




  dilateBitmask(nDilate, true, // dilate opaque region
		mx, my, mz,
		bitmask);


  
  progress.setLabelText("writing to mask");
  for(int d2=ds; d2<=de; d2++)
    {
      progress.setValue(100*(d2-ds)/(mz));
      qApp->processEvents();
      for(int w2=ws; w2<=we; w2++)
	for(int h2=hs; h2<=he; h2++)
	  {
	    qint64 bidx = (d2-ds)*mx*my+(w2-ws)*mx+(h2-hs);
	    if (bitmask.testBit(bidx))
	      {
		bool clipped = false;
		for(int i=0; i<m_cPos.count(); i++)
		  {
		    Vec p = Vec(h2, w2, d2) - m_cPos[i];
		    if (m_cNorm[i]*p > 0)
		      {
			clipped = true;
			break;
		      }
		  }
		if (!clipped)
		  {
		    qint64 idx = d2*m_width*m_height + w2*m_height + h2;
		    int val = m_volData[idx];
		    if (m_volDataUS) val = m_volDataUS[idx];
		    
		    uchar mtag = m_maskData[idx];
		    bool opaque =  (lut[4*val+3]*Global::tagColors()[4*mtag+3] > 0);
		    opaque &= (mtag == 0 || allVisible);
			  
		    //-------
		    if (opaque && (minGrad >=0.01 || maxGrad <= 0.99))
		      {
			float gradMag;
			if (gradType == 0)
			  {
			    float gx,gy,gz;
			    qint64 d3 = qBound(0, (int)d2+1, m_depth-1);
			    qint64 d4 = qBound(0, (int)d2-1, m_depth-1);
			    qint64 w3 = qBound(0, (int)w2+1, m_width-1);
			    qint64 w4 = qBound(0, (int)w2-1, m_width-1);
			    qint64 h3 = qBound(0, (int)h2+1, m_height-1);
			    qint64 h4 = qBound(0, (int)h2-1, m_height-1);
			    if (!m_volDataUS)
			      {
				gz = (m_volData[d3*m_width*m_height + w2*m_height + h2] -
				      m_volData[d4*m_width*m_height + w2*m_height + h2]);
				gy = (m_volData[d2*m_width*m_height + w3*m_height + h2] -
				      m_volData[d2*m_width*m_height + w4*m_height + h2]);
				gx = (m_volData[d2*m_width*m_height + w2*m_height + h3] -
				      m_volData[d2*m_width*m_height + w2*m_height + h4]);
				gx/=255.0;
				gy/=255.0;
				gz/=255.0;
			      }
			    else
			      {
				gz = (m_volDataUS[d3*m_width*m_height + w2*m_height + h2] -
				      m_volDataUS[d4*m_width*m_height + w2*m_height + h2]);
				gy = (m_volDataUS[d2*m_width*m_height + w3*m_height + h2] -
				      m_volDataUS[d2*m_width*m_height + w4*m_height + h2]);
				gx = (m_volDataUS[d2*m_width*m_height + w2*m_height + h3] -
				      m_volDataUS[d2*m_width*m_height + w2*m_height + h4]);
				gx/=65535.0;
				gy/=65535.0;
				gz/=65535.0;
			      }
			    
			    Vec dv = Vec(gx, gy, gz); // surface gradient
			    gradMag = dv.norm();
			  } // gradType == 0
			
			if (gradType > 0)
			  {
			    int sz = 1;
			    float divisor = 10.0;
			    if (gradType == 2)
			      {
				sz = 2;
				divisor = 70.0;
			      }
			    if (!m_volDataUS)
			      {	      
				float sum = 0;
				float vval = m_volData[d2*m_width*m_height + w2*m_height + h2];
				for(int a=d2-sz; a<=d2+sz; a++)
				  for(int b=w2-sz; b<=w2+sz; b++)
				    for(int c=h2-sz; c<=h2+sz; c++)
				      {
					qint64 a0 = qBound(0, a, m_depth-1);
					qint64 b0 = qBound(0, b, m_width-1);
					qint64 c0 = qBound(0, c, m_height-1);
					sum += m_volData[a0*m_width*m_height + b0*m_height + c0];
				      }
				
				sum = (sum-vval)/divisor;
				gradMag = fabs(sum-vval)/255.0;
			      }
			    else
			      {
				float sum = 0;
				float vval = m_volDataUS[d2*m_width*m_height + w2*m_height + h2];
				for(int a=d2-sz; a<=d2+sz; a++)
				  for(int b=w2-sz; b<=w2+sz; b++)
				    for(int c=h2-sz; c<=h2+sz; c++)
				      {
					qint64 a0 = qBound(0, a, m_depth-1);
					qint64 b0 = qBound(0, b, m_width-1);
					qint64 c0 = qBound(0, c, m_height-1);
					sum += m_volDataUS[a0*m_width*m_height + b0*m_height + c0];
				      }
				
				sum = (sum-vval)/divisor;
				gradMag = fabs(sum-vval)/65535.0;
			      }
			  } // gradType > 0
			    
			gradMag = qBound(0.0f, gradMag, 1.0f);	
			
			if (gradMag < minGrad || gradMag > maxGrad)
			  opaque = false;
		      }
		    //-------
		    
		    if (opaque) // dilate only in connected opaque region
		      {
			
			qint64 idx = d2*m_width*m_height + w2*m_height + h2;
			m_maskData[idx] = tag;
		      }
		  } // if (!clipped)
	      } // test bitmask 
	  } // loop over h
    } // loop over d

  minD = ds;
  maxD = de;
  minW = ws;
  maxW = we;
  minH = hs;
  maxH = he;

  return;
}

void
VolumeOperations::erodeConnected(int dr, int wr, int hr,
				 Vec bmin, Vec bmax, int tag,
				 int nErode,
				 int& minD, int& maxD,
				 int& minW, int& maxW,
				 int& minH, int& maxH,
				 int gradType, float minGrad, float maxGrad)
{
  if (dr < 0 || wr < 0 || hr < 0 ||
      dr > m_depth-1 ||
      wr > m_width-1 ||
      hr > m_height-1)
    {
      QMessageBox::information(0, "", QString("No visible region found at %1 %2 %3").\
			       arg(hr).arg(wr).arg(dr));
      return;
    }

  uchar *lut = Global::lut();

  {
    qint64 idx = (qint64)dr*m_width*m_height + (qint64)wr*m_height + (qint64)hr;
    int val = m_volData[idx];
    if (m_volDataUS) val = m_volDataUS[idx];
    if (lut[4*val+3] == 0 || m_maskData[idx] != tag)
      {
	QMessageBox::information(0, "Erode",
				 QString("Cannot erode.\nYou are on voxel with tag %1, was expecting tag %2").arg(m_maskData[idx]).arg(tag));
	return;
      }
  }

    
  QProgressDialog progress("Updating voxel structure",
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

  MyBitArray bitmask;
  bitmask.resize(mx*my*mz);
  bitmask.fill(false);

  MyBitArray cbitmask;
  cbitmask.resize(mx*my*mz);
  cbitmask.fill(false);

  int indices[] = {-1, 0, 0,
		   1, 0, 0,
		   0,-1, 0,
		   0, 1, 0,
		   0, 0,-1,
		   0, 0, 1};

  // find connected region before erosion

  QQueue<Vec> que;
  que.enqueue(Vec(dr,wr,hr));

  qint64 bidx = (dr-ds)*mx*my+(wr-ws)*mx+(hr-hs);
  bitmask.setBit(bidx, true);
  cbitmask.setBit(bidx, true);

  minD = maxD = dr;
  minW = maxW = wr;
  minH = maxH = hr;

  int pgstep = 10*m_width*m_height;
  int prevpgv = 0;
  int ip=0;
  while(!que.isEmpty())
    {
      ip = (ip+1)%pgstep;
      int pgval = 99*(float)ip/(float)pgstep;
      progress.setValue(pgval);
      if (pgval != prevpgv)
	{
	  progress.setLabelText(QString("Updating voxel structure %1").arg(que.count()));
	  qApp->processEvents();
	}
      prevpgv = pgval;
      
      Vec dwh = que.dequeue();
      int dx = qCeil(dwh.x);
      int wx = qCeil(dwh.y);
      int hx = qCeil(dwh.z);

      for(int i=0; i<6; i++)
	{
	  int da = indices[3*i+0];
	  int wa = indices[3*i+1];
	  int ha = indices[3*i+2];

	  qint64 d2 = qBound(ds, dx+da, de);
	  qint64 w2 = qBound(ws, wx+wa, we);
	  qint64 h2 = qBound(hs, hx+ha, he);

	  qint64 bidx = (d2-ds)*mx*my+(w2-ws)*mx+(h2-hs);
	  if (!cbitmask.testBit(bidx))
	    {
	      cbitmask.setBit(bidx, true);
	      qint64 idx = d2*m_width*m_height + w2*m_height + h2;
	      int val = m_volData[idx];
	      if (m_volDataUS) val = m_volDataUS[idx];

	      //-------
	      bool opaque = true;
	      bool clipped = false;
	      for(int i=0; i<m_cPos.count(); i++)
		{
		  Vec p = Vec(h2, w2, d2) - m_cPos[i];
		  if (m_cNorm[i]*p > 0)
		    {
		      clipped = true;
		      opaque = false;
		      break;
		    }
		}
	      if (!clipped)
	      {
		float gradMag;
		if (gradType == 0)
		  {
		    float gx,gy,gz;
		    qint64 d3 = qBound(0, (int)d2+1, m_depth-1);
		    qint64 d4 = qBound(0, (int)d2-1, m_depth-1);
		    qint64 w3 = qBound(0, (int)w2+1, m_width-1);
		    qint64 w4 = qBound(0, (int)w2-1, m_width-1);
		    qint64 h3 = qBound(0, (int)h2+1, m_height-1);
		    qint64 h4 = qBound(0, (int)h2-1, m_height-1);
		    if (!m_volDataUS)
		      {
			gz = (m_volData[d3*m_width*m_height + w2*m_height + h2] -
			      m_volData[d4*m_width*m_height + w2*m_height + h2]);
			gy = (m_volData[d2*m_width*m_height + w3*m_height + h2] -
			      m_volData[d2*m_width*m_height + w4*m_height + h2]);
			gx = (m_volData[d2*m_width*m_height + w2*m_height + h3] -
			      m_volData[d2*m_width*m_height + w2*m_height + h4]);
			gx/=255.0;
			gy/=255.0;
			gz/=255.0;
		      }
		    else
		      {
			gz = (m_volDataUS[d3*m_width*m_height + w2*m_height + h2] -
			      m_volDataUS[d4*m_width*m_height + w2*m_height + h2]);
			gy = (m_volDataUS[d2*m_width*m_height + w3*m_height + h2] -
			      m_volDataUS[d2*m_width*m_height + w4*m_height + h2]);
			gx = (m_volDataUS[d2*m_width*m_height + w2*m_height + h3] -
			      m_volDataUS[d2*m_width*m_height + w2*m_height + h4]);
			gx/=65535.0;
			gy/=65535.0;
			gz/=65535.0;
		      }
		
		    Vec dv = Vec(gx, gy, gz); // surface gradient
		    gradMag = dv.norm();
		  } // gradType = 0
		
		if (gradType > 0)
		  {
		    int sz = 1;
		    float divisor = 10.0;
		    if (gradType == 2)
		      {
			sz = 2;
			divisor = 70.0;
		      }
		    if (!m_volDataUS)
		      {	      
			float sum = 0;
			float vval = m_volData[d2*m_width*m_height + w2*m_height + h2];
			for(int a=d2-sz; a<=d2+sz; a++)
			for(int b=w2-sz; b<=w2+sz; b++)
			for(int c=h2-sz; c<=h2+sz; c++)
			  {
			    qint64 a0 = qBound(0, a, m_depth-1);
			    qint64 b0 = qBound(0, b, m_width-1);
			    qint64 c0 = qBound(0, c, m_height-1);
			    sum += m_volData[a0*m_width*m_height + b0*m_height + c0];
			  }
			
			sum = (sum-vval)/divisor;
			gradMag = fabs(sum-vval)/255.0;
		      }
		    else
		      {
			float sum = 0;
			float vval = m_volDataUS[d2*m_width*m_height + w2*m_height + h2];
			for(int a=d2-sz; a<=d2+sz; a++)
			for(int b=w2-sz; b<=w2+sz; b++)
			  for(int c=h2-sz; c<=h2+sz; c++)
			    {
			      qint64 a0 = qBound(0, a, m_depth-1);
			      qint64 b0 = qBound(0, b, m_width-1);
			      qint64 c0 = qBound(0, c, m_height-1);
			      sum += m_volDataUS[a0*m_width*m_height + b0*m_height + c0];
			    }
			
			sum = (sum-vval)/divisor;
			gradMag = fabs(sum-vval)/65535.0;
		      }
		  } // gradType > 0
		
		gradMag = qBound(0.0f, gradMag, 1.0f);	
		
		if (gradMag < minGrad || gradMag > maxGrad)
		  opaque = false;
	      }
	      //-------
	      
	      if (opaque && lut[4*val+3] > 0 && m_maskData[idx] == tag)
		{
		  bitmask.setBit(bidx, true);
		  que.enqueue(Vec(d2,w2,h2)); 
		  minD = qMin(minD, (int)d2);
		  maxD = qMax(maxD, (int)d2);
		  minW = qMin(minW, (int)w2);
		  maxW = qMax(maxW, (int)w2);
		  minH = qMin(minH, (int)h2);
		  maxH = qMax(maxH, (int)h2);
		}
	    }
	}
    }

  progress.setLabelText("Erode");
  qApp->processEvents();


  //========================

  // copy bitmask into cbitmask
  cbitmask = bitmask;

  dilateBitmask(nErode, false, // dilate transparent region
		mx, my, mz,
		bitmask);


  
  progress.setLabelText("writing to mask");
  for(int d2=ds; d2<=de; d2++)
    {
      progress.setValue(100*(d2-ds)/(mz));
      qApp->processEvents();
      for(int w2=ws; w2<=we; w2++)
	for(int h2=hs; h2<=he; h2++)
	  {
	    qint64 bidx = (d2-ds)*mx*my+(w2-ws)*mx+(h2-hs);
	    if (!bitmask.testBit(bidx) && cbitmask.testBit(bidx))
	      {
		qint64 idx = d2*m_width*m_height + w2*m_height + h2;
		m_maskData[idx] = 0;
	      } // test bitmask 
	  } // loop over h
    } // loop over d
  
  minD = ds;
  maxD = de;
  minW = ws;
  maxW = we;
  minH = hs;
  maxH = he;
}


void
VolumeOperations::modifyOriginalVolume(Vec bmin, Vec bmax,
				       int val,
				       int& minD, int& maxD,
				       int& minW, int& maxW,
				       int& minH, int& maxH)
{
  int ds = bmin.z;
  int ws = bmin.y;
  int hs = bmin.x;

  int de = bmax.z;
  int we = bmax.y;
  int he = bmax.x;

  QProgressDialog progress("Updating Original Volume",
			   QString(),
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);

  minD = maxD = -1;
  minW = maxW = -1;
  minH = maxH = -1;

  uchar *lut = Global::lut();

  for(qint64 d=ds; d<=de; d++)
    {
      progress.setValue(90*(d-ds)/((de-ds+1)));
      if (d%10 == 0)
	qApp->processEvents();
      for(qint64 w=ws; w<=we; w++)
	for(qint64 h=hs; h<=he; h++)
	  {
	    bool clipped = false;
	    for(int i=0; i<m_cPos.count(); i++)
	      {
		Vec p = Vec(h, w, d) - m_cPos[i];
		if (m_cNorm[i]*p > 0)
		  {
		    clipped = true;
		    break;
		  }
	      }
	    
	    qint64 idx = d*m_width*m_height + w*m_height + h;
	    int vox = m_volData[idx];
	    if (m_volDataUS) vox = m_volDataUS[idx];
	    uchar mtag = m_maskData[idx];
	    bool visible =  (lut[4*vox+3]*Global::tagColors()[4*mtag+3] > 0);
	    if (clipped || !visible)
	      {
		if (!m_volDataUS)
		  m_volData[idx] = val;
		else
		  m_volDataUS[idx] = val;
		if (minD > -1)
		  {
		    minD = qMin(minD, (int)d);
		    maxD = qMax(maxD, (int)d);
		    minW = qMin(minW, (int)w);
		    maxW = qMax(maxW, (int)w);
		    minH = qMin(minH, (int)h);
		    maxH = qMax(maxH, (int)h);
		  }
		else
		  {
		    minD = maxD = d;
		    minW = maxW = w;
		    minH = maxH = h;
		  }
	      } // modify original volume if voxel is clipped or transparent
	  }
    }
}


void
VolumeOperations::tagTubes(Vec bmin, Vec bmax, int tag,
			   bool all,
			   int dr, int wr, int hr, int ctag,
			   int& minD, int& maxD,
			   int& minW, int& maxW,
			   int& minH, int& maxH,
			   int gradType, float minGrad, float maxGrad)
{
  //-------------------------
  int tubeSize = 0;
  tubeSize = QInputDialog::getInt(0,
				  "Tube/Sheet Size",
				  "Size",
				  0, 0, 100, 1);
  if (tubeSize == 0)
    {
      QMessageBox::information(0, "", "0 size not valid");
      return;
    }
  //-------------------------

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

  MyBitArray cbitmask;
  cbitmask.resize(mx*my*mz);
  cbitmask.fill(false);


  int indices[] = {-1, 0, 0,
		    1, 0, 0,
		    0,-1, 0,
		    0, 1, 0,
		    0, 0,-1,
		    0, 0, 1};
  
  //----------------------------  
  if (all) // identify all opaque region  
    {
      getTransparentRegion(ds, ws, hs,
			 de, we, he,
			 cbitmask,
			 gradType, minGrad, maxGrad);
      // invert all values in cbitmask
      cbitmask.invert();
    }
  else // identify connected opaque region
    {
      getConnectedRegion(dr, wr, hr,
			 ds, ws, hs,
			 de, we, he,
			 ctag, false,
			 cbitmask,
			 gradType, minGrad, maxGrad);
    }
  //----------------------------  


  //----------------------------
    {
      MyBitArray o_bitmask;
      o_bitmask.resize(mx*my*mz);
      // make a copy of bitmask into o_bitmask
      o_bitmask = cbitmask;
  
//      // erosion
//      dilateBitmask(tubeSize, false,
//		    mx, my, mz,
//		    cbitmask);
//
//      // followed by dilation
//      dilateBitmask(tubeSize+1, true,
//		    mx, my, mz,
//		    cbitmask);

      for(int i=0; i<tubeSize; i++)
	{
	  int tsz = tubeSize-i;
	  dilateBitmask(tsz, false,
			mx, my, mz,
			cbitmask);
	  
	  // followed by dilation
	  dilateBitmask(tsz+1, true,
			mx, my, mz,
			cbitmask);
	}

      // remove the dilation from original 
      for(qint64 i=0; i<mx*my*mz; i++)
	cbitmask.setBit(i, !cbitmask.testBit(i) & o_bitmask.testBit(i));
    }
  //----------------------------  

  //----------------------------  
  // now set the maskData
  for(qint64 d2=ds; d2<=de; d2++)
  for(qint64 w2=ws; w2<=we; w2++)
  for(qint64 h2=hs; h2<=he; h2++)
    {
      qint64 bidx = (d2-ds)*mx*my+(w2-ws)*mx+(h2-hs);
      if (cbitmask.testBit(bidx))
	{
	  qint64 idx = d2*m_width*m_height + w2*m_height + h2;
	  m_maskData[idx] = tag;
	}
    }
  //----------------------------  

  minD = ds;  maxD = de;
  minW = ws;  maxW = we;
  minH = hs;  maxH = he;
}
