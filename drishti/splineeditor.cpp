#include "global.h"
#include "splineeditor.h"
#include "dcolordialog.h"
#include "propertyeditor.h"

#include <QFileDialog>

QPointF
SplineEditor::convertLocalToWidget(QPointF p)
{
  double x = m_bounds.x() + p.x()*m_bounds.width();
  double y = m_bounds.y() + p.y()*m_bounds.height();
  return QPointF(x,y);
}

QPointF
SplineEditor::convertWidgetToLocal(QPointF p)
{
  double x = (p.x()-m_bounds.x())/m_bounds.width();
  double y = (p.y()-m_bounds.y())/m_bounds.height();
  x = qMin(1.0, qMax(0.0, x));
  y = qMin(1.0, qMax(0.0, y));
  return QPointF(x,y);
}

QRectF
SplineEditor::pointBoundingRect(QPointF pt, int rad)
{
  QPointF p = convertLocalToWidget(pt);
  double x = p.x() - rad/2;
  double y = p.y() - rad/2;

  return QRectF(x, y, rad, rad);
}


QRectF
SplineEditor::boundingRect() const
{
  if (m_bounds.isEmpty())
    return m_parent->rect();
  else
    return m_bounds;
}

int SplineEditor::border() const { return m_border; }
void SplineEditor::setBorder(const int border) { m_border = border; }

QSizeF SplineEditor::pointSize() const { return m_pointSize; }
void SplineEditor::setPointSize(const QSizeF &size) { m_pointSize = size; }

void SplineEditor::setConnectionPen(const QPen &pen) { m_connectionPen = pen; }
void SplineEditor::setShapePen(const QPen &pen) { m_pointPen = pen; }
void SplineEditor::setShapeBrush(const QBrush &brush) { m_pointBrush = brush; }

QImage SplineEditor::colorMapImage() { return m_splineTF->colorMapImage(); }

SplineEditor::SplineEditor(QWidget *parent) :
  QWidget(parent)
{
  setMinimumSize(100, 100);

  m_splineTF = 0;

  setMouseTracking(true);

  m_parent = parent;

  m_dragging = false;

  m_border = 10;
  m_connectionType = CurveConnection;
  m_pointShape = CircleShape;
  m_pointPen = QPen(QColor(255, 255, 255, 191), 1);
  m_connectionPen = QPen(QColor(255, 255, 255, 127), 2);
  m_pointBrush = QBrush(QColor(191, 191, 191, 127));
  m_pointSize = QSize(13, 13);
  m_currentIndex = -1;
  m_prevCurrentIndex = -1;
  m_normalIndex = -1;
  m_hoverIndex = -1;

  m_bounds = m_parent->rect().adjusted(m_border, m_border,
				       -2*m_border, -2*m_border);

  m_histogramImage1D = QImage(256, 256, QImage::Format_ARGB32);
  m_histogramImage1D.fill(0);

  m_histogramImage2D = QImage(256, 256, QImage::Format_ARGB32);
  m_histogramImage2D.fill(0);

  m_mapping.clear();

  m_show2D = true;
  m_showOverlay = true;
  m_showGrid = true;

  save_transferfunction_image_action = new QAction(tr("Save Transfer Function Image"), this);
  connect(save_transferfunction_image_action, SIGNAL(triggered()),
	  this, SLOT(saveTransferFunctionImage_cb()));
}

void
SplineEditor::setTransferFunction(SplineTransferFunction *stf)
{
  m_splineTF = stf;
}

void SplineEditor::show2D(bool flag) { m_show2D = flag; update(); }
void
SplineEditor::setHistogramImage(QImage histImg1D,
				QImage histImg2D)
{
  m_histogramImage1D = histImg1D;
  m_histogramImage2D = histImg2D;
}
void
SplineEditor::setMapping(QPolygonF fmap)
{
  m_mapping = fmap;
}

void
SplineEditor::resizeEvent(QResizeEvent *event)
{
  // m_border is unsigned int therefore needs to be multiplied by -1
  m_bounds = rect().adjusted(m_border, m_border,
			     -1*m_border, -1*m_border);

}


