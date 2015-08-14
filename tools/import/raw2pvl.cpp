#include "global.h"
#include "staticfunctions.h"
#include "raw2pvl.h"
#include <netcdfcpp.h>
#include <algorithm>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <limits.h>

#include <QtXml>
#include <QFile>

#include "savepvldialog.h"
#include "volumefilemanager.h"

#ifdef Q_OS_WIN
#include <float.h>
#define ISNAN(v) _isnan(v)
#else
#define ISNAN(v) isnan(v)
#endif

#define REMAPVOLUME()							\
  {									\
    for(uint j=0; j<width*height; j++)					\
      {									\
	float v = ptr[j];						\
	int idx;							\
	float frc;							\
	if (v <= rawMap[0] || ISNAN(v))					\
	  {								\
	    idx = 0;							\
	    frc = 0;							\
	  }								\
	else if (v >= rawMap[rawSize])					\
	  {								\
	    idx = rawSize-1;						\
	    frc = 1;							\
	  }								\
	else								\
	  {								\
	    for(uint m=0; m<rawSize; m++)				\
	      {								\
		if (v >= rawMap[m] &&					\
		    v <= rawMap[m+1])					\
		  {							\
		    idx = m;						\
		    frc = ((float)v-rawMap[m])/				\
		      (rawMap[m+1]-rawMap[m]);				\
		  }							\
	      }								\
	  }								\
									\
	int pv = pvlMap[idx] + frc*(pvlMap[idx+1]-pvlMap[idx]);		\
	pvl[j] = pv;							\
      }									\
  }


void
Raw2Pvl::applyMapping(uchar *raw, int voxelType,
		      QList<float> rawMap,
		      uchar *pvlslice, int pvlbpv,
		      QList<int> pvlMap,
		      int width, int height)
{
  int rawSize = rawMap.size()-1;

  if (rawMap.count() == pvlMap.count())
    {
      bool same = true;
      for(int i=0; i<rawMap.count(); i++)
	if (rawMap[i] != pvlMap[i])
	  same = false;

      if (same)
	{
	  memcpy(pvlslice, raw, width*height*pvlbpv);
	  return;
	}
    }


  if (pvlbpv == 1)
    {
      uchar *pvl = (uchar*)pvlslice;
      if (voxelType == _UChar)
	{
	  uchar *ptr = raw;
	  REMAPVOLUME();
	}
      else if (voxelType == _Char)
	{
	  char *ptr = (char*)raw;
	  REMAPVOLUME();
	}
      else if (voxelType == _UShort)
	{
	  ushort *ptr = (ushort*)raw;
	  REMAPVOLUME();
	}
      else if (voxelType == _Short)
	{
	  short *ptr = (short*)raw;
	  REMAPVOLUME();
	}
      else if (voxelType == _Int)
	{
	  int *ptr = (int*)raw;
	  REMAPVOLUME();
	}
      else if (voxelType == _Float)
	{
	  float *ptr = (float*)raw;
	  REMAPVOLUME();
	}
    }
  else
    {
      ushort *pvl = (ushort*)pvlslice;
      if (voxelType == _UChar)
	{
	  uchar *ptr = raw;
	  REMAPVOLUME();
	}
      else if (voxelType == _Char)
	{
	  char *ptr = (char*)raw;
	  REMAPVOLUME();
	}
      else if (voxelType == _UShort)
	{
	  ushort *ptr = (ushort*)raw;
	  REMAPVOLUME();
	}
      else if (voxelType == _Short)
	{
	  short *ptr = (short*)raw;
	  REMAPVOLUME();
	}
      else if (voxelType == _Int)
	{
	  int *ptr = (int*)raw;
	  REMAPVOLUME();
	}
      else if (voxelType == _Float)
	{
	  float *ptr = (float*)raw;
	  REMAPVOLUME();
	}
    }
}

//-----------------------------------------
//-----------------------------------------
#define MEANFILTER()				\
  {						\
    for(int j=0; j<width; j++)			\
      for(int k=0; k<height; k++)		\
	{					\
	  int js = qMax(0, j-1);		\
	  int je = qMin(width-1, j+1);		\
	  int ks = qMax(0, k-1);		\
	  int ke = qMin(height-1, k+1);		\
						\
	  int dn = 0;				\
	  float avg = 0;			\
	  for(int j1=js; j1<=je; j1++)		\
	    for(int k1=ks; k1<=ke; k1++)	\
	      {					\
		int idx = j1*height+k1;		\
		float savg = (p0[idx] +		\
			      4*p1[idx] +	\
			      p2[idx]);		\
		dn += 6;			\
		if (j1 == j)			\
		  {				\
		    avg += 4*savg;		\
		    dn += 18;			\
		  }				\
		else				\
		  avg += savg;			\
	      }					\
	  avg = avg/dn;				\
	  p[j*height + k] = avg;		\
	}					\
  }

#define MEDIANFILTER()				\
  {						\
    for(int j=0; j<width; j++)			\
      for(int k=0; k<height; k++)		\
	{					\
	  int js = qMax(0, j-1);		\
	  int je = qMin(width-1, j+1);		\
	  int ks = qMax(0, k-1);		\
	  int ke = qMin(height-1, k+1);		\
	  					\
	  QList<float> vlist;			\
	  for(int j1=js; j1<=je; j1++)		\
	    for(int k1=ks; k1<=ke; k1++)	\
	      {					\
		int idx = j1*height+k1;		\
		vlist.append((float)p0[idx]);	\
		vlist.append((float)p1[idx]);	\
		vlist.append((float)p2[idx]);	\
	      }					\
	  qSort(vlist.begin(), vlist.end());	\
	  p[j*height + k] = vlist[vlist.size()/2];\
	}					\
  }

//-----------------------------------------
// sigma2 is used to define the domain filter
// it will decide the decay based on how far the
// value is from the central voxel value
#define BILATERALFILTER(dsigma)			\
  {						\
    float sigma2 = 2*dsigma*dsigma;		\
    for(int j=0; j<width; j++)			\
      for(int k=0; k<height; k++)		\
	{					\
	  int js = qMax(0, j-1);		\
	  int je = qMin(width-1, j+1);		\
	  int ks = qMax(0, k-1);		\
	  int ke = qMin(height-1, k+1);		\
						\
	  float cv = p1[j*height + k];		\
	  float dn = 0;				\
	  float avg = 0;			\
	  for(int j1=js; j1<=je; j1++)		\
	    for(int k1=ks; k1<=ke; k1++)	\
	      {					\
		int idx = j1*height+k1;		\
		float a0 = exp(-((p0[idx]-cv)*(p0[idx]-cv))/sigma2);\
		float a1 = exp(-((p1[idx]-cv)*(p1[idx]-cv))/sigma2);\
		float a2 = exp(-((p2[idx]-cv)*(p2[idx]-cv))/sigma2);\
		float savg = (a0*p0[idx] +			    \
			      4*a1*p1[idx] +			    \
			      a2*p2[idx]);			    \
		float ddn = a0+4*a1+a2;				    \
		if (j1 != j && k1 != k)			\
		  {					\
		    avg += savg;			\
		    dn += ddn;				\
		  }					\
		else					\
		  {					\
		    if (j1 == j)			\
		      {					\
			avg += 4*savg;			\
			dn += 4*ddn;			\
		      }					\
		    if (k1 == k)			\
		      {					\
			avg += 4*savg;			\
			dn += 4*ddn;			\
		      }					\
		  }					\
	      }					\
	  avg = avg/dn;				\
	  p[j*height + k] = avg;		\
	}					\
  }


void
Raw2Pvl::applyFilter(uchar *val0,
		     uchar *val1,
		     uchar *val2,
		     uchar *vg,
		     int voxelType,
		     int width, int height,
		     int filter)
{
  if (voxelType == _UChar)
    {
      uchar *p0 = val0;
      uchar *p1 = val1;
      uchar *p2 = val2;
      uchar *p  = vg;
      if (filter == _MeanFilter)
	MEANFILTER()
      else if (filter == _MedianFilter)
	MEDIANFILTER()
//      else if (filter == _BilateralFilter)
//	BILATERALFILTER(dsigma)
    }
  else if (voxelType == _Char)
    {
      char *p0 = (char*)val0;
      char *p1 = (char*)val1;
      char *p2 = (char*)val2;
      char *p  = (char*)vg;
      if (filter == _MeanFilter)
	MEANFILTER()
      else if (filter == _MedianFilter)
	MEDIANFILTER()
//      else if (filter == _BilateralFilter)
//	BILATERALFILTER(dsigma)
    }
  else if (voxelType == _UShort)
    {
      ushort *p0 = (ushort*)val0;
      ushort *p1 = (ushort*)val1;
      ushort *p2 = (ushort*)val2;
      ushort *p  = (ushort*)vg;
      if (filter == _MeanFilter)
	MEANFILTER()
      else if (filter == _MedianFilter)
	MEDIANFILTER()
//      else if (filter == _BilateralFilter)
//	BILATERALFILTER(dsigma)
    }
  else if (voxelType == _Short)
    {
      short *p0 = (short*)val0;
      short *p1 = (short*)val1;
      short *p2 = (short*)val2;
      short *p  = (short*)vg;
      if (filter == _MeanFilter)
	MEANFILTER()
      else if (filter == _MedianFilter)
	MEDIANFILTER()
//      else if (filter == _BilateralFilter)
//	BILATERALFILTER(dsigma)
    }
  else if (voxelType == _Int)
    {
      int *p0 = (int*)val0;
      int *p1 = (int*)val1;
      int *p2 = (int*)val2;
      int *p  = (int*)vg;
      if (filter == _MeanFilter)
	MEANFILTER()
      else if (filter == _MedianFilter)
	MEDIANFILTER()
//      else if (filter == _BilateralFilter)
//	BILATERALFILTER(dsigma)
    }
  else if (voxelType == _Float)
    {
      float *p0 = (float*)val0;
      float *p1 = (float*)val1;
      float *p2 = (float*)val2;
      float *p  = (float*)vg;
      if (filter == _MeanFilter)
	MEANFILTER()
      else if (filter == _MedianFilter)
	MEDIANFILTER()
//      else if (filter == _BilateralFilter)
//	BILATERALFILTER(dsigma)
    }
}

