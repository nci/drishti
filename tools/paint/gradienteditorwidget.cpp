#include "staticfunctions.h"
#include "gradienteditorwidget.h"


void
GradientEditorWidget::setGeneralLock(GradientEditor::LockType lock)
{
  m_gradientEditor->setGeneralLock(lock);
}

QGradientStops
GradientEditorWidget::colorGradient()
{
  return m_gradientEditor->gradientStops();
}

void
GradientEditorWidget::setColorGradient(QGradientStops gradStops)
{
  setEnabled(true);

  m_gradientEditor->setGradientStops(gradStops);

  emit update();
  return;
}

void GradientEditorWidget::setDrawBox(bool flag) { m_drawBox = flag; }

GradientEditorWidget::GradientEditorWidget(QWidget *parent) :
  QWidget(parent)
{
  setEnabled(false);

  m_parent = parent;
  m_drawBox = true;

  //---------------------------------------------
  // set checkers background
  m_backgroundPixmap = QPixmap(10, 10);
  QPainter p(&m_backgroundPixmap);
  p.fillRect(0, 0, 5, 5, Qt::lightGray);
  p.fillRect(5, 5, 5, 5, Qt::lightGray);
  p.fillRect(0, 5, 5, 5, Qt::darkGray);
  p.fillRect(5, 0, 5, 5, Qt::darkGray);
  p.end();
  //---------------------------------------------


  QPen m_pointPen;
  QPen m_connectionPen;
  m_pointPen = QPen(QColor(0, 0, 0, 255), 1);
  m_connectionPen = QPen(QColor(255, 235, 205, 220), 1);

  m_gradientEditor = new GradientEditor(this);

  m_gradientEditor->setShapePen(m_pointPen);
  m_gradientEditor->setConnectionPen(m_connectionPen);

  QGradientStops gradStops;
  gradStops << QGradientStop(0.0, QColor(0,0,0, 0))
	    << QGradientStop(1.0, QColor(0,0,0, 0));

  m_gradientEditor->setGradientStops(gradStops);

  connect(m_gradientEditor, SIGNAL(gradientChanged()),
	  this, SLOT(updateColorGradient()));

  connect(m_gradientEditor, SIGNAL(refreshGradient()),
	  this, SLOT(update()));

  setMinimumSize(100, 100);
}

void
GradientEditorWidget::updateColorGradient()
{
  QGradientStops stops = m_gradientEditor->gradientStops();
  emit gradientChanged(stops);
  update();
}

void
GradientEditorWidget::resizeEvent(QResizeEvent *event)
{
  m_gradientEditor->resize(event->size());
}


void
GradientEditorWidget::paintEvent(QPaintEvent *event)
{
  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing);
  
  generateShade();
  
  QRectF brect = m_gradientEditor->boundingRect();
  p.drawImage(brect.x(), brect.y(), m_shade);    

  if (m_drawBox)
    {
      p.setPen(QColor(146, 146, 146));
      p.drawRect(brect);

      p.setPen(QColor(46, 86, 146));
      p.drawRect(rect());
    }
}

void
GradientEditorWidget::generateShade()
{
  QGradientStops stops = m_gradientEditor->gradientStops();
  QGradientStops colorStops = StaticFunctions::resampleGradientStops(stops);

  QRectF brect = m_gradientEditor->boundingRect();
  QLinearGradient colorGrad(0, 0, brect.width(), 0); 
  colorGrad.setStops(colorStops);

  QRect drawRect = QRect(0, 0, brect.width(), brect.height());

  if (m_shade.width() != brect.width() ||
      m_shade.height() != brect.height())
    m_shade = QImage(brect.width(),
		     brect.height(),
		     QImage::Format_RGB32);

  QPainter p(&m_shade);
  p.drawTiledPixmap(drawRect, m_backgroundPixmap);
  p.fillRect(drawRect, colorGrad);
}