void
SplineEditor::setGradientStops(QGradientStops stops)
{
  if (!m_splineTF)
    return;

  m_splineTF->setGradientStops(stops);
  emit splineChanged();
}

void
SplineEditor::setGradLimits(int bot, int top)
{
  m_splineTF->setGradLimits(bot, top);
  emit splineChanged();
}

void
SplineEditor::setOpmod(float bot, float top)
{
  if(m_splineTF)
  {
	  m_splineTF->setOpmod(bot, top);
	  emit splineChanged();
  }
}

void 
SplineEditor::saveTransferFunctionImage_cb()
{
  QString filename = QFileDialog::getSaveFileName(this,
						  tr("Save Transfer Function Image"),
						  ".",
						  tr("PNG Image (*.png)"));
  if (!filename.isEmpty()) {

    if (m_splineTF->colorMapImage().save(filename, "PNG")) {
      QMessageBox::information(this, "Success", "Image saved successfully.");
    } else {
      QMessageBox::critical(this, "Error", "Failed to save image.");
    }
  }
}

void SplineEditor::enterEvent(QEvent *e) { setFocus(); grabKeyboard(); }
void SplineEditor::leaveEvent(QEvent *e) { clearFocus(); releaseKeyboard(); }

void
SplineEditor::keyPressEvent(QKeyEvent *event)
{
  if (event->key() == Qt::Key_Space)
    {      
      m_showOverlay = !m_showOverlay;
      update();
    }
  else if (event->key() == Qt::Key_G)
    {      
      m_showGrid = !m_showGrid;
      update();
    }
  else if (event->key() == Qt::Key_Z &&
	   (event->modifiers() & Qt::ControlModifier ||
	    event->modifiers() & Qt::MetaModifier) )
    emit applyUndo(true);
  else if (event->key() == Qt::Key_Y &&
	   (event->modifiers() & Qt::ControlModifier ||
	    event->modifiers() & Qt::MetaModifier) )
    emit applyUndo(false);
  else if (event->key() == Qt::Key_H &&
	   (event->modifiers() & Qt::ControlModifier ||
	    event->modifiers() & Qt::MetaModifier) )
    showHelp();
}

void
SplineEditor::mouseMoveEvent(QMouseEvent *event)
{
  // return if no transfer function available
  if (!m_splineTF)
    return;

  // return if no button is currently pressed
  if (! event->buttons())
    return;

  m_dragging = false;

  QPointF clickPos = event->pos();

  if (m_currentIndex >= 0 && m_moveSpine)
    {
      QPointF pt, newpt, diff;
      pt = m_splineTF->pointAt(m_currentIndex);
      newpt = convertWidgetToLocal(clickPos);
      diff = newpt - pt;
      if (Global::use1D())
	diff.setY(0);

      m_splineTF->moveAllPoints(diff);

      m_dragging = true;
      emit refreshDisplay();
    }
  else if (m_currentIndex >= 0 && m_normalIndex > -1)
    {
      if (Global::use1D() == false &&
	  (event->modifiers() & Qt::ControlModifier ||
	   event->modifiers() & Qt::MetaModifier) )
	m_splineTF->rotateNormalAt(m_currentIndex,
				   m_normalIndex,
				   convertWidgetToLocal(clickPos));
      else
	m_splineTF->moveNormalAt(m_currentIndex,
				 m_normalIndex,
				 convertWidgetToLocal(clickPos),
				 event->modifiers() & Qt::ShiftModifier);

      m_dragging = true;
      emit refreshDisplay();
    }
  else if (m_currentIndex >= 0)
    {
      QPointF pt = convertWidgetToLocal(clickPos);
      QPointF opt = m_splineTF->pointAt(m_currentIndex);
      m_hoverIndex = m_currentIndex;
            
      if (event->modifiers() & Qt::ShiftModifier)
	pt.setX(opt.x()); // move only vertically
      else if (event->modifiers() & Qt::AltModifier)
	pt.setY(opt.y()); // move only horizontally

      if (Global::use1D())
	{
	  pt.setY(opt.y()); // move only horizontally
	}
      else
	m_splineTF->movePointAt(m_currentIndex, pt);

      m_dragging = true;
      emit refreshDisplay();
    }

  return;
}

