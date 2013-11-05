#include "dcolorwheel.h"
#include <math.h>

static int colorGridSize = 7;

DColorWheel::DColorWheel(QWidget *parent, int margin) :
  QWidget(parent)
{
  m_parent = parent;
  m_parent->installEventFilter(this);

  m_margin = margin;

  m_wheelSize = QRectF(5,5, 10, 10);
  m_wheelCenter = QPointF(m_wheelSize.x()+m_wheelSize.width()/2,
			  m_wheelSize.y()+m_wheelSize.height()/2);

  m_more = new QPushButton("+", this);
  m_less = new QPushButton("-", this);

  connect(m_more, SIGNAL(pressed()),
	  this, SLOT(moreShades()));

  connect(m_less, SIGNAL(pressed()),
	  this, SLOT(lessShades()));


  m_hue = 0;
  m_saturation = 1;
  m_value = 1;

  m_currFlag = 0;
}

void
DColorWheel::setColor(QColor col)
{
  qreal h,s,v;
  col.getHsvF(&h, &s, &v);
  m_hue = h;
  m_saturation = s;
  m_value = v;
  setButtonColors();
}

QColor
DColorWheel::getColor()
{
  float sat, val;
  int colx, coly;
  colx = ceil(m_saturation*(colorGridSize)) - 1;
  coly = ceil(m_value*(colorGridSize)) - 1;
  colx = qMax(0, qMin(colx, colorGridSize-1));
  coly = qMax(0, qMin(coly, colorGridSize-1));
  if (colx == 0 || coly == 0)
    {
      sat = 0;
      val = 0;
      if (coly == 0)
	val = (colorGridSize-1-colx)/(float)(2*colorGridSize-2);
      else
	val = (colorGridSize-1+coly)/(float)(2*colorGridSize-2);
    }
  else
    {
      if (m_hue < 0)
	{
	  val = (colx*colorGridSize + coly);
	  val/=(colorGridSize*colorGridSize);
	  sat = 0;
	}
      else
	{
	  sat=(float)colx/(colorGridSize-1);
	  val=(float)coly/(colorGridSize-1);
	}
    }

  return QColor::fromHsvF(m_hue, sat, val);
}


void
DColorWheel::paintEvent(QPaintEvent *event)
{
  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing);

  // set conical color gradient for outer color wheel
  QConicalGradient conicalGradient(m_wheelCenter, 0); 
  conicalGradient.setColorAt(0.0, Qt::red);
  conicalGradient.setColorAt(60.0/360.0, Qt::yellow);
  conicalGradient.setColorAt(120.0/360.0, Qt::green);
  conicalGradient.setColorAt(180.0/360.0, Qt::cyan);
  conicalGradient.setColorAt(240.0/360.0, Qt::blue);
  conicalGradient.setColorAt(300.0/360.0, Qt::magenta);
  conicalGradient.setColorAt(1.0, Qt::red);
  p.setBrush(QBrush(conicalGradient));
  p.setPen(Qt::darkGray);
  p.drawEllipse(m_wheelSize);


  // color inner circle with selected color
  QColor color = getColor();
  p.setBrush(color);
  p.drawEllipse(m_innerWheelSize);


  // draw hue marker
  p.save();
  p.translate(m_wheelCenter.x(),m_wheelCenter.y());
  p.rotate(-m_hue*360.0);
  p.translate(-m_wheelCenter.x(),-m_wheelCenter.y());
  p.translate(m_innerWheelSize.x()+m_innerWheelSize.width()-2,
	      m_innerWheelSize.x()+m_innerWheelSize.height()/2-5);
  p.setBrush(Qt::transparent);
  p.setPen(QPen(Qt::darkGray, 3));
  p.drawRoundRect(m_hueMarker, 10, 10);
  p.setPen(QPen(Qt::white, 1));
  p.drawRoundRect(m_hueMarker, 10, 10);
  p.restore();

  p.setPen(Qt::transparent);

  int i,j;
  float width = (float)m_innerRect.width()/colorGridSize;
  float height = (float)m_innerRect.height()/colorGridSize;
  // plot gray values
  for(i=0; i<2*colorGridSize-1; i++)
    {
      int x, y;
      float v;
      if (i < colorGridSize)
	{
	  x = colorGridSize-1-i;
	  y = 0;
	}
      else
	{
	  x = 0;
	  y = i-colorGridSize+1;
	}
      v = i/(float)(2*colorGridSize-2);
      QColor col;
      col = QColor::fromHsvF(m_hue, 0.0, v);
      p.setBrush(col);
      p.drawEllipse(m_innerRect.x()+x*width + 0.5,
		    m_innerRect.y()+y*height + 0.5,
		    width-1, height-1);
    }

  // plot color values
  for(i=1; i<colorGridSize; i++)
  for(j=1; j<colorGridSize; j++)
    {
      float s,v;
      if (m_hue < 0)
	{
	  v = (i*colorGridSize + j);
	  v/=(colorGridSize*colorGridSize);
	  s = 0;
	}
      else
	{
	  s = i/(float)(colorGridSize-1);
	  v = j/(float)(colorGridSize-1);
	}
      QColor col;
      col = QColor::fromHsvF(m_hue, s, v);
      p.setBrush(col);
      p.drawEllipse(m_innerRect.x()+i*width + 0.5,
		    m_innerRect.y()+j*height + 0.5,
		    width-1, height-1);
    }

  // draw saturation+value marker
  int colx, coly;
  colx = ceil(m_saturation*(colorGridSize)) - 1;
  coly = ceil(m_value*(colorGridSize)) - 1;
  colx = qMax(0, qMin(colx, colorGridSize-1));
  coly = qMax(0, qMin(coly, colorGridSize-1));
  p.setBrush(Qt::transparent);
  p.setPen(QPen(Qt::darkGray, 3));
  p.drawEllipse(m_innerRect.x()+colx*width-1,
		m_innerRect.y()+coly*height-1,
		width+2, height+2);
  p.setPen(QPen(Qt::white, 1));
  p.drawEllipse(m_innerRect.x()+colx*width-1,
		m_innerRect.y()+coly*height-1,
		width+2, height+2);
}


