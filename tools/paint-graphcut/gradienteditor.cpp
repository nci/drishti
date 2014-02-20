#include "gradienteditor.h"
#include "dcolordialog.h"
#include <QDomDocument>

#include <QMessageBox>
#include <QInputDialog>
#include <QLineEdit>

int GradientEditor::border() const { return m_border; }
void GradientEditor::setBorder(const int border) { m_border = border; }

QSizeF GradientEditor::pointSize() const { return m_pointSize; }
void GradientEditor::setPointSize(const QSizeF &size) { m_pointSize = size; }

void GradientEditor::setConnectionPen(const QPen &pen) { m_connectionPen = pen; }
void GradientEditor::setShapePen(const QPen &pen) { m_pointPen = pen; }
void GradientEditor::setPointLock(int pos, LockType lock) { m_locks[pos] = lock | m_generalLock; }

void GradientEditor::setGeneralLock(LockType lock) { m_generalLock = lock; }

QPointF
GradientEditor::convertLocalToWidget(QPointF p)
{
  double x = m_bounds.x() + p.x()*m_bounds.width();
  double y = m_bounds.y() + p.y()*m_bounds.height();
  return QPointF(x,y);
}

QPointF
GradientEditor::convertWidgetToLocal(QPointF p)
{
  double x = (p.x()-m_bounds.x())/m_bounds.width();
  double y = (p.y()-m_bounds.y())/m_bounds.height();
  x = qBound(0.0, x, 1.0);
  y = qBound(0.0, y, 1.0);
  return QPointF(x,y);
}

QRectF
GradientEditor::pointBoundingRect(int i)
{
  QPointF p = convertLocalToWidget(m_points.at(i));
  double w = m_pointSize.width();
  double h = m_pointSize.height();
  double x = p.x() - w / 2;
  double y = p.y() - h / 2;

  return QRectF(x, y, w, h);
}

QRectF
GradientEditor::boundingRect() const
{
  if (m_bounds.isEmpty())
    return m_parent->rect();
  else
    return m_bounds;
}



QGradientStops
GradientEditor::gradientStops()
{
  QGradientStops gradStops;

  for(int i=0; i<m_points.size(); i++)
    {
      float pos, r,g,b,a;
      QPointF pt;
      pt = m_points[i];
      
      pos = pt.x();
      
      a = 255*(1-pt.y());
      r = m_colors[i].red();
      g = m_colors[i].green();
      b = m_colors[i].blue();

      gradStops << QGradientStop(pos, QColor(r,g,b,a));
    }

  return gradStops;
}

void
GradientEditor::setGradientStops(QGradientStops gradStops)
{
  m_points.clear();
  m_colors.clear();

  foreach (QGradientStop stop, gradStops)
  {
    QPointF pt;
    QColor col;

    pt = QPointF(stop.first,
		 1 - stop.second.alphaF());

    col = QColor(stop.second.red(),
		 stop.second.green(),
		 stop.second.blue());

    m_points << pt;
    m_colors << stop.second;
  }

  m_locks.clear();
  if (m_points.size() > 0) {
    m_locks.resize(m_points.size());
    m_locks.fill(m_generalLock);
  }
  m_locks[0] = LockToLeft | m_generalLock;
  m_locks[m_locks.size()-1] = LockToRight | m_generalLock;
}

GradientEditor::GradientEditor(QWidget *parent) :
  QWidget(parent)
{
  //setMouseTracking(true);

  m_parent = parent;

  m_generalLock = NoLock;
  m_border = 5;
  m_pointPen = QPen(QColor(255, 255, 255, 191), 1);
  m_connectionPen = QPen(QColor(255, 255, 255, 127), 2);
  m_pointSize = QSize(11, 11);
  m_currentIndex = -1;
  m_hoverIndex = -1;

  m_points.clear();
  m_locks.clear();
  m_colors.clear();

  m_bounds = m_parent->rect().adjusted(m_border, m_border,
				       -2*m_border, -2*m_border);
}

void
GradientEditor::resizeEvent(QResizeEvent *event)
{
  QRect newrect = rect();
  m_bounds = rect().adjusted(m_border+5, m_border+7,
			     -2*m_border, -2*m_border);
}

