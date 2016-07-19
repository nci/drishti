#include "imagewidget.h"
#include "global.h"
#include "staticfunctions.h"
#include <math.h>
#include "graphcut.h"
#include "morphslice.h"

#include <QFile>
#include <QLabel>

CurveGroup*
ImageWidget::getCg()
{
  CurveGroup *cg;
  if (m_sliceType == DSlice)
    cg = &m_dCurves;
  else if (m_sliceType == WSlice)
    cg = &m_wCurves;
  else
    cg = &m_hCurves;

  return cg;
}

void
ImageWidget::setLambda(float l)
{
  m_dCurves.setLambda(l);
  m_wCurves.setLambda(l);
  m_hCurves.setLambda(l);
}
void
ImageWidget::setSegmentLength(int l)
{
  m_dCurves.setSegmentLength(l);
  m_wCurves.setSegmentLength(l);
  m_hCurves.setSegmentLength(l);
}

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

ImageWidget::ImageWidget(QWidget *parent, QStatusBar *sb) :
  QWidget(parent)
{
  m_statusBar = sb;
  setFocusPolicy(Qt::StrongFocus);
  
  setMouseTracking(true);

  m_livewireMode = false;
  m_curveMode = false;
  m_fiberMode = false;
  m_addingCurvePoints = false;

  m_Depth = m_Width = m_Height = 0;
  m_imgHeight = 100;
  m_imgWidth = 100;
  m_simgHeight = 100;
  m_simgWidth = 100;
  m_simgX = 10;
  m_simgY = 20;


  m_livewire.reset();
  m_dCurves.reset();
  m_wCurves.reset();
  m_hCurves.reset();

  m_fibers.reset();

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

  m_image.fill(0);
  m_imageScaled.fill(0);
  m_maskimage.fill(0);
  m_maskimageScaled.fill(0);
  m_prevslicetagimage.fill(0);
  m_prevslicetagimageScaled.fill(0);
  m_userimage.fill(0);
  m_userimageScaled.fill(0);

  m_slice = 0;
  m_sliceImage = 0;
  m_maskslice = 0;
  m_tags = 0;
  m_prevtags = 0;
  m_usertags = 0;
  m_prevslicetags = 0;

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

void ImageWidget::enterEvent(QEvent *e) { setFocus(); grabKeyboard(); }
void ImageWidget::leaveEvent(QEvent *e) { clearFocus(); releaseKeyboard(); }

void ImageWidget::setScrollBars(QScrollBar *hbar,
				QScrollBar *vbar)
{
  m_hbar = hbar;
  m_vbar = vbar;
}

void
ImageWidget::showTags(QList<int> t)
{
  m_showTags = t;

  uchar *tagColors = Global::tagColors();

  m_tagColors.clear();
  m_tagColors.resize(256);
  m_tagColors[0] = qRgba(0,0,0,0);
  for(int i=1; i<256; i++)
    {
      uchar r = tagColors[4*i+0];
      uchar g = tagColors[4*i+1];
      uchar b = tagColors[4*i+2];
//      if (!m_fiberMode ||
//	  m_showTags.count() == 0 ||
//	  m_showTags[0] == -1 ||
//	  m_showTags.contains(i))
//	m_tagColors[i] = qRgba(r, g, b, 127);
//      else
//	m_tagColors[i] = qRgba(r, g, b, 20);

      uchar a = tagColors[4*i+3];
      if (a > 2)
	m_tagColors[i] = qRgba(r, g, b, 127);
      else
	m_tagColors[i] = qRgba(r, g, b, 50);
    }


  m_maskimage.setColorTable(m_tagColors);
  m_userimage.setColorTable(m_tagColors);

  update();
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
  setMinimumSize(QSize(m_imgWidth, m_imgHeight));
  if (z < 0)
    {
      QWidget *prt = (QWidget*)parent();
      int frmHeight = prt->rect().height()-50;
      int frmWidth = prt->rect().width()-50;
      float zn = qMin((float)frmWidth/m_imgWidth,
		      (float)frmHeight/m_imgHeight);
      m_zoom = qMax(0.01f, zn);
    }
  else
    m_zoom = qMax(0.01f, z);

  resizeImage();
  update();
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
ImageWidget::sliceChanged(int slc)
{
  // if we are in curve add points mode switch it off
  if (m_addingCurvePoints) endCurve();
  
  // if we are modifying livewire object freeze it before moving to another slice  
  if (!m_applyRecursive && m_livewire.seedMoveMode())
    {
      freezeModifyUsingLivewire();
      modifyUsingLivewire();
    }

  if (m_sliceType == DSlice)
    m_currSlice = slc;
  else if (m_sliceType == WSlice)
    m_currSlice = slc;
  else
    m_currSlice = slc;

  emit getSlice(m_currSlice);
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
ImageWidget::resetCurves()
{
  m_dCurves.reset();
  m_wCurves.reset();
  m_hCurves.reset();
  m_fibers.reset();
}

void
ImageWidget::setGridSize(int d, int w, int h)
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

void
ImageWidget::setSliceType(int st)
{
  m_sliceType = st;

  CurveGroup *cg = getCg();
  m_currSlice = m_minDSlice;
  emit polygonLevels(cg->polygonLevels());


  //---------------------------------
  int wd, ht;
  if (m_sliceType == DSlice)
    {
      wd = m_Height+1;
      ht = m_Width+1;
    }
  else if (m_sliceType == WSlice)
    {
      wd = m_Height+1;
      ht = m_Depth+1;
    }
  else
    {
      wd = m_Width+1;
      ht = m_Depth+1;
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
  m_slice = new uchar[2*wd*ht];

  if (m_maskslice) delete [] m_maskslice;
  m_maskslice = new uchar[wd*ht];
  
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
  
  emit getSlice(m_currSlice);
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
//      if (!m_fiberMode ||
//	  m_showTags.count() == 0 ||
//	  m_showTags[0] == -1 ||
//	  m_showTags.contains(i))
//	m_tagColors[i] = qRgba(r, g, b, 127);
//      else
//	m_tagColors[i] = qRgba(r, g, b, 20);

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
ImageWidget::setImage(uchar *slice, uchar *mask)
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

  memcpy(m_prevslicetags, m_prevtags, m_imgWidth*m_imgHeight);
  processPrevSliceTags();

  memcpy(m_slice, slice, 2*m_imgWidth*m_imgHeight);
  memcpy(m_maskslice, mask, m_imgWidth*m_imgHeight);
  memcpy(m_prevtags, mask, m_imgWidth*m_imgHeight);

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
      uchar v = m_slice[2*i];
      uchar g = m_slice[2*i+1];

      int idx = 4*(256*g + v);
      m_sliceImage[4*i+0] = m_lut[idx+0];
      m_sliceImage[4*i+1] = m_lut[idx+1];
      m_sliceImage[4*i+2] = m_lut[idx+2];
      m_sliceImage[4*i+3] = m_lut[idx+3];
    }

  //-----
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
  //-----

  m_image = QImage(m_sliceImage,
		   m_imgWidth,
		   m_imgHeight,
		   QImage::Format_RGB32);
  
  updateMaskImage();
}

void
ImageWidget::setRawValue(QList<uchar> vgt)
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
				 Qt::SmoothTransformation);
  
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

  m_gradImageScaled = m_livewire.gradientImage().scaled(m_simgWidth,
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
ImageWidget::drawSeedPoints(QPainter *p,
			    QVector<QPointF> seeds,
			    QColor seedColor)
{
  drawPoints(p, seeds, Qt::black, 4, m_pointSize);
  drawPoints(p, seeds, seedColor, 2, m_pointSize);
}

void
ImageWidget::drawPoints(QPainter *p,
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
ImageWidget::drawLivewire(QPainter *p)
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
ImageWidget::drawPoints(QPainter *p,
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

void
ImageWidget::drawFibers(QPainter *p)
{
  QVector<QVector4D> pts;
  QPoint move = QPoint(m_simgX, m_simgY);
  float sx = (float)m_simgWidth/(float)m_imgWidth;

  //-------------------------------
  // draw fiber strands
  if (m_sliceType == DSlice)
    pts = m_fibers.xyPoints(0, m_currSlice);
  else if (m_sliceType == WSlice)
    pts = m_fibers.xyPoints(1, m_currSlice);
  else if (m_sliceType == HSlice)
    pts = m_fibers.xyPoints(2, m_currSlice);

  for(int i=0; i<pts.count(); i++)
    {
      QPointF pos = pts[i].toPointF();
      pos = pos*sx + move;
      pts[i] = QVector4D(pos.x(), pos.y(), pts[i].z(), pts[i].w());
    }
  drawPoints(p, pts, Qt::black, 0);

  //-------------------------------

  //-------------------------------
  // draw fiber selected strands
  if (m_sliceType == DSlice)
    pts = m_fibers.xyPointsSelected(0, m_currSlice);
  else if (m_sliceType == WSlice)
    pts = m_fibers.xyPointsSelected(1, m_currSlice);
  else if (m_sliceType == HSlice)
    pts = m_fibers.xyPointsSelected(2, m_currSlice);

  for(int i=0; i<pts.count(); i++)
    {
      QPointF pos = pts[i].toPointF();
      pos = pos*sx + move;
      pts[i] = QVector4D(pos.x(), pos.y(), pts[i].z(), pts[i].w());
    }

  drawPoints(p, pts, Qt::black, 2);
  drawPoints(p, pts, QColor(0,0,0,100), 0);
  //-------------------------------


  //-------------------------------
  // now draw seeds
  if (m_sliceType == DSlice)
    pts = m_fibers.xySeeds(0, m_currSlice);
  else if (m_sliceType == WSlice)
    pts = m_fibers.xySeeds(1, m_currSlice);
  else if (m_sliceType == HSlice)
    pts = m_fibers.xySeeds(2, m_currSlice);

  for(int i=0; i<pts.count(); i++)
    {
      QPointF pos = pts[i].toPointF();
      pos = pos*sx + move;
      pts[i] = QVector4D(pos.x(), pos.y(), pts[i].z(), pts[i].w());
    }

  drawPoints(p, pts, Qt::black, 6);
  drawPoints(p, pts, Qt::white, 0);
  //-------------------------------


  //-------------------------------
  // now draw seeds selected fiber
  if (m_sliceType == DSlice)
    pts = m_fibers.xySeedsSelected(0, m_currSlice);
  else if (m_sliceType == WSlice)
    pts = m_fibers.xySeedsSelected(1, m_currSlice);
  else if (m_sliceType == HSlice)
    pts = m_fibers.xySeedsSelected(2, m_currSlice);

  for(int i=0; i<pts.count(); i++)
    {
      QPointF pos = pts[i].toPointF();
      pos = pos*sx + move;
      pts[i] = QVector4D(pos.x(), pos.y(), pts[i].z(), pts[i].w());
    }

  drawPoints(p, pts, Qt::black, 8);
  drawPoints(p, pts, QColor(250,0,0,250), 0);
  //-------------------------------

}

int
ImageWidget::drawCurves(QPainter *p)
{  
  QList<Curve*> curves;
  if (m_sliceType == DSlice)
    curves = m_dCurves.getCurvesAt(m_currSlice, true);
  else if (m_sliceType == WSlice)
    curves = m_wCurves.getCurvesAt(m_currSlice, true);
  else
    curves = m_hCurves.getCurvesAt(m_currSlice, true);
  
  QPoint move = QPoint(m_simgX, m_simgY);
  float sx = (float)m_simgWidth/(float)m_imgWidth;
  for(int l=0; l<curves.count(); l++)
    {
      int tag = curves[l]->tag;
      if (m_showTags.count() == 0 ||
	  m_showTags[0] == -1 ||
	  m_showTags.contains(tag))
	{
	  uchar r = Global::tagColors()[4*tag+0];
	  uchar g = Global::tagColors()[4*tag+1];
	  uchar b = Global::tagColors()[4*tag+2];
	  

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
	  if (m_addingCurvePoints && l == 0)
	    onlyline = true;
	  
	  if (curves[l]->closed && !onlyline)
	    {
	      p->setPen(QPen(QColor(r,g,b), 1));
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
	} // showtags
    }

  return curves.count();
}

void ImageWidget::setPointSize(int p) { m_pointSize = p; }

QList<QPointF>
ImageWidget::trimPointList(QList<QPointF> pl, bool switchcoord)
{
  QList<QPointF> npl;

  if (pl.count() > 2)
    {
      QList<int> yc;
      int x = pl[0].x();
      yc << pl[0].y();
      for(int i=1; i<pl.count(); i++)
	{
	  if (pl[i].x() != x)
	    {
	      if (yc.count()>1)
		{
		  std::sort(yc.begin(), yc.end());
		  npl << QPointF(x, yc[0]);
		  int pp = yc[0];
		  for(int j=1; j<yc.count()-1; j++)
		    {
		      if (qAbs(pp-yc[j]) > 1)
			npl << QPointF(x, yc[j]);
		      pp = yc[j];
		    }
		  npl << QPointF(x, yc[yc.count()-1]);
		}
	      else
		npl << QPointF(x, yc[0]);
	      x = pl[i].x();
	      yc.clear();
	    }
	  yc << pl[i].y();
	}
      if (yc.count()>1)
	{
	  std::sort(yc.begin(), yc.end());
	  npl << QPointF(x, yc[0]);
	  int pp = yc[0];
	  for(int j=1; j<yc.count()-1; j++)
	    {
	      if (qAbs(pp-yc[j]) > 1)
		npl << QPointF(x, yc[j]);
	      pp = yc[j];
	    }
	  npl << QPointF(x, yc[yc.count()-1]);
	}
      else
	npl << QPointF(x, yc[0]);
    }
  else
    npl = pl;

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
ImageWidget::drawOtherCurvePoints(QPainter *p)
{  
  QList<Curve*> curves;
  QPoint move = QPoint(m_simgX, m_simgY);
  float sx = (float)m_simgWidth/(float)m_imgWidth;
  QVector<QPointF> vptd, vptw, vpth;

  if (m_sliceType == DSlice)
    {
      QList<QPointF> ptw = m_wCurves.ypoints(m_currSlice);
      vptw = QVector<QPointF>::fromList(trimPointList(ptw, true));

      QList<QPointF> pth = m_hCurves.ypoints(m_currSlice);
      vpth = QVector<QPointF>::fromList(trimPointList(pth, false));
    }
  else if (m_sliceType == WSlice)
    {
      QList<QPointF> pth = m_hCurves.xpoints(m_currSlice);
      vpth = QVector<QPointF>::fromList(trimPointList(pth, false));

      QList<QPointF> ptd = m_dCurves.ypoints(m_currSlice);
      vptd = QVector<QPointF>::fromList(trimPointList(ptd, true));
    }    
  else
    {
      QList<QPointF> ptw = m_wCurves.xpoints(m_currSlice);
      vptw = QVector<QPointF>::fromList(trimPointList(ptw, false));

      QList<QPointF> ptd = m_dCurves.xpoints(m_currSlice);
      vptd = QVector<QPointF>::fromList(trimPointList(ptd, true));
    }

  if (vptd.count() > 0)
    {
      for(int l=0; l<vptd.count(); l++)
	vptd[l] = vptd[l]*sx + move;

      drawSeedPoints(p, vptd, Qt::red);
    }

  if (vptw.count() > 0)
    {
      for(int l=0; l<vptw.count(); l++)
	vptw[l] = vptw[l]*sx + move;

      drawSeedPoints(p, vptw, Qt::yellow);
    }

  if (vpth.count() > 0)
    {
      for(int l=0; l<vpth.count(); l++)
	vpth[l] = vpth[l]*sx + move;

      drawSeedPoints(p, vpth, Qt::cyan);
    }
}

int
ImageWidget::drawMorphedCurves(QPainter *p)
{  
  QList<Curve> curves;
  if (m_sliceType == DSlice)
    curves = m_dCurves.getMorphedCurvesAt(m_currSlice);
  else if (m_sliceType == WSlice)
    curves = m_wCurves.getMorphedCurvesAt(m_currSlice);
  else
    curves = m_hCurves.getMorphedCurvesAt(m_currSlice);

  QPoint move = QPoint(m_simgX, m_simgY);
  float sx = (float)m_simgWidth/(float)m_imgWidth;
  for(int l=0; l<curves.count(); l++)
    {
      int tag = curves[l].tag;
      if (m_showTags.count() == 0 ||
	  m_showTags[0] == -1 ||
	  m_showTags.contains(tag))
	{
	  uchar r = Global::tagColors()[4*tag+0];
	  uchar g = Global::tagColors()[4*tag+1];
	  uchar b = Global::tagColors()[4*tag+2];


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
    }

  return curves.count();
}

void
ImageWidget::paintEvent(QPaintEvent *event)
{
  QPainter p(this);

  int shiftModifier = QGuiApplication::keyboardModifiers() & Qt::ShiftModifier;
  int ctrlModifier = QGuiApplication::keyboardModifiers() & Qt::ControlModifier;

  p.setRenderHint(QPainter::Antialiasing);
  p.setCompositionMode(QPainter::CompositionMode_SourceOver);

  p.drawImage(m_simgX, m_simgY, m_imageScaled);
  p.drawImage(m_simgX, m_simgY, m_maskimageScaled);
  p.drawImage(m_simgX, m_simgY, m_userimageScaled);

  if (Global::copyPrev())
    p.drawImage(m_simgX, m_simgY, m_prevslicetagimageScaled);

  drawRubberBand(&p);
  

  if (m_fiberMode)
    drawFibers(&p);

  if (!m_fiberMode)
    {
      int nc = drawCurves(&p);
      int nmc = drawMorphedCurves(&p);
      drawSizeText(&p, nc, nmc);

      drawLivewire(&p);
      
      if (!m_livewire.seedMoveMode())
	drawOtherCurvePoints(&p);
    }


  if (m_pickPoint)
    drawRawValue(&p);

  if (!m_rubberBandActive &&
      !m_livewireMode &&
      !m_curveMode &&
      !m_fiberMode)
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

void ImageWidget::setCurve(bool b)
{
  m_curveMode = b;
  if (!m_curveMode) m_livewireMode = false;
  updateTagColors();
}

void ImageWidget::setLivewire(bool b) { m_livewireMode = b; }

void ImageWidget::setFiberMode(bool b)
{
  m_fiberMode = b;
  updateTagColors();
}

void ImageWidget::newFiber()
{
  m_fibers.newFiber();
  emit showEndFiber();
}
void ImageWidget::endFiber()
{
  m_fibers.endFiber();
  emit hideEndFiber();
  emit saveWork();
}

void
ImageWidget::freezeLivewire(bool select)
{
  if (!m_livewireMode)
    {
      //QMessageBox::information(0, "Error", "No livewire found to be transferred to curve");
      return;
    }

  if (Global::closed())
    m_livewire.freeze();

  // if propagating then renew guess curve with the currently generated livewire
  if (m_livewire.propagateLivewire())
    m_livewire.renewGuessCurve();

  QVector<QPointF> pts = m_livewire.poly();
  QVector<int> seedpos = m_livewire.seedpos();
  QVector<QPointF> seeds = m_livewire.seeds();

  if (pts.count() < 5)
    {
      //QMessageBox::information(0, "Error", "No livewire found to be transferred to curve");
      if (m_livewire.propagateLivewire())
	{
	  endLivewirePropagation();
	  m_applyRecursive = false;
	  emit saveWork();
	}
      return;
    }

  CurveGroup *cg = getCg();
  cg->deselectAll();

  cg->setPolygonAt(m_currSlice,
		   pts, seedpos,
		   Global::closed(),
		   Global::tag(),
		   Global::thickness(),
		   select, 0,
		   seeds); 
  emit polygonLevels(cg->polygonLevels());

  m_livewire.resetPoly();

  if (m_showTags.count() != 0 &&
      m_showTags[0] != -1 &&
      !m_showTags.contains(Global::tag()) &&
      !m_livewire.propagateLivewire())
    QMessageBox::information(0, "", QString("Curve with tag value %1 will not be seen because you have chosen not to show it via the Show Tags parameter").arg(Global::tag()));
  
  update(); 
}

void
ImageWidget::newPolygon(bool smooth, bool line)
{
  if (m_sliceType == DSlice)
    m_dCurves.newPolygon(m_currSlice,
			 (m_minHSlice+m_maxHSlice)/2,
			 (m_minWSlice+m_maxWSlice)/2,
			 smooth, line);
  else if (m_sliceType == WSlice)
    m_wCurves.newPolygon(m_currSlice,
			 (m_minHSlice+m_maxHSlice)/2,
			 (m_minDSlice+m_maxDSlice)/2,
			 smooth, line);
  else
    m_hCurves.newPolygon(m_currSlice,
			 (m_minWSlice+m_maxWSlice)/2,
			 (m_minDSlice+m_maxDSlice)/2,
			 smooth, line);

  CurveGroup *cg = getCg();
  emit polygonLevels(cg->polygonLevels());

  update();
}

void
ImageWidget::newEllipse()
{
  if (m_sliceType == DSlice)
    m_dCurves.newEllipse(m_currSlice,
			 (m_minHSlice+m_maxHSlice)/2,
			 (m_minWSlice+m_maxWSlice)/2);
  else if (m_sliceType == WSlice)
    m_wCurves.newEllipse(m_currSlice,
			 (m_minHSlice+m_maxHSlice)/2,
			 (m_minDSlice+m_maxDSlice)/2);
  else
    m_hCurves.newEllipse(m_currSlice,
			 (m_minWSlice+m_maxWSlice)/2,
			 (m_minDSlice+m_maxDSlice)/2);

  CurveGroup *cg = getCg();
  emit polygonLevels(cg->polygonLevels());

  update();
}

void
ImageWidget::newCurve(bool showoptions)
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
				   "Draw curve by dragging with left mouse button pressed.\nYou can also start new curve by pressing key a.");
	}
    }

  CurveGroup *cg = getCg();
  cg->newCurve(m_currSlice, Global::closed());

  m_addingCurvePoints = true;
  emit showEndCurve();

  emit polygonLevels(cg->polygonLevels());
}

void
ImageWidget::endCurve()
{
  m_addingCurvePoints = false;
  emit hideEndCurve();
  emit saveWork();
}

void
ImageWidget::deselectAll()
{
  CurveGroup *cg = getCg();
  cg->deselectAll();
  update();
}

void
ImageWidget::morphCurves()
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

  CurveGroup *cg = getCg();
  cg->morphCurves(minS, maxS);
  
  update();
}

void
ImageWidget::morphSlices()
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

  CurveGroup *cg = getCg();
  cg->morphSlices(minS, maxS);
  
  update();
}

void
ImageWidget::deleteAllCurves()
{
  QStringList items;
  items << "No";
  items << "Yes";
  bool ok;
  QString st;
  if (m_sliceType == DSlice) st = "Z";
  else if (m_sliceType == WSlice) st = "Y";
  else st = "X";  
  QString item = QInputDialog::getItem(this,
					"Delete All",
					QString("You sure you want to remove all curves for %1?").\
					arg(st),
					items, 0, false, &ok);
  if (!ok || item == "No")
    return;
					
  CurveGroup *cg = getCg();
  cg->reset();
  emit polygonLevels(cg->polygonLevels());

  emit saveWork();
  QMessageBox::information(0, "", QString("Removed all curves for %1").arg(st));
  
  update();
}

void
ImageWidget::endLivewirePropagation()
{
  m_livewire.setPropagateLivewire(false);

  CurveGroup *cg = getCg();
  cg->endAddingCurves();
  cg->deselectAll();
}

void
ImageWidget::startLivewirePropagation()
{
  m_livewire.setPropagateLivewire(true);

  if (m_sliceType == DSlice) m_dCurves.startAddingCurves();
  else if (m_sliceType == WSlice) m_wCurves.startAddingCurves();
  else m_hCurves.startAddingCurves();

//  // take curve from previous/next slice and propagate it to current slice
//  int cs = qMax(0, m_currSlice-1);
//  if (!m_forward) cs = m_currSlice+1;  

  int cs = m_currSlice;

  QList<Curve*> curves;
  if (m_sliceType == DSlice)
    curves = m_dCurves.getCurvesAt(cs);
  else if (m_sliceType == WSlice)
    curves = m_wCurves.getCurvesAt(cs);
  else
    curves = m_hCurves.getCurvesAt(cs);
  
  for(int l=0; l<curves.count(); l++)
    {
      if (curves[l]->selected)
	{
	  QVector<int> seedpos = curves[l]->seedpos;
	  QVector<QPointF> seeds;
	  if (seedpos.count() > 0)
	    {
	      for(int i=0; i<seedpos.count(); i++)
		seeds << curves[l]->pts[seedpos[i]];
	    }
	  
	  m_livewire.setGuessCurve(curves[l]->pts, seeds);
	  
  
	  //  move current slice for propagation
	  checkRecursive();	  
	  if (!m_applyRecursive)
	    emit setPropagation(false);

	  return;
	}
    }

}

void
ImageWidget::propagateLivewire()
{
  QVector<QPointF> ospts;
  // first get points from orthogonal sets
  if (m_sliceType == DSlice)
    {
      QList<QPointF> ptw = m_wCurves.ypoints(m_currSlice);
      ospts = QVector<QPointF>::fromList(trimPointList(ptw, true));

      QList<QPointF> pth = m_hCurves.ypoints(m_currSlice);
      ospts += QVector<QPointF>::fromList(trimPointList(pth, false));
    }
  else if (m_sliceType == WSlice)
    {
      QList<QPointF> pth = m_hCurves.xpoints(m_currSlice);
      ospts = QVector<QPointF>::fromList(trimPointList(pth, false));

      QList<QPointF> ptd = m_dCurves.ypoints(m_currSlice);
      ospts += QVector<QPointF>::fromList(trimPointList(ptd, true));
    }    
  else
    {
      QList<QPointF> ptw = m_wCurves.xpoints(m_currSlice);
      ospts = QVector<QPointF>::fromList(trimPointList(ptw, false));

      QList<QPointF> ptd = m_dCurves.xpoints(m_currSlice);
      ospts += QVector<QPointF>::fromList(trimPointList(ptd, true));
    }

  m_livewire.livewireFromSeeds(ospts);
  freezeLivewire(true);
  
  checkRecursive();
}

void
ImageWidget::modifyUsingLivewire()
{
  m_livewire.setSeedMoveMode(true);
  m_livewire.resetPoly();
}
      
void
ImageWidget::modifyUsingLivewire(int x, int y)
{
  if (! m_livewire.seedMoveMode())
    return;

  CurveGroup *cg = getCg();  
  int ic = cg->getActiveCurve(m_currSlice, x, y);
  if (ic == -1)
    {
      QMessageBox::information(0, "",
			       "Cannot modify curve - not a livewire curve");
      return;
    }
  Curve* c = cg->getCurvesAt(m_currSlice)[ic];
  if (c->seedpos.count() == 0)
    {
      QMessageBox::information(0, "",
			       "Cannot modify this curve - not a livewire curve");
      return;
    }

  m_livewire.setPolygonToUpdate(c->pts, c->seedpos, c->closed, c->type, c->seeds);
  cg->copyCurve(m_currSlice,  x, y);
  cg->removePolygonAt(m_currSlice, x, y);
}

void
ImageWidget::freezeModifyUsingLivewire()
{
  if (! m_livewire.seedMoveMode())
    return;
  
  m_livewire.setSeedMoveMode(false);
  QVector<QPointF> pts = m_livewire.poly();
  QVector<int> seedpos = m_livewire.seedpos();

  if (pts.count() > 0 &&
      seedpos.count() > 0)
    {
      bool closed = m_livewire.closed();
      uchar type = m_livewire.type();
      QVector<QPointF> seeds = m_livewire.seeds();

      CurveGroup *cg = getCg();
      Curve c = cg->getCopyCurve();
      cg->setPolygonAt(m_currSlice,
		       pts, seedpos,
		       closed, c.tag, c.thickness,
		       false, c.type, seeds); 
      emit polygonLevels(cg->polygonLevels());
      emit saveWork();
    }
  m_livewire.resetPoly();
}

void
ImageWidget::setSliceLOD(int lod)
{
  m_livewire.setLod(lod);
  m_gradImageScaled = m_livewire.gradientImage().scaled(m_simgWidth,
							m_simgHeight);
  update();
}

void
ImageWidget::propagateCurves(bool flag)
{
  if (flag)
    {
      applyRecursive(Qt::Key_J);
      startLivewirePropagation();
    }
  else
    {
      if (m_applyRecursive) // stop the recursive process
	{
	  endLivewirePropagation();
	  emit saveWork();

	  m_applyRecursive = false;
	  m_extraPressed = false;
	  m_cslc = 0;
	  m_maxslc = 0;
	  m_key = 0;
	  m_forward = true;
	  QMessageBox::information(0, "", "Propagation process stopped");
	}
    }
}

bool
ImageWidget::fiberModeKeyPressEvent(QKeyEvent *event)
{
  QPoint pp = mapFromGlobal(QCursor::pos());
  float ypos = pp.y();
  float xpos = pp.x();
  validPickPoint(xpos, ypos);

  int shiftModifier = event->modifiers() & Qt::ShiftModifier;
  int ctrlModifier = event->modifiers() & Qt::ControlModifier;

  if (event->key() == Qt::Key_Delete ||
      event->key() == Qt::Key_Backspace)
    {
      m_fibers.removeFiber(m_pickDepth, m_pickWidth, m_pickHeight);
      emit viewerUpdate();
      emit saveWork();
      update();
    }
  else if (event->key() == Qt::Key_Space)
    {
      m_fibers.selectFiber(m_pickDepth, m_pickWidth, m_pickHeight);
      emit viewerUpdate();
      update();
    }
  else if (event->key() == Qt::Key_T)
    {
      m_fibers.setTag(m_pickDepth, m_pickWidth, m_pickHeight);
      emit viewerUpdate();
      update();
    }
  else if (event->key() == Qt::Key_W)
    {
      m_fibers.setThickness(m_pickDepth, m_pickWidth, m_pickHeight);
      emit viewerUpdate();
      update();
    }
  else if (event->key() == Qt::Key_I)
    {
      m_fibers.showInformation(m_pickDepth, m_pickWidth, m_pickHeight);
      update();
    }

  return true;
}

bool
ImageWidget::curveModeKeyPressEvent(QKeyEvent *event)
{
  if (m_applyRecursive && event->key() == Qt::Key_Escape)
    return false;

  {
    QPoint pp = mapFromGlobal(QCursor::pos());
    float ypos = pp.y();
    float xpos = pp.x();
    validPickPoint(xpos, ypos);
  }

  int shiftModifier = event->modifiers() & Qt::ShiftModifier;
  int ctrlModifier = event->modifiers() & Qt::ControlModifier;

  if (event->key() == Qt::Key_J)
    {
      bool ar = m_applyRecursive;

      if (shiftModifier) // apply operation for multiple slices
	{
	  applyRecursive(event->key());
	  ar = true;

	  startLivewirePropagation();
	  return true;
	}

      propagateLivewire();

      if (ar && m_applyRecursive == false)
	endLivewirePropagation();

      return true;
    }

  if (m_livewireMode)
    {
      if (m_livewire.seedMoveMode())
	{
	  if (event->key() == Qt::Key_Escape)
	    {
	      m_livewire.resetPoly();
	      
	      CurveGroup *cg = getCg();
	      cg->pasteCurve(m_currSlice);
	      emit polygonLevels(cg->polygonLevels());
	      
	      cg->resetCopyCurve();
	    }
	  else if (event->key() == Qt::Key_Space)
	    {
	      freezeModifyUsingLivewire();
	      modifyUsingLivewire();
	    }
	  else if (event->key() != Qt::Key_Shift)
	    QMessageBox::information(0, "", "Cannot perform this operation in modify mode.  Please quit modify mode to perform this operation.");
	  
	  return true;
	}

      if (m_livewire.keyPressEvent(event))
	{
	  update();
	  return true;
	}

      if (event->key() == Qt::Key_Space)
	{
	  if (m_livewire.poly().count() > 5)
	    {
	      freezeLivewire(false);
	      emit saveWork();
	      return true;
	    }
	}
    }

  if (event->key() == Qt::Key_A)
    {
      if (m_addingCurvePoints)
	update();
      newCurve(false);
      emit saveWork();
      return true;
    }

  if (ctrlModifier && event->key() == Qt::Key_C)
    {
      int cc = -1;
      if (m_sliceType == DSlice)
	cc = m_dCurves.copyCurve(m_currSlice, m_pickHeight, m_pickWidth);
      else if (m_sliceType == WSlice)
	cc = m_wCurves.copyCurve(m_currSlice, m_pickHeight, m_pickDepth);
      else
	cc = m_hCurves.copyCurve(m_currSlice, m_pickWidth,  m_pickDepth);

      if (cc >= 0)
	QMessageBox::information(0, "", QString("Curve copied to buffer"));
      else
	QMessageBox::information(0, "", QString("No curve found to copy"));

      return true;
    }

  if (ctrlModifier && event->key() == Qt::Key_V)
    {
      CurveGroup *cg = getCg();
      cg->pasteCurve(m_currSlice);
      emit polygonLevels(cg->polygonLevels());      
      update();
      return true;
    }

  if (shiftModifier && event->key() == Qt::Key_Q)
    {
      morphSlices();
      return true;
    }

  if (event->key() == Qt::Key_I)
    {
      int ic = -1;
      if (m_sliceType == DSlice)
	ic = m_dCurves.showPolygonInfo(0, m_currSlice, m_pickHeight, m_pickWidth);
      else if (m_sliceType == WSlice)
	ic = m_wCurves.showPolygonInfo(1, m_currSlice, m_pickHeight, m_pickDepth);
      else
	ic = m_hCurves.showPolygonInfo(2, m_currSlice, m_pickWidth,  m_pickDepth);
      
      return true;
    }

  if (event->key() == Qt::Key_S)
    { // curve smoothing
      if (m_sliceType == DSlice)
	m_dCurves.smooth(m_currSlice, m_pickHeight, m_pickWidth,
			 shiftModifier,
			 m_minDSlice, m_maxDSlice);
      else if (m_sliceType == WSlice)
	m_wCurves.smooth(m_currSlice, m_pickHeight, m_pickDepth,
			 shiftModifier,
			 m_minWSlice, m_maxWSlice);
      else
	m_hCurves.smooth(m_currSlice, m_pickWidth,  m_pickDepth,
			 shiftModifier,
			 m_minHSlice, m_maxHSlice);
      
      emit saveWork();
      update();
      return true;
    }

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
      return true;
    }

  if (event->key() == Qt::Key_D)
    { // curve dilation
      if (m_sliceType == DSlice)
	m_dCurves.dilateErode(m_currSlice, m_pickHeight, m_pickWidth,
			      shiftModifier,
			      m_minDSlice, m_maxDSlice, 0.5);
      else if (m_sliceType == WSlice)
	m_wCurves.dilateErode(m_currSlice, m_pickHeight, m_pickDepth,
			      shiftModifier,
			      m_minWSlice, m_maxWSlice, 0.5);
      else
	m_hCurves.dilateErode(m_currSlice, m_pickWidth,  m_pickDepth,
			      shiftModifier,
			      m_minHSlice, m_maxHSlice, 0.5);
      
      emit saveWork();
      update();
      return true;
    }

  if (event->key() == Qt::Key_E)
    { // curve erosion
      if (m_sliceType == DSlice)
	m_dCurves.dilateErode(m_currSlice, m_pickHeight, m_pickWidth,
			      shiftModifier,
			      m_minDSlice, m_maxDSlice, -0.5);
      else if (m_sliceType == WSlice)
	m_wCurves.dilateErode(m_currSlice, m_pickHeight, m_pickDepth,
			      shiftModifier,
			      m_minWSlice, m_maxWSlice, -0.5);
      else
	m_hCurves.dilateErode(m_currSlice, m_pickWidth,  m_pickDepth,
			      shiftModifier,
			      m_minHSlice, m_maxHSlice, -0.5);
      
      emit saveWork();
      update();
      return true;
    }

  if (event->key() == Qt::Key_Space)
    {
      bool selected;
      if (m_sliceType == DSlice)
	selected = m_dCurves.selectPolygon(m_currSlice,
					   m_pickHeight, m_pickWidth,
					   shiftModifier,
					   m_minDSlice, m_maxDSlice);
      else if (m_sliceType == WSlice)
	selected = m_wCurves.selectPolygon(m_currSlice,
					   m_pickHeight, m_pickDepth,
					   shiftModifier,
					   m_minWSlice, m_maxWSlice);
      else
	selected = m_hCurves.selectPolygon(m_currSlice,
					   m_pickWidth,  m_pickDepth,
					   shiftModifier,
					   m_minHSlice, m_maxHSlice);
      
      if (selected)
	update();
      else
	QMessageBox::information(0, "", "No curve found for selection.\nNote : morphed curves cannot be selected.");
      return true;
    }      

  if (event->key() == Qt::Key_O)
    {
      if (m_sliceType == DSlice)
	m_dCurves.setClosed(m_currSlice, m_pickHeight, m_pickWidth, false);
      if (m_sliceType == WSlice)
	m_wCurves.setClosed(m_currSlice, m_pickHeight, m_pickDepth, false);
      else
	m_hCurves.setClosed(m_currSlice, m_pickWidth,  m_pickDepth, false);
      
      update();
      return true;
    }

  if (event->key() == Qt::Key_C)
    {
      if (m_sliceType == DSlice)
	m_dCurves.setClosed(m_currSlice, m_pickHeight, m_pickWidth, true);
      if (m_sliceType == WSlice)
	m_wCurves.setClosed(m_currSlice, m_pickHeight, m_pickDepth, true);
      else
	m_hCurves.setClosed(m_currSlice, m_pickWidth,  m_pickDepth, true);
      
      update();
      return true;
    }
 
  if (event->key() == Qt::Key_T)
    {
      QPair<int , int> swsel;
      int ic = -1;
      if (m_sliceType == DSlice)
	{
	  ic = m_dCurves.getActiveCurve(m_currSlice, m_pickHeight, m_pickWidth);
	  if (ic == -1)
	    ic = m_dCurves.getActiveMorphedCurve(m_currSlice, m_pickHeight, m_pickWidth);
	  if (ic == -1)
	    {
	      swsel = m_dCurves.getActiveShrinkwrapCurve(m_currSlice, m_pickHeight, m_pickWidth);
	      ic = swsel.first;
	    }
	}
      else if (m_sliceType == WSlice)
	{
	  ic = m_wCurves.getActiveCurve(m_currSlice, m_pickHeight, m_pickDepth);
	  if (ic == -1)
	    ic = m_wCurves.getActiveMorphedCurve(m_currSlice, m_pickHeight, m_pickDepth);
	  if (ic == -1)
	    {
	      swsel = m_wCurves.getActiveShrinkwrapCurve(m_currSlice, m_pickHeight, m_pickDepth);
	      ic = swsel.first;
	    }
	}
      else
	{
	  ic = m_hCurves.getActiveCurve(m_currSlice, m_pickWidth, m_pickDepth);
	  if (ic == -1)
	    ic = m_hCurves.getActiveMorphedCurve(m_currSlice, m_pickWidth, m_pickDepth);
	  if (ic == -1)
	    {
	      swsel = m_hCurves.getActiveShrinkwrapCurve(m_currSlice, m_pickWidth, m_pickDepth);
	      ic = swsel.first;
	    }
	}
      
      if (ic == -1)
	{
	  if (!m_applyRecursive) m_extraPressed = false;
	  
	  if (shiftModifier) // apply paint for multiple slices
	    applyRecursive(event->key());
	  
	  if (ctrlModifier) // apply paint for non-selected areas
	    m_extraPressed = true;
	  
	  applyPaint(true); // keep existing tags while painting the curves
	  return true;
	}
      else
	{
	  if (m_sliceType == DSlice)
	    m_dCurves.setTag(m_currSlice, m_pickHeight, m_pickWidth, Global::tag());
	  else if (m_sliceType == WSlice)
	    m_wCurves.setTag(m_currSlice, m_pickHeight, m_pickDepth, Global::tag());
	  else
	    m_hCurves.setTag(m_currSlice, m_pickWidth,  m_pickDepth, Global::tag());
	  
	  update();
	  return true;
	}
    }

  if (event->key() == Qt::Key_P)
    {
      if (!m_applyRecursive) m_extraPressed = false;
      
      if (shiftModifier) // apply paint for multiple slices
	applyRecursive(event->key());
      
      if (ctrlModifier) // apply paint for non-selected areas
	m_extraPressed = true;
      
      applyPaint(false); // overwrite existing tags while painting the curves
      return true;
    }

  if (event->key() == Qt::Key_W)
    {
      bool closed = shiftModifier;
      if (m_sliceType == DSlice)
	m_dCurves.setThickness(m_currSlice, m_pickHeight, m_pickWidth, Global::thickness());
      else if (m_sliceType == WSlice)
	m_wCurves.setThickness(m_currSlice, m_pickHeight, m_pickDepth, Global::thickness());
      else
	m_hCurves.setThickness(m_currSlice, m_pickWidth,  m_pickDepth, Global::thickness());
      
      update();
      return true;
    }

  if (event->key() == Qt::Key_F)
    {
      if (m_sliceType == DSlice)
	m_dCurves.flipPolygon(m_currSlice, m_pickHeight, m_pickWidth);
      else if (m_sliceType == WSlice)
	m_wCurves.flipPolygon(m_currSlice, m_pickHeight, m_pickDepth);
      else
	m_hCurves.flipPolygon(m_currSlice, m_pickWidth,  m_pickDepth);
      update();
      return true;
    }

  if (event->key() == Qt::Key_Delete ||
      event->key() == Qt::Key_Backspace)
    {
      if (m_sliceType == DSlice)
	{
	  m_dCurves.removePolygonAt(m_currSlice, m_pickHeight, m_pickWidth, shiftModifier);
	  emit polygonLevels(m_dCurves.polygonLevels());
	}
      else if (m_sliceType == WSlice)
	{
	  m_wCurves.removePolygonAt(m_currSlice, m_pickHeight, m_pickDepth, shiftModifier);
	  emit polygonLevels(m_wCurves.polygonLevels());
	}
      else
	{
	  m_hCurves.removePolygonAt(m_currSlice, m_pickWidth,  m_pickDepth, shiftModifier);
	  emit polygonLevels(m_hCurves.polygonLevels());
	}

      if (m_addingCurvePoints)
	endCurve();

      update();
      return true;
    }

  return false;
}

void ImageWidget::zoom0() { setZoom(1); }
void ImageWidget::zoom9() { setZoom(-1); }
void ImageWidget::zoomUp() { setZoom(m_zoom+0.1); }
void ImageWidget::zoomDown() { setZoom(m_zoom-0.1); }

void
ImageWidget::graphcutModeKeyPressEvent(QKeyEvent *event)
{
  int shiftModifier = event->modifiers() & Qt::ShiftModifier;
  int ctrlModifier = event->modifiers() & Qt::ControlModifier;

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

      applyGraphCut();
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
	  endLivewirePropagation();
	  endShrinkwrap();
	  emit saveWork();

	  m_applyRecursive = false;
	  m_extraPressed = false;
	  m_cslc = 0;
	  m_maxslc = 0;
	  m_key = 0;
	  m_forward = true;
	  QMessageBox::information(0, "", "Repeat process stopped");
	  if (m_curveMode)
	    emit setPropagation(false);
	  else
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

  if (m_fiberMode)
    processed = fiberModeKeyPressEvent(event);
  if (processed)
    return;

  if (m_livewireMode || m_curveMode)
    processed = curveModeKeyPressEvent(event);
  if (processed)
    return;

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
ImageWidget::fiberMousePressEvent(QMouseEvent *event)
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

  if (m_button == Qt::LeftButton && altModifier)
    {
      checkRubberBand(xpos, ypos);
      return;
    }  
  
  if (m_button == Qt::LeftButton)
    m_fibers.addPoint(m_pickDepth, m_pickWidth, m_pickHeight);
  else if (m_button == Qt::RightButton)
    m_fibers.removePoint(m_pickDepth, m_pickWidth, m_pickHeight);
  
  emit viewerUpdate();
  update();
  return;
}

void
ImageWidget::curveMousePressEvent(QMouseEvent *event)
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
      if (m_livewire.seedMoveMode())
	{
	  if (m_livewire.poly().count() == 0)
	    {
	      if (m_sliceType == DSlice)
		modifyUsingLivewire(m_pickHeight, m_pickWidth);
	      else if (m_sliceType == WSlice)
		modifyUsingLivewire(m_pickHeight, m_pickDepth);
	      else
		modifyUsingLivewire(m_pickWidth, m_pickDepth);
	    }
	}

      
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
		m_dCurves.addPoint(m_currSlice, m_pickHeight, m_pickWidth);
	      else if (m_sliceType == WSlice)
		m_wCurves.addPoint(m_currSlice, m_pickHeight, m_pickDepth);
	      else
		m_hCurves.addPoint(m_currSlice, m_pickWidth,  m_pickDepth);
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
	m_dCurves.setMoveCurve(m_currSlice, m_pickHeight, m_pickWidth);
      else if (m_sliceType == WSlice)
	m_wCurves.setMoveCurve(m_currSlice, m_pickHeight, m_pickDepth);
      else
	m_hCurves.setMoveCurve(m_currSlice, m_pickWidth,  m_pickDepth);
    }
  else if (m_button == Qt::RightButton &&
	   !m_livewireMode)
    {
      if (m_sliceType == DSlice)
	m_dCurves.removePoint(m_currSlice, m_pickHeight, m_pickWidth);
      else if (m_sliceType == WSlice)
	m_wCurves.removePoint(m_currSlice, m_pickHeight, m_pickDepth);
      else
	m_hCurves.removePoint(m_currSlice, m_pickWidth,  m_pickDepth);
    }
}


