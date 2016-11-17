#include "imagewidget.h"
#include "global.h"
#include "staticfunctions.h"
#include <math.h>
#include "graphcut.h"
#include "morphslice.h"

#include <QFile>
#include <QLabel>

void
ImageWidget::getBox(int &minD, int &maxD, 
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
ImageWidget::setBox(int minD, int maxD, 
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

ImageWidget::ImageWidget(QWidget *parent) :
  QWidget(parent)
{
  setFocusPolicy(Qt::StrongFocus);
  
  setMouseTracking(true);

  m_bytesPerVoxel = 1;
  m_Depth = m_Width = m_Height = 0;
  m_imgHeight = 100;
  m_imgWidth = 100;
  m_simgHeight = 100;
  m_simgWidth = 100;

  m_volPtr = 0;
  m_maskPtr = 0;

  //m_modeType = 0; // graphcut
  m_modeType = 1; // superpixels

  m_hline = m_vline = 0;

  m_zoom = 1;

  m_lut = new uchar[4*256*256];
  memset(m_lut, 0, 4*256*256);

  m_image = QImage(100, 100, QImage::Format_RGB32);
  m_imageScaled = QImage(100, 100, QImage::Format_RGB32);
  m_maskimage = QImage(100, 100, QImage::Format_Indexed8);
  m_maskimageScaled = QImage(100, 100, QImage::Format_Indexed8);
  m_prevslicetagimage = QImage(100, 100, QImage::Format_Indexed8);
  m_prevslicetagimageScaled = QImage(100, 100, QImage::Format_Indexed8);
  m_userimage = QImage(100, 100, QImage::Format_Indexed8);
  m_userimageScaled = QImage(100, 100, QImage::Format_Indexed8);
  m_spcimage = QImage(100, 100, QImage::Format_ARGB32);
  m_spcimageScaled = QImage(100, 100, QImage::Format_ARGB32);

  m_image.fill(0);
  m_imageScaled.fill(0);
  m_maskimage.fill(0);
  m_maskimageScaled.fill(0);
  m_prevslicetagimage.fill(0);
  m_prevslicetagimageScaled.fill(0);
  m_userimage.fill(0);
  m_userimageScaled.fill(0);
  m_spcimage.fill(0);
  m_spcimageScaled.fill(0);

  m_slice = 0;
  m_sliceImage = 0;
  m_maskslice = 0;
  m_tags = 0;
  m_prevtags = 0;
  m_usertags = 0;
  m_prevslicetags = 0;
  m_labels = 0;
  
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

  m_showTags.clear();
  m_showTags << -1;

  m_prevslicetagColors.clear();
  m_prevslicetagColors.resize(256);
  m_prevslicetagColors[0] = qRgba(0,0,0,0);
  for(int i=1; i<256; i++)
    m_prevslicetagColors[i] = qRgba(0, 0, 127, 127);
}

void
ImageWidget::setModeType(int mt)
{
  m_modeType = mt;

  if (m_volPtr)
    getSlice();
}

void ImageWidget::enterEvent(QEvent *e)
{
  //setFocus();
  grabKeyboard();
}
void ImageWidget::leaveEvent(QEvent *e)
{
  //clearFocus();
  releaseKeyboard();
}

void
ImageWidget::saveImage()
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
ImageWidget::setZoom(float z)
{  
  if (!m_volPtr)
    return;

  setMinimumSize(QSize(m_imgWidth, m_imgHeight));

  bool z9 = false;
  int z9x, z9y, z9mx, z9my;
  if (z < 0) // zoom to selection
    {
      int size1, size2;
      int imin, imax, jmin, jmax;
      getSliceLimits(size1, size2, imin, imax, jmin, jmax);

      QWidget *prt = (QWidget*)parent();
      int frmHeight = prt->rect().height()-50;
      int frmWidth = prt->rect().width()-50;
//      float zn = qMin((float)frmWidth/m_imgWidth,
//		      (float)frmHeight/m_imgHeight);

      m_zoom = 1;
      if (size1 > 0 && size2 > 0)
	{
	  m_zoom = qMin((float)frmWidth/size2,
			(float)frmHeight/size1);
	  m_zoom = qMax(0.01f, m_zoom);
      
	  z9 = true;
	  z9x = m_zoom*(jmax+jmin)/2;
	  z9y = m_zoom*(imax+imin)/2;
	  z9mx = frmWidth/2;
	  z9my = frmHeight/2;
	}
    }
  else
    m_zoom = qMax(0.01f, z);

  resizeImage();
  update();

  if (z9)
    m_scrollArea->ensureVisible(z9x, z9y, z9mx, z9my);
}

void
ImageWidget::setSlice(int s)
{
  m_currSlice = qBound(0, s, m_maxSlice-1);

  getSlice();
}

void
ImageWidget::getSlice()
{
  qint64 bps = m_Width*m_Height;

  if (m_sliceType == DSlice)
    {
      memcpy(m_slice, m_volPtr+m_currSlice*bps*m_bytesPerVoxel, bps*m_bytesPerVoxel);
      memcpy(m_maskslice, m_maskPtr+m_currSlice*bps, bps);
    }

  if (m_sliceType == WSlice)
    {
      for(int d=0; d<m_Depth; d++)
	memcpy(m_slice + d*m_Height*m_bytesPerVoxel,
	       m_volPtr + (d*bps + m_currSlice*m_Height)*m_bytesPerVoxel,
	       m_Height*m_bytesPerVoxel);

      for(int d=0; d<m_Depth; d++)
	memcpy(m_maskslice + d*m_Height,
	       m_maskPtr + d*bps + m_currSlice*m_Height,
	       m_Height);
    }

  if (m_sliceType == HSlice)
    {
      int it = 0;
      for(int d=0; d<m_Depth; d++)
	{
	  for(int j=0; j<m_Width; j++, it++)
	    memcpy(m_slice + it*m_bytesPerVoxel,
		   m_volPtr + (d*bps + (j*m_Height + m_currSlice))*m_bytesPerVoxel,
		   m_bytesPerVoxel);
	}

      it = 0;
      for(int d=0; d<m_Depth; d++)
	{
	  for(int j=0; j<m_Width; j++, it++)
	    memcpy(m_maskslice + it,
		   m_maskPtr + d*bps + (j*m_Height + m_currSlice),
		   1);
	}
    }


  memcpy(m_prevslicetags, m_prevtags, m_imgWidth*m_imgHeight);
  processPrevSliceTags();

  memcpy(m_prevtags, m_maskslice, m_imgWidth*m_imgHeight);

  m_spcimage.fill(0);

  recolorImage();

  resizeImage();
  
  update();

  if (m_applyRecursive)
    {
      qApp->processEvents();
      QKeyEvent dummy(QEvent::KeyPress,
		      m_key,
		      Qt::NoModifier);
      keyPressEvent(&dummy);
    }
}

void
ImageWidget::depthUserRange(int& umin, int& umax)
{
  umin = m_minDSlice;
  umax = m_maxDSlice;
}
void
ImageWidget::widthUserRange(int& umin, int& umax)
{
  umin = m_minWSlice;
  umax = m_maxWSlice;
}
void
ImageWidget::heightUserRange(int& umin, int& umax)
{
  umin = m_minHSlice;
  umax = m_maxHSlice;
}

void
ImageWidget::userRangeChanged(int umin, int umax)
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
ImageWidget::loadLookupTable(QImage colorMap)
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
ImageWidget::setGridSize(int d, int w, int h)
{
  m_bytesPerVoxel = Global::bytesPerVoxel();

  m_Depth = d;
  m_Width = w;
  m_Height= h;
   
  m_minDSlice = 0;
  m_maxDSlice = m_Depth-1;
  m_minWSlice = 0;
  m_maxWSlice = m_Width-1;
  m_minHSlice = 0;
  m_maxHSlice = m_Height-1;
}

void ImageWidget::setHLine(int h) { m_hline = h; update(); }
void ImageWidget::setVLine(int v) { m_vline = v; update(); }

void
ImageWidget::setSliceType(int st)
{
  m_sliceType = st;
}

void
ImageWidget::resetSliceType()
{
  m_currSlice = 0;

  //---------------------------------
  int wd, ht;
  if (m_sliceType == DSlice)
    {
      m_imgWidth = m_Height;
      m_imgHeight = m_Width;
      wd = m_Height;
      ht = m_Width;
      m_maxSlice = m_Depth-1;
    }
  else if (m_sliceType == WSlice)
    {
      m_imgWidth = m_Height;
      m_imgHeight = m_Depth;
      wd = m_Height;
      ht = m_Depth;
      m_maxSlice = m_Width-1;
    }
  else
    {
      m_imgWidth = m_Width;
      m_imgHeight = m_Depth;
      wd = m_Width;
      ht = m_Depth;
      m_maxSlice = m_Height-1;
    }

  if (m_tags) delete [] m_tags;
  m_tags = new uchar[wd*ht];
  memset(m_tags, 0, wd*ht);

  if (m_prevtags) delete [] m_prevtags;
  m_prevtags = new uchar[wd*ht];
  memset(m_prevtags, 0, wd*ht);

  if (m_prevslicetags) delete [] m_prevslicetags;
  m_prevslicetags = new uchar[wd*ht];
  memset(m_prevslicetags, 0, wd*ht);

  if (m_usertags) delete [] m_usertags;
  m_usertags = new uchar[wd*ht];
  memset(m_usertags, 0, wd*ht);

  if (m_slice) delete [] m_slice;
  m_slice = new uchar[wd*ht*m_bytesPerVoxel];

  if (m_maskslice) delete [] m_maskslice;
  m_maskslice = new uchar[wd*ht];
  
  if (m_sliceImage) delete [] m_sliceImage;
  m_sliceImage = new uchar[4*wd*ht];

  if (m_labels) delete [] m_labels;
  m_labels = new int[wd*ht];
  for(int i=0; i<wd*ht; i++)
    m_labels[i] = -1;
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
  
  getSlice();
}

void
ImageWidget::updateTagColors()
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
	m_tagColors[i] = qRgba(r, g, b, 50);
    }


  m_maskimage.setColorTable(m_tagColors);
  m_maskimageScaled = m_maskimage.scaled(m_simgWidth,
					 m_simgHeight,
					 Qt::IgnoreAspectRatio,
					 Qt::FastTransformation);  

  m_userimage.setColorTable(m_tagColors);
  m_userimageScaled = m_userimage.scaled(m_simgWidth,
					 m_simgHeight,
					 Qt::IgnoreAspectRatio,
					 Qt::FastTransformation);  

  m_prevslicetagimage.setColorTable(m_prevslicetagColors);
  m_prevslicetagimageScaled = m_prevslicetagimage.scaled(m_simgWidth,
							 m_simgHeight,
							 Qt::IgnoreAspectRatio,
							 Qt::FastTransformation);  
  update();
}

