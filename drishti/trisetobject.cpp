#include "global.h"
#include "shaderfactory.h"
#include "staticfunctions.h"
#include "trisetobject.h"
#include "matrix.h"
#include "ply.h"
#include "matrix.h"
#include "volumeinformation.h"
#include "captiondialog.h"
#include "mainwindowui.h"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

#include <QFileDialog>

void
TrisetObject::gridSize(int &nx, int &ny, int &nz)
{
  nx = m_nX;
  ny = m_nY;
  nz = m_nZ;
}

void
TrisetObject::setLighting(QVector4D l)
{
  m_ambient = l.x();
  m_diffuse = l.y();
  m_specular = l.z();
  m_roughness = l.w();
}

TrisetObject::TrisetObject()
{
  QStringList ps;
  ps << "x";
  ps << "y";
  ps << "z";
  ps << "nx";
  ps << "ny";
  ps << "nz";
  ps << "red";
  ps << "green";
  ps << "blue";
  ps << "vertex_indices";
  ps << "vertex";
  ps << "face";

  for(int i=0; i<ps.count(); i++)
    {
      char *s;
      s = new char[ps[i].size()+1];
      strcpy(s, ps[i].toLatin1().data());
      plyStrings << s;
    }

  m_glVertBuffer = 0;
  m_glIndexBuffer = 0;
  m_glVertArray = 0;

  m_material.clear();

  m_co.clear();
  
  clear();
}

TrisetObject::~TrisetObject() { clear(); }

void
TrisetObject::rotate(Vec axis, float angle)
{
  Quaternion q(axis, DEG2RAD(angle));
  m_q = q*m_q;
}
void
TrisetObject::rotate(Quaternion q)
{
  m_q = q*m_q;
}
void
TrisetObject::setRotation(Quaternion q)
{
  m_q = q;
}

void
TrisetObject::enclosingBox(Vec &boxMin,
			   Vec &boxMax)
{
  boxMin = m_enclosingBox[0];
  boxMax = m_enclosingBox[6];

//  Vec tb[8];  
//  for(int i=0; i<8; i++)    
//    tb[i] = Matrix::xformVec(m_localXform, m_enclosingBox[i]);
//
//  boxMin = boxMax = tb[0];
//  for(int i=1; i<8; i++)    
//    {
//      boxMin = Vec(qMin(boxMin.x,tb[i].x),
//		   qMin(boxMin.y,tb[i].y),
//		   qMin(boxMin.z,tb[i].z));
//		   
//      boxMax = Vec(qMax(boxMax.x,tb[i].x),
//		   qMax(boxMax.y,tb[i].y),
//		   qMax(boxMax.z,tb[i].z));
//    }
}

void
TrisetObject::clear()
{
  m_show = true;
  m_clip = true;
  m_activeScale = 1.2;
  
  m_fileName.clear();
  m_centroid = Vec(0,0,0);
  m_position = Vec(0,0,0);
  m_scale = Vec(1,1,1);
  m_reveal = 0.0;
  m_outline = 0.0;
  m_glow = 0.0;  
  m_dark = 0.5;
  m_pattern = Vec(0,10,0.5);
  m_q = Quaternion();
  m_nX = m_nY = m_nZ = 0;
  m_color = Vec(0,0,0);
  m_cropcolor = Vec(0.0f,0.0f,0.0f);
  m_roughness = 7;
  m_specular = 1.0f;
  m_diffuse = 1.0f;
  m_ambient = 0.0f;
  m_opacity = 0.7;
  m_vertices.clear();
  m_vcolor.clear();
  m_uv.clear();
  m_drawcolor.clear();
  m_normals.clear();
  m_triangles.clear();
  m_samplePoints.clear();
  
  if(m_glVertArray)
    {
      glDeleteBuffers(1, &m_glIndexBuffer);
      glDeleteVertexArrays( 1, &m_glVertArray );
      glDeleteBuffers(1, &m_glVertBuffer);
      m_glIndexBuffer = 0;
      m_glVertArray = 0;
      m_glVertBuffer = 0;
    }
  if (m_material.count() > 0)
    {
      glDeleteTextures(m_material.count(), m_diffuseTex);
      for(int d=0; d<m_material.count(); d++)
	m_diffuseTex[d] = 0;
    }
  m_material.clear();
  m_meshInfo.clear();
  
  m_featherSize = 1;

  m_captionPosition.clear();
  if (m_co.count() > 0)
    {
      for(int i=0; i<m_co.count(); i++)
	delete m_co[i];
    }
  m_co.clear();
  
  Matrix::identity(m_localXform);
}

void
TrisetObject::setColor(Vec color)
{
  if (m_uv.count() > 0)
    {
      QMessageBox::information(0, "", "Cannot change color for textured objects");
      return;
    }
  
  m_color = color;

  for(int i=0; i<m_vcolor.count()/3; i++)
    {
      m_vcolor[3*i+0] = m_color.x;
      m_vcolor[3*i+1] = m_color.y;
      m_vcolor[3*i+2] = m_color.z;
    }  

  copy2OrigVcolor();

  loadVertexBufferData();
}

void
TrisetObject::mirror(int type)
{
  int nvert = m_vertices.count()/3;
  for(int i=0; i<nvert; i++)
    {
      Vec v = Vec(m_vertices[3*i+0],m_vertices[3*i+1],m_vertices[3*i+2]);
      v[type] = -v[type]+2*m_centroid[type];
      m_vertices[3*i+0] = v.x;
      m_vertices[3*i+1] = v.y;
      m_vertices[3*i+2] = v.z;

      v = Vec(m_normals[3*i+0],m_normals[3*i+1],m_normals[3*i+2]);
      v[type] = -v[type];
      m_normals[3*i+0] = v.x;
      m_normals[3*i+1] = v.y;
      m_normals[3*i+2] = v.z;
    }
  loadVertexBufferData();
}

