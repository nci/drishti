#include "global.h"
#include "staticfunctions.h"
#include "rcviewer.h"
#include "geometryobjects.h"
#include "rcshaderfactory.h"

#include <QProgressDialog>
#include <QInputDialog>


RcViewer::RcViewer() :
  QObject()
{
  m_lut = 0;

  m_eBuffer = 0;
  m_ebId = 0;
  m_ebTex[0] = 0;
  m_ebTex[1] = 0;
  m_ebTex[2] = 0;

  m_slcBuffer = 0;
  m_rboId = 0;
  m_slcTex[0] = 0;
  m_slcTex[1] = 0;

  m_blurShader = 0;
  m_rcShader = 0;
  m_eeShader = 0;

  m_dataTex = 0;
  m_lutTex = 0;
  m_corner = Vec(0,0,0);
  m_vsize = Vec(1,1,1);
  m_sslevel = 1;

  m_amb = 1.0;
  m_diff = 0.0;
  m_spec = 1.0;
  m_shadow = 10;
  m_edge = 5.0;
  m_shdX = 0;
  m_shdY = 0;

  m_shadowColor = Vec(0.0,0.0,0.0);
  m_edgeColor = Vec(0.0,0.0,0.0);

  m_stillstep = 0.7;

  init();
}

RcViewer::~RcViewer()
{
  init();
}

void
RcViewer::init()
{
  m_skipLayers = 0;
  m_fullRender = false;
  m_dragMode = true;
  m_exactCoord = false;

  m_dbox = m_wbox = m_hbox = 0;
  m_boxSize = 64;
  m_boxMinMax.clear();
  m_filledBoxes.clear();

  m_volPtr = 0;


  if (m_rcShader)
    glDeleteObjectARB(m_rcShader);
  m_rcShader = 0;

  if (m_eeShader)
    glDeleteObjectARB(m_eeShader);
  m_eeShader = 0;

  if (m_blurShader)
    glDeleteObjectARB(m_blurShader);
  m_blurShader = 0;


  if (m_eBuffer) glDeleteFramebuffers(1, &m_eBuffer);
  if (m_ebId) glDeleteRenderbuffers(1, &m_ebId);
  if (m_ebTex[0]) glDeleteTextures(3, m_ebTex);
  m_eBuffer = 0;
  m_ebId = 0;
  m_ebTex[0] = m_ebTex[1] = m_ebTex[2] = 0;

  if (m_slcBuffer) glDeleteFramebuffers(1, &m_slcBuffer);
  if (m_rboId) glDeleteRenderbuffers(1, &m_rboId);
  if (m_slcTex[0]) glDeleteTextures(2, m_slcTex);
  m_slcBuffer = 0;
  m_rboId = 0;
  m_slcTex[0] = m_slcTex[1] = 0;

  if (m_dataTex) glDeleteTextures(1, &m_dataTex);
  m_dataTex = 0;

  if (m_lutTex) glDeleteTextures(1, &m_lutTex);
  m_lutTex = 0;

  m_corner = Vec(0,0,0);
  m_vsize = Vec(1,1,1);
  m_sslevel = 1;
}

void RcViewer::setLut(uchar *l) { m_lut = l; }

void
RcViewer::setVolDataPtr(VolumeFileManager *ptr)
{
  if (ptr)
    {      
      ptr->setMemMapped(true);
      ptr->loadMemFile();
      m_volPtr = ptr->memVolDataPtr();
    }
}

void
RcViewer::resizeGL(int width, int height)
{
  if (m_volPtr)
    createFBO();
}

void
RcViewer::setGridSize(int d, int w, int h)
{
  if (!m_volPtr)
    return;
  
  m_depth = d;
  m_width = w;
  m_height = h;

  m_dataMin = Vec(0,0,0);
  m_dataMax = Vec(h,w,d);

  m_minDSlice = 0;
  m_maxDSlice = m_depth-1;
  m_minWSlice = 0;
  m_maxWSlice = m_width-1;
  m_minHSlice = 0;
  m_maxHSlice = m_height-1;  

  glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &m_max3DTexSize);

  createShaders();

  generateBoxMinMax();
}

void
RcViewer::updateSubvolume(Vec bmin, Vec bmax)
{
  m_dataMin = bmin;
  m_dataMax = bmax;

  m_minDSlice = bmin.z;
  m_maxDSlice = bmax.z;
  m_minWSlice = bmin.y;
  m_maxWSlice = bmax.y;
  m_minHSlice = bmin.x;
  m_maxHSlice = bmax.x;  

//  QMessageBox::information(0, "", QString("%1 %2 %3\n%4 %5 %6").	\
//			   arg(m_dataMin.x).arg(m_dataMin.y).arg(m_dataMin.z).
//			   arg(m_dataMax.x).arg(m_dataMax.y).arg(m_dataMax.z));
}


void
RcViewer::generateBoxMinMax()
{
  QProgressDialog progress(QString("Updating min-max structure (%1)").arg(m_boxSize),
			   QString(),
			   0, 100,
			   0);
  progress.setMinimumDuration(0);

  m_dbox = m_depth/m_boxSize;
  m_wbox = m_width/m_boxSize;
  m_hbox = m_height/m_boxSize;
  if (m_depth%m_boxSize > 0) m_dbox++;
  if (m_width%m_boxSize > 0) m_wbox++;
  if (m_height%m_boxSize > 0) m_hbox++;

  m_boxMinMax.clear();
  m_boxMinMax.reserve(2*m_dbox*m_wbox*m_hbox);

  m_filledBoxes.resize(m_dbox*m_wbox*m_hbox);
  m_filledBoxes.fill(false);

  for(int d=0; d<m_dbox; d++)
    {
      progress.setValue(100*(float)d/m_dbox);
      qApp->processEvents();

      for(int w=0; w<m_wbox; w++)
	for(int h=0; h<m_hbox; h++)
	  {
	    int vmax = -1;
	    int vmin = 65535;
	    int dmin = d*m_boxSize;
	    int wmin = w*m_boxSize;
	    int hmin = h*m_boxSize;
	    int dmax = qMin((d+1)*m_boxSize, (int)m_depth);
	    int wmax = qMin((w+1)*m_boxSize, (int)m_width);
	    int hmax = qMin((h+1)*m_boxSize, (int)m_height);
	    for(int dm=dmin; dm<dmax; dm++)
	      for(int wm=wmin; wm<wmax; wm++)
		for(int hm=hmin; hm<hmax; hm++)
		  {
		    int v = m_volPtr[dm*m_width*m_height + wm*m_height + hm];
		    vmin = qMin(vmin, v);
		    vmax = qMax(vmax, v);
		  }
	    m_boxMinMax << vmin;
	    m_boxMinMax << vmax;
	  }
    }

  progress.setValue(100);
}

