#include "drawlowresvolume.h"
#include "viewer.h"
#include "volume.h"
#include "staticfunctions.h"
#include "shaderfactory.h"
#include "shaderfactoryrgb.h"
#include "global.h"
#include "mainwindowui.h"

#include <QDomDocument>

void DrawLowresVolume::activateBounds() {m_boundingBox.activateBounds();}
bool DrawLowresVolume::raised() { return showing; }
void DrawLowresVolume::raise()
{
  showing = true;
  m_boundingBox.activateBounds();
  load3dTexture();

  if (MainWindowUI::mainWindowUI()->actionHiresMode->isChecked())
    MainWindowUI::mainWindowUI()->actionHiresMode->setChecked(false);
}
void DrawLowresVolume::lower()
{
  showing = false;
  m_boundingBox.deactivateBounds();
  if (m_dataTex)
    glDeleteTextures(1, &m_dataTex);
  m_dataTex = 0;

  if (!MainWindowUI::mainWindowUI()->actionHiresMode->isChecked())
    MainWindowUI::mainWindowUI()->actionHiresMode->setChecked(true);
}

void DrawLowresVolume::setCurrentVolume(int vnum) { m_currentVolume = vnum; }

QImage DrawLowresVolume::histogramImage1D() { return m_histogramImage1D; }
QImage DrawLowresVolume::histogramImage2D() { return m_histogramImage2D; }

int* DrawLowresVolume::histogram2D() { return m_Volume->getLowres2dHistogram(m_currentVolume); }

Vec DrawLowresVolume::volumeSize() { return m_virtualTextureSize; }
Vec DrawLowresVolume::volumeMin() { return m_dataMin; }
Vec DrawLowresVolume::volumeMax() { return m_dataMax; }

void DrawLowresVolume::subvolumeBounds(Vec& bmin, Vec& bmax)
{
  m_boundingBox.bounds(bmin, bmax);

//  Vec bsize = bmax-bmin;
//
//  if ((int)bsize.x%2)
//    bmax.x = bmin.x + bsize.x;
//  else 
//    bmax.x = bmin.x + bsize.x-1;
//
//  if ((int)bsize.y%2)
//    bmax.y = bmin.y + bsize.y;
//  else 
//    bmax.y = bmin.y + bsize.y-1;
//
//  if ((int)bsize.z%2)
//    bmax.z = bmin.z + bsize.z;
//  else 
//    bmax.z = bmin.z + bsize.z-1;
}

void DrawLowresVolume::setSubvolumeBounds(Vec obmin, Vec obmax)
{
  Vec bmin = StaticFunctions::clampVec(m_dataMin, m_dataMax, obmin);
  Vec bmax = StaticFunctions::clampVec(m_dataMin, m_dataMax, obmax);
  //m_boundingBox.setBounds(bmin, bmax);
  m_boundingBox.setPositions(bmin, bmax);
}

DrawLowresVolume::DrawLowresVolume(Viewer *viewer,
				   Volume *volume) :
  QObject()
{
  m_progObj = 0;
  showing = true;

  m_currentVolume = 0;

  m_Viewer = viewer;
  m_Volume = volume;

  m_virtualTextureSize = m_virtualTextureMin = m_virtualTextureMax = Vec(0,0,0);
  m_dataMin = m_dataMax = Vec(0,0,0);

  m_histImageData1D = new unsigned char[256*256*4];
  m_histImageData2D = new unsigned char[256*256*4];
  m_histogramImage1D = QImage(256, 256, QImage::Format_RGB32);
  m_histogramImage2D = QImage(256, 256, QImage::Format_RGB32);

  m_dataTex = 0;
  //  glGenTextures(1, &m_dataTex);
}

DrawLowresVolume::~DrawLowresVolume()
{
  delete [] m_histImageData1D;
  delete [] m_histImageData2D;
  m_virtualTextureSize = m_virtualTextureMin = m_virtualTextureMax = Vec(0,0,0);
  m_dataMin = m_dataMax = Vec(0,0,0);

  if (m_dataTex)
    glDeleteTextures(1, &m_dataTex);
  m_dataTex = 0;
}

