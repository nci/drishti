#include "menu01.h"
#include "shaderfactory.h"
#include "global.h"
#include "staticfunctions.h"

#include <QFont>
#include <QColor>
#include <QtMath>
#include <QMessageBox>

Menu01::Menu01() : QObject()
{
  m_glTexture = 0;
  m_glVertBuffer = 0;
  m_glIndexBuffer = 0;
  m_glVertArray = 0;
  m_vertData = 0;
  m_textData = 0;

  m_menuScale = 0.5;
  m_menuDist = 0.7;

  m_menuList.clear();

  m_relGeom.clear();
  m_optionsGeom.clear();

  m_selected = -1;

  m_visible = true;
  m_pointingToMenu = false;

  m_nIcons = 0;
  m_nSliders = 0;
  m_checkBox.clear();
  m_buttons.clear();
}

Menu01::~Menu01()
{
  if(m_vertData)
    {
      delete [] m_vertData;
      delete [] m_textData;
      m_vertData = 0;
      m_textData = 0;
      glDeleteBuffers(1, &m_glIndexBuffer);
      glDeleteVertexArrays( 1, &m_glVertArray );
      glDeleteBuffers(1, &m_glVertBuffer);
      m_glIndexBuffer = 0;
      m_glVertArray = 0;
      m_glVertBuffer = 0;
    }

  m_buttons.clear();
}

float
Menu01::value(QString name)
{
  for (int i=0; i<m_buttons.count(); i++)
    {
      QString attr = m_buttons[i].text().toLower().trimmed();
      if (attr == name)
      {
	return m_buttons[i].value();
      }
    }
  return -1000000;
}

void
Menu01::setValue(QString name, float val)
{
  for (int i=0; i<m_buttons.count(); i++)
    {
      QString attr = m_buttons[i].text().toLower().trimmed();
      if (attr == name)
      {
	m_buttons[i].setValue(val);
	break;
      }
    }
}

