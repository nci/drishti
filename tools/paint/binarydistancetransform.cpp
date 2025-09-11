//-------------------------
//adapted from -
//Author: William Silversmith
//Affiliation: Seung Lab, Princeton Neuroscience Insitute
//Date: July 2018
//https://github.com/seung-lab/euclidean-distance-transform-3d
//-------------------------

#include <QProgressDialog>
#include <QtConcurrentMap>

#include "binarydistancetransform.h"

#define sq(x) (static_cast<float>(x) * static_cast<float>(x))

//---------------------------
void
BinaryDistanceTransform::toFinite(float *f, const qint64 voxels)
{
  for (qint64 i = 0; i < voxels; i++)
    {
      if (f[i] == INFINITY)
	{
	  f[i] = std::numeric_limits<float>::max() - 1;
	}
    }
}
//---------------------------


//---------------------------
void
BinaryDistanceTransform::toInfinite(float *f, const qint64 voxels)
{
  for (qint64 i = 0; i < voxels; i++)
    {
      if (f[i] >= std::numeric_limits<float>::max() - 1)
	{
	  f[i] = INFINITY;
	}
    }
}
//---------------------------


//---------------------------
void
BinaryDistanceTransform::squared_edt_1d_multi_seg(MyBitArray& bitmask,
						  qint64 bidx,
						  float *d,
						  const int n, 
						  const qint64 stride,
						  const float anistropy,
						  const bool black_border)
{  
  qint64 i;
  
  bool working_segid = bitmask.testBit(bidx);

  if (black_border)
    d[0] = working_segid != false ? anistropy : 0; // 0 or 1
  else
    d[0] = working_segid == false ? 0 : INFINITY;

  for (i = stride; i < n * stride; i += stride)
    {
      if (bitmask.testBit(bidx + i) == false)
	{
	  d[i] = 0.0;
	}
    else if (bitmask.testBit(bidx + i) == working_segid)
      {
	d[i] = d[i - stride] + anistropy;
      }
    else
      {
	d[i] = anistropy;
	d[i - stride] = bitmask.testBit(bidx + i - stride) != 0 ? anistropy : 0;
	working_segid = bitmask.testBit(bidx + i);
      }
    }

  qint64 min_bound = 0;

  if (black_border)
    {
      d[n - stride] = bitmask.testBit(bidx + n - stride) != 0 ? anistropy : 0;
      min_bound = stride;
    }

  for (i = (n - 2) * stride; i >= min_bound; i -= stride)
    {
      d[i] = std::fminf(d[i], d[i + stride] + anistropy);
    }

  for (i = 0; i < n * stride; i += stride)
    {
      d[i] *= d[i];
    }
}
//---------------------------


//---------------------------
void
BinaryDistanceTransform::squared_edt_1d_parabolic(float* f,
						  const qint64 n, 
						  const qint64 stride, 
						  const float anisotropy, 
						  const bool black_border_left,
						  const bool black_border_right)
{
  if (n == 0)
    {
      return;
    }

  const float w2 = anisotropy * anisotropy;

  int k = 0;
  std::unique_ptr<int[]> v(new int[n]());
  std::unique_ptr<float[]> ff(new float[n]());
  for (qint64 i = 0; i < n; i++)
    {
      ff[i] = f[i * stride];
    }
  
  std::unique_ptr<float[]> ranges(new float[n + 1]());

  ranges[0] = -INFINITY;
  ranges[1] = +INFINITY;

  /* Unclear if this adds much but I certainly find it easier to get the parens right.
   *
   * Eqn: s = ( f(r) + r^2 ) - ( f(p) + p^2 ) / ( 2r - 2p )
   * 1: s = (f(r) - f(p) + (r^2 - p^2)) / 2(r-p)
   * 2: s = (f(r) - r(p) + (r+p)(r-p)) / 2(r-p) <-- can reuse r-p, replace mult w/ add
   */
  float s;
  float factor1, factor2;
  for (qint64 i = 1; i < n; i++)
    {
      factor1 = (i - v[k]) * w2;
      factor2 =  i + v[k];
      s = (ff[i] - ff[v[k]] + factor1 * factor2) / (2.0 * factor1);
      
      while (k > 0 && s <= ranges[k])
	{
	  k--;
	  factor1 = (i - v[k]) * w2;
	  factor2 =  i + v[k];
	  s = (ff[i] - ff[v[k]] + factor1 * factor2) / (2.0 * factor1);
	}

      k++;
      v[k] = i;
      ranges[k] = s;
      ranges[k + 1] = +INFINITY;
    }

  k = 0;
  float envelope;
  for (qint64 i = 0; i < n; i++)
    {
      while (ranges[k + 1] < i)
	{ 
	  k++;
	}

      f[i * stride] = w2 * sq(i - v[k]) + ff[v[k]];
      // Two lines below only about 3% of perf cost, thought it would be more
      // They are unnecessary if you add a black border around the image.
      if (black_border_left && black_border_right)
	{
	  envelope = std::fminf(w2 * sq(i + 1), w2 * sq(n - i));
	  f[i * stride] = std::fminf(envelope, f[i * stride]);
	}
      else if (black_border_left)
	{
	  f[i * stride] = std::fminf(w2 * sq(i + 1), f[i * stride]);
	}
      else if (black_border_right)
	{
	  f[i * stride] = std::fminf(w2 * sq(n - i), f[i * stride]);      
	}
    }
}
//---------------------------


