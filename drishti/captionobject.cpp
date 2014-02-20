#include "global.h"
#include "captionobject.h"

QImage CaptionObject::image()
{
  if (m_recreate)
    createImage();

  return m_image;
}

CaptionObject::CaptionObject() { clear(); }
CaptionObject::~CaptionObject() { clear(); }
void
CaptionObject::clear()
{
  m_image = QImage();
  m_pos = QPointF(0,0);
  m_text.clear();
  m_font = QFont();
  m_color = Qt::white;
  m_haloColor = Qt::white;
  m_angle = 0;
  m_recreate = false;

  m_height = 0;
  m_width = 0;
}

void
CaptionObject::createImage()
{
  m_recreate = false;
  QStringList regExprs;
  regExprs << "\\$[0-9]*[f|F]";
  regExprs << "\\$[0-9]*[v|V][0-3]*";
  regExprs << "\\$[n|N]\\(\\d*\\.*\\d*\\)";
  regExprs << "\\$[d|D]\\(\\d*\\.*\\d*\\)";

  QString finalText = m_text;

  bool drawPi = false;
  float drawPiAngle = 0;
  for (int nr=0; nr<regExprs.count(); nr++)
    {
      QRegExp rx(regExprs[nr]);
      if (rx.indexIn(finalText) > -1)
	{
	  m_recreate = true;

	  QString txt = rx.cap();
	  QChar fillChar = '0';
	  int fieldWidth = 0;
	  if (txt.length() > 2)
	    {
	      txt.remove(0,1);
	      txt.chop(1);
	      if (txt.endsWith("v", Qt::CaseInsensitive))
		txt.chop(1);
	      fieldWidth = txt.toInt();
	    }

	  QString ftxt;
	  if (nr == 0)
	    ftxt = QString("%1").arg((int)Global::frameNumber(),
				     fieldWidth, 10, fillChar);
	  else if (nr == 1)
	    {
	      QString txt = rx.cap();
	      QStringList rem = txt.split("v",
					  QString::SkipEmptyParts,
					  Qt::CaseInsensitive);
	      int vol = 0;
	      if (rem.count() > 1)
		vol = rem[1].toInt();

	      ftxt = QString("%1").arg((int)Global::actualVolumeNumber(vol),
				       fieldWidth, 10, fillChar);
	    }
	  else if (nr == 2)
	    {
	      QStringList rem = txt.split("(");
	      if (rem.count() > 1)
		ftxt = rem[1];
	    }
	  else if (nr == 3)
	    {
	      drawPi = true;
	      QStringList rem = txt.split("(");
	      if (rem.count() > 1)
		drawPiAngle = rem[1].toDouble();
	    }
	    
	  finalText.replace(rx, ftxt);
	}
    }

  // replace symbol for micron
    if (finalText.endsWith(" um"))
      {
	finalText.chop(2);
	finalText += QChar(0xB5);
	finalText += "m";
      }

  QFontMetrics metric(m_font);
  m_width = metric.width(finalText);

  int mde = metric.descent();
  int fwd = m_width;
  int fht = m_height;
  if (drawPi > 0)
    fwd += fht;

  QImage bImage = QImage(fwd, fht, QImage::Format_ARGB32);
  bImage.fill(0);
  {
    QPainter bpainter(&bImage);
    // we have image as ARGB, but we want RGBA
    // so switch red and blue colors here itself
    QColor penColor(m_haloColor.blue(),
		    m_haloColor.green(),
		    m_haloColor.red());
    bpainter.setPen(penColor);
    bpainter.setFont(m_font);
    if (drawPi)
      bpainter.drawText(fht+1, fht-mde, finalText);
    else
      bpainter.drawText(1, fht-mde, finalText);

    if (drawPi)
      {
	bpainter.setBrush(penColor);
	bpainter.setPen(Qt::NoPen);
	bpainter.drawPie(3,3, fht-6, fht-6,
			 90*16, -360*16);
      }

    uchar *dbits = new uchar[4*fht*fwd];
    uchar *bits = bImage.bits();
    for(int nt=0; nt < 3; nt++)
      {
	memcpy(dbits, bits, 4*fht*fwd);

	for(int i=2; i<fht-2; i++)
	  for(int j=2; j<fwd-2; j++)
	    {
	      for (int k=0; k<4; k++)
		{
		  int sum = 0;
		  
		  for(int i0=-2; i0<=2; i0++)
		    for(int j0=-2; j0<=2; j0++)
		      sum += dbits[4*((i+i0)*fwd+(j+j0)) + k];
		  
		  bits[4*(i*fwd+j) + k] = sum/25;
		}
	    }
      }
    delete [] dbits;
  }


  QImage cImage = QImage(fwd, fht, QImage::Format_ARGB32);
  cImage.fill(0);
  {
    QPainter cpainter(&cImage);
    // first draw the halo image
    cpainter.drawImage(0, 0, bImage);

    // we have image as ARGB, but we want RGBA
    // so switch red and blue colors here itself
    QColor penColor(m_color.blue(),
		    m_color.green(),
		    m_color.red());
    cpainter.setPen(penColor);
    cpainter.setFont(m_font);
    if (drawPi)
      cpainter.drawText(fht+1, fht-mde, finalText);
    else
      cpainter.drawText(1, fht-mde, finalText);

    if (drawPi)
      {
	QColor bgColor(m_haloColor.blue(),
		       m_haloColor.green(),
		       m_haloColor.red());
	cpainter.setPen(Qt::NoPen);
	int nturns = drawPiAngle/360;
	if (qAbs(nturns)%2 == 0)
	  {
	    cpainter.setBrush(bgColor);
	    cpainter.drawPie(3,3, fht-6, fht-6,
			     90*16, -360*16);
	    if(fabs(drawPiAngle-nturns*360) > 0)
	      {		
		cpainter.setBrush(penColor);
		cpainter.drawPie(3,3, fht-6, fht-6,
				 90*16, -(drawPiAngle-nturns*360)*16);
	      }
	  }
	else
	  {
	    cpainter.setBrush(penColor);
	    cpainter.drawPie(3,3, fht-6, fht-6,
			     90*16, -360*16);
	    if(fabs(drawPiAngle-nturns*360) > 0)
	      {		
		cpainter.setBrush(bgColor);
		cpainter.drawPie(3,3, fht-6, fht-6,
				 90*16, -(drawPiAngle-nturns*360)*16);
	      }
	  }

	cpainter.setBrush(Qt::NoBrush);
	cpainter.setPen(penColor);
	cpainter.drawPie(3,3, fht-6, fht-6,
			 90*16, -drawPiAngle*16);

      }

    float alpha = m_color.alpha()/255.0;
    uchar *bits = cImage.bits();
    for(int i=0; i<fht*fwd; i++)
      {
	bits[4*i+0] *= alpha;
	bits[4*i+1] *= alpha;
	bits[4*i+2] *= alpha;
	bits[4*i+3] *= alpha;
      }
  }
  
  m_image = cImage.mirrored();
}