void
Menu01::genVertData()
{
  if (!m_vertData)
    m_vertData = new float[32];
  memset(m_vertData, 0, sizeof(float)*32);

  if (!m_textData)
    m_textData = new float[32];
  memset(m_textData, 0, sizeof(float)*32);

  // create and bind a VAO to hold state for this model
  if (!m_glVertArray)
    glGenVertexArrays( 1, &m_glVertArray );
  glBindVertexArray( m_glVertArray );
      
      // Populate a vertex buffer
  if (!m_glVertBuffer)
    glGenBuffers( 1, &m_glVertBuffer );
  glBindBuffer( GL_ARRAY_BUFFER, m_glVertBuffer );
  glBufferData( GL_ARRAY_BUFFER,
		sizeof(float)*8*4,
		NULL,
		GL_STATIC_DRAW );
      
  // Identify the components in the vertex buffer
  glEnableVertexAttribArray( 0 );
  glVertexAttribPointer( 0, //attribute 0
			 3, // size
			 GL_FLOAT, // type
			 GL_FALSE, // normalized
			 sizeof(float)*8, // stride
			 (void *)0 ); // starting offset

  glEnableVertexAttribArray( 1 );
  glVertexAttribPointer( 1,
			 3,
			 GL_FLOAT,
			 GL_FALSE,
			 sizeof(float)*8,
			 (char *)NULL + sizeof(float)*3 );
  
  glEnableVertexAttribArray( 2 );
  glVertexAttribPointer( 2,
			 2,
			 GL_FLOAT,
			 GL_FALSE, 
			 sizeof(float)*8,
			 (char *)NULL + sizeof(float)*6 );
  
  
  
  uchar indexData[6];
  indexData[0] = 0;
  indexData[1] = 1;
  indexData[2] = 2;
  indexData[3] = 0;
  indexData[4] = 2;
  indexData[5] = 3;
  
  // Create and populate the index buffer
  if (!m_glIndexBuffer)
    glGenBuffers( 1, &m_glIndexBuffer );
  glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_glIndexBuffer );
  glBufferData( GL_ELEMENT_ARRAY_BUFFER,
		sizeof(uchar) * 6,
		&indexData[0],
		GL_STATIC_DRAW );
  
  glBindVertexArray( 0 );
    
      
  QImage aplus(":/images/aplus.png");
  aplus = aplus.scaledToHeight(90, Qt::SmoothTransformation);

  QImage grabImg(":/images/grab_white.png");
  QImage clipImg(":/images/clip.png");
  QImage transImg(":/images/translate.png");
  QImage cimg[3];
  cimg[0] = grabImg.scaledToHeight(90, Qt::SmoothTransformation);
  cimg[1] = clipImg.scaledToHeight(90, Qt::SmoothTransformation);
  cimg[2] = transImg.scaledToHeight(90, Qt::SmoothTransformation);


  
  QImage scaleImg(":/images/scale.png");
  scaleImg = scaleImg.scaledToHeight(90, Qt::SmoothTransformation);

  
  if (m_relGeom.count() == 0)
    {
      QFont font = QFont("Helvetica", 48);
      QColor color(250, 230, 210); 
      
      m_checkBox.clear();

      // single icons here
      m_checkBox << "Grab";
      //m_checkBox << "Translate";
      m_nIcons = m_checkBox.count();

      // all sliders here
      m_checkBox << "   Scale";
      m_checkBox << "  Bright";
      m_checkBox << "Softness";
      m_checkBox << "   Edges";
      m_checkBox << " Explode";
      //m_checkBox << "   PAINT";
      m_nSliders = m_checkBox.count()-m_nIcons;
      
      // min and max slider values
      QList<int> sldMin, sldMax;
      sldMin << 1 << 0 << 0 << 0 << 0;
      sldMax << 5 << 1 << 1 << 1 << 1;
      
      m_texWd = 560;
      m_texHt = 210 + (m_nSliders+1)*100;
      
      QImage pimg = QImage(m_texWd, m_texHt, QImage::Format_ARGB32);
      pimg.fill(Qt::black);

      QPainter p(&pimg);      
      p.setBrush(Qt::black);
      QPen ppen(Qt::white);
      ppen.setWidth(5);
      p.setPen(ppen);
      

      p.drawRoundedRect(5,5,m_texWd-10,m_texHt-10, 5, 5);

      // just for display
      p.drawImage(m_texWd/2-45, 10, scaleImg);

        
      Button button;
      m_buttons.clear();
  
      button.setType(0); // simple pushbutton
      button.setText("RESET");
      m_buttons << button; 

      // reset button
      QImage img = StaticFunctions::renderText("RESET",
					       font,
					       Qt::black, color, true);
      img = img.scaledToHeight(90, Qt::SmoothTransformation);
      int twd = img.width();
      int tht = img.height();
      
      int ws = (560-twd)/2;

      //-------------------
      //-------------------
      m_relGeom.clear();
      m_relGeom << QRect(ws, 10, twd, 90); // RESET

      if (m_checkBox.count() > 0)
	{
	  for(int i=0; i<m_nIcons; i++)
	    m_relGeom << QRect(10 + i*100, 110, 90, 90); // checkBox

	  for(int i=m_nIcons; i<m_checkBox.count(); i++)
	    {
	      int sldh = 210 + (i-m_nIcons)*100;
	      m_relGeom << QRect(240, sldh+40, 300, 40); // sliders
	    }
	}
      //-------------------
      //-------------------


      // RESET
      p.drawImage(ws, m_texHt-m_relGeom[0].y()-90, img.mirrored(false,true));


      //-------------------
      // check boxes
      for(int i=0; i<m_nIcons; i++)
      {
	//--------------
	// we are not going to put any text here
	//QImage img = StaticFunctions::renderText(m_checkBox[i],
	//					 font,
	//					 Qt::black, color, false);
	//img = img.scaledToHeight(90, Qt::SmoothTransformation); 
	//if (img.width() > 350)
	//  img = img.scaledToWidth(350, Qt::SmoothTransformation);
	//
	//int twd = img.width();
	//int ws = 360 - twd;
	//--------------

	p.drawImage(m_relGeom[i+1].x(),
		    m_texHt-m_relGeom[i+1].y()-90,
		    cimg[i]);

	Button button;
	//button.setText(m_checkBox[m_nSliders+i]);
	button.setText(m_checkBox[i]);
	button.setType(1); // checkBox
	button.setMinMaxValues(0, 1);
	button.setValue(0);
	m_buttons << button;      
      }
      //-------------------

      
      //-------------------
      // sliders
      for(int i=m_nIcons; i<m_checkBox.count(); i++)
      {
	//QImage img = StaticFunctions::renderText(m_checkBox[i-m_nIcons],
	QImage img = StaticFunctions::renderText(m_checkBox[i],
						 font,
						 Qt::black, color, false);
	img = img.scaledToHeight(90, Qt::SmoothTransformation); 
	if (img.width() > 200)
	  img = img.scaledToWidth(200, Qt::SmoothTransformation);
	
	int twd = img.width();
	int ws = 210 - twd;
	p.drawImage(ws,
		    m_texHt-m_relGeom[i+1].y()-50,
		    img.mirrored(false,true));


	{ // min value
	  QImage img = StaticFunctions::renderText(QString("%1").arg(sldMin[i-m_nIcons]),
						   font,
						   Qt::transparent, color, false);
	  img = img.scaledToHeight(20, Qt::SmoothTransformation); 
	  p.drawImage(240,
		      m_texHt-m_relGeom[i+1].y(),
		      img.mirrored(false,true));
	}
	
	{ // max value
	  QImage img = StaticFunctions::renderText(QString("%1").arg(sldMax[i-m_nIcons]),
						   font,
						   Qt::transparent, color, false);
	  img = img.scaledToHeight(20, Qt::SmoothTransformation); 
	  p.drawImage(520,
		      m_texHt-m_relGeom[i+1].y(),
		      img.mirrored(false,true));
	}

	p.drawRoundedRect(240, m_texHt-m_relGeom[i+1].y()-40,
			  300, 40, 5, 5);

	Button button;
	button.setText(m_checkBox[i]);
	button.setType(2); // slider
	QString txt = m_checkBox[i].toLower().trimmed();
	if (txt == "scale")
	  {
	    button.setMinMaxValues(0, 100);
	    button.setValue(0);
	  }
	else if (txt == "edges")
	  {
	    button.setMinMaxValues(0, 10);
	    button.setValue(5);
	  }
	else if (txt == "softness")
	  {
	    button.setMinMaxValues(0, 10);
	    button.setValue(5);
	  }
	else if (txt == "bright")
	  {
	    button.setMinMaxValues(0, 100);
	    button.setValue(50);
	  }
	else if (txt == "explode")
	  {
	    button.setMinMaxValues(0, 1);
	    button.setValue(0);
	  }
	
	m_buttons << button;      
      }
      //-------------------

	      
      //-------------------
      if (!m_glTexture)
	glGenTextures(1, &m_glTexture);
  
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
		   pimg.bits());
  
      glDisable(GL_TEXTURE_2D);
      //-------------------
      
    }

  m_optionsGeom.clear();
  for (int i=0; i<m_relGeom.count(); i++)
    {
       float cx = m_relGeom[i].x();
       float cy = m_relGeom[i].y();
      float cwd = m_relGeom[i].width();
      float cht = m_relGeom[i].height();

      m_optionsGeom << QRectF(cx/(float)m_texWd,
			      cy/(float)m_texHt,
			      cwd/(float)m_texWd,
			      cht/(float)m_texHt);
    }


  for(int i=0; i<m_buttons.count(); i++)
    {
      m_buttons[i].setRect(m_relGeom[i]);
      m_buttons[i].setGeom(m_optionsGeom[i]);
    }

}

