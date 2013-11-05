#include "viewer.h"
#include "messagedisplayer.h"
 
bool MessageDisplayer::renderScene() { return (!m_mesgShowing || m_firstTime); }
bool MessageDisplayer::showingMessage() { return m_mesgShowing; }

MessageDisplayer::MessageDisplayer(Viewer *viewer)
{
  m_Viewer = viewer;
  m_message = "";
  m_mesgShowing = false;
  m_firstTime = false;
  m_mesgShift = 0;
  m_mesgImage = 0;
}

MessageDisplayer::~MessageDisplayer()
{
  m_message = "";
  m_mesgShowing = false;
  m_firstTime = false;
  m_mesgShift = 0;
  if (m_mesgImage)
    delete [] m_mesgImage;  
}

void
MessageDisplayer::holdMessage(QString mesg, bool warn)
{
  m_message = mesg;
  m_warning = warn;
  m_mesgShowing = true;
  m_firstTime = true;
  m_mesgShift = 0;
  emit updateGL();
}

void
MessageDisplayer::turnOffMessage()
{
  m_mesgShowing = false;
  m_firstTime = false;
  m_message = "";
}

void
MessageDisplayer::renderMessage(QSize isize)
{
  QFont font("Helvetica", 14, QFont::Normal);
  QFontMetrics fm(font);
  int mde = fm.descent();

  QStringList strlist = m_message.split(". ",
					QString::SkipEmptyParts);

  int nlines = strlist.count();

  int ht0, ht1, lft;
  
  if (m_warning)
    {
      float cost = fabs(cos(m_mesgShift*0.1));  
      float expt;
      if (m_mesgShift < 50) // expt = 1.0 -> 0.25
	expt = 1.0f - 0.015f*m_mesgShift;
      else if (m_mesgShift < 101) // expt = 0.25 -> 0.0
	expt = 0.5f - 0.005f*m_mesgShift;
      else
	expt = 0.0;

      ht0 = isize.height()/2-nlines*fm.height()/2;
      ht0 -= 200*(cost*expt);
      ht1 = ht0 + nlines*fm.height();
      lft = 5;
    }
  else
    {
      ht0 = 15;
      ht1 = ht0 + nlines*fm.height();

      float cost = fabs(cos(m_mesgShift*0.4));  
      float expt;
      if (m_mesgShift < 20) // expt = 1.0 -> 0.0
	expt = 1.0f - 0.05f*m_mesgShift;
      else
	expt = 0.0;
      lft = 5 + 20*(cost*expt);
    }

  int maxlen = 0;
  for (int si=0; si<nlines; si++)
    {
      maxlen = qMax(maxlen, fm.width(strlist[si]));
    }
  maxlen += lft+20;

  // show transparent message
  // darken the image then render the message
  m_Viewer->startScreenCoordinatesSystem();
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // blend on top
    glColor4f(0.0f, 0.0f, 0.0f, 0.7f);
    glBegin(GL_QUADS);
    glVertex2f(lft-5, ht0-10);
    glVertex2f(maxlen, ht0-10);
    glVertex2f(maxlen, ht1+10);
    glVertex2f(lft-5, ht1+10);
    glEnd();
    if (m_warning)
      glColor4f(0.9f, 0.7f, 0.7f, 0.9f);
    else
      glColor4f(0.7f, 0.87f, 0.9f, 0.9f);
    glBegin(GL_LINE_STRIP);
    glVertex2f(lft, ht0-7);
    glVertex2f(maxlen-5, ht0-7);
    glVertex2f(maxlen-5, ht1+7);
    glVertex2f(lft, ht1+7);
    glVertex2f(lft, ht0-7);
    glEnd();
    int hi = (ht1-ht0)/nlines;
    float dim = qMin(1.0, m_mesgShift*0.05);
    for (int si=0; si<nlines; si++)
      {
	int ht = ht0 + (si+1)*hi;
	glColor4f(0.9f, 0.85f, 0.7f, 0.9f*dim);
	m_Viewer->renderText(lft+5, ht-mde,
			     strlist[si],
			     font);
      }
  m_Viewer->stopScreenCoordinatesSystem();
  
  if (m_mesgShift <= 100)
    QTimer::singleShot(5, this, SLOT(refreshGL()));

  m_mesgShift++;
}

void
MessageDisplayer::refreshGL()
{
  emit updateGL();
}

void
MessageDisplayer::drawMessage(QSize isize)
{
  if (!m_mesgShowing)
    return;

  if (m_firstTime)
    {
      // save screen image so that we can render message on top
      if (m_mesgImage) delete [] m_mesgImage;
      m_mesgImage = new GLubyte[4*isize.width()*
				  isize.height()];
      glFinish();
      glReadBuffer(GL_BACK);
      glReadPixels(0,
		   0,
		   isize.width(),
		   isize.height(),
		   GL_RGBA,
		   GL_UNSIGNED_BYTE,
		   m_mesgImage);
      QTimer::singleShot(1000, this, SLOT(turnOffMessage()));
      m_firstTime = false;
    }
  else
    {
      // splat the saved image
      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      glOrtho(0, isize.width(), 0, isize.height(), -1, 1);
      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();
      glRasterPos2i(0,0);
      glDrawPixels(isize.width(),
		   isize.height(),
		   GL_RGBA,
		   GL_UNSIGNED_BYTE,
		   m_mesgImage);
      glFlush();
    }

  renderMessage(isize);
}