void DrawLowresVolume::init() {}

void
DrawLowresVolume::generateHistogramImage()
{
  if (m_Volume->pvlVoxelType(0) > 0)
    return;

  int *hist2D = m_Volume->getLowres2dHistogram(m_currentVolume);
  if (hist2D)
    {
      for (int i=0; i<256*256; i++)
	{
	  m_histImageData2D[4*i + 3] = 255;
	  m_histImageData2D[4*i + 0] = hist2D[i];
	  m_histImageData2D[4*i + 1] = hist2D[i];
	  m_histImageData2D[4*i + 2] = hist2D[i];
	}
    }
  m_histogramImage2D = QImage(m_histImageData2D, 256, 256, QImage::Format_ARGB32);
  m_histogramImage2D = m_histogramImage2D.mirrored();  


  int *hist1D = m_Volume->getLowres1dHistogram(m_currentVolume);
  memset(m_histImageData1D, 0, 4*256*256);
  if (hist1D)
    {
      for (int i=0; i<256; i++)
	{
	  for (int j=0; j<256; j++)
	    {
	      int idx = 256*j + i;
	      m_histImageData1D[4*idx + 3] = 255;
	    }
	  
	  int h = hist1D[i];
	  for (int j=0; j<h; j++)
	    {
	      int idx = 256*j + i;
	      m_histImageData1D[4*idx + 0] = 255*j/h;
	      m_histImageData1D[4*idx + 1] = 255*j/h;
	      m_histImageData1D[4*idx + 2] = 255*j/h;
	    }
	}
    }
  m_histogramImage1D = QImage(m_histImageData1D, 256, 256, QImage::Format_ARGB32);
  m_histogramImage1D = m_histogramImage1D.mirrored();  
}