void
Raw2Pvl::applyVolumeFilter(QString rawFile,
			   QString smoothFile,
			   int voxelType,
			   int headerBytes,
			   int depth,
			   int width,
			   int height,
			   int filter)
{
  QString ftxt;
//  if (filter == _BilateralFilter)
//    {
//      if (dsigma > 0.0)
//	ftxt = QString("Applying Bilateral Filter (sigma-D = %1)").arg(dsigma);
//      else
//	filter = _MeanFilter;
//    }

  if (filter == _MeanFilter)
    ftxt = "Applying Gaussian Filter";
  else if (filter == _MedianFilter)
    ftxt = "Applying Median Filter";
    
  QProgressDialog progress(ftxt,
			   "Cancel",
			   0, 100,
			   0);
  progress.setMinimumDuration(0);


  QFile fin(rawFile);
  fin.open(QFile::ReadOnly);

  QFile fout(smoothFile);
  fout.open(QFile::WriteOnly);

  if (headerBytes > 0)
    {
      uchar *hb = new uchar[headerBytes];  
      fin.read((char*)hb, headerBytes);
      fout.write((char*)hb, headerBytes);
      delete [] hb;
    }

  int bpv = 1;
  if (voxelType == _UChar) bpv = 1;
  else if (voxelType == _Char) bpv = 1;
  else if (voxelType == _UShort) bpv = 2;
  else if (voxelType == _Short) bpv = 2;
  else if (voxelType == _Int) bpv = 4;
  else if (voxelType == _Float) bpv = 4;

  int nbytes = width*height*bpv;
  uchar *val0 = new uchar[nbytes];
  uchar *val1 = new uchar[nbytes];
  uchar *val2 = new uchar[nbytes];
  uchar *vg   = new uchar[nbytes];

  fin.read((char*)val1, nbytes);
  fin.read((char*)val2, nbytes);
  memcpy(val0, val1, nbytes);

  applyFilter(val0, val1, val2, vg,
	      voxelType, width, height,
	      filter);
  fout.write((char*)vg, nbytes);

  for(uint d=1; d<depth-1; d++)
    {
      progress.setValue((int)(100.0*(float)d/(float)depth));
      qApp->processEvents();

      // shift the planes
      uchar *tmp = val0;
      val0 = val1;
      val1 = val2;
      val2 = tmp;
    
      fin.read((char*)val2, nbytes);

      applyFilter(val0, val1, val2, vg,
		  voxelType, width, height,
		  filter);
      fout.write((char*)vg, nbytes);
    }
  fin.close();
  
  // shift the planes
  uchar *tmp = val0;
  val0 = val1;
  val1 = val2;
  val2 = tmp;
  memcpy(val2, val1, nbytes);

  applyFilter(val0, val1, val2, vg,
	      voxelType, width, height,
	      filter);
  fout.write((char*)vg, nbytes);

  fout.close();

  delete [] val0;
  delete [] val1;
  delete [] val2;
  delete [] vg;

  progress.setValue(100);
  qApp->processEvents();
}


#define MEANSLICEFILTER(T)						\
  {									\
    T *ptr = (T*)tmp;							\
    T *ptr1 = (T*)tmp1;							\
									\
    int ji=0;								\
    for(int j=0; j<sw; j++)						\
      {									\
	int y = j*subSamplingLevel;					\
	int loy = qMax(0, y-subSamplingLevel+1);			\
	int hiy = qMin(width-1, y+subSamplingLevel-1);			\
	for(int i=0; i<sh; i++)						\
	  {								\
	    int x = i*subSamplingLevel;					\
	    int lox = qMax(0, x-subSamplingLevel+1);			\
	    int hix = qMin(height-1, x+subSamplingLevel-1);		\
									\
	    int dn = 0;							\
	    float sum = 0;						\
	    for(int jy=loy; jy<=hiy; jy++)				\
	      {								\
		for(int ix=lox; ix<=hix; ix++)				\
		  {							\
		    int idx = jy*height+ix;				\
		    float v = ptr[idx];					\
		    int ddn = 1;					\
		    if (jy == y)					\
		      {							\
			v *= 4;						\
			ddn *= 4;					\
		      }							\
		    if (ix == x)					\
		      {							\
			v *= 4;						\
			ddn *= 4;					\
		      }							\
		    sum += v;						\
		    dn += ddn;						\
		  }							\
	      }								\
	    ptr1[ji] = sum/dn;						\
	    ji++;							\
	  }								\
      }									\
									\
    uchar *vptr = volX[0];						\
    for (int c=0; c<totcount-1; c++)					\
      volX[c] = volX[c+1];						\
    volX[totcount-1] = vptr;						\
									\
    memcpy(volX[totcount-1], tmp1, bpv*sw*sh);				\
  }


#define MEDIANSLICEFILTER(T)						\
  {									\
    T *ptr = (T*)tmp;							\
    T *ptr1 = (T*)tmp1;							\
									\
    int ji=0;								\
    for(int j=0; j<sw; j++)						\
      {									\
	int y = j*subSamplingLevel;					\
	int loy = qMax(0, y-subSamplingLevel+1);			\
	int hiy = qMin(width-1, y+subSamplingLevel-1);			\
	for(int i=0; i<sh; i++)						\
	  {								\
	    int x = i*subSamplingLevel;					\
	    int lox = qMax(0, x-subSamplingLevel+1);			\
	    int hix = qMin(height-1, x+subSamplingLevel-1);		\
									\
	    QList<T> vlist;						\
	    for(int jy=loy; jy<=hiy; jy++)				\
	      {								\
		for(int ix=lox; ix<=hix; ix++)				\
		  {							\
		    int idx = jy*height+ix;				\
		    vlist.append(ptr[idx]);				\
		  }							\
	      }								\
	    qSort(vlist.begin(), vlist.end());				\
	    ptr1[ji] = vlist[vlist.size()/2];				\
	    ji++;							\
	  }								\
      }									\
									\
    uchar *vptr = volX[0];						\
    for (int c=0; c<totcount-1; c++)					\
      volX[c] = volX[c+1];						\
    volX[totcount-1] = vptr;						\
									\
    memcpy(volX[totcount-1], tmp1, bpv*sw*sh);				\
  }


#define BILATERALSLICEFILTER(T)						\
  {									\
    T *ptr = (T*)tmp;							\
    T *ptr1 = (T*)tmp1;							\
									\
    int ji=0;								\
    for(int j=0; j<sw; j++)						\
      {									\
	int y = j*subSamplingLevel;					\
	int loy = qMax(0, y-subSamplingLevel+1);			\
	int hiy = qMin(width-1, y+subSamplingLevel-1);			\
	for(int i=0; i<sh; i++)						\
	  {								\
	    int x = i*subSamplingLevel;					\
	    int lox = qMax(0, x-subSamplingLevel+1);			\
	    int hix = qMin(height-1, x+subSamplingLevel-1);		\
									\
	    float vmin = ptr[loy*height+lox];				\
	    float vmax = ptr[loy*height+lox];				\
	    for(int jy=loy; jy<=hiy; jy++)				\
	      {								\
		for(int ix=lox; ix<=hix; ix++)				\
		  {							\
		    if (jy!=y || ix!=x)					\
		      {							\
			int idx = jy*height+ix;				\
			vmin = qMin(vmin, (float)ptr[idx]);		\
			vmax = qMax(vmax, (float)ptr[idx]);		\
		      }							\
		  }							\
	      }								\
	    int idx = y*height+x;					\
	    ptr1[ji] = qBound(vmin, (float)ptr[idx], vmax);		\
	    ji++;							\
	  }								\
      }									\
									\
    uchar *vptr = volX[0];						\
    for (int c=0; c<totcount-1; c++)					\
      volX[c] = volX[c+1];						\
    volX[totcount-1] = vptr;						\
									\
    memcpy(volX[totcount-1], tmp1, bpv*sw*sh);				\
  }

#define NOSLICEFILTER(T)						\
  {									\
    T *ptr = (T*)tmp;							\
    T *ptr1 = (T*)tmp1;							\
									\
    int ji=0;								\
    for(int j=0; j<sw; j++)						\
      {									\
	int y = j*subSamplingLevel;					\
	for(int i=0; i<sh; i++)						\
	  {								\
	    int x = i*subSamplingLevel;					\
	    int idx = y*height+x;					\
	    ptr1[ji] = ptr[idx];					\
	    ji++;							\
	  }								\
      }									\
									\
    uchar *vptr = volX[0];						\
    for (int c=0; c<totcount-1; c++)					\
      volX[c] = volX[c+1];						\
    volX[totcount-1] = vptr;						\
									\
    memcpy(volX[totcount-1], tmp1, bpv*sw*sh);				\
  }



#define MEANSUBSAMPLE(T)			\
  {						\
    memset(tmp1f, 0, 4*width*height);		\
    memset(tmp1, 0, bpv*sw*sh);			\
						\
    T *ptr1 = (T*)tmp1;				\
    T **volS;					\
    volS = new T*[totcount];			\
    for(uint i=0; i<totcount; i++)		\
      volS[i] = (T*)volX[i];			\
    						\
    for(int x=0; x<totcount; x++)		\
      for(int j=0; j<sw*sh; j++)		\
	tmp1f[j]+=volS[x][j];			\
    for(int j=0; j<sw*sh; j++)			\
      ptr1[j] = tmp1f[j]/totcount;		\
    						\
    delete [] volS;				\
  }


#define MEDIANSUBSAMPLE(T)			\
  {						\
    memset(tmp1, 0, bpv*sw*sh);			\
						\
    T *ptr1 = (T*)tmp1;				\
						\
    T **volS;					\
    volS = new T*[totcount];			\
    for(uint i=0; i<totcount; i++)		\
      volS[i] = (T*)volX[i];			\
						\
    for(int j=0; j<sw*sh; j++)			\
      {						\
	QList<T> vlist;				\
	for(int x=0; x<totcount; x++)		\
	  vlist.append(volS[x][j]);		\
	qSort(vlist.begin(), vlist.end());	\
	ptr1[j] = vlist[vlist.size()/2];	\
      }						\
    						\
    delete [] volS;				\
  }


#define BILATERALSUBSAMPLE(T)			  \
  {							  \
    memset(tmp1, 0, bpv*sw*sh);				  \
    T *ptr1 = (T*)tmp1;					  \
    T **volS;						  \
    volS = new T*[totcount];				  \
    for(uint i=0; i<totcount; i++)			  \
      volS[i] = (T*)volX[i];				  \
    							  \
    int tc2 = totcount/2;				  \
    for(int j=0; j<sw*sh; j++)				  \
      {							  \
	float vmin = volS[0][0];			  \
	float vmax = volS[0][0];			  \
	for(int x=0; x<tc2; x++)			  \
	  {						  \
	    vmin = qMin(vmin, (float)volS[x][j]);	  \
	    vmax = qMax(vmax, (float)volS[x][j]);	  \
	  }						  \
	for(int x=tc2+1; x<totcount; x++)		  \
	  {						  \
	    vmin = qMin(vmin, (float)volS[x][j]);	  \
	    vmax = qMax(vmax, (float)volS[x][j]);	  \
	  }						  \
	ptr1[j] = qBound(vmin, (float)volS[tc2][j], vmax);\
      }							  \
    							  \
    delete [] volS;					  \
  }


#define SUBSAMPLE(T)				\
  {						\
    int x = totcount/2;				\
    memcpy(tmp1, volX[x], bpv*sw*sh);		\
  }

