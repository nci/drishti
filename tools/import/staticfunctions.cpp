#include "staticfunctions.h"
#include <QRegExp>

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
StaticFunctions::checkRegExp(QString flnm, QString regex)
{
  QRegExp rx(regex);
  rx.setPatternSyntax(QRegExp::Wildcard);

  bool ok = true;

  QFileInfo info(flnm);
  if (info.exists() && info.isFile())
    {
      if (! rx.exactMatch(flnm))
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

  for(uint i=0; i<urls.count(); i++)
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
StaticFunctions::checkURLsRegExp(QList<QUrl> urls, QString regex)
{
  QRegExp rx(regex);
  rx.setPatternSyntax(QRegExp::Wildcard);

  bool ok = true;

  for(uint i=0; i<urls.count(); i++)
    {
      QUrl url = urls[i];
      QFileInfo info(url.toLocalFile());
      if (info.exists() && info.isFile())
	{
	  if (! rx.exactMatch(url.toLocalFile()))
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

QGradientStops
StaticFunctions::resampleGradientStops(QGradientStops stops)
{
  QColor colorMap[101];

  int startj, endj;
  for(int i=0; i<stops.size(); i++)
    {
      float pos = stops[i].first;
      QColor color = stops[i].second;
      endj = pos*100;
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
  for (int i=0; i<100; i++)
    {
      float pos = (float)i/100.0f;
      newStops << QGradientStop(pos, colorMap[i]);
    }

  return newStops;
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
StaticFunctions::swapbytes(uchar *ptr, int nbytes)
{
  for(uint i=0; i<nbytes/2; i++)
    {
      uchar t;
      t = ptr[i];
      ptr[i] = ptr[nbytes-1-i];
      ptr[nbytes-1-i] = t;
    }
}

void
StaticFunctions::swapbytes(uchar *ptr, int bpv, int nbytes)
{
  int nb = nbytes/bpv;
  for(uint j=0; j<nb; j++)
    {
      uchar *p = ptr + bpv*j;
      for(uint i=0; i<bpv/2; i++)
	{
	  uchar t;
	  t = p[i];
	  p[i] = p[bpv-1-i];
	  p[bpv-1-i] = t;
	}
    }

}