bool
TrisetObject::load(QString flnm)
{
  bool loaded;
  loaded = loadAssimpModel(flnm);

  m_featherSize = 0.005*(m_enclosingBox[6]-m_enclosingBox[0]).norm();

  
  if (loaded)
    {
      copy2OrigVcolor();
      loadVertexBufferData();
      return true;
    }
  return false;
}

void
TrisetObject::loadVertexBufferData()
{
  int stride = 3;
  if (m_normals.count()) stride += 3; // vertex normal
  if (m_uv.count() > 0 ||
      m_vcolor.count()) stride += 3; // vertex color or texture uv
  
  int nvert = m_vertices.count()/3;
  int nv = stride*nvert;
  int ni = m_triangles.count();

  //---------------------
  float *vertData;
  vertData = new float[nv];  
  for(int i=0; i<nvert; i++)
    {
      vertData[stride*i + 0] = m_vertices[3*i+0];
      vertData[stride*i + 1] = m_vertices[3*i+1];
      vertData[stride*i + 2] = m_vertices[3*i+2];
      vertData[stride*i + 3] =  m_normals[3*i+0];
      vertData[stride*i + 4] =  m_normals[3*i+1];
      vertData[stride*i + 5] =  m_normals[3*i+2];
    }

  if (m_uv.count() > 0)
    {
      for(int i=0; i<nvert; i++)
	{
	  vertData[stride*i + 6] = m_uv[3*i+0];
	  vertData[stride*i + 7] = m_uv[3*i+1];
	  vertData[stride*i + 8] = m_uv[3*i+2];
	}
    }
  else
    {
      for(int i=0; i<nvert; i++)
	{
	  vertData[stride*i + 6] = m_vcolor[3*i+0];
	  vertData[stride*i + 7] = m_vcolor[3*i+1];
	  vertData[stride*i + 8] = m_vcolor[3*i+2];
	}
    }


  unsigned int *indexData;
  indexData = new unsigned int[ni];
  for(int i=0; i<m_triangles.count(); i++)
    indexData[i] = m_triangles[i];
  //---------------------


  //--------------------
  if (m_material.count() > 0)
    {
      glGenTextures(m_material.count(), m_diffuseTex);
      for(int d=0; d<m_material.count(); d++)
	{
	  if (!m_material[d].isEmpty())
	    {
	      glActiveTexture(GL_TEXTURE0);
	      glBindTexture(GL_TEXTURE_2D, m_diffuseTex[d]);
	      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
	      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
	      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	      QImage img(m_material[d]);
	      glTexImage2D(GL_TEXTURE_2D,
			   0,
			   4,
			   img.width(),
			   img.height(),
			   0,
			   GL_BGRA,
			   GL_UNSIGNED_BYTE,
			   img.bits());
	    }
	  else if (m_uv.count() > 0)
	    {
	      glActiveTexture(GL_TEXTURE0);
	      glBindTexture(GL_TEXTURE_2D, m_diffuseTex[d]);
	      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
	      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
	      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	      QImage img(1,1,QImage::Format_ARGB32);
	      img.fill(Qt::lightGray);
	      glTexImage2D(GL_TEXTURE_2D,
			   0,
			   4,
			   img.width(),
			   img.height(),
			   0,
			   GL_BGRA,
			   GL_UNSIGNED_BYTE,
			   img.bits());
	    }
	}
    }
  //---------------------
  
  
  if (!m_glVertArray)
    glGenVertexArrays(1, &m_glVertArray);
  glBindVertexArray(m_glVertArray);
      
  // Populate a vertex buffer
  if (!m_glVertBuffer)
    glGenBuffers(1, &m_glVertBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, m_glVertBuffer);
  glBufferData(GL_ARRAY_BUFFER,
	       sizeof(float)*nv,
	       vertData,
	       GL_STATIC_DRAW);

  // Identify the components in the vertex buffer
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
			sizeof(float)*stride, // stride
			(void *)0); // starting offset

  if (stride > 3)
    {
      glEnableVertexAttribArray(1);
      glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
			    sizeof(float)*stride,
			    (char *)NULL + sizeof(float)*3);
    }
  
  if (stride > 6)
    {
      glEnableVertexAttribArray(2);
      glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 
			    sizeof(float)*stride,
			    (char *)NULL + sizeof(float)*6);
    }

  // Create and populate the index buffer
  if (!m_glIndexBuffer)
    glGenBuffers(1, &m_glIndexBuffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_glIndexBuffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
	       sizeof(unsigned int)*ni,
	       indexData,
	       GL_STATIC_DRAW);
  
  glBindVertexArray(0);

  delete [] vertData;
  delete [] indexData;

  // create shader  
  ShaderFactory::meshShader();
}

