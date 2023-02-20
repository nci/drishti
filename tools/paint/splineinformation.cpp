#include "splineinformation.h"
#include "staticfunctions.h"
#include <math.h>

void SplineInformation::setName(QString str) {m_name = str; };
void SplineInformation::setOn(QList<bool> flags) { m_on = flags; };
void SplineInformation::setPoints(QPolygonF pts) { m_points = pts; };
void SplineInformation::setNormalWidths(QPolygonF nw) { m_normalWidths = nw; };
void SplineInformation::setNormalRotations(QVector<float> nr) { m_normalRotations = nr; };
void SplineInformation::setGradientStops(QGradientStops gs) { m_gradientStops = gs; };

QString SplineInformation::name() { return m_name; };
QList<bool> SplineInformation::on() { return m_on; };
QPolygonF SplineInformation::points() { return m_points; };
QPolygonF SplineInformation::normalWidths() { return m_normalWidths; };
QVector<float> SplineInformation::normalRotations() { return m_normalRotations; };
QGradientStops SplineInformation::gradientStops() { return m_gradientStops; };
  


SplineInformation::SplineInformation()
{
  m_name.clear();
  m_on.clear();
  m_points.clear();
  m_normalWidths.clear();
  m_normalRotations.clear();
  m_gradientStops.clear(); 
}
SplineInformation&
SplineInformation::operator=(const SplineInformation& splineInfo)
{  
  m_name = splineInfo.m_name;
  m_on = splineInfo.m_on;
  m_points = splineInfo.m_points;
  m_normalWidths = splineInfo.m_normalWidths;
  m_normalRotations = splineInfo.m_normalRotations;
  m_gradientStops = splineInfo.m_gradientStops;
  return *this;
}

void
SplineInformation::load(std::fstream &fin)
{
  m_name.clear();
  m_on.clear();
  m_points.clear();
  m_normalWidths.clear();
  m_normalRotations.clear();
  m_gradientStops.clear(); 

  int len;
  bool done = false;
  char keyword[100];
  while (!done)
    {
      fin.getline(keyword, 100, 0);

      if (strcmp(keyword, "splineinfoend") == 0)
	done = true;
      else if (strcmp(keyword, "name") == 0)
	{
	  fin.read((char*)&len, sizeof(int));
	  char *str = new char[len];
	  fin.read((char*)str, len*sizeof(char));
	  m_name = QString(str);
	  delete [] str;
	}
      else if (strcmp(keyword, "on") == 0)
	{
	  fin.read((char*)&len, sizeof(int));
	  for(int i=0; i<len; i++)
	    {
	      bool t;
	      fin.read((char*)&t, sizeof(bool));
	      m_on.append(t);
	    }
	}
      else if (strcmp(keyword, "points") == 0)
	{
	  fin.read((char*)&len, sizeof(int));
	  float *p = new float[2*len];
	  fin.read((char*)p, 2*len*sizeof(float));
	  for(int i=0; i<len; i++)
	    {
	      QPointF pt(p[2*i], p[2*i+1]);
	      m_points << pt;
	    }
	  delete [] p;
	}
      else if (strcmp(keyword, "normalwidths") == 0)
	{
	  fin.read((char*)&len, sizeof(int));
	  float *p = new float[2*len];
	  fin.read((char*)p, 2*len*sizeof(float));
	  for(int i=0; i<len; i++)
	    {
	      QPointF pt(p[2*i], p[2*i+1]);
	      m_normalWidths << pt;
	    }
	  delete [] p;
	}
      else if (strcmp(keyword, "normalrotations") == 0)
	{
	  fin.read((char*)&len, sizeof(int));
	  float *p = new float[len];
	  fin.read((char*)p, len*sizeof(float));
	  for(int i=0; i<len; i++)
	    {
	      m_normalRotations << p[i];
	    }
	  delete [] p;
	}
      else if (strcmp(keyword, "gradientstops") == 0)
	{
	  fin.read((char*)&len, sizeof(int));
	  float *p = new float[5*len];
	  fin.read((char*)p, 5*len*sizeof(float));
	  for(int i=0; i<len; i++)
	    {
	      QColor col = QColor::fromRgbF(p[5*i+1],
					    p[5*i+2],
					    p[5*i+3],
					    p[5*i+4]);
	      QPair<qreal, QColor> gradStop(p[5*i], col);
	      m_gradientStops.append(gradStop);
	    }
	  delete [] p;
	}
    }
}

