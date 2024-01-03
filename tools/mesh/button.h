#ifndef BUTTON_H
#define BUTTON_H

#include <GL/glew.h>

#include <QString>
#include <QRect>
#include <QRectF>

class Button
{
 public :
  Button();
  ~Button();

  int type() { return m_type; }
  QString text() { return m_text; }
  float minValue() { return m_minValue; }
  float maxValue() { return m_maxValue; }
  float value() { return m_value; }
  QRect rect() { return m_rect; }
  QRectF geom() { return m_geom; }
  bool grabbed() { return m_grabbed; }

  void setType(int t) { m_type = t; }
  void setText(QString s) { m_text = s; }
  void setMinMaxValues(float min, float max) { m_minValue = min; m_maxValue = max; }
  void setValue(float v) { m_value = v; }
  void setRect(QRect r) { m_rect = r; }
  void setGeom(QRectF g) { m_geom = g; }
  void setGrabbed(bool g) { m_grabbed = g; }
  void resetValue() { m_value = m_minValue; }
  void toggle() { m_value = m_value < m_maxValue ? m_maxValue : m_minValue; }

  bool on() { return m_value < m_maxValue ? false : true; }

  bool inBox(float, float);
  void boxPos(float, float, float&, float&);
    
 private :
  bool m_grabbed;
  int m_type;
  QString m_text;
  float m_minValue;
  float m_maxValue;
  float m_value;
  QRect m_rect;
  QRectF m_geom;
};

#endif