void
Raw2Pvl::subsampleVolume(QString rawFilename,
			 QString subsampledFilename,
			 int voxelType, int headerBytes,
			 int depth, int width, int height,
			 int subSamplingLevel, int filter)
{
  QProgressDialog progress(QString("Subsampling %1").arg(rawFilename),
			   "Cancel",
			   0, 100,
			   0);
  progress.setMinimumDuration(0);


  int sd = StaticFunctions::getScaledown(subSamplingLevel, depth);
  int sw = width/subSamplingLevel;
  int sh = height/subSamplingLevel;

  QFile fout(subsampledFilename);
  fout.open(QFile::WriteOnly);
  uchar svt = voxelType;

  if (svt == 5) // _Float
    svt = 8;

  fout.write((char*)&svt, 1);
  fout.write((char*)&sd, sizeof(int));
  fout.write((char*)&sw, sizeof(int));
  fout.write((char*)&sh, sizeof(int));

  QFile fin(rawFilename);
  fin.open(QFile::ReadOnly);
  fin.seek(headerBytes);

  int bpv = 1;
  if (voxelType == _UChar) bpv = 1;
  else if (voxelType == _Char) bpv = 1;
  else if (voxelType == _UShort) bpv = 2;
  else if (voxelType == _Short) bpv = 2;
  else if (voxelType == _Int) bpv = 4;
  else if (voxelType == _Float) bpv = 4;

  int nbytes = width*height*bpv;
  uchar *tmp = new uchar[nbytes];
  uchar *tmp1 = new uchar [nbytes];  
  float *tmp1f = new float [width*height];

  int totcount = 2*subSamplingLevel-1;
  uchar **volX;
  volX = 0;
  if (totcount > 1)
    {
      volX = new uchar*[totcount];
      for(int i=0; i<totcount; i++)
	volX[i] = new uchar[nbytes];
    }

  int count=0;
  int kslc=0;
  for(uint k=0; k<depth; k++)
    {
      progress.setValue((int)(100.0*(float)k/(float)depth));
      qApp->processEvents();
      
      fin.read((char*)tmp, nbytes);

      // apply filter and scaledown the slice
      if (voxelType == _UChar)
	{
	  if (filter == _NoFilter)
	      NOSLICEFILTER(uchar)
	  else if (filter == _BilateralFilter)
	    BILATERALSLICEFILTER(uchar)
	  else if (filter == _MeanFilter)
	    MEANSLICEFILTER(uchar)
	  else
	    MEDIANSLICEFILTER(uchar)
	}
      else if (voxelType == _Char)
	{
	  if (filter == _NoFilter)
	    NOSLICEFILTER(char)
	  else if (filter == _BilateralFilter)
	    BILATERALSLICEFILTER(char)
	  else if (filter == _MeanFilter)
	    MEANSLICEFILTER(char)
	  else
	    MEDIANSLICEFILTER(char)
	}
      else if (voxelType == _UShort)
	{
	  if (filter == _NoFilter)
	    NOSLICEFILTER(ushort)
	  else if (filter == _BilateralFilter)
	    BILATERALSLICEFILTER(ushort)
	  else if (filter == _MeanFilter)
	    MEANSLICEFILTER(ushort)
	  else
	    MEDIANSLICEFILTER(ushort)
	}
      else if (voxelType == _Short)
	{
	  if (filter == _NoFilter)
	    NOSLICEFILTER(short)
	  else if (filter == _BilateralFilter)
	    BILATERALSLICEFILTER(short)
	  else if (filter == _MeanFilter)
	    MEANSLICEFILTER(short)
	  else
	    MEDIANSLICEFILTER(short)
	}
      else if (voxelType == _Int)
	{
	  if (filter == _NoFilter)
	    NOSLICEFILTER(int)
	  else if (filter == _BilateralFilter)
	    BILATERALSLICEFILTER(int)
	  else if (filter == _MeanFilter)
	    MEANSLICEFILTER(int)
	  else
	    MEDIANSLICEFILTER(int)
	}
      else if (voxelType == _Float)
	{
	  if (filter == _NoFilter)
	    NOSLICEFILTER(float)
	  else if (filter == _BilateralFilter)
	    BILATERALSLICEFILTER(float)
	  else if (filter == _MeanFilter)
	    MEANSLICEFILTER(float)
	  else
	    MEDIANSLICEFILTER(float)
	}

      count ++;

      if (count == totcount)
	{
	  if (voxelType == _UChar)
	    {
	      if (filter == _NoFilter)
		SUBSAMPLE(uchar)
	      else if (filter == _BilateralFilter)
		BILATERALSUBSAMPLE(uchar)
	      else if (filter == _MeanFilter)
		MEANSUBSAMPLE(uchar)	    
	      else
		MEDIANSUBSAMPLE(uchar)
          }
	  else if (voxelType == _Char)
	    {
	      if (filter == _NoFilter)
		SUBSAMPLE(char)
	      else if (filter == _BilateralFilter)
		BILATERALSUBSAMPLE(char)
	      else if (filter == _MeanFilter)
		MEANSUBSAMPLE(char)	    
	      else
		MEDIANSUBSAMPLE(char)
	    }
	  else if (voxelType == _UShort)
	    {
	      if (filter == _NoFilter)
		SUBSAMPLE(ushort)
	      else if (filter == _BilateralFilter)
		BILATERALSUBSAMPLE(ushort)
	      else if (filter == _MeanFilter)
		MEANSUBSAMPLE(ushort)	    
	      else
		MEDIANSUBSAMPLE(ushort)
	    }
	  else if (voxelType == _Short)
	    {
	      if (filter == _NoFilter)
		SUBSAMPLE(short)
	      else if (filter == _BilateralFilter)
		BILATERALSUBSAMPLE(short)
	      else if (filter == _MeanFilter)
		MEANSUBSAMPLE(short)	    
	      else
		MEDIANSUBSAMPLE(short)
	    }
	  else if (voxelType == _Int)
	    {
	      if (filter == _NoFilter)
		SUBSAMPLE(int)
	      else if (filter == _BilateralFilter)
		BILATERALSUBSAMPLE(int)
	      else if (filter == _MeanFilter)
		MEANSUBSAMPLE(int)	    
	      else
		MEDIANSUBSAMPLE(int)
	    }
	  else if (voxelType == _Float)
	    {
	      if (filter == _NoFilter)
		SUBSAMPLE(float)
	      else if (filter == _BilateralFilter)
		BILATERALSUBSAMPLE(float)
	      else if (filter == _MeanFilter)
		MEANSUBSAMPLE(float)	    
	      else
		MEDIANSUBSAMPLE(float)
	    }

	  count = totcount/2;
	  
	  // save subsampled slice
	  fout.write((char*)tmp1, bpv*sw*sh);

	  // increment slice number
	  kslc++;
	  if (kslc == sd)
	    break;
	}
    }

  delete [] tmp;
  delete [] tmp1;
  delete [] tmp1f;
  if (totcount > 1)
    {
      for(int i=0; i<totcount; i++)
	delete [] volX[i];
      delete [] volX;
    }

  fin.close();
  fout.close();

  progress.setValue(100);
}

//-----------------------------
QString
getPvlNcFilename()
{
  QFileDialog fdialog(0,
		      "Save processed volume",
		      Global::previousDirectory(),
		      "Drishti (*.pvl.nc) ;; MetaImage (*.mhd)");
  fdialog.setAcceptMode(QFileDialog::AcceptSave);

  if (!fdialog.exec() == QFileDialog::Accepted)
    return "";

  QString pvlFilename = fdialog.selectedFiles().value(0);
  if (fdialog.selectedNameFilter() == "MetaImage (*.mhd)")
    {
      if (!pvlFilename.endsWith(".mhd"))
	pvlFilename += ".mhd";
      return pvlFilename;
    }
  if (fdialog.selectedNameFilter() == "Drishti (*.pvl.nc)")
    {
      if (pvlFilename.endsWith(".pvl.nc.pvl.nc"))
	  pvlFilename.chop(7);
      if (!pvlFilename.endsWith(".pvl.nc"))
	pvlFilename += ".pvl.nc";

      return pvlFilename;
    }

  return "";
}

bool
saveSliceZeroAtTop()
{
  bool save0attop = false;
  bool ok = false;
  QStringList slevels;
  slevels << "No - (default)";  
  slevels << "Yes - save slice 0 as top slice";
  QString option = QInputDialog::getItem(0,
		   "Save Data",
		   "Save slice 0 as top slice ?",
		    slevels,
			  0,
		      false,
		       &ok);
  if (ok)
    {
      QStringList op = option.split(' ');
      if (op[0] == "Yes")
	save0attop = true;
    }
  else
    QMessageBox::information(0, "Save Data", "Will not save slice 0 as top slice.");

  return save0attop;
}

bool
getSaveRawFile()
{
  bool saveRawFile = false;
  bool ok = false;
  QStringList slevels;
  slevels << "Yes - save raw file";
  slevels << "No";  
  QString option = QInputDialog::getItem(0,
		   "Save Processed Volume",
		   "Save RAW file along with preprocessed volume ?",
		    slevels,
			  1,
		      false,
		       &ok);
  if (ok)
    {
      QStringList op = option.split(' ');
      if (op[0] == "Yes")
	saveRawFile = true;
    }
  else
    QMessageBox::information(0, "RAW Volume", "Will not save raw volume");

  return saveRawFile;
}

QString
getRawFilename(QString pvlFilename)
{
  QString rawfile = QFileDialog::getSaveFileName(0,
						 "Save processed volume",
						 QFileInfo(pvlFilename).absolutePath(),
						 "RAW Files (*.raw)",
						 0,
						 QFileDialog::DontUseNativeDialog);
  return rawfile;
}

int
getZSubsampling(int dsz, int wsz, int hsz)
{
  bool ok = false;
  QStringList slevels;

  slevels.clear();
  slevels << "No subsampling in Z";
  slevels << QString("2 [Z(%1) %2 %3]").arg(dsz/2).arg(wsz).arg(hsz);
  slevels << QString("3 [Z(%1) %2 %3]").arg(dsz/3).arg(wsz).arg(hsz);
  slevels << QString("4 [Z(%1) %2 %3]").arg(dsz/4).arg(wsz).arg(hsz);
  slevels << QString("5 [Z(%1) %2 %3]").arg(dsz/5).arg(wsz).arg(hsz);
  slevels << QString("6 [Z(%1) %2 %3]").arg(dsz/6).arg(wsz).arg(hsz);
  QString option = QInputDialog::getItem(0,
					 "Volume Size",
					 "Z subsampling",
					 slevels,
					 0,
					 false,
					 &ok);
  int svslz = 1;
  if (ok)
    {   
      QStringList op = option.split(' ');
      svslz = qMax(1, op[0].toInt());
    }
  return svslz;
}

int
getXYSubsampling(int svslz, int dsz, int wsz, int hsz)
{
  bool ok = false;
  QStringList slevels;

  slevels.clear();
  slevels << "No subsampling in XY";
  slevels << QString("2 [%1 Y(%2) X(%3)]").arg(dsz/svslz).arg(wsz/2).arg(hsz/2);
  slevels << QString("3 [%1 Y(%2) X(%3)]").arg(dsz/svslz).arg(wsz/3).arg(hsz/3);
  slevels << QString("4 [%1 Y(%2) X(%3)]").arg(dsz/svslz).arg(wsz/4).arg(hsz/4);
  slevels << QString("5 [%1 Y(%2) X(%3)]").arg(dsz/svslz).arg(wsz/5).arg(hsz/5);
  slevels << QString("6 [%1 Y(%2) X(%3)]").arg(dsz/svslz).arg(wsz/6).arg(hsz/6);
  QString option = QInputDialog::getItem(0,
					 "Volume Size",
					 "XY subsampling",
					 slevels,
					 0,
					 false,
					 &ok);
  int svsl = 1;
  if (ok)
    {   
      QStringList op = option.split(' ');
      svsl = qMax(1, op[0].toInt());
    }
  return svsl;
}


#define AVERAGEFILTER(n)			\
  {						\
    for(int j=0; j<width; j++)			\
      for(int k=0; k<height; k++)		\
	{					\
	  float avg = 0;			\
	  for(int i=0; i<2*n+1; i++)		\
	    avg += pv[i][j*height+k]; 		\
	  p[j*height + k] = avg/(2*n+1);	\
	}					\
  }