void
Menu01::reGenerateHUD()
{
  generateHUD(m_hmdMat);
}

void
Menu01::generateHUD(QMatrix4x4 hmdMat)
{
  m_hmdMat = hmdMat;
  
  if (!m_vertData)
    genVertData();

  QVector3D cen= QVector3D(hmdMat * QVector4D(0,0,0,1));
  QVector3D cx = QVector3D(hmdMat * QVector4D(1,0,0,1));
  QVector3D cy = QVector3D(hmdMat * QVector4D(0,1,0,1));
  QVector3D cz = QVector3D(hmdMat * QVector4D(0,0,1,1));

  float frc = 2.0/qMax(m_texWd, m_texHt);

  QVector3D vDir = (cz-cen).normalized();
  QVector3D rDir = QVector3D::crossProduct(QVector3D(0,1,0), vDir).normalized();
  QVector3D uDir = QVector3D::crossProduct(vDir, rDir).normalized();
  

  uDir = QVector3D(0,1,0);
  vDir = -QVector3D::crossProduct(uDir, rDir).normalized();

  // set menu 2.0m in front and 0.5m downwards
  cen= QVector3D(hmdMat * QVector4D(0,-0.5,-2.0,1));
  cen = cen - m_menuScale*uDir;


  QVector3D fv = uDir;
  QVector3D fr = rDir;

  QVector3D vu = frc*m_texHt*m_menuScale*fv;
  QVector3D vr0 = cen - frc*m_texWd*m_menuScale*fr/2;
  QVector3D vr1 = cen + frc*m_texWd*m_menuScale*fr/2;


  m_up = vDir;
  m_vleft = vr0;
  m_vright = frc*m_texWd*rDir;
  m_vfrontActual = -frc*m_texHt*uDir;
  
  QVector3D v0 = vr0;
  QVector3D v1 = vr0 + vu;
  QVector3D v2 = vr1 + vu;
  QVector3D v3 = vr1;

  QVector3D v[4];  
  v[0] = vr0;
  v[1] = vr0 + vu;
  v[2] = vr1 + vu;
  v[3] = vr1;
      
  for(int i=0; i<4; i++)
    {
      m_vertData[8*i + 0] = v[i].x();
      m_vertData[8*i + 1] = v[i].y();
      m_vertData[8*i + 2] = v[i].z();
      m_vertData[8*i+3] = 0;
      m_vertData[8*i+4] = 0;
      m_vertData[8*i+5] = 0;
    }

  float texC[] = {0,1, 0,0, 1,0, 1,1};
  texC[0] = 0;
  texC[1] = 1;
  texC[2] = 0;
  texC[3] = 0;
  texC[4] = 1;
  texC[5] = 0;
  texC[6] = 1;
  texC[7] = 1;
  

  m_vertData[6] =  texC[0];  m_vertData[7] =  texC[1];
  m_vertData[14] = texC[2];  m_vertData[15] = texC[3];
  m_vertData[22] = texC[4];  m_vertData[23] = texC[5];
  m_vertData[30] = texC[6];  m_vertData[31] = texC[7];
  
  glBindVertexArray(m_glVertArray);
  glBindBuffer( GL_ARRAY_BUFFER, m_glVertBuffer);
  glBufferSubData(GL_ARRAY_BUFFER,
		  0,
		  sizeof(float)*8*4,
		  &m_vertData[0]);
}

