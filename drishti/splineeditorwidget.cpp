#include "global.h"
#include "splineeditorwidget.h"

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
  m_connectionPen = QPen(QColor(200, 200, 200, 150), 1);
  m_pointBrush = QBrush(QColor(255, 255, 255, 200));

  m_1dHist  = new QRadioButton("1D", this);
  m_2dHist  = new QRadioButton("2D", this);

  m_vol1 = new QRadioButton("V1", this);
  m_vol2 = new QRadioButton("V2", this);
  m_vol3 = new QRadioButton("V3", this);
  m_vol4 = new QRadioButton("V4", this);

  QButtonGroup *butGrp1 = new QButtonGroup(this);
  butGrp1->setExclusive(true);
  butGrp1->addButton(m_1dHist);
  butGrp1->addButton(m_2dHist);
  QButtonGroup *butGrp2 = new QButtonGroup(this);
  butGrp2->setExclusive(true);
  butGrp2->addButton(m_vol1);
  butGrp2->addButton(m_vol2);
  butGrp2->addButton(m_vol3);
  butGrp2->addButton(m_vol4);

  m_2dHist->setChecked(true);
  m_vol1->setChecked(true);

  QHBoxLayout *hbox = new QHBoxLayout();
  hbox->addStretch();
  hbox->addWidget(m_vol4);
  hbox->addWidget(m_vol3);
  hbox->addWidget(m_vol2);
  hbox->addWidget(m_vol1);

  m_gbotValue = new QLineEdit("1.0");
  m_gbotValue->setMaximumSize(30, 25);
  QRegExp rx("(\\-?\\d*\\.?\\d*\\s?)");
  QRegExpValidator *validator = new QRegExpValidator(rx,0);
  m_gbotValue->setValidator(validator);

  m_gtopValue = new QLineEdit("1.0");
  m_gtopValue->setMaximumSize(30, 25);
  m_gtopValue->setValidator(validator);

  QHBoxLayout *hboxt = new QHBoxLayout();
  hboxt->addWidget(m_gbotValue);
  hboxt->addStretch();
  hboxt->addWidget(m_1dHist);
  hboxt->addWidget(m_2dHist);
  hboxt->addStretch();
  hboxt->addWidget(m_gtopValue);

  m_gbotSlider = new QSlider(Qt::Vertical);
  m_gbotSlider->setRange(0, 255);
  m_gbotSlider->setValue(0);
  
  m_gtopSlider = new QSlider(Qt::Vertical);
  m_gtopSlider->setRange(0, 255);
  m_gtopSlider->setValue(255);

  m_16bitEditor = new RemapHistogramWidget(this);
  m_16bitEditor->setSizePolicy(QSizePolicy::Expanding,
			       QSizePolicy::Expanding);

  m_splineEditor = new SplineEditor(this);
  m_splineEditor->setSizePolicy(QSizePolicy::Expanding,
				QSizePolicy::Expanding);
  QHBoxLayout *hbox1 = new QHBoxLayout;
  hbox1->addWidget(m_gbotSlider);
  hbox1->addWidget(m_splineEditor);
  hbox1->addWidget(m_16bitEditor);
  hbox1->addWidget(m_gtopSlider);

  QVBoxLayout *vbox = new QVBoxLayout();
  vbox->addLayout(hboxt);
  vbox->addLayout(hbox1);
  vbox->addLayout(hbox);

  setLayout(vbox);

  m_vol1->hide();
  m_vol2->hide();
  m_vol3->hide();
  m_vol4->hide();


  m_splineEditor->setShapePen(m_pointPen);
  m_splineEditor->setConnectionPen(m_connectionPen);
  m_splineEditor->setShapeBrush(m_pointBrush);


  connect(m_gtopSlider, SIGNAL(sliderReleased()),
	  this, SLOT(gtopSliderReleased()));
  connect(m_gbotSlider, SIGNAL(sliderReleased()),
	  this, SLOT(gbotSliderReleased()));
  connect(m_gtopValue, SIGNAL(editingFinished()),
	  this, SLOT(gValueChanged()));
  connect(m_gbotValue, SIGNAL(editingFinished()),
	  this, SLOT(gValueChanged()));

  connect(m_1dHist, SIGNAL(toggled(bool)),
	  this, SLOT(hist1DClicked(bool)));

  connect(m_2dHist, SIGNAL(toggled(bool)),
	  this, SLOT(hist2DClicked(bool)));

  connect(m_vol1, SIGNAL(toggled(bool)),
	  this, SLOT(vol1Clicked(bool)));

  connect(m_vol2, SIGNAL(toggled(bool)),
	  this, SLOT(vol2Clicked(bool)));

  connect(m_vol3, SIGNAL(toggled(bool)),
	  this, SLOT(vol3Clicked(bool)));

  connect(m_vol4, SIGNAL(toggled(bool)),
	  this, SLOT(vol4Clicked(bool)));

  connect(m_splineEditor, SIGNAL(refreshDisplay()),
	  this, SLOT(update()));

  connect(m_splineEditor, SIGNAL(splineChanged()),
	  this, SLOT(updateTransferFunction()));

  connect(m_splineEditor, SIGNAL(selectEvent(QGradientStops)),
	  this, SLOT(selectSpineEvent(QGradientStops)));

  connect(m_splineEditor, SIGNAL(deselectEvent()),
	  this, SLOT(deselectSpineEvent()));

  connect(m_splineEditor, SIGNAL(applyUndo(bool)),
	  this, SIGNAL(applyUndo(bool)));

  connect(m_16bitEditor, SIGNAL(newMapping(float, float)),
	  this, SLOT(updateTransferFunction(float, float)));

  setMinimumSize(200, 200);

  m_16bitEditor->hide();
  m_show16BitEditor = false;
}

