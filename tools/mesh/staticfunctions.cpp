#include "staticfunctions.h"
#include "matrix.h"
#include "enums.h"
#include "../../common/src/mesh/binaryplywriter.h"
#include "mainwindowui.h"
#include "global.h"
#include "../../common/src/pvlmanifest.h"

#include <fstream>
using namespace std;

#include <QColorDialog>
#include <QInputDialog>
#include <QUrl>
#include <QFileInfo>
#include <QDir>
#include <QDomDocument>
#include <QTextStream>
#include <QFileDialog>
#include <QPainterPath>
#include <QSaveFile>

#include <limits>

namespace
{
const qint64 c_megabyte = 1024LL*1024LL;
const int c_reservedTextureMemoryMB = 32;

bool
validTextureDimension(double value, qint64 &dimension)
{
  if (!qIsFinite(value) ||
      value < 1.0 ||
      value > static_cast<double>(std::numeric_limits<int>::max()))
    return false;

  dimension = static_cast<qint64>(value);
  return dimension > 0;
}

qint64
sampledTextureDimension(qint64 dimension, int lod)
{
  return qMax<qint64>(1, dimension/qMax(1, lod));
}

bool
fitsTextureBudget(qint64 nx, qint64 ny, qint64 nz,
		  int lod, qint64 arrayTextureSize,
		  quint64 voxelCapacity)
{
  const qint64 x = sampledTextureDimension(nx, lod);
  const qint64 y = sampledTextureDimension(ny, lod);
  const qint64 z = sampledTextureDimension(nz, lod);

  if (z > arrayTextureSize)
    return false;

  // Compare by division so x*y*z cannot overflow before the budget check.
  if (static_cast<quint64>(x) > voxelCapacity)
    return false;
  voxelCapacity /= static_cast<quint64>(x);

  if (static_cast<quint64>(y) > voxelCapacity)
    return false;
  voxelCapacity /= static_cast<quint64>(y);

  return static_cast<quint64>(z) <= voxelCapacity;
}
}

Vec
StaticFunctions::getVec(QString str)
{
  QStringList xyz = str.split(" ", QString::SkipEmptyParts);
  bool ok;
  float x=0,y=0,z=0;
  if (xyz.size() > 0) x = xyz[0].toFloat(&ok);
  if (xyz.size() > 1) y = xyz[1].toFloat(&ok);
  if (xyz.size() > 2) z = xyz[2].toFloat(&ok);

  Vec val;
  val = Vec(x,y,z);
  return val;
}

int
StaticFunctions::getPowerOf2(int val)
{
  int p, q;
  p = 1;
  q = 0;
  while (val > p)
    {
      p*=2;
      q++;
    }
  return q;
}

Vec
StaticFunctions::interpolate(Vec a, Vec b, float frc)
{
  Vec c;

  if ((a-b).squaredNorm()<0.0001f)
    c = a;
  else
    c = a + frc*(b-a);

  return c;
}

Vec
StaticFunctions::clampVec(Vec minv, Vec maxv, Vec v)
{
  Vec cv;
  cv.x = qBound(minv.x, v.x, maxv.x);
  cv.y = qBound(minv.y, v.y, maxv.y);
  cv.z = qBound(minv.z, v.z, maxv.z);

  return cv;
}

Vec
StaticFunctions::maxVec(Vec a, Vec b)
{
  Vec cv;
  cv.x = qMax(a.x, b.x);
  cv.y = qMax(a.y, b.y);
  cv.z = qMax(a.z, b.z);

  return cv;
}

Vec
StaticFunctions::minVec(Vec a, Vec b)
{
  Vec cv;
  cv.x = qMin(a.x, b.x);
  cv.y = qMin(a.y, b.y);
  cv.z = qMin(a.z, b.z);

  return cv;
}

int
StaticFunctions::getSubsamplingLevel(int tms, int textureSize,
				     int bytesPerVoxel,
				     Vec boxMin, Vec boxMax)
{
  const Vec subvolumeSize = boxMax - boxMin + Vec(1,1,1);
  qint64 nx, ny, nz;
  if (!validTextureDimension(subvolumeSize.x, nx) ||
      !validTextureDimension(subvolumeSize.y, ny) ||
      !validTextureDimension(subvolumeSize.z, nz))
    return 1;

  const qint64 usableMemoryMB =
    qMax<qint64>(1,
		static_cast<qint64>(tms) - c_reservedTextureMemoryMB);
  const quint64 bytes =
    static_cast<quint64>(usableMemoryMB)*static_cast<quint64>(c_megabyte);
  const quint64 voxelCapacity =
    bytes/static_cast<quint64>(qMax(1, bytesPerVoxel));
  const qint64 layerLimit = qMax<qint64>(1, textureSize);

  int low = 1;
  int high = static_cast<int>(qMax(nx, qMax(ny, nz)));
  while (low < high)
    {
      const int lod = low + (high-low)/2;
      if (fitsTextureBudget(nx, ny, nz, lod,
			    layerLimit, voxelCapacity))
	high = lod;
      else
	low = lod+1;
    }

  return low;
}



void
StaticFunctions::getRotationBetweenVectors(Vec p, Vec q,
					   Vec &axis, float &angle)
{
  axis = p^q;

  if (axis.norm() == 0) // parallel vectors
    {
      axis = p;
      angle = 0;
      return;
    }

  axis.normalize();

  float cost = (p*q)/(p.norm()*q.norm());
  cost = qMax(-1.0f, qMin(1.0f, cost));
  angle = acos(cost);
}