void
SplineInformation::save(std::fstream &fout)
{
  char keyword[100];
  int len;
  float *p;

  memset(keyword, 0, 100);
  sprintf(keyword, "splineinfostart");
  fout.write((char*)keyword, strlen(keyword)+1);

  memset(keyword, 0, 100);
  sprintf(keyword, "name");
  fout.write((char*)keyword, strlen(keyword)+1);
  len = m_name.size()+1;
  fout.write((char*)&len, sizeof(int));
  fout.write((char*)m_name.toLatin1().data(), len*sizeof(char));

  memset(keyword, 0, 100);
  sprintf(keyword, "on");
  fout.write((char*)keyword, strlen(keyword)+1);
  len = m_on.count();
  fout.write((char*)&len, sizeof(int));
  for(int i=0; i<len; i++)
    fout.write((char*)&m_on[i], sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "points");
  fout.write((char*)keyword, strlen(keyword)+1);
  len = m_points.count();
  fout.write((char*)&len, sizeof(int));
  p = new float [2*len];
  for(int i=0; i<len; i++)
    {
      p[2*i] = m_points[i].x();
      p[2*i+1] = m_points[i].y();
    }
  fout.write((char*)p, 2*len*sizeof(float));
  delete [] p;


  memset(keyword, 0, 100);
  sprintf(keyword, "normalwidths");
  fout.write((char*)keyword, strlen(keyword)+1);
  len = m_points.count();
  fout.write((char*)&len, sizeof(int));
  p = new float [2*len];
  for(int i=0; i<len; i++)
    {
      p[2*i] = m_normalWidths[i].x();
      p[2*i+1] = m_normalWidths[i].y();
    }
  fout.write((char*)p, 2*len*sizeof(float));
  delete [] p;

  memset(keyword, 0, 100);
  sprintf(keyword, "normalrotations");
  fout.write((char*)keyword, strlen(keyword)+1);
  len = m_points.count();
  fout.write((char*)&len, sizeof(int));
  p = new float [len];
  for(int i=0; i<len; i++)
    {
      p[i] = m_normalRotations[i];
    }
  fout.write((char*)p, len*sizeof(float));
  delete [] p;

  memset(keyword, 0, 100);
  sprintf(keyword, "gradientstops");
  fout.write((char*)keyword, strlen(keyword)+1);
  len = m_gradientStops.count();
  fout.write((char*)&len, sizeof(int));
  p = new float [5*len];
  for(int i=0; i<len; i++)
    {
      float pos = m_gradientStops[i].first;
      QColor color = m_gradientStops[i].second;
      p[5*i] = pos;
      p[5*i+1] = color.redF();
      p[5*i+2] = color.greenF();
      p[5*i+3] = color.blueF();
      p[5*i+4] = color.alphaF();
    }
  fout.write((char*)p, 5*len*sizeof(float));
  delete [] p;


  memset(keyword, 0, 100);
  sprintf(keyword, "splineinfoend");
  fout.write((char*)keyword, strlen(keyword)+1);
}

SplineInformation
SplineInformation::interpolate(SplineInformation& splineInfo1,
			       SplineInformation& splineInfo2,
			       float frc)
{
  SplineInformation splineInfo;
  QPolygonF points, points1, points2;
  QPolygonF normalWidths, normalWidths1, normalWidths2;
  QVector<float> normalRotations, normalRotations1, normalRotations2;
  QGradientStops gradStops, gradStops1, gradStops2;

  int m1, m2;
  
  points1 = splineInfo1.points();
  points2 = splineInfo2.points();
  m1 = points1.size();
  m2 = points2.size();
  for(int i=0; i<qMax(m1,m2); i++)
    {
      float x1, y1, x2, y2;
      if (i < m1)
	{
	  x1 = points1[i].x();
	  y1 = points1[i].y();
	}
      else
	{
	  x1 = points1[m1-1].x();
	  y1 = points1[m1-1].y();
	}
      if (i < m2)
	{
	  x2 = points2[i].x();
	  y2 = points2[i].y();
	}
      else
	{
	  x2 = points2[m2-1].x();
	  y2 = points2[m2-1].y();
	}

      float x, y;
      x = x1 + frc*(x2-x1);
      y = y1 + frc*(y2-y1);
      
      points << QPointF(x,y);
    }

  normalWidths1 = splineInfo1.normalWidths();
  normalWidths2 = splineInfo2.normalWidths();
  m1 = normalWidths1.size();
  m2 = normalWidths2.size();
  for(int i=0; i<qMax(m1,m2); i++)
    {
      float x1, y1, x2, y2;
      if (i < m1)
	{
	  x1 = normalWidths1[i].x();
	  y1 = normalWidths1[i].y();
	}
      else
	{
	  x1 = normalWidths1[m1-1].x();
	  y1 = normalWidths1[m1-1].y();
	}
      if (i < m2)
	{
	  x2 = normalWidths2[i].x();
	  y2 = normalWidths2[i].y();
	}
      else
	{
	  x2 = normalWidths2[m2-1].x();
	  y2 = normalWidths2[m2-1].y();
	}
	
      float x, y;
      x = x1 + frc*(x2-x1);
      y = y1 + frc*(y2-y1);
      
      normalWidths << QPointF(x,y);
    }


  normalRotations1 = splineInfo1.normalRotations();
  normalRotations2 = splineInfo2.normalRotations();
  m1 = normalRotations1.size();
  m2 = normalRotations2.size();
  for(int i=0; i<qMax(m1,m2); i++)
    {      
      float r1, r2;
      if (i<m1)
       r1 = normalRotations1[i];
      else
       r1 = normalRotations1[m1-1];

      if (i<m2)
       r2 = normalRotations2[i];
      else
       r2 = normalRotations2[m2-1];

      float r = r1 + frc*(r2-r1);
      normalRotations << r;
    }


  gradStops = interpolateGradientStops(splineInfo1.gradientStops(),
				       splineInfo2.gradientStops(),
				       frc);


  // keep same "name" and "on"
  splineInfo.setName(splineInfo1.name());
  splineInfo.setOn(splineInfo1.on());
  // interpolate the rest
  splineInfo.setPoints(points);
  splineInfo.setNormalWidths(normalWidths);
  splineInfo.setNormalRotations(normalRotations);
  splineInfo.setGradientStops(gradStops);

  return splineInfo;
}

