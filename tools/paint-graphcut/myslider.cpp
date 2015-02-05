#include "myslider.h"

MySlider::MySlider(QWidget *parent) :
  QWidget(parent)
{
  setFocusPolicy(Qt::StrongFocus);
  
  m_rangeMin = 0;
  m_rangeMax = 0;

  m_userMin = 0;
  m_userMax = 0;

  m_value = 0;

  m_keepGrabbingValue = false;
  m_keepGrabbingUserMin = false;
  m_keepGrabbingUserMax = false;

  m_baseX = 70;
  m_baseY = 10;
  m_height = 10;
  m_range = m_rangeMax-m_rangeMin;

  m_polyLevels.clear();

  setMinimumSize(QSize(1.7*m_baseX, 20));      
}

void MySlider::polygonLevels(QList<int> pl)
{
  m_polyLevels = pl;
  update();
}

int MySlider::value() { return m_value; }
void
MySlider::userRange(int& umin, int& umax)
{
  umin = m_userMin;
  umax = m_userMax;
}
void
MySlider::range(int& rmin, int& rmax)
{
  rmin = m_rangeMin;
  rmax = m_rangeMax;
}

void
MySlider::set(int rmin, int rmax,
	      int umin, int umax,
	      int val)
{
  m_rangeMin = rmin;
  m_rangeMax = rmax;
  m_range = m_rangeMax-m_rangeMin;
  m_userMin = umin;
  m_userMax = umax;
  m_value = qBound(umin, val, umax);

  if (m_rangeMax > 1000)
    m_baseX = 70;
  else if (m_rangeMax > 100)
    m_baseX = 60;
  else
    m_baseX = 50;

  setMinimumSize(QSize(1.7*m_baseX, 20));
      
  update();
}
void
MySlider::setRange(int rmin, int rmax)
{
  m_rangeMin = rmin;
  m_rangeMax = rmax;
  m_range = m_rangeMax-m_rangeMin;

  m_userMin = rmin;
  m_userMax = rmax;
  m_value = rmin;

  if (m_rangeMax > 1000)
    m_baseX = 70;
  else if (m_rangeMax > 100)
    m_baseX = 60;
  else
    m_baseX = 50;

  setMinimumSize(QSize(1.7*m_baseX, 20));

  update();
}
void
MySlider::setUserRange(int umin, int umax)
{
  m_userMin = umin;
  m_userMax = umax;
  update();
}
void
MySlider::setValue(int val)
{
  m_value = val;
  update();
}

void
MySlider::drawPolygonLevels(QPainter *p)
{
  p->setPen(QColor(150,150,250));
  int xp = m_baseX;
  for (int i=0; i<m_polyLevels.count(); i++)
    {
      float frc = (float)m_polyLevels[i]/(float)m_range;
      int yp = m_baseY + frc*m_height;
      p->drawLine(xp-30, yp, xp+30, yp);
    }
}

void
MySlider::drawBoundText(QPainter *p,
			QFont pfont,
			QString text,
			int x, int y)
{
  QPainterPath pp;
  pp.addText(QPointF(0,0), pfont, text);
  QRectF br = pp.boundingRect();
  float by = br.height()/2;      
  float bw = br.width(); 
  p->setPen(Qt::darkRed);
  QPolygon poly;
  poly << QPoint(x, y);
  poly << QPoint(x+10, y-by-2);
  poly << QPoint(x+bw+15, y-by-2);
  poly << QPoint(x+bw+15, y+by+2);
  poly << QPoint(x+10, y+by+2);
  p->drawConvexPolygon(poly);
  p->setPen(Qt::white);
  p->drawText(x+10, y+by, text);
  p->setPen(Qt::black);
}

void
MySlider::drawSliderText(QPainter *p,
			 QFont pfont,
			 QString text,
			 int x, int y)
{
  QPainterPath pp;
  pp.addText(QPointF(0,0), pfont, text);
  QRectF br = pp.boundingRect();
  float by = br.height()/2;      
  float bw = br.width(); 
  p->setPen(Qt::darkRed);
  p->drawRect(x-bw-30,
	      y-by-2,
	      bw+10, 2*by+4);
  p->setPen(Qt::white);
  p->drawText(x-bw-25, y+by, text);
  p->setPen(Qt::black);
  p->drawLine(x-20, y, x, y);
}