void
Menu01::draw(QMatrix4x4 mvp, QMatrix4x4 matL, bool triggerPressed)
{
  if (!m_visible)
    return;
  
  if (!m_vertData)
    genVertData();


  glActiveTexture(GL_TEXTURE4);
  glBindTexture(GL_TEXTURE_2D, m_glTexture);
  glEnable(GL_TEXTURE_2D);

  glBindVertexArray(m_glVertArray);
  glBindBuffer( GL_ARRAY_BUFFER, m_glVertBuffer);
  glBufferSubData(GL_ARRAY_BUFFER,
		  0,
		  sizeof(float)*8*4,
		  &m_vertData[0]);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_glIndexBuffer);  

  
  glDisable(GL_DEPTH_TEST);
  glDepthMask(GL_FALSE);
  
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);


  glUseProgram(ShaderFactory::rcShader());
  GLint *rcShaderParm = ShaderFactory::rcShaderParm();
  glUniformMatrix4fv(rcShaderParm[0], 1, GL_FALSE, mvp.data() );  
  glUniform1i(rcShaderParm[1], 4); // texture
  glUniform3f(rcShaderParm[2], 0, 0, 0); // color
  glUniform3f(rcShaderParm[3], 0, 0, 0); // view direction
  glUniform1f(rcShaderParm[4], 0.7); // opacity modulator
  glUniform1i(rcShaderParm[5], 5); // texture + color
  glUniform1f(rcShaderParm[6], 20); // pointsize

  glUniform1f(rcShaderParm[7], 0.5); // mixcolor

  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0);  
    

  for(int b=0; b<m_buttons.count(); b++)
    {
      QRectF geom = m_buttons[b].geom();
      
      float cx = geom.x();
      float cy = geom.y();
      float cwd = geom.width();
      float cht = geom.height();

      Vec mc = Vec(0,0,0);
	  
      if (m_buttons[b].type() != 2) // not a slider
	{
	  if (m_buttons[b].value() > m_buttons[b].minValue())
	    mc = Vec(0.5, 1.0, 0.5);

	  if (b == m_selected)
	    {
	      if (triggerPressed)
		mc = Vec(0.2, 0.8, 0.2);
	      else
		mc = Vec(0.5, 0.7, 1);
	    }

	  if (mc.x > 0.0)	    
	    showText(m_glTexture, geom,
		     m_vleft, m_vright, m_vfrontActual, m_up,
		     cx, cx+cwd, cy, cy+cht,
		     mc);
	}

      if (m_buttons[b].type() == 2) // slider
	{
	  float minV = m_buttons[b].minValue();
	  float maxV = m_buttons[b].maxValue();
	  float val = m_buttons[b].value();
	  float frc = (val-minV)/(maxV-minV);
	  float sx = cx + frc*(cwd-20.0/m_texWd);
	  float sy = cy;
	  float swd = 20.0/m_texWd;
	  float sht = cht;
	  showText(m_glTexture, QRectF(sx, sy, swd, sht),
		   m_vleft, m_vright, m_vfrontActual, m_up,
		   sx, sx+swd, sy, sy+sht,
		   Vec(0.2,1.0,0.2));
	}
    }

  // ----
  // show pinPt
  if (m_pinPt.x() > -1000)
    {
      glBlendFunc(GL_ONE, GL_ONE);
      //glBindTexture(GL_TEXTURE_2D, Global::hollowSpriteTexture());
      glBindTexture(GL_TEXTURE_2D, Global::spriteTexture());
      glEnable(GL_PROGRAM_POINT_SIZE );
      glEnable(GL_POINT_SPRITE);
      m_textData[0] = m_pinPt.x();
      m_textData[1] = m_pinPt.y();
      m_textData[2] = m_pinPt.z();
      glBufferSubData(GL_ARRAY_BUFFER,
		      0,
		      sizeof(float)*3,
		      &m_textData[0]);
      glUniform3f(rcShaderParm[2], 1, 1, 0); // mix color
      glUniform1i(rcShaderParm[5], 3); // point sprite
      glDrawArrays(GL_POINTS, 0, 1);
      glDisable(GL_POINT_SPRITE);
    }
  // ----

  glBindVertexArray(0);
  glDisable(GL_TEXTURE_2D);
  
  glUseProgram( 0 );
  
  glDisable(GL_BLEND);

  glDepthMask(GL_TRUE);
  glEnable(GL_DEPTH_TEST);
}

