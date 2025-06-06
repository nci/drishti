#include "remaphistogramline.h"
#include <QMessageBox>

RemapHistogramLine::RemapHistogramLine()
{
  m_activeTickNumber = -1;
  m_activeTick = -1;  

  m_tickMinKey = 0;
  m_tickMaxKey = 65535;

  m_activeB1 = m_tickMinKey;
  m_activeB2 = m_tickMaxKey;

  // insert two ticks
  m_ticks[0] = 0;
  m_ticks[m_tickMaxKey] = m_tickMaxKey;

  m_ticksOriginal[0] = 0;
  m_ticksOriginal[m_tickMaxKey] = m_tickMaxKey;


  m_start = 0;
  m_width = 10;

  m_keepEndsFixed = false;
  m_keepGrabbing = false;
}

bool RemapHistogramLine::grabsMouse() { return m_keepGrabbing; }

void
RemapHistogramLine::setTickMaxKey(uint tmax)
{
  m_tickMinKey = 0;
  m_tickMaxKey = tmax;

  m_activeB1 = m_tickMinKey;
  m_activeB2 = m_tickMaxKey;

  m_ticks[0] = 0;
  m_ticks[m_tickMaxKey] = m_tickMaxKey;

  m_ticksOriginal[0] = 0;
  m_ticksOriginal[m_tickMaxKey] = m_tickMaxKey;
}

void
RemapHistogramLine::setLine(int x, int w)
{
  m_start = x;
  m_width = w;
}

QPoint RemapHistogramLine::line() { return QPoint(m_start, m_width); }

void RemapHistogramLine::setKeepEndsFixed(bool flag) { m_keepEndsFixed = flag; }

int RemapHistogramLine::activeTickNumber() { return m_activeTickNumber; }

QList<float>
RemapHistogramLine::ticks()
{
  QList<uint> tk = m_ticks.keys();
  
  QList<float> ftk;
  float d = m_tickMaxKey-m_tickMinKey;
  for(uint i=0; i<tk.size(); i++)
    {
      float t = (tk[i]-m_tickMinKey)/d;
      ftk.append(t);
    }
  return ftk;
}

QList<float>
RemapHistogramLine::ticksOriginal()
{
  QList<uint> tk = m_ticksOriginal.keys();
  QList<float> ftk;
  float d = m_tickMaxKey-m_tickMinKey;
  for(uint i=0; i<tk.size(); i++)
    {
      float t = (tk[i]-m_tickMinKey)/d;
      ftk.append(t);
    }
  return ftk;
}

void
RemapHistogramLine::moveTick(int i, float frc)
{
  QList<uint> keys = m_ticks.keys();
  uint tk = m_tickMinKey + frc*(m_tickMaxKey-m_tickMinKey);
  if (i == 0)
    {
      m_ticks.remove(keys[0]);      
      m_ticks[tk] = tk;
    }
  else if (i == 1)
    {
      m_ticks.remove(keys[keys.count()-1]);      
      m_ticks[tk] = tk;
    }
}

void
RemapHistogramLine::addTick(float frc)
{
  float d = m_tickMaxKey-m_tickMinKey;
  uint tk = m_tickMinKey + d*frc;

  if (m_ticksOriginal.contains(tk))
    {
      QMessageBox::information(0, "Error",
			       "A marker already exists at this place");
      return;
    }

  QList<uint> keys = m_ticks.keys();
  if (tk <= keys[0] ||
      tk >= keys[keys.size()-1])
    {
      QMessageBox::information(0, "Error",
			       "Cannot add point beyond the ends");
      return;
    }
  
  
  // add the tick
  m_ticks[tk] = tk;
  m_ticksOriginal[tk] = tk;
  m_activeTick = tk;

  int tkn = -1;
  // now get the previous tick and relative proportions
  for(int k=0; k<keys.size(); k++)
    {
      if (keys[k] < tk)
	tkn = k;
      else
	break;
    }
  int tickNumber = tkn+1;

  emit addTick(tickNumber);
}