void
ImageWidget::setMaskImage(uchar *mask)
{
  memcpy(m_prevslicetags, m_prevtags, m_imgWidth*m_imgHeight);
  processPrevSliceTags();

  memcpy(m_maskslice, mask, m_imgWidth*m_imgHeight);
  memcpy(m_prevtags, mask, m_imgWidth*m_imgHeight);

  updateMaskImage();
  update();

  if (m_applyRecursive)
    repaint();
}

void
ImageWidget::processPrevSliceTags()
{
  if (!Global::copyPrev())
    return;

  bool ok = false;
  for (int i=0; i<m_imgHeight*m_imgWidth; i++)
    {
      if (m_prevslicetags[i] == Global::tag())
	{
	  ok = true;
	  break;
	}
    }

  if (!ok)
    {
      memset(m_prevslicetags, 0, m_imgHeight*m_imgWidth); // reset any other tags 
      return; // no need to continue
    }

  uchar *maskData = new uchar[m_imgWidth*m_imgHeight];
  memset(maskData, 0, m_imgWidth*m_imgHeight);

  for (int i=0; i<m_imgHeight*m_imgWidth; i++)
    m_prevslicetags[i] = (m_prevslicetags[i] == Global::tag() ? 255 : 0);


  int nb = Global::prevErode();

  uchar *t1 = m_prevslicetags;
  uchar *t2 = maskData;
  for(int n=0; n<nb; n++)
    {
      memset(t2, 0, m_imgWidth*m_imgHeight);
      for(int j=0; j<m_imgHeight; j++)
	for(int i=0; i<m_imgWidth; i++)
	  {
	    if (t1[j*m_imgWidth+i] == 255)
	      {
		if (t1[j*m_imgWidth+qBound(0, i+1, m_imgWidth-1)] == 255 &&
		    t1[j*m_imgWidth+qBound(0, i-1, m_imgWidth-1)] == 255 &&
		    t1[qBound(0, j+1, m_imgHeight-1)*m_imgWidth + i] == 255 &&
		    t1[qBound(0, j-1, m_imgHeight-1)*m_imgWidth + i] == 255)	      
		  t2[j*m_imgWidth+i] = 255;
	      }
	  }

      uchar *tmp = t1;
      t1 = t2;
      t2 = tmp;
    }

  delete [] t2;

  for (int i=0; i<m_imgHeight*m_imgWidth; i++)
    t1[i] = (t1[i] > 192 ? Global::tag() : 0);

  m_prevslicetags = t1;
}


void
ImageWidget::recolorImage()
{
  for(int i=0; i<m_imgHeight*m_imgWidth; i++)
    {
      int idx = m_slice[i];
      if (m_bytesPerVoxel == 2)
	idx = ((ushort*)m_slice)[i];

      m_sliceImage[4*i+0] = m_lut[4*idx+0];
      m_sliceImage[4*i+1] = m_lut[4*idx+1];
      m_sliceImage[4*i+2] = m_lut[4*idx+2];
      m_sliceImage[4*i+3] = m_lut[4*idx+3];
    }

  m_image = QImage(m_sliceImage,
		   m_imgWidth,
		   m_imgHeight,
		   QImage::Format_RGB32);
  
  updateMaskImage();
}

void
ImageWidget::setRawValue(QList<int> vgt)
{
  m_vgt = vgt;
  update();
}

void
ImageWidget::resizeImage()
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
				 //Qt::SmoothTransformation);
				 Qt::FastTransformation);
  
  m_maskimageScaled = m_maskimage.scaled(m_simgWidth,
					 m_simgHeight,
					 Qt::IgnoreAspectRatio,
					 Qt::FastTransformation);

  m_userimageScaled = m_userimage.scaled(m_simgWidth,
					 m_simgHeight,
					 Qt::IgnoreAspectRatio,
					 Qt::FastTransformation);

  m_prevslicetagimageScaled = m_prevslicetagimage.scaled(m_simgWidth,
							 m_simgHeight,
							 Qt::IgnoreAspectRatio,
							 Qt::FastTransformation);


  m_spcimageScaled = m_spcimage.scaled(m_simgWidth,
				       m_simgHeight,
				       Qt::IgnoreAspectRatio,
				       Qt::FastTransformation);  
}

void
ImageWidget::resizeEvent(QResizeEvent *event)
{
  resizeImage();
  update();
}