void
DrawLowresVolume::load3dTexture()
{
  if (!m_dataTex)
    glGenTextures(1, &m_dataTex);

  Vec textureSize = m_Volume->getLowresTextureVolumeSize();
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_3D, m_dataTex);
  glTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
  glTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
  glTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  if (Global::interpolationType(Global::TextureInterpolation)) // linear
    {
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
  else
    {
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

  if (Global::volumeType() == Global::DummyVolume)
    {
      glTexImage3D(GL_TEXTURE_3D,
		   0, // single resolution
		   1,
		   textureSize.x, textureSize.y, textureSize.z,
		   0, // no border
		   GL_RED,
		   GL_UNSIGNED_BYTE,
		   NULL);
    }
  else
    {
      int nvol = 1;
      if (Global::volumeType() == Global::DoubleVolume) nvol = 2;
      if (Global::volumeType() == Global::TripleVolume) nvol = 3;
      if (Global::volumeType() == Global::QuadVolume) nvol = 4;
      if (Global::volumeType() == Global::RGBVolume) nvol = 3;
      if (Global::volumeType() == Global::RGBAVolume) nvol = 4;

      int format = GL_RED;
      if (nvol == 2) format = GL_LUMINANCE_ALPHA;
      else if (nvol == 3) format = GL_RGB;
      else if (nvol == 4) format = GL_RGBA;

      int internalFormat = nvol;
      int vtype = GL_UNSIGNED_BYTE;

      if (m_Volume->pvlVoxelType(0) > 0)
	{
	  if (nvol == 1) internalFormat = GL_LUMINANCE16;
	  if (nvol == 2) internalFormat = GL_LUMINANCE16_ALPHA16;
	  if (nvol == 3) internalFormat = GL_RGB16;
	  if (nvol == 4) internalFormat = GL_RGBA16;

	  if (nvol == 1) format = GL_LUMINANCE;
	  if (nvol == 2) format = GL_LUMINANCE_ALPHA;
	  if (nvol == 3) format = GL_RGB;
	  if (nvol == 4) format = GL_RGBA;

	  vtype = GL_UNSIGNED_SHORT;
	}
    
      glTexImage3D(GL_TEXTURE_3D,
		   0, // single resolution
		   internalFormat,
		   textureSize.x, textureSize.y, textureSize.z,
		   0, // no border
		   format,
		   vtype,
		   m_Volume->getLowresTextureVolume());
    }

  m_Viewer->setSceneBoundingBox(m_dataMin, m_dataMax);
  m_Viewer->showEntireScene();
}

void
DrawLowresVolume::loadVolume()
{
  generateHistogramImage();

  Vec textureSize = m_Volume->getLowresTextureVolumeSize();
  Vec fullVolSize = m_Volume->getFullVolumeSize();
  Vec lowVolSize = m_Volume->getLowresVolumeSize();

  Vec vscale = VECDIVIDE(fullVolSize, lowVolSize);

  m_virtualTextureSize = VECPRODUCT(textureSize,vscale);
  m_virtualTextureMin = Vec(0, 0, 0);
  m_virtualTextureMax = fullVolSize-Vec(1,1,1);

  m_dataMin = Vec(0,0,0);
  m_dataMax = fullVolSize-Vec(1,1,1);

  m_boundingBox.setBounds(m_dataMin, m_dataMax);

//  Vec voxelScaling = Global::voxelScaling();
//  m_boundingBox.setBounds(VECPRODUCT(m_dataMin,voxelScaling),
//			  VECPRODUCT(m_dataMax,voxelScaling));
  raise();
  load3dTexture();
}

void
DrawLowresVolume::updateScaling()
{
//  Vec voxelScaling = Global::voxelScaling();
//  m_boundingBox.setBounds(VECPRODUCT(m_dataMin,voxelScaling),
//			  VECPRODUCT(m_dataMax,voxelScaling));
}

void
DrawLowresVolume::createShaders()
{ 
  QString shaderString;
  int nvol = 1;
  if (Global::volumeType() == Global::DoubleVolume) nvol = 2;
  if (Global::volumeType() == Global::TripleVolume) nvol = 3;
  if (Global::volumeType() == Global::QuadVolume) nvol = 4;
  
  // no emissivity in lowres window
  if (Global::volumeType() == Global::RGBVolume)
    shaderString = ShaderFactoryRGB::genDefaultShaderString(false, true);
  else if (Global::volumeType() == Global::RGBAVolume)
    shaderString = ShaderFactoryRGB::genDefaultShaderString(false, true);
  else 
    shaderString = ShaderFactory::genDefaultShaderString((m_Volume->pvlVoxelType(0) > 0), // 16-bit data
							 false,
							 nvol);

  m_progObj = glCreateProgramObjectARB();
  if (! ShaderFactory::loadShader(m_progObj,
				  shaderString))
    exit(0);
  m_parm[0] = glGetUniformLocationARB(m_progObj, "lutTex");
  m_parm[1] = glGetUniformLocationARB(m_progObj, "dataTex");

  if (Global::volumeType() != Global::RGBVolume &&
      Global::volumeType() != Global::RGBAVolume)
    m_parm[2] = glGetUniformLocationARB(m_progObj, "tfSet");
  else
    m_parm[2] = glGetUniformLocationARB(m_progObj, "layerSpacing");

  m_parm[3] = glGetUniformLocationARB(m_progObj, "delta");
}
//--------------------------------------------------------

void
DrawLowresVolume::enableTextureUnits()
{
  m_Viewer->enableTextureUnits();
  glActiveTexture(GL_TEXTURE1);
  glEnable(GL_TEXTURE_3D);  
}

void
DrawLowresVolume::disableTextureUnits()
{
  m_Viewer->disableTextureUnits();

  glActiveTexture(GL_TEXTURE4);
  glDisable(GL_TEXTURE_RECTANGLE_ARB);

  glActiveTexture(GL_TEXTURE1);
  glDisable(GL_TEXTURE_3D);
}

void
DrawLowresVolume::draw(float stepsize,
		       int posx, int posy)
{
  m_boundingBox.draw();

  Vec pn;
  Vec minvert, maxvert;
  int maxidx, layers;
  float maxlen, zdepth;

  
  pn = m_Viewer->camera()->viewDirection();


  StaticFunctions::getMinMaxBrickVertices(m_Viewer->camera(),
					  m_dataMin, m_dataMax,
					  zdepth,
					  minvert, maxvert,
					  maxidx);
  maxlen = zdepth;
  layers = maxlen/stepsize;

  disableTextureUnits();
  glUseProgramObjectARB(0);
  glLineWidth(2);
  glColor4f(0.9f, 0.9f, 0.9f, 0.9f);
  glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
  StaticFunctions::drawEnclosingCube(m_dataMin, m_dataMax);
  glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);

  glLineWidth(1);


  if (!m_progObj)
    createShaders();

  //if (m_progObj > 0)
    {
      glUseProgramObjectARB(m_progObj);

      glUniform1iARB(m_parm[0], 0);
      glUniform1iARB(m_parm[1], 1);

      if (Global::volumeType() == Global::RGBVolume ||
	  Global::volumeType() == Global::RGBAVolume)
	{
	  float frc = Global::stepsizeStill();
	  glUniform1fARB(m_parm[2], frc);
	}

      Vec textureSize = m_Volume->getLowresTextureVolumeSize();
      Vec delta = Vec(1.0/textureSize.x,
		      1.0/textureSize.y,
		      1.0/textureSize.z);
      glUniform3fARB(m_parm[3], delta.x, delta.y, delta.z);


      glEnable(GL_BLEND);
      glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

      enableTextureUnits();

      drawSlices(pn, maxvert, minvert,
		 layers, stepsize);

      disableTextureUnits();

      glUseProgramObjectARB(0);

      glDisable(GL_BLEND);
    }

  Vec bbmin, bbmax;
  int minX, maxX, minY, maxY, minZ, maxZ;

  Vec textureSize = m_Volume->getLowresTextureVolumeSize();

  m_boundingBox.bounds(bbmin, bbmax);

  minX = bbmin.x;
  maxX = bbmax.x;
  minY = bbmin.y;
  maxY = bbmax.y;
  minZ = bbmin.z;
  maxZ = bbmax.z;

  Vec vscl = Global::voxelScaling();

  m_Viewer->startScreenCoordinatesSystem();

  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // blend on top

  int screenWidth = m_Viewer->size().width();
  int screenHeight = m_Viewer->size().height();
  glColor4f(0, 0, 0, 0.8f);
  glBegin(GL_QUADS);
  glVertex2f(0, posy-45);
  glVertex2f(0, screenHeight);
  glVertex2f(screenWidth, screenHeight);
  glVertex2f(screenWidth, posy-45);
  glEnd();

  m_Viewer->stopScreenCoordinatesSystem();

  //-------------
  // calculate font scale based on dpi
  float fscl = 120.0/Global::dpi();
  //-------------

  glDisable(GL_DEPTH_TEST);
  QFont tfont = QFont("Helvetica", 8*fscl);
  tfont.setStyleStrategy(QFont::PreferAntialias);  

  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_TEXTURE_2D);

  glColor3f(0.6f,0.6f,0.6f);
  m_Viewer->drawText(posx, posy-33,
		     QString("LowRes : stepsize = %1 : %2 %3 %4").\
		     arg(stepsize).\
		     arg(vscl.x).arg(vscl.y).arg(vscl.z),
		     tfont);

  tfont.setPointSize(10*fscl);
  glColor3f(0.8f,0.8f,0.8f);
  m_Viewer->drawText(posx, posy-18,
		     QString("Bounds : %1-%2, %3-%4, %5-%6").		\
		     arg(minX).arg(maxX).arg(minY).arg(maxY).arg(minZ).arg(maxZ),
		     tfont);
  tfont.setPointSize(12*fscl);
  glColor3f(1,1,1);
  m_Viewer->drawText(posx, posy,
		     QString("Selected SubVolume Size : %1x%2x%3 - (%4x%5x%6)"). \
		     arg(maxX-minX+1).arg(maxY-minY+1).arg(maxZ-minZ+1). \
		     arg((int)textureSize.x).\
		     arg((int)textureSize.y).\
		     arg((int)textureSize.z),
		     tfont);

  glDisable(GL_TEXTURE_2D);
}

