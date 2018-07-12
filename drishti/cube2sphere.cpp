#include "cube2sphere.h"
#include <QtMath>

Vec
Cube2Sphere::sphericalCoordinates(float i, float j, float w, float h)
{
  // Returns spherical coordinates of the pixel from the output image.
  float theta = 2*i/w-1;
  float phi = 2*j/h-1;
  // phi = lat, theta = long
  return Vec(phi*(M_PI/2), theta*M_PI, 0);
}

Vec
Cube2Sphere::vectorCoordinates(float phi, float theta)
{
  // Returns 3D vector which points to the pixel location inside a sphere.
  return Vec(qCos(phi) * qCos(theta), //X
	     qSin(phi),               // Y
	     qCos(phi) * qSin(theta));// Z
}

int
Cube2Sphere::getFace(Vec v)
{
  //  Uses 3D vector to find which cube face the pixel lies on.
  float largest_magnitude = qMax(qAbs(v.x), qMax(qAbs(v.y), qAbs(v.z)));

  if (largest_magnitude - qAbs(v.x) < 0.00001)
    {
      if (v.x < 0)
	return 4; // Front
      else
	return 5; // Back
    }

  if (largest_magnitude - qAbs(v.y) < 0.00001)
    {
      if (v.y < 0)
	return 2; // Bottom
      else
	return 3; // Top
    }

  if (largest_magnitude - qAbs(v.z) < 0.00001)
    {
      if (v.z < 0)
	return 0; // Right
      else
	return 1; // Left
    }
}

//FACE_Z_POS = 0  # Left
//FACE_Z_NEG = 1  # Right
//FACE_Y_POS = 2  # Top
//FACE_Y_NEG = 3  # Bottom
//FACE_X_NEG = 4  # Front
//FACE_X_POS = 5  # Back

Vec
Cube2Sphere::rawFaceCoordinates(int face, Vec v)
{
  // Return coordinates with necessary sign (- or +)
  // depending on which face they lie on.

  if (face == 0) return Vec(-v.x, v.y, v.z); // left
  if (face == 1) return Vec( v.x, v.y, v.z); // right

  if (face == 2) return Vec( v.z, v.x, v.y); // top
  if (face == 3) return Vec( v.z,-v.x, v.y); // bottom

  if (face == 4) return Vec( v.z, v.y, v.x); // front
  if (face == 5) return Vec(-v.z, v.y, v.x); // back
}

Vec
Cube2Sphere::rawCoordinates(Vec v)
{
  // Return 2D coordinates on the specified face relative
  // to the bottom-left corner of the face.
  
  return Vec((v.x/qAbs(v.z)+1)/2, (v.y/qAbs(v.z)+1)/2, 0);
}

Vec
Cube2Sphere::findCorrespondingPixel(int i, int j, int w, int h, int n)
{
  // Pixel coordinates for the input image that a
  // specified pixel in the output image maps to.
  // i: X coordinate of output image pixel
  // j: Y coordinate of output image pixel
  // w: Width of output image
  // h: Height of output image
  // n: Height/Width of each square face

  Vec spherical = Cube2Sphere::sphericalCoordinates(i, j, w, h);
  Vec vector_coords = Cube2Sphere::vectorCoordinates(spherical.x, spherical.y);
  int face = Cube2Sphere::getFace(vector_coords);
  Vec raw_face_coords = Cube2Sphere::rawFaceCoordinates(face, vector_coords);
  Vec cube_coords = Cube2Sphere::rawCoordinates(raw_face_coords);

  return Vec(face, cube_coords.x * n, cube_coords.y * n);
}

QImage
Cube2Sphere::convert(QList<QImage> cubeImages)
{
  // Right = FACE_Z_NEG
  // Left = FACE_Z_POS
  // Top = FACE_Y_POS
  // Bottom = FACE_Y_NEG
  // Front = FACE_X_NEG
  // Back = FACE_X_POS

  // square images required
  if (cubeImages[0].width() != cubeImages[0].height())
    return QImage();

  
  int imgSz = cubeImages[0].width();
  int outH = imgSz*1.333;
  int outW = 2*outH;
  
  QImage outImage(outW, outH, QImage::Format_ARGB32);
  uchar *bitOut = outImage.bits();
  
  // For each pixel in output image find colour value from input image
  for(int y=0; y<outH; y++)
    for(int x=0; x<outW; x++)
      {
	Vec crd = findCorrespondingPixel(x, y, outW, outH, imgSz);
	int face = crd.x;

	int inIdx = crd.z*imgSz + crd.y;
	int outIdx = y*outW + x;

	QRgb rgb = cubeImages[face].pixel(crd.y, crd.z);	
	bitOut[4*outIdx + 0] = qBlue(rgb);
	bitOut[4*outIdx + 1] = qGreen(rgb);
	bitOut[4*outIdx + 2] = qRed(rgb);
	bitOut[4*outIdx + 3] = qAlpha(rgb);
      }

  return outImage;
}