void
GradientEditor::paintEvent(QPaintEvent *event)
{
  QPainter p(this);

  p.setRenderHint(QPainter::Antialiasing);

  p.setPen(m_connectionPen);
  
  QPolygonF points;
  for (int i=0; i<m_points.size(); ++i)
    points << convertLocalToWidget(m_points.at(i));
  
  p.drawPolyline(points);


  p.setPen(m_pointPen);

  for (int i=0; i<m_points.size(); ++i)
    {
      //p.setBrush(QBrush(m_colors[i]));
      QColor clr = m_colors[i];
      clr.setAlpha(255);
      p.setBrush(QBrush(clr));
      QRectF bounds = pointBoundingRect(i);
      p.drawEllipse(bounds);
    }
  
  if (m_hoverIndex >= 0)
    {
      QRectF brect = boundingRect();
      QPointF hoverPoint = convertLocalToWidget(m_points[m_hoverIndex]);
      QRectF valRect;

      valRect = QRectF(hoverPoint.x()+7,
		       hoverPoint.y()-15,
		       45, 30);
      if ((valRect.x() + valRect.width()) >= (brect.x()+brect.width()))
	valRect.adjust(-60, 0, -60, 0);

      int dely;
      dely = (valRect.y() + valRect.height()) - (brect.y()+brect.height());
      if (dely > 0)
	valRect.adjust(0, -dely, 0, -dely);
      dely = valRect.y() - brect.y();
      if (dely < 0)
	valRect.adjust(0, -dely, 0, -dely);

      // draw position & alpha values
      p.setBrush(Qt::white);
      p.setPen(Qt::darkGray);
      p.drawRoundRect(valRect);
      
      float pos = m_points[m_hoverIndex].x();
      float alpha = 1 - m_points[m_hoverIndex].y();

      QPainterPath path;
      path.addText(valRect.x()+4,
		   valRect.y()+valRect.height()/2-4,
		   QFont("Helvetica", 10),
		   QString("p:%1").arg(pos, 0, 'f', 2));
      path.addText(valRect.x()+4,
		   valRect.y()+valRect.height()-4,
		   QFont("Helvetica", 10),
		   QString("a:%1").arg(alpha, 0, 'f', 2));
      p.setBrush(Qt::black);
      p.setPen(Qt::transparent);
      p.drawPath(path);
    }
  
}

QPointF
GradientEditor::bound_point(QPointF point, int lock)
{
    QPointF p = point;

    if (p.x() < 0 || (lock & GradientEditor::LockToLeft))
      p.setX(0);
    else if (p.x() > 1 || (lock & GradientEditor::LockToRight))
      p.setX(1);

    if (p.y() < 0 || (lock & GradientEditor::LockToTop))
      p.setY(0);
    else if (p.y() > 1 || (lock & GradientEditor::LockToBottom))
      p.setY(1);

    return p;
}

void
GradientEditor::movePoint(int index, QPointF point)
{
  // restrict the point movement between the previous and next points
  float delx = 1.0/m_bounds.width();
  QPointF pt;
  pt = bound_point(point, m_locks.at(index));
  if (index > 0)
    {
      if (pt.x() <= m_points[index-1].x())
	pt.setX(m_points[index-1].x()+delx);
    }
  if (index < m_points.size()-1)
    {
      if (pt.x() >= m_points[index+1].x())
	pt.setX(m_points[index+1].x()-delx);
    }

  m_points[index] = pt;

  emit refreshGradient();
}



void
GradientEditor::mouseMoveEvent(QMouseEvent *event)
{
  QPointF clickPos = event->pos();
  if (m_currentIndex >= 0)
    {
      QPointF pt = m_points[m_currentIndex];
      m_hoverIndex = m_currentIndex;
      
      movePoint(m_currentIndex, convertWidgetToLocal(clickPos));

      // move only vertically
      if (event->modifiers() & Qt::ShiftModifier)
	m_points[m_currentIndex].setX(pt.x());

      // move only horizontally
      if (event->modifiers() & Qt::AltModifier)
	m_points[m_currentIndex].setY(pt.y());

      return;
    }
  
  int prev_hoverIndex = m_hoverIndex;
  m_hoverIndex = -1;
  for (int i=0; i<m_points.size(); ++i)
    {
      QPainterPath path;
      path.addEllipse(pointBoundingRect(i));
      
      if (path.contains(clickPos))
	{
	  m_hoverIndex = i;
	  break;
	}
    }
  
  if (prev_hoverIndex != m_hoverIndex)
    update();
  
}

