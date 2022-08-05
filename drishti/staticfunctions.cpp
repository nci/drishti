#include "staticfunctions.h"
#include "matrix.h"
#include "enums.h"
#include "volumefilemanager.h"
#include "ply.h"

#include <fstream>
using namespace std;

#include <QColorDialog>
#include <QInputDialog>
#include <QUrl>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <QFileDialog>

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
StaticFunctions::getSubsamplingLevel(int textureMemorySize, int arrayTextureSize,
				     int bytesPerVoxel,
				     Vec boxMin, Vec boxMax)
{
  Vec subvolumeSize = boxMax - boxMin + Vec(1,1,1); 
  qint64 nx = subvolumeSize.x;
  qint64 ny = subvolumeSize.y;
  qint64 nz = subvolumeSize.z;

  int mb = 1024*1024;
  qint64 volsize = bytesPerVoxel*nx*ny*nz;
  volsize /= mb; // vosize in Mb
  
  int lod = 1;
  //while (nz/lod > arrayTextureSize ||  // check for array texture
  while (volsize >= (textureMemorySize-128)) // we are just going to worry about the available texture memory less 128MB
    {
      lod ++;
      qint64 x = nx/lod;
      qint64 y = ny/lod;
      qint64 z = nz/lod;
      volsize = (x*y*z*bytesPerVoxel)/mb;
    }

  return lod;
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
  QColor colorMap[1001];

  mapSize = qMin(1000, mapSize);

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
      float pos = (float)i/(float)mapSize;
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
StaticFunctions::drawEnclosingCube(Vec subvolmin,
				   Vec subvolmax, bool allFaces)
{
//  glEnable(GL_LINE_SMOOTH);  // antialias lines	
//  glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

  glBegin(GL_QUADS);  
  glVertex3f(subvolmin.x, subvolmin.y, subvolmin.z);
  glVertex3f(subvolmax.x, subvolmin.y, subvolmin.z);
  glVertex3f(subvolmax.x, subvolmax.y, subvolmin.z);
  glVertex3f(subvolmin.x, subvolmax.y, subvolmin.z);
  glEnd();
  
  // FRONT 
  glBegin(GL_QUADS);  
  glVertex3f(subvolmin.x, subvolmin.y, subvolmax.z);
  glVertex3f(subvolmax.x, subvolmin.y, subvolmax.z);
  glVertex3f(subvolmax.x, subvolmax.y, subvolmax.z);
  glVertex3f(subvolmin.x, subvolmax.y, subvolmax.z);
  glEnd();
  
  // TOP
  glBegin(GL_QUADS);  
  glVertex3f(subvolmin.x, subvolmax.y, subvolmin.z);
  glVertex3f(subvolmax.x, subvolmax.y, subvolmin.z);
  glVertex3f(subvolmax.x, subvolmax.y, subvolmax.z);
  glVertex3f(subvolmin.x, subvolmax.y, subvolmax.z);
  glEnd();
  
  // BOTTOM
  glBegin(GL_QUADS);  
  glVertex3f(subvolmin.x, subvolmin.y, subvolmin.z);
  glVertex3f(subvolmax.x, subvolmin.y, subvolmin.z);
  glVertex3f(subvolmax.x, subvolmin.y, subvolmax.z);
  glVertex3f(subvolmin.x, subvolmin.y, subvolmax.z);  
  glEnd();  

  if (allFaces)
    {
      glBegin(GL_QUADS);  
      glVertex3f(subvolmin.x, subvolmin.y, subvolmin.z);
      glVertex3f(subvolmin.x, subvolmin.y, subvolmax.z);
      glVertex3f(subvolmin.x, subvolmax.y, subvolmax.z);
      glVertex3f(subvolmin.x, subvolmax.y, subvolmin.z);
      glEnd();
  
      glBegin(GL_QUADS);  
      glVertex3f(subvolmax.x, subvolmin.y, subvolmin.z);
      glVertex3f(subvolmax.x, subvolmin.y, subvolmax.z);
      glVertex3f(subvolmax.x, subvolmax.y, subvolmax.z);
      glVertex3f(subvolmax.x, subvolmax.y, subvolmin.z);
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
StaticFunctions::drawAxis(Vec origin,
			  Vec xAxis, Vec yAxis, Vec zAxis)
{
  Vec x,y,z;

//  x = origin+xAxis;
//  y = origin+yAxis;
//  z = origin+zAxis;
//  glColor4f(0, 0, 0, 0.7);
//  glLineWidth(7);
//  glBegin(GL_LINES);
//  glVertex3fv(origin);
//  glVertex3fv(x);
//  glVertex3fv(origin);
//  glVertex3fv(y);
//  glVertex3fv(origin);
//  glVertex3fv(z);
//  glEnd();

//  glEnable(GL_LINE_SMOOTH);  // antialias lines	
//  glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

  x = origin+1.1f*xAxis;
  y = origin+1.1f*yAxis;
  z = origin+1.1f*zAxis;
  glLineWidth(5);
  glColor4f(0.7f, 0.3f, 0.3f, 0.7f);
  glBegin(GL_LINES);
  glVertex3fv(origin);
  glVertex3fv(x);
  glEnd();
  glColor4f(0.3f, 0.7f, 0.3f, 0.7f);
  glBegin(GL_LINES);
  glVertex3fv(origin);
  glVertex3fv(y);
  glEnd();
  glColor4f(0.3f, 0.3f, 0.7f, 0.7f);
  glBegin(GL_LINES);
  glVertex3fv(origin);
  glVertex3fv(z);
  glEnd();

  x = origin+1.2f*xAxis;
  y = origin+1.2f*yAxis;
  z = origin+1.2f*zAxis;
  glLineWidth(3);
  glColor4f(1, 0.3f, 0.3f, 0.8f);
  glBegin(GL_LINES);
  glVertex3fv(origin);
  glVertex3fv(x);
  glEnd();
  glColor4f(0.3f, 1, 0.3f, 0.8f);
  glBegin(GL_LINES);
  glVertex3fv(origin);
  glVertex3fv(y);
  glEnd();
  glColor4f(0.3f, 0.3f, 1, 0.8f);
  glBegin(GL_LINES);
  glVertex3fv(origin);
  glVertex3fv(z);
  glEnd();

  x = origin+1.3f*xAxis;
  y = origin+1.3f*yAxis;
  z = origin+1.3f*zAxis;
  glLineWidth(1);
  glColor4f(0.9f, 0.9f, 0.9f, 0.9f);
  glBegin(GL_LINES);
  glVertex3fv(origin);
  glVertex3fv(x);
  glVertex3fv(origin);
  glVertex3fv(y);
  glVertex3fv(origin);
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
StaticFunctions::intersectType1(Vec po, Vec pn,
				Vec v0, Vec v1,
				Vec &v)
{
  Vec v1m0 = v1-v0;
  float deno = pn*v1m0;
  if (fabs(deno) > 0.0001)
    {
      float t = pn*(po - v0)/deno;
      if (t >= 0 && t <= 1)
	{
	  v = v0 + v1m0*t;
	  return 1;
	}
    }
  return 0;
}

int
StaticFunctions::intersectType1WithTexture(Vec po, Vec pn,
					   Vec v0, Vec v1,
					   Vec t0, Vec t1,
					   Vec &v, Vec &t)
{
  Vec v1m0 = v1-v0;
  float deno = pn*v1m0;
  if (fabs(deno) > 0.0001)
    {
      float a = pn*(po - v0)/deno;
      if (a >= 0 && a <= 1)
	{
	  v = v0 + v1m0*a;
	  t = t0 + (t1-t0)*a;
	  return 1;
	}
    }
  return 0;
}

int
StaticFunctions::intersectType2(Vec Po, Vec Pn, Vec& v0, Vec& v1)
{
  float d0, d1;
  d0 = Pn*(v0-Po); 
  d1 = Pn*(v1-Po); 

  if (d0 > 0 && d1 > 0)
    // both points are clipped
    return 0;

  if (d0 <= 0 && d1 <= 0)
    // both points not clipped
    return 1;

  Vec Rnew, Rd;
  float RdPn;

  Rd = v1-v0;
  
  if (d0 > 0)
    Rd = -Rd;
  
  RdPn = Rd*Pn;

  if (d1 > 0) 
    {
      v1 = v0 + Rd * (((Po-v0)*Pn)/RdPn);
      return 2;
    }
  else
    {
      v0 = v1 + Rd * (((Po-v1)*Pn)/RdPn);
      return 1;
    }
}

int
StaticFunctions::intersectType2WithTexture(Vec Po, Vec Pn,
					   Vec& v0, Vec& v1,
					   Vec& t0, Vec& t1)
{
  float d0, d1;
  d0 = Pn*(v0-Po); 
  d1 = Pn*(v1-Po); 

  if (d0 > 0 && d1 > 0)
    // both points are clipped
    return 0;

  if (d0 <= 0 && d1 <= 0)
    // both points not clipped
    return 1;

  Vec Td, Rd;
  float RdPn;

  Rd = v1-v0;
  Td = t1-t0;

  if (d0 > 0)
    {
      Rd = -Rd;
      Td = -Td;
    }
  
  RdPn = Rd*Pn;

  if (d1 > 0) 
    {
      float a = Pn*(Po-v0)/RdPn;
      v1 = v0 + Rd*a;
      t1 = t0 + Td*a;
      return 2;
    }
  else
    {
      float a = Pn*(Po-v1)/RdPn;
      v0 = v1 + Rd*a;
      t0 = t1 + Td*a;
      return 1;
    }
}

int
StaticFunctions::intersectType3WithTexture(Vec Po, Vec Pn,
					   Vec& v0, Vec& v1,
					   Vec& t0, Vec& t1,
					   Vec& s0, Vec& s1)
{
  float d0, d1;
  d0 = Pn*(v0-Po); 
  d1 = Pn*(v1-Po); 

  if (d0 > 0 && d1 > 0)
    // both points are clipped
    return 0;

  if (d0 <= 0 && d1 <= 0)
    // both points not clipped
    return 1;

  Vec Sd, Td, Rd;
  float RdPn;

  Rd = v1-v0;
  Td = t1-t0;
  Sd = s1-s0;

  if (d0 > 0)
    {
      Rd = -Rd;
      Td = -Td;
      Sd = -Sd;
    }
  
  RdPn = Rd*Pn;

  if (d1 > 0) 
    {
      float a = Pn*(Po-v0)/RdPn;
      v1 = v0 + Rd*a;
      t1 = t0 + Td*a;
      s1 = s0 + Sd*a;
      return 2;
    }
  else
    {
      float a = Pn*(Po-v1)/RdPn;
      v0 = v1 + Rd*a;
      t0 = t1 + Td*a;
      s0 = s1 + Sd*a;
      return 1;
    }
}


void
StaticFunctions::getMinMaxProjectionVertices(Camera *camera,
					     Vec *subvol,
					     float &zdepth,
					     Vec& minvert, Vec& maxvert,
					     int& maxidx)
{
 float mindepth, maxdepth;

 maxidx = 0;
      
  for (int i=0; i<8; i++)
    {
      Vec camCoord = camera->cameraCoordinatesOf(subvol[i]);
      float zval = camCoord.z;

      if (i == 0)
	{
	  mindepth = maxdepth = zval;
	  minvert = maxvert = subvol[i];
	  maxidx = i;
	}
      else
	{
	  if (zval < mindepth)
	    {
	      mindepth = zval;
	      minvert = subvol[i];
	    }

	  if (zval > maxdepth)
	    {
	      maxdepth = zval;
	      maxvert = subvol[i];
	      maxidx = i;
	    }
	}
    }

  zdepth = maxdepth - mindepth;
}

void
StaticFunctions::getMinMaxBrickVertices(Camera *camera,
					Vec bmin, Vec bmax,
					float &zdepth,
					Vec& minvert, Vec& maxvert,
					int& maxidx)
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

  getMinMaxProjectionVertices(camera,
			      subvol,
			      zdepth, minvert, maxvert, maxidx);
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

QList<Vec>
StaticFunctions::voxelizeLine(Vec v1, Vec v2)
{
  QList<Vec> points;

  int x1, y1, z1;
  int x2, y2, z2;
  int xd, yd, zd;
  int x, y, z;
  int ax, ay, az;
  int sx, sy, sz;
  int dx, dy, dz;

  x1 = v1.x;
  y1 = v1.y;
  z1 = v1.z;

  x2 = v2.x;
  y2 = v2.y;
  z2 = v2.z;

  dx = x2 - x1;
  dy = y2 - y1;
  dz = z2 - z1;

  if (dx == 0 &&
      dy == 0 &&
      dz == 0)
    return points; // return empty list
    
  ax = qAbs(dx) << 1;
  ay = qAbs(dy) << 1;
  az = qAbs(dz) << 1;
    
  sx = ( dx<0 ? -1 : (dx>0 ? 1: 0) );
  sy = ( dy<0 ? -1 : (dy>0 ? 1: 0) );
  sz = ( dz<0 ? -1 : (dz>0 ? 1: 0) );
  
  x = x1;
  y = y1;
  z = z1;

  bool done = false;
  if (ax >= qMax(ay, az))            /* x dominant */
    {
      yd = ay - (ax >> 1);
      zd = az - (ax >> 1);
      while (!done)
        {
	  points << Vec(x, y, z);

	  if (x == x2)
	    done = true;
	  else
	    {
	      if (yd >= 0)
		{
		  y += sy;
		  yd -= ax;
		}	 
	      if (zd >= 0)
		{
		  z += sz;
		  zd -= ax;
		}	  
	      x += sx;
	      yd += ay;
	      zd += az;
	    }
	}
    }
  else if (ay >= qMax(ax, az))            /* y dominant */
    {
      xd = ax - (ay >> 1);
      zd = az - (ay >> 1);
      while (!done)
        {
	  points << Vec(x, y, z);

	  if (y == y2)
	    done = true;
	  else
	    {
	      if (xd >= 0)
		{
		  x += sx;
		  xd -= ay;
		}	  
	      if (zd >= 0)
		{
		  z += sz;
		  zd -= ay;
		}	  
	      y += sy;
	      xd += ax;
	      zd += az;
	    }
        }
    }
  else if (az >= qMax(ax, ay))            /* z dominant */
    {
      xd = ax - (az >> 1);
      yd = ay - (az >> 1);
      while (!done)
        {
	  points << Vec(x, y, z);

	  if (z == z2)
	    done = true;
	  else
	    {
	      if (xd >= 0)
		{
		  x += sx;
		  xd -= az;
		}	      
	      if (yd >= 0)
		{
		  y += sy;
		  yd -= az;
		}	      
	      z += sz;
	      xd += ax;
	      yd += ay;
	    }
	}
    }

  return points;
}

VoxelizedPath
StaticFunctions::voxelizePath(QList<Vec> path)
{
  VoxelizedPath vp;

  for(int i=0; i<path.count()-1; i++)
    {
      Vec v0 = path[i];
      Vec v1 = path[i+1];

      vp.index.append(vp.voxels.size());
      vp.voxels += voxelizeLine(v0, v1);
    }
  vp.index.append(vp.voxels.size());

  return vp;
}

VoxelizedPath
StaticFunctions::voxelizePath(QList< QPair<Vec, Vec> > path)
{
  VoxelizedPath vp;

  for(int i=0; i<path.count(); i++)
    {
      Vec v0 = path[i].first;
      Vec n0 = path[i].second;

      vp.index.append(vp.voxels.size());
      vp.voxels.append(v0);
      vp.normals.append(n0);
    }
  vp.index.append(vp.voxels.size());

//  for(int i=0; i<path.count()-1; i++)
//    {
//      Vec v0 = path[i].first;
//      Vec v1 = path[i+1].first;
//
//      vp.index.append(vp.voxels.size());
//
//      QList<Vec> points = voxelizeLine(v0, v1);
//
//      // linearly interpolate intermediate normals
//      Vec n0 = path[i].second;
//      Vec n1 = path[i+1].second;
//      QList<Vec> normals;
//      int npt = points.count();
//      for(int p=0; p<npt; p++)
//	{
//	  Vec nrml = ((npt-1-p)*n0 + p*n1)/float(npt-1);
//	  normals.append(nrml);
//	}
//
//      vp.voxels += points;
//      vp.normals += normals;
//
//      vp.voxels.append(v0);
//      vp.normals.append(n0);
//    }
//  vp.index.append(vp.voxels.size());

  return vp;
}

void
StaticFunctions::generateHistograms(float *flhist1D,
				    float *flhist2D,
				    int* hist1D,
				    int* hist2D)
{
  int i;

  float totvox = 0;

  //------------------
  for (i=0; i<256; i++)
    totvox += flhist1D[i];

  for (i=0; i<256; i++)
    flhist1D[i] /= totvox;
  
  for (i=0; i<256; i++)
    {
      if (flhist1D[i] > 0)
	{
	  float t = qBound(0.0,(8+qMax(-8.0f, log10(100*flhist1D[i])))/10.0, 1.0);
	  hist1D[i] = qBound(0, (int)(t*t*t*300), 255);
	  //hist1D[i] = (8+qMax(-8.0f, log10(flhist1D[i])))*25;
	}
      else
	hist1D[i] = 0;
    }
  //------------------

  //------------------
  totvox = 0;
  for (i=0; i<256*256; i++)
    totvox += flhist2D[i];

  for (i=0; i<256*256; i++)
    flhist2D[i] /= totvox;

  for (i=0; i<256*256; i++)
    {
      if (flhist2D[i] > 0)
	hist2D[i] = qMin(255.0f, (7+qMax(-7.0f, log10(flhist2D[i])))*50);
      else
	hist2D[i] = 0;
    }
      
//------------------
  
//  // generate 1d histogram
//  float maxf, minf, mlen;
//  maxf = 1;
//  for (i=1; i<255; i++) // leave out two extremes
//    maxf = qMax(maxf,flhist1D[i]);
//
//  for (i=0; i<256; i++)
//    hist1D[i] = (int)(255*flhist1D[i]/maxf);
//  hist1D[0] = qMin(hist1D[0],255);
//  hist1D[255] = qMin(hist1D[255],255);
//
//
//  // generate 2d histogram
//  for (i=0; i<256*256; i++)
//    if (flhist2D[i] > 1)
//      flhist2D[i] = log(flhist2D[i]);
//    
//  float maxf, minf, mlen;
//  maxf = -1.0f;
//  minf = 1000000.0f;
//  for (i=0; i<256*256; i++)
//    {
//      maxf = qMax(maxf,flhist2D[i]);
//      minf = qMin(minf,flhist2D[i]);
//    }
//
//  mlen = maxf-minf;
//  for (i=0; i<256*256; i++)
//    hist2D[i] = (int)(255*(flhist2D[i]-minf)/mlen);
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
      fi = QFileInfo(dir, lflnm);
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
  enum VoxelUnit {
    _Nounit = 0,
    _Angstrom,
    _Nanometer,
    _Micron,
    _Millimeter,
    _Centimeter,
    _Meter,
    _Kilometer,
    _Parsec,
    _Kiloparsec
  };

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
    if (voxelType ==      VolumeFileManager::_UChar) vstr = "unsigned char";
    else if (voxelType == VolumeFileManager::_Char)  vstr = "char";
    else if (voxelType == VolumeFileManager::_UShort)vstr = "unsigned short";
    else if (voxelType == VolumeFileManager::_Short) vstr = "short";
    else if (voxelType == VolumeFileManager::_Int)   vstr = "int";
    else if (voxelType == VolumeFileManager::_Float) vstr = "float";
    
    QDomElement de0 = doc.createElement("voxeltype");
    QDomText tn0;
    tn0 = doc.createTextNode(QString("%1").arg(vstr));
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }


  {      
    QString vstr;
    if (pvlVoxelType ==      VolumeFileManager::_UChar) vstr = "unsigned char";
    else if (pvlVoxelType == VolumeFileManager::_Char)  vstr = "char";
    else if (pvlVoxelType == VolumeFileManager::_UShort)vstr = "unsigned short";
    else if (pvlVoxelType == VolumeFileManager::_Short) vstr = "short";
    else if (pvlVoxelType == VolumeFileManager::_Int)   vstr = "int";
    else if (pvlVoxelType == VolumeFileManager::_Float) vstr = "float";
    
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
    if (voxelUnit ==      _Nounit)    vstr = "no units";
    else if (voxelUnit == _Angstrom)  vstr = "angstrom";
    else if (voxelUnit == _Nanometer) vstr = "nanometer";
    else if (voxelUnit == _Micron)    vstr = "micron";
    else if (voxelUnit == _Millimeter)vstr = "millimeter";
    else if (voxelUnit == _Centimeter)vstr = "centimeter";
    else if (voxelUnit == _Meter)     vstr = "meter";
    else if (voxelUnit == _Kilometer) vstr = "kilometer";
    else if (voxelUnit == _Parsec)    vstr = "parsec";
    else if (voxelUnit == _Kiloparsec)vstr = "kiloparsec";
    
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
StaticFunctions::savePLY(QVector<float> m_vertices,
			 QVector<float> m_normals,
			 QVector<float> m_vcolor,
			 QVector<uint> m_triangles,
			 double *s,
			 QString prevDir)
{
  

  bool has_normals = (m_normals.count() > 0);
  bool per_vertex_color = (m_vcolor.count() > 0);

  QString flnm = QFileDialog::getSaveFileName(0,
					      "Export mesh to file",
					      prevDir,
					      "*.ply");
  if (flnm.size() == 0)
    return;

  QStringList ps;
  ps << "x";
  ps << "y";
  ps << "z";
  ps << "nx";
  ps << "ny";
  ps << "nz";
  ps << "red";
  ps << "green";
  ps << "blue";
  ps << "vertex_indices";
  ps << "vertex";
  ps << "face";

  QList<char*> plyStrings;
  for(int i=0; i<ps.count(); i++)
    {
      char *s;
      s = new char[ps[i].size()+1];
      strcpy(s, ps[i].toLatin1().data());
      plyStrings << s;
    }


  typedef struct PlyFace
  {
    unsigned char nverts;    /* number of Vertex indices in list */
    int *verts;              /* Vertex index list */
  } PlyFace;

  typedef struct
  {
    float  x,  y,  z;  /**< Vertex coordinates */
    float nx, ny, nz;  /**< Vertex normal */
    uchar r, g, b;
  } myVertex ;


  PlyProperty vert_props[] = { /* list of property information for a vertex */
    {plyStrings[0], Float32, Float32, offsetof(myVertex,x), 0, 0, 0, 0},
    {plyStrings[1], Float32, Float32, offsetof(myVertex,y), 0, 0, 0, 0},
    {plyStrings[2], Float32, Float32, offsetof(myVertex,z), 0, 0, 0, 0},
    {plyStrings[3], Float32, Float32, offsetof(myVertex,nx), 0, 0, 0, 0},
    {plyStrings[4], Float32, Float32, offsetof(myVertex,ny), 0, 0, 0, 0},
    {plyStrings[5], Float32, Float32, offsetof(myVertex,nz), 0, 0, 0, 0},
    {plyStrings[6], Uint8, Uint8, offsetof(myVertex,r), 0, 0, 0, 0},
    {plyStrings[7], Uint8, Uint8, offsetof(myVertex,g), 0, 0, 0, 0},
    {plyStrings[8], Uint8, Uint8, offsetof(myVertex,b), 0, 0, 0, 0},
  };


  PlyProperty face_props[] = { /* list of property information for a face */
    {plyStrings[9], Int32, Int32, offsetof(PlyFace,verts),
     1, Uint8, Uint8, offsetof(PlyFace,nverts)},
  };

  PlyFile    *ply;
  FILE       *fp = fopen(flnm.toLatin1().data(), bin ? "wb" : "w");

  PlyFace     face ;
  int         verts[3] ;
  char       *elem_names[]  = {plyStrings[10],plyStrings[11]};
  ply = write_ply (fp,
		   2,
		   elem_names,
		   bin ? PLY_BINARY_LE : PLY_ASCII );

  int nvertices = m_vertices.count()/3;
  /* describe what properties go into the PlyVertex elements */
  describe_element_ply ( ply, plyStrings[10], nvertices );
  describe_property_ply ( ply, &vert_props[0] );
  describe_property_ply ( ply, &vert_props[1] );
  describe_property_ply ( ply, &vert_props[2] );
  describe_property_ply ( ply, &vert_props[3] );
  describe_property_ply ( ply, &vert_props[4] );
  describe_property_ply ( ply, &vert_props[5] );
  describe_property_ply ( ply, &vert_props[6] );
  describe_property_ply ( ply, &vert_props[7] );
  describe_property_ply ( ply, &vert_props[8] );

  /* describe PlyFace properties (just list of PlyVertex indices) */
  int ntriangles = m_triangles.count()/3;
  describe_element_ply ( ply, plyStrings[11], ntriangles );
  describe_property_ply ( ply, &face_props[0] );

  header_complete_ply ( ply );


  /* set up and write the PlyVertex elements */
  put_element_setup_ply ( ply, plyStrings[10] );

  for(int i=0; i<m_vertices.count()/3; i++)
    {
      myVertex vertex;
      Vec v = Matrix::xformVec(s,Vec(m_vertices[3*i+0],m_vertices[3*i+1],m_vertices[3*i+2]));
      vertex.x = v.x;
      vertex.y = v.y;
      vertex.z = v.z;
      if (has_normals)
	{
	  Vec vn = Matrix::rotateVec(s,Vec(m_normals[3*i+0],m_normals[3*i+1],m_normals[3*i+2]));
	  vertex.nx = vn.x;
	  vertex.ny = vn.y;
	  vertex.nz = vn.z;
	}
      if (per_vertex_color)
	{
	  vertex.r = 255*m_vcolor[3*i+0];
	  vertex.g = 255*m_vcolor[3*i+1];
	  vertex.b = 255*m_vcolor[3*i+2];
	}
      put_element_ply ( ply, ( void * ) &vertex );
    }

  put_element_setup_ply ( ply, plyStrings[11] );
  face.nverts = 3 ;
  face.verts  = verts ;
  for(int i=0; i<m_triangles.count()/3; i++)
    {
      int v0 = m_triangles[3*i];
      int v1 = m_triangles[3*i+1];
      int v2 = m_triangles[3*i+2];

      face.verts[0] = v0;
      face.verts[1] = v1;
      face.verts[2] = v2;

      put_element_ply ( ply, ( void * ) &face );
    }

  close_ply ( ply );
  free_ply ( ply );
  fclose( fp ) ;

  QMessageBox::information(0, "Save Mesh", "done");
}