void
RcViewer::updateFilledBoxes()
{
  int lmin = 255;
  int lmax = 0;

  for(int i=0; i<255; i++)
    {
      if (m_lut[4*i+3] > 2)
	{
	  lmin = i;
	  break;
	}
    }

  for(int i=255; i>0; i--)
    {
      if (m_lut[4*i+3] > 2)
	{
	  lmax = i;
	  break;
	}
    }

  Vec bminO = m_dataMin;
  Vec bmaxO = m_dataMax;
  bminO = StaticFunctions::maxVec(bminO, Vec(m_minHSlice, m_minWSlice, m_minDSlice));
  bmaxO = StaticFunctions::minVec(bmaxO, Vec(m_maxHSlice, m_maxWSlice, m_maxDSlice));

  m_filledBoxes.resize(m_dbox*m_wbox*m_hbox);
  m_filledBoxes.fill(true);
  for(int d=0; d<m_dbox; d++)
    for(int w=0; w<m_wbox; w++)
      for(int h=0; h<m_hbox; h++)
	{
	  bool ok = true;
	  // consider only current bounding box	 
	  if ((d*m_boxSize < bminO.z && (d+1)*m_boxSize < bminO.z) ||
	      (d*m_boxSize > bmaxO.z && (d+1)*m_boxSize > bmaxO.z) ||
	      (w*m_boxSize < bminO.y && (w+1)*m_boxSize < bminO.y) ||
	      (w*m_boxSize > bmaxO.y && (w+1)*m_boxSize > bmaxO.y) ||
	      (h*m_boxSize < bminO.x && (h+1)*m_boxSize < bminO.x) ||
	      (h*m_boxSize > bmaxO.x && (h+1)*m_boxSize > bmaxO.x))
	    ok = false;
	  
	  int idx = d*m_wbox*m_hbox+w*m_hbox+h;
	  if (ok)
	    {
	      int bmin = m_boxMinMax[2*idx+0];
	      int bmax = m_boxMinMax[2*idx+1];
	      if ((bmin < lmin && bmax < lmin) || 
		  (bmin > lmax && bmax > lmax))
		m_filledBoxes.setBit(idx, false);
	    }
	  else
	    m_filledBoxes.setBit(idx, false);
	}


  MyBitArray tfb;
  tfb.resize(m_filledBoxes.size());
  for(int i=0; i<m_filledBoxes.size(); i++)
    tfb.setBit(i, m_filledBoxes.testBit(i));

  // now remove the internal ones
  for(int d=1; d<m_dbox-1; d++)
    for(int w=1; w<m_wbox-1; w++)
      for(int h=1; h<m_hbox-1; h++)
	{
	  int idx = d*m_wbox*m_hbox+w*m_hbox+h;
	  if (tfb.testBit(idx))
	    {
	      bool ok = false;
	      for(int d1=d-1; d1<=d+1; d1++)
		for(int w1=w-1; w1<=w+1; w1++)
		  for(int h1=h-1; h1<=h+1; h1++)
		    {
		      int idx1 = d1*m_wbox*m_hbox+w1*m_hbox+h1;
		      if (!tfb.testBit(idx1))
			{
			  ok = true;
			  break;
			}
		    }
	      m_filledBoxes.setBit(idx, ok);
	    }
	}


  QList<Vec> cPos = GeometryObjects::clipplanes()->positions();
  QList<Vec> cNorm = GeometryObjects::clipplanes()->normals();

  cPos << m_viewer->camera()->position() + 50*m_viewer->camera()->viewDirection();
  cNorm << -(m_viewer->camera()->viewDirection());

  // now check internal ones for clipping and boundary
  for(int d=1; d<m_dbox-1; d++)
    for(int w=1; w<m_wbox-1; w++)
      for(int h=1; h<m_hbox-1; h++)
	{
	  int idx = d*m_wbox*m_hbox+w*m_hbox+h;
	  if (!m_filledBoxes.testBit(idx) && tfb.testBit(idx)) // interior box
	    {
	      // check whether on clipping plane
	      for(int ci=0; ci<cPos.count(); ci++)
		{
		  Vec cpo = cPos[ci];
		  Vec cpn = cNorm[ci];

		  Vec bmin = Vec(h*m_boxSize,w*m_boxSize,d*m_boxSize);
		  Vec bmax = Vec((h+1)*m_boxSize,(w+1)*m_boxSize,(d+1)*m_boxSize);
		  Vec box[8];
		  box[0] = Vec(bmin.x,bmin.y,bmin.z);
		  box[1] = Vec(bmin.x,bmin.y,bmax.z);
		  box[2] = Vec(bmin.x,bmax.y,bmin.z);
		  box[3] = Vec(bmin.x,bmax.y,bmax.z);
		  box[4] = Vec(bmax.x,bmin.y,bmin.z);
		  box[5] = Vec(bmax.x,bmin.y,bmax.z);
		  box[6] = Vec(bmax.x,bmax.y,bmin.z);
		  box[7] = Vec(bmax.x,bmax.y,bmax.z);

		  bool border = false;
		  for(int b=0; b<8; b++)
		    {
		      if (qAbs((box[b]-cpo)*cpn) <= m_boxSize)
			{
			  border = true;
			  break;
			}
		    }

		  if (border)
		    {
		      m_filledBoxes.setBit(idx);		    
		      break;
		    }
		} // loop over clip planes
	    }
	}


  tfb.clear();
}

