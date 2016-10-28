#include <math.h>

#include "splinetransferfunction.h"
#include "staticfunctions.h"
#include "global.h"


QImage SplineTransferFunction::colorMapImage() { return m_colorMapImage; }
int SplineTransferFunction::size() { return m_points.size(); }
QGradientStops SplineTransferFunction::gradientStops() { return m_gradientStops; }

QString SplineTransferFunction::name() {return m_name;}
void SplineTransferFunction::setName(QString name) {m_name = name;}

void SplineTransferFunction::setOn(int i, bool f)
{
  while(m_on.count() <= i)
    m_on << false;

  m_on[i] = f;
}
bool SplineTransferFunction::on(int i)
{
  if (m_on.count() <= i)
    return false;
  else
    return m_on[i];
}


void
SplineTransferFunction::switch1D()
{
  if (Global::use1D() == false)
    return;

  QPointF pt = m_points[0];
  m_points.clear();
  pt.setY(1.0);
  m_points << pt;
  pt.setY(0.0);
  m_points << pt;

  pt = m_normalWidths[0];
  m_normalWidths.clear();
  m_normalWidths << pt;
  m_normalWidths << pt;

  m_normalRotations.clear();
  m_normalRotations << 0.0
                    << 0.0;
  

  updateNormals();
  updateColorMapImage();
}


SplineTransferFunction::SplineTransferFunction() : QObject()
{
  m_name = "TF";

  m_on.clear();

  m_points.clear();
  m_points << QPointF(0.5, 1.0)
           << QPointF(0.5, 0.0);

  m_normals.clear();
  m_rightNormals.clear();
  m_leftNormals.clear();

  m_normalWidths.clear();
  m_normalWidths << QPointF(0.4, 0.4)
		 << QPointF(0.4, 0.4);

  m_normalRotations.clear();
  m_normalRotations << 0.0
                    << 0.0;

  m_gradientStops.clear();
  m_gradientStops << QGradientStop(0.0, QColor(50,50,50,128))
                  << QGradientStop(1.0, QColor(255,255,255,128));

  m_colorMapImage = QImage(256, 256, QImage::Format_ARGB32);
  m_colorMapImage.fill(0);

  switch1D();

  updateNormals();
  updateColorMapImage();
}

SplineTransferFunction::~SplineTransferFunction()
{
  m_on.clear();
  m_name.clear();
  m_points.clear();
  m_normals.clear();
  m_rightNormals.clear();
  m_leftNormals.clear();
  m_normalWidths.clear();
  m_normalRotations.clear();
  m_gradientStops.clear();
}

QDomElement
SplineTransferFunction::domElement(QDomDocument& doc)
{
  QDomElement de = doc.createElement("transferfunction");
  QString str;

  // -- name
  QDomText tn0 = doc.createTextNode(m_name);

  // -- points
  str.clear();
  for(int i=0; i<m_points.count(); i++)
    str += QString("%1 %2  ").arg(m_points[i].x()).arg(m_points[i].y());
  QDomText tn1 = doc.createTextNode(str);

  // -- normalWidths
  str.clear();
  for(int i=0; i<m_points.count(); i++)
    str += QString("%1 %2  ").arg(m_normalWidths[i].x()).arg(m_normalWidths[i].y());
  QDomText tn2 = doc.createTextNode(str);

  // -- normalRotations
  str.clear();
  for(int i=0; i<m_points.count(); i++)
    str += QString("%1 ").arg(m_normalRotations[i]);
  QDomText tn3 = doc.createTextNode(str);
  
  // -- gradientStops
  str.clear();
  for(int i=0; i<m_gradientStops.count(); i++)
    {
      float pos = m_gradientStops[i].first;
      QColor color = m_gradientStops[i].second;
      str += QString("%1 %2 %3 %4 %5   ").arg(pos).			\
	arg(color.red()).arg(color.green()).arg(color.blue()).arg(color.alpha());
    }
  QDomText tn4 = doc.createTextNode(str);

  // -- sets
  str.clear();
  for(int i=0; i<m_on.count(); i++)
    str += QString("%1 ").arg(m_on[i]);
  QDomText tn5 = doc.createTextNode(str);


  QDomElement de0 = doc.createElement("name");
  QDomElement de1 = doc.createElement("points");
  QDomElement de2 = doc.createElement("normalwidths");
  QDomElement de3 = doc.createElement("normalrotations");
  QDomElement de4 = doc.createElement("gradientstops");
  QDomElement de5 = doc.createElement("sets");

  de0.appendChild(tn0);
  de1.appendChild(tn1);
  de2.appendChild(tn2);
  de3.appendChild(tn3);
  de4.appendChild(tn4);
  de5.appendChild(tn5);

  de.appendChild(de0);
  de.appendChild(de1);
  de.appendChild(de2);
  de.appendChild(de3);
  de.appendChild(de4);
  de.appendChild(de5);

  return de;
}

