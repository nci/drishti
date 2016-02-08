#include "staticfunctions.h"
#include <math.h>

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


bool
StaticFunctions::checkExtension(QString flnm, const char *ext)
{
  bool ok = true;
  int extlen = strlen(ext);

  QByteArray exten = flnm.toLatin1().right(extlen);
  if (exten != ext)
    ok = false;

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
	  QByteArray exten = url.toLocalFile().toLatin1().right(extlen);
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
StaticFunctions::checkRGB(QString volfile)
{
  if (!xmlHeaderFile(volfile))
    {
      QMessageBox::information(0, "Error",
      QString("%1 is not a valid preprocessed volume file").arg(volfile));
      return false;
    }

  QDomDocument document;
  QFile f(volfile.toLatin1().data());
  if (f.open(QIODevice::ReadOnly))
    {
      document.setContent(&f);
      f.close();
    }

  QDomElement main = document.documentElement();
  QDomNodeList dlist = main.childNodes();
  for(int i=0; i<dlist.count(); i++)
    {
      if (dlist.at(i).nodeName() == "voxeltype")
	{
	  QString pvalue = dlist.at(i).toElement().text();
	  if (pvalue == "RGB" ||
	      pvalue == "RGBA")
	    return true;
	}
    }
  return false;
}

bool
StaticFunctions::checkRGBA(QString volfile)
{
  if (!xmlHeaderFile(volfile))
    {
      QMessageBox::information(0, "Error",
      QString("%1 is not a valid preprocessed volume file").arg(volfile));
      return false;
    }

  QDomDocument document;
  QFile f(volfile.toLatin1().data());
  if (f.open(QIODevice::ReadOnly))
    {
      document.setContent(&f);
      f.close();
    }

  QDomElement main = document.documentElement();
  QDomNodeList dlist = main.childNodes();
  for(int i=0; i<dlist.count(); i++)
    {
      if (dlist.at(i).nodeName() == "voxeltype")
	{
	  QString pvalue = dlist.at(i).toElement().text();
	  if (pvalue == "RGBA")
	    return true;
	}
    }
  return false;
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
			   0); 
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
  QDomDocument document;
  QFile f(pvlFilename.toLatin1().data());
  if (f.open(QIODevice::ReadOnly))
    {
      document.setContent(&f);
      f.close();
    }

  QDomElement main = document.documentElement();
  QDomNodeList dlist = main.childNodes();
  for(int i=0; i<dlist.count(); i++)
    {
      if (dlist.at(i).nodeName() == "voxelsize")
	{
	  QStringList str = (dlist.at(i).toElement().text()).split(" ", QString::SkipEmptyParts);
	  float vx = str[0].toFloat();
	  float vy = str[1].toFloat();
	  float vz = str[2].toFloat();
	  return Vec(vx, vy, vz);
	}
    }
  return Vec(1,1,1);
}

QString
StaticFunctions::getVoxelUnitFromHeader(QString pvlFilename)
{
  QDomDocument document;
  QFile f(pvlFilename.toLatin1().data());
  if (f.open(QIODevice::ReadOnly))
    {
      document.setContent(&f);
      f.close();
    }

  QDomElement main = document.documentElement();
  QDomNodeList dlist = main.childNodes();
  for(int i=0; i<dlist.count(); i++)
    {
      if (dlist.at(i).nodeName() == "voxelunit")
	{
	  QString pvalue = dlist.at(i).toElement().text();	  
	  if (pvalue == "angstrom")
	    return "A";
	  else if (pvalue == "nanometer")
	    return "nm";
	  else if (pvalue == "micron")
	    return "um";
	  else if (pvalue == "millimeter")
	    return "mm";
	  else if (pvalue == "centimeter")
	    return "cm";
	  else if (pvalue == "meter")
	    return "m";
	  else if (pvalue == "kilometer")
	    return "km";
	  else if (pvalue == "parsec")
	    return "p";
	  else if (pvalue == "kiloparsec")
	    return "kp";	  
	}
    }

  return "";
}

void
StaticFunctions::getDimensionsFromHeader(QString pvlFilename,
					 int &d, int &w, int &h)
{
  QDomDocument document;
  QFile f(pvlFilename.toLatin1().data());
  if (f.open(QIODevice::ReadOnly))
    {
      document.setContent(&f);
      f.close();
    }

  QDomElement main = document.documentElement();
  QDomNodeList dlist = main.childNodes();
  for(int i=0; i<dlist.count(); i++)
    {
      if (dlist.at(i).nodeName() == "gridsize")
	{
	  QStringList str = (dlist.at(i).toElement().text()).split(" ", QString::SkipEmptyParts);
	  d = str[0].toFloat();
	  w = str[1].toFloat();
	  h = str[2].toFloat();
	  return;
	}
    }
}