//---------------------------
// about 5% faster
void
BinaryDistanceTransform::squared_edt_1d_parabolic(float* f,
						  const int n, 
						  const qint64 stride, 
						  const float anisotropy)
{
  if (n == 0)
    {
      return;
    }

  const float w2 = anisotropy * anisotropy;

  int k = 0;
  std::unique_ptr<int[]> v(new int[n]());
  std::unique_ptr<float[]> ff(new float[n]());
  for (qint64 i = 0; i < n; i++)
    {
      ff[i] = f[i * stride];
    }

  std::unique_ptr<float[]> ranges(new float[n + 1]());

  ranges[0] = -INFINITY;
  ranges[1] = +INFINITY;

  /* Unclear if this adds much but I certainly find it easier to get the parens right.
   *
   * Eqn: s = ( f(r) + r^2 ) - ( f(p) + p^2 ) / ( 2r - 2p )
   * 1: s = (f(r) - f(p) + (r^2 - p^2)) / 2(r-p)
   * 2: s = (f(r) - r(p) + (r+p)(r-p)) / 2(r-p) <-- can reuse r-p, replace mult w/ add
   */
  float s;
  float factor1, factor2;
  for (qint64 i = 1; i < n; i++)
    {
      factor1 = (i - v[k]) * w2;
      factor2 = i + v[k];
      s = (ff[i] - ff[v[k]] + factor1 * factor2) / (2.0 * factor1);
      
      while (k > 0 && s <= ranges[k])
	{
	  k--;
	  factor1 = (i - v[k]) * w2;
	  factor2 = i + v[k];
	  s = (ff[i] - ff[v[k]] + factor1 * factor2) / (2.0 * factor1);
	}

      k++;
      v[k] = i;
      ranges[k] = s;
      ranges[k + 1] = +INFINITY;
    }

  k = 0;
  float envelope;
  for (qint64 i = 0; i < n; i++)
    {
      while (ranges[k + 1] < i)
	{ 
	  k++;
	}
      
      f[i * stride] = w2 * sq(i - v[k]) + ff[v[k]];
      // Two lines below only about 3% of perf cost, thought it would be more
      // They are unnecessary if you add a black border around the image.
      envelope = std::fminf(w2 * sq(i + 1), w2 * sq(n - i));
      f[i * stride] = std::fminf(envelope, f[i * stride]);
    }
}
//---------------------------



//---------------------------
void
BinaryDistanceTransform::_squared_edt_1d_parabolic(float* f, 
						   const int n, 
						   const qint64 stride, 
						   const float anisotropy, 
						   const bool black_border_left,
						   const bool black_border_right)
{
  
  if (black_border_left && black_border_right)
    {
      squared_edt_1d_parabolic(f, n, stride, anisotropy);
    }
  else
    {
      squared_edt_1d_parabolic(f, n, stride, anisotropy, black_border_left, black_border_right); 
    }
}
//---------------------------


//---------------------------
//---------------------------

void BinaryDistanceTransform::par_squared_edt_1d_multi_seg(QList<QVariant> plist)
{
  qint64 z = plist[0].toInt();
  qint64 y = plist[1].toInt();
  qint64 sx = plist[2].toInt();
  qint64 sy = plist[3].toInt();
  qint64 sz = plist[4].toInt();
  bool black_border = plist[5].toBool();
  float *workspace = static_cast<float*>(plist[6].value<void*>());
  MyBitArray *bitmask = static_cast<MyBitArray*>(plist[7].value<void*>());

  const qint64 sxy = sx * sy;

  squared_edt_1d_multi_seg(*bitmask,
			   sx * y + sxy * z,
			   (workspace + sx * y + sxy * z), 
			   sx, 1, 1,
			   black_border); 
}