void
SplineTransferFunction::fromDomElement(QDomElement de)
{
  m_on.clear();
  m_name.clear();
  m_points.clear();
  m_normals.clear();
  m_rightNormals.clear();
  m_leftNormals.clear();
  m_normalWidths.clear();
  m_normalRotations.clear();
  m_gradientStops.clear();

  QDomNodeList dlist = de.childNodes();
  for(int i=0; i<dlist.count(); i++)
    {
      QDomElement dnode = dlist.at(i).toElement();
      if (dnode.tagName() == "name")
	{
	  m_name = dnode.toElement().text();
	}
      else if (dnode.tagName() == "points")
	{
	  QString str = dnode.toElement().text();
	  QStringList strlist = str.split(" ", QString::SkipEmptyParts);
	  for(int j=0; j<strlist.count()/2; j++)
	    {
	      float x,y;
	      x = strlist[2*j].toFloat();
	      y = strlist[2*j+1].toFloat();
	      m_points << QPointF(x,y);
	    }
	}
      else if (dnode.tagName() == "normalwidths")
	{
	  QString str = dnode.toElement().text();
	  QStringList strlist = str.split(" ", QString::SkipEmptyParts);
	  for(int j=0; j<strlist.count()/2; j++)
	    {
	      float x,y;
	      x = strlist[2*j].toFloat();
	      y = strlist[2*j+1].toFloat();
	      m_normalWidths << QPointF(x,y);
	    }
	}
      else if (dnode.tagName() == "normalrotations")
	{
	  QString str = dnode.toElement().text();
	  QStringList strlist = str.split(" ", QString::SkipEmptyParts);
	  for(int j=0; j<strlist.count(); j++)
	    m_normalRotations << strlist[j].toFloat();
	}
      else if (dnode.tagName() == "gradientstops")
	{
	  QString str = dnode.toElement().text();
	  QStringList strlist = str.split(" ", QString::SkipEmptyParts);
	  for(int j=0; j<strlist.count()/5; j++)
	    {
	      float pos, r,g,b,a;
	      pos = strlist[5*j].toFloat();
	      r = strlist[5*j+1].toInt();
	      g = strlist[5*j+2].toInt();
	      b = strlist[5*j+3].toInt();
	      a = strlist[5*j+4].toInt();
	      m_gradientStops << QGradientStop(pos, QColor(r,g,b,a));
	    }
	}
      else if (dnode.tagName() == "sets")
	{
	  QString str = dnode.toElement().text();
	  QStringList strlist = str.split(" ", QString::SkipEmptyParts);
	  for(int j=0; j<strlist.count(); j++)
	    m_on.append(strlist[j].toInt() > 0);
	}
    }

  updateNormals();
  updateColorMapImage();
}


SplineInformation
SplineTransferFunction::getSpline()
{
  SplineInformation splineInfo;
  splineInfo.setName(m_name);
  splineInfo.setOn(m_on);
  splineInfo.setPoints(m_points);
  splineInfo.setNormalWidths(m_normalWidths);
  splineInfo.setNormalRotations(m_normalRotations);
  splineInfo.setGradientStops(m_gradientStops);
  return splineInfo;
}

void
SplineTransferFunction::setSpline(SplineInformation splineInfo)
{  
  m_name = splineInfo.name();
  m_on = splineInfo.on();
  m_points = splineInfo.points();
  m_normalWidths = splineInfo.normalWidths();
  m_normalRotations = splineInfo.normalRotations();
  m_gradientStops = splineInfo.gradientStops();

  updateNormals();
  updateColorMapImage();
}