void
TrisetObject::drawTrisetBuffer(Camera *camera,
			       float pnear, float pfar,
			       bool active,
			       GLuint meshShader,
			       GLint *meshShaderParm,
			       bool oit)
{
  glUseProgram(meshShader);

  int ni = m_triangles.count();
  glBindVertexArray(m_glVertArray);
  glBindBuffer(GL_ARRAY_BUFFER, m_glVertBuffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_glIndexBuffer);  
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glEnableVertexAttribArray(2);

  // model-view-projection matrix
  GLfloat lxfrm[16];
  GLfloat mvp[16];
  GLdouble m[16];

  camera->getModelViewProjectionMatrix(m);

  double dmvp[16];
  Matrix::matmult(m_localXform, m, dmvp);
  for(int i=0; i<16; i++) mvp[i] = dmvp[i];
  
  for(int i=0; i<16; i++) lxfrm[i] = m_localXform[i];

 
  Vec vd = camera->viewDirection();
  

  glUniformMatrix4fv(meshShaderParm[0], 1, GL_FALSE, mvp);
  glUniformMatrix4fv(meshShaderParm[3], 1, GL_FALSE, lxfrm);

  // pack opacity and outline
  if (!oit)
    glUniform1f(meshShaderParm[4], m_opacity*110 + m_outline*10);

  glUniform3f(meshShaderParm[1], vd.x, vd.y, vd.z); // view direction
  glUniform1f(meshShaderParm[5], 1.0-m_roughness*0.1);
  glUniform1f(meshShaderParm[6], m_ambient);
  glUniform1f(meshShaderParm[7], m_diffuse);
  glUniform1f(meshShaderParm[8], m_specular);
  glUniform1f(meshShaderParm[15], m_featherSize);

  if (oit)
    glUniform1f(meshShaderParm[16], 1.0-m_opacity);
  
  
  for(int i=0; i<m_meshInfo.count(); i++)
    {
      QPolygon poly = m_meshInfo[i];
      int vStart = poly.point(0).x();
      int vEnd = poly.point(0).y();
      int idxStart = poly.point(1).x();
      int idxEnd = poly.point(1).y();
      int matIdx = poly.point(2).x();
      int nTri = idxEnd-idxStart+1;
      
      if (m_uv.count() > 0)
	{
	  glActiveTexture(GL_TEXTURE0);
	  glEnable(GL_TEXTURE_2D);
	  glBindTexture(GL_TEXTURE_2D, m_diffuseTex[matIdx]);
	  glUniform1i(meshShaderParm[13], 1); // hasVY
	  glUniform1i(meshShaderParm[14], 0); // diffuseTex
	}
      else
	glUniform1i(meshShaderParm[13], 0); // no diffuse texture
      
      glDrawElements(GL_TRIANGLES, nTri, GL_UNSIGNED_INT, (char *)NULL + idxStart*sizeof(unsigned int));
    }
  
  
  glDisable(GL_TEXTURE_2D);

  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
  glDisableVertexAttribArray(2);
  glBindVertexArray(0);
  
  glUseProgram(0);
}


void
TrisetObject::drawCaption(int cpid, QGLViewer *viewer)
{
  if (m_co[cpid]->text().isEmpty())
    return;


  float cpDx, cpDy;
  QPointF cpd = m_co[cpid]->position();
  cpDx = cpd.x();
  cpDy = cpd.y();
  
  
  int screenWidth = viewer->size().width();
  int screenHeight = viewer->size().height();

  Vec vc = m_captionPosition[cpid]+m_centroid+m_position;
  vc = Matrix::xformVec(m_brick0Xform, vc);
  Vec cppos = viewer->camera()->projectedCoordinatesOf(vc);
  int cx = cppos.x;
  int cy = cppos.y;
  
  cx *= (float)screenWidth/(float)viewer->camera()->screenWidth();
  cy *= (float)screenHeight/(float)viewer->camera()->screenHeight();

  QImage cimage = m_co[cpid]->image();
  QFontMetrics metric(m_co[cpid]->font());

  int wd = m_co[cpid]->width();
  int ht = m_co[cpid]->height();

  int x = cx;
  int y = cy - metric.descent();

  cimage = cimage.scaled(cimage.width()*viewer->camera()->screenWidth()/
			 screenWidth,
			 cimage.height()*viewer->camera()->screenHeight()/
			 screenHeight);


  QMatrix mat;
  mat.rotate(-m_co[cpid]->angle());
  QImage pimg = cimage.transformed(mat, Qt::SmoothTransformation);

  int px = x+(wd-pimg.width())/2;
  int py = y+(pimg.height()-ht)/2;
  if (px < 0 || py > screenHeight)
    {
      int pwd = pimg.width();
      int pht = pimg.height();
      int sx = 0;
      int sy = 0;
      if (px < 0)
	{
	  pwd = pimg.width()+px;
	  sx = -px;
	}
      if (py > screenHeight)
	{
	  pht = pimg.height()-(py-screenHeight);
	  sy = (py-screenHeight);
	}
      
      pimg = pimg.copy(sx, sy, pwd, pht);
    }

  int rpx, rpy;
  if (qAbs(cpDx) < 1.1 && qAbs(cpDy) < 1.1)
    {      
      rpx = cpDx*screenWidth+(wd-pimg.width())/2;
      rpy = cpDy*screenHeight+(pimg.height()-ht)/2;
    }
  else
    {
      rpx = cpDx+x+(wd-pimg.width())/2;
      rpy = cpDy+y+(pimg.height()-ht)/2;
    }
  
  if (qAbs(cpDx) < 1.1 && qAbs(cpDy) < 1.1 ||
      qAbs(cpDx) > 30 ||
      qAbs(cpDy) > 30)
    {
      float wo = pimg.width()/2;
      float ho = -pimg.height()/2;

      if (cy > rpy)
	ho = 0;
      else if (cy < rpy-pimg.height())
	ho = -pimg.height();
      else
	ho = pimg.height()*(float)(cy-rpy)/(float)(pimg.height());
      
      if (cx < rpx-50)
	wo = 0;
      else if (cx > rpx+pimg.width()+50)
	wo = pimg.width();
      else
	{
	  float frc = (float)(cx - rpx+50)/(float)(pimg.width()+100);
	  wo = frc*pimg.width();
	  if (cy > rpy)
	    ho = 0;
	  else
	    ho = -pimg.height();
	}

      
      wo *= (float)screenWidth/(float)viewer->camera()->screenWidth();
      ho *= (float)screenHeight/(float)viewer->camera()->screenHeight();

      QColor cclr = m_co[cpid]->color();
	
      float alpha;
      alpha = 0.5*cclr.alpha()/255.0;
      glLineWidth(4.0);
      glEnable(GL_LINE_SMOOTH);
      glBegin(GL_LINE_STRIP);
      glColor4f(alpha*cclr.redF(),
		alpha*cclr.greenF(),
		alpha*cclr.blueF(),
		alpha);
      glVertex2f(cx, cy);
      glVertex2f(rpx+wo, rpy+ho);
      glEnd();

      alpha = cclr.alpha()/255.0;
      glLineWidth(2.0);
      glEnable(GL_LINE_SMOOTH);
      glBegin(GL_LINE_STRIP);
      glColor4f(alpha*cclr.redF(),
		alpha*cclr.greenF(),
		alpha*cclr.blueF(),
		alpha);
      glVertex2f(cx, cy);
      glVertex2f(rpx+wo, rpy+ho);
      glEnd();

      glLineWidth(1.0);
      glDisable(GL_LINE_SMOOTH);
    }
  

//  wd = pimg.width();
//  ht = pimg.height();
//  wd *= (float)screenWidth/(float)viewer->camera()->screenWidth();
//  ht *= (float)screenHeight/(float)viewer->camera()->screenHeight();
//  glColor4f(0.2,0.2,0.2,0.2);
//  glBegin(GL_TRIANGLE_STRIP);
//  glVertex2f(rpx-5,    rpy+2);
//  glVertex2f(rpx+wd+5, rpy+2);
//  glVertex2f(rpx-5,    rpy-ht-2);
//  glVertex2f(rpx+wd+5, rpy-ht-2);
//  glEnd();


  glRasterPos2i(rpx, rpy);

  glDrawPixels(pimg.width(), pimg.height(),
	       GL_RGBA, GL_UNSIGNED_BYTE,
	       pimg.bits());
}

