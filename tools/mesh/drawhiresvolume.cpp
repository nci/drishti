#include <stdlib.h>
#include "drawhiresvolume.h"
#include "viewer.h"
#include "staticfunctions.h"
#include "matrix.h"
#include "global.h"
#include "geometryobjects.h"
#include "enums.h"
#include "mainwindowui.h"

#include <QInputDialog>
#include <QFileDialog>
#include <QDataStream>

double* DrawHiresVolume::brick0Xform() { return m_bricks->getMatrix(); }

bool DrawHiresVolume::raised() { return m_showing; }
void DrawHiresVolume::lower() { m_showing = false; }
void DrawHiresVolume::raise() { m_showing = true; }
void DrawHiresVolume::setBricks(Bricks *bricks) { m_bricks = bricks; }

Vec DrawHiresVolume::volumeMin() { return m_dataMin; }
Vec DrawHiresVolume::volumeMax() { return m_dataMax; }

LightingInformation DrawHiresVolume::lightInfo() { return m_lightInfo; }
QList<BrickInformation> DrawHiresVolume::bricks() { return m_bricks->bricks(); }

void
DrawHiresVolume::loadVolume()
{
  m_bricks->reset();
  GeometryObjects::clipplanes()->reset();
}

// only called when we have not volume data
void
DrawHiresVolume::overwriteDataMinMax(Vec smin, Vec smax)
{
  m_dataMin = smin;
  m_dataMax = smax;
  GeometryObjects::clipplanes()->setBounds(m_dataMin, m_dataMax);
  Global::setBounds(m_dataMin, m_dataMax);
  m_bricks->setBounds(m_dataMin, m_dataMax);
}
void
DrawHiresVolume::setBrickBounds(Vec smin, Vec smax)
{
  m_bricks->setBounds(smin, smax);
}


//DrawHiresVolume::DrawHiresVolume(Viewer *viewer,
//				 Volume *volume) :
DrawHiresVolume::DrawHiresVolume(Viewer *viewer) :
  QObject()
{
  m_Viewer = viewer;

  renew();
}

DrawHiresVolume::~DrawHiresVolume()
{
  cleanup();
}

void
DrawHiresVolume::renew()
{
  m_focalPoint = 0.0;
  m_dofBlur = 0;

  m_frontOpMod = 1.0;
  m_backOpMod = 1.0;
  m_saveImage2Volume = false;

  m_showing = true;
  m_forceBackToFront = false;

  m_loadingData = false;

  m_currentVolume = 0;

  m_virtualTextureSize = m_virtualTextureMin = m_virtualTextureMax = Vec(0,0,0);
  m_dataMin = m_dataMax = m_dataSize = Vec(0,0,0);

  m_backlit = false;  

  m_imgSizeRatio = 1;

  m_renderQuality = Enums::RenderDefault;

  m_paths.clear();

  m_updateSubvolume = true;
}

void
DrawHiresVolume::cleanup()
{
  m_dataMin = m_dataMax = m_dataSize = Vec(0,0,0);

  m_paths.clear();
}


void
DrawHiresVolume::updateScaling()
{
  m_bricks->updateScaling();
}


void DrawHiresVolume::setImageSizeRatio(float ratio) { m_imgSizeRatio = ratio; }


void
DrawHiresVolume::drawDragImage()
{
  draw(false);
}

void
DrawHiresVolume::drawStillImage()
{
  draw(true);
}

