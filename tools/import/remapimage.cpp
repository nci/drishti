#include "global.h"
#include "remapimage.h"

RemapImage::RemapImage(QWidget *parent) :
  QWidget(parent)
{
  setFocusPolicy(Qt::StrongFocus);

  m_Depth = m_Width = m_Height = 0;
  m_imgHeight = 100;
  m_imgWidth = 100;
  m_simgHeight = 100;
  m_simgWidth = 100;
  m_simgX = 10;
  m_simgY = 10;

  m_image = QImage(100, 100, QImage::Format_RGB32);
  m_imageScaled = QImage(100, 100, QImage::Format_RGB32);
  m_currSlice = 0;
  m_minDSlice = m_maxDSlice = 0;
  m_minWSlice = m_maxWSlice = 0;
  m_minHSlice = m_maxHSlice = 0;

  m_cursorPos = QPoint(0,0);
  m_pickPoint = false;
  m_pickDepth = 0;
  m_pickWidth = 0;
  m_pickHeight = 0;

  m_zoom = 1;
  setMinimumSize(QSize(100, 100));

  m_rubberBand = QRect(0,0,0,0);
  m_rubberXmin = false;
  m_rubberYmin = false;
  m_rubberXmax = false;
  m_rubberYmax = false;
  m_rubberNew = false;
  m_rubberBandActive = false;

  m_gradientStops << QGradientStop(0, Qt::darkBlue)
		  << QGradientStop(0.5, Qt::yellow)
		  << QGradientStop(1, Qt::red);
  generateColorTable();

  m_rawValue = QVariant("OutOfBounds");
  m_pvlValue = QVariant("OutOfBounds");
}

QVector<QRgb> RemapImage::colorMap() { return m_colorMap; }

void RemapImage::enterEvent(QEvent* event) { setFocus(); }
void RemapImage::leaveEvent(QEvent* event) { clearFocus(); }


void
RemapImage::loadLimits()
{
  QString flnm = QFileDialog::getOpenFileName(0,
					      "Load limits information",
					      Global::previousDirectory(),
					      "Files (*.*)",
					      0,
					      QFileDialog::DontUseNativeDialog);

  if (flnm.isEmpty())
    return;

  QFile file(flnm);
  if (!file.open(QIODevice::ReadOnly |
		 QIODevice::Text))
    {
      QMessageBox::critical(this, "Error", "Cannot write to file");
      return;
    }
  
  QTextStream in(&file);
  QString gs = in.readLine();
  QString ht = in.readLine();
  QString wd = in.readLine();
  QString dp = in.readLine();
  
  QStringList str;

  str = gs.split(":");
  str = str[1].split(" ", QString::SkipEmptyParts);
  int h = str[0].toInt();
  int w = str[1].toInt();
  int d = str[2].toInt();

  float scaleh = 1.0;
  float scalew = 1.0;
  float scaled = 1.0;

  if (d-1 != m_Depth ||
      w-1 != m_Width ||
      h-1 != m_Height)
    {
      QString str;
      str = "Grid sizes do not match\n";

      str += QString("Loaded volume : %1 %2 %3\n").\
	arg(m_Height).\
	arg(m_Width).\
	arg(m_Depth);

      str += QString("From limits file : %1 %2 %3\n").\
	arg(h-1).\
	arg(w-1).\
	arg(d-1);

      str += "\nWould you like to apply appropriate scaling to match the volume ?";

      //QMessageBox::critical(this, "Error", str);

      QStringList items;
      items << "Yes" << "No";
      bool ok;
      QString item = QInputDialog::getItem(this,
					   "Load Limits",
					   str,
					   items,
					   0,
					   false,
					   &ok);
  
      if (ok && item == "Yes")
	{
	  scaleh = (float)m_Height/(float)(h-1);
	  scalew = (float)m_Width/(float)(w-1);
	  scaled = (float)m_Depth/(float)(d-1);
	}
      else
	return;
    }


  str = ht.split(":");
  str = str[1].split(" ", QString::SkipEmptyParts);
  m_minHSlice = str[0].toInt() * scaleh;
  m_maxHSlice = str[1].toInt() * scaleh;

  str = wd.split(":");
  str = str[1].split(" ", QString::SkipEmptyParts);
  m_minWSlice = str[0].toInt() * scalew;
  m_maxWSlice = str[1].toInt() * scalew;


  str = dp.split(":");
  str = str[1].split(" ", QString::SkipEmptyParts);
  m_minDSlice = str[0].toInt() * scaled;
  m_maxDSlice = str[1].toInt() * scaled;
  

  QMessageBox::information(this, "Load Limits",
			   QString("%1 %2\n%3 %4\n%5 %6").\
			   arg(m_minHSlice).arg(m_maxHSlice).\
			   arg(m_minWSlice).arg(m_maxWSlice).\
			   arg(m_minDSlice).arg(m_maxDSlice));

  setSliceType(m_sliceType);
}

