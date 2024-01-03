#include "captionwidget.h"
#include "shaderfactory.h"
#include "staticfunctions.h"

#include <QMessageBox>
#include <QtMath>

QList<CaptionWidget*> CaptionWidget::widgets;

CaptionWidget*
CaptionWidget::getWidget(QString name)
{
  for (int i=0; i<widgets.count(); i++)
    {
      QString attr = widgets[i]->name().toLower().trimmed();
      if (attr == name)
	return widgets[i];
    }
  return 0;
}

void
CaptionWidget::setText(QString name, QString text)
{
  for(int i=0; i<widgets.count(); i++)
    {
      QString attr = widgets[i]->name().toLower().trimmed();
      if (attr == name)
	{
	  widgets[i]->setText(text);
	  return;
	}
    }
}

void
CaptionWidget::blink(QString name, int val)
{
  for(int i=0; i<widgets.count(); i++)
    {
      QString attr = widgets[i]->name().toLower().trimmed();
      if (attr == name)
	{
	  widgets[i]->blink(val);
	  return;
	}
    }
}

void
CaptionWidget::blinkAndHide(QString name, int val)
{
  for(int i=0; i<widgets.count(); i++)
    {
      QString attr = widgets[i]->name().toLower().trimmed();
      if (attr == name)
	{
	  widgets[i]->blinkAndHide(val);
	  return;
	}
    }
}

CaptionWidget::CaptionWidget() : QObject()
{
  m_glVA = 0;
  m_glVB = 0;
  m_glIB = 0;
  m_glTexture = 0;

  m_fontSize = 30;
  m_caption = "Information";
  m_color = Vec(1,1,1);

  m_texWd = m_texHt = 0;
  m_captionMat.setToIdentity();

  m_blink = 0;
  m_hideAfterBlink = false;
  
  widgets << this;
}

CaptionWidget::~CaptionWidget()
{
  glDeleteVertexArrays( 1, &m_glVA );
  glDeleteBuffers(1, &m_glVB);
  glDeleteBuffers(1, &m_glIB);
  glDeleteTextures(1, &m_glTexture);
  
  m_glVA = 0;
  m_glVB = 0;
  m_glIB = 0;
  m_glTexture = 0;

  m_captionMat.setToIdentity();
}

void
CaptionWidget::blink(int val)
{
  m_blink = val;
  m_hideAfterBlink = false;
}

void
CaptionWidget::blinkAndHide(int val)
{
  m_blink = val;
  m_hideAfterBlink = true;
}

void
CaptionWidget::generateCaption(QString name, QString caption)
{
  m_name = name;

  
  if (!m_glVA)
    glGenVertexArrays(1, &m_glVA);
  
  if (!m_glVB)
    glGenBuffers(1, &m_glVB);

  if (!m_glIB)
    glGenBuffers( 1, &m_glIB );

  if (!m_glTexture)
    glGenTextures(1, &m_glTexture);

  
  glBindVertexArray(m_glVA);
  glBindBuffer(GL_ARRAY_BUFFER, m_glVB );
  
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0,  // attribute 0
			3,  // size
			GL_FLOAT, // type
			GL_FALSE, // normalized
			sizeof(float)*8, // stride
			(char *)NULL); // starting offset
    
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1,  // attribute 1
			3,  // size
			GL_UNSIGNED_BYTE, // type
			GL_FALSE, // normalized
			sizeof(float)*8, // stride
			(char *)NULL + sizeof(float)*3 );
  
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2,
			2,
			GL_FLOAT,
			GL_FALSE, 
			sizeof(float)*8,
			(char *)NULL + sizeof(float)*6 );
  

  float *vertData = new float[32];      
  memset(vertData, 0, 32*sizeof(float));  
  vertData[6]  = 0.0;   vertData[7] = 0.0;
  vertData[14] = 0.0;  vertData[15] = 1.0;
  vertData[22] = 1.0;  vertData[23] = 1.0;
  vertData[30] = 1.0;  vertData[31] = 0.0;
    
  vertData[0] = -0.5;  vertData[1] =-0.5;  vertData[2] = 0;
  vertData[8] = -0.5;  vertData[9] = 0.5; vertData[10] = 0;
  vertData[16] = 0.5; vertData[17] = 0.5; vertData[18] = 0;
  vertData[24] = 0.5; vertData[25] =-0.5; vertData[26] = 0;

  glBufferData(GL_ARRAY_BUFFER,
	       sizeof(float)*32,
	       &vertData[0],
	       GL_STATIC_DRAW);
  
  delete [] vertData;

  glBindVertexArray(0);
  
  
  uchar indexData[6];
  indexData[0] = 0;
  indexData[1] = 1;
  indexData[2] = 2;
  indexData[3] = 0;
  indexData[4] = 2;
  indexData[5] = 3;

  glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_glIB );
  glBufferData( GL_ELEMENT_ARRAY_BUFFER,
		sizeof(uchar) * 6,
		&indexData[0],
		GL_STATIC_DRAW );

  setText(caption);
}