void
DrawHiresVolume::loadCameraMatrices()
{
  if (Global::saveImageType() == Global::NoImage)
    {
      if (m_Viewer->displaysInStereo() == false)
	{
	  m_Viewer->camera()->loadProjectionMatrix(true);
	  m_Viewer->camera()->loadModelViewMatrix(true);
	}
      else
	{
	  m_Viewer->camera()->setFocusDistance(m_focusDistance);
	  m_Viewer->camera()->setPhysicalScreenWidth(m_screenWidth);

	  //if (m_currentDrawbuf == GL_BACK_LEFT)
	  if (Global::saveImageType() == Global::LeftImage ||
	      Global::saveImageType() == Global::LeftImageAnaglyph)
	    {
	      m_Viewer->camera()->loadProjectionMatrixStereo(false);
	      m_Viewer->camera()->loadModelViewMatrixStereo(false);
	    }
	  else
	    {
	      m_Viewer->camera()->loadProjectionMatrixStereo(true);
	      m_Viewer->camera()->loadModelViewMatrixStereo(true);
	    }
	}
    }
  else
    {
      if (Global::saveImageType() == Global::MonoImage)
	{
	  m_Viewer->camera()->loadProjectionMatrix(true);
	  m_Viewer->camera()->loadModelViewMatrix(true);
	}
      else if (Global::saveImageType() == Global::RightImage ||
	       Global::saveImageType() == Global::LeftImage ||
	       Global::saveImageType() == Global::RightImageAnaglyph ||
	       Global::saveImageType() == Global::LeftImageAnaglyph)
	{
	  // Stereo Images

	  m_Viewer->camera()->setFocusDistance(m_focusDistance);
	  m_Viewer->camera()->setPhysicalScreenWidth(m_screenWidth);

	  if (Global::saveImageType() == Global::LeftImage ||
	      Global::saveImageType() == Global::LeftImageAnaglyph)
	    {
	      m_Viewer->camera()->loadProjectionMatrixStereo(false);
	      m_Viewer->camera()->loadModelViewMatrixStereo(false);
	    }
	  else if (Global::saveImageType() == Global::RightImage ||
		   Global::saveImageType() == Global::RightImageAnaglyph)
	    {
	      m_Viewer->camera()->loadProjectionMatrixStereo(true);
	      m_Viewer->camera()->loadModelViewMatrixStereo(true);
	    }
	}
      else
	{
	  // Cubic Images (or Pano)
	  m_Viewer->camera()->loadProjectionMatrix(true);
	  m_Viewer->camera()->loadModelViewMatrix(true);
	}
    }
}

void
DrawHiresVolume::draw(bool stillimage)
{  
//  if (MainWindowUI::mainWindowUI()->actionRedBlue->isChecked() ||
//      MainWindowUI::mainWindowUI()->actionRedCyan->isChecked())
//    {
//      if (Global::saveImageType() == Global::LeftImageAnaglyph)
//	{
//	  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
//	  glClearColor(0, 0, 0, 0);
//	  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//
//	  glColorMask(GL_TRUE, GL_FALSE, GL_FALSE, GL_TRUE);
//	}
//      else // right image
//	{
//	  glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);
//	  glClearColor(0, 0, 0, 0);
//	  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//	  
//	  
//	  if (MainWindowUI::mainWindowUI()->actionRedBlue->isChecked())
//	    glColorMask(GL_FALSE, GL_FALSE, GL_TRUE, GL_TRUE);
//	  else
//	    glColorMask(GL_FALSE, GL_TRUE, GL_TRUE, GL_TRUE);
//	}
//    }

  m_focusDistance = m_Viewer->camera()->focusDistance();
  m_screenWidth = m_Viewer->camera()->physicalScreenWidth();
  m_cameraWidth = m_Viewer->camera()->screenWidth();
  m_cameraHeight = m_Viewer->camera()->screenHeight();

  glGetIntegerv(GL_DRAW_BUFFER, &m_currentDrawbuf);

  glDepthMask(GL_TRUE);
  glEnable(GL_ALPHA_TEST);
  glAlphaFunc(GL_GREATER, 0);

  collectEnclosingBoxInfo();

  // light direction and position is different for cubic images
  // we want to keep same direction and position for all cube face images
  if ( Global::saveImageType() >= Global::CubicFrontImage)
    m_lightVector = m_Viewer->camera()->viewDirection();
  else
    m_lightVector = (m_lightInfo.userLightVector.x * m_Viewer->camera()->rightVector() +
		     m_lightInfo.userLightVector.y * m_Viewer->camera()->upVector() +
		     m_lightInfo.userLightVector.z * m_Viewer->camera()->viewDirection());
  float camdist = m_Viewer->camera()->sceneRadius();

  if ( Global::saveImageType() >= Global::CubicFrontImage)
    m_lightPosition = m_Viewer->camera()->position();
  else
    m_lightPosition = (m_Viewer->camera()->sceneCenter() -
		       (3.0f + m_lightInfo.lightDistanceOffset)*camdist*m_lightVector);  


  loadCameraMatrices();

  

  //----------------------
  if (m_Viewer->imageBuffer()->isBound() &&
      (MainWindowUI::mainWindowUI()->actionFor3DTV->isChecked() ||
       MainWindowUI::mainWindowUI()->actionCrosseye->isChecked()))
    {
      if (MainWindowUI::mainWindowUI()->actionFor3DTV->isChecked())
	{
	  if (Global::saveImageType() == Global::LeftImage)
	    {
	      glClear(GL_DEPTH_BUFFER_BIT);
	      glClearColor(0, 0, 0, 0);
	      glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
	      glClear(GL_COLOR_BUFFER_BIT);
	    }
	  else
	    {
	    }
	}
      else
	{
	  if (MainWindowUI::mainWindowUI()->actionCrosseye->isChecked() &&
	      Global::saveImageType() == Global::LeftImage)
	    {
	    }
	  else
	    {
	      glClear(GL_DEPTH_BUFFER_BIT);
	      glClearColor(0, 0, 0, 0);
	      glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
	      glClear(GL_COLOR_BUFFER_BIT);
	    }
	}
    }
  //----------------------
      
  if (Global::visualizationMode() == Global::Surfaces)
    drawGeometryOnly();
}