void
SplineTransferFunction::updateNormals()
{
  m_normals.clear();
  m_rightNormals.clear();
  m_leftNormals.clear();

  for (int i=0; i<m_points.size(); ++i)
    {
      QLineF ln;
      if (i == 0)
	ln = QLineF(m_points[i], m_points[i+1]);
      else if (i == m_points.size()-1)
	ln = QLineF(m_points[i-1], m_points[i]);
      else
	ln = QLineF(m_points[i-1], m_points[i+1]);
      
      QLineF unitVec;
      if (ln.length() > 0)
	unitVec = ln.normalVector().unitVector();
      else
	unitVec = QLineF(QPointF(0,0), QPointF(1,0));

      unitVec.translate(-unitVec.p1());

      float a = m_normalRotations[i];
      QPointF p1 = unitVec.p2();
      QPointF p2;
      p2.setX(p1.x()*cos(a) + p1.y()*sin(a));
      p2.setY(-p1.x()*sin(a) + p1.y()*cos(a));
      unitVec = QLineF(QPointF(0,0), p2);

      QPointF v1, v2;
      v1 = m_points[i] + m_normalWidths[i].x()*unitVec.p2();
      v2 = m_points[i] - m_normalWidths[i].y()*unitVec.p2();

      m_normals << unitVec.p2();
      m_rightNormals << v1;
      m_leftNormals << v2;
    }  
}

void
SplineTransferFunction::updateColorMapImage()
{
  updateColorMapImageFor16bit();
  return;

  m_colorMapImage.fill(0);

  QPainter colorMapPainter(&m_colorMapImage);
  colorMapPainter.setCompositionMode(QPainter::CompositionMode_Source);

  QPolygonF pointsLeft;
  pointsLeft.clear();
  for (int i=0; i<m_points.size(); i++)
    {
      QPointF pt = m_leftNormals[i];
      pointsLeft << QPointF(pt.x()*255, pt.y()*255);
    }
  
	  
  QPolygonF pointsRight;
  pointsRight.clear();
  for (int i=0; i<m_points.size(); i++)
    {
      QPointF pt = m_rightNormals[i];
      pointsRight << QPointF(pt.x()*255, pt.y()*255);
    }

  QGradientStops gstops = StaticFunctions::resampleGradientStops(m_gradientStops);
  for (int i=1; i<m_points.size(); i++)
    {
      QPainterPath pathRight, pathLeft;
      getPainterPathSegment(&pathRight, pointsRight, i);
      getPainterPathSegment(&pathLeft, pointsLeft, i);
      
      float pathLen = 1.5*qMax(pathRight.length(), pathLeft.length());
      for (int l=0; l<pathLen+1; l++)
	{
	  QPointF vLeft, vRight;
	  float frc = (float)l/(float)pathLen;
	  
	  vLeft = pathLeft.pointAtPercent(frc);
	  vRight = pathRight.pointAtPercent(frc);
	  
	  QLinearGradient lg(vRight.x(), vRight.y(),
			     vLeft.x(), vLeft.y());
	  lg.setStops(gstops);
	  
	  QPen pen;
	  pen.setBrush(QBrush(lg));
	  pen.setWidth(2);
	  
	  colorMapPainter.setPen(pen);
	  colorMapPainter.drawLine(vRight, vLeft);
	}
    }
}

void
SplineTransferFunction::updateColorMapImageFor16bit()
{
  m_colorMapImage.fill(0);

  int val0, val1;
  if (Global::bytesPerVoxel() == 1)
    {
      val0 = m_leftNormals[0].x()*255;
      val1 = m_rightNormals[0].x()*255;
    }
  else
    {
      val0 = m_leftNormals[0].x()*65535;
      val1 = m_rightNormals[0].x()*65535;
    }
  if (val1 < val0)
    {
      int vtmp = val0;
      val0 = val1;
      val1 = vtmp;
    }


  QGradientStops gstops;

//  // limit opacity to 252 to avoid overflow
//  for(int i=0; i<m_gradientStops.size(); i++)
//    {
//      float pos = m_gradientStops[i].first;
//      QColor color = m_gradientStops[i].second;
//      int r = color.red();
//      int g = color.green();
//      int b = color.blue();
//      int a = color.alpha();
//      a = qMin(252, a);
//      gstops << QGradientStop(pos, QColor(r,g,b,a));
//    }
//  gstops = StaticFunctions::resampleGradientStops(gstops, 101);

  gstops = StaticFunctions::resampleGradientStops(m_gradientStops, 101);
  
  for (int i=val0; i<=val1; i++)
    {
      float frc = (float)(i-val0)/(float)(val1-val0);
      int g0 = frc*100;
      int g1 = qMin(100, g0+1);
      frc = frc*100 - g0;
      if (g0 == g1) frc = 0.0;
      
      QColor color0 = gstops[g0].second;
      QColor color1 = gstops[g1].second;

      float rb,gb,bb,ab, re,ge,be,ae,r,g,b,a;
      rb = color0.red();
      gb = color0.green();
      bb = color0.blue();
      ab = color0.alpha();
      re = color1.red();
      ge = color1.green();
      be = color1.blue();
      ae = color1.alpha();
      r = rb + frc*(re-rb);
      g = gb + frc*(ge-gb);
      b = bb + frc*(be-bb);
      a = ab + frc*(ae-ab);
      QColor finalColor = QColor(r, g, b, a);
      
      int x = i/256;
      int y = i%256;
      m_colorMapImage.setPixel(y,255-x, qRgba(r,g,b,a));
    }
}