int
StaticFunctions::getSlabsizeFromHeader(QString pvlFilename)
{
  QDomDocument document;
  QFile f(pvlFilename.toLatin1().data());
  if (f.open(QIODevice::ReadOnly))
    {
      document.setContent(&f);
      f.close();
    }

  QDomElement main = document.documentElement();
  QDomNodeList dlist = main.childNodes();
  for(int i=0; i<dlist.count(); i++)
    {
      if (dlist.at(i).nodeName() == "slabsize")
	return (dlist.at(i).toElement().text()).toInt();
    }
  return 0;
}

int
StaticFunctions::getPvlHeadersizeFromHeader(QString pvlFilename)
{
  QDomDocument document;
  QFile f(pvlFilename.toLatin1().data());
  if (f.open(QIODevice::ReadOnly))
    {
      document.setContent(&f);
      f.close();
    }

  QDomElement main = document.documentElement();
  QDomNodeList dlist = main.childNodes();
  for(int i=0; i<dlist.count(); i++)
    {
      if (dlist.at(i).nodeName() == "pvlheadersize")
	return (dlist.at(i).toElement().text()).toInt();
    }

  // default is 13 byte header
  return 13;
}

int
StaticFunctions::getRawHeadersizeFromHeader(QString pvlFilename)
{
  QDomDocument document;
  QFile f(pvlFilename.toLatin1().data());
  if (f.open(QIODevice::ReadOnly))
    {
      document.setContent(&f);
      f.close();
    }

  QDomElement main = document.documentElement();
  QDomNodeList dlist = main.childNodes();
  for(int i=0; i<dlist.count(); i++)
    {
      if (dlist.at(i).nodeName() == "rawheadersize")
	return (dlist.at(i).toElement().text()).toInt();
    }

  // default is 13 byte header
  return 13;
}

QStringList
StaticFunctions::getPvlNamesFromHeader(QString pvlFilename)
{
  QStringList filenames;

  QDomDocument document;
  QFile f(pvlFilename.toLatin1().data());
  if (f.open(QIODevice::ReadOnly))
    {
      document.setContent(&f);
      f.close();
    }

  QDomElement main = document.documentElement();
  QDomNodeList dlist = main.childNodes();
  for(int i=0; i<dlist.count(); i++)
    {
      if (dlist.at(i).nodeName() == "pvlnames")
	{
	  QString names = (dlist.at(i).toElement().text()).simplified();
	  QStringList flnms = names.split(" ", QString::SkipEmptyParts);

	  QFileInfo fileInfo(pvlFilename);
	  QDir direc = fileInfo.absoluteDir();
	  for(int fi=0; fi<flnms.count(); fi++)
	    {
	      fileInfo.setFile(direc, flnms[fi]);
	      filenames << fileInfo.absoluteFilePath();
	    }

	  return filenames;
	}
    }

  return filenames;
}

QStringList
StaticFunctions::getRawNamesFromHeader(QString pvlFilename)
{
  QStringList filenames;

  QDomDocument document;
  QFile f(pvlFilename.toLatin1().data());
  if (f.open(QIODevice::ReadOnly))
    {
      document.setContent(&f);
      f.close();
    }

  QDomElement main = document.documentElement();
  QDomNodeList dlist = main.childNodes();
  for(int i=0; i<dlist.count(); i++)
    {
      if (dlist.at(i).nodeName() == "rawnames")
	{
	  QString names = (dlist.at(i).toElement().text()).simplified();
	  QStringList flnms = names.split(" ", QString::SkipEmptyParts);

	  QFileInfo fileInfo(pvlFilename);
	  QDir direc = fileInfo.absoluteDir();
	  for(int fi=0; fi<flnms.count(); fi++)
	    {
	      fileInfo.setFile(direc, flnms[fi]);
	      filenames << fileInfo.absoluteFilePath();
	    }

	  return filenames;
	}
    }

  return filenames;
}

bool
StaticFunctions::xmlHeaderFile(QString volfile)
{
  bool xmlheader = false;

  QFile qfl(volfile);
  if (!qfl.open(QIODevice::ReadOnly | QIODevice::Text))
    {
      QMessageBox::information(0, "Cannot open", volfile);
      return false;
    }
  QString line = qfl.readLine();
  // interested in second line
  line = qfl.readLine();
  line = line.trimmed();
  //QMessageBox::information(0, volfile, "["+line+"]");
  if (line == "<PvlDotNcFileHeader>")
    xmlheader = true;
  qfl.close();

  return xmlheader;
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

void
StaticFunctions::drawEnclosingCube(Vec subvolmin,
				   Vec subvolmax)
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