void
MySlider::drawSlider(QPainter *p)
{
  if (m_range <= 0)
    return;

  int xp = m_baseX;
  int yp;
  float frc;

  //---- draw polygon levels
  drawPolygonLevels(p);

  //---- draw slider
  QLinearGradient lg(xp-3,
		     0,
		     xp+3,
		     0);
  lg.setColorAt(0, Qt::black);
  lg.setColorAt(0.1, Qt::darkGray);
  lg.setColorAt(0.5, Qt::white);
  lg.setColorAt(1, Qt::black);
  p->setPen(Qt::black);
  p->setBrush(QBrush(lg));
  p->drawRect(xp-2, m_baseY, 5, m_height);

  QFont pfont = QFont("Helvetica", 10);
  QPen linepen(Qt::darkGray);
  p->setBrush(QColor(0,0,0,100));
  p->setFont(pfont);

  //---- draw slider start and end
  drawSliderText(p, pfont,
		 QString("%1").arg(m_rangeMin),
		 xp, m_baseY);
  drawSliderText(p, pfont,
		 QString("%1").arg(m_rangeMax),
		 xp, m_baseY+m_height);
  

  //---- draw user defined slice bounds

  p->setBrush(QColor(100,0,0,100));
  frc = (float)m_userMin/(float)m_range;
  yp = m_baseY + frc*m_height;
  drawBoundText(p, pfont,
		QString("%1").arg(m_userMin),
		xp, yp);

  frc = (float)m_userMax/(float)m_range;
  yp = m_baseY + frc*m_height;
  drawBoundText(p, pfont,
		QString("%1").arg(m_userMax),
		xp, yp);



  //---- now draw the current slice
  p->setBrush(QColor(0,0,0,100));
  frc = (float)m_value/(float)m_range;
  yp = m_baseY + frc*m_height;
  drawSliderText(p, pfont,
		 QString("%1").arg(m_value),
		 xp, yp);  
  QRadialGradient rg(xp, yp, 7);
  rg.setColorAt(0, Qt::white);
  rg.setColorAt(1, Qt::darkGreen);
  p->setBrush(QBrush(rg));
  p->setPen(Qt::black);
  p->drawEllipse(xp-6, yp-5, 11, 11);
}

void
MySlider::paintEvent(QPaintEvent *event)
{
  QPainter p(this);

  drawSlider(&p);

  if (hasFocus())
    {
      p.setPen(QPen(QColor(250, 100, 0), 2));
      p.setBrush(Qt::transparent);
      p.drawRect(rect());
    }
}

void
MySlider::resizeEvent(QResizeEvent *event)
{
  m_height = rect().height()-2*m_baseY;

  update();
}

void
MySlider::keyPressEvent(QKeyEvent *event)
{
  if (event->key() == Qt::Key_Up)
    {
      int pv = m_value;
      m_value--;
      m_value = qBound(m_userMin, m_value, m_userMax);
      if (pv != m_value)
	{
	  emit valueChanged(m_value);
	  update();
	}
    }
  else if (event->key() == Qt::Key_Down)
    {
      int pv = m_value;
      m_value++;
      m_value = qBound(m_userMin, m_value, m_userMax);
      if (pv != m_value)
	{
	  emit valueChanged(m_value);
	  update();
	}
    } 
}

void
MySlider::wheelEvent(QWheelEvent *event)
{
  int numSteps = event->delta()/8.0f/15.0f;

  QPoint pp = mapFromParent(event->pos());
  float ypos = pp.y();
  float xpos = pp.x();
  int yp = m_baseY + m_height*((float)m_userMin+(m_userMax-m_userMin)*0.5)/(float)m_range;
  if (xpos >= m_baseX && xpos <= rect().width())
    {
      if (ypos <= yp)
	{ // change min slice position
	  m_userMin -= numSteps;
	  m_userMin = qBound(m_rangeMin, m_userMin, m_userMax-1);      
	  emit userRangeChanged(m_userMin, m_userMax);
	  update();
	}
      else
	{ // change max slice position
	  m_userMax -= numSteps;
	  m_userMax = qBound(m_userMin+1, m_userMax, m_rangeMax);      
	  emit userRangeChanged(m_userMin, m_userMax);
	  update();
	}
    }
  else
    { // change slice position
      int pv = m_value;
      m_value -= numSteps;
      m_value = qBound(m_userMin, m_value, m_userMax);
      if (pv != m_value)
	{
	  emit valueChanged(m_value);
	  update();
	}
    }
}

