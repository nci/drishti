#include "staticfunctions.h"
#include <math.h>

#include <QMessageBox>
#include <QDialog>
#include <QFileDialog>
#include <QTextEdit>
#include <QVBoxLayout>

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
  QColor *colorMap;
  colorMap = new QColor[65536];

  mapSize = qMin(65536, mapSize);
  
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

  delete [] colorMap;
  
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

  QByteArray exten = flnm.toUtf8().right(extlen);
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
	  QByteArray exten = url.toLocalFile().toUtf8().right(extlen);
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
  QFile f(volfile.toUtf8().data());
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
  QFile f(volfile.toUtf8().data());
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
  QDomDocument document;
  QFile f(pvlFilename.toUtf8().data());
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
	  if (vx <=0 || vy <= 0 || vz <= 0)
	    {
	      QMessageBox::critical(0, "Voxel Size Error",
				    QString("Voxel size <= 0 not allowed\nDefaulting to 1 1 1"),
				    QString("%1 %2 %3").arg(vx).arg(vy).arg(vz));
	      vx = vy = vz = 1;
	    }
	  
	  return Vec(vx, vy, vz);
	}
    }
  return Vec(1,1,1);
}

QString
StaticFunctions::getVoxelUnitFromHeader(QString pvlFilename)
{
  QDomDocument document;
  QFile f(pvlFilename.toUtf8().data());
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
  QFile f(pvlFilename.toUtf8().data());
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
  QFile f(pvlFilename.toUtf8().data());
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
StaticFunctions::getPvlVoxelTypeFromHeader(QString pvlFilename)
{
  QDomDocument document;
  QFile f(pvlFilename.toUtf8().data());
  if (f.open(QIODevice::ReadOnly))
    {
      document.setContent(&f);
      f.close();
    }

  QDomElement main = document.documentElement();
  QDomNodeList dlist = main.childNodes();
  for(int i=0; i<dlist.count(); i++)
    {
      if (dlist.at(i).nodeName() == "pvlvoxeltype")
	{
	  if (dlist.at(i).toElement().text() == "unsigned char") return 0;
	  if (dlist.at(i).toElement().text() == "char") return 1;
	  if (dlist.at(i).toElement().text() == "unsigned short") return 2;
	  if (dlist.at(i).toElement().text() == "short") return 3;
	  if (dlist.at(i).toElement().text() == "int") return 4;
	  if (dlist.at(i).toElement().text() == "float") return 5;
	}
    }
  return 0;
}

int
StaticFunctions::getPvlHeadersizeFromHeader(QString pvlFilename)
{
  QDomDocument document;
  QFile f(pvlFilename.toUtf8().data());
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
  QFile f(pvlFilename.toUtf8().data());
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
  QFile f(pvlFilename.toUtf8().data());
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
  QFile f(pvlFilename.toUtf8().data());
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

QSize
StaticFunctions::getImageSize(int width, int height)
{
  QSize imgSize = QSize(width, height);

  QStringList items;
  items << QString("%1 %2 (Current)").arg(width).arg(height);
  items << "320 200 (CGA)";
  items << "320 240 (QVGA)";
  items << "640 480 (VGA)";
  items << "720 480 (NTSC)";
  items << "720 576 (PAL)";
  items << "800 480 (WVGA)";
  items << "800 600 (SVGA)";
  items << "854 480 (WVGA 16:9)";
  items << "1024 600 (WSVGA)";
  items << "1024 768 (XGA)";
  items << "1280 720 (720p 16:9)";
  items << "1280 768 (WXGA)";
  items << "1280 1024 (SXGA)";
  items << "1366 768 (16:9)";
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

QList<Vec>
StaticFunctions::line3d(Vec v0, Vec v1)
{
  QList<Vec> line;

  int gx0, gy0, gz0, gx1, gy1, gz1;
  gx0 = v0.x;
  gy0 = v0.y;
  gz0 = v0.z;
  gx1 = v1.x;
  gy1 = v1.y;
  gz1 = v1.z;

  float vx = gx1 - gx0;
  float vy = gy1 - gy0;
  float vz = gz1 - gz0;
  
  int sx = (gx1>gx0) ? 1 : (gx1<gx0) ? -1 : 0;
  int sy = (gy1>gy0) ? 1 : (gy1<gy0) ? -1 : 0;
  int sz = (gz1>gz0) ? 1 : (gz1<gz0) ? -1 : 0;
 
  int gx = gx0;
  int gy = gy0;
  int gz = gz0;

  //Planes for each axis that we will next cross
  int gxp = gx0 + ((gx1>gx0) ? 1 : 0);
  int gyp = gy0 + ((gy1>gy0) ? 1 : 0);
  int gzp = gz0 + ((gz1>gz0) ? 1 : 0);
  
  do {
    line << Vec(gx, gy, gz);

    if (gx == gx1 &&
	gy == gy1 &&
	gz == gz1)
      break;
    
    //Which plane do we cross first?
    float xr=0,yr=0,zr=0;
    if (qAbs(vx) > 0) xr = qAbs((gxp - gx0)/vx);
    if (qAbs(vy) > 0) yr = qAbs((gyp - gy0)/vy);
    if (qAbs(vz) > 0) zr = qAbs((gzp - gz0)/vz);

    if (sx != 0 &&
	(sy == 0 || xr < yr) &&
	(sz == 0 || xr < zr))
      {
      gx += sx;
      gxp += sx;
    }
    else if (sy != 0 && 
	     (sz == 0 || yr < zr))
      {
	gy += sy;
	gyp += sy;
      }
    else if (sz != 0)
      {
	gz += sz;
	gzp += sz;
      }
    
    if (qAbs(gx-gx0) > qAbs(vx) ||
	qAbs(gy-gy0) > qAbs(vy) ||
	qAbs(gz-gz0) > qAbs(vz))
      break;
    
  } while (true);
  
  return line;
}


QList<int>
StaticFunctions::dda2D(int xa, int ya, int xb, int yb)
{
  int dx = xb-xa;
  int dy = yb-ya;
  int steps = qAbs(dx)>qAbs(dy) ? qAbs(dx) : qAbs(dy);
  float xinc = dx/(float)steps;
  float yinc = dy/(float)steps;
  QList<int> xy;
  for(int i=0; i<=steps; i++)
    {
      float x = xa + i*xinc;
      float y = ya + i*yinc;
      xy << x << y;
    }
  return xy;
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

void
StaticFunctions::smooth2DArray(float* in, float* out, int nW, int nH)
{
  float *b = new float[nW*nH];
  memset(b, 0, nW*nH*sizeof(float));
  
  for (int i=0; i<nW; i++)
    for (int j=0; j<nH; j++)
      {
	b[i*nH + j] = (in[i*nH + qMax(0,j-1)] +in[i*nH + j] + in[i*nH + qMin(nH-1,j+1)]);
      }
    
  
  for (int j=0; j<nH; j++)
    for (int i=0; i<nW; i++)
      {
	out[i*nH + j] = (b[qMax(0,i-1)*nH + j] +b[i*nH + j] + b[qMin(nW-1,i+1)*nH + j])/9;
      }

  delete [] b;
}


void
StaticFunctions::showMessage(QString title, QString mesg)
{
  QTextEdit *tedit = new QTextEdit();
  tedit->setPlainText(mesg);
  tedit->setReadOnly(true);
  
  QVBoxLayout *layout = new QVBoxLayout();
  layout->addWidget(tedit);
	
  QDialog *info = new QDialog();
  info->setWindowTitle(title);
  info->setSizeGripEnabled(true);
  info->setModal(true);
  info->setLayout(layout);
  info->exec();
}


void
StaticFunctions::imageFromDataAndColor(QImage &img, ushort *data, uchar *tagColors)
{
  if (data == 0)
    return;
  
  uchar *cbit = img.bits();
  int idx = 0;
  for(int h=0; h<img.height(); h++)
    for(int w=0; w<img.width(); w++)
      {
	int tag = data[idx];

	int r = tagColors[4*tag+0];
	int g = tagColors[4*tag+1];
	int b = tagColors[4*tag+2];
	int a = tagColors[4*tag+3];
	
	cbit[4*idx+0] = b;
	cbit[4*idx+1] = g;
	cbit[4*idx+2] = r;
	cbit[4*idx+3] = a;

	idx++;
      }
}


void
StaticFunctions::saveMesgToFile(QString prevDir, QString mesg)
{
  QString tflnm = QFileDialog::getSaveFileName(0,
					       "Save Information",
					       prevDir,
					       "Files (*.txt)",
					       0);
  if (tflnm.isEmpty())
    return;

  QFile txtfile(tflnm);
  if (txtfile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
      QTextStream out(&txtfile);
      out << mesg;
      QMessageBox::information(0, "Save", QString("Saved to %1").arg(tflnm));
    }
  else
    QMessageBox::information(0, "Error", QString("Cannot write to %1").arg(tflnm));
}


void
StaticFunctions::saveVolumeToFile(QString vflnm,
				  uchar vt, char *vol,
				  int mx, int my, int mz)
{
  int bpv = 1;
  if (vt < 2)
    bpv = 1;
  else if (vt < 4)
    bpv = 2;
  else
    bpv = 4;
  
  QFile qfile;
  qfile.setFileName(vflnm);
  qfile.open(QFile::WriteOnly);  
  qfile.write((char*)&vt, 1);
  qfile.write((char*)&mz, 4);
  qfile.write((char*)&my, 4);
  qfile.write((char*)&mx, 4);
  qfile.write((char*)vol, (qint64)mx*(qint64)my*(qint64)mz*bpv);
  qfile.close();

  QMessageBox::information(0, "Save", QString("Saved to %1").arg(vflnm));
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
    if (voxelType == 0)      vstr = "unsigned char";
    else if (voxelType == 1)  vstr = "char";
    else if (voxelType == 2)vstr = "unsigned short";
    else if (voxelType == 3) vstr = "short";
    else if (voxelType == 4)   vstr = "int";
    else if (voxelType == 5) vstr = "float";
    
    QDomElement de0 = doc.createElement("voxeltype");
    QDomText tn0;
    tn0 = doc.createTextNode(QString("%1").arg(vstr));
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }


  {      
    QString vstr;
    if (pvlVoxelType == 0)      vstr = "unsigned char";
    else if (pvlVoxelType == 1)  vstr = "char";
    else if (pvlVoxelType == 2)vstr = "unsigned short";
    else if (pvlVoxelType == 3) vstr = "short";
    else if (pvlVoxelType == 4)   vstr = "int";
    else if (pvlVoxelType == 5) vstr = "float";
    
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
    if (voxelUnit == 0)      vstr = "no units";
    else if (voxelUnit == 1) vstr = "angstrom";
    else if (voxelUnit == 2) vstr = "nanometer";
    else if (voxelUnit == 3) vstr = "micron";
    else if (voxelUnit == 4) vstr = "millimeter";
    else if (voxelUnit == 5) vstr = "centimeter";
    else if (voxelUnit == 6) vstr = "meter";
    else if (voxelUnit == 7) vstr = "kilometer";
    else if (voxelUnit == 8) vstr = "parsec";
    else if (voxelUnit == 9) vstr = "kiloparsec";
    
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
  
  QFile f(xmlfile.toUtf8().data());
  if (f.open(QIODevice::WriteOnly))
    {
      QTextStream out(&f);
      doc.save(out, 2);
      f.close();
    }
}