QList<SplineInformation>
SplineInformation::interpolate(QList<SplineInformation> splineInfo1,
			       QList<SplineInformation> splineInfo2,
			       float frc)
{

  QList<SplineInformation> splineInfo;

  for(int i=0; i<qMin(splineInfo1.size(),
		      splineInfo2.size()); i++)
    {
      splineInfo.append(interpolate(splineInfo1[i],
				    splineInfo2[i],
				    frc));
    }

  int st = splineInfo.size();
  for(int i=st; i<splineInfo1.size(); i++)
    {
      QGradientStops stops = splineInfo1[i].gradientStops();
      QGradientStops newStops;
      for(int j=0; j<stops.size(); j++)
	{
	  float pos = stops[j].first;
	  QColor col = stops[j].second;
	  float r,g,b,a;
	  r = col.red();
	  g = col.green();
	  b = col.blue();
	  a = (1-frc)*col.alpha();
	  col = QColor(r,g,b,a);

	  newStops << QGradientStop(pos, col);
	}

      SplineInformation sinfo;
      sinfo.setName(splineInfo1[i].name());
      sinfo.setOn(splineInfo1[i].on());
      sinfo.setPoints(splineInfo1[i].points());
      sinfo.setNormalWidths(splineInfo1[i].normalWidths());
      sinfo.setNormalRotations(splineInfo1[i].normalRotations());
      sinfo.setGradientStops(newStops);

      splineInfo.append(sinfo);
    }

  st = splineInfo.size();
  for(int i=st; i<splineInfo2.size(); i++)
    {
      QGradientStops stops = splineInfo2[i].gradientStops();
      QGradientStops newStops;
      for(int j=0; j<stops.size(); j++)
	{
	  float pos = stops[j].first;
	  QColor col = stops[j].second;
	  float r,g,b,a;
	  r = col.red();
	  g = col.green();
	  b = col.blue();
	  a = frc*col.alpha();
	  col = QColor(r,g,b,a);

	  newStops << QGradientStop(pos, col);
	}

      SplineInformation sinfo;
      sinfo.setName(splineInfo2[i].name());
      sinfo.setOn(splineInfo2[i].on());
      sinfo.setPoints(splineInfo2[i].points());
      sinfo.setNormalWidths(splineInfo2[i].normalWidths());
      sinfo.setNormalRotations(splineInfo2[i].normalRotations());
      sinfo.setGradientStops(newStops);

      splineInfo.append(sinfo);
    }

  
  return splineInfo;
}

QGradientStops
SplineInformation::interpolateGradientStops(QGradientStops stops1,
					    QGradientStops stops2,
					    float frc)
{
  QVector<float> pos; 

  for(int i=0; i<stops1.size(); i++)
    pos.append(stops1[i].first);

  for(int i=0; i<stops2.size(); i++)
    {
      float pos2 = stops2[i].first;
      bool flag = true;
      for(int j=0; j<stops1.size(); j++)
	{
	  if (fabs(pos[j] - pos2) < 0.0001)
	    {
	      flag = false;
	      break;
	    }
	}
      if (flag)
	pos.append(pos2);
    }

  qSort(pos.begin(), pos.end());

  QGradientStops gradStops1 = StaticFunctions::resampleGradientStops(stops1);
  QGradientStops gradStops2 = StaticFunctions::resampleGradientStops(stops2);

  QGradientStops gradStops;
  int gsize = gradStops1.size()-1;
  for(int i=0; i<pos.size(); i++)
    {
      int idx = pos[i]*gsize;
      QColor color1 = gradStops1[idx].second;
      QColor color2 = gradStops2[idx].second;

      // linear interpolation of colors
      float rb,gb,bb,ab, re,ge,be,ae;
      rb = color1.red();
      gb = color1.green();
      bb = color1.blue();
      ab = color1.alpha();
      re = color2.red();
      ge = color2.green();
      be = color2.blue();
      ae = color2.alpha();
      
      float r,g,b,a;
      r = rb + frc*(re-rb);
      g = gb + frc*(ge-gb);
      b = bb + frc*(be-bb);
      a = ab + frc*(ae-ab);


      QColor color = QColor(r,g,b,a);
      gradStops << QGradientStop(pos[i], color);
    }

  return gradStops;
}