int
DrawLowresVolume::drawpoly(Vec po, Vec pn,
			   Vec *subvol,
			   Vec subdim, Vec subcorner)
{
  Vec poly[100];
  int edges = 0;

  edges += StaticFunctions::intersectType1(po, pn,  subvol[0], subvol[1], poly[edges]);
  edges += StaticFunctions::intersectType1(po, pn,  subvol[0], subvol[3], poly[edges]);
  edges += StaticFunctions::intersectType1(po, pn,  subvol[2], subvol[1], poly[edges]);
  edges += StaticFunctions::intersectType1(po, pn,  subvol[2], subvol[3], poly[edges]);
  edges += StaticFunctions::intersectType1(po, pn,  subvol[4], subvol[5], poly[edges]);
  edges += StaticFunctions::intersectType1(po, pn,  subvol[4], subvol[7], poly[edges]);
  edges += StaticFunctions::intersectType1(po, pn,  subvol[6], subvol[5], poly[edges]);
  edges += StaticFunctions::intersectType1(po, pn,  subvol[6], subvol[7], poly[edges]);
  edges += StaticFunctions::intersectType1(po, pn,  subvol[0], subvol[4], poly[edges]);
  edges += StaticFunctions::intersectType1(po, pn,  subvol[1], subvol[5], poly[edges]);
  edges += StaticFunctions::intersectType1(po, pn,  subvol[2], subvol[6], poly[edges]);
  edges += StaticFunctions::intersectType1(po, pn,  subvol[3], subvol[7], poly[edges]);

  if (!edges) return 0;

  Vec cen;
  int i;
  for(i=0; i<edges; i++)
    cen += poly[i];
  cen/=edges;

  float angle[6];
  Vec vaxis, vperp;
  vaxis = poly[0]-cen;
  vaxis.normalize();

  vperp = vaxis^(poly[1]-cen);
  vperp = vperp^vaxis;
  vperp.normalize();

  angle[0] = 1;
  for(i=1; i<edges; i++)
    {
      Vec v;
      v = poly[i]-cen;
      v.normalize();
      angle[i] = vaxis*v;
      if (vperp*v < 0)
	angle[i] = -2 - angle[i];
    }

  // sort angle
  int order[] = {0, 1, 2, 3, 4, 5 };
  for(i=edges-1; i>=0; i--)
    for(int j=1; j<=i; j++)
      {
	if (angle[order[i]] < angle[order[j]])
	  {
	    int tmp = order[i];
	    order[i] = order[j];
	    order[j] = tmp;
	  }
      }

  //---- apply bounding box limits
  int tedges;
  Vec tpoly[100];

  for(i=0; i<edges; i++)
    tpoly[i] = poly[order[i]];
  for(i=0; i<edges; i++)
    poly[i] = tpoly[i];

  Vec bbmin, bbmax;
  m_boundingBox.bounds(bbmin, bbmax);
  for (int bp=0; bp<6; bp++)
    {      
      Vec cpo, cpn;
      switch (bp)
	{
	case 0 :
	  {
	    cpo = Vec(bbmin.x, 0, 0);
	    cpn = Vec(-1, 0, 0);
	    break;
	  }
	case 1 :
	  {
	    cpo = Vec(bbmax.x, 0, 0);
	    cpn = Vec(1, 0, 0);
	    break;
	  
	  }
	case 2 :
	  {
	    cpo = Vec(0, bbmin.y, 0);
	    cpn = Vec(0, -1, 0);
	    break;
	  }
	case 3 :
	  {
	    cpo = Vec(0, bbmax.y, 0);
	    cpn = Vec(0, 1, 0);
	    break;
	  
	  }
	case 4 :
	  {
	    cpo = Vec(0, 0, bbmin.z);
	    cpn = Vec(0, 0, -1);
	    break;
	  }
	case 5 :
	  {
	    cpo = Vec(0, 0, bbmax.z);
	    cpn = Vec(0, 0, 1);
	    break;
	  
	  }
	}
      
      tedges = 0;
      for(i=0; i<edges; i++)
	{
	  Vec v0, v1, Rnew;
	  Vec Rd;
		  
	  v0 = poly[i];
	  if (i<edges-1)
	    v1 = poly[i+1];
	  else
	    v1 = poly[0];
	      
	  Rd = v0-v1;
	  Rd.normalize();


	  int ret = StaticFunctions::intersectType2(cpo, cpn, v0, v1);
	  if (ret)
	    {
	      tpoly[tedges] = v0;
	      tedges ++;
	      if (ret == 2)
		{
		  tpoly[tedges] = v1;
		  tedges ++;
		}
	    }
	}
      edges = tedges;
      for(i=0; i<tedges; i++)
	poly[i] = tpoly[i];	  

    }
  //---- clipping applied


  glBegin(GL_POLYGON);
  for(i=0; i<edges; i++)
    {  
      Vec tx, tc;
      tx = poly[i]-subcorner;
      tc = VECDIVIDE(tx,subdim);

      glTexCoord3f(tc.x, tc.y, tc.z);
      glVertex3f(poly[i].x, poly[i].y, poly[i].z);
    }
  glEnd();


  return 1;
}