#define DILATEFILTER(n)					\
  {							\
    for(int j=0; j<width; j++)				\
      for(int k=0; k<height; k++)			\
	{						\
	  float avg = 0;				\
	  for(int i=0; i<2*n+1; i++)			\
	    avg = qMax(avg,(float)pv[i][j*height+k]);	\
	  p[j*height + k] = avg;			\
	}						\
  }

void
Raw2Pvl::applyMeanFilter(uchar **val, uchar *vg,
			 int voxelType,
			 int width, int height,
			 int spread, bool dilateFilter)
{
  if (voxelType == _UChar)
    {
      uchar **pv = val;
      uchar *p  = vg;
      if (dilateFilter)
	DILATEFILTER(spread)
      else 
        AVERAGEFILTER(spread)
    }
  else if (voxelType == _Char)
    {
      char **pv = (char**)val;
      char *p  = (char*)vg;
      if (dilateFilter)
	DILATEFILTER(spread)
      else 
        AVERAGEFILTER(spread)
    }
  else if (voxelType == _UShort)
    {
      ushort **pv = (ushort**)val;
      ushort *p  = (ushort*)vg;
      if (dilateFilter)
	DILATEFILTER(spread)
      else 
        AVERAGEFILTER(spread)
    }
  else if (voxelType == _Short)
    {
      short **pv = (short**)val;
      short *p  = (short*)vg;
      if (dilateFilter)
	DILATEFILTER(spread)
      else 
        AVERAGEFILTER(spread)
    }
  else if (voxelType == _Int)
    {
      int **pv = (int**)val;
      int *p  = (int*)vg;
      if (dilateFilter)
	DILATEFILTER(spread)
      else 
        AVERAGEFILTER(spread)
    }
  else if (voxelType == _Float)
    {
      float **pv = (float**)val;
      float *p  = (float*)vg;
      if (dilateFilter)
	DILATEFILTER(spread)
      else 
        AVERAGEFILTER(spread)
    }
}



#define SLICEAVERAGEFILTER(n)					\
  {								\
    for(int i=0; i<height; i++)					\
      for(int j=0; j<width; j++)				\
	{							\
	  float pj = 0;						\
	  for(int j1=j-n; j1<=j+n; j1++)			\
	    {							\
	      int idx = qBound(0, j1, width-1)*height+i;	\
	      pj += pv[idx];					\
	    }							\
	  p[j*height+i] = pj/(2*n+1);				\
	}							\
    								\
    for(int j=0; j<width; j++)					\
      for(int i=0; i<height; i++)				\
	{							\
	  float pi = 0;						\
	  for(int i1=i-n; i1<=i+n; i1++)			\
	    {							\
	      int idx = j*height + qBound(0, i1, height-1);	\
	      pi += p[idx];					\
	    }							\
	  pv[j*height+i] = pi/(2*n+1);				\
	}							\
    								\
  }

#define SLICEDILATEFILTER(n)					\
  {								\
    for(int i=0; i<height; i++)					\
      for(int j=0; j<width; j++)				\
	{							\
	  float pj = 0;						\
	  int jst = qMax(0, j-n);				\
	  int jed = qMin(width-1, j+n);				\
	  for(int j1=jst; j1<=jed; j1++)			\
	    {							\
	      int idx = qBound(0, j1, width-1)*height+i;	\
	      pj = qMax((float)pv[idx],pj);			\
	    }							\
	  p[j*height+i] = pj;					\
	}							\
    								\
    for(int j=0; j<width; j++)					\
      for(int i=0; i<height; i++)				\
	{							\
	  float pi = 0;						\
	  int ist = qMax(0, i-n);				\
	  int ied = qMin(height-1, i+n);			\
	  for(int i1=ist; i1<=ied; i1++)			\
	    {							\
	      int idx = j*height+qBound(0, i1, height-1);	\
	      pi = qMax((float)p[idx],pi);			\
	    }							\
	  pv[j*height+i] = pi;					\
	}							\
  }

void
Raw2Pvl::applyMeanFilterToSlice(uchar *val, uchar *vg,
				int voxelType,
				int width, int height,
				int spread,
				bool dilateFilter)
{
  if (voxelType == _UChar)
    {
      uchar *pv = val;
      uchar *p  = vg;
      if (dilateFilter)
	SLICEDILATEFILTER(spread)
      else
        SLICEAVERAGEFILTER(spread)
    }
  else if (voxelType == _Char)
    {
      char *pv = (char*)val;
      char *p  = (char*)vg;
      if (dilateFilter)
	SLICEDILATEFILTER(spread)
      else
        SLICEAVERAGEFILTER(spread)
    }
  else if (voxelType == _UShort)
    {
      ushort *pv = (ushort*)val;
      ushort *p  = (ushort*)vg;
      if (dilateFilter)
	SLICEDILATEFILTER(spread)
      else
	SLICEAVERAGEFILTER(spread)
    }
  else if (voxelType == _Short)
    {
      short *pv = (short*)val;
      short *p  = (short*)vg;
      if (dilateFilter)
	SLICEDILATEFILTER(spread)
      else
	SLICEAVERAGEFILTER(spread)
    }
  else if (voxelType == _Int)
    {
      int *pv = (int*)val;
      int *p  = (int*)vg;
      if (dilateFilter)
	SLICEDILATEFILTER(spread)
      else
	SLICEAVERAGEFILTER(spread)
    }
  else if (voxelType == _Float)
    {
      float *pv = (float*)val;
      float *p  = (float*)vg;
      if (dilateFilter)
	SLICEDILATEFILTER(spread)
      else
	SLICEAVERAGEFILTER(spread)
    }
}

void
Raw2Pvl::savePvlHeader(QString pvlFilename,
		       bool saveRawFile, QString rawfile,
		       int voxelType, int pvlVoxelType, int voxelUnit,
		       int d, int w, int h,
		       float vx, float vy, float vz,
		       QList<float> rawMap, QList<int> pvlMap,
		       QString description,
		       int slabSize)
{
  QString xmlfile = pvlFilename;

  QDomDocument doc("Drishti_Header");

  QDomElement topElement = doc.createElement("PvlDotNcFileHeader");
  doc.appendChild(topElement);

  {      
    QString vstr;
    if (saveRawFile)
      {
	// save relative path for the rawfile
	QFileInfo fileInfo(pvlFilename);
	QDir direc = fileInfo.absoluteDir();
	vstr = direc.relativeFilePath(rawfile);
      }
    else
      vstr = "";

    QDomElement de0 = doc.createElement("rawfile");
    QDomText tn0;
    tn0 = doc.createTextNode(QString("%1").arg(vstr));
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }
      
  {      
    QString vstr;
    if (voxelType == Raw2Pvl::_UChar)      vstr = "unsigned char";
    else if (voxelType == Raw2Pvl::_Char)  vstr = "char";
    else if (voxelType == Raw2Pvl::_UShort)vstr = "unsigned short";
    else if (voxelType == Raw2Pvl::_Short) vstr = "short";
    else if (voxelType == Raw2Pvl::_Int)   vstr = "int";
    else if (voxelType == Raw2Pvl::_Float) vstr = "float";
    
    QDomElement de0 = doc.createElement("voxeltype");
    QDomText tn0;
    tn0 = doc.createTextNode(QString("%1").arg(vstr));
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }


  {      
    QString vstr;
    if (pvlVoxelType == Raw2Pvl::_UChar)      vstr = "unsigned char";
    else if (pvlVoxelType == Raw2Pvl::_Char)  vstr = "char";
    else if (pvlVoxelType == Raw2Pvl::_UShort)vstr = "unsigned short";
    else if (pvlVoxelType == Raw2Pvl::_Short) vstr = "short";
    else if (pvlVoxelType == Raw2Pvl::_Int)   vstr = "int";
    else if (pvlVoxelType == Raw2Pvl::_Float) vstr = "float";
    
    QDomElement de0 = doc.createElement("pvlvoxeltype");
    QDomText tn0;
    tn0 = doc.createTextNode(QString("%1").arg(vstr));
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }


  {      
    QDomElement de0 = doc.createElement("gridsize");
    QDomText tn0;
    tn0 = doc.createTextNode(QString("%1 %2 %3").arg(d).arg(w).arg(h));
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }

  {      
    QString vstr;
    if (voxelUnit == Raw2Pvl::_Nounit)         vstr = "no units";
    else if (voxelUnit == Raw2Pvl::_Angstrom)  vstr = "angstrom";
    else if (voxelUnit == Raw2Pvl::_Nanometer) vstr = "nanometer";
    else if (voxelUnit == Raw2Pvl::_Micron)    vstr = "micron";
    else if (voxelUnit == Raw2Pvl::_Millimeter)vstr = "millimeter";
    else if (voxelUnit == Raw2Pvl::_Centimeter)vstr = "centimeter";
    else if (voxelUnit == Raw2Pvl::_Meter)     vstr = "meter";
    else if (voxelUnit == Raw2Pvl::_Kilometer) vstr = "kilometer";
    else if (voxelUnit == Raw2Pvl::_Parsec)    vstr = "parsec";
    else if (voxelUnit == Raw2Pvl::_Kiloparsec)vstr = "kiloparsec";
    
    QDomElement de0 = doc.createElement("voxelunit");
    QDomText tn0;
    tn0 = doc.createTextNode(QString("%1").arg(vstr));
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }

  {      
    QDomElement de0 = doc.createElement("voxelsize");
    QDomText tn0;
    tn0 = doc.createTextNode(QString("%1 %2 %3").arg(vx).arg(vy).arg(vz));
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }
  
  {
    QString vstr = description.trimmed();
    QDomElement de0 = doc.createElement("description");
    QDomText tn0;
    tn0 = doc.createTextNode(QString("%1").arg(vstr));
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }

  {      
    QDomElement de0 = doc.createElement("slabsize");
    QDomText tn0;
    tn0 = doc.createTextNode(QString("%1").arg(slabSize));
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }
  
  {      
    QString vstr;
    for(int i=0; i<rawMap.size(); i++)
      vstr += QString("%1 ").arg(rawMap[i]);
    
    QDomElement de0 = doc.createElement("rawmap");
    QDomText tn0;
    tn0 = doc.createTextNode(QString("%1").arg(vstr));
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }

  {      
    QString vstr;
    for(int i=0; i<pvlMap.size(); i++)
      vstr += QString("%1 ").arg(pvlMap[i]);
    
    QDomElement de0 = doc.createElement("pvlmap");
    QDomText tn0;
    tn0 = doc.createTextNode(QString("%1").arg(vstr));
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }
  
  QFile f(xmlfile.toLatin1().data());
  if (f.open(QIODevice::WriteOnly))
    {
      QTextStream out(&f);
      doc.save(out, 2);
      f.close();
    }
}

