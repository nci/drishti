#include "prunehandler.h"
#include "global.h"
#include "staticfunctions.h"
#include "pruneshaderfactory.h"
#include "shaderfactory.h"
#include "shaderfactory2.h"
#include "mainwindowui.h"

#include <QFileDialog>
#include <QInputDialog>

#define VECDIVIDE(a, b) Vec(a.x/b.x, a.y/b.y, a.z/b.z)

QGLFramebufferObject *PruneHandler::m_pruneBuffer=0;
QGLFramebufferObject *PruneHandler::m_savedPruneBuffer=0;
GLuint PruneHandler::m_lutTex=0;
GLuint PruneHandler::m_dataTex=0;

uchar* PruneHandler::m_lut=0;

bool PruneHandler::m_useSavedBuffer = false;
bool PruneHandler::m_mopActive = false;

bool PruneHandler::m_blendActive = false;
bool PruneHandler::m_paintActive = false;
int  PruneHandler::m_tag = 0;

bool PruneHandler::m_carveActive = false;
float PruneHandler::m_carveRadius = 10.0f;
float PruneHandler::m_carveDecay = 2.0f;
Vec PruneHandler::m_carveP = Vec(0,0,0);
Vec PruneHandler::m_carveN = Vec(0,0,0);
float PruneHandler::m_carveT = 1.0f;
Vec PruneHandler::m_carveX = Vec(0,0,0);
Vec PruneHandler::m_carveY = Vec(0,0,0);

GLhandleARB PruneHandler::m_pruneShader=0;
GLhandleARB PruneHandler::m_dilateShader=0;
GLhandleARB PruneHandler::m_erodeShader=0;
GLhandleARB PruneHandler::m_shrinkShader=0;
GLhandleARB PruneHandler::m_invertShader=0;
GLhandleARB PruneHandler::m_thickenShader=0;
GLhandleARB PruneHandler::m_edgeShader=0;
GLhandleARB PruneHandler::m_dilateEdgeShader=0;
GLhandleARB PruneHandler::m_copyChannelShader=0;
GLhandleARB PruneHandler::m_setValueShader=0;
GLhandleARB PruneHandler::m_minShader=0;
GLhandleARB PruneHandler::m_maxShader=0;
GLhandleARB PruneHandler::m_xorShader=0;
GLhandleARB PruneHandler::m_localmaxShader=0;
GLhandleARB PruneHandler::m_rDilateShader=0;
GLhandleARB PruneHandler::m_carveShader=0;
GLhandleARB PruneHandler::m_paintShader=0;
GLhandleARB PruneHandler::m_triShader=0;
GLhandleARB PruneHandler::m_removePatchShader=0;
GLhandleARB PruneHandler::m_clipShader=0;
GLhandleARB PruneHandler::m_maxValueShader=0;
GLhandleARB PruneHandler::m_histogramShader=0;
GLhandleARB PruneHandler::m_localThicknessShader=0;
GLhandleARB PruneHandler::m_smoothChannelShader=0;
GLhandleARB PruneHandler::m_averageShader=0;
GLhandleARB PruneHandler::m_patternShader=0;
GLhandleARB PruneHandler::m_cropShader=0;

GLint PruneHandler::m_pruneParm[20];
GLint PruneHandler::m_dilateParm[20];
GLint PruneHandler::m_erodeParm[20];
GLint PruneHandler::m_shrinkParm[20];
GLint PruneHandler::m_invertParm[20];
GLint PruneHandler::m_thickenParm[20];
GLint PruneHandler::m_edgeParm[20];
GLint PruneHandler::m_dilateEdgeParm[20];
GLint PruneHandler::m_copyChannelParm[5];
GLint PruneHandler::m_setValueParm[5];
GLint PruneHandler::m_minParm[5];
GLint PruneHandler::m_maxParm[5];
GLint PruneHandler::m_xorParm[5];
GLint PruneHandler::m_localmaxParm[20];
GLint PruneHandler::m_rDilateParm[20];
GLint PruneHandler::m_carveParm[20];
GLint PruneHandler::m_paintParm[20];
GLint PruneHandler::m_triParm[20];
GLint PruneHandler::m_removePatchParm[5];
GLint PruneHandler::m_clipParm[20];
GLint PruneHandler::m_maxValueParm[5];
GLint PruneHandler::m_histogramParm[20];
GLint PruneHandler::m_localThicknessParm[20];
GLint PruneHandler::m_smoothChannelParm[20];
GLint PruneHandler::m_averageParm[5];
GLint PruneHandler::m_patternParm[20];
GLint PruneHandler::m_cropParm[20];

int PruneHandler::m_channel=-1;
void PruneHandler::setChannel(int c) { m_channel = c; }
int PruneHandler::channel() { return m_channel; }

GLuint PruneHandler::m_pruneTex=0;
GLuint PruneHandler::texture() { return m_pruneBuffer->texture(); }

void PruneHandler::setUseSavedBuffer(bool b) { m_useSavedBuffer = b; }
bool PruneHandler::useSavedBuffer() { return m_useSavedBuffer; }

int PruneHandler::m_dtexX = 0;
int PruneHandler::m_dtexY = 0;
Vec PruneHandler::m_dragInfo;
Vec PruneHandler::m_subVolSize;

void PruneHandler::clean()
{
  Global::setMaskTF(-1);
  Global::setMopOp(NoMorphOp, 1);

  m_mopActive = false;
  m_useSavedBuffer = false;
  m_channel = -1;

  if (m_pruneBuffer) delete m_pruneBuffer;
  if (m_savedPruneBuffer) delete m_savedPruneBuffer;
  if (m_lutTex) glDeleteTextures(1, &m_lutTex);

  if (m_pruneShader) glDeleteObjectARB(m_pruneShader);
  if (m_dilateShader) glDeleteObjectARB(m_dilateShader);
  if (m_erodeShader) glDeleteObjectARB(m_erodeShader);
  if (m_shrinkShader) glDeleteObjectARB(m_shrinkShader);
  if (m_invertShader) glDeleteObjectARB(m_invertShader);
  if (m_thickenShader) glDeleteObjectARB(m_thickenShader);
  if (m_edgeShader) glDeleteObjectARB(m_edgeShader);
  if (m_dilateEdgeShader) glDeleteObjectARB(m_dilateEdgeShader);
  if (m_copyChannelShader) glDeleteObjectARB(m_copyChannelShader);
  if (m_setValueShader) glDeleteObjectARB(m_setValueShader);
  if (m_minShader) glDeleteObjectARB(m_minShader);
  if (m_maxShader) glDeleteObjectARB(m_maxShader);
  if (m_xorShader) glDeleteObjectARB(m_xorShader);
  if (m_localmaxShader) glDeleteObjectARB(m_localmaxShader);
  if (m_rDilateShader) glDeleteObjectARB(m_rDilateShader);
  if (m_carveShader) glDeleteObjectARB(m_carveShader);
  if (m_paintShader) glDeleteObjectARB(m_paintShader);
  if (m_triShader) glDeleteObjectARB(m_triShader);
  if (m_removePatchShader) glDeleteObjectARB(m_removePatchShader);
  if (m_clipShader) glDeleteObjectARB(m_clipShader);
  if (m_cropShader) glDeleteObjectARB(m_cropShader);
  if (m_maxValueShader) glDeleteObjectARB(m_maxValueShader);
  if (m_histogramShader) glDeleteObjectARB(m_histogramShader);
  if (m_localThicknessShader) glDeleteObjectARB(m_localThicknessShader);
  if (m_smoothChannelShader) glDeleteObjectARB(m_smoothChannelShader);
  if (m_averageShader) glDeleteObjectARB(m_averageShader);
  if (m_patternShader) glDeleteObjectARB(m_patternShader);

  m_pruneBuffer = 0;
  m_savedPruneBuffer = 0;
  m_lutTex = 0;

  m_clipShader = 0;
  m_cropShader = 0;
  m_pruneShader = 0;
  m_dilateShader = 0;
  m_rDilateShader = 0;
  m_erodeShader = 0;
  m_shrinkShader = 0;
  m_invertShader=0;
  m_thickenShader=0;
  m_edgeShader = 0;
  m_dilateEdgeShader = 0;
  m_copyChannelShader = 0;
  m_setValueShader = 0;
  m_minShader = 0;
  m_maxShader = 0;
  m_xorShader = 0;
  m_localmaxShader = 0;
  m_carveShader = 0;
  m_paintShader = 0;
  m_triShader = 0;
  m_removePatchShader = 0;
  m_maxValueShader = 0;
  m_histogramShader = 0;
  m_localThicknessShader = 0;
  m_smoothChannelShader = 0;
  m_averageShader = 0;
  m_patternShader = 0;
}

#define swapFBO(fbo1,  fbo2)			\
  {						\
    QGLFramebufferObject *tpb = fbo1;		\
    fbo1 = fbo2;				\
    fbo2 = tpb;					\
  }

QGLFramebufferObject*
PruneHandler::newFBO()
{
  QGLFramebufferObject* fbo;
  fbo = new QGLFramebufferObject(QSize(m_dtexX,
				       m_dtexY),
				 QGLFramebufferObject::NoAttachment,
				 GL_TEXTURE_RECTANGLE_EXT);
  return fbo;
}

void
PruneHandler::getRaw(uchar *raw,
		     int chan,
		     Vec dragInfo, Vec subVolSize,
		     bool maskUsingRed)
{
  // remember that we get BGRA

  int dtextureX = m_pruneBuffer->width();
  int dtextureY = m_pruneBuffer->height();  
  int ncols = dragInfo.x;
  int nrows = dragInfo.y;
  int lod = dragInfo.z;
  int gridx = dtextureX/ncols;
  int gridy = dtextureY/nrows;
  int gridz = subVolSize.z/lod;

  QImage bimg = (m_pruneBuffer->toImage()).mirrored(false, true);
  uchar *bits = bimg.bits();

  for(int z=0; z<gridz; z++)
    {
      int sy = z/ncols;
      int sx = z%ncols;
      
      sx *= gridx;
      sy *= gridy;

      if (chan != -1)
	{
	  int off = z*gridx*gridy;
	  int i=0;
	  for(int y=0; y<gridy; y++)
	    for(int x=0; x<gridx; x++)
	      {
		int idx = (sy+y)*dtextureX + (sx+x);
		if (!maskUsingRed)
		  raw[off+i] = bits[4*idx+chan];
		else if (bits[4*idx+2] > 0) // mask with Red channel
		  raw[off+i] = bits[4*idx+chan];
		i++;
	      }
	}
      else // send back three channels - 0,1,2
	{
	  int off = 3*z*gridx*gridy;
	  int i=0;
	  for(int y=0; y<gridy; y++)
	    for(int x=0; x<gridx; x++)
	      {
		int idx = (sy+y)*dtextureX + (sx+x);
		raw[off+3*i+0] = bits[4*idx+2]; //R
		raw[off+3*i+1] = bits[4*idx+1]; //G
		raw[off+3*i+2] = bits[4*idx+0]; //B
		i++;
	      }
	}
    }
}

void
PruneHandler::setRaw(uchar *raw,
		     int chan,
		     Vec dragInfo, Vec subVolSize)
{
  // remember that we get BGRA

  int dtextureX = m_pruneBuffer->width();
  int dtextureY = m_pruneBuffer->height();  
  int ncols = dragInfo.x;
  int nrows = dragInfo.y;
  int lod = dragInfo.z;
  int gridx = dtextureX/ncols;
  int gridy = dtextureY/nrows;
  int gridz = subVolSize.z/lod;

  QImage bimg = (m_pruneBuffer->toImage()).mirrored(false, true);
  uchar *bits = bimg.bits();

  uchar *pbdata = new uchar[4*m_dtexX*m_dtexY];
  memcpy(pbdata, bits, 4*m_dtexX*m_dtexY);

  for(int z=0; z<gridz; z++)
    {
      int sy = z/ncols;
      int sx = z%ncols;
      
      sx *= gridx;
      sy *= gridy;

      if (chan != -1)
	{
	  int off = z*gridx*gridy;
	  int i=0;
	  for(int y=0; y<gridy; y++)
	    for(int x=0; x<gridx; x++)
	      {
		int idx = (sy+y)*dtextureX + (sx+x);
		if (raw[off+i] > 0)
		  pbdata[4*idx+chan] = raw[off+i];
		i++;
	      }
	}
      else // send back channels 0,1,2
	{
	  int off = 3*z*gridx*gridy;
	  int i=0;
	  for(int y=0; y<gridy; y++)
	    for(int x=0; x<gridx; x++)
	      {
		int idx = (sy+y)*dtextureX + (sx+x);
		pbdata[4*idx+2] = raw[off+3*i+0];//R
		pbdata[4*idx+1] = raw[off+3*i+1];//G
		pbdata[4*idx+0] = raw[off+3*i+2];//B
		i++;
	      }
	}
    }

  GLuint tex;
  glGenTextures(1, &tex);

  glActiveTexture(GL_TEXTURE0);  
  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tex);
  glEnable(GL_TEXTURE_RECTANGLE_ARB);
  glTexImage2D(GL_TEXTURE_RECTANGLE_ARB,
	       0, // single resolution
	       4, 
	       m_dtexX, m_dtexY,
	       0, // no border
	       GL_BGRA,
	       GL_UNSIGNED_BYTE,
	       pbdata);
  glTexEnvf(GL_TEXTURE_ENV,
	    GL_TEXTURE_ENV_MODE,
	    GL_MODULATE);

  delete [] pbdata;

  m_pruneBuffer->bind();
  glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);

  glColor4f(1,1,1,1);
  StaticFunctions::pushOrthoView(0, 0, m_dtexX, m_dtexY);
  StaticFunctions::drawQuad(0, 0, m_dtexX, m_dtexY, 1.0);
  StaticFunctions::popOrthoView();

  m_pruneBuffer->release();

  glDisable(GL_TEXTURE_RECTANGLE_ARB);

  glUseProgramObjectARB(0);

  glDeleteTextures(1, &tex);
}

