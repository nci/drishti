#include "global.h"
#include "scalebarobject.h"
#include "volumeinformation.h"

ScaleBarObject::ScaleBarObject() { clear(); }
ScaleBarObject::~ScaleBarObject() { clear(); }

void
ScaleBarObject::clear()
{
  m_pos = QPointF(0,0);
  m_voxels = 10;
  m_type = true; // horizontal
  m_textpos = true; // up/right
}

ScaleBarObject::ScaleBarObject(const ScaleBarObject &co)
{
  m_pos = co.m_pos;
  m_voxels = co.m_voxels;
  m_type = co.m_type;
  m_textpos = co.m_textpos;
}

ScaleBarObject&
ScaleBarObject::operator=(const ScaleBarObject &co)
{
  m_pos = co.m_pos;
  m_voxels = co.m_voxels;
  m_type = co.m_type;
  m_textpos = co.m_textpos;

  return *this;
}

void
ScaleBarObject::setScaleBar(ScaleBarObject co)
{
  m_pos = co.position();
  m_voxels = co.voxels();
  m_type = co.type();
  m_textpos = co.textpos();
}

void
ScaleBarObject::set(QPointF pos, float vox, bool t, bool tp)
{
  m_pos = pos;
  m_voxels = vox;
  m_type = t;
  m_textpos = tp;
}

void ScaleBarObject::setPosition(QPointF pos) { m_pos = pos; }
void ScaleBarObject::setVoxels(float vox) { m_voxels = vox; }
void ScaleBarObject::setType(bool t) { m_type = t; }
void ScaleBarObject::setTextpos(bool tp) { m_textpos = tp; }

QPointF ScaleBarObject::position() { return m_pos; }
float ScaleBarObject::voxels() { return m_voxels; }
bool ScaleBarObject::type() { return m_type; }
bool ScaleBarObject::textpos() { return m_textpos; }

QList<ScaleBarObject>
ScaleBarObject::interpolate(QList<ScaleBarObject> sb1,
			    QList<ScaleBarObject> sb2,
			    float frc)
{
  QList<ScaleBarObject> sb;
  for (int i=0; i<qMin(sb1.count(),
		       sb2.count()); i++)
    sb.append(interpolate(sb1[i], sb2[i], frc));
  
  int st = sb.count();
  for (int i=st; i<sb1.size(); i++)
    sb.append(sb1[i]);
  
  return sb;
}

ScaleBarObject
ScaleBarObject::interpolate(ScaleBarObject& sb1,
			    ScaleBarObject& sb2,
			    float frc)
{
  ScaleBarObject sb;
  QPointF pos = (1-frc)*sb1.position() + frc*sb2.position();
  sb.setPosition(pos);
  if (frc <=0.5)
    {
      sb.setVoxels(sb1.voxels());
      sb.setType(sb1.type());
      sb.setTextpos(sb1.textpos());
    }
  else
    {
      sb.setVoxels(sb2.voxels());
      sb.setType(sb2.type());
      sb.setTextpos(sb2.textpos());
    }
  return sb;
}

void
ScaleBarObject::load(fstream& fin)
{
  clear();

  int len;
  bool done = false;
  char keyword[100];
  while (!done)
    {
      fin.getline(keyword, 100, 0);

      if (strcmp(keyword, "scalebarobjectend") == 0)
	done = true;
      else if (strcmp(keyword, "position") == 0)
	{
	  float x, y;
	  fin.read((char*)&x, sizeof(float));
	  fin.read((char*)&y, sizeof(float));
	  m_pos = QPointF(x,y);
	}
      else if (strcmp(keyword, "voxels") == 0)
	fin.read((char*)&m_voxels, sizeof(float));
      else if (strcmp(keyword, "type") == 0)
	fin.read((char*)&m_type, sizeof(bool));
      else if (strcmp(keyword, "textpos") == 0)
	fin.read((char*)&m_textpos, sizeof(bool));
    }
}

void
ScaleBarObject::save(fstream& fout)
{
  char keyword[100];
  int len;

  memset(keyword, 0, 100);
  sprintf(keyword, "scalebarobjectstart");
  fout.write((char*)keyword, strlen(keyword)+1);

  float x = m_pos.x();
  float y = m_pos.y();
  memset(keyword, 0, 100);
  sprintf(keyword, "position");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&x, sizeof(float));
  fout.write((char*)&y, sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "voxels");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_voxels, sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "type");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_type, sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "textpos");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_textpos, sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "scalebarobjectend");
  fout.write((char*)keyword, strlen(keyword)+1);
}

