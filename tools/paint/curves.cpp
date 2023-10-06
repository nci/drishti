#include "curves.h"
#include "global.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QMessageBox>

Curves::Curves(QWidget *parent, QStatusBar *sb) :
  QWidget(parent)
{
  m_curvesWidget = new CurvesWidget(this, sb);
  
  QScrollArea *scrollArea = new QScrollArea;
  scrollArea->setBackgroundRole(QPalette::Dark);
  scrollArea->setWidget(m_curvesWidget);

  QHBoxLayout *hl = new QHBoxLayout();
  QVBoxLayout *vl = new QVBoxLayout();
  
  createMenu(hl, vl);
  
  hl->addWidget(scrollArea);
  vl->addLayout(hl);

  setLayout(vl);

  m_maximized = false;

  m_curvesWidget->setScrollArea(scrollArea);
  scrollArea->setStyleSheet("background-color:honeydew;");
  m_slider->setBackgroundColor(QColor(240, 255, 240));
}

void
Curves::createMenu(QHBoxLayout *hl,
		   QVBoxLayout *vl)
{
  QHBoxLayout *thl = new QHBoxLayout();
  
  m_zoom9 = new QPushButton("F",this);
  m_zoom0 = new QPushButton("O",this);
  m_zoomUp = new QPushButton(QIcon(":/images/zoom-in.png"),"");
  m_zoomDown = new QPushButton(QIcon(":/images/zoom-out.png"),"");

  m_changeLayout = new QPushButton(QIcon(":/images/enlarge.png"),"");

  m_sliceNum = new QLineEdit("");

  QFont font("Helvetica", 15);
  m_sliceNum->setFont(font);
  QFontMetrics metric(font);
  int mwd = metric.width("000000");
  m_sliceNum->setMaximumWidth(mwd);
  
  thl->addWidget(m_zoom9);
  thl->addWidget(m_zoom0);
  thl->addWidget(m_zoomUp);
  thl->addWidget(m_zoomDown);
  thl->addStretch();
  thl->addWidget(m_sliceNum);
  thl->addStretch();
  thl->addWidget(m_changeLayout);

  vl->addLayout(thl);

  m_slider = new MySlider();
  hl->addWidget(m_slider);

  m_zoom9->setMaximumSize(32,32);
  m_zoom0->setMaximumSize(32,32);
  m_zoomUp->setMaximumSize(32,32);
  m_zoomDown->setMaximumSize(32,32);
  m_changeLayout->setMaximumSize(32,32);

  m_zoomUp->setAutoRepeat(true);
  m_zoomDown->setAutoRepeat(true);

  connect(m_changeLayout, SIGNAL(clicked()), this, SIGNAL(changeLayout()));
  
  connect(m_curvesWidget, SIGNAL(xPos(int)), this, SIGNAL(xPos(int)));
  connect(m_curvesWidget, SIGNAL(yPos(int)), this, SIGNAL(yPos(int)));

  connect(m_curvesWidget, SIGNAL(gotFocus()), this, SIGNAL(gotFocus()));

  connect(m_sliceNum, SIGNAL(editingFinished()),
	  this, SLOT(sliceNumChanged()));

  connect(m_zoom0, SIGNAL(clicked()), m_curvesWidget, SLOT(zoom0Clicked()));
  connect(m_zoom9, SIGNAL(clicked()), m_curvesWidget, SLOT(zoom9Clicked()));
  connect(m_zoomUp, SIGNAL(clicked()), m_curvesWidget, SLOT(zoomUpClicked()));
  connect(m_zoomDown, SIGNAL(clicked()), m_curvesWidget, SLOT(zoomDownClicked()));
  
  connect(m_slider, SIGNAL(valueChanged(int)),
	  m_curvesWidget, SLOT(sliceChanged(int)));
  connect(m_slider, SIGNAL(valueChanged(int)),
	  this, SLOT(setSliceNumber(int)));

  connect(m_slider, SIGNAL(userRangeChanged(int, int)),
	  m_curvesWidget, SLOT(userRangeChanged(int, int)));

  connect(m_curvesWidget, SIGNAL(saveWork()), this, SIGNAL(saveWork()));
  connect(m_curvesWidget, SIGNAL(saveMask()), this, SIGNAL(saveMask()));
  connect(m_curvesWidget, SIGNAL(viewerUpdate()), this, SIGNAL(viewerUpdate()));

  connect(m_curvesWidget, SIGNAL(tagDSlice(int, uchar*)),
	  this, SIGNAL(tagDSlice(int, uchar*)));

  connect(m_curvesWidget, SIGNAL(tagWSlice(int, uchar*)),
	  this, SIGNAL(tagWSlice(int, uchar*)));

  connect(m_curvesWidget, SIGNAL(tagHSlice(int, uchar*)),
	  this, SIGNAL(tagHSlice(int, uchar*)));

  connect(m_curvesWidget, SIGNAL(showEndCurve()),
	  this, SIGNAL(showEndCurve()));
  connect(m_curvesWidget, SIGNAL(hideEndCurve()),
	  this, SIGNAL(hideEndCurve()));

  connect(m_curvesWidget, SIGNAL(getSlice(int)),
	  this, SIGNAL(getSlice(int)));

  connect(m_curvesWidget, SIGNAL(getRawValue(int, int, int)),
	  this, SIGNAL(getRawValue(int, int, int)));

  connect(m_curvesWidget, SIGNAL(getRawValue(int, int, int)),
	  this, SIGNAL(getRawValue(int, int, int)));

  connect(m_curvesWidget, SIGNAL(polygonLevels(QList<int>)),
	  m_slider, SLOT(polygonLevels(QList<int>)));

  connect(m_curvesWidget, SIGNAL(updateViewerBox(int, int, int, int, int, int)),
	  this, SIGNAL(updateViewerBox(int, int, int, int, int, int)));

  connect(m_curvesWidget, SIGNAL(setPropagation(bool)),
	  this, SIGNAL(setPropagation(bool)));
}