void
TrisetObject::postdraw(QGLViewer *viewer,
		       int x, int y,
		       bool active, int idx)
{
  if (!m_show)
    return;

  
  if (active)
  {
    Vec lineColor = Vec(1,1,1);
    
    glUseProgram(0);
    StaticFunctions::drawEnclosingCube(m_tenclosingBox, lineColor);


    Vec axisX =  m_tenclosingBox[1] - m_tenclosingBox[0];
    Vec axisY =  m_tenclosingBox[3] - m_tenclosingBox[0];
    Vec axisZ =  m_tenclosingBox[4] - m_tenclosingBox[0];    
    axisX *= 0.75;
    axisY *= 0.75;
    axisZ *= 0.75;
    StaticFunctions::drawAxis(m_tenclosingBox[0],
			      axisX, axisY, axisZ);
  }

  glDisable(GL_DEPTH_TEST);

  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // blend on top

  viewer->startScreenCoordinatesSystem();

  
  //------------------
  for (int i=0; i<m_co.count(); i++)
    drawCaption(i, viewer);
  //------------------

  
  if (active)
    {
      QString str = QString("mesh %1").arg(idx);
      str += QString(" (%1)").arg(QFileInfo(m_fileName).fileName());
      QFont font = QFont();
      QFontMetrics metric(font);
      int ht = metric.height();
      int wd = metric.width(str);
      x += 10;

      StaticFunctions::renderText(x+2, y, str, font, Qt::black, Qt::white);
      //Vec cepos = viewer->camera()->projectedCoordinatesOf(position()+centroid());
      //StaticFunctions::renderText(cepos.x-wd/2, cepos.y+ht/2, str, font, Qt::black, Qt::white);
    }
  
  glEnable(GL_DEPTH_TEST);

  viewer->stopScreenCoordinatesSystem();
}

void
TrisetObject::draw(Camera *camera,
		   bool active)
{
  glDisable(GL_LIGHTING);

  if (!m_show)
    return;
  
  GLuint meshShader = ShaderFactory::meshShader();
  GLint* meshShaderParm = ShaderFactory::meshShaderParm();
  
  drawTrisetBuffer(camera, 0, -1, active,
		   meshShader, meshShaderParm,
		   false);
}

void
TrisetObject::drawOIT(Camera *camera,
		      bool active)
{
  glDisable(GL_LIGHTING);

  if (!m_show)
    return;


  GLuint oitShader = ShaderFactory::oitShader();
  GLint* oitShaderParm = ShaderFactory::oitShaderParm();
  
  drawTrisetBuffer(camera, 0, -1, active,
		   oitShader, oitShaderParm,
		   true);
}

void
TrisetObject::genLocalXform()
{
  double *s0 = new double[16];
  double *s1 = new double[16];
  double *s2 = new double[16];

  Matrix::identity(s1);
  s1[3] = m_position.x;
  s1[7] = m_position.y;
  s1[11]= m_position.z;

  Matrix::identity(s0);
  s0[3] = m_centroid.x;
  s0[7] = m_centroid.y;
  s0[11]= m_centroid.z;
  Matrix::matmult(s1, s0, s2);

  Vec scale;
  scale = m_scale;
  
  Matrix::identity(s0);
  s0[0] = scale.x;
  s0[5] = scale.y;
  s0[10]= scale.z;
  Matrix::matmult(s2, s0, s1);

  Matrix::identity(s0);
  Matrix::matmult(s1, (double*)m_q.matrix(), s2);
  
  Matrix::identity(s0);
  s0[3] = -m_centroid.x;
  s0[7] = -m_centroid.y;
  s0[11]= -m_centroid.z;  
  Matrix::matmult(s2, s0, s1);

  for(int i=0; i<4; i++)
    for(int j=0; j<4; j++)
      {
	s0[j*4+i] = s1[4*i+j];
      }

  memcpy(m_localXform, s0, 16*sizeof(double));

  delete [] s0;
  delete [] s1;
  delete [] s2;
}