void
CaptionWidget::setText(QString caption)
{
  m_caption = caption;

  QFont font = QFont("Helvetica", m_fontSize);
  QFontMetrics metric(font);
  QColor color(m_color.z*255,m_color.y*255,m_color.x*255); 

  QImage image = StaticFunctions::renderText(m_caption,
					     font,
					     Qt::black,
					     Qt::white,
					     true);
  m_texWd = image.width();
  m_texHt = image.height();

//  int slen = metric.width(m_caption);
//  int cps = 512; // texture width is 512
//  int nls = slen/cps;
//  if (slen >= nls*cps)
//    nls++;
//  QImage tmpTex = StaticFunctions::drawText(512, metric.height()*nls,
//					    m_caption,
//					    color,
//					    Qt::gray,
//					    font);      
//  m_texWd = tmpTex.width()+5;
//  m_texHt = tmpTex.height()+5;
//
//  QImage image = QImage(m_texWd, m_texHt, QImage::Format_ARGB32);
//  image.fill(Qt::black);
//  QPainter p(&image);
//  p.setPen(QPen(Qt::gray, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
//  p.drawRoundedRect(1, 1, m_texWd-2, m_texHt-2, 5, 5);
//  p.drawImage(1, 1, tmpTex);

  glActiveTexture(GL_TEXTURE4);
  glBindTexture(GL_TEXTURE_2D, m_glTexture);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D,
	       0,
	       4,
	       m_texWd,
	       m_texHt,
	       0,
	       GL_RGBA,
	       GL_UNSIGNED_BYTE,
	       image.bits());
  
  glDisable(GL_TEXTURE_2D);

  float frc = 1.0/qMax(m_texWd, m_texHt);
  float twd = m_texWd*frc;
  float tht = m_texHt*frc;

  float sclw = 0.5*twd;
  float sclh = 0.5*tht;
  int nlines = m_caption.split("\n").count();
  //if (sclh > 0.1)// limit height
    {
      frc = nlines*0.1/sclh;
      sclh *= frc;
      sclw *= frc;
    }
  m_captionMat.setToIdentity();
  //m_captionMat.translate(-0.4, 0.3, -1.0);
  //m_captionMat.rotate(45, 0, 1, 0);
  Vec v(0, -0.6-nlines*0.15+sclh, -1.5);
  float angle = Quaternion(v, Vec(0,1,0)).angle();
  Vec axis = Quaternion(v, Vec(0,1,0)).axis();
  angle = qRadiansToDegrees(angle);
  m_captionMat.translate(v.x, v.y, v.z);  
  m_captionMat.rotate(-angle/2, axis.x, axis.y, axis.z);
  m_captionMat.scale(sclw, sclh, 1);  
}

void
CaptionWidget::draw(QMatrix4x4 mvp, QMatrix4x4 hmdMat)
{
  if (m_hideAfterBlink && m_blink < 1)
    return;

  glDisable(GL_DEPTH_TEST);

  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  QMatrix4x4 cmvp = mvp * hmdMat * m_captionMat;  

  float op = qMin(1.0, 0.3 + (m_blink%100)*0.01);
  m_blink = qMax(m_blink-1, 0);

  glUseProgram(ShaderFactory::rcShader());
  GLint *rcShaderParm = ShaderFactory::rcShaderParm();
  glUniformMatrix4fv(rcShaderParm[0], 1, GL_FALSE, cmvp.data() );  
  glUniform1i(rcShaderParm[1], 4); // texture
  glUniform3f(rcShaderParm[2], 0,0,0); // color
  glUniform3f(rcShaderParm[3], 0,0,0); // view direction
  glUniform1f(rcShaderParm[4], op); // opacity modulator
  glUniform1i(rcShaderParm[5], 2); // apply texture
  glUniform1f(rcShaderParm[7], 0); // mix color
  glUniform1i(rcShaderParm[8], 0); // apply light


  // Create and populate the index buffer

  glBindVertexArray(m_glVA);  
  glBindBuffer(GL_ARRAY_BUFFER, m_glVB);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_glIB);  


  glActiveTexture(GL_TEXTURE4);
  glBindTexture(GL_TEXTURE_2D, m_glTexture);
  glEnable(GL_TEXTURE_2D);

  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0);  

  glBindVertexArray(0);

  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);  
  glDisableVertexAttribArray(2);  

  glBindVertexArray(0);

  glUseProgram(0);

  glDisable(GL_TEXTURE_2D);

  glDisable(GL_BLEND);

  glEnable(GL_DEPTH_TEST);
}