void
SplineEditorWidget::show16BitEditor(bool b)
{
  m_show16BitEditor = b;
  if (m_show16BitEditor)
    {
      m_16bitEditor->show();

      m_splineEditor->hide();
      m_1dHist->hide();
      m_2dHist->hide();
      m_gbotValue->hide();
      m_gtopValue->hide();
      m_gbotSlider->hide();
      m_gtopSlider->hide();
    }
  else
    {
      m_16bitEditor->hide();

      m_splineEditor->show();
      m_1dHist->show();
      m_2dHist->show();
      m_gbotValue->show();
      m_gtopValue->show();
      m_gbotSlider->show();
      m_gtopSlider->show();
    }
}
void
SplineEditorWidget::gbotSliderReleased()
{
  int val = m_gbotSlider->value();
  if (val > m_gtopSlider->value())
    {
      m_gtopSlider->setValue(val);
    }
//  int bv = qMin(val, m_gtopSlider->value()-1);
//  bv = qMax(0, bv);
//  m_gbotSlider->setValue(bv);

  m_splineEditor->setGradLimits(m_gbotSlider->value(),
				m_gtopSlider->value());
}
void
SplineEditorWidget::gtopSliderReleased()
{
  int val = m_gtopSlider->value();
  if (val < m_gbotSlider->value())
    {
      m_gbotSlider->setValue(val);
    }
//  int bv = qMax(val, m_gbotSlider->value()+1);
//  bv = qMin(255, bv);
//  m_gtopSlider->setValue(bv);

  m_splineEditor->setGradLimits(m_gbotSlider->value(),
				m_gtopSlider->value());
}
void
SplineEditorWidget::gValueChanged()
{
  float bot = (m_gbotValue->text()).toFloat();
  float top = (m_gtopValue->text()).toFloat();
  m_splineEditor->setOpmod(bot, top);
}

void
SplineEditorWidget::hist1DClicked(bool flag)
{
  if (flag)
    m_splineEditor->show2D(false);
}

void
SplineEditorWidget::hist2DClicked(bool flag)
{
  if (flag)
    m_splineEditor->show2D(true);
}

void
SplineEditorWidget::vol1Clicked(bool flag)
{
  if (flag)
    emit giveHistogram(0);
}

void
SplineEditorWidget::vol2Clicked(bool flag)
{
  if (flag)
    emit giveHistogram(1);
}

void
SplineEditorWidget::vol3Clicked(bool flag)
{
  if (flag)
    emit giveHistogram(2);
}

void
SplineEditorWidget::vol4Clicked(bool flag)
{
  if (flag)
    emit giveHistogram(3);
}