void
RemapHistogramLine::mousePress(int xpos, int button, float scale)
{
  m_startDrag = -1;
  m_activeTick = -1;
  m_activeTickNumber = -1;

  {
    int tk;
    float frc;
    
    frc = (xpos-m_start)/(float)m_width;
    tk = m_tickMinKey + frc*(m_tickMaxKey-m_tickMinKey);
    
    int nearest = -1;
    int nd = 100000000;
    QList<uint> keys = m_ticks.keys();
    for(uint i=0; i<m_ticks.size(); i++)
      {
	int v = keys[i];
	int d = qAbs(tk-v);
	if (d < nd)
	  {
	    nd = d;
	    nearest = i;
	  }
      }
    int delta = (10.0f*scale/(float)m_width)*(m_tickMaxKey-m_tickMinKey);

    if (nd<delta)
      m_activeTick = keys[nearest];
  }

  m_activeB1 = m_tickMinKey;
  m_activeB2 = m_tickMaxKey;
  if (m_activeTick > -1)
    {
      QList<uint> keys = m_ticks.keys();

      for(int k=0; k<keys.size(); k++)
	if (keys[k] == m_activeTick)
	  {
	    m_activeTickNumber = k;
	    break;
	  }


      for(int k=0; k<keys.size(); k++)
	{
	  if (m_activeTick > keys[k])
	    m_activeB1 = keys[k];
	}
      for(int k=keys.size()-1; k>=0; k--)
	{
	  if (m_activeTick < keys[k])
	    m_activeB2 = keys[k];
	}
    }
  
  int delta = 11.0*(m_tickMaxKey-m_tickMinKey)/(float)m_width;
  if (m_activeTickNumber > 0)
    m_activeB1 += delta;
  if (m_activeTickNumber < m_ticks.size()-1)
    m_activeB2 -= delta;

  if (m_activeTick < 0)
    {
      QList<uint> tkeys = m_ticks.keys();
      float frc = (xpos-m_start)/(float)m_width;
      uint tk = m_tickMinKey + frc*(m_tickMaxKey-m_tickMinKey);
      if (tk > tkeys[0] && tk < tkeys[1])
	m_startDrag = tk;	  
    }
}
void
RemapHistogramLine::mouseMove(int xpos)
{
  if (m_activeTick < 0)
    {
      QList<uint> tkeys = m_ticks.keys();
      float frc = (xpos-m_start)/(float)m_width;
      uint tk = m_tickMinKey + frc*(m_tickMaxKey-m_tickMinKey);
      if (m_startDrag > 0)
	{
	  uint tmin = tkeys[0];
	  uint tmax = tkeys[1];
	  uint tlen = tmax-tmin;
	  int df = tk-m_startDrag;
	  tmin = qBound(0, (int)tmin + df, (int)m_tickMaxKey);
	  tmax = qBound(0, (int)tmax + df, (int)m_tickMaxKey);
	  m_ticks.clear();
	  m_ticks[tmin] = tmin;
	  m_ticks[tmax] = tmax;
	  m_startDrag = tk;
	  m_keepGrabbing = true;
	}
      return;
    }

  m_keepGrabbing = true;

  if (m_keepEndsFixed &&
      (m_activeTickNumber == 0 ||
       m_activeTickNumber == m_ticks.size()-1))
    {
      QMessageBox::information(0, "Error",
			       "Cannot move these end markers");
      return;
    }

  float frc = (xpos-m_start)/(float)m_width;
  int tk = m_tickMinKey + frc*(m_tickMaxKey-m_tickMinKey);

  // make sure ticks do not go outside the limits
  if (tk < m_tickMinKey || tk > m_tickMaxKey)
      return;
      
  tk = qMax(m_activeB1, qMin((int)tk, m_activeB2));

  // make sure ticks are not on top of eachother
  if (m_ticks.contains(tk))
    return;

  m_ticks.remove(m_activeTick);
  m_ticks[tk] = tk;
  m_activeTick = tk;
  
  emit updateScene(m_activeTickNumber);
}

void
RemapHistogramLine::mouseRelease()
{
  m_keepGrabbing = false;
  if (m_activeTick > -1 ||
      m_startDrag > -1)
    {
      m_activeTickNumber = -1;
      m_activeTick = -1;
      emit updateScene(m_activeTickNumber);
    }

  m_startDrag = -1;
}

void
RemapHistogramLine::setTicks(float tmin, float tmax)
{
  m_ticks.clear();
  m_ticksOriginal.clear();

  m_ticksOriginal[0] = 0;
  m_ticksOriginal[m_tickMaxKey] = m_tickMaxKey;

  uint tk = m_tickMinKey + tmin*(m_tickMaxKey-m_tickMinKey);
  m_ticks[tk] = tk;
  tk = m_tickMinKey + tmax*(m_tickMaxKey-m_tickMinKey);
  m_ticks[tk] = tk;
}
