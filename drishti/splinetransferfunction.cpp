#include <math.h>

#include "global.h"
#include "staticfunctions.h"
#include "splinetransferfunction.h"

#define qClamp(val, min, max) qMin(qMax(val, min), max)


//------------------------------------------------------------------
SplineTransferFunctionUndo::SplineTransferFunctionUndo() { clear(); }
SplineTransferFunctionUndo::~SplineTransferFunctionUndo() { clear(); }

void
SplineTransferFunctionUndo::clear()
{
  m_splineInfo.clear();
  m_index = -1;
}

void
SplineTransferFunctionUndo::clearTop()
{
  if (m_index == m_splineInfo.count()-1)
    return;

  while(m_index < m_splineInfo.count()-1)
    m_splineInfo.removeLast();
}

void
SplineTransferFunctionUndo::append(SplineInformation si)
{
  clearTop();
  m_splineInfo << si;
  m_index = m_splineInfo.count()-1;
}

void SplineTransferFunctionUndo::redo() { m_index = qMin(m_index+1, m_splineInfo.count()-1); }
void SplineTransferFunctionUndo::undo() { m_index = qMax(m_index-1, 0); }

SplineInformation
SplineTransferFunctionUndo::splineInfo()
{
  SplineInformation si;

  if (m_index >= 0 && m_index < m_splineInfo.count())
    si = m_splineInfo[m_index];

  return si;
}
//------------------------------------------------------------------




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

void SplineTransferFunction::setGradLimits(int bot, int top)
{
  m_gbot = qBound(0, bot, 255);
  m_gtop = qBound(0, top, 255);
  updateColorMapImage();
}
void SplineTransferFunction::gradLimits(int& bot, int& top)
{
  bot = m_gbot;
  top = m_gtop;
}