void
Raw2Pvl::savePvl(VolumeData* volData,
		 int dmin, int dmax,
		 int wmin, int wmax,
		 int hmin, int hmax,
		 QStringList timeseriesFiles)
{
  //------------------------------------------------------
  int rvdepth, rvwidth, rvheight;    
  volData->gridSize(rvdepth, rvwidth, rvheight);

  int dsz=dmax-dmin+1;
  int wsz=wmax-wmin+1;
  int hsz=hmax-hmin+1;

  uchar voxelType = volData->voxelType();  
  int headerBytes = volData->headerBytes();

  int bpv = 1;
  if (voxelType == _UChar) bpv = 1;
  else if (voxelType == _Char) bpv = 1;
  else if (voxelType == _UShort) bpv = 2;
  else if (voxelType == _Short) bpv = 2;
  else if (voxelType == _Int) bpv = 4;
  else if (voxelType == _Float) bpv = 4;

  int slabSize = (1024*1024*1024)/(bpv*wsz*hsz);
//  if (slabSize < dsz)
//    {  
//      QStringList items;
//      items << "no" << "yes";
//      QString yn = QInputDialog::getItem(0, "Split Volume",
//					 "Split volume larger than 1Gb into multiple files ?",
//					 items,
//					 0,
//					 false);
//      //*** max 1Gb per slab
//      if (yn != "yes") // put all in a single file
//	slabSize = dsz+1;
//    }
  slabSize = dsz+1;
  //------------------------------------------------------

  QString pvlFilename = getPvlNcFilename();
  if (pvlFilename.endsWith(".mhd"))
    {
      saveMHD(pvlFilename,
	      volData,
	      dmin, dmax,
	      wmin, wmax,
	      hmin, hmax);
      return;
    }

  if (pvlFilename.count() < 4)
    {
      QMessageBox::information(0, "pvl.nc", "No .pvl.nc filename chosen.");
      return;
    }

  bool save0AtTop = saveSliceZeroAtTop();;

  bool saveRawFile = getSaveRawFile();

  QString rawfile;
  if (saveRawFile) rawfile = getRawFilename(pvlFilename);
  if (rawfile.isEmpty())
    saveRawFile = false;

  int svslz = getZSubsampling(dsz, wsz, hsz);
  int svsl = getXYSubsampling(svslz, dsz, wsz, hsz);

  int dsz2 = dsz/svslz;
  int wsz2 = wsz/svsl;
  int hsz2 = hsz/svsl;
  int svsl3 = svslz*svsl*svsl;
  //------------------------------------------------------

  //------------------------------------------------------
  // get final volume size
  int final_dsz2 = dsz2;
  int final_wsz2 = wsz2;
  int final_hsz2 = hsz2;
  int pad_value = 0;
  int sfd = 0;
  int sfw = 0;
  int sfh = 0;
  int efd = 0;
  int efw = 0;
  int efh = 0;
  {
    bool ok;
    QString text;
    text = QInputDialog::getText(0,
				 "Final Volume Grid Size With Padding",
				 "Final Volume Grid Size With Padding",
				 QLineEdit::Normal,
				 QString("%1 %2 %3").\
				 arg(final_dsz2).\
				 arg(final_wsz2).\
				 arg(final_hsz2),
				 &ok);
    if (ok && !text.isEmpty())
      {
	QStringList list = text.split(" ", QString::SkipEmptyParts);
	if (list.count() == 3)
	  {
	    final_dsz2 = qMax(dsz2, list[0].toInt());
	    final_wsz2 = qMax(wsz2, list[1].toInt());
	    final_hsz2 = qMax(hsz2, list[2].toInt());

	    int td = final_dsz2 - dsz2;
	    int tw = final_wsz2 - wsz2;
	    int th = final_hsz2 - hsz2;

	    sfd = td/2;
	    efd = td - sfd;

	    sfw = tw/2;
	    efw = tw - sfw;

	    sfh = th/2;
	    efh = th - sfh;

	    QString text;
	    text = QInputDialog::getText(0,
					 "Pad volume With Value",
					 "Pad Volume With Value",
					 QLineEdit::Normal,
					 "0",
					 &ok);
	    if (ok && !text.isEmpty())
	      pad_value = text.toInt();
	  }
      }

    slabSize = final_dsz2+1;
  }
  //------------------------------------------------------

  //------------------------------------------------------
  // -- get saving parameters for processed file
  SavePvlDialog savePvlDialog;
  float vx, vy, vz;
  volData->voxelSize(vx, vy, vz);
  QString desc = volData->description();
  int vu = volData->voxelUnit();
  savePvlDialog.setVoxelUnit(vu);
  // scale the voxelsize according to subsampling used
  savePvlDialog.setVoxelSize(vx*svsl, vy*svsl, vz*svslz);
  savePvlDialog.setDescription(desc);
  savePvlDialog.exec();

  int spread = savePvlDialog.volumeFilter();
  bool dilateFilter = savePvlDialog.dilateFilter();
  int voxelUnit = savePvlDialog.voxelUnit();
  QString description = savePvlDialog.description();
  savePvlDialog.voxelSize(vx, vy, vz);

  QList<float> rawMap = volData->rawMap();
  QList<int> pvlMap = volData->pvlMap();

  int pvlbpv = 1;
  if (pvlMap[pvlMap.count()-1] > 255)
    pvlbpv = 2;

  int pvlVoxelType = 0;
  if (pvlbpv == 2) pvlVoxelType = 2;

  int nbytes = rvwidth*rvheight*bpv;
  double *filtervol = new double[wsz2*hsz2];
  uchar *pvlslice = new uchar[pvlbpv*wsz2*hsz2];
  uchar *raw = new uchar[nbytes];
  uchar **val;
  if (spread > 0)
    {
      val = new uchar*[2*spread+1];
      for (int i=0; i<2*spread+1; i++)
	val[i] = new uchar[nbytes];
    }
  int rawSize = rawMap.size()-1;
  int width = wsz2;
  int height = hsz2;
  bool subsample = (svsl > 1 || svslz > 1);
  bool trim = (dmin != 0 ||
	       wmin != 0 ||
	       hmin != 0 ||
	       dsz2 != rvdepth ||
	       wsz2 != rvwidth ||
	       hsz2 != rvheight);

  uchar *final_val = new uchar[pvlbpv*final_wsz2*final_hsz2];

  VolumeFileManager rawFileManager;
  VolumeFileManager pvlFileManager;

  QProgressDialog progress("Saving processed volume",
			   "Cancel",
			   0, 100,
			   0);
  progress.setMinimumDuration(0);

  //------------------------------------------------------
  int tsfcount = qMax(1, timeseriesFiles.count());
  for (int tsf=0; tsf<tsfcount; tsf++)
    {
      QString pvlflnm = pvlFilename;
      QString rawflnm = rawfile;

      if (tsfcount > 1)
	{
	  QFileInfo ftpvl(pvlFilename);
	  QFileInfo ftraw(timeseriesFiles[tsf]);
	  pvlflnm = QFileInfo(ftpvl.absolutePath(),
			      ftraw.completeBaseName() + ".pvl.nc").absoluteFilePath();

	  rawflnm = QFileInfo(ftpvl.absolutePath(),
			      ftraw.completeBaseName() + ".raw").absoluteFilePath();

	  volData->replaceFile(timeseriesFiles[tsf]);
	}

      pvlFileManager.setBaseFilename(pvlflnm);
//      pvlFileManager.setDepth(dsz2);
//      pvlFileManager.setWidth(wsz2);
//      pvlFileManager.setHeight(hsz2);
      pvlFileManager.setDepth(final_dsz2);
      pvlFileManager.setWidth(final_wsz2);
      pvlFileManager.setHeight(final_hsz2);
      pvlFileManager.setVoxelType(pvlVoxelType);
      pvlFileManager.setHeaderSize(13);
      pvlFileManager.setSlabSize(slabSize);
      pvlFileManager.setSliceZeroAtTop(save0AtTop);
      pvlFileManager.createFile(true);
      
      if (saveRawFile)
	{
	  rawFileManager.setBaseFilename(rawflnm);
	  rawFileManager.setDepth(dsz2);
	  rawFileManager.setWidth(wsz2);
	  rawFileManager.setHeight(hsz2);
	  rawFileManager.setVoxelType(voxelType);
	  rawFileManager.setHeaderSize(13);
	  rawFileManager.setSlabSize(slabSize);
	  rawFileManager.setSliceZeroAtTop(save0AtTop);
	  if (rawFileManager.exists())
	    {
	      bool ok = false;
	      QStringList slevels;
	      slevels << "Yes - overwrite";
	      slevels << "No";  
	      QString option = QInputDialog::getItem(0,
						     "Save RAW Volume",
						     QString("%1 exists - Overwrite ?"). \
						     arg(rawFileManager.fileName()),
						     slevels,
						     0,
						     false,
						     &ok);
	      if (!ok)
		return;
	      
	      QStringList op = option.split(' ');
	      if (op[0] != "Yes")
		{
		  QMessageBox::information(0, "Save",
	        QString("Please choose a different name for the preprocessed volume - RAW file not overwritten"));
		  return;
		}
	    }
	  rawFileManager.createFile(true);
	}
      //------------------------------------------------------


      savePvlHeader(pvlflnm,
		    saveRawFile, rawflnm,
		    voxelType, pvlVoxelType, voxelUnit,
		    //dsz/svslz, wsz/svsl, hsz/svsl,
		    //dsz2, wsz2, hsz2,
		    final_dsz2, final_wsz2, final_hsz2,
		    vx, vy, vz,
		    rawMap, pvlMap,
		    description,
		    slabSize);


      // ------------------
      // add padding
      if (sfd > 0)
	{
	  memset(final_val, pad_value, pvlbpv*final_wsz2*final_hsz2);
	  for(int esl=0; esl<sfd; esl++)
	    pvlFileManager.setSlice(esl, final_val);
	}
      // ------------------
	

      for(int dd=0; dd<dsz2; dd++)
	{
	  int d0 = dmin + dd*svslz; 
	  int d1 = d0 + svslz-1;
	  
	  progress.setValue((int)(100*(float)dd/(float)dsz2));
	  qApp->processEvents();
	  
	  memset(filtervol, 0, 8*wsz2*hsz2);
	  for (int d=d0; d<=d1; d++)
	    {
	      if (spread > 0)
		{
		  if (d == d0)
		    {
		      volData->getDepthSlice(d, val[spread]);
		      applyMeanFilterToSlice(val[spread], raw,
					     voxelType, rvwidth, rvheight,
					     spread, dilateFilter);

		      for(int i=-spread; i<0; i++)
			{
			  if (d+i >= 0)
			    volData->getDepthSlice(d+i, val[spread+i]);
			  else
			    volData->getDepthSlice(0, val[spread+i]);

			  applyMeanFilterToSlice(val[spread+i], raw,
						 voxelType, rvwidth, rvheight,
						 spread, dilateFilter);
			}
		      
		      for(int i=1; i<=spread; i++)
			{
			  if (d+i < rvdepth)
			    volData->getDepthSlice(d+i, val[spread+i]);
			  else
			    volData->getDepthSlice(rvdepth-1, val[spread+i]);

			  applyMeanFilterToSlice(val[spread+i], raw,
						 voxelType, rvwidth, rvheight,
						 spread, dilateFilter);
			}
		    }
		  else if (d < rvdepth-spread)
		    {
		      volData->getDepthSlice(d+spread, val[2*spread]);
		      applyMeanFilterToSlice(val[2*spread], raw,
					     voxelType, rvwidth, rvheight,
					     spread, dilateFilter);
		    }		  
		  else
		    {
		      volData->getDepthSlice(rvdepth-1, val[2*spread]);
		      applyMeanFilterToSlice(val[2*spread], raw,
					     voxelType, rvwidth, rvheight,
					     spread, dilateFilter);
		    }		  
		}
	      else
		volData->getDepthSlice(d, raw);
	      
	      if (spread > 0)
		{
		  applyMeanFilter(val, raw,
				  voxelType, rvwidth, rvheight,
				  spread, dilateFilter);
		  
		  // now shift the planes
		  uchar *tmp = val[0];
		  for(int i=0; i<2*spread; i++)
		    val[i] = val[i+1];
		  val[2*spread] = tmp;
		}
	      
	      if (trim || subsample)
		{
		  int fi = 0;
		  for(int j=0; j<wsz2; j++)
		    {
		      int y0 = wmin+j*svsl;
		      int y1 = y0+svsl-1;
		      for(int i=0; i<hsz2; i++)
			{
			  int x0 = hmin+i*svsl;
			  int x1 = x0+svsl-1;
			  for(int y=y0; y<=y1; y++)
			    for(int x=x0; x<=x1; x++)
			      {
				if (voxelType == _UChar)
				  { uchar *ptr = raw; filtervol[fi] += ptr[y*rvheight+x]; }
				else if (voxelType == _Char)
				  { char *ptr = (char*)raw; filtervol[fi] += ptr[y*rvheight+x]; }
				else if (voxelType == _UShort)
				  { ushort *ptr = (ushort*)raw; filtervol[fi] += ptr[y*rvheight+x]; }
				else if (voxelType == _Short)
				  { short *ptr = (short*)raw; filtervol[fi] += ptr[y*rvheight+x]; }
				else if (voxelType == _Int)
				  { int *ptr = (int*)raw; filtervol[fi] += ptr[y*rvheight+x]; }
				else if (voxelType == _Float)
				  { float *ptr = (float*)raw; filtervol[fi] += ptr[y*rvheight+x]; }
			      }
			  fi++;
			}
		    }
		} // trim || subsample
	    }
	  
	  if (trim || subsample)
	    {
	      if (subsample)
		{
		  for(int fi=0; fi<wsz2*hsz2; fi++)
		    filtervol[fi] /= svsl3;
		}
	      
	      if (voxelType == _UChar)
		{
		  uchar *ptr = raw;
		  for(int fi=0; fi<wsz2*hsz2; fi++)
		    ptr[fi] = filtervol[fi];
		}
	      else if (voxelType == _Char)
		{
		  char *ptr = (char*)raw;
		  for(int fi=0; fi<wsz2*hsz2; fi++)
		    ptr[fi] = filtervol[fi];
		}
	      else if (voxelType == _UShort)
		{
		  ushort *ptr = (ushort*)raw;
		  for(int fi=0; fi<wsz2*hsz2; fi++)
		    ptr[fi] = filtervol[fi];
		}
	      else if (voxelType == _Short)
		{
		  short *ptr = (short*)raw;
		  for(int fi=0; fi<wsz2*hsz2; fi++)
		    ptr[fi] = filtervol[fi];
		}
	      else if (voxelType == _Int)
		{
		  int *ptr = (int*)raw;
		  for(int fi=0; fi<wsz2*hsz2; fi++)
		    ptr[fi] = filtervol[fi];
		}
	      else if (voxelType == _Float)
		{
		  float *ptr = (float*)raw;
		  for(int fi=0; fi<wsz2*hsz2; fi++)
		    ptr[fi] = filtervol[fi];
		}
	    } // trim || subsample
	  
	  if (saveRawFile)
	    rawFileManager.setSlice(dd, raw);
	  
	  applyMapping(raw, voxelType, rawMap,
		       pvlslice, pvlbpv, pvlMap,
		       width, height);

	  if (sfw == 0 && sfh == 0)
	    pvlFileManager.setSlice(sfd+dd, pvlslice);
	  else // add padding if required
	    {
	      memset(final_val, pad_value, pvlbpv*final_wsz2*final_hsz2);
	      if (pvlbpv == 1)
		{
		  for(int wi=0; wi<wsz2; wi++)
		    for(int hi=0; hi<hsz2; hi++)
		      final_val[(wi+sfw)*final_hsz2+(hi+sfh)] = pvlslice[wi*hsz2+hi];
		}
	      else
		{
		  for(int wi=0; wi<wsz2; wi++)
		    for(int hi=0; hi<hsz2; hi++)
		      ((ushort*)final_val)[(wi+sfw)*final_hsz2+(hi+sfh)] = ((ushort*)pvlslice)[wi*hsz2+hi];
		}
	      pvlFileManager.setSlice(sfd+dd, final_val);
	    }
	}
    }

  // -------------------------
  // add padding if required
  if (efd > 0)
    {
      memset(final_val, pad_value, pvlbpv*final_wsz2*final_hsz2);
      for(int esl=0; esl<efd; esl++)
	pvlFileManager.setSlice(dsz2+sfd+esl, final_val);
    }
  // -------------------------

  delete [] final_val;

  delete [] filtervol;
  delete [] pvlslice;
  delete [] raw;
  if (spread > 0)
    {
      for (int i=0; i<2*spread+1; i++)
	delete [] val[i];
      delete [] val;
    }
  
  progress.setValue(100);
  
  QMessageBox::information(0, "Save", "-----Done-----");
}