void
RemapImage::saveLimits()
{
  QString flnm = QFileDialog::getSaveFileName(0,
					      "Save limits information",
					      Global::previousDirectory(),
					      "Files (*.txt)");
//					      0,
//					      QFileDialog::DontUseNativeDialog);

  if (flnm.isEmpty())
    return;

  if (!flnm.endsWith(".txt"))
    flnm += ".txt";

  QFile file(flnm);
  if (!file.open(QIODevice::WriteOnly |
		 QIODevice::Text))
    {
      QMessageBox::critical(this, "Error", "Cannot write to file");
      return;
    }
  
  QTextStream out(&file);
  out << "grid size : "<<m_Height+1<<" "<<m_Width+1<<" "<<m_Depth+1<<"\n";
  out << "height limits : " <<m_minHSlice<<" "<<m_maxHSlice<<"\n";
  out << "width limits : " <<m_minWSlice<<" "<<m_maxWSlice<<"\n";
  out << "depth limits : " <<m_minDSlice<<" "<<m_maxDSlice<<"\n";
  
  QMessageBox::information(this, "Save Limits", "done");
}

void
RemapImage::saveImage()
{
//  QString imgFile = QFileDialog::getSaveFileName(0,
//						 "Save composite image",
//						 Global::previousDirectory(),
//						 "Image Files (*.png *.tif *.bmp *.jpg)",
//						 0,
//						 QFileDialog::DontUseNativeDialog);

  QString imgFile = QFileDialog::getSaveFileName(0,
						 "Save composite image",
						 Global::previousDirectory(),
						 "Image Files (*.png *.tif *.bmp *.jpg)");
  if (imgFile.isEmpty())
    return;

  m_image.save(imgFile);
  QMessageBox::information(0, "Save Image", "Done");
}

void
RemapImage::setZoom(float z)
{
  setMinimumSize(QSize(m_imgWidth, m_imgHeight));

  m_zoom = qMax(0.01f, z);
  resizeImage();
  update();
}

void
RemapImage::setGridSize(int d, int w, int h)
{
  m_Depth = d-1;
  m_Width = w-1;
  m_Height = h-1;
   
  m_minDSlice = 0;
  m_maxDSlice = m_Depth;
  m_minWSlice = 0;
  m_maxWSlice = m_Width;
  m_minHSlice = 0;
  m_maxHSlice = m_Height;
}

void
RemapImage::sliceChanged(int slc)
{
  if (m_sliceType == DSlice)
    m_currSlice = slc;
  else if (m_sliceType == WSlice)
    m_currSlice = slc;
  else
    m_currSlice = slc;

  emit getSlice(m_currSlice);
}

void
RemapImage::userRangeChanged(int umin, int umax)
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

  updateStatusText();
  update();
}

