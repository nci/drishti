#include "keyframeeditor.h"
#include "global.h"
#include "propertyeditor.h"

#include <algorithm>
using namespace std;

// ---------- class for sorting -----------
class tag
{
 public :
  int id;
  float loc;
  bool operator<(const tag& a) const
  {
    return (loc < a.loc);
  };
  tag& operator=(const tag& a)
  {
    id = a.id;
    loc = a.loc;
    return *this;
  };
};
//-----------------------------------------

int
KeyFrameEditor::startFrame()
{
  if (m_fno.count() > 0)
    return m_fno[0];
  else
    return 1;
}
int
KeyFrameEditor::endFrame()
{
  if (m_fno.count() > 0)
    return m_fno[m_fno.count()-1];
  else
    return 1;
}

int KeyFrameEditor::currentFrame() { return m_currFrame; }

QSize
KeyFrameEditor::sizeHint() const
{
  return QSize(size().width(), m_editorHeight);
}

KeyFrameEditor::KeyFrameEditor(QWidget *parent):
  QWidget(parent)
{
  m_parent = parent;

  m_copyFno = -1;
  m_selected = -1;

  m_set = new QPushButton("Set Keyframe");
  m_remove = new QPushButton("Remove KeyFrame");

  m_plus = new QPushButton(QPixmap(":/images/zoomin.png"), "Z");
  m_minus = new QPushButton(QPixmap(":/images/zoomout.png"), "z");
  m_play = new QPushButton(QPixmap(":/images/animation-start.png"), ">");
  m_reset = new QPushButton(QPixmap(":/images/animation-reset.png"), "|<");

  m_plus->setText("");
  m_minus->setText("");
  m_play->setText("");
  m_reset->setText("");

  m_plus->setAutoRepeat(true);
  m_minus->setAutoRepeat(true);

  m_set->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  m_remove->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_plus->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
   m_minus->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

  m_set->setMaximumSize(100, 30);
  m_remove->setMaximumSize(150, 30);
  m_plus->setMaximumSize(30, 30);
  m_minus->setMaximumSize(30, 30);

  m_play->setMaximumSize(30, 30);
  m_reset->setMaximumSize(30, 30);

  m_set->setMinimumSize(120, 30);
  m_remove->setMinimumSize(150, 30);
  m_plus->setMinimumSize(30, 30);
  m_minus->setMinimumSize(30, 30);

  m_play->setMinimumSize(30, 30);
  m_reset->setMinimumSize(30, 30);


  QHBoxLayout *hbox = new QHBoxLayout();
  hbox->addWidget(m_set, 0, Qt::AlignTop);
  hbox->addWidget(m_remove, 0, Qt::AlignTop);
  hbox->addStretch();
  hbox->addWidget(m_plus, 0, Qt::AlignTop);
  hbox->addWidget(m_minus, 0, Qt::AlignTop);

  QVBoxLayout *vbox = new QVBoxLayout();
  vbox->setSpacing(1);
  vbox->addWidget(m_play);
  vbox->addWidget(m_reset);
  vbox->addStretch();
  
  QVBoxLayout *mainhbox = new QVBoxLayout();
  mainhbox->addLayout(hbox);
  mainhbox->addLayout(vbox);
  setLayout(mainhbox);

  m_hiresMode = false;
  m_playFrames = false;
  m_currFrame = 1;

  m_lineHeight = 70;
  m_tickStep = 50;
  m_tickHeight = 10;

  m_imgSpacer = 35;
  m_imgSize = 100;

  m_minFrame = 1;
  m_frameStep = 10;

  m_p0 = QPoint(60, m_lineHeight);
  m_p1 = QPoint(100, m_lineHeight);

  calcMaxFrame();

  m_editorHeight = m_lineHeight + m_tickHeight +
                   m_imgSpacer + m_imgSize + 10;

  clear();

  connect(m_plus, SIGNAL(pressed()),
	  this, SLOT(decreaseFrameStep()));

  connect(m_minus, SIGNAL(pressed()),
	  this, SLOT(increaseFrameStep()));

  connect(m_set, SIGNAL(pressed()),
	  this, SLOT(setKeyFrame()));

  connect(m_remove, SIGNAL(pressed()),
	  this, SLOT(removeKeyFrame()));

  connect(m_play, SIGNAL(pressed()),
	  this, SLOT(playPressed()));

  connect(this, SIGNAL(startPlay()),
	  this, SLOT(playKeyFrames()));

  connect(m_reset, SIGNAL(pressed()),
	  this, SLOT(resetCurrentFrame()));


  setMinimumSize(400, 150);
  setMaximumSize(5000, m_editorHeight);
  setBaseSize(400, m_editorHeight);
}

void
KeyFrameEditor::setHiresMode(bool flag)
{
  m_hiresMode = flag;
  setPlayFrames(false);      
}

void
KeyFrameEditor::resetCurrentFrame()
{
  if (m_fno.count() > 0)
    m_currFrame = m_fno[0];
  else
    m_currFrame = 1;

  m_minFrame = qMax(1, m_currFrame);
  calcMaxFrame();

  setPlayFrames(false);
}

