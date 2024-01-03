#include "button.h"

Button::Button()
{
  m_type = 0;
  m_text.clear();
  m_minValue = 0;
  m_maxValue = 1;
  m_value = 0;
  m_rect = QRect();
  m_geom = QRectF();
  m_grabbed = false;
}

Button::~Button()
{
  m_type = 0;
  m_text.clear();
  m_minValue = 0;
  m_maxValue = 1;
  m_value = 0;
  m_rect = QRect();
  m_geom = QRectF();
  m_grabbed = false;
}

bool
Button::inBox(float rh, float rw)
{
  float cx =  m_geom.x();
  float cy =  m_geom.y();
  float cwd = m_geom.width();
  float cht = m_geom.height();
  if (rh > cx && rh < cx+cwd &&
      rw > cy && rw < cy+cht)
    return true;

  return false;
}

void
Button::boxPos(float rh, float rw,
	       float& xPos, float& yPos)
{
  float cx =  m_geom.x();
  float cy =  m_geom.y();
  float cwd = m_geom.width();
  float cht = m_geom.height();

  // note that values can be less than 0.0 or greater than 1.0
  xPos = (rh-cx)/cwd;
  yPos = (rw-cy)/cht;
}