void
RemapImage::depthUserRange(int& umin, int& umax)
{
  umin = m_minDSlice;
  umax = m_maxDSlice;
}
void
RemapImage::widthUserRange(int& umin, int& umax)
{
  umin = m_minWSlice;
  umax = m_maxWSlice;
}
void
RemapImage::heightUserRange(int& umin, int& umax)
{
  umin = m_minHSlice;
  umax = m_maxHSlice;
}

void
RemapImage::setSliceType(int st)
{
  m_sliceType = st;
  if (m_sliceType == DSlice)
    m_currSlice = m_minDSlice;
  else if (m_sliceType == WSlice)
    m_currSlice = m_minWSlice;
  else
    m_currSlice = m_minHSlice;

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

  updateStatusText();
}

void
RemapImage::setGradientStops(QGradientStops stops)
{
  m_gradientStops = stops;
  generateColorTable();
  m_image.setColorTable(m_colorMap);
  resizeImage();
  update();
}

void
RemapImage::setImage(QImage img)
{
  m_image = img;

  if (m_image.format() == QImage::Format_Indexed8)
    m_image.setColorTable(m_colorMap);  

  m_imgHeight = m_image.height();
  m_imgWidth = m_image.width();

  resizeImage();

  update();
}

void
RemapImage::generateColorTable()
{
  m_colorMap.clear();
  m_colorMap.resize(256);

  int startj, endj;
  for(int i=0; i<m_gradientStops.size(); i++)
    {
      float pos = m_gradientStops[i].first;
      QColor color = m_gradientStops[i].second;
      endj = pos*255;
      m_colorMap[endj] = color.rgb();
      if (i > 0)
	{
	  QColor colStart, colEnd;
	  colStart = m_colorMap[startj];
	  colEnd = m_colorMap[endj];
	  float rb,gb,bb,ab, re,ge,be,ae;
	  rb = colStart.red();
	  gb = colStart.green();
	  bb = colStart.blue();
	  ab = colStart.alpha();
	  re = colEnd.red();
	  ge = colEnd.green();
	  be = colEnd.blue();
	  ae = colEnd.alpha();
	  for (int j=startj+1; j<endj; j++)
	    {
	      float frc = (float)(j-startj)/(float)(endj-startj);
	      float r,g,b,a;
	      r = rb + frc*(re-rb);
	      g = gb + frc*(ge-gb);
	      b = bb + frc*(be-bb);
	      a = ab + frc*(ae-ab);
	      m_colorMap[j] = qRgb(r, g, b);
	    }
	}
      startj = endj;
    }
}

void
RemapImage::setRawValue(QPair<QVariant, QVariant> vpair)
{
  m_rawValue = vpair.first;
  m_pvlValue = vpair.second;
  update();
}

void
RemapImage::resizeImage()
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
}

void
RemapImage::resizeEvent(QResizeEvent *event)
{
  resizeImage();
  update();
}

void
RemapImage::updateStatusText()
{
  QString gbtext;
  float gb;
  gb = (m_maxHSlice-m_minHSlice+1);
  gb /= 1024.0;
  gb *= (m_maxWSlice-m_minWSlice+1);
  gb /= 1024.0;
  gb *= (m_maxDSlice-m_minDSlice+1);
  gb /= 1024.0;

  if (gb > 1.0)
    gbtext = QString("%1 Gb").arg(gb);
  else
    {
      gb *= 1024;
      gbtext = QString("%1 Mb").arg(gb);
    }

  QString txt = QString("%1 %2 %3 [%4:%5 %6:%7 %8:%9] (%10 %11 %12) : ").\
    arg(m_Height+1).arg(m_Width+1).arg(m_Depth+1).		      \
    arg(m_minHSlice).arg(m_maxHSlice).				      \
    arg(m_minWSlice).arg(m_maxWSlice).				      \
    arg(m_minDSlice).arg(m_maxDSlice).				      \
    arg(m_maxHSlice-m_minHSlice+1).				      \
    arg(m_maxWSlice-m_minWSlice+1).				      \
    arg(m_maxDSlice-m_minDSlice+1);
  txt += gbtext;
  Global::statusBar()->showMessage(txt);
}