void
ScaleBarObject::draw(float pixelGLRatio,
		     int screenWidth, int screenHeight,
		     int viewWidth, int viewHeight,
		     bool grabsMouse)
{
  VolumeInformation pvlInfo = VolumeInformation::volumeInformation();

  QString str;
  bool horizontal = m_type;
  float slen = voxels();
  if (pvlInfo.voxelUnit > 0)
    {
      float avg = (pvlInfo.voxelSize[0] +
		   pvlInfo.voxelSize[1] +
		   pvlInfo.voxelSize[2])/3.0f;
      
      str = QString("%1 %2").			     \
	arg(slen, 0, 'f', Global::floatPrecision()).	\
	arg(pvlInfo.voxelUnitStringShort());     

      slen /= avg;
    }
  else
    str = QString("%1 voxels").arg(slen);

  slen = slen/pixelGLRatio;

  Vec s0 = Vec(m_pos.x()*viewWidth,
	       m_pos.y()*viewHeight,
	       1);
  
  Vec s1 = s0;
  if (horizontal)
    {
      slen *= (float)viewWidth/(float)screenWidth;
      s0 -= Vec(slen/2, 0, 0);
      s1 += Vec(slen/2, 0, 0);
    }
  else
    {
      slen *= (float)viewHeight/(float)screenHeight;
      s0 -= Vec(0, slen/2, 0);
      s1 += Vec(0, slen/2, 0);
    }
  
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // back to front
  glColor4f(0, 0, 0, 0.8f);
  if (grabsMouse)
    {
      glColor4f(0.5f, 0, 0, 0.8f);
      glBegin(GL_QUADS);
      if (horizontal)
	{
	  glVertex2f(s0.x-5, s0.y-10);
	  glVertex2f(s1.x+5, s0.y-10);
	  glVertex2f(s1.x+5, s0.y+10);
	  glVertex2f(s0.x-5, s0.y+10);
	}
      else
	{
	  glVertex2f(s0.x-10, s0.y-5);
	  glVertex2f(s0.x-10, s1.y+5);
	  glVertex2f(s0.x+10, s1.y+5);
	  glVertex2f(s0.x+10, s0.y-5);
	}
      glEnd();
    }
  
  
  glDisable(GL_BLEND);
  glColor3f(1,1,1);
  glLineWidth(10);
  glBegin(GL_LINES);
  glVertex3fv(s0);
  glVertex3fv(s1);
  glEnd();
  
  glColor3f(0.5,0.5,0.5);
  glLineWidth(2);
  glBegin(GL_LINES);
  if (horizontal)
    {
      glVertex2f(s0.x+slen/2, s0.y-3);
      glVertex2f(s0.x+slen/2, s1.y+3);
    }
  else
    {
      glVertex2f(s1.x-3, (s1.y+s0.y)/2);
      glVertex2f(s0.x+3, (s1.y+s0.y)/2);
    }
  glEnd();
  glColor3f(0,0,0);
  glLineWidth(2);
  glBegin(GL_LINES);
  if (horizontal)
    {
      glVertex2f(s0.x+1, s0.y);
      glVertex2f(s1.x-1, s1.y);
    }
  else
    {
      glVertex2f(s0.x, s0.y+1);
      glVertex2f(s1.x, s1.y-1);
    }
  glEnd();
  glLineWidth(1);
  glColor3f(1,1,1);
  
  {
    Vec w0 = Vec(m_pos.x()*screenWidth, (1-m_pos.y())*screenHeight,1);
    Vec w1 = w0;
    if (horizontal)
      {
	w0 -= Vec(slen/2, 0, 0);
	w1 += Vec(slen/2, 0, 0);
      }
    else
      {
	w0 -= Vec(0, slen/2, 0);
	w1 += Vec(0, slen/2, 0);
      }
    
    QFont tfont = QFont("Helvetica", 12);
    tfont.setStyleStrategy(QFont::PreferAntialias);
    QFontMetrics metric(tfont);
    int mde = metric.descent();
    int fht = metric.height()+2;

    int fwd = metric.width(str)+2;
    QImage bImage = QImage(fwd, fht, QImage::Format_ARGB32);
    bImage.fill(0);
    QPainter bpainter(&bImage);
    Vec bgcolor = Global::backgroundColor();
    bpainter.setBackgroundMode(Qt::OpaqueMode);
    bpainter.setBackground(QColor(bgcolor.z*255,
				  bgcolor.y*255,
				  bgcolor.x*255));
    float bgintensity = (0.3*bgcolor.x +
			 0.5*bgcolor.y +
			 0.2*bgcolor.z);
    QColor penColor(Qt::white);
    if (bgintensity > 0.5) penColor = Qt::black;
    bpainter.setPen(penColor);
    bpainter.setFont(tfont);
    bpainter.drawText(1, fht-mde, str);
    
    QImage cImage = bImage.mirrored();
    if (!horizontal)
      {	    
	QMatrix matrix;
	matrix.rotate(90);
	cImage = cImage.transformed(matrix);
      }
    int x,y;
    if (horizontal)
      {
	x = (w0.x+w1.x)/2 - cImage.width()/2;
	y = w0.y-3-cImage.height();
	if (!m_textpos)
	  y = w0.y+6;
      }
    else
      {
	x = w1.x+3;
	if (!m_textpos)
	  x = w1.x-5-cImage.width();
	y = (w0.y+w1.y)/2 - cImage.height()/2;
      }
    glWindowPos2i(x,y);
    const uchar *bits = cImage.bits();
    glDrawPixels(cImage.width(), cImage.height(),
		 GL_RGBA,
		 GL_UNSIGNED_BYTE,
		 bits);
  }
}