void
PruneHandler::saveBuffer()
{
  QFileDialog fdialog(0,
		      "Save Buffer",
		      Global::previousDirectory(),
		      "Raw File (*.raw) ;; Image (*.png *.jpg *.bmp *.tif)");

  fdialog.setAcceptMode(QFileDialog::AcceptSave);

  if (!fdialog.exec() == QFileDialog::Accepted)
    return;

  QString flnm = fdialog.selectedFiles().value(0);
  if (flnm.isEmpty())
    return;
  
  if (fdialog.selectedNameFilter() == "Image (*.png *.jpg *.bmp *.tif)")
    {
      if (!flnm.endsWith(".png") &&
	  !flnm.endsWith(".jpg") &&
	  !flnm.endsWith(".tif") &&
	  !flnm.endsWith(".bmp"))
	flnm += ".png";
    }
  else if (fdialog.selectedNameFilter() == "Raw File (*.raw)")
    {
      if (flnm.endsWith(".raw.raw"))
	flnm.chop(4);
      if (!flnm.endsWith(".raw"))
	flnm += ".raw";
    }

  if (flnm.endsWith(".raw"))
    saveRaw(flnm);
  else
    saveImage(flnm);
}

void
PruneHandler::saveImage(QString flnm)
{
  QImage pimg = (m_pruneBuffer->toImage()).mirrored(false, true);
  pimg.save(flnm);
  QMessageBox::information(0,"Save Image",
			   QString("Saved to %1").arg(flnm));

}

QByteArray
PruneHandler::getPruneBuffer()
{
  if (!m_mopActive)
    return QByteArray();

  //QMessageBox::information(0, "", "getprunebuffer");

  QImage pimg = (m_pruneBuffer->toImage()).mirrored(false, true);
  uchar *pbdata = new uchar[4*m_dtexX*m_dtexY];
  memcpy(pbdata, pimg.bits(), 4*m_dtexX*m_dtexY);
  QByteArray pb((char*)pbdata, 4*m_dtexX*m_dtexY);
//  QByteArray compressed = qCompress(pb, 9);
  delete [] pbdata;
//  return compressed;
  return pb;
}

void
PruneHandler::setPruneBuffer(QByteArray cpb, bool compressed)
{
  if (cpb.isEmpty())
    {
      // reset channel 2
      //QMessageBox::information(0, "", "reset channel 2");
      setValue(0, 2, 0, 255);
      setValue(0, 1, 0, 255);
      m_mopActive = false;
      return;
    }

  //QMessageBox::information(0, "", "setprunebuffer");

  //copyToFromSavedChannel(true, 0, 0, false); // save current channel 0
  
  m_mopActive = true;
  QByteArray pb;
  pb = cpb;

  uchar *pbdata = new uchar[4*m_dtexX*m_dtexY];
  memcpy(pbdata, (uchar*)pb.data(), pb.count());

  GLuint tex;
  glGenTextures(1, &tex);

  glActiveTexture(GL_TEXTURE0);  
  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tex);
  glEnable(GL_TEXTURE_RECTANGLE_ARB);
  glTexImage2D(GL_TEXTURE_RECTANGLE_ARB,
	       0, // single resolution
	       4, 
	       m_dtexX, m_dtexY,
	       0, // no border
	       GL_BGRA,
	       GL_UNSIGNED_BYTE,
	       pbdata);
  glTexEnvf(GL_TEXTURE_ENV,
	    GL_TEXTURE_ENV_MODE,
	    GL_MODULATE);

  delete [] pbdata;

  m_pruneBuffer->bind();
  glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);

  glColor4f(1,1,1,1);
  StaticFunctions::pushOrthoView(0, 0, m_dtexX, m_dtexY);
  StaticFunctions::drawQuad(0, 0, m_dtexX, m_dtexY, 1.0);
  StaticFunctions::popOrthoView();

  m_pruneBuffer->release();

  glDisable(GL_TEXTURE_RECTANGLE_ARB);

  glUseProgramObjectARB(0);

  glDeleteTextures(1, &tex);

  //copyToFromSavedChannel(false, 0, 0, false); // restore channel 0
}

void
PruneHandler::saveRaw(QString flnm)
{
  QStringList items;
  items << "0";
  items << "1";
  items << "2";
  bool ok;
  QString str;
  str = QInputDialog::getItem(0,
			      "Channel Number",
			      "Channel Number",
			      items,
			      0,
			      false, // text is not editable
			      &ok);
  int ch = 0;
  if (!ok || str == "0")
    ch = 0;
  else
    ch = str.toInt();

  ch = 2-ch;  // 0->2, 1->1, 2->0; because we have BGR

  int ncols = m_dragInfo.x;
  int nrows = m_dragInfo.y;
  int lod = m_dragInfo.z;
  int gridx = m_dtexX/ncols;
  int gridy = m_dtexY/nrows;
  int gridz = m_subVolSize.z/lod;

  QImage bimg = (m_pruneBuffer->toImage()).mirrored(false, true);
  uchar *bits = bimg.bits();

  QFile qfl;
  qfl.setFileName(flnm);
  qfl.open(QFile::WriteOnly);

  uchar vt = 0;  
  qfl.write((char*)&vt, 1);
  qfl.write((char*)&gridz, 4);
  qfl.write((char*)&gridy, 4);
  qfl.write((char*)&gridx, 4);

  QProgressDialog progress(QString("Saving %1").arg(flnm),
			   QString(),
			   0, 100,
			   0);

  uchar *tmp = new uchar[gridx*gridy];
  for(int z=0; z<gridz; z++)
    {
      progress.setValue(100*(float)z/(float)gridz);

      int sy = z/ncols;
      int sx = z%ncols;
      
      sx *= gridx;
      sy *= gridy;

      int i=0;
      for(int y=0; y<gridy; y++)
	for(int x=0; x<gridx; x++)
	  {
	    int idx = (sy+y)*m_dtexX + (sx+x);
	    tmp[i] = bits[4*idx+ch];
	    i++;
	  }
      qfl.write((char*)tmp, gridx*gridy);
    }
  qfl.close();
  progress.setValue(100);
  QMessageBox::information(0,"Save RAW",
			   QString("Saved channel %1 to %2").arg(2-ch).arg(flnm));
}

bool
PruneHandler::standardChecks()
{
  if (Global::volumeType() == Global::DummyVolume ||
      Global::volumeType() == Global::RGBVolume ||
      Global::volumeType() == Global::RGBAVolume)
    {
      QMessageBox::information(0, "Error MOP", "Does not work on dummy or colour volumes");
      return false;
    }

  if (!Global::emptySpaceSkip())
    {
      QMessageBox::information(0, "Error MOP", "EmptySpaceSkipping not switched on.");
      return false;
    }

  return true;
}