QGradientStops
StaticFunctions::resampleGradientStops(QGradientStops stops, int mapSize)
{
  //QColor colorMap[1001];

  //mapSize = qMin(1000, mapSize);

  QVector<QColor> colorMap;
  colorMap.resize(mapSize);
  
  
  int startj, endj;
  for(int i=0; i<stops.size(); i++)
    {
      float pos = stops[i].first;
      QColor color = stops[i].second;
      endj = pos*mapSize;
      colorMap[endj] = color;
      if (i > 0)
	{
	  QColor colStart, colEnd;
	  colStart = colorMap[startj];
	  colEnd = colorMap[endj];
	  float rb,gb,bb,ab, re,ge,be,ae;
	  rb = colStart.red();
	  gb = colStart.green();
	  bb = colStart.blue();
	  ab = colStart.alpha();
	  re = colEnd.red();
	  ge = colEnd.green();
	  be = colEnd.blue();
	  ae = colEnd.alpha();
	  for (int j=startj+1; j<endj; j++)
	    {
	      float frc = (float)(j-startj)/(float)(endj-startj);
	      float r,g,b,a;
	      r = rb + frc*(re-rb);
	      g = gb + frc*(ge-gb);
	      b = bb + frc*(be-bb);
	      a = ab + frc*(ae-ab);
	      colorMap[j] = QColor(r, g, b, a);
	    }
	}
      startj = endj;
    }

  QGradientStops newStops;
  for (int i=0; i<mapSize; i++)
    {
      float pos = (float)i/(float)(mapSize-1);
      newStops << QGradientStop(pos, colorMap[i]);
    }

  return newStops;
}

void
StaticFunctions::initQColorDialog()
{
  int i;
  
  int nNodes = 16;
  int Nodes[] = { 0,   4,   8,  12,  16,  20,  23,  24,  28,  32,  36,  40,  44,  45,  46,  47 };
  int RED[] = { 255, 255,   0,   0,   0, 255, 255, 255, 255, 128, 255, 128, 128,   0, 128, 255 };
  int GREEN[]={   0, 255, 255, 255,   0,   0,   0, 255, 128, 255, 128, 255, 128,   0, 128, 255 };
  int BLUE[] ={   0,   0,   0, 255, 255, 255,  64, 128, 255, 255, 128, 128, 255,   0, 128, 255 };

  int ai;
  for(i = 0; i < 48; i++)
    {
      ai = i;
      if (ai >= 8 && ai < 16) ai = 15 - ai%8;
      else if (ai >= 32 && ai < 40) ai = 39 - ai%32;
      
      int n, r, g, b;
      for(n=nNodes-1; n>=0; n--)
	if (ai >= Nodes[n])
	  break;
      
      r = RED[n];
      g = GREEN[n];
      b = BLUE[n];
      if (n < nNodes-1)
	{
	  float frc;
	  frc = Nodes[n+1]-Nodes[n];
	  frc = (ai-Nodes[n])/frc;
	  r = (int)(r + frc*(RED[n+1]-r));
	  g = (int)(g + frc*(GREEN[n+1]-g));
	  b = (int)(b + frc*(BLUE[n+1]-b));
	}

      int x, y;
      y = i%8;
      x = i/8;
      
      QColorDialog::setStandardColor(y*6+x, qRgb(r,g,b));
    }
}


void
StaticFunctions::drawEnclosingCube(Vec bmin, Vec bmax, bool allFaces)
{
//  glEnable(GL_LINE_SMOOTH);  // antialias lines	
//  glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

  Vec subvol[8];
  subvol[0] = Vec(bmin.x, bmin.y, bmax.z);
  subvol[1] = Vec(bmax.x, bmin.y, bmax.z);
  subvol[2] = Vec(bmax.x, bmax.y, bmax.z);
  subvol[3] = Vec(bmin.x, bmax.y, bmax.z);
  subvol[4] = Vec(bmin.x, bmin.y, bmin.z);
  subvol[5] = Vec(bmax.x, bmin.y, bmin.z);
  subvol[6] = Vec(bmax.x, bmax.y, bmin.z);
  subvol[7] = Vec(bmin.x, bmax.y, bmin.z);  

  int indices[] = {0, 1, 2, 3,   // 24 of indices
		   1, 5, 6, 2,
		   7, 6, 5, 4,
		   4, 0, 3, 7,
		   4, 5, 1, 0,
		   3, 2, 6, 7};
		   
		   
  for (int i=0; i<6; i++)
    {
      glBegin(GL_QUADS);
      int v0 = indices[4*i];
      int v1 = indices[4*i+1];
      int v2 = indices[4*i+2];
      int v3 = indices[4*i+3];
      glVertex3f(subvol[v0].x,subvol[v0].y,subvol[v0].z);
      glVertex3f(subvol[v1].x,subvol[v1].y,subvol[v1].z);
      glVertex3f(subvol[v2].x,subvol[v2].y,subvol[v2].z);
      glVertex3f(subvol[v3].x,subvol[v3].y,subvol[v3].z);
      glEnd();
    }
}