void
TrisetObject::predraw(QGLViewer *viewer,
		      bool active,
		      double *Xform)
{
  memcpy(&m_brick0Xform[0], Xform, 16*sizeof(double));
  
  double *s0 = new double[16];
  double *s1 = new double[16];
  double *s2 = new double[16];

  Matrix::identity(s1);
  s1[3] = m_position.x;
  s1[7] = m_position.y;
  s1[11]= m_position.z;

  Matrix::identity(s0);
  s0[3] = m_centroid.x;
  s0[7] = m_centroid.y;
  s0[11]= m_centroid.z;
  Matrix::matmult(s1, s0, s2);

  Vec scale;
  scale = (active ? m_scale*m_activeScale : m_scale);
  
  Matrix::identity(s0);
  s0[0] = scale.x;
  s0[5] = scale.y;
  s0[10]= scale.z;
  Matrix::matmult(s2, s0, s1);

  Matrix::identity(s0);
  Matrix::matmult(s1, (double*)m_q.matrix(), s2);
  
  Matrix::identity(s0);
  s0[3] = -m_centroid.x;
  s0[7] = -m_centroid.y;
  s0[11]= -m_centroid.z;  
  Matrix::matmult(s2, s0, s1);

  Matrix::matmult(Xform, s1, m_localXform);

  for(int i=0; i<8; i++)    
    m_tenclosingBox[i] = Matrix::xformVec(m_localXform, m_enclosingBox[i]);


  for(int i=0; i<4; i++)
    for(int j=0; j<4; j++)
      {
	s0[j*4+i] = m_localXform[4*i+j];
      }

  memcpy(m_localXform, s0, 16*sizeof(double));
  
  
  delete [] s0;
  delete [] s1;
  delete [] s2;
  
}


void
TrisetObject::copy2OrigVcolor()
{  
  m_OrigVcolor.clear();
  m_OrigVcolor.resize(m_vcolor.count()*3);
  for(int i=0; i<m_vcolor.count()/3; i++)
    {
      m_OrigVcolor[3*i+0] = m_vcolor[3*i+0];
      m_OrigVcolor[3*i+1] = m_vcolor[3*i+1];
      m_OrigVcolor[3*i+2] = m_vcolor[3*i+2];
    }
}

void
TrisetObject::makeReadyForPainting()
{
  if (!m_origColorBuffer)
    glGenBuffers(1, &m_origColorBuffer);

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_origColorBuffer);
  glBufferData(GL_SHADER_STORAGE_BUFFER,
	       m_OrigVcolor.count()*sizeof(float),
	       m_OrigVcolor.data(),
	       GL_STATIC_DRAW);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_origColorBuffer);
}

bool
TrisetObject::paint(Vec hitPt)
{
  if (!m_show)
    return false;

  Vec bmin = m_enclosingBox[0];
  Vec bmax = m_enclosingBox[6];

  if (hitPt.x <= bmin.x || hitPt.y <= bmin.y || hitPt.z <= bmin.z ||
      hitPt.x >= bmax.x || hitPt.y >= bmax.y || hitPt.z >= bmax.z)
    return false;



  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_glVertBuffer);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_origColorBuffer);

  glDispatchCompute(m_vcolor.count()/128, 1, 1);
    
  
  return true;
}


TrisetInformation
TrisetObject::get()
{
  TrisetInformation ti;
  ti.show = m_show;
  ti.clip = m_clip;
  ti.filename = m_fileName;
  ti.position = m_position;
  ti.scale = m_scale;
  ti.q = m_q;
  ti.color = m_color;
  ti.cropcolor = m_cropcolor;
  ti.roughness = m_roughness;
  ti.ambient = m_ambient;
  ti.diffuse = m_diffuse;
  ti.specular = m_specular;
  ti.reveal = m_reveal;
  ti.outline = m_outline;
  ti.glow = m_glow;
  ti.dark = m_dark;
  ti.pattern = m_pattern;
  ti.opacity = m_opacity;

  for(int i=0; i<m_captionPosition.count(); i++)
    {
      ti.captionText << m_co[i]->text();
      ti.captionFont << m_co[i]->font();
      ti.captionColor << m_co[i]->color();
      ti.captionPosition << m_captionPosition[i];
      QPointF cpd = m_co[i]->position();
      ti.cpDx << cpd.x();
      ti.cpDy << cpd.y();
    }

  return ti;
}

bool
TrisetObject::set(TrisetInformation ti)
{
  bool ok = false;

  if (m_fileName != ti.filename)
    ok = load(ti.filename);
  else
    ok = true;

  bool reloadColor = false;
  if (m_color != ti.color)
    {
      reloadColor = true;
    }
  
  m_show = ti.show;
  m_clip = ti.clip;
  m_position = ti.position;
  m_scale = ti.scale;
  m_q = ti.q;
  m_roughness = ti.roughness;
  m_color = ti.color;
  m_cropcolor = ti.cropcolor;
  m_ambient = ti.ambient;
  m_diffuse = ti.diffuse;
  m_specular = ti.specular;
  m_reveal = ti.reveal;
  m_outline = ti.outline;
  m_glow = ti.glow;
  m_dark = ti.dark;
  m_pattern = ti.pattern;
  m_opacity = ti.opacity;
      
  if (reloadColor)
    setColor(m_color);
  

  if (m_co.count() > 0)
    {      
      for(int i=0; i<m_co.count(); i++)
	delete m_co[i];
    }
  m_co.clear();

  m_captionPosition.clear();
  
  for(int i=0; i<ti.captionPosition.count(); i++)
    {
      m_captionPosition << ti.captionPosition[i];
      CaptionObject *co = new CaptionObject();
      co->setPosition(QPointF(ti.cpDx[i], ti.cpDy[i]));
      co->setText(ti.captionText[i]);
      co->setFont(ti.captionFont[i]);
      co->setColor(ti.captionColor[i]);
      co->setHaloColor(Qt::transparent);
      m_co << co;
    }
  
  return ok;
}