void
PruneHandler::createMopShaders()
{
  QString shaderString;

  //---------------------------
  shaderString = PruneShaderFactory::minTexture();

  if (m_minShader)
    glDeleteObjectARB(m_minShader);

  m_minShader = glCreateProgramObjectARB();
  if (! ShaderFactory::loadShader(m_minShader,
				  shaderString))
    exit(0);

  m_minParm[0] = glGetUniformLocationARB(m_minShader, "pruneTex1");
  m_minParm[1] = glGetUniformLocationARB(m_minShader, "pruneTex2");
  m_minParm[2] = glGetUniformLocationARB(m_minShader, "ch1");
  m_minParm[3] = glGetUniformLocationARB(m_minShader, "ch2");
  //---------------------------

  //---------------------------
  shaderString = PruneShaderFactory::maxTexture();

  if (m_maxShader)
    glDeleteObjectARB(m_maxShader);

  m_maxShader = glCreateProgramObjectARB();
  if (! ShaderFactory::loadShader(m_maxShader,
				  shaderString))
    exit(0);

  m_maxParm[0] = glGetUniformLocationARB(m_maxShader, "pruneTex1");
  m_maxParm[1] = glGetUniformLocationARB(m_maxShader, "pruneTex2");
  m_maxParm[2] = glGetUniformLocationARB(m_maxShader, "ch1");
  m_maxParm[3] = glGetUniformLocationARB(m_maxShader, "ch2");
  //---------------------------

  //---------------------------
  shaderString = PruneShaderFactory::xorTexture();

  if (m_xorShader)
    glDeleteObjectARB(m_xorShader);

  m_xorShader = glCreateProgramObjectARB();
  if (! ShaderFactory::loadShader(m_xorShader,
				  shaderString))
    exit(0);

  m_xorParm[0] = glGetUniformLocationARB(m_xorShader, "pruneTex1");
  m_xorParm[1] = glGetUniformLocationARB(m_xorShader, "pruneTex2");
  m_xorParm[2] = glGetUniformLocationARB(m_xorShader, "ch1");
  m_xorParm[3] = glGetUniformLocationARB(m_xorShader, "ch2");
  //---------------------------

  //---------------------------
  shaderString = PruneShaderFactory::invert();

  if (m_invertShader)
    glDeleteObjectARB(m_invertShader);

  m_invertShader = glCreateProgramObjectARB();
  if (! ShaderFactory::loadShader(m_invertShader,
				  shaderString))
    exit(0);

  m_invertParm[0] = glGetUniformLocationARB(m_invertShader, "pruneTex");
  m_invertParm[1] = glGetUniformLocationARB(m_invertShader, "chan");
  //---------------------------

  //---------------------------
  shaderString = PruneShaderFactory::setValue();

  if (m_setValueShader)
    glDeleteObjectARB(m_setValueShader);

  m_setValueShader = glCreateProgramObjectARB();
  if (! ShaderFactory::loadShader(m_setValueShader,
				  shaderString))
    exit(0);

  m_setValueParm[0] = glGetUniformLocationARB(m_setValueShader, "pruneTex");
  m_setValueParm[1] = glGetUniformLocationARB(m_setValueShader, "val");
  m_setValueParm[2] = glGetUniformLocationARB(m_setValueShader, "chan");
  m_setValueParm[3] = glGetUniformLocationARB(m_setValueShader, "minval");
  m_setValueParm[4] = glGetUniformLocationARB(m_setValueShader, "maxval");
  //---------------------------

  //---------------------------
  shaderString = PruneShaderFactory::copyChannel();

  if (m_copyChannelShader)
    glDeleteObjectARB(m_copyChannelShader);

  m_copyChannelShader = glCreateProgramObjectARB();
  if (! ShaderFactory::loadShader(m_copyChannelShader,
				  shaderString))
    exit(0);

  m_copyChannelParm[0] = glGetUniformLocationARB(m_copyChannelShader, "pruneTex");
  m_copyChannelParm[1] = glGetUniformLocationARB(m_copyChannelShader, "src");
  m_copyChannelParm[2] = glGetUniformLocationARB(m_copyChannelShader, "dst");
  //---------------------------

  //---------------------------
  shaderString = PruneShaderFactory::removePatch();

  if (m_removePatchShader)
    glDeleteObjectARB(m_removePatchShader);

  m_removePatchShader = glCreateProgramObjectARB();
  if (! ShaderFactory::loadShader(m_removePatchShader,
				  shaderString))
    exit(0);

  m_removePatchParm[0] = glGetUniformLocationARB(m_removePatchShader, "pruneTex");
  m_removePatchParm[1] = glGetUniformLocationARB(m_removePatchShader, "remove");
  //---------------------------

  //---------------------------
  shaderString = PruneShaderFactory::average();

  if (m_averageShader)
    glDeleteObjectARB(m_averageShader);

  m_averageShader = glCreateProgramObjectARB();
  if (! ShaderFactory::loadShader(m_averageShader,
				  shaderString))
    exit(0);

  m_averageParm[0] = glGetUniformLocationARB(m_averageShader, "pruneTex");
  m_averageParm[1] = glGetUniformLocationARB(m_averageShader, "chan1");
  m_averageParm[2] = glGetUniformLocationARB(m_averageShader, "chan2");
  m_averageParm[3] = glGetUniformLocationARB(m_averageShader, "dst");
  //---------------------------

  //---------------------------
  shaderString = PruneShaderFactory::thicken();

  if (m_thickenShader)
    glDeleteObjectARB(m_thickenShader);

  m_thickenShader = glCreateProgramObjectARB();
  if (! ShaderFactory::loadShader(m_thickenShader,
				  shaderString))
    exit(0);

  m_thickenParm[0] = glGetUniformLocationARB(m_thickenShader, "pruneTex");
  m_thickenParm[1] = glGetUniformLocationARB(m_thickenShader, "gridx");
  m_thickenParm[2] = glGetUniformLocationARB(m_thickenShader, "gridy");
  m_thickenParm[3] = glGetUniformLocationARB(m_thickenShader, "gridz");
  m_thickenParm[4] = glGetUniformLocationARB(m_thickenShader, "nrows");
  m_thickenParm[5] = glGetUniformLocationARB(m_thickenShader, "ncols");
  m_thickenParm[6] = glGetUniformLocationARB(m_thickenShader, "cityblock");
  //---------------------------

  //---------------------------
  shaderString = PruneShaderFactory::dilateEdgeTexture();
  
  if (m_dilateEdgeShader)
    glDeleteObjectARB(m_dilateEdgeShader);

  m_dilateEdgeShader = glCreateProgramObjectARB();
  if (! ShaderFactory::loadShader(m_dilateEdgeShader,
				  shaderString))
    exit(0);

  m_dilateEdgeParm[0] = glGetUniformLocationARB(m_dilateEdgeShader, "pruneTex");
  m_dilateEdgeParm[1] = glGetUniformLocationARB(m_dilateEdgeShader, "gridx");
  m_dilateEdgeParm[2] = glGetUniformLocationARB(m_dilateEdgeShader, "gridy");
  m_dilateEdgeParm[3] = glGetUniformLocationARB(m_dilateEdgeShader, "gridz");
  m_dilateEdgeParm[4] = glGetUniformLocationARB(m_dilateEdgeShader, "nrows");
  m_dilateEdgeParm[5] = glGetUniformLocationARB(m_dilateEdgeShader, "ncols");
  m_dilateEdgeParm[6] = glGetUniformLocationARB(m_dilateEdgeShader, "erodeval");
  //---------------------------

  //---------------------------
  shaderString = PruneShaderFactory::localMaximum();

  if (m_localmaxShader)
    glDeleteObjectARB(m_localmaxShader);

  m_localmaxShader = glCreateProgramObjectARB();
  if (! ShaderFactory::loadShader(m_localmaxShader,
				  shaderString))
    exit(0);

  m_localmaxParm[0] = glGetUniformLocationARB(m_localmaxShader, "pruneTex");
  m_localmaxParm[1] = glGetUniformLocationARB(m_localmaxShader, "gridx");
  m_localmaxParm[2] = glGetUniformLocationARB(m_localmaxShader, "gridy");
  m_localmaxParm[3] = glGetUniformLocationARB(m_localmaxShader, "gridz");
  m_localmaxParm[4] = glGetUniformLocationARB(m_localmaxShader, "nrows");
  m_localmaxParm[5] = glGetUniformLocationARB(m_localmaxShader, "ncols");
  //---------------------------

  //---------------------------
  shaderString = PruneShaderFactory::localThickness();

  if (m_localThicknessShader)
    glDeleteObjectARB(m_localThicknessShader);

  m_localThicknessShader = glCreateProgramObjectARB();
  if (! ShaderFactory::loadShader(m_localThicknessShader,
				  shaderString))
    exit(0);

  m_localThicknessParm[0] = glGetUniformLocationARB(m_localThicknessShader, "pruneTex");
  m_localThicknessParm[1] = glGetUniformLocationARB(m_localThicknessShader, "gridx");
  m_localThicknessParm[2] = glGetUniformLocationARB(m_localThicknessShader, "gridy");
  m_localThicknessParm[3] = glGetUniformLocationARB(m_localThicknessShader, "gridz");
  m_localThicknessParm[4] = glGetUniformLocationARB(m_localThicknessShader, "nrows");
  m_localThicknessParm[5] = glGetUniformLocationARB(m_localThicknessShader, "ncols");
  m_localThicknessParm[6] = glGetUniformLocationARB(m_localThicknessShader, "thickness");
  //---------------------------

  //---------------------------
  shaderString = PruneShaderFactory::dilate();

  if (m_dilateShader)
    glDeleteObjectARB(m_dilateShader);

  m_dilateShader = glCreateProgramObjectARB();
  if (! ShaderFactory::loadShader(m_dilateShader,
				  shaderString))
    exit(0);

  m_dilateParm[0] = glGetUniformLocationARB(m_dilateShader, "pruneTex");
  m_dilateParm[1] = glGetUniformLocationARB(m_dilateShader, "gridx");
  m_dilateParm[2] = glGetUniformLocationARB(m_dilateShader, "gridy");
  m_dilateParm[3] = glGetUniformLocationARB(m_dilateShader, "gridz");
  m_dilateParm[4] = glGetUniformLocationARB(m_dilateShader, "nrows");
  m_dilateParm[5] = glGetUniformLocationARB(m_dilateShader, "ncols");
  m_dilateParm[6] = glGetUniformLocationARB(m_dilateShader, "chan");
  //---------------------------

  //---------------------------
  shaderString = PruneShaderFactory::restrictedDilate();

  if (m_rDilateShader)
    glDeleteObjectARB(m_rDilateShader);

  m_rDilateShader = glCreateProgramObjectARB();
  if (! ShaderFactory::loadShader(m_rDilateShader,
				  shaderString))
    exit(0);

  m_rDilateParm[0] = glGetUniformLocationARB(m_rDilateShader, "pruneTex");
  m_rDilateParm[1] = glGetUniformLocationARB(m_rDilateShader, "gridx");
  m_rDilateParm[2] = glGetUniformLocationARB(m_rDilateShader, "gridy");
  m_rDilateParm[3] = glGetUniformLocationARB(m_rDilateShader, "gridz");
  m_rDilateParm[4] = glGetUniformLocationARB(m_rDilateShader, "nrows");
  m_rDilateParm[5] = glGetUniformLocationARB(m_rDilateShader, "ncols");
  m_rDilateParm[6] = glGetUniformLocationARB(m_rDilateShader, "val");
  //---------------------------

  //---------------------------
  shaderString = PruneShaderFactory::erode();

  if (m_erodeShader)
    glDeleteObjectARB(m_erodeShader);

  m_erodeShader = glCreateProgramObjectARB();
  if (! ShaderFactory::loadShader(m_erodeShader,
				  shaderString))
    exit(0);

  m_erodeParm[0] = glGetUniformLocationARB(m_erodeShader, "pruneTex");
  m_erodeParm[1] = glGetUniformLocationARB(m_erodeShader, "gridx");
  m_erodeParm[2] = glGetUniformLocationARB(m_erodeShader, "gridy");
  m_erodeParm[3] = glGetUniformLocationARB(m_erodeShader, "gridz");
  m_erodeParm[4] = glGetUniformLocationARB(m_erodeShader, "nrows");
  m_erodeParm[5] = glGetUniformLocationARB(m_erodeShader, "ncols");
  m_erodeParm[6] = glGetUniformLocationARB(m_erodeShader, "chan");
  //---------------------------

  //---------------------------
  shaderString = PruneShaderFactory::shrink();

  if (m_shrinkShader)
    glDeleteObjectARB(m_shrinkShader);

  m_shrinkShader = glCreateProgramObjectARB();
  if (! ShaderFactory::loadShader(m_shrinkShader,
				  shaderString))
    exit(0);

  m_shrinkParm[0] = glGetUniformLocationARB(m_shrinkShader, "pruneTex");
  m_shrinkParm[1] = glGetUniformLocationARB(m_shrinkShader, "gridx");
  m_shrinkParm[2] = glGetUniformLocationARB(m_shrinkShader, "gridy");
  m_shrinkParm[3] = glGetUniformLocationARB(m_shrinkShader, "gridz");
  m_shrinkParm[4] = glGetUniformLocationARB(m_shrinkShader, "nrows");
  m_shrinkParm[5] = glGetUniformLocationARB(m_shrinkShader, "ncols");
  m_shrinkParm[6] = glGetUniformLocationARB(m_shrinkShader, "chan");
  //---------------------------

  //---------------------------
  shaderString = PruneShaderFactory::edgeTexture();

  if (m_edgeShader)
    glDeleteObjectARB(m_edgeShader);

  m_edgeShader = glCreateProgramObjectARB();
  if (! ShaderFactory::loadShader(m_edgeShader,
				  shaderString))
    exit(0);

  m_edgeParm[0] = glGetUniformLocationARB(m_edgeShader, "pruneTex");
  m_edgeParm[1] = glGetUniformLocationARB(m_edgeShader, "gridx");
  m_edgeParm[2] = glGetUniformLocationARB(m_edgeShader, "gridy");
  m_edgeParm[3] = glGetUniformLocationARB(m_edgeShader, "gridz");
  m_edgeParm[4] = glGetUniformLocationARB(m_edgeShader, "nrows");
  m_edgeParm[5] = glGetUniformLocationARB(m_edgeShader, "ncols");
  m_edgeParm[6] = glGetUniformLocationARB(m_edgeShader, "val");
  m_edgeParm[7] = glGetUniformLocationARB(m_edgeShader, "sz");
  //---------------------------

  //---------------------------
  shaderString = PruneShaderFactory::carve();

  if (m_carveShader)
    glDeleteObjectARB(m_carveShader);

  m_carveShader = glCreateProgramObjectARB();
  if (! ShaderFactory::loadShader(m_carveShader,
				  shaderString))
    exit(0);

  m_carveParm[0] = glGetUniformLocationARB(m_carveShader, "pruneTex");
  m_carveParm[1] = glGetUniformLocationARB(m_carveShader, "gridx");
  m_carveParm[2] = glGetUniformLocationARB(m_carveShader, "gridy");
  m_carveParm[3] = glGetUniformLocationARB(m_carveShader, "gridz");
  m_carveParm[4] = glGetUniformLocationARB(m_carveShader, "nrows");
  m_carveParm[5] = glGetUniformLocationARB(m_carveShader, "ncols");
  m_carveParm[6] = glGetUniformLocationARB(m_carveShader, "center");
  m_carveParm[7] = glGetUniformLocationARB(m_carveShader, "radius");
  m_carveParm[8] = glGetUniformLocationARB(m_carveShader, "decay");
  m_carveParm[9] = glGetUniformLocationARB(m_carveShader, "docarve");
  m_carveParm[11] = glGetUniformLocationARB(m_carveShader, "planarcarve");
  m_carveParm[12] = glGetUniformLocationARB(m_carveShader, "clipp");
  m_carveParm[13] = glGetUniformLocationARB(m_carveShader, "clipn");
  m_carveParm[14] = glGetUniformLocationARB(m_carveShader, "clipt");
  m_carveParm[15] = glGetUniformLocationARB(m_carveShader, "clipx");
  m_carveParm[16] = glGetUniformLocationARB(m_carveShader, "clipy");
  //---------------------------

  //---------------------------
  shaderString = PruneShaderFactory::paint();

  if (m_paintShader)
    glDeleteObjectARB(m_paintShader);

  m_paintShader = glCreateProgramObjectARB();
  if (! ShaderFactory::loadShader(m_paintShader,
				  shaderString))
    exit(0);

  m_paintParm[0] = glGetUniformLocationARB(m_paintShader, "pruneTex");
  m_paintParm[1] = glGetUniformLocationARB(m_paintShader, "gridx");
  m_paintParm[2] = glGetUniformLocationARB(m_paintShader, "gridy");
  m_paintParm[3] = glGetUniformLocationARB(m_paintShader, "gridz");
  m_paintParm[4] = glGetUniformLocationARB(m_paintShader, "nrows");
  m_paintParm[5] = glGetUniformLocationARB(m_paintShader, "ncols");
  m_paintParm[6] = glGetUniformLocationARB(m_paintShader, "center");
  m_paintParm[7] = glGetUniformLocationARB(m_paintShader, "radius");
  m_paintParm[8] = glGetUniformLocationARB(m_paintShader, "decay");
  m_paintParm[9] = glGetUniformLocationARB(m_paintShader, "dopaint");
  m_paintParm[10] = glGetUniformLocationARB(m_paintShader, "tag");
  m_paintParm[11] = glGetUniformLocationARB(m_paintShader, "planarcarve");
  m_paintParm[12] = glGetUniformLocationARB(m_paintShader, "clipp");
  m_paintParm[13] = glGetUniformLocationARB(m_paintShader, "clipn");
  m_paintParm[14] = glGetUniformLocationARB(m_paintShader, "clipt");
  m_paintParm[15] = glGetUniformLocationARB(m_paintShader, "clipx");
  m_paintParm[16] = glGetUniformLocationARB(m_paintShader, "clipy");

  //---------------------------

  //---------------------------
  shaderString = PruneShaderFactory::clip();

  if (m_clipShader)
    glDeleteObjectARB(m_clipShader);

  m_clipShader = glCreateProgramObjectARB();
  if (! ShaderFactory::loadShader(m_clipShader,
				  shaderString))
    exit(0);

  m_clipParm[0] = glGetUniformLocationARB(m_clipShader, "pruneTex");
  m_clipParm[1] = glGetUniformLocationARB(m_clipShader, "gridx");
  m_clipParm[2] = glGetUniformLocationARB(m_clipShader, "gridy");
  m_clipParm[3] = glGetUniformLocationARB(m_clipShader, "gridz");
  m_clipParm[4] = glGetUniformLocationARB(m_clipShader, "nrows");
  m_clipParm[5] = glGetUniformLocationARB(m_clipShader, "ncols");
  m_clipParm[6] = glGetUniformLocationARB(m_clipShader, "pos");
  m_clipParm[7] = glGetUniformLocationARB(m_clipShader, "normal");
  //---------------------------

  //---------------------------
  shaderString = PruneShaderFactory::fillTriangle();

  if (m_triShader)
    glDeleteObjectARB(m_triShader);

  m_triShader = glCreateProgramObjectARB();
  if (! ShaderFactory::loadShader(m_triShader,
				  shaderString))
    exit(0);

  m_triParm[0] = glGetUniformLocationARB(m_triShader, "pruneTex");
  m_triParm[1] = glGetUniformLocationARB(m_triShader, "gridx");
  m_triParm[2] = glGetUniformLocationARB(m_triShader, "gridy");
  m_triParm[3] = glGetUniformLocationARB(m_triShader, "gridz");
  m_triParm[4] = glGetUniformLocationARB(m_triShader, "nrows");
  m_triParm[5] = glGetUniformLocationARB(m_triShader, "ncols");
  m_triParm[6] = glGetUniformLocationARB(m_triShader, "A");
  m_triParm[7] = glGetUniformLocationARB(m_triShader, "B");
  m_triParm[8] = glGetUniformLocationARB(m_triShader, "C");
  m_triParm[9] = glGetUniformLocationARB(m_triShader, "thick");
  m_triParm[10] = glGetUniformLocationARB(m_triShader, "val");
  m_triParm[11] = glGetUniformLocationARB(m_triShader, "paint");
  //---------------------------

  //---------------------------
  shaderString = PruneShaderFactory::maxValue();

  if (m_maxValueShader)
    glDeleteObjectARB(m_maxValueShader);

  m_maxValueShader = glCreateProgramObjectARB();
  if (! ShaderFactory::loadShader(m_maxValueShader,
				  shaderString))
    exit(0);

  m_maxValueParm[0] = glGetUniformLocationARB(m_maxValueShader, "pruneTex");
  m_maxValueParm[1] = glGetUniformLocationARB(m_maxValueShader, "cols");
  m_maxValueParm[2] = glGetUniformLocationARB(m_maxValueShader, "rows");
  //---------------------------

  //---------------------------
  shaderString = PruneShaderFactory::histogram();

  if (m_histogramShader)
    glDeleteObjectARB(m_histogramShader);

  m_histogramShader = glCreateProgramObjectARB();
  if (! ShaderFactory::loadShader(m_histogramShader,
				  shaderString))
    exit(0);

  m_histogramParm[0] = glGetUniformLocationARB(m_histogramShader, "pruneTex");
  m_histogramParm[1] = glGetUniformLocationARB(m_histogramShader, "cols");
  m_histogramParm[2] = glGetUniformLocationARB(m_histogramShader, "rows");
  m_histogramParm[3] = glGetUniformLocationARB(m_histogramShader, "val");
  m_histogramParm[4] = glGetUniformLocationARB(m_histogramShader, "addstep");
  m_histogramParm[5] = glGetUniformLocationARB(m_histogramShader, "chan");
  //---------------------------

  //---------------------------
  shaderString = PruneShaderFactory::smoothChannel();

  if (m_smoothChannelShader)
    glDeleteObjectARB(m_smoothChannelShader);

  m_smoothChannelShader = glCreateProgramObjectARB();
  if (! ShaderFactory::loadShader(m_smoothChannelShader,
				  shaderString))
    exit(0);

  m_smoothChannelParm[0] = glGetUniformLocationARB(m_smoothChannelShader, "pruneTex");
  m_smoothChannelParm[1] = glGetUniformLocationARB(m_smoothChannelShader, "gridx");
  m_smoothChannelParm[2] = glGetUniformLocationARB(m_smoothChannelShader, "gridy");
  m_smoothChannelParm[3] = glGetUniformLocationARB(m_smoothChannelShader, "gridz");
  m_smoothChannelParm[4] = glGetUniformLocationARB(m_smoothChannelShader, "nrows");
  m_smoothChannelParm[5] = glGetUniformLocationARB(m_smoothChannelShader, "ncols");
  m_smoothChannelParm[6] = glGetUniformLocationARB(m_smoothChannelShader, "chan");
  //---------------------------

  //---------------------------
  shaderString = PruneShaderFactory::pattern();

  if (m_patternShader)
    glDeleteObjectARB(m_patternShader);

  m_patternShader = glCreateProgramObjectARB();
  if (! ShaderFactory::loadShader(m_patternShader,
				  shaderString))
    exit(0);

  m_patternParm[0] = glGetUniformLocationARB(m_patternShader, "pruneTex");
  m_patternParm[1] = glGetUniformLocationARB(m_patternShader, "gridx");
  m_patternParm[2] = glGetUniformLocationARB(m_patternShader, "gridy");
  m_patternParm[3] = glGetUniformLocationARB(m_patternShader, "gridz");
  m_patternParm[4] = glGetUniformLocationARB(m_patternShader, "nrows");
  m_patternParm[5] = glGetUniformLocationARB(m_patternShader, "ncols");
  m_patternParm[6] = glGetUniformLocationARB(m_patternShader, "flag");
  m_patternParm[7] = glGetUniformLocationARB(m_patternShader, "pat");
  //---------------------------

}