void
Menu01::showText(GLuint tex, QRectF geom,
		 QVector3D vleft, QVector3D vright,
		 QVector3D vfrontActual, QVector3D up,
		 float tx0, float tx1, float ty0, float ty1,
		 Vec mc)
{
  //glBindTexture(GL_TEXTURE_2D, tex);

  GLint *rcShaderParm = ShaderFactory::rcShaderParm();
  glUniform3f(rcShaderParm[2], mc.x, mc.y, mc.z); // mix color

  float cx =  geom.x();
  float cy =  geom.y();
  float cwd = geom.width();
  float cht = geom.height();

  QVector3D v[4];  
  v[0] = vleft + m_menuScale*cx*vright
               - m_menuScale*cy*vfrontActual
               + 0.002*up; // slightly raised
  v[1] = v[0] - m_menuScale*cht*vfrontActual;
  v[2] = v[1] + m_menuScale*cwd*vright;
  v[3] = v[2] + m_menuScale*cht*vfrontActual;
  
  for(int i=0; i<4; i++)
    {
      m_textData[8*i + 0] = v[i].x();
      m_textData[8*i + 1] = v[i].y();
      m_textData[8*i + 2] = v[i].z();
      m_textData[8*i+3] = 0;
      m_textData[8*i+4] = 0;
      m_textData[8*i+5] = 0;
    }
  
  float texD[] = {tx0,1-ty0, tx0,1-ty1, tx1,1-ty1, tx1,1-ty0};
	  
  m_textData[6] =  texD[0];  m_textData[7] =  texD[1];
  m_textData[14] = texD[2];  m_textData[15] = texD[3];
  m_textData[22] = texD[4];  m_textData[23] = texD[5];
  m_textData[30] = texD[6];  m_textData[31] = texD[7];
  
  glBufferSubData(GL_ARRAY_BUFFER,
		  0,
		  sizeof(float)*8*4,
		  &m_textData[0]);
  
  
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0);  
}