void
SplineEditor::mouseReleaseEvent(QMouseEvent *event)
{
  if (!m_splineTF)
    return;

  if ( m_currentIndex >= 0)
    emit splineChanged();

  m_dragging = false;  
  m_currentIndex = -1;
  m_normalIndex = -1;
  m_hoverIndex = m_currentIndex;
  update();
}

int
SplineEditor::checkNormalPressEvent(QMouseEvent *event)
{
  if (!m_splineTF)
    return 0;

  m_normalIndex = -1;

  if (event->button() != Qt::LeftButton)
    return 0;

  QPointF clickPos = event->pos();
  int index = -1;
  int i;

  for (i=0; i<m_splineTF->size(); ++i)
    {
      QPainterPath path;
      path.addEllipse(pointBoundingRect(m_splineTF->rightNormalAt(i),
					m_pointSize.width()));
      
      if (path.contains(clickPos))
	{
	  index = i;
	  break;
	}
    }
  if (index >= 0)
    {
      m_currentIndex = index;
      m_normalIndex = SplineTransferFunction::RightNormal;
      m_hoverIndex = m_currentIndex;
      return 1;
    }

  for (int i=0; i<m_splineTF->size(); ++i)
    {
      QPainterPath path;
      path.addEllipse(pointBoundingRect(m_splineTF->leftNormalAt(i),
					m_pointSize.width()));
      
      if (path.contains(clickPos))
	{
	  index = i;
	  break;
	}
    }
  if (index >= 0)
    {
      m_currentIndex = index;
      m_normalIndex = SplineTransferFunction::LeftNormal;
      m_hoverIndex = m_currentIndex;
      return 1;
    }

  return 0;
}

void
SplineEditor::mousePressEvent(QMouseEvent *event)
{
  if (!m_splineTF)
    return;

  m_dragging = false;
  m_moveSpine = false;

  if (checkNormalPressEvent(event) != 0)
    {
      m_prevCurrentIndex = m_currentIndex;
      emit refreshDisplay();
      emit selectEvent(m_splineTF->gradientStops());
      return;
    }

  QPointF clickPos = event->pos();
  int index = -1;

  for (int i=0; i<m_splineTF->size(); ++i)
    {
      QPainterPath path;
      path.addEllipse(pointBoundingRect(m_splineTF->pointAt(i),
					m_pointSize.width()));
      
      if (path.contains(clickPos))
	{
	  index = i;
	  break;
	}
    }
  
  if (index > -1)
    {
      if (event->button() == Qt::MidButton ||
	  Global::use1D())
	{
	  m_moveSpine = true;
	  m_currentIndex = index;
	  m_hoverIndex = m_currentIndex;
	  
	  m_prevCurrentIndex = m_currentIndex;
	  emit selectEvent(m_splineTF->gradientStops());

	  return;
	}
    }

  if (event->button() == Qt::LeftButton)
    {
      if (index == -1)
	{
	  bool inserted = false;
	  QPointF sp;
	  sp = convertWidgetToLocal(clickPos);
	  for (int i=0; i<m_splineTF->size()-1; ++i)
	    {
	      QLineF tang, ts;
	      float tanglen;
	      tang = QLineF(m_splineTF->pointAt(i),
			    m_splineTF->pointAt(i+1));
	      ts = QLineF(m_splineTF->pointAt(i),
			  sp);
	      tanglen = tang.length();
	      float tdot;
	      tdot = (tang.dx()*ts.dx()+tang.dy()*ts.dy())/(tanglen*tanglen);
	      if (tdot >= 0.0f && tdot <= 1.0f)
		{
		  QPointF p0;
		  p0 = m_splineTF->pointAt(i);
		  p0 += tdot*QPointF(tang.dx(), tang.dy());
		  
		  QLineF perp = QLineF(p0, sp);
		  if (perp.length() < 0.05)
		    {
		      m_splineTF->insertPointAt(i+1, sp);
		      m_currentIndex = i+1;
		      inserted = true;
		      break;
		    }
		}
	    }

	  if (!inserted)
	    {
	      QLineF l1 = QLineF(m_splineTF->pointAt(0), sp);
	      QLineF l2 = QLineF(m_splineTF->pointAt(m_splineTF->size()-1), sp);
	      if (l1.length() < l2.length())
		{
		  m_splineTF->insertPointAt(0, sp);
		  m_currentIndex = 0;
		}
	      else
		{
		  m_splineTF->appendPoint(sp);
		  m_currentIndex = m_splineTF->size()-1;
		}
	    }
	  
	  m_hoverIndex = m_currentIndex;
	  emit refreshDisplay();
	}
      else
	{
	  m_currentIndex = index;
	  m_hoverIndex = m_currentIndex;
	  emit refreshDisplay();
	}
      
      m_prevCurrentIndex = m_currentIndex;
      emit selectEvent(m_splineTF->gradientStops());

      return;      
    }
  else if (event->button() == Qt::RightButton)
    {
      if (index >= 0)
	{
	  if (m_splineTF->size() > 2)
	    {
	      m_prevCurrentIndex = -1;
	      m_splineTF->removePointAt(index);
	      emit deselectEvent();
	      emit splineChanged();
	      return;
	    }
	}
//      else
//	{
//	  QMenu menu(this);
//	  menu.addAction(save_transferfunction_image_action);
//	  menu.exec(event->globalPos());
//        }
    }

//  if (clickPos.y() < m_bounds.y())
//    {
//      m_showOverlay = !m_showOverlay;
//      update();
//    }  
}

