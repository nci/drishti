#include "global.h"
#include "shaderfactory.h"
#include "staticfunctions.h"
#include "trisetobject.h"
#include "matrix.h"
#include "ply.h"
#include "matrix.h"
#include "mainwindowui.h"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

#include <QFileDialog>
#include <QTime>
#include <QInputDialog>


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
  m_origColorBuffer = 0;
  
  m_diffuseMat.clear();
  m_normalMat.clear();

  m_co.clear();

  m_dialog = 0;
  
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

Vec
TrisetObject::tcentroid()
{
  m_tcentroid = (m_tenclosingBox[0] + m_tenclosingBox[6])*0.5;
  return m_tcentroid;
}

void
TrisetObject::tenclosingBox(Vec &boxMin,
			    Vec &boxMax)
{
  boxMin = m_tenclosingBox[0];
  boxMax = m_tenclosingBox[6];
}

void
TrisetObject::enclosingBox(Vec &boxMin,
			   Vec &boxMax)
{
  boxMin = m_enclosingBox[0];
  boxMax = m_enclosingBox[6];
}

void
TrisetObject::clear()
{
  if (m_dialog)
    {
      m_dialog->close();
      delete m_dialog;
      m_dialog = 0;
    }

  m_show = true;
  m_clip = true;
  m_clearView = false;
  m_activeScale = 1.0;
  
  m_interactive_mode = 0;
  m_interactive_axis = 0;
  
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
  m_materialId = 0;
  m_materialMix = 0.5;
  m_cropcolor = Vec(0.0f,0.0f,0.0f);
  m_roughness = 2;
  m_specular = 1.0f;
  m_diffuse = 1.0f;
  m_ambient = 0.0f;
  m_opacity = 0.7;
  m_vertices.clear();
  m_normals.clear();
  m_triangles.clear();
  m_vcolor.clear();
  m_OrigVcolor.clear();
  m_uv.clear();
  m_samplePoints.clear();
  
  if(m_glVertArray)
    {
      glDeleteVertexArrays( 1, &m_glVertArray );
      glDeleteBuffers(1, &m_glVertBuffer);
      glDeleteBuffers(1, &m_glIndexBuffer);
      m_glIndexBuffer = 0;
      m_glVertArray = 0;
      m_glVertBuffer = 0;
    }
  if (m_diffuseMat.count() > 0)
    {
      glDeleteTextures(m_diffuseMat.count(), m_diffuseTex);
      for(int d=0; d<m_diffuseMat.count(); d++)
	m_diffuseTex[d] = 0;
    }
  m_diffuseMat.clear();

  if (m_normalMat.count() > 0)
    {
      glDeleteTextures(m_normalMat.count(), m_normalTex);
      for(int d=0; d<m_normalMat.count(); d++)
	m_normalTex[d] = 0;
    }
  m_normalMat.clear();

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

  // used during painting
  if (m_origColorBuffer)
    {
      glDeleteBuffers(1, &m_origColorBuffer);
      m_origColorBuffer = 0;
    }
}

void
TrisetObject::setColor(Vec color, bool ignoreBlack)
{  
  if (m_uv.count() > 0)
    {
      QMessageBox::information(0, "", "Cannot change color for textured objects");
      return;
    }
  
  m_color = color;

  for(int i=0; i<m_vcolor.count()/3; i++)
    {
      if (ignoreBlack ||
	  m_vcolor[3*i+0]+m_vcolor[3*i+1]+m_vcolor[3*i+2] > 0.0001)
	{
	  m_vcolor[3*i+0] = m_color.x;
	  m_vcolor[3*i+1] = m_color.y;
	  m_vcolor[3*i+2] = m_color.z;
	}
    }

  copyToOrigVcolor();

  loadVertexBufferData();
}

bool
TrisetObject::haveBlack()
{
  for(int i=0; i<m_vcolor.count()/3; i++)
    {
      if (m_vcolor[3*i+0]+m_vcolor[3*i+1]+m_vcolor[3*i+2] < 0.0001)
	return true;
    }
  return false;
}