void
RemapImage::paintEvent(QPaintEvent *event)
{
  QPainter p(this);

  p.drawImage(m_simgX, m_simgY, m_imageScaled);

  drawRubberBand(&p);

  if (m_pickPoint)
    drawRawValue(&p);

  if (hasFocus())
    {
      p.setPen(QPen(QColor(250, 100, 0), 2));
      p.setBrush(Qt::transparent);
      p.drawRect(rect());
    }
}

void
RemapImage::drawRubberBand(QPainter *p)
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
RemapImage::drawRawValue(QPainter *p)
{
  QVariant vr = m_rawValue;
  QVariant vp = m_pvlValue;

  QString str;
  bool isString = false;
  if (vr.type() == QVariant::String)
    {
      isString = true;

      str = vr.toString();
      if (str == "OutOfBounds")
	return;
    }

  int xp = m_cursorPos.x();
  int yp = m_cursorPos.y()-20;

  str = QString("%1 %2 %3").\
          arg(m_pickHeight).\
          arg(m_pickWidth).\
          arg(m_pickDepth);
    
  if (isString)
    str += vr.toString();
  else
    {
      if (vr.type() == QVariant::UInt)
	str += QString("-ui(%1)").arg(vr.toUInt());
      else if (vr.type() == QVariant::Int)
	str += QString("-u(%1)").arg(vr.toInt());
      else if (vr.type() == QVariant::Double)
	str += QString("-f(%1)").arg(vr.toDouble());

      str += QString("--(%1)").arg(vp.toUInt());
    }

  QFont pfont = QFont("Helvetica", 10);
  QPainterPath pp;
  pp.addText(QPointF(0,0), 
	     pfont,
	     str);
  QRectF br = pp.boundingRect();
  float by = br.height()/2;      
  float bw = br.width();      

  int x = xp-bw-30;
  if (x < 1) x = xp+20;
  if (yp < 10) yp += 70;

  p->setPen(Qt::yellow);
  p->setBrush(QColor(0,0,0,200));
  p->drawRect(x, yp-by-5,
	      bw+70, 2*by+10);
  p->setPen(Qt::white);
  p->drawText(x+5, yp+by,
	      str);
}

void
RemapImage::emitSaveTrimmed()
{
  emit saveTrimmed(m_minDSlice, m_maxDSlice,
		   m_minWSlice, m_maxWSlice,
		   m_minHSlice, m_maxHSlice);
}

void
RemapImage::emitSaveIsosurface()
{
  emit saveIsosurface(m_minDSlice, m_maxDSlice,
		      m_minWSlice, m_maxWSlice,
		      m_minHSlice, m_maxHSlice);
}

void
RemapImage::emitSaveTrimmedImages()
{
  emit saveTrimmedImages(m_minDSlice, m_maxDSlice,
			 m_minWSlice, m_maxWSlice,
			 m_minHSlice, m_maxHSlice);
}

void
RemapImage::keyPressEvent(QKeyEvent *event)
{
  int ctrlModifier = event->modifiers() & Qt::ControlModifier;

  if (ctrlModifier)
    {
      if (event->key() == Qt::Key_0)
	setZoom(1);
      else if (event->key() == Qt::Key_Plus ||
	       event->key() == Qt::Key_Equal)
	setZoom(m_zoom+0.1);
      else if (event->key() == Qt::Key_Underscore ||
	       event->key() == Qt::Key_Minus)
	setZoom(m_zoom-0.1);

      return;
    }

  if (event->key() == Qt::Key_S &&
      (event->modifiers() & Qt::AltModifier) )
    {
      saveImage();
      return;
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


  if (event->key() == Qt::Key_S)
    {
      emitSaveTrimmed();
    }
  else if (event->key() == Qt::Key_I)
    {
      emitSaveIsosurface();
    }
  else if (event->key() == Qt::Key_T)
    {
      emit extractRawVolume();
    }
  else if (event->key() == Qt::Key_Up)
    {
      m_currSlice--;
      m_currSlice = qBound(minS, m_currSlice, maxS);
      emit getSlice(m_currSlice);
      update();
    }
  else if (event->key() == Qt::Key_Down)
    {
      m_currSlice++;
      m_currSlice = qBound(minS, m_currSlice, maxS);
      emit getSlice(m_currSlice);
      update();
    }
}

bool
RemapImage::checkRubberBand(int xpos, int ypos,
			    bool shiftModifier)
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
      // give some slack (2px) at the ends
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
      else if (shiftModifier)
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
      m_rubberBandActive = true;
    }

  return m_rubberBandActive;
}

