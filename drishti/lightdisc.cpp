#include "lightdisc.h"
#include <math.h>

QPointF LightDisc::direction() { return m_direction; }
void LightDisc::setDirection(bool flag,
			     QPointF dir)
{
  m_backlit = flag;
  m_direction = dir;
  update();
}

bool LightDisc::backlit() { return m_backlit; }
void LightDisc::setBacklit(bool flag)
{ 
  m_backlit = flag;
  update();
}

QSize
LightDisc::sizeHint() const
{
  QSize sz;
  sz = QSize(m_wheelSize.width(), m_wheelSize.height());
  return sz;
}

LightDisc::LightDisc(QWidget *parent) :
  QWidget(parent)
{
  m_backlit = 0;
  m_size = 10;
  m_wheelSize = QRectF(5,5, m_size, m_size);
  m_wheelCenter = QPointF(m_wheelSize.x()+m_size/2,
			  m_wheelSize.y()+m_size/2);

  m_mousePos = m_wheelCenter;
  m_direction = QPointF(0, 0);

  setMinimumSize(120, 120);
}

void LightDisc::enterEvent(QEvent *e) { setFocus(); grabKeyboard(); }
void LightDisc::leaveEvent(QEvent *e) { clearFocus(); releaseKeyboard(); }

void
LightDisc::keyPressEvent(QKeyEvent *event)
{
  if (event->key() == Qt::Key_Space)
    {      
      m_direction = QPointF(0, 0);
      update();
      emit directionChanged(m_direction);
    }
}

void
LightDisc::paintEvent(QPaintEvent *event)
{
  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing);

  float x,y,w;
  w = m_wheelSize.width()/2;
  x = w*(-m_direction.x()) + m_wheelCenter.x();
  y = w*(m_direction.y()) + m_wheelCenter.y();
  QPointF lit = QPointF(x,y);
  QRadialGradient radialGradient(m_wheelCenter,
				 m_wheelSize.width(),
				 lit);

  if (m_backlit)
    {
      radialGradient.setColorAt(0.0, Qt::black);
      radialGradient.setColorAt(0.5, Qt::lightGray);
      radialGradient.setColorAt(1.0, Qt::white);
    }
  else
    {
      radialGradient.setColorAt(0.0, Qt::white);
      radialGradient.setColorAt(0.5, Qt::darkGray);
      radialGradient.setColorAt(1.0, Qt::black);
    }

  p.setBrush(QBrush(radialGradient));
  p.setPen(Qt::darkGray);
  p.drawEllipse(m_wheelSize);

  p.setBrush(Qt::transparent);
  p.drawEllipse(m_wheelSize.x() + m_wheelSize.width()/4,
		m_wheelSize.y() + m_wheelSize.height()/4,
		m_wheelSize.width()/2,
		m_wheelSize.height()/2);

  p.drawLine(5, 5+m_wheelSize.height()/2,
	     5+m_wheelSize.width(), 5+m_wheelSize.height()/2);

  p.drawLine(5+m_wheelSize.width()/2, 5,
	     5+m_wheelSize.width()/2, 5+m_wheelSize.height());
}

void
LightDisc::resizeEvent(QResizeEvent *event)
{
  m_size = qMin(rect().width(),
        	rect().height()) - 11;
  m_wheelSize = rect().adjusted(5, 5, -5, -5);
  m_wheelCenter = QPointF(m_wheelSize.x()+m_size/2,
			  m_wheelSize.y()+m_size/2);
  m_mousePos = m_wheelCenter;

  update();
}

void
LightDisc::calculateDirection()
{
  float x,y, len, rad;
  m_direction = m_mousePos - m_wheelCenter;
  x = m_direction.x();
  y = m_direction.y();
  len = sqrt(x*x + y*y);
  rad = m_size/2;
  if (len > rad)
    {
      x = rad*x/len;
      y = rad*y/len;
    }
  m_direction = QPointF(-x/rad, y/rad);

  emit directionChanged(m_direction);
}

void
LightDisc::mousePressEvent(QMouseEvent *event)
{
  m_mousePos = event->pos();
  calculateDirection();
  update();
}

void
LightDisc::mouseMoveEvent(QMouseEvent *event)
{
  m_mousePos = event->pos();
  calculateDirection();
  update();
}
