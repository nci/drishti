#include "matrix.h"
#include <stdlib.h>

void
Matrix::identity(double *xform)
{
  for(int i=0; i<16; i++)
    xform[i] = 0;

  xform[0] = 1;
  xform[5] = 1;
  xform[10] = 1;
  xform[15] = 1;
}

void
Matrix::matmult(double *A, double *B, double *C)
{
  for(int i=0; i<4; i++)
    for(int j=0; j<4; j++)
      {
	double sum = 0;
	for(int k=0; k<4; k++)
	  sum += A[i*4+k]*B[k*4+j];
	C[i*4+j] = sum;
      }
}

void
Matrix::matrixFromAxisAngle(double *m,
			    Vec a1, float angle)
{
  // axis assumed to be normalized
  // angle assumed to be in radians
    double c = cos(angle);
    double s = sin(angle);
    double t = 1.0 - c;

    m[0] = c + a1.x*a1.x*t;
    m[5] = c + a1.y*a1.y*t;
    m[10]= c + a1.z*a1.z*t;
    m[15] = 1;


    double tmp1 = a1.x*a1.y*t;
    double tmp2 = a1.z*s;
    m[4*1+0] = tmp1 + tmp2;
    m[4*0+1] = tmp1 - tmp2;
    tmp1 = a1.x*a1.z*t;
    tmp2 = a1.y*s;
    m[4*2+0] = tmp1 - tmp2;
    m[4*0+2] = tmp1 + tmp2;
    tmp1 = a1.y*a1.z*t;
    tmp2 = a1.x*s;
    m[4*2+1] = tmp1 + tmp2;
    m[4*1+2] = tmp1 - tmp2;
}

void
Matrix::createTransformationMatrix(double *xform,
				   Vec pos,
				   Vec pivot, Vec axis, float angle)
{
  double X[16], T0[16], T1[16], R[16], T[16];

  identity(T);
  identity(T0);
  identity(T1);
  identity(R);

  T[3] = pos.x;
  T[7] = pos.y;
  T[11]= pos.z;

  T0[3] = -pivot.x;
  T0[7] = -pivot.y;
  T0[11]= -pivot.z;
  
  T1[3] = pivot.x; 
  T1[7] = pivot.y; 
  T1[11]= pivot.z; 

  // axis assumed to be normalized
  // angle assumed to be in radians
  matrixFromAxisAngle(R, axis, angle);
  
  matmult(T, T1, xform);   // T*T1
  matmult(xform, R, X);    // T*T1*R
  matmult(X, T0, xform);   // T*T1*R*T0
}

Vec
Matrix::xformVec(double *xform, Vec v)
{
  Vec xv;

  xv.x = xform[0]*v[0] + xform[1]*v[1] + xform[2]*v[2] + xform[3];
  xv.y = xform[4]*v[0] + xform[5]*v[1] + xform[6]*v[2] + xform[7];
  xv.z = xform[8]*v[0] + xform[9]*v[1] + xform[10]*v[2]+ xform[11];

  return xv;
}

Vec
Matrix::rotateVec(double *xform, Vec v)
{
  Vec xv;

  xv.x = xform[0]*v[0] + xform[1]*v[1] + xform[2]*v[2];
  xv.y = xform[4]*v[0] + xform[5]*v[1] + xform[6]*v[2];
  xv.z = xform[8]*v[0] + xform[9]*v[1] + xform[10]*v[2];

  return xv;
}

void
Matrix::inverse(double *xform, double *xformInv)
{
  memset(xformInv, 0, 16*sizeof(double));

  double fA0 = xform[ 0]*xform[ 5] - xform[ 1]*xform[ 4];
  double fA1 = xform[ 0]*xform[ 6] - xform[ 2]*xform[ 4];
  double fA2 = xform[ 0]*xform[ 7] - xform[ 3]*xform[ 4];
  double fA3 = xform[ 1]*xform[ 6] - xform[ 2]*xform[ 5];
  double fA4 = xform[ 1]*xform[ 7] - xform[ 3]*xform[ 5];
  double fA5 = xform[ 2]*xform[ 7] - xform[ 3]*xform[ 6];
  double fB0 = xform[ 8]*xform[13] - xform[ 9]*xform[12];
  double fB1 = xform[ 8]*xform[14] - xform[10]*xform[12];
  double fB2 = xform[ 8]*xform[15] - xform[11]*xform[12];
  double fB3 = xform[ 9]*xform[14] - xform[10]*xform[13];
  double fB4 = xform[ 9]*xform[15] - xform[11]*xform[13];
  double fB5 = xform[10]*xform[15] - xform[11]*xform[14];

    double fDet = fA0*fB5-fA1*fB4+fA2*fB3+fA3*fB2-fA4*fB1+fA5*fB0;
    if (fabs(fDet) <= 0.00001)
      return;

    xformInv[ 0] =
        + xform[ 5]*fB5 - xform[ 6]*fB4 + xform[ 7]*fB3;
    xformInv[ 4] =
        - xform[ 4]*fB5 + xform[ 6]*fB2 - xform[ 7]*fB1;
    xformInv[ 8] =
        + xform[ 4]*fB4 - xform[ 5]*fB2 + xform[ 7]*fB0;
    xformInv[12] =
        - xform[ 4]*fB3 + xform[ 5]*fB1 - xform[ 6]*fB0;
    xformInv[ 1] =
        - xform[ 1]*fB5 + xform[ 2]*fB4 - xform[ 3]*fB3;
    xformInv[ 5] =
        + xform[ 0]*fB5 - xform[ 2]*fB2 + xform[ 3]*fB1;
    xformInv[ 9] =
        - xform[ 0]*fB4 + xform[ 1]*fB2 - xform[ 3]*fB0;
    xformInv[13] =
        + xform[ 0]*fB3 - xform[ 1]*fB1 + xform[ 2]*fB0;
    xformInv[ 2] =
        + xform[13]*fA5 - xform[14]*fA4 + xform[15]*fA3;
    xformInv[ 6] =
        - xform[12]*fA5 + xform[14]*fA2 - xform[15]*fA1;
    xformInv[10] =
        + xform[12]*fA4 - xform[13]*fA2 + xform[15]*fA0;
    xformInv[14] =
        - xform[12]*fA3 + xform[13]*fA1 - xform[14]*fA0;
    xformInv[ 3] =
        - xform[ 9]*fA5 + xform[10]*fA4 - xform[11]*fA3;
    xformInv[ 7] =
        + xform[ 8]*fA5 - xform[10]*fA2 + xform[11]*fA1;
    xformInv[11] =
        - xform[ 8]*fA4 + xform[ 9]*fA2 - xform[11]*fA0;
    xformInv[15] =
        + xform[ 8]*fA3 - xform[ 9]*fA1 + xform[10]*fA0;

    double fInvDet = 1.0/fDet;
    xformInv[ 0] *= fInvDet;
    xformInv[ 1] *= fInvDet;
    xformInv[ 2] *= fInvDet;
    xformInv[ 3] *= fInvDet;
    xformInv[ 4] *= fInvDet;
    xformInv[ 5] *= fInvDet;
    xformInv[ 6] *= fInvDet;
    xformInv[ 7] *= fInvDet;
    xformInv[ 8] *= fInvDet;
    xformInv[ 9] *= fInvDet;
    xformInv[10] *= fInvDet;
    xformInv[11] *= fInvDet;
    xformInv[12] *= fInvDet;
    xformInv[13] *= fInvDet;
    xformInv[14] *= fInvDet;
    xformInv[15] *= fInvDet;
}