void
ImageWidget::drawSizeText(QPainter *p, int nc, int nmc)
{  
  p->setPen(QPen(Qt::black, 1));

  QString txt;
  txt += QString("%1 %2 %3  ").arg(m_pickDepth).arg(m_pickWidth).arg(m_pickHeight);

  txt += QString("dim:(%1 %2 %3)").	      \
    arg(m_Height).arg(m_Width).arg(m_Depth);

  int hsz = m_maxHSlice-m_minHSlice+1;
  int wsz = m_maxWSlice-m_minWSlice+1;
  int dsz = m_maxDSlice-m_minDSlice+1;

  if (hsz < m_Height || wsz < m_Width || dsz < m_Depth)      
    txt += QString("[%4:%5 %6:%7 %8:%9]  subvol:(%10 %11 %12)").      \
      arg(m_minHSlice).arg(m_maxHSlice).			      \
      arg(m_minWSlice).arg(m_maxWSlice).			      \
      arg(m_minDSlice).arg(m_maxDSlice).			      \
      arg(m_maxHSlice-m_minHSlice+1).				      \
      arg(m_maxWSlice-m_minWSlice+1).				      \
      arg(m_maxDSlice-m_minDSlice+1);				      \
}

void
ImageWidget::paintEvent(QPaintEvent *event)
{
  QPainter p(this);

  int shiftModifier = QGuiApplication::keyboardModifiers() & Qt::ShiftModifier;
  int ctrlModifier = QGuiApplication::keyboardModifiers() & Qt::ControlModifier;

  p.setRenderHint(QPainter::Antialiasing);
  p.setCompositionMode(QPainter::CompositionMode_SourceOver);

  p.drawImage(0, 0, m_imageScaled);
  p.drawImage(0, 0, m_maskimageScaled);
  p.drawImage(0, 0, m_userimageScaled);

  if (m_modeType == 1)
    p.drawImage(0, 0, m_spcimageScaled);    

  if (Global::copyPrev())
    p.drawImage(0, 0, m_prevslicetagimageScaled);
  
  //----------------------------------------------------
  // draw slice positions for other 2 slice directions
  p.setPen(QPen(Qt::cyan, 0.7));
  p.drawLine(m_hline*m_zoom, 0, m_hline*m_zoom, m_simgHeight);
  p.drawLine(0, m_vline*m_zoom, m_simgWidth, m_vline*m_zoom);
  //----------------------------------------------------

  drawRubberBand(&p);
  
  if (m_pickPoint)
    drawRawValue(&p);

  if (!m_pickPoint && !m_rubberBandActive && !ctrlModifier)
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
	  p.setPen(QPen(Qt::white, 0.5));
	  //p.setBrush(QColor(50, 0, 0, 10));
	  if (m_modeType == 0) // graphcut
	    p.drawEllipse(xpos-rad,
			  ypos-rad,
			  2*rad, 2*rad);
	  else
	    {
	      p.drawLine(xpos-rad, ypos,
			 xpos+rad, ypos);
	      p.drawLine(xpos, ypos-rad,
			 xpos, ypos+rad);
	    }
	}
    }     
  if (hasFocus())
    {
      p.setPen(QPen(QColor(250, 100, 0), 2));
      p.setBrush(Qt::transparent);
      p.drawRect(rect());
    }

}