void
PruneHandler::createPruneShader(bool bit16)
{
  if (Global::volumeType() == Global::DummyVolume)
    return;

  if (Global::volumeType() == Global::RGBVolume ||
      Global::volumeType() == Global::RGBAVolume)
    return;


  QString shaderString;

  //---------------------------
  if (Global::volumeType() == Global::SingleVolume)
    shaderString = PruneShaderFactory::genPruneTexture(bit16);
  else
    {
      int nvol = 1;
      if (Global::volumeType() == Global::DoubleVolume) nvol = 2;
      if (Global::volumeType() == Global::TripleVolume) nvol = 3;
      if (Global::volumeType() == Global::QuadVolume) nvol = 4;

      shaderString = ShaderFactory2::genPruneTexture(nvol);
    }    

  if (m_pruneShader)
    glDeleteObjectARB(m_pruneShader);

  m_pruneShader = glCreateProgramObjectARB();
  if (! ShaderFactory::loadShader(m_pruneShader,
				  shaderString))
    exit(0);

  m_pruneParm[0] = glGetUniformLocationARB(m_pruneShader, "lutTex");
  m_pruneParm[1] = glGetUniformLocationARB(m_pruneShader, "dragTex");
  m_pruneParm[2] = glGetUniformLocationARB(m_pruneShader, "gridx");
  m_pruneParm[3] = glGetUniformLocationARB(m_pruneShader, "gridy");
  m_pruneParm[4] = glGetUniformLocationARB(m_pruneShader, "gridz");
  m_pruneParm[5] = glGetUniformLocationARB(m_pruneShader, "nrows");
  m_pruneParm[6] = glGetUniformLocationARB(m_pruneShader, "ncols");
  //---------------------------
}


#define WRITECHANNEL(dst)					\
  {								\
    if (dst == -1)						\
      glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);		\
    else if (dst == 0)						\
      glColorMask(GL_TRUE, GL_FALSE, GL_FALSE, GL_TRUE);	\
    else if (dst == 1)						\
      glColorMask(GL_FALSE, GL_TRUE, GL_FALSE, GL_TRUE);	\
    else if (dst == 2)						\
      glColorMask(GL_FALSE, GL_FALSE, GL_TRUE, GL_TRUE);	\
  }


void
PruneHandler::generatePruneTexture(QGLFramebufferObject *pruneBuffer1,
				   uchar *vlut)
{
  // just save all max values and send them for pruning
  if (!m_lut) m_lut = new uchar[256*256];

  memset(m_lut, 0, 256*256);

  if (Global::maskTF() == -1)
    {
      for(int l=0; l<Global::lutSize(); l++)
	{
	  int lsize = l*256*256*4;
	  for(int i=0; i<256*256; i++)
	    m_lut[i] = qMax(m_lut[i], vlut[lsize + 4*i+3]);
	}
    }
  else
    {
      int l = Global::maskTF();
      int lsize = l*256*256*4;
      for(int i=0; i<256*256; i++)
	m_lut[i] = qMax(m_lut[i], vlut[lsize + 4*i+3]);
    }


  // load "max" lookup table
  if (!m_lutTex) glGenTextures(1, &m_lutTex);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, m_lutTex); //max values from all tfsets
  glEnable(GL_TEXTURE_2D);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D,
	       0, // single resolution
	       1,
	       256, 256, // width, height
	       0, // no border
	       GL_LUMINANCE,
	       GL_UNSIGNED_BYTE,
	       m_lut);

  // enable drag texture
  glActiveTexture(GL_TEXTURE1);
  glEnable(GL_TEXTURE_RECTANGLE_ARB);
  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_dataTex);


  // generate prune texture on gpu
  pruneBuffer1->bind();
  glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);
  glUseProgramObjectARB(m_pruneShader);

  int dtextureX = m_pruneBuffer->width();
  int dtextureY = m_pruneBuffer->height();

  int ncols = m_dragInfo.x;
  int nrows = m_dragInfo.y;
  int lod = m_dragInfo.z;
  int gridx = dtextureX/ncols;
  int gridy = dtextureY/nrows;
  int gridz = m_subVolSize.z/lod;

  glUniform1iARB(m_pruneParm[0], 0); // lutTex
  glUniform1iARB(m_pruneParm[1], 1); // dragTex
  glUniform1iARB(m_pruneParm[2], gridx); // gridx
  glUniform1iARB(m_pruneParm[3], gridy); // gridy
  glUniform1iARB(m_pruneParm[4], gridz); // gridz
  glUniform1iARB(m_pruneParm[5], nrows); // nrows
  glUniform1iARB(m_pruneParm[6], ncols); // ncols

  StaticFunctions::pushOrthoView(0, 0, dtextureX, dtextureY);
  StaticFunctions::drawQuad(0, 0, dtextureX, dtextureY, 1.0);
  StaticFunctions::popOrthoView();

  glFinish();
  pruneBuffer1->release();
  glDisable(GL_TEXTURE_RECTANGLE_ARB);
  glUseProgramObjectARB(0);
}

#define BIND()								\
  {									\
    glActiveTexture(GL_TEXTURE1);					\
    glEnable(GL_TEXTURE_RECTANGLE_ARB);					\
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, pruneBuffer1->texture());	\
									\
    pruneBuffer2->bind();						\
    glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);				\
    glDisable(GL_DEPTH_TEST);						\
    glDisable(GL_BLEND);						\
  }
//    glClearColor(0, 0, 0, 0);						\
//    glClear(GL_COLOR_BUFFER_BIT);					\

#define DRAW_RELEASE()						\
  {								\
    StaticFunctions::pushOrthoView(0, 0, m_dtexX, m_dtexY);	\
    StaticFunctions::drawQuad(0, 0, m_dtexX, m_dtexY, 1.0);	\
    StaticFunctions::popOrthoView();				\
    								\
    glFinish();							\
    pruneBuffer2->release();					\
								\
    glActiveTexture(GL_TEXTURE1);				\
    glDisable(GL_TEXTURE_RECTANGLE_ARB);			\
  								\
    glUseProgramObjectARB(0);					\
  }