void
RcViewer::fastDraw()
{
  m_dragMode = true;

  if (!m_volPtr)
    return;

  raycasting();
}


void
RcViewer::draw()
{
  m_dragMode = false;

  if (!m_volPtr)
    return;

  raycasting();
}

void
RcViewer::createFBO()
{
  int wd = m_viewer->camera()->screenWidth();
  int ht = m_viewer->camera()->screenHeight();

  //-----------------------------------------
  if (m_eBuffer) glDeleteFramebuffers(1, &m_eBuffer);
  if (m_ebId) glDeleteRenderbuffers(1, &m_ebId);
  if (m_ebTex[0]) glDeleteTextures(3, m_ebTex);
  glGenFramebuffers(1, &m_eBuffer);
  glGenRenderbuffers(1, &m_ebId);
  glGenTextures(3, m_ebTex);
  glBindFramebuffer(GL_FRAMEBUFFER, m_eBuffer);
  glBindRenderbuffer(GL_RENDERBUFFER, m_ebId);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, wd, ht);
  glBindRenderbuffer(GL_RENDERBUFFER, 0);
  // attach the renderbuffer to depth attachment point
  glFramebufferRenderbuffer(GL_FRAMEBUFFER,      // 1. fbo target: GL_FRAMEBUFFER
			    GL_DEPTH_ATTACHMENT, // 2. attachment point
			    GL_RENDERBUFFER,     // 3. rbo target: GL_RENDERBUFFER
			    m_ebId);              // 4. rbo ID
  for(int i=0; i<3; i++)
    {
      glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_ebTex[i]);
      glTexImage2D(GL_TEXTURE_RECTANGLE_ARB,
		   0,
		   GL_RGBA16,
		   wd, ht,
		   0,
		   GL_RGBA,
		   GL_UNSIGNED_SHORT,
		   0);
    }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  //-----------------------------------------


  //-----------------------------------------
  if (m_slcBuffer) glDeleteFramebuffers(1, &m_slcBuffer);
  if (m_rboId) glDeleteRenderbuffers(1, &m_rboId);
  if (m_slcTex[0]) glDeleteTextures(2, m_slcTex);  

  glGenFramebuffers(1, &m_slcBuffer);
  glGenRenderbuffers(1, &m_rboId);
  glGenTextures(2, m_slcTex);

  glBindFramebuffer(GL_FRAMEBUFFER, m_slcBuffer);
  glBindRenderbuffer(GL_RENDERBUFFER, m_rboId);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, wd, ht);
  glBindRenderbuffer(GL_RENDERBUFFER, 0);
  // attach the renderbuffer to depth attachment point
  glFramebufferRenderbuffer(GL_FRAMEBUFFER,      // 1. fbo target: GL_FRAMEBUFFER
			    GL_DEPTH_ATTACHMENT, // 2. attachment point
			    GL_RENDERBUFFER,     // 3. rbo target: GL_RENDERBUFFER
			    m_rboId);            // 4. rbo ID

  for(int i=0; i<2; i++)
    {
      glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_slcTex[i]);
      glTexImage2D(GL_TEXTURE_RECTANGLE_ARB,
		   0,
		   GL_RGBA16,
		   wd, ht,
		   0,
		   GL_RGBA,
		   GL_UNSIGNED_SHORT,
		   0);
    }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  //-----------------------------------------
}

void
RcViewer::createRaycastShader()
{
  QString shaderString;

  int maxSteps = qSqrt(m_vsize.x*m_vsize.x +
		       m_vsize.y*m_vsize.y +
		       m_vsize.z*m_vsize.z);
  maxSteps *= 1.0/m_stillstep;

  shaderString = RcShaderFactory::genRaycastShader(maxSteps, !m_fullRender,
						 m_exactCoord, false);

  if (m_rcShader)
    glDeleteObjectARB(m_rcShader);

  m_rcShader = glCreateProgramObjectARB();
  if (! RcShaderFactory::loadShader(m_rcShader,
				  shaderString))
    {
      m_rcShader = 0;
      QMessageBox::information(0, "", "Cannot create rc shader.");
    }

  m_rcParm[0] = glGetUniformLocationARB(m_rcShader, "dataTex");
  m_rcParm[1] = glGetUniformLocationARB(m_rcShader, "lutTex");
  m_rcParm[2] = glGetUniformLocationARB(m_rcShader, "exitTex");
  m_rcParm[3] = glGetUniformLocationARB(m_rcShader, "stepSize");
  m_rcParm[4] = glGetUniformLocationARB(m_rcShader, "eyepos");
  m_rcParm[5] = glGetUniformLocationARB(m_rcShader, "viewDir");
  m_rcParm[6] = glGetUniformLocationARB(m_rcShader, "vcorner");
  m_rcParm[7] = glGetUniformLocationARB(m_rcShader, "vsize");
  m_rcParm[8] = glGetUniformLocationARB(m_rcShader, "minZ");
  m_rcParm[9] = glGetUniformLocationARB(m_rcShader, "maxZ");
  m_rcParm[10]= glGetUniformLocationARB(m_rcShader, "maskTex");
  m_rcParm[11]= glGetUniformLocationARB(m_rcShader, "saveCoord");
  m_rcParm[12]= glGetUniformLocationARB(m_rcShader, "skipLayers");
  m_rcParm[13]= glGetUniformLocationARB(m_rcShader, "tagTex");
  m_rcParm[14] = glGetUniformLocationARB(m_rcShader, "entryTex");
  m_rcParm[15] = glGetUniformLocationARB(m_rcShader, "bgcolor");
}

