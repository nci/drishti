#include "slices.h"
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


  m_imageWidget->setScrollArea(scrollArea);
}

void
Slices::createMenu(QHBoxLayout *hl,
		   QVBoxLayout *vl)
{
  QHBoxLayout *thl = new QHBoxLayout();
  
  m_zoom9 = new QPushButton("F",this);
  m_zoom0 = new QPushButton("O",this);
  m_zoomUp = new QPushButton("+",this);
  m_zoomDown = new QPushButton("-",this);

  m_mesg = new QLabel("Graph Cut");

  thl->addWidget(m_zoom9);
  thl->addWidget(m_zoom0);
  thl->addWidget(m_zoomUp);
  thl->addWidget(m_zoomDown);
  thl->addStretch();
  thl->addWidget(m_mesg);

  vl->addLayout(thl);

  m_slider = new QSlider(Qt::Vertical, this);
  m_slider->setInvertedAppearance(true);
  hl->addWidget(m_slider);

  m_zoom9->setMaximumSize(30,30);
  m_zoom0->setMaximumSize(30,30);
  m_zoomUp->setMaximumSize(30,30);
  m_zoomDown->setMaximumSize(30,30);

  m_zoomUp->setAutoRepeat(true);
  m_zoomDown->setAutoRepeat(true);

  connect(m_zoom0, SIGNAL(clicked()), m_imageWidget, SLOT(zoom0Clicked()));
  connect(m_zoom9, SIGNAL(clicked()), m_imageWidget, SLOT(zoom9Clicked()));
  connect(m_zoomUp, SIGNAL(clicked()), m_imageWidget, SLOT(zoomUpClicked()));
  connect(m_zoomDown, SIGNAL(clicked()), m_imageWidget, SLOT(zoomDownClicked()));
  
  connect(m_slider, SIGNAL(valueChanged(int)), m_imageWidget, SLOT(setSlice(int)));

  connect(m_slider, SIGNAL(valueChanged(int)), this, SIGNAL(sliceChanged(int)));

  connect(m_imageWidget, SIGNAL(xPos(int)), this, SIGNAL(xPos(int)));
  connect(m_imageWidget, SIGNAL(yPos(int)), this, SIGNAL(yPos(int)));
  connect(m_imageWidget, SIGNAL(sliceChanged(int)), this, SIGNAL(sliceChanged(int)));

  connect(m_imageWidget, SIGNAL(sliceChanged(int)),
	  m_slider, SLOT(setValue(int)));

  connect(m_imageWidget, SIGNAL(updateBB(Vec, Vec)),
	  this, SIGNAL(updateBB(Vec, Vec)));

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

  connect(m_imageWidget, SIGNAL(saveMask()), this, SIGNAL(saveMask()));

  connect(m_imageWidget, SIGNAL(shrinkwrap(Vec, Vec, int, bool, int,
					   bool, int, int, int, int)),
	  this, SIGNAL(shrinkwrap(Vec, Vec, int, bool, int,
					   bool, int, int, int, int)));

  connect(m_imageWidget, SIGNAL(connectedRegion(int,int,int,Vec,Vec,int,int)),
	  this, SIGNAL(connectedRegion(int,int,int,Vec,Vec,int,int)));
}

void Slices::setSliceType(int s) { m_imageWidget->setSliceType(s);}
void Slices::setVolPtr(uchar *v) { m_imageWidget->setVolPtr(v); }
void Slices::setMaskPtr(uchar *v) { m_imageWidget->setMaskPtr(v); }

void
Slices::setGridSize(int d, int w, int h)
{
  m_imageWidget->setGridSize(d,w,h);
  m_imageWidget->resetSliceType();

  if (m_imageWidget->sliceType() == ImageWidget::DSlice)
    {
      m_slider->setMaximum(d-1);
      m_slider->setValue(d/2);
    }
  if (m_imageWidget->sliceType() == ImageWidget::WSlice)
    {
      m_slider->setMaximum(w-1);
      m_slider->setValue(w/2);
    }
  if (m_imageWidget->sliceType() == ImageWidget::HSlice)
    {
      m_slider->setMaximum(h-1);
      m_slider->setValue(h/2);
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
Slices::setSlice(int s)
{
  m_slider->setValue(s);
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

void Slices::setRawValue(QList<int> u) { m_imageWidget->setRawValue(u); }

void
Slices::setModeType(int mt)
{
  if (mt == 0)
    m_mesg->setText("<font color=green><h2>Graph Cut</h2>");

  if (mt == 1)
    m_mesg->setText("<font color=red><h2>Superpixels</h2>");

  m_imageWidget->setModeType(mt);
}