void
DrawLowresVolume::drawSlices(Vec pn, Vec minvert, Vec maxvert,
			     int layers, float stepsize)
{
  float xmin, xmax, ymin, ymax, zmin, zmax;
  Vec subdim, subcorner, subvol[8];

  xmin = m_dataMin.x + 1;
  ymin = m_dataMin.y + 1;
  zmin = m_dataMin.z + 1;
  xmax = m_dataMax.x - 1;
  ymax = m_dataMax.y - 1;
  zmax = m_dataMax.z - 1;

  subvol[0] = Vec(xmin, ymin, zmin);
  subvol[1] = Vec(xmax, ymin, zmin);
  subvol[2] = Vec(xmax, ymax, zmin);
  subvol[3] = Vec(xmin, ymax, zmin);
  subvol[4] = Vec(xmin, ymin, zmax);
  subvol[5] = Vec(xmax, ymin, zmax);
  subvol[6] = Vec(xmax, ymax, zmax);
  subvol[7] = Vec(xmin, ymax, zmax);

  subcorner = m_virtualTextureMin,
     subdim = m_virtualTextureSize - Vec(1,1,1);

  Vec step = stepsize*pn;
  Vec po = minvert+layers*step;
  for(int s=0; s<layers; s++)
    {
      po -= step;
      drawpoly(po, pn, subvol, subdim, subcorner);
    }

}

