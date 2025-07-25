#include <GL/glew.h>
#include "volumeoperations.h"
#include "volumeinformation.h"
#include "global.h"
#include "mybitarray.h"
#include "morphslice.h"
#include "vdbvolume.h"
#include "geometryobjects.h"
#include "staticfunctions.h"
#include "structuringelement.h"
#include "binarydistancetransform.h"

#include <QInputDialog>
#include <QtConcurrentMap>

#include "cc3d.h"

//#include "edt.hpp"
//using namespace edt;


uchar* VolumeOperations::m_volData = 0;
ushort* VolumeOperations::m_volDataUS = 0;

ushort* VolumeOperations::m_maskDataUS = 0;

MyBitArray VolumeOperations::m_visibilityMap;

QMap<QString, MyBitArray> VolumeOperations::m_roi;


void VolumeOperations::setVolData(uchar *v)
{
  StructuringElement::init();
  
  m_volData = v;
  m_volDataUS = 0;
  if (Global::bytesPerVoxel() == 2)
    m_volDataUS = (ushort*) m_volData;
}
void VolumeOperations::setMaskData(uchar *v)
{
  m_maskDataUS = (ushort*)v;
}


int VolumeOperations::m_depth = 0;
int VolumeOperations::m_width = 0;
int VolumeOperations::m_height = 0;
void VolumeOperations::setGridSize(int d, int w, int h)
{
  m_depth = d;
  m_width = w;
  m_height = h;

  m_visibilityMap.clear();
  m_visibilityMap.resize((qint64)d*(qint64)w*(qint64)h);
  m_visibilityMap.fill(false); 
}


bool
VolumeOperations::saveToROI(Vec bmin, Vec bmax,
			    int tag,
			    int& minD, int& maxD,
			    int& minW, int& maxW,
			    int& minH, int& maxH,
			    int gradType, float minGrad, float maxGrad)
{
  minD = maxD = minW = maxW = minH = maxH = -1;

  int ds = qMax(0, qFloor(bmin.z));
  int ws = qMax(0, qFloor(bmin.y));
  int hs = qMax(0, qFloor(bmin.x));

  int de = qMin(qCeil(bmax.z), m_depth-1);
  int we = qMin(qCeil(bmax.y), m_width-1);
  int he = qMin(qCeil(bmax.x), m_height-1);

  qint64 mx = he-hs+1;
  qint64 my = we-ws+1;
  qint64 mz = de-ds+1;

  bool ok;
  QString name = QInputDialog::getText(0,
				       "Save to ROI",
				       "ROI Name",
				       QLineEdit::Normal,
				       "",
				       &ok);
  name = name.trimmed();
  if (!ok || name.isEmpty())
    {
      QMessageBox::information(0, "Save To ROI", "Empty Name not allowed - ROI not saved\nPlease try again.");
      return false;
    }
  
  MyBitArray bitmask;
  bitmask.resize(mx*my*mz);
  bitmask.fill(false);
  
  getVisibleRegion(ds, ws, hs,
		   de, we, he,		   
		   tag, false,
		   gradType, minGrad, maxGrad,
		   bitmask);

  m_roi[name] = bitmask;
  
  minD = ds;
  minW = ws;
  minH = hs;
  maxD = de;
  maxW = we;
  maxH = he;

  QMessageBox::information(0, "Save To ROI", "Done");
  return true;
}

QString
VolumeOperations::getROIName()
{
  QString mask;
  
  QStringList records = m_roi.keys();

  bool ok;
  mask = QInputDialog::getItem(0,
			       "ROI Operation",
			       "Select ROI to use",
			       records,
			       0,
			       false,
			       &ok);
  if (!ok)
    return QString();

  return mask;
}

void
VolumeOperations::deleteROI()
{
  QStringList records = m_roi.keys();

  QString roiName = getROIName();
  if (roiName.isEmpty())
    return;
  
  int rid = records.indexOf(roiName);
  if (rid < 0)
    {
      QMessageBox::information(0, "ROI Operation Error", "Cannot find ROI : "+roiName);
      return;
    }
  
  m_roi.remove(roiName);

  QMessageBox::information(0, "ROI Delete", "Done");	
}

bool
VolumeOperations::roiOperation(Vec bmin, Vec bmax,
			       int tag,
			       int& minD, int& maxD,
			       int& minW, int& maxW,
			       int& minH, int& maxH,
			       int gradType, float minGrad, float maxGrad)
{
  minD = maxD = minW = maxW = minH = maxH = -1;

  int ds = qMax(0, qFloor(bmin.z));
  int ws = qMax(0, qFloor(bmin.y));
  int hs = qMax(0, qFloor(bmin.x));

  int de = qMin(qCeil(bmax.z), m_depth-1);
  int we = qMin(qCeil(bmax.y), m_width-1);
  int he = qMin(qCeil(bmax.x), m_height-1);

  qint64 mx = he-hs+1;
  qint64 my = we-ws+1;
  qint64 mz = de-ds+1;


  qint64 msize;
  msize = mx*my*mz;
  
  MyBitArray roibitmask;
  MyBitArray tagbitmask;
  int opType = 0;
  int resultTag = -1;
  
  roibitmask.resize(msize);
  tagbitmask.resize(msize);

  roibitmask.fill(false);
  tagbitmask.fill(false);

  //--------------------
  // select mask to operate on
  {  
    QStringList records = m_roi.keys();
    QString roiName = getROIName();
    if (roiName.isEmpty())
      return false;
    
    int rid = records.indexOf(roiName);
    if (rid < 0)
      {
	QMessageBox::information(0, "ROI Operation Error", "Cannot find ROI : "+roiName);
	return false;
      }

    roibitmask = m_roi[roiName];
  }
  //--------------------

  
  //--------------------
  // select operation type
  {
    QStringList dtypes;
    dtypes << "intersection"
	   << "union"
	   << "ROI - label"
	   << "label - ROI"
	   << "connected to ROI";
    
    bool ok;
    QString option = QInputDialog::getItem(0,
					   "ROI Operation",
					   "Select operation type",
					   dtypes,
					   0,
					   false,
					   &ok);
    
    if (ok)
      {
	if (option == "intersection") opType = 0;
	if (option == "union") opType = 1;
	if (option == "ROI - label") opType = 2;
	if (option == "label - ROI") opType = 3;
	if (option == "connected to ROI") opType = 4;
      }
    else
      {
	QMessageBox::information(0, "ROI Operation", "Operation not performed");	
	return false;
      }	
  }
  //--------------------

  {
    bool ok;
    resultTag = QInputDialog::getInt(0,
				     "Save result to label",
				     "Label (0-65535) to save the result\n.",
				     0, 0, 65535, 1,
				     &ok);
    
    if (!ok)
      return false;
  }

  QProgressDialog progress("ROI Operation",
			   QString(),
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);  
  qApp->processEvents();

  
  
  //--------------------
  // get tag visibility
  getVisibleRegion(ds, ws, hs,
		   de, we, he,
		   tag, false,
		   gradType, minGrad, maxGrad,
		   tagbitmask);
  //--------------------
  
  progress.setValue(50);
  qApp->processEvents();

  
  //--------------------
  if (opType == 0) // intersection
    tagbitmask &= roibitmask;

  if (opType == 1) // union
    tagbitmask |= roibitmask;

  if (opType == 2) // mask - label 
    {
      tagbitmask = ~tagbitmask;
      tagbitmask &= roibitmask;
    }
  
  if (opType == 3) // mask - label
    {
      roibitmask = ~roibitmask;
      tagbitmask &= roibitmask;
    }

  if (opType == 4) // get labeled region that is connected to ROI
    {
      getRegionConnectedToROI(ds, ws, hs,
			      de, we, he,
			      tagbitmask,
			      roibitmask);
    }
  //--------------------

  
  progress.setValue(90);
  qApp->processEvents();


  //--------------------
  // update mask
  for(qint64 d2=ds; d2<=de; d2++)
  for(qint64 w2=ws; w2<=we; w2++)
  for(qint64 h2=hs; h2<=he; h2++)
    {      
      qint64 bidx = (d2-ds)*mx*my+(w2-ws)*mx+(h2-hs);
      if (tagbitmask.testBit(bidx))
	{
	  qint64 idx = d2*m_width*m_height + w2*m_height + h2;
	  m_maskDataUS[idx] = resultTag;
	}
    }
  //--------------------

  progress.setValue(100);
  
  minD = ds;
  minW = ws;
  minH = hs;
  maxD = de;
  maxW = we;
  maxH = he;

  QMessageBox::information(0, "ROI Operation", "Done");	
  return true;
}


void
VolumeOperations::genVisibilityMap(int gradType, float minGrad, float maxGrad)
{
  m_visibilityMap.fill(false);
  getVisibleRegion(0, 0, 0,
		   m_depth-1, m_width-1, m_height-1,
		   -1, false,
		   gradType, minGrad, maxGrad,
		   m_visibilityMap);
}

QList<Vec> VolumeOperations::m_cPos;
QList<Vec> VolumeOperations::m_cNorm;
void VolumeOperations::setClip(QList<Vec> cpos, QList<Vec> cnorm)
{
  m_cPos = cpos;
  m_cNorm = cnorm;
}

float
VolumeOperations::calcGrad(int gradType,
			   qint64 d2, qint64 w2, qint64 h2,
			   int depth, int width, int height,
			   uchar *volData,
			   ushort *volDataUS)
{
  float gradMag;
  if (gradType == 0)
    {
      float gx,gy,gz;
      qint64 d3 = qBound(0, (int)d2+1, depth-1);
      qint64 d4 = qBound(0, (int)d2-1, depth-1);
      qint64 w3 = qBound(0, (int)w2+1, width-1);
      qint64 w4 = qBound(0, (int)w2-1, width-1);
      qint64 h3 = qBound(0, (int)h2+1, height-1);
      qint64 h4 = qBound(0, (int)h2-1, height-1);
      if (!volDataUS)
	{
	  gz = (volData[d3*width*height + w2*height + h2] -
		volData[d4*width*height + w2*height + h2]);
	  gy = (volData[d2*width*height + w3*height + h2] -
		volData[d2*width*height + w4*height + h2]);
	  gx = (volData[d2*width*height + w2*height + h3] -
		volData[d2*width*height + w2*height + h4]);
	  gx/=255.0;
	  gy/=255.0;
	  gz/=255.0;
	}
      else
	{
	  gz = (volDataUS[d3*width*height + w2*height + h2] -
		volDataUS[d4*width*height + w2*height + h2]);
	  gy = (volDataUS[d2*width*height + w3*height + h2] -
		volDataUS[d2*width*height + w4*height + h2]);
	  gx = (volDataUS[d2*width*height + w2*height + h3] -
		volDataUS[d2*width*height + w2*height + h4]);
	  gx/=65535.0;
	  gy/=65535.0;
	  gz/=65535.0;
	}
      
      Vec dv = Vec(gx, gy, gz); // surface gradient
      gradMag = dv.norm();
    } // gradType == 0
  else if (gradType == 1)  // Sobel
    {
      float h[9] = {1,2,1, 2,4,2, 1,2,1};
      if (!volDataUS)
	{
	  float vx = 0;
	  float vy = 0;
	  float vz = 0;
	  int k = -1;
	  for(int b=-1; b<=1; b++)
	    for(int c=-1; c<=1; c++)
	      {
		k++;
		{
		  qint64 a0 = qBound(0, (int)d2-1, depth-1);
		  qint64 a1 = qBound(0, (int)d2+1, depth-1);
		  qint64 b0 = qBound(0, (int)w2+b, width-1);
		  qint64 c0 = qBound(0, (int)h2+c, height-1);
		  
		  vx -= h[k]*volData[a0*width*height + b0*height + c0];
		  vx += h[k]*volData[a1*width*height + b0*height + c0];
		}
		
		{
		  qint64 a0 = qBound(0, (int)d2+b, depth-1);
		  qint64 b0 = qBound(0, (int)w2-1, width-1);
		  qint64 b1 = qBound(0, (int)w2+1, width-1);
		  qint64 c0 = qBound(0, (int)h2+c, height-1);
		  
		  vy -= h[k]*volData[a0*width*height + b0*height + c0];
		  vy += h[k]*volData[a0*width*height + b1*height + c0];
		}
		
		{
		  qint64 a0 = qBound(0, (int)d2+b, depth-1);
		  qint64 b0 = qBound(0, (int)w2+c, width-1);
		  qint64 c0 = qBound(0, (int)h2-1, height-1);
		  qint64 c1 = qBound(0, (int)h2+1, height-1);
		  
		  vz -= h[k]*volData[a0*width*height + b0*height + c0];
		  vz += h[k]*volData[a0*width*height + b0*height + c1];
		}
	      }
	  
	  Vec dv = Vec(vx, vy, vz)/255.0; // surface gradient
	  gradMag = dv.norm();
	}
      else
	{
	  float vx = 0;
	  float vy = 0;
	  float vz = 0;
	  int k = -1;
	  for(int b=-1; b<=1; b++)
	    for(int c=-1; c<=1; c++)
	      {
		k++;
		{
		  qint64 a0 = qBound(0, (int)d2-1, depth-1);
		  qint64 a1 = qBound(0, (int)d2+1, depth-1);
		  qint64 b0 = qBound(0, (int)w2+b, width-1);
		  qint64 c0 = qBound(0, (int)h2+c, height-1);
		  
		  vx -= h[k]*volDataUS[a0*width*height + b0*height + c0];
		  vx += h[k]*volDataUS[a1*width*height + b0*height + c0];
		}
		
		{
		  qint64 a0 = qBound(0, (int)d2+b, depth-1);
		  qint64 b0 = qBound(0, (int)w2-1, width-1);
		  qint64 b1 = qBound(0, (int)w2+1, width-1);
		  qint64 c0 = qBound(0, (int)h2+c, height-1);
		  
		  vy -= h[k]*volDataUS[a0*width*height + b0*height + c0];
		  vy += h[k]*volDataUS[a0*width*height + b1*height + c0];
		}
		
		{
		  qint64 a0 = qBound(0, (int)d2+b, depth-1);
		  qint64 b0 = qBound(0, (int)w2+c, width-1);
		  qint64 c0 = qBound(0, (int)h2-1, height-1);
		  qint64 c1 = qBound(0, (int)h2+1, height-1);
		  
		  vz -= h[k]*volDataUS[a0*width*height + b0*height + c0];
		  vz += h[k]*volDataUS[a0*width*height + b0*height + c1];
		}
	      }
	  
	  Vec dv = Vec(vx, vy, vz)/65535.0; // surface gradient
	  gradMag = dv.norm();
	}
    }
  else if (gradType == 2)  // Laplacian
    {
      float h[27] = {2,3,2, 3,6,3, 2,3,2,   3,6,3, 6,-88,6, 3,6,3,   2,3,2, 3,6,3, 2,3,2};
      float sum = 0;
      int k = -1;
      for(int a=-1; a<=1; a++)
	for(int b=-1; b<=1; b++)
	  for(int c=-1; c<=1; c++)
	    {
	      qint64 a0 = qBound(0, (int)d2+a, depth-1);
	      qint64 b0 = qBound(0, (int)w2+b, width-1);
	      qint64 c0 = qBound(0, (int)h2+c, height-1);
	      k++;
	      if (!volDataUS)
		sum += h[k]*volData[a0*width*height + b0*height + c0];
	      else
		sum += h[k]*volDataUS[a0*width*height + b0*height + c0];
	    }
      
      gradMag = sum/26.0;
      if (!volDataUS)
	gradMag /= 255.0;
      else
	gradMag /= 65535.0;

      gradMag += 0.5;
    }
  
  
  gradMag = qBound(0.0f, gradMag, 1.0f);

  return gradMag;
}