void SplineTransferFunction::setOpmod(float bot, float top)
{
  m_gbotop = qBound(0.0f, bot, 1.0f);
  m_gtopop = qBound(0.0f, top, 1.0f);
  updateColorMapImage();
}
void SplineTransferFunction::opMod(float& bot, float& top)
{
  bot = m_gbotop;
  top = m_gtopop;
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


SplineTransferFunction::SplineTransferFunction(QObject * parent) : QObject(parent)
{
  m_name = "TF";

  m_gbot = 0;
  m_gtop = 255;
  m_gbotop = 1.0;
  m_gtopop = 1.0;

  m_on.clear();

  m_points.clear();
  if (Global::volumeType() != Global::RGBVolume &&
      Global::volumeType() != Global::RGBAVolume)
    {
      m_points << QPointF(0.5, 1.0)
	       << QPointF(0.5, 0.25);
    }
  else
    {
      m_points << QPointF(0.5, 1.0)
	       << QPointF(0.5, 0.0);
    }

  m_normals.clear();
  m_rightNormals.clear();
  m_leftNormals.clear();

  m_normalWidths.clear();
  if (Global::volumeType() != Global::RGBVolume &&
      Global::volumeType() != Global::RGBAVolume)
    {
      m_normalWidths << QPointF(0.05, 0.05)
		     << QPointF(0.3, 0.3);
    }
  else
    {
      m_normalWidths << QPointF(0.5, 0.5)
		     << QPointF(0.5, 0.5);
    }

  m_normalRotations.clear();
  m_normalRotations << 0.0
                    << 0.0;

  m_gradientStops.clear();
  if (Global::volumeType() != Global::RGBVolume &&
      Global::volumeType() != Global::RGBAVolume)
    {
      m_gradientStops << QGradientStop(0.0, QColor(0,0,0,0))
		      << QGradientStop(0.5, QColor(255,255,255,128))
		      << QGradientStop(1.0, QColor(0,0,0,0));
    }
  else
    {
      m_gradientStops << QGradientStop(0.0, QColor(255,255,255,128))
		      << QGradientStop(1.0, QColor(255,255,255,128));
    }

  m_colorMapImage = QImage(256, 256, QImage::Format_ARGB32);
  m_colorMapImage.fill(0);

  switch1D();

  updateNormals();
  updateColorMapImage();

  m_undo.clear();
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

  m_undo.clear();
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


  // -- gradmodop
  str.clear();
  str += QString("%1 %2 %3 %4 ").\
    arg(m_gbot).arg(m_gtop).arg(m_gbotop).arg(m_gtopop);
  QDomText tn6 = doc.createTextNode(str);

  QDomElement de0 = doc.createElement("name");
  QDomElement de1 = doc.createElement("points");
  QDomElement de2 = doc.createElement("normalwidths");
  QDomElement de3 = doc.createElement("normalrotations");
  QDomElement de4 = doc.createElement("gradientstops");
  QDomElement de5 = doc.createElement("sets");
  QDomElement de6 = doc.createElement("gradmodop");

  de0.appendChild(tn0);
  de1.appendChild(tn1);
  de2.appendChild(tn2);
  de3.appendChild(tn3);
  de4.appendChild(tn4);
  de5.appendChild(tn5);
  de6.appendChild(tn6);

  de.appendChild(de0);
  de.appendChild(de1);
  de.appendChild(de2);
  de.appendChild(de3);
  de.appendChild(de4);
  de.appendChild(de5);
  de.appendChild(de6);

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
  m_gbot = 0;
  m_gtop = 255;
  m_gbotop = 1.0;
  m_gtopop = 1.0;

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
      else if (dnode.tagName() == "gradmodop")
	{
	  QString str = dnode.toElement().text();
	  QStringList strlist = str.split(" ", QString::SkipEmptyParts);
	  if (strlist.count() == 4)
	    {
	      m_gbot = strlist[0].toInt();
	      m_gtop = strlist[1].toInt();
	      m_gbotop = strlist[2].toFloat();
	      m_gtopop = strlist[3].toFloat();
	    }
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
  splineInfo.setGradLimits(m_gbot, m_gtop);
  splineInfo.setOpmod(m_gbotop, m_gtopop);
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
  splineInfo.gradLimits(m_gbot, m_gtop);
  splineInfo.opMod(m_gbotop, m_gtopop);

  updateNormals();
  updateColorMapImage();

  m_undo.append(splineInfo);
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
  if (Global::pvlVoxelType() > 0)
    {
      updateColorMapImageFor16bit();
      return;
    }

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

  //QGradientStops gstops = StaticFunctions::resampleGradientStops(m_gradientStops);
  QGradientStops gstops;
  // limit opacity to 252 to avoid overflow
  for(int i=0; i<m_gradientStops.size(); i++)
    {
      float pos = m_gradientStops[i].first;
      QColor color = m_gradientStops[i].second;
      int r = color.red();
      int g = color.green();
      int b = color.blue();
      int a = color.alpha();
      a = qMin(252, a);
      gstops << QGradientStop(pos, QColor(r,g,b,a));
    }
  gstops = StaticFunctions::resampleGradientStops(gstops);

  QGradientStops ogstops = gstops;

  int tpathlen = 0;
  for (int i=1; i<m_points.size(); i++)
    {
      QPainterPath pathRight, pathLeft;
      getPainterPathSegment(&pathRight, pointsRight, i);
      getPainterPathSegment(&pathLeft, pointsLeft, i);
      float pathLen = 1.5*qMax(pathRight.length(), pathLeft.length());
      tpathlen += pathLen;
    }

  bool useRadial = (qAbs(m_gtopop-m_gbotop) < 0.001 &&
		    qAbs(m_gtopop-0.5f) < 0.001);
  float gbot = m_gbot/255.0;
  float gtop = m_gtop/255.0;
  int ptn = 0;
  for (int i=1; i<m_points.size(); i++)
    {
      QPainterPath pathRight, pathLeft;
      getPainterPathSegment(&pathRight, pointsRight, i);
      getPainterPathSegment(&pathLeft, pointsLeft, i);
      
      float pathLen = 1.5*qMax(pathRight.length(), pathLeft.length());
      for (int l=0; l<pathLen+1; l++)
	{
	  QPointF vLeft, vRight;
	  float frc = (float)l/((float)pathLen);

	  // Note: pointAtPercent must be clamped to 0.0 -> 1.0 or an error will be printed
	  frc = frc < 0.0f ? 0.0f : frc;
	  frc = frc > 1.0f ? 1.0f : frc;
	  
	  vLeft = pathLeft.pointAtPercent(frc);
	  vRight = pathRight.pointAtPercent(frc);
	  

	  // modulate opacity based on path length
	  if (m_gbotop < 1 || m_gtopop < 1)
	    {
	      float pfrc = (ptn*1.0)/tpathlen;
	      if (useRadial)
		pfrc = 1.0-qBound(0.0f,2.0f*qAbs(0.5f-pfrc),1.0f);
	      else
		{
		  pfrc = qBound(0.0f, (pfrc-gbot)/(gtop-gbot), 1.0f);
		  pfrc = m_gbotop*(1-pfrc) + m_gtopop*pfrc;
		}
	      QGradientStops qgs = ogstops;
	      gstops.clear();
	      for(int gi=0; gi<qgs.size(); gi++)
		{
		  float pos = qgs[gi].first;
		  QColor color = qgs[gi].second;
//		  int r = color.red();
//		  int g = color.green();
//		  int b = color.blue();
//		  int a = color.alpha();
//		  gstops << QGradientStop(pos, QColor(r,g,b,a*pfrc));

		  qreal r = color.redF();
		  qreal g = color.greenF();
		  qreal b = color.blueF();
		  qreal a = color.alphaF();
		  QColor newc;
		  newc.setRgbF(r,g,b,a*pfrc);
		  gstops << QGradientStop(pos, newc);
		}
	    }

	  QLinearGradient lg(vRight.x(), vRight.y(),
			     vLeft.x(), vLeft.y());
	  lg.setStops(gstops);
	  
	  QPen pen;
	  pen.setBrush(QBrush(lg));
	  pen.setWidth(2);
	  
	  colorMapPainter.setPen(pen);
	  colorMapPainter.drawLine(vRight, vLeft);

	  ptn++;
	}
    }
}


void
SplineTransferFunction::updateColorMapImageFor16bit()
{
  m_colorMapImage.fill(0);

  int val0 = m_leftNormals[0].x()*65536;
  int val1 = m_rightNormals[0].x()*65536;
  if (val1 < val0)
    {
      int vtmp = val0;
      val0 = val1;
      val1 = vtmp;
    }

  QGradientStops gstops;
  // limit opacity to 252 to avoid overflow
  for(int i=0; i<m_gradientStops.size(); i++)
    {
      float pos = m_gradientStops[i].first;
      QColor color = m_gradientStops[i].second;
      int r = color.red();
      int g = color.green();
      int b = color.blue();
      int a = color.alpha();
      a = qMin(252, a);
      gstops << QGradientStop(pos, QColor(r,g,b,a));
    }
  gstops = StaticFunctions::resampleGradientStops(gstops, 101);
  
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
SplineTransferFunction::append()
{
  m_undo.append(getSpline());
}

void
SplineTransferFunction::applyUndo(bool flag)
{
  if (flag)
    m_undo.undo();
  else
    m_undo.redo();
  
  SplineInformation splineInfo = m_undo.splineInfo();

  m_name = splineInfo.name();
  m_on = splineInfo.on();
  m_points = splineInfo.points();
  m_normalWidths = splineInfo.normalWidths();
  m_normalRotations = splineInfo.normalRotations();
  m_gradientStops = splineInfo.gradientStops();
  splineInfo.gradLimits(m_gbot, m_gtop);
  splineInfo.opMod(m_gbotop, m_gtopop);

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