void
SplineEditor::getPainterPath(QPainterPath *path, QPolygonF points)
{
  if (!m_splineTF)
    return;

  path->moveTo(points[0]);
  for (int i=1; i<points.size(); ++i)
    {
      QPointF p1, c1, c2, p2;
      QPointF midpt;
      p1 = points[i-1];
      p2 = points[i];
      midpt = (p1+p2)/2;
      
      QLineF l0, l1, l2;
      float dotp;
      
      l1 = QLineF(p1, p2);
      if (i > 1)
	l1 = QLineF(points[i-2], p2);
      l1 = l1.unitVector();
      
      l0 = QLineF(p1, midpt);
      dotp = l0.dx()*l1.dx() + l0.dy()*l1.dy();
      
      if (dotp < 0)
	l1.setLength(-dotp);
      else
	l1.setLength(dotp);
      
      c1 = p1 + QPointF(l1.dx(), l1.dy());
      
      
      
      l2 = QLineF(p1, p2);;
      if (i < m_splineTF->size()-1)
	l2 = QLineF(p1, points[i+1]);
      l2 = l2.unitVector();
      
      l0 = QLineF(p2, midpt);
      dotp = l0.dx()*l2.dx() + l0.dy()*l2.dy();
      
      if (dotp < 0)
	l2.setLength(-dotp);
      else
	l2.setLength(dotp);
      
      c2 = p2 - QPointF(l2.dx(), l2.dy());	  
      

      path->cubicTo(c1, c2, p2);
    }
}

void
SplineEditor::paintEvent(QPaintEvent *event)
{
  QPainter p(this);

  p.setRenderHint(QPainter::Antialiasing);
  p.setRenderHint(QPainter::SmoothPixmapTransform);
  p.setCompositionMode(QPainter::CompositionMode_SourceOver);

  if (!m_splineTF)
    {
      p.setPen(QColor(146, 146, 146));
      p.drawRect(m_bounds);
      return;
    }

  paintUnderlay(&p);

  if (m_connectionPen.style() != Qt::NoPen &&
      m_connectionType != NoConnection)
    paintPatchLines(&p);

  QImage colImage = (m_splineTF->colorMapImage()).scaled(m_bounds.width(),
							 m_bounds.height());
  
  p.drawImage(m_bounds.x(), m_bounds.y(), colImage);

  paintPatchEndPoints(&p);

  // draw bounding rectangle 
  p.setBrush(Qt::transparent);
  p.setPen(QColor(146, 146, 146));
  p.drawRect(m_bounds);

  if (m_showOverlay)
    paintOverlay(&p);
}