void
ImageWidget::graphcutMousePressEvent(QMouseEvent *event)
{

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

  bool shiftModifier = event->modifiers() & Qt::ShiftModifier;
  bool ctrlModifier = event->modifiers() & Qt::ControlModifier;
  bool altModifier = event->modifiers() & Qt::AltModifier;

  if (m_button == Qt::LeftButton)
    {
      if (altModifier)
	{
	  checkRubberBand(xpos, ypos);
	  return;
	}
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
ImageWidget::mousePressEvent(QMouseEvent *event)
{
  m_button = event->button();

  if (m_fiberMode)
    fiberMousePressEvent(event);
  else if (m_livewireMode || m_curveMode)
    curveMousePressEvent(event);
  else
    graphcutMousePressEvent(event);

  update();
}

void
ImageWidget::curveMouseMoveEvent(QMouseEvent *event)
{
  QPoint pp = event->pos();
  float ypos = pp.y();
  float xpos = pp.x();

  bool shiftModifier = event->modifiers() & Qt::ShiftModifier;
  bool ctrlModifier = event->modifiers() & Qt::ControlModifier;
  bool altModifier = event->modifiers() & Qt::AltModifier;

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
		    m_dCurves.addPoint(m_currSlice, m_pickHeight, m_pickWidth);
		  else if (m_sliceType == WSlice)
		    m_wCurves.addPoint(m_currSlice, m_pickHeight, m_pickDepth);
		  else
		    m_hCurves.addPoint(m_currSlice, m_pickWidth,  m_pickDepth);
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
		m_dCurves.moveCurve(m_currSlice, dh, dw);
	      else if (m_sliceType == WSlice)
		m_wCurves.moveCurve(m_currSlice, dh, dd);
	      else
		m_hCurves.moveCurve(m_currSlice, dw, dd);
	    }
	  update();
	  return;
	}
    }
}

