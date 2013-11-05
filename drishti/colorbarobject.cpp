#include "colorbarobject.h"

ColorBarObject::ColorBarObject() { clear(); }
ColorBarObject::~ColorBarObject() { clear(); }

void
ColorBarObject::clear()
{
  m_pos = QPointF(0,0);
  m_type = 1;
  m_tfset = 1;
  m_height = 500;
  m_width = 100;
  m_onlyColor = true;
}

void
ColorBarObject::setColorBar(ColorBarObject co)
{
  m_pos = co.position();
  m_type = co.type();
  m_tfset = co.tfset();
  m_height = co.height();
  m_width = co.width();
  m_onlyColor = co.onlyColor();
}

void
ColorBarObject::set(QPointF pos, int type, float tfset,
		    int wd, int ht, bool oc)
{
  m_pos = pos;
  m_type = type;
  m_tfset = tfset;
  m_width = wd;
  m_height = ht;
  m_onlyColor = oc;
}

void ColorBarObject::setOnlyColor(bool oc) { m_onlyColor = oc; }
void ColorBarObject::setWidth(int wd) { m_width = wd; }
void ColorBarObject::setHeight(int ht) { m_height = ht; }
void ColorBarObject::setPosition(QPointF pos) { m_pos = pos; }
void ColorBarObject::setTFset(float tfset) { m_tfset = tfset; }
void ColorBarObject::setType(int type)
{
  if (m_type != type)
    { // flip height and width
      int t = m_height;
      m_height = m_width;
      m_width = t;
    }
  
  m_type = type;
}

QPointF ColorBarObject::position() { return m_pos; }
int ColorBarObject::height() { return m_height; }
int ColorBarObject::width() { return m_width; }
int ColorBarObject::type() { return m_type; }
float ColorBarObject::tfset() { return m_tfset; }
bool ColorBarObject::onlyColor() { return m_onlyColor; }

void
ColorBarObject::scale(int dx, int dy)
{
  if (m_width + dx > 10)
    m_width += dx;

  if (m_height + dy > 10)
    m_height += dy;
}

void
ColorBarObject::load(fstream& fin)
{
  clear();

  int len;
  bool done = false;
  char keyword[100];
  while (!done)
    {
      fin.getline(keyword, 100, 0);

      if (strcmp(keyword, "colorbarobjectend") == 0)
	done = true;
      else if (strcmp(keyword, "position") == 0)
	{
	  float x, y;
	  fin.read((char*)&x, sizeof(float));
	  fin.read((char*)&y, sizeof(float));
	  m_pos = QPointF(x,y);
	}
      else if (strcmp(keyword, "type") == 0)
	fin.read((char*)&m_type, sizeof(int));
      else if (strcmp(keyword, "tfset") == 0)
	fin.read((char*)&m_tfset, sizeof(float));
      else if (strcmp(keyword, "width") == 0)
	fin.read((char*)&m_width, sizeof(int));
      else if (strcmp(keyword, "height") == 0)
	fin.read((char*)&m_height, sizeof(int));
      else if (strcmp(keyword, "onlycolor") == 0)
	fin.read((char*)&m_onlyColor, sizeof(bool));
    }
}

void
ColorBarObject::save(fstream& fout)
{
  char keyword[100];
  int len;

  memset(keyword, 0, 100);
  sprintf(keyword, "colorbarobjectstart");
  fout.write((char*)keyword, strlen(keyword)+1);

  float x = m_pos.x();
  float y = m_pos.y();
  memset(keyword, 0, 100);
  sprintf(keyword, "position");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&x, sizeof(float));
  fout.write((char*)&y, sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "type");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_type, sizeof(int));

  memset(keyword, 0, 100);
  sprintf(keyword, "tfset");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_tfset, sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "width");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_width, sizeof(int));

  memset(keyword, 0, 100);
  sprintf(keyword, "height");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_height, sizeof(int));

  memset(keyword, 0, 100);
  sprintf(keyword, "onlycolor");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_onlyColor, sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "colorbarobjectend");
  fout.write((char*)keyword, strlen(keyword)+1);
}

