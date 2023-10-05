#include "slices.h"
#include "global.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QMessageBox>

Slices::Slices(QWidget *parent) :
  QWidget(parent)
{
  m_imageWidget = new ImageWidget(this);

  QScrollArea *scrollArea = new QScrollArea;
  scrollArea->setBackgroundRole(QPalette::Dark);
  scrollArea->setWidget(m_imageWidget);
  
  QHBoxLayout *hl = new QHBoxLayout();
  QVBoxLayout *vl = new QVBoxLayout();
  
  createMenu(hl, vl);
  
  hl->addWidget(scrollArea);
  vl->addLayout(hl);

  setLayout(vl);

  m_maximized = false;

  m_imageWidget->setScrollArea(scrollArea);
}

void
Slices::createMenu(QHBoxLayout *hl,
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

  connect(m_zoom0, SIGNAL(clicked()), m_imageWidget, SLOT(zoom0Clicked()));
  connect(m_zoom9, SIGNAL(clicked()), m_imageWidget, SLOT(zoom9Clicked()));
  connect(m_zoomUp, SIGNAL(clicked()), m_imageWidget, SLOT(zoomUpClicked()));
  connect(m_zoomDown, SIGNAL(clicked()), m_imageWidget, SLOT(zoomDownClicked()));
  
  connect(m_slider, SIGNAL(valueChanged(int)), m_imageWidget, SLOT(setSlice(int)));
  connect(m_slider, SIGNAL(valueChanged(int)), this, SLOT(setSliceNumber(int)));
  connect(m_slider, SIGNAL(valueChanged(int)), this, SIGNAL(sliceChanged(int)));

  connect(m_slider, SIGNAL(userRangeChanged(int, int)),
	  m_imageWidget, SLOT(userRangeChanged(int, int)));


  connect(m_imageWidget, SIGNAL(xPos(int)), this, SIGNAL(xPos(int)));
  connect(m_imageWidget, SIGNAL(yPos(int)), this, SIGNAL(yPos(int)));
  connect(m_imageWidget, SIGNAL(sliceChanged(int)), this, SIGNAL(sliceChanged(int)));

  connect(m_imageWidget, SIGNAL(sliceChanged(int)),
	  this, SLOT(setSlice(int)));
  connect(m_imageWidget, SIGNAL(setSliceNumber(int)),
	  this, SLOT(setSliceNumber(int)));

  connect(m_imageWidget, SIGNAL(disconnectSlider()),
	  this, SLOT(disconnectSlider()));
  connect(m_imageWidget, SIGNAL(reconnectSlider()),
	  this, SLOT(reconnectSlider()));

  connect(m_imageWidget, SIGNAL(saveWork()), this, SIGNAL(saveWork()));

  connect(m_imageWidget, SIGNAL(getRawValue(int, int, int)),
	  this, SIGNAL(getRawValue(int, int, int)));

  connect(m_imageWidget, SIGNAL(applyMaskOperation(int, int, int)),
	  this, SIGNAL(applyMaskOperation(int, int, int)));

  connect(m_imageWidget, SIGNAL(tagDSlice(int, uchar*)),
	  this, SIGNAL(tagDSlice(int, uchar*)));

  connect(m_imageWidget, SIGNAL(tagWSlice(int, uchar*)),
	  this, SIGNAL(tagWSlice(int, uchar*)));

  connect(m_imageWidget, SIGNAL(tagHSlice(int, uchar*)),
	  this, SIGNAL(tagHSlice(int, uchar*)));

  connect(m_imageWidget, SIGNAL(updateViewerBox(int, int, int, int, int, int)),
	  this, SIGNAL(updateViewerBox(int, int, int, int, int, int)));

  connect(m_imageWidget, SIGNAL(viewerUpdate()), this, SIGNAL(viewerUpdate()));

  connect(m_imageWidget, SIGNAL(shrinkwrap(Vec, Vec, int, bool, int,
					   bool, int, int, int, int)),
	  this, SIGNAL(shrinkwrap(Vec, Vec, int, bool, int,
					   bool, int, int, int, int)));

  connect(m_imageWidget, SIGNAL(connectedRegion(int,int,int,Vec,Vec,int,int)),
	  this, SIGNAL(connectedRegion(int,int,int,Vec,Vec,int,int)));


  connect(m_imageWidget, SIGNAL(updateSliderLimits()),
	  this, SLOT(updateSliderLimits()));
  connect(m_imageWidget, SIGNAL(resetSliderLimits()),
	  this, SLOT(resetSliderLimits()));

  connect(m_sliceNum, SIGNAL(editingFinished()),
	  this, SLOT(sliceNumChanged()));
}

void Slices::setSliceType(int s) { m_imageWidget->setSliceType(s);}
void Slices::setVolPtr(uchar *v) { m_imageWidget->setVolPtr(v); }
void Slices::setMaskPtr(uchar *v) { m_imageWidget->setMaskPtr(v); }

void
Slices::setGridSize(int d, int w, int h)
{
  m_Depth = d;
  m_Width = w;
  m_Height = h;
  m_s0 = 0;

  m_imageWidget->setGridSize(d,w,h);
  m_imageWidget->resetSliceType();

  if (m_imageWidget->sliceType() == ImageWidget::DSlice)
    {
      m_slider->set(0, d-1, 0, d-1, d/2);
      QValidator *valid = new QIntValidator(0, d-1);
      m_sliceNum->setValidator(valid);
      m_s1 = d-1;
    }
  if (m_imageWidget->sliceType() == ImageWidget::WSlice)
    {
      m_slider->set(0, w-1, 0, w-1, w/2);
      QValidator *valid = new QIntValidator(0, w-1);
      m_sliceNum->setValidator(valid);
      m_s1 = w-1;
    }
  if (m_imageWidget->sliceType() == ImageWidget::HSlice)
    {
      m_slider->set(0, h-1, 0, h-1, h/2);
      QValidator *valid = new QIntValidator(0, h-1);
      m_sliceNum->setValidator(valid);
      m_s1 = h-1;
    }  
}