void Curves::releaseFocus() { m_curvesWidget->releaseFocus(); }
void Curves::setSliceType(int s) { m_curvesWidget->setSliceType(s);}
void Curves::resetSliceType() { m_curvesWidget->resetSliceType();}
void Curves::setVolPtr(uchar *v) { m_curvesWidget->setVolPtr(v); }
//void Curves::setMaskPtr(uchar *v) { m_curvesWidget->setMaskPtr(v); }

void Curves::saveCurves() { m_curvesWidget->saveCurves(); }
void Curves::saveCurves(QString curvesfile) { m_curvesWidget->saveCurves(curvesfile); }

void Curves::loadCurves() { m_curvesWidget->loadCurves(); }
void Curves::loadCurves(QString curvesfile) { m_curvesWidget->loadCurves(curvesfile); }

void Curves::setCurve(bool b) { m_curvesWidget->setCurve(b); }
void Curves::setLivewire(bool b) { m_curvesWidget->setLivewire(b); }

void Curves::freezeLivewire(bool b) { m_curvesWidget->freezeLivewire(b); }

void Curves::sliceChanged() { m_curvesWidget->sliceChanged(m_slider->value()); }

void Curves::depthUserRange(int& u0, int& u1) { m_curvesWidget->depthUserRange(u0, u1); }
void Curves::widthUserRange(int& u0, int& u1) { m_curvesWidget->widthUserRange(u0, u1); }
void Curves::heightUserRange(int& u0, int& u1) { m_curvesWidget->heightUserRange(u0, u1); }

void Curves::resetCurves() { m_curvesWidget->resetCurves(); }

bool Curves::curvesPresent() { return m_curvesWidget->curvesPresent(); }

void Curves::paintUsingCurves(int slctype,
			      int slc, int wd, int ht,
			      uchar *maskData)
{
  m_curvesWidget->paintUsingCurves(slctype, slc, wd, ht, maskData);
}