void
RcViewer::createShaders()
{
  QString shaderString;

  createRaycastShader();


  //----------------------
  shaderString = RcShaderFactory::genEdgeEnhanceShader();

  m_eeShader = glCreateProgramObjectARB();
  if (! RcShaderFactory::loadShader(m_eeShader,
				  shaderString))
    {
      m_eeShader = 0;
      QMessageBox::information(0, "", "Cannot create ee shader.");
    }

  m_eeParm[0] = glGetUniformLocationARB(m_eeShader, "normalTex");
  m_eeParm[1] = glGetUniformLocationARB(m_eeShader, "minZ");
  m_eeParm[2] = glGetUniformLocationARB(m_eeShader, "maxZ");
  m_eeParm[3] = glGetUniformLocationARB(m_eeShader, "eyepos");
  m_eeParm[4] = glGetUniformLocationARB(m_eeShader, "viewDir");
  m_eeParm[5] = glGetUniformLocationARB(m_eeShader, "dzScale");
  m_eeParm[6] = glGetUniformLocationARB(m_eeShader, "tagTex");
  m_eeParm[7] = glGetUniformLocationARB(m_eeShader, "lutTex");
  m_eeParm[8] = glGetUniformLocationARB(m_eeShader, "pvtTex");
  m_eeParm[9] = glGetUniformLocationARB(m_eeShader, "lightparm");
  m_eeParm[10] = glGetUniformLocationARB(m_eeShader, "isoshadow");
  m_eeParm[11] = glGetUniformLocationARB(m_eeShader, "shadowcolor");
  m_eeParm[12] = glGetUniformLocationARB(m_eeShader, "edgecolor");
  m_eeParm[13] = glGetUniformLocationARB(m_eeShader, "bgcolor");
  m_eeParm[14] = glGetUniformLocationARB(m_eeShader, "shdoffset");
  //----------------------



  //----------------------
  shaderString = RcShaderFactory::genRectBlurShaderString(1); // bilateral filter

  m_blurShader = glCreateProgramObjectARB();
  if (! RcShaderFactory::loadShader(m_blurShader,
				  shaderString))
    {
      m_blurShader = 0;
      QMessageBox::information(0, "", "Cannot create blur shader.");
    }

  m_blurParm[0] = glGetUniformLocationARB(m_blurShader, "blurTex");
  m_blurParm[1] = glGetUniformLocationARB(m_blurShader, "minZ");
  m_blurParm[2] = glGetUniformLocationARB(m_blurShader, "maxZ");
  //----------------------
}

void
RcViewer::updateVoxelsForRaycast()
{
  if (!m_volPtr)
    return;

  if (!m_lutTex) glGenTextures(1, &m_lutTex);

  
  qint64 dsz = (m_maxDSlice-m_minDSlice);
  qint64 wsz = (m_maxWSlice-m_minWSlice);
  qint64 hsz = (m_maxHSlice-m_minHSlice);
  qint64 tsz = dsz*wsz*hsz;

  m_sslevel = 1;
  while (tsz/1024.0/1024.0 > Global::textureMemorySize() ||
	 dsz > m_max3DTexSize ||
	 wsz > m_max3DTexSize ||
	 hsz > m_max3DTexSize)
    {
      m_sslevel++;
      dsz = (m_maxDSlice-m_minDSlice)/m_sslevel;
      wsz = (m_maxWSlice-m_minWSlice)/m_sslevel;
      hsz = (m_maxHSlice-m_minHSlice)/m_sslevel;

      if (dsz*m_sslevel < m_maxDSlice-m_minDSlice) dsz++;
      if (wsz*m_sslevel < m_maxWSlice-m_minWSlice) wsz++;
      if (hsz*m_sslevel < m_maxHSlice-m_minHSlice) hsz++;

      tsz = dsz*wsz*hsz;      
    }

  //-------------------------
  m_sslevel = QInputDialog::getInt(m_viewer,
				   "Subsampling Level",
				   "Subsampling Level",
				    m_sslevel, m_sslevel, 5, 1);

  dsz = (m_maxDSlice-m_minDSlice)/m_sslevel;
  wsz = (m_maxWSlice-m_minWSlice)/m_sslevel;
  hsz = (m_maxHSlice-m_minHSlice)/m_sslevel;
  if (dsz*m_sslevel < m_maxDSlice-m_minDSlice) dsz++;
  if (wsz*m_sslevel < m_maxWSlice-m_minWSlice) wsz++;
  if (hsz*m_sslevel < m_maxHSlice-m_minHSlice) hsz++;
  tsz = dsz*wsz*hsz;      
  //-------------------------

  m_corner = Vec(m_minHSlice, m_minWSlice, m_minDSlice);
  m_vsize = Vec(hsz, wsz, dsz);

  createRaycastShader();

  uchar *voxelVol = new uchar[tsz];


  QProgressDialog progress("Updating voxel structure",
			   QString(),
			   0, 100,
			   0);
  progress.setMinimumDuration(0);
  //----------------------------
  // load data volume
  progress.setValue(20);
  qApp->processEvents();
  int i = 0;
  for(int d=m_minDSlice; d<m_maxDSlice; d+=m_sslevel)
    for(int w=m_minWSlice; w<m_maxWSlice; w+=m_sslevel)
      for(int h=m_minHSlice; h<m_maxHSlice; h+=m_sslevel)
	{
	  voxelVol[i] = m_volPtr[d*m_width*m_height + w*m_height + h];
	  i++;
	}
  progress.setValue(50);
  qApp->processEvents();

  if (!m_dataTex) glGenTextures(1, &m_dataTex);
  glActiveTexture(GL_TEXTURE1);
  glEnable(GL_TEXTURE_3D);
  glBindTexture(GL_TEXTURE_3D, m_dataTex);	 
  glTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
  glTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
  glTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE); 
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  progress.setValue(70);
  glTexImage3D(GL_TEXTURE_3D,
	       0, // single resolution
	       1,
	       hsz, wsz, dsz,
	       0, // no border
	       GL_LUMINANCE,
	       GL_UNSIGNED_BYTE,
	       voxelVol);
  glDisable(GL_TEXTURE_3D);
  //----------------------------

  delete [] voxelVol;
  
  progress.setValue(100);

  m_viewer->update();
}