void
DrawHiresVolume::drawAxisText()
{
  glEnable(GL_DEPTH_TEST);
  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_TEXTURE_2D);

  glColor3f(1,1,1);

  //-------------
  // calculate font scale based on dpi
  float fscl = 120.0/Global::dpi();
  //-------------

  QFont tfont = QFont("Helvetica", 18*fscl);
  tfont.setStyleStrategy(QFont::PreferAntialias);
  tfont.setBold(true);

  Vec posX, posY, posZ;
  posX = m_enclosingBox[0] + m_axisArrow[0];
  m_Viewer->renderText(posX.x, posX.y, posX.z,
		       QString("X"),
		       tfont);
  posY = m_enclosingBox[0] + m_axisArrow[1];
  m_Viewer->renderText(posY.x, posY.y, posY.z,
		       QString("Y"),
		       tfont);
  posZ = m_enclosingBox[0] + m_axisArrow[2];
  m_Viewer->renderText(posZ.x, posZ.y, posZ.z,
		       QString("Z"),
		       tfont);
  
  
  tfont.setBold(false);
  glColor3f(0,0,0);
  m_Viewer->renderText(posX.x, posX.y, posX.z,
		       QString("X"),
		       tfont);
  m_Viewer->renderText(posY.x, posY.y, posY.z,
		       QString("Y"),
		       tfont);
  m_Viewer->renderText(posZ.x, posZ.y, posZ.z,
		       QString("Z"),
		       tfont);

  glDisable(GL_DEPTH_TEST);
  glActiveTexture(GL_TEXTURE0);
  glDisable(GL_TEXTURE_2D);
}


