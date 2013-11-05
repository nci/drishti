#include "remapcontainer.h"

RemapContainer::RemapContainer(QWidget *parent) :
  QWidget(parent)
{
  m_rawVolume.setFile("test.raw");

  m_histogramWidget = new RemapHistogramWidget(this);

  QVBoxLayout *layout = new QVBoxLayout();
  layout->addWidget(m_histogramWidget);
  setLayout(layout);

  setRawMinMax();

  connect(m_histogramWidget, SIGNAL(getHistogram()),
	  this, SLOT(getHistogram()));  
}

void
RemapContainer::setRawMinMax()
{
  float rawMin = m_rawVolume.rawMin();
  float rawMax = m_rawVolume.rawMax();
  m_histogramWidget->setRawMinMax(rawMin, rawMax);
}

void
RemapContainer::getHistogram()
{
  m_histogramWidget->setHistogram(m_rawVolume.histogram());
}

void
RemapContainer::keyPressEvent(QKeyEvent *event)
{
  m_histogramWidget->keyPress(event);
}

void
RemapContainer::mousePressEvent(QMouseEvent *event)
{
  m_histogramWidget->mousePress(event);
}

void
RemapContainer::mouseMoveEvent(QMouseEvent *event)
{
  m_histogramWidget->mouseMove(event);
}

void
RemapContainer::mouseReleaseEvent(QMouseEvent *event)
{
  m_histogramWidget->mouseRelease(event);
}

void
RemapContainer::wheelEvent(QWheelEvent *event)
{
  m_histogramWidget->wheel(event);
}