void BinaryDistanceTransform::par_squared_edt_1d_parabolic_z(QList<QVariant> plist)
{
  qint64 z = plist[0].toInt();
  qint64 sx = plist[1].toInt();
  qint64 sy = plist[2].toInt();
  qint64 sz = plist[3].toInt();
  qint64 sxy = plist[4].toInt();
  bool black_border = plist[5].toBool();
  float *workspace = static_cast<float*>(plist[6].value<void*>());

  qint64 offset;
  for (qint64 x = 0; x < sx; x++)
    {
      offset = x + sxy * z;
      qint64 y = 0;
      for (y = 0; y < sy; y++)
	{
	  if (workspace[offset + sx*y] > 0)
	    {
	      break;
	    }
	}
      
      _squared_edt_1d_parabolic((workspace + offset + sx * y),
				sy - y, sx, 1,
				black_border || (y > 0), black_border);
    }
}

void BinaryDistanceTransform::par_squared_edt_1d_parabolic_y(QList<QVariant> plist)
{
  qint64 y = plist[0].toInt();
  qint64 sx = plist[1].toInt();
  qint64 sy = plist[2].toInt();
  qint64 sz = plist[3].toInt();
  qint64 sxy = plist[4].toInt();
  bool black_border = plist[5].toBool();
  float *workspace = static_cast<float*>(plist[6].value<void*>());

  qint64 offset;
  for (qint64 x = 0; x < sx; x++)
    {
      offset = x + sx * y;
      qint64 z = 0;
      for (z = 0; z < sz; z++)
	{
	  if (workspace[offset + sxy*z] > 0)
	    {
	      break;
	    }
	}
      _squared_edt_1d_parabolic((workspace + offset + sxy*z), 
				sz - z, sxy, 1, 
				black_border || (z > 0), black_border);
    }
}

//---------------------------
//---------------------------