void
Raw2Pvl::Old2New(QString oldflnm, QString direc)
{
  NcError err(NcError::verbose_nonfatal);

  NcFile pvlFile(oldflnm.toLatin1().data(), NcFile::ReadOnly);

  if (!pvlFile.is_valid())
    {
      QMessageBox::information(0, "Error",
			       QString("%1 is not a valid preprocessed volume file").arg(oldflnm));
      return;
    }

  bool ok = true;
  NcVar *ncvar;
  ncvar = pvlFile.get_var("RVolume"); ok &= (ncvar != 0);
  ncvar = pvlFile.get_var("GVolume"); ok &= (ncvar != 0);
  ncvar = pvlFile.get_var("BVolume"); ok &= (ncvar != 0);  
  pvlFile.close();

  if (ok)
    Old2NewRGBA(oldflnm, direc);
  else
    Old2NewScalar(oldflnm, direc);
}

void
Raw2Pvl::Old2NewScalar(QString oldflnm, QString direc)
{
  QMessageBox::information(0, "Error", "Sorry not implemented");
}


void
Raw2Pvl::Old2NewRGBA(QString oldflnm, QString direc)
{
  QMessageBox::information(0, "Error", "Sorry not implemented");
}

void
saveSettings(int memGb,
	     int spareMb)
{
  QString homePath = QDir::homePath();
  QFileInfo settingsFile(homePath, ".meshgenerator");

  QFile fin(settingsFile.absoluteFilePath());
  if (fin.open(QIODevice::WriteOnly | QIODevice::Text))
    {
      QTextStream out(&fin);
      out << "main memory :: " << memGb << "\n";
      out << "keep spare :: " << spareMb << "\n";
    }
}

bool
loadSettings(int &memGb,
	     int &spareMb)
{
  QString homePath = QDir::homePath();
  QFileInfo settingsFile(homePath, ".meshgenerator");

  memGb = 1;
  spareMb = 500;

  bool ok = false;
  if (settingsFile.exists())
    {
      QFile fin(settingsFile.absoluteFilePath());
      if (fin.open(QIODevice::ReadOnly | QIODevice::Text))
	{
	  QTextStream in(&fin);
	  if (!in.atEnd())
	    {
	      QString line = in.readLine();
	      QStringList words = line.split("::");
	      memGb = words[1].toInt();

	      if (!in.atEnd())
		{
		  line = in.readLine();
		  words = line.split("::");
		  spareMb = words[1].toInt();
		}
	      
	      ok = true;
	    }
	}
    }

  return ok;
}

bool
checkSettings(int memGb,
	     int spareMb)
{
  bool ok = true;

  QStringList items;
  items << "Do not change memory settings";
  items << "Change memory settings";
  QString item = QInputDialog::getItem(0,
				       "Memory settings",
				       QString("Main memory : %1 GB\nKeep spare : %2 Mb").\
				       arg(memGb).arg(spareMb),
				       items,
				       0,
				       false,
				       &ok);
  if (ok && !item.isEmpty())
    {
      if (item == "Change memory settings")
	ok = false;
    }
  else if (!ok)
    ok = true;

  return ok;
}

void
getSettings(int &memGb,
	    int &spareMb)
{
  int mem = QInputDialog::getInt(0, "Main Memory Size in GB", "size (GB)", 1, 1, 1000);
  memGb = mem;

  mem = QInputDialog::getInt(0, "Keep Spare Memory (in MB)", "size (MB)", 1, 1, 1000);
  spareMb = mem;
}

void
Raw2Pvl::saveIsosurface(VolumeData* volData,
			int dmin, int dmax,
			int wmin, int wmax,
			int hmin, int hmax,
			QStringList timeseriesFiles)
{
  QMessageBox::information(0, "Error", "This option is no longer available");
}