void
GradientEditor::mouseDoubleClickEvent(QMouseEvent *event)
{
  QPointF clickPos = event->pos();
  int index = -1;
  for (int i=0; i<m_points.size(); ++i)
    {
      QPainterPath path;
      path.addEllipse(pointBoundingRect(i));
      
      if (path.contains(clickPos))
	{
	  index = i;
	  break;
	}
    }
  if (index != -1)
    {
      QColor color = DColorDialog::getColor(m_colors[index]);
      if (color.isValid())
	{
	  m_colors[index] = color;
	  emit gradientChanged();
	}
    }
}

void
GradientEditor::mouseReleaseEvent(QMouseEvent *event)
{
  if (m_currentIndex > -1)
    emit gradientChanged();

  m_currentIndex = -1;
  m_hoverIndex = m_currentIndex;
}

void
GradientEditor::mousePressEvent(QMouseEvent *event)
{
  QPointF clickPos = event->pos();
  int index = -1;

  if (clickPos.x() <= m_bounds.x()+2)
    index = 0;
  else if (clickPos.x() >= m_bounds.x()+m_bounds.width()-2)
    index = m_points.size()-1;
  else
    {
      for (int i=0; i<m_points.size(); ++i)
	{
	  QPainterPath path;
	  path.addEllipse(pointBoundingRect(i));
	  
	  if (path.contains(clickPos))
	    {
	      index = i;
	      break;
	    }
	}
    }
  
  if (event->button() == Qt::LeftButton)
    {
      if (index == -1)
	{
	  int pos = 0;
	  for (int i=0; i<m_points.size(); ++i)
	    if (convertLocalToWidget(m_points.at(i)).x() > clickPos.x())
	      {
		pos = i;
		break;
	      }

	  m_points.insert(pos, convertWidgetToLocal(clickPos));
	  QColor color = m_colors[pos];
	  if (pos > 0)
	    {
	      float x0 = m_points.at(pos-1).x();
	      float x1 = m_points.at(pos+1).x();
	      float x = m_points.at(pos).x();
	      float frc = (x-x1)/(x0-x1);
	      int r0,g0,b0;
	      int r1, g1,b1;
	      m_colors[pos].getRgb(&r0, &g0, &b0);
	      m_colors[pos-1].getRgb(&r1, &g1, &b1);
	      int r = r0 + frc*(r1-r0);
	      int g = g0 + frc*(g1-g0);
	      int b = b0 + frc*(b1-b0);
	      color = QColor(r,g,b);
	    }
	  m_colors.insert(pos, color);
	  m_locks.insert(pos, m_generalLock);
	  m_currentIndex = pos;
	  m_hoverIndex = m_currentIndex;
	}
      else
	{
	  m_currentIndex = index;
	  m_hoverIndex = m_currentIndex;
	}
      
      return;
      
    }
  else if (event->button() == Qt::RightButton)
    {
      if (index >= 0)
	{
	  //if (m_locks[index] == 0)
	  if (index > 0 && index < m_points.size()-1)
	    {
	      m_locks.remove(index);
	      m_points.remove(index);
	      m_colors.remove(index);
	    }
	  emit gradientChanged();
	  return;
	}
    }
  
}

void GradientEditor::enterEvent(QEvent *e) { setFocus(); grabKeyboard(); }
void GradientEditor::leaveEvent(QEvent *e) { clearFocus(); releaseKeyboard(); }

void
GradientEditor::keyPressEvent(QKeyEvent *event)
{
  if (event->key() == Qt::Key_Space)
    {      
      askGradientChoice();
      update();
    }
  else if (event->key() == Qt::Key_S)
    {      
      saveGradientStops();      
    }
  else if (event->key() == Qt::Key_F)
    {      
      flipGradientStops();
    }
  else if (event->key() == Qt::Key_Up)
    {      
     scaleOpacity(1.25f);
    }
  else if (event->key() == Qt::Key_Down)
    {      
     scaleOpacity(0.8f);
    }
}