void
StaticFunctions::drawAxis(Vec origin,
			  Vec xAxis, Vec yAxis, Vec zAxis,
			  bool bothSides,
			  int lw)
{
  Vec x0, y0, z0;
  Vec x,y,z;

  if (!bothSides)
    {
      x0 = origin;
      y0 = origin;
      z0 = origin;
    }
  else
    {
      x0 = origin-xAxis;
      y0 = origin-yAxis;
      z0 = origin-zAxis;
    }
  x = origin+xAxis;
  y = origin+yAxis;
  z = origin+zAxis;
  glLineWidth(lw);
  glColor4f(0.9f, 0.1f, 0.1f, 0.9f);
  Vec x9 = 0.9*x + 0.1*x0;
  glBegin(GL_LINES);
  glVertex3fv(x0);
  glVertex3fv(x);
  glEnd();
  glColor4f(0.1f, 0.9f, 0.1f, 0.9f);
  glBegin(GL_LINES);
  glVertex3fv(y0);
  glVertex3fv(y);
  glEnd();
  glColor4f(0.1f, 0.2f, 0.9f, 0.9f);
  glBegin(GL_LINES);
  glVertex3fv(z0);
  glVertex3fv(z);
  glEnd();
}

void
StaticFunctions::drawEnclosingCube(Vec *subvol, Vec line_color)
{
  glColor4f(line_color.x, line_color.y, line_color.z, 0.9f);
//  glEnable(GL_LINE_SMOOTH);  // antialias lines	
//  glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

  glBegin(GL_LINES);

  for(int i=0; i<4; i++)
    {
      int j;
      j = (i==3)?0:i+1;

      glVertex3f(subvol[i].x, subvol[i].y, subvol[i].z);
      glVertex3f(subvol[j].x, subvol[j].y, subvol[j].z);
    }

  for(int i=4; i<8; i++)
    {
      int j;
      j = (i==7)?4:i+1;

      glVertex3f(subvol[i].x, subvol[i].y, subvol[i].z);
      glVertex3f(subvol[j].x, subvol[j].y, subvol[j].z);
    }

  for(int i=0; i<4; i++)
    {
      glVertex3f(subvol[i].x, subvol[i].y, subvol[i].z);
      glVertex3f(subvol[i+4].x, subvol[i+4].y, subvol[i+4].z);
    }

  glEnd();
}

void
StaticFunctions::drawEnclosingCubeWithTransformation(Vec bmin,
						     Vec bmax,
						     double *Xform,
						     Vec line_color)
{
  Vec subvol[8];
  subvol[0] = Vec(bmin.x, bmin.y, bmin.z);
  subvol[1] = Vec(bmax.x, bmin.y, bmin.z);
  subvol[2] = Vec(bmax.x, bmax.y, bmin.z);
  subvol[3] = Vec(bmin.x, bmax.y, bmin.z);
  subvol[4] = Vec(bmin.x, bmin.y, bmax.z);
  subvol[5] = Vec(bmax.x, bmin.y, bmax.z);
  subvol[6] = Vec(bmax.x, bmax.y, bmax.z);
  subvol[7] = Vec(bmin.x, bmax.y, bmax.z);

  for (int i=0; i<8; i++)
    subvol[i] = Matrix::xformVec(Xform, subvol[i]);

  StaticFunctions::drawEnclosingCube(subvol, line_color);
}




int
StaticFunctions::getScaledown(int scldn, int nX)
{
  int ttct = 2*scldn-1;
  int ct=0;
  int snx = 0;
  for(int i=0; i<nX; i++)
    {
      ct ++;
      if (ct == ttct)
	{
	  snx ++;
	  ct = ttct/2;
	}
    }
  return snx;
}

void
StaticFunctions::pushOrthoView(float x, float y, float width, float height)
{
  glViewport(x,y, width, height);
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho(0,width, 0,height, -1,1);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
}

void
StaticFunctions::popOrthoView()
{
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
}

void
StaticFunctions::drawQuad(float xmin, float ymin,
			  float xmax, float ymax,
			  float scale)
{
  glBegin(GL_QUADS);
  glTexCoord2f(xmin*scale, ymin*scale); glVertex2f(xmin, ymin);
  glTexCoord2f(xmax*scale, ymin*scale); glVertex2f(xmax, ymin);
  glTexCoord2f(xmax*scale, ymax*scale); glVertex2f(xmax, ymax);
  glTexCoord2f(xmin*scale, ymax*scale); glVertex2f(xmin, ymax);
  glEnd();
}

bool
StaticFunctions::checkExtension(QString flnm, const char *ext)
{
  bool ok = true;
  int extlen = strlen(ext);

//  QFileInfo info(flnm);
//  if (info.exists() && info.isFile())
    {
      QByteArray exten = flnm.toLatin1().right(extlen).toLower();
      if (exten != ext)
	ok = false;
    }
//  else
//    ok = false;

  return ok;
}

bool
StaticFunctions::checkURLs(QList<QUrl> urls, const char *ext)
{
  bool ok = true;
  int extlen = strlen(ext);

  for(int i=0; i<urls.count(); i++)
    {
      QUrl url = urls[i];
      QFileInfo info(url.toLocalFile());
      if (info.exists() && info.isFile())
	{
	  QByteArray exten = url.toLocalFile().toLatin1().right(extlen).toLower();
	  if (exten != ext)
	    {
	      ok = false;
	      break;
	    }
	}
      else
	{
	  ok = false;
	  break;
	}
    }
  return ok;
}