int
Menu01::checkOptions(QMatrix4x4 matL, QMatrix4x4 matR, int triggered)
{
  m_pinPt = QVector3D(-1000,-1000,-1000);
  m_pointingToMenu = false;
  m_selected = -1;

  if (!m_visible || m_optionsGeom.count()==0)
    return -1;

  QVector3D centerL = QVector3D(matL * QVector4D(0,0,0,1));
  QVector3D ofrontL, orightL, oupL;
  QVector3D frontL, rightL, upL;

  ofrontL = QVector3D(matL * QVector4D(0,0,1,1)) - centerL; 
  oupL = QVector3D(matL * QVector4D(0,1,0,1)) - centerL;
  orightL = QVector3D(matL * QVector4D(1,0,0,1)) - centerL;

  QVector3D centerR = QVector3D(matR * QVector4D(0,0,0,1));
  QVector3D frontR;
  frontR = QVector3D(matR * QVector4D(0,0,-0.1,1)) - centerR;    

  
  float rw, rh;
  m_pointingToMenu = StaticFunctions::intersectRayPlane(m_vleft,
						       -m_menuScale*m_vfrontActual,
							m_menuScale*m_vright,
							m_up,
							centerR, frontR.normalized(),
							rh, rw);

  if (!m_pointingToMenu)
    {
      if (triggered == 2) // released
	{
	  for(int b=0; b<m_buttons.count(); b++)
	    m_buttons[b].setGrabbed(false);
	}

      return -1;
    }


  m_pinPt = m_vleft + m_menuScale*rh*m_vright - m_menuScale*rw*m_vfrontActual;

  int tp = -1;
  float sldval = 0;

  // first handle button that currently have focus
  for(int b=0; b<m_buttons.count(); b++)
    {
      if (m_buttons[b].grabbed())
	{
	  tp = b;
	  if (m_buttons[b].type() == 2)
	    {
	      float xPos, yPos;
	      m_buttons[b].boxPos(rh, rw,
				  xPos, yPos);
	      sldval = qBound(0.0f, xPos, 1.0f);
	    }
	  break;
	}
    }
  
  // if no button found to have focus then check for others
  if (tp < 0)
    {
      for(int b=0; b<m_buttons.count(); b++)
	{
	  if (m_buttons[b].inBox(rh, rw))
	    {
	      tp = b;
	      if (m_buttons[b].type() == 2)
		{
		  float xPos, yPos;
		  m_buttons[b].boxPos(rh, rw,
				      xPos, yPos);
		  sldval = qBound(0.0f, xPos, 1.0f);
		}
	      break;
	    }
	}
    }

  m_selected = tp;

  if (m_selected < 0)
    return m_selected;
    

  if (triggered == 1) // pressed
    {
      // slider test
      if (m_buttons[m_selected].type() == 2)
	{
	  float minV = m_buttons[m_selected].minValue();
	  float maxV = m_buttons[m_selected].maxValue();
	  float val = (1-sldval)*minV + sldval*maxV;
	  m_buttons[m_selected].setValue(val);
	  
	  m_buttons[m_selected].setGrabbed(true);

	  emit toggle(m_buttons[m_selected].text(),
		      m_buttons[m_selected].value());
	    
	  return m_selected;
	}      
    }
  
  if (triggered == 2) // released
    {
      for(int b=0; b<m_buttons.count(); b++)
	m_buttons[b].setGrabbed(false);
      
      for(int b=0; b<m_buttons.count(); b++)
	{
	  if (m_selected == b)
	    {
	      if (m_buttons[b].type() == 0) // simple button
		{
		  // do nothing
		}
	      if (m_buttons[b].type() == 1) // checkbox
		{
		  if (m_buttons[b].value() == 0)
		    m_buttons[b].setValue(1);
		  else
		    m_buttons[b].setValue(0);
		}
	      else if (m_buttons[b].type() == 2) // slider
		{
	      	  float minV = m_buttons[b].minValue();
		  float maxV = m_buttons[b].maxValue();
		  float val = (1-sldval)*minV + sldval*maxV;
		  m_buttons[b].setValue(val);
		}

	      emit toggle(m_buttons[b].text(),
			  m_buttons[b].value());
	    }
	}
    }

  return m_selected;
}