void
SplineTransferFunction::getPainterPathSegment(QPainterPath *path,
					      QPolygonF points,
					      int i)
{
  path->moveTo(points[i-1]);

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
  if (l1.length() > 0)
    l1 = l1.unitVector();
  else
    l1 = QLineF(l1.p1(), l1.p1()+QPointF(1,0));

  l0 = QLineF(p1, midpt);
  dotp = l0.dx()*l1.dx() + l0.dy()*l1.dy();
  
  if (dotp < 0)
    l1.setLength(-dotp);
  else
    l1.setLength(dotp);
  
  c1 = p1 + QPointF(l1.dx(), l1.dy());
  
  
  
  l2 = QLineF(p1, p2);;
  if (i < m_points.size()-1)
    l2 = QLineF(p1, points[i+1]);
  if (l2.length() > 0)
    l2 = l2.unitVector();
  else
    l2 = QLineF(l2.p1(), l2.p1()+QPointF(1,0));
  
  l0 = QLineF(p2, midpt);
  dotp = l0.dx()*l2.dx() + l0.dy()*l2.dy();
  
  if (dotp < 0)
    l2.setLength(-dotp);
  else
    l2.setLength(dotp);
  
  c2 = p2 - QPointF(l2.dx(), l2.dy());	  
  


  path->cubicTo(c1, c2, p2);
}

void
SplineTransferFunction::setGradientStops(QGradientStops stop)
{
  m_gradientStops = stop;
  updateColorMapImage();
}

QPointF
SplineTransferFunction::pointAt(int i)
{
  if (i < m_points.size())
    return m_points[i];
  else
    return QPointF(-1,-1);
}

QPointF
SplineTransferFunction::rightNormalAt(int i)
{
  if (i < m_points.size())
    return m_rightNormals[i];
  else
    return QPointF(-1,-1);
}

QPointF
SplineTransferFunction::leftNormalAt(int i)
{
  if (i < m_points.size())
    return m_leftNormals[i];
  else
    return QPointF(-1,-1);
}

void
SplineTransferFunction::removePointAt(int index)
{
  if (Global::use1D())
    return;

  m_points.remove(index);
  m_normalRotations.remove(index);
  m_normalWidths.remove(index);
  updateNormals();
  updateColorMapImage();
}

void
SplineTransferFunction::appendPoint(QPointF sp)
{
  if (Global::use1D())
    return;

  m_points.push_back(sp);
  
  float rot = m_normalRotations[m_normalRotations.size()-1];
  m_normalRotations.push_back(rot);
  
  QPointF nw = m_normalWidths[m_normalWidths.size()-1];
  m_normalWidths.push_back(nw);

  updateNormals();
  updateColorMapImage();
}


void
SplineTransferFunction::insertPointAt(int i, QPointF sp)
{
  if (Global::use1D())
    return;

  if (i == 0)
    {
      m_points.insert(i, sp);

      float rot = m_normalRotations[i];
      m_normalRotations.insert(i, rot);

      QPointF nw = m_normalWidths[i];
      m_normalWidths.insert(i, nw);
    }
  else
    {
      m_points.insert(i, sp);
      
      float rot = (m_normalRotations[i]+m_normalRotations[i-1])/2;
      m_normalRotations.insert(i, rot);
      
      QPointF nw = (m_normalWidths[i]+m_normalWidths[i-1])/2;
      m_normalWidths.insert(i, nw);
    }

  updateNormals();
  updateColorMapImage();
}