bool
VolumeOperations::checkClipped(Vec p0)
{  
  bool clipped = GeometryObjects::clipplanes()->checkClipped(p0);
  clipped |= GeometryObjects::crops()->checkCrop(p0);

  return clipped;
}


QList<Vec>
VolumeOperations::getSurfaceVoxels(qint64 mx, qint64 my, qint64 mz,
				   MyBitArray &bitmask)
{
  QList<Vec> svox;
  for(int d=0; d<mz; d++)
  for(int w=0; w<my; w++)
  for(int h=0; h<mx; h++)
    {
      qint64 bidx = d*mx*my + w*mx + h;
      if (bitmask.testBit(bidx))
	{
	  qint64 d2s = qBound(0, (int)d-1, (int)mz-1);
	  qint64 w2s = qBound(0, (int)w-1, (int)my-1);
	  qint64 h2s = qBound(0, (int)h-1, (int)mx-1);
	  qint64 d2e = qBound(0, (int)d+1, (int)mz-1);
	  qint64 w2e = qBound(0, (int)w+1, (int)my-1);
	  qint64 h2e = qBound(0, (int)h+1, (int)mx-1);
	  
	  bool ok = true;
	  for(qint64 d2=d2s; ok && d2<=d2e; d2++)
	  for(qint64 w2=w2s; ok && w2<=w2e; w2++)
	  for(qint64 h2=h2s; ok && h2<=h2e; h2++)
	    {
	      qint64 idx = d2*mx*my + w2*mx + h2;
	      if (!bitmask.testBit(idx))
		{
		  ok = false;
		  svox << Vec(d, w, h);
		  break;
		}
	    }
	}
    }

  return svox;
}


//---------//---------//---------//
//---------//---------//---------//
void
VolumeOperations::resetT(int ds, int ws, int hs,
			 int de, int we, int he,
			 int tag)
{
  GeometryObjects::crops()->collectCropInfoBeforeCheckCropped();
  
  QProgressDialog progress("Identifying visible region",
			   QString(),
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);

  // collect stuff for parallel processing
  QList<QList<QVariant>> param;
  for(int d2=ds; d2<=de; d2++)
    {
      QList<QVariant> plist;
      plist << QVariant(ds);
      plist << QVariant(de);
      plist << QVariant(ws);
      plist << QVariant(we);
      plist << QVariant(hs);
      plist << QVariant(he);
      plist << QVariant(d2);
      plist << QVariant(tag);
      //plist << QVariant::fromValue(static_cast<void*>(m_maskData));
      
      param << plist;
    }

  int nThreads = qMax(1, (int)(QThread::idealThreadCount()));
  //QThreadPool::globalInstance()->setMaxThreadCount(nThreads);
						   
  // Create a QFutureWatcher and connect signals and slots.
  progress.setLabelText(QString("Identifying visible region using %1 thread(s)...").arg(nThreads));
  QFutureWatcher<void> futureWatcher;
  QObject::connect(&futureWatcher, &QFutureWatcher<void>::finished, &progress, &QProgressDialog::reset);
  QObject::connect(&progress, &QProgressDialog::canceled, &futureWatcher, &QFutureWatcher<void>::cancel);
  QObject::connect(&futureWatcher,  &QFutureWatcher<void>::progressRangeChanged, &progress, &QProgressDialog::setRange);
  QObject::connect(&futureWatcher, &QFutureWatcher<void>::progressValueChanged,  &progress, &QProgressDialog::setValue);
  
  // Start generation of isosurface for all values within the range
  futureWatcher.setFuture(QtConcurrent::map(param, VolumeOperations::parResetTag));
  
  // Display the dialog and start the event loop.
  progress.exec();
  
  futureWatcher.waitForFinished();
}

void
VolumeOperations::parResetTag(QList<QVariant> plist)
{
  int ds = plist[0].toInt();
  int de = plist[1].toInt();
  int ws = plist[2].toInt();
  int we = plist[3].toInt();
  int hs = plist[4].toInt();
  int he = plist[5].toInt();
  qint64 d2 = plist[6].toInt();
  int tag = plist[7].toInt();

  uchar *lut = Global::lut();

  qint64 mx = he-hs+1;
  qint64 my = we-ws+1;
  qint64 mz = de-ds+1;
  
  for(qint64 w2=ws; w2<=we; w2++)
    for(qint64 h2=hs; h2<=he; h2++)
      {
	bool clipped = VolumeOperations::checkClipped(Vec(h2, w2, d2));
	
	if (!clipped)
	  {      
	    qint64 idx = d2*m_width*m_height + w2*m_height + h2;
	    m_maskDataUS[idx] = tag;
	  }
      }
}
//---------//---------//---------//
//---------//---------//---------//


void
VolumeOperations::resetTag(Vec bmin, Vec bmax,
			   int tag,
			   int& minD, int& maxD,
			   int& minW, int& maxW,
			   int& minH, int& maxH)
{
  int ds = qMax(0, qFloor(bmin.z));
  int ws = qMax(0, qFloor(bmin.y));
  int hs = qMax(0, qFloor(bmin.x));

  int de = qMin(qCeil(bmax.z), m_depth-1);
  int we = qMin(qCeil(bmax.y), m_width-1);
  int he = qMin(qCeil(bmax.x), m_height-1);


  resetT(ds, ws, hs,
	 de, we, he,
	 tag);
  minD = 0;
  minW = 0;
  minH = 0;
  maxD = m_depth;
  maxW = m_width;
  maxH = m_height;
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

  int ds = qMax(0, qFloor(bmin.z));
  int ws = qMax(0, qFloor(bmin.y));
  int hs = qMax(0, qFloor(bmin.x));

  int de = qMin(qCeil(bmax.z), m_depth-1);
  int we = qMin(qCeil(bmax.y), m_width-1);
  int he = qMin(qCeil(bmax.x), m_height-1);

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
	      m_maskDataUS[idx] = tag;
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



//---------//---------//---------//
//---------//---------//---------//
void
VolumeOperations::getVisibleRegion(int ds, int ws, int hs,
				   int de, int we, int he,
				   int tag, bool checkZero,
				   int gradType, float minGrad, float maxGrad,
				   MyBitArray& cbitmask,
				   bool showProgress)
{
  GeometryObjects::crops()->collectCropInfoBeforeCheckCropped();

  QProgressDialog progress;

  if (showProgress)
    {
      progress.setLabelText("Identifying visible region");
      progress.setCancelButton(NULL);
      progress.setWindowFlags(Qt::WindowStaysOnTopHint);
      progress.setMinimumDuration(0);
    }

  // collect stuff for parallel processing
  QList<QList<QVariant>> param;
  for(int d2=ds; d2<=de; d2++)
    {
      QList<QVariant> plist;
      plist << QVariant(ds);
      plist << QVariant(de);
      plist << QVariant(ws);
      plist << QVariant(we);
      plist << QVariant(hs);
      plist << QVariant(he);
      plist << QVariant(d2);
      plist << QVariant(gradType);
      plist << QVariant(minGrad);
      plist << QVariant(maxGrad);
      plist << QVariant(tag);
      plist << QVariant(checkZero);
      plist << QVariant::fromValue(static_cast<void*>(&cbitmask));
      
      param << plist;
    }

  int nThreads = qMax(1, (int)(QThread::idealThreadCount()));
  //QThreadPool::globalInstance()->setMaxThreadCount(nThreads);

  // Create a QFutureWatcher and connect signals and slots.
  QFutureWatcher<void> futureWatcher;

  if (showProgress)
    {
      progress.setLabelText(QString("Identifying visible region using %1 thread(s)...").arg(nThreads));
      QObject::connect(&futureWatcher, &QFutureWatcher<void>::finished, &progress, &QProgressDialog::reset);
      QObject::connect(&progress, &QProgressDialog::canceled, &futureWatcher, &QFutureWatcher<void>::cancel);
      QObject::connect(&futureWatcher,  &QFutureWatcher<void>::progressRangeChanged, &progress, &QProgressDialog::setRange);
      QObject::connect(&futureWatcher, &QFutureWatcher<void>::progressValueChanged,  &progress, &QProgressDialog::setValue);
    }
  
  // Start generation for all values within the range
  futureWatcher.setFuture(QtConcurrent::map(param, VolumeOperations::parVisibleRegionGeneration));
  
  if (showProgress)
    {
      // Display the dialog and start the event loop.
      progress.exec();
    }
  
  futureWatcher.waitForFinished();
}

void
VolumeOperations::parVisibleRegionGeneration(QList<QVariant> plist)
{
  int ds = plist[0].toInt();
  int de = plist[1].toInt();
  int ws = plist[2].toInt();
  int we = plist[3].toInt();
  int hs = plist[4].toInt();
  int he = plist[5].toInt();
  qint64 d2 = plist[6].toInt();
  int gradType = plist[7].toInt();
  float minGrad = plist[8].toFloat();
  float maxGrad = plist[9].toFloat();
  int tag = plist[10].toInt();
  bool checkZero = plist[11].toBool();
  MyBitArray *cbitmask = static_cast<MyBitArray*>(plist[12].value<void*>());

  uchar *lut = Global::lut();

  qint64 mx = he-hs+1;
  qint64 my = we-ws+1;
  qint64 mz = de-ds+1;
  
  for(qint64 w2=ws; w2<=we; w2++)
    for(qint64 h2=hs; h2<=he; h2++)
      {
	bool clipped = VolumeOperations::checkClipped(Vec(h2, w2, d2));
	
	bool opaque = false;
	
	if (!clipped)
	  {      
	    qint64 idx = d2*m_width*m_height + w2*m_height + h2;
	    int val = m_volData[idx];
	    if (m_volDataUS) val = m_volDataUS[idx];

	    int mtag = m_maskDataUS[idx];

	    opaque =  (lut[4*val+3]*Global::tagColors()[4*mtag+3] > 0);      
	    
	    //-------
	    if (opaque &&
		(minGrad > 0.0 || maxGrad < 1.0))
	      {
		float gradMag = VolumeOperations::calcGrad(gradType, d2, w2, h2,
							   m_depth, m_width, m_height,
							   m_volData, m_volDataUS);
		
		if (gradMag < minGrad || gradMag > maxGrad)
		  opaque = false;
	      }
	    //-------      
	    
	    if (tag > -1)
	      {
		if (checkZero)
		  opaque &= (mtag == 0 || mtag == tag);
		else
		  opaque &= (mtag == tag);
	      }
	  }
	
	if (!clipped && opaque)
	  {
	    qint64 bidx = (d2-ds)*mx*my+(w2-ws)*mx+(h2-hs);
	    cbitmask->setBit(bidx, true);
	  }  // visible voxel
      }
}
//---------//---------//---------//
//---------//---------//---------//



//---------//---------//---------//
//---------//---------//---------//
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

  int ds = qMax(0, qFloor(bmin.z));
  int ws = qMax(0, qFloor(bmin.y));
  int hs = qMax(0, qFloor(bmin.x));

  int de = qMin(qCeil(bmax.z), m_depth-1);
  int we = qMin(qCeil(bmax.y), m_width-1);
  int he = qMin(qCeil(bmax.x), m_height-1);

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
	  m_maskDataUS[idx] = tag;
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
				     int tag, bool checkZero,
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

  int indices[] = {-1, 0, 0,
		    1, 0, 0,
		    0,-1, 0,
		    0, 1, 0,
		    0, 0,-1,
		    0, 0, 1};

  cbitmask.fill(false);

  getVisibleRegion(ds, ws, hs,
		   de, we, he,
		   tag, checkZero,
		   gradType, minGrad, maxGrad,
		   cbitmask);

    
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
//---------//---------//---------//
//---------//---------//---------//



//---------//---------//---------//
//---------//---------//---------//
void
VolumeOperations::getRegionConnectedToROI(int ds, int ws, int hs,
					  int de, int we, int he,
					  MyBitArray& tagbitmask,
					  MyBitArray& maskbitmask)
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

  int indices[] = {-1, 0, 0,
		    1, 0, 0,
		    0,-1, 0,
		    0, 1, 0,
		    0, 0,-1,
		    0, 0, 1};

    
  MyBitArray bitmask;
  bitmask.resize(mx*my*mz);
  bitmask.fill(false);

  QList<Vec> seeds;
  seeds.clear();
  for(qint64 d2=ds; d2<=de; d2++)
  for(qint64 w2=ws; w2<=we; w2++)
  for(qint64 h2=hs; h2<=he; h2++)
    {
      qint64 bidx = (d2-ds)*mx*my+(w2-ws)*mx+(h2-hs);
      if (maskbitmask.testBit(bidx))
	seeds << Vec(d2,w2,h2);
    }

  bitmask = maskbitmask;

  

  //------------------------------------------------------
  // dilate from seeds
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

      QList<Vec> tseeds;
      tseeds.clear();

      progress.setLabelText(QString("Flooding %1").arg(seeds.count()));
      qApp->processEvents();

            
      // find outer boundary to fill
      for(int e=0; e<seeds.count(); e++)
	{
	  int dx = seeds[e].x;
	  int wx = seeds[e].y;
	  int hx = seeds[e].z;
	  	  
	  for(int i=0; i<6; i++)
	    {
	      int da = indices[3*i+0];
	      int wa = indices[3*i+1];
	      int ha = indices[3*i+2];
	      
	      qint64 d2 = qBound(ds, dx+da, de);
	      qint64 w2 = qBound(ws, wx+wa, we);
	      qint64 h2 = qBound(hs, hx+ha, he);
	      
	      qint64 bidx = (d2-ds)*mx*my+(w2-ws)*mx+(h2-hs);
	      if (tagbitmask.testBit(bidx) &&
		  !bitmask.testBit(bidx))
		{
		  bitmask.setBit(bidx, true);
		  tseeds << Vec(d2,w2,h2);		  
		}
	    }
	}

      seeds.clear();

      if (tseeds.count() > 0)
	seeds = tseeds;
      else
	done = true;

      tseeds.clear();
    }
  //------------------------------------------------------

  tagbitmask &= bitmask;
}
//---------//---------//---------//
//---------//---------//---------//