void
GradientEditor::scaleOpacity(float factor)
{
  QGradientStops origstops = gradientStops();
  QGradientStops stops;

  int npts = origstops.count();
  for(int i=0; i<npts; i++)
    {
      float pos = origstops.at(i).first;
      QColor col = origstops.at(i).second;
      int r = col.red();
      int g = col.green();
      int b = col.blue();
      int a = col.alpha();
      a = qBound(0, int(a*factor), 255);
      stops << QGradientStop(pos, QColor(r,g,b,a));
    }

  setGradientStops(stops);
  emit gradientChanged();
}

void
GradientEditor::flipGradientStops()
{
  QGradientStops origstops = gradientStops();
  QGradientStops stops;

  int npts = origstops.count();
  for(int i=npts-1; i>=0; i--)
    {
      float pos = 1.0 - origstops.at(i).first;
      QColor col = origstops.at(i).second;
      stops << QGradientStop(pos, col);
    }

  setGradientStops(stops);
  emit gradientChanged();
}

void
GradientEditor::copyGradientFile(QString stopsflnm)
{
#ifdef __APPLE__
  QDir app = QCoreApplication::applicationDirPath();
  app.cdUp();
  app.cdUp();
  app.cdUp();
  app.cd("Shared");
  app.cd("data");
#else
  QDir app = QCoreApplication::applicationDirPath();
  app.cdUp();
  app.cd("data");
#endif // __APPLE__
      
  QString sflnm = QFileInfo(app, "gradientstops.xml").absoluteFilePath();
  QFileInfo fi(sflnm);
  if (! fi.exists())
    {
      QMessageBox::information(0, "Gradient Stops",
			       QString("Gradient Stops file does not exit at %1").arg(sflnm));
      
      return;
    }
  // copy information from DRISHTI/data/gradientstops.xml to HOME/drishtigradients.xml
  QDomDocument document;
  QFile f(sflnm);
  if (f.open(QIODevice::ReadOnly))
    {
      document.setContent(&f);
      f.close();
    }
  
  QFile fout(stopsflnm);
  if (fout.open(QIODevice::WriteOnly))
    {
      QTextStream out(&fout);
      document.save(out, 2);
      fout.close();
    }
}

void
GradientEditor::askGradientChoice()
{
  QString homePath = QDir::homePath();
  QFileInfo sfi(homePath, ".drishtigradients.xml");
  QString stopsflnm = sfi.absoluteFilePath();
  if (!sfi.exists())
    copyGradientFile(stopsflnm);

  QDomDocument document;
  QFile f(stopsflnm);
  if (f.open(QIODevice::ReadOnly))
    {
      document.setContent(&f);
      f.close();
    }

  QStringList glist;

  QDomElement main = document.documentElement();
  QDomNodeList dlist = main.childNodes();
  for(int i=0; i<dlist.count(); i++)
    {
      if (dlist.at(i).nodeName() == "gradient")
	{
	  QDomNodeList cnode = dlist.at(i).childNodes();
	  for(int j=0; j<cnode.count(); j++)
	    {
	      QDomElement dnode = cnode.at(j).toElement();
	      if (dnode.nodeName() == "name")
		glist << dnode.text();
	    }
	}
    }

  bool ok;
  QString gstr = QInputDialog::getItem(0,
				       "Color Gradient",
				       "Color Gradient",
				       glist, 0, false,
				       &ok);
  if (!ok)
    return;

  int cno = -1;
  for(int i=0; i<dlist.count(); i++)
    {
      if (dlist.at(i).nodeName() == "gradient")
	{
	  QDomNodeList cnode = dlist.at(i).childNodes();
	  for(int j=0; j<cnode.count(); j++)
	    {
	      QDomElement dnode = cnode.at(j).toElement();
	      if (dnode.tagName() == "name" && dnode.text() == gstr)
		{
		  cno = i;
		  break;
		}
	    }
	}
    }
	
  if (cno < 0)
    return;

  QGradientStops stops;
  QDomNodeList cnode = dlist.at(cno).childNodes();
  for(int j=0; j<cnode.count(); j++)
    {
      QDomElement de = cnode.at(j).toElement();
      if (de.tagName() == "gradientstops")
	{
	  QString str = de.text();
	  QStringList strlist = str.split(" ", QString::SkipEmptyParts);
	  for(int j=0; j<strlist.count()/5; j++)
	    {
	      float pos, r,g,b,a;
	      pos = strlist[5*j].toFloat();
	      r = strlist[5*j+1].toInt();
	      g = strlist[5*j+2].toInt();
	      b = strlist[5*j+3].toInt();
	      a = strlist[5*j+4].toInt();
	      stops << QGradientStop(pos, QColor(r,g,b,a));
	    }
	}
    }
  setGradientStops(stops);
  emit gradientChanged();
}

