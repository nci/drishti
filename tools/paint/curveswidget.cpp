#include "curveswidget.h"
#include "global.h"
#include "staticfunctions.h"
#include <math.h>
#include "morphslice.h"

#include <QFile>
#include <QLabel>

CurveGroup* CurvesWidget::m_dCurves = 0;
CurveGroup* CurvesWidget::m_wCurves = 0;
CurveGroup* CurvesWidget::m_hCurves = 0;


void
CurvesWidget::setLambda(float l)
{
  m_Curves.setLambda(l);
}
void
CurvesWidget::setSegmentLength(int l)
{
  m_Curves.setSegmentLength(l);
}

void
CurvesWidget::getBox(int &minD, int &maxD, 
		    int &minW, int &maxW, 
		    int &minH, int &maxH)
{
  minD = m_minDSlice;
  maxD = m_maxDSlice;
  minW = m_minWSlice;
  maxW = m_maxWSlice;
  minH = m_minHSlice;
  maxH = m_maxHSlice;
}

void
CurvesWidget::setBox(int minD, int maxD, 
		    int minW, int maxW, 
		    int minH, int maxH)
{
  m_minDSlice = minD;
  m_maxDSlice = maxD;
  m_minWSlice = minW;
  m_maxWSlice = maxW;
  m_minHSlice = minH;
  m_maxHSlice = maxH;
}

CurvesWidget::CurvesWidget(QWidget *parent, QStatusBar *sb) :
  QWidget(parent)
{
  m_statusBar = sb;
  setFocusPolicy(Qt::StrongFocus);
  
  setMouseTracking(true);

  m_hasFocus = false;
  
  m_showPosition = false;
  m_hline = m_vline = 0;

  m_livewireMode = false;
  m_curveMode = true;
  m_addingCurvePoints = false;

  m_Depth = m_Width = m_Height = 0;
  m_imgHeight = 100;
  m_imgWidth = 100;
  m_simgHeight = 100;
  m_simgWidth = 100;
  m_simgX = 10;
  m_simgY = 20;


  m_livewire.reset();
  m_Curves.reset();
  
  m_zoom = 1;

  m_lut = new uchar[4*256*256];
  memset(m_lut, 0, 4*256*256);

  m_minGrad = 0;
  m_maxGrad = 1;
  m_gradType = 0;
  
  m_volPtr = 0;
  
  m_image = QImage(100, 100, QImage::Format_RGB32);
  m_imageScaled = QImage(100, 100, QImage::Format_RGB32);
  m_maskimage = QImage(100, 100, QImage::Format_Indexed8);
  m_maskimageScaled = QImage(100, 100, QImage::Format_Indexed8);

  m_image.fill(0);
  m_imageScaled.fill(0);
  m_maskimage.fill(0);
  m_maskimageScaled.fill(0);

  m_sliceType = DSlice;
  m_slice = 0;
  m_sliceImage = 0;
  m_maskslice = 0;
  m_tags = 0;

  m_currSlice = 0;
  m_minDSlice = m_maxDSlice = 0;
  m_minWSlice = m_maxWSlice = 0;
  m_minHSlice = m_maxHSlice = 0;

  m_button = Qt::NoButton;
  m_cursorPos = QPoint(0,0);
  m_pickPoint = false;
  m_pickDepth = 0;
  m_pickWidth = 0;
  m_pickHeight = 0;
  m_lastPickDepth = 0;
  m_lastPickWidth = 0;
  m_lastPickHeight = 0;

  m_rubberBand = QRect(0,0,0,0);
  m_rubberXmin = false;
  m_rubberYmin = false;
  m_rubberXmax = false;
  m_rubberYmax = false;
  m_rubberNew = false;
  m_rubberBandActive = false;

  m_applyRecursive = false;
  m_extraPressed = false;
  m_cslc = 0;
  m_maxslc = 0;
  m_key = 0;
  m_forward = true;

  m_vgt.clear();

  updateTagColors();

  m_pointSize = 5;
}

void
CurvesWidget::setShowPosition(bool s)
{
  m_showPosition = s;

  if (!m_volPtr)
    return;

  update();
}

void
CurvesWidget::focusInEvent(QFocusEvent *e)
{
  setInFocus();
}
void
CurvesWidget::setInFocus()
{
  m_hasFocus = true;
  emit gotFocus();
}

void
CurvesWidget::releaseFocus()
{
  m_hasFocus = false;
  update();
}

void CurvesWidget::enterEvent(QEvent *e)
{
  if (m_hasFocus)
    {
      setFocus();
      //grabKeyboard();
    }
}
void CurvesWidget::leaveEvent(QEvent *e)
{
  //clearFocus();
  //releaseKeyboard();
}

void CurvesWidget::setScrollBars(QScrollBar *hbar,
				QScrollBar *vbar)
{
  m_hbar = hbar;
  m_vbar = vbar;
}


void
CurvesWidget::saveImage()
{
  QString imgFile = QFileDialog::getSaveFileName(0,
			 "Save composite image",
			 Global::previousDirectory(),
			 "Image Files (*.png *.tif *.bmp *.jpg)",
		         0,
		         QFileDialog::DontUseNativeDialog);

  if (imgFile.isEmpty())
    return;

  QImage sImage = m_image;
  QPainter p(&sImage);
  p.setCompositionMode(QPainter::CompositionMode_Overlay);
  p.drawImage(0,0, m_maskimage);

  sImage.save(imgFile);
  QMessageBox::information(0, "Save Image", "Done");
}

void
CurvesWidget::setZoom(float z)
{  
  setMinimumSize(QSize(m_imgWidth, m_imgHeight));

  bool z9 = false;
  int z9x, z9y, z9mx, z9my;
  if (qAbs(z-1) < 0.01)
    {
      m_zoom = qMax(0.01f, z);
    }
  else
    {
      int size1, size2;
      int imin, imax, jmin, jmax;
      getSliceLimits(size1, size2, imin, imax, jmin, jmax);

      if (z < 0)
	m_zoom = 1;
      else
	m_zoom = qMax(0.01f, z);

      if (size1 > 0 && size2 > 0)
	{
	  QWidget *prt = (QWidget*)parent();
	  int frmHeight = prt->rect().height()-50;
	  int frmWidth = prt->rect().width()-50;

	  if (z < 0)
	    {
	      m_zoom = qMin((float)frmWidth/size2,
			    (float)frmHeight/size1);
	      m_zoom = qMax(0.01f, m_zoom);
	    }
      
	  z9 = true;
	  z9x = m_zoom*(jmax+jmin)/2;
	  z9y = m_zoom*(imax+imin)/2;
	  z9mx = (frmWidth+65)/2;
	  z9my = (frmHeight+65)/2;
	}
    }

  resizeImage();
  update();
    
  if (z9)
    m_scrollArea->ensureVisible(z9x, z9y, z9mx, z9my);
}



void
CurvesWidget::depthUserRange(int& umin, int& umax)
{
  umin = m_minDSlice;
  umax = m_maxDSlice;
}
void
CurvesWidget::widthUserRange(int& umin, int& umax)
{
  umin = m_minWSlice;
  umax = m_maxWSlice;
}
void
CurvesWidget::heightUserRange(int& umin, int& umax)
{
  umin = m_minHSlice;
  umax = m_maxHSlice;
}

void
CurvesWidget::sliceChanged(int slc)
{
  // if we are in curve add points mode switch it off
  if (m_addingCurvePoints) endCurve();

  m_currSlice = slc;

  emit getSlice(m_currSlice);
}

void
CurvesWidget::userRangeChanged(int umin, int umax)
{
  if (m_sliceType == DSlice)
    {
      m_minDSlice = umin;
      m_maxDSlice = umax;
    }
  else if (m_sliceType == WSlice)
    {
      m_minWSlice = umin;
      m_maxWSlice = umax;
    }
  else
    {
      m_minHSlice = umin;
      m_maxHSlice = umax;
    }
  
  emit updateViewerBox(m_minDSlice, m_maxDSlice,
		       m_minWSlice, m_maxWSlice,
		       m_minHSlice, m_maxHSlice);
  
  update();
}


void
CurvesWidget::loadLookupTable(QImage colorMap)
{
  QImage lutImage = colorMap.mirrored(false, true);
  uchar *bits = lutImage.bits();
  memcpy(m_lut, bits, 4*256*256);

  Global::setLut(m_lut);

  recolorImage();
  resizeImage();
  update();
}

void
CurvesWidget::resetCurves()
{
  m_Curves.reset();
}

void
CurvesWidget::setGridSize(int d, int w, int h)
{
  m_Depth = qMax(0, d-1);
  m_Width = qMax(0, w-1);
  m_Height = qMax(0, h-1);
   
  m_minDSlice = 0;
  m_maxDSlice = m_Depth;
  m_minWSlice = 0;
  m_maxWSlice = m_Width;
  m_minHSlice = 0;
  m_maxHSlice = m_Height;
}

void CurvesWidget::setHLine(int h) { m_hline = h; update(); }
void CurvesWidget::setVLine(int v) { m_vline = v; update(); }

void
CurvesWidget::setSliceType(int st)
{
  m_sliceType = st;
  
  if (m_sliceType == DSlice)
    CurvesWidget::m_dCurves = &m_Curves;
  else if (m_sliceType == WSlice)
    CurvesWidget::m_wCurves = &m_Curves;
  else
    CurvesWidget::m_hCurves = &m_Curves;
}

void
CurvesWidget::resetSliceType()
{
  emit polygonLevels(m_Curves.polygonLevels());
  m_currSlice = 0;

  
  //---------------------------------
  int wd, ht;
  if (m_sliceType == DSlice)
    {
      wd = m_Height+1;
      ht = m_Width+1;
      m_maxSlice = m_Depth-1;
    }
  else if (m_sliceType == WSlice)
    {
      wd = m_Height+1;
      ht = m_Depth+1;
      m_maxSlice = m_Width-1;
    }
  else
    {
      wd = m_Width+1;
      ht = m_Depth+1;
      m_maxSlice = m_Height-1;
    }

  if (m_tags) delete [] m_tags;
  m_tags = new uchar[wd*ht];
  memset(m_tags, 0, wd*ht);

  if (m_slice) delete [] m_slice;
  m_slice = new uchar[Global::bytesPerVoxel()*wd*ht];

  if (m_maskslice) delete [] m_maskslice;
  m_maskslice = new uchar[wd*ht];
  memset(m_maskslice, 0, wd*ht);
  
  if (m_sliceImage) delete [] m_sliceImage;
  m_sliceImage = new uchar[4*wd*ht];
  //---------------------------------


  //---------------------------------
  // update rubberband extents
  float left, right, top, bottom;

  float width, height;
  if (m_sliceType == DSlice)
    {
      left = (float)m_minHSlice/(float)m_Height;
      right = (float)m_maxHSlice/(float)m_Height;
      top = (float)m_minWSlice/(float)m_Width;
      bottom = (float)m_maxWSlice/(float)m_Width;

      width = m_Height;
      height = m_Width;
    }
  else if (m_sliceType == WSlice)
    {
      left = (float)m_minHSlice/(float)m_Height;
      right = (float)m_maxHSlice/(float)m_Height;
      top = (float)m_minDSlice/(float)m_Depth;
      bottom = (float)m_maxDSlice/(float)m_Depth;

      width = m_Height;
      height = m_Depth;
    }
  else
    {
      left = (float)m_minWSlice/(float)m_Width;
      right = (float)m_maxWSlice/(float)m_Width;
      top = (float)m_minDSlice/(float)m_Depth;
      bottom = (float)m_maxDSlice/(float)m_Depth;

      width = m_Width;
      height = m_Depth;
    }

  left = qBound(0.0f, left, 1.0f);
  top = qBound(0.0f, top, 1.0f);
  right = qBound(0.0f, right, 1.0f);
  bottom = qBound(0.0f, bottom, 1.0f);

  m_rubberBand.setLeft(left);
  m_rubberBand.setTop(top);
  m_rubberBand.setRight(right);
  m_rubberBand.setBottom(bottom);
  //---------------------------------
 
  // reset zoom
  QWidget *prt = (QWidget*)parent();
  int frmHeight = prt->rect().height()-50;
  int frmWidth = prt->rect().width()-50;
  float rH = (float)frmHeight/(float)height;
  float rW = (float)frmWidth/(float)width;
  m_zoom = qMin(rH, rW);


  m_currSlice = m_maxSlice/2-1;

  if (m_volPtr)
    emit getSlice(m_currSlice);
}