void
KeyFrameEditor::playPressed()
{
  if (!m_hiresMode)
    {
      emit showMessage("Play available only in Hires Mode. Press F2 to switch to Hires mode", true);      
      return;
    }


  if (m_playFrames == false)
    setPlayFrames(true);
  else
    setPlayFrames(false);
}

void
KeyFrameEditor::setPlayFrames(bool flag)
{
  m_playFrames = flag;

  if (m_playFrames == false)
    m_play->setIcon(QPixmap(":/images/animation-start.png"));
  else
    m_play->setIcon(QPixmap(":/images/animation-pause.png"));

  update();

  if (m_playFrames)
    emit startPlay();
  else
    emit endPlay();

  qApp->processEvents();
}

void
KeyFrameEditor::playKeyFrames(int start, int end, int step)
{
  // ---- set play on ----
  m_play->setIcon(QPixmap(":/images/animation-pause.png"));
  m_playFrames = true;
  // ---------------------

  if (m_fno.count() <= 1)
    {
      setPlayFrames(false);
      emit showMessage("Need more than one key frame for playing", true);
      qApp->processEvents();
      return;
    }

  if (start > m_fno[m_fno.count()-1])
    {
      setPlayFrames(false);
      emit showMessage(QString("Current Frame (%1) greater than the last key frame (%2)").\
		       arg(start).arg(m_fno[m_fno.count()-1]), true);
      qApp->processEvents();
      return;
    }


  int startFrame = start;
  int endFrame = end;
  for (int i=startFrame; i<=endFrame; i+=step)
    {
      if (!m_playFrames)
	return;

      //----------------------------
      // update current frame and display
      m_currFrame = i;
      if (m_currFrame > m_maxFrame)
	{
	  m_minFrame = qMax(1, m_currFrame-1);
	  calcMaxFrame();
	}
      update();
      //----------------------------

      emit playFrameNumber(m_currFrame);
      qApp->processEvents();
    }

  setPlayFrames(false);  
  emit showMessage("Done", false);
  qApp->processEvents();

  if (Global::batchMode())
    qApp->quit();
}


void
KeyFrameEditor::playKeyFrames()
{
  if (!m_playFrames)
    return;

  if (m_fno.count() <= 1)
    {
      setPlayFrames(false);
      emit showMessage("Need more than one Key Frames for playing", true);
      qApp->processEvents();
      return;
    }

  if (m_currFrame > m_fno[m_fno.count()-1])
    {
      setPlayFrames(false);
      emit showMessage(QString("Current Frame (%1) greater than the last Key Frame (%2)"). \
		       arg(m_currFrame).arg(m_fno[m_fno.count()-1]), true);
      qApp->processEvents();
      return;
    }

  if (m_currFrame < m_fno[0]) m_currFrame = m_fno[0];

  int startFrame, endFrame;
  startFrame = m_currFrame;
  endFrame = m_fno[m_fno.count()-1];
  if (m_selectRegion.valid &&
      m_selectRegion.frame1 > m_selectRegion.frame0)
    {
      startFrame = m_selectRegion.frame0;
      endFrame = m_selectRegion.frame1;

      if (startFrame < m_fno[0])
	startFrame = m_fno[0];

      if (endFrame > m_fno[m_fno.count()-1])
	endFrame = m_fno[m_fno.count()-1];
    }

  playKeyFrames(startFrame, endFrame, 1);

//  playKeyFrames(m_currFrame,           // -- start Frame
//		m_fno[m_fno.count()-1],// -- end Frame
//		1);                    // -- step Frame
}

void
KeyFrameEditor::clear()
{
  m_copyFno = -1;

  m_playFrames = false;
  m_draggingCurrFrame = false;
  m_reordered = false;
  m_selectRegion.valid = false;

  m_minFrame = 1;
  m_frameStep = 10;

  calcMaxFrame();

  m_fno.clear();
  m_fRect.clear();
  m_fImage.clear();

  update();
}

void
KeyFrameEditor::calcRect()
{
  for(int i=0; i<m_fno.count(); i++)
    {
      int fno = m_fno[i];
      if (fno >= m_minFrame || fno <= m_maxFrame)
	{
	  int tick = m_p0.x() + (fno-m_minFrame)*m_tickStep/(m_frameStep);
	  m_fRect[i] = QRect(tick-m_imgSize/2,
			     m_lineHeight+m_tickHeight+m_imgSpacer,
			     m_imgSize, m_imgSize);
	}
      else
	m_fRect[i] = QRect(-10,-10,1,1);
    }
}

void
KeyFrameEditor::calcMaxFrame()
{
  float frc = (float)(m_p1.x()-m_p0.x())/(float)m_tickStep;
  m_maxFrame = m_minFrame + m_frameStep*frc;
  calcRect();
}