void
GradientEditor::saveGradientStops()
{
  QString gname;
  bool ok;
  QString text;
  text = QInputDialog::getText(this,
			      "Save Gradient Stops",
			      "Name for the stops",
			      QLineEdit::Normal,
			      "",
			      &ok);
  
  if (ok && !text.isEmpty())
    gname = text;
  else
    {
      QMessageBox::information(0, "Save Gradient Stops",
			       "Please specify name for the gradient stops");
      return;
    }

  QString homePath = QDir::homePath();
  QFileInfo sfi(homePath, ".drishtigradients.xml");
  QString stopsflnm = sfi.absoluteFilePath();
  if (!sfi.exists())
    copyGradientFile(stopsflnm);

//
//#ifdef __APPLE__
//  QDir app = QCoreApplication::applicationDirPath();
//  app.cdUp();
//  app.cdUp();
//  app.cdUp();
//  app.cd("Shared");
//  app.cd("data");
//#else
//  QDir app = QCoreApplication::applicationDirPath();
//  app.cdUp();
//  app.cd("data");
//#endif // __APPLE__
//
//
//  QString stopsflnm = QFileInfo(app, "gradientstops.xml").absoluteFilePath();
//  QFileInfo fi(stopsflnm);
//  if (! fi.exists())
//    {
//      QMessageBox::information(0, "Gradient Stops",
//			       QString("Gradient Stops File does not exit at %1").arg(stopsflnm));
//
//      return;
//    }    


  QDomDocument doc;
  QFile f(stopsflnm);
  if (f.open(QIODevice::ReadOnly))
    {
      doc.setContent(&f);
      f.close();
    }

  //-------------------------------------------------
  QDomElement de = doc.createElement("gradient");
  QDomElement de0 = doc.createElement("name");
  QDomElement de1 = doc.createElement("gradientstops");
  QDomText tn0 = doc.createTextNode(gname);
  QString str;
  for(int i=0; i<m_points.count(); i++)
    {
      float pos;
      int r,g,b,a;
      QPointF pt;
      pt = m_points[i];      
      pos = pt.x();      
      a = 255*(1-pt.y());
      r = m_colors[i].red();
      g = m_colors[i].green();
      b = m_colors[i].blue();

      str += QString("%1 %2 %3 %4 %5   ").arg(pos). \
	                arg(r).arg(g).arg(b).arg(a);
    }
  QDomText tn1 = doc.createTextNode(str);
  de0.appendChild(tn0);
  de1.appendChild(tn1);
  de.appendChild(de0);
  de.appendChild(de1);
  //-------------------------------------------------



  QDomElement topElement = doc.documentElement();

  int replace = -1;
  QDomNodeList dlist = topElement.childNodes();
  for(int i=0; i<dlist.count(); i++)
    {
      if (dlist.at(i).nodeName() == "gradient")
	{
	  QDomNodeList cnode = dlist.at(i).childNodes();
	  for(int j=0; j<cnode.count(); j++)
	    {
	      QDomElement dnode = cnode.at(j).toElement();
	      if (dnode.nodeName() == "name" &&
		  dnode.text() == gname)
		{
		  replace = i;
		  break;
		}
	    }
	}
    }

  if (replace >= 0)
    {
      QStringList items;
      items << "Yes";
      items << "No";
      bool ok;
      QString str;
      str = QInputDialog::getItem(0,
				  "Replace existing gradient stops",
				  QString("Replace %1").arg(gname),
				  items,
				  0,
				  false, // text is not editable
				  &ok);
      if (!ok || str == "No")
	return;

      topElement.replaceChild(de, dlist.at(replace));
    }
  else
    topElement.appendChild(de);


  QFile fout(stopsflnm);
  if (fout.open(QIODevice::WriteOnly))
    {
      QTextStream out(&fout);
      doc.save(out, 2);
      fout.close();
    }

  QMessageBox::information(0, "Save Gradient Stops", "Saved");
}