void
DrawHiresVolume::collectEnclosingBoxInfo()
{
  Vec voxelScaling = Global::voxelScaling();
  Vec bmin = VECPRODUCT(m_dataMin,voxelScaling);
  Vec bmax = VECPRODUCT(m_dataMax,voxelScaling);

  double *Xform = m_bricks->getMatrix(0);

  m_enclosingBox[0] = Vec(bmin.x, bmin.y, bmin.z);
  m_enclosingBox[1] = Vec(bmax.x, bmin.y, bmin.z);
  m_enclosingBox[2] = Vec(bmax.x, bmax.y, bmin.z);
  m_enclosingBox[3] = Vec(bmin.x, bmax.y, bmin.z);
  m_enclosingBox[4] = Vec(bmin.x, bmin.y, bmax.z);
  m_enclosingBox[5] = Vec(bmax.x, bmin.y, bmax.z);
  m_enclosingBox[6] = Vec(bmax.x, bmax.y, bmax.z);
  m_enclosingBox[7] = Vec(bmin.x, bmax.y, bmax.z);

  for (int i=0; i<8; i++)
    m_enclosingBox[i] = Matrix::xformVec(Xform, m_enclosingBox[i]);

  m_axisArrow[0] =  m_enclosingBox[1] - m_enclosingBox[0];
  m_axisArrow[1] =  m_enclosingBox[3] - m_enclosingBox[0];
  m_axisArrow[2] =  m_enclosingBox[4] - m_enclosingBox[0];

  m_axisArrow[0] *= 0.5;
  m_axisArrow[1] *= 0.5;
  m_axisArrow[2] *= 0.5;
}

void
DrawHiresVolume::drawGeometry()
{
  if (Global::volumeType() == Global::DummyVolume)
    {
      GLfloat MV[16];
      m_Viewer->camera()->getModelViewMatrix(MV);
      GLdouble MVP[16];
      m_Viewer->camera()->getModelViewProjectionMatrix(MVP);
      Vec viewDir = m_Viewer->camera()->viewDirection();
      int scrW = m_Viewer->camera()->screenWidth();
      int scrH = m_Viewer->camera()->screenHeight();


      Camera::Type ctype = m_Viewer->camera()->type();

      //---------------------
      // setting shadow camera
      Quaternion orot = m_Viewer->camera()->orientation();
      Vec opos = m_Viewer->camera()->position();
      Vec center = m_Viewer->camera()->sceneCenter();
      float distFromCam = (center-opos)*viewDir;
      Vec pos = (opos + distFromCam*viewDir);
      Vec offset = center - pos;

      Vec lightDir = GeometryObjects::trisets()->lightDirection();

      //----
      // no shadows if lightdirection is too far off to a side
      if (qAbs(lightDir.x) < 0.9 &&
	  qAbs(lightDir.y) < 0.9)
	{
	  Quaternion rot = orot * Quaternion(Vec(1,0,0), DEG2RAD(lightDir.y*90))
	                        * Quaternion(Vec(0,1,0),-DEG2RAD(lightDir.x*90));
	  m_Viewer->camera()->setOrientation(rot);
	  // set shadow camera to be orthographic
	  // meaning the light is directional
	  m_Viewer->camera()->setType(Camera::ORTHOGRAPHIC);
	  m_Viewer->camera()->loadProjectionMatrix(true);
	  // now reposition the camera so that it faces the scene
	}
      //----
      Vec shadowViewDir = m_Viewer->camera()->viewDirection();
      Vec shadowCam = center - distFromCam*shadowViewDir - offset;
      m_Viewer->camera()->setPosition(shadowCam);
      m_Viewer->camera()->loadModelViewMatrix(true);
      //---------------------
      

      GLdouble shadowMVP[16];
      m_Viewer->camera()->getModelViewProjectionMatrix(shadowMVP);

      //---------------------
      // restore camera
      m_Viewer->camera()->setOrientation(orot);
      m_Viewer->camera()->setPosition(opos);
      m_Viewer->camera()->setType(ctype);
      m_Viewer->camera()->loadModelViewMatrix(true);
      m_Viewer->camera()->loadProjectionMatrix(true);
      //---------------------

      
      GeometryObjects::trisets()->draw(m_Viewer,
				       MVP, MV, viewDir, 
				       shadowMVP, shadowCam, shadowViewDir,
				       scrW, scrH,
				       m_clipPos, m_clipNormal);

    }

  glLineWidth(1.5);

  Vec lineColor = Vec(0.9, 0.9, 0.9);

  Vec bgcolor = Global::backgroundColor();
  float bgintensity = (0.3*bgcolor.x +
		       0.5*bgcolor.y +
		       0.2*bgcolor.z);
  if (bgintensity > 0.5)
    lineColor = Vec(0, 0, 0);

  
  if (Global::volumeType() == Global::DummyVolume)
    {
      glDisable(GL_DEPTH_TEST);
      glBlendFunc(GL_ONE, GL_ONE);      
      m_bricks->draw();
      glEnable(GL_DEPTH_TEST);
      glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    }
  

  if (Global::drawBox())
    StaticFunctions::drawEnclosingCube(m_enclosingBox,
				       lineColor);
  

  if (Global::drawAxis())
    StaticFunctions::drawAxis(m_enclosingBox[0],
			      m_axisArrow[0],
			      m_axisArrow[1],
			      m_axisArrow[2]);

  //-----------------------------------------

  // draw clipplanes before applying brick0 transformations
//  glDisable(GL_DEPTH_TEST);
//  glBlendFunc(GL_ONE, GL_ONE);
  GeometryObjects::clipplanes()->draw(m_Viewer, m_backlit);
//  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // blend on top
//  glEnable(GL_DEPTH_TEST);

  //----- apply brick0 transformation -------
  Vec voxelScaling = Global::voxelScaling();
  Vec pos = VECPRODUCT(m_bricks->getTranslation(0), voxelScaling);
  Vec pivot = VECPRODUCT(m_bricks->getPivot(0),voxelScaling);
  Vec axis = m_bricks->getAxis(0);
  float angle = m_bricks->getAngle(0);

  glPushMatrix();

  glTranslatef(pos.x, pos.y, pos.z);
    
  glTranslatef(pivot.x, pivot.y, pivot.z);
  glRotatef(angle,
	    axis.x,
	    axis.y,
	    axis.z);
  glTranslatef(-pivot.x, -pivot.y, -pivot.z);
  //-----------------------------------------


  Vec pn = m_Viewer->camera()->viewDirection(); // slicing direction
  GeometryObjects::paths()->draw(m_Viewer, pn, 1, -1, m_backlit, m_lightPosition);
  GeometryObjects::hitpoints()->draw();
  GeometryObjects::trisets()->drawHitPoints();
  

  //-----------------------------------------
  //----- apply brick0 transformation -------
  glPopMatrix();
  //-----------------------------------------

  glLineWidth(1);
}