void
KeyFrameEditor::drawSelectRegion(QPainter *p)
{
  if (m_selectRegion.valid == false)
    return;

  if (m_selectRegion.frame1 < m_minFrame ||
      m_selectRegion.frame0 > m_maxFrame)
    return; // selectRegion is out of range

  int f0, f1, tick0, tick1;

  f0 = m_selectRegion.frame0;
  f1 = m_selectRegion.frame1;

  if (m_selectRegion.frame0 < m_minFrame)
    f0 = m_minFrame;

  if (m_selectRegion.frame1 > m_maxFrame)
    f1 = m_maxFrame;

  tick0 = m_p0.x() + (f0-m_minFrame)*m_tickStep/(m_frameStep);
  tick1 = m_p0.x() + (f1-m_minFrame)*m_tickStep/(m_frameStep);


  p->setBrush(QColor(250, 150, 100));
  p->setPen(Qt::transparent);
  p->drawRect(tick0,
	      m_lineHeight-m_tickHeight,
	      tick1-tick0,
	      m_tickHeight);
}

void
KeyFrameEditor::drawVisibleRange(QPainter *p)
{
  int bg = 50;
  int sz = size().width()-100;

  p->setBrush(Qt::transparent);

  p->setPen(QPen(QColor(50, 50, 50), 7,
		 Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  p->drawLine(QPoint(5,5), QPoint(bg-10, 5));
  p->drawLine(QPoint(bg+sz+10,5), QPoint(bg+sz+45, 5));

  p->setPen(QPen(QColor(150, 150, 150), 7,
		 Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  p->drawLine(QPoint(bg,5), QPoint(bg+sz, 5));

  if (m_fno.count() < 2)
    return;

  float nframes = (m_fno[m_fno.count()-1]-m_fno[0]);

  // --- draw for selection range ----
  if (m_selectRegion.valid)
    {
      int f0 = m_selectRegion.frame0;
      int f1 = m_selectRegion.frame1;

      f0 = sz*(f0-m_fno[0])/nframes;
      f1 = sz*(f1-m_fno[0])/nframes;
  
      if (f0 < 0) f0 = -bg+10;
      if (f0 > sz) f0 = sz+10;
      f0 += bg;
      
      if (f1 < 0) f1 = -bg+40;
      if (f1 > sz) f1 = sz+40;
      f1 += bg;
      
      p->setPen(QPen(QColor(250, 150, 100), 5,
		     Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
      p->drawLine(QPoint(f0,5), QPoint(f1, 5));
    }

  for(int i=0; i<m_fno.count(); i++)
    {
      int v0 = sz*(m_fno[i]-m_fno[0])/nframes;
      if (v0 < 0) v0 = -bg+10;
      if (v0 > sz) v0 = sz+10;
      v0 += bg;

      p->setPen(QPen(Qt::black, 7,
		     Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
      p->drawPoint(v0,5);
      p->setPen(QPen(Qt::lightGray, 5,
		     Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
      p->drawPoint(v0,5);
    }

  int bg0 = sz*(m_minFrame-m_fno[0])/nframes;
  int bg1 = sz*(m_maxFrame-m_fno[0])/nframes;
  
  if (bg0 < 0) bg0 = -bg+10;
  if (bg0 > sz) bg0 = sz+10;
  bg0 += bg;

  if (bg1 < 0) bg1 = -bg+40;
  if (bg1 > sz) bg1 = sz+40;
  bg1 += bg;
  
  p->setPen(QPen(QColor(255, 255, 255), 2,
		 Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  p->drawLine(QPoint(bg0,5), QPoint(bg1, 5));
}

void
KeyFrameEditor::drawTicks(QPainter *p)
{
  if (m_playFrames || m_draggingCurrFrame)
    {
      p->setBrush(QColor(70, 50, 20, 50));
      p->setPen(Qt::transparent);
      p->drawRect(m_p0.x()-5,
		  m_lineHeight-m_tickHeight-15,
		  m_p1.x()-m_p0.x()+10,
		  2*m_tickHeight+25);
    }

  p->setBrush(Qt::transparent);
  p->setPen(Qt::darkGray);
  p->drawLine(m_p0, m_p1);

  int nticks = (m_p1.x() - m_p0.x())/m_tickStep;
  if (nticks*m_tickStep < m_p1.x() - m_p0.x())
    nticks ++;
  int prevtick = 0;
  for(int t=0; t<=nticks; t++)
    {
      int frameNumber = m_minFrame + t*m_frameStep;
      int tick = m_p0.x() + t*m_tickStep;
      int sticks = 10;
      if (t == nticks)
	{
	  float frc = (float)(frameNumber-m_maxFrame)/(float)m_frameStep;
	  sticks *= (1-frc);
	  frameNumber = m_maxFrame;
	  tick = m_p1.x();	  
	}

      p->drawLine(tick, m_lineHeight-m_tickHeight,
		  tick, m_lineHeight+m_tickHeight);

      int tk = tick;
      if (frameNumber < 10)
	tk = tick-2;
      else if (frameNumber < 100)
	tk = tick-7;
      else if (frameNumber < 1000)
	tk = tick-10;
      else
	tk = tick-15;

      p->drawText(tk, m_lineHeight-m_tickHeight-2,
		 QString("%1").arg(frameNumber));


      // draw inbetween smaller ticks
      if (m_frameStep > 1 && t > 0 && sticks > 0)
	{
	  int tkstep = (tick-prevtick)/sticks;
	  for(int s=1; s<sticks; s++)
	    {
	      int stk = prevtick + s*tkstep;
	      p->drawLine(stk, m_lineHeight,
			 stk, m_lineHeight-m_tickHeight/2);
	    }
	}

      prevtick = tick;
    }  
}

void
KeyFrameEditor::drawkeyframe(QPainter *p,
			     QColor penColor, QColor brushColor,
			      int fno, QRect rect, QImage img)
{
  if (fno < m_minFrame || fno > m_maxFrame)
    return;

  int tick = m_p0.x() + (fno-m_minFrame)*m_tickStep/(m_frameStep);

  p->setPen(penColor);
  p->drawLine(tick, m_lineHeight,
	      tick, m_lineHeight+m_tickHeight+m_imgSpacer);

  p->drawText(tick+2, rect.y()-2,
	      QString("%1").arg(fno));
  
  QRect prect = rect.adjusted(2, 2, -2, -2);
  p->drawImage(prect, img, QRect(0, 0, 100, 100));

  p->setBrush(Qt::transparent);
  p->setPen(QPen(penColor, 7,
		 Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  p->drawRoundRect(rect);
  p->setPen(QPen(brushColor, 3,
		 Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  p->drawRoundRect(rect);
}

void
KeyFrameEditor::drawKeyFrames(QPainter *p)
{
  p->setFont(QFont("Helvetica", 12));

  for(int i=0; i<m_fno.count(); i++)
    {
      QRect rect = m_fRect[i];
      QColor penColor, brushColor;

      bool flag = true;
      if (m_selectRegion.valid &&
	  m_selectRegion.keyframe0 > -1)
	{
	  if (i >= m_selectRegion.keyframe0 &&
	      i <= m_selectRegion.keyframe1)
	    {
	      brushColor = QColor(250, 200, 150);
	      penColor = QColor(100, 100, 100);

	      if (m_selected == i)
		{
		  brushColor = QColor(250, 150, 100);
		  penColor = QColor(50, 30, 10);
		}
	      flag = false;
	    }
	}

      if (flag)
	{
	  if (m_selected == i)
	    {
	      brushColor = QColor(250, 150, 100);
	      penColor = QColor(50, 30, 10);
	    }
	  else
	    {
	      brushColor = QColor(170, 170, 170);
	      penColor = QColor(100, 100, 100);
	    }
	}
      
      drawkeyframe(p,
		   penColor, brushColor,
		   m_fno[i], rect, m_fImage[i]);      
    }
}

void
KeyFrameEditor::drawCurrentFrame(QPainter *p)
{
  if (m_currFrame < m_minFrame ||
      m_currFrame > m_maxFrame)
    return;

  p->setFont(QFont("Helvetica", 10));
  p->setPen(QPen(QColor(30, 100, 60),2));
  
  int tick = m_p0.x() + (m_currFrame-m_minFrame)*m_tickStep/(m_frameStep);
  
  p->drawLine(tick, m_lineHeight-m_tickHeight-5,
	      tick, m_lineHeight+m_tickHeight+5);
  
  int tk = tick-10;
  if (m_currFrame < 10)
    tk = tick-15;
  else if (m_currFrame < 100)
    tk = tick-20;
  else if (m_currFrame < 1000)
    tk = tick-25;
  else
    tk = tick-30;
  p->drawText(tk, m_lineHeight+m_tickHeight+5,
	      QString("%1").arg(m_currFrame));

}

void
KeyFrameEditor::paintEvent(QPaintEvent *event)
{
  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing);

  drawSelectRegion(&p);

  drawVisibleRange(&p);

  drawTicks(&p);

  drawCurrentFrame(&p);

  drawKeyFrames(&p);

  if (Global::morphTF())
    {
      p.setFont(QFont("Helvetica", 8));
      p.setPen(QPen(QColor(130, 100, 60),2));
      p.drawText(230, 30, "Transfer function morphing enabled");
    }
}

void
KeyFrameEditor::resizeEvent(QResizeEvent *event)
{
  m_p1 = QPoint(size().width()-60, m_lineHeight);
  calcMaxFrame();
}

void
KeyFrameEditor::increaseFrameStep()
{
  if (m_frameStep == 1)
    {
      m_frameStep = 10;
      m_tickStep = 50;
    }
  else if (m_frameStep == 10)
    {
      m_frameStep = 100;
      m_tickStep = 100;
    }
  else
    {
      m_frameStep += 100;
      m_tickStep = 200;
    }

  m_plus->setEnabled(true);

  if (m_frameStep >= 10000)
    {
      m_frameStep = 10000;
      m_minus->setEnabled(false);
    }

  calcMaxFrame();
  update();
}

void
KeyFrameEditor::decreaseFrameStep()
{
  if (m_frameStep > 100)
    m_frameStep -= 100;
  else if (m_frameStep > 10)
    m_frameStep = 10;
  else
    m_frameStep = 1;

  if (m_frameStep > 100)
    m_tickStep = 200;
  else if (m_frameStep > 10)
    m_tickStep = 100;
  else 
    m_tickStep = 50;

  m_minus->setEnabled(true);

  if (m_frameStep <= 1)
    {
      m_frameStep = 1;
      m_tickStep = 20;
      m_plus->setEnabled(false);
    }

  calcMaxFrame();
  update();
}

void
KeyFrameEditor::moveGrid(int shift)
{
  m_minFrame = m_pressedMinFrame + m_frameStep*((float)shift/(float)m_tickStep);
  m_minFrame = qMax(1, m_minFrame);
  calcMaxFrame();
  update();
}


int
KeyFrameEditor::frameUnderPoint(QPoint pos)
{
  int frame;
  if (pos.x() <= m_p0.x())
    frame = m_minFrame;
  else if (pos.x() >= m_p1.x())
    frame = m_maxFrame;
  else
    {
      float frc = (float)(pos.x()-m_p0.x())/(float)(m_p1.x()-m_p0.x());
      frame = m_minFrame + frc * (m_maxFrame-m_minFrame);
    }

  return frame;
}

void
KeyFrameEditor::moveCurrentFrame(QPoint pos)
{
  m_currFrame = frameUnderPoint(pos);

  update();

  emit playFrameNumber(m_currFrame);
  qApp->processEvents();
}

void
KeyFrameEditor::preShift()
{
  m_ratioBefore.clear();
  m_ratioAfter.clear();

  int startKF = 0;
  int endKF = m_fno.count()-1;
  
  if (m_selectRegion.valid &&
      m_selectRegion.frame1 > m_selectRegion.frame0)
    {
      startKF = m_selectRegion.keyframe0;
      endKF = m_selectRegion.keyframe1;
    }

  if (m_selected < startKF ||
      m_selected > endKF)
    return; 

  QString str;

  int firstFrame = m_fno[startKF];
  for(int fi=startKF; fi<m_selected; fi++)
    {
      float frc = (float)(m_fno[fi]-firstFrame)/
	(float)(m_currFrame - firstFrame);
      m_ratioBefore.append(frc);
    }

  int lastFrame = m_fno[endKF];
  for(int fi=m_selected+1; fi<=endKF; fi++)
    {
      float frc = (float)(m_fno[fi]-lastFrame)/
	(float)(m_currFrame - lastFrame);
      m_ratioAfter.append(frc);
    }
}

void
KeyFrameEditor::applyShift()
{
  int startKF = 0;
  int endKF = m_fno.count()-1;
  
  if (m_selectRegion.valid &&
      m_selectRegion.frame1 > m_selectRegion.frame0)
    {
      startKF = m_selectRegion.keyframe0;
      endKF = m_selectRegion.keyframe1;
    }

  if (m_selected < startKF ||
      m_selected > endKF)
    return; 

  
  int firstFrame = m_fno[startKF];
  int len = qMax(0, m_fno[m_selected] - m_fno[startKF]); 
  for(int fi=startKF; fi<m_selected; fi++)
    m_fno[fi] = firstFrame + m_ratioBefore[fi-startKF]*len;
  for(int fi=startKF; fi<m_selected; fi++)
    {
      if (m_fno[fi] >= m_fno[fi+1])
	m_fno[fi+1] = m_fno[fi]+1;
    }
  for(int fi=m_selected; fi>0; fi--)
    {
      if (m_fno[fi] <= m_fno[fi-1])
	m_fno[fi-1] = m_fno[fi] - 1;
    }
  m_fno[0] = qMax(1, m_fno[0]);

  int lastFrame = m_fno[endKF];
  len = qMin(0, m_fno[m_selected] - lastFrame);
  for(int fi=m_selected+1; fi<=endKF; fi++)
    m_fno[fi] = lastFrame + m_ratioAfter[fi-m_selected-1]*len; 
  for(int fi=endKF; fi>m_selected; fi--)
    {
      if (m_fno[fi] <= m_fno[fi-1])
	m_fno[fi-1] = m_fno[fi]-1;
    }
  for(int fi=1; fi<m_fno.count(); fi++)
    {
      if (m_fno[fi] <= m_fno[fi-1])
	m_fno[fi] = m_fno[fi-1] + 1;
    }

  emit setKeyFrameNumbers(m_fno);
}

void
KeyFrameEditor::applyMove(int pfrm, int nfrm)
{
  int startKF = 0;
  int endKF = m_fno.count()-1;
  
  if (m_selectRegion.valid &&
      m_selected >= m_selectRegion.keyframe0 &&
      m_selected <= m_selectRegion.keyframe1)
    {
      startKF = m_selectRegion.keyframe0;
      endKF = m_selectRegion.keyframe1;
    }

  if (m_selected < startKF ||
      m_selected > endKF)
    return; 

  
  int diff = nfrm - pfrm;
  bool ok = true;

  int frame0 = m_fno[startKF];
  int frame1 = m_fno[endKF];
  int zeroframe = 0;
  if (startKF > 0) zeroframe = m_fno[startKF-1];

  if (frame0 + diff <= zeroframe)
    {
      diff = zeroframe - frame0;
      diff++;
    }
  if (endKF < m_fno.count()-1)
    {
      if (frame1 + diff >= m_fno[endKF+1])
	{
	  diff = m_fno[endKF+1] - frame1;
	  diff--;
	}
    }

  for(int fi=startKF; fi<=endKF; fi++)
    m_fno[fi] += diff;
  
  emit setKeyFrameNumbers(m_fno);
}

void
KeyFrameEditor::updateSelectRegion()
{
  if (m_selectRegion.valid == false)
    return;

  if (m_fno.count() == 0)
    return;

  int sf, ef;
  sf = m_selectRegion.frame0;
  ef = m_selectRegion.frame1;
  if (sf > ef)
    {
      ef = m_selectRegion.frame0;
      sf = m_selectRegion.frame1;
    }

  m_selectRegion.keyframe0 = -1;
  m_selectRegion.keyframe1 = -1;

  for(int i=0; i<m_fno.count(); i++)
    {
      if (m_fno[i] >= sf)
	{
	  m_selectRegion.keyframe0 = i;
	  break;
	}
    }
  for(int i=m_fno.count()-1; i>=0; i--)
    {
      if (m_fno[i] <= ef)
	{
	  m_selectRegion.keyframe1 = i;
	  break;
	}
    }
}

void KeyFrameEditor::enterEvent(QEvent *e) { setFocus(); grabKeyboard(); }
void KeyFrameEditor::leaveEvent(QEvent *e) { clearFocus(); releaseKeyboard(); }

void
KeyFrameEditor::keyPressEvent(QKeyEvent *event)
{
  if (event->key() == Qt::Key_Space)
    {
      if (m_selected > -1)
	emit editFrameInterpolation(m_selected);
    }  
  else if (event->key() == Qt::Key_Left)
    {
      m_currFrame = qMax(m_currFrame-1, 1);
      m_selected = -1;
      for(int f=0; f<m_fno.count(); f++)
	{
	  if (m_currFrame == m_fno[f])
	    {
	      m_selected = f;
	      break;
	    }
	}
      update();
      emit playFrameNumber(m_currFrame);
      qApp->processEvents();
    }
  else if (event->key() == Qt::Key_Right)
    {
      m_currFrame = m_currFrame+1;
      m_selected = -1;
      for(int f=0; f<m_fno.count(); f++)
	{
	  if (m_currFrame == m_fno[f])
	    {
	      m_selected = f;
	      break;
	    }
	}
      update();
      emit playFrameNumber(m_currFrame);
      qApp->processEvents();
    }
  else if (event->key() == Qt::Key_C &&
	   (event->modifiers() & Qt::ControlModifier ||
	    event->modifiers() & Qt::MetaModifier) )
    {
      if (m_selected > -1)
	{
	  m_copyFno = m_fno[m_selected];
	  m_copyImage = m_fImage[m_selected];

	  emit copyFrame(m_selected);
	  emit showMessage(QString("frame at %1 copied").arg(m_copyFno), false);
	}
      else
	{
	  emit showMessage("No frame selected for copying", true);
	  return;
	}
    }
  else if (event->key() == Qt::Key_H &&
	   (event->modifiers() & Qt::ControlModifier ||
	    event->modifiers() & Qt::MetaModifier) )
    showHelp();
  else if (event->key() == Qt::Key_V &&
	   (event->modifiers() & Qt::ControlModifier ||
	    event->modifiers() & Qt::MetaModifier) )
	   {
      if (m_copyFno < 0)
	{
	  emit showMessage("No frame selected for pasting", true);
	  return;
	}

      if (m_selectRegion.valid)
	{
	  int startKF = m_selectRegion.keyframe0;
	  int endKF = m_selectRegion.keyframe1;

	  emit pasteFrameOnTop(startKF, endKF);
	}
      else
	{
	  if (m_selected > -1)
	    {
	      if (m_selected < m_fno.count())
		emit pasteFrameOnTop(m_selected);
	      else
		QMessageBox::information(0, "",
		 QString("Selected keyframe %1 does not exist").arg(m_selected));
	    }
	  else
	    {
	      int onTop = -1;
	      for(int f=0; f<m_fno.count(); f++)
		{
		  if (m_currFrame == m_fno[f])
		    {
		      onTop = f;
		      break;
		    }
		}

	      if (onTop > -1)
		emit pasteFrameOnTop(onTop);
	      else
		{
		  emit pasteFrame(m_currFrame);
	      
		  m_fno.append(m_currFrame);
		  m_fRect.append(QRect(-10,-10,1,1));
		  m_fImage.append(m_copyImage);
	      
		  calcRect();
		  reorder();
		  update();
		}
	    }
	}
    }
}


void
KeyFrameEditor::mouseMoveEvent(QMouseEvent *event)
{
  QPoint clickPos = event->pos();
  
  if (m_draggingCurrFrame)
    moveCurrentFrame(clickPos);
  else if (m_selected < 0)
    {
      if (m_modifiers & Qt::ShiftModifier)
	{
	  if (m_selectRegion.valid)
	    {
	      m_selectRegion.frame1 = frameUnderPoint(clickPos);	      
	      updateSelectRegion();
	      update();
	    }
	}
      else
	moveGrid(m_prevX-clickPos.x());
    }
  else
    { // dragging keyframe
      int pfrm = m_fno[m_selected];

      if (event->buttons() & Qt::MidButton)
	{
	  int nfrm;

	  if (clickPos.x() <= m_p0.x())
	    nfrm = m_minFrame;
	  else if (clickPos.x() >= m_p1.x())
	    nfrm = m_maxFrame;
	  else
	    {
	      float frc = (float)(clickPos.x()-m_p0.x())/(float)(m_p1.x()-m_p0.x());
	      nfrm = m_minFrame + frc * (m_maxFrame-m_minFrame);
	    }
	  
	  applyMove(pfrm, nfrm);	  
	  calcRect();
	}
      else
	{
	  if (clickPos.x() <= m_p0.x())
	    m_fno[m_selected] = m_minFrame;
	  else if (clickPos.x() >= m_p1.x())
	    m_fno[m_selected] = m_maxFrame;
	  else
	    {
	      float frc = (float)(clickPos.x()-m_p0.x())/(float)(m_p1.x()-m_p0.x());
	      m_fno[m_selected] = m_minFrame + frc * (m_maxFrame-m_minFrame);
	    }
	  
	  if (m_selectRegion.valid &&
	      m_selected >= m_selectRegion.keyframe0 &&
	      m_selected <= m_selectRegion.keyframe1)
	    {
	      applyShift();	  
	      calcRect();
	    }
	  else
	    {
	      if (m_selectRegion.valid &&
		  m_selectRegion.frame1 > m_selectRegion.frame0)
		{
		  if (pfrm < m_selectRegion.frame0 &&
		      m_fno[m_selected] >= m_selectRegion.frame0)
		    {
		      m_fno[m_selected] = m_selectRegion.frame0 - 1;
		      emit showMessage("Cannot move keyframe into select region. Deselect region to move keyframe into that space. Right click to deselect region", true);
		    }
		  else if (pfrm > m_selectRegion.frame1 &&
			   m_fno[m_selected] <= m_selectRegion.frame1)
		    {
		      m_fno[m_selected] = m_selectRegion.frame1 + 1;
		      emit showMessage("Cannot move keyframe into select region. Deselect region to move keyframe into that space. Right click to deselect region", true);
		    }
		}
	      
	      emit setKeyFrameNumber(m_selected, m_fno[m_selected]);     
	      
	      calcRect();
	      reorder();
	    }
	}

      m_currFrame = m_fno[m_selected];

      update();
    }
}

void
KeyFrameEditor::mouseReleaseEvent(QMouseEvent *event)
{

  m_modifiers = 0;
  m_draggingCurrFrame = false;

  if (m_reordered)
    {
      m_reordered = false;
      emit updateGL();
    }

  if (m_selectRegion.valid &&
      m_selectRegion.frame0 > m_selectRegion.frame1)
    {
      int tframe = m_selectRegion.frame0;
      m_selectRegion.frame0 = m_selectRegion.frame1;
      m_selectRegion.frame1 = tframe;
    }

  updateSelectRegion();
  update();

  emit checkKeyFrameNumbers();
}

void
KeyFrameEditor::mousePressEvent(QMouseEvent *event)
{
  QPoint clickPos = event->pos();

  m_pressed = event->button();
  m_pressedMinFrame = m_minFrame;
  m_draggingCurrFrame = false;
  m_modifiers = event->modifiers();

  if (m_pressed == Qt::RightButton)
    m_selectRegion.valid = false;

  m_selected = -1;
  for(int i=m_fno.count()-1; i>=0; i--)
    {
      if (m_fRect[i].contains(clickPos))
	{
	  m_selected = i;
	  m_currFrame = m_fno[m_selected];

	  if (m_selectRegion.valid)
	    preShift();
	  
	  // pause if currently playing frames
	  if (m_playFrames)
	    setPlayFrames(false);

	  update();
	  emit playFrameNumber(m_currFrame);
	  return;
	}
    }

    if (m_pressed == Qt::LeftButton &&
	m_modifiers & Qt::ShiftModifier)
      {
	m_selectRegion.frame0 = frameUnderPoint(clickPos);
	m_selectRegion.frame1 = m_selectRegion.frame0;
	m_selectRegion.valid = true;
      }
    else if (clickPos.y() >= m_lineHeight - m_tickHeight - 5 &&
	     clickPos.y() <= m_lineHeight + m_tickHeight + 5)
      {
	m_draggingCurrFrame = true;
	if (m_playFrames)
	  setPlayFrames(false);
	moveCurrentFrame(clickPos);
      }

  m_prevX = clickPos.x();

  update();
}

void
KeyFrameEditor::setImage(int fno, QImage img)
{
  if (fno >= m_fImage.size())
    {
      emit showMessage(QString("Cannot setImage : %1 %2").	\
		       arg(fno).arg(m_fImage.size()), true);
      qApp->processEvents();
      return;
    }

  m_fImage[fno] = img;

  reorder();
  updateSelectRegion();
  update();  
}

void
KeyFrameEditor::addKeyFrameNumbers(QList<int> fnos)
{
  for(int i=0; i<fnos.count(); i++)
    {
      m_fno.append(fnos[i]);
      
      m_fRect.append(QRect(-10,-10,1,1));
      
      QImage img(100,100, QImage::Format_RGB32);
      m_fImage.append(img);
    }

  m_minFrame = qMax(1, m_fno[m_fno.count()-1]-3*m_frameStep);
  calcMaxFrame();
      
  calcRect();
  update();
}

void
KeyFrameEditor::setKeyFrame()
{
  if (!m_hiresMode)
    {
      emit showMessage("Set Keyframe available only in Hires Mode. Press F2 to switch to Hires mode", true);
      qApp->processEvents();
      return;
    }

  bool found = false;
  for(int i=0; i<m_fno.count(); i++)
    {
      if (m_fno[i] == m_currFrame)
	{
	  found = true;
	  break;
	}
    }

  if (found)
    {
      QApplication::beep();
      int but = QMessageBox::question(0, "Set KeyFrame",
	 QString("Do you want to overwrite current keyFrame at %1").\
						 arg(m_currFrame),
			       QMessageBox::Ok | QMessageBox::Cancel,
				 		      QMessageBox::Ok);
      if (but != QMessageBox::Ok)
	return;
    }

  if (!found)
    {
      m_fno.append(m_currFrame);
      
      m_fRect.append(QRect(-10,-10,1,1));
      
      QImage img(100,100, QImage::Format_RGB32);
      m_fImage.append(img);
      
      m_minFrame = qMax(1, m_fno[m_fno.count()-1]-3*m_frameStep);
      calcMaxFrame();
      
      calcRect();
      update();
    }

  emit setKeyFrame(m_currFrame);
}

void
KeyFrameEditor::removeKeyFrame()
{
  if (!m_hiresMode)
    {
      emit showMessage("Remove Keyframe available only in Hires Mode. Press F2 to switch to Hires mode", true);
      qApp->processEvents();
      return;
    }

  if (m_selectRegion.valid)
    {
      int f0 = m_selectRegion.keyframe0;
      int f1 = m_selectRegion.keyframe1;

      for(int i=f0; i<=f1; i++)
	{
	  m_fno.removeAt(f0);
	  m_fRect.removeAt(f0);
	  m_fImage.removeAt(f0);

//	    QList<int>::iterator itfno=m_fno.begin()+f0;
//	    m_fno.erase(itfno);
//
//	    QList<QRect>::iterator itfRect=m_fRect.begin()+f0;
//	    m_fRect.erase(itfRect);
//
//	    QList<QImage>::iterator itfImage=m_fImage.begin()+f0;
//	    m_fImage.erase(itfImage);
	}

      emit removeKeyFrames(f0, f1);

      m_selected = -1;
      m_selectRegion.valid = false;

      update();

      return;
    }


  if (m_selected < 0)
    {
      emit showMessage("No keyframe selected for removal", true);
      qApp->processEvents();
      return;
    }

  QList<int>::iterator itfno=m_fno.begin()+m_selected;
  m_fno.erase(itfno);

  QList<QRect>::iterator itfRect=m_fRect.begin()+m_selected;
  m_fRect.erase(itfRect);

  QList<QImage>::iterator itfImage=m_fImage.begin()+m_selected;
  m_fImage.erase(itfImage);

  emit removeKeyFrame(m_selected);

  m_selected = -1;

  update();
}

void
KeyFrameEditor::reorder()
{
  m_reordered = true;

  tag *fnoTags;
  fnoTags = new tag[m_fno.count()];

  for(int i=0; i<m_fno.count(); i++)
    {
      fnoTags[i].id = i;
      fnoTags[i].loc = m_fno[i];
    }

  sort(fnoTags, fnoTags+m_fno.count());

  QList<int> dfno(m_fno);
  QList<QRect> dRect(m_fRect);
  QList<QImage> dImg(m_fImage);

  for(int i=0; i<m_fno.count(); i++)
    {
      int id = fnoTags[i].id;
      m_fno[i] = dfno[id];
      m_fRect[i] = dRect[id];
      m_fImage[i] = dImg[id];
    }

  if (m_selected >= 0)
    m_selected = fnoTags[m_selected].id;

  QList<int>sortedId;
  for(int i=0; i<m_fno.count(); i++)
    sortedId << fnoTags[i].id;

  emit reorder(sortedId);

  delete [] fnoTags;
}

void
KeyFrameEditor::loadKeyframes(QList<int> framenumbers,
			      QList<QImage> images)
{
  clear();

  m_fno += framenumbers;
  m_fImage += images;
  for(int i=0; i<framenumbers.size(); i++)
    m_fRect.append(QRect(-10,-10,1,1));

  m_currFrame = 1;
  calcMaxFrame();

  reorder();

  update();  
}

void
KeyFrameEditor::moveTo(int fno)
{
  // update current frame and display
  m_currFrame = fno;
  if (m_currFrame < m_minFrame)
    {
      m_minFrame = m_currFrame;
      calcMaxFrame();
    }
  if (m_currFrame > m_maxFrame)
    {
      m_minFrame = qMax(1, m_currFrame-1);
      calcMaxFrame();
    }
  update();
  //----------------------------
  
  emit playFrameNumber(m_currFrame);
  qApp->processEvents();
}

void
KeyFrameEditor::showHelp()
{
  PropertyEditor propertyEditor;
  QMap<QString, QVariantList> plist;
  QVariantList vlist;

  vlist.clear();
  QFile helpFile(":/keyframeeditor.help");
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
  
  propertyEditor.set("Keyframe Editor Help", plist, keys);
  propertyEditor.exec();
}