bool
DrawLowresVolume::keyPressEvent(QKeyEvent *event)
{
  return m_boundingBox.keyPressEvent(event);
}

void
DrawLowresVolume::load(const char *flnm)
{
  QDomDocument document;
  QFile f(flnm);
  if (f.open(QIODevice::ReadOnly))
    {
      document.setContent(&f);
      f.close();
    }

  QDomElement main = document.documentElement();
  QDomNodeList dlist = main.childNodes();
  Vec svmin = m_dataMin;
  Vec svmax = m_dataMax;
  for(int i=0; i<dlist.count(); i++)
    {
      if (dlist.at(i).nodeName() == "volmin")
	{
	  QString str = dlist.at(i).toElement().text();
	  m_dataMin = StaticFunctions::getVec(str);
	}
      else if (dlist.at(i).nodeName() == "volmax")
	{
	  QString str = dlist.at(i).toElement().text();
	  m_dataMax = StaticFunctions::getVec(str);
	}
      else if (dlist.at(i).nodeName() == "subvolmin")
	{
	  QString str = dlist.at(i).toElement().text();
	  svmin = StaticFunctions::getVec(str);
	}
      else if (dlist.at(i).nodeName() == "subvolmax")
	{
	  QString str = dlist.at(i).toElement().text();
	  svmax = StaticFunctions::getVec(str);
	}
    }

//  Vec voxelScaling = Global::voxelScaling();
//  m_boundingBox.setBounds(VECPRODUCT(m_dataMin,voxelScaling),
//			  VECPRODUCT(m_dataMax,voxelScaling));
  m_boundingBox.setBounds(m_dataMin, m_dataMax);
//  svmin = VECPRODUCT(svmin, voxelScaling);
//  svmax = VECPRODUCT(svmax, voxelScaling);
  m_boundingBox.setPositions(svmin, svmax);
}