bool
MySlider::checkSlider(int xpos, int ypos)
{
  if (xpos >= 0 && xpos <= m_baseX+5 &&
      ypos >= m_baseY-10 && ypos <= m_baseY+m_height+10)
    {
      m_keepGrabbingValue = true;
      float frc = ((float)ypos-(float)m_baseY)/(float)m_height;
      int val = m_range*frc;
      val = qBound(m_userMin, val, m_userMax);
      if (m_value != val)
	{
	  m_value = val;
	  return true;
	}
    }

  int yp = m_baseY + m_height*((float)m_userMin/(float)m_range);
  if (xpos >= m_baseX && xpos <= rect().width() &&
      ypos >= yp-10 && ypos <= yp+10)
    m_keepGrabbingUserMin = true;
  else
    {
      yp = m_baseY + m_height*((float)m_userMax/(float)m_range);
      if (xpos >= m_baseX && xpos <= rect().width() &&
	  ypos >= yp-10 && ypos <= yp+10)
	m_keepGrabbingUserMax = true;
    }

  return false;
}

void
MySlider::mousePressEvent(QMouseEvent *event)
{
  QPoint pp = mapFromParent(event->pos());
  float ypos = pp.y();
  float xpos = pp.x();
  
  m_keepGrabbingValue = false;
  m_keepGrabbingUserMin = false;
  m_keepGrabbingUserMax = false;

  if (checkSlider(xpos, ypos))
    {
      emit valueChanged(m_value);
      update();
    }
}

void
MySlider::mouseMoveEvent(QMouseEvent *event)
{
  QPoint pp = mapFromParent(event->pos());
  float ypos = pp.y();
  float xpos = pp.x();

  if (event->buttons() & Qt::LeftButton)
    {
      if (m_keepGrabbingValue)
	{
	  if (updateValue(xpos, ypos))
	    update();
	}
      else if (m_keepGrabbingUserMin)
	{
	  if (updateUserMin(xpos, ypos))
	    update();
	}
      else if (m_keepGrabbingUserMax)
	{
	  if (updateUserMax(xpos, ypos))
	    update();
	}
    }
}

void
MySlider::mouseReleaseEvent(QMouseEvent *event)
{
  m_keepGrabbingValue = false;
  m_keepGrabbingUserMin = false;
  m_keepGrabbingUserMax = false;
}

bool
MySlider::updateValue(int xpos, int ypos)
{
  float frc = ((float)ypos-(float)m_baseY)/(float)m_height;
  int val = m_range*frc;
  val = qBound(m_userMin, val, m_userMax);
  if (m_value != val)
    {
      m_value = val;
      emit valueChanged(m_value);
      return true;
    }
  return false;
}

bool
MySlider::updateUserMin(int xpos, int ypos)
{
  float frc = ((float)ypos-(float)m_baseY)/(float)m_height;
  int slc = m_range*frc;
  slc = qBound(m_rangeMin, slc, m_rangeMax);
  if (m_userMin != slc)
    {
      m_userMin = slc;
      m_userMin = qMin(m_userMin, m_userMax);
      
      emit userRangeChanged(m_userMin, m_userMax);

      int cs = m_value;
      m_value = qBound(m_userMin, m_value, m_userMax);
      if (cs != m_value)
	emit valueChanged(m_value);

      return true;
    }
  return false;
}

bool
MySlider::updateUserMax(int xpos, int ypos)
{
  float frc = ((float)ypos-(float)m_baseY)/(float)m_height;
  int slc = m_range*frc;
  slc = qBound(m_rangeMin, slc, m_rangeMax);
  if (m_userMax != slc)
    {
      m_userMax = slc;
      m_userMax = qMax(m_userMin, m_userMax);
      
      emit userRangeChanged(m_userMin, m_userMax);

      int cs = m_value;
      m_value = qBound(m_userMin, m_value, m_userMax);
      if (cs != m_value)
	emit valueChanged(m_value);

      return true;
    }
  return false;
}