//---------------------------
float*
BinaryDistanceTransform::binaryEDTsq(MyBitArray& bitmask, 
				     const qint64 sx, const qint64 sy, const qint64 sz,
				     const bool black_border,
				     float* workspace)
{
  const qint64 sxy = sx * sy;
  const qint64 voxels = sz * sxy;
  
  qint64 x,y,z;

  
  if (workspace == NULL)
    {
      workspace = new float[sx*sy*sz]();
    }

  
  //----------------------------
  // multihtreaded
  {
    QList<QList<QVariant>> param;
    for (z = 0; z < sz; z++)
    for (y = 0; y < sy; y++)
      {
	QList<QVariant> plist;
	plist << QVariant(z);
	plist << QVariant(y);
	plist << QVariant(sx);
	plist << QVariant(sy);
	plist << QVariant(sz);
	plist << QVariant(black_border);
	plist << QVariant::fromValue(static_cast<void*>(workspace));
	plist << QVariant::fromValue(static_cast<void*>(&bitmask));
	
	param << plist;
      }
    
    QFutureWatcher<void> futureWatcher;
    
    QProgressDialog progress(QString("distance transform yz : %1").arg(sy*sz),
			     QString(),
			     0, 100,
			     0,
			     Qt::WindowStaysOnTopHint);
    if (sz > 300)
      {
	progress.setMinimumDuration(0);
	QObject::connect(&futureWatcher, &QFutureWatcher<void>::finished, &progress, &QProgressDialog::reset);
	QObject::connect(&progress, &QProgressDialog::canceled, &futureWatcher, &QFutureWatcher<void>::cancel);
	QObject::connect(&futureWatcher,  &QFutureWatcher<void>::progressRangeChanged, &progress, &QProgressDialog::setRange);
	QObject::connect(&futureWatcher, &QFutureWatcher<void>::progressValueChanged,  &progress, &QProgressDialog::setValue);
      }

    futureWatcher.setFuture(QtConcurrent::map(param, par_squared_edt_1d_multi_seg)); 

    if (sz > 300)
      progress.exec();

    futureWatcher.waitForFinished();
  }
  //----------------------------
  
  
  toFinite(workspace, voxels);


  //----------------------------
  // multihtreaded
  {
    QList<QList<QVariant>> param;
    for(z=0; z<sz; z++)
      {
	QList<QVariant> plist;
	plist << QVariant(z);
	plist << QVariant(sx);
	plist << QVariant(sy);
	plist << QVariant(sz);
	plist << QVariant(sxy);
	plist << QVariant(black_border);
	plist << QVariant::fromValue(static_cast<void*>(workspace));
	
	param << plist;
      }
    
    QFutureWatcher<void> futureWatcher;
    
    QProgressDialog progress(QString("distance transform z : %1").arg(sz),
			     QString(),
			     0, 100,
			     0,
			     Qt::WindowStaysOnTopHint);
    if (sz > 300)
      {
	progress.setMinimumDuration(0);
	QObject::connect(&futureWatcher, &QFutureWatcher<void>::finished, &progress, &QProgressDialog::reset);
	QObject::connect(&progress, &QProgressDialog::canceled, &futureWatcher, &QFutureWatcher<void>::cancel);
	QObject::connect(&futureWatcher,  &QFutureWatcher<void>::progressRangeChanged, &progress, &QProgressDialog::setRange);
	QObject::connect(&futureWatcher, &QFutureWatcher<void>::progressValueChanged,  &progress, &QProgressDialog::setValue);
      }

    futureWatcher.setFuture(QtConcurrent::map(param, par_squared_edt_1d_parabolic_z)); 

    if (sz > 300)
      progress.exec();

    futureWatcher.waitForFinished();
  }
  //----------------------------
  

  //----------------------------
  // multihtreaded
  {
    QList<QList<QVariant>> param;
    for(y=0; y<sy; y++)
      {
	QList<QVariant> plist;
	plist << QVariant(y);
	plist << QVariant(sx);
	plist << QVariant(sy);
	plist << QVariant(sz);
	plist << QVariant(sxy);
	plist << QVariant(black_border);
	plist << QVariant::fromValue(static_cast<void*>(workspace));
	
	param << plist;
      }
    
    QFutureWatcher<void> futureWatcher;
    
    QProgressDialog progress(QString("distance transform y : %1").arg(sy),
			     QString(),
			     0, 100,
			     0,
			     Qt::WindowStaysOnTopHint);
    if (sy > 300)
      {
	progress.setMinimumDuration(0);
	QObject::connect(&futureWatcher, &QFutureWatcher<void>::finished, &progress, &QProgressDialog::reset);
	QObject::connect(&progress, &QProgressDialog::canceled, &futureWatcher, &QFutureWatcher<void>::cancel);
	QObject::connect(&futureWatcher,  &QFutureWatcher<void>::progressRangeChanged, &progress, &QProgressDialog::setRange);
	QObject::connect(&futureWatcher, &QFutureWatcher<void>::progressValueChanged,  &progress, &QProgressDialog::setValue);
      }

    futureWatcher.setFuture(QtConcurrent::map(param, par_squared_edt_1d_parabolic_y)); 

    if (sy > 300)
      progress.exec();

    futureWatcher.waitForFinished();
  }
  //----------------------------

  toInfinite(workspace, voxels);

  return workspace; 

  
  
//////----------------------------
////// Sequential run
//////----------------------------
//  for (z = 0; z < sz; z++)
//  for (y = 0; y < sy; y++)
//    { 
//      squared_edt_1d_multi_seg(bitmask,
//			       sx * y + sxy * z,
//			       (workspace + sx * y + sxy * z), 
//			       sx, 1, 1,
//			       black_border); 
//    }
//
//  qint64 offset;
//
//  for (z = 0; z < sz; z++)
//    {
//      for (x = 0; x < sx; x++)
//	{
//	  offset = x + sxy * z;
//	  for (y = 0; y < sy; y++)
//	    {
//	      if (workspace[offset + sx*y])
//		{
//		  break;
//		}
//	    }
//
//	  _squared_edt_1d_parabolic((workspace + offset + sx * y),
//				    sy - y, sx, 1,
//				    black_border || (y > 0), black_border);
//	}
//    }
//
//  progress.setValue(75);
//  qApp->processEvents();
//
//  for (y = 0; y < sy; y++)
//    {
//      for (x = 0; x < sx; x++)
//	{
//	  offset = x + sx * y;
//	  qint64 z = 0;
//	  for (z = 0; z < sz; z++)
//	    {
//	      if (workspace[offset + sxy*z])
//		{
//		  break;
//		}
//	    }
//	  _squared_edt_1d_parabolic((workspace + offset + sxy * z), 
//				    sz - z, sxy, 1, 
//				    black_border || (z > 0), black_border);
//	}
//    }
//////----------------------------
//////----------------------------
//
//  toInfinite(workspace, voxels);
//
//  return workspace; 
}
