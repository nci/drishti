#include "staticfunctions.h"
#include <math.h>


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

  QFileInfo info(flnm);
  if (info.exists() && info.isFile())
    {
      QByteArray exten = flnm.toLatin1().right(extlen);
      if (exten != ext)
	ok = false;
    }
  else
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
