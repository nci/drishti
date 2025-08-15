#ifndef BINARYDISTANCETRANSFORM
#define BINARYDISTANCETRANSFORM

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>

#include "mybitarray.h"

class BinaryDistanceTransform
{
 public :
  static float* binaryEDTsq(MyBitArray&,
			    const qint64, const qint64, const qint64,
			    const bool black_border=false,
			    float* workspace=NULL);

 private :
  static void toFinite(float*, const qint64);
  static void toInfinite(float*, const qint64);
  
  static void squared_edt_1d_multi_seg(MyBitArray&, qint64,
				       float*,
				       const int, 
				       const qint64,
				       const float,
				       const bool);
  
  static void squared_edt_1d_parabolic(float*,
				       const int, 
				       const qint64, 
				       const float);
  
  static void squared_edt_1d_parabolic(float*, 
				       const qint64, 
				       const qint64, 
				       const float,
				       const bool,
				       const bool);

  static void _squared_edt_1d_parabolic(float*, 
					const int, 
					const qint64, 
					const float,
					const bool,
					const bool);

  static void par_squared_edt_1d_parabolic_z(QList<QVariant>);
  static void par_squared_edt_1d_parabolic_y(QList<QVariant>);

};

#endif