void
DrawHiresVolume::drawGeometryOnly()
{

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);

//  glClearDepth(1);
//  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  Vec voxelScaling = Global::voxelScaling();
  
  m_clipPos = GeometryObjects::clipplanes()->positions();
  m_clipNormal = GeometryObjects::clipplanes()->normals();
  for(int c=0; c<m_clipPos.count(); c++)
    {
      Vec pos = m_clipPos[c];
      pos = VECPRODUCT(pos, voxelScaling);
      pos = Matrix::xformVec(brick0Xform(), pos);
      Vec nrm = m_clipNormal[c];
      nrm = Matrix::rotateVec(brick0Xform(), nrm);

      m_clipPos[c] = pos;
      m_clipNormal[c]= nrm;
    }

  QVector4D lighting = QVector4D(m_lightInfo.highlights.ambient,
				 m_lightInfo.highlights.diffuse,
				 m_lightInfo.highlights.specular,
				 m_lightInfo.highlights.specularCoefficient);
  GeometryObjects::trisets()->setLighting(lighting);
  GeometryObjects::trisets()->setShapeEnhancements(m_lightInfo.softness,
						   m_lightInfo.edges,
						   m_lightInfo.shadowIntensity,
						   m_lightInfo.valleyIntensity,
						   m_lightInfo.peakIntensity); 
  GeometryObjects::trisets()->setLightDirection(m_lightInfo.userLightVector);
  GeometryObjects::trisets()->predraw(m_bricks->getMatrix(0));

  drawGeometry();
  

  drawBackground();
  
  glUseProgramObjectARB(0);

  if (Global::drawAxis())
    drawAxisText();

  GeometryObjects::clipplanes()->postdraw((QGLViewer*)m_Viewer);

  glEnable(GL_DEPTH_TEST);
}


