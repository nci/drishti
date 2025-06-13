//-------------------------
//adapted from -
//Author: William Silversmith
//Affiliation: Seung Lab, Princeton Neuroscience Insitute
//Date: July 2018
//https://github.com/seung-lab/euclidean-distance-transform-3d
//-------------------------

#include "binarydistancetransform.h"

#define sq(x) (static_cast<float>(x) * static_cast<float>(x))

//---------------------------
void
BinaryDistanceTransform::toFinite(float *f, const size_t voxels)
{
  for (size_t i = 0; i < voxels; i++)
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
BinaryDistanceTransform::toInfinite(float *f, const size_t voxels)
{
  for (size_t i = 0; i < voxels; i++)
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
BinaryDistanceTransform::squared_edt_1d_multi_seg(unsigned char* segids,
						  float *d,
						  const int n, 
						  const long int stride,
						  const float anistropy)
{  
  long int i;
  
  unsigned char working_segid = segids[0];

  d[0] = working_segid == 0 ? 0 : INFINITY;

  for (i = stride; i < n * stride; i += stride)
    {
      if (segids[i] == 0)
	{
	  d[i] = 0.0;
	}
    else if (segids[i] == working_segid)
      {
	d[i] = d[i - stride] + anistropy;
      }
    else
      {
	d[i] = anistropy;
	d[i - stride] = static_cast<float>(segids[i - stride] != 0) * anistropy;
	working_segid = segids[i];
      }
    }

  long int min_bound = 0;

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
						  const long int n, 
						  const long int stride, 
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
  for (long int i = 0; i < n; i++)
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
  for (long int i = 1; i < n; i++)
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
  for (long int i = 0; i < n; i++)
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
						  const long int stride, 
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
  for (long int i = 0; i < n; i++)
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
  for (long int i = 1; i < n; i++)
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
  for (long int i = 0; i < n; i++)
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
void BinaryDistanceTransform::_squared_edt_1d_parabolic(float* f, 
							const int n, 
							const long int stride, 
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
float*
BinaryDistanceTransform::binaryEDTsq(unsigned char* binaryimg, 
				     const size_t sx, const size_t sy, const size_t sz, 
				     float* workspace)
{
  const size_t sxy = sx * sy;
  const size_t voxels = sz * sxy;
  
  size_t x,y,z;

  if (workspace == NULL)
    {
      workspace = new float[sx*sy*sz]();
    }
  
  for (z = 0; z < sz; z++)
  for (y = 0; y < sy; y++)
    { 
      squared_edt_1d_multi_seg((binaryimg + sx * y + sxy * z), 
			       (workspace + sx * y + sxy * z), 
			       sx, 1, 1); 
    }
  
  toFinite(workspace, voxels);


  size_t offset;
  for (z = 0; z < sz; z++)
    {
      for (x = 0; x < sx; x++)
	{
	  offset = x + sxy * z;
	  for (y = 0; y < sy; y++)
	    {
	      if (workspace[offset + sx*y])
		{
		  break;
		}
	    }

	  _squared_edt_1d_parabolic((workspace + offset + sx * y),
				    sy - y, sx, 1, 
				    (y > 0), false);
	}
    }


  for (y = 0; y < sy; y++)
    {
      for (x = 0; x < sx; x++)
	{
	  offset = x + sx * y;
	  size_t z = 0;
	  for (z = 0; z < sz; z++)
	    {
	      if (workspace[offset + sxy*z])
		{
		  break;
		}
	    }
	  _squared_edt_1d_parabolic((workspace + offset + sxy * z), 
				    sz - z, sxy, 1, 
				    (z > 0), false);
	}
    }


  toInfinite(workspace, voxels);

  return workspace; 
}