bool
CaptionObject::hasCaption(QStringList str)
{
  for(int i=0; i<str.count(); i++)
    if (m_text.contains(str[i], Qt::CaseInsensitive) == false)
      return false;
  
  return true;
}

void
CaptionObject::setCaption(CaptionObject co)
{
  m_pos = co.position();
  m_text = co.text();
  m_font = co.font();
  m_color = co.color();
  m_haloColor = co.haloColor();
  m_angle = co.m_angle;

  QFontMetrics metric(m_font);
  m_height = metric.height();
  m_width = metric.width(m_text);

  createImage();
}

void
CaptionObject::set(QPointF pos, QString txt, QFont fnt,
		   QColor col, QColor halocol, float angle)
{
  m_pos = pos;
  m_text = txt;
  m_font = fnt;
  m_color = col;
  m_haloColor = halocol;
  m_angle = angle;

  QFontMetrics metric(m_font);
  m_height = metric.height();
  m_width = metric.width(m_text);

  createImage();
}

void CaptionObject::setAngle(float angle) { m_angle = angle; createImage(); }
void CaptionObject::setColor(QColor col) { m_color = col; createImage(); }
void CaptionObject::setHaloColor(QColor col) { m_haloColor = col; createImage(); }
void CaptionObject::setPosition(QPointF pos) { m_pos = pos; }
void CaptionObject::setText(QString txt)
{
  m_text = txt;
  QFontMetrics metric(m_font);
  m_height = metric.height();
  m_width = metric.width(m_text);
  createImage();
}
void CaptionObject::setFont(QFont fnt)
{
  m_font = fnt;

  QFontMetrics metric(m_font);
  m_height = metric.height();
  m_width = metric.width(m_text);

  createImage();
}

float CaptionObject::angle() { return m_angle; }
QColor CaptionObject::color() { return m_color; }
QColor CaptionObject::haloColor() { return m_haloColor; }
QPointF CaptionObject::position() { return m_pos; }
QString CaptionObject::text() { return m_text; }
QFont CaptionObject::font() { return m_font; }
int CaptionObject::height() { return m_image.height(); }
int CaptionObject::width() { return m_image.width(); }