void
DColorWheel::resizeEvent(QResizeEvent *event)
{
  float rad;
  rad = qMin(event->size().width(),
	     event->size().height());
  m_wheelSize = QRectF(5, 5, rad-10, rad-10);
  
  m_innerWheelSize = m_wheelSize.adjusted(20, 20, -20, -20);

  float shift = 0.2*m_innerWheelSize.width();
  m_innerRect = m_innerWheelSize.adjusted(shift, shift, -shift, -shift);

  m_hueMarker = QRectF(0,0, 24, 10);

  m_wheelCenter = QPointF(m_wheelSize.x()+m_wheelSize.width()/2,
			  m_wheelSize.y()+m_wheelSize.height()/2);

  m_more->setGeometry(m_innerRect.x()+m_innerRect.width()/2-15,
		      m_innerRect.y()-30,
		      30, 30);

  m_less->setGeometry(m_innerRect.x()+m_innerRect.width()/2-15,
		      m_innerRect.y()+m_innerRect.height()+1,
		      30, 30);
}

void
DColorWheel::wheelEvent(QWheelEvent *event)
{
  float step = event->delta()/120.0;
  m_hue += step/360.0;
  if (m_hue < 0)
    m_hue += 1.0;
  else if (m_hue > 1.0)
    m_hue -= 1.0;

  m_hue = qMax(0.0f, qMin(1.0f, m_hue));
  setButtonColors();
  emit update();
}


void
DColorWheel::calculateHsv(QPointF clickPos)
{
  float rx, ry;
  rx = qAbs(clickPos.x() - m_wheelCenter.x());
  ry = qAbs(clickPos.y() - m_wheelCenter.y());
  if ((m_currFlag == 0 &&
       clickPos.x() >= m_innerRect.x()+10 &&
       clickPos.y() >= m_innerRect.y()+10 &&
       clickPos.x() < m_innerRect.x()+m_innerRect.width()+10 &&
       clickPos.y() < m_innerRect.y()+m_innerRect.height()+10) ||
      m_currFlag == 1)
    {
      // As we are using layout manager for placement
      // we need to shift innerRect by m_margin units in x & y
      m_currFlag = 1;
      m_saturation = (clickPos.x()-m_innerRect.x()-m_margin)/m_innerRect.width();
      m_value = (clickPos.y()-m_innerRect.y()-m_margin)/m_innerRect.height();
      m_saturation = qMax(0.0f, qMin(1.0f, m_saturation));
      m_value = qMax(0.0f, qMin(1.0f, m_value));

      setButtonColors();
      emit update();
    }
  else if (m_currFlag == 0 || m_currFlag == 2) 
    {
      m_currFlag = 2;
      rx = (clickPos.x() - m_wheelCenter.x());
      ry = (clickPos.y() - m_wheelCenter.y());
      m_hue = 180.0*atan2(ry, rx)/3.14159265;
      if (m_hue < 0) m_hue = 360 + m_hue;
      m_hue = 1 - m_hue/360.0;
      m_hue = qMax(0.0f, qMin(1.0f, m_hue));
      setButtonColors();
      emit update();
    }
}

bool
DColorWheel::eventFilter(QObject *object, QEvent *event)
{
  static int mousePressed = 0;
  if (object == m_parent)
    {
      switch (event->type())
	{
	case QEvent::MouseButtonPress :
	  {
	    mousePressed = 1;
	    QMouseEvent *me = (QMouseEvent*)event;
	    QPointF clickPos = me->pos();
	    calculateHsv(clickPos);
	    emit colorChanged();
	    break;
	  }
	case QEvent::MouseButtonRelease :
	  {
	    mousePressed = 0;
	    m_currFlag = 0;
	    break;
	  }
	case QEvent::MouseMove :
	  {
	    if (mousePressed == 1)
	      {
		QMouseEvent *me = (QMouseEvent*)event;
		QPointF clickPos = me->pos();
		calculateHsv(clickPos);
		emit colorChanged();
	      }
	    break;
	  }
	}
    }

  return false;
}

void
DColorWheel::setButtonColors()
{
  QColor color = getColor();
  QString str;
  str = QString("QPushButton {"						\
		"border-radius: 15px;"					\
		"border: 1px solid #777777;"				\
		"background-color: qradialgradient("			\
		"cx: 0.5, cy: 0.5, radius: 0.5, fx: 0.5, fy:0.2 "	\
		"stop: 0 #ffffff, stop: 0.9 %1 );"	\
		"}"							\
		"QPushButton:pressed {"					\
		"background-color: qradialgradient("			\
		"cx: 0.5, cy: 0.5, radius: 0.5, fx: 0.5, fy:0.8 "	\
		"stop: 0 #ffffff, stop: 0.5 %1 );"		\
		"}" ).arg(color.name()).arg(color.darker().name());
  this->setStyleSheet(str);
}

void
DColorWheel::moreShades()
{
  colorGridSize++;
  if (colorGridSize > 15)
    {
      colorGridSize = 15;
      m_more->setEnabled(false);
    }
  m_less->setEnabled(true);
  emit(update());
}
void
DColorWheel::lessShades()
{
  colorGridSize--;
  if (colorGridSize < 4)
    {
      colorGridSize = 4;
      m_less->setEnabled(false);
    }
  m_more->setEnabled(true);
  emit(update());
}