void
Curves::setGridSize(int d, int w, int h)
{
  m_Depth = d;
  m_Width = w;
  m_Height = h;
  m_s0 = 0;

  m_curvesWidget->setGridSize(d,w,h);

  if (m_curvesWidget->sliceType() == CurvesWidget::DSlice)
    {
      QValidator *valid = new QIntValidator(0, d-1);
      m_sliceNum->setValidator(valid);
      m_s1 = d-1;
    }
  if (m_curvesWidget->sliceType() == CurvesWidget::WSlice)
    {
      QValidator *valid = new QIntValidator(0, w-1);
      m_sliceNum->setValidator(valid);
      m_s1 = w-1;
    }
  if (m_curvesWidget->sliceType() == CurvesWidget::HSlice)
    {
      QValidator *valid = new QIntValidator(0, h-1);
      m_sliceNum->setValidator(valid);
      m_s1 = h-1;
    }  
}

void Curves::setHLine(int h) { m_curvesWidget->setHLine(h); }
void Curves::setVLine(int v) { m_curvesWidget->setVLine(v); }


void
Curves::sliceNumChanged()
{
  int s = m_sliceNum->text().toInt();
  m_slider->setValue(s);  
  emit getSlice(s);
}


void
Curves::setSliceNumber(int s)
{
  m_sliceNum->setText(QString("%1").arg(s));
}

void
Curves::setSlice(int s)
{
  m_slider->setValue(s);
}

void Curves::updateTagColors() { m_curvesWidget->updateTagColors(); }
void Curves::getBox(int& minD, int& maxD, 
		    int& minW, int& maxW, 
		    int& minH, int& maxH)
{
  m_curvesWidget->getBox(minD, maxD, minW, maxW, minH, maxH);
}
void Curves::setBox(int minD, int maxD, 
		    int minW, int maxW, 
		    int minH, int maxH)
{
  m_curvesWidget->setBox(minD, maxD, minW, maxW, minH, maxH);
}

void Curves::loadLookupTable(QImage img) { m_curvesWidget->loadLookupTable(img); }

void Curves::setImage(uchar *slice, uchar *maskslice) { m_curvesWidget->setImage(slice, maskslice); }
void Curves::setMaskImage(uchar *maskslice) { m_curvesWidget->setMaskImage(maskslice); }

void Curves::setRawValue(QList<int> u) { m_curvesWidget->setRawValue(u); }

void Curves::setGradThresholdType(int t) { m_curvesWidget->setGradThresholdType(t); }
void Curves::setMinGrad(float t) { m_curvesWidget->setMinGrad(t); }
void Curves::setMaxGrad(float t) { m_curvesWidget->setMaxGrad(t); }

void Curves::setSliceLOD(int b) { m_curvesWidget->setSliceLOD(b); }
void Curves::setPointSize(int b) { m_curvesWidget->setPointSize(b); }
void Curves::setSmoothType(int b) { m_curvesWidget->setSmoothType(b); }
void Curves::setGradType(int b) { m_curvesWidget->setGradType(b); }
void Curves::newCurve() { m_curvesWidget->newCurve(false); }
void Curves::endCurve() { m_curvesWidget->endCurve(); }
void Curves::morphCurves() { m_curvesWidget->morphCurves(); }
void Curves::deleteAllCurves() { m_curvesWidget->deleteAllCurves(); }


void
Curves::setLarge(bool ms)
{
  m_maximized = ms;

  if (m_maximized)
    {
      m_changeLayout->setIcon(QIcon(":/images/shrink.png"));
      m_curvesWidget->setInFocus();
    }
  else
    {
      m_changeLayout->setIcon(QIcon(":/images/enlarge.png"));
      m_curvesWidget->releaseFocus();
    }
}

void
Curves::setShowPosition(bool s)
{
  m_curvesWidget->setShowPosition(s);
}

void
Curves::setSliderRange(int s0, int s1, int u0, int u1, int v)
{
  m_slider->set(s0, s1, u0, u1, v);
}