QList<CaptionObject>
CaptionObject::interpolate(QList<CaptionObject> cap1,
			   QList<CaptionObject> cap2,
			   float frc)
{
  QList<CaptionObject> cap;
  for (int i=0; i<qMin(cap1.count(),
			cap2.count()); i++)
    {
      cap.append(interpolate(cap1[i], cap2[i], frc));
    }

  int st = cap.count();
  for (int i=st; i<cap1.size(); i++)
    {
      cap.append(cap1[i]);
    }
  
  return cap;
}

CaptionObject
CaptionObject::interpolate(CaptionObject& cap1,
			   CaptionObject& cap2,
			   float frc)
{
  bool ok = false;

  QString finalText = cap1.text();


  QStringList regExprs;
  regExprs << "\\$[n|N]\\(\\d*\\.*\\d*\\)";
  regExprs << "\\$[d|D]\\(\\d*\\.*\\d*\\)";
  for (int nr=0; nr<regExprs.count(); nr++)
    {
      QRegExp rx(regExprs[nr]);
      QStringList rem;
      QString txt;

      float val1;
      rx.indexIn(cap1.text());      
      txt = rx.cap();
      rem = txt.split(QRegExp("\\(|\\)"));
      if (rem.count() > 1) val1 = rem[1].toFloat();

      float val2;
      rx.indexIn(cap2.text());      
      txt = rx.cap();
      rem = txt.split(QRegExp("\\(|\\)"));
      if (rem.count() > 1) val2 = rem[1].toFloat();

      float val = (1-frc)*val1 + frc*val2;

      QString ftxt = QString("%1").arg(val, 0, 'f', Global::floatPrecision());
      if (Global::floatPrecision() > 0)
	{
	  QStringList frem = ftxt.split(".");
	  float fraction = frem[1].toFloat();
	  if (fraction < 0.00001)
	    ftxt = frem[0];
	}

      if (nr == 0)
	finalText.replace(rx, ftxt);
      else
	finalText.replace(rx, "$d("+ftxt+")");
      ok = true;
    }

  if (!ok &&
      cap1.text() != cap2.text())
    {
      // apply fadein-fadeout
      CaptionObject cap;

      if (frc <= 0.5)
	cap.setCaption(cap1);
      else
	cap.setCaption(cap2);
      
//      QColor c = cap.color();
//      float r = c.redF();
//      float g = c.greenF();
//      float b = c.blueF();
//      float a;
//
//      if (frc <= 0.5)
//	a = (1-frc)*c.alphaF();
//      else
//	a = frc*c.alphaF();      
//      c = QColor::fromRgbF(r,g,b,a);      
//      cap.setColor(c);
//
//      QColor hc = cap.haloColor();
//      r = hc.redF();
//      g = hc.greenF();
//      b = hc.blueF();
//      if (frc <= 0.5)
//	a = (1-frc)*hc.alphaF();
//      else
//	a = frc*hc.alphaF();
//      hc = QColor::fromRgbF(r,g,b,a);      
//      cap.setHaloColor(hc);

      return cap;
    }

  // interpolate position, color and font

  CaptionObject cap;

  QFont fnt = cap1.font();
  QFont fnt2 = cap2.font();

  if (fnt.family() == fnt2.family())
    {
      // interpolate pointsize
      int pt1 = fnt.pointSize();
      int pt2 = fnt2.pointSize();
      int pt = (1-frc)*pt1 + frc*pt2;
      fnt.setPointSize(pt);
    }

  // interpolate position
  QPointF pos1 = cap1.position();
  QPointF pos2 = cap2.position();
  QPointF pos = pos1-pos2;  
  if (fabs(pos.x()) > 5 || fabs(pos.y()) > 5 ) // more than 5 pixel change
    pos = (1-frc)*pos1 + frc*pos2;
  else
    {
      if (frc <0.5)
	pos = pos1;
      else
	pos = pos2;
    }

  // interpolate color
  QColor c = cap1.color();
  float r1 = c.redF();
  float g1 = c.greenF();
  float b1 = c.blueF();
  float a1 = c.alphaF();
  c = cap2.color();
  float r2 = c.redF();
  float g2 = c.greenF();
  float b2 = c.blueF();
  float a2 = c.alphaF();
  float r = (1-frc)*r1 + frc*r2;
  float g = (1-frc)*g1 + frc*g2;
  float b = (1-frc)*b1 + frc*b2;
  float a = (1-frc)*a1 + frc*a2;
  c = QColor(r*255, g*255, b*255, a*255);

  // interpolate halocolor
  QColor hc = cap1.haloColor();
  r1 = hc.redF();
  g1 = hc.greenF();
  b1 = hc.blueF();
  a1 = hc.alphaF();
  hc = cap2.haloColor();
  r2 = hc.redF();
  g2 = hc.greenF();
  b2 = hc.blueF();
  a2 = c.alphaF();
  r = (1-frc)*r1 + frc*r2;
  g = (1-frc)*g1 + frc*g2;
  b = (1-frc)*b1 + frc*b2;
  a = (1-frc)*a1 + frc*a2;
  hc = QColor(r*255, g*255, b*255, a*255);

  float angle = (1-frc)*cap1.angle() + frc*cap2.angle();
  cap.set(pos,
	  finalText,
	  fnt,
	  c, hc,
	  angle);

  return cap;
}