//---------------------------------
//---------------------------------
QFont
TrisetObject::captionFont()
{
  if (m_co.count() == 0)
    return QFont("MS Reference Sans Serif", 16);
  
  return m_co[m_co.count()-1]->font();
}
QFont
TrisetObject::captionFont(int cpid)
{
  return m_co[cpid]->font();
}
void
TrisetObject::setCaptionFont(QFont f)
{
  m_co[m_co.count()-1]->setFont(f);
}
void
TrisetObject::setCaptionFont(int cpid, QFont f)
{
  m_co[cpid]->setFont(f);
}
//---------------------------------
//---------------------------------
QColor
TrisetObject::captionColor()
{
  if (m_co.count() == 0)
    return Qt::gray;

  return m_co[m_co.count()-1]->color();
}
QColor
TrisetObject::captionColor(int cpid)
{
  return m_co[cpid]->color();
}
void
TrisetObject::setCaptionColor(QColor c)
{
  m_co[m_co.count()-1]->setColor(c);
  m_co[m_co.count()-1]->setHaloColor(Qt::transparent);
}
void
TrisetObject::setCaptionColor(int cpid, QColor c)
{
  m_co[cpid]->setColor(c);
  m_co[cpid]->setHaloColor(Qt::transparent);
}
//---------------------------------
//---------------------------------
QString
TrisetObject::captionText()
{
  if (m_co.count() == 0)
    return "";

  return m_co[m_co.count()-1]->text();
}
QString
TrisetObject::captionText(int cpid)
{
  return m_co[cpid]->text();
}
void TrisetObject::setCaptionText(QString t)
{ // used only to add a label
  if (t.isEmpty())
    return;

  //---
  // create a random offset for the label
  // between 0 and 0.1
  float m = QTime::currentTime().msec();
  float s = QTime::currentTime().second();
  m /= 10000.0;
  s /= 600.0;
  //---
    
  m_co << new CaptionObject();
  m_co[m_co.count()-1]->setPosition(QPointF(0.1+m, 0.1+s));
  m_captionPosition << Vec(0,0,0);
  
  m_co[m_co.count()-1]->setText(t);
}
void TrisetObject::setCaptionText(int cpid, QString t)
{ // used for changing or deleting the label
  if (t.isEmpty())
    {
      delete m_co[cpid];
      m_co.removeAt(cpid);
      m_captionPosition.removeAt(cpid);

      return;
    }

  m_co[cpid]->setText(t);
}
//---------------------------------
//---------------------------------
QList<Vec>
TrisetObject::captionPositions()
{
  return m_captionPosition;
}
Vec
TrisetObject::captionPosition()
{
  return m_captionPosition[0];
}
Vec
TrisetObject::captionPosition(int cpid)
{
  return m_captionPosition[cpid];
}
void
TrisetObject::setCaptionPosition(Vec v)
{  
  m_captionPosition[m_co.count()-1] = v - m_centroid - m_position;
}
void
TrisetObject::setCaptionPosition(int cpid, Vec v)
{
  m_captionPosition[cpid] = v - m_centroid - m_position;
}
//---------------------------------
//---------------------------------
QList<Vec>
TrisetObject::captionSizes()
{
  QList<Vec> sizes;
  for(int i=0; i<m_co.count(); i++)
    sizes << Vec(m_co[i]->width(), m_co[i]->height(), 0);
  return sizes;
}
Vec
TrisetObject::captionSize()
{
  if (m_co.count() == 0)
    return Vec(0,0,0);
    
  return Vec(m_co[m_co.count()-1]->width(), m_co[m_co.count()-1]->height(), 0);
}
//---------------------------------
//---------------------------------
QList<Vec>
TrisetObject::captionOffsets()
{
  QList<Vec> off;
  for(int i=0; i<m_co.count(); i++)
    {
      QPointF v = m_co[i]->position();
      off << Vec(v.x(), v.y(),0);
    }
  return off;
}
void
TrisetObject::setCaptionOffset(float dx, float dy)
{
  m_co[m_co.count()-1]->setPosition(QPointF(dx, dy));
}
void
TrisetObject::setCaptionOffset(int cpid, float dx, float dy)
{
  m_co[cpid]->setPosition(QPointF(dx, dy));
}
Vec
TrisetObject::captionOffset()
{
  if (m_co.count() == 0)
    return Vec(0.1,0.1,0);

  QPointF v = m_co[m_co.count()-1]->position();
  return Vec(v.x(), v.y(),0);
}
Vec
TrisetObject::captionOffset(int cpid)
{
  QPointF v = m_co[cpid]->position();
  return Vec(v.x(), v.y(),0);
}
//---------------------------------
//---------------------------------


void
TrisetObject::save()
{
  // if the mesh has been painted
  // get colors back from gpu
  if (m_origColorBuffer)
    {
      int nvert = m_vertices.count()/3;
      int nv = 9*nvert;
      glBindBuffer(GL_ARRAY_BUFFER, m_glVertBuffer);
      float *ptr = (float*)(glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY));

      for(int i=0; i<nvert; i++)
	{
	  m_vcolor[3*i+0] = ptr[9*i+6];
	  m_vcolor[3*i+1] = ptr[9*i+7];
	  m_vcolor[3*i+2] = ptr[9*i+8];	  
	  
	}
      glUnmapBuffer(GL_ARRAY_BUFFER);
    }
  

  // regenerate local transfromation so we don't have scaling due to surface being selected(active)
  genLocalXform();    
  // use the transpose for transformation
  double s[16];
  s[0] = m_localXform[0];
  s[1] = m_localXform[4];
  s[2] = m_localXform[8];
  s[3] = m_localXform[12];
  s[4] = m_localXform[1];
  s[5] = m_localXform[5];
  s[6] = m_localXform[9];
  s[7] = m_localXform[13];
  s[8] = m_localXform[2];
  s[9] = m_localXform[6];
  s[10] = m_localXform[10];
  s[11] = m_localXform[14];
  s[12] = m_localXform[3];
  s[13] = m_localXform[7];
  s[14] = m_localXform[11];
  s[15] = m_localXform[15];

  QString prevDir = Global::previousDirectory();

  return StaticFunctions::savePLY(m_vertices,
				  m_normals,
				  m_vcolor,
				  m_triangles,
				  &s[0],
				  prevDir);
}

