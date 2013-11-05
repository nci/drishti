#ifndef COLORBAROBJECT_H
#define COLORBAROBJECT_H

#include <QtGui>

#include <fstream>
using namespace std;

class ColorBarObject
{
 public :
  ColorBarObject();
  ~ColorBarObject();
  
  void load(fstream&);
  void save(fstream&);

  void clear();

  void set(QPointF, int, float, int, int, bool);
  void setColorBar(ColorBarObject);
  void setPosition(QPointF);
  void setType(int);
  void setTFset(float);
  void setHeight(int);
  void setWidth(int);
  void setOnlyColor(bool);

  void scale(int, int);

  QPointF position();
  int type();
  float tfset();
  int height();
  int width();
  bool onlyColor();
  
 private :
  QPointF m_pos;  
  int m_type;
  float m_tfset;
  int m_height, m_width;
  bool m_onlyColor;
};

#endif