void
SplineEditor::paintPatchLines(QPainter *p)
{
  p->setBrush(m_pointBrush);
  p->setPen(m_connectionPen);
  
  p->setBrush(Qt::transparent);
  p->setPen(m_connectionPen);
  
  if (m_dragging)
    {
      QPolygonF points;
      for (int i=0; i<m_splineTF->size(); i++)
	points << convertLocalToWidget(m_splineTF->pointAt(i));
      QPainterPath path;
      getPainterPath(&path, points);	  
      p->drawPath(path);
    }
  
  QPolygonF pointsLeft;
  pointsLeft.clear();
  for (int i=0; i<m_splineTF->size(); i++)
    pointsLeft << convertLocalToWidget(m_splineTF->leftNormalAt(i));
  QPainterPath pathLeft;
  getPainterPath(&pathLeft, pointsLeft);
  
  
  QPolygonF pointsRight;
  pointsRight.clear();
  for (int i=0; i<m_splineTF->size(); i++)
    pointsRight << convertLocalToWidget(m_splineTF->rightNormalAt(i));
  QPainterPath pathRight;
  getPainterPath(&pathRight, pointsRight);
  
  if (m_dragging)
    {
      p->drawPath(pathLeft);
      p->drawPath(pathRight);
      
      for (int i=0; i<m_splineTF->size(); ++i)
	{
	  QPointF v1, v2;
	  v1 = convertLocalToWidget(m_splineTF->leftNormalAt(i));
	  v2 = convertLocalToWidget(m_splineTF->rightNormalAt(i));
	  
	  p->drawLine(v1, v2);
	}
    }
}

void
SplineEditor::paintPatchEndPoints(QPainter *p)
{
  p->setPen(m_pointPen);
  p->setBrush(m_pointBrush);
  
  float x,y,w,h;
  w = m_pointSize.width();
  h = m_pointSize.width();
  // draw spine centers
  for (int i=0; i<m_splineTF->size(); ++i)
    {
      QPointF v1;
      v1 = convertLocalToWidget(m_splineTF->pointAt(i));
      x = v1.x() - w / 2;
      y = v1.y() - h / 2;
      p->drawEllipse(QRectF(x, y, w, h));
    }
  
  // draw normal endpoints
  w = m_pointSize.width()-3;
  h = m_pointSize.width()-3;
  for (int i=0; i<m_splineTF->size(); ++i)
    {
      QPointF v1, v2;
      v1 = convertLocalToWidget(m_splineTF->leftNormalAt(i));
      v2 = convertLocalToWidget(m_splineTF->rightNormalAt(i));
      
      x = v1.x() - w / 2;
      y = v1.y() - h / 2;
      p->drawEllipse(QRectF(x, y, w, h));
      
      x = v2.x() - w / 2;
      y = v2.y() - h / 2;
      p->drawEllipse(QRectF(x, y, w, h));	  
    }

  if (m_prevCurrentIndex > -1)
    {
      float x,y,w,h;
      QPointF v1;
      v1 = convertLocalToWidget(m_splineTF->pointAt(m_prevCurrentIndex));
      p->setBrush(Qt::darkGray);

      w = 13;
      h = 13;
      x = v1.x() - w / 2;
      y = v1.y() - h / 2;
      p->drawEllipse(QRectF(x, y, w, h));

      w = 8;
      h = 8;
      v1 = convertLocalToWidget(m_splineTF->leftNormalAt(m_prevCurrentIndex));
      x = v1.x() - w / 2;
      y = v1.y() - h / 2;
      p->drawEllipse(QRectF(x, y, w, h));

      v1 = convertLocalToWidget(m_splineTF->rightNormalAt(m_prevCurrentIndex));
      x = v1.x() - w / 2;
      y = v1.y() - h / 2;
      p->drawEllipse(QRectF(x, y, w, h));	  
    }
}