void
SplineEditorWidget::changeVol(int vnum)
{
  if (vnum == 0)
    {
      m_vol1->setChecked(true);
      emit giveHistogram(0);
    }
  else if (vnum == 1)
    {
      m_vol2->setChecked(true);
      emit giveHistogram(1);
    }
  else if (vnum == 2)
    {
      m_vol3->setChecked(true);
      emit giveHistogram(2);
    }
  else if (vnum == 3)
    {
      m_vol4->setChecked(true);
      emit giveHistogram(3);
    }
}


void
SplineEditorWidget::setTransferFunction(SplineTransferFunction *stf)
{
  if (stf == NULL)
    {
      m_vol1->setText("V1");
      m_vol2->setText("V2");
      m_vol3->setText("V3");
      m_vol4->setText("V4");


      m_vol1->hide();
      m_vol2->hide();
      m_vol3->hide();
      m_vol4->hide();

      if (Global::volumeType() == Global::DoubleVolume)
	{
	  m_vol1->show();
	  m_vol1->setChecked(true);
	  m_vol2->show();
	  m_vol3->hide();
	  m_vol4->hide();
	}
      else if (Global::volumeType() == Global::TripleVolume)
	{
	  m_vol1->show();
	  m_vol1->setChecked(true);
	  m_vol2->show();
	  m_vol3->show();
	  m_vol4->hide();
	}
      else if (Global::volumeType() == Global::QuadVolume)
	{
	  m_vol1->show();
	  m_vol1->setChecked(true);
	  m_vol2->show();
	  m_vol3->show();
	  m_vol4->show();
	}
      else if (Global::volumeType() == Global::RGBVolume ||
	       Global::volumeType() == Global::RGBAVolume)
	{
	  m_vol1->setText("R-G");
	  m_vol2->setText("G-B");
	  m_vol3->setText("B-R");

	  m_vol1->show();
	  m_vol1->setChecked(true);
	  m_vol2->show();
	  m_vol3->show();
	  m_vol4->hide();

	  if (Global::volumeType() == Global::RGBAVolume)
	    {	      
	     m_vol4->setText("A-max(RGB)");
	     m_vol4->show();
	    }
	  else
	    m_vol4->hide();
	}
    }

  if (stf != NULL)
    {
      int top, bot;
      stf->gradLimits(bot, top);
      m_gbotSlider->setValue(bot);
      m_gtopSlider->setValue(top);

      float topop, botop;
      stf->opMod(botop, topop);
      m_gbotValue->setText(QString("%1").arg(botop));
      m_gtopValue->setText(QString("%1").arg(topop));
    }
  else
    {
      m_gbotSlider->setValue(0);
      m_gtopSlider->setValue(255);
      m_gbotValue->setText("1.0");
      m_gtopValue->setText("1.0");
    }
    
  m_splineEditor->setTransferFunction(stf);

//  if (Global::pvlVoxelType() > 0)
    {
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
}

void
SplineEditorWidget::setMapping(QPolygonF fmap)
{
  if (m_show16BitEditor)
    {
      m_16bitEditor->show();

      m_splineEditor->hide();
      m_1dHist->hide();
      m_2dHist->hide();
      m_gbotValue->hide();
      m_gtopValue->hide();
      m_gbotSlider->hide();
      m_gtopSlider->hide();
    }
  else
    {
      m_16bitEditor->hide();

      m_splineEditor->show();
      m_splineEditor->setMapping(fmap);
      m_1dHist->show();
      m_2dHist->show();
      m_gbotValue->show();
      m_gtopValue->show();
      m_gbotSlider->show();
      m_gtopSlider->show();
    }

}

void
SplineEditorWidget::setHistogramImage(QImage histImg1,
				      QImage histImg2)
{
  if (Global::pvlVoxelType() == 0)
    m_splineEditor->setHistogramImage(histImg1, histImg2);
}
void
SplineEditorWidget::setHistogram2D(int* hist2D)
{
  //if (Global::pvlVoxelType() > 0)
    {
      QList<uint> h;
      if (Global::pvlVoxelType() == 0)
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
}

void
SplineEditorWidget::setGradientStops(QGradientStops stops)
{
  m_splineEditor->setGradientStops(stops);
  //  if (Global::pvlVoxelType() > 0)
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