bool
StaticFunctions::checkExtension(QString flnm, QStringList extensions)
{
  foreach(QString ext, extensions)
    {
      int extlen = ext.count();
      QString exten = flnm.right(extlen).toLower();
      if (exten == ext)
	return true;
    }

  return false;
}
bool
StaticFunctions::checkURLs(QList<QUrl> urls, QStringList extensions)
{
  for(int i=0; i<urls.count(); i++)
    {
      QUrl url = urls[i];
      QFileInfo info(url.toLocalFile());
      if (info.exists() && info.isFile())
	{
	  foreach(QString ext, extensions)
	    {
	      int extlen = ext.count();
	      QString exten = url.toLocalFile().right(extlen).toLower();
	      if (exten == ext)
		return true;
	    }
	}
    }
  return false;
}

float
StaticFunctions::calculateAngle(Vec p0, Vec p1, Vec p2)
{
  Vec a = p0-p1;
  Vec b = p2-p1;

  if (a.norm() > 0)
    a.normalize();
  else
    return 0;

  if (b.norm() > 0)
    b.normalize();
  else
    return 0;

  float angle = atan2((a^b).norm(), a*b);

//  float cost = a*b;
//  cost = qMax(-1.0f, qMin(1.0f, cost));
//  float angle = acos(cost);
  
  return RAD2DEG(angle);
}



		     
bool
StaticFunctions::getClip(Vec po,
			 QList<Vec> clipPos,
			 QList<Vec> clipNormal)
{
  if (clipPos.size() == 0)
    return true;

  bool ok = true;
  for(int ci=0; ci<clipPos.size(); ci++)
    {
      Vec cpo = clipPos[ci];
      Vec cpn = clipNormal[ci];
      if ((po-cpo)*cpn > 0)
	{
	  ok = false;
	  break;
	}
    }
  return ok;
}

QSize
StaticFunctions::getImageSize(int width, int height)
{
  QSize imgSize = QSize(width, height);

  QStringList items;
  items << QString("%1 %2 (Current)").arg(width).arg(height);
  items << "640 480 (VGA)";
  items << "720 480 (NTSC)";
  items << "720 576 (PAL)";
  items << "800 480 (WVGA)";
  items << "800 600 (SVGA)";
  items << "1024 600 (WSVGA)";
  items << "1024 768 (XGA)";
  items << "1280 720 (720p 16:9)";
  items << "1280 768 (WXGA)";
  items << "1280 1024 (SXGA)";
  items << "1400 1050 (SXGA+)";
  items << "1600 1200 (UXGA)";
  items << "1920 1080 (1080p 16:9)";
  items << "1920 1200 (WUXGA)";

  bool ok;
  QString str;
  str = QInputDialog::getItem(0,
			      "Image size",
			      "Image Size",
			      items,
			      0,
			      true, // text is editable
			      &ok);
  
  if (ok)
    {
      QStringList strlist = str.split(" ", QString::SkipEmptyParts);
      int x=0, y=0;
      if (strlist.count() > 1)
	{
	  x = strlist[0].toInt(&ok);
	  y = strlist[1].toInt(&ok);
	}

      if (x > 0 && y > 0)
	imgSize = QSize(x, y);
      else
	QMessageBox::critical(0, "Image Size",
			      "Image Size improperly set : "+str);
    }
  
  return imgSize;
}


float
StaticFunctions::easeIn(float a)
{
  return a*a;
}

float
StaticFunctions::easeOut(float a)
{
  float b = (1-a);
  return 1.0f-b*b;
}

float
StaticFunctions::smoothstep(float a)
{
  return a*a*(3.0f-2.0f*a);
}

float
StaticFunctions::smoothstep(float min, float max, float v)
{
  if (v <= min) return 0.0f;
  if (v >= max) return 1.0f;
  float a = (v-min)/(max-min);
  return a*a*(3.0f-2.0f*a);
}



float
StaticFunctions::remapKeyframe(int type, float a)
{
  if (type == Enums::KFIT_Linear)
    return a;
  else if (type == Enums::KFIT_SmoothStep)
    return smoothstep(a);
  else if (type == Enums::KFIT_EaseIn)
    return easeIn(a);
  else if (type == Enums::KFIT_EaseOut)
    return easeOut(a);
  else
    return 0;
  
  return a;
}

QVector<Vec>
StaticFunctions::generateUnitSphere(int iterations)
{
  QVector<Vec> tri;

   int i, it;
   double a;
   Vec p[6];
   Vec pa, pb, pc;

   p[0] = Vec(0,0,1);
   p[1] = Vec(0,0,-1);
   p[2] = Vec(-1,-1,0);
   p[3] = Vec(1,-1,0);
   p[4] = Vec(1,1,0);
   p[5] = Vec(-1,1,0);

   /* Create the level 0 object */
   a = 1 / sqrt(2.0);
   for (i=0;i<6;i++)
     {
       p[i].x *= a;
       p[i].y *= a;
     }

   tri.append(p[0]); tri.append(p[3]); tri.append(p[4]);
   tri.append(p[0]); tri.append(p[4]); tri.append(p[5]);
   tri.append(p[0]); tri.append(p[5]); tri.append(p[2]);
   tri.append(p[0]); tri.append(p[2]); tri.append(p[3]);
   tri.append(p[1]); tri.append(p[4]); tri.append(p[3]);
   tri.append(p[1]); tri.append(p[5]); tri.append(p[4]);
   tri.append(p[1]); tri.append(p[2]); tri.append(p[5]);
   tri.append(p[1]); tri.append(p[3]); tri.append(p[2]);

   if (iterations < 1)
     return tri;

   int nt = 8;

   /* Bisect each edge and move to the surface of a unit sphere */
   for (it=0;it<iterations;it++)
     {
       int ntold = nt;
       for (i=0;i<ntold;i++)
	 {
	   int i0 = 3*i+0;
	   int i1 = 3*i+1;
	   int i2 = 3*i+2;

	   pa = (tri[i0]+tri[i1])/2;
	   pb = (tri[i1]+tri[i2])/2;
	   pc = (tri[i2]+tri[i0])/2;
	   
	   pa.normalize();
	   pb.normalize();
	   pc.normalize();
	   
	   
	   tri.append(tri[i0]); tri.append(pa); tri.append(pc);
	   tri.append(pa); tri.append(tri[i1]); tri.append(pb);
	   tri.append(pb); tri.append(tri[i2]); tri.append(pc);

	   // replace the split triangle
	   tri[i0] = pa; tri[i1] = pb; tri[i2] = pc;
	 }
     }

   return tri;
}