void
DrawLowresVolume::save(const char *flnm)
{
  QDomDocument doc;
  QFile fin(flnm);
  if (fin.open(QIODevice::ReadOnly))
    {
      doc.setContent(&fin);
      fin.close();
    }
  QDomElement topElement = doc.documentElement();

  QString str;

  {
    QDomElement de0 = doc.createElement("volmin");
    if (Global::volumeType() != Global::DummyVolume)
      str = QString("%1 %2 %3").		\
  	    arg(m_dataMin.x).\
  	    arg(m_dataMin.y).\
            arg(m_dataMin.z);
    else
      str = QString("%1 %2 %3").		\
  	    arg(m_dataMin.z).\
  	    arg(m_dataMin.y).\
            arg(m_dataMin.x);
    QDomText tn0 = doc.createTextNode(str);
    de0.appendChild(tn0);
    topElement.appendChild(de0);

    QDomElement de1 = doc.createElement("volmax");
    if (Global::volumeType() != Global::DummyVolume)
      str = QString("%1 %2 %3").		\
  	    arg(m_dataMax.x).\
  	    arg(m_dataMax.y).\
  	    arg(m_dataMax.z);
    else
      str = QString("%1 %2 %3").		\
  	    arg(m_dataMax.z).\
  	    arg(m_dataMax.y).\
  	    arg(m_dataMax.x);
    QDomText tn1 = doc.createTextNode(str);
    de1.appendChild(tn1);
    topElement.appendChild(de1);
  }
  {
    Vec bbmin, bbmax;
    m_boundingBox.bounds(bbmin, bbmax);

    QDomElement de0 = doc.createElement("subvolmin");
    if (Global::volumeType() != Global::DummyVolume)
      str = QString("%1 %2 %3").		\
            arg(bbmin.x).\
            arg(bbmin.y).\
            arg(bbmin.z);
    else
      str = QString("%1 %2 %3").		\
            arg(bbmin.z).\
            arg(bbmin.y).\
            arg(bbmin.x);
    QDomText tn0 = doc.createTextNode(str);
    de0.appendChild(tn0);
    topElement.appendChild(de0);

    QDomElement de1 = doc.createElement("subvolmax");
    if (Global::volumeType() != Global::DummyVolume)
      str = QString("%1 %2 %3").		\
  	  arg(bbmax.x).\
  	  arg(bbmax.y).\
  	  arg(bbmax.z);
    else
      str = QString("%1 %2 %3").		\
  	  arg(bbmax.z).\
  	  arg(bbmax.y).\
  	  arg(bbmax.x);
    QDomText tn1 = doc.createTextNode(str);
    de1.appendChild(tn1);
    topElement.appendChild(de1);
  }

  QFile fout(flnm);
  if (fout.open(QIODevice::WriteOnly))
    {
      QTextStream out(&fout);
      doc.save(out, 2);
      fout.close();
    }
}

void
DrawLowresVolume::switchInterpolation()
{
  glActiveTexture(GL_TEXTURE1);
  if (Global::interpolationType(Global::TextureInterpolation)) // linear
    {
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
  else
    {
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
}