void
SplineEditor::paintUnderlay(QPainter *p)
{
  QImage bgImage;
  if (m_show2D)
    bgImage = m_histogramImage2D.scaled(m_bounds.width(),
					m_bounds.height());
  else
    bgImage = m_histogramImage1D.scaled(m_bounds.width(),
					m_bounds.height());

  p->drawImage(m_bounds.x(), m_bounds.y(), bgImage);

  // -- draw grid -----------------------------
  if (m_showGrid)
    {
      QVector<QPoint> grid;
      float x = m_bounds.x();
      float xstep = m_bounds.width()/10.0;
      float y = m_bounds.x();
      float ystep = m_bounds.height()/10.0;
      p->setBrush(Qt::transparent);
      p->setPen(m_connectionPen);
      for(int g=1; g<10; g++)
	{
	  grid.append(QPoint(x + g*xstep, m_bounds.y()));
	  grid.append(QPoint(x + g*xstep, m_bounds.y()+m_bounds.height()));
	  grid.append(QPoint(m_bounds.x(), y + g*ystep));
	  grid.append(QPoint(m_bounds.x()+m_bounds.width(), y + g*ystep));
	}
      p->drawLines(grid);
    }
}

void
SplineEditor::paintOverlay(QPainter *p)
{
  if (m_mapping.count() == 0)
    return;

  QFont pfont = QFont("Helvetica", 10);
  QPen linepen(Qt::yellow);
  linepen.setWidth(2);

  p->setBrush(QColor(0,0,0,100));
  p->setFont(pfont);

  float x = m_bounds.x();
  float xstep = m_bounds.width()/255.0;
  if (Global::pvlVoxelType() > 0)
    xstep = m_bounds.width()/65535.0;

  for(int i=0; i<m_mapping.count(); i++)
    {
      float f = m_mapping[i].x();
      int b = m_mapping[i].y();
      
      QPoint v0, v1;
      v0 = QPoint(x + b*xstep, m_bounds.y());
      v1 = QPoint(x + b*xstep, m_bounds.y()+m_bounds.height());

      QPainterPath pp;
      pp.addText(QPointF(0,0), 
		 pfont,
		 QString("%1").arg(f));
      QRectF br = pp.boundingRect();
      float by = br.height();      
      p->translate(v0.x()-by/2, v0.y()-7);
      p->rotate(90);

      br.adjust(-3,-5,3,4);
      p->setPen(Qt::yellow);
      p->drawRect(br);
      p->setPen(Qt::white);
      p->drawText(QPointF(0,0), 
		  QString("%1").arg(f));      
      p->resetTransform();


      v0.setY(v0.y()+br.width()-7);
      p->setPen(linepen);
      p->drawLine(v0, v1);
    }
}

void
SplineEditor::showHelp()
{
  PropertyEditor propertyEditor;
  QMap<QString, QVariantList> plist;
  QVariantList vlist;

  vlist.clear();
  QFile helpFile(":/transferfunctioneditor.help");
  if (helpFile.open(QFile::ReadOnly))
    {
      QTextStream in(&helpFile);
      QString line = in.readLine();
      while (!line.isNull())
	{
	  if (line == "#begin")
	    {
	      QString keyword = in.readLine();
	      QString helptext;
	      line = in.readLine();
	      while (!line.isNull())
		{
		  helptext += line;
		  helptext += "\n";
		  line = in.readLine();
		  if (line == "#end") break;
		}
	      vlist << keyword << helptext;
	    }
	  line = in.readLine();
	}
    }  
  plist["commandhelp"] = vlist;
  
  QStringList keys;
  keys << "commandhelp";
  
  propertyEditor.set("Transfer Function Manager Help", plist, keys);
  propertyEditor.exec();
}

bool
SplineEditor::set16BitPoint(float tmin, float tmax)
{
  if (m_splineTF)
    {
      m_splineTF->set16BitPoint(tmin, tmax);
      return true;
    }
  return false;
}