void
DrawHiresVolume::setLightInfo(LightingInformation lightInfo)
{
  m_lightInfo = lightInfo;
}


void
DrawHiresVolume::updateLightVector(Vec dir)
{
  //m_lightInfo.userLightVector = dir.unit();

  // just apply linear scaling to x & y components
  // we want more details near the center
  Vec ut = Vec(dir.x*0.5, dir.y*0.5, dir.z);
  ut = ut.unit();
  m_lightInfo.userLightVector = ut;
}
void DrawHiresVolume::updateLightDistanceOffset(float val) { m_lightInfo.lightDistanceOffset = val; }
void DrawHiresVolume::updateShadowFOV(float val){ m_lightInfo.shadowFovOffset = val; }
void DrawHiresVolume::updateHighlights(Highlights highlights)
{
  m_lightInfo.highlights = highlights;
}

void DrawHiresVolume::applyBackplane(bool flag) { m_lightInfo.applyBackplane = flag; }
void DrawHiresVolume::updateBackplaneShadowScale(float val) { m_lightInfo.backplaneShadowScale = val; }

void
DrawHiresVolume::updateSoftness(float val)
{
  m_lightInfo.softness = val;
}
void DrawHiresVolume::updateEdges(float val)
{
  m_lightInfo.edges = val;
}
void DrawHiresVolume::updateShadowIntensity(float val)
{
  m_lightInfo.shadowIntensity = val;
}
void DrawHiresVolume::updateValleyIntensity(float val)
{
  m_lightInfo.valleyIntensity = val;
}
void DrawHiresVolume::updatePeakIntensity(float val)
{
  m_lightInfo.peakIntensity = val;
}


void
DrawHiresVolume::updateBackplaneIntensity(float val)
{
  m_lightInfo.backplaneIntensity = val;
}


bool
DrawHiresVolume::keyPressEvent(QKeyEvent *event)
{
  // process brick events
  if (m_bricks->keyPressEvent(event))
    return true;

//  // process clipplane events
//  if (GeometryObjects::clipplanes()->keyPressEvent(event))
//    return true;

  if (event->modifiers() != Qt::NoModifier)
    return false;

//  if (event->key() == Qt::Key_G)
//    {
//      // toggle mouse grabs for geometry objects
//      GeometryObjects::inPool = ! GeometryObjects::inPool;
//      if (GeometryObjects::inPool)
//	GeometryObjects::addInMouseGrabberPool();
//      else
//	GeometryObjects::removeFromMouseGrabberPool();
//    }
//
//  if (event->key() == Qt::Key_V)
//    {
//      GeometryObjects::showGeometry = ! GeometryObjects::showGeometry;
//      if (GeometryObjects::showGeometry)
//	GeometryObjects::show();
//      else
//	GeometryObjects::hide();
//
//      return true;
//    }

  
  return false;
}