//---------//---------//---------//
//---------//---------//---------//
void
VolumeOperations::getTransparentRegion(int ds, int ws, int hs,
				       int de, int we, int he,
				       MyBitArray& cbitmask,
				       int gradType, float minGrad, float maxGrad)
{
  GeometryObjects::crops()->collectCropInfoBeforeCheckCropped();

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

  QList<QList<QVariant>> param;
  for(qint64 d2=ds; d2<=de; d2++)
  {
      QList<QVariant> plist;
      plist << QVariant(ds);
      plist << QVariant(de);
      plist << QVariant(ws);
      plist << QVariant(we);
      plist << QVariant(hs);
      plist << QVariant(he);
      plist << QVariant(d2);
      plist << QVariant(gradType);
      plist << QVariant(minGrad);
      plist << QVariant(maxGrad);
      plist << QVariant(mx);
      plist << QVariant(my);
      plist << QVariant(mz);
      plist << QVariant::fromValue(static_cast<void*>(&cbitmask));      
      
      param << plist;
  }


  int nThreads = qMax(1, (int)(QThread::idealThreadCount()));
  //QThreadPool::globalInstance()->setMaxThreadCount(nThreads);

  // Create a QFutureWatcher and connect signals and slots.
  progress.setLabelText(QString("Identifying transparent region using %1 thread(s)...").arg(nThreads));
  QFutureWatcher<void> futureWatcher;
  QObject::connect(&futureWatcher, &QFutureWatcher<void>::finished, &progress, &QProgressDialog::reset);
  QObject::connect(&progress, &QProgressDialog::canceled, &futureWatcher, &QFutureWatcher<void>::cancel);
  QObject::connect(&futureWatcher,  &QFutureWatcher<void>::progressRangeChanged, &progress, &QProgressDialog::setRange);
  QObject::connect(&futureWatcher, &QFutureWatcher<void>::progressValueChanged,  &progress, &QProgressDialog::setValue);
  
  // Start generation of isosurface for all values within the range
  futureWatcher.setFuture(QtConcurrent::map(param, VolumeOperations::parTransparentRegionGeneration));
  
  // Display the dialog and start the event loop.
  progress.exec();
  
  futureWatcher.waitForFinished();
  
  
  progress.setValue(100);
}

void
VolumeOperations::parTransparentRegionGeneration(QList<QVariant> plist)
{
  int ds = plist[0].toInt();
  int de = plist[1].toInt();
  int ws = plist[2].toInt();
  int we = plist[3].toInt();
  int hs = plist[4].toInt();
  int he = plist[5].toInt();
  qint64 d2 = plist[6].toLongLong();
  int gradType = plist[7].toInt();
  float minGrad = plist[8].toFloat();
  float maxGrad = plist[9].toFloat();
  qint64 mx = plist[10].toLongLong();
  qint64 my = plist[11].toLongLong();
  qint64 mz = plist[12].toLongLong();
  MyBitArray *cbitmask = static_cast<MyBitArray*>(plist[13].value<void*>());
  uchar *lut = Global::lut();

  for(qint64 w2=ws; w2<=we; w2++)
    for(qint64 h2=hs; h2<=he; h2++)
    {
      bool clipped = VolumeOperations::checkClipped(Vec(h2, w2, d2));
      bool transparent = false;
      
      if (!clipped)
	{
	  qint64 idx = d2*m_width*m_height + w2*m_height + h2;

	  int val = m_volData[idx];
	  if (m_volDataUS) val = m_volDataUS[idx];

	  int mtag = m_maskDataUS[idx];

	  transparent =  (lut[4*val+3]*Global::tagColors()[4*mtag+3] == 0);

	  //-------
	  if (!transparent &&
	      (minGrad > 0.0 || maxGrad < 1.0))

	    {
	      float gradMag = VolumeOperations::calcGrad(gradType, d2, w2, h2,
							 m_depth, m_width, m_height,
							 m_volData, m_volDataUS);
	      
	      if (gradMag < minGrad || gradMag > maxGrad)
		transparent = true;
	    }
	  //-------
	}
      
      if (clipped || transparent)
	{
	  qint64 bidx = (d2-ds)*mx*my+(w2-ws)*mx+(h2-hs);
	  cbitmask->setBit(bidx, true);
	}  // transparent voxel
    }
}
//---------//---------//---------//
//---------//---------//---------//



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

  int ds = qMax(0, qFloor(bmin.z));
  int ws = qMax(0, qFloor(bmin.y));
  int hs = qMax(0, qFloor(bmin.x));

  int de = qMin(qCeil(bmax.z), m_depth-1);
  int we = qMin(qCeil(bmax.y), m_width-1);
  int he = qMin(qCeil(bmax.x), m_height-1);

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
  // cbitmask is true for transparent region
  // cbitmask is false for opaque region
  //----------------------------  
  

  QProgressDialog progress("Shrinkwrap",
			   QString(),
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);

  //----------------------------
  //----------------------------
  // apply open operation to transparent region
  // to fill the holes before shrinkwrapping the region
  if (holeSize > 0)
    {
      progress.setValue(50);
      qApp->processEvents();
      openCloseBitmask(holeSize, holeSize,		       
		       true, // open transparent region
		       mx, my, mz,
		       cbitmask); // cbitmask contains transparent region
      progress.setLabelText("Shrinkwrap - holes closed");
      progress.setValue(75);
      qApp->processEvents();
    }
  //----------------------------
  //----------------------------

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
  // set all side faces
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

      progress.setLabelText(QString("Shrinkwrap - Boundary detection %1").arg(edges.count()));
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
	  m_maskDataUS[idx] = tag;
	}
    }
  //----------------------------  

  minD = ds;  maxD = de;
  minW = ws;  maxW = we;
  minH = hs;  maxH = he;
}

void
VolumeOperations::openCloseBitmask(int offset1, int offset2,
				   bool htype,
				   qint64 mx, qint64 my, qint64 mz,
				   MyBitArray &bitmask)
{
  QProgressDialog progress(htype?"Open":"Close",
			   QString(),
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);
  qApp->processEvents();

  progress.setValue(10);
  qApp->processEvents();

  progress.setValue(25);
  qApp->processEvents();
  
  if (htype) // Open
    {
      _dilatebitmask(offset1, false, // erode
		     mx, my, mz,
		     bitmask);
      _dilatebitmask(offset2, true, // dilate
		     mx, my, mz,
		     bitmask);
    }
  else // Close
    {
      _dilatebitmask(offset1, true, // dilate
		     mx, my, mz,
		     bitmask);
      _dilatebitmask(offset2, false, // erode
		     mx, my, mz,
		     bitmask);
    }

  progress.setValue(100);
  qApp->processEvents();
}


void
VolumeOperations::_dilatebitmask(int nDilate, bool htype,
				 qint64 mx, qint64 my, qint64 mz,
				 MyBitArray &bitmask,
				 bool showProgress)
{
  // convert to vdb levelset and dilate
  QProgressDialog progress;

  if (showProgress)
    {
      progress.setLabelText(htype?"Dilate":"Erode");
      progress.setCancelButton(NULL);
      progress.setWindowFlags(Qt::WindowStaysOnTopHint);
      progress.setMinimumDuration(0);
      qApp->processEvents();
    }

  
  progress.setValue(50);
  qApp->processEvents();

  // generate squared distance transform
  float *dt = BinaryDistanceTransform::binaryEDTsq(bitmask,
						   mx, my, mz,
						   true);
  
  progress.setValue(75);
  qApp->processEvents();

  
  // check distance transform
  for(qint64 idx=0; idx<mx*my*mz; idx++)
    {
      //m_maskDataUS[idx] = qFloor(sqrt(dt[idx]));
      if (qFloor(sqrt(dt[idx])) <= nDilate)
	bitmask.setBit(idx, htype);
    }
  
  delete [] dt;
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
		    if (!inside)
		      break;
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
  int ds = qMax(0, qFloor(bmin.z));
  int ws = qMax(0, qFloor(bmin.y));
  int hs = qMax(0, qFloor(bmin.x));

  int de = qMin(qCeil(bmax.z), m_depth-1);
  int we = qMin(qCeil(bmax.y), m_width-1);
  int he = qMin(qCeil(bmax.x), m_height-1);

  qint64 mx = he-hs+1;
  qint64 my = we-ws+1;
  qint64 mz = de-ds+1;

  MyBitArray bitmask;
  bitmask.resize(mx*my*mz);
  bitmask.fill(false);
  getVisibleRegion(ds, ws, hs,
		   de, we, he,
		   -1, false,
		   gradType, minGrad, maxGrad,
		   bitmask);

  for(qint64 d=ds; d<=de; d++)
    for(qint64 w=ws; w<=we; w++)
      for(qint64 h=hs; h<=he; h++)
	{
	  qint64 idx = d*m_width*m_height + w*m_height + h;
	  qint64 bidx = (d-ds)*mx*my + (w-ws)*mx + (h-hs);
	  if (bitmask.testBit(bidx) == visible)
	    m_maskDataUS[idx] = tag;
	}
  minD = ds;
  maxD = de;
  minW = ws;
  maxW = we;
  minH = hs;
  maxH = he;
}

void
VolumeOperations::stepTags(Vec bmin, Vec bmax,
			   int tagStep, int tagVal,
			   int& minD, int& maxD,
			   int& minW, int& maxW,
			   int& minH, int& maxH)
  