bool
StaticFunctions::inTriangle(Vec a, Vec b, Vec c, Vec p)
{
  Vec v0 = c - a;
  Vec v1 = b - a;
  Vec v2 = p - a;

  float dot00 = v0 * v0;
  float dot01 = v0 * v1;
  float dot02 = v0 * v2;
  float dot11 = v1 * v1;
  float dot12 = v1 * v2;

  // compute barycentric coordinates
  float invdenom = 1.0 / (dot00 * dot11 - dot01 * dot01);
  float u = (dot11 * dot02 - dot01 * dot12) * invdenom;
  float v = (dot00 * dot12 - dot01 * dot02) * invdenom;
  
  // check if point is in triangle
  return (u > 0) && (v > 0) && (u + v < 1);
}

QString
StaticFunctions::replaceDirectory(QString dir, QString flnm)
{
  if (!dir.isEmpty()) // save file to temporary directory
    {
      QFileInfo fi(flnm);
      QString lflnm = fi.fileName();
      fi = QFileInfo(QDir(dir), lflnm);
      return fi.filePath();
    }  
  return flnm;
}

void // Qt version
StaticFunctions::convertFromGLImage(QImage &img, int w, int h)
{
  if (QSysInfo::ByteOrder == QSysInfo::BigEndian)
    {
      // OpenGL gives RGBA; Qt wants ARGB
      uint *p = (uint*)img.bits();
      uint *end = p + w*h;
      while (p < end)
	{
	  uint a = *p << 24;
	  *p = (*p >> 8) | a;
	  p++;
	}
      // This is an old legacy fix for PowerPC based Macs, which
      // we shouldn't remove
      while (p < end)
	{
	  *p = 0xff000000 | (*p>>8);
	  ++p;
	}
    }
  else
    {
      // OpenGL gives ABGR (i.e. RGBA backwards); Qt wants ARGB
      QImage res = img.rgbSwapped();
      img = res;
    }
  img = img.mirrored();
}

void
StaticFunctions::savePvlHeader(QString pvlFilename,
			       bool saveRawFile, QString rawfile,
			       int voxelType, int pvlVoxelType, int voxelUnit,
			       int d, int w, int h,
			       float vx, float vy, float vz,
		       QList<float> rawMap, QList<int> pvlMap,
		       QString description,
		       int slabSize)
{
  if (d <= 0 || w <= 0 || h <= 0 || slabSize <= 0)
    return;

  const auto voxelTypeName = [](int type) {
    switch (type)
      {
      case 0: return QString("unsigned char");
      case 1: return QString("char");
      case 2: return QString("unsigned short");
      case 3: return QString("short");
      case 4: return QString("int");
      case 5: return QString("float");
      default: return QString();
      }
  };
  const QString rawType = voxelTypeName(voxelType);
  const QString pvlType = voxelTypeName(pvlVoxelType);
  if (rawType.isEmpty() || pvlType.isEmpty())
    return;

  QDomDocument doc("Drishti_Header");
  QDomElement root = doc.createElement("PvlDotNcFileHeader");
  doc.appendChild(root);

  auto addText = [&doc, &root](const QString& tag, const QString& value) {
    QDomElement element = doc.createElement(tag);
    element.appendChild(doc.createTextNode(value));
    root.appendChild(element);
  };

  addText("rawfile", saveRawFile ?
          QFileInfo(pvlFilename).absoluteDir().relativeFilePath(rawfile) :
          QString());
  addText("voxeltype", rawType);
  addText("pvlvoxeltype", pvlType);
  addText("gridsize", QString("%1 %2 %3").arg(d).arg(w).arg(h));
  addText("voxelsize", QString("%1 %2 %3").arg(vx).arg(vy).arg(vz));
  addText("description", description.trimmed());
  addText("slabsize", QString::number(slabSize));
  addText("pvlheadersize", "13");
  addText("rawheadersize", "13");

  QDomElement names = doc.createElement("pvlnames");
  const int slabCount = 1 + (d-1)/slabSize;
  const QFileInfo headerInfo(pvlFilename);
  for (int i = 0; i < slabCount; ++i)
    {
      QDomElement name = doc.createElement("name");
      name.appendChild(doc.createTextNode(
        headerInfo.absoluteDir().relativeFilePath(
          QString("%1.%2").arg(headerInfo.fileName())
          .arg(i+1, 3, 10, QChar('0')))));
      names.appendChild(name);
    }
  root.appendChild(names);

  auto addList = [&doc, &root](const QString& tag, const QString& value) {
    QDomElement element = doc.createElement(tag);
    element.appendChild(doc.createTextNode(value));
    root.appendChild(element);
  };
  QString rawMapText;
  for (int i = 0; i < rawMap.count(); ++i)
    rawMapText += QString::number(rawMap.at(i)) + " ";
  QString pvlMapText;
  for (int i = 0; i < pvlMap.count(); ++i)
    pvlMapText += QString::number(pvlMap.at(i)) + " ";
  addList("rawmap", rawMapText);
  addList("pvlmap", pvlMapText);

  QSaveFile file(pvlFilename);
  file.setDirectWriteFallback(false);
  if (!file.open(QIODevice::WriteOnly))
    return;
  QTextStream out(&file);
  doc.save(out, 2);
  if (out.status() != QTextStream::Ok)
    {
      file.cancelWriting();
      return;
    }
  file.commit();
}