void
RcViewer::raycasting()
{
  if (qAbs(m_stillstep-Global::stepsizeStill()) > 0.001)
    {
      m_stillstep = Global::stepsizeStill();
      createRaycastShader();
    }


  Vec box[8];
  box[0] = Vec(m_minHSlice, m_minWSlice, m_minDSlice);
  box[1] = Vec(m_minHSlice, m_minWSlice, m_maxDSlice);
  box[2] = Vec(m_minHSlice, m_maxWSlice, m_maxDSlice);
  box[3] = Vec(m_minHSlice, m_maxWSlice, m_minDSlice);
  box[4] = Vec(m_maxHSlice, m_minWSlice, m_minDSlice);
  box[5] = Vec(m_maxHSlice, m_minWSlice, m_maxDSlice);
  box[6] = Vec(m_maxHSlice, m_maxWSlice, m_maxDSlice);
  box[7] = Vec(m_maxHSlice, m_maxWSlice, m_minDSlice);

  Vec eyepos = m_viewer->camera()->position();
  Vec viewDir = m_viewer->camera()->viewDirection();
  float minZ = 1000000;
  float maxZ = -1000000;
  for(int b=0; b<8; b++)
    {
      float zv = (box[b]-eyepos)*viewDir;
      minZ = qMin(minZ, zv);
      maxZ = qMax(maxZ, zv);
    }
  
  glEnable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);


  volumeRaycast(minZ, maxZ, false); // run full raycast process


  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE);
      
  GeometryObjects::clipplanes()->draw(m_viewer, false);
      
  if (Global::drawBox())
    {
      Vec lineColor = Vec(0.9, 0.9, 0.9);
      Vec bgcolor = Global::backgroundColor();
      float bgintensity = (0.3*bgcolor.x +
			   0.5*bgcolor.y +
			   0.2*bgcolor.z);
      if (bgintensity > 0.5)
	{
	  glDisable(GL_BLEND);
	  lineColor = Vec(0,0,0);
	}

      glColor4f(lineColor.x, lineColor.y, lineColor.z, 1.0);

      glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
      StaticFunctions::drawEnclosingCube(m_dataMin, m_dataMax);
      glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);

      //m_boundingBox.draw();
      //drawWireframeBox();
    }
  
  glDisable(GL_BLEND);
  glEnable(GL_DEPTH_TEST);
}