void
PruneHandler::modifyPruneTexture(int shaderType,
				 QGLFramebufferObject *pruneBuffer1,
				 QGLFramebufferObject *pruneBuffer2,
				 QVariantList vlist)
{
  BIND()

  GLint *parm;
  if (shaderType == CopyShader)
    {
      glUseProgramObjectARB(Global::copyShader());
      parm = Global::copyParm();
    }
  else if (shaderType == InvertShader)
    {
      glUseProgramObjectARB(m_invertShader);
      parm = m_invertParm;
    }
  else if (shaderType == RemovePatchShader)
    {
      glUseProgramObjectARB(m_removePatchShader);
      parm = m_removePatchParm;
    }
  else if (shaderType == AverageShader)
    {
      glUseProgramObjectARB(m_averageShader);
      parm = m_averageParm;
    }
  else if (shaderType == PatternShader)
    {
      glUseProgramObjectARB(m_patternShader);
      parm = m_patternParm;
    }
  else if (shaderType == SetValueShader)
    {
      glUseProgramObjectARB(m_setValueShader);
      parm = m_setValueParm;
    }
  else if (shaderType == DilateShader)
    {
      glUseProgramObjectARB(m_dilateShader);
      parm = m_dilateParm;
    }
  else if (shaderType == RestrictedDilateShader)
    {
      glUseProgramObjectARB(m_rDilateShader);
      parm = m_rDilateParm;
    }
  else if (shaderType == ErodeShader)
    {
      glUseProgramObjectARB(m_erodeShader);
      parm = m_erodeParm;
    }
  else if (shaderType == ShrinkShader)
    {
      glUseProgramObjectARB(m_shrinkShader);
      parm = m_shrinkParm;
    }
  else if (shaderType == ThickenShader)
    {
      glUseProgramObjectARB(m_thickenShader);
      parm = m_thickenParm;
    }
  else if (shaderType == EdgeShader)
    {
      glUseProgramObjectARB(m_edgeShader);
      parm = m_edgeParm;
    }
  else if (shaderType == CarveShader)
    {
      glUseProgramObjectARB(m_carveShader);
      parm = m_carveParm;
    }
  else if (shaderType == PaintShader)
    {
      glUseProgramObjectARB(m_paintShader);
      parm = m_paintParm;
    }
  else if (shaderType == ClipShader)
    {
      glUseProgramObjectARB(m_clipShader);
      parm = m_clipParm;
    }
  else if (shaderType == CropShader)
    {
      glUseProgramObjectARB(m_cropShader);
      parm = m_cropParm;
    }
  else if (shaderType == DilateEdgeShader)
    {
      glUseProgramObjectARB(m_dilateEdgeShader);
      parm = m_dilateEdgeParm;
    }
  else if (shaderType == LocalMaxShader)
    {
      glUseProgramObjectARB(m_localmaxShader);
      parm = m_localmaxParm;
    }
  else if (shaderType == LocalThicknessShader)
    {
      glUseProgramObjectARB(m_localThicknessShader);
      parm = m_localThicknessParm;
    }
  else if (shaderType == TriangleShader)
    {
      glUseProgramObjectARB(m_triShader);
      parm = m_triParm;
    }
  else if (shaderType == SmoothChannelShader)
    {
      glUseProgramObjectARB(m_smoothChannelShader);
      parm = m_smoothChannelParm;
    }

  glUniform1iARB(parm[0], 1); // prunebuffer1    

  if (shaderType == RemovePatchShader)
    {
      bool remove = vlist[0].toBool();
      glUniform1iARB(parm[1], remove);
    }
  else if (shaderType == InvertShader)
    {
      int chan = vlist[0].toInt();
      glUniform1iARB(parm[1], chan);
    }
  else if (shaderType == AverageShader)
    {
      int chan1 = vlist[0].toInt();
      int chan2 = vlist[1].toInt();
      int dst = vlist[2].toInt();
      glUniform1iARB(parm[1], chan1);
      glUniform1iARB(parm[2], chan2);
      glUniform1iARB(parm[3], dst);
    }
  else if (shaderType == SetValueShader)
    {
      int val = vlist[0].toInt();
      glUniform1iARB(parm[1], val);

      int chan = vlist[1].toInt();
      glUniform1iARB(parm[2], chan);

      int minval = vlist[2].toInt();
      glUniform1iARB(parm[3], minval);

      int maxval = vlist[3].toInt();
      glUniform1iARB(parm[4], maxval);
    }
  else if (shaderType > InvertShader)
    {
      int ncols = m_dragInfo.x;
      int nrows = m_dragInfo.y;
      int lod = m_dragInfo.z;
      int gridx = m_dtexX/ncols;
      int gridy = m_dtexY/nrows;
      int gridz = m_subVolSize.z/lod;

      glUniform1iARB(parm[1], gridx); // gridx
      glUniform1iARB(parm[2], gridy); // gridy
      glUniform1iARB(parm[3], gridz); // gridz
      glUniform1iARB(parm[4], nrows); // nrows
      glUniform1iARB(parm[5], ncols); // ncols

      if (shaderType == DilateShader ||
	  shaderType == ErodeShader ||
	  shaderType == ShrinkShader)
	{
	  int chan = vlist[0].toInt();
	  glUniform1iARB(parm[6], chan);
	}
      else if (shaderType == ThickenShader)
	{
	  bool cityblock = vlist[0].toBool();
	  glUniform1iARB(parm[6], cityblock);
	}
      else if (shaderType == CarveShader ||
	       shaderType == PaintShader)
	{	  
	  float cen[3];
	  cen[0] = vlist[0].toFloat();
	  cen[1] = vlist[1].toFloat();
	  cen[2] = vlist[2].toFloat();
	  float rad = vlist[3].toFloat();
	  float decay = vlist[4].toFloat();
	  int docarve  = vlist[5].toInt();
	  glUniform3fARB(parm[6], cen[0],cen[1],cen[2]);
	  glUniform1fARB(parm[7], rad);
	  glUniform1fARB(parm[8], decay);
	  glUniform1iARB(parm[9], docarve);

	  if (shaderType == PaintShader)
	    {
	      float tag = vlist[6].toFloat();
	      glUniform1fARB(parm[10], tag);
	    }

	  bool planarcarve = vlist[7].toBool();
	  glUniform1iARB(parm[11], planarcarve);
	  
	  if (planarcarve)
	    {
	      glUniform3fARB(parm[12], m_carveP.x,m_carveP.y,m_carveP.z);
	      glUniform3fARB(parm[13], m_carveN.x,m_carveN.y,m_carveN.z);
	      glUniform1fARB(parm[14], m_carveT);
	      glUniform3fARB(parm[15], m_carveX.x,m_carveX.y,m_carveX.z);
	      glUniform3fARB(parm[16], m_carveY.x,m_carveY.y,m_carveY.z);
	    }
	}
      else if (shaderType == CropShader)
	{	  
	  int lod;
	  float vs[3],dmin[3];
	  lod = vlist[0].toInt();
	  vs[0] = vlist[1].toFloat();
	  vs[1] = vlist[2].toFloat();
	  vs[2] = vlist[3].toFloat();
	  dmin[0] = vlist[4].toFloat();
	  dmin[1] = vlist[5].toFloat();
	  dmin[2] = vlist[6].toFloat();

	  glUniform1iARB(parm[6], lod);
	  glUniform3fARB(parm[7], vs[0],vs[1],vs[2]);
	  glUniform3fARB(parm[8], dmin[0],dmin[1],dmin[2]);
	}
      else if (shaderType == ClipShader)
	{	  
	  float cen[3], nrm[3];
	  cen[0] = vlist[0].toFloat();
	  cen[1] = vlist[1].toFloat();
	  cen[2] = vlist[2].toFloat();
	  nrm[0] = vlist[3].toFloat();
	  nrm[1] = vlist[4].toFloat();
	  nrm[2] = vlist[5].toFloat();

	  glUniform3fARB(parm[6], cen[0],cen[1],cen[2]);
	  glUniform3fARB(parm[7], nrm[0],nrm[1],nrm[2]);
	}
      else if (shaderType == TriangleShader)
	{	  
	  float A[3],B[3],C[3];
	  A[0] = vlist[0].toFloat();
	  A[1] = vlist[1].toFloat();
	  A[2] = vlist[2].toFloat();
	  B[0] = vlist[3].toFloat();
	  B[1] = vlist[4].toFloat();
	  B[2] = vlist[5].toFloat();
	  C[0] = vlist[6].toFloat();
	  C[1] = vlist[7].toFloat();
	  C[2] = vlist[8].toFloat();
	  float thick = vlist[9].toInt();
	  int val = vlist[10].toInt();
	  bool paint = vlist[11].toBool();
	  glUniform3fARB(parm[6], A[0],A[1],A[2]);
	  glUniform3fARB(parm[7], B[0],B[1],B[2]);
	  glUniform3fARB(parm[8], C[0],C[1],C[2]);
	  glUniform1fARB(parm[9], thick);
	  glUniform1iARB(parm[10], val);
	  glUniform1iARB(parm[11], paint);
	}
      else if (shaderType == RestrictedDilateShader ||
	       shaderType == EdgeShader ||
	       shaderType == DilateEdgeShader ||
	       shaderType == LocalThicknessShader ||
	       shaderType == SmoothChannelShader)
	{
	  int val = vlist[0].toInt();
	  glUniform1iARB(parm[6], val);

	  if (shaderType == EdgeShader)
	    {
	      int sz = vlist[1].toInt();
	      glUniform1iARB(parm[7], sz);
	    }
	}
      else if (shaderType == PatternShader)
	{
	  bool flag = vlist[0].toBool();
	  glUniform1iARB(parm[6], flag);
	  int pat[6];
	  pat[0] = vlist[1].toInt();
	  pat[1] = vlist[2].toInt();
	  pat[2] = vlist[3].toInt();
	  pat[3] = vlist[4].toInt();
	  pat[4] = vlist[5].toInt();
	  pat[5] = vlist[6].toInt();
	  glUniform1ivARB(parm[7], 6, pat);
	}
    }

  DRAW_RELEASE()
}


void
PruneHandler::copyChannelTexture(int src, int dst,
				 QGLFramebufferObject *pruneBuffer1,
				 QGLFramebufferObject *pruneBuffer2)
{
  m_mopActive = true;

  BIND()

  GLint *parm;
  glUseProgramObjectARB(m_copyChannelShader);
  parm = m_copyChannelParm;

  glUniform1iARB(parm[0], 1); // prunebuffer1
  glUniform1iARB(parm[1], src);
  glUniform1iARB(parm[2], dst);

  int dtextureX = pruneBuffer2->width();
  int dtextureY = pruneBuffer2->height();

  DRAW_RELEASE()
}

#define DILATE(sz, chan)				\
  QVariantList vlistd;					\
  vlistd << QVariant(chan);				\
  for(int ne=0; ne<sz; ne++)				\
    {							\
      modifyPruneTexture(DilateShader,			\
			 m_pruneBuffer,			\
			 pruneBuffer1,			\
			 vlistd);			\
							\
      QGLFramebufferObject *tpb = m_pruneBuffer;	\
      m_pruneBuffer = pruneBuffer1;			\
      pruneBuffer1 = tpb;				\
    }							

#define ERODE(sz, chan)					\
  QVariantList vliste;					\
  vliste << QVariant(chan);				\
  for(int ne=0; ne<sz; ne++)				\
    {							\
      modifyPruneTexture(ErodeShader,			\
			 m_pruneBuffer,			\
			 pruneBuffer1,			\
			 vliste);			\
							\
      QGLFramebufferObject *tpb = m_pruneBuffer;	\
      m_pruneBuffer = pruneBuffer1;			\
      pruneBuffer1 = tpb;				\
    } 

void
PruneHandler::genBuffer(int dtextureX, int dtextureY)
{
  if (m_pruneBuffer)
    {
      if (m_pruneBuffer->width() != dtextureX ||
	  m_pruneBuffer->height() != dtextureY)
	{
	  delete m_pruneBuffer;
	  m_pruneBuffer = 0;

	  delete m_savedPruneBuffer;
	  m_savedPruneBuffer = 0;
	}
    }

  if (!m_pruneBuffer)
    m_pruneBuffer = newFBO();
  
  if (m_useSavedBuffer)
    {      
      if (!m_savedPruneBuffer ||
	  m_pruneBuffer->size() != m_savedPruneBuffer->size())
	QMessageBox::information(0,
				 "Error using saved buffer",
				 "No saved buffer or buffer size not correct\n Generating new one.");
    }

}

//bool firstTimePruneTextureGeneration = true;