void
RemapImage::mouseDoubleClickEvent(QMouseEvent *event)
{
  m_rubberBand = QRect(0,0,1,1);
  m_rubberXmin = false;
  m_rubberYmin = false;
  m_rubberXmax = false;
  m_rubberYmax = false;
  m_rubberNew = false;
  m_rubberBandActive = false;

  // now update the min and max limits
  updateLimits();
      
  updateStatusText();  
}

void
RemapImage::mousePressEvent(QMouseEvent *event)
{
  //QPoint pp = mapFromParent(event->pos());
  QPoint pp = event->pos();
  float ypos = pp.y();
  float xpos = pp.x();

  m_button = event->button();

  m_cursorPos = pp;
  m_pickPoint = false;

  if (m_button == Qt::LeftButton)
    {
      bool shiftModifier = event->modifiers() & Qt::ShiftModifier;
      if (checkRubberBand(xpos, ypos, shiftModifier))
	return;
    }
  else if (m_button == Qt::RightButton)
    {
      if (validPickPoint(xpos,  ypos))
	emit getRawValue(m_pickDepth,
			 m_pickWidth,
			 m_pickHeight);

    }

  update();
}

void
RemapImage::updateRubberBand(int xpos, int ypos)
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
RemapImage::validPickPoint(int xpos, int ypos)
{
  if (xpos >= m_simgX-2 && xpos <= m_simgX+m_simgWidth+2 &&
      ypos >= m_simgY-2 && ypos <= m_simgY+m_simgHeight+2)
    {

      // give some slack(2px) at the ends
      float frcX = (float)(xpos-m_simgX)/(float)m_simgWidth;
      float frcY = (float)(ypos-m_simgY)/(float)m_simgHeight;
      
      frcX = qBound(0.0f, frcX, 1.0f);
      frcY = qBound(0.0f, frcY, 1.0f);
      
      m_pickPoint = true;
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
RemapImage::mouseMoveEvent(QMouseEvent *event)
{
  if (!hasFocus())
    setFocus();

  //QPoint pp = mapFromParent(event->pos());
  QPoint pp = event->pos();
  float ypos = pp.y();
  float xpos = pp.x();

  m_cursorPos = pp;

  if (m_button == Qt::LeftButton)
    {
      if (m_rubberBandActive)
	{
	  updateRubberBand(xpos, ypos);
	  update();
	  return;
	}
    }
  else if (m_button == Qt::RightButton)
    {
      if (validPickPoint(xpos, ypos))
	{
	  emit getRawValue(m_pickDepth,
			   m_pickWidth,
			   m_pickHeight);
	  update();
	  return;
	}
    }
  
  if (m_pickPoint)
    {
      m_pickPoint = false;
      update();
    }
}

void
RemapImage::mouseReleaseEvent(QMouseEvent *event)
{
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

      // now update the min and max limits
      updateLimits();
      
      updateStatusText();
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
RemapImage::updateLimits()
{
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
}

void
RemapImage::wheelEvent(QWheelEvent *event)
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

  int numSteps = event->delta()/8.0f/15.0f;
  m_currSlice -= numSteps;
  m_currSlice = qBound(minS, m_currSlice, maxS);
  emit getSlice(m_currSlice);
  update();
}