void
ImageWidget::graphcutMouseMoveEvent(QMouseEvent *event)
{
  QPoint pp = event->pos();
  float ypos = pp.y();
  float xpos = pp.x();

  bool shiftModifier = event->modifiers() & Qt::ShiftModifier;
  bool ctrlModifier = event->modifiers() & Qt::ControlModifier;
  bool altModifier = event->modifiers() & Qt::AltModifier;

  if (event->buttons() == Qt::NoButton)
    {
      m_pickPoint = false;
      preselect();
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

      // carry on only if Alt key is not pressed
      if (validPickPoint(xpos, ypos))
	{
	  m_lastPickDepth = m_pickDepth;
	  m_lastPickWidth = m_pickWidth;
	  m_lastPickHeight= m_pickHeight;

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
  if (!hasFocus())
    setFocus();

  QPoint pp = event->pos();
  float ypos = pp.y();
  float xpos = pp.x();

  m_cursorPos = pp;
  
  bool shiftModifier = event->modifiers() & Qt::ShiftModifier;
  bool ctrlModifier = event->modifiers() & Qt::ControlModifier;
  bool altModifier = event->modifiers() & Qt::AltModifier;

  if (m_livewireMode || m_curveMode)
    curveMouseMoveEvent(event);
  else if (!m_fiberMode)
    graphcutMouseMoveEvent(event);
  else if (m_fiberMode)
    {
      if (m_rubberBandActive)
	{
	  updateRubberBand(xpos, ypos);
	  update();
	  return;
	}
    }
}

void
ImageWidget::mouseReleaseEvent(QMouseEvent *event)
{
  m_button = Qt::NoButton;
  m_pickPoint = false;

  m_livewire.mouseReleaseEvent(event);
  m_dCurves.resetMoveCurve();
  m_wCurves.resetMoveCurve();
  m_hCurves.resetMoveCurve();

  if (m_curveMode || m_livewireMode)
    {
      CurveGroup *cg = getCg();
      emit polygonLevels(cg->polygonLevels());
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
ImageWidget::doAnother(int step)
{
  // if we are in curve add points mode switch it off
  if (m_addingCurvePoints) endCurve();
  
  // if we are modifying livewire object freeze it before moving to another slice  
  if (!m_applyRecursive && m_livewire.seedMoveMode())
    {
      freezeModifyUsingLivewire();
      modifyUsingLivewire();
    }

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
ImageWidget::validPickPoint(int xpos, int ypos)
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
	      m_usertags[sidx] = tg;
	  }
      }

  updateMaskImage();
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
	      m_usertags[sidx] = 0;
	  }
      }

  updateMaskImage();
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
ImageWidget::paintUsingCurves(CurveGroup* cg,
			      int slc, int wd, int ht,
			      uchar *maskData,
			      QList<int> ptag)
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
	    int tag = curves[l]->tag;
	    if (ptag[0] <= 0 || ptag.contains(tag))
	      {
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
  }

  { // interpolated curves
    QList<Curve> curves;
    curves = cg->getMorphedCurvesAt(slc);

    if (curves.count() > 0)
      {
	for(int l=0; l<curves.count(); l++)
	  {
	    int tag = curves[l].tag;
	    if (ptag[0] <= 0 || ptag.contains(tag))
	      {
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
  }

//  QLabel *lbl = new QLabel();
//  lbl->setPixmap(QPixmap::fromImage(pimg));
//  lbl->show();

  QRgb *rgb = (QRgb*)(pimg.bits());
  for(int i=0; i<wd*ht; i++)
    maskData[i] = qRed(rgb[i]);
}

void
ImageWidget::paintUsingCurves(int slctype,
			      int slc, int wd, int ht,
			      uchar *maskData,
			      QList<int> tag)
{
  CurveGroup *cg;
  if (slctype == 0) cg = &m_dCurves;
  else if (slctype == 1) cg = &m_wCurves;
  else cg = &m_hCurves;

  paintUsingCurves(cg,
		   slc, wd, ht,
		   maskData,
		   tag);
}

void
ImageWidget::paintUsingCurves(uchar *maskData)
{
  if (!m_curveMode && !m_livewireMode)
    return;

  QList<int> gtag;
  gtag << Global::tag();
  CurveGroup *cg = getCg();  
  paintUsingCurves(cg,
		   m_currSlice, m_imgWidth, m_imgHeight,
		   maskData,
		   gtag);
}

void
ImageWidget::applyGraphCut()
{
  uchar *imageData = new uchar[m_imgWidth*m_imgHeight];
  uchar *maskData = new uchar[m_imgWidth*m_imgHeight];

  for(int i=0; i<m_imgWidth*m_imgHeight; i++)
    imageData[i] = m_sliceImage[4*i+0];

  //----------------------------------------
  //----------------------------------------
//// smooth imageData
//  for(int i=0; i<m_imgHeight; i++)
//    for(int j=0; j<m_imgWidth; j++)
//      {	
//	int bs = 1;
//	int imin,imax,jmin,jmax;
//	imin = qMax(0, i-bs);
//	imax = qMin(m_imgHeight-1, i+bs);
//	jmin = qMax(0, j-bs);
//	jmax = qMin(m_imgWidth-1, j+bs);
//	float sum = 0;
//	for(int ii=imin; ii<=imax; ii++)
//	  for(int jj=jmin; jj<=jmax; jj++)
//	    sum += imageData[ii*m_imgWidth+jj];
//	
//	maskData[i*m_imgWidth+j] = sum/((jmax-jmin+1)*(imax-imin+1));
//      }
//  memcpy(imageData, maskData, m_imgWidth*m_imgHeight);
  //----------------------------------------
  //----------------------------------------

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
  m_statusBar->showMessage(QString("No. of voxels tagged : %1").arg(tagged));

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
//  for(int i=0; i<m_imgWidth*m_imgHeight; i++) // overwrite with usertags
//    {
//      if (m_usertags[i] == 255)
//	m_tags[i] = 0;
//      else if (m_usertags[i] > 0)
//	m_tags[i] = m_usertags[i];
//    }
 
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

  if (m_curveMode || m_livewireMode)
    {
      // limit graphcut output within the bounding curve
      uchar *mask = new uchar[m_imgWidth*m_imgHeight];
      memset(mask, 0, m_imgWidth*m_imgHeight);
      paintUsingCurves(mask);
      // overwrite within bounding box with usertags
      int chkval = Global::tag();
      if (m_extraPressed) chkval = 0;

      if (!keepTags)
	{
	  for(int i=imin; i<=imax; i++)
	    for(int j=jmin; j<=jmax; j++)
	      {
		if (mask[i*m_imgWidth+j] == chkval)
		  m_tags[i*m_imgWidth+j] = Global::tag();
	      }
	}
      else
	{
	  for(int i=imin; i<=imax; i++)
	    for(int j=jmin; j<=jmax; j++)
	      {
		if (m_tags[i*m_imgWidth+j] == 0)
		  {
		    if (mask[i*m_imgWidth+j] == chkval)
		      m_tags[i*m_imgWidth+j] = Global::tag();
		  }
	      }
	}
      delete [] mask;
    }
  else
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
//  if (all)
//    memset(m_tags, 0, m_imgWidth*m_imgHeight);
//  else
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
//  for(int i=0; i<m_imgWidth*m_imgHeight; i++)
//    if (m_usertags[i] > 0)
//      m_tags[i] = m_usertags[i];

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
ImageWidget::setSmoothType(int i)
{
  m_livewire.setSmoothType(i);
 
  // transfer data for livewire calculation
  uchar *sliceData = new uchar[m_imgHeight*m_imgWidth];
  for(int i=0; i<m_imgHeight*m_imgWidth; i++)
    sliceData[i] = m_sliceImage[4*i];
    //sliceData[i] = m_slice[2*i];
  m_livewire.setImageData(m_imgWidth, m_imgHeight, sliceData);
  delete [] sliceData;
  m_gradImageScaled = m_livewire.gradientImage().scaled(m_simgWidth,
							m_simgHeight,
							Qt::IgnoreAspectRatio,
							Qt::FastTransformation);
}

void
ImageWidget::setGradType(int i)
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
ImageWidget::saveCurveData(QFile *cfile, int key, Curve *c)
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
ImageWidget::loadCurveData(QFile *cfile)
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
ImageWidget::saveMorphedCurves(QFile *cfile, CurveGroup *cg)
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
ImageWidget::saveShrinkwrapCurves(QFile *cfile, CurveGroup *cg)
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
ImageWidget::saveCurves(QFile *cfile, CurveGroup *cg)
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
ImageWidget::saveFibers(QFile *cfile)
{
  char keyword[100];

  memset(keyword, 0, 100);
  sprintf(keyword, "fibergroupstart\n");
  cfile->write((char*)keyword, strlen(keyword));


  QList<Fiber*>* fibers = m_fibers.fibers();
  int nfibers = fibers->count();
  for(int i=0; i<nfibers; i++)
    {
      Fiber *fb = fibers->at(i);
      int tag = fb->tag;
      int thickness = fb->thickness;
      QVector<Vec> seeds = fb->seeds;
      
      if (seeds.count() > 0) // save only if we have non-empty fiber
	{
	  memset(keyword, 0, 100);
	  sprintf(keyword, "fiberstart\n");
	  cfile->write((char*)keyword, strlen(keyword));

	  memset(keyword, 0, 100);
	  sprintf(keyword, "tag\n");
	  cfile->write((char*)keyword, strlen(keyword));
	  cfile->write((char*)&tag, sizeof(int));
  
	  memset(keyword, 0, 100);
	  sprintf(keyword, "thickness\n");
	  cfile->write((char*)keyword, strlen(keyword));
	  cfile->write((char*)&thickness, sizeof(int));

	  memset(keyword, 0, 100);
	  sprintf(keyword, "seeds\n");
	  cfile->write((char*)keyword, strlen(keyword));
	  int npts = seeds.count();
	  cfile->write((char*)&npts, sizeof(int));
	  float *pt = new float [3*npts];
	  for(int j=0; j<npts; j++)
	    {
	      pt[3*j+0] = seeds[j].x;
	      pt[3*j+1] = seeds[j].y;
	      pt[3*j+2] = seeds[j].z;		
	    }
	  cfile->write((char*)pt, 3*npts*sizeof(float));
	  delete [] pt;
      
	  memset(keyword, 0, 100);
	  sprintf(keyword, "fiberend\n");
	  cfile->write((char*)keyword, strlen(keyword));
	}
    }
  
  memset(keyword, 0, 100);
  sprintf(keyword, "fibergroupend\n");
  cfile->write((char*)keyword, strlen(keyword));
}

void
ImageWidget::loadFibers(QFile *cfile)
{
  char keyword[100];
  cfile->readLine((char*)&keyword, 100);

  if (strcmp(keyword, "fibergroupstart\n") != 0)
    return;

  bool cgend = false;
  while(!cgend)
    {
      cfile->readLine((char*)&keyword, 100);

      if (strcmp(keyword, "fibergroupend\n") == 0)
	cgend = true;      
      else if (strcmp(keyword, "fiberstart\n") == 0)
	{
	  Fiber fb;
	  bool done = false;
	  while(!done)
	    {
	      cfile->readLine((char*)&keyword, 100);
	      int t;
	      if (strcmp(keyword, "fiberend\n") == 0)
		done = true;
	      else if (strcmp(keyword, "tag\n") == 0)
		{
		  cfile->read((char*)&t, sizeof(int));
		  fb.tag = t;
		}
	      else if (strcmp(keyword, "thickness\n") == 0)
		{
		  cfile->read((char*)&t, sizeof(int));
		  fb.thickness = t;
		}
	      else if (strcmp(keyword, "seeds\n") == 0)
		{
		  int npts;
		  float *pt;
		  cfile->read((char*)&npts, sizeof(int));
		  pt = new float[3*npts];
		  cfile->read((char*)pt, 3*npts*sizeof(float));
		  for(int ni=0; ni<npts; ni++)
		    fb.seeds << Vec(pt[3*ni+0],
				    pt[3*ni+1],
				    pt[3*ni+2]);
		  delete [] pt;
		}	      
	    }
	  if (fb.seeds.count() > 0)
	    m_fibers.addFiber(fb);
	}
    }

}

void
ImageWidget::saveCurves()
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
ImageWidget::saveCurves(QString curvesfile)
{
  if (!StaticFunctions::checkExtension(curvesfile, ".curve") &&
      !StaticFunctions::checkExtension(curvesfile, ".curves"))
      curvesfile += ".curves";


  // if no curves present return
  if (!dCurvesPresent() &&
      !wCurvesPresent() &&
      !hCurvesPresent() &&
      !fibersPresent())
    { // remove existing file
      if (QFile(curvesfile).exists())
	QFile(curvesfile).remove();
      return;
    }
  
  QFile cfile;

  cfile.setFileName(curvesfile);
  cfile.open(QFile::WriteOnly);

  saveCurves(&cfile, &m_dCurves);
  saveCurves(&cfile, &m_wCurves);
  saveCurves(&cfile, &m_hCurves);

  saveFibers(&cfile);

  cfile.close();
}

void
ImageWidget::loadCurves(QFile *cfile, CurveGroup *cg)
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
ImageWidget::loadMorphedCurves(QFile *cfile, CurveGroup *cg)
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
ImageWidget::loadShrinkwrapCurves(QFile *cfile, CurveGroup *cg)
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
ImageWidget::loadCurves(QString curvesfile)
{
  QFile cfile;

  cfile.setFileName(curvesfile);
  if (cfile.exists() == false)
    return;
  
  cfile.open(QFile::ReadOnly);

  loadCurves(&cfile, &m_dCurves);
  loadCurves(&cfile, &m_wCurves);
  loadCurves(&cfile, &m_hCurves);

  loadFibers(&cfile);

  cfile.close();

  CurveGroup *cg = getCg();
  emit polygonLevels(cg->polygonLevels());

  //QMessageBox::information(0, "", QString("Curves loaded from %1").arg(curvesfile));
}

void
ImageWidget::loadCurves()
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
ImageWidget::saveFibers()
{
  QString flnm = QFileDialog::getSaveFileName(0,
					      "Save Fibers",
					      Global::previousDirectory(),
					      "Fibers Files (*.fibers)",
					      0,
					      QFileDialog::DontUseNativeDialog);

  if (flnm.isEmpty())
    return;

  if (!StaticFunctions::checkExtension(flnm, ".fiber") &&
      !StaticFunctions::checkExtension(flnm, ".fibers"))
      flnm += ".fibers";
  
  QFile cfile;
  cfile.setFileName(flnm);
  if (!cfile.open(QFile::WriteOnly | QIODevice::Text))
    {
      QMessageBox::information(0, "Error", QString("Cannot write to %1").arg(flnm));
      return;
    }

  QTextStream out(&cfile);
  QList<Fiber*>* fibers = m_fibers.fibers();
  int nfibers = fibers->count();
  for(int i=0; i<nfibers; i++)
    {
      Fiber *fb = fibers->at(i);
      int tag = fb->tag;
      int thickness = fb->thickness;
      QVector<Vec> seeds = fb->smoothSeeds;      
      int npts = seeds.count();
      if (npts > 0) // save only if we have non-empty fiber
	{
	  out << "fiberstart\n";
	  out << "tag  " << tag << "\n";
	  out << "thickness  " << thickness << "\n";
	  out << "seeds " << npts << "\n";
	  for(int j=0; j<npts; j++)
	    out << seeds[j].x << "  " << seeds[j].y << "  " << seeds[j].z << "\n";
	  out << "fiberend\n";	  
	}
    }
  QMessageBox::information(0, "", "Fibers saved to file.");
}

void
ImageWidget::loadFibers()
{
  QString fibersfile = QFileDialog::getOpenFileName(0,
						    "Load Fibers",
						    Global::previousDirectory(),
						    "Fibers Files (*.fibers)",
						    0,
						    QFileDialog::DontUseNativeDialog);
  
  if (fibersfile.isEmpty())
    return;

  loadFibers(fibersfile);
}

void
ImageWidget::loadFibers(QString flnm)
{
  QFile cfile;
  cfile.setFileName(flnm);
  if (!cfile.open(QFile::ReadOnly | QIODevice::Text))
    {
      QMessageBox::information(0, "Error", QString("Cannot read to %1").arg(flnm));
      return;
    }

  QTextStream in(&cfile);
  while (!in.atEnd())
    {
      QString line = in.readLine();
      if (line.contains("fiberstart"))
	{
	  Fiber fb;
	  bool done = false;
	  while(!done && !in.atEnd())
	    {
	      line = in.readLine();
	      QStringList words = line.split(" ", QString::SkipEmptyParts);
	      if (words.count() > 0)
		{
		  if (words[0].contains("fiberend"))
		    done = true;
		  else if (words[0].contains("tag"))
		    {		  
		      if (words.count() > 1)
			fb.tag = words[1].toInt();;
		    }
		  else if (words[0].contains("thickness"))
		    {
		      if (words.count() > 1)
			fb.thickness = words[1].toInt();;
		    }
		  else if (words[0].contains("seeds"))
		    {
		      int npts;
		      if (words.count() > 1)
			npts = words[1].toInt();;
		      for(int ni=0; ni<npts; ni++)
			{
			  line = in.readLine();
			  QStringList words = line.split(" ", QString::SkipEmptyParts);
			  if (words.count() == 3)
			    fb.seeds << Vec(words[0].toFloat(),
					    words[1].toFloat(),
					    words[2].toFloat());
			}
		    }
		}
	    }
	  if (fb.seeds.count() > 0)
	    m_fibers.addFiber(fb);
	}
    }

  QMessageBox::information(0, "", "Fibers read from file.");
}

void
ImageWidget::startShrinkwrap()
{
  CurveGroup *cg = getCg();
  cg->startShrinkwrap();
}

void
ImageWidget::endShrinkwrap()
{
  CurveGroup *cg = getCg();
  cg->endShrinkwrap();
}

void
ImageWidget::shrinkwrapCurve()
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

  for(int i=0; i<m_imgWidth*m_imgHeight; i++)
    imageData[i] = (m_sliceImage[4*i+0]>0 && imageData[i]>0 ? 255 : 0);
      
  CurveGroup *cg = getCg();
  cg->shrinkwrap(m_currSlice, imageData, m_imgWidth, m_imgHeight);
    
  delete [] imageData;
  
  checkRecursive();
}

void
ImageWidget::setMinCurveLength(int sz)
{
  m_dCurves.setShrinkwrapIgnoreSize(sz);
  m_wCurves.setShrinkwrapIgnoreSize(sz);
  m_hCurves.setShrinkwrapIgnoreSize(sz);
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