void
Raw2Pvl::batchProcess(VolumeData* volData,
		      QStringList timeseriesFiles)
{
  QString pvlFilename = getPvlNcFilename();
  if (pvlFilename.count() < 4)
    {
      QMessageBox::information(0, "pvl.nc", "No .pvl.nc filename chosen.");
      return;
    }

  bool save0AtTop = saveSliceZeroAtTop();;

  bool saveRawFile = getSaveRawFile();

  QString rawfile;
  if (saveRawFile) rawfile = getRawFilename(pvlFilename);
  if (rawfile.isEmpty())
    saveRawFile = false;

  int svslz = getZSubsampling(1024, 1024, 1024);
  int svsl = getXYSubsampling(svslz, 1024, 1024, 1024);
  int svsl3 = svslz*svsl*svsl;
  //------------------------------------------------------

  //------------------------------------------------------
  // -- get saving parameters for processed file
  SavePvlDialog savePvlDialog;
  float vx, vy, vz;
  volData->voxelSize(vx, vy, vz);
  QString desc = volData->description();
  savePvlDialog.setVoxelUnit(Raw2Pvl::_Micron);
  savePvlDialog.setVoxelSize(vx, vy, vz);
  savePvlDialog.setDescription(desc);
  savePvlDialog.exec();

  int spread = savePvlDialog.volumeFilter();
  bool dilateFilter = savePvlDialog.dilateFilter();
  int voxelUnit = savePvlDialog.voxelUnit();
  QString description = savePvlDialog.description();
  savePvlDialog.voxelSize(vx, vy, vz);

  bool subsample = (svsl > 1 || svslz > 1);
  bool trim = false;

  QProgressDialog progress("Saving processed volume",
			   "Cancel",
			   0, 100,
			   0);
  progress.setMinimumDuration(0);

  //------------------------------
  int rvdepth, rvwidth, rvheight;    
  volData->gridSize(rvdepth, rvwidth, rvheight);
  int dmin = 0;
  int wmin = 0;
  int hmin = 0;
  int dmax = rvdepth-1;
  int wmax = rvwidth-1;
  int hmax = rvheight-1;
  int dsz=rvdepth;
  int wsz=rvwidth;
  int hsz=rvheight;

  uchar voxelType = volData->voxelType();  
  int headerBytes = volData->headerBytes();
      
  int bpv = 1;
  if (voxelType == _UChar) bpv = 1;
  else if (voxelType == _Char) bpv = 1;
  else if (voxelType == _UShort) bpv = 2;
  else if (voxelType == _Short) bpv = 2;
  else if (voxelType == _Int) bpv = 4;
  else if (voxelType == _Float) bpv = 4;
  
  //*** max 1Gb per slab
  int slabSize;
  slabSize = (1024*1024*1024)/(bpv*wsz*hsz);
  
  int dsz2 = dsz/svslz;
  int wsz2 = wsz/svsl;
  int hsz2 = hsz/svsl;

  QList<float> rawMap = volData->rawMap();
  QList<int> pvlMap = volData->pvlMap();
  
  int pvlbpv = 1;
  if (pvlMap[pvlMap.count()-1] > 255)
    pvlbpv = 2;
      
  int pvlVoxelType = 0;
  if (pvlbpv == 2) pvlVoxelType = 2;

  int nbytes = rvwidth*rvheight*bpv;
  double *filtervol = new double[wsz2*hsz2];
  uchar *pvlslice = new uchar[pvlbpv*wsz2*hsz2];
  uchar *raw = new uchar[nbytes];
  uchar **val;
  if (spread > 0)
    {
      val = new uchar*[2*spread+1];
      for (int i=0; i<2*spread+1; i++)
	val[i] = new uchar[nbytes];
    }
  int rawSize = rawMap.size()-1;
  int width = wsz2;
  int height = hsz2;
  //------------------------------

  
  //------------------------------------------------------
  int tsfcount = qMax(1, timeseriesFiles.count());
  bool vol4d = tsfcount > 0;
  for (int tsf=0; tsf<tsfcount; tsf++)
    {
      QString pvlflnm = pvlFilename;
      QString rawflnm = rawfile;

      if (tsfcount > 1)
	{
	  QFileInfo ftpvl(pvlFilename);
	  QFileInfo ftraw(timeseriesFiles[tsf]);
	  pvlflnm = QFileInfo(ftpvl.absolutePath(),
			      ftraw.completeBaseName() + ".pvl.nc").absoluteFilePath();

	  rawflnm = QFileInfo(ftpvl.absolutePath(),
			      ftraw.completeBaseName() + ".raw").absoluteFilePath();

	  volData->replaceFile(timeseriesFiles[tsf]);
	  //QStringList flnms;
	  //flnms << timeseriesFiles[tsf];
	  //volData->setFile(flnms, (tsf>0));
	  //volData->setFile(flnms, vol4d);
	}

      VolumeFileManager rawFileManager;
      VolumeFileManager pvlFileManager;

      pvlFileManager.setBaseFilename(pvlflnm);
      pvlFileManager.setDepth(dsz2);
      pvlFileManager.setWidth(wsz2);
      pvlFileManager.setHeight(hsz2);
      pvlFileManager.setVoxelType(pvlVoxelType);
      pvlFileManager.setHeaderSize(13);
      pvlFileManager.setSlabSize(slabSize);
      pvlFileManager.setSliceZeroAtTop(save0AtTop);
      pvlFileManager.createFile(true);
      
      if (saveRawFile)
	{
	  rawFileManager.setBaseFilename(rawflnm);
	  rawFileManager.setDepth(dsz2);
	  rawFileManager.setWidth(wsz2);
	  rawFileManager.setHeight(hsz2);
	  rawFileManager.setVoxelType(voxelType);
	  rawFileManager.setHeaderSize(13);
	  rawFileManager.setSlabSize(slabSize);
	  rawFileManager.setSliceZeroAtTop(save0AtTop);
	  rawFileManager.createFile(true);
	}
      //------------------------------------------------------


      savePvlHeader(pvlflnm,
		    saveRawFile, rawflnm,
		    voxelType, pvlVoxelType, voxelUnit,
		    dsz/svslz, wsz/svsl, hsz/svsl,
		    vx, vy, vz,
		    rawMap, pvlMap,
		    description,
		    slabSize);

      progress.setLabelText(pvlflnm);
      
      for(int dd=0; dd<dsz2; dd++)
	{
	  int d0 = dmin + dd*svslz; 
	  int d1 = d0 + svslz-1;
	  
	  progress.setValue((int)(100*(float)dd/(float)dsz2));
	  qApp->processEvents();
	  
	  memset(filtervol, 0, 8*wsz2*hsz2);
	  for (int d=d0; d<=d1; d++)
	    {
	      if (spread > 0)
		{
		  if (d == d0)
		    {
		      volData->getDepthSlice(d, val[spread]);
		      applyMeanFilterToSlice(val[spread], raw,
					     voxelType, rvwidth, rvheight,
					     spread, dilateFilter);

		      for(int i=-spread; i<0; i++)
			{
			  if (d+i >= 0)
			    volData->getDepthSlice(d+i, val[spread+i]);
			  else
			    volData->getDepthSlice(0, val[spread+i]);

			  applyMeanFilterToSlice(val[spread+i], raw,
						 voxelType, rvwidth, rvheight,
						 spread, dilateFilter);
			}
		      
		      for(int i=1; i<=spread; i++)
			{
			  if (d+i < rvdepth)
			    volData->getDepthSlice(d+i, val[spread+i]);
			  else
			    volData->getDepthSlice(rvdepth-1, val[spread+i]);

			  applyMeanFilterToSlice(val[spread+i], raw,
						 voxelType, rvwidth, rvheight,
						 spread, dilateFilter);
			}
		    }
		  else if (d < rvdepth-spread)
		    {
		      volData->getDepthSlice(d+spread, val[2*spread]);
		      applyMeanFilterToSlice(val[2*spread], raw,
					     voxelType, rvwidth, rvheight,
					     spread, dilateFilter);
		    }
		  else
		    {
		      volData->getDepthSlice(rvdepth-1, val[2*spread]);
		      applyMeanFilterToSlice(val[2*spread], raw,
					     voxelType, rvwidth, rvheight,
					     spread, dilateFilter);
		    }
		}
	      else
		volData->getDepthSlice(d, raw);
	      
	      if (spread > 0)
		{
		  applyMeanFilter(val, raw,
				  voxelType, rvwidth, rvheight,
				  spread, dilateFilter);
		  
		  // now shift the planes
		  uchar *tmp = val[0];
		  for(int i=0; i<2*spread; i++)
		    val[i] = val[i+1];
		  val[2*spread] = tmp;
		}
	      
	      if (trim || subsample)
		{
		  int fi = 0;
		  for(int j=0; j<wsz2; j++)
		    {
		      int y0 = wmin+j*svsl;
		      int y1 = y0+svsl-1;
		      for(int i=0; i<hsz2; i++)
			{
			  int x0 = hmin+i*svsl;
			  int x1 = x0+svsl-1;
			  for(int y=y0; y<=y1; y++)
			    for(int x=x0; x<=x1; x++)
			      {
				if (voxelType == _UChar)
				  { uchar *ptr = raw; filtervol[fi] += ptr[y*rvheight+x]; }
				else if (voxelType == _Char)
				  { char *ptr = (char*)raw; filtervol[fi] += ptr[y*rvheight+x]; }
				else if (voxelType == _UShort)
				  { ushort *ptr = (ushort*)raw; filtervol[fi] += ptr[y*rvheight+x]; }
				else if (voxelType == _Short)
				  { short *ptr = (short*)raw; filtervol[fi] += ptr[y*rvheight+x]; }
				else if (voxelType == _Int)
				  { int *ptr = (int*)raw; filtervol[fi] += ptr[y*rvheight+x]; }
				else if (voxelType == _Float)
				  { float *ptr = (float*)raw; filtervol[fi] += ptr[y*rvheight+x]; }
			      }
			  fi++;
			}
		    }
		} // trim || subsample
	    }
	  
	  if (trim || subsample)
	    {
	      if (subsample)
		{
		  for(int fi=0; fi<wsz2*hsz2; fi++)
		    filtervol[fi] /= svsl3;
		}
	      
	      if (voxelType == _UChar)
		{
		  uchar *ptr = raw;
		  for(int fi=0; fi<wsz2*hsz2; fi++)
		    ptr[fi] = filtervol[fi];
		}
	      else if (voxelType == _Char)
		{
		  char *ptr = (char*)raw;
		  for(int fi=0; fi<wsz2*hsz2; fi++)
		    ptr[fi] = filtervol[fi];
		}
	      else if (voxelType == _UShort)
		{
		  ushort *ptr = (ushort*)raw;
		  for(int fi=0; fi<wsz2*hsz2; fi++)
		    ptr[fi] = filtervol[fi];
		}
	      else if (voxelType == _Short)
		{
		  short *ptr = (short*)raw;
		  for(int fi=0; fi<wsz2*hsz2; fi++)
		    ptr[fi] = filtervol[fi];
		}
	      else if (voxelType == _Int)
		{
		  int *ptr = (int*)raw;
		  for(int fi=0; fi<wsz2*hsz2; fi++)
		    ptr[fi] = filtervol[fi];
		}
	      else if (voxelType == _Float)
		{
		  float *ptr = (float*)raw;
		  for(int fi=0; fi<wsz2*hsz2; fi++)
		    ptr[fi] = filtervol[fi];
		}
	    } // trim || subsample
	  
	  if (saveRawFile)
	    rawFileManager.setSlice(dd, raw);
	  
	  applyMapping(raw, voxelType, rawMap,
		       pvlslice, pvlbpv, pvlMap,
		       width, height);
	  
	  pvlFileManager.setSlice(dd, pvlslice);
	} // end of dd loop

      progress.setLabelText(QString("Processed %1 of %2").arg(tsf).arg(tsfcount));
    }

  delete [] filtervol;
  delete [] pvlslice;
  delete [] raw;
  if (spread > 0)
    {
      for (int i=0; i<2*spread+1; i++)
	delete [] val[i];
      delete [] val;
    }

  progress.setValue(100);
  
  QMessageBox::information(0, "Batch Processing", "-----Done-----");
}