void
RcViewer::volumeRaycast(float minZ, float maxZ, bool firstPartOnly)
{
  updateFilledBoxes();

  Vec bgColor = Global::backgroundColor();

  Vec eyepos = m_viewer->camera()->position();
  Vec viewDir = m_viewer->camera()->viewDirection();
  Vec subvolcorner = Vec(m_minHSlice, m_minWSlice, m_minDSlice);
  Vec subvolsize = Vec(m_maxHSlice-m_minHSlice+1,
		       m_maxWSlice-m_minWSlice+1,
		       m_maxDSlice-m_minDSlice+1);

  glClearDepth(0);
  glClearColor(0, 0, 0, 0);

  glBindFramebuffer(GL_FRAMEBUFFER_EXT, m_slcBuffer);
  for(int fbn=0; fbn<2; fbn++)
    {
      glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
			     GL_COLOR_ATTACHMENT0_EXT,
			     GL_TEXTURE_RECTANGLE_ARB,
			     m_slcTex[fbn],
			     0);
      glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

  glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);


  //----------------------------
  // create exit points
  glBindFramebuffer(GL_FRAMEBUFFER_EXT, m_slcBuffer);
  glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
			 GL_COLOR_ATTACHMENT0_EXT,
			 GL_TEXTURE_RECTANGLE_ARB,
			 m_slcTex[1],
			 0);
  glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);  
  glDepthFunc(GL_GEQUAL);
  drawBox(GL_FRONT);
  glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
  //----------------------------

  //----------------------------
  // create entry points
  glBindFramebuffer(GL_FRAMEBUFFER_EXT, m_slcBuffer);
  glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
			 GL_COLOR_ATTACHMENT0_EXT,
			 GL_TEXTURE_RECTANGLE_ARB,
			 m_slcTex[0],
			 0);
  glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
  glClearDepth(1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glDepthFunc(GL_LEQUAL);
  drawBox(GL_BACK);
  glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
  //----------------------------


  //----------------------------
  if (!m_fullRender || firstPartOnly)
    {
      glBindFramebuffer(GL_FRAMEBUFFER_EXT, m_eBuffer);

      glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
			     GL_COLOR_ATTACHMENT0_EXT,
			     GL_TEXTURE_RECTANGLE_ARB,
			     m_ebTex[0],
			     0);

      glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
			     GL_COLOR_ATTACHMENT1_EXT,
			     GL_TEXTURE_RECTANGLE_ARB,
			     m_ebTex[1],
			     0);

      GLenum buffers[2] = { GL_COLOR_ATTACHMENT0_EXT,
			    GL_COLOR_ATTACHMENT1_EXT };
      glDrawBuffersARB(2, buffers);

      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
  else
    {
      glClearColor(bgColor.x, bgColor.y, bgColor.z, 0);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
  //----------------------------


  float stepsize = Global::stepsizeStill();
  if (m_dragMode)
    stepsize = Global::stepsizeDrag();

  stepsize /= qMax(m_vsize.x,qMax(m_vsize.y,m_vsize.z));


  glUseProgramObjectARB(m_rcShader);
  glUniform1iARB(m_rcParm[0], 1); // dataTex
  glUniform1iARB(m_rcParm[1], 0); // lutTex
  glUniform1iARB(m_rcParm[2], 2); // slcTex[1] - contains exit coordinates
  glUniform1fARB(m_rcParm[3], stepsize); // stepSize
  glUniform3fARB(m_rcParm[4], eyepos.x, eyepos.y, eyepos.z); // eyepos
  glUniform3fARB(m_rcParm[5], viewDir.x, viewDir.y, viewDir.z); // viewDir
  glUniform3fARB(m_rcParm[6], subvolcorner.x, subvolcorner.y, subvolcorner.z);
  glUniform3fARB(m_rcParm[7], m_vsize.x, m_vsize.y, m_vsize.z);
  glUniform1fARB(m_rcParm[8], minZ); // minZ
  glUniform1fARB(m_rcParm[9], maxZ); // maxZ
  //glUniform1iARB(m_rcParm[10],4); // maskTex
  glUniform1iARB(m_rcParm[11],firstPartOnly); // save voxel coordinates
  glUniform1iARB(m_rcParm[12],m_skipLayers); // skip first layers
  //glUniform1iARB(m_rcParm[13],5); // tagTex
  glUniform1iARB(m_rcParm[14],6); // slcTex[0] - contains entry coordinates
  glUniform3fARB(m_rcParm[15], bgColor.x,
		               bgColor.y,
		               bgColor.z);

  glActiveTexture(GL_TEXTURE2);
  glEnable(GL_TEXTURE_RECTANGLE_ARB);
  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_slcTex[1]);

  glActiveTexture(GL_TEXTURE6);
  glEnable(GL_TEXTURE_RECTANGLE_ARB);
  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_slcTex[0]);

  glActiveTexture(GL_TEXTURE1);
  glEnable(GL_TEXTURE_3D);
  glBindTexture(GL_TEXTURE_3D, m_dataTex);
  if (firstPartOnly || m_exactCoord)
    {
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
  else
    {
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, m_lutTex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D,
	       0, // single resolution
	       GL_RGBA,
	       256, Global::lutSize()*256, // width, height
	       0, // no border
	       GL_RGBA,
	       GL_UNSIGNED_BYTE,
	       m_lut);

  int wd = m_viewer->camera()->screenWidth();
  int ht = m_viewer->camera()->screenHeight();
  StaticFunctions::pushOrthoView(0, 0, wd, ht);
  StaticFunctions::drawQuad(0, 0, wd, ht, 1);
  StaticFunctions::popOrthoView();

  if (!m_fullRender || firstPartOnly)
    glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
  //----------------------------

  glActiveTexture(GL_TEXTURE6);
  glDisable(GL_TEXTURE_RECTANGLE_ARB);

  if (firstPartOnly)
    {
      glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
      glUseProgramObjectARB(0);
      
      glActiveTexture(GL_TEXTURE0);
      glDisable(GL_TEXTURE_2D);
      
      glActiveTexture(GL_TEXTURE1);
      glDisable(GL_TEXTURE_3D);
      
      glActiveTexture(GL_TEXTURE2);
      glDisable(GL_TEXTURE_RECTANGLE_ARB);
      return;
    }

  if (!m_fullRender)
    {
      int wd = m_viewer->camera()->screenWidth();
      int ht = m_viewer->camera()->screenHeight();

      //--------------------------------
      glUseProgramObjectARB(m_blurShader);
      glUniform1iARB(m_blurParm[0], 2); // blurTex
      glUniform1fARB(m_blurParm[1], minZ); // minZ
      glUniform1fARB(m_blurParm[2], maxZ); // maxZ
      
      int eb0 = 0;
      int eb2 = 2;
      for(int nb=0; nb<3; nb++)
	{
	  glBindFramebuffer(GL_FRAMEBUFFER_EXT, m_eBuffer);
	  glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
				 GL_COLOR_ATTACHMENT2_EXT,
				 GL_TEXTURE_RECTANGLE_ARB,
				 m_ebTex[eb2],
				 0);
	  glDrawBuffer(GL_COLOR_ATTACHMENT2_EXT);  
	  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	  glActiveTexture(GL_TEXTURE2);
	  glEnable(GL_TEXTURE_RECTANGLE_ARB);
	  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_ebTex[eb0]);

	  StaticFunctions::pushOrthoView(0, 0, wd, ht);
	  StaticFunctions::drawQuad(0, 0, wd, ht, 1);
	  StaticFunctions::popOrthoView();

	  int ebidx = eb0;
	  eb0 = eb2;
	  eb2 = ebidx;
	}

      glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
      //--------------------------------



      //--------------------------------
      glUseProgramObjectARB(m_eeShader);
      
      glActiveTexture(GL_TEXTURE6);
      glEnable(GL_TEXTURE_RECTANGLE_ARB);
      glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_ebTex[1]);
      
      glActiveTexture(GL_TEXTURE2);
      glEnable(GL_TEXTURE_RECTANGLE_ARB);
      glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_ebTex[eb2]);


      glUniform1iARB(m_eeParm[0], 6); // normals (ebtex[1]) tex
      glUniform1fARB(m_eeParm[1], minZ); // minZ
      glUniform1fARB(m_eeParm[2], maxZ); // maxZ
      glUniform3fARB(m_eeParm[3], eyepos.x, eyepos.y, eyepos.z); // eyepos
      glUniform3fARB(m_eeParm[4], viewDir.x, viewDir.y, viewDir.z); // viewDir
      glUniform1fARB(m_eeParm[5], m_edge);
      //glUniform1iARB(m_eeParm[6], 5); // tagtex
      glUniform1iARB(m_eeParm[7], 0); // luttex
      glUniform1iARB(m_eeParm[8], 2); // pos, val, tag tex (ebtex[eb2])
      glUniform3fARB(m_eeParm[9], m_amb, m_diff, m_spec); // lightparm
      glUniform1iARB(m_eeParm[10], m_shadow); // shadows
      glUniform3fARB(m_eeParm[11], m_shadowColor.x/255,
		                   m_shadowColor.y/255,
		                   m_shadowColor.z/255);
      glUniform3fARB(m_eeParm[12], m_edgeColor.x/255,
		                   m_edgeColor.y/255,
		                   m_edgeColor.z/255);
      glUniform3fARB(m_eeParm[13], bgColor.x,
		                   bgColor.y,
		                   bgColor.z);
      glUniform2fARB(m_eeParm[14], m_shdX, -m_shdY);

      StaticFunctions::pushOrthoView(0, 0, wd, ht);
      StaticFunctions::drawQuad(0, 0, wd, ht, 1);
      StaticFunctions::popOrthoView();
      //----------------------------
    }

  glUseProgramObjectARB(0);

  glActiveTexture(GL_TEXTURE6);
  glDisable(GL_TEXTURE_RECTANGLE_ARB);

  glActiveTexture(GL_TEXTURE0);
  glDisable(GL_TEXTURE_2D);

  glActiveTexture(GL_TEXTURE1);
  glDisable(GL_TEXTURE_3D);

  glActiveTexture(GL_TEXTURE2);
  glDisable(GL_TEXTURE_RECTANGLE_ARB);
}

