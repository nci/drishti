void
DrawHiresVolume::drawVR(GLdouble *MVP, GLfloat *MV, Vec viewDir, 
			GLdouble *shadowMVP, Vec shadowCam, Vec shadowViewDir,
			int scrW, int scrH)
{  
  m_focusDistance = m_Viewer->camera()->focusDistance();
  m_screenWidth = m_Viewer->camera()->physicalScreenWidth();
  m_cameraWidth = m_Viewer->camera()->screenWidth();
  m_cameraHeight = m_Viewer->camera()->screenHeight();

  glGetIntegerv(GL_DRAW_BUFFER, &m_currentDrawbuf);

  glDepthMask(GL_TRUE);
  glEnable(GL_ALPHA_TEST);
  glAlphaFunc(GL_GREATER, 0);

  collectEnclosingBoxInfo();

  m_lightVector = (m_lightInfo.userLightVector.x * m_Viewer->camera()->rightVector() +
		   m_lightInfo.userLightVector.y * m_Viewer->camera()->upVector() +
		   m_lightInfo.userLightVector.z * m_Viewer->camera()->viewDirection());
  float camdist = m_Viewer->camera()->sceneRadius();

  m_lightPosition = (m_Viewer->camera()->sceneCenter() -
		     (3.0f + m_lightInfo.lightDistanceOffset)*camdist*m_lightVector);  

  
  drawGeometryOnlyVR(MVP, MV, viewDir, 
		     shadowMVP, shadowCam, shadowViewDir,
		     scrW, scrH);
}

void
DrawHiresVolume::drawGeometryOnlyVR(GLdouble *MVP, GLfloat *MV, Vec viewDir, 
				    GLdouble *shadowMVP, Vec shadowCam, Vec shadowViewDir,
				    int scrW, int scrH)
{

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);

  glClearDepth(1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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
//  GeometryObjects::trisets()->setShapeEnhancements(m_lightInfo.shadowBlur,
//						   m_lightInfo.shadowIntensity); 
  GeometryObjects::trisets()->setLightDirection(m_lightInfo.userLightVector);
  GeometryObjects::trisets()->predraw(m_bricks->getMatrix(0));

  drawGeometryVR(MVP, MV, viewDir, 
		 shadowMVP, shadowCam, shadowViewDir,
		 scrW, scrH);
    
  glUseProgramObjectARB(0);


  glEnable(GL_DEPTH_TEST);
}

void
DrawHiresVolume::drawGeometryVR(GLdouble *MVP, GLfloat *MV, Vec viewDir, 
				GLdouble *shadowMVP, Vec shadowCam, Vec shadowViewDir,
				int scrW, int scrH)
{
  GeometryObjects::trisets()->draw(m_Viewer,
				   MVP, MV, viewDir, 
				   shadowMVP, shadowCam, shadowViewDir,
				   scrW, scrH,
				   m_clipPos, m_clipNormal);
}