void
DrawHiresVolume::drawBackground()
{
  // if we have already drawn a backplane return
  // without drawing the background
  if (m_renderQuality == Enums::RenderHighQuality &&
      m_lightInfo.applyShadows &&
      m_lightInfo.applyBackplane &&
      !m_backlit)
    return;

  glDepthMask(GL_FALSE); // disable writing to depth buffer


  int width = m_Viewer->camera()->screenWidth();
  int height = m_Viewer->camera()->screenHeight();

  if (MainWindowUI::mainWindowUI()->actionFor3DTV->isChecked())
    {
      //width /= 2;
      if (Global::saveImageType() == Global::LeftImage)
	StaticFunctions::pushOrthoView(0,0, width, height);
      else
	StaticFunctions::pushOrthoView(width, 0, width, height);
    }
  else
    {
      if (MainWindowUI::mainWindowUI()->actionCrosseye->isChecked() &&
	  Global::saveImageType() == Global::LeftImage)
	StaticFunctions::pushOrthoView(width, 0, width, height);
      else
	StaticFunctions::pushOrthoView(0,0, width, height);
    }

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);

  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE);

  glUseProgramObjectARB(0);

  QImage bgImage = Global::backgroundImage();
  if (bgImage.isNull())
    {
      if (Global::backgroundColor().norm() > 0 ||
	  m_lightInfo.applyBackplane)
	glColor3dv(Global::backgroundColor());
      else
	glColor4f(0,0,0,0);

      glBegin(GL_QUADS);
      glVertex3f(0,     0,      1);
      glVertex3f(width, 0,      1);
      
      glVertex3f(width, height, 1);
      glVertex3f(0,     height, 1);

      glEnd();
    }
  else
    {
      int ht = bgImage.height();
      int wd = bgImage.width();

      int nbytes = bgImage.byteCount();
      int rgb = nbytes/(wd*ht);

      GLuint fmt;
      if (rgb == 1) fmt = GL_LUMINANCE;
      else if (rgb == 2) fmt = GL_LUMINANCE_ALPHA;
      else if (rgb == 3) fmt = GL_RGB;
      else if (rgb == 4) fmt = GL_RGBA;

      glActiveTexture(GL_TEXTURE0);
      glEnable(GL_TEXTURE_RECTANGLE_ARB);
      glTexEnvf(GL_TEXTURE_ENV,
		GL_TEXTURE_ENV_MODE,
		GL_MODULATE);
      glBindTexture(GL_TEXTURE_RECTANGLE_ARB, Global::backgroundTexture());
      glTexParameterf(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
      glTexParameterf(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
      glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexImage2D(GL_TEXTURE_RECTANGLE_ARB,
		   0,
		   rgb,
		   wd,
		   ht,
		   0,
		   fmt,
		   GL_UNSIGNED_BYTE,
		   bgImage.bits());

      glColor3dv(Global::backgroundColor());

      glUseProgramObjectARB(Global::copyShader());
      glUniform1iARB(Global::copyParm(0), 0); // lutTex

      glBegin(GL_QUADS);

      glTexCoord2f(0, ht);
      glVertex3f(0,     0,      1);
      
      glTexCoord2f(wd, ht);
      glVertex3f(width, 0,      1);

      glTexCoord2f(wd, 0);
      glVertex3f(width, height, 1);

      glTexCoord2f(0, 0);
      glVertex3f(0,     height, 1);

      glEnd();

      glActiveTexture(GL_TEXTURE0);
      glDisable(GL_TEXTURE_RECTANGLE_ARB);
    }

  StaticFunctions::popOrthoView();

  glDepthMask(GL_TRUE); // enable writing to depth buffer

  glUseProgramObjectARB(0);
}


void
DrawHiresVolume::saveForDrishtiPrayog(QString sfile)
{
  fstream fout(sfile.toLatin1().data(), ios::binary|ios::out);

  char keyword[100];

  sprintf(keyword, "drishtiPrayog v2.0");
  fout.write((char*)keyword, strlen(keyword)+1);


  sprintf(keyword, "enddrishtiprayog");
  fout.write((char*)keyword, strlen(keyword)+1);
}

Vec
DrawHiresVolume::pointUnderPixel(QPoint scr, bool& found)
{
  int sw = m_Viewer->camera()->screenWidth();
  int sh = m_Viewer->camera()->screenHeight();  

  Vec pos;
  int cx = scr.x();
  int cy = scr.y();
  GLfloat depth = 0;

  m_forceBackToFront = true;
  glEnable(GL_SCISSOR_TEST);
  glScissor(cx, sh-1-cy, 1, 1);
  drawDragImage();
  glDisable(GL_SCISSOR_TEST);
  m_forceBackToFront = false;

  glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
  glReadPixels(cx, sh-1-cy, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
  glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);

  found = false;
  if (depth > 0.0 && depth < 1.0)
    {
      pos = m_Viewer->camera()->unprojectedCoordinatesOf(Vec(cx, cy, depth));
      found = true;
    }

  return pos;
}

#include "drawVR.hpp"
