#include "remaphistogramline.h"

RemapHistogramLine::RemapHistogramLine()
{
  m_activeTickNumber = -1;
  m_activeTick = -1;  

  m_tickMinKey = 0;
  m_tickMaxKey = 1000000;

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
RemapHistogramLine::mousePress(int xpos, int button)
{
  m_activeTick = -1;
  m_activeTickNumber = -1;

  int tk1, tk2;
  float frc;

  frc = ((xpos-10)-m_start)/(float)m_width;
  tk1 = m_tickMinKey + frc*(m_tickMaxKey-m_tickMinKey);

  frc = ((xpos+10)-m_start)/(float)m_width;
  tk2 = m_tickMinKey + frc*(m_tickMaxKey-m_tickMinKey);

  QList<uint> keys = m_ticks.keys();
  for(uint i=0; i<m_ticks.size(); i++)
    {
      int v = keys[i];
      if (v >= tk1 && v <= tk2)
	{
	  m_activeTick = keys[i];
	  break;
	}
    }

  bool deletedTick = false;
  bool addedTick = false;
  int tickNumber = -1;
  if (button == Qt::RightButton)
    {
      if (m_activeTick != -1)
	{ // we need to delete this tick
	  int tkn = -1;
	  QList<uint> keys = m_ticks.keys();
	  for(int k=0; k<keys.size(); k++)
	    if (keys[k] == m_activeTick)
	      {
		tkn = k;
		break;
	      }
	  if (tkn == 0 || tkn == m_ticks.size()-1)
	    {
	      QMessageBox::information(0, "Error",
				       "End markers cannot be removed");
	      return;
	    }
	  m_ticks.remove(m_activeTick);
	  m_ticksOriginal.remove(m_ticksOriginal.keys()[tkn]);
	  m_activeTick = -1;

	  deletedTick = true;
	  tickNumber = tkn;
	}
    }
  else if (m_activeTick == -1)
    { // we need to add one tick here
      QList<uint> keys = m_ticks.keys();

      frc = (xpos-m_start)/(float)m_width;
      uint tk = m_tickMinKey + frc*(m_tickMaxKey-m_tickMinKey);
      int tkn = -1;

      if (m_ticksOriginal.contains(tk))
	{
	  QMessageBox::information(0, "Error",
				   "A marker already exists at this place");
	  return;
	}


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
      

      // now get the previous tick and relative proportions
      for(int k=0; k<keys.size(); k++)
	{
	  if (keys[k] < tk)
	    tkn = k;
	  else
	    break;
	}
      frc = ( ((float)tk-(float)keys[tkn]) /
	      ((float)keys[tkn+1]-(float)keys[tkn]) );
      addedTick = true;
      tickNumber = tkn+1;
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

  if (deletedTick)
    emit removeTick();
  else if (addedTick)
    emit addTick(tickNumber);
}

void
RemapHistogramLine::mouseMove(int xpos)
{
  if (m_activeTick < 0)
    return;

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
  uint tk = m_tickMinKey + frc*(m_tickMaxKey-m_tickMinKey);
  tk = qMax(m_activeB1, qMin((int)tk, m_activeB2));

  m_ticks.remove(m_activeTick);
  m_ticks[tk] = tk;
  m_activeTick = tk;
  
  emit updateScene(m_activeTickNumber);
}

void
RemapHistogramLine::mouseRelease()
{
  m_keepGrabbing = false;

  if (m_activeTick > -1)
    {
      m_activeTickNumber = -1;
      m_activeTick = -1;
      emit updateScene(m_activeTickNumber);
    }
}