void
PruneHandler::updateAndLoadPruneTexture(GLuint dataTex,
					int dtextureX, int dtextureY,
					Vec dragInfo, Vec subVolSize,
					uchar *vlut)
{
  bool prevModified = m_mopActive;
  m_mopActive = false;


  m_dtexX = dtextureX;
  m_dtexY = dtextureY;
  m_dragInfo = dragInfo;
  m_subVolSize = subVolSize;
  m_dataTex = dataTex;

  if (!standardChecks()) return;
  
  //--------------------
  bool prune = true;
  if (m_lut && vlut)
    {
      prune = false;
      if (Global::maskTF() == -1)
	{
	  uchar plut[256*256];
	  memset(plut, 0, 256*256);
	  for(int l=0; l<Global::lutSize(); l++)
	    {
	      int lsize = l*256*256*4;
	      for(int i=0; i<256*256; i++)
		plut[i] = qMax(plut[i], vlut[lsize + 4*i+3]);
	    }
	  for(int i=0; i<256*256; i++)
	    {
	      bool pl = plut[i] > 0;
	      bool ml = m_lut[i] > 0;
	      if (pl != ml)
		{
		  prune = true;
		  break;
		}
	    }
	}
      else
	{
	  int l = Global::maskTF();
	  int lsize = l*256*256*4;
	  for(int i=0; i<256*256; i++)
	    {
	      bool pl = vlut[lsize + 4*i+3] > 0;
	      bool ml = m_lut[i] > 0;
	      if (pl != ml)
		{
		  prune = true;
		  break;
		}
	    }
	}
    }

  if (!prevModified && !prune)
    {
      //QMessageBox::information(0, "", QString("ret %1 %2").arg(prevModified).arg(prune));
      return;
    }
  //--------------------

  genBuffer(m_dtexX, m_dtexY);

//  // copy channel 2 into saved buffer
//  // save tag information
//  if (prevModified)
//    copyToFromSavedChannel(true, 2, 2, false);


  MainWindowUI::mainWindowUI()->menubar->parentWidget()->\
    setWindowTitle("Updating prune texture");

  generatePruneTexture(m_pruneBuffer, vlut);

  dilate(1, 0);
  dilate(1, 1);
  m_mopActive = false;

  // copy channel 2 back from saved buffer
  // retrieve tag information
//  if (!firstTimePruneTextureGeneration)
//    copyToFromSavedChannel(false, 2, 2, false); 
//  firstTimePruneTextureGeneration = false;


//  if (prevModified)
//    copyToFromSavedChannel(false, 2, 2, false); 
//  else
    m_mopActive = false;


    //QMessageBox::information(0, "", QString("%1 %2").arg(prevModified).arg(m_mopActive));

  MainWindowUI::mainWindowUI()->menubar->parentWidget()->\
    setWindowTitle("Drishti");
}

void
PruneHandler::copyBuffer(bool flag)
{
  m_mopActive = true;

  // true  => copy from m_pruneBuffer to m_savedPruneBuffer
  // false => copy from m_savedPruneBuffer to m_pruneBuffer

  if (!m_savedPruneBuffer)
    m_savedPruneBuffer = newFBO();

  WRITECHANNEL(m_channel);

  if (flag)
    modifyPruneTexture(CopyShader,
		       m_pruneBuffer,
		       m_savedPruneBuffer);
  else
    modifyPruneTexture(CopyShader,
		       m_savedPruneBuffer,
		       m_pruneBuffer);

  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

void
PruneHandler::copyChannel(int src, int dst, bool doPrint)
{
  if (!standardChecks()) return;
  m_mopActive = true;

  // assuming m_pruneBuffer exists
  QGLFramebufferObject *pruneBuffer1 = newFBO();

  modifyPruneTexture(CopyShader,
		     m_pruneBuffer,
		     pruneBuffer1);

  copyChannelTexture(src, dst,
		     pruneBuffer1,
		     m_pruneBuffer);
  
  delete pruneBuffer1;

  if (doPrint)
    QMessageBox::information(0,"Copy Channel","Done"); 
}

void
PruneHandler::copyToFromSavedChannel(bool toSaved, int src, int dst, bool showmesg)
{
  if (!standardChecks()) return;
  m_mopActive = true;

  // assuming m_pruneBuffer exists

  if (!m_savedPruneBuffer ||
      m_pruneBuffer->size() != m_savedPruneBuffer->size())
    {
      if (m_savedPruneBuffer) delete m_savedPruneBuffer;

      m_savedPruneBuffer = newFBO();
    }

  int dtextureX = m_pruneBuffer->width();
  int dtextureY = m_pruneBuffer->height();

  WRITECHANNEL(dst)

  if (toSaved)
    {
      copyChannelTexture(src, dst,
			 m_pruneBuffer,
			 m_savedPruneBuffer);
      if (showmesg)
	QMessageBox::information(0,"Copy to saved channel","Done"); 
    }
  else
    {
      copyChannelTexture(src, dst,
			 m_savedPruneBuffer,
			 m_pruneBuffer);
      if (showmesg)
	QMessageBox::information(0,"Copy from saved channel","Done"); 
    }

  WRITECHANNEL(-1)

}

void
PruneHandler::invert(int chan)
{
  if (!standardChecks()) return;
  m_mopActive = true;

  QGLFramebufferObject *pruneBuffer1 = newFBO();

  QVariantList vlist;
  vlist << QVariant(chan);

  modifyPruneTexture(InvertShader,
		     m_pruneBuffer,
		     pruneBuffer1,
		     vlist);

  swapFBO(m_pruneBuffer, pruneBuffer1);

  delete pruneBuffer1;
}

void
PruneHandler::removePatch(bool remove)
{
  if (!standardChecks()) return;
  m_mopActive = true;

  QGLFramebufferObject *pruneBuffer1 = newFBO();

  QVariantList vlist;
  vlist << QVariant(remove);

  modifyPruneTexture(RemovePatchShader,
		     m_pruneBuffer,
		     pruneBuffer1,
		     vlist);

  swapFBO(m_pruneBuffer, pruneBuffer1);

  delete pruneBuffer1;
}

void
PruneHandler::setValue(int val, int chan, int minval, int maxval)
{
  if (!standardChecks()) return;
  m_mopActive = true;

  QGLFramebufferObject *pruneBuffer1 = newFBO();

  QVariantList vlist;
  vlist << QVariant(val);
  vlist << QVariant(chan);
  vlist << QVariant(minval);
  vlist << QVariant(maxval);

  modifyPruneTexture(SetValueShader,
		     m_pruneBuffer,
		     pruneBuffer1,
		     vlist);

  swapFBO(m_pruneBuffer, pruneBuffer1);

  delete pruneBuffer1;
}

void
PruneHandler::dilate(int sz, int chan)
{
  if (!standardChecks()) return;
  m_mopActive = true;

  QGLFramebufferObject *pruneBuffer1 = newFBO();

  DILATE(sz, chan)

  delete pruneBuffer1;    
}

void
PruneHandler::erode(int sz, int chan)
{
  if (!standardChecks()) return;
  m_mopActive = true;

  QGLFramebufferObject *pruneBuffer1 = newFBO();

  ERODE(sz, chan)

  delete pruneBuffer1;    
}

void
PruneHandler::shrink(int sz, int chan)
{
  if (!standardChecks()) return;
  m_mopActive = true;

  QGLFramebufferObject *pruneBuffer1 = newFBO();

  QVariantList vlist;
  vlist << QVariant(chan);
  for(int ne=0; ne<sz; ne++)
    {
      modifyPruneTexture(ShrinkShader,
			 m_pruneBuffer,
			 pruneBuffer1,
			 vlist);

      QGLFramebufferObject *tpb = m_pruneBuffer;
      m_pruneBuffer = pruneBuffer1;
      pruneBuffer1 = tpb;
    } 
  delete pruneBuffer1;    
}

void
PruneHandler::open(int sz, int chan)
{
  if (!standardChecks()) return;
  m_mopActive = true;

  QGLFramebufferObject *pruneBuffer1 = newFBO();

  ERODE(sz, chan)
  DILATE(sz, chan)

  delete pruneBuffer1;    
}

void
PruneHandler::close(int sz, int chan)
{
  if (!standardChecks()) return;
  m_mopActive = true;

  QGLFramebufferObject *pruneBuffer1 = newFBO();

  DILATE(sz, chan)
  ERODE(sz, chan)

  delete pruneBuffer1;    
}

void
PruneHandler::thicken(int sz, bool cityBlock)
{
  if (!standardChecks()) return;
  m_mopActive = true;

  QProgressDialog progress("Thicken",
			   QString(),
			   0, 100,
			   0);
  progress.setCancelButton(0);

  QGLFramebufferObject *pruneBuffer1 = newFBO();
  QGLFramebufferObject *pruneBuffer2 = newFBO();

  modifyPruneTexture(CopyShader,
		     m_pruneBuffer,
		     pruneBuffer1);

  QVariantList vlist;
  vlist << QVariant(cityBlock);

  for(int ne=0; ne<sz; ne++)
    {
      progress.setValue(100*(float)ne/(float)sz);

      modifyPruneTexture(ThickenShader,
			 pruneBuffer1,
			 pruneBuffer2,
			 vlist);

      swapFBO(pruneBuffer2, pruneBuffer1);
    }

  modifyPruneTexture(CopyShader,
		     pruneBuffer1,
		     m_pruneBuffer);

  delete pruneBuffer2;
  delete pruneBuffer1;    
}

void
PruneHandler::shrinkwrap(int sz, int sz2)

{
  if (!standardChecks()) return;
  m_mopActive = true;

  if (sz < 1) return;


  QProgressDialog progress("Applying Shrinkwrap",
			   QString(),
			   0, 100,
			   0);
  progress.setCancelButton(0);

  QGLFramebufferObject *pruneBuffer1 = newFBO();
  QGLFramebufferObject *pruneBuffer2 = newFBO();

  progress.setLabelText("Shrinkwrap : copy");
  progress.setValue(10);
  modifyPruneTexture(CopyShader,
		     m_pruneBuffer,
		     pruneBuffer1);

  bool cityBlock = false; // use chessboard distance transform
  QVariantList vlist;
  vlist << QVariant(cityBlock);

  progress.setLabelText("Shrinkwrap : thicken");
  for(int ne=0; ne<sz; ne++)
    {
      progress.setValue(100*(float)ne/(float)sz);

      modifyPruneTexture(ThickenShader,
			 pruneBuffer1,
			 pruneBuffer2,
			 vlist);

      swapFBO(pruneBuffer2, pruneBuffer1);
    }      
  
  progress.setLabelText("Shrinkwrap : edge");
  vlist.clear();
  vlist << QVariant(0); // set edge value to 0
  vlist << QVariant(1); // 1-voxel thin edge
  modifyPruneTexture(EdgeShader,
		     pruneBuffer1,
		     m_pruneBuffer,
		     vlist);

  int ssz = sz+sz2; // sz2 give more control over shrinkage after thickening
  for(int ne=0; ne<ssz; ne++)
    {
      progress.setLabelText("Shrinkwrap : thicken edge");
      progress.setValue(100*(float)ne/(float)ssz);
      
      int erodeval = 255 - (ssz-1-ne);
      QVariantList vlist;
      vlist << QVariant(erodeval);
      
      modifyPruneTexture(DilateEdgeShader,
			 m_pruneBuffer,
			 pruneBuffer1,
			 vlist);

      swapFBO(m_pruneBuffer, pruneBuffer1);
    }

  
  delete pruneBuffer1;
  delete pruneBuffer2;

  progress.setValue(100);
}

void
PruneHandler::distanceTransform(int sz,  bool cityBlock)
{
  if (!standardChecks()) return;
  m_mopActive = true;

  if (sz < 1) return;

  QString cbstr;
  if (cityBlock) cbstr = "city-block : ";
  else cbstr = "chess-board : ";


  // just make sure that channel 0 values are either 0 or 255 and nothing in between
  setValue(255, 0, 0, 255);

  QProgressDialog progress("Applying city-block distance transform",
			   QString(),
			   0, 100,
			   0);
  progress.setCancelButton(0);

  QGLFramebufferObject *pruneBuffer1 = newFBO();
  QGLFramebufferObject *pruneBuffer2 = newFBO();


  QVariantList vlist;
  vlist << QVariant(0); // invert channel 0

  progress.setLabelText(cbstr + "invert");
  progress.setValue(10);
  modifyPruneTexture(InvertShader,
		     m_pruneBuffer,
		     pruneBuffer1,
		     vlist);

  vlist.clear();
  vlist << QVariant(cityBlock);

  progress.setLabelText(cbstr+"thicken");
  for(int ne=0; ne<sz; ne++)
    {
      progress.setValue(100*(float)ne/(float)sz);
      modifyPruneTexture(ThickenShader,
			 pruneBuffer1,
			 pruneBuffer2,
			 vlist);

      swapFBO(pruneBuffer1, pruneBuffer2);
    }      
  
  vlist.clear();
  vlist << QVariant(0); // invert channel 0
  progress.setLabelText(cbstr+"invert");
  modifyPruneTexture(InvertShader,
		     pruneBuffer1,
		     m_pruneBuffer,
		     vlist);
  
  delete pruneBuffer1;
  delete pruneBuffer2;

  progress.setValue(100);
}

void
PruneHandler::minmax(bool useMin, int ch1, int ch2)
{
  if (!standardChecks()) return;
  m_mopActive = true;

  if (!m_savedPruneBuffer ||
      m_pruneBuffer->size() != m_savedPruneBuffer->size())
    {
      QMessageBox::information(0,
			       "Error using saved buffer",
			       "No saved buffer or buffer size not correct\nCannot proceed."); 
      return;
    }

  QGLFramebufferObject *pruneBuffer1 = newFBO();

  glActiveTexture(GL_TEXTURE0); 
  glEnable(GL_TEXTURE_RECTANGLE_ARB); 
  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_pruneBuffer->texture()); 
 
  glActiveTexture(GL_TEXTURE1); 
  glEnable(GL_TEXTURE_RECTANGLE_ARB); 
  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_savedPruneBuffer->texture()); 

  pruneBuffer1->bind(); 
  glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT); 
  glDisable(GL_DEPTH_TEST); 
  glDisable(GL_BLEND); 

  GLint *parm;
  if (useMin)
    {
      glUseProgramObjectARB(m_minShader);
      parm = m_minParm;
    }
  else
    {
      glUseProgramObjectARB(m_maxShader);
      parm = m_maxParm;
    }
  glUniform1iARB(parm[0], 0); // m_prunebuffer    
  glUniform1iARB(parm[1], 1); // m_savedPrunebuffer
  glUniform1iARB(parm[2], ch1);
  glUniform1iARB(parm[3], ch2);

  StaticFunctions::pushOrthoView(0, 0, m_dtexX, m_dtexY); 
  StaticFunctions::drawQuad(0, 0, m_dtexX, m_dtexY, 1.0); 
  StaticFunctions::popOrthoView(); 
 
  pruneBuffer1->release(); 
 
  glActiveTexture(GL_TEXTURE0); 
  glDisable(GL_TEXTURE_RECTANGLE_ARB); 

  glActiveTexture(GL_TEXTURE1); 
  glDisable(GL_TEXTURE_RECTANGLE_ARB); 
  glUseProgramObjectARB(0); 

  swapFBO(m_pruneBuffer, pruneBuffer1);

  delete pruneBuffer1;
}