void
Raw2Pvl::saveMHD(QString mhdFilename,
		 VolumeData* volData,
		 int dmin, int dmax,
		 int wmin, int wmax,
		 int hmin, int hmax)
{
  bool saveByteData = false;
  bool ok = false;
  QStringList slevels;
  slevels << "Yes - (default)";  
  slevels << "No - save byte-mapped data";
  QString option = QInputDialog::getItem(0,
		   "Save Original Data",
		   "Save Original Data in MetaImage Format  ?",
		    slevels,
			  0,
		      false,
		       &ok);
  if (ok)
    {
      QStringList op = option.split(' ');
      if (op[0] == "No")
	saveByteData = true;
    }
  

  //------------------------------------------------------
  int rvdepth, rvwidth, rvheight;    
  volData->gridSize(rvdepth, rvwidth, rvheight);

  int dsz=dmax-dmin+1;
  int wsz=wmax-wmin+1;
  int hsz=hmax-hmin+1;

  int svslz = getZSubsampling(dsz, wsz, hsz);
  int svsl = getXYSubsampling(svslz, dsz, wsz, hsz);

  int dsz2 = dsz/svslz;
  int wsz2 = wsz/svsl;
  int hsz2 = hsz/svsl;
  int svsl3 = svslz*svsl*svsl;

  uchar voxelType = volData->voxelType();  
  int headerBytes = volData->headerBytes();

  int bpv = 1;
  if (voxelType == _UChar) bpv = 1;
  else if (voxelType == _Char) bpv = 1;
  else if (voxelType == _UShort) bpv = 2;
  else if (voxelType == _Short) bpv = 2;
  else if (voxelType == _Int) bpv = 4;
  else if (voxelType == _Float) bpv = 4;
  //------------------------------------------------------

  //------------------------------------------------------
  // -- get saving parameters for processed file
  SavePvlDialog savePvlDialog;
  float vx, vy, vz;
  volData->voxelSize(vx, vy, vz);
  QString desc = volData->description();
  savePvlDialog.setVoxelUnit(Raw2Pvl::_Micron);
  savePvlDialog.setVoxelSize(vx, vy, vz);
  savePvlDialog.setDescription(desc);
  savePvlDialog.exec();

  int spread = savePvlDialog.volumeFilter();
  bool dilateFilter = savePvlDialog.dilateFilter();
  int voxelUnit = savePvlDialog.voxelUnit();
  QString description = savePvlDialog.description();
  savePvlDialog.voxelSize(vx, vy, vz);
  //------------------------------------------------------


  QString zrawFilename = mhdFilename;
  zrawFilename.chop(3);
  zrawFilename += "raw";
  
  if (QFile::exists(zrawFilename))
    {
      QString zfl = QFileDialog::getSaveFileName(0,
						 "Save raw volume",
						 Global::previousDirectory(),
						 "File (*.raw)",
						 0,
						 QFileDialog::DontUseNativeDialog);

      if (zfl.isEmpty())
	{
	  QStringList items;
	  items << "No";
	  items << "Yes";
	  QString item = QInputDialog::getItem(0,
					       "Overwrite existing file ?",
					       QString("Overwrite %1 ").arg(zrawFilename),
					       items,
					       0,
					       false,
					       &ok);
	  if (item == "No" || !ok)
	    return;
	}
      else
	zrawFilename = zfl;
      
      if (!zrawFilename.endsWith(".raw"))
	zrawFilename += ".raw";
    }

  
  {
    QFile mhd;
    mhd.setFileName(mhdFilename);
    mhd.open(QFile::WriteOnly | QFile::Text);
    QTextStream out(&mhd);
    out << "ObjectType = Image\n";
    out << "NDims = 3\n";
    out << "BinaryData = True\n";
    out << "BinaryDataByteOrderMSB = False\n";
    out << "CompressedData = False\n";
    out << "TransformMatrix = 1 0 0 0 1 0 0 0 1\n";
    out << "Offset = 0 0 0\n";
    out << "CenterOfRotation = 0 0 0\n";
    out << QString("ElementSpacing = %1 %2 %3\n").arg(vz).arg(vy).arg(vx);
    out << QString("DimSize = %1 %2 %3\n").arg(hsz2).arg(wsz2).arg(dsz2);
    out << "HeaderSize = 0\n";
    out << "AnatomicalOrientation = ???\n";
    if (saveByteData)
      out << "ElementType = MET_UCHAR\n";
    else
      {
	if (voxelType == _UChar)      out << "ElementType = MET_UCHAR\n";
	else if (voxelType == _Char)  out << "ElementType = MET_CHAR\n";
	else if (voxelType == _UShort)out << "ElementType = MET_USHORT\n";
	else if (voxelType == _Short) out << "ElementType = MET_SHORT\n";
	else if (voxelType == _Int)   out << "ElementType = MET_INT\n";
	else if (voxelType == _Float) out << "ElementType = MET_FLOAT\n";
      }
    QString rflnm = QFileInfo(zrawFilename).fileName();
    out << QString("ElementDataFile = %1\n").arg(rflnm);
  }

  {
    QFile zraw;
    zraw.setFileName(zrawFilename);
    zraw.open(QFile::WriteOnly);

    int nbytes = rvwidth*rvheight*bpv;
    double *filtervol = new double[wsz2*hsz2];
    uchar *pvl = new uchar[wsz2*hsz2];
    uchar *raw = new uchar[nbytes];
    uchar **val;
    if (spread > 0)
      {
	val = new uchar*[2*spread+1];
	for (int i=0; i<2*spread+1; i++)
	  val[i] = new uchar[nbytes];
      }
    int width = wsz2;
    int height = hsz2;
    bool subsample = (svsl > 1 || svslz > 1);
    bool trim = (dmin != 0 ||
		 wmin != 0 ||
		 hmin != 0 ||
		 dsz2 != rvdepth ||
		 wsz2 != rvwidth ||
		 hsz2 != rvheight);

    QList<float> rawMap = volData->rawMap();
    QList<int> pvlMap = volData->pvlMap();
    int rawSize = rawMap.size()-1;

    QProgressDialog progress("Saving MetaImage volume",
			     "Cancel",
			     0, 100,
			     0);
    progress.setMinimumDuration(0);
    
    for(int dd=0; dd<dsz2; dd++)
      {
	int d0 = dmin + dd*svslz; 
	int d1 = d0 + svslz-1;
	  
	progress.setValue((int)(100*(float)dd/(float)dsz2));
	qApp->processEvents();
	  
	memset(filtervol, 0, 8*wsz2*hsz2);
	for (int d=d0; d<=d1; d++)
	  {
	    if (spread > 0)
	      {
		if (d == d0)
		  {
		    volData->getDepthSlice(d, val[spread]);
		    applyMeanFilterToSlice(val[spread], raw,
					   voxelType, rvwidth, rvheight,
					   spread, dilateFilter);
		    
		    for(int i=-spread; i<0; i++)
		      {
			if (d+i >= 0)
			  volData->getDepthSlice(d+i, val[spread+i]);
			else
			  volData->getDepthSlice(0, val[spread+i]);
			
			applyMeanFilterToSlice(val[spread+i], raw,
					       voxelType, rvwidth, rvheight,
					       spread, dilateFilter);
		      }
		    
		    for(int i=1; i<=spread; i++)
		      {
			if (d+i < rvdepth)
			  volData->getDepthSlice(d+i, val[spread+i]);
			else
			  volData->getDepthSlice(rvdepth-1, val[spread+i]);
			
			applyMeanFilterToSlice(val[spread+i], raw,
					       voxelType, rvwidth, rvheight,
					       spread, dilateFilter);
		      }
		  }
		else if (d < rvdepth-spread)
		  {
		    volData->getDepthSlice(d+spread, val[2*spread]);
		    applyMeanFilterToSlice(val[2*spread], raw,
					   voxelType, rvwidth, rvheight,
					   spread, dilateFilter);
		  }		  
		else
		  {
		    volData->getDepthSlice(rvdepth-1, val[2*spread]);
		    applyMeanFilterToSlice(val[2*spread], raw,
					   voxelType, rvwidth, rvheight,
					   spread, dilateFilter);
		  }		  
	      }
	    else
	      volData->getDepthSlice(d, raw);
	    
	    if (spread > 0)
	      {
		applyMeanFilter(val, raw,
				voxelType, rvwidth, rvheight,
				spread, dilateFilter);
		
		// now shift the planes
		uchar *tmp = val[0];
		for(int i=0; i<2*spread; i++)
		  val[i] = val[i+1];
		val[2*spread] = tmp;
	      }
	    
	    if (trim || subsample)
	      {
		int fi = 0;
		for(int j=0; j<wsz2; j++)
		  {
		    int y0 = wmin+j*svsl;
		    int y1 = y0+svsl-1;
		    for(int i=0; i<hsz2; i++)
		      {
			int x0 = hmin+i*svsl;
			int x1 = x0+svsl-1;
			for(int y=y0; y<=y1; y++)
			  for(int x=x0; x<=x1; x++)
			    {
			      if (voxelType == _UChar)
				{ uchar *ptr = raw; filtervol[fi] += ptr[y*rvheight+x]; }
			      else if (voxelType == _Char)
				{ char *ptr = (char*)raw; filtervol[fi] += ptr[y*rvheight+x]; }
			      else if (voxelType == _UShort)
				{ ushort *ptr = (ushort*)raw; filtervol[fi] += ptr[y*rvheight+x]; }
			      else if (voxelType == _Short)
				{ short *ptr = (short*)raw; filtervol[fi] += ptr[y*rvheight+x]; }
			      else if (voxelType == _Int)
				{ int *ptr = (int*)raw; filtervol[fi] += ptr[y*rvheight+x]; }
			      else if (voxelType == _Float)
				{ float *ptr = (float*)raw; filtervol[fi] += ptr[y*rvheight+x]; }
			    }
			fi++;
		      }
		  }
	      } // trim || subsample
	  }
	
	if (trim || subsample)
	  {
	    if (subsample)
	      {
		for(int fi=0; fi<wsz2*hsz2; fi++)
		  filtervol[fi] /= svsl3;
	      }
	    
	    if (voxelType == _UChar)
	      {
		uchar *ptr = raw;
		for(int fi=0; fi<wsz2*hsz2; fi++)
		  ptr[fi] = filtervol[fi];
	      }
	    else if (voxelType == _Char)
	      {
		char *ptr = (char*)raw;
		for(int fi=0; fi<wsz2*hsz2; fi++)
		  ptr[fi] = filtervol[fi];
	      }
	    else if (voxelType == _UShort)
		{
		  ushort *ptr = (ushort*)raw;
		  for(int fi=0; fi<wsz2*hsz2; fi++)
		    ptr[fi] = filtervol[fi];
		}
	    else if (voxelType == _Short)
	      {
		short *ptr = (short*)raw;
		for(int fi=0; fi<wsz2*hsz2; fi++)
		  ptr[fi] = filtervol[fi];
	      }
	    else if (voxelType == _Int)
	      {
		int *ptr = (int*)raw;
		for(int fi=0; fi<wsz2*hsz2; fi++)
		  ptr[fi] = filtervol[fi];
	      }
	    else if (voxelType == _Float)
	      {
		float *ptr = (float*)raw;
		for(int fi=0; fi<wsz2*hsz2; fi++)
		  ptr[fi] = filtervol[fi];
	      }
	  } // trim || subsample
	
	if (!saveByteData) // save original volume
	  zraw.write((char*)raw, wsz2*hsz2*bpv);
	else
	  {
	    if (voxelType == _UChar)
	      {
		uchar *ptr = raw;
		REMAPVOLUME();
	      }
	    else if (voxelType == _Char)
	      {
		char *ptr = (char*)raw;
		REMAPVOLUME();
	      }
	    else if (voxelType == _UShort)
	      {
		ushort *ptr = (ushort*)raw;
		REMAPVOLUME();
	      }
	    else if (voxelType == _Short)
	      {
		short *ptr = (short*)raw;
		REMAPVOLUME();
	      }
	    else if (voxelType == _Int)
	      {
		int *ptr = (int*)raw;
		REMAPVOLUME();
	      }
	    else if (voxelType == _Float)
	      {
		float *ptr = (float*)raw;
		REMAPVOLUME();
	      }
	  
	    zraw.write((char*)pvl, wsz2*hsz2);
	  }
      }
    progress.setValue(100);

    delete [] filtervol;
    delete [] raw;
    delete [] pvl;
    if (spread > 0)
      {
	for (int i=0; i<2*spread+1; i++)
	  delete [] val[i];
	delete [] val;
      }    
  }
  
  QMessageBox::information(0, "Save MetaImage Volume", "-----Done-----");
}