bool
TrisetObject::loadAssimpModel(QString flnm)
{
  MainWindowUI::mainWindowUI()->statusBar->showMessage(flnm);
  Global::progressBar()->show();
  qApp->processEvents();

  m_position = Vec(0,0,0);
  m_scale = Vec(1,1,1);

  Global::progressBar()->setValue(10);
  qApp->processEvents();

  Assimp::Importer importer;
  const aiScene* scene = importer.ReadFile( flnm.toLatin1().data(), 
					    aiProcess_Triangulate            |
					    aiProcess_GenSmoothNormals       |
					    aiProcess_JoinIdenticalVertices  |
					    aiProcess_SortByPType);

  if(!scene)
    {
      QMessageBox::information(0, "Error Importing Asset",
			       QString("Couldn't load model %1").arg(flnm));
      return false;
    }
  
  m_vertices.clear();
  m_vcolor.clear();
  m_normals.clear();
  m_triangles.clear();
  m_uv.clear();
  m_meshInfo.clear();
  m_material.clear();
  
  //QMessageBox::information(0, "", QString("%1").arg(scene->mNumMeshes));
  
  int nvert = 0;
  for(int i=0; i<scene->mNumMeshes; i++)
    {
      Global::progressBar()->setValue((int)(100.0*(float)(i)/(float)(scene->mNumMeshes)));
      qApp->processEvents();
  
      int vStart = m_vertices.count()/3;
      int iStart = m_triangles.count();
      
      aiMesh* mesh = scene->mMeshes[i];
      bool hasVertexColors = mesh->HasVertexColors(0);
      bool hasUV = mesh->HasTextureCoords(0);

      for(int j=0; j<mesh->mNumVertices; j++)
	{	  
	  aiVector3D pos = mesh->mVertices[j];
	  m_vertices << pos.x << pos.y << pos.z;
      
	  aiVector3D normal = mesh->mNormals[j];
	  Vec vn = Vec(normal.x, normal.y, normal.z).unit();
	  m_normals << vn.x << vn.y << vn.z;
	  
	  if (hasVertexColors)
	    m_vcolor << mesh->mColors[0][j].r
		     << mesh->mColors[0][j].g
		     << mesh->mColors[0][j].b;

	  if (hasUV)
	    {
	      m_uv << mesh->mTextureCoords[0][j].x
		   << mesh->mTextureCoords[0][j].y
		   << mesh->mTextureCoords[0][j].z;
	    }
	} // verticeas

      for(int j=0; j<mesh->mNumFaces; j++)
	{
	  const aiFace& face = mesh->mFaces[j];
	  for(int k=0; k<face.mNumIndices; k++)
	    m_triangles << nvert+face.mIndices[k];
	} // faces

      QPolygon poly;
      poly << QPoint(vStart, m_vertices.count()/3);
      poly << QPoint(iStart, m_triangles.count());
      poly << QPoint(mesh->mMaterialIndex, 0);
      m_meshInfo << poly;
      
      nvert += mesh->mNumVertices;
    }
  
  //--------------
  if (scene->mNumMaterials > 0)
    {
      bool texFound = false;
      for(int m=0; m<scene->mNumMaterials; m++)
	{
	  Global::progressBar()->setValue((int)(100.0*(float)(m)/(float)(scene->mNumMaterials)));
	  qApp->processEvents();
  
	  aiMaterial *mat = scene->mMaterials[m];
	  aiString path;
	  QString diffuseTexFile;
	  if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &path) == AI_SUCCESS)
	    {
	      diffuseTexFile = QFileInfo(QFileInfo(flnm).dir(),
					 QString(path.data)).absoluteFilePath();
	      //QMessageBox::information(0, "", diffuseTexFile);
	      //break;
	      if (!QFile::exists(diffuseTexFile))
		{
		  QString mesg;
		  if (diffuseTexFile.isEmpty())
		    mesg = QString("Diffuse texture not found for %1").arg(flnm);
		  else
		    mesg = "Cannot locate " + diffuseTexFile;
		  
		  QMessageBox::information(0, "Texture Error", mesg);

		  QString imgFile = QFileDialog::getOpenFileName(0,
								 QString("Load diffuse texture"),
								 QFileInfo(flnm).dir().path(),
								 "Image Files (*.png *.tif *.bmp *.jpg *.gif)");
		  
		  QFileInfo f(imgFile);

		  if (f.exists() == true)
		    diffuseTexFile = imgFile;
		  else
		    diffuseTexFile = QString();
		}	    
	      m_material << diffuseTexFile;
	      texFound = texFound || (!diffuseTexFile.isEmpty());
	    }
	  else
	    m_material << QString();
	}
      
      if (!texFound)
	{
	  if (m_uv.count() > 0)
	    {
	      QString mesg;
	      mesg = QString("Diffuse texture not found for %1").arg(flnm);
	      QMessageBox::information(0, "Texture Error", mesg);

	      QString imgFile = QFileDialog::getOpenFileName(0,
							     QString("Load diffuse texture"),
							     QFileInfo(flnm).dir().path(),
							     "Image Files (*.png *.tif *.bmp *.jpg *.gif)");
	      
	      QFileInfo f(imgFile);
	      
	      if (f.exists() == true)
		{
		  for (int m=0; m<m_material.count(); m++)
		    m_material[m] = imgFile;
		}
	      else
		m_uv.clear();
	    }	  
	}
	  