void
ImageWidget::drawRubberBand(QPainter *p)
{
  int x = m_simgWidth*m_rubberBand.left();
  int y = m_simgHeight*m_rubberBand.top();
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
      
  p->drawRect(0, 0, m_simgWidth, y);
  p->drawRect(0, bottom, m_simgWidth, m_simgHeight-bottom);

  height = height + (bottom - (y+height));
  p->drawRect(0, y, x, height);
  p->drawRect(right, y, m_simgWidth-right, height);
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
ImageWidget::drawRawValue(QPainter *p)
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
ImageWidget::applyRecursive(int key)
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
ImageWidget::checkRecursive()
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

void ImageWidget::zoom0Clicked() { setZoom(1); }
void ImageWidget::zoom9Clicked() { setZoom(-1); }
void ImageWidget::zoomUpClicked() { setZoom(m_zoom+0.1); }
void ImageWidget::zoomDownClicked() { setZoom(m_zoom-0.1); }

void
ImageWidget::graphcutModeKeyPressEvent(QKeyEvent *event)
{
  int shiftModifier = event->modifiers() & Qt::ShiftModifier;
  int ctrlModifier = event->modifiers() & Qt::ControlModifier;

  if (event->key() == Qt::Key_Space && m_modeType == 1)
    {
      genSuperPixels();
      update();
      return;
    }

  if (event->key() == Qt::Key_S &&
      (event->modifiers() & Qt::AltModifier) )
    {
      saveImage();
      return;
    } 
  else if (event->key() == Qt::Key_D) // apply dilate
    {
      if (ctrlModifier)
	{
	  emit applyMaskOperation(Global::tag(),
				  1, // smoothType dilate
				  Global::smooth());
	}
      else
	{
	  if (shiftModifier) // apply dilation for multiple slices
	    applyRecursive(event->key());
	  
	  smooth(64, false, false);
	}
    }
  else if (event->key() == Qt::Key_E) // apply erode
    {
      if (ctrlModifier)
	{
	  emit applyMaskOperation(Global::tag(),
				  2, // smoothType erode
				  Global::smooth());
	}
      else
	{
	  if (shiftModifier) // apply erosion for multiple slices
	    applyRecursive(event->key());

	  smooth(192, false, false);
	}
    }
  else if (event->key() == Qt::Key_C) // apply close operation
    {
      if (ctrlModifier)
	{
	  emit applyMaskOperation(Global::tag(),
				  3, // smoothType close
				  Global::smooth());
	}
      else
	{
	  if (shiftModifier) // apply close for multiple slices
	    applyRecursive(event->key());
	  
	  smooth(64, false, true); // first dilate
	  smooth(192, false, false); // then erode
	}
    }
  else if (event->key() == Qt::Key_O) // apply open operation
    {
      if (ctrlModifier)
	{
	  emit applyMaskOperation(Global::tag(),
				  4, // smoothType open
				  Global::smooth());
	}
      else
	{
	  if (shiftModifier) // apply open for multiple slices
	    applyRecursive(event->key());
	  
	  smooth(192, false, true); // first erode
	  smooth(64, false, false); // then dilate
	}
    }
  if (event->key() == Qt::Key_F)
    {
      if (ctrlModifier)
	{
	  QStringList dtypes;
	  dtypes << "Shrinkwrap";
	  dtypes << "Shell";
	  bool ok;
	  QString option = QInputDialog::getItem(0,
						 "Shrinkwrap",
						 "Shrinkwrap or Shell",
						 dtypes,
						 0,
						 false,
						 &ok);
	  if (!ok)
	    return;
	  
	  bool shell = false;
	  if (option == "Shell")
	    shell = true;
	  
	  int ctag = -1;
	  ctag = QInputDialog::getInt(0,
				      "Shrinkwrap/Shell",
				      QString("Connected region will be shrinkwrapped/shelled with current tag value (%1).\nSpecify tag value of connected region (-1 for connected visible region).").arg(Global::tag()),
				      -1, -1, 255, 1);
	  
	  int thickness = 1;
	  if (shell)
	    thickness = QInputDialog::getInt(0,
					     "Shell thickness",
					     "Shell thickness",
					     1, 1, 50, 1);

	  Vec bmin = Vec(m_minHSlice,
			 m_minWSlice,
			 m_minDSlice);
	  
	  Vec bmax = Vec(m_maxHSlice,
			 m_maxWSlice,
			 m_maxDSlice);

	  emit shrinkwrap(bmin, bmax,
			  Global::tag(), shell, thickness,
			  false,
			  m_pickDepth,
			  m_pickWidth,
			  m_pickHeight,
			  ctag);

//	  emit connectedRegion(m_pickDepth,
//			       m_pickWidth,
//			       m_pickHeight,
//			       bmin, bmax,
//			       Global::tag(),
//			       ctag);
	  return;
	}

      if (!m_applyRecursive) m_extraPressed = false;
      
      if (shiftModifier) // apply paint for multiple slices
	applyRecursive(event->key());
      
	shrinkwrapPaintedRegion();
    }
  else if (event->key() == Qt::Key_V)
    {
      if (!m_applyRecursive) m_extraPressed = false;
      
      if (shiftModifier) // apply paint for multiple slices
	applyRecursive(event->key());
      
      shrinkwrapVisibleRegion();
    }
  else if (event->key() == Qt::Key_S) // apply smoothing
    {
      if (ctrlModifier)
	{
	  emit applyMaskOperation(Global::tag(),
				  0, // smoothType smooth
				  Global::smooth());
	}
      else
	{
	  if (shiftModifier) // apply smoothing for multiple slices
	    applyRecursive(event->key());
	  
	  smooth(192, true, false);
	}
    }
  else if (event->key() == Qt::Key_T) // apply graphcut
    {
      if (shiftModifier) // apply graphcut for multiple slices
	applyRecursive(event->key());

      if (ctrlModifier) // apply paint for non-selected areas
	m_extraPressed = true;

      if (m_modeType == 0) applyGraphCut();
    }
  else if (event->key() == Qt::Key_P) // apply paint
    {
      if (!m_applyRecursive) m_extraPressed = false;

      if (shiftModifier) // apply paint for multiple slices
	applyRecursive(event->key());

      if (ctrlModifier) // apply paint for non-selected areas
	m_extraPressed = true;

      applyPaint(false);
    }
  else if (event->key() == Qt::Key_R) // reset slice tags to 0
    {
      if (!m_applyRecursive) m_extraPressed = false;

      if (shiftModifier) // apply reset for multiple slices
	applyRecursive(event->key());

      if (ctrlModifier) // apply paint for non-selected areas
	m_extraPressed = true;

      applyReset();
    }
  else if (event->key() == Qt::Key_Escape)
    {
      if (m_applyRecursive) // stop the recursive process
	{
	  emit saveWork();

	  m_applyRecursive = false;
	  m_extraPressed = false;
	  m_cslc = 0;
	  m_maxslc = 0;
	  m_key = 0;
	  m_forward = true;
	  QMessageBox::information(0, "", "Repeat process stopped");
	  emit saveMask();
	}
      else
	{
	  memset(m_usertags, 0, m_imgWidth*m_imgHeight);
	  memcpy(m_prevtags, m_maskslice, m_imgWidth*m_imgHeight);
	  updateMaskImage();
	  update();
	}
    }
  else if (event->key() == Qt::Key_Right)
    doAnother(1);
  else if (event->key() == Qt::Key_Left)
    doAnother(-1);
}

void
ImageWidget::keyPressEvent(QKeyEvent *event)
{
  bool processed = false;

  if (event->key() == Qt::Key_S &&
      (event->modifiers() & Qt::ControlModifier) )
    {
      emit saveWork();
      return;
    }

  graphcutModeKeyPressEvent(event);
}

bool
ImageWidget::checkRubberBand(int xpos, int ypos)
{
  m_rubberXmin = false;
  m_rubberYmin = false;
  m_rubberXmax = false;
  m_rubberYmax = false;
  m_rubberNew = false;
  m_rubberBandActive = false;

  if (xpos >= 0 && xpos <= m_simgWidth &&
      ypos >= 0 && ypos <= m_simgHeight)
    {
      int rxmin = m_simgWidth*m_rubberBand.left();
      int rxmax = m_simgWidth*m_rubberBand.right();
      int rymin = m_simgHeight*m_rubberBand.top();
      int rymax = m_simgHeight*m_rubberBand.bottom();
      
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
	  
	  float frcX = (float)xpos/(float)m_simgWidth;
	  float frcY = (float)ypos/(float)m_simgHeight;	  
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
ImageWidget::mouseDoubleClickEvent(QMouseEvent *event)
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
      m_minHSlice = qBound(0, (int)(left*m_Height), m_Height-1);
      m_maxHSlice = qBound(0, (int)(right*m_Height), m_Height-1);
      m_minWSlice = qBound(0, (int)(top*m_Width), m_Width-1);
      m_maxWSlice = qBound(0, (int)(bottom*m_Width), m_Width-1);
    }
  else if (m_sliceType == WSlice)
    {
      m_minHSlice = qBound(0, (int)(left*m_Height), m_Height-1);
      m_maxHSlice = qBound(0, (int)(right*m_Height), m_Height-1);
      m_minDSlice = qBound(0, (int)(top*m_Depth), m_Depth-1);
      m_maxDSlice = qBound(0, (int)(bottom*m_Depth), m_Depth-1);
    }
  else
    {
      m_minWSlice = qBound(0, (int)(left*m_Width), m_Width-1);
      m_maxWSlice = qBound(0, (int)(right*m_Width), m_Width-1);
      m_minDSlice = qBound(0, (int)(top*m_Depth), m_Depth-1);
      m_maxDSlice = qBound(0, (int)(bottom*m_Depth), m_Depth-1);
    }

  emit updateViewerBox(m_minDSlice, m_maxDSlice,
		       m_minWSlice, m_maxWSlice,
		       m_minHSlice, m_maxHSlice);

  update();
}

void
ImageWidget::graphcutMousePressEvent(QMouseEvent *event)
{

  QPoint pp = event->pos();
  float ypos = pp.y();
  float xpos = pp.x();
  
//  m_cursorPos = pp;
//  m_pickPoint = false;
//
//  if (!validPickPoint(xpos, ypos))
//    return;
//  
//  m_lastPickDepth = m_pickDepth;
//  m_lastPickWidth = m_pickWidth;
//  m_lastPickHeight= m_pickHeight;

  bool shiftModifier = event->modifiers() & Qt::ShiftModifier;
  bool ctrlModifier = event->modifiers() & Qt::ControlModifier;
  bool altModifier = event->modifiers() & Qt::AltModifier;

  if (m_button == Qt::LeftButton)
    {
//      if (altModifier)
//	{
//	  checkRubberBand(xpos, ypos);
//	  return;
//	}
      // carry on only if Alt key is not pressed
      if (m_sliceType == DSlice)
	dotImage(m_pickHeight,
		 m_pickWidth,
		 shiftModifier);
      else if (m_sliceType == WSlice)
	dotImage(m_pickHeight,
		 m_pickDepth,
		 shiftModifier);
      else
	dotImage(m_pickWidth,
		 m_pickDepth,
		 shiftModifier);      
    }
  else if (m_button == Qt::RightButton)
    {
      if (shiftModifier)
	{
	  m_pickPoint = true;
	  emit getRawValue(m_pickDepth,
			   m_pickWidth,
			   m_pickHeight);
	}
      else
	{
	  // update user mask image
	  if (m_sliceType == DSlice)
	    removeDotImage(m_pickHeight,
			   m_pickWidth);
	  else if (m_sliceType == WSlice)
	    removeDotImage(m_pickHeight,
			   m_pickDepth);
	  else
	    removeDotImage(m_pickWidth,
			   m_pickDepth);
	}
    }
}

void
ImageWidget::superpixelEvent()
{
  if (m_button == Qt::MiddleButton)
    return;

  bool paint = true;
  if (m_button == Qt::RightButton)
    paint = false;

  if (m_sliceType == DSlice)
    applySuperPixels(m_pickHeight,
		     m_pickWidth,
		     paint);
  else if (m_sliceType == WSlice)
    applySuperPixels(m_pickHeight,
		     m_pickDepth,
		     paint);
  else
    applySuperPixels(m_pickWidth,
		     m_pickDepth,
		     paint);      
  
  update();
}

void
ImageWidget::mousePressEvent(QMouseEvent *event)
{
  m_button = event->button();

  QPoint pp = event->pos();
  float ypos = pp.y();
  float xpos = pp.x();
  
  m_cursorPos = pp;
  m_pickPoint = false;

  if (!validPickPoint(xpos, ypos))
    return;
  
  m_lastPickDepth = m_pickDepth;
  m_lastPickWidth = m_pickWidth;
  m_lastPickHeight= m_pickHeight;

  bool ctrlModifier = event->modifiers() & Qt::ControlModifier;
  if (ctrlModifier)
    {
      m_vline = ypos/m_zoom;
      m_hline = xpos/m_zoom;
      emit xPos(m_hline);
      emit yPos(m_vline);
    }

  bool altModifier = event->modifiers() & Qt::AltModifier;

  if (m_button == Qt::LeftButton)
    {
      if (altModifier)
	{
	  checkRubberBand(xpos, ypos);
	  return;
	}
    }

  if (!ctrlModifier)
    {
      if (m_modeType == 0) // graphcut
	graphcutMousePressEvent(event);
      else // superpixels
	superpixelEvent();
    }

  update();
}

void
ImageWidget::graphcutMouseMoveEvent(QMouseEvent *event)
{
  QPoint pp = event->pos();
  float ypos = pp.y();
  float xpos = pp.x();

  bool shiftModifier = event->modifiers() & Qt::ShiftModifier;
  bool altModifier = event->modifiers() & Qt::AltModifier;

  if (event->buttons() == Qt::NoButton)
    {
      m_pickPoint = false;
      preselect();
    }
  else if (m_button == Qt::LeftButton)
    {
//      if (m_rubberBandActive)
//	{
//	  updateRubberBand(xpos, ypos);
//	  update();
//	  return;
//	}
//      if (altModifier)
//	return;

      // carry on only if Alt key is not pressed
      if (validPickPoint(xpos, ypos))
	{
	  //m_lastPickDepth = m_pickDepth;
	  //m_lastPickWidth = m_pickWidth;
	  //m_lastPickHeight= m_pickHeight;

	  if (m_sliceType == DSlice)
	    dotImage(m_pickHeight,
		     m_pickWidth,
		     shiftModifier);
	  else if (m_sliceType == WSlice)
	    dotImage(m_pickHeight,
		     m_pickDepth,
		     shiftModifier);
	  else
	    dotImage(m_pickWidth,
		     m_pickDepth,
		     shiftModifier);

	  m_lastPickDepth = m_pickDepth;
	  m_lastPickWidth = m_pickWidth;
	  m_lastPickHeight= m_pickHeight;

	  update();
	}
    }
  else if (m_button == Qt::RightButton)
    {
      if (validPickPoint(xpos, ypos))
	{
	  m_lastPickDepth = m_pickDepth;
	  m_lastPickWidth = m_pickWidth;
	  m_lastPickHeight= m_pickHeight;

	  if (!shiftModifier)
	    {
	      if (m_sliceType == DSlice)
		removeDotImage(m_pickHeight,
			       m_pickWidth);
	      else if (m_sliceType == WSlice)
		removeDotImage(m_pickHeight,
			       m_pickDepth);
	      else
		removeDotImage(m_pickWidth,
			       m_pickDepth);
	    }
	  else
	    {
	      m_pickPoint = true;
	      emit getRawValue(m_pickDepth,
			       m_pickWidth,
			       m_pickHeight);
	    }
	  update();
	}
    }
}

void
ImageWidget::mouseMoveEvent(QMouseEvent *event)
{
//  if (!hasFocus())
//    setFocus();

  QPoint pp = event->pos();
  float ypos = pp.y();
  float xpos = pp.x();

  m_cursorPos = pp;
  
  bool ctrlModifier = event->modifiers() & Qt::ControlModifier;

  if (ctrlModifier)
    {
      if (validPickPoint(xpos, ypos))
	{
	  m_vline = ypos/m_zoom;
	  m_hline = xpos/m_zoom;
	  emit xPos(m_hline);
	  emit yPos(m_vline);
	  update();
	}
      return;
    }


  bool altModifier = event->modifiers() & Qt::AltModifier;
  if (event->buttons() == Qt::NoButton)
    {
      m_pickPoint = false;
      preselect();
      return;
    }
  else if (m_button == Qt::LeftButton)
    {
      if (m_rubberBandActive)
	{
	  updateRubberBand(xpos, ypos);
	  update();
	  return;
	}
      if (altModifier)
	return;
    }


  if (m_modeType == 0) // graphcut
    graphcutMouseMoveEvent(event);
  else // superpixels
    superpixelEvent();
}

void
ImageWidget::mouseReleaseEvent(QMouseEvent *event)
{
  m_button = Qt::NoButton;
  m_pickPoint = false;

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
ImageWidget::doAnother(int step)
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

  m_currSlice += step;
  m_currSlice = qBound(minS, m_currSlice, maxS);

  emit sliceChanged(m_currSlice);

  getSlice();
}

void
ImageWidget::wheelEvent(QWheelEvent *event)
{
  int numSteps = event->delta()/8.0f/15.0f;
  doAnother(-numSteps);
}

void
ImageWidget::updateRubberBand(int xpos, int ypos)
{
  if (m_rubberNew)
    {
      float frcX = (float)xpos/(float)m_simgWidth;
      float frcY = (float)ypos/(float)m_simgHeight;	  
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
      float frcX = (float)xpos/(float)m_simgWidth;
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
      float frcX = (float)xpos/(float)m_simgWidth;
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
      float frcY = (float)ypos/(float)m_simgHeight;	  
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
      float frcY = (float)ypos/(float)m_simgHeight;	  
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
ImageWidget::validPickPoint(int xpos, int ypos)
{
  if (! m_slice)
    return false;

  if (xpos >= 0 && xpos <= m_simgWidth &&
      ypos >= 0 && ypos <= m_simgHeight)
    {
      // give some slack(2px) at the ends
      float frcX = (float)xpos/(float)m_simgWidth;
      float frcY = (float)ypos/(float)m_simgHeight;
      
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
ImageWidget::preselect()
{
  float ypos = m_cursorPos.y();
  float xpos = m_cursorPos.x();

  if (validPickPoint(xpos, ypos))
    update();
}

void
ImageWidget::processCommands(QString cmd)
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
ImageWidget::withinBounds(int scrx, int scry)
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
ImageWidget::dotImage(int x, int y, bool backgroundTag)
{
  if (! withinBounds(x, y))
    return ;
  
  int ist, ied, jst, jed;
  if (m_sliceType == DSlice)
    {
      ist = m_minHSlice;
      ied = m_maxHSlice;

      jst = m_minWSlice;
      jed = m_maxWSlice;
    }
  else if (m_sliceType == WSlice)
    {
      ist = m_minHSlice;
      ied = m_maxHSlice;

      jst = m_minDSlice;
      jed = m_maxDSlice;
    }
  else
    {
      ist = m_minWSlice;
      ied = m_maxWSlice;

      jst = m_minDSlice;
      jed = m_maxDSlice;
    }

  //------------------------------

  int xstart = qMax(ist, x-Global::spread());
  int xend = qMin(ied, x+Global::spread());
  int ystart = qMax(jst, y-Global::spread());
  int yend = qMin(jed, y+Global::spread());

  int tg = Global::tag();
  if (backgroundTag)
    tg = 255;
  uchar r = Global::tagColors()[4*tg+0];
  uchar g = Global::tagColors()[4*tg+1];
  uchar b = Global::tagColors()[4*tg+2];

  bool redo = false;
  for(int i=xstart; i<xend; i++)
    for(int j=ystart; j<yend; j++)
      {
	int xi = qAbs(i-x);
	int yj = qAbs(j-y);
	float frc = ((float)(xi*xi + yj*yj)/
		     (float)(Global::spread()*Global::spread()));
	if (frc < 1)
	  {
	    int sidx = j*m_imgWidth + i;
	    if (m_sliceImage[4*sidx+3] > 0)
	      {
		redo = true;
		m_usertags[sidx] = tg;
	      }
	  }
      }

  if (redo)
    {
      m_userimage = QImage(m_usertags,
			   m_imgWidth,
			   m_imgHeight,
			   m_imgWidth,
			   QImage::Format_Indexed8);
      m_userimage.setColorTable(m_tagColors);
      m_userimageScaled = m_userimage.scaled(m_simgWidth,
					     m_simgHeight,
					     Qt::IgnoreAspectRatio,
					     Qt::FastTransformation);
    }
  //updateMaskImage();
}

void
ImageWidget::removeDotImage(int x, int y)
{
  if (! withinBounds(x, y))
    return ;
		    
  int xstart = qMax(0, x-Global::spread());
  int xend = qMin(m_imgWidth, x+Global::spread());
  int ystart = qMax(0, y-Global::spread());
  int yend = qMin(m_imgHeight, y+Global::spread());

  bool redo = false;
  for(int i=xstart; i<xend; i++)
    for(int j=ystart; j<yend; j++)
      {
	int xi = qAbs(i-x);
	int yj = qAbs(j-y);
	float frc = ((float)(xi*xi + yj*yj)/
		     (float)(Global::spread()*Global::spread()));
	if (frc < 1)
	  {
	    int sidx = j*m_imgWidth + i;
	    if (m_sliceImage[4*sidx+3] > 0)
	      {
		redo = true;
		m_usertags[sidx] = 0;
	      }
	  }
      }

  if (redo)
    {
      m_userimage = QImage(m_usertags,
			   m_imgWidth,
			   m_imgHeight,
			   m_imgWidth,
			   QImage::Format_Indexed8);
      m_userimage.setColorTable(m_tagColors);
      m_userimageScaled = m_userimage.scaled(m_simgWidth,
					     m_simgHeight,
					     Qt::IgnoreAspectRatio,
					     Qt::FastTransformation);
    }
//  updateMaskImage();
}

void
ImageWidget::getSliceLimits(int &size1, int &size2,
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
ImageWidget::genSuperPixels()
{
  uchar *imageData = new uchar[m_imgWidth*m_imgHeight];

  for(int i=0; i<m_imgWidth*m_imgHeight; i++)
    imageData[i] = m_sliceImage[4*i+0];

  int size1, size2;
  int imin, imax, jmin, jmax;
  getSliceLimits(size1, size2, imin, imax, jmin, jmax);

  int idx=0;
  for(int i=imin; i<=imax; i++)
    for(int j=jmin; j<=jmax; j++)
      {
	imageData[idx] = imageData[i*m_imgWidth+j];
	idx++;
      }

  ushort *sdata = new ushort[size1*size2];
  for(int i=0; i<size1*size2; i++)
    sdata[i] = imageData[i];


  delete [] imageData;

  int K = (size1*size2)/(Global::lambda()*100);

  m_slic.PerformSLICO_ForGivenK(sdata,
				size1,
				size2,
				K);

  int* labels = m_slic.getLabels();
  //----------------------
  // copy labels

  for(int i=0; i<m_imgWidth*m_imgHeight; i++)
    m_labels[i] = -1;

  idx=0;
  for(int i=imin; i<=imax; i++)
    for(int j=jmin; j<=jmax; j++)
      {
	int ibdx = i*m_imgWidth+j;
	m_labels[ibdx] = labels[idx];
	idx++;
      }
  //----------------------

  m_slic.DrawContoursAroundSegmentsTwoColors(sdata,
					     labels,
					     size1,
					     size2);

  m_spcimage = QImage(m_imgWidth,
		      m_imgHeight,
		      QImage::Format_ARGB32);
  m_spcimage.fill(0);


  idx=0;
  uchar *bits = m_spcimage.bits();
  for(int i=imin; i<=imax; i++)
    for(int j=jmin; j<=jmax; j++)
      {
	if (sdata[idx] > 0)
	  {
	    int ibdx = i*m_imgWidth+j;
	    bits[4*ibdx+0] = 0;
	    bits[4*ibdx+1] = 0;
	    bits[4*ibdx+2] = 250;
	    bits[4*ibdx+3] = 255;
	  }
	idx++;
      }

  delete [] sdata;

  m_spcimageScaled = m_spcimage.scaled(m_simgWidth,
				       m_simgHeight,
				       Qt::IgnoreAspectRatio,
				       Qt::FastTransformation);  
}

void
ImageWidget::applySuperPixels(int x, int y, bool paint)
{
  if (! withinBounds(x, y))
    return ;

  if (m_labels[y*m_imgWidth + x] < 0)
    return;
  
  int size1, size2;
  int imin, imax, jmin, jmax;
  getSliceLimits(size1, size2, imin, imax, jmin, jmax);

  if (y<imin || y>imax ||
      x<jmin || x>jmax)
    return;
  
  QBitArray bitmask;
  bitmask.resize(m_imgWidth*m_imgHeight);
  bitmask.fill(false);

  int lbl = m_labels[y*m_imgWidth+x];
  QQueue<QPoint> que;
  que << QPoint(x,y);
  bitmask.setBit(y*m_imgWidth+x);

  uchar ptag = Global::tag();
  if (!paint)
    ptag = 0;

  m_usertags[y*m_imgWidth+x] = ptag;
  
  const int dx4[4] = {-1,  0,  1,  0};
  const int dy4[4] = { 0, -1,  0,  1};
  while (!que.isEmpty())
    {
      QPoint ij = que.dequeue();
      int i = ij.x();
      int j = ij.y();
      for( int n = 0; n < 4; n++ )
	{
	  int a = i + dx4[n];
	  int b = j + dy4[n];
	  int idx = b*m_imgWidth + a;
	  if( (a >= 0 && a < m_imgWidth) &&
	      (b >= 0 && b < m_imgHeight) &&
	      m_labels[idx] == lbl &&
	      !bitmask.testBit(idx))
	    {
	      que << QPoint(a,b);
	      bitmask.setBit(idx);
	      m_usertags[idx] = ptag;
	    }
	}
    }

  m_userimage = QImage(m_usertags,
		       m_imgWidth,
		       m_imgHeight,
		       m_imgWidth,
		       QImage::Format_Indexed8);
  m_userimage.setColorTable(m_tagColors);
  m_userimageScaled = m_userimage.scaled(m_simgWidth,
					 m_simgHeight,
					 Qt::IgnoreAspectRatio,
					 Qt::FastTransformation);
}

void
ImageWidget::applyGraphCut()
{
  uchar *imageData = new uchar[m_imgWidth*m_imgHeight];
  uchar *maskData = new uchar[m_imgWidth*m_imgHeight];


  for(int i=0; i<m_imgWidth*m_imgHeight; i++)
    imageData[i] = m_sliceImage[4*i+0];


  memset(maskData, 0, m_imgWidth*m_imgHeight);
  for(int i=0; i<m_imgWidth*m_imgHeight; i++)
    {
      if (m_prevtags[i] > 0) // set as background so that we don't overwrite it
	maskData[i] = 255;

      if (m_prevtags[i] == Global::tag()) // add seed points
	maskData[i] = Global::tag();
    }
  for(int i=0; i<m_imgWidth*m_imgHeight; i++) // overwrite with usertags
    {
      if (m_usertags[i] > 0)
	maskData[i] = m_usertags[i];
    }

  if (Global::copyPrev())
    {
      for(int i=0; i<m_imgWidth*m_imgHeight; i++) // apply prevslicetags
	{
	  if (maskData[i] == 0)
	    maskData[i] = m_prevslicetags[i];
	}
    }

  int size1, size2;
  int imin, imax, jmin, jmax;
  getSliceLimits(size1, size2, imin, imax, jmin, jmax);

  int idx=0;
  for(int i=imin; i<=imax; i++)
    for(int j=jmin; j<=jmax; j++)
      {
	maskData[idx] = maskData[i*m_imgWidth+j];
	idx++;
      }

  idx=0;
  for(int i=imin; i<=imax; i++)
    for(int j=jmin; j<=jmax; j++)
      {
	imageData[idx] = imageData[i*m_imgWidth+j];
	idx++;
      }

  MaxFlowMinCut mfmc;
  memset(m_tags, 0, size1*size2);
  int tagged = mfmc.run(size1, size2,
			Global::boxSize(),
			Global::lambda()*0.1,
			Global::tagSimilar(), // tag similar looking features
			imageData, maskData,
			Global::tag(), m_tags);

  memcpy(maskData, m_tags, size1*size2);
  memset(m_tags, 0, m_imgWidth*m_imgHeight);

  idx=0;
  for(int i=imin; i<=imax; i++)
    for(int j=jmin; j<=jmax; j++)
      {
	m_tags[i*m_imgWidth+j] = maskData[idx];
	idx++;
      }

  
  for(int i=0; i<m_imgWidth*m_imgHeight; i++)
    {
      if (m_tags[i] == 0)
	m_tags[i] = m_prevtags[i];
    }

  delete [] imageData;
  delete [] maskData;

  
  if (m_sliceType == DSlice)
    emit tagDSlice(m_currSlice, m_tags);
  else if (m_sliceType == WSlice)
    emit tagWSlice(m_currSlice, m_tags);
  else if (m_sliceType == HSlice)
    emit tagHSlice(m_currSlice, m_tags);

  setMaskImage(m_tags);

  checkRecursive();
}

void
ImageWidget::smooth(int thresh, bool smooth, bool morecoming)
{
  if (Global::smooth() == 0)
    return;

  int nb = Global::smooth();

  int size1, size2;
  int imin, imax, jmin, jmax;
  getSliceLimits(size1, size2, imin, imax, jmin, jmax);

  uchar *maskData = new uchar[m_imgWidth*m_imgHeight];
  memset(maskData, 0, m_imgWidth*m_imgHeight);

  int idx=0;
  for(int i=imin; i<=imax; i++)
    for(int j=jmin; j<=jmax; j++)
      {
	maskData[idx] = m_prevtags[i*m_imgWidth+j];
	idx++;
      }

  for(int i=0; i<size1*size2; i++)
    maskData[i] = (maskData[i] == Global::tag() ? 255 : 0);  

  //--------------------------
  // smooth row
  for(int j=0; j<size2; j++)
    {
      for(int i=0; i<size1; i++)
	{
	  float sum = 0;
	  for(int x=-nb; x<=nb; x++)
	    sum += maskData[j*size1+qBound(0, i+x, size1-1)];
	  m_tags[j*size1+i] = sum/(2*nb+1);
	}
    }
  // followed by smooth column
  for(int i=0; i<size1; i++)
    {
      for(int j=0; j<size2; j++)
	{
	  float sum = 0;
	  for(int y=-nb; y<=nb; y++)
	    sum += m_tags[qBound(0, j+y, size2-1)*size1+i];
	  maskData[j*size1+i] = sum/(2*nb+1);
	}
    }
  memcpy(m_tags, maskData, size1*size2);
  //--------------------------

  if (smooth) // dilate first and then erode
    {
      for(int i=0; i<size1*size2; i++)
	maskData[i] = (m_tags[i] > 64  ? 255 : 0);  

      //--------------------------
      // smooth row
      for(int j=0; j<size2; j++)
	{
	  for(int i=0; i<size1; i++)
	    {
	      float sum = 0;
	      for(int x=-nb; x<=nb; x++)
		sum += maskData[j*size1+qBound(0, i+x, size1-1)];
	      m_tags[j*size1+i] = sum/(2*nb+1);
	    }
	}
      // followed by smooth column
      for(int i=0; i<size1; i++)
	{
	  for(int j=0; j<size2; j++)
	    {
	      float sum = 0;
	      for(int y=-nb; y<=nb; y++)
		sum += m_tags[qBound(0, j+y, size2-1)*size1+i];
	      maskData[j*size1+i] = sum/(2*nb+1);
	    }
	}
      memcpy(m_tags, maskData, size1*size2);
      //--------------------------
    }

  delete [] maskData;

  for(int i=0; i<size1*size2; i++)
    m_tags[i] = (m_tags[i] > thresh  ? Global::tag() : 0);  

  idx=0;
  for(int i=imin; i<=imax; i++)
    for(int j=jmin; j<=jmax; j++)
      {
	if (m_sliceImage[4*(i*m_imgWidth+j)] > 0)
	  {
	    if (m_prevtags[i*m_imgWidth+j] == 0 ||
		m_prevtags[i*m_imgWidth+j] == Global::tag())
	      m_prevtags[i*m_imgWidth+j] = m_tags[idx];
	  }
	idx++;
      }

  memcpy(m_tags, m_prevtags, m_imgWidth*m_imgHeight);

  if (!morecoming)
    {
      if (m_sliceType == DSlice)
	emit tagDSlice(m_currSlice, m_tags);
      else if (m_sliceType == WSlice)
	emit tagWSlice(m_currSlice, m_tags);
      else if (m_sliceType == HSlice)
	emit tagHSlice(m_currSlice, m_tags);
      
      setMaskImage(m_tags);

      checkRecursive();
    }
}

void
ImageWidget::applyPaint(bool keepTags)
{
  int size1, size2;
  int imin, imax, jmin, jmax;
  getSliceLimits(size1, size2, imin, imax, jmin, jmax);

  memcpy(m_tags, m_prevtags, m_imgWidth*m_imgHeight);

  if (Global::copyPrev())
    {
      for (int i=0; i<m_imgHeight*m_imgWidth; i++)
	if (m_sliceImage[4*i] > 0 &&
	    m_tags[i] == 0 &&
	    m_prevslicetags[i] > 0 && m_prevslicetags[i] < 255)
	  m_tags[i] = m_prevslicetags[i];
    }

//  if (m_curveMode || m_livewireMode)
//    {
//      // limit graphcut output within the bounding curve
//      uchar *mask = new uchar[m_imgWidth*m_imgHeight];
//      memset(mask, 0, m_imgWidth*m_imgHeight);
//      paintUsingCurves(mask);
//      // overwrite within bounding box with usertags
//      int chkval = Global::tag();
//      if (m_extraPressed) chkval = 0;
//
//      if (!keepTags)
//	{
//	  for(int i=imin; i<=imax; i++)
//	    for(int j=jmin; j<=jmax; j++)
//	      {
//		if (mask[i*m_imgWidth+j] == chkval)
//		  m_tags[i*m_imgWidth+j] = Global::tag();
//	      }
//	}
//      else
//	{
//	  for(int i=imin; i<=imax; i++)
//	    for(int j=jmin; j<=jmax; j++)
//	      {
//		if (m_tags[i*m_imgWidth+j] == 0)
//		  {
//		    if (mask[i*m_imgWidth+j] == chkval)
//		      m_tags[i*m_imgWidth+j] = Global::tag();
//		  }
//	      }
//	}
//      delete [] mask;
//    }
//  else
    {
      // overwrite within bounding box with usertags
      for(int i=imin; i<=imax; i++)
	for(int j=jmin; j<=jmax; j++)
	  {
	    if (m_sliceImage[4*(i*m_imgWidth+j)] > 0)
	      {
		if (m_usertags[i*m_imgWidth+j] == 255)
		  m_tags[i*m_imgWidth+j] = 0;
		else if (m_usertags[i*m_imgWidth+j] > 0)
		  m_tags[i*m_imgWidth+j] = m_usertags[i*m_imgWidth+j];
	      }
	  }
    }

  if (m_sliceType == DSlice)
    emit tagDSlice(m_currSlice, m_tags);
  else if (m_sliceType == WSlice)
    emit tagWSlice(m_currSlice, m_tags);
  else if (m_sliceType == HSlice)
    emit tagHSlice(m_currSlice, m_tags);

  setMaskImage(m_tags);

  checkRecursive();
}

void
ImageWidget::applyReset()
{
  memcpy(m_tags, m_prevtags, m_imgWidth*m_imgHeight);
  
  int size1, size2;
  int imin, imax, jmin, jmax;
  getSliceLimits(size1, size2, imin, imax, jmin, jmax);
  
  if (m_extraPressed)
    {
      // reset all tags within bounding box
      for(int i=imin; i<=imax; i++)
	for(int j=jmin; j<=jmax; j++)
	  m_tags[i*m_imgWidth+j] = 0;
    }
  else
    {
      // reset within bounding box
      for(int i=imin; i<=imax; i++)
	for(int j=jmin; j<=jmax; j++)
	  if (m_tags[i*m_imgWidth+j] == Global::tag())
	    m_tags[i*m_imgWidth+j] = 0;
    }
     
  if (m_sliceType == DSlice)
    emit tagDSlice(m_currSlice, m_tags);
  else if (m_sliceType == WSlice)
    emit tagWSlice(m_currSlice, m_tags);
  else if (m_sliceType == HSlice)
    emit tagHSlice(m_currSlice, m_tags);

  setMaskImage(m_tags);

  checkRecursive();
}

void
ImageWidget::updateMaskImage()
{
  memcpy(m_tags, m_prevtags, m_imgWidth*m_imgHeight);

  m_maskimage = QImage(m_tags,
		       m_imgWidth,
		       m_imgHeight,
		       m_imgWidth,
		       QImage::Format_Indexed8);
  m_maskimage.setColorTable(m_tagColors);
  m_maskimageScaled = m_maskimage.scaled(m_simgWidth,
					 m_simgHeight,
					 Qt::IgnoreAspectRatio,
					 Qt::FastTransformation);


  m_userimage = QImage(m_usertags,
		       m_imgWidth,
		       m_imgHeight,
		       m_imgWidth,
		       QImage::Format_Indexed8);
  m_userimage.setColorTable(m_tagColors);
  m_userimageScaled = m_userimage.scaled(m_simgWidth,
					 m_simgHeight,
					 Qt::IgnoreAspectRatio,
					 Qt::FastTransformation);


  m_prevslicetagimage = QImage(m_prevslicetags,
			       m_imgWidth,
			       m_imgHeight,
			       m_imgWidth,
			       QImage::Format_Indexed8);
  m_prevslicetagimage.setColorTable(m_prevslicetagColors);
  m_prevslicetagimageScaled = m_prevslicetagimage.scaled(m_simgWidth,
							 m_simgHeight,
							 Qt::IgnoreAspectRatio,
							 Qt::FastTransformation);
}

void
ImageWidget::shrinkwrapPaintedRegion()
{
  int size1, size2;
  int imin, imax, jmin, jmax;
  getSliceLimits(size1, size2, imin, imax, jmin, jmax);

  uchar *maskData = new uchar[m_imgWidth*m_imgHeight];
  memset(maskData, 0, m_imgWidth*m_imgHeight);

  int idx=0;
  for(int i=imin; i<=imax; i++)
    for(int j=jmin; j<=jmax; j++)
      {
	maskData[idx] = m_prevtags[i*m_imgWidth+j];
	idx++;
      }

  for(int i=0; i<size1*size2; i++)
    maskData[i] = (maskData[i] == Global::tag() ? 255 : 0);  

  MorphSlice ms;
  QList<QPolygonF> poly = ms.boundaryCurves(maskData, size1, size2, true);

  QImage pimg = QImage(size1, size2, QImage::Format_RGB32);
  pimg.fill(0);
  QPainter p(&pimg);
  p.setPen(QPen(Qt::white, 1));
  p.setBrush(Qt::white);

  for (int npc=0; npc<poly.count(); npc++)
    p.drawPolygon(poly[npc]);

  QRgb *rgb = (QRgb*)(pimg.bits());
  for(int i=0; i<size1*size2; i++)
    maskData[i] = (maskData[i]>0 || qRed(rgb[i])>0 ? 255 : 0);  

  idx=0;
  for(int i=imin; i<=imax; i++)
    for(int j=jmin; j<=jmax; j++)
      {
	m_prevtags[i*m_imgWidth+j] = (maskData[idx] > 0 ?
				      Global::tag() : m_tags[i*m_imgWidth+j]);
	idx++;
      }

  memcpy(m_tags, m_prevtags, m_imgWidth*m_imgHeight);

  if (m_sliceType == DSlice)
    emit tagDSlice(m_currSlice, m_tags);
  else if (m_sliceType == WSlice)
    emit tagWSlice(m_currSlice, m_tags);
  else if (m_sliceType == HSlice)
    emit tagHSlice(m_currSlice, m_tags);

  setMaskImage(m_tags);

  checkRecursive();
}

void
ImageWidget::shrinkwrapVisibleRegion()
{
  int size1, size2;
  int imin, imax, jmin, jmax;
  getSliceLimits(size1, size2, imin, imax, jmin, jmax);

  uchar *maskData = new uchar[m_imgWidth*m_imgHeight];
  memset(maskData, 0, m_imgWidth*m_imgHeight);

  int idx=0;
  for(int i=imin; i<=imax; i++)
    for(int j=jmin; j<=jmax; j++)
      {
	maskData[idx] = m_sliceImage[4*(i*m_imgWidth+j)+3];
	idx++;
      }

  for(int i=0; i<size1*size2; i++)
    maskData[i] = (maskData[i] > 0 ? 255 : 0);  

  MorphSlice ms;
  QList<QPolygonF> poly = ms.boundaryCurves(maskData, size1, size2, true);

  for (int npc=0; npc<poly.count(); npc++)
    {
      QImage pimg = QImage(size1, size2, QImage::Format_RGB32);
      pimg.fill(0);
      QPainter p(&pimg);
      p.setPen(QPen(Qt::white, 1));
      p.setBrush(Qt::white);
      p.drawPolygon(poly[npc]);
      QRgb *rgb = (QRgb*)(pimg.bits());
      for(int i=0; i<size1*size2; i++)
	maskData[i] = (maskData[i]>0 || qRed(rgb[i])>0 ? 255 : 0);  
    }

  idx=0;
  for(int i=imin; i<=imax; i++)
    for(int j=jmin; j<=jmax; j++)
      {
	m_prevtags[i*m_imgWidth+j] = (maskData[idx] > 0 ?
				      Global::tag() : m_tags[i*m_imgWidth+j]);
	idx++;
      }

  memcpy(m_tags, m_prevtags, m_imgWidth*m_imgHeight);

  if (m_sliceType == DSlice)
    emit tagDSlice(m_currSlice, m_tags);
  else if (m_sliceType == WSlice)
    emit tagWSlice(m_currSlice, m_tags);
  else if (m_sliceType == HSlice)
    emit tagHSlice(m_currSlice, m_tags);

  setMaskImage(m_tags);

  checkRecursive();
}