void
CaptionObject::load(fstream& fin)
{
  clear();

  int len;
  bool done = false;
  char keyword[100];
  while (!done)
    {
      fin.getline(keyword, 100, 0);

      if (strcmp(keyword, "captionobjectend") == 0)
	done = true;
      else if (strcmp(keyword, "position") == 0)
	{
	  float x, y;
	  fin.read((char*)&x, sizeof(float));
	  fin.read((char*)&y, sizeof(float));
	  m_pos = QPointF(x,y);
	}
      else if (strcmp(keyword, "text") == 0)
	{
	  fin.read((char*)&len, sizeof(int));
	  char *str = new char[len];
	  fin.read((char*)str, len*sizeof(char));
	  m_text = QString(str);
	  delete [] str;
	}
      else if (strcmp(keyword, "font") == 0)
	{
	  fin.read((char*)&len, sizeof(int));
	  char *str = new char[len];
	  fin.read((char*)str, len*sizeof(char));
	  QString fontStr = QString(str);
	  m_font.fromString(fontStr); 
	  delete [] str;
	}
      else if (strcmp(keyword, "color") == 0)
	{
	  unsigned char r, g, b, a;
	  fin.read((char*)&r, sizeof(unsigned char));
	  fin.read((char*)&g, sizeof(unsigned char));
	  fin.read((char*)&b, sizeof(unsigned char));
	  fin.read((char*)&a, sizeof(unsigned char));
	  m_color = QColor(r,g,b,a);
	}
      else if (strcmp(keyword, "halocolor") == 0)
	{
	  unsigned char r, g, b, a;
	  fin.read((char*)&r, sizeof(unsigned char));
	  fin.read((char*)&g, sizeof(unsigned char));
	  fin.read((char*)&b, sizeof(unsigned char));
	  fin.read((char*)&a, sizeof(unsigned char));
	  m_haloColor = QColor(r,g,b,a);
	}
      else if (strcmp(keyword, "angle") == 0)
	{
	  fin.read((char*)&m_angle, sizeof(float));
	}
    }

  QFontMetrics metric(m_font);
  m_height = metric.height();
  m_width = metric.width(m_text);

  createImage();
}

void
CaptionObject::save(fstream& fout)
{
  char keyword[100];
  int len;

  memset(keyword, 0, 100);
  sprintf(keyword, "captionobjectstart");
  fout.write((char*)keyword, strlen(keyword)+1);

  float x = m_pos.x();
  float y = m_pos.y();
  memset(keyword, 0, 100);
  sprintf(keyword, "position");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&x, sizeof(float));
  fout.write((char*)&y, sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "text");
  fout.write((char*)keyword, strlen(keyword)+1);
  len = m_text.size()+1;
  fout.write((char*)&len, sizeof(int));
  fout.write((char*)m_text.toLatin1().data(), len*sizeof(char));

  QString fontStr = m_font.toString();
  memset(keyword, 0, 100);
  sprintf(keyword, "font");
  fout.write((char*)keyword, strlen(keyword)+1);
  len = fontStr.size()+1;
  fout.write((char*)&len, sizeof(int));
  fout.write((char*)fontStr.toLatin1().data(), len*sizeof(char));
  
  unsigned char r = m_color.red();
  unsigned char g = m_color.green();
  unsigned char b = m_color.blue();
  unsigned char a = m_color.alpha();
  memset(keyword, 0, 100);
  sprintf(keyword, "color");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&r, sizeof(unsigned char));
  fout.write((char*)&g, sizeof(unsigned char));
  fout.write((char*)&b, sizeof(unsigned char));
  fout.write((char*)&a, sizeof(unsigned char));

  r = m_haloColor.red();
  g = m_haloColor.green();
  b = m_haloColor.blue();
  a = m_haloColor.alpha();
  memset(keyword, 0, 100);
  sprintf(keyword, "halocolor");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&r, sizeof(unsigned char));
  fout.write((char*)&g, sizeof(unsigned char));
  fout.write((char*)&b, sizeof(unsigned char));
  fout.write((char*)&a, sizeof(unsigned char));

  memset(keyword, 0, 100);
  sprintf(keyword, "angle");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_angle, sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "captionobjectend");
  fout.write((char*)keyword, strlen(keyword)+1);
}