void Slices::setHLine(int h) { m_imageWidget->setHLine(h); }
void Slices::setVLine(int v) { m_imageWidget->setVLine(v); }

void
Slices::reloadSlice()
{
  m_imageWidget->setSlice(m_slider->value());
}

void
Slices::sliceNumChanged()
{
  int s = m_sliceNum->text().toInt();
  m_slider->setValue(s);  
  m_imageWidget->setSlice(s);
}


void
Slices::disconnectSlider()
{
  disconnect(m_imageWidget, SIGNAL(sliceChanged(int)), this, SIGNAL(sliceChanged(int)));
  disconnect(m_slider, SIGNAL(valueChanged(int)), m_imageWidget, SLOT(setSlice(int)));
  disconnect(m_slider, SIGNAL(valueChanged(int)), this, SLOT(setSliceNumber(int)));
  disconnect(m_slider, SIGNAL(valueChanged(int)), this, SIGNAL(sliceChanged(int)));
  
}

void
Slices::reconnectSlider()
{
  connect(m_imageWidget, SIGNAL(sliceChanged(int)), this, SIGNAL(sliceChanged(int)));
  connect(m_slider, SIGNAL(valueChanged(int)), m_imageWidget, SLOT(setSlice(int)));
  connect(m_slider, SIGNAL(valueChanged(int)), this, SLOT(setSliceNumber(int)));
  connect(m_slider, SIGNAL(valueChanged(int)), this, SIGNAL(sliceChanged(int)));
}

void
Slices::setSliceNumber(int s)
{
  m_sliceNum->setText(QString("%1").arg(s));
}

void
Slices::setSlice(int s)
{
  m_slider->updateValue(s);
}

//void Slices::bbupdated(Vec bmin, Vec bmax) { m_imageWidget->bbupdated(bmin, bmax); }

void Slices::updateTagColors() { m_imageWidget->updateTagColors(); }
void Slices::resetSliceType() { m_imageWidget->resetSliceType(); }
void Slices::getBox(int& minD, int& maxD, 
		    int& minW, int& maxW, 
		    int& minH, int& maxH)
{
  m_imageWidget->getBox(minD, maxD, minW, maxW, minH, maxH);
}
void Slices::setBox(int minD, int maxD, 
		    int minW, int maxW, 
		    int minH, int maxH)
{
  m_imageWidget->setBox(minD, maxD, minW, maxW, minH, maxH);
}
void Slices::processPrevSliceTags() { m_imageWidget->processPrevSliceTags(); }

void Slices::loadLookupTable(QImage img) { m_imageWidget->loadLookupTable(img); }

void Slices::saveImage() { m_imageWidget->saveImage(); }
void Slices::saveImageSequence() { m_imageWidget->saveImageSequence(); }

void Slices::setRawValue(QList<int> u) { m_imageWidget->setRawValue(u); }

void
Slices::setModeType(int mt)
{
  m_imageWidget->setModeType(mt);
}

void
Slices::setLarge(bool ms)
{
  m_maximized = ms;

  if (m_maximized)
    m_changeLayout->setIcon(QIcon(":/images/shrink.png"));
  else
    m_changeLayout->setIcon(QIcon(":/images/enlarge.png"));
}

void
Slices::setShowPosition(bool s)
{
  m_imageWidget->setShowPosition(s);
}

void
Slices::resetSliderLimits()
{
  m_s0 = 0;
  if (m_imageWidget->sliceType() == ImageWidget::DSlice)
    m_s1 = m_Depth-1;
  if (m_imageWidget->sliceType() == ImageWidget::WSlice)
    m_s1 = m_Width-1;
  if (m_imageWidget->sliceType() == ImageWidget::HSlice)
    m_s1 = m_Height-1;

  m_slider->set(m_s0, m_s1, m_s0, m_s1, 0);
}

void
Slices::updateSliderLimits()
{
  Vec bsz = Global::boxSize3D();
  int cs = m_imageWidget->currentSliceNumber();
  
  if (m_imageWidget->sliceType() == ImageWidget::DSlice)
    {
      m_s0 = qMax(0, cs-(int)bsz.x/2);
      m_s1 =  qMin(m_Depth, m_s0+(int)bsz.x-1);
      m_s0 = qMax(0, m_s1 - (int)bsz.x+1);
    }
  if (m_imageWidget->sliceType() == ImageWidget::WSlice)
    {
      m_s0 = qMax(0, cs-(int)bsz.y/2);
      m_s1 =  qMin(m_Width, m_s0+(int)bsz.y-1);
      m_s0 = qMax(0, m_s1 - (int)bsz.y+1);
    }
  if (m_imageWidget->sliceType() == ImageWidget::HSlice)
    {
      m_s0 = qMax(0, cs-(int)bsz.z/2);
      m_s1 = qMin(m_Height, m_s0+(int)bsz.z-1);
      m_s0 = qMax(0, m_s1 - (int)bsz.z+1);
    }

  m_slider->set(m_s0, m_s1, m_s0, m_s1, 0);
}