void
RcViewer::drawBox(GLenum glFaces)
{
  int faces[] = {1, 5, 7, 3,
		 0, 2, 6, 4,
		 0, 1, 3, 2,
		 7, 5, 4, 6,
		 2, 3, 7, 6,
		 1, 0, 4, 5};
	  
  glEnable(GL_CULL_FACE);
  glCullFace(glFaces);

  Vec bminO = m_dataMin;
  Vec bmaxO = m_dataMax;

  bminO = StaticFunctions::maxVec(bminO, Vec(m_minHSlice, m_minWSlice, m_minDSlice));
  bmaxO = StaticFunctions::minVec(bmaxO, Vec(m_maxHSlice, m_maxWSlice, m_maxDSlice));

  int imin = (int)bminO.x/m_boxSize;
  int jmin = (int)bminO.y/m_boxSize;
  int kmin = (int)bminO.z/m_boxSize;

  int imax = (int)bmaxO.x/m_boxSize;
  int jmax = (int)bmaxO.y/m_boxSize;
  int kmax = (int)bmaxO.z/m_boxSize;
  if (imax*m_boxSize < (int)bmaxO.x) imax++;
  if (jmax*m_boxSize < (int)bmaxO.y) jmax++;
  if (kmax*m_boxSize < (int)bmaxO.z) kmax++;
  
  for(int k=kmin; k<kmax; k++)
  for(int j=jmin; j<jmax; j++)
  for(int i=imin; i<imax; i++)
    {
      int idx = k*m_wbox*m_hbox+j*m_hbox+i;
      if (m_filledBoxes.testBit(idx))
	{
	  Vec bmin, bmax;
	  bmin = Vec(qMax(i*m_boxSize, (int)bminO.x),
		     qMax(j*m_boxSize, (int)bminO.y),
		     qMax(k*m_boxSize, (int)bminO.z));

	  bmax = Vec(qMin((i+1)*m_boxSize, (int)bmaxO.x),
		     qMin((j+1)*m_boxSize, (int)bmaxO.y),
		     qMin((k+1)*m_boxSize, (int)bmaxO.z));

	  Vec box[8];
	  box[0] = Vec(bmin.x,bmin.y,bmin.z);
	  box[1] = Vec(bmin.x,bmin.y,bmax.z);
	  box[2] = Vec(bmin.x,bmax.y,bmin.z);
	  box[3] = Vec(bmin.x,bmax.y,bmax.z);
	  box[4] = Vec(bmax.x,bmin.y,bmin.z);
	  box[5] = Vec(bmax.x,bmin.y,bmax.z);
	  box[6] = Vec(bmax.x,bmax.y,bmin.z);
	  box[7] = Vec(bmax.x,bmax.y,bmax.z);
	  
	  float xmin, xmax, ymin, ymax, zmin, zmax;
	  xmin = (bmin.x-m_minHSlice)/(m_maxHSlice-m_minHSlice);
	  xmax = (bmax.x-m_minHSlice)/(m_maxHSlice-m_minHSlice);
	  ymin = (bmin.y-m_minWSlice)/(m_maxWSlice-m_minWSlice);
	  ymax = (bmax.y-m_minWSlice)/(m_maxWSlice-m_minWSlice);
	  zmin = (bmin.z-m_minDSlice)/(m_maxDSlice-m_minDSlice);
	  zmax = (bmax.z-m_minDSlice)/(m_maxDSlice-m_minDSlice);
	  
	  Vec col[8];
	  col[0] = Vec(xmin,ymin,zmin);
	  col[1] = Vec(xmin,ymin,zmax);
	  
	  col[2] = Vec(xmin,ymax,zmin);
	  col[3] = Vec(xmin,ymax,zmax);
	  
	  col[4] = Vec(xmax,ymin,zmin);
	  col[5] = Vec(xmax,ymin,zmax);
	  
	  col[6] = Vec(xmax,ymax,zmin);
	  col[7] = Vec(xmax,ymax,zmax);
	  
	  for(int i=0; i<6; i++)
	    {
	      Vec poly[100];
	      Vec tex[100];
	      for(int j=0; j<4; j++)
		{
		  int idx = faces[4*i+j];
		  poly[j] = box[idx];
		  tex[j] = col[idx];
		}
	      drawFace(4, &poly[0], &tex[0]);
	    }
	  
	  drawClipFaces(&box[0], &col[0]);
	}
    }

  glDisable(GL_CULL_FACE);
}