{    
  int ds = qMax(0, qFloor(bmin.z));
  int ws = qMax(0, qFloor(bmin.y));
  int hs = qMax(0, qFloor(bmin.x));

  int de = qMin(qCeil(bmax.z), m_depth-1);
  int we = qMin(qCeil(bmax.y), m_width-1);
  int he = qMin(qCeil(bmax.x), m_height-1);

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
//
//	    clipped |= GeometryObjects::crops()->checkCrop(Vec(h,w,d));
	    
	    if (!clipped)
	      {
		qint64 idx = d*m_width*m_height + w*m_height + h;
		if (m_maskDataUS[idx] < tagStep)
		  m_maskDataUS[idx] = 0;
		else
		  m_maskDataUS[idx] = tagVal;		
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
  int ds = qMax(0, qFloor(bmin.z));
  int ws = qMax(0, qFloor(bmin.y));
  int hs = qMax(0, qFloor(bmin.x));

  int de = qMin(qCeil(bmax.z), m_depth-1);
  int we = qMin(qCeil(bmax.y), m_width-1);
  int he = qMin(qCeil(bmax.x), m_height-1);

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
		bool clipped = checkClipped(Vec(h, w, d));
		
		if (!clipped)
		  {
		    qint64 idx = d*m_width*m_height + w*m_height + h;
		    int mtag = m_maskDataUS[idx];
		    if (tag2 == -1 || mtag == tag2)
		      {
			int val = m_volData[idx];
			if (m_volDataUS) val = m_volDataUS[idx];
			int a =  lut[4*val+3];
			if (a > 0)
			  {
			    m_maskDataUS[idx] = tag1;

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
		bool clipped = checkClipped(Vec(h, w, d));
	    
		if (!clipped)
		  {
		    qint64 idx = d*m_width*m_height + w*m_height + h;
		    int mtag = m_maskDataUS[idx];
		    if (tag2 == -1 || mtag == tag2)
		      {
			m_maskDataUS[idx] = tag1;

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
VolumeOperations::dilateAllTags(Vec bmin, Vec bmax,
				int nDilate,
				int& minD, int& maxD,
				int& minW, int& maxW,
				int& minH, int& maxH,
				int gradType, float minGrad, float maxGrad)
{  
  int ds = qMax(0, (int)bmin.z);
  int ws = qMax(0, (int)bmin.y);
  int hs = qMax(0, (int)bmin.x);

  int de = qMin((int)bmax.z, m_depth-1);
  int we = qMin((int)bmax.y, m_width-1);
  int he = qMin((int)bmax.x, m_height-1);

  qint64 mx = he-hs+1;
  qint64 my = we-ws+1;
  qint64 mz = de-ds+1;

  QList<int> ut;
  for(qint64 d=ds; d<=de; d++)
    for(qint64 w=ws; w<=we; w++)
      for(qint64 h=hs; h<=he; h++)
	{
	  qint64 idx = ((qint64)d)*m_width*m_height + ((qint64)w)*m_height + h;
	  if (m_maskDataUS[idx] > 0 && !ut.contains(m_maskDataUS[idx]))
	    ut << m_maskDataUS[idx];
	}

  QMessageBox::information(0, "Labels", QString("Total labels : %1").arg(ut.count()));


  QProgressDialog progress("Growing all labels",
			   QString(),
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);
  qApp->processEvents();


  MyBitArray zbitmask;
  zbitmask.resize(mx*my*mz);
  getVisibleRegion(ds, ws, hs,
		   de, we, he,
		   -1, false,
		   gradType, minGrad, maxGrad,
		   zbitmask);


  MyBitArray bitmask;
  bitmask.resize(mx*my*mz);

  //----------------------------
  // identify tag regions
  QList<VdbVolume*> tagVdb;
  for(int i=0; i<ut.count(); i++)
    {
      progress.setValue(100*(float)i/(float)ut.count());
      qApp->processEvents();
      
      bitmask.fill(false);
      
      getVisibleRegion(ds, ws, hs,
		       de, we, he,
		       ut[i], false,  // no tag zero checking
		       gradType, minGrad, maxGrad,
		       bitmask,
		       false);  
      
      VdbVolume *vdb;
      vdb = new VdbVolume;
      openvdb::FloatGrid::Accessor accessor = vdb->getAccessor();
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
      
      tagVdb << vdb;
    }
  
  
  for(int u=0; u<nDilate; u++)
    {
      progress.setLabelText(QString("Growing : %1 of %2").arg(u+1).arg(nDilate));
      for(int i=0; i<ut.count(); i++)
	{
	  progress.setValue(100*(float)i/(float)ut.count());
	  qApp->processEvents();
	 
	  tagVdb[i]->convertToLevelSet(1, 0);
	  tagVdb[i]->offset(-1); // dilate

	  openvdb::FloatGrid::Accessor accessor = tagVdb[i]->getAccessor();
	  openvdb::Coord ijk;
	  int &d = ijk[0];
	  int &w = ijk[1];
	  int &h = ijk[2];
	  for(d=0; d<mz; d++)
	    for(w=0; w<my; w++)
	      for(h=0; h<mx; h++)
		{
		  if (accessor.getValue(ijk) <= 0)
		    {
		      qint64 bidx = ((qint64)d)*mx*my+w*mx+h;
		      if (zbitmask.testBit(bidx))
			{
			  qint64 d2 = ds + d;
			  qint64 w2 = ws + w;
			  qint64 h2 = hs + h;
			  qint64 idx = d2*m_width*m_height + w2*m_height + h2;
			  if (m_maskDataUS[idx] == 0) // expand into unlabelled region
			    m_maskDataUS[idx] = ut[i];
			  else if (m_maskDataUS[idx] != ut[i]) // encroaching another label
			    accessor.setValue(ijk, 0);
			}
		      else // do not venture into invisible region
			accessor.setValue(ijk, 0);
		    }
		  
		}	  
	}
    }

  
  for(int i=0; i<ut.count(); i++)
    delete tagVdb[i];

  tagVdb.clear();

  minD = ds;
  maxD = de;
  minW = ws;
  maxW = we;
  minH = hs;
  maxH = he;
}


//void
//VolumeOperations::dilateAllTags(Vec bmin, Vec bmax,
//				int nDilate,
//				int& minD, int& maxD,
//				int& minW, int& maxW,
//				int& minH, int& maxH,
//				int gradType, float minGrad, float maxGrad)
//{
//  QProgressDialog progress("Dilating all labels",
//			   QString(),
//			   0, 100,
//			   0,
//			   Qt::WindowStaysOnTopHint);
//  progress.setMinimumDuration(0);
//
//  
//  int ds = qMax(0, (int)bmin.z);
//  int ws = qMax(0, (int)bmin.y);
//  int hs = qMax(0, (int)bmin.x);
//
//  int de = qMin((int)bmax.z, m_depth-1);
//  int we = qMin((int)bmax.y, m_width-1);
//  int he = qMin((int)bmax.x, m_height-1);
//
//  qint64 mx = he-hs+1;
//  qint64 my = we-ws+1;
//  qint64 mz = de-ds+1;
//
//  QList<int> ut;
//  for(qint64 d=ds; d<=de; d++)
//    for(qint64 w=ws; w<=we; w++)
//      for(qint64 h=hs; h<=he; h++)
//	{
//	  qint64 idx = ((qint64)d)*m_width*m_height + ((qint64)w)*m_height + h;
//	  if (m_maskData[idx] > 0 && !ut.contains(m_maskData[idx]))
//	    ut << m_maskData[idx]; 
//	}
//
//  QMessageBox::information(0, "Labels", QString("Total labels : %1").arg(ut.count()));
//  for(int u=0; u<nDilate; u++)
//    {
//      for(int i=0; i<ut.count(); i++)
//	{
//	  progress.setValue(100*(float)i/(float)ut.count());
//	  qApp->processEvents();
//	  dilateAll(bmin, bmax, ut[i],
//		    1,
//		    minD, maxD,
//		    minW, maxW,
//		    minH, maxH,
//		    false,
//		    gradType, minGrad, maxGrad,
//		    false);
//	}
//    }
//}

void
VolumeOperations::dilateAll(Vec bmin, Vec bmax, int tag,
			    int nDilate,
			    int& minD, int& maxD,
			    int& minW, int& maxW,
			    int& minH, int& maxH,
			    bool allVisible,
			    int gradType, float minGrad, float maxGrad,
			    bool showProgress)
{
  minD = minW = minH = maxD = maxW = maxH = -1;

  uchar *lut = Global::lut();

  QProgressDialog progress;

  if (showProgress)
    {
      progress.setLabelText("Updating voxel structure");
      progress.setCancelButton(NULL);
      progress.setWindowFlags(Qt::WindowStaysOnTopHint);
      progress.setMinimumDuration(0);
    }

  int ds = qMax(0, qFloor(bmin.z));
  int ws = qMax(0, qFloor(bmin.y));
  int hs = qMax(0, qFloor(bmin.x));

  int de = qMin(qCeil(bmax.z), m_depth-1);
  int we = qMin(qCeil(bmax.y), m_width-1);
  int he = qMin(qCeil(bmax.x), m_height-1);


  qint64 mx = he-hs+1;
  qint64 my = we-ws+1;
  qint64 mz = de-ds+1;

  MyBitArray bitmask;
  bitmask.resize(mx*my*mz);
  bitmask.fill(false);

  getVisibleRegion(ds, ws, hs,
		   de, we, he,
		   tag, false,
		   gradType, minGrad, maxGrad,
		   bitmask);
  

  
  if (showProgress)
    {
      progress.setLabelText("Dilate");
      qApp->processEvents();
    }



  _dilatebitmask(nDilate, true, // dilate opaque region
		 mx, my, mz,
		 bitmask,
		 showProgress);


  
  if (showProgress)
    {
      progress.setLabelText("writing to mask");
    }
  for(qint64 d2=ds; d2<=de; d2++)
    {
      if (showProgress)
	{
	  progress.setValue(100*(d2-ds)/(mz));
	  qApp->processEvents();
	}
      for(qint64 w2=ws; w2<=we; w2++)
	for(qint64 h2=hs; h2<=he; h2++)
	  {
	    qint64 bidx = (d2-ds)*mx*my+(w2-ws)*mx+(h2-hs);
	    if (bitmask.testBit(bidx))
	      {
		bool clipped = checkClipped(Vec(h2, w2, d2));
		
		if (!clipped)
		  {
		    qint64 idx = d2*m_width*m_height + w2*m_height + h2;
		    int val = m_volData[idx];
		    if (m_volDataUS) val = m_volDataUS[idx];
		    
		    int mtag = m_maskDataUS[idx];
		    
		    bool opaque =  (lut[4*val+3]*Global::tagColors()[4*mtag+3] > 0);
		    opaque &= (mtag == 0 || allVisible);
			  
		    //-------
		    if (opaque &&
			(minGrad > 0 || maxGrad < 1))
		      {
			float gradMag = VolumeOperations::calcGrad(gradType, d2, w2, h2,
								   m_depth, m_width, m_height,
								   m_volData, m_volDataUS);
	
			if (gradMag < minGrad || gradMag > maxGrad)
			  opaque = false;
		      }
		    //-------
		    
		    if (opaque) // dilate only in connected opaque region
		      m_maskDataUS[idx] = tag;
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


//---------//---------//---------//
//---------//---------//---------//
void
VolumeOperations::writeToMask(int ds, int ws, int hs,
			      int de, int we, int he,
			      int tag,
			      int gradType, float minGrad, float maxGrad,
			      bool allVisible,
			      MyBitArray& bitmask)
{
  GeometryObjects::crops()->collectCropInfoBeforeCheckCropped();

  QProgressDialog progress("Identifying visible region",
			   QString(),
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);

  // collect stuff for parallel processing
  QList<QList<QVariant>> param;
  for(int d2=ds; d2<=de; d2++)
    {
      QList<QVariant> plist;
      plist << QVariant(ds);
      plist << QVariant(de);
      plist << QVariant(ws);
      plist << QVariant(we);
      plist << QVariant(hs);
      plist << QVariant(he);
      plist << QVariant(d2);
      plist << QVariant(gradType);
      plist << QVariant(minGrad);
      plist << QVariant(maxGrad);
      plist << QVariant(tag);
      plist << QVariant(allVisible);
      plist << QVariant::fromValue(static_cast<void*>(&bitmask));
      
      param << plist;
    }

  int nThreads = qMax(1, (int)(QThread::idealThreadCount()));
  //QThreadPool::globalInstance()->setMaxThreadCount(nThreads);
						   
  // Create a QFutureWatcher and connect signals and slots.
  progress.setLabelText(QString("Identifying visible region using %1 thread(s)...").arg(nThreads));
  QFutureWatcher<void> futureWatcher;
  QObject::connect(&futureWatcher, &QFutureWatcher<void>::finished, &progress, &QProgressDialog::reset);
  QObject::connect(&progress, &QProgressDialog::canceled, &futureWatcher, &QFutureWatcher<void>::cancel);
  QObject::connect(&futureWatcher,  &QFutureWatcher<void>::progressRangeChanged, &progress, &QProgressDialog::setRange);
  QObject::connect(&futureWatcher, &QFutureWatcher<void>::progressValueChanged,  &progress, &QProgressDialog::setValue);
  
  // Start generation of isosurface for all values within the range
  futureWatcher.setFuture(QtConcurrent::map(param, VolumeOperations::parWriteToMask));
  
  // Display the dialog and start the event loop.
  progress.exec();
  
  futureWatcher.waitForFinished();
}

void
VolumeOperations::parWriteToMask(QList<QVariant> plist)
{
  int ds = plist[0].toInt();
  int de = plist[1].toInt();
  int ws = plist[2].toInt();
  int we = plist[3].toInt();
  int hs = plist[4].toInt();
  int he = plist[5].toInt();
  qint64 d2 = plist[6].toInt();
  int gradType = plist[7].toInt();
  float minGrad = plist[8].toFloat();
  float maxGrad = plist[9].toFloat();
  int tag = plist[10].toInt();
  bool allVisible = plist[11].toBool();
  MyBitArray *bitmask = static_cast<MyBitArray*>(plist[12].value<void*>());

  uchar *lut = Global::lut();

  qint64 mx = he-hs+1;
  qint64 my = we-ws+1;
  qint64 mz = de-ds+1;
  
  for(qint64 w2=ws; w2<=we; w2++)
    for(qint64 h2=hs; h2<=he; h2++)
      {
	qint64 bidx = (d2-ds)*mx*my+(w2-ws)*mx+(h2-hs);
	if (bitmask->testBit(bidx))
	      {
		bool clipped = VolumeOperations::checkClipped(Vec(h2, w2, d2));
		
		if (!clipped)
		  {      
		    qint64 idx = d2*m_width*m_height + w2*m_height + h2;
		    int val = m_volData[idx];
		    if (m_volDataUS) val = m_volDataUS[idx];

		    int mtag = m_maskDataUS[idx];

		    bool opaque =  (lut[4*val+3]*Global::tagColors()[4*mtag+3] > 0);
		    opaque &= (mtag == 0 || allVisible);
		    
		    //-------
		    if (opaque &&
			(minGrad > 0.0 || maxGrad < 1.0))
		      {
			float gradMag = VolumeOperations::calcGrad(gradType, d2, w2, h2,
								   m_depth, m_width, m_height,
								   m_volData, m_volDataUS);
			
			if (gradMag < minGrad || gradMag > maxGrad)
			  opaque = false;
		      }
		    //-------      
		
		    // grow only in zero or same tagged region
		    // or if tag is 0 then grow in all visible regions
		    if (opaque)
		      m_maskDataUS[idx] = tag;
		  } // if (!clipped)
	      } // test bitmask
      }
}
//---------//---------//---------//
//---------//---------//---------//


void
VolumeOperations::openAll(Vec bmin, Vec bmax, int tag,
			  int nErode, int nDilate,
			  int& minD, int& maxD,
			  int& minW, int& maxW,
			  int& minH, int& maxH,
			  int gradType, float minGrad, float maxGrad)
{
  minD = minW = minH = maxD = maxW = maxH = -1;

  uchar *lut = Global::lut();

  QProgressDialog progress("Open : Updating voxel structure",
			   QString(),
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);

  
  int ds = qMax(0, qFloor(bmin.z));
  int ws = qMax(0, qFloor(bmin.y));
  int hs = qMax(0, qFloor(bmin.x));

  int de = qMin(qCeil(bmax.z), m_depth-1);
  int we = qMin(qCeil(bmax.y), m_width-1);
  int he = qMin(qCeil(bmax.x), m_height-1);

  qint64 mx = he-hs+1;
  qint64 my = we-ws+1;
  qint64 mz = de-ds+1;

  MyBitArray bitmask;
  bitmask.resize(mx*my*mz);
  bitmask.fill(false);

  getVisibleRegion(ds, ws, hs,
		   de, we, he,
		   tag, false,
		   gradType, minGrad, maxGrad,
		   bitmask);
  

  // copy bitmask into cbitmask
  MyBitArray cbitmask;
  cbitmask = bitmask;


  openCloseBitmask(nErode, nDilate,
		   true, // open operation
		   mx, my, mz,
		   bitmask);
  
  
  progress.setLabelText("writing to mask");
  qApp->processEvents();
  for(qint64 d2=ds; d2<=de; d2++)
    {
      progress.setValue(100*(d2-ds)/(mz));
      qApp->processEvents();
      for(qint64 w2=ws; w2<=we; w2++)
	for(qint64 h2=hs; h2<=he; h2++)
	  {
	    qint64 bidx = (d2-ds)*mx*my+(w2-ws)*mx+(h2-hs);
	    if (!bitmask.testBit(bidx) && cbitmask.testBit(bidx))
	      {
		qint64 idx = d2*m_width*m_height + w2*m_height + h2;
		m_maskDataUS[idx] = 0;
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
VolumeOperations::closeAll(Vec bmin, Vec bmax, int tag,
			   int nDilate, int nErode,
			   int& minD, int& maxD,
			   int& minW, int& maxW,
			   int& minH, int& maxH,
			   int gradType, float minGrad, float maxGrad)
{
  minD = minW = minH = maxD = maxW = maxH = -1;

  uchar *lut = Global::lut();

  QProgressDialog progress("Close : Updating voxel structure",
			   QString(),
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);

  
  int ds = qMax(0, qFloor(bmin.z));
  int ws = qMax(0, qFloor(bmin.y));
  int hs = qMax(0, qFloor(bmin.x));

  int de = qMin(qCeil(bmax.z), m_depth-1);
  int we = qMin(qCeil(bmax.y), m_width-1);
  int he = qMin(qCeil(bmax.x), m_height-1);

  qint64 mx = he-hs+1;
  qint64 my = we-ws+1;
  qint64 mz = de-ds+1;

  MyBitArray bitmask;
  bitmask.resize(mx*my*mz);
  bitmask.fill(false);

  getVisibleRegion(ds, ws, hs,
		   de, we, he,
		   tag, false,
		   gradType, minGrad, maxGrad,
		   bitmask);
  
  // copy bitmask into cbitmask
  MyBitArray cbitmask;
  cbitmask = bitmask;
  
  openCloseBitmask(nDilate, nErode,
		   false, // close operation
		   mx, my, mz,
		   bitmask);
  

  
  progress.setLabelText("writing to mask");
  qApp->processEvents();
  for(qint64 d2=ds; d2<=de; d2++)
    {
      progress.setValue(100*(d2-ds)/(mz));
      qApp->processEvents();
      for(qint64 w2=ws; w2<=we; w2++)
	for(qint64 h2=hs; h2<=he; h2++)
	  {
	    qint64 bidx = (d2-ds)*mx*my+(w2-ws)*mx+(h2-hs);
	    if (bitmask.testBit(bidx) && !cbitmask.testBit(bidx))
	      {
		qint64 idx = d2*m_width*m_height + w2*m_height + h2;
		m_maskDataUS[idx] = tag;
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
VolumeOperations::dilateConnected(int dr, int wr, int hr,
				  Vec bmin, Vec bmax, int tag,
				  int nDilate,
				  int& minD, int& maxD,
				  int& minW, int& maxW,
				  int& minH, int& maxH,
				  bool allVisible,
				  int gradType, float minGrad, float maxGrad)
{
  minD = minW = minH = maxD = maxW = maxH = -1;


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

    int mtag = m_maskDataUS[idx];

    if (lut[4*val+3] == 0 || mtag != tag)
      {
	QMessageBox::information(0, "Dilate",
				 QString("Cannot dilate.\nYou are on voxel with tag %1, was expecting tag %2").arg(mtag).arg(tag));
	return;
      }
  }

  QProgressDialog progress("Updating voxel structure",
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


  //-------------------------------
  //-------------------------------
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

	      bool clipped = checkClipped(Vec(h2, w2, d2));

	      if (!clipped)
		{
		  qint64 idx = d2*m_width*m_height + w2*m_height + h2;
		  int val = m_volData[idx];
		  if (m_volDataUS) val = m_volDataUS[idx];
		    
		  int mtag = m_maskDataUS[idx];

		  bool opaque =  (lut[4*val+3]*Global::tagColors()[4*mtag+3] > 0);
		  opaque &= mtag == tag;

		  //-------
		  if (opaque &&
		      (minGrad > 0.0 || maxGrad < 1.0))
		    {
		      float gradMag = VolumeOperations::calcGrad(gradType, d2, w2, h2,
								 m_depth, m_width, m_height,
								 m_volData, m_volDataUS);
		      		      
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
  //-------------------------------
  //-------------------------------

  
  progress.setLabelText("Dilate");
  qApp->processEvents();




  _dilatebitmask(nDilate, true, // dilate opaque region
		 mx, my, mz,
		 bitmask);
  


  writeToMask(ds, ws, hs,
	      de, we, he,
	      tag,
	      gradType, minGrad, maxGrad,
	      allVisible,
	      bitmask);


  minD = ds;
  maxD = de;
  minW = ws;
  maxW = we;
  minH = hs;
  maxH = he;

  return;
}

void
VolumeOperations::erodeAll(Vec bmin, Vec bmax,
			   int tag, int tag2,
			   int nErode,
			   int& minD, int& maxD,
			   int& minW, int& maxW,
			   int& minH, int& maxH,
			   int gradType, float minGrad, float maxGrad)
{
  minD = minW = minH = maxD = maxW = maxH = -1;

  uchar *lut = Global::lut();
    
  QProgressDialog progress("Updating voxel structure",
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

  MyBitArray bitmask;
  bitmask.resize(mx*my*mz);
  bitmask.fill(false);

  getVisibleRegion(ds, ws, hs,
		   de, we, he,
		   tag, false,
		   gradType, minGrad, maxGrad,
		   bitmask);


  
  progress.setLabelText("Erode");
  qApp->processEvents();


  

  //========================

  // copy bitmask into cbitmask
  MyBitArray cbitmask;
  cbitmask = bitmask;

  _dilatebitmask(nErode, false, // dilate transparent region
		 mx, my, mz,
		 bitmask);

  
  progress.setLabelText("writing to mask");
  for(qint64 d2=ds; d2<=de; d2++)
    {
      progress.setValue(100*(d2-ds)/(mz));
      qApp->processEvents();
      for(qint64 w2=ws; w2<=we; w2++)
	for(qint64 h2=hs; h2<=he; h2++)
	  {
	    qint64 bidx = (d2-ds)*mx*my+(w2-ws)*mx+(h2-hs);
	    if (!bitmask.testBit(bidx) && cbitmask.testBit(bidx))
	      {
		qint64 idx = d2*m_width*m_height + w2*m_height + h2;
		m_maskDataUS[idx] = tag2;
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
VolumeOperations::erodeConnected(int dr, int wr, int hr,
				 Vec bmin, Vec bmax, int tag,
				 int nErode,
				 int& minD, int& maxD,
				 int& minW, int& maxW,
				 int& minH, int& maxH,
				 int gradType, float minGrad, float maxGrad)
{
  minD = minW = minH = maxD = maxW = maxH = -1;


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
		    
    int mtag = m_maskDataUS[idx];
		    
    if (lut[4*val+3] == 0 || mtag != tag)
      {
	QMessageBox::information(0, "Erode",
				 QString("Cannot erode.\nYou are on voxel with tag %1, was expecting tag %2").arg(mtag).arg(tag));
	return;
      }
  }

    
  QProgressDialog progress("Updating voxel structure",
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
	      qint64 idx = d2*m_width*m_height + w2*m_height + h2;
	      int val = m_volData[idx];
	      if (m_volDataUS) val = m_volDataUS[idx];

	      //-------
	      bool opaque = true;

	      bool clipped = checkClipped(Vec(h2, w2, d2));
	      
	      if (!clipped &&
		  (minGrad > 0.0 || maxGrad < 1.0))
		{
		  float gradMag = VolumeOperations::calcGrad(gradType, d2, w2, h2,
							     m_depth, m_width, m_height,
							     m_volData, m_volDataUS);
		  
		  if (gradMag < minGrad || gradMag > maxGrad)
		    opaque = false;
		}
	      //-------
	      
	      int mtag = m_maskDataUS[idx];
	      if (opaque && lut[4*val+3] > 0 && mtag == tag)
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

  _dilatebitmask(nErode, false, // dilate transparent region
		 mx, my, mz,
		 bitmask);


  
  progress.setLabelText("writing to mask");
  for(qint64 d2=ds; d2<=de; d2++)
    {
      progress.setValue(100*(d2-ds)/(mz));
      qApp->processEvents();
      for(qint64 w2=ws; w2<=we; w2++)
	for(qint64 h2=hs; h2<=he; h2++)
	  {
	    qint64 bidx = (d2-ds)*mx*my+(w2-ws)*mx+(h2-hs);
	    if (!bitmask.testBit(bidx) && cbitmask.testBit(bidx))
	      {
		qint64 idx = d2*m_width*m_height + w2*m_height + h2;
		m_maskDataUS[idx] = 0;
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
  int ds = qMax(0, qFloor(bmin.z));
  int ws = qMax(0, qFloor(bmin.y));
  int hs = qMax(0, qFloor(bmin.x));

  int de = qMin(qCeil(bmax.z), m_depth-1);
  int we = qMin(qCeil(bmax.y), m_width-1);
  int he = qMin(qCeil(bmax.x), m_height-1);

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
	    bool clipped = checkClipped(Vec(h, w, d));
	    
	    qint64 idx = d*m_width*m_height + w*m_height + h;
	    int vox = m_volData[idx];
	    if (m_volDataUS) vox = m_volDataUS[idx];
		    
	    int mtag = m_maskDataUS[idx];

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

  int ds = qMax(0, qFloor(bmin.z));
  int ws = qMax(0, qFloor(bmin.y));
  int hs = qMax(0, qFloor(bmin.x));

  int de = qMin(qCeil(bmax.z), m_depth-1);
  int we = qMin(qCeil(bmax.y), m_width-1);
  int he = qMin(qCeil(bmax.x), m_height-1);

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
	  m_maskDataUS[idx] = tag;
	}
    }
  //----------------------------  

  minD = ds;  maxD = de;
  minW = ws;  maxW = we;
  minH = hs;  maxH = he;
}




//---------//---------//---------//
//---------//---------//---------//
void
VolumeOperations::bakeC(int ds, int ws, int hs,
			int de, int we, int he,
			int tag,
			int gradType, float minGrad, float maxGrad,
			uchar *curveMask)
{
  GeometryObjects::crops()->collectCropInfoBeforeCheckCropped();

  QProgressDialog progress("Bake Curves",
			   QString(),
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);

  // collect stuff for parallel processing
  QList<QList<QVariant>> param;
  for(int d2=ds; d2<=de; d2++)
    {
      QList<QVariant> plist;
      plist << QVariant(ds);
      plist << QVariant(de);
      plist << QVariant(ws);
      plist << QVariant(we);
      plist << QVariant(hs);
      plist << QVariant(he);
      plist << QVariant(d2);
      plist << QVariant(gradType);
      plist << QVariant(minGrad);
      plist << QVariant(maxGrad);
      plist << QVariant(tag);
      plist << QVariant::fromValue(static_cast<void*>(curveMask));
      
      param << plist;
    }

  int nThreads = qMax(1, (int)(QThread::idealThreadCount()));
  //QThreadPool::globalInstance()->setMaxThreadCount(nThreads);
						   
  // Create a QFutureWatcher and connect signals and slots.
  progress.setLabelText(QString("Baking Curves using %1 thread(s)...").arg(nThreads));
  QFutureWatcher<void> futureWatcher;
  QObject::connect(&futureWatcher, &QFutureWatcher<void>::finished, &progress, &QProgressDialog::reset);
  QObject::connect(&progress, &QProgressDialog::canceled, &futureWatcher, &QFutureWatcher<void>::cancel);
  QObject::connect(&futureWatcher,  &QFutureWatcher<void>::progressRangeChanged, &progress, &QProgressDialog::setRange);
  QObject::connect(&futureWatcher, &QFutureWatcher<void>::progressValueChanged,  &progress, &QProgressDialog::setValue);
  
  // Start generation of isosurface for all values within the range
  futureWatcher.setFuture(QtConcurrent::map(param, VolumeOperations::parBakeCurves));
  
  // Display the dialog and start the event loop.
  progress.exec();
  
  futureWatcher.waitForFinished();
}

void
VolumeOperations::parBakeCurves(QList<QVariant> plist)
{
  int ds = plist[0].toInt();
  int de = plist[1].toInt();
  int ws = plist[2].toInt();
  int we = plist[3].toInt();
  int hs = plist[4].toInt();
  int he = plist[5].toInt();
  qint64 d2 = plist[6].toInt();
  int gradType = plist[7].toInt();
  float minGrad = plist[8].toFloat();
  float maxGrad = plist[9].toFloat();
  int tag = plist[10].toInt();
  uchar* curveMask = static_cast<uchar*>(plist[11].value<void*>());

  uchar *lut = Global::lut();

  qint64 mx = he-hs+1;
  qint64 my = we-ws+1;
  qint64 mz = de-ds+1;
  
  for(qint64 w2=ws; w2<=we; w2++)
    for(qint64 h2=hs; h2<=he; h2++)
      {
	bool clipped = VolumeOperations::checkClipped(Vec(h2, w2, d2));
	
	if (!clipped)
	  {      
	    uchar cm = curveMask[d2*mx*my + (w2-ws)*mx + (h2-hs)];
	    if (cm > 0)
	      {
		qint64 idx = d2*m_width*m_height + w2*m_height + h2;
		int val = m_volData[idx];
		if (m_volDataUS) val = m_volDataUS[idx];
		    
		int mtag = m_maskDataUS[idx];

		bool opaque =  (lut[4*val+3]*Global::tagColors()[4*mtag+3] > 0);      
	    
		if (opaque &&
		    (minGrad > 0.0 || maxGrad < 1.0))
		  {
		    float gradMag = VolumeOperations::calcGrad(gradType, d2, w2, h2,
							       m_depth, m_width, m_height,
							       m_volData, m_volDataUS);
		    
		    if (gradMag < minGrad || gradMag > maxGrad)
		      opaque = false;
		  }

		if (opaque)
		  m_maskDataUS[idx] = tag;
	      } // cm > 0
	  } // !clipped
      } // h2
}
//---------//---------//---------//
//---------//---------//---------//

void
VolumeOperations::bakeCurves(uchar *curveMask,
			     int minDSlice, int maxDSlice,
			     int minWSlice, int maxWSlice,
			     int minHSlice, int maxHSlice,
			     int tag,
			     int gradType, float minGrad, float maxGrad)
{
  bakeC(minDSlice, minWSlice, minHSlice,
	maxDSlice, maxWSlice, maxHSlice,
	tag,
	gradType, minGrad, maxGrad,
	curveMask);  
}


void
VolumeOperations::smoothConnectedRegion(int dr, int wr, int hr,
					Vec bmin, Vec bmax,
					int ctag,
					int& minD, int& maxD,
					int& minW, int& maxW,
					int& minH, int& maxH,
					int gradType, float minGrad, float maxGrad,
					int filterWidth)
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

  int ds = qMax(0, qFloor(bmin.z));
  int ws = qMax(0, qFloor(bmin.y));
  int hs = qMax(0, qFloor(bmin.x));

  int de = qMin(qCeil(bmax.z), m_depth-1);
  int we = qMin(qCeil(bmax.y), m_width-1);
  int he = qMin(qCeil(bmax.x), m_height-1);

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

  
  
  convertToVDBandSmooth(ds, ws, hs,
			de, we, he,
			bitmask,
			filterWidth);
  
  minD = ds;
  minW = ws;
  minH = hs;
  maxD = de;
  maxW = we;
  maxH = he;
}

void
VolumeOperations::smoothAllRegion(Vec bmin, Vec bmax,
				  int tag,
				  int& minD, int& maxD,
				  int& minW, int& maxW,
				  int& minH, int& maxH,
				  int gradType, float minGrad, float maxGrad,
				  int filterWidth)
{
  minD = maxD = minW = maxW = minH = maxH = -1;

  int ds = qMax(0, qFloor(bmin.z));
  int ws = qMax(0, qFloor(bmin.y));
  int hs = qMax(0, qFloor(bmin.x));

  int de = qMin(qCeil(bmax.z), m_depth-1);
  int we = qMin(qCeil(bmax.y), m_width-1);
  int he = qMin(qCeil(bmax.x), m_height-1);

  qint64 mx = he-hs+1;
  qint64 my = we-ws+1;
  qint64 mz = de-ds+1;

  MyBitArray bitmask;
  bitmask.resize(mx*my*mz);
  bitmask.fill(false);


  getVisibleRegion(ds, ws, hs,
		   de, we, he,
		   tag, false,
		   gradType, minGrad, maxGrad,
		   bitmask);

  
  convertToVDBandSmooth(ds, ws, hs,
			de, we, he,
			bitmask,
			filterWidth);

  minD = ds;
  minW = ws;
  minH = hs;
  maxD = de;
  maxW = we;
  maxH = he;
}


void
VolumeOperations::convertToVDBandSmooth(int ds, int ws, int hs,
					int de, int we, int he,
					MyBitArray &bitmask,
					int filterWidth)
{
  qint64 mx = he-hs+1;
  qint64 my = we-ws+1;
  qint64 mz = de-ds+1;

  QProgressDialog progress("Updating voxel structure",
			   QString(),
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);  
  qApp->processEvents();

  VdbVolume vdb;

  openvdb::FloatGrid::Accessor accessor = vdb.getAccessor();
  openvdb::Coord ijk;
  int &d = ijk[0];
  int &w = ijk[1];
  int &h = ijk[2];
  for(d=ds; d<=de; d++)
    for(w=ws; w<=we; w++)
      for(h=hs; h<=he; h++)
	{
	  qint64 bidx = ((qint64)(d-ds))*mx*my+(w-ws)*mx+(h-hs);
	  if (bitmask.testBit(bidx))
	    accessor.setValue(ijk, 255);
	}

  progress.setLabelText("Smoothing - converting to levelset");
  progress.setValue(10);
  qApp->processEvents();
  vdb.convertToLevelSet(128, 0);
  progress.setLabelText("Smooth - gaussian");
  progress.setValue(25);
  qApp->processEvents();
  vdb.gaussian(filterWidth); // one iteration of gaussian filter
  progress.setValue(50);
  qApp->processEvents();

  {
    openvdb::FloatGrid::Accessor accessor = vdb.getAccessor();
    openvdb::Coord ijk;
    int &d = ijk[0];
    int &w = ijk[1];
    int &h = ijk[2];
    for(d=ds; d<=de; d++)
      for(w=ws; w<=we; w++)
	for(h=hs; h<=he; h++)
	  {
	    if (accessor.getValue(ijk) > 2)
	      {
		qint64 bidx = ((qint64)(d-ds))*mx*my+(w-ws)*mx+(h-hs);
		if (bitmask.testBit(bidx))
		  {
		    qint64 idx = d*m_width*m_height + w*m_height + h;
		    m_maskDataUS[idx] = 0;
		  }
	      }
	  }
  }

  progress.setValue(100);
  qApp->processEvents();
}



void
VolumeOperations::removeComponents(Vec bmin, Vec bmax,
				   int tag,
				   int& minD, int& maxD,
				   int& minW, int& maxW,
				   int& minH, int& maxH,
				   int gradType, float minGrad, float maxGrad)
{
  minD = maxD = minW = maxW = minH = maxH = -1;

  int ds = qMax(0, qFloor(bmin.z));
  int ws = qMax(0, qFloor(bmin.y));
  int hs = qMax(0, qFloor(bmin.x));

  int de = qMin(qCeil(bmax.z), m_depth-1);
  int we = qMin(qCeil(bmax.y), m_width-1);
  int he = qMin(qCeil(bmax.x), m_height-1);

  qint64 mx = he-hs+1;
  qint64 my = we-ws+1;
  qint64 mz = de-ds+1;

  MyBitArray bitmask;
  bitmask.resize(mx*my*mz);
  bitmask.fill(false);


  getVisibleRegion(ds, ws, hs,
		   de, we, he,
		   tag, false,
		   gradType, minGrad, maxGrad,
		   bitmask);



  uchar *vol = new uchar[mx*my*mz];
  memset(vol, 0, mx*my*mz);
  for(qint64 d=ds; d<=de; d++)
    for(qint64 w=ws; w<=we; w++)
      for(qint64 h=hs; h<=he; h++)
	{
	  qint64 bidx = ((qint64)(d-ds))*mx*my+((qint64)(w-ws))*mx+(h-hs);
	  if (bitmask.testBit(bidx))
	    vol[bidx] = 255;
	}

  

  //------------------
  // ignore all components below componentThreshold
  int componentThreshold = 1000;
  componentThreshold = QInputDialog::getInt(0,
					    "Component Threshold",
					    "Minimum number of voxels per labeled component",
					    1000);
  //------------------
  


  //------------------
  // find connected components
  int connectivity = 6;
  uint32_t* labels = cc3d::connected_components3d(vol,
						  mx, my, mz,
						  connectivity);

  //------------------

 
  //------------------
  // calculate volume (no. of voxels) per component
  QMap<int, int> labelMap; // contains (number of voxels) volume for each label
  for(qint64 d=ds; d<=de; d++)
    for(qint64 w=ws; w<=we; w++)
      for(qint64 h=hs; h<=he; h++)
	{
	  qint64 bidx = ((qint64)(d-ds))*mx*my+((qint64)(w-ws))*mx+(h-hs);
	  vol[bidx] = labels[bidx];

 	  if (vol[bidx] > 0)
	    labelMap[vol[bidx]] = labelMap[vol[bidx]] + 1;
	}
  //------------------

   
  //------------------
  // remove components with volume less than componentThreshold
  for(qint64 d=ds; d<=de; d++)
    for(qint64 w=ws; w<=we; w++)
      for(qint64 h=hs; h<=he; h++)
	{
	  qint64 bidx = ((qint64)(d-ds))*mx*my+((qint64)(w-ws))*mx+(h-hs);
	  qint64 idx = ((qint64)d)*m_width*m_height + ((qint64)w)*m_height + h;
	  if (labelMap[vol[bidx]] <= componentThreshold)
	    m_maskDataUS[idx] = 0;
	}
  //------------------
  
  delete [] vol;
  delete [] labels;
  
  minD = ds;
  minW = ws;
  minH = hs;
  maxD = de;
  maxW = we;
  maxH = he;

  QMessageBox::information(0, "", "Done");
}

void
VolumeOperations::removeLargestComponents(Vec bmin, Vec bmax,
					  int tag,
					  int& minD, int& maxD,
					  int& minW, int& maxW,
					  int& minH, int& maxH,
					  int gradType, float minGrad, float maxGrad)
{
  minD = maxD = minW = maxW = minH = maxH = -1;

  int ds = qMax(0, qFloor(bmin.z));
  int ws = qMax(0, qFloor(bmin.y));
  int hs = qMax(0, qFloor(bmin.x));

  int de = qMin(qCeil(bmax.z), m_depth-1);
  int we = qMin(qCeil(bmax.y), m_width-1);
  int he = qMin(qCeil(bmax.x), m_height-1);

  qint64 mx = he-hs+1;
  qint64 my = we-ws+1;
  qint64 mz = de-ds+1;

  MyBitArray bitmask;
  bitmask.resize(mx*my*mz);
  bitmask.fill(false);


  getVisibleRegion(ds, ws, hs,
		   de, we, he,
		   tag, false,
		   gradType, minGrad, maxGrad,
		   bitmask);



  uchar *vol = new uchar[mx*my*mz];
  memset(vol, 0, mx*my*mz);
  for(qint64 d=ds; d<=de; d++)
    for(qint64 w=ws; w<=we; w++)
      for(qint64 h=hs; h<=he; h++)
	{
	  qint64 bidx = ((qint64)(d-ds))*mx*my+((qint64)(w-ws))*mx+(h-hs);
	  if (bitmask.testBit(bidx))
	    vol[bidx] = 255;
	}

  

  //------------------
  // ignore all components below componentThreshold
  int largestComponents = 1;
  largestComponents = QInputDialog::getInt(0,
					   "Largest Components",
					   "Remove how many first largest components.\n1 = remove only the largest component",
					   1);
  if (largestComponents < 1)
    return;
  //------------------
  


  //------------------
  // find connected components
  int connectivity = 6;
  uint32_t* labels = cc3d::connected_components3d(vol,
						  mx, my, mz,
						  connectivity);

  //------------------

 
  //------------------
  // calculate volume (no. of voxels) per component
  QMap<int, int> labelMap; // contains (number of voxels) volume for each label
  for(qint64 d=ds; d<=de; d++)
    for(qint64 w=ws; w<=we; w++)
      for(qint64 h=hs; h<=he; h++)
	{
	  qint64 bidx = ((qint64)(d-ds))*mx*my+((qint64)(w-ws))*mx+(h-hs);
	  vol[bidx] = labels[bidx];

 	  if (vol[bidx] > 0)
	    labelMap[vol[bidx]] = labelMap[vol[bidx]] + 1;
	}
  //------------------


  //------------------
  // new remap in sequential order
  QList<int> oldLabel = labelMap.keys();  // component labels
  QList<int> compVol = labelMap.values(); // volume
  int nLabels = oldLabel.count();
  
  //-------
  // do this to sort on volume
  QMultiMap<int, int> remapLabel; // contains remapping info
  for (int i=0; i<nLabels; i++)
    remapLabel.insert(compVol[i], oldLabel[i]);
  //-------

  QList<int> largest = remapLabel.values();
  QList<int> removeComponents;
  for(int i=0; i<largestComponents; i++)
    removeComponents << largest[nLabels-1-i];
  
  //------------------
  // remove components with volume less than componentThreshold
  for(qint64 d=ds; d<=de; d++)
    for(qint64 w=ws; w<=we; w++)
      for(qint64 h=hs; h<=he; h++)
	{
	  qint64 bidx = ((qint64)(d-ds))*mx*my+((qint64)(w-ws))*mx+(h-hs);
	  qint64 idx = ((qint64)d)*m_width*m_height + ((qint64)w)*m_height + h;
	  if (removeComponents.contains(vol[bidx]))
	    m_maskDataUS[idx] = 0;
	}
  //------------------
  
  delete [] vol;
  delete [] labels;
  
  minD = ds;
  minW = ws;
  minH = hs;
  maxD = de;
  maxW = we;
  maxH = he;

  QMessageBox::information(0, "", "Done");
}


void
VolumeOperations::connectedComponents(Vec bmin, Vec bmax,
				      int tag,
				      int& minD, int& maxD,
				      int& minW, int& maxW,
				      int& minH, int& maxH,
				      int gradType, float minGrad, float maxGrad)
{
  minD = maxD = minW = maxW = minH = maxH = -1;

  int ds = qMax(0, qFloor(bmin.z));
  int ws = qMax(0, qFloor(bmin.y));
  int hs = qMax(0, qFloor(bmin.x));

  int de = qMin(qCeil(bmax.z), m_depth-1);
  int we = qMin(qCeil(bmax.y), m_width-1);
  int he = qMin(qCeil(bmax.x), m_height-1);

  qint64 mx = he-hs+1;
  qint64 my = we-ws+1;
  qint64 mz = de-ds+1;

  MyBitArray bitmask;
  bitmask.resize(mx*my*mz);
  bitmask.fill(false);


  getVisibleRegion(ds, ws, hs,
		   de, we, he,
		   tag, false,
		   gradType, minGrad, maxGrad,
		   bitmask);



  uchar *vol = new uchar[mx*my*mz];
  memset(vol, 0, mx*my*mz);
  for(qint64 d=ds; d<=de; d++)
    for(qint64 w=ws; w<=we; w++)
      for(qint64 h=hs; h<=he; h++)
	{
	  qint64 bidx = ((qint64)(d-ds))*mx*my+((qint64)(w-ws))*mx+(h-hs);
	  if (bitmask.testBit(bidx))
	    vol[bidx] = 255;
	}


  //------------------
  // starting label number
  int startLabel = 0;
  startLabel = QInputDialog::getInt(0,
				    "Starting Label",
				    "Starting label number (label numbers will be offset by this value)",
				    startLabel);

  startLabel = qBound(0, startLabel, 65530);
  //------------------
  
  
  //------------------
  int connectivity = 6;
  {
    QStringList dtypes;
    dtypes << "6"
	   << "18"
	   << "26";
    
    bool ok;
    QString option = QInputDialog::getItem(0,
					   "Connectivity Choice",
					   "3D neighbourhood connectivity",
					   dtypes,
					   0,
					   false,
					   &ok);
    
    if (ok)
      {
	if (option == "18") connectivity = 18;
	if (option == "26") connectivity = 26;
      }
  }
  //------------------
  

  //------------------
  // ignore all components below componentThreshold
  int componentThreshold = 1000;
  componentThreshold = QInputDialog::getInt(0,
					    "Component Threshold",
					    "Minimum number of voxels per labeled component",
					    1000);
  //------------------
  

  //------------------
  bool ascending = true;  
  {
    QStringList dtypes;
    dtypes << "lowest first"
	   << "highest first";
    
    bool ok;
    QString option = QInputDialog::getItem(0,
					   "Sort based on voxel count",
					   "Ascending/Descending ?",
					   dtypes,
					   0,
					   false,
					   &ok);
    
    if (ok)
      {
	if (option == "highest first")
	  ascending = false;
      }
  }
  //------------------


  //------------------
  // find connected components
  uint32_t* labels = cc3d::connected_components3d(vol,
						  mx, my, mz,
						  connectivity);
  
  delete [] vol;
  //------------------

  
  
  QMap<int, int> labelMap; // contains (number of voxels) volume for each label
  
  //------------------
  // calculate volume (no. of voxels) per component
  for(qint64 d=ds; d<=de; d++)
    for(qint64 w=ws; w<=we; w++)
      for(qint64 h=hs; h<=he; h++)
	{
	  qint64 bidx = ((qint64)(d-ds))*mx*my+((qint64)(w-ws))*mx+(h-hs);
	  if (labels[bidx] > 0)
	    labelMap[labels[bidx]] = labelMap[labels[bidx]] + 1;
	}
  //------------------

  //------------------
  // remove components with volume less than componentThreshold
  for(qint64 d=ds; d<=de; d++)
    for(qint64 w=ws; w<=we; w++)
      for(qint64 h=hs; h<=he; h++)
	{
	  qint64 bidx = ((qint64)(d-ds))*mx*my+((qint64)(w-ws))*mx+(h-hs);
	  if (labels[bidx] > 0)
	    {
	      if (labelMap[labels[bidx]] <= componentThreshold)
		labels[bidx] = 0;
	    }
	}
  //------------------
  
  //------------------
  // update labelMap to reflect removal of small components
  {
    QList<int> keys = labelMap.keys();  // component labels
    int nLabels = keys.count();
    for(int i=0; i<nLabels; i++)
      if (labelMap[keys[i]] <= componentThreshold)
	labelMap.remove(keys[i]);
  }
  //------------------
   

  //QMessageBox::information(0, "", QString("%1").arg(labelMap.keys().count()));

  //------------------
  // new remap in sequential order
  QString mesg;
  {
    QList<int> oldLabel = labelMap.keys();  // component labels
    QList<int> compVol = labelMap.values(); // volume
    int nLabels = oldLabel.count();

    //-------
    // do this to sort on volume
    QMultiMap<int, int> remapLabel; // contains remapping info
    for (int i=0; i<nLabels; i++)
      remapLabel.insert(compVol[i], oldLabel[i]);
    //-------

    QList<int> newLabel = remapLabel.values();  // labels sorted on volume
    QList<int> sortedVol = remapLabel.keys();  // sorted component volume
    

    mesg  = "---------------------\n";
    mesg += " Label : Voxel Count \n";
    mesg += "---------------------\n";
    
    labelMap.clear();
    if (ascending)  // lowest volume first
      {
	for(int i=0; i<nLabels; i++)
	  {
	    labelMap[newLabel[i]] = i+1 + startLabel;
	    mesg += QString("  %1 : %2\n").arg(labelMap[newLabel[i]], 6).arg(sortedVol[i]);
	  }
      }
    else  // highest volume first
      {
	int l=0;
	for(int i=nLabels-1; i>=0; i--)
	  {
	    l++;
	    labelMap[newLabel[i]] = l + startLabel;
	    mesg += QString("  %1 : %2\n").arg(labelMap[newLabel[i]], 6).arg(sortedVol[i]);
	  }
      }
  }
  //------------------


  //-------
  // displace labels and respective volumes
  StaticFunctions::showMessage("Labeled Component Volumes", mesg);
  //-------


  //------------------
  // apply remapping of labels to reflect sorted component volumes
  for(qint64 d=ds; d<=de; d++)
    for(qint64 w=ws; w<=we; w++)
      for(qint64 h=hs; h<=he; h++)
	{
	  qint64 bidx = ((qint64)(d-ds))*mx*my+((qint64)(w-ws))*mx+(h-hs);
	  if (labels[bidx] > 0)
	    {
	      qint64 idx = ((qint64)d)*m_width*m_height + ((qint64)w)*m_height + h;
	      m_maskDataUS[idx] = labelMap[labels[bidx]];
	    }
	}
  delete [] labels;
  //------------------
  
  
  minD = ds;
  minW = ws;
  minH = hs;
  maxD = de;
  maxW = we;
  maxH = he;

  QMessageBox::information(0, "", "Done");
}


void
VolumeOperations::sortLabels(Vec bmin, Vec bmax,
			     int gradType, float minGrad, float maxGrad)
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

  
  bool ascending = true;
  
  QStringList dtypes;
  dtypes << "lowest first"
	 << "highest first";

  bool ok;
  QString option = QInputDialog::getItem(0,
					 "Sort based on voxel count",
					 "Ascending/Descending ?",
					 dtypes,
					 0,
					 false,
					 &ok);

  if (ok)
    {
      if (option == "highest first")
	ascending = false;
    }

  
  QMap<int, int> labelMap; // contains (number of voxels) volume for each label

  //-------
  uchar *lut = Global::lut();

  for(qint64 d=ds; d<=de; d++)
    for(qint64 w=ws; w<=we; w++)
      for(qint64 h=hs; h<=he; h++)
	{
	  bool clipped = checkClipped(Vec(h, w, d));
	  if (!clipped)
	    {
	      qint64 idx = ((qint64)d)*m_width*m_height + ((qint64)w)*m_height + h;
	      int val = m_volData[idx];
	      if (m_volDataUS) val = m_volDataUS[idx];
		    
	      int mtag = m_maskDataUS[idx];
		    
	      bool opaque =  (lut[4*val+3]*Global::tagColors()[4*mtag+3] > 0);      
	      
	      if (opaque &&
		  (minGrad > 0.0 || maxGrad < 1.0))
		{
		  float gradMag = VolumeOperations::calcGrad(gradType, d, w, h,
							     m_depth, m_width, m_height,
							     m_volData, m_volDataUS);
		  
		  if (gradMag < minGrad || gradMag > maxGrad)
		    opaque = false;
		}
	      
	      if (opaque && mtag > 0)
		labelMap[mtag] = labelMap[mtag] + 1;
	    }
	}
  //-------
  
  QList<int> oldkeys = labelMap.keys();  // label
  QList<int> values = labelMap.values(); // volume
  int nLabels = oldkeys.count();
    
  //-------
  // do this to sort on volume
  QMultiMap<int, int> remapLabel; // contains remapping info
  for (int i=0; i<nLabels; i++)
    remapLabel.insert(values[i], oldkeys[i]);
  //-------

  QList<int> newkeys = remapLabel.values();  // labels sorted on volume
  
  //-------
  labelMap.clear();
  if (ascending)
    {
      for (int i=0; i<nLabels; i++)
	labelMap[newkeys[i]] = oldkeys[i];
    }
  else
    {
      for (int i=0; i<nLabels; i++)
	labelMap[newkeys[i]] = oldkeys[nLabels-1-i];
    }
  //-------


  //-------
  // relabel
  for(qint64 d=ds; d<=de; d++)
    for(qint64 w=ws; w<=we; w++)
      for(qint64 h=hs; h<=he; h++)
	{
	  qint64 idx = ((qint64)d)*m_width*m_height + ((qint64)w)*m_height + h;
	  if (m_maskDataUS[idx] > 0)
	    m_maskDataUS[idx] = labelMap[m_maskDataUS[idx]];
	}
  //-------

  QMessageBox::information(0, "Sort on voxel count", "Done sort on voxel count");
}

void
VolumeOperations::connectedComponentsPlus(Vec bmin, Vec bmax, int tag,
					  int nErode,
					  int& minD, int& maxD,
					  int& minW, int& maxW,
					  int& minH, int& maxH,
					  int gradType, float minGrad, float maxGrad)
{  
  //------------------
  // starting label number
  int startLabel = 0;
  startLabel = QInputDialog::getInt(0,
				    "Starting Label",
				    "Starting label number (label numbers will be offset by this value)",
				    startLabel);

  startLabel = qBound(0, startLabel, 65530);
  //------------------

  //------------------
  // ignore all components below componentThreshold
  int componentThreshold = 1000;
  componentThreshold = QInputDialog::getInt(0,
					    "Component Threshold",
					    "Minimum number of voxels per labeled component",
					    1000);
  //------------------


  QProgressDialog progress("Connected Components Plus",
			   QString(),
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);  
  qApp->processEvents();

  
  minD = maxD = minW = maxW = minH = maxH = -1;

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

  MyBitArray visibleMask;
  visibleMask.resize(mx*my*mz);
  visibleMask.fill(false);


  getVisibleRegion(ds, ws, hs,
		   de, we, he,
		   tag, false,
		   gradType, minGrad, maxGrad,
		   visibleMask);

  
  //------------------
  // just reset the visible portion - it will be labeled in subsequent phases
  for(qint64 d=ds; d<=de; d++)
    for(qint64 w=ws; w<=we; w++)
      for(qint64 h=hs; h<=he; h++)
	{
	  qint64 idx = ((qint64)d)*m_width*m_height + ((qint64)w)*m_height + h;
	  m_maskDataUS[idx] = 0;
	}
  //------------------

  
  // bitmask
  MyBitArray bitmask;
  bitmask = visibleMask;

  //--------------------------------------------------------------------
  //------------------
  // Erosion phase
  //------------------
  _dilatebitmask(nErode, false, // dilate transparent region (i.e. erode solid region)
		 mx, my, mz,
		 bitmask);
  //------------------
  //--------------------------------------------------------------------


  
  progress.setLabelText("Generating markers");
  progress.setValue(10);
  qApp->processEvents();

  //--------------------------------------------------------------------
  //------------------  
  // Connected components phase
  //------------------
  uchar *vol = new uchar[mx*my*mz];
  memset(vol, 0, mx*my*mz);
  for(qint64 d=ds; d<=de; d++)
    for(qint64 w=ws; w<=we; w++)
      for(qint64 h=hs; h<=he; h++)
	{
	  qint64 bidx = ((qint64)(d-ds))*mx*my+((qint64)(w-ws))*mx+(h-hs);
	  if (bitmask.testBit(bidx))
	    vol[bidx] = 255;
	}  

  
  progress.setLabelText("Generating markers");
  progress.setValue(20);
  qApp->processEvents();

  //------------------
  // find connected components
  int connectivity = 26;
  uint32_t* labels = cc3d::connected_components3d(vol,
						  mx, my, mz,
						  connectivity);  
  delete [] vol;
  //------------------


  progress.setLabelText("Generating markers");
  progress.setValue(30);
  qApp->processEvents();

  QMap<int, int> labelMap; // contains (number of voxels) volume for each label
 
  //------------------
  // calculate volume (no. of voxels) per component
  for(qint64 d=ds; d<=de; d++)
    for(qint64 w=ws; w<=we; w++)
      for(qint64 h=hs; h<=he; h++)
	{
	  qint64 bidx = ((qint64)(d-ds))*mx*my+((qint64)(w-ws))*mx+(h-hs);
	  if (labels[bidx] > 0)
	    labelMap[labels[bidx]] = labelMap[labels[bidx]] + 1;
	}
  //------------------

  //------------------
  // remove components with volume less than componentThreshold
  for(qint64 d=ds; d<=de; d++)
    for(qint64 w=ws; w<=we; w++)
      for(qint64 h=hs; h<=he; h++)
	{
	  qint64 bidx = ((qint64)(d-ds))*mx*my+((qint64)(w-ws))*mx+(h-hs);
	  if (labels[bidx] > 0)
	    {
	      if (labelMap[labels[bidx]] <= componentThreshold)
		labels[bidx] = 0;
	    }
	}
  //------------------
  
  //------------------
  // update labelMap to reflect removal of small components
  {
    QList<int> keys = labelMap.keys();  // component labels
    int nLabels = keys.count();
    for(int i=0; i<nLabels; i++)
      if (labelMap[keys[i]] <= componentThreshold)
	labelMap.remove(keys[i]);
  }
  //------------------

  progress.setLabelText("Generating markers");
  progress.setValue(40);
  qApp->processEvents();
  //------------------
  // new remap in sequential order
  {
    QList<int> oldLabel = labelMap.keys();  // component labels
    QList<int> compVol = labelMap.values(); // volume
    int nLabels = oldLabel.count();

    //-------
    // do this to sort on volume
    QMultiMap<int, int> remapLabel; // contains remapping info
    for (int i=0; i<nLabels; i++)
      remapLabel.insert(compVol[i], oldLabel[i]);
    //-------

    QList<int> newLabel = remapLabel.values();  // labels sorted on volume
    QList<int> sortedVol = remapLabel.keys();  // sorted component volume
    
    labelMap.clear();
    for(int i=0; i<nLabels; i++)
      labelMap[newLabel[i]] = i+1 + startLabel;
  }
  //------------------


  //------------------
  // apply remapping of labels to reflect sorted component volumes
  for(qint64 d=ds; d<=de; d++)
    for(qint64 w=ws; w<=we; w++)
      for(qint64 h=hs; h<=he; h++)
	{
	  qint64 bidx = ((qint64)(d-ds))*mx*my+((qint64)(w-ws))*mx+(h-hs);
	  if (labels[bidx] > 0)
	    {
	      qint64 idx = ((qint64)d)*m_width*m_height + ((qint64)w)*m_height + h;
	      m_maskDataUS[idx] = labelMap[labels[bidx]];
	    }
	}
  delete [] labels;
  //------------------
  //--------------------------------------------------------------------



  progress.setLabelText("Flooding");
  progress.setValue(50);
  qApp->processEvents();

  
  //--------------------------------------------------------------------
  //----------------------------
  // Growing seeds phase
  //----------------------------
  //----------------------------
  // figure out all te labels
  QList<int> ut;
  for(qint64 d=ds; d<=de; d++)
    for(qint64 w=ws; w<=we; w++)
      for(qint64 h=hs; h<=he; h++)
	{
	  qint64 idx = ((qint64)d)*m_width*m_height + ((qint64)w)*m_height + h;
	  if (m_maskDataUS[idx] > 0 && !ut.contains(m_maskDataUS[idx]))
	    ut << m_maskDataUS[idx];
	}
  //----------------------------

//  QList<int> ut;
//  for(int i=0; i<labelMap.count(); i++)
//    ut << i+1;

  
  // identify tag regions
  QList<VdbVolume*> tagVdb;
  for(int i=0; i<ut.count(); i++)
    {
      progress.setValue(100*(float)i/(float)ut.count());
      qApp->processEvents();
      
      bitmask.fill(false);
      
      getVisibleRegion(ds, ws, hs,
		       de, we, he,
		       ut[i], false,  // no tag zero checking
		       gradType, minGrad, maxGrad,
		       bitmask,
		       false);  
      
      VdbVolume *vdb;
      vdb = new VdbVolume;
      openvdb::FloatGrid::Accessor accessor = vdb->getAccessor();
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
      
      tagVdb << vdb;
    }
  
  
  for(int u=0; u<nErode; u++)
    {
      progress.setLabelText(QString("Flooding : %1 of %2").arg(u+1).arg(nErode));
      for(int i=0; i<ut.count(); i++)
	{
	  progress.setValue(100*(float)i/(float)ut.count());
	  qApp->processEvents();
	 
	  tagVdb[i]->convertToLevelSet(1, 0);
	  tagVdb[i]->offset(-1); // dilate

	  openvdb::FloatGrid::Accessor accessor = tagVdb[i]->getAccessor();
	  openvdb::Coord ijk;
	  int &d = ijk[0];
	  int &w = ijk[1];
	  int &h = ijk[2];
	  for(d=0; d<mz; d++)
	    for(w=0; w<my; w++)
	      for(h=0; h<mx; h++)
		{
		  if (accessor.getValue(ijk) <= 0)
		    {
		      qint64 bidx = ((qint64)d)*mx*my+w*mx+h;
		      if (visibleMask.testBit(bidx))
			{
			  qint64 d2 = ds + d;
			  qint64 w2 = ws + w;
			  qint64 h2 = hs + h;
			  qint64 idx = d2*m_width*m_height + w2*m_height + h2;
			  if (m_maskDataUS[idx] == 0) // expand into unlabelled region
			    m_maskDataUS[idx] = ut[i];
			  else if (m_maskDataUS[idx] != ut[i]) // encroaching another label
			    accessor.setValue(ijk, 0);
			}
		      else // do not venture into invisible region
			accessor.setValue(ijk, 0);
		    }
		  
		}	  
	}
    }
  progress.setValue(100);

  
  for(int i=0; i<ut.count(); i++)
    delete tagVdb[i];

  tagVdb.clear();

  minD = ds;
  minW = ws;
  minH = hs;
  maxD = de;
  maxW = we;
  maxH = he;

  QMessageBox::information(0, "", "Done");
}


void
VolumeOperations::distanceTransform(Vec bmin, Vec bmax, int tag,
				    int& minD, int& maxD,
				    int& minW, int& maxW,
				    int& minH, int& maxH,
				    int gradType, float minGrad, float maxGrad)
{
  QProgressDialog progress("Distance Transform",
			   QString(),
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);  
  qApp->processEvents();

  
  minD = maxD = minW = maxW = minH = maxH = -1;

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

  MyBitArray visibleMask;
  visibleMask.resize(mx*my*mz);
  visibleMask.fill(false);

  getVisibleRegion(ds, ws, hs,
		   de, we, he,
		   tag, false,
		   gradType, minGrad, maxGrad,
		   visibleMask);

  
  progress.setValue(50);
  qApp->processEvents();

  // generate squared distance transform
  float *dt = BinaryDistanceTransform::binaryEDTsq(visibleMask,
						   mx, my, mz,
						   true);
  
  progress.setValue(75);
  qApp->processEvents();

  
  // check distance transform
  qint64 idx = 0;
  for(qint64 d=0; d<mz; d++)
  for(qint64 w=0; w<my; w++)
  for(qint64 h=0; h<mx; h++)
    {
      qint64 bidx = ((qint64)(d+ds))*m_width*m_height+((qint64)(w+ws))*m_height+(h+hs);
      m_maskDataUS[bidx] = qRound(sqrt(dt[idx]));
      idx++;
    }
  
  
  delete [] dt;


  progress.setValue(100);
  
  minD = ds;
  minW = ws;
  minH = hs;
  maxD = de;
  maxW = we;
  maxH = he;

  QMessageBox::information(0, "", "Done");
  
}


void
VolumeOperations::localThickness(Vec bmin, Vec bmax, int tag,
				 int& minD, int& maxD,
				 int& minW, int& maxW,
				 int& minH, int& maxH,
				 int gradType, float minGrad, float maxGrad)
{
  QProgressDialog progress("Local Thickness",
			   QString(),
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);  
  qApp->processEvents();


  minD = maxD = minW = maxW = minH = maxH = -1;

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

  MyBitArray visibleMask;
  visibleMask.resize(mx*my*mz);
  visibleMask.fill(false);

  getVisibleRegion(ds, ws, hs,
		   de, we, he,
		   tag, false,
		   gradType, minGrad, maxGrad,
		   visibleMask);

  
  progress.setValue(10);
  qApp->processEvents();

  
  // generate squared distance transform
  float *lt = BinaryDistanceTransform::binaryEDTsq(visibleMask,
						   mx, my, mz,
						   true);

  
  //----------------------------
  // find max distance
  float maxLT = 0;
  for(qint64 i=0; i<mx*my*mz; i++)
    {
      lt[i] = qSqrt(lt[i]);
      maxLT = qMax(maxLT, lt[i]);
    }
  //----------------------------


  
  //----------------------------
  // find local thickness as described in
  // V. A. Dahl and A. B. Dahl, "Fast Local Thickness",
  // https://github.com/vedranaa/local-thickness
  float *out = new float[mx*my*mz];
  for(int r=0; r<(int)maxLT; r++)
    {
      progress.setLabelText(QString("%1 of %2").arg(r).arg((int)maxLT));
      progress.setValue(100*float(r)/maxLT);
      qApp->processEvents();

      // dilate distance
      distDilate(lt, out,
		 mx, my, mz,
		 r, (int)maxLT); // this is just for progress dialog
     
      for(qint64 i=0; i<mx*my*mz; i++)
	{
	  if (lt[i] > r)
	    lt[i] = out[i];
	}
    }
  //----------------------------
  
  
  delete [] out;

  
  //----------------------------
  // find maximum local thickness
  VolumeInformation pvlInfo;
  pvlInfo = VolumeInformation::volumeInformation();
  Vec voxelSize = pvlInfo.voxelSize;

  maxLT = 0;
  for(qint64 i=0; i<mx*my*mz; i++)
    {
      lt[i] *= voxelSize.x;  // assuming isotropic voxel
      maxLT = qMax(maxLT, lt[i]);
    }
  //----------------------------

  
  //----------------------------
  // set the local thickness as labels
  // from 64000 to 64999
  qint64 idx = 0;
  for(qint64 d=0; d<mz; d++)
  for(qint64 w=0; w<my; w++)
  for(qint64 h=0; h<mx; h++)
    {
      qint64 bidx = ((qint64)(d+ds))*m_width*m_height+((qint64)(w+ws))*m_height+(h+hs);
      if (lt[idx] > 0)
	m_maskDataUS[bidx] = 64000 + 999*lt[idx]/maxLT;
      else
	m_maskDataUS[bidx] = 0;
      idx++;
    }
  //----------------------------

  progress.setValue(100);
  qApp->processEvents();

  

  QMessageBox::information(0, "Max Local Thickness", QString("%1").arg(maxLT));

  
  //------------------------------
  //------------------------------
  // save local thickness to file
  QString rawflnm = QFileDialog::getSaveFileName(0,
						 "Save Local Thickness To RAW File",
						 Global::previousDirectory(),
						 "Files (*.raw)",
						 0);
  if (!rawflnm.isEmpty())
    {
      //----------
      // save raw local thickness file
      StaticFunctions::saveVolumeToFile(rawflnm,
				    8, (char*)lt,
				    mx, my, mz);
      //----------
  

      //----------
      // save pvl.nc file
      QString pvlflnm = rawflnm;
      pvlflnm.chop(3);
      pvlflnm += "pvl.nc";
      
      QList<float> rawMap;
      rawMap << 0.0;
      rawMap << maxLT;
      
      QList<int> pvlMap;
      pvlMap << 0;
      pvlMap << 255;
      
      StaticFunctions::savePvlHeader(pvlflnm,
				     true,
				     rawflnm,
				     0, 0, pvlInfo.voxelUnit,
				     mz, my, mx,
				     voxelSize.x, voxelSize.y, voxelSize.z,
				     rawMap, pvlMap,
				     "Local Thickness",
				     mz+1);
      //----------
      
      
      //----------
      // map float local thickness (lt) to uchar (ltUC) using the same array
      // and save pvl.nc.001 file
      uchar *ltUC = (uchar *)lt;
      for(qint64 i=0; i<mx*my*mz; i++)
	ltUC[i] = 255*lt[i]/maxLT;
      StaticFunctions::saveVolumeToFile(pvlflnm + ".001",
					0, (char*)ltUC,
					mx, my, mz);
      //----------

      QMessageBox::information(0, "Local Thickness", "Saved raw, pvl.nc and pvl.nc.001 files");
    }
  //------------------------------
  //------------------------------

  
  delete [] lt;
  
  
  minD = ds;
  minW = ws;
  minH = hs;
  maxD = de;
  maxW = we;
  maxH = he;

  if (rawflnm.isEmpty())
    QMessageBox::information(0, "", "Done");  
}

void
VolumeOperations::distDilate(float *vol, float *out,
			     qint64 mx, qint64 my, qint64 mz,
			     int r, int maxLT)
{
  memset(out, 0, mx*my*mz*sizeof(float));

  // collect stuff for parallel processing
  QList<QList<QVariant>> param;
  for(int d=0; d<mz; d++)
    {
      QList<QVariant> plist;
      plist << QVariant(d);
      plist << QVariant(mx);
      plist << QVariant(my);
      plist << QVariant(mz);
      plist << QVariant::fromValue(static_cast<void*>(vol));
      plist << QVariant::fromValue(static_cast<void*>(out));
      
      param << plist;
    }
						     
  QFutureWatcher<void> futureWatcher;

  QProgressDialog progress(QString("Distance dilation %1 of %2").arg(r).arg(maxLT),
			   QString(),
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
  if (mz > 300)
    {
      progress.setMinimumDuration(0);
      QObject::connect(&futureWatcher, &QFutureWatcher<void>::finished, &progress, &QProgressDialog::reset);
      QObject::connect(&progress, &QProgressDialog::canceled, &futureWatcher, &QFutureWatcher<void>::cancel);
      QObject::connect(&futureWatcher,  &QFutureWatcher<void>::progressRangeChanged, &progress, &QProgressDialog::setRange);
      QObject::connect(&futureWatcher, &QFutureWatcher<void>::progressValueChanged,  &progress, &QProgressDialog::setValue);
    }
  
  futureWatcher.setFuture(QtConcurrent::map(param, VolumeOperations::parDistDilate));
  
  
  // Display the dialog and start the event loop.
  if (mz > 300)
    progress.exec();
  
  futureWatcher.waitForFinished();
}

void
VolumeOperations::parDistDilate(QList<QVariant> plist)
{
  // displacement indices
  float index[26][3] = {{ 0, -1,  0},
			{ 0,  1,  0},
			{-1,  0,  0},
			{ 1,  0,  0},
			{ 0,  0, -1},
			{ 0,  0,  1},
			
			{ 0, -1, -1},
			{ 0, -1,  1},
			{ 0,  1, -1},
			{ 0,  1,  1},
			{-1,  0, -1},
			{-1,  0,  1},
			{ 1,  0, -1},
			{ 1,  0,  1},
			{-1, -1,  0},
			{-1,  1,  0},
			{ 1, -1,  0},
			{ 1,  1,  0},
			
			{ 1,  1,  1},
			{ 1, -1,  1},
			{ 1, -1, -1},
			{ 1,  1, -1},
			{-1,  1,  1},
			{-1, -1,  1},
			{-1, -1, -1},
			{-1,  1, -1}};
 		      

  // weights
  float q632 = qSqrt(6)+qSqrt(3)+qSqrt(2);
  float W[3] = {qSqrt(6)/q632, qSqrt(3)/q632, qSqrt(2)/q632}; 

  int ise[4] = {0, 6, 18, 26};

  int d = plist[0].toInt();
  qint64 mx = plist[1].toInt();
  qint64 my = plist[2].toInt();
  qint64 mz = plist[3].toInt();
  float *vol = static_cast<float*>(plist[4].value<void*>());
  float *out = static_cast<float*>(plist[5].value<void*>());
    
  for(int w=0; w<my; w++)
    for(int h=0; h<mx; h++)
      {
	qint64 idx = d*mx*my + w*mx + h;
	float v = vol[idx];
	if (v > 0.0)
	  {
	    for(int iter=0; iter<3; iter++)
	      {
		for (int i=ise[iter]; i<ise[iter+1]; i++)
		  {
		    int d1 = d + index[i][0];
		    int w1 = w + index[i][1];
		    int h1 = h + index[i][2];
		    
		    d1 = qBound(0, d1, (int)mz-1);
		    w1 = qBound(0, w1, (int)my-1);
		    h1 = qBound(0, h1, (int)mx-1);
		    
		    qint64 idx1 = d1*mx*my + w1*mx + h1;
		    
		    v = qMax(v, vol[idx1]);
		  }
		out[idx] += W[iter] * v;
	      }
	  }
      }
}
    


//void
//VolumeOperations::seqDistDilate(float *vol, float *out,
//			     qint64 mx, qint64 my, qint64 mz )
//{
//  // displacement indices
//  float index[26][3] = {{ 0, -1,  0},
//			{ 0,  1,  0},
//			{-1,  0,  0},
//			{ 1,  0,  0},
//			{ 0,  0, -1},
//			{ 0,  0,  1},
//			
//			{ 0, -1, -1},
//			{ 0, -1,  1},
//			{ 0,  1, -1},
//			{ 0,  1,  1},
//			{-1,  0, -1},
//			{-1,  0,  1},
//			{ 1,  0, -1},
//			{ 1,  0,  1},
//			{-1, -1,  0},
//			{-1,  1,  0},
//			{ 1, -1,  0},
//			{ 1,  1,  0},
//			
//			{ 1,  1,  1},
//			{ 1, -1,  1},
//			{ 1, -1, -1},
//			{ 1,  1, -1},
//			{-1,  1,  1},
//			{-1, -1,  1},
//			{-1, -1, -1},
//			{-1,  1, -1}};
// 		      
//
//  // weights
//  float q632 = qSqrt(6)+qSqrt(3)+qSqrt(2);
//  float W[3] = {qSqrt(6)/q632, qSqrt(3)/q632, qSqrt(2)/q632};
//  
//
//  int ise[4] = {0, 6, 18, 26};
//    
//  memset(out, 0, mx*my*mz*sizeof(float));
//
//  for(int d=0; d<mz-1; d++)
//    for(int w=0; w<my-1; w++)
//      for(int h=0; h<mx-1; h++)
//	for(int iter=0; iter<3; iter++)
//	  {
//	    qint64 idx = d*mx*my + w*mx + h;
//	    float v = vol[idx];
//	    for (int i=ise[iter]; i<ise[iter+1]; i++)
//	      {
//		int d1 = d + index[i][0];
//		int w1 = w + index[i][1];
//		int h1 = h + index[i][2];
//		
//		d1 = qBound(0, d1, (int)mz-1);
//		w1 = qBound(0, w1, (int)my-1);
//		h1 = qBound(0, h1, (int)mx-1);
//		
//		qint64 idx1 = d1*mx*my + w1*mx + h1;
//		
//		v = qMax(v, vol[idx1]);
//	      }
//	    out[idx] += W[iter] * v;
//	  }
//
//}