void
PruneHandler::xorTexture(int ch1, int ch2)
{
  if (!standardChecks()) return;
  m_mopActive = true;

  if (!m_savedPruneBuffer ||
      m_pruneBuffer->size() != m_savedPruneBuffer->size())
    {
      QMessageBox::information(0,
			       "Error using saved buffer",
			       "No saved buffer or buffer size not correct\nCannot proceed."); 
      return;
    }

  QGLFramebufferObject *pruneBuffer1 = newFBO();

  glActiveTexture(GL_TEXTURE0); 
  glEnable(GL_TEXTURE_RECTANGLE_ARB); 
  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_pruneBuffer->texture()); 
 
  glActiveTexture(GL_TEXTURE1); 
  glEnable(GL_TEXTURE_RECTANGLE_ARB); 
  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_savedPruneBuffer->texture()); 

  pruneBuffer1->bind(); 
  glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT); 
  glDisable(GL_DEPTH_TEST); 
  glDisable(GL_BLEND); 

  glUseProgramObjectARB(m_xorShader);
  glUniform1iARB(m_xorParm[0], 0); // m_prunebuffer    
  glUniform1iARB(m_xorParm[1], 1); // m_savedPrunebuffer
  glUniform1iARB(m_xorParm[2], ch1);
  glUniform1iARB(m_xorParm[3], ch2);

  StaticFunctions::pushOrthoView(0, 0, m_dtexX, m_dtexY); 
  StaticFunctions::drawQuad(0, 0, m_dtexX, m_dtexY, 1.0); 
  StaticFunctions::popOrthoView(); 
 
  pruneBuffer1->release(); 
 
  glActiveTexture(GL_TEXTURE0); 
  glDisable(GL_TEXTURE_RECTANGLE_ARB); 

  glActiveTexture(GL_TEXTURE1); 
  glDisable(GL_TEXTURE_RECTANGLE_ARB); 
  glUseProgramObjectARB(0); 

  swapFBO(m_pruneBuffer, pruneBuffer1);

  delete pruneBuffer1;
}

void
PruneHandler::edge(int val, int sz)
{
  if (!standardChecks()) return;
  m_mopActive = true;

  QVariantList vlist;
  vlist << QVariant(val);
  vlist << QVariant(sz);

  QGLFramebufferObject *pruneBuffer1 = newFBO();
  modifyPruneTexture(EdgeShader,
		     m_pruneBuffer,
		     pruneBuffer1,
		     vlist);
  swapFBO(m_pruneBuffer, pruneBuffer1);

  delete pruneBuffer1;
}

void
PruneHandler::localMaximum()
{
  if (!standardChecks()) return;
  m_mopActive = true;

  QGLFramebufferObject *pruneBuffer1 = newFBO();
  modifyPruneTexture(LocalMaxShader,
		     m_pruneBuffer,
		     pruneBuffer1);
  swapFBO(m_pruneBuffer, pruneBuffer1);

  delete pruneBuffer1;
}

void
PruneHandler::dilateEdge(int sz1, int sz2)
{
  if (!standardChecks()) return;
  m_mopActive = true;


  QProgressDialog progress("Dilate edge",
			   QString(),
			   0, 100,
			   0);
  progress.setCancelButton(0);

  QGLFramebufferObject *pruneBuffer1 = newFBO();

  for(int ne=sz1; ne<=sz2; ne++)
    {
      progress.setValue(100*(float)(ne-sz1)/(float)(sz2-sz1+1));
      
      int erodeval = ne;
      QVariantList vlist;
      vlist << QVariant(erodeval);
      
      modifyPruneTexture(DilateEdgeShader,
			 m_pruneBuffer,
			 pruneBuffer1,
			 vlist);

      swapFBO(m_pruneBuffer, pruneBuffer1);
    }

  delete pruneBuffer1;
}

void
PruneHandler::restrictedDilate(int val, int sz)
{
  if (!standardChecks()) return;
  m_mopActive = true;

  QProgressDialog progress("Restricted Dilate",
			   QString(),
			   0, 100,
			   0);
  progress.setCancelButton(0);

  QGLFramebufferObject *pruneBuffer1 = newFBO();

  QVariantList vlist;
  vlist << QVariant(val);

  for(int ne=0; ne<sz; ne++)
    {
      progress.setValue(100*(float)ne/(float)sz);
            
      modifyPruneTexture(RestrictedDilateShader,
			 m_pruneBuffer,
			 pruneBuffer1,
			 vlist);
      
      swapFBO(m_pruneBuffer, pruneBuffer1);
    }

  delete pruneBuffer1;
}

bool PruneHandler::blend() { return m_blendActive; }
void PruneHandler::setBlend(bool flag) { m_blendActive = flag; }

bool PruneHandler::paint() { return m_paintActive; }
void PruneHandler::setPaint(bool flag)
{
  m_paintActive = flag;
  if (m_paintActive) m_carveActive = false;
}
void PruneHandler::setTag(int tag) { m_tag = tag; }
int PruneHandler::tag() { return m_tag; }

bool PruneHandler::carve() { return m_carveActive; }
void PruneHandler::setCarve(bool flag) { m_carveActive = flag; }
void
PruneHandler::setCarveRad(float rad, float decay)
{
  if (rad > -1.0) m_carveRadius = rad;
  if (decay > -1.0) m_carveDecay = decay;
}
void
PruneHandler::carveRad(float& rad, float& decay)
{
  rad = m_carveRadius;
  decay = m_carveDecay;
}
void
PruneHandler::setPlanarCarve(Vec cp, Vec cn, Vec cx, Vec cy, float ct, Vec dmin)
{
  int lod = m_dragInfo.z;
  m_carveP = (cp-dmin)/lod;
  m_carveN = cn;
  m_carveX = cx;
  m_carveY = cy;
  m_carveT = ct;
}

void
PruneHandler::sculpt(int docarve, Vec dmin,
		     QList<Vec> points,
		     float rad, float decay,
		     int tag)
{
  if (!standardChecks()) return;

  m_mopActive = true;
  
  int lod = m_dragInfo.z;

  float crad, cdecay;
  if (rad < 0.0f)
    {
      crad = m_carveRadius;
      cdecay = m_carveDecay;
    }
  else
    {
      crad = rad;
      cdecay = decay;
    }

  int ctag;
  if (tag < 0)
    ctag = m_tag;
  else
    ctag = tag;

  if (docarve == 2) // restore
    ctag = 0;

  bool planarCarve = (m_carveN.squaredNorm() > 0.1);

  QGLFramebufferObject *pruneBuffer1 = newFBO();
  for(int i=0; i<points.count(); i++)
    {
      Vec p = points[i];
      p -= dmin;
      p /= lod;

      QVariantList vlist;
      vlist << QVariant((float)(p.x));
      vlist << QVariant((float)(p.y));
      vlist << QVariant((float)(p.z));
      vlist << QVariant(crad);
      vlist << QVariant(cdecay);
      vlist << QVariant(docarve);
      vlist << QVariant(ctag);
      vlist << QVariant(planarCarve);

      if (m_paintActive || docarve == 0)
	modifyPruneTexture(PaintShader,
			   m_pruneBuffer,
			   pruneBuffer1,
			   vlist);
      else
	modifyPruneTexture(CarveShader,
			   m_pruneBuffer,
			   pruneBuffer1,
			   vlist);
      
      swapFBO(m_pruneBuffer, pruneBuffer1);
    }

  delete pruneBuffer1;
}

QByteArray
PruneHandler::interpolate(QByteArray cpb1, QByteArray cpb2, float frc)
{
  QByteArray pb;

  if (cpb2.isEmpty() && cpb1.isEmpty())
    {
      return pb;
    }

  if (cpb2.isEmpty())
    {
      pb = cpb1;
      return pb;
    }

  if (cpb1.isEmpty())
    {
      pb = cpb2;
      return pb;
    }

  QByteArray pb1 = qUncompress(cpb1);
  QByteArray pb2 = qUncompress(cpb2);
  
  if (pb1.count() != pb2.count())
    {
      QMessageBox::information(0, "", QString("%1 %2").arg(pb1.count()).arg(pb2.count()));
      return pb;
    }

  uchar *tmp = new uchar[pb1.count()];
  for (int i=0; i<pb1.count(); i++)
    tmp[i] = (1.0f-frc)*(uchar)pb1[i] + frc*(uchar)pb2[i];

  pb = QByteArray((char*)tmp, pb1.count());
  QByteArray cpb = qCompress(pb, 0); // no compression

  delete [] tmp;
  return cpb;
}

void
PruneHandler::fillPathPatch(Vec dmin,
			    QList<Vec> points,
			    int thick, int val,
			    bool paint)
{
  if (!standardChecks()) return;
  if (points.count() < 3) return;

  m_mopActive = true;
  
  int lod = m_dragInfo.z;

  Vec centroid = points[0];
  for(int i=1; i<points.count(); i++)
    centroid += points[i];
  centroid /= points.count();
  centroid -= dmin;
  centroid /= lod;
  
  QGLFramebufferObject *pruneBuffer1 = newFBO();
  for(int i=0; i<points.count(); i++)
    {
      int i1 = i+1;
      if (i1 >= points.count())
	i1 = 0;

      Vec p = points[i];
      p -= dmin;
      p /= lod;

      Vec q = points[i1];
      q -= dmin;
      q /= lod;

      QVariantList vlist;
      vlist << QVariant((float)(centroid.x));
      vlist << QVariant((float)(centroid.y));
      vlist << QVariant((float)(centroid.z));
      vlist << QVariant((float)(p.x));
      vlist << QVariant((float)(p.y));
      vlist << QVariant((float)(p.z));
      vlist << QVariant((float)(q.x));
      vlist << QVariant((float)(q.y));
      vlist << QVariant((float)(q.z));
      vlist << QVariant(thick);
      vlist << QVariant(val);
      vlist << QVariant(paint);

      modifyPruneTexture(TriangleShader,
			 m_pruneBuffer,
			 pruneBuffer1,
			 vlist);

      swapFBO(m_pruneBuffer, pruneBuffer1);
    }

  delete pruneBuffer1;
}

void
PruneHandler::swapBuffer()
{
  m_mopActive = true;

  if (!m_savedPruneBuffer)
    {
      copyToFromSavedChannel(true, -1, -1); // create savedPruneBuffer and copy into it
      return;
    }

  swapFBO(m_pruneBuffer, m_savedPruneBuffer);
}