void
SplineTransferFunction::moveAllPoints(const QPointF diff)
{
  for (int i=0; i<m_points.size(); i++)
    m_points[i] += diff;

  updateNormals();
  updateColorMapImage();
}

void
SplineTransferFunction::movePointAt(int index,
				    QPointF point)
{
  if (Global::use1D())
    {
      m_points[0].setX(point.x());      
      m_points[1].setX(point.x());
    }
  else
      m_points[index] = point;

  updateNormals();
  updateColorMapImage();
}

void
SplineTransferFunction::rotateNormalAt(int idxCenter,
				       int idxNormal,
				       QPointF point)
{
  if (Global::use1D())
    return;

  QLineF dir, normVec, ln;

  if (idxCenter == 0)
    dir = QLineF(m_points[idxCenter], m_points[idxCenter+1]);
  else if (idxCenter == m_points.size()-1)
    dir = QLineF(m_points[idxCenter-1], m_points[idxCenter]);
  else
    dir = QLineF(m_points[idxCenter-1], m_points[idxCenter+1]);
  
  if (dir.length() > 0)
    dir = dir.unitVector();
  else
    dir = QLineF(dir.p1(), dir.p1()+QPointF(1,0));
  normVec = dir.normalVector();

  ln = QLineF(m_points[idxCenter], point);
  ln.translate(-ln.p1());

  float angle = 0;;
  if (idxNormal == RightNormal)
    {
      float angle1, angle2;
      angle1 = atan2(-ln.dy(), ln.dx());
      angle2 = atan2(-normVec.dy(), normVec.dx());
      angle = angle1-angle2;
    }
  else // LeftNormal
    {
      float angle1, angle2;
      angle1 = atan2(-ln.dy(), ln.dx());
      angle2 = atan2(normVec.dy(), -normVec.dx());
      angle = angle1-angle2;
    }
      
  m_normalRotations[idxCenter] = angle;

  updateNormals();
  updateColorMapImage();
}

void
SplineTransferFunction::moveNormalAt(int idxCenter,
				     int idxNormal,
				     QPointF point,
				     bool shiftModifier)
{
  if (idxNormal == RightNormal) // move rigntNormal
    {
      QLineF l0 = QLineF(m_points[idxCenter], point);
      float dotp = (m_normals[idxCenter].x()*l0.dx() +
		    m_normals[idxCenter].y()*l0.dy());
      if (dotp >= 0)
	m_normalWidths[idxCenter].setX(dotp);
      else
	m_normalWidths[idxCenter].setX(0);

      if (shiftModifier)
	{
	  float w = m_normalWidths[idxCenter].x();
	  m_normalWidths[idxCenter].setY(w);
	}
    }
  else if (idxNormal == LeftNormal) // move rigntNormal
    {      
      QLineF l0 = QLineF(point, m_points[idxCenter]);
      float dotp = (m_normals[idxCenter].x()*l0.dx() +
		    m_normals[idxCenter].y()*l0.dy());
      if (dotp >= 0)
	m_normalWidths[idxCenter].setY(dotp);
      else
	m_normalWidths[idxCenter].setY(0);

      if (shiftModifier)
	{
	  float w = m_normalWidths[idxCenter].y();
	  m_normalWidths[idxCenter].setX(w);
	}	  
    }

  if (Global::use1D())
    {
      if (idxCenter == 0)
	m_normalWidths[1] = m_normalWidths[0];
      else
	m_normalWidths[0] = m_normalWidths[1];
    }

  updateNormals();
  updateColorMapImage();
}

void
SplineTransferFunction::set16BitPoint(float tmin, float tmax)
{
  float tmid = (tmax+tmin)*0.5;
  float tlen2 = (tmax-tmin)*0.5;
  m_points.clear();
  m_points << QPointF(tmid, 1.0)
	   << QPointF(tmid, 0.0);

  m_normalWidths.clear();
  m_normalWidths << QPointF(tlen2,tlen2)
		 << QPointF(tlen2,tlen2);

  m_normalRotations.clear();
  m_normalRotations << 0.0
                    << 0.0;

  updateNormals();
  updateColorMapImage();
}