void
RcViewer::drawFace(int oedges, Vec *opoly, Vec *otex)
{
  QList<Vec> cPos = GeometryObjects::clipplanes()->positions();
  QList<Vec> cNorm = GeometryObjects::clipplanes()->normals();

  cPos << m_viewer->camera()->position()+50*m_viewer->camera()->viewDirection();
  cNorm << -(m_viewer->camera()->viewDirection());


  int edges = oedges;
  Vec poly[100];
  Vec tex[100];
  for(int i=0; i<edges; i++) poly[i] = opoly[i];
  for(int i=0; i<edges; i++) tex[i] = otex[i];

  //---- apply clipping
  for(int ci=0; ci<cPos.count(); ci++)
    {
      Vec cpo = cPos[ci];
      Vec cpn = cNorm[ci];
      
      int tedges = 0;
      Vec tpoly[100];
      Vec ttex[100];
      for(int i=0; i<edges; i++)
	{
	  Vec v0, v1, t0, t1;
	  
	  v0 = poly[i];
	  t0 = tex[i];
	  if (i<edges-1)
	    {
	      v1 = poly[i+1];
	      t1 = tex[i+1];
	    }
	  else
	    {
	      v1 = poly[0];
	      t1 = tex[0];
	    }
	  
	  int ret = StaticFunctions::intersectType2WithTexture(cpo, cpn,
							       v0, v1,
							       t0, t1);
	  if (ret)
	    {
	      tpoly[tedges] = v0;
	      ttex[tedges] = t0;
	      tedges ++;
	      if (ret == 2)
		{
		  tpoly[tedges] = v1;
		  ttex[tedges] = t1;
		  tedges ++;
		}
	    }
	}

      //QMessageBox::information(0, "", QString("%1").arg(tedges));

      edges = tedges;
      for(int i=0; i<edges; i++) poly[i] = tpoly[i];
      for(int i=0; i<edges; i++) tex[i] = ttex[i];
    }
  //---- clipping applied

  if (edges > 0)
    {
      glBegin(GL_POLYGON);
      for(int i=0; i<edges; i++)
	{
	  glColor3f(tex[i].x, tex[i].y, tex[i].z);
	  glVertex3f(poly[i].x, poly[i].y, poly[i].z);
	}
      glEnd();
    }  
}

void
RcViewer::drawClipFaces(Vec *subvol, Vec *texture)
{
  int sidx[] = {0, 1,
		0, 2,
		0, 4,
		7, 5,
		7, 3,
		7, 6,
		1, 3,
		1, 5,
		2, 3,
		2, 6,
		4, 5,
		4, 6 };

  QList<Vec> cPos = GeometryObjects::clipplanes()->positions();
  QList<Vec> cNorm = GeometryObjects::clipplanes()->normals();

  cPos << m_viewer->camera()->position() + 50*m_viewer->camera()->viewDirection();
  cNorm << -(m_viewer->camera()->viewDirection());

  for(int oci=0; oci<cPos.count(); oci++)
    {
      Vec cpo = cPos[oci];
      Vec cpn = cNorm[oci];

      Vec opoly[100];
      Vec otex[100];
      int edges = 0;

      QString mesg;
      for(int si=0; si<12; si++)
	{
	  int k = sidx[2*si];
	  int l = sidx[2*si+1];
	  edges += StaticFunctions::intersectType1WithTexture(cpo, cpn,
							      subvol[k], subvol[l],
							      texture[k], texture[l],
							      opoly[edges], otex[edges]);
	}

      if (edges > 2)
	{      
	  Vec cen;
	  int i;
	  for(i=0; i<edges; i++)
	    cen += opoly[i];
	  cen/=edges;
	  
	  float angle[12];
	  Vec vaxis, vperp;
	  vaxis = opoly[0]-cen;
	  vaxis.normalize();
	  
	  vperp = cpn ^ vaxis ;
	  
	  angle[0] = 1;
	  for(i=1; i<edges; i++)
	    {
	      Vec v;
	      v = opoly[i]-cen;
	      v.normalize();
	      angle[i] = vaxis*v;
	      if (vperp*v < 0)
		angle[i] = -2 - angle[i];
	    }
	  
	  // sort angle
	  int order[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
	  for(i=edges-1; i>=0; i--)
	    for(int j=1; j<=i; j++)
	      {
		if (angle[order[i]] > angle[order[j]])
		  {
		    int tmp = order[i];
		    order[i] = order[j];
		    order[j] = tmp;
		  }
	      }
	  
	  Vec poly[100];
	  Vec tex[100];
	  for(int i=0; i<edges; i++) poly[i] = opoly[order[i]];
	  for(int i=0; i<edges; i++) tex[i] = otex[order[i]];

	  //---- apply clipping
	  for(int ci=0; ci<cPos.count(); ci++)
	    {
	      if (ci != oci)
		{
		  Vec cpo = cPos[ci];
		  Vec cpn = cNorm[ci];
		  
		  int tedges = 0;
		  Vec tpoly[100];
		  Vec ttex[100];
		  for(int i=0; i<edges; i++)
		    {
		      Vec v0, v1, t0, t1;
		      
		      v0 = poly[i];
		      t0 = tex[i];
		      if (i<edges-1)
			{
			  v1 = poly[i+1];
			  t1 = tex[i+1];
			}
		      else
			{
			  v1 = poly[0];
			  t1 = tex[0];
			}
		      
		      // clip on texture coordinates
		      int ret = StaticFunctions::intersectType2WithTexture(cpo, cpn,
									   v0, v1,
									   t0, t1);
		      if (ret)
			{
			  tpoly[tedges] = v0;
			  ttex[tedges] = t0;
			  tedges ++;
			  if (ret == 2)
			    {
			      tpoly[tedges] = v1;
			      ttex[tedges] = t1;
			      tedges ++;
			    }
			}
		    }
		  
		  edges = tedges;
		  for(int i=0; i<edges; i++) poly[i] = tpoly[i];
		  for(int i=0; i<edges; i++) tex[i] = ttex[i];
		}
	    }
	  //---- clipping applied
	  
	  if (edges > 0)
	    {
	      glBegin(GL_POLYGON);
	      for(int i=0; i<edges; i++)
		{
		  glColor3f(tex[i].x, tex[i].y, tex[i].z);
		  glVertex3f(poly[i].x, poly[i].y, poly[i].z);
		}
	      glEnd();
	    }  
	}
    }
}