void
TrisetObject::bakeColors()
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
  if (!QFile::exists(flnm))
    {
      QMessageBox::information(0, "", QString("%1 not found").arg(flnm));
      return false;
    }
    
  bool loaded;
  loaded = loadAssimpModel(flnm);
  

  m_featherSize = 0.005*(m_enclosingBox[6]-m_enclosingBox[0]).norm();

  
  if (loaded)
    {
      copyToOrigVcolor();
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

  if (m_tangents.count() > 0)
    stride += 3;
      
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

  if (m_tangents.count() > 0)
    {
      for(int i=0; i<nvert; i++)
	{
	  vertData[stride*i + 9]  = m_tangents[3*i+0];
	  vertData[stride*i + 10] = m_tangents[3*i+1];
	  vertData[stride*i + 11] = m_tangents[3*i+2];
	}
    }


  unsigned int *indexData;
  indexData = new unsigned int[ni];
  for(int i=0; i<ni; i++)
    indexData[i] = m_triangles[i];
  //---------------------


  //--------------------
  if (m_diffuseMat.count() > 0)
    {
      glGenTextures(m_diffuseMat.count(), m_diffuseTex);
      for(int d=0; d<m_diffuseMat.count(); d++)
	{
	  if (!m_diffuseMat[d].isEmpty())
	    {
	      glActiveTexture(GL_TEXTURE1);
	      glBindTexture(GL_TEXTURE_2D, m_diffuseTex[d]);
	      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
	      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
	      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	      QImage img(m_diffuseMat[d]);
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
	      glActiveTexture(GL_TEXTURE1);
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
  

    //--------------------
  if (m_normalMat.count() > 0)
    {
      glGenTextures(m_normalMat.count(), m_normalTex);
      for(int d=0; d<m_normalMat.count(); d++)
	{
	  if (!m_normalMat[d].isEmpty())
	    {
	      glActiveTexture(GL_TEXTURE5);
	      glBindTexture(GL_TEXTURE_2D, m_normalTex[d]);
	      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
	      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
	      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	      QImage img(m_normalMat[d]);
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

  int vpSz = 3;
  if (m_tangents.count() > 0)
    vpSz = 4;

  // Identify the components in the vertex buffer
  for (int vp=0; vp<vpSz; vp++)
    {
      glEnableVertexAttribArray(vp);
      if (vp > 0)
	glVertexAttribPointer(vp, vpSz, GL_FLOAT, GL_FALSE,
			      sizeof(float)*stride, // stride
			      (void *)(0 + (sizeof(float)*(3*vp)))); // starting offset
      else
	glVertexAttribPointer(vp, vpSz, GL_FLOAT, GL_FALSE,
			      sizeof(float)*stride, // stride
			      (void *)NULL); // starting offset
    }
  
//  // Identify the components in the vertex buffer
//  glEnableVertexAttribArray(0);
//  glVertexAttribPointer(0, vapSz, GL_FLOAT, GL_FALSE,
//			sizeof(float)*stride, // stride
//			(void *)0); // starting offset
//
//  if (stride > 3) // normals
//    {
//      glEnableVertexAttribArray(1);
//      glVertexAttribPointer(1, vapSz, GL_FLOAT, GL_FALSE,
//			    sizeof(float)*stride,
//			    (char *)NULL + sizeof(float)*3);
//    }
//  
//  if (stride > 6) // uv/color
//    {
//      glEnableVertexAttribArray(2);
//      glVertexAttribPointer(2, vapSz, GL_FLOAT, GL_FALSE, 
//			    sizeof(float)*stride,
//			    (char *)NULL + sizeof(float)*6);
//    }
//
//  if (stride > 9) // tangents
//    {
//      glEnableVertexAttribArray(3);
//      glVertexAttribPointer(3, vapSz, GL_FLOAT, GL_FALSE, 
//			    sizeof(float)*stride,
//			    (char *)NULL + sizeof(float)*9);
//    }

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
TrisetObject::drawTrisetBuffer(GLdouble *MVP,
			       Vec vd,
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

  if (m_tangents.count()>0)
    glEnableVertexAttribArray(3);

  // model-view-projection matrix
  GLfloat lxfrm[16];
  GLfloat mvp[16];


  double dmvp[16];
  Matrix::matmult(m_localXform, MVP, dmvp);
  for(int i=0; i<16; i++) mvp[i] = dmvp[i];
  
  for(int i=0; i<16; i++) lxfrm[i] = m_localXform[i];

   

  glUniformMatrix4fv(meshShaderParm[0], 1, GL_FALSE, mvp);
  glUniformMatrix4fv(meshShaderParm[3], 1, GL_FALSE, lxfrm);

			   
  // pack opacity and outline
  if (!oit)
    //offset outline to avoid aliasing
    glUniform1f(meshShaderParm[4], (int)(m_opacity*110) + qFloor(m_outline*10));	

  
  glUniform1f(meshShaderParm[5], m_roughness*0.1);
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
      //int nTri = idxEnd-idxStart+1;
      int nTri = idxEnd-idxStart;
      
      if (m_uv.count() > 0)
	{
	  glActiveTexture(GL_TEXTURE1);
	  glEnable(GL_TEXTURE_2D);
	  glBindTexture(GL_TEXTURE_2D, m_diffuseTex[matIdx]);
	  glUniform1i(meshShaderParm[13], 1); // hasUY
	  glUniform1i(meshShaderParm[14], 1); // diffuseTex

	  if (m_normalMat.count() > 0 && !m_normalMat[matIdx].isEmpty())
	    {
	      glActiveTexture(GL_TEXTURE5);
	      glEnable(GL_TEXTURE_2D);
	      glBindTexture(GL_TEXTURE_2D, m_normalTex[matIdx]);
	      glUniform1i(meshShaderParm[26], 1); // hasNormalTex
	      glUniform1i(meshShaderParm[27], 5); // normalTex
	    }
	  else
	    glUniform1i(meshShaderParm[26], 0); // hasNormalTex
	}
      else
	glUniform1i(meshShaderParm[13], 0); // no diffuse texture

      
      glDrawElements(GL_TRIANGLES, nTri, GL_UNSIGNED_INT, (char *)NULL + idxStart*sizeof(unsigned int));

      if (m_uv.count() > 0)
	{
	  glActiveTexture(GL_TEXTURE1);
	  glDisable(GL_TEXTURE_2D);
	  if (m_normalMat.count() > 0 && !m_normalMat[matIdx].isEmpty())
	    {
	      glActiveTexture(GL_TEXTURE5);
	      glDisable(GL_TEXTURE_2D);
	    }
	}  
    }  

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


      float wfrc = -1;
      float hfrc = 1;
	
      if (cy > rpy)
	ho = 0;	  
      else if (cy < rpy-pimg.height())
	ho = -pimg.height();
      else
	{
	  hfrc = (float)(cy-rpy)/(float)(pimg.height());
	  ho = hfrc*pimg.height();
	}
      
      if (cx < rpx-50)
	wo = 0;
      else if (cx > rpx+pimg.width()+50)
	wo = pimg.width();
      else
	{
	  wfrc = (float)(cx - rpx+50)/(float)(pimg.width()+100);
	  wo = wfrc*pimg.width();
	  if (cy > rpy)
	    {
	      hfrc = 1;
	      ho = 0;
	    }
	  else
	    {
	      hfrc = 1;
	      ho = -pimg.height();
	    }
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

//--------------------
//  corner lines for captions
//      {
//	if (hfrc < 1 || wfrc < 0)
//	  {
//	    glBegin(GL_LINE_STRIP);
//	    glVertex2f(rpx+wo, rpy);
//	    glVertex2f(rpx+wo, rpy-pimg.height());
//	    glEnd();
//	  }
//	if (wfrc > -1 || hfrc > 0)
//	  {
//	    glBegin(GL_LINE_STRIP);
//	    glVertex2f(rpx, rpy+ho);
//	    glVertex2f(rpx+pimg.width(), rpy+ho);
//	    glEnd();
//	  }
//      }
//--------------------
      
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
TrisetObject::getAxes(Vec& axisX, Vec& axisY, Vec& axisZ)
{
  axisX =  (m_tenclosingBox[1] - m_tenclosingBox[0])*0.5;
  axisY =  (m_tenclosingBox[3] - m_tenclosingBox[0])*0.5;
  axisZ =  (m_tenclosingBox[4] - m_tenclosingBox[0])*0.5;    
 }


void
TrisetObject::postdrawInteractionWidget(QGLViewer *viewer)
{
    Vec axisX, axisY, axisZ;
    getAxes(axisX, axisY, axisZ);

    //Vec c = tcentroid();
    Vec c = m_centroid + m_position;
    Vec scrC = viewer->camera()->projectedCoordinatesOf(c);
    Vec scrX = viewer->camera()->projectedCoordinatesOf(axisX);
    Vec scrY = viewer->camera()->projectedCoordinatesOf(axisY);
    Vec scrZ = viewer->camera()->projectedCoordinatesOf(axisZ);

    Vec vX = viewer->camera()->unprojectedCoordinatesOf(scrC + Vec(100,0,0));
    float rad = (vX - c).norm();

    Vec uX = axisX.unit();
    Vec uY = axisY.unit();
    Vec uZ = axisZ.unit();

    Vec sC = scrC;
    Vec sX = viewer->camera()->projectedCoordinatesOf(c + rad*uX);
    Vec sY = viewer->camera()->projectedCoordinatesOf(c + rad*uY);
    Vec sZ = viewer->camera()->projectedCoordinatesOf(c + rad*uZ);

    viewer->startScreenCoordinatesSystem();    

    if (m_interactive_axis == 0 || m_interactive_axis == 1)
    {
      // draw x-axis gizmo
      glColor4f(0.7f, 0.35f, 0.35f, 0.9f);
      glBegin(GL_LINES);
      glVertex2f(sC.x, sC.y);
      glVertex2f(sX.x, sX.y);
      glEnd();
      
      // x-rotation gizmo
      glColor4f(0.9f, 0.5f, 0.5f, 0.9f);
      if (m_interactive_mode == 2 &&
	  m_interactive_axis == 1)
	{
	  glColor4f(0.7f, 0.0f, 0.0f, 0.7f);
	  glBegin(GL_POLYGON);
	}
      else
	glBegin(GL_LINE_STRIP);      
      Vec v = rad*uY;
      Quaternion q(uX, qDegreesToRadians(10.0));
      Vec u = viewer->camera()->projectedCoordinatesOf(c + v);
      glVertex2f(u.x, u.y);
      for(int i=0; i<36; i++)
	{
	  v = q.rotate(v);
	  Vec u = viewer->camera()->projectedCoordinatesOf(c + v);
	  glVertex2f(u.x, u.y);
	}
      glEnd();
    }

    if (m_interactive_axis == 0 || m_interactive_axis == 2)
    {
      // draw y-axis gizmo
      glColor4f(0.35f, 0.7f, 0.35f, 0.9f);
      glBegin(GL_LINES);
      glVertex2f(sC.x, sC.y);
      glVertex2f(sY.x, sY.y);
      glEnd();
      
      // y-rotation gizmo
      glColor4f(0.5f, 0.9f, 0.5f, 0.9f);
      if (m_interactive_mode == 2 &&
	  m_interactive_axis == 2)
	{
	  glColor4f(0.0f, 0.7f, 0.0f, 0.7f);
	  glBegin(GL_POLYGON);
	}
      else
      glBegin(GL_LINE_STRIP);
      Vec v = rad*uZ;
      Quaternion q(uY, qDegreesToRadians(10.0));
      Vec u = viewer->camera()->projectedCoordinatesOf(c + v);
      glVertex2f(u.x, u.y);
      for(int i=0; i<36; i++)
	{
	  v = q.rotate(v);
	  Vec u = viewer->camera()->projectedCoordinatesOf(c + v);
	  glVertex2f(u.x, u.y);
	}
      glEnd();
    }

    if (m_interactive_axis == 0 || m_interactive_axis == 3)
    {
      // draw z-axis gizmo
      glColor4f(0.35f, 0.35f, 0.7f, 0.9f);
      glBegin(GL_LINES);
      glVertex2f(sC.x, sC.y);
      glVertex2f(sZ.x, sZ.y);
      glEnd();
      
      // z-rotation gizmo
      glColor4f(0.5f, 0.5f, 0.9f, 0.9f);
      if (m_interactive_mode == 2 &&
	  m_interactive_axis == 3)
	{
	  glColor4f(0.0f, 0.0f, 0.7f, 0.7f);
	  glBegin(GL_POLYGON);
	}
      else
	glBegin(GL_LINE_STRIP);
      Vec v = rad*uX;
      Quaternion q(uZ, qDegreesToRadians(10.0));
      Vec u = viewer->camera()->projectedCoordinatesOf(c + v);
      glVertex2f(u.x, u.y);
      for(int i=0; i<36; i++)
	{
	  v = q.rotate(v);
	  Vec u = viewer->camera()->projectedCoordinatesOf(c + v);
	  glVertex2f(u.x, u.y);
	}
      glEnd();
    }

    if (m_interactive_mode != 2) // rotation mode
      {
	sX -= sC;
	sY -= sC;
	sZ -= sC;

	Vec t0, t1, t2, t3;
    
	// draw faces gizmo
	t0 = sC + sX*0.4 + sY*0.4;
	t1 = sC + sX*0.4 + sY*0.6;
	t2 = sC + sX*0.6 + sY*0.6;
	t3 = sC + sX*0.6 + sY*0.4;
	glColor4f(0.9f, 0.9f, 0.1f, 0.9f);
	glBegin(GL_QUADS);
	glVertex2f(t0.x, t0.y);
	glVertex2f(t1.x, t1.y);
	glVertex2f(t2.x, t2.y);
	glVertex2f(t3.x, t3.y);
	glEnd();
	
	t0 = sC + sX*0.4 + sZ*0.4;
	t1 = sC + sX*0.4 + sZ*0.6;
	t2 = sC + sX*0.6 + sZ*0.6;
	t3 = sC + sX*0.6 + sZ*0.4;
	glColor4f(0.9f, 0.1f, 0.9f, 0.9f);
	glBegin(GL_QUADS);
	glVertex2f(t0.x, t0.y);
	glVertex2f(t1.x, t1.y);
	glVertex2f(t2.x, t2.y);
	glVertex2f(t3.x, t3.y);
	glEnd();
	
	t0 = sC + sY*0.4 + sZ*0.4;
	t1 = sC + sY*0.4 + sZ*0.6;
	t2 = sC + sY*0.6 + sZ*0.6;
	t3 = sC + sY*0.6 + sZ*0.4;
	glColor4f(0.1f, 0.9f, 0.9f, 0.9f);
	glBegin(GL_QUADS);
	glVertex2f(t0.x, t0.y);
	glVertex2f(t1.x, t1.y);
	glVertex2f(t2.x, t2.y);
	glVertex2f(t3.x, t3.y);
	glEnd();
      }
    

    viewer->stopScreenCoordinatesSystem();
    //-------------------
}

void
TrisetObject::postdraw(QGLViewer *viewer,
		       int x, int y,
		       bool grabMode, bool displayName,
		       int active, int idx, int moveAxis)
{
  if (!m_show)
    return;

  
  if (active)
  {
    Vec lineColor = Vec(1,1,1);

    if (!displayName)
      glLineWidth(1);
    else
      glLineWidth(5);
    
    //glLineWidth(1);

    glUseProgram(0);
    StaticFunctions::drawEnclosingCube(m_tenclosingBox, lineColor);          

    if (grabMode)
      {
	glLineWidth(5);
	postdrawInteractionWidget(viewer);
      }
  }

  glDisable(GL_DEPTH_TEST);

  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // blend on top

  viewer->startScreenCoordinatesSystem();

  //------------------
//  if (grabMode)
//  {
//    glBlendFunc(GL_ONE, GL_ONE);
//    // draw centroid
//    Vec vc = (m_tenclosingBox[0] + m_tenclosingBox[6])*0.5;
//    Vec cppos = viewer->camera()->projectedCoordinatesOf(vc);
//    glEnable(GL_POINT_SPRITE);
//    glActiveTexture(GL_TEXTURE0);
//    glEnable(GL_TEXTURE_2D);
//    glBindTexture(GL_TEXTURE_2D, Global::hollowSpriteTexture());
//    glTexEnvf( GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE );
//    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
//    glEnable(GL_POINT_SMOOTH);    
//    glColor3f(1,1,1);
//    glPointSize(20);
//    glBegin(GL_POINTS);
//    glVertex3fv(cppos);
//    glEnd();
//    glPointSize(1);
//    glDisable(GL_POINT_SPRITE);
//    glActiveTexture(GL_TEXTURE0);
//    glDisable(GL_TEXTURE_2D);  
//    glDisable(GL_POINT_SMOOTH);
//
////    QString str = QString("%1").arg(idx);
////    QFont font = QFont("MS Reference Sans Serif", 10);
////    QFontMetrics metric(font);
////    int ht = metric.height();
////    int wd = metric.width(str);
////    
////    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // blend on top
////    StaticFunctions::renderText(cppos.x-wd/2, cppos.y+30, str, font, QColor("darkslategray"), Qt::white);
//  }
  //------------------
  
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // blend on top
  //------------------
  for (int i=0; i<m_co.count(); i++)
    drawCaption(i, viewer);
  //------------------

  if (displayName)
    {
      QString str = QString("%1").arg(idx);

//      str += QString(" (  %1 %2  )  ").				\
//	arg(m_interactive_mode).arg(m_interactive_axis);
	
      str += QString(" (%1)").arg(QFileInfo(m_fileName).fileName());
      //QFont font = QFont();
      QFont font = QFont("MS Reference Sans Serif", 10);
      QFontMetrics metric(font);
      int ht = metric.height();
      int wd = metric.width(str);

      StaticFunctions::renderText(x-wd/2, y+40, str, font, QColor("darkslategray"), Qt::white);
    }
  
  glEnable(GL_DEPTH_TEST);

  viewer->stopScreenCoordinatesSystem();
}

void
TrisetObject::drawHitPoints()
{
  if (m_hitpoints.count() == 0)
    return;

  // offset to draw coplanar points
  glDepthRange(0.0, 0.99);
  
  glEnable(GL_POINT_SPRITE);
  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, Global::spriteTexture());
  glTexEnvf( GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE );
  glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
  glEnable(GL_POINT_SMOOTH);    
  glColor3f(0.8, 0.4, 1);
  glPointSize(20);
  glBegin(GL_POINTS);
  for (int i=0; i<m_hitpoints.count(); i++)
    {
      Vec v = m_hitpoints[i] - m_centroid;
      v = m_q.inverseRotate(v) + m_centroid + m_position;
      glVertex3fv(v);
    }
  glEnd();
  glPointSize(1);
  glDisable(GL_POINT_SPRITE);
  glActiveTexture(GL_TEXTURE0);
  glDisable(GL_TEXTURE_2D);  
  glDisable(GL_POINT_SMOOTH);  

  glDepthRange(0.0, 1.0);
}

void
TrisetObject::draw(GLdouble *MVP,
		   Vec vd,
		   bool active)
{
  glDisable(GL_LIGHTING);

  if (!m_show)
    return;
  
  GLuint meshShader = ShaderFactory::meshShader();
  GLint* meshShaderParm = ShaderFactory::meshShaderParm();
  
  drawTrisetBuffer(MVP, vd,
		   0, -1, active,
		   meshShader, meshShaderParm,
		   false);
}

void
TrisetObject::drawOIT(GLdouble *MVP,
		      Vec vd,
		      bool active)
{
  glDisable(GL_LIGHTING);

  if (!m_show)
    return;


  GLuint oitShader = ShaderFactory::oitShader();
  GLint* oitShaderParm = ShaderFactory::oitShaderParm();
  
  drawTrisetBuffer(MVP, vd,
		   0, -1, active,
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
TrisetObject::predraw(bool active,
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
TrisetObject::copyFromOrigVcolor()
{
  m_vcolor = m_OrigVcolor;
}

void
TrisetObject::copyToOrigVcolor()
{
  m_OrigVcolor = m_vcolor;  
//  m_OrigVcolor.clear();
//  m_OrigVcolor.resize(m_vcolor.count());
//  for(int i=0; i<m_vcolor.count()/3; i++)
//    {
//      m_OrigVcolor[3*i+0] = m_vcolor[3*i+0];
//      m_OrigVcolor[3*i+1] = m_vcolor[3*i+1];
//      m_OrigVcolor[3*i+2] = m_vcolor[3*i+2];
//    }
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
  
  glDispatchCompute((m_vcolor.count()/3)/128, 1, 1);
  
  return true;
}


bool
TrisetObject::loadPLY(QString flnm)
{
//  m_position = Vec(0,0,0);
//  m_scale = Vec(1,1,1);
//  m_reveal = 0.0;
//  m_outline = 0.0;
//  m_glow = 0.0;
//  m_dark = 0.5;
//  m_pattern = Vec(0,10,0.5);
//  m_q = Quaternion();
//
//  typedef struct Vertex {
//    float x,y,z;
//    float r,g,b;
//    float nx,ny,nz;
//    void *other_props;       /* other properties */
//  } Vertex;
//
//  typedef struct Face {
//    unsigned char nverts;    /* number of vertex indices in list */
//    int *verts;              /* vertex index list */
//    void *other_props;       /* other properties */
//  } Face;
//
//  PlyProperty vert_props[] = { /* list of property information for a vertex */
//    {plyStrings[0], Float32, Float32, offsetof(Vertex,x), 0, 0, 0, 0},
//    {plyStrings[1], Float32, Float32, offsetof(Vertex,y), 0, 0, 0, 0},
//    {plyStrings[2], Float32, Float32, offsetof(Vertex,z), 0, 0, 0, 0},
//    {plyStrings[6], Float32, Float32, offsetof(Vertex,r), 0, 0, 0, 0},
//    {plyStrings[7], Float32, Float32, offsetof(Vertex,g), 0, 0, 0, 0},
//    {plyStrings[8], Float32, Float32, offsetof(Vertex,b), 0, 0, 0, 0},
//    {plyStrings[3], Float32, Float32, offsetof(Vertex,nx), 0, 0, 0, 0},
//    {plyStrings[4], Float32, Float32, offsetof(Vertex,ny), 0, 0, 0, 0},
//    {plyStrings[5], Float32, Float32, offsetof(Vertex,nz), 0, 0, 0, 0},
//  };
//
//  PlyProperty face_props[] = { /* list of property information for a face */
//    {plyStrings[9], Int32, Int32, offsetof(Face,verts),
//     1, Uint8, Uint8, offsetof(Face,nverts)},
//  };
//
//
//  /*** the PLY object ***/
//
//  int nverts,nfaces;
//  Vertex **vlist;
//  Face **flist;
//
//  PlyOtherProp *vert_other,*face_other;
//
//  bool per_vertex_color = false;
//  bool has_normals = false;
//
//  int i,j;
//  int elem_count;
//  char *elem_name;
//  PlyFile *in_ply;
//
//
//  /*** Read in the original PLY object ***/
//  FILE *fp = fopen(flnm.toLatin1().data(), "rb");
//
//  in_ply  = read_ply (fp);
//
//  for (i = 0; i < in_ply->num_elem_types; i++) {
//
//    /* prepare to read the i'th list of elements */
//    elem_name = setup_element_read_ply (in_ply, i, &elem_count);
//
//
//    if (QString("vertex") == QString(elem_name)) {
//
//      /* create a vertex list to hold all the vertices */
//      vlist = (Vertex **) malloc (sizeof (Vertex *) * elem_count);
//      nverts = elem_count;
//
//      /* set up for getting vertex elements */
//
//      setup_property_ply (in_ply, &vert_props[0]);
//      setup_property_ply (in_ply, &vert_props[1]);
//      setup_property_ply (in_ply, &vert_props[2]);
//
//      for (j = 0; j < in_ply->elems[i]->nprops; j++) {
//	PlyProperty *prop;
//	prop = in_ply->elems[i]->props[j];
//	if (QString("r") == QString(prop->name) ||
//	    QString("red") == QString(prop->name)) {
//	  setup_property_ply (in_ply, &vert_props[3]);
//	  per_vertex_color = true;
//	}
//	if (QString("g") == QString(prop->name) ||
//	    QString("green") == QString(prop->name)) {
//	  setup_property_ply (in_ply, &vert_props[4]);
//	  per_vertex_color = true;
//	}
//	if (QString("b") == QString(prop->name) ||
//	    QString("blue") == QString(prop->name)) {
//	  setup_property_ply (in_ply, &vert_props[5]);
//	  per_vertex_color = true;
//	}
//	if (QString("nx") == QString(prop->name)) {
//	  setup_property_ply (in_ply, &vert_props[6]);
//	  has_normals = true;
//	}
//	if (QString("ny") == QString(prop->name)) {
//	  setup_property_ply (in_ply, &vert_props[7]);
//	  has_normals = true;
//	}
//	if (QString("nz") == QString(prop->name)) {
//	  setup_property_ply (in_ply, &vert_props[8]);
//	  has_normals = true;
//	}
//      }
//
//      vert_other = get_other_properties_ply (in_ply, 
//					     offsetof(Vertex,other_props));
//
//      /* grab all the vertex elements */
//      for (j = 0; j < elem_count; j++) {
//        vlist[j] = (Vertex *) malloc (sizeof (Vertex));
//        get_element_ply (in_ply, (void *) vlist[j]);
//      }
//    }
//    else if (QString("face") == QString(elem_name)) {
//
//      /* create a list to hold all the face elements */
//      flist = (Face **) malloc (sizeof (Face *) * elem_count);
//      nfaces = elem_count;
//
//      /* set up for getting face elements */
//
//      setup_property_ply (in_ply, &face_props[0]);
//      face_other = get_other_properties_ply (in_ply, 
//					     offsetof(Face,other_props));
//
//      /* grab all the face elements */
//      for (j = 0; j < elem_count; j++) {
//        flist[j] = (Face *) malloc (sizeof (Face));
//        get_element_ply (in_ply, (void *) flist[j]);
//      }
//    }
//    else
//      get_other_element_ply (in_ply);
//  }
//
//  close_ply (in_ply);
//  free_ply (in_ply);
//
//  
//  float minX, maxX;
//  float minY, maxY;
//  float minZ, maxZ;
//  minX = maxX = vlist[0]->x;
//  minY = maxY = vlist[0]->y;
//  minZ = maxZ = vlist[0]->z;
//  for(int i=0; i<nverts; i++)
//    {
//      minX = qMin(minX, vlist[i]->x);
//      maxX = qMax(maxX, vlist[i]->x);
//      minY = qMin(minY, vlist[i]->y);
//      maxY = qMax(maxY, vlist[i]->y);
//      minZ = qMin(minZ, vlist[i]->z);
//      maxZ = qMax(maxZ, vlist[i]->z);
//    }
//  minX = floor(minX);
//  minY = floor(minY);
//  minZ = floor(minZ);
//  maxX = ceil(maxX);
//  maxY = ceil(maxY);
//  maxZ = ceil(maxZ);
//  int h = maxX-minX+1;
//  int w = maxY-minY+1;
//  int d = maxZ-minZ+1;
//  
//  m_nX = d;
//  m_nY = w;
//  m_nZ = h;
//
//  
//  m_vertices.resize(nverts);
//  for(int i=0; i<nverts; i++)
//    m_vertices[i] = Vec(vlist[i]->x,
//			vlist[i]->y,
//			vlist[i]->z);
//
//
//  m_normals.clear();
//  if (has_normals)
//    {
//      m_normals.resize(nverts);
//      for(int i=0; i<nverts; i++)
//	m_normals[i] = Vec(vlist[i]->nx,
//			   vlist[i]->ny,
//			   vlist[i]->nz);
//    }
//
//  m_vcolor.clear();
//  if (per_vertex_color)
//    {
//      m_vcolor.resize(nverts);
//      for(int i=0; i<nverts; i++)
//	m_vcolor[i] = Vec(vlist[i]->r/255.0f,
//			  vlist[i]->g/255.0f,
//			  vlist[i]->b/255.0f);
//    }
//
//
//  // only triangles considered
//  int ntri=0;
//  for (int i=0; i<nfaces; i++)
//    {
//      if (flist[i]->nverts >= 3)
//	ntri++;
//    }
//  m_triangles.resize(3*ntri);
//
//  int tri=0;
//  for(int i=0; i<nfaces; i++)
//    {
//      if (flist[i]->nverts >= 3)
//	{
//	  m_triangles[3*tri+0] = flist[i]->verts[0];
//	  m_triangles[3*tri+1] = flist[i]->verts[1];
//	  m_triangles[3*tri+2] = flist[i]->verts[2];
//	  tri++;
//	}
//    }
//
//
//  Vec bmin = m_vertices[0];
//  Vec bmax = m_vertices[0];
//  for(int i=0; i<nverts; i++)
//    {
//      bmin = StaticFunctions::minVec(bmin, m_vertices[i]);
//      bmax = StaticFunctions::maxVec(bmax, m_vertices[i]);
//    }
//  m_centroid = (bmin + bmax)/2;
//
//  m_enclosingBox[0] = Vec(bmin.x, bmin.y, bmin.z);
//  m_enclosingBox[1] = Vec(bmax.x, bmin.y, bmin.z);
//  m_enclosingBox[2] = Vec(bmax.x, bmax.y, bmin.z);
//  m_enclosingBox[3] = Vec(bmin.x, bmax.y, bmin.z);
//  m_enclosingBox[4] = Vec(bmin.x, bmin.y, bmax.z);
//  m_enclosingBox[5] = Vec(bmax.x, bmin.y, bmax.z);
//  m_enclosingBox[6] = Vec(bmax.x, bmax.y, bmax.z);
//  m_enclosingBox[7] = Vec(bmin.x, bmax.y, bmax.z);
//
//
////  QMessageBox::information(0, "", QString("%1 %2 %3\n%4 %5").	\
////			   arg(m_nX).arg(m_nY).arg(m_nZ).	\
////			   arg(m_vertices.count()).		\
////			   arg(m_triangles.count()/3));
//
//
//  m_fileName = flnm;

  return true;
}

bool
TrisetObject::loadTriset(QString flnm)
{
//  QFile fd(flnm);
//  fd.open(QFile::ReadOnly);
//
//  uchar stype = 0;
//  fd.read((char*)&stype, sizeof(uchar));
//  if (stype != 0)
//    {
//      QMessageBox::critical(0, "Cannot load triset",
//			    "Wrong input format : First byte not equal to 0");
//      return false;
//    }
//
//  fd.read((char*)&m_nX, sizeof(int));
//  fd.read((char*)&m_nY, sizeof(int));
//  fd.read((char*)&m_nZ, sizeof(int));
//
//
//  int nvert, ntri;
//  fd.read((char*)&nvert, sizeof(int));
//  fd.read((char*)&ntri, sizeof(int));
//   
//
////  QMessageBox::information(0, "", QString("%1 %2 %3\n%4 %5").	\
////			   arg(m_nX).arg(m_nY).arg(m_nZ).\
////			   arg(nvert).\
////			   arg(ntri));
//
//
//  float *vert = new float[nvert*3];
//  fd.read((char*)vert, sizeof(float)*3*nvert);
//
//  float *vnorm = new float[nvert*3];
//  fd.read((char*)vnorm, sizeof(float)*3*nvert);
//
//  int *tri = new int[ntri*3];
//  fd.read((char*)tri, sizeof(int)*3*ntri);
//
//  fd.close();
//
//
//  m_vertices.resize(nvert);
//  for(int i=0; i<nvert; i++)
//    m_vertices[i] = Vec(vert[3*i],
//			vert[3*i+1],
//			vert[3*i+2]);
//  delete [] vert;
//
//  m_normals.resize(nvert);
//  for(int i=0; i<nvert; i++)
//    m_normals[i] = Vec(vnorm[3*i],
//		       vnorm[3*i+1],
//		       vnorm[3*i+2]);
//  delete [] vnorm;
//
//  m_triangles.resize(3*ntri);
//  for(int i=0; i<3*ntri; i++)
//    m_triangles[i] = tri[i];
//  delete [] tri;
//
//  
//
//  Vec bmin = m_vertices[0];
//  Vec bmax = m_vertices[0];
//  for(int i=0; i<nvert; i++)
//    {
//      bmin = StaticFunctions::minVec(bmin, m_vertices[i]);
//      bmax = StaticFunctions::maxVec(bmax, m_vertices[i]);
//    }
//  m_centroid = (bmin + bmax)/2;
//
//  m_position = Vec(0,0,0);
//  m_scale = Vec(1,1,1);
//  m_reveal = 0.0;
//  m_outline = 0.0;
//  m_glow = 0.0;
//  m_dark = 0.5;
//  m_q = Quaternion();
//  m_pattern = Vec(0,10,0.5);
//
//  m_enclosingBox[0] = Vec(bmin.x, bmin.y, bmin.z);
//  m_enclosingBox[1] = Vec(bmax.x, bmin.y, bmin.z);
//  m_enclosingBox[2] = Vec(bmax.x, bmax.y, bmin.z);
//  m_enclosingBox[3] = Vec(bmin.x, bmax.y, bmin.z);
//  m_enclosingBox[4] = Vec(bmin.x, bmin.y, bmax.z);
//  m_enclosingBox[5] = Vec(bmax.x, bmin.y, bmax.z);
//  m_enclosingBox[6] = Vec(bmax.x, bmax.y, bmax.z);
//  m_enclosingBox[7] = Vec(bmin.x, bmax.y, bmax.z);
//
//
////  QMessageBox::information(0, "", QString("%1 %2 %3\n%4 %5").	\
////			   arg(m_nX).arg(m_nY).arg(m_nZ).\
////			   arg(m_vertices.count()).\
////			   arg(m_triangles.count()));
//
//  m_fileName = flnm;

  return true;
}


TrisetInformation
TrisetObject::get()
{
  TrisetInformation ti;
  ti.show = m_show;
  ti.clip = m_clip;
  ti.clearView = m_clearView;
  ti.filename = m_fileName;
  ti.position = m_position;
  ti.scale = m_scale;
  ti.q = m_q;
  ti.color = m_color;
  ti.materialId = m_materialId;
  ti.materialMix = m_materialMix;
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
  m_clearView = ti.clearView;
  m_position = ti.position;
  m_scale = ti.scale;
  m_q = ti.q;
  m_roughness = ti.roughness;
  m_color = ti.color;
  m_materialId = ti.materialId;
  m_materialMix = ti.materialMix;
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
//  //---------------
//  //---------------
//  
//  bool has_normals = (m_normals.count() > 0);
//  bool per_vertex_color = (m_vcolor.count() > 0);
//
//  QString flnm = QFileDialog::getSaveFileName(0,
//					      "Export mesh to file",
//					      Global::previousDirectory(),
//					      "*.ply");
//  if (flnm.size() == 0)
//    return;
//
//  typedef struct PlyFace
//  {
//    unsigned char nverts;    /* number of Vertex indices in list */
//    int *verts;              /* Vertex index list */
//  } PlyFace;
//
//  typedef struct
//  {
//    float  x,  y,  z;  /**< Vertex coordinates */
//    float nx, ny, nz;  /**< Vertex normal */
//    uchar r, g, b;
//  } myVertex ;
//
//
//  PlyProperty vert_props[] = { /* list of property information for a vertex */
//    {plyStrings[0], Float32, Float32, offsetof(myVertex,x), 0, 0, 0, 0},
//    {plyStrings[1], Float32, Float32, offsetof(myVertex,y), 0, 0, 0, 0},
//    {plyStrings[2], Float32, Float32, offsetof(myVertex,z), 0, 0, 0, 0},
//    {plyStrings[3], Float32, Float32, offsetof(myVertex,nx), 0, 0, 0, 0},
//    {plyStrings[4], Float32, Float32, offsetof(myVertex,ny), 0, 0, 0, 0},
//    {plyStrings[5], Float32, Float32, offsetof(myVertex,nz), 0, 0, 0, 0},
//    {plyStrings[6], Uint8, Uint8, offsetof(myVertex,r), 0, 0, 0, 0},
//    {plyStrings[7], Uint8, Uint8, offsetof(myVertex,g), 0, 0, 0, 0},
//    {plyStrings[8], Uint8, Uint8, offsetof(myVertex,b), 0, 0, 0, 0},
//  };
//
//  PlyProperty face_props[] = { /* list of property information for a face */
//    {plyStrings[9], Int32, Int32, offsetof(PlyFace,verts),
//     1, Uint8, Uint8, offsetof(PlyFace,nverts)},
//  };
//
//  PlyFile    *ply;
//  FILE       *fp = fopen(flnm.toLatin1().data(), bin ? "wb" : "w");
//
//  PlyFace     face ;
//  int         verts[3] ;
//  char       *elem_names[]  = {plyStrings[10], plyStrings[11]};
//  ply = write_ply (fp,
//		   2,
//		   elem_names,
//		   bin ? PLY_BINARY_LE : PLY_ASCII );
//
//  int nvertices = m_vertices.count()/3;
//  /* describe what properties go into the PlyVertex elements */
//  describe_element_ply ( ply, plyStrings[10], nvertices );
//  describe_property_ply ( ply, &vert_props[0] );
//  describe_property_ply ( ply, &vert_props[1] );
//  describe_property_ply ( ply, &vert_props[2] );
//  describe_property_ply ( ply, &vert_props[3] );
//  describe_property_ply ( ply, &vert_props[4] );
//  describe_property_ply ( ply, &vert_props[5] );
//  describe_property_ply ( ply, &vert_props[6] );
//  describe_property_ply ( ply, &vert_props[7] );
//  describe_property_ply ( ply, &vert_props[8] );
//
//  /* describe PlyFace properties (just list of PlyVertex indices) */
//  int ntriangles = m_triangles.count()/3;
//  describe_element_ply ( ply, plyStrings[11], ntriangles );
//  describe_property_ply ( ply, &face_props[0] );
//
//  header_complete_ply ( ply );
//
//
//  /* set up and write the PlyVertex elements */
//  put_element_setup_ply ( ply, plyStrings[10] );
//
////  // regenerate local transfromation so we don't have scaling due to surface being selected(active)
////  genLocalXform();    
////  // use the transpose for transformation
////  double s[16];
////  s[0] = m_localXform[0];
////  s[1] = m_localXform[4];
////  s[2] = m_localXform[8];
////  s[3] = m_localXform[12];
////  s[4] = m_localXform[1];
////  s[5] = m_localXform[5];
////  s[6] = m_localXform[9];
////  s[7] = m_localXform[13];
////  s[8] = m_localXform[2];
////  s[9] = m_localXform[6];
////  s[10] = m_localXform[10];
////  s[11] = m_localXform[14];
////  s[12] = m_localXform[3];
////  s[13] = m_localXform[7];
////  s[14] = m_localXform[11];
////  s[15] = m_localXform[15];
//     
//  for(int i=0; i<m_vertices.count()/3; i++)
//    {
//      myVertex vertex;
//      Vec v = Matrix::xformVec(s,Vec(m_vertices[3*i+0],m_vertices[3*i+1],m_vertices[3*i+2]));
//      vertex.x = v.x;
//      vertex.y = v.y;
//      vertex.z = v.z;
//      if (has_normals)
//	{
//	  Vec vn = Matrix::rotateVec(s,Vec(m_normals[3*i+0],m_normals[3*i+1],m_normals[3*i+2]));
//	  vertex.nx = vn.x;
//	  vertex.ny = vn.y;
//	  vertex.nz = vn.z;
//	}
//      if (per_vertex_color)
//	{
//	  vertex.r = 255*m_vcolor[3*i+0];
//	  vertex.g = 255*m_vcolor[3*i+1];
//	  vertex.b = 255*m_vcolor[3*i+2];
//	}
//      put_element_ply ( ply, ( void * ) &vertex );
//    }
//
//  put_element_setup_ply ( ply, plyStrings[11] );
//  face.nverts = 3 ;
//  face.verts  = verts ;
//  for(int i=0; i<m_triangles.count()/3; i++)
//    {
//      int v0 = m_triangles[3*i];
//      int v1 = m_triangles[3*i+1];
//      int v2 = m_triangles[3*i+2];
//
//      face.verts[0] = v0;
//      face.verts[1] = v1;
//      face.verts[2] = v2;
//
//      put_element_ply ( ply, ( void * ) &face );
//    }
//
//  close_ply ( ply );
//  free_ply ( ply );
//  fclose( fp ) ;
//
//  QMessageBox::information(0, "Save Mesh", "done");
}

// load binary STL files
bool
TrisetObject::loadSTLModel(QString flnm)
{
  m_vertices.clear();
  m_vcolor.clear();
  m_normals.clear();
  m_triangles.clear();
  m_uv.clear();
  m_meshInfo.clear();
  m_diffuseMat.clear();

  QFile sfile(flnm);
  sfile.open(QFile::ReadOnly);
  sfile.seek(80); // skip first 80 bytes

  int ntri;
  sfile.read((char*)&ntri, 4);

  
  ntri = qMin(39600000, ntri);
  
  m_vertices.reserve(ntri*9);
  m_normals.reserve(ntri*9);
  m_triangles.reserve(ntri*3);

  int chunkSize = 5000;
  uchar *data = new uchar[50 * chunkSize];

  // read int chunks of chunkSize
  int ic = 0;
  for(int i=0; i<ntri; i+=chunkSize)
    {
      Global::progressBar()->setValue((int)(100.0*(float)(i)/(float)ntri));
      qApp->processEvents();

      int chnk = qMin(chunkSize, ntri-i);
      sfile.read((char*)data, chnk*50);

      for (int c=0; c<chnk; c++)
	{
	  float *fdata = (float*)(data+c*50);
	      
	  m_normals << fdata[0] << fdata[1] << fdata[2];
	  m_normals << fdata[0] << fdata[1] << fdata[2];
	  m_normals << fdata[0] << fdata[1] << fdata[2];
	  
	  m_vertices << fdata[3] << fdata[4]  << fdata[5];
	  m_vertices << fdata[6] << fdata[7]  << fdata[8];
	  m_vertices << fdata[9] << fdata[10] << fdata[11];
	  
	  m_triangles << 3*ic;
	  m_triangles << 3*ic+1;
	  m_triangles << 3*ic+2;
	  ic ++;
	}
    }

  sfile.close();
  delete [] data;
  
  QPolygon poly;
  poly << QPoint(0, m_vertices.count()/3);
  poly << QPoint(0, m_triangles.count());
  poly << QPoint(0, 0);
  m_meshInfo << poly;

  
  float minX, maxX;
  float minY, maxY;
  float minZ, maxZ;
  minX = maxX = m_vertices[0];
  minY = maxY = m_vertices[1];
  minZ = maxZ = m_vertices[2];
  m_centroid = Vec(0,0,0);
//  int istep = 10;
//  if (m_vertices.count() < 300)
//    istep = 1;
  int istep = qMax(1, m_vertices.count()/300);
  int vc = 0;
  for(int i=0; i<m_vertices.count()/3; i+=istep)
    {
      vc++;
      m_centroid += Vec(m_vertices[3*i+0], m_vertices[3*i+1], m_vertices[3*i+2]);

      minX = qMin(minX, m_vertices[3*i+0]);
      maxX = qMax(maxX, m_vertices[3*i+0]);
      minY = qMin(minY, m_vertices[3*i+1]);
      maxY = qMax(maxY, m_vertices[3*i+1]);
      minZ = qMin(minZ, m_vertices[3*i+2]);
      maxZ = qMax(maxZ, m_vertices[3*i+2]);
    }
  
  m_centroid /= vc;
  //m_centroid /= (m_vertices.count()/3);

  //-----------------------------------
  m_samplePoints.clear();
  m_samplePoints << m_centroid;
  //-----------------------------------

  Global::progressBar()->setValue(95);
  qApp->processEvents();

  
  Vec bmin = Vec(minX, minY, minZ);
  Vec bmax = Vec(maxX, maxY, maxZ);

//  QMessageBox::information(0, "", QString("%1 : %2\n %3 %4 %5\n %6 %7 %8").\
//			   arg(m_vertices.count()/3).\
//			   arg(m_triangles.count()/3).\
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

  m_centroid = (bmin + bmax)/2;

  m_position = Vec(0,0,0);
  m_scale = Vec(1,1,1);

  m_fileName = flnm;

  if (m_vcolor.count() == 0)
    {
      m_color = Vec(0.9,0.9,0.9);
      m_vcolor.reserve(m_vertices.count());
      for(int f=0; f<m_vertices.count()/3; f++)
	m_vcolor << m_color.x << m_color.y << m_color.z;
    }
    
//  //---------------
//  // set caption
//  m_captionPosition = Vec(0,0,0);
//  if (m_co) delete m_co;
//  m_co = new CaptionObject();
//  m_co->setPosition(QPointF(0.1, 0.1));
//  m_co->setText("");
//  m_co->setFont(QFont("MS Reference Sans Serif", 16));
//  m_co->setColor(Qt::gray);
//  m_co->setHaloColor(Qt::transparent);
//  //---------------
  
  Global::progressBar()->setValue(100);
  Global::progressBar()->hide();
  qApp->processEvents();


  MainWindowUI::mainWindowUI()->statusBar->showMessage("");
  
  return true;
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

  const aiScene* scene;

  if (StaticFunctions::checkExtension(flnm, ".stl"))
    {
      QFile sfile(flnm);
      sfile.open(QFile::ReadOnly);
      char word[10];
      memset(word, 0, 10);
      sfile.read(&word[0], 9);
      QString solid(word);
      solid.truncate(5);
      if (solid != "solid") // binary STL
      	{
	  sfile.close();
	  return loadSTLModel(flnm);
	}
      sfile.close();
    }

  //----------------------
  bool possibleNormalMap = false;
  scene = importer.ReadFile( flnm.toLatin1().data(), aiProcess_SortByPType);

  if (scene->mNumMaterials > 0)
    {
      for(int m=0; m<scene->mNumMaterials; m++)
	{
	  aiMaterial *mat = scene->mMaterials[m];
	  aiString path;
	  if (mat->GetTexture(aiTextureType_NORMALS, 0, &path) == AI_SUCCESS)
	    possibleNormalMap = true;
	}
    }
  
  if(!scene)
    {
      QMessageBox::information(0, "Error Importing Asset",
			       QString("Couldn't load model %1").arg(flnm));
      MainWindowUI::mainWindowUI()->statusBar->showMessage("");
      return false;
    }  
  //----------------------


  
  if (possibleNormalMap)
    scene = importer.ReadFile( flnm.toLatin1().data(), 
			       aiProcess_Triangulate            |
			       aiProcess_GenSmoothNormals       |
			       aiProcess_JoinIdenticalVertices  |
			       aiProcess_CalcTangentSpace |
			       aiProcess_SortByPType);
  else // don't calculate tangents and bitangents
    scene = importer.ReadFile( flnm.toLatin1().data(), 
			       aiProcess_Triangulate            |
			       aiProcess_GenSmoothNormals       |
			       aiProcess_JoinIdenticalVertices  |
			       aiProcess_SortByPType);

  m_vertices.clear();
  m_vcolor.clear();
  m_normals.clear();
  m_tangents.clear();
  m_triangles.clear();
  m_uv.clear();
  m_meshInfo.clear();
  m_diffuseMat.clear();
  m_normalMat.clear();
  
  //QMessageBox::information(0, "", QString("%1").arg(scene->mNumMeshes));

  int nvert = 0;
  //--------------
  for(int i=0; i<scene->mNumMeshes; i++)
    {
      Global::progressBar()->setValue((int)(100.0*(float)(i)/(float)(scene->mNumMeshes)));
      qApp->processEvents();
  

      int vStart = m_vertices.count()/3;
      int iStart = m_triangles.count();
      
      aiMesh* mesh = scene->mMeshes[i];
      bool hasVertexColors = mesh->HasVertexColors(0);
      bool hasUV = mesh->HasTextureCoords(0);
      bool hasTangents = mesh->HasTangentsAndBitangents();

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

	  if (hasTangents)
	    {
	      aiVector3D tangent = mesh->mTangents[j];
	      Vec vn = Vec(tangent.x, tangent.y, tangent.z).unit();
	      m_tangents << vn.x << vn.y << vn.z;
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
    } // loop over mNumMeshes
  //--------------

			     
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
								 "Image Files (*.png *.tif *.tga *.bmp *.jpg *.jpeg *.gif)");
		  
		  QFileInfo f(imgFile);

		  if (f.exists() == true)
		    diffuseTexFile = imgFile;
		  else
		    diffuseTexFile = QString();
		}	    
	      m_diffuseMat << diffuseTexFile;
	      texFound = texFound || (!diffuseTexFile.isEmpty());
	    }
	  else
	    m_diffuseMat << QString();
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
							     "Image Files (*.png *.tif *.tga *.bmp *.jpg *.jpeg *.gif)");
	      
	      QFileInfo f(imgFile);
	      
	      if (f.exists() == true)
		{
		  for (int m=0; m<m_diffuseMat.count(); m++)
		    m_diffuseMat[m] = imgFile;
		}
	      else
		m_uv.clear();
	    }	  
	}	  
    } // mNumMaterials
  //--------------

  
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
	  QString normalTexFile;

	  if (mat->GetTexture(aiTextureType_NORMALS, 0, &path) == AI_SUCCESS)
	    {
	      normalTexFile = QFileInfo(QFileInfo(flnm).dir(),
					 QString(path.data)).absoluteFilePath();
	      if (!QFile::exists(normalTexFile))
		{
		  QString mesg;
		  if (normalTexFile.isEmpty())
		    mesg = QString("Normal texture not found for %1").arg(flnm);
		  else
		    mesg = "Cannot locate " + normalTexFile;
		  
		  QMessageBox::information(0, "Texture Error", mesg);

		  QString imgFile = QFileDialog::getOpenFileName(0,
								 QString("Load normal texture"),
								 QFileInfo(flnm).dir().path(),
								 "Image Files (*.png *.tif *.tga *.bmp *.jpg *.jpeg *.gif)");
		  
		  QFileInfo f(imgFile);

		  if (f.exists() == true)
		    normalTexFile = imgFile;
		  else
		    normalTexFile = QString();
		}	    
	      m_normalMat << normalTexFile;
	      texFound = texFound || (!normalTexFile.isEmpty());
	    }
	  else
	    m_normalMat << QString();
	}      
    } // mNumMaterials
  //--------------
  bool clearNormalMat = true;
  for(int d=0; d<m_normalMat.count(); d++)
    if (!m_normalMat[d].isEmpty())
      {
	clearNormalMat = false;
	break;
      }
  if (clearNormalMat)
    {
      m_normalMat.clear();
      m_tangents.clear();
    }

 
//  if (m_normalMat.count() > 0 || m_diffuseMat.count() > 0)
//    QMessageBox::information(0, "", QString("%1 %2 : %3").			\
//			     arg(m_diffuseMat.count()).arg(m_normalMat.count()).arg(m_tangents.count()));
  
  Global::progressBar()->setValue(90);
  qApp->processEvents();



  
  float minX, maxX;
  float minY, maxY;
  float minZ, maxZ;
  minX = maxX = m_vertices[0];
  minY = maxY = m_vertices[1];
  minZ = maxZ = m_vertices[2];
  m_centroid = Vec(0,0,0);
//  int istep = 10;
//  if (m_vertices.count() < 300)
//    istep = 1;
  int istep = qMax(1, m_vertices.count()/300);

  int vc = 0;
  for(int i=0; i<m_vertices.count()/3; i+=istep)
    {
      vc ++;
      m_centroid += Vec(m_vertices[3*i+0],m_vertices[3*i+1],m_vertices[3*i+2]);

      minX = qMin(minX, m_vertices[3*i+0]);
      maxX = qMax(maxX, m_vertices[3*i+0]);
      minY = qMin(minY, m_vertices[3*i+1]);
      maxY = qMax(maxY, m_vertices[3*i+1]);
      minZ = qMin(minZ, m_vertices[3*i+2]);
      maxZ = qMax(maxZ, m_vertices[3*i+2]);
    }

  m_centroid /= vc;

  
  //-----------------------------------
  m_samplePoints.clear();
  m_samplePoints << m_centroid;
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
      //m_vcolor.fill(m_color, m_vertices.count()/3);
      m_vcolor.fill(0.9f, m_vertices.count()); // since all components are the same
    }
  
//  //---------------
//  // set caption
//  m_captionPosition = Vec(0,0,0);
//  if (m_co) delete m_co;
//  m_co = new CaptionObject();
//  m_co->setPosition(QPointF(0.1, 0.1));
//  m_co->setText("");
//  m_co->setFont(QFont("MS Reference Sans Serif", 16));
//  m_co->setColor(Qt::gray);
//  m_co->setHaloColor(Qt::transparent);
//  //---------------
  
  Global::progressBar()->setValue(100);
  Global::progressBar()->hide();
  qApp->processEvents();


  MainWindowUI::mainWindowUI()->statusBar->showMessage("");
  
  return true;
}


void
TrisetObject::smoothVertexColors(int niter)
{
  if (m_vcolor.count() == 0)
    {
      QMessageBox::information(0, "Smoothing", "No vertex color for this mesh.");
      return;
    }
  
  m_vcolor = m_OrigVcolor;
  
  int ntri = m_triangles.count()/3;
  for (int ni=0; ni<niter; ni++)
    {
      QVector<float> tcolor = m_vcolor;
      QVector<int> count;
      count.fill(0, m_vcolor.count());
      m_vcolor.fill(0);
      for (int t=0; t<ntri; t++)
	{
	  int v0 = m_triangles[3*t];
	  int v1 = m_triangles[3*t+1];
	  int v2 = m_triangles[3*t+2];
	  
	  Vec c0 = Vec(tcolor[3*v0],tcolor[3*v0+1],tcolor[3*v0+2]);
	  Vec c1 = Vec(tcolor[3*v1],tcolor[3*v1+1],tcolor[3*v1+2]);
	  Vec c2 = Vec(tcolor[3*v2],tcolor[3*v2+1],tcolor[3*v2+2]);
	  
	  Vec c = (c0 + c1 + c2)/3;
	  
	  m_vcolor[3*v0] += c[0]; m_vcolor[3*v0+1] += c[1]; m_vcolor[3*v0+2] += c[2];
	  m_vcolor[3*v1] += c[0]; m_vcolor[3*v1+1] += c[1]; m_vcolor[3*v1+2] += c[2];
	  m_vcolor[3*v2] += c[0]; m_vcolor[3*v2+1] += c[1]; m_vcolor[3*v2+2] += c[2];

	  count[v0]++;
	  count[v1]++;
	  count[v2]++;
	}
      for (int i=0; i<m_vcolor.count()/3; i++)
	{
	  m_vcolor[3*i+0] /= count[i];
	  m_vcolor[3*i+1] /= count[i];
	  m_vcolor[3*i+2] /= count[i];
	}
    }

  loadVertexBufferData();
}