void
StaticFunctions::renderText(int x, int y,
			    QString str, QFont font,
			    QColor bcolor, QColor color,
			    bool useTextPath)
{
  QFontMetrics metric(font);
  int ht = metric.height()+4;
  int wd = metric.width(str)+6;
  QImage img(wd, ht, QImage::Format_ARGB32);
  img.fill(bcolor);
  QPainter p(&img);
  p.setRenderHints(QPainter::TextAntialiasing, true);
  //p.setPen(color);
  p.setPen(QPen(color, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  p.setFont(font);

  if (!useTextPath)
    p.drawText(3, ht-metric.descent()-2, str);
  else
    {
      QPainterPath textPath;
      textPath.addText(3, ht-metric.descent()-2, font, str);
      p.drawPath(textPath);
    }

  QImage mimg = img.mirrored();
  glRasterPos2i(x, y);
  glDrawPixels(wd, ht, GL_RGBA, GL_UNSIGNED_BYTE, mimg.bits());
}

QImage
StaticFunctions::renderText(QString str, QFont font,
			    QColor bcolor, QColor color, bool border)
{
  if (str.contains("\n"))
    {
      QStringList strlist = str.split("\n");
      return renderText(strlist, font, bcolor, color, border);
    }
  
  QFontMetrics metric(font);
  int ht = metric.height()+9;
  int wd = metric.width(str)+11;
  QImage img(wd, ht, QImage::Format_ARGB32);
  img.fill(bcolor);
  QPainter p(&img);
  p.setRenderHints(QPainter::TextAntialiasing, true);
  p.setPen(QPen(color, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  p.setFont(font);

  if (border)
    p.drawRoundedRect(1, 1, wd-2, ht-2, 5, 5);
  
  p.drawText(5, ht-metric.descent()-5, str);

  QImage mimg = img.mirrored();

  return mimg;
}
QImage
StaticFunctions::renderText(QStringList strlist, QFont font,
			    QColor bcolor, QColor color, bool border)
{
  QFontMetrics metric(font);

  int ht = strlist.count()*metric.height();
  int wd = 0;
  for(int i=0; i<strlist.count(); i++)
    wd = qMax(wd, metric.width(strlist[i]));
  ht += 10;
  wd += 10;

  QImage img(wd, ht, QImage::Format_ARGB32);
  img.fill(bcolor);
  QPainter p(&img);
  p.setRenderHints(QPainter::TextAntialiasing, true);
  p.setPen(QPen(color, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  p.setFont(font);

  if (border)
    p.drawRoundedRect(1, 1, wd-2, ht-2, 5, 5);
  
  QString str = strlist.join("\n");  
  p.drawText(QRect(5,5,wd-10,ht-10),
	     Qt::AlignCenter,
	     str);
  
  QImage mimg = img.mirrored();

  return mimg;
}

void
StaticFunctions::renderRotatedText(int x, int y,
				   QString str, QFont font,
				   QColor bcolor, QColor color,
				   float angle, bool ydir,
				   bool useTextPath)
{
  QFontMetrics metric(font);
  int ht = metric.height()+1;
  int wd = metric.width(str)+3;
  QImage img(wd, ht, QImage::Format_ARGB32);
  img.fill(bcolor);
  QPainter p(&img);
  p.setRenderHints(QPainter::Antialiasing |
		   QPainter::TextAntialiasing |
		   QPainter::SmoothPixmapTransform);
  p.setPen(color);
  p.setFont(font);
  p.drawText(1, ht-metric.descent()-1, str);
  QImage mimg = img.mirrored();
  QMatrix mat;
  mat.rotate(angle);
  QImage pimg = mimg.transformed(mat, Qt::SmoothTransformation);
  if (ydir) // (0,0) is bottom left
    glRasterPos2i(x-pimg.width()/2, y+pimg.height()/2);
  else // (0,0) is top left
    glRasterPos2i(x-pimg.width()/2, y-pimg.height()/2);
  glDrawPixels(pimg.width(), pimg.height(),
	       GL_RGBA, GL_UNSIGNED_BYTE,
	       pimg.bits());
}

/*! Returns "pseudo-distance" from (x,y) to ball of radius size.
\arg for a point inside the ball, it is proportional to the euclidean distance to the ball
\arg for a point outside the ball, it is proportional to the inverse of this distance (tends to
zero) on the ball, the function is continuous. */
float
StaticFunctions::projectOnBall(float x, float y)
{
  // If you change the size value, change angle computation in deformedBallQuaternion().
  const qreal size       = 1.0;
  const qreal size2      = size*size;
  const qreal size_limit = size2*0.5;
  
  const qreal d = x*x + y*y;
  return d < size_limit ? sqrt(size2 - d) : size_limit/sqrt(d);
}

/*! Returns a quaternion computed according to the mouse motion. Mouse positions are projected on a
deformed ball, centered on (\p cx,\p cy). */
Quaternion
StaticFunctions::deformedBallQuaternion(int x, int y,
					qreal cx, qreal cy,
					qreal prevx, qreal prevy,
					const Camera* const camera)
{
  // Points on the deformed ball
  qreal px = (prevx - cx)/ camera->screenWidth();
  qreal py = (cy - prevy)/ camera->screenHeight();
  qreal dx = ( x - cx)/ camera->screenWidth();
  qreal dy = (cy - y) / camera->screenHeight();
  
  const Vec p1(px, py, projectOnBall(px, py));
  const Vec p2(dx, dy, projectOnBall(dx, dy));
  // Approximation of rotation angle
  // Should be divided by the projectOnBall size, but it is 1.0
  const Vec axis = cross(p2,p1);
  const qreal angle = 5.0 * asin(sqrt(axis.squaredNorm() / p1.squaredNorm() / p2.squaredNorm()));
  return Quaternion(axis, angle);
}

void
StaticFunctions::copyGradientFile(QString stopsflnm)
{
  QString sflnm = ":/images/gradientstops.xml";
  QFileInfo fi(sflnm);
  if (! fi.exists())
    {
      QMessageBox::information(0, "Gradient Stops",
			       QString("Gradient Stops file does not exit at %1").arg(sflnm));
      
      return;
    }

  // copy information from gradientstops.xml to HOME/.drishtigradients.xml
  QDomDocument document;
  QFile f(sflnm);
  if (f.open(QIODevice::ReadOnly))
    {
      document.setContent(&f);
      f.close();
    }
  
  QFile fout(stopsflnm);
  if (fout.open(QIODevice::WriteOnly))
    {
      QTextStream out(&fout);
      document.save(out, 2);
      fout.close();
    }
}

void
StaticFunctions::loadTexture(QImage img, GLuint glTex)
{
  float twd = img.width();
  float tht = img.height();

  glBindTexture(GL_TEXTURE_2D, glTex);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D,
	       0,
	       4,
	       twd,
	       tht,
	       0,
	       GL_RGBA,
	       GL_UNSIGNED_BYTE,
	       img.bits());  
}

bool
StaticFunctions::intersectRayPlane(QVector3D v0, QVector3D vy,
				   QVector3D vx, QVector3D pn,
				   QVector3D rayO, QVector3D ray,
				   float &x, float &y)
{
  float deno = QVector3D::dotProduct(ray, pn);

  if (deno > -0.00001) // point along same direction
    return false;

  float t = -QVector3D::dotProduct(rayO - v0, pn) / deno;
  if (t >= 0)
    {
      QVector3D pt = rayO + t*ray;
      float lx = vx.length();
      float ly = vy.length();
      x = QVector3D::dotProduct(pt-v0, vx.normalized())/lx;
      y = QVector3D::dotProduct(pt-v0, vy.normalized())/ly;
      if (x >= 0 && x <= 1 &&
	  y >= 0 && y <= 1)
	return true;
    } 

  return false;
}


void
StaticFunctions::savePLY(QVector<float> m_vertices,
			 QVector<float> m_normals,
			 QVector<float> m_vcolor,
			 QVector<uint> m_triangles,
			 double *s,
			 QString prevDir)
{
  QString flnm = QFileDialog::getSaveFileName(0,
					      "Export mesh to file",
					      prevDir,
					      "*.ply");
  if (flnm.size() == 0)
    return;

  QString error;
  const bool saved = BinaryPlyWriter::save(
    flnm, m_vertices, m_normals, m_vcolor, m_triangles,
    [s](float x, float y, float z)
    {
      const Vec value = Matrix::xformVec(s, Vec(x, y, z));
      return QVector3D(value.x, value.y, value.z);
    },
    [s](float x, float y, float z)
    {
      const Vec value = Matrix::rotateVec(s, Vec(x, y, z));
      return QVector3D(value.x, value.y, value.z);
    },
    &error);
  if (!saved)
    {
      QMessageBox::critical(0, "Save Mesh", error);
      return;
    }
  QMessageBox::information(0, "Save Mesh", "done");
}


//split large binary STL files
void
StaticFunctions::splitBinarySTL(QString flnm)
{
  QFile sfile(flnm);
  sfile.open(QFile::ReadOnly);
  char word[10];
  memset(word, 0, 10);
  sfile.read(&word[0], 9);
  sfile.close();

  QString solid(word);
  solid.truncate(5);
  if (solid == "solid") // not a binary STL
    {
      QMessageBox::information(0, "Split Binary STL", "Cannot split "+flnm+". This is not a binary stl file.");
      return;
    }

  // split
  sfile.open(QFile::ReadOnly);
  uchar b80[80];
  sfile.read((char*)&b80[0], 80);
  
  int ntri;
  sfile.read((char*)&ntri, 4);

  if (ntri < 39600000)
    {
      QMessageBox::information(0, "Split Binary STL", "No split for "+flnm+".  Binary stl file not big enough to split.");
      sfile.close();
      return;
    }

  int nfiles = ntri/39000000;
  if (nfiles*39000000 < ntri)
    nfiles++;

  int ntriF = ntri/nfiles;

  int chunkSize = 5000;
  uchar *data = new uchar[50 * chunkSize];

  for(int n=0; n<nfiles; n++)
    {
      QString cflnm = flnm.chopped(4)+QString("-part%1of%2").arg(n+1).arg(nfiles)+".stl";
      MainWindowUI::mainWindowUI()->statusBar->showMessage(cflnm);

      QFile cfile(cflnm);
      cfile.open(QFile::WriteOnly);
      cfile.write((char*)&b80, 80);

      int mtri = ntriF;
      if (n == nfiles-1)
	mtri = ntri - n*ntriF;

      cfile.write((char*)&mtri, 4);
      
      // read chunks of chunkSize
      int ic = 0;
      for(int i=0; i<mtri; i+=chunkSize)
	{
	  Global::progressBar()->setValue((int)(100.0*(float)(i)/(float)mtri));
	  qApp->processEvents();
	  
	  int chnk = qMin(chunkSize, mtri-i);
	  sfile.read((char*)data, chnk*50);
	  cfile.write((char*)data, chnk*50);	  
	}
      cfile.close();
    }

  QMessageBox::information(0, "", QString("%1 splits done").arg(nfiles));
}








void
StaticFunctions::generateHistograms(float *flhist1D,
				    float *flhist2D,
				    int* hist1D,
				    int* hist2D)
{
  QProgressDialog progress("Generating Histograms",
			   "Cancel",
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);

  progress.setValue(0);
  qApp->processEvents();

  int i;

  //-----------------------------------
  // generate 1d histogram
  float maxf, minf, mlen;
  maxf = 0;
  for (i=1; i<256; i++)
    maxf = qMax(maxf,flhist1D[i]);

  for (i=0; i<256; i++)
    hist1D[i] = (int)(255*flhist1D[i]/maxf);
  hist1D[0] = qMin(hist1D[0],255);
  //-----------------------------------
  progress.setValue(50);
  qApp->processEvents();

  //-----------------------------------
  // generate 2d histogram
  for (i=0; i<256*256; i++)
    if (flhist2D[i] > 1)
      flhist2D[i] = log(flhist2D[i]);
  progress.setValue(70);
  qApp->processEvents();
    
  maxf = -1.0;
  minf = 1000000.0;
  for (i=0; i<256*256; i++)
    {
      maxf = qMax(maxf,flhist2D[i]);
      minf = qMin(minf,flhist2D[i]);
    }
  progress.setValue(90);
  qApp->processEvents();

  mlen = maxf-minf;
  for (i=0; i<256*256; i++)
    hist2D[i] = (int)(255*(flhist2D[i]-minf)/mlen);
  //-----------------------------------
  progress.setValue(100);
  qApp->processEvents();
}

Vec
StaticFunctions::getVoxelSizeFromHeader(QString pvlFilename)
{
  PvlManifest manifest;
  if (PvlManifestParser::parse(pvlFilename, manifest, false))
    return Vec(manifest.voxelSizeX, manifest.voxelSizeY, manifest.voxelSizeZ);
  return Vec(1,1,1);
}

QString
StaticFunctions::getVoxelUnitFromHeader(QString pvlFilename)
{
  PvlManifest manifest;
  if (!PvlManifestParser::parse(pvlFilename, manifest, false)) return "";
  const QString unit = manifest.voxelUnit.toLower();
  if (unit == "angstrom") return "A";
  if (unit == "nanometer") return "nm";
  if (unit == "micron") return "um";
  if (unit == "millimeter") return "mm";
  if (unit == "centimeter") return "cm";
  if (unit == "meter") return "m";
  if (unit == "kilometer") return "km";
  if (unit == "parsec") return "p";
  if (unit == "kiloparsec") return "kp";
  return "";
}

void
StaticFunctions::getDimensionsFromHeader(QString pvlFilename,
					 int &d, int &w, int &h)
{
  PvlManifest manifest;
  if (PvlManifestParser::parse(pvlFilename, manifest, false))
    { d = manifest.depth; w = manifest.width; h = manifest.height; return; }
  d = w = h = 0;
}

int
StaticFunctions::getSlabsizeFromHeader(QString pvlFilename)
{
  PvlManifest manifest;
  return PvlManifestParser::parse(pvlFilename, manifest, false) ?
    manifest.slabSize : 0;
}

int
StaticFunctions::getPvlVoxelTypeFromHeader(QString pvlFilename)
{
  PvlManifest manifest;
  if (PvlManifestParser::parse(pvlFilename, manifest, false) &&
      manifest.voxelType <= PvlManifestParser::Float)
    return manifest.voxelType;
  return -1;
}

int
StaticFunctions::getPvlHeadersizeFromHeader(QString pvlFilename)
{
  PvlManifest manifest;
  return PvlManifestParser::parse(pvlFilename, manifest, false) ?
    manifest.headerSize : -1;
}

int
StaticFunctions::getRawHeadersizeFromHeader(QString pvlFilename)
{
  PvlManifest manifest;
  return PvlManifestParser::parse(pvlFilename, manifest, false) ?
    manifest.rawHeaderSize : -1;
}

QStringList
StaticFunctions::getPvlNamesFromHeader(QString pvlFilename)
{
  PvlManifest manifest;
  return PvlManifestParser::parse(pvlFilename, manifest, false) ?
    manifest.pvlNames : QStringList();
}

QStringList
StaticFunctions::getRawNamesFromHeader(QString pvlFilename)
{
  PvlManifest manifest;
  return PvlManifestParser::parse(pvlFilename, manifest, false) ?
    manifest.rawNames : QStringList();
}

bool
StaticFunctions::xmlHeaderFile(QString volfile)
{
  PvlManifest manifest;
  return PvlManifestParser::parse(volfile, manifest, false);
}