//      if (m_uv.count() > 0 &&
//	  !QFile::exists(m_diffuseTexFile))
//	{
//	  QString mesg;
//	  if (m_diffuseTexFile.isEmpty())
//	    mesg = "Diffuse texture not found.";
//	  else
//	    mesg = "Cannot locate " + m_diffuseTexFile;
//	  QMessageBox::information(0, "Texture Error", mesg);
//	  QString imgFile = QFileDialog::getOpenFileName(0,
//							 QString("Load diffuse texture"),
//							 QFileInfo(flnm).dir().path(),
//							 "Image Files (*.png *.tif *.bmp *.jpg *.gif)");
//	  
//	  QFileInfo f(imgFile);
//	  if (f.exists() == true)
//	    m_diffuseTexFile = imgFile;
//	  else
//	    m_uv.clear();
//	}
    }
  //--------------


//  //--------------
//  {
//    QString mesg;
//    for(int i=0; i<m_meshInfo.count(); i++)
//      {
//	QPolygon poly = m_meshInfo[i];
//	mesg += QString("V : %1 %2       I : %3 %4     M : %5\n").\
//	  arg(poly.point(0).x()).arg(poly.point(0).y()).
//	  arg(poly.point(1).x()).arg(poly.point(1).y()).
//	  arg(poly.point(2).x());
//      }
//    for(int i=0; i<m_material.count(); i++)
//      mesg += QString("\nM : %1").arg(m_material[i]);
//    
//    QMessageBox::information(0, "", mesg);
//  }
//  //--------------

  Global::progressBar()->setValue(90);
  qApp->processEvents();
  

  float minX, maxX;
  float minY, maxY;
  float minZ, maxZ;
  minX = maxX = m_vertices[0];
  minY = maxY = m_vertices[1];
  minZ = maxZ = m_vertices[2];
  m_centroid = Vec(0,0,0);
  for(int i=0; i<m_vertices.count()/3; i++)
    {
      m_centroid += Vec(m_vertices[3*i+0],m_vertices[3*i+1],m_vertices[3*i+2]);

      minX = qMin(minX, m_vertices[3*i+0]);
      maxX = qMax(maxX, m_vertices[3*i+0]);
      minY = qMin(minY, m_vertices[3*i+1]);
      maxY = qMax(maxY, m_vertices[3*i+1]);
      minZ = qMin(minZ, m_vertices[3*i+2]);
      maxZ = qMax(maxZ, m_vertices[3*i+2]);
    }

  m_centroid /= (m_vertices.count()/3);

//  //-----------------------------------
//  // choose vertex closest to center as centroid
//  int cp = 0;
//  float cdst = (m_vertices[0]-m_centroid).squaredNorm();
//  for(int i=1; i<m_vertices.count(); i++)
//    {
//      float dst = (m_vertices[i]-m_centroid).squaredNorm();
//      if (dst < cdst)
//	{
//	  cp = i;
//	  cdst = dst;
//	}
//    }
//  m_centroid = m_vertices[cp];
//  //-----------------------------------

  //-----------------------------------
  // take 10 sample points for grab checking
  m_samplePoints.clear();
  m_samplePoints << m_centroid;
//  if (m_vertices.count() > 50)
//    {
//      int stp = 0.2*m_vertices.count();
//      for(int i=0; i<m_vertices.count(); i+=stp)
//	m_samplePoints << m_vertices[i];
//    }  
  //-----------------------------------

  Global::progressBar()->setValue(95);
  qApp->processEvents();

  
  Vec bmin = Vec(minX, minY, minZ);
  Vec bmax = Vec(maxX, maxY, maxZ);

//  QMessageBox::information(0, "", QString("%1 : %2\n %3 %4 %5\n %6 %7 %8").\
//			   arg(m_vertices.count()).\
//			   arg(m_triangles.count()).\
//			   arg(bmin.x).arg(bmin.y).arg(bmin.z).\
//			   arg(bmax.x).arg(bmax.y).arg(bmax.z));
  
  minX = floor(minX);
  minY = floor(minY);
  minZ = floor(minZ);
  maxX = ceil(maxX);
  maxY = ceil(maxY);
  maxZ = ceil(maxZ);
  m_nZ = maxX-minX+1;
  m_nY = maxY-minY+1;
  m_nX = maxZ-minZ+1;
  
  m_enclosingBox[0] = Vec(bmin.x, bmin.y, bmin.z);
  m_enclosingBox[1] = Vec(bmax.x, bmin.y, bmin.z);
  m_enclosingBox[2] = Vec(bmax.x, bmax.y, bmin.z);
  m_enclosingBox[3] = Vec(bmin.x, bmax.y, bmin.z);
  m_enclosingBox[4] = Vec(bmin.x, bmin.y, bmax.z);
  m_enclosingBox[5] = Vec(bmax.x, bmin.y, bmax.z);
  m_enclosingBox[6] = Vec(bmax.x, bmax.y, bmax.z);
  m_enclosingBox[7] = Vec(bmin.x, bmax.y, bmax.z);

  //m_centroid = (bmin + bmax)/2;

  m_position = Vec(0,0,0);
  m_scale = Vec(1,1,1);

  m_fileName = flnm;

  if (m_vcolor.count() == 0)
    {
      m_color = Vec(0.9,0.9,0.9);
      //m_vcolor.fill(m_color, m_vertices.count());
      m_vcolor.fill(0.9f, m_vertices.count()); // since all components are the same
    }
  
  
  Global::progressBar()->setValue(100);
  Global::progressBar()->hide();
  qApp->processEvents();


  MainWindowUI::mainWindowUI()->statusBar->showMessage("");
  return true;
}