void
PruneHandler::clip(Vec pos, Vec normal, Vec dmin)
{
  if (!standardChecks()) return;

  m_mopActive = true;
  
  int lod = m_dragInfo.z;

  Vec voxelScaling = Global::voxelScaling();
  Vec p = VECDIVIDE(pos, voxelScaling);
  p -= dmin;
  p /= lod;

  QVariantList vlist;
  vlist << QVariant((float)(p.x));
  vlist << QVariant((float)(p.y));
  vlist << QVariant((float)(p.z));
  vlist << QVariant((float)(normal.x));
  vlist << QVariant((float)(normal.y));
  vlist << QVariant((float)(normal.z));

  QGLFramebufferObject *pruneBuffer1 = newFBO();
  modifyPruneTexture(ClipShader,
		     m_pruneBuffer,
		     pruneBuffer1,
		     vlist);
  swapFBO(pruneBuffer1, m_pruneBuffer);

  delete pruneBuffer1;

  QMessageBox::information(0, "Mop Clip", "Done.\nThe clip plane can be removed.");
}

void
PruneHandler::crop(QString cropShaderString, Vec dmin)
{
  if (!standardChecks()) return;

  m_mopActive = true;

  if (m_cropShader) glDeleteObjectARB(m_cropShader);

  //---------------------------
  QString shaderString = PruneShaderFactory::crop(cropShaderString);

  if (m_cropShader)
    glDeleteObjectARB(m_cropShader);

  m_cropShader = glCreateProgramObjectARB();
  if (! ShaderFactory::loadShader(m_cropShader,
				  shaderString))
    exit(0);

  Vec voxelScaling = Global::voxelScaling();

  m_cropParm[0] = glGetUniformLocationARB(m_cropShader, "pruneTex");
  m_cropParm[1] = glGetUniformLocationARB(m_cropShader, "gridx");
  m_cropParm[2] = glGetUniformLocationARB(m_cropShader, "gridy");
  m_cropParm[3] = glGetUniformLocationARB(m_cropShader, "gridz");
  m_cropParm[4] = glGetUniformLocationARB(m_cropShader, "nrows");
  m_cropParm[5] = glGetUniformLocationARB(m_cropShader, "ncols");
  m_cropParm[6] = glGetUniformLocationARB(m_cropShader, "lod");
  m_cropParm[7] = glGetUniformLocationARB(m_cropShader, "voxelScaling");
  m_cropParm[8] = glGetUniformLocationARB(m_cropShader, "dmin");

  int lod = m_dragInfo.z;
  QVariantList vlist;
  vlist << QVariant(lod);
  vlist << QVariant((float)(voxelScaling.x));
  vlist << QVariant((float)(voxelScaling.y));
  vlist << QVariant((float)(voxelScaling.z));
  vlist << QVariant((float)(dmin.x));
  vlist << QVariant((float)(dmin.y));
  vlist << QVariant((float)(dmin.z));

  QGLFramebufferObject *pruneBuffer1 = newFBO();
  modifyPruneTexture(CropShader,
		     m_pruneBuffer,
		     pruneBuffer1,
		     vlist);
  swapFBO(pruneBuffer1, m_pruneBuffer);

  delete pruneBuffer1;

  QMessageBox::information(0, "Mop Crop", "Done.\nThe crop can be removed.");
}

QList<int>
PruneHandler::getMaxValue()
{
  QList<int> maxVals;

  if (!standardChecks()) return maxVals;

  QGLFramebufferObject *pruneBuffer1;
  QGLFramebufferObject *pruneBuffer2;

  pruneBuffer1 = newFBO();
  pruneBuffer2 = newFBO();

  modifyPruneTexture(CopyShader,
		     m_pruneBuffer,
		     pruneBuffer1);

  // max for each column
  BIND()

  glUseProgramObjectARB(m_maxValueShader);
  glUniform1iARB(m_maxValueParm[0], 1); // m_prunebuffer    
  glUniform1iARB(m_maxValueParm[1], m_dtexX);
  glUniform1iARB(m_maxValueParm[2], 1);

  StaticFunctions::pushOrthoView(0, 0, m_dtexX, m_dtexY);
  glBegin(GL_LINES);
  glTexCoord2f(0, 0); glVertex2f(0,0);
  glTexCoord2f(0, m_dtexY-1); glVertex2f(0, m_dtexY-1);
  glEnd();
  StaticFunctions::popOrthoView();

  glFinish(); 
  pruneBuffer2->release();  
  glActiveTexture(GL_TEXTURE1); 
  glDisable(GL_TEXTURE_RECTANGLE_ARB);  
  glUseProgramObjectARB(0); 

  swapFBO(pruneBuffer1, pruneBuffer2);



  // total max for row
  BIND()

  glUseProgramObjectARB(m_maxValueShader);
  glUniform1iARB(m_maxValueParm[0], 1); // m_prunebuffer    
  glUniform1iARB(m_maxValueParm[1], 1);
  glUniform1iARB(m_maxValueParm[2], m_dtexY);

  StaticFunctions::pushOrthoView(0, 0, m_dtexX, m_dtexY);
  glBegin(GL_POINTS);
  glTexCoord2f(0, 0); glVertex2f(0,0);
  glEnd();
  StaticFunctions::popOrthoView();

  glFinish(); 
  pruneBuffer2->release();  
  glActiveTexture(GL_TEXTURE1); 
  glDisable(GL_TEXTURE_RECTANGLE_ARB);  
  glUseProgramObjectARB(0); 


  // readback the values
  uchar *data = new uchar[4];
  pruneBuffer2->bind();
  glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
  glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, data);
  pruneBuffer2->release();

  maxVals << data[0];
  maxVals << data[1];
  maxVals << data[2];
  maxVals << data[3];

  delete pruneBuffer2;
  delete pruneBuffer1;    

  return maxVals;
}

void
PruneHandler::localThickness(int sz)
{
  if (!standardChecks()) return;

  //distanceTransform(sz, false);

  // apply cityblock and copy results to channel 2
  distanceTransform(sz, true);
  copyChannel(0, 2, false);

  // apply chessboard and copy results to channel 1
  distanceTransform(sz, false);
  copyChannel(0, 1, false);

  // now take average into channel 0
  average(1, 2, 0);


  QList<int> maxVals = getMaxValue();

  int maxThickness = maxVals[0];
  QProgressDialog progress("Calculating local thickness",
			   QString(),
			   0, 100,
			   0);
  progress.setCancelButton(0);

  QGLFramebufferObject *pruneBuffer1 = newFBO();
  QGLFramebufferObject *pruneBuffer2 = newFBO();

  modifyPruneTexture(CopyShader,
		     m_pruneBuffer,
		     pruneBuffer1);

  for(int t=2; t<=maxThickness; t++)
    {
      progress.setLabelText(QString("local thickness : %1").arg(t));
      progress.setValue(100*(float)(t-1)/(float)maxThickness);

      QVariantList vlist;
      vlist << QVariant(t);

      for(int ti=0; ti<t; ti++)
	{
	  modifyPruneTexture(LocalThicknessShader,
			     pruneBuffer1,
			     pruneBuffer2,
			     vlist);

	  swapFBO(pruneBuffer1, pruneBuffer2);
	}
    }      
  
  swapFBO(m_pruneBuffer, pruneBuffer1);
  
  delete pruneBuffer1;
  delete pruneBuffer2;

  // copy the results from channel 0 to channel 2
  // where the tag colours are applied
  copyChannel(0, 2, false);

  // reset channel 0 so that values are either 0 or 255 and nothing in between
  setValue(255, 0, 0, 255);


  QString mesg;
  for(int dv=0; dv<maxVals.count(); dv++)
    mesg += QString("%1 ").arg(maxVals[dv]);
  QMessageBox::information(0, "Channel max values", mesg);

  progress.setValue(100);
}

void
PruneHandler::smoothChannel(int chan)
{
  if (!standardChecks()) return;
  m_mopActive = true;

  QVariantList vlist;
  vlist << QVariant(chan);

  QGLFramebufferObject *pruneBuffer1 = newFBO();

  modifyPruneTexture(SmoothChannelShader,
		     m_pruneBuffer,
		     pruneBuffer1,
		     vlist);

  swapFBO(m_pruneBuffer, pruneBuffer1);

  delete pruneBuffer1;
}

void
PruneHandler::average(int chan1, int chan2, int dst)
{
  if (!standardChecks()) return;
  m_mopActive = true;

  QVariantList vlist;
  vlist << QVariant(chan1);
  vlist << QVariant(chan2);
  vlist << QVariant(dst);

  QGLFramebufferObject *pruneBuffer1 = newFBO();


  modifyPruneTexture(AverageShader,
		     m_pruneBuffer,
		     pruneBuffer1,
		     vlist);

  swapFBO(m_pruneBuffer, pruneBuffer1);

  delete pruneBuffer1;
}

QList<int>
PruneHandler::getHistogram(int chan)
{
  QList<int> histogram;

  if (!standardChecks()) return histogram;

  QList<int> maxVals = getMaxValue();
  int maxHistogram = maxVals[chan];

  QGLFramebufferObject *pruneBuffer1;
  QGLFramebufferObject *pruneBuffer2;

  pruneBuffer1 = newFBO();
  pruneBuffer2 = newFBO();

  for (int h=1; h<=maxHistogram; h++)
    {
      modifyPruneTexture(CopyShader,
			 m_pruneBuffer,
			 pruneBuffer1);

      // max for each column
      BIND()

      glUseProgramObjectARB(m_histogramShader);
      glUniform1iARB(m_histogramParm[0], 1); // m_prunebuffer    
      glUniform1iARB(m_histogramParm[1], m_dtexX);
      glUniform1iARB(m_histogramParm[2], 1);
      glUniform1iARB(m_histogramParm[3], h); // calculate frequency of h
      glUniform1iARB(m_histogramParm[4], 0); // step 0
      glUniform1iARB(m_histogramParm[5], chan); // calculate histogram for chan

      StaticFunctions::pushOrthoView(0, 0, m_dtexX, m_dtexY);
      glBegin(GL_LINES);
      glTexCoord2f(0, 0); glVertex2f(0,0);
      glTexCoord2f(0, m_dtexY-1); glVertex2f(0, m_dtexY-1);
      glEnd();
      StaticFunctions::popOrthoView();

      glFinish(); 
      pruneBuffer2->release();  
      glActiveTexture(GL_TEXTURE1); 
      glDisable(GL_TEXTURE_RECTANGLE_ARB);  
      glUseProgramObjectARB(0); 

      swapFBO(pruneBuffer1, pruneBuffer2);


      // total max for row
      BIND()

      glUseProgramObjectARB(m_histogramShader);
      glUniform1iARB(m_histogramParm[0], 1); // m_prunebuffer    
      glUniform1iARB(m_histogramParm[1], 1);
      glUniform1iARB(m_histogramParm[2], m_dtexY);
      glUniform1iARB(m_histogramParm[3], h); // calculate frequency of h
      glUniform1iARB(m_histogramParm[4], 1); // step 1
      glUniform1iARB(m_histogramParm[5], chan); // calculate histogram for chan

      StaticFunctions::pushOrthoView(0, 0, m_dtexX, m_dtexY);
      glBegin(GL_POINTS);
      glTexCoord2f(0, 0); glVertex2f(0,0);
      glEnd();
      StaticFunctions::popOrthoView();

      glFinish(); 
      pruneBuffer2->release();  
      glActiveTexture(GL_TEXTURE1); 
      glDisable(GL_TEXTURE_RECTANGLE_ARB);  
      glUseProgramObjectARB(0); 
      

      // readback the values
      uchar data[4];
      pruneBuffer2->bind();
      glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
      glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &(data[0]));
      pruneBuffer2->release();

      int x = data[0];
      int y = data[1];
      int z = data[2];

      int freq = x*255*255 + y*255 + z;
      histogram << freq;
    }

  delete pruneBuffer2;
  delete pruneBuffer1;    

  return histogram;
}

void
PruneHandler::pattern(bool flag,
		      int xn, int xd,
		      int yn, int yd,
		      int zn, int zd)
{
  if (!standardChecks()) return;
  m_mopActive = true;

  QVariantList vlist;
  vlist << QVariant(flag);
  vlist << QVariant(xn);
  vlist << QVariant(xd);
  vlist << QVariant(yn);
  vlist << QVariant(yd);
  vlist << QVariant(zn);
  vlist << QVariant(zd);

  QGLFramebufferObject *pruneBuffer1 = newFBO();


  modifyPruneTexture(PatternShader,
		     m_pruneBuffer,
		     pruneBuffer1,
		     vlist);

  swapFBO(m_pruneBuffer, pruneBuffer1);

  delete pruneBuffer1;
}
