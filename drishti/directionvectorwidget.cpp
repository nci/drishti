#include "directionvectorwidget.h"

DirectionVectorWidget::DirectionVectorWidget(QWidget *parent) :
  QWidget(parent)
{
  ui.setupUi(this);

  m_direction = Vec(0,0,1);

  m_lightdisc = new LightDisc(ui.disc);

  connect(m_lightdisc, SIGNAL(directionChanged(QPointF)),
	  this, SLOT(updateDirection(QPointF)));
}

void
DirectionVectorWidget::setRange(float min, float max, float val)
{
  ui.distance->setRange(min, max);
  ui.distance->setValue(val);
}
void DirectionVectorWidget::setDistance(float val)
{
  ui.distance->setValue(val);
}

void
DirectionVectorWidget::setVector(Vec vo)
{
  Vec v = vo.unit();
  if (v.z < 0)
    {
      m_lightdisc->setDirection(true, QPointF(v.x, v.y));
      ui.reverse->setChecked(true);
    }
  else
    {
      m_lightdisc->setDirection(false, QPointF(v.x, v.y));
      ui.reverse->setChecked(false);
    }

  m_direction = v;
}

Vec
DirectionVectorWidget::vector()
{
  float len = (float)(ui.distance->value());
  Vec dv = len*m_direction;
  return dv;
}

void
DirectionVectorWidget::updateDirection(QPointF pt)
{
  float x = pt.x();
  float y = pt.y();
  m_direction = Vec(x, y, sqrt(qAbs(1.0f-sqrt(x*x+y*y))));

  if (m_direction.norm() > 0)
    m_direction.normalize();

  if (ui.reverse->isChecked() == true)
    m_direction.z = -m_direction.z;

  on_distance_valueChanged(0);
}

void
DirectionVectorWidget::on_reverse_clicked(bool flag)
{
  m_lightdisc->setBacklit(flag);
  if (flag)
    m_direction.z = -qAbs(m_direction.z);
  else
    m_direction.z = qAbs(m_direction.z);

  on_distance_valueChanged(0);
}

void
DirectionVectorWidget::on_distance_valueChanged(double d)
{
  float len = (float)(ui.distance->value());
  emit directionChanged(len, m_direction);
}
