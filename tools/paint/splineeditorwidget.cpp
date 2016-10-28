#include "splineeditorwidget.h"
#include "splineeditor.h"
#include "global.h"

#include <math.h>

#include <QDomDocument>
#include <QTextStream>
#include <QFile>
#include <QButtonGroup>
#include <QHBoxLayout>
#include <QVBoxLayout>

SplineEditorWidget::SplineEditorWidget(QWidget *parent) :
  QWidget(parent)
{
  m_parent = parent;

  QPen m_pointPen;
  QBrush m_pointBrush;
  QPen m_connectionPen;
  m_pointPen = QPen(QColor(0, 0, 0, 255), 1);
  m_connectionPen = QPen(QColor(135, 135, 135, 220), 1);
  m_pointBrush = QBrush(QColor(255, 255, 255, 200));

  m_1dHist  = new QRadioButton("1D", this);
  m_2dHist  = new QRadioButton("2D", this);

  QButtonGroup *butGrp1 = new QButtonGroup(this);
  butGrp1->setExclusive(true);
  butGrp1->addButton(m_1dHist);
  butGrp1->addButton(m_2dHist);

  m_1dHist->setChecked(true);

  QHBoxLayout *hbox = new QHBoxLayout;
  hbox->addWidget(m_1dHist);
  hbox->addWidget(m_2dHist);
  hbox->addStretch();

  m_16bitEditor = new RemapHistogramWidget(this);
  m_16bitEditor->setSizePolicy(QSizePolicy::Expanding,
			       QSizePolicy::Expanding);

  QVBoxLayout *vbox = new QVBoxLayout;
  m_splineEditor = new SplineEditor(this);
  vbox->addWidget(m_splineEditor);
  vbox->addWidget(m_16bitEditor);
  vbox->addLayout(hbox);

  setLayout(vbox);

  connect(m_16bitEditor, SIGNAL(newMapping(float, float)),
	  this, SLOT(updateTransferFunction(float, float)));


  m_splineEditor->setShapePen(m_pointPen);
  m_splineEditor->setConnectionPen(m_connectionPen);
  m_splineEditor->setShapeBrush(m_pointBrush);


  m_16bitEditor->show();
  m_splineEditor->hide();
  m_1dHist->hide();
  m_2dHist->hide();


  connect(m_1dHist, SIGNAL(toggled(bool)),
	  this, SLOT(hist1DClicked(bool)));

  connect(m_2dHist, SIGNAL(toggled(bool)),
	  this, SLOT(hist2DClicked(bool)));

  connect(m_splineEditor, SIGNAL(refreshDisplay()),
	  this, SLOT(update()));

  connect(m_splineEditor, SIGNAL(splineChanged()),
	  this, SLOT(updateTransferFunction()));

  connect(m_splineEditor, SIGNAL(selectEvent(QGradientStops)),
	  this, SLOT(selectSpineEvent(QGradientStops)));

  connect(m_splineEditor, SIGNAL(deselectEvent()),
	  this, SLOT(deselectSpineEvent()));


  setMinimumSize(100, 100);
}

void
SplineEditorWidget::hist1DClicked(bool flag)
{
//  if (flag)
//    m_splineEditor->show2D(false);
}

void
SplineEditorWidget::hist2DClicked(bool flag)
{
//  if (flag)
//    m_splineEditor->show2D(true);
}

void
SplineEditorWidget::setTransferFunction(SplineTransferFunction *stf)
{
  m_splineEditor->setTransferFunction(stf);

  if (stf != NULL)
    {
      float tmin, tmax;
      tmin = (stf->leftNormalAt(0)).x();
      tmax = (stf->rightNormalAt(0)).x();
      m_16bitEditor->setMapping(tmin, tmax);
      m_16bitEditor->setGradientStops(stf->gradientStops());
    }
  else
    m_16bitEditor->setGradientStops(QGradientStops());
}

void
SplineEditorWidget::setMapping(QPolygonF fmap)
{
//  m_splineEditor->setMapping(fmap);
}

void
SplineEditorWidget::setHistogramImage(QImage histImg1,
				      QImage histImg2)
{
//  m_splineEditor->setHistogramImage(histImg1, histImg2);
}

void
SplineEditorWidget::setHistogram2D(int* hist2D)
{
  QList<uint> h;
  if (Global::bytesPerVoxel() == 1)
    {
      for(int i=0; i<256; i++)
	h << hist2D[i];
    }
  else
    {
      for(int i=0; i<256*256; i++)
	h << hist2D[i];
    }

  m_16bitEditor->setHistogram(h);
}

void
SplineEditorWidget::setGradientStops(QGradientStops stops)
{
  m_splineEditor->setGradientStops(stops);
  m_16bitEditor->setGradientStops(stops);
}

void
SplineEditorWidget::deselectSpineEvent()
{
  emit deselectEvent();
}

void
SplineEditorWidget::selectSpineEvent(QGradientStops stops)
{
  emit selectEvent(stops);
}

void
SplineEditorWidget::updateTransferFunction()
{
  QImage lookupTable = m_splineEditor->colorMapImage();
  emit transferFunctionChanged(lookupTable);
}

void
SplineEditorWidget::updateTransferFunction(float tmin, float tmax)
{
  if (m_splineEditor->set16BitPoint(tmin, tmax))
    {
      QImage lookupTable = m_splineEditor->colorMapImage();
      emit transferFunctionChanged(lookupTable);
    }
}

void
SplineEditorWidget::paintEvent(QPaintEvent *event)
{
  QRectF brect = m_splineEditor->boundingRect();
 
  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing);
  
  p.setPen(QColor(46, 86, 146));
  p.drawRect(rect());
}