void
CurvesWidget::updateTagColors()
{
  uchar *tagColors = Global::tagColors();

  m_tagColors.clear();
  m_tagColors.resize(256);

  m_tagColors[0] = qRgba(0,0,0,0);
  for(int i=1; i<256; i++)
    {
      uchar r = tagColors[4*i+0];
      uchar g = tagColors[4*i+1];
      uchar b = tagColors[4*i+2];

      uchar a = tagColors[4*i+3];
      if (a > 2)
	m_tagColors[i] = qRgba(r, g, b, 127);
      else
	m_tagColors[i] = qRgba(r, g, b, 0);
    }


  if (!m_volPtr)
    return;
  
  recolorImage();
  resizeImage();
  
  update();
}

void
CurvesWidget::setImage(uchar *slice, uchar *mask)
{
  m_livewire.resetPoly();

  if (m_sliceType == DSlice)
    {
      m_imgWidth = m_Height+1;
      m_imgHeight = m_Width+1;
    }
  else if (m_sliceType == WSlice)
    {
      m_imgWidth = m_Height+1;
      m_imgHeight = m_Depth+1;
    }
  else
    {
      m_imgWidth = m_Width+1;
      m_imgHeight = m_Depth+1;
    }

  memcpy(m_slice, slice, Global::bytesPerVoxel()*m_imgWidth*m_imgHeight);
  memcpy(m_maskslice, mask, m_imgWidth*m_imgHeight);

  recolorImage();

  resizeImage();
  
  update();

  qApp->processEvents();
  if (m_applyRecursive)
    {
      QKeyEvent dummy(QEvent::KeyPress,
		      m_key,
		      Qt::NoModifier);
      keyPressEvent(&dummy);
    }
}

void
CurvesWidget::setMaskImage(uchar *mask)
{
  memcpy(m_maskslice, mask, m_imgWidth*m_imgHeight);

  updateMaskImage();
  update();

  if (m_applyRecursive)
    repaint();
}

void
CurvesWidget::applyGradLimits()
{
  if (m_minGrad < 0.0001 && m_maxGrad > 0.999)
    return;

  ushort *volPtrUS = 0;
  if (Global::bytesPerVoxel() == 2)
    volPtrUS = (ushort*)m_volPtr;

  int D = m_Depth+1;
  int W = m_Width+1;
  int H = m_Height+1;
    
  int dstart = 0;
  int dend = m_Depth;
  int wstart = 0;
  int wend = m_Width;
  int hstart = 0;
  int hend = m_Height;

  if (m_sliceType == DSlice) { dstart = dend = m_currSlice; }
  if (m_sliceType == WSlice) { wstart = wend = m_currSlice; }
  if (m_sliceType == HSlice) { hstart = hend = m_currSlice; }

  int idx = 0;
  float gradMag;
  
  for(int d0=dstart; d0<=dend; d0++)
  for(int w0=wstart; w0<=wend; w0++)
  for(int h0=hstart; h0<=hend; h0++)
    {
      if (m_gradType == 0)
	{
	  float gx,gy,gz;
	  qint64 d3 = qBound(0, d0+1, m_Depth);
	  qint64 d4 = qBound(0, d0-1, m_Depth);
	  qint64 w3 = qBound(0, w0+1, m_Width);
	  qint64 w4 = qBound(0, w0-1, m_Width);
	  qint64 h3 = qBound(0, h0+1, m_Height);
	  qint64 h4 = qBound(0, h0-1, m_Height);
	  if (Global::bytesPerVoxel() == 1)
	    {
	      gz = (m_volPtr[d3*W*H + w0*H + h0] -
		    m_volPtr[d4*W*H + w0*H + h0]);
	      gy = (m_volPtr[d0*W*H + w3*H + h0] -
		    m_volPtr[d0*W*H + w4*H + h0]);
	      gx = (m_volPtr[d0*W*H + w0*H + h3] -
		    m_volPtr[d0*W*H + w0*H + h4]);
	      gx/=255.0;
	      gy/=255.0;
	      gz/=255.0;
	    }
	  else
	    {
	      gz = (volPtrUS[d3*W*H + w0*H + h0] -
		    volPtrUS[d4*W*H + w0*H + h0]);
	      gy = (volPtrUS[d0*W*H + w3*H + h0] -
		    volPtrUS[d0*W*H + w4*H + h0]);
	      gx = (volPtrUS[d0*W*H + w0*H + h3] -
		    volPtrUS[d0*W*H + w0*H + h4]);
	      gx/=65535.0;
	      gy/=65535.0;
	      gz/=65535.0;
	    }
	  
	  Vec dv = Vec(gx, gy, gz); // surface gradient
	  gradMag = dv.norm();
	} // gradType == 0

      if (m_gradType > 0)
	{
	  int sz = 1;
	  float divisor = 10.0;
	  if (m_gradType == 2)
	    {
	      sz = 2;
	      divisor = 70.0;
	    }
	  if (Global::bytesPerVoxel() == 1)
	    {
	      float vval = m_volPtr[d0*W*H + w0*H + h0];
	      float sum = 0;
	      for(int a=d0-sz; a<=d0+sz; a++)
	      for(int b=w0-sz; b<=w0+sz; b++)
	      for(int c=h0-sz; c<=h0+sz; c++)
		{
		  qint64 a0 = qBound(0, a, m_Depth);
		  qint64 b0 = qBound(0, b, m_Width);
		  qint64 c0 = qBound(0, c, m_Height);
		  sum += m_volPtr[a0*W*H + b0*H + c0];
		}

	      sum = (sum-vval)/divisor;
	      gradMag = fabs(sum-vval)/255.0;
	    }
	  else
	    {
	      float vval = volPtrUS[d0*W*H + w0*H + h0];
	      float sum = 0;
	      for(int a=d0-sz; a<=d0+sz; a++)
	      for(int b=w0-sz; b<=w0+sz; b++)
	      for(int c=h0-sz; c<=h0+sz; c++)
		{
		  qint64 a0 = qBound(0, a, m_Depth);
		  qint64 b0 = qBound(0, b, m_Width);
		  qint64 c0 = qBound(0, c, m_Height);
		  sum += volPtrUS[a0*W*H + b0*H + c0];
		}

	      sum = (sum-vval)/divisor;
	      gradMag = fabs(sum-vval)/65535.0;
	    }
	} // gradType == 1
      
      
      gradMag = qBound(0.0f, gradMag, 1.0f);
      if (gradMag < m_minGrad || gradMag > m_maxGrad)
	{
	  m_sliceImage[4*idx+0] = 0;
	  m_sliceImage[4*idx+1] = 0;
	  m_sliceImage[4*idx+2] = 0;
	  m_sliceImage[4*idx+3] = 0;
	}
      idx ++;
    }
}

void CurvesWidget::setGradThresholdType(int t)
{
  m_gradType = t;
  recolorImage();
  onlyImageScaled();
  update();
}

void CurvesWidget::setMinGrad(float g)
{
  m_minGrad = g;
  recolorImage();
  onlyImageScaled();
  update();
}
void
CurvesWidget::setMaxGrad(float g)
{
  m_maxGrad = g;
  recolorImage();
  onlyImageScaled();
  update();
}

void
CurvesWidget::onlyImageScaled()
{
  m_imageScaled = m_image.scaled(m_simgWidth,
				 m_simgHeight,
				 Qt::IgnoreAspectRatio,
				 Qt::SmoothTransformation);
}

void
CurvesWidget::recolorImage()
{
  uchar *tagColors = Global::tagColors();

  for(int i=0; i<m_imgHeight*m_imgWidth; i++)
    {
      int idx = m_slice[i];
      if (Global::bytesPerVoxel() == 2)
	idx = ((ushort*)m_slice)[i];

      m_sliceImage[4*i+0] = m_lut[4*idx+0];
      m_sliceImage[4*i+1] = m_lut[4*idx+1];
      m_sliceImage[4*i+2] = m_lut[4*idx+2];
      m_sliceImage[4*i+3] = m_lut[4*idx+3];

      if (tagColors[4*m_maskslice[i]+3] == 0)
	{
	  m_sliceImage[4*i+0] = 0;
	  m_sliceImage[4*i+1] = 0;
	  m_sliceImage[4*i+2] = 0;
	  m_sliceImage[4*i+3] = 0;
	}
    }

  applyGradLimits();
  
  //-----
  // transfer data for livewire calculation
  uchar *sliceData = new uchar[m_imgHeight*m_imgWidth];
  for(int i=0; i<m_imgHeight*m_imgWidth; i++)
    sliceData[i] = m_sliceImage[4*i];

  if (m_livewireMode)
    {
      m_livewire.setImageData(m_imgWidth, m_imgHeight, sliceData);
      delete [] sliceData;
      m_gradImageScaled = m_livewire.gradientImage().scaled(m_simgWidth,
							    m_simgHeight,
							    Qt::IgnoreAspectRatio,
							    Qt::FastTransformation);
    }
  //-----

  m_image = QImage(m_sliceImage,
		   m_imgWidth,
		   m_imgHeight,
		   QImage::Format_RGB32);
  
  updateMaskImage();
}

void
CurvesWidget::setRawValue(QList<int> vgt)
{
  m_vgt = vgt;
  update();
}

void
CurvesWidget::resizeImage()
{  
  setMinimumSize(QSize(m_zoom*m_imgWidth + 20,
		       m_zoom*m_imgHeight + 40));

  setMaximumSize(QSize(m_zoom*m_imgWidth + 20,
		       m_zoom*m_imgHeight + 40));


  int frmWidth = m_zoom*m_imgWidth;
  int frmHeight = m_zoom*m_imgHeight;

  float rH = (float)m_imgHeight/(float)frmHeight;
  float rW = (float)m_imgWidth/(float)frmWidth;

  m_simgHeight = m_imgHeight;
  m_simgWidth = m_imgWidth;
  //if (rH > 1 || rW > 1)
    {
      // scaledown the image
      if (rH > rW)
	{
	  m_simgHeight = m_imgHeight/rH;
	  m_simgWidth = m_imgWidth/rH;
	}
      else
	{
	  m_simgHeight = m_imgHeight/rW;
	  m_simgWidth = m_imgWidth/rW;
	}
    }

  m_imageScaled = m_image.scaled(m_simgWidth,
				 m_simgHeight,
				 Qt::IgnoreAspectRatio,
				 Qt::SmoothTransformation);
  
  m_maskimageScaled = m_maskimage.scaled(m_simgWidth,
					 m_simgHeight,
					 Qt::IgnoreAspectRatio,
					 Qt::FastTransformation);

  if (m_livewireMode)
    m_gradImageScaled = m_livewire.gradientImage().scaled(m_simgWidth,
							  m_simgHeight,
							  Qt::IgnoreAspectRatio,
							  Qt::FastTransformation);
}

void
CurvesWidget::resizeEvent(QResizeEvent *event)
{
  resizeImage();
  update();
}

void
CurvesWidget::drawSizeText(QPainter *p, int nc, int nmc)
{  
  p->setPen(QPen(Qt::black, 1));

  QString txt;
  txt += QString("%1 %2 %3  ").arg(m_pickDepth).arg(m_pickWidth).arg(m_pickHeight);

  txt += QString("dim:(%1 %2 %3)").	      \
    arg(m_Height+1).arg(m_Width+1).arg(m_Depth+1);

  int hsz = m_maxHSlice-m_minHSlice+1;
  int wsz = m_maxWSlice-m_minWSlice+1;
  int dsz = m_maxDSlice-m_minDSlice+1;

  if (hsz < m_Height+1 || wsz < m_Width+1 || dsz < m_Depth+1)      
    txt += QString("[%4:%5 %6:%7 %8:%9]  subvol:(%10 %11 %12)").      \
      arg(m_minHSlice).arg(m_maxHSlice).			      \
      arg(m_minWSlice).arg(m_maxWSlice).			      \
      arg(m_minDSlice).arg(m_maxDSlice).			      \
      arg(m_maxHSlice-m_minHSlice+1).				      \
      arg(m_maxWSlice-m_minWSlice+1).				      \
      arg(m_maxDSlice-m_minDSlice+1);				      \

  //txt += QString("  radius:%1").arg(Global::spread());
  txt += QString("  curves : %1 %2").arg(nc).arg(nmc);
  p->drawText(m_simgX, m_simgY-3, txt);
}

void
CurvesWidget::drawSeedPoints(QPainter *p,
			    QVector<QPointF> seeds,
			    QColor seedColor)
{
  drawPoints(p, seeds, Qt::black, 4, m_pointSize);
  drawPoints(p, seeds, seedColor, 2, m_pointSize);
}

void
CurvesWidget::drawPoints(QPainter *p,
			QVector<QPointF> seeds,
			QColor seedColor,
			int penSize,
			int pointSize)
{
  p->setPen(QPen(seedColor, penSize, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  for(int i=0; i<seeds.count(); i++)
    {
      int x = seeds[i].x() - pointSize/2;
      int y = seeds[i].y() - pointSize/2;
      p->drawArc(x,y,pointSize,pointSize, 0, 16*360);
    }
}

void
CurvesWidget::drawLivewire(QPainter *p)
{
  if (!m_livewireMode)
    return;

  QPoint move = QPoint(m_simgX, m_simgY);
  float sx = (float)m_simgWidth/(float)m_imgWidth;

  p->drawImage(m_simgX, m_simgY, m_gradImageScaled);

  QVector<QPointF> poly = m_livewire.poly();
  {
    p->setPen(QPen(Qt::red, 2));
    p->setBrush(Qt::transparent);
    if (poly.count() > 0)
      {
	for(int i=0; i<poly.count(); i++)
	  poly[i] = poly[i]*sx + move;
	p->drawPolyline(poly);
      }
  }

  {
    p->setPen(QPen(QColor(250,0,250), 2));
    QVector<QPointF> pts = m_livewire.livewire();
    if (pts.count() > 0)
      {
	for(int i=0; i<pts.count(); i++)
	  pts[i] = pts[i]*sx + move;
	p->drawPolyline(pts);
      }
  }

  {
    QVector<int> seedpos = m_livewire.seedpos();
    if (seedpos.count() > 0)
      {
	QVector<QPointF> seeds;
	for(int i=0; i<seedpos.count(); i++)
	  seeds << poly[seedpos[i]];

	drawSeedPoints(p, seeds, Qt::lightGray);
      }
  }
}

void
CurvesWidget::drawPoints(QPainter *p,
			QVector<QVector4D> seeds,
			QColor defaultColor,
			int wd)
{
  float sx = (float)m_simgWidth/(float)m_imgWidth;

  for(int i=0; i<seeds.count(); i++)
    {
      int pointSize = qMax(1, (int)(sx*seeds[i].w())) + wd;
      int tag = seeds[i].z();
      uchar r = Global::tagColors()[4*tag+0];
      uchar g = Global::tagColors()[4*tag+1];
      uchar b = Global::tagColors()[4*tag+2];
      if (defaultColor == Qt::black)
	{
	  p->setPen(QPen(QColor(r,g,b), 1));
	  p->setBrush(QColor(r,g,b));
	}
      else
	{
	  p->setPen(QPen(defaultColor, 1));
	  p->setBrush(defaultColor);
	}
      int x = seeds[i].x() - pointSize/2;
      int y = seeds[i].y() - pointSize/2;
      p->drawEllipse(x,y,pointSize,pointSize);
    }
}

int
CurvesWidget::drawCurves(QPainter *p)
{  
  QList<Curve*> curves;
  curves = m_Curves.getCurvesAt(m_currSlice, true);
  
  QPoint move = QPoint(m_simgX, m_simgY);
  float sx = (float)m_simgWidth/(float)m_imgWidth;
  for(int l=0; l<curves.count(); l++)
    {
      int tag = curves[l]->tag;
      //uchar r = Global::tagColors()[4*tag+0];
      //uchar g = Global::tagColors()[4*tag+1];
      //uchar b = Global::tagColors()[4*tag+2];
      uchar r = 20;
      uchar g = 220;
      uchar b = 150;
	  

      QVector<QPointF> pts = curves[l]->pts;
      for(int i=0; i<pts.count(); i++)
	pts[i] = pts[i]*sx + move;

      if (curves[l]->selected)
	{
	  p->setBrush(Qt::transparent);
	  if (curves[l]->closed)
	    {
	      p->setPen(QPen(QColor(250,250,250,250), 5));
	      p->drawPolygon(pts);
	    }
	  else
	    {
	      p->setPen(QPen(QColor(250,250,250,250), curves[l]->thickness+4,
			     Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
	      p->drawPolyline(pts);
	    }
	}

      bool onlyline = false;
	  
      if (curves[l]->closed && !onlyline)
	{
	  p->setPen(QPen(QColor(r,g,b), 2));
	  p->setBrush(QColor(r, g, b, 70));
	  p->drawPolygon(pts);
	}
      else
	{
	  p->setPen(QPen(QColor(r,g,b), curves[l]->thickness,
			 Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
	  p->drawPolyline(pts);
	}
      
      {
	QVector<int> seedpos = curves[l]->seedpos;
	if (seedpos.count() > 0)
	  {
	    QVector<QPointF> seeds;
	    for(int i=0; i<seedpos.count(); i++)
	      seeds << pts[seedpos[i]];
	    
	    drawSeedPoints(p, seeds, Qt::lightGray);
	  }
      }
    }

  return curves.count();
}

void CurvesWidget::setPointSize(int p) { m_pointSize = p; }

QList<QPointF>
CurvesWidget::trimPointList(QList<QPointF> pl, bool switchcoord)
{
  QList<QPointF> npl;

  if (pl.count() > 1)
    {
      QMultiMap<int, int> ptlines;

      for(int i=0; i<pl.count(); i++)
	{
	  int x = pl[i].x();
	  int y = pl[i].y();
	  ptlines.insert(x, y);
	}
      QList<int> xs = ptlines.uniqueKeys();
      for(int k=0; k<xs.count(); k++)
	{
	  QList<int> ys = ptlines.values(xs[k]);
	  if (ys.count() > 1)
	    {
	      std::sort(ys.begin(), ys.end());
	      npl << QPointF(xs[k], ys[0]);
	      npl << QPointF(xs[k], ys[ys.count()-1]);
	    }
	}
    }  
  

  if (switchcoord)
    { // switch coordinates
      for(int i=0; i<npl.count(); i++)
	{
	  QPointF xy = npl[i];
	  npl[i] = QPointF(xy.y(), xy.x());
	}
    }  

  return npl;

}

void
CurvesWidget::drawOtherCurvePoints(QPainter *p)
{  
  QList<Curve*> curves;
  QPoint move = QPoint(m_simgX, m_simgY);
  float sx = (float)m_simgWidth/(float)m_imgWidth;
  QVector<QPointF> vptd, vptw, vpth;

  if (m_sliceType == DSlice)
    {
      QList<QPointF> ptw = m_wCurves->ypoints(m_currSlice);
      vptw = QVector<QPointF>::fromList(trimPointList(ptw, true));

      QList<QPointF> pth = m_hCurves->ypoints(m_currSlice);
      vpth = QVector<QPointF>::fromList(trimPointList(pth, false));
    }
  else if (m_sliceType == WSlice)
    {
      QList<QPointF> pth = m_hCurves->xpoints(m_currSlice);
      vpth = QVector<QPointF>::fromList(trimPointList(pth, false));

      QList<QPointF> ptd = m_dCurves->ypoints(m_currSlice);
      vptd = QVector<QPointF>::fromList(trimPointList(ptd, true));
    }    
  else
    {
      QList<QPointF> ptw = m_wCurves->xpoints(m_currSlice);
      vptw = QVector<QPointF>::fromList(trimPointList(ptw, false));

      QList<QPointF> ptd = m_dCurves->xpoints(m_currSlice);
      vptd = QVector<QPointF>::fromList(trimPointList(ptd, true));
    }

  if (vptd.count() > 0)
    {
      for(int l=0; l<vptd.count(); l++)
	vptd[l] = vptd[l]*sx + move;

      p->setPen(QPen(QColor(255,127,80), 1));
      for(int l=0; l<vptd.count()/2; l++)	  
	p->drawLine(vptd[2*l].x(),   vptd[2*l].y(),
		    vptd[2*l+1].x(), vptd[2*l+1].y());
    }

  if (vptw.count() > 0)
    {
      for(int l=0; l<vptw.count(); l++)
	vptw[l] = vptw[l]*sx + move;

      p->setPen(QPen(QColor(218,112,214), 1));
      for(int l=0; l<vptw.count()/2; l++)	  
	p->drawLine(vptw[2*l].x(),   vptw[2*l].y(),
		    vptw[2*l+1].x(), vptw[2*l+1].y());
    }

  if (vpth.count() > 0)
    {
      for(int l=0; l<vpth.count(); l++)
	vpth[l] = vpth[l]*sx + move;

      p->setPen(QPen(QColor(70,130,180), 1));
      for(int l=0; l<vpth.count()/2; l++)	  
	p->drawLine(vpth[2*l].x(),   vpth[2*l].y(),
		    vpth[2*l+1].x(), vpth[2*l+1].y());
    }
}

int
CurvesWidget::drawMorphedCurves(QPainter *p)
{  
  QList<Curve> curves;
  curves = m_Curves.getMorphedCurvesAt(m_currSlice);

  QPoint move = QPoint(m_simgX, m_simgY);
  float sx = (float)m_simgWidth/(float)m_imgWidth;
  for(int l=0; l<curves.count(); l++)
    {
      int tag = curves[l].tag;
      //uchar r = Global::tagColors()[4*tag+0];
      //uchar g = Global::tagColors()[4*tag+1];
      //uchar b = Global::tagColors()[4*tag+2];
      uchar r = 120;
      uchar g = 200;
      uchar b = 150;
      

      QVector<QPointF> pts = curves[l].pts;
      for(int i=0; i<pts.count(); i++)
	pts[i] = pts[i]*sx + move;
      
      if (curves[l].closed)
	{
	  p->setPen(QPen(QColor(r,g,b), 1));
	  p->setBrush(QColor(r, g, b, 70));
	  p->drawPolygon(pts);
	}
      else
	{
	  p->setPen(QPen(QColor(r,g,b), curves[l].thickness,
			 Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
	  p->drawPolyline(pts);
	}
    }

  return curves.count();
}

void
CurvesWidget::paintEvent(QPaintEvent *event)
{  
  QPainter p(this);

  int shiftModifier = QGuiApplication::keyboardModifiers() & Qt::ShiftModifier;
  int ctrlModifier = QGuiApplication::keyboardModifiers() & Qt::ControlModifier;

  p.setRenderHint(QPainter::Antialiasing);
  p.setCompositionMode(QPainter::CompositionMode_SourceOver);

  
  p.drawImage(m_simgX, m_simgY, m_imageScaled);
  p.drawImage(m_simgX, m_simgY, m_maskimageScaled);
  
  //----------------------------------------------------
  // draw slice positions for other 2 slice directions
  if (m_showPosition)
    {
      p.setPen(QPen(Qt::cyan, 0.7));
      p.drawLine(m_simgX+m_hline*m_zoom, m_simgY+0,
		 m_simgX+m_hline*m_zoom, m_simgY+m_simgHeight);
      p.drawLine(m_simgX+0, m_simgY+m_vline*m_zoom,
		 m_simgX+m_simgWidth, m_simgY+m_vline*m_zoom);
    }
  //----------------------------------------------------

  drawRubberBand(&p);
  

  int nc = drawCurves(&p);
  int nmc = drawMorphedCurves(&p);

  //drawSizeText(&p, nc, nmc);
  
  drawLivewire(&p);
      
  if (!m_livewire.seedMoveMode())
    drawOtherCurvePoints(&p);


  if (m_pickPoint)
    drawRawValue(&p);

  if (!m_rubberBandActive &&
      !m_livewireMode &&
      !m_curveMode)
    {
      float xpos = m_cursorPos.x();
      float ypos = m_cursorPos.y();
      if (validPickPoint(xpos, ypos))
	{
	  bool ok = false;
	  if (m_sliceType == DSlice)
	    ok = withinBounds(m_pickHeight,
			      m_pickWidth);
	  else if (m_sliceType == WSlice)
	    ok = withinBounds(m_pickHeight,
			      m_pickDepth);
	  else
	    ok = withinBounds(m_pickWidth,
			      m_pickDepth);

	  int rad = Global::spread()*(float)m_simgWidth/(float)m_imgWidth;
	  p.setPen(Qt::white);
	  p.setBrush(QColor(150, 0, 0, 150));
	  p.drawEllipse(xpos-rad,
			ypos-rad,
			2*rad, 2*rad);
	}
    }

  //if (hasFocus())
  if (m_hasFocus)
    {
      p.setPen(QPen(QColor(50, 250, 100), 10));
      p.setBrush(Qt::transparent);
      p.drawRect(rect());
    }
}

void
CurvesWidget::drawRubberBand(QPainter *p)
{
  int x = m_simgX + m_simgWidth*m_rubberBand.left();
  int y = m_simgY + m_simgHeight*m_rubberBand.top();
  int width = m_simgWidth*m_rubberBand.width();
  int height = m_simgHeight*m_rubberBand.height();

  p->setBrush(Qt::transparent);

  p->setPen(QPen(Qt::white, 1));
  p->drawRect(x, y, width, height);


  //-------------------------------------------------
  // slightly blacken region outside the bounding box
  p->setPen(Qt::transparent);
  p->setBrush(QColor(0, 0, 0, 100));

  int bottom = m_simgHeight*m_rubberBand.bottom();
  int right = m_simgWidth*m_rubberBand.right();
      
  p->drawRect(m_simgX, m_simgY, m_simgWidth, y-m_simgY);
  p->drawRect(m_simgX, m_simgY+bottom,
	      m_simgWidth, m_simgHeight-bottom);

  height = height + (m_simgY+bottom - (y+height));
  p->drawRect(m_simgX, y, x-m_simgX, height);
  p->drawRect(m_simgX+right, y,
	      m_simgWidth-right, height);
  //-------------------------------------------------


  if (m_rubberBandActive)
    {
      int x0,x1,y0,y1;
      if (m_rubberXmin)
	{
	  x1 = x0 = x;
	  y0 = y;
	  y1 = y+height;
	}
      else if (m_rubberXmax)
	{
	  x1 = x0 = x+width;
	  y0 = y;
	  y1 = y+height;
	}
      else if (m_rubberYmin)
	{
	  x0 = x;
	  y1 = y0 = y;
	  x1 = x+width;
	}
      else if (m_rubberYmax)
	{
	  x0 = x;
	  y1 = y0 = y+height;
	  x1 = x+width;
	}
      p->setPen(QPen(Qt::white, 3));
      p->drawLine(x0, y0, x1, y1);
      p->setPen(QPen(Qt::black, 1));
      p->drawLine(x0, y0, x1, y1);
    }
}

void
CurvesWidget::drawRawValue(QPainter *p)
{
  if (m_vgt.count() < 3)
    return;

  QString str;
  int xp = m_cursorPos.x();
  int yp = m_cursorPos.y();

  str = QString("%1 %2 %3 :").\
          arg(m_pickHeight).\
          arg(m_pickWidth).\
          arg(m_pickDepth);
    
  str += QString(" val(%1) tag(%2)").\
	     arg(m_vgt[0]).\
	     arg(m_vgt[2]);

  QFont pfont = QFont("Helvetica", 10);
  QPainterPath pp;
  pp.addText(QPointF(0,0), 
	     pfont,
	     str);
  QRectF br = pp.boundingRect();
  float by = br.height()/2;      
  float bw = br.width();      

  int x = xp-bw-30;
  int xe = xp-20;
  if (x < 1)
    {
      x = xp+20;
      xe = x;
    }

  p->setPen(Qt::darkRed);
  p->setBrush(QColor(0,0,0,200));
  p->drawRect(x, yp-by,
	      bw+10, 2*by+5);
  p->setPen(Qt::white);
  p->drawText(x+5, yp+by,
	      str);
  p->setPen(Qt::black);
  p->drawLine(xe, yp, xp, yp);  
}

void
CurvesWidget::applyRecursive(int key)
{
  QStringList dtypes;
  dtypes << "Forward"
	 << "Backward";

  bool ok;
  QString option = QInputDialog::getItem(0,
					 "Slice Direction",
					 "Apply operation in which direction ?",
					 dtypes,
					 0,
					 false,
					 &ok);

  if (!ok)
    return;

  m_forward = true;
  if (option == "Backward")
    m_forward = false;

  m_applyRecursive = true;
  m_key = key;
  m_cslc = 0;
  m_maxslc = 10;

  if (m_sliceType == DSlice)
    {
      int maxD = m_maxDSlice;
      int minD = m_minDSlice;
      if (m_forward)
	m_maxslc = maxD - m_currSlice + 1;
      else
	m_maxslc = m_currSlice - minD + 1;
    }
  else if (m_sliceType == WSlice)
    {
      int maxW = m_maxWSlice;
      int minW = m_minWSlice;
      if (m_forward)
	m_maxslc = maxW - m_currSlice + 1;
      else
	m_maxslc = m_currSlice - minW + 1;
    }
  else if (m_sliceType == HSlice)
    {
      int maxH = m_maxHSlice;
      int minH = m_minHSlice;
      if (m_forward)
	m_maxslc = maxH - m_currSlice + 1;
      else
	m_maxslc = m_currSlice - minH + 1;
    }
}

void
CurvesWidget::checkRecursive()
{
  if (m_applyRecursive)
    {
      m_cslc++;
      if (m_cslc >= m_maxslc)
	{
	  m_applyRecursive = false;
	  emit saveWork();
	  QMessageBox::information(0, "", "Reached the end");
	}
      else
	{
	  if (m_forward)
	    doAnother(1);
	  else
	    doAnother(-1);
	}
    }
}

void CurvesWidget::setCurve(bool b)
{
  if (!m_volPtr)
    return;
    
  m_curveMode = b;
  if (!m_curveMode) m_livewireMode = false;
  updateTagColors();
}

void
CurvesWidget::setLivewire(bool b)
{
  if (!m_volPtr)
    return;
    
  m_livewireMode = b;

  recolorImage();
  update();
}

void
CurvesWidget::freezeLivewire(bool select)
{
  if (!m_livewireMode)
    {
      //QMessageBox::information(0, "Error", "No livewire found to be transferred to curve");
      return;
    }

  if (Global::closed())
    m_livewire.freeze();


  QVector<QPointF> pts = m_livewire.poly();
  QVector<int> seedpos = m_livewire.seedpos();
  QVector<QPointF> seeds = m_livewire.seeds();

  if (pts.count() < 5)
    return;

  m_Curves.deselectAll();

  m_Curves.setPolygonAt(m_currSlice,
			pts, seedpos,
			Global::closed(),
			Global::tag(),
			Global::thickness(),
			select, 0,
			seeds); 
  emit polygonLevels(m_Curves.polygonLevels());

  m_livewire.resetPoly();

//
//
//
//
// show it via the Show Tags parameter").arg(Global::tag()));
  
  update(); 
}

void
CurvesWidget::newPolygon(bool smooth, bool line)
{
  if (m_sliceType == DSlice)
    m_Curves.newPolygon(m_currSlice,
			(m_minHSlice+m_maxHSlice)/2,
			(m_minWSlice+m_maxWSlice)/2,
			smooth, line);
  else if (m_sliceType == WSlice)
    m_Curves.newPolygon(m_currSlice,
			(m_minHSlice+m_maxHSlice)/2,
			(m_minDSlice+m_maxDSlice)/2,
			smooth, line);
  else
    m_Curves.newPolygon(m_currSlice,
			(m_minWSlice+m_maxWSlice)/2,
			(m_minDSlice+m_maxDSlice)/2,
			smooth, line);

  emit polygonLevels(m_Curves.polygonLevels());

  update();
}

void
CurvesWidget::newEllipse()
{
  if (m_sliceType == DSlice)
    m_Curves.newEllipse(m_currSlice,
			(m_minHSlice+m_maxHSlice)/2,
			(m_minWSlice+m_maxWSlice)/2);
  else if (m_sliceType == WSlice)
    m_Curves.newEllipse(m_currSlice,
			(m_minHSlice+m_maxHSlice)/2,
			(m_minDSlice+m_maxDSlice)/2);
  else
    m_Curves.newEllipse(m_currSlice,
			(m_minWSlice+m_maxWSlice)/2,
			(m_minDSlice+m_maxDSlice)/2);

  emit polygonLevels(m_Curves.polygonLevels());

  update();
}

void
CurvesWidget::newCurve(bool showoptions)
{
  if (showoptions)
    {
      QStringList items;
      items << "Curve";
      items << "Ellipse";
      items << "SPolygon";
      items << "Polygon";
      items << "SPolyline";
      items << "Polyline";
      bool ok;
      QString item = QInputDialog::getItem(this, "Add Element",
					   "New Element", items, 0, false, &ok);
      int type = 0;
      if (ok && !item.isEmpty())
	{
	  if (item == "Ellipse")
	    {
	      newEllipse();
	      QMessageBox::information(this, "Add Ellipse",
				       "Move points to suit your needs");
	      return;
	    }
	  else if (item == "SPolygon")
	    {
	      newPolygon(true, false);
	      QMessageBox::information(this, "Add Smooth Polygon",
				       "Add/remove/move points to suit your needs");
	      return;
	    }      
	  else if (item == "Polygon")
	    {
	      newPolygon(false, false);
	      QMessageBox::information(this, "Add Polygon",
				       "Add/remove/move points to suit your needs");
	      return;
	    }      
	  else if (item == "SPolyline")
	    {
	      newPolygon(true, true);
	      QMessageBox::information(this, "Add Smooth Polyline",
				       "Add/remove/move points to suit your needs");
	      return;
	    }      
	  else if (item == "Polyline")
	    {
	      newPolygon(false, true);
	      QMessageBox::information(this, "Add Polyline",
				       "Add/remove/move points to suit your needs");
	      return;
	    }      
	  
	  QMessageBox::information(this, "Add Curve",
				   "Draw curve by dragging with left mouse button pressed.\nYou can also start new curve by pressing key c.");
	}
    }

  m_Curves.newCurve(m_currSlice, Global::closed());

  m_addingCurvePoints = true;
  emit showEndCurve();

  emit polygonLevels(m_Curves.polygonLevels());
}

void
CurvesWidget::endCurve()
{
  m_addingCurvePoints = false;
  emit hideEndCurve();
  emit saveWork();
}

void
CurvesWidget::deselectAll()
{
  m_Curves.deselectAll();
  update();
}

void
CurvesWidget::morphCurves()
{
  int minS, maxS;
  if (m_sliceType == DSlice)
    { 
      minS = m_minDSlice; 
      maxS = m_maxDSlice; 
    } 
  else if (m_sliceType == WSlice) 
    { 
      minS = m_minWSlice; 
      maxS = m_maxWSlice; 
    } 
  else 
    { 
      minS = m_minHSlice; 
      maxS = m_maxHSlice; 
    }  

  m_Curves.morphCurves(minS, maxS);
  
  update();
}

void
CurvesWidget::morphSlices()
{
  int minS, maxS;
  if (m_sliceType == DSlice)
    { 
      minS = m_minDSlice; 
      maxS = m_maxDSlice; 
    } 
  else if (m_sliceType == WSlice) 
    { 
      minS = m_minWSlice; 
      maxS = m_maxWSlice; 
    } 
  else 
    { 
      minS = m_minHSlice; 
      maxS = m_maxHSlice; 
    }  

  m_Curves.morphSlices(minS, maxS);
  
  update();
}

void
CurvesWidget::deleteAllCurves()
{
  QString st;
  if (m_sliceType == DSlice) st = "Z";
  else if (m_sliceType == WSlice) st = "Y";
  else st = "X";  
					
  m_Curves.reset();
  emit polygonLevels(m_Curves.polygonLevels());

  emit saveWork();
  QMessageBox::information(0, "", QString("Removed all %1-curves").arg(st));
  
  update();
}


void
CurvesWidget::setSliceLOD(int lod)
{
  m_livewire.setLod(lod);
  m_gradImageScaled = m_livewire.gradientImage().scaled(m_simgWidth,
							m_simgHeight);
  update();
}



void
CurvesWidget::curveModeKeyPressEvent(QKeyEvent *event)
{
//  if (m_applyRecursive && event->key() == Qt::Key_Escape)
//    {
//      m_applyRecursive = false;
//      return;
//    }

  {
    QPoint pp = mapFromGlobal(QCursor::pos());
    float ypos = pp.y();
    float xpos = pp.x();
    validPickPoint(xpos, ypos);
  }

  int shiftModifier = event->modifiers() & Qt::ShiftModifier;
  int ctrlModifier = event->modifiers() & Qt::ControlModifier;


  if (m_livewireMode)
    {
      if (m_livewire.keyPressEvent(event))
	{
	  update();
	  return;
	}

      if (event->key() == Qt::Key_Space)
	{
	  if (m_livewire.poly().count() > 5)
	    {
	      freezeLivewire(false);
	      emit saveWork();
	      return;
	    }
	}
    }

  if (event->key() == Qt::Key_C)
    {
      if (!ctrlModifier)
	{
	  if (m_addingCurvePoints)
	    update();
	  newCurve(false);
	  //emit saveWork();
	  return;
	}    
      else if (ctrlModifier)	
	{
	  int cc = -1;
	  if (m_sliceType == DSlice)
	    cc = m_Curves.copyCurve(m_currSlice, m_pickHeight, m_pickWidth);
	  else if (m_sliceType == WSlice)
	    cc = m_Curves.copyCurve(m_currSlice, m_pickHeight, m_pickDepth);
	  else
	    cc = m_Curves.copyCurve(m_currSlice, m_pickWidth,  m_pickDepth);
	  
	  if (cc >= 0)
	    QMessageBox::information(0, "", QString("Curve copied to buffer"));
	  else
	    QMessageBox::information(0, "", QString("No curve found to copy"));
	  
	  return;
	}
    }

  if (ctrlModifier && event->key() == Qt::Key_V)
    {
      m_Curves.pasteCurve(m_currSlice);
      emit polygonLevels(m_Curves.polygonLevels());      
      update();
      return;
    }

//  if (shiftModifier && event->key() == Qt::Key_Q)
//    {
//      morphCurves();
//      return;
//    }
//
//  if (event->key() == Qt::Key_I)
//    {
//      int ic = -1;
//      if (m_sliceType == DSlice)
//	ic = m_Curves.showPolygonInfo(0, m_currSlice, m_pickHeight, m_pickWidth);
//      else if (m_sliceType == WSlice)
//	ic = m_Curves.showPolygonInfo(1, m_currSlice, m_pickHeight, m_pickDepth);
//      else
//	ic = m_Curves.showPolygonInfo(2, m_currSlice, m_pickWidth,  m_pickDepth);
//      
//      return;
//    }

  if (event->key() == Qt::Key_G) // shrink wrap 
    {
      bool ar = m_applyRecursive;

      if (shiftModifier)
	{
	  ar = true;

	  applyRecursive(event->key());
	  startShrinkwrap();
	}
      else if (!ar) startShrinkwrap();
      
      shrinkwrapCurve();

      if (m_applyRecursive == false)
	{
	  endShrinkwrap();
	  emit saveWork();
	}


      update();
      return;
    }

  if (event->key() == Qt::Key_S)
    { // curve smoothing
      if (m_sliceType == DSlice)
	m_Curves.smooth(m_currSlice, m_pickHeight, m_pickWidth,
			shiftModifier,
			m_minDSlice, m_maxDSlice);
      else if (m_sliceType == WSlice)
	m_Curves.smooth(m_currSlice, m_pickHeight, m_pickDepth,
			shiftModifier,
			m_minWSlice, m_maxWSlice);
      else
	m_Curves.smooth(m_currSlice, m_pickWidth,  m_pickDepth,
			shiftModifier,
			m_minHSlice, m_maxHSlice);
      
      emit saveWork();
      update();
      return;
    }

  if (event->key() == Qt::Key_D)
    { // curve dilation
      if (m_sliceType == DSlice)
	m_Curves.dilateErode(m_currSlice, m_pickHeight, m_pickWidth,
			     shiftModifier,
			     m_minDSlice, m_maxDSlice, 0.5);
      else if (m_sliceType == WSlice)
	m_Curves.dilateErode(m_currSlice, m_pickHeight, m_pickDepth,
			     shiftModifier,
			     m_minWSlice, m_maxWSlice, 0.5);
      else
	m_Curves.dilateErode(m_currSlice, m_pickWidth,  m_pickDepth,
			     shiftModifier,
			     m_minHSlice, m_maxHSlice, 0.5);
      
      emit saveWork();
      update();
      return;
    }

  if (event->key() == Qt::Key_E)
    { // curve erosion
      if (m_sliceType == DSlice)
	m_Curves.dilateErode(m_currSlice, m_pickHeight, m_pickWidth,
			     shiftModifier,
			     m_minDSlice, m_maxDSlice, -0.5);
      else if (m_sliceType == WSlice)
	m_Curves.dilateErode(m_currSlice, m_pickHeight, m_pickDepth,
			     shiftModifier,
			     m_minWSlice, m_maxWSlice, -0.5);
      else
	m_Curves.dilateErode(m_currSlice, m_pickWidth,  m_pickDepth,
			     shiftModifier,
			     m_minHSlice, m_maxHSlice, -0.5);
      
      emit saveWork();
      update();
      return;
    }

  if (event->key() == Qt::Key_F)
    {
      if (m_sliceType == DSlice)
	m_Curves.flipPolygon(m_currSlice, m_pickHeight, m_pickWidth);
      else if (m_sliceType == WSlice)
	m_Curves.flipPolygon(m_currSlice, m_pickHeight, m_pickDepth);
      else
	m_Curves.flipPolygon(m_currSlice, m_pickWidth,  m_pickDepth);
      update();
      return;
    }

  if (event->key() == Qt::Key_Delete ||
      event->key() == Qt::Key_Backspace)
    {
      if (m_sliceType == DSlice)
	{
	  m_Curves.removePolygonAt(m_currSlice, m_pickHeight, m_pickWidth, shiftModifier);
	  emit polygonLevels(m_Curves.polygonLevels());
	}
      else if (m_sliceType == WSlice)
	{
	  m_Curves.removePolygonAt(m_currSlice, m_pickHeight, m_pickDepth, shiftModifier);
	  emit polygonLevels(m_Curves.polygonLevels());
	}
      else
	{
	  m_Curves.removePolygonAt(m_currSlice, m_pickWidth,  m_pickDepth, shiftModifier);
	  emit polygonLevels(m_Curves.polygonLevels());
	}

      if (m_addingCurvePoints)
	endCurve();

      update();
      return;
    }
}

void CurvesWidget::zoom0Clicked() { setZoom(1); }
void CurvesWidget::zoom9Clicked() { setZoom(-1); }
void CurvesWidget::zoomUpClicked() { setZoom(m_zoom+0.1); }
void CurvesWidget::zoomDownClicked() { setZoom(m_zoom-0.1); }

void
CurvesWidget::keyPressEvent(QKeyEvent *event)
{
  bool processed = false;
  
  curveModeKeyPressEvent(event);
}

bool
CurvesWidget::checkRubberBand(int xpos, int ypos)
{
  m_rubberXmin = false;
  m_rubberYmin = false;
  m_rubberXmax = false;
  m_rubberYmax = false;
  m_rubberNew = false;
  m_rubberBandActive = false;

  if (xpos >= m_simgX-2 && xpos <= m_simgX+m_simgWidth+2 &&
      ypos >= m_simgY-2 && ypos <= m_simgY+m_simgHeight+2)
    {
      int rxmin = m_simgX + m_simgWidth*m_rubberBand.left();
      int rxmax = m_simgX + m_simgWidth*m_rubberBand.right();
      int rymin = m_simgY + m_simgHeight*m_rubberBand.top();
      int rymax = m_simgY + m_simgHeight*m_rubberBand.bottom();
      
      if (qAbs(xpos-rxmin) < 10)
	m_rubberXmin = true;
      else if (qAbs(xpos-rxmax) < 10)
	m_rubberXmax = true;
      else if (qAbs(ypos-rymin) < 10)
	m_rubberYmin = true;
      else if (qAbs(ypos-rymax) < 10)
	m_rubberYmax = true;
      else
	{	  
	  m_rubberNew = true;
	  
	  float frcX = (float)(xpos-m_simgX)/(float)m_simgWidth;
	  float frcY = (float)(ypos-m_simgY)/(float)m_simgHeight;	  
	  frcX = qBound(0.0f, frcX, 1.0f);
	  frcY = qBound(0.0f, frcY, 1.0f);
	  float wd = 1.0/m_imgWidth;
	  float ht = 1.0/m_imgHeight;
	  m_rubberBand = QRectF(frcX,frcY,wd,ht);
	}
    }

  m_rubberBandActive =   ( m_rubberXmin |
			   m_rubberYmin |
			   m_rubberXmax |
			   m_rubberYmax |
			   m_rubberNew );

  return m_rubberBandActive;
}

void
CurvesWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
  m_rubberBand.setLeft(0);
  m_rubberBand.setTop(0);
  m_rubberBand.setRight(1.0);
  m_rubberBand.setBottom(1.0);

  // --- now update the min and max limits
  float left = m_rubberBand.left();
  float right = m_rubberBand.right();
  float top = m_rubberBand.top();
  float bottom = m_rubberBand.bottom();
  if (m_sliceType == DSlice)
    {
      m_minHSlice = left*m_Height;
      m_maxHSlice = right*m_Height;
      m_minWSlice = top*m_Width;
      m_maxWSlice = bottom*m_Width;
    }
  else if (m_sliceType == WSlice)
    {
      m_minHSlice = left*m_Height;
      m_maxHSlice = right*m_Height;
      m_minDSlice = top*m_Depth;
      m_maxDSlice = bottom*m_Depth;
    }
  else
    {
      m_minWSlice = left*m_Width;
      m_maxWSlice = right*m_Width;
      m_minDSlice = top*m_Depth;
      m_maxDSlice = bottom*m_Depth;
    }

  emit updateViewerBox(m_minDSlice, m_maxDSlice,
		       m_minWSlice, m_maxWSlice,
		       m_minHSlice, m_maxHSlice);

  update();
}

void
CurvesWidget::curveMousePressEvent(QMouseEvent *event)
{
  QPoint pp = event->pos();
  float ypos = pp.y();
  float xpos = pp.x();
  
  m_cursorPos = pp;
  m_pickPoint = false;

  if (!validPickPoint(xpos, ypos))
    return;
  

  bool shiftModifier = event->modifiers() & Qt::ShiftModifier;
  bool ctrlModifier = event->modifiers() & Qt::ControlModifier;
  bool altModifier = event->modifiers() & Qt::AltModifier;

  m_lastPickDepth = m_pickDepth;
  m_lastPickWidth = m_pickWidth;
  m_lastPickHeight= m_pickHeight;
  

  if(m_livewireMode &&
     !ctrlModifier &&
     !altModifier)
    {      
      if (m_sliceType == DSlice)
	m_livewire.mousePressEvent(m_pickHeight, m_pickWidth, event);
      else if (m_sliceType == WSlice)
	m_livewire.mousePressEvent(m_pickHeight, m_pickDepth, event);
      else
	m_livewire.mousePressEvent(m_pickWidth, m_pickDepth, event);

      update();
      return;
    }
  
  if (m_button == Qt::LeftButton)
    {
      if (altModifier)
	{
	  checkRubberBand(xpos, ypos);
	  return;
	}

      // carry on only if Alt key is not pressed
      if(!m_livewireMode)
	{
	  if (m_addingCurvePoints) // curveMode
	    {
	      if (m_sliceType == DSlice)
		m_Curves.addPoint(m_currSlice, m_pickHeight, m_pickWidth);
	      else if (m_sliceType == WSlice)
		m_Curves.addPoint(m_currSlice, m_pickHeight, m_pickDepth);
	      else
		m_Curves.addPoint(m_currSlice, m_pickWidth,  m_pickDepth);
	    }
//	  else if (shiftModifier)
//	    {
//	      if (m_sliceType == DSlice)
//		{
//		  int a = m_pickHeight-m_lastPickHeight;
//		  int b = m_pickWidth-m_lastPickWidth;
//		  int rad = qBound(Global::selectionPrecision(),
//				   (int)qSqrt(a*a + b*b), 100);
//		  m_dCurves.startPush(m_currSlice, m_pickHeight, m_pickWidth, rad);
//		}
//	      else if (m_sliceType == WSlice)
//		{
//		  int a = m_pickHeight-m_lastPickHeight;
//		  int b = m_pickDepth-m_lastPickDepth;
//		  int rad = qBound(Global::selectionPrecision(),
//				   (int)qSqrt(a*a + b*b), 100);
//		  m_wCurves.startPush(m_currSlice, m_pickHeight, m_pickDepth, rad);
//		}
//	      else
//		{
//		  int a = m_pickWidth-m_lastPickWidth;
//		  int b = m_pickDepth-m_lastPickDepth;
//		  int rad = qBound(Global::selectionPrecision(),
//				   (int)qSqrt(a*a + b*b), 100);
//		  m_hCurves.startPush(m_currSlice, m_pickWidth,  m_pickDepth, rad);
//		}
//	      update();
//	      return;
//	    }
	}
    }
  else if (m_button == Qt::MiddleButton &&
	   (!m_livewireMode ||
	    !m_livewire.seedMoveMode()))
    {
      if (m_sliceType == DSlice)
	m_Curves.setMoveCurve(m_currSlice, m_pickHeight, m_pickWidth);
      else if (m_sliceType == WSlice)
	m_Curves.setMoveCurve(m_currSlice, m_pickHeight, m_pickDepth);
      else
	m_Curves.setMoveCurve(m_currSlice, m_pickWidth,  m_pickDepth);
    }
  else if (m_button == Qt::RightButton &&
	   !m_livewireMode)
    {
      if (m_sliceType == DSlice)
	m_Curves.removePoint(m_currSlice, m_pickHeight, m_pickWidth);
      else if (m_sliceType == WSlice)
	m_Curves.removePoint(m_currSlice, m_pickHeight, m_pickDepth);
      else
	m_Curves.removePoint(m_currSlice, m_pickWidth,  m_pickDepth);
    }
}

void
CurvesWidget::mousePressEvent(QMouseEvent *event)
{
  m_button = event->button();
  curveMousePressEvent(event);

  update();
}

void
CurvesWidget::curveMouseMoveEvent(QMouseEvent *event)
{
  QPoint pp = event->pos();
  float ypos = pp.y();
  float xpos = pp.x();

  bool shiftModifier = event->modifiers() & Qt::ShiftModifier;
  bool ctrlModifier = event->modifiers() & Qt::ControlModifier;
  bool altModifier = event->modifiers() & Qt::AltModifier;

  
  if (ctrlModifier)
    {
      if (validPickPoint(xpos, ypos))
	{
	  m_vline = (ypos-m_simgY)/m_zoom;
	  m_hline = (xpos-m_simgX)/m_zoom;
	  emit xPos(m_hline);
	  emit yPos(m_vline);
	  update();
	}
      return;
    }

  
  if (m_livewireMode &&
      event->buttons() != Qt::MiddleButton &&
      !shiftModifier &&
      !ctrlModifier &&
      !altModifier)
    {
      if (!validPickPoint(xpos, ypos))
	return;
      
      m_lastPickDepth = m_pickDepth;
      m_lastPickWidth = m_pickWidth;
      m_lastPickHeight= m_pickHeight;
      
      if (m_sliceType == DSlice)
	m_livewire.mouseMoveEvent(m_pickHeight, m_pickWidth, event);
      else if (m_sliceType == WSlice)
	m_livewire.mouseMoveEvent(m_pickHeight, m_pickDepth, event);
      else
	m_livewire.mouseMoveEvent(m_pickWidth, m_pickDepth, event);
      
      update();
      return;
    }

  if (m_button == Qt::LeftButton)
    {
      if (m_rubberBandActive)
	{
	  updateRubberBand(xpos, ypos);
	  update();
	  return;
	}
      if (altModifier)
	return;

      // carry on only if Alt key is not pressed
      if (validPickPoint(xpos, ypos))
	{
//	  if (!m_livewireMode && shiftModifier)
//	    {
//	      if (m_sliceType == DSlice)
//		{
//		  int a = m_pickHeight-m_lastPickHeight;
//		  int b = m_pickWidth-m_lastPickWidth;
//		  int rad = qBound(5, (int)qSqrt(a*a + b*b), 100);
//		  m_dCurves.push(m_currSlice, m_pickHeight, m_pickWidth, rad);
//		}
//	      else if (m_sliceType == WSlice)
//		{
//		  int a = m_pickHeight-m_lastPickHeight;
//		  int b = m_pickDepth-m_lastPickDepth;
//		  int rad = qBound(5, (int)qSqrt(a*a + b*b), 100);
//		  m_wCurves.push(m_currSlice, m_pickHeight, m_pickDepth, rad);
//		}
//	      else
//		{
//		  int a = m_pickWidth-m_lastPickWidth;
//		  int b = m_pickDepth-m_lastPickDepth;
//		  int rad = qBound(5, (int)qSqrt(a*a + b*b), 100);
//		  m_hCurves.push(m_currSlice, m_pickWidth,  m_pickDepth, rad);
//		}
//	      update();
//	      return;
//	    }
//	  else
	    {
	      m_lastPickDepth = m_pickDepth;
	      m_lastPickWidth = m_pickWidth;
	      m_lastPickHeight= m_pickHeight;
	      
	      if(!m_livewireMode &&
		 m_addingCurvePoints)
		{
		  if (m_sliceType == DSlice)
		    m_Curves.addPoint(m_currSlice, m_pickHeight, m_pickWidth);
		  else if (m_sliceType == WSlice)
		    m_Curves.addPoint(m_currSlice, m_pickHeight, m_pickDepth);
		  else
		    m_Curves.addPoint(m_currSlice, m_pickWidth,  m_pickDepth);
		  update();
		}
	    }
	}
    }
  else if (m_button == Qt::MiddleButton)
    {
      if (validPickPoint(xpos, ypos))
	{
	  int dd, dw, dh;
	  dd = m_pickDepth - m_lastPickDepth;
	  dw = m_pickWidth - m_lastPickWidth;
	  dh = m_pickHeight - m_lastPickHeight;
	  
	  m_lastPickDepth = m_pickDepth;
	  m_lastPickWidth = m_pickWidth;
	  m_lastPickHeight= m_pickHeight;

	  if (m_livewireMode &&
	      m_livewire.seedMoveMode())
	    {
	      if (m_sliceType == DSlice)
		m_livewire.moveShape(dh, dw);
	      else if (m_sliceType == WSlice)
		m_livewire.moveShape(dh, dd);
	      else
		m_livewire.moveShape(dw, dd);
	    }
	  else
	    {
	      if (m_sliceType == DSlice)
		m_Curves.moveCurve(m_currSlice, dh, dw);
	      else if (m_sliceType == WSlice)
		m_Curves.moveCurve(m_currSlice, dh, dd);
	      else
		m_Curves.moveCurve(m_currSlice, dw, dd);
	    }
	  update();
	  return;
	}
    }
}

void
CurvesWidget::mouseMoveEvent(QMouseEvent *event)
{
//  if (!hasFocus())
//    setFocus();

  QPoint pp = event->pos();
  float ypos = pp.y();
  float xpos = pp.x();

  m_cursorPos = pp;
  
  bool shiftModifier = event->modifiers() & Qt::ShiftModifier;
  bool ctrlModifier = event->modifiers() & Qt::ControlModifier;
  bool altModifier = event->modifiers() & Qt::AltModifier;

  curveMouseMoveEvent(event);
}

void
CurvesWidget::mouseReleaseEvent(QMouseEvent *event)
{
  m_button = Qt::NoButton;
  m_pickPoint = false;

  m_livewire.mouseReleaseEvent(event);
  m_Curves.resetMoveCurve();
  //m_wCurves.resetMoveCurve();
  //m_hCurves.resetMoveCurve();

  if (m_curveMode || m_livewireMode)
    {
      emit polygonLevels(m_Curves.polygonLevels());
    }

  if (m_rubberBandActive)
    {
      float left = m_rubberBand.left();
      float right = m_rubberBand.right();
      float top = m_rubberBand.top();
      float bottom = m_rubberBand.bottom();
      float width = m_rubberBand.width();
      float height = m_rubberBand.height();
      if (width < 0)
	{
	  m_rubberBand.setLeft(right);
	  m_rubberBand.setRight(left);
	}
      if (height < 0)
	{
	  m_rubberBand.setTop(bottom);
	  m_rubberBand.setBottom(top);
	}

      // --- now update the min and max limits
      left = m_rubberBand.left();
      right = m_rubberBand.right();
      top = m_rubberBand.top();
      bottom = m_rubberBand.bottom();
      if (m_sliceType == DSlice)
	{
	  m_minHSlice = left*m_Height;
	  m_maxHSlice = right*m_Height;
	  m_minWSlice = top*m_Width;
	  m_maxWSlice = bottom*m_Width;
	}
      else if (m_sliceType == WSlice)
	{
	  m_minHSlice = left*m_Height;
	  m_maxHSlice = right*m_Height;
	  m_minDSlice = top*m_Depth;
	  m_maxDSlice = bottom*m_Depth;
	}
      else
	{
	  m_minWSlice = left*m_Width;
	  m_maxWSlice = right*m_Width;
	  m_minDSlice = top*m_Depth;
	  m_maxDSlice = bottom*m_Depth;
	}

      emit updateViewerBox(m_minDSlice, m_maxDSlice,
			   m_minWSlice, m_maxWSlice,
			   m_minHSlice, m_maxHSlice);
    }

  m_rubberXmin = false;
  m_rubberYmin = false;
  m_rubberXmax = false;
  m_rubberYmax = false;
  m_rubberNew = false;
  m_rubberBandActive = false;

  update();
}

void
CurvesWidget::doAnother(int step)
{
  // if we are in curve add points mode switch it off
  if (m_addingCurvePoints) endCurve();
  
  int minS, maxS;
  if (m_sliceType == DSlice)
    { 
      minS = m_minDSlice; 
      maxS = m_maxDSlice; 
    } 
  else if (m_sliceType == WSlice) 
    { 
      minS = m_minWSlice; 
      maxS = m_maxWSlice; 
    } 
  else 
    { 
      minS = m_minHSlice; 
      maxS = m_maxHSlice; 
    }  

  m_currSlice += step;
  m_currSlice = qBound(minS, m_currSlice, maxS);
  emit getSlice(m_currSlice);
  update();
}

void
CurvesWidget::wheelEvent(QWheelEvent *event)
{
  event->setAccepted(true);
  
  int numSteps = event->delta()/8.0f/15.0f;
  doAnother(-numSteps);
}

void
CurvesWidget::updateRubberBand(int xpos, int ypos)
{
  if (m_rubberNew)
    {
      float frcX = (float)(xpos-m_simgX)/(float)m_simgWidth;
      float frcY = (float)(ypos-m_simgY)/(float)m_simgHeight;	  
      frcX = qBound(0.0f, frcX, 1.0f);
      frcY = qBound(0.0f, frcY, 1.0f);
      float left = m_rubberBand.left();
      float right = frcX;
      float top = m_rubberBand.top();
      float bottom = frcY;
      m_rubberBand = QRectF(left, top, right-left, bottom-top);
    }
  else if (m_rubberXmin)
    {
      float frcX = (float)(xpos-m_simgX)/(float)m_simgWidth;
      frcX = qBound(0.0f, frcX, 1.0f);
      m_rubberBand.setLeft(frcX);
      int w = m_imgWidth*(m_rubberBand.right()-m_rubberBand.left());
      if (w < 10)
	{
	  float delta = 10.0/m_imgWidth;
	  delta = m_rubberBand.right()-delta;
	  delta = qBound(0.0f, delta, 1.0f);
	  m_rubberBand.setLeft(delta);
	}
    }
  else if (m_rubberXmax)
    {
      float frcX = (float)(xpos-m_simgX)/(float)m_simgWidth;
      frcX = qBound(0.0f, frcX, 1.0f);
      m_rubberBand.setRight(frcX);
      int w = m_imgWidth*(m_rubberBand.right()-m_rubberBand.left());
      if (w < 10)
	{
	  float delta = 10.0/m_imgWidth;
	  delta = m_rubberBand.left()+delta;
	  delta = qBound(0.0f, delta, 1.0f);
	  m_rubberBand.setRight(delta);
	}
    }
  else if (m_rubberYmin)
    {
      float frcY = (float)(ypos-m_simgY)/(float)m_simgHeight;	  
      frcY = qBound(0.0f, frcY, 1.0f);
      m_rubberBand.setTop(frcY);
      int h = m_imgHeight*(m_rubberBand.bottom()-m_rubberBand.top());
      if (h < 10)
	{
	  float delta = 10.0/m_imgHeight;
	  delta = m_rubberBand.bottom()-delta;
	  delta = qBound(0.0f, delta, 1.0f);
	  m_rubberBand.setTop(delta);
	}
    }
  else if (m_rubberYmax)
    {
      float frcY = (float)(ypos-m_simgY)/(float)m_simgHeight;	  
      frcY = qBound(0.0f, frcY, 1.0f);
      m_rubberBand.setBottom(frcY);
      int h = m_imgHeight*(m_rubberBand.bottom()-m_rubberBand.top());
      if (h < 10)
	{
	  float delta = 10.0/m_imgHeight;
	  delta = m_rubberBand.top()+delta;
	  delta = qBound(0.0f, delta, 1.0f);
	  m_rubberBand.setBottom(delta);
	}
    }
}

bool
CurvesWidget::validPickPoint(int xpos, int ypos)
{
  if (! m_slice)
    return false;

  if (xpos >= m_simgX-2 && xpos <= m_simgX+m_simgWidth+2 &&
      ypos >= m_simgY-2 && ypos <= m_simgY+m_simgHeight+2)
    {
      // give some slack(2px) at the ends
      float frcX = (float)(xpos-m_simgX)/(float)m_simgWidth;
      float frcY = (float)(ypos-m_simgY)/(float)m_simgHeight;
      
      frcX = qBound(0.0f, frcX, 1.0f);
      frcY = qBound(0.0f, frcY, 1.0f);
      
      if (m_sliceType == DSlice)
	{
	  m_pickDepth = m_currSlice;
	  m_pickHeight = frcX*m_imgWidth;
	  m_pickWidth = frcY*m_imgHeight;
	}
      else if (m_sliceType == WSlice)
	{
	  m_pickWidth = m_currSlice;
	  m_pickHeight = frcX*m_imgWidth;
	  m_pickDepth = frcY*m_imgHeight;
	}
      else
	{
	  m_pickHeight = m_currSlice;
	  m_pickWidth = frcX*m_imgWidth;
	  m_pickDepth = frcY*m_imgHeight;
	}

      return true;
    }
  return false;
}

void
CurvesWidget::preselect()
{
  float ypos = m_cursorPos.y();
  float xpos = m_cursorPos.x();


  if (validPickPoint(xpos, ypos))
    update();
}

void
CurvesWidget::processCommands(QString cmd)
{
  cmd = cmd.toLower();
  QStringList list = cmd.split(" ", QString::SkipEmptyParts);

  if (list.size() == 2)
    {
      int smin = list[0].toInt();
      int smax = list[1].toInt();

      if (smin > smax)
	{ // swap
	  int t = smin;
	  smin = smax;
	  smax = t;
	}

      if (m_sliceType == DSlice)
	{
	  m_minDSlice = qBound(0, smin, m_Depth);
	  m_maxDSlice = qBound(0, smax, m_Depth);
	  m_currSlice = qBound(m_minDSlice,
			       m_currSlice,
			       m_maxDSlice);
	}
      else if (m_sliceType == WSlice)
	{
	  m_minWSlice = qBound(0, smin, m_Width);
	  m_maxWSlice = qBound(0, smax, m_Width);
	  m_currSlice = qBound(m_minWSlice,
			       m_currSlice,
			       m_maxWSlice);
	}
      else if (m_sliceType == HSlice)
	{
	  m_minHSlice = qBound(0, smin, m_Height);
	  m_maxHSlice = qBound(0, smax, m_Height);
	  m_currSlice = qBound(m_minHSlice,
			       m_currSlice,
			       m_maxHSlice);
	}

      emit updateViewerBox(m_minDSlice, m_maxDSlice,
			   m_minWSlice, m_maxWSlice,
			   m_minHSlice, m_maxHSlice);
      update();
    }
  else
    QMessageBox::information(0, "Error",
			     "Please specify both the limits");

}

bool
CurvesWidget::withinBounds(int scrx, int scry)
{
  if (m_sliceType == DSlice)
    {
      if (scrx >= m_minHSlice &&
	  scrx <= m_maxHSlice &&
	  scry >= m_minWSlice &&
	  scry <= m_maxWSlice)
	return true;
    }
  else if (m_sliceType == WSlice)
    {
      if (scrx >= m_minHSlice &&
	  scrx <= m_maxHSlice &&
	  scry >= m_minDSlice &&
	  scry <= m_maxDSlice)
	return true;
    }
  else
    {
      if (scrx >= m_minWSlice &&
	  scrx <= m_maxWSlice &&
	  scry >= m_minDSlice &&
	  scry <= m_maxDSlice)
	return true;
    }

  return false;
}


void
CurvesWidget::getSliceLimits(int &size1, int &size2,
			    int &imin, int &imax,
			    int &jmin, int &jmax)
{
  if (m_sliceType == DSlice)
    {
      size1 = m_maxHSlice-m_minHSlice+1;
      size2 = m_maxWSlice-m_minWSlice+1;
      imin = m_minWSlice;
      imax = m_maxWSlice;
      jmin = m_minHSlice;
      jmax = m_maxHSlice;
    }
  else if (m_sliceType == WSlice)
    {
      size1 = m_maxHSlice-m_minHSlice+1;
      size2 = m_maxDSlice-m_minDSlice+1;
      imin = m_minDSlice;
      imax = m_maxDSlice;
      jmin = m_minHSlice;
      jmax = m_maxHSlice;
    }
  else if (m_sliceType == HSlice)
    {
      size1 = m_maxWSlice-m_minWSlice+1;
      size2 = m_maxDSlice-m_minDSlice+1;
      imin = m_minDSlice;
      imax = m_maxDSlice;
      jmin = m_minWSlice;
      jmax = m_maxWSlice;
    }

}

void
CurvesWidget::paintUsingCurves(CurveGroup* cg,
			       int slc, int wd, int ht,
			       uchar *maskData)
{
  QImage pimg= QImage(wd, ht, QImage::Format_RGB32);
  pimg.fill(0);
  QPainter p(&pimg);
  { // normal curves
    QList<Curve*> curves;
    curves = cg->getCurvesAt(slc, true);
    
    if (curves.count() > 0)
      {
	for(int l=0; l<curves.count(); l++)
	  {
	    //int tag = curves[l]->tag;
	    int tag = 255;
	    p.setBrush(QColor(tag, 255, 255)); 
	    if (curves[l]->closed)
	      {
		p.setPen(QPen(QColor(tag, 255, 255), 1));
		p.drawPolygon(curves[l]->pts);
	      }
	    else
	      {
		p.setPen(QPen(QColor(tag, 255, 255), curves[l]->thickness,
			      Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
		p.drawPolyline(curves[l]->pts);
	      }
	  }
      }
  }

  { // interpolated curves
    QList<Curve> curves;
    curves = cg->getMorphedCurvesAt(slc);

    if (curves.count() > 0)
      {
	for(int l=0; l<curves.count(); l++)
	  {
	    //int tag = curves[l].tag;
	    int tag = 255;
	    p.setBrush(QColor(tag, 255, 255)); 
	    if (curves[l].closed)
	      {
		p.setPen(QPen(QColor(tag, 255, 255), 1));
		p.drawPolygon(curves[l].pts);
	      }
	    else
	      {
		p.setPen(QPen(QColor(tag, 255, 255), curves[l].thickness,
			      Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
		p.drawPolyline(curves[l].pts);
	      }
	  }
      }
  }

//  QLabel *lbl = new QLabel();
//  lbl->setPixmap(QPixmap::fromImage(pimg));
//  lbl->show();

  QRgb *rgb = (QRgb*)(pimg.bits());
  for(int i=0; i<wd*ht; i++)
    maskData[i] = qRed(rgb[i]);
}

void
CurvesWidget::paintUsingCurves(int slctype,
			       int slc, int wd, int ht,
			       uchar *maskData)
{
  paintUsingCurves(&m_Curves,
		   slc, wd, ht,
		   maskData);
}

void
CurvesWidget::paintUsingCurves(uchar *maskData)
{
  if (!m_curveMode && !m_livewireMode)
    return;

  QList<int> gtag;
  gtag << Global::tag();

  paintUsingCurves(&m_Curves,
		   m_currSlice, m_imgWidth, m_imgHeight,
		   maskData);
}

void
CurvesWidget::updateMaskImage()
{
  memcpy(m_tags, m_maskslice, m_imgWidth*m_imgHeight);

  m_maskimage = QImage(m_tags,  //data
		       m_imgWidth, // width
		       m_imgHeight, // height
		       m_imgWidth, // bytesperline
		       QImage::Format_Indexed8);
  m_maskimage.setColorTable(m_tagColors);
  m_maskimageScaled = m_maskimage.scaled(m_simgWidth,
					 m_simgHeight,
					 Qt::IgnoreAspectRatio,
					 Qt::FastTransformation);

}

void
CurvesWidget::setSmoothType(int i)
{
  m_livewire.setSmoothType(i);
 
  // transfer data for livewire calculation
  uchar *sliceData = new uchar[m_imgHeight*m_imgWidth];
  for(int i=0; i<m_imgHeight*m_imgWidth; i++)
    sliceData[i] = m_sliceImage[4*i];

  m_livewire.setImageData(m_imgWidth, m_imgHeight, sliceData);
  delete [] sliceData;
  m_gradImageScaled = m_livewire.gradientImage().scaled(m_simgWidth,
							m_simgHeight,
							Qt::IgnoreAspectRatio,
							Qt::FastTransformation);
}

void
CurvesWidget::setGradType(int i)
{
  m_livewire.setGradType(i);
 
  // transfer data for livewire calculation
  uchar *sliceData = new uchar[m_imgHeight*m_imgWidth];
  for(int i=0; i<m_imgHeight*m_imgWidth; i++)
    sliceData[i] = m_sliceImage[4*i];
  m_livewire.setImageData(m_imgWidth, m_imgHeight, sliceData);
  delete [] sliceData;
  m_gradImageScaled = m_livewire.gradientImage().scaled(m_simgWidth,
							m_simgHeight,
							Qt::IgnoreAspectRatio,
							Qt::FastTransformation);
}

void
CurvesWidget::saveCurveData(QFile *cfile, int key, Curve *c)
{
  char keyword[100];

  memset(keyword, 0, 100);
  sprintf(keyword, "curvestart\n");
    
  cfile->write((char*)keyword, strlen(keyword));

  uchar type = c->type;
  int tag = c->tag;
  int thickness = c->thickness;
  bool closed = c->closed;
  memset(keyword, 0, 100);
  sprintf(keyword, "key\n");
  cfile->write((char*)keyword, strlen(keyword));
  cfile->write((char*)&key, sizeof(int));
  
  memset(keyword, 0, 100);
  sprintf(keyword, "tag\n");
  cfile->write((char*)keyword, strlen(keyword));
  cfile->write((char*)&tag, sizeof(int));
  
  memset(keyword, 0, 100);
  sprintf(keyword, "type\n");
  cfile->write((char*)keyword, strlen(keyword));
  cfile->write((char*)&type, sizeof(uchar));
  
  memset(keyword, 0, 100);
  sprintf(keyword, "thickness\n");
  cfile->write((char*)keyword, strlen(keyword));
  cfile->write((char*)&thickness, sizeof(int));
  
  memset(keyword, 0, 100);
  sprintf(keyword, "closed\n");
  cfile->write((char*)keyword, strlen(keyword));
  cfile->write((char*)&closed, sizeof(bool));
  
  {
    QVector<QPointF> pts = c->pts;
    memset(keyword, 0, 100);
    sprintf(keyword, "points\n");
    cfile->write((char*)keyword, strlen(keyword));
    int npts = pts.count();
    cfile->write((char*)&npts, sizeof(int));
    float *pt = new float [2*npts];
    for(int j=0; j<npts; j++)
      {
	pt[2*j+0] = pts[j].x();
	pt[2*j+1] = pts[j].y();
      }
    cfile->write((char*)pt, 2*npts*sizeof(float));
    delete [] pt;
  }
  {
    QVector<int> seedpos = c->seedpos;
    memset(keyword, 0, 100);
    sprintf(keyword, "seedpos\n");
    cfile->write((char*)keyword, strlen(keyword));
    int npts = seedpos.count();
    cfile->write((char*)&npts, sizeof(int));
    cfile->write((char*)(seedpos.data()), npts*sizeof(int));
  }
  {
    QVector<QPointF> seeds = c->seeds;
    if (seeds.count() > 0)
      {
	memset(keyword, 0, 100);
	sprintf(keyword, "seeds\n");
	cfile->write((char*)keyword, strlen(keyword));
	int npts = seeds.count();
	cfile->write((char*)&npts, sizeof(int));
	float *pt = new float [2*npts];
	for(int j=0; j<npts; j++)
	  {
	    pt[2*j+0] = seeds[j].x();
	    pt[2*j+1] = seeds[j].y();
	  }
	cfile->write((char*)pt, 2*npts*sizeof(float));
	delete [] pt;
      }
  }
  
  memset(keyword, 0, 100);
  sprintf(keyword, "curveend\n");
  cfile->write((char*)keyword, strlen(keyword));
}

QPair<int, Curve>
CurvesWidget::loadCurveData(QFile *cfile)
{
  char keyword[100];

  Curve c;
  int key=0;
  bool done = false;
  while(!done)
    {
      cfile->readLine((char*)&keyword, 100);

      int t;
      bool b;
      if (strcmp(keyword, "curveend\n") == 0)
	done = true;
      else if (strcmp(keyword, "key\n") == 0)
	cfile->read((char*)&key, sizeof(int));
      else if (strcmp(keyword, "tag\n") == 0)
	{
	  cfile->read((char*)&t, sizeof(int));
	  c.tag = t;
	}
      else if (strcmp(keyword, "type\n") == 0)
	{
	  cfile->read((char*)&t, sizeof(uchar));
	  c.type = t;
	}
      else if (strcmp(keyword, "thickness\n") == 0)
	{
	  cfile->read((char*)&t, sizeof(int));
	  c.thickness = t;
	}
      else if (strcmp(keyword, "closed\n") == 0)
	{
	  cfile->read((char*)&b, sizeof(bool));
	  c.closed = b;
	}
      else if (strcmp(keyword, "points\n") == 0)
	{
	  int npts;
	  float *pt;
	  cfile->read((char*)&npts, sizeof(int));
	  pt = new float[2*npts];
	  cfile->read((char*)pt, 2*npts*sizeof(float));
	  for(int ni=0; ni<npts; ni++)
	    c.pts << QPointF(pt[2*ni+0], pt[2*ni+1]);
	  delete [] pt;
	}	      
      else if (strcmp(keyword, "seeds\n") == 0)
	{
	  int npts;
	  float *pt;
	  cfile->read((char*)&npts, sizeof(int));
	  pt = new float[2*npts];
	  cfile->read((char*)pt, 2*npts*sizeof(float));
	  for(int ni=0; ni<npts; ni++)
	    c.seeds << QPointF(pt[2*ni+0], pt[2*ni+1]);
	  delete [] pt;
	}	      
      else if (strcmp(keyword, "seedpos\n") == 0)
	{
	  int npts;
	  int *pt;
	  cfile->read((char*)&npts, sizeof(int));
	  pt = new int[npts];
	  cfile->read((char*)pt, npts*sizeof(int));
	  for(int ni=0; ni<npts; ni++)
	    c.seedpos << pt[ni];
	  delete [] pt;
	}	      
    }

  return qMakePair(key, c);
}

void
CurvesWidget::saveMorphedCurves(QFile *cfile, CurveGroup *cg)
{
  char keyword[100];

  memset(keyword, 0, 100);
  sprintf(keyword, "morphcurvegroupstart\n");
  cfile->write((char*)keyword, strlen(keyword));

  //QList< QMap<int, Curve> > *mcg = cg->getPointerToMorphedCurves();
  QList< QMap<int, Curve> > *mcg = cg->morphedCurves();
  int mcgcount = mcg->count();
  if (mcgcount == 0)
    {
      memset(keyword, 0, 100);
      sprintf(keyword, "morphcurvegroupend\n");
      cfile->write((char*)keyword, strlen(keyword));
      return;
    }

  for(int m=0; m<mcgcount; m++)
    {
      QList<int> keys = (*mcg)[m].keys();
      int ncurves = keys.count();

      memset(keyword, 0, 100);
      sprintf(keyword, "morphblockstart\n");
      cfile->write((char*)keyword, strlen(keyword));
      
      for(int i=0; i<ncurves; i++)
	{
	  Curve c = (*mcg)[m].value(keys[i]);
	  saveCurveData(cfile, keys[i], &c);
	} 

      memset(keyword, 0, 100);
      sprintf(keyword, "morphblockend\n");
      cfile->write((char*)keyword, strlen(keyword));
    }

  memset(keyword, 0, 100);
  sprintf(keyword, "morphcurvegroupend\n");
  cfile->write((char*)keyword, strlen(keyword));
}

void
CurvesWidget::saveShrinkwrapCurves(QFile *cfile, CurveGroup *cg)
{
  char keyword[100];

  memset(keyword, 0, 100);
  sprintf(keyword, "shrinkwrapgroupstart\n");
  cfile->write((char*)keyword, strlen(keyword));

  QList< QMultiMap<int, Curve*> > *mcg = cg->shrinkwrapCurves();
  int mcgcount = mcg->count();
  if (mcgcount == 0)
    {
      memset(keyword, 0, 100);
      sprintf(keyword, "shrinkwrapgroupend\n");
      cfile->write((char*)keyword, strlen(keyword));
      return;
    }

  for(int m=0; m<mcgcount; m++)
    {
      QList<int> keys = (*mcg)[m].uniqueKeys();
      int nkeys = keys.count();

      memset(keyword, 0, 100);
      sprintf(keyword, "shrinkwrapblockstart\n");
      cfile->write((char*)keyword, strlen(keyword));
      
      for(int i=0; i<nkeys; i++)
	{
	  QList<Curve*> curves = (*mcg)[m].values(keys[i]);
	  for(int j=0; j<curves.count(); j++)
	    saveCurveData(cfile, keys[i], curves[j]);
	} 

      memset(keyword, 0, 100);
      sprintf(keyword, "shrinkwrapblockend\n");
      cfile->write((char*)keyword, strlen(keyword));
    }

  memset(keyword, 0, 100);
  sprintf(keyword, "shrinkwrapgroupend\n");
  cfile->write((char*)keyword, strlen(keyword));
}

void
CurvesWidget::saveCurves(QFile *cfile, CurveGroup *cg)
{
  QList<int> keys = cg->polygonLevels();

  char keyword[100];

  memset(keyword, 0, 100);
  sprintf(keyword, "curvegroupstart\n");
  cfile->write((char*)keyword, strlen(keyword));
  
  int ncurves = 0;
  for(int i=0; i<keys.count(); i++)
    ncurves += cg->getCurvesAt(keys[i]).count();

  if (ncurves <= 0)
    {
      memset(keyword, 0, 100);
      sprintf(keyword, "curvegroupend\n");
      cfile->write((char*)keyword, strlen(keyword));  
      memset(keyword, 0, 100);
      sprintf(keyword, "morphcurvegroupstart\n");
      cfile->write((char*)keyword, strlen(keyword));
      memset(keyword, 0, 100);
      sprintf(keyword, "morphcurvegroupend\n");
      cfile->write((char*)keyword, strlen(keyword));
    }
  else
    {
      for(int i=0; i<keys.count(); i++)
	{
	  QList<Curve*> c = cg->getCurvesAt(keys[i]);
	  for (int j=0; j<c.count(); j++)
	    saveCurveData(cfile, keys[i], c[j]);
	} 
      memset(keyword, 0, 100);
      sprintf(keyword, "curvegroupend\n");
      cfile->write((char*)keyword, strlen(keyword));  

      saveMorphedCurves(cfile, cg);
    }

  saveShrinkwrapCurves(cfile, cg);
}

void
CurvesWidget::saveCurves()
{
  QString curvesfile = QFileDialog::getSaveFileName(0,
					      "Save Curves",
					      Global::previousDirectory(),
					      "Curves Files (*.curves)",
					      0,
					      QFileDialog::DontUseNativeDialog);

  if (curvesfile.isEmpty())
    return;

  saveCurves(curvesfile);
}

void
CurvesWidget::saveCurves(QString curvesfile)
{
  if (!StaticFunctions::checkExtension(curvesfile, ".curve") &&
      !StaticFunctions::checkExtension(curvesfile, ".curves"))
      curvesfile += ".curves";

  if (m_sliceType == DSlice)
    curvesfile += "d";
  else if (m_sliceType == WSlice)
    curvesfile += "w";
  else
    curvesfile += "h";

  // if no curves present return
  if (!curvesPresent())
    { // remove existing file
      if (QFile(curvesfile).exists())
	QFile(curvesfile).remove();
      return;
    }
  
  QFile cfile;

  cfile.setFileName(curvesfile);
  cfile.open(QFile::WriteOnly);

  saveCurves(&cfile, &m_Curves);

  cfile.close();
}

void
CurvesWidget::loadCurves(QFile *cfile, CurveGroup *cg)
{
  char keyword[100];
  cfile->readLine((char*)&keyword, 100);

  if (strcmp(keyword, "curvegroupstart\n") != 0)
    {
      QMessageBox::information(0, "", QString("curvegroupstart not found!\n%1").arg(keyword));
      return;
    }

  bool cgend = false;
  while(!cgend)
    {
      cfile->readLine((char*)&keyword, 100);

      if (strcmp(keyword, "curvegroupend\n") == 0)
	cgend = true;      
      else if (strcmp(keyword, "curvestart\n") == 0)
	{
	  QPair<int, Curve> cpair = loadCurveData(cfile);
	  // do not pass on zero length curves
	  if (cpair.second.pts.count() > 0)
	    cg->setCurveAt(cpair.first, cpair.second);
	}
    }

  loadMorphedCurves(cfile, cg);

  loadShrinkwrapCurves(cfile, cg);
}

void
CurvesWidget::loadMorphedCurves(QFile *cfile, CurveGroup *cg)
{
  char keyword[100];
  cfile->readLine((char*)&keyword, 100);

  if (strcmp(keyword, "morphcurvegroupstart\n") != 0)
    {
      QMessageBox::information(0, "", QString("morphcurvegroupstart not found!").arg(keyword));
      return;
    }

  QMap<int, Curve> mcg;
  bool cgend = false;
  while(!cgend)
    {
      cfile->readLine((char*)&keyword, 100);

      if (strcmp(keyword, "morphcurvegroupend\n") == 0)
	cgend = true;      
      else if (strcmp(keyword, "morphblockstart\n") == 0)
	mcg.clear();
      else if (strcmp(keyword, "morphblockend\n") == 0)
	cg->addMorphBlock(mcg);
      else if (strcmp(keyword, "curvestart\n") == 0)
	{
	  QPair<int, Curve> cpair = loadCurveData(cfile);
	  // do not pass on zero length curves
	  if (cpair.second.pts.count() > 0)
	    mcg.insert(cpair.first, cpair.second);
	}
    }
}

void
CurvesWidget::loadShrinkwrapCurves(QFile *cfile, CurveGroup *cg)
{
  char keyword[100];
  cfile->readLine((char*)&keyword, 100);

  if (strcmp(keyword, "shrinkwrapgroupstart\n") != 0)
    {
      QMessageBox::information(0, "", QString("shrinkwrapgroupstart not found!").arg(keyword));
      return;
    }

  QMultiMap<int, Curve*> mcg;
  bool cgend = false;
  while(!cgend)
    {
      cfile->readLine((char*)&keyword, 100);

      if (strcmp(keyword, "shrinkwrapgroupend\n") == 0)
	cgend = true;      
      else if (strcmp(keyword, "shrinkwrapblockstart\n") == 0)
	mcg.clear();
      else if (strcmp(keyword, "shrinkwrapblockend\n") == 0)
	cg->addShrinkwrapBlock(mcg);
      else if (strcmp(keyword, "curvestart\n") == 0)
	{
	  QPair<int, Curve> cpair = loadCurveData(cfile);
	  // do not pass on zero length curves
	  if (cpair.second.pts.count() > 0)
	    {
	      Curve *c = new Curve;
	      *c = cpair.second;
	      mcg.insert(cpair.first, c);
	    }
	}
    }
}

void
CurvesWidget::loadCurves(QString curvesfile)
{
  QFile cfile;

  if (m_sliceType == DSlice)
    curvesfile += "d";
  else if (m_sliceType == WSlice)
    curvesfile += "w";
  else
    curvesfile += "h";

  cfile.setFileName(curvesfile);
  if (cfile.exists() == false)
    return;
  
  cfile.open(QFile::ReadOnly);

  loadCurves(&cfile, &m_Curves);

  cfile.close();

  emit polygonLevels(m_Curves.polygonLevels());

  //QMessageBox::information(0, "", QString("Curves loaded from %1").arg(curvesfile));
}

void
CurvesWidget::loadCurves()
{
  QString curvesfile = QFileDialog::getOpenFileName(0,
						    "Load Curves",
						    Global::previousDirectory(),
						    "Curves Files (*.curves)",
						    0,
						    QFileDialog::DontUseNativeDialog);
  
  if (curvesfile.isEmpty())
    return;

  loadCurves(curvesfile);
}

void
CurvesWidget::startShrinkwrap()
{
  m_Curves.startShrinkwrap();
}

void
CurvesWidget::endShrinkwrap()
{
  m_Curves.endShrinkwrap();
  emit polygonLevels(m_Curves.polygonLevels());
}

void
CurvesWidget::shrinkwrapCurve()
{
  uchar *imageData = new uchar[m_imgWidth*m_imgHeight];

  int size1, size2, imin, imax, jmin, jmax;
  getSliceLimits(size1, size2, imin, imax, jmin, jmax);
  if (size1 != m_imgHeight || size2 != m_imgWidth) 
    {
      // reset outside bounding box
      memset(imageData, 0, m_imgWidth*m_imgHeight);
      for(int i=imin; i<=imax; i++)
	for(int j=jmin; j<=jmax; j++)
	  imageData[i*m_imgWidth+j] = 255;
    }
  else
    memset(imageData, 255, m_imgWidth*m_imgHeight);

//  for(int i=0; i<m_imgWidth*m_imgHeight; i++)
//    imageData[i] = (m_sliceImage[4*i+0]>0 && imageData[i]>0 ? 255 : 0);

  uchar *tagColors = Global::tagColors();
  for(int i=0; i<m_imgWidth*m_imgHeight; i++)
    {
      uchar a = tagColors[4*m_tags[i]+3];      
      imageData[i] = (a>0 && m_sliceImage[4*i+0]>0 && imageData[i]>0 ? 255 : 0);
    }
      
  m_Curves.shrinkwrap(m_currSlice, imageData, m_imgWidth, m_imgHeight);
    
  delete [] imageData;
  
  checkRecursive();
}

void
CurvesWidget::setMinCurveLength(int sz)
{
  m_Curves.setShrinkwrapIgnoreSize(sz);
}
