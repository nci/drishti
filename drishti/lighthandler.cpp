#include "lighthandler.h"
#include "global.h"
#include "staticfunctions.h"
#include "lightshaderfactory.h"
#include "shaderfactory.h"
#include "shaderfactory2.h"
#include "mainwindowui.h"
#include "propertyeditor.h"
#include "pruneshaderfactory.h"
#include "cropshaderfactory.h"
#include "blendshaderfactory.h"
#include "geometryobjects.h"
#include "prunehandler.h"
#include "../framebufferbudget.h"

#include <QDebug>

#include <limits>
#include <new>

#define VECDIVIDE(a, b) Vec(a.x/b.x, a.y/b.y, a.z/b.z)

QGLFramebufferObject *LightHandler::m_opacityBuffer=0;
QGLFramebufferObject *LightHandler::m_finalLightBuffer=0;
QGLFramebufferObject *LightHandler::m_pruneBuffer=0;

GLuint LightHandler::m_lightBuffer=0;
GLuint LightHandler::m_lightTex[2]={0,0};
GLuint LightHandler::m_emisBuffer=0;
GLuint LightHandler::m_emisTex[2]={0,0};
 
bool LightHandler::m_doAll = false;
bool LightHandler::m_onlyLightBuffers = false;
bool LightHandler::m_lutChanged = false;
bool LightHandler::m_available = true;
QString LightHandler::m_failureReason;
uchar* LightHandler::m_lut=0;

GLuint LightHandler::m_lutTex=0;
GLuint LightHandler::m_dataTex=0;

GLhandleARB LightHandler::m_opacityShader=0;
GLhandleARB LightHandler::m_mergeOpPruneShader=0;
GLhandleARB LightHandler::m_aoLightShader=0;
GLhandleARB LightHandler::m_initdLightShader=0;
GLhandleARB LightHandler::m_initpLightShader=0;
GLhandleARB LightHandler::m_initEmisShader=0;
GLhandleARB LightHandler::m_dLightShader=0;
GLhandleARB LightHandler::m_pLightShader=0;
GLhandleARB LightHandler::m_fLightShader=0;
GLhandleARB LightHandler::m_efLightShader=0;
GLhandleARB LightHandler::m_diffuseLightShader=0;
GLhandleARB LightHandler::m_invertLightShader=0;
GLhandleARB LightHandler::m_emisShader=0;
GLhandleARB LightHandler::m_expandLightShader=0;

GLhandleARB LightHandler::m_clipShader=0;
GLint LightHandler::m_clipParm[20];
GLhandleARB LightHandler::m_cropShader=0;
GLint LightHandler::m_cropParm[20];
GLhandleARB LightHandler::m_blendShader=0;
GLint LightHandler::m_blendParm[20];

GLint LightHandler::m_opacityParm[20];
GLint LightHandler::m_mergeOpPruneParm[20];
GLint LightHandler::m_initdLightParm[20];
GLint LightHandler::m_initpLightParm[20];
GLint LightHandler::m_initEmisParm[20];
GLint LightHandler::m_aoLightParm[20];
GLint LightHandler::m_dLightParm[20];
GLint LightHandler::m_pLightParm[20];
GLint LightHandler::m_fLightParm[20];
GLint LightHandler::m_efLightParm[20];
GLint LightHandler::m_diffuseLightParm[20];
GLint LightHandler::m_invertLightParm[20];
GLint LightHandler::m_emisParm[20];
GLint LightHandler::m_expandLightParm[20];

QList<Vec> LightHandler::m_clipPos;
QList<Vec> LightHandler::m_clipNorm;
QList<CropObject> LightHandler::m_crops;

int LightHandler::m_dtexX = 0;
int LightHandler::m_dtexY = 0;
Vec LightHandler::m_dragInfo;
Vec LightHandler::m_subVolSize;
Vec LightHandler::m_dataMin;
Vec LightHandler::m_dataMax;
Vec LightHandler::m_dragVolSize;

int LightHandler::m_ncols=0;
int LightHandler::m_nrows=0;
int LightHandler::m_gridx=0;
int LightHandler::m_gridy=0;
int LightHandler::m_gridz=0;

int LightHandler::m_lightLod = 2;
int LightHandler::m_lightDiffuse = 1;
Vec LightHandler::m_aoLightColor = Vec(1,1,1);
int LightHandler::m_aoRad = 0;
float LightHandler::m_aoFrac = 0.7;
float LightHandler::m_aoDensity1 = 0.3;
float LightHandler::m_aoDensity2 = 0.75;
int LightHandler::m_aoTimes = 1;
float LightHandler::m_aoOpMod = 1.0;
bool LightHandler::m_onlyAOLight = false;
bool LightHandler::m_basicLight = false;
bool LightHandler::m_applyClip = true;
bool LightHandler::m_applyCrop = true;

int LightHandler::m_opacityTF=0;

int LightHandler::m_emisTF=-1;
float LightHandler::m_emisDecay=1.0;
float LightHandler::m_emisBoost=5.0;
int LightHandler::m_emisTimes=20;

int LightHandler::m_dilatedEmisTex;
int LightHandler::m_origEmisTex;

bool LightHandler::inPool = true;
bool LightHandler::showLights = true;

bool LightHandler::m_amrData = false;
GLuint LightHandler::m_amrTex = 0;


void LightHandler::setClips(QList<Vec> cpos, QList<Vec> cnorm)
{
  m_clipPos = cpos;
  m_clipNorm = cnorm;
}

bool
LightHandler::checkCrops()
{
  if (!m_applyCrop)
    return false;

  bool doit = false;
  if (GeometryObjects::crops()->count() != m_crops.count())
    doit = true;
  
  if (!doit)
    {
      QList<CropObject> co;
      co = GeometryObjects::crops()->crops();
      for(int i=0; i<m_crops.count(); i++)
	{
	  if (m_crops[i] != co[i])
	    {
	      doit = true;
	      break;
	    }
	}
    }
      
  if (doit)
    {
      m_crops.clear();
      m_crops = GeometryObjects::crops()->crops();    

      createCropShader();
      if (!m_available)
	return true;
      createBlendShader();
    }

  return doit;
}

bool
LightHandler::checkClips(QList<Vec> cpos, QList<Vec> cnorm)
{
  if (!m_applyClip)
    return false;

  bool doit = false;
  if (cpos.count() != m_clipPos.count())
    doit = true;
  else
    {
      for(int i=0; i<cpos.count(); i++)
	{
	  if ((cpos[i]-m_clipPos[i]).squaredNorm() > 0.0001)
	    {
	      doit = true;
	      break;
	    }
	  if ((cnorm[i]-m_clipNorm[i]).squaredNorm() > 0.0001)
	    {
	      doit = true;
	      break;
	    }
	}
    }

  m_clipPos = cpos;
  m_clipNorm = cnorm;

  return doit;
}

GLuint LightHandler::texture()
{
  if (m_available && m_finalLightBuffer && m_finalLightBuffer->isValid())
    return m_finalLightBuffer->texture();
  else
    return 0;
}

void LightHandler::show()
{
  showLights = true;
  m_giLights->setShow(-1, showLights);
}
void LightHandler::hide()
{
  showLights = false;
  m_giLights->setShow(-1, showLights);
}

void
LightHandler::lightBufferInfo(int &lgx, int &lgy, int &lgz,
			      int &rows, int &cols, int &lod)
{
  lgx = m_gridx;
  lgy = m_gridy;
  lgz = m_gridz;

  rows = m_nrows;
  cols = m_ncols;
  
  lod = (!basicLight() ? m_lightLod : -1);
}

GiLightInfo
LightHandler::giLightInfo()
{
  GiLightInfo gi;

  gi.gloInfo = m_giLights->giLightObjectInfo();
  gi.basicLight = m_basicLight;
  gi.onlyAOLight = m_onlyAOLight;
  gi.applyClip = m_applyClip;
  gi.applyCrop = m_applyCrop;
  gi.lightLod = m_lightLod;
  gi.lightDiffuse = m_lightDiffuse;
  gi.aoLightColor = m_aoLightColor;
  gi.aoRad = m_aoRad;
  gi.aoTimes = m_aoTimes;
  gi.aoFrac = m_aoFrac;
  gi.aoOpMod = m_aoOpMod;
  gi.aoDensity1 = m_aoDensity1;
  gi.aoDensity2 = m_aoDensity2;
  gi.opacityTF = m_opacityTF;
  gi.emisTF = m_emisTF;
  gi.emisDecay = m_emisDecay;
  gi.emisBoost = m_emisBoost;
  gi.emisTimes = m_emisTimes;

  return gi;
}

void
LightHandler::setGiLightInfo(GiLightInfo gi)
{
  m_doAll = m_onlyLightBuffers = false;

  bool lightsChanged = false;
  bool basicLightChanged = false;
  bool lodChanged = false;

  if (giLights()->setGiLightObjectInfo(gi.gloInfo)) lightsChanged = true;
  if (gi.basicLight != m_basicLight) basicLightChanged = true;
  if (gi.applyClip != m_applyClip) lightsChanged = true;
  if (gi.applyCrop != m_applyCrop) lightsChanged = true;
  if (gi.onlyAOLight != m_onlyAOLight) lightsChanged = true;
  if (gi.lightLod != m_lightLod) lodChanged = true;
  if (gi.lightDiffuse != m_lightDiffuse) lightsChanged = true;
  if ((gi.aoLightColor-m_aoLightColor).squaredNorm() > 0.0001) lightsChanged = true;
  if (gi.aoRad != m_aoRad) lightsChanged = true;
  if (gi.aoTimes != m_aoTimes) lightsChanged = true;
  if (fabs(gi.aoFrac-m_aoFrac) > 0.001) lightsChanged = true;
  if (fabs(gi.aoOpMod-m_aoOpMod) > 0.001) lightsChanged = true;
  if (fabs(gi.aoDensity1-m_aoDensity1) > 0.001) lightsChanged = true;
  if (fabs(gi.aoDensity2-m_aoDensity2) > 0.001) lightsChanged = true;
  if (gi.opacityTF != m_opacityTF) lodChanged = true;
  if (gi.emisTF != m_emisTF) lodChanged = true;
  if (fabs(gi.emisDecay-m_emisDecay) > 0.001) lightsChanged = true;
  if (gi.emisBoost != m_emisBoost) lightsChanged = true;
  if (gi.emisTimes != m_emisTimes) lightsChanged = true;

  m_basicLight = gi.basicLight;
  m_applyClip = gi.applyClip;
  m_applyCrop = gi.applyCrop;
  m_onlyAOLight = gi.onlyAOLight;
  m_lightLod = gi.lightLod;
  m_lightDiffuse = gi.lightDiffuse;
  m_aoLightColor = gi.aoLightColor;
  m_aoRad = gi.aoRad;
  m_aoTimes = gi.aoTimes;
  m_aoFrac = gi.aoFrac;
  m_aoOpMod = gi.aoOpMod;
  m_aoDensity1 = gi.aoDensity1;
  m_aoDensity2 = gi.aoDensity2;
  m_opacityTF = gi.opacityTF;
  m_emisTF = gi.emisTF;
  m_emisDecay = gi.emisDecay;
  m_emisBoost = gi.emisBoost;
  m_emisTimes = gi.emisTimes;

  if (m_basicLight)
    return;

  if (!basicLightChanged && !lightsChanged)
    {
      QList<GiLightGrabber*> lightsPtr = m_giLights->giLightsPtr();
      for(int i=0; i<lightsPtr.count(); i++)
	lightsPtr[i]->setLightChanged(false);      

      return;
    }

  if (lodChanged)
    {
      m_doAll = true;
      m_onlyLightBuffers = true;
    }
  else
    m_onlyLightBuffers = true;
}

void
LightHandler::setLut(uchar *vlut)
{
  bool gilite = false;

  if (m_lut)
    {
      // check for any change is transfer functions
      for (int i=0; i<Global::lutSize()*256*256; i++)
	{
	  if (fabs((float)(vlut[4*i+0] - m_lut[4*i+0])) > 2 ||
	      fabs((float)(vlut[4*i+1] - m_lut[4*i+1])) > 2 ||
	      fabs((float)(vlut[4*i+2] - m_lut[4*i+2])) > 2 ||
	      fabs((float)(vlut[4*i+3] - m_lut[4*i+3])) > 1)
	    {
	      gilite = true;	      
	      break;
	    }
	}
    }
  else
    gilite = true;

  if (gilite || m_doAll)
    updateAndLoadLightTexture(m_dataTex,
			      m_dtexX, m_dtexY,
			      m_dragInfo, m_dragVolSize,
			      m_dataMin, m_dataMax,
			      m_subVolSize,
			      vlut);
  else if (m_onlyLightBuffers)
    updateLightBuffers();
}

bool LightHandler::m_initialized = false;
void LightHandler::init()
{
  if(m_initialized)
    return;

  m_giLights = new GiLights();
  m_initialized = true;
}
bool LightHandler::grabsMouse()
{
  return (giLights()->grabsMouse());
}
void
LightHandler::mouseReleaseEvent(QMouseEvent *e, Camera *c)
{
  giLights()->mouseReleaseEvent(e, c);
}
bool
LightHandler::keyPressEvent(QKeyEvent *event)
{
  if (giLights()->grabsMouse())
    {
      int nlights = giLights()->count();
      bool ev = giLights()->keyPressEvent(event);

      // perform lightbuffer update only if point/direction lights are switched on
      if (!m_onlyAOLight)
	{
	  bool lightDeleted = (nlights != giLights()->count());
	  if (lightsChanged() || lightDeleted)
	    updateLightBuffers();

	  return ev;
	}
    }
  return false;
}
GiLights* LightHandler::m_giLights = NULL;
GiLights* LightHandler::giLights() 
{ 
  if(!LightHandler::m_initialized) LightHandler::init();
  return m_giLights; 
}
void
LightHandler::removeFromMouseGrabberPool()
{
  if(!LightHandler::m_initialized) LightHandler::init();
  m_giLights->removeFromMouseGrabberPool();
}
void
LightHandler::addInMouseGrabberPool()
{
  if(!LightHandler::m_initialized) LightHandler::init();
  m_giLights->addInMouseGrabberPool();
}
bool
LightHandler::lightsChanged()
{
  if (m_onlyAOLight)
    return false;

  QList<GiLightGrabber*> lightsPtr = m_giLights->giLightsPtr();
  if (lightsPtr.count() > 0)
    {
      for(int i=0; i<lightsPtr.count(); i++)
	{
	  if (lightsPtr[i]->lightChanged())
	    return true;
	}
    }
  return false;
}
void
LightHandler::reset()
{
  giLights()->clear();
  m_lightLod = 2;
  m_aoLightColor = Vec(1,1,1);
  m_aoOpMod = 1.0;
  m_aoDensity2 = 0.75;

  //-- not used, will be removed
  m_aoTimes = 1;
  m_aoDensity1 = 0.3;
  m_aoRad = 0;
  m_aoFrac = 0.7;
}

void LightHandler::clean()
{
  giLights()->clear();

  releaseBuffers();

  //if (m_lutTex) glDeleteTextures(1, &m_lutTex);
  //m_lutTex = 0;

  if (m_opacityShader) glDeleteObjectARB(m_opacityShader);
  if (m_mergeOpPruneShader) glDeleteObjectARB(m_mergeOpPruneShader);
  if (m_aoLightShader) glDeleteObjectARB(m_aoLightShader);
  if (m_initdLightShader) glDeleteObjectARB(m_initdLightShader);
  if (m_initpLightShader) glDeleteObjectARB(m_initpLightShader);
  if (m_initEmisShader) glDeleteObjectARB(m_initEmisShader);
  if (m_dLightShader) glDeleteObjectARB(m_dLightShader);
  if (m_pLightShader) glDeleteObjectARB(m_pLightShader);
  if (m_fLightShader) glDeleteObjectARB(m_fLightShader);
  if (m_efLightShader) glDeleteObjectARB(m_efLightShader);
  if (m_diffuseLightShader) glDeleteObjectARB(m_diffuseLightShader);
  if (m_invertLightShader) glDeleteObjectARB(m_invertLightShader);
  if (m_emisShader) glDeleteObjectARB(m_emisShader);
  if (m_expandLightShader) glDeleteObjectARB(m_expandLightShader);
  if (m_clipShader) glDeleteObjectARB(m_clipShader);
  if (m_cropShader) glDeleteObjectARB(m_cropShader);
  if (m_blendShader) glDeleteObjectARB(m_blendShader);
  m_opacityShader = 0;
  m_mergeOpPruneShader = 0;
  m_aoLightShader = 0;
  m_initdLightShader = 0;
  m_initpLightShader = 0;
  m_initEmisShader = 0;
  m_dLightShader = 0;
  m_pLightShader = 0;
  m_fLightShader = 0;
  m_efLightShader = 0;
  m_diffuseLightShader = 0;
  m_invertLightShader = 0;
  m_emisShader = 0;
  m_expandLightShader = 0;
  m_clipShader = 0;
  m_cropShader = 0;
  m_blendShader = 0;
  m_available = true;
  m_failureReason.clear();
}

#define swapFBO(fbo1,  fbo2)			\
  {						\
    QGLFramebufferObject *tpb = fbo1;		\
    fbo1 = fbo2;				\
    fbo2 = tpb;					\
  }

void
LightHandler::releaseBuffers()
{
  delete m_opacityBuffer;
  delete m_finalLightBuffer;
  delete m_pruneBuffer;
  m_opacityBuffer = 0;
  m_finalLightBuffer = 0;
  m_pruneBuffer = 0;

  if (m_lightBuffer) glDeleteFramebuffers(1, &m_lightBuffer);
  if (m_emisBuffer) glDeleteFramebuffers(1, &m_emisBuffer);
  m_lightBuffer = 0;
  m_emisBuffer = 0;

  if (m_lightTex[0]) glDeleteTextures(2, m_lightTex);
  if (m_emisTex[0]) glDeleteTextures(2, m_emisTex);
  m_lightTex[0] = m_lightTex[1] = 0;
  m_emisTex[0] = m_emisTex[1] = 0;
}

void
LightHandler::fail(const QString &label, const QString &detail)
{
  const QString message = QStringLiteral("%1 unavailable: %2").arg(label, detail);
  m_available = false;
  m_failureReason = message;
  m_doAll = false;
  m_onlyLightBuffers = false;
  releaseBuffers();
  qWarning().noquote() << message;
  MainWindowUI::mainWindowUI()->statusBar->showMessage(message);
}

bool
LightHandler::loadProgram(GLhandleARB &program,
			  const QString &source,
			  const QString &label)
{
  GLhandleARB candidate = glCreateProgramObjectARB();
  if (!candidate || !ShaderFactory::loadShader(candidate, source))
    {
      const GLenum error = glGetError();
      if (candidate) glDeleteObjectARB(candidate);
      if (program) glDeleteObjectARB(program);
      program = 0;
      fail(label,
	   QStringLiteral("shader compile/link failed (OpenGL error 0x%1)").arg(
	     QString::number(static_cast<qulonglong>(error), 16)));
      return false;
    }

  if (program) glDeleteObjectARB(program);
  program = candidate;
  return true;
}

QGLFramebufferObject*
LightHandler::newFBO(int sx, int sy)
{
  if (sx <= 0 || sy <= 0 ||
      sx > Global::max2dTextureSize() || sy > Global::max2dTextureSize())
    {
      fail(QStringLiteral("lighting/framebuffer"),
	   QStringLiteral("invalid size %1x%2 (limit %3)")
	   .arg(sx).arg(sy).arg(Global::max2dTextureSize()));
      return 0;
    }

  for (int i=0; i<32 && glGetError() != GL_NO_ERROR; ++i) {}
  QGLFramebufferObject* fbo = new (std::nothrow) QGLFramebufferObject(
    QSize(sx, sy), QGLFramebufferObject::NoAttachment,
    GL_TEXTURE_RECTANGLE_EXT);
  GLenum error = glGetError();
  GLenum status = 0;
  bool bound = false;
  if (fbo && fbo->isValid() && fbo->texture())
    {
      bound = fbo->bind();
      if (bound)
	{
	  status = glCheckFramebufferStatus(GL_FRAMEBUFFER_EXT);
	  fbo->release();
	  const GLenum validationError = glGetError();
	  if (error == GL_NO_ERROR) error = validationError;
	}
    }

  if (!fbo || !fbo->isValid() || !fbo->texture() || !bound ||
      status != GL_FRAMEBUFFER_COMPLETE_EXT || error != GL_NO_ERROR)
    {
      delete fbo;
      fail(QStringLiteral("lighting/framebuffer"),
	   QStringLiteral("creation failed (status 0x%1, OpenGL error 0x%2)")
	   .arg(QString::number(static_cast<qulonglong>(status), 16),
		QString::number(static_cast<qulonglong>(error), 16)));
      return 0;
    }

  return fbo;
}

bool
LightHandler::standardChecks()
{
  if (!m_available)
    return false;

  if (Global::volumeType() == Global::DummyVolume)
    {
      //QMessageBox::information(0, "Error Lighting", "Does not work on dummy or colour volumes");
      return false;
    }

  return true;
}

void
LightHandler::createOpacityShader(bool bit16)
{
  if (Global::volumeType() == Global::DummyVolume)
    return;

  m_available = true;
  m_failureReason.clear();

  QString shaderString;

  //---------------------------
  if (Global::volumeType() == Global::RGBVolume ||
      Global::volumeType() == Global::RGBAVolume)
    shaderString = LightShaderFactory::genOpacityShaderRGB();
  else
    {
      int nvol = 1;
      if (Global::volumeType() == Global::DoubleVolume) nvol = 2;
      if (Global::volumeType() == Global::TripleVolume) nvol = 3;
      if (Global::volumeType() == Global::QuadVolume) nvol = 4;

      shaderString = LightShaderFactory::genOpacityShader(nvol, bit16, m_amrData);
    }    

  if (!loadProgram(m_opacityShader, shaderString,
		   QStringLiteral("lighting/opacity")))
    return;

  m_opacityParm[0] = glGetUniformLocationARB(m_opacityShader, "lutTex");
  m_opacityParm[1] = glGetUniformLocationARB(m_opacityShader, "dragTex");
  m_opacityParm[2] = glGetUniformLocationARB(m_opacityShader, "gridx");
  m_opacityParm[3] = glGetUniformLocationARB(m_opacityShader, "gridy");
  m_opacityParm[4] = glGetUniformLocationARB(m_opacityShader, "gridz");
  m_opacityParm[6] = glGetUniformLocationARB(m_opacityShader, "ncols");
  m_opacityParm[7] = glGetUniformLocationARB(m_opacityShader, "llod");
  m_opacityParm[8] = glGetUniformLocationARB(m_opacityShader, "lgridx");
  m_opacityParm[9] = glGetUniformLocationARB(m_opacityShader, "lgridy");
  m_opacityParm[10] = glGetUniformLocationARB(m_opacityShader, "lgridz");
  m_opacityParm[12] = glGetUniformLocationARB(m_opacityShader, "lncols");
  m_opacityParm[13] = glGetUniformLocationARB(m_opacityShader, "opshader");
  m_opacityParm[14] = glGetUniformLocationARB(m_opacityShader, "tfSet");
  m_opacityParm[15] = glGetUniformLocationARB(m_opacityShader, "dragsize");

  if (m_amrData)
    {
      m_opacityParm[16] = glGetUniformLocationARB(m_opacityShader, "amrTex");
      m_opacityParm[17] = glGetUniformLocationARB(m_opacityShader, "dragLod");
    }
  m_opacityParm[18] = glGetUniformLocationARB(m_opacityShader, "pruneTex");
  //---------------------------
}

void
LightHandler::createLightShaders()
{
  createFinalLightShader();
  if (!m_available) return;
  createAmbientOcclusionLightShader();
  if (!m_available) return;
  createDirectionalLightShader();
  if (!m_available) return;
  createDiffuseLightShader();
  if (!m_available) return;
  createInvertLightShader();
  if (!m_available) return;
  createPointLightShader();
  if (!m_available) return;
  createEmissiveShader();
  if (!m_available) return;
  createExpandShader();
  if (!m_available) return;
  createMergeOpPruneShader();
  if (!m_available) return;
  createClipShader();
}

void
LightHandler::createCropShader()
{
  if (m_crops.count() == 0)
    {
      if (m_cropShader) glDeleteObjectARB(m_cropShader);
      m_cropShader = 0;
      return;
    }

  int ncrops = 0;
  for (int ci=0; ci<m_crops.count(); ci++)
    {
      if (m_crops[ci].cropType() < CropObject::Tear_Tear)
	ncrops ++;
    }
  if (ncrops == 0)
    {
      if (m_cropShader) glDeleteObjectARB(m_cropShader);
      m_cropShader = 0;
      return;
    }

  QString cropShaderString = CropShaderFactory::generateCropping(m_crops);
  QString shaderString = PruneShaderFactory::crop(cropShaderString);

  if (!loadProgram(m_cropShader, shaderString,
		   QStringLiteral("lighting/crop")))
    return;

  m_cropParm[0] = glGetUniformLocationARB(m_cropShader, "pruneTex");
  m_cropParm[1] = glGetUniformLocationARB(m_cropShader, "gridx");
  m_cropParm[2] = glGetUniformLocationARB(m_cropShader, "gridy");
  m_cropParm[3] = glGetUniformLocationARB(m_cropShader, "gridz");
  m_cropParm[4] = glGetUniformLocationARB(m_cropShader, "nrows");
  m_cropParm[5] = glGetUniformLocationARB(m_cropShader, "ncols");
  m_cropParm[6] = glGetUniformLocationARB(m_cropShader, "lod");
  m_cropParm[7] = glGetUniformLocationARB(m_cropShader, "voxelScaling");
  m_cropParm[8] = glGetUniformLocationARB(m_cropShader, "dmin");
}

void
LightHandler::createBlendShader()
{
  if (m_crops.count() == 0)
    {
      if (m_blendShader) glDeleteObjectARB(m_blendShader);
      m_blendShader = 0;
      return;
    }

  int nblends = 0;
  for (int ci=0; ci<m_crops.count(); ci++)
    {
      if (m_crops[ci].cropType() >= CropObject::View_Tear &&
	  m_crops[ci].cropType() < CropObject::Glow_Ball)
	nblends ++;
    }
  if (nblends == 0)
    {
      if (m_blendShader) glDeleteObjectARB(m_blendShader);
      m_blendShader = 0;
      return;
    }

  QString blendShaderString = BlendShaderFactory::generateBlend(m_crops);
  QString shaderString = LightShaderFactory::blend(blendShaderString);

  if (!loadProgram(m_blendShader, shaderString,
		   QStringLiteral("lighting/blend")))
    return;

  m_blendParm[0] = glGetUniformLocationARB(m_blendShader, "opTex");
  m_blendParm[1] = glGetUniformLocationARB(m_blendShader, "gridx");
  m_blendParm[2] = glGetUniformLocationARB(m_blendShader, "gridy");
  m_blendParm[3] = glGetUniformLocationARB(m_blendShader, "gridz");
  m_blendParm[4] = glGetUniformLocationARB(m_blendShader, "nrows");
  m_blendParm[5] = glGetUniformLocationARB(m_blendShader, "ncols");
  m_blendParm[6] = glGetUniformLocationARB(m_blendShader, "lod");
  m_blendParm[7] = glGetUniformLocationARB(m_blendShader, "voxelScaling");
  m_blendParm[8] = glGetUniformLocationARB(m_blendShader, "dmin");
  m_blendParm[9] = glGetUniformLocationARB(m_blendShader, "lightTex");
  m_blendParm[10] = glGetUniformLocationARB(m_blendShader, "lutTex");
  m_blendParm[11] = glGetUniformLocationARB(m_blendShader, "eyepos");
  m_blendParm[12] = glGetUniformLocationARB(m_blendShader, "dirUp");
  m_blendParm[13] = glGetUniformLocationARB(m_blendShader, "dirRight");
}

void
LightHandler::createClipShader()
{
  QString shaderString = PruneShaderFactory::clip();

  if (!loadProgram(m_clipShader, shaderString,
		   QStringLiteral("lighting/clip")))
    return;

  m_clipParm[0] = glGetUniformLocationARB(m_clipShader, "pruneTex");
  m_clipParm[1] = glGetUniformLocationARB(m_clipShader, "gridx");
  m_clipParm[2] = glGetUniformLocationARB(m_clipShader, "gridy");
  m_clipParm[3] = glGetUniformLocationARB(m_clipShader, "gridz");
  m_clipParm[4] = glGetUniformLocationARB(m_clipShader, "nrows");
  m_clipParm[5] = glGetUniformLocationARB(m_clipShader, "ncols");
  m_clipParm[6] = glGetUniformLocationARB(m_clipShader, "pos");
  m_clipParm[7] = glGetUniformLocationARB(m_clipShader, "normal");
  m_clipParm[8] = glGetUniformLocationARB(m_clipShader, "lod");
}

int
LightHandler::applyBlending(int ct)
{
  if (!m_blendShader)
    return ct;

//  MainWindowUI::mainWindowUI()->menubar->parentWidget()->\
//    setWindowTitle("----------Updating crop buffers----------");

  Vec voxelScaling = Global::voxelScaling();
  int sX = m_pruneBuffer->width();
  int sY = m_pruneBuffer->height();

  int lod = m_dragInfo.z;
  lod *= m_lightLod;

  glUseProgramObjectARB(m_blendShader);
  glUniform1iARB(m_blendParm[0], 2); // opacitybuffer
  glUniform1iARB(m_blendParm[1], m_gridx); // gridx
  glUniform1iARB(m_blendParm[2], m_gridy); // gridy
  glUniform1iARB(m_blendParm[3], m_gridz); // gridz
  glUniform1iARB(m_blendParm[4], m_nrows); // nrows
  glUniform1iARB(m_blendParm[5], m_ncols); // ncols
  glUniform1iARB(m_blendParm[6], lod); // lod
  glUniform3fARB(m_blendParm[7], voxelScaling.x, voxelScaling.y, voxelScaling.z);
  glUniform3fARB(m_blendParm[8], m_dataMin.x, m_dataMin.y, m_dataMin.z);
  glUniform1iARB(m_blendParm[9], 1); // light buffer
  glUniform1iARB(m_blendParm[10], 0); // lookup table
  glUniform3fARB(m_blendParm[11], 0, 0, -1000); // eyepos
  glUniform3fARB(m_blendParm[12], 0, 1, 0); // up
  glUniform3fARB(m_blendParm[13], 1, 0, 0); // right

  // enable lookup texture
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, m_lutTex);
  glEnable(GL_TEXTURE_2D);

  // enable prune texture
  glActiveTexture(GL_TEXTURE1);
  glEnable(GL_TEXTURE_RECTANGLE_ARB);
  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_lightTex[ct]);
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 

  // enable opacity texture
  glActiveTexture(GL_TEXTURE2);
  glEnable(GL_TEXTURE_RECTANGLE_ARB);
  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_opacityBuffer->texture());
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  // multiply opacity by pruned data into pruneBuffer
  m_pruneBuffer->bind();
  glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);

  // first clear prune buffer
  glClearColor(0, 0, 0, 0);
  glClear(GL_COLOR_BUFFER_BIT);
  
  StaticFunctions::pushOrthoView(0, 0, sX, sY);
  StaticFunctions::drawQuad(0, 0, sX, sY, 1.0);
  StaticFunctions::popOrthoView();
  
  glFinish();

  glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
  glUseProgramObjectARB(0);

  glActiveTexture(GL_TEXTURE0);
  glDisable(GL_TEXTURE_2D);

  glActiveTexture(GL_TEXTURE1);
  glDisable(GL_TEXTURE_RECTANGLE_ARB);

  glActiveTexture(GL_TEXTURE2);
  glDisable(GL_TEXTURE_RECTANGLE_ARB);

  return ct;
}

void
LightHandler::createMergeOpPruneShader()
{
  QString shaderString;

  if (m_mergeOpPruneShader)
    return;

  shaderString = LightShaderFactory::genMergeOpPruneShader();
  if (!loadProgram(m_mergeOpPruneShader, shaderString,
		   QStringLiteral("lighting/merge-opacity-prune")))
    return;
  m_mergeOpPruneParm[0] = glGetUniformLocationARB(m_mergeOpPruneShader, "lightTex");
  m_mergeOpPruneParm[1] = glGetUniformLocationARB(m_mergeOpPruneShader, "opTex");
}

void
LightHandler::createFinalLightShader()
{
  QString shaderString;

  if (m_fLightShader)
    return;

  shaderString = LightShaderFactory::genFinalLightShader();
  if (!loadProgram(m_fLightShader, shaderString,
		   QStringLiteral("lighting/final")))
    return;
  m_fLightParm[0] = glGetUniformLocationARB(m_fLightShader, "lightTex");
  m_fLightParm[1] = glGetUniformLocationARB(m_fLightShader, "lcol");
  //---------------------------

  if (m_efLightShader)
    return;

  shaderString = LightShaderFactory::genEFinalLightShader();

  if (!loadProgram(m_efLightShader, shaderString,
		   QStringLiteral("lighting/final-emissive")))
    {
      glDeleteObjectARB(m_fLightShader);
      m_fLightShader = 0;
      return;
    }

  m_efLightParm[0] = glGetUniformLocationARB(m_efLightShader, "lightTex");
  m_efLightParm[1] = glGetUniformLocationARB(m_efLightShader, "eTex");
  //---------------------------
}

void
LightHandler::createDiffuseLightShader()
{
  if (m_diffuseLightShader)
    return;

  QString shaderString;

  //---------------------------
  shaderString = LightShaderFactory::genDiffuseLightShader();
  if (!loadProgram(m_diffuseLightShader, shaderString,
		   QStringLiteral("lighting/diffuse")))
    return;
  m_diffuseLightParm[0] = glGetUniformLocationARB(m_diffuseLightShader, "lightTex");
  m_diffuseLightParm[1] = glGetUniformLocationARB(m_diffuseLightShader, "gridx");
  m_diffuseLightParm[2] = glGetUniformLocationARB(m_diffuseLightShader, "gridy");
  m_diffuseLightParm[3] = glGetUniformLocationARB(m_diffuseLightShader, "gridz");
  m_diffuseLightParm[5] = glGetUniformLocationARB(m_diffuseLightShader, "ncols");
  m_diffuseLightParm[6] = glGetUniformLocationARB(m_diffuseLightShader, "boost");
  //---------------------------
}

void
LightHandler::createInvertLightShader()
{
  if (m_invertLightShader)
    return;

  QString shaderString;

  //---------------------------
  shaderString = LightShaderFactory::genInvertLightShader();
  if (!loadProgram(m_invertLightShader, shaderString,
		   QStringLiteral("lighting/invert")))
    return;
  m_invertLightParm[0] = glGetUniformLocationARB(m_invertLightShader, "lightTex");
  //---------------------------
}

void
LightHandler::createAmbientOcclusionLightShader()
{
  if (m_aoLightShader)
    return;

  QString shaderString;

  //---------------------------
  shaderString = LightShaderFactory::genAOLightShader();
  if (!loadProgram(m_aoLightShader, shaderString,
		   QStringLiteral("lighting/ambient-occlusion")))
    return;
  m_aoLightParm[0] = glGetUniformLocationARB(m_aoLightShader, "opTex");
  m_aoLightParm[1] = glGetUniformLocationARB(m_aoLightShader, "gridx");
  m_aoLightParm[2] = glGetUniformLocationARB(m_aoLightShader, "gridy");
  m_aoLightParm[3] = glGetUniformLocationARB(m_aoLightShader, "gridz");
  m_aoLightParm[5] = glGetUniformLocationARB(m_aoLightShader, "ncols");
  m_aoLightParm[6] = glGetUniformLocationARB(m_aoLightShader, "orad");
  m_aoLightParm[7] = glGetUniformLocationARB(m_aoLightShader, "ofrac");
  m_aoLightParm[8] = glGetUniformLocationARB(m_aoLightShader, "den1");
  m_aoLightParm[9] = glGetUniformLocationARB(m_aoLightShader, "den2");
  m_aoLightParm[10] = glGetUniformLocationARB(m_aoLightShader, "opmod");
  //---------------------------
}

void
LightHandler::createDirectionalLightShader()
{
  if (m_dLightShader)
    return;

  QString shaderString;

  //---------------------------
  shaderString = LightShaderFactory::genDLightShader();
  if (!loadProgram(m_dLightShader, shaderString,
		   QStringLiteral("lighting/directional")))
    return;
  m_dLightParm[0] = glGetUniformLocationARB(m_dLightShader, "lightTex");
  m_dLightParm[1] = glGetUniformLocationARB(m_dLightShader, "gridx");
  m_dLightParm[2] = glGetUniformLocationARB(m_dLightShader, "gridy");
  m_dLightParm[3] = glGetUniformLocationARB(m_dLightShader, "gridz");
  m_dLightParm[4] = glGetUniformLocationARB(m_dLightShader, "ncols");
  m_dLightParm[5] = glGetUniformLocationARB(m_dLightShader, "ldir");
  m_dLightParm[6] = glGetUniformLocationARB(m_dLightShader, "cangle");
  //---------------------------


  //---------------------------
  shaderString = LightShaderFactory::genInitDLightShader();
  if (!loadProgram(m_initdLightShader, shaderString,
		   QStringLiteral("lighting/directional-init")))
    {
      glDeleteObjectARB(m_dLightShader);
      m_dLightShader = 0;
      return;
    }
  m_initdLightParm[0] = glGetUniformLocationARB(m_initdLightShader, "opTex");
  m_initdLightParm[1] = glGetUniformLocationARB(m_initdLightShader, "gridx");
  m_initdLightParm[2] = glGetUniformLocationARB(m_initdLightShader, "gridy");
  m_initdLightParm[3] = glGetUniformLocationARB(m_initdLightShader, "gridz");
  m_initdLightParm[4] = glGetUniformLocationARB(m_initdLightShader, "ncols");
  m_initdLightParm[5] = glGetUniformLocationARB(m_initdLightShader, "ldir");
  m_initdLightParm[6] = glGetUniformLocationARB(m_initdLightShader, "oplod");
  m_initdLightParm[7] = glGetUniformLocationARB(m_initdLightShader, "opgridx");
  m_initdLightParm[8] = glGetUniformLocationARB(m_initdLightShader, "opgridy");
  m_initdLightParm[9] = glGetUniformLocationARB(m_initdLightShader, "opncols");
  m_initdLightParm[10] = glGetUniformLocationARB(m_initdLightShader, "opmod");
  //---------------------------
}

void
LightHandler::createPointLightShader()
{
  if (m_pLightShader)
    return;

  QString shaderString;

  //---------------------------
  shaderString = LightShaderFactory::genTubeLightShader();
  if (!loadProgram(m_pLightShader, shaderString,
		   QStringLiteral("lighting/point")))
    return;
  m_pLightParm[0] = glGetUniformLocationARB(m_pLightShader, "lightTex");
  m_pLightParm[1] = glGetUniformLocationARB(m_pLightShader, "gridx");
  m_pLightParm[2] = glGetUniformLocationARB(m_pLightShader, "gridy");
  m_pLightParm[3] = glGetUniformLocationARB(m_pLightShader, "gridz");
  m_pLightParm[4] = glGetUniformLocationARB(m_pLightShader, "ncols");
  m_pLightParm[5] = glGetUniformLocationARB(m_pLightShader, "npts");
  m_pLightParm[6] = glGetUniformLocationARB(m_pLightShader, "lpos");
  m_pLightParm[7] = glGetUniformLocationARB(m_pLightShader, "lradius");
  m_pLightParm[8] = glGetUniformLocationARB(m_pLightShader, "ldecay");
  m_pLightParm[9] = glGetUniformLocationARB(m_pLightShader, "cangle");
  //---------------------------

  //---------------------------
  shaderString = LightShaderFactory::genInitTubeLightShader();
  if (!loadProgram(m_initpLightShader, shaderString,
		   QStringLiteral("lighting/point-init")))
    {
      glDeleteObjectARB(m_pLightShader);
      m_pLightShader = 0;
      return;
    }
  m_initpLightParm[0] = glGetUniformLocationARB(m_initpLightShader, "opTex");
  m_initpLightParm[1] = glGetUniformLocationARB(m_initpLightShader, "gridx");
  m_initpLightParm[2] = glGetUniformLocationARB(m_initpLightShader, "gridy");
  m_initpLightParm[3] = glGetUniformLocationARB(m_initpLightShader, "gridz");
  m_initpLightParm[4] = glGetUniformLocationARB(m_initpLightShader, "ncols");
  m_initpLightParm[5] = glGetUniformLocationARB(m_initpLightShader, "npts");
  m_initpLightParm[6] = glGetUniformLocationARB(m_initpLightShader, "lpos");
  m_initpLightParm[7] = glGetUniformLocationARB(m_initpLightShader, "lradius");
  m_initpLightParm[8] = glGetUniformLocationARB(m_initpLightShader, "ldecay");
  m_initpLightParm[9] = glGetUniformLocationARB(m_initpLightShader, "oplod");
  m_initpLightParm[10] = glGetUniformLocationARB(m_initpLightShader, "opgridx");
  m_initpLightParm[11] = glGetUniformLocationARB(m_initpLightShader, "opgridy");
  m_initpLightParm[12] = glGetUniformLocationARB(m_initpLightShader, "opncols");
  m_initpLightParm[13] = glGetUniformLocationARB(m_initpLightShader, "doshadows");
  m_initpLightParm[14] = glGetUniformLocationARB(m_initpLightShader, "opmod");
  //---------------------------

}

void
LightHandler::createExpandShader()
{
  if (m_expandLightShader)
    return;

  QString shaderString;

  //---------------------------
  shaderString = LightShaderFactory::genExpandLightShader();
  if (!loadProgram(m_expandLightShader, shaderString,
		   QStringLiteral("lighting/expand")))
    return;
  m_expandLightParm[0] = glGetUniformLocationARB(m_expandLightShader, "lightTex");
  m_expandLightParm[1] = glGetUniformLocationARB(m_expandLightShader, "gridx");
  m_expandLightParm[2] = glGetUniformLocationARB(m_expandLightShader, "gridy");
  m_expandLightParm[3] = glGetUniformLocationARB(m_expandLightShader, "gridz");
  m_expandLightParm[4] = glGetUniformLocationARB(m_expandLightShader, "ncols");
  m_expandLightParm[5] = glGetUniformLocationARB(m_expandLightShader, "llod");
  m_expandLightParm[6] = glGetUniformLocationARB(m_expandLightShader, "lgridx");
  m_expandLightParm[7] = glGetUniformLocationARB(m_expandLightShader, "lgridy");
  m_expandLightParm[8] = glGetUniformLocationARB(m_expandLightShader, "lncols");
  //---------------------------
}

void
LightHandler::createEmissiveShader()
{
  if (m_emisShader)
    return;

  QString shaderString;

  //---------------------------
  shaderString = LightShaderFactory::genEmissiveShader();
  if (!loadProgram(m_emisShader, shaderString,
		   QStringLiteral("lighting/emissive")))
    return;
  m_emisParm[0] = glGetUniformLocationARB(m_emisShader, "lightTex");
  m_emisParm[1] = glGetUniformLocationARB(m_emisShader, "gridx");
  m_emisParm[2] = glGetUniformLocationARB(m_emisShader, "gridy");
  m_emisParm[3] = glGetUniformLocationARB(m_emisShader, "gridz");
  m_emisParm[5] = glGetUniformLocationARB(m_emisShader, "ncols");
  //m_emisParm[6] = glGetUniformLocationARB(m_emisShader, "ldecay");
  //---------------------------


  //---------------------------
  shaderString = LightShaderFactory::genInitEmissiveShader();
  if (!loadProgram(m_initEmisShader, shaderString,
		   QStringLiteral("lighting/emissive-init")))
    {
      glDeleteObjectARB(m_emisShader);
      m_emisShader = 0;
      return;
    }
  m_initEmisParm[0] = glGetUniformLocationARB(m_initEmisShader, "opTex");
  m_initEmisParm[1] = glGetUniformLocationARB(m_initEmisShader, "gridx");
  m_initEmisParm[2] = glGetUniformLocationARB(m_initEmisShader, "gridy");
  m_initEmisParm[3] = glGetUniformLocationARB(m_initEmisShader, "gridz");
  m_initEmisParm[4] = glGetUniformLocationARB(m_initEmisShader, "opmod");
  m_initEmisParm[5] = glGetUniformLocationARB(m_initEmisShader, "ncols");
  m_initEmisParm[6] = glGetUniformLocationARB(m_initEmisShader, "eTex");
  //---------------------------
}

void
LightHandler::generateOpacityTexture()
{
  if (basicLight() || !m_opacityBuffer || !m_opacityShader)
    return;
  
  // enable lookup texture
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, m_lutTex);
  glEnable(GL_TEXTURE_2D);

  // enable drag texture
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D_ARRAY, m_dataTex);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);



  // generate opacity texture on gpu
  m_opacityBuffer->bind();
  glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
  glClearColor(0, 0, 0, 0);
  glClear(GL_COLOR_BUFFER_BIT);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);
  glUseProgramObjectARB(m_opacityShader);

  int sX = m_opacityBuffer->width();
  int sY = m_opacityBuffer->height();
  int ncols = m_dragInfo.x;
  int nrows = m_dragInfo.y;
  int lod = m_dragInfo.z;
  int gridx = m_dtexX/ncols;
  int gridy = m_dtexY/nrows;
  int gridz = m_subVolSize.z/lod;
  bool opshader = true;
  float tfSet = (float)m_opacityTF/(float)Global::lutSize();

  glUniform1iARB(m_opacityParm[0], 0); // lutTex
  glUniform1iARB(m_opacityParm[1], 1); // dragTex
  glUniform1iARB(m_opacityParm[2], gridx); // gridx
  glUniform1iARB(m_opacityParm[3], gridy); // gridy
  glUniform1iARB(m_opacityParm[4], gridz); // gridz
  glUniform1iARB(m_opacityParm[6], ncols); // ncols
  glUniform1iARB(m_opacityParm[7], m_lightLod); // light lod
  glUniform1iARB(m_opacityParm[8], m_gridx); // light gridx
  glUniform1iARB(m_opacityParm[9], m_gridy); // light gridy
  glUniform1iARB(m_opacityParm[10], m_gridz); // light gridz
  glUniform1iARB(m_opacityParm[12], m_ncols); // light ncols
  glUniform1iARB(m_opacityParm[13], opshader); // opacity shader
  glUniform1fARB(m_opacityParm[14], tfSet); // opacity tfSet
  glUniform3fARB(m_opacityParm[15], m_dragVolSize.x,
		                    m_dragVolSize.y,
		                    m_dragVolSize.z);

  if (m_amrData)
    {
      glActiveTexture(GL_TEXTURE7);
      glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_amrTex);
      glEnable(GL_TEXTURE_RECTANGLE_ARB);
      glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glUniform1iARB(m_opacityParm[16], 7); // AMR texture

      glUniform1fARB(m_opacityParm[17], m_dragInfo.z); // LOD to get back to original volume size
    }

    // enable PruneHandler::prune texture
  glActiveTexture(GL_TEXTURE6);
  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, PruneHandler::texture());  
  glEnable(GL_TEXTURE_RECTANGLE_ARB);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glUniform1iARB(m_opacityParm[18], 6); // PruneHandler::pruneTex

  

  StaticFunctions::pushOrthoView(0, 0, sX, sY);
  StaticFunctions::drawQuad(0, 0, sX, sY, 1.0);
  StaticFunctions::popOrthoView();

  glFinish();
  m_opacityBuffer->release();
  glUseProgramObjectARB(0);

  glActiveTexture(GL_TEXTURE1);

  glActiveTexture(GL_TEXTURE2);
  glDisable(GL_TEXTURE_2D);

  if (m_amrData)
    {
      glActiveTexture(GL_TEXTURE7);
      glDisable(GL_TEXTURE_RECTANGLE_ARB);
    }

  glActiveTexture(GL_TEXTURE6);
  glDisable(GL_TEXTURE_RECTANGLE_ARB);
}

void
LightHandler::generateEmissiveTexture()
{
  if (basicLight() || !m_emisBuffer || !m_emisTex[0] ||
      !m_initEmisShader)
    return;

  glBindFramebuffer(GL_FRAMEBUFFER_EXT, m_emisBuffer);
  for(int i=0; i<2; i++)
    {
      glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
			     GL_COLOR_ATTACHMENT0_EXT,
			     GL_TEXTURE_RECTANGLE_ARB,
			     m_emisTex[i],
			     0);
      glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
      glClearColor(0, 0, 0, 0);
      glClear(GL_COLOR_BUFFER_BIT);
    }
  
  if (m_emisTF < 0 || m_emisTF >= Global::lutSize())
    return;

  // enable lookup texture
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, m_lutTex);
  glEnable(GL_TEXTURE_2D);

  // enable drag texture
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D_ARRAY, m_dataTex);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 


  m_origEmisTex = 0;
  m_dilatedEmisTex = 1;
  // generate emis texture on gpu
  glBindFramebuffer(GL_FRAMEBUFFER_EXT, m_emisBuffer);
  glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
			 GL_COLOR_ATTACHMENT0_EXT,
			 GL_TEXTURE_RECTANGLE_ARB,
			 m_emisTex[m_origEmisTex],
			 0);
  glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
  glClearColor(0, 0, 0, 0);
  glClear(GL_COLOR_BUFFER_BIT);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);
  glUseProgramObjectARB(m_opacityShader);

  int sX = m_opacityBuffer->width();
  int sY = m_opacityBuffer->height();
  int ncols = m_dragInfo.x;
  int nrows = m_dragInfo.y;
  int lod = m_dragInfo.z;
  int gridx = m_dtexX/ncols;
  int gridy = m_dtexY/nrows;
  int gridz = m_subVolSize.z/lod;
  bool opshader = false;
  float tfSet = (float)m_emisTF/(float)Global::lutSize();

  glUniform1iARB(m_opacityParm[0], 0); // lutTex
  glUniform1iARB(m_opacityParm[1], 1); // dragTex
  glUniform1iARB(m_opacityParm[2], gridx); // gridx
  glUniform1iARB(m_opacityParm[3], gridy); // gridy
  glUniform1iARB(m_opacityParm[4], gridz); // gridz
  glUniform1iARB(m_opacityParm[6], ncols); // ncols
  glUniform1iARB(m_opacityParm[7], m_lightLod); // light lod
  glUniform1iARB(m_opacityParm[8], m_gridx); // light gridx
  glUniform1iARB(m_opacityParm[9], m_gridy); // light gridy
  glUniform1iARB(m_opacityParm[10], m_gridz); // light gridz
  glUniform1iARB(m_opacityParm[12], m_ncols); // light ncols
  glUniform1iARB(m_opacityParm[13], opshader); // opacity shader
  glUniform1fARB(m_opacityParm[14], tfSet); // emissive tfSet
  glUniform3fARB(m_opacityParm[15], m_dragVolSize.x,
		                    m_dragVolSize.y,
		                    m_dragVolSize.z);

  StaticFunctions::pushOrthoView(0, 0, sX, sY);
  StaticFunctions::drawQuad(0, 0, sX, sY, 1.0);
  StaticFunctions::popOrthoView();

  glFinish();
  glUseProgramObjectARB(0);

  glActiveTexture(GL_TEXTURE1);


  glActiveTexture(GL_TEXTURE2);
  glDisable(GL_TEXTURE_2D);

  dilateEmissiveTexture();
}

void
LightHandler::dilateEmissiveTexture()
{
  if (basicLight() || !m_emisBuffer || !m_emisTex[0] ||
      !m_diffuseLightShader)
    return;

  int sX = m_ncols*m_gridx;
  int sY = m_nrows*m_gridy;

  for(int i=0; i<2; i++)
    {
      glBindFramebuffer(GL_FRAMEBUFFER_EXT, m_lightBuffer);
      glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
			     GL_COLOR_ATTACHMENT0_EXT,
			     GL_TEXTURE_RECTANGLE_ARB,
			     m_lightTex[i],
			     0);  
      glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
      glClearColor(0, 0, 0, 0);
      glClear(GL_COLOR_BUFFER_BIT);
    }
  glBindFramebuffer(GL_FRAMEBUFFER_EXT, m_emisBuffer);
  glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
			 GL_COLOR_ATTACHMENT0_EXT,
			 GL_TEXTURE_RECTANGLE_ARB,
			 m_emisTex[m_dilatedEmisTex],
			 0);  
  glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
  glClearColor(0, 0, 0, 0);
  glClear(GL_COLOR_BUFFER_BIT);

  glDisable(GL_BLEND);

  glActiveTexture(GL_TEXTURE2);
  glEnable(GL_TEXTURE_RECTANGLE_ARB);
  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_emisTex[m_origEmisTex]);
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 

  // diffuse to lightTex[1] from emisBuffer
  glUseProgramObjectARB(m_diffuseLightShader);
  glUniform1iARB(m_diffuseLightParm[0], 2); // emisBuffer
  glUniform1iARB(m_diffuseLightParm[1], m_gridx); // gridx
  glUniform1iARB(m_diffuseLightParm[2], m_gridy); // gridy
  glUniform1iARB(m_diffuseLightParm[3], m_gridz); // gridz
  glUniform1iARB(m_diffuseLightParm[5], m_ncols); // ncols
  glUniform1fARB(m_diffuseLightParm[6], 1.0 + 0.05*m_emisBoost); // boost
  glBindFramebuffer(GL_FRAMEBUFFER_EXT, m_lightBuffer);
  glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
			 GL_COLOR_ATTACHMENT0_EXT,
			 GL_TEXTURE_RECTANGLE_ARB,
			 m_lightTex[1],
			 0);
  glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
  StaticFunctions::pushOrthoView(0, 0, sX, sY);
  StaticFunctions::drawQuad(0, 0, sX, sY, 1.0);
  StaticFunctions::popOrthoView();
  glFinish();

  int ct = lightBufferCalculations(m_emisTimes);
  glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);

  // now diffuse back to emisBuffer from lightTex[ct]
  glActiveTexture(GL_TEXTURE2);
  glEnable(GL_TEXTURE_RECTANGLE_ARB);
  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_lightTex[ct]);
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 

  glBindFramebuffer(GL_FRAMEBUFFER_EXT, m_emisBuffer);
  glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
			 GL_COLOR_ATTACHMENT0_EXT,
			 GL_TEXTURE_RECTANGLE_ARB,
			 m_emisTex[m_dilatedEmisTex],
			 0);
  glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
  StaticFunctions::pushOrthoView(0, 0, sX, sY);
  StaticFunctions::drawQuad(0, 0, sX, sY, 1.0);
  StaticFunctions::popOrthoView();  
  glFinish();

  glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);

  glUseProgramObjectARB(0);

  glActiveTexture(GL_TEXTURE2);
  glDisable(GL_TEXTURE_RECTANGLE_ARB);
}

void
LightHandler::updateEmissiveBuffer(float ldecay)
{
  glActiveTexture(GL_TEXTURE2);
  glEnable(GL_TEXTURE_RECTANGLE_ARB);
  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_opacityBuffer->texture());
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 

  glActiveTexture(GL_TEXTURE3);
  glEnable(GL_TEXTURE_RECTANGLE_ARB);
  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_emisTex[m_origEmisTex]);
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 

  int sX = m_opacityBuffer->width();
  int sY = m_opacityBuffer->height();

  glDisable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);

  glUseProgramObjectARB(m_initEmisShader);
  glUniform1iARB(m_initEmisParm[0], 2); // opTex
  glUniform1iARB(m_initEmisParm[1], m_gridx); // gridx
  glUniform1iARB(m_initEmisParm[2], m_gridy); // gridy
  glUniform1iARB(m_initEmisParm[3], m_gridz); // gridz
  glUniform1iARB(m_initEmisParm[4], ldecay); // opmod
  glUniform1iARB(m_initEmisParm[5], m_ncols); // ncols
  glUniform1iARB(m_initEmisParm[6], 3); // eTex
  glBindFramebuffer(GL_FRAMEBUFFER_EXT, m_lightBuffer);
  glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
			 GL_COLOR_ATTACHMENT0_EXT,
			 GL_TEXTURE_RECTANGLE_ARB,
			 m_lightTex[1],
			 0);
  glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
  StaticFunctions::pushOrthoView(0, 0, sX, sY);
  StaticFunctions::drawQuad(0, 0, sX, sY, 1.0);
  StaticFunctions::popOrthoView();
  glFinish();
  glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
  //---------------------------------------------


  //---------------------------------------------
  // now start propagating the light front
  glUseProgramObjectARB(m_emisShader);
  glUniform1iARB(m_emisParm[0], 2); // opacity texture
  glUniform1iARB(m_emisParm[1], m_gridx); // gridx
  glUniform1iARB(m_emisParm[2], m_gridy); // gridy
  glUniform1iARB(m_emisParm[3], m_gridz); // gridz
  glUniform1iARB(m_emisParm[5], m_ncols); // ncols
  //glUniform1fARB(m_emisParm[6], qPow(ldecay, 0.5f)); // light falloff
  
  glBindFramebuffer(GL_FRAMEBUFFER_EXT, m_lightBuffer);
  glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
			 GL_COLOR_ATTACHMENT0_EXT,
			 GL_TEXTURE_RECTANGLE_ARB,
			 m_lightTex[0],
			 0);
  glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
  glClearColor(0, 0, 0, 0);
  glClear(GL_COLOR_BUFFER_BIT);
  glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);


  int ntimes = qMax(m_gridx, qMax(m_gridy, m_gridz));
  //ntimes /= 2;

  glBindFramebuffer(GL_FRAMEBUFFER_EXT, m_lightBuffer);
  int ct = lightBufferCalculations(ntimes);
  glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);

  glUseProgramObjectARB(0);

  //-------------------------------------------
  //ct = invertLightBuffer(ct);
  //-------------------------------------------
  
  glActiveTexture(GL_TEXTURE2);
  glDisable(GL_TEXTURE_RECTANGLE_ARB);

  glActiveTexture(GL_TEXTURE3);
  glDisable(GL_TEXTURE_RECTANGLE_ARB);

  // now update final light buffer
  updateFinalLightBuffer(ct);
}

bool
LightHandler::genBuffers()
{
  const auto reportGenerationFailure = [&](const QString& label,
                                            const QString& detail)
    {
      const QString message = QStringLiteral("%1 unavailable: %2")
        .arg(label, detail);
      m_failureReason = message;
      qWarning().noquote() << message;
      if (MainWindowUI::mainWindowUI() &&
          MainWindowUI::mainWindowUI()->statusBar)
        MainWindowUI::mainWindowUI()->statusBar->showMessage(message);
    };
  const qint64 sx64 = static_cast<qint64>(m_ncols)*m_gridx;
  const qint64 sy64 = static_cast<qint64>(m_nrows)*m_gridy;
  if (sx64 <= 0 || sy64 <= 0 ||
      sx64 > Global::max2dTextureSize() || sy64 > Global::max2dTextureSize())
    {
      reportGenerationFailure(QStringLiteral("lighting/framebuffer"),
	   QStringLiteral("computed size %1x%2 is invalid (limit %3)")
	   .arg(sx64).arg(sy64).arg(Global::max2dTextureSize()));
      return false;
    }

  const int sX = static_cast<int>(sx64);
  const int sY = static_cast<int>(sy64);

  const std::uint64_t budgetBytes = 512ULL*1024ULL*1024ULL;
  const FramebufferBudget::Admission candidateBudget =
    FramebufferBudget::evaluate(
      sX, sY, 152ULL, budgetBytes,
      static_cast<int>(Global::max2dTextureSize()));
  if (!candidateBudget.approved)
    {
      reportGenerationFailure(QStringLiteral("lighting/framebuffer"),
	   QStringLiteral("candidate plus old attachments require %1 MiB (budget %2 MiB)")
	   .arg(static_cast<double>(candidateBudget.requiredBytes)/(1024.0*1024.0), 0, 'f', 1)
	   .arg(budgetBytes/(1024ULL*1024ULL)));
      return false;
    }

  if (m_opacityBuffer && m_opacityBuffer->width() == sX &&
      m_opacityBuffer->height() == sY && m_opacityBuffer->isValid() &&
      m_finalLightBuffer && m_finalLightBuffer->isValid() &&
      m_pruneBuffer && m_pruneBuffer->isValid() && m_lightBuffer &&
      m_lightTex[0] && m_lightTex[1] && m_emisBuffer && m_emisTex[0] &&
      m_emisTex[1])
    return true;

  std::uint64_t oldBytes = 0;
  if (m_opacityBuffer && m_opacityBuffer->width() > 0 &&
      m_opacityBuffer->height() > 0)
    {
      const FramebufferBudget::Admission oldBudget =
        FramebufferBudget::evaluate(
          m_opacityBuffer->width(), m_opacityBuffer->height(), 152ULL,
          std::numeric_limits<std::uint64_t>::max(),
          static_cast<int>(Global::max2dTextureSize()));
      if (!oldBudget.approved ||
          oldBudget.requiredBytes > budgetBytes - candidateBudget.requiredBytes)
        {
          reportGenerationFailure(QStringLiteral("lighting/framebuffer"),
               QStringLiteral("candidate plus old attachments exceed the %1 MiB budget")
               .arg(budgetBytes/(1024ULL*1024ULL)));
          return false;
        }
      oldBytes = oldBudget.requiredBytes;
    }
  Q_UNUSED(oldBytes);

  GLint savedFramebuffer = 0;
  GLint savedActiveTexture = GL_TEXTURE0;
  GLint savedRectangleTexture = 0;
  glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &savedFramebuffer);
  glGetIntegerv(GL_ACTIVE_TEXTURE, &savedActiveTexture);
  glGetIntegerv(GL_TEXTURE_BINDING_RECTANGLE_ARB, &savedRectangleTexture);
  const auto restoreGlState = [&]()
    {
      glBindFramebuffer(GL_FRAMEBUFFER_EXT,
                        static_cast<GLuint>(savedFramebuffer));
      glActiveTexture(static_cast<GLenum>(savedActiveTexture));
      glBindTexture(GL_TEXTURE_RECTANGLE_ARB,
                    static_cast<GLuint>(savedRectangleTexture));
    };

  // Keep the current set alive while the complete replacement is allocated.
  // This prevents a failed resize from destroying a still-renderable scene.
  QGLFramebufferObject *candidateOpacity = 0;
  QGLFramebufferObject *candidateFinal = 0;
  QGLFramebufferObject *candidatePrune = 0;
  GLuint candidateLight = 0;
  GLuint candidateLightTex[2] = {0, 0};
  GLuint candidateEmis = 0;
  GLuint candidateEmisTex[2] = {0, 0};
  const auto cleanupCandidate = [&]()
    {
      delete candidateOpacity;
      delete candidateFinal;
      delete candidatePrune;
      candidateOpacity = candidateFinal = candidatePrune = 0;
      if (candidateLight) glDeleteFramebuffers(1, &candidateLight);
      if (candidateEmis) glDeleteFramebuffers(1, &candidateEmis);
      if (candidateLightTex[0]) glDeleteTextures(2, candidateLightTex);
      if (candidateEmisTex[0]) glDeleteTextures(2, candidateEmisTex);
      candidateLight = candidateEmis = 0;
      candidateLightTex[0] = candidateLightTex[1] = 0;
      candidateEmisTex[0] = candidateEmisTex[1] = 0;
      restoreGlState();
    };
  const auto createCandidateFbo = [&](QGLFramebufferObject *&fbo) -> bool
    {
      fbo = new (std::nothrow) QGLFramebufferObject(
        QSize(sX, sY), QGLFramebufferObject::NoAttachment,
        GL_TEXTURE_RECTANGLE_EXT);
      if (!fbo || !fbo->isValid() || !fbo->texture()) return false;
      return true;
    };
  const auto failCandidate = [&](const QString& label,
                                 const QString& detail) -> bool
    {
      cleanupCandidate();
      const QString message = QStringLiteral("%1 unavailable: %2")
        .arg(label, detail);
      m_failureReason = message;
      qWarning().noquote() << message;
      if (MainWindowUI::mainWindowUI() &&
          MainWindowUI::mainWindowUI()->statusBar)
        MainWindowUI::mainWindowUI()->statusBar->showMessage(message);
      return false;
    };

  for (int i=0; i<32 && glGetError() != GL_NO_ERROR; ++i) {}
  if (!createCandidateFbo(candidateOpacity) ||
      !createCandidateFbo(candidatePrune) ||
      !createCandidateFbo(candidateFinal))
    return failCandidate(QStringLiteral("lighting/framebuffer"),
                         QStringLiteral("candidate FBO creation failed"));

  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, candidateFinal->texture());
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glGenFramebuffers(1, &candidateLight);
  glGenTextures(2, candidateLightTex);
  glGenFramebuffers(1, &candidateEmis);
  glGenTextures(2, candidateEmisTex);
  if (!candidateLight || !candidateLightTex[0] || !candidateLightTex[1] ||
      !candidateEmis || !candidateEmisTex[0] || !candidateEmisTex[1])
    return failCandidate(QStringLiteral("lighting/framebuffer"),
                         QStringLiteral("OpenGL did not create all candidate handles"));

  for (int i=0; i<2; ++i)
    {
      glBindTexture(GL_TEXTURE_RECTANGLE_ARB, candidateLightTex[i]);
      glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA32F,
                   sX, sY, 0, GL_RGBA, GL_FLOAT, 0);
      glBindTexture(GL_TEXTURE_RECTANGLE_ARB, candidateEmisTex[i]);
      glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA32F,
                   sX, sY, 0, GL_RGBA, GL_FLOAT, 0);
    }

  const GLenum allocationError = glGetError();
  if (allocationError != GL_NO_ERROR)
    return failCandidate(QStringLiteral("lighting/float textures"),
                         QStringLiteral("allocation failed (OpenGL error 0x%1)")
                         .arg(QString::number(static_cast<qulonglong>(allocationError), 16)));

  const auto validateAttachments = [&](GLuint buffer, GLuint *textures,
                                       const QString &label)
    {
      Q_UNUSED(label);
      glBindFramebuffer(GL_FRAMEBUFFER_EXT, buffer);
      for (int i=0; i<2; ++i)
        {
          glFramebufferTexture2D(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
                                 GL_TEXTURE_RECTANGLE_ARB, textures[i], 0);
          const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER_EXT);
          const GLenum error = glGetError();
          if (status != GL_FRAMEBUFFER_COMPLETE_EXT || error != GL_NO_ERROR)
            {
              glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
              return false;
            }
        }
      glFramebufferTexture2D(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
                             GL_TEXTURE_RECTANGLE_ARB, 0, 0);
      glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
      return true;
    };
  if (!validateAttachments(candidateLight, candidateLightTex,
                           QStringLiteral("lighting/work framebuffer")) ||
      !validateAttachments(candidateEmis, candidateEmisTex,
                           QStringLiteral("lighting/emissive framebuffer")))
    return failCandidate(QStringLiteral("lighting/framebuffer"),
                         QStringLiteral("candidate attachment validation failed"));

  QGLFramebufferObject *oldOpacity = m_opacityBuffer;
  QGLFramebufferObject *oldFinal = m_finalLightBuffer;
  QGLFramebufferObject *oldPrune = m_pruneBuffer;
  const GLuint oldLight = m_lightBuffer;
  const GLuint oldEmis = m_emisBuffer;
  GLuint oldLightTex[2] = {m_lightTex[0], m_lightTex[1]};
  GLuint oldEmisTex[2] = {m_emisTex[0], m_emisTex[1]};
  m_opacityBuffer = candidateOpacity;
  m_finalLightBuffer = candidateFinal;
  m_pruneBuffer = candidatePrune;
  m_lightBuffer = candidateLight;
  m_lightTex[0] = candidateLightTex[0];
  m_lightTex[1] = candidateLightTex[1];
  m_emisBuffer = candidateEmis;
  m_emisTex[0] = candidateEmisTex[0];
  m_emisTex[1] = candidateEmisTex[1];
  candidateOpacity = candidateFinal = candidatePrune = 0;
  candidateLight = candidateEmis = 0;
  candidateLightTex[0] = candidateLightTex[1] = 0;
  candidateEmisTex[0] = candidateEmisTex[1] = 0;
  delete oldOpacity;
  delete oldFinal;
  delete oldPrune;
  if (oldLight) glDeleteFramebuffers(1, &oldLight);
  if (oldEmis) glDeleteFramebuffers(1, &oldEmis);
  if (oldLightTex[0]) glDeleteTextures(2, oldLightTex);
  if (oldEmisTex[0]) glDeleteTextures(2, oldEmisTex);
  restoreGlState();
  return true;
}

void
LightHandler::updateOpacityTexture(GLuint dataTex,
				   int dtX, int dtY,
				   Vec dragInfo, Vec dragVolSize,
				   Vec dataMin, Vec dataMax,
				   Vec subVolSize,
				   uchar *vlut)
{
  m_dtexX = dtX;
  m_dtexY = dtY;
  m_dragInfo = dragInfo;
  m_subVolSize = subVolSize;
  m_dataTex = dataTex;
  m_dataMin = dataMin;
  m_dataMax = dataMax;
  m_dragVolSize = dragVolSize;

  if (vlut)
    {
      if (m_lut) delete [] m_lut;
      m_lut = new uchar[Global::lutSize()*256*256*4];
      memcpy(m_lut, vlut, Global::lutSize()*256*256*4);
    }

  if (!standardChecks()) return;

  //--------------------------
  int ncols = m_dragInfo.x;
  int nrows = m_dragInfo.y;
  int lod = m_dragInfo.z;
  int gridx = m_dtexX/ncols;
  int gridy = m_dtexY/nrows;
  int gridz = m_subVolSize.z/lod;

  m_gridx = gridx/m_lightLod;
  m_gridy = gridy/m_lightLod;
  m_gridz = gridz/m_lightLod;
  m_ncols = dtX/m_gridx;
  if (m_ncols > m_gridz)
    {
      m_ncols = m_gridz;
      m_nrows = 1;
    }
  else
    m_nrows = m_gridz/m_ncols + (m_gridz%m_ncols > 0);  
  //--------------------------

  if (m_basicLight)
    return;

  MainWindowUI::mainWindowUI()->menubar->parentWidget()->\
    setWindowTitle("**********Updating opacity buffer**********");
  
  if (!genBuffers())
    {
      MainWindowUI::mainWindowUI()->menubar->parentWidget()->
	setWindowTitle(Global::DrishtiVersion());
      return;
    }

  generateOpacityTexture();

  generateEmissiveTexture();

  MainWindowUI::mainWindowUI()->menubar->parentWidget()->\
    setWindowTitle(Global::DrishtiVersion());
}

int
LightHandler::applyClipping(int ct)
{
//  MainWindowUI::mainWindowUI()->menubar->parentWidget()->\
//    setWindowTitle("----------apply clipping----------");

  Vec voxelScaling = Global::voxelScaling();
  int sX = m_pruneBuffer->width();
  int sY = m_pruneBuffer->height();

  glUseProgramObjectARB(m_clipShader);
  glUniform1iARB(m_clipParm[0], 2); // lightbuffer
  glUniform1iARB(m_clipParm[1], m_gridx); // gridx
  glUniform1iARB(m_clipParm[2], m_gridy); // gridy
  glUniform1iARB(m_clipParm[3], m_gridz); // gridz
  glUniform1iARB(m_clipParm[4], m_nrows); // nrows
  glUniform1iARB(m_clipParm[5], m_ncols); // ncols

  glActiveTexture(GL_TEXTURE2);
  glEnable(GL_TEXTURE_RECTANGLE_ARB);
  glBindFramebuffer(GL_FRAMEBUFFER_EXT, m_lightBuffer);
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  int lod = m_dragInfo.z;
  lod *= m_lightLod;

  glUniform1iARB(m_clipParm[8], lod); // ncols

  for(int i=0; i<m_clipPos.count(); i++)
    {
      //Vec p = VECDIVIDE(m_clipPos[i], voxelScaling);
      Vec p = m_clipPos[i];
      p -= m_dataMin;
      p /= lod;

      Vec n = VECPRODUCT(m_clipNorm[i], voxelScaling);

      n.normalize();
      
      //------
      p -= 2*n;
      //------

      glUniform3fARB(m_clipParm[6], p.x,p.y,p.z);
      glUniform3fARB(m_clipParm[7], n.x,n.y,n.z);

      glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
			     GL_COLOR_ATTACHMENT0_EXT,
			     GL_TEXTURE_RECTANGLE_ARB,
			     m_lightTex[(ct+1)%2],
			     0);

      glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_lightTex[ct]);      
      glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      
      StaticFunctions::pushOrthoView(0, 0, sX, sY);
      StaticFunctions::drawQuad(0, 0, sX, sY, 1.0);
      StaticFunctions::popOrthoView();
      
      glFinish();

      ct = (ct+1)%2;
    }

  glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
  glUseProgramObjectARB(0);

  //glDisable(GL_TEXTURE_RECTANGLE_ARB);

  return ct;
}

int
LightHandler::applyCropping(int ct)
{
  if (!m_cropShader)
    return ct;

//  MainWindowUI::mainWindowUI()->menubar->parentWidget()->\
//    setWindowTitle("----------Updating crop buffers----------");

  Vec voxelScaling = Global::voxelScaling();
  int sX = m_pruneBuffer->width();
  int sY = m_pruneBuffer->height();

  int lod = m_dragInfo.z;
  lod *= m_lightLod;

  glUseProgramObjectARB(m_cropShader);
  glUniform1iARB(m_cropParm[0], 2); // lightbuffer
  glUniform1iARB(m_cropParm[1], m_gridx); // gridx
  glUniform1iARB(m_cropParm[2], m_gridy); // gridy
  glUniform1iARB(m_cropParm[3], m_gridz); // gridz
  glUniform1iARB(m_cropParm[4], m_nrows); // nrows
  glUniform1iARB(m_cropParm[5], m_ncols); // ncols
  glUniform1iARB(m_cropParm[6], lod); // lod
  glUniform3fARB(m_cropParm[7], voxelScaling.x, voxelScaling.y, voxelScaling.z);
  glUniform3fARB(m_cropParm[8], m_dataMin.x, m_dataMin.y, m_dataMin.z);

  glActiveTexture(GL_TEXTURE2);
  glEnable(GL_TEXTURE_RECTANGLE_ARB);
  glBindFramebuffer(GL_FRAMEBUFFER_EXT, m_lightBuffer);


  glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
			 GL_COLOR_ATTACHMENT0_EXT,
			 GL_TEXTURE_RECTANGLE_ARB,
			 m_lightTex[(ct+1)%2],
			 0);
  
  glActiveTexture(GL_TEXTURE2);
  glEnable(GL_TEXTURE_RECTANGLE_ARB);
  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_lightTex[ct]);      
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  
  StaticFunctions::pushOrthoView(0, 0, sX, sY);
  StaticFunctions::drawQuad(0, 0, sX, sY, 1.0);
  StaticFunctions::popOrthoView();
  
  glFinish();

  ct = (ct+1)%2;

  glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
  glUseProgramObjectARB(0);

  return ct;
}

void
LightHandler::mergeOpPruneBuffers(int ct)
{
  int sX = m_pruneBuffer->width();
  int sY = m_pruneBuffer->height();

  // multiply opacity by pruned data into pruneBuffer
  m_pruneBuffer->bind();
  glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);

  // first clear prune buffer
  glClearColor(0, 0, 0, 0);
  glClear(GL_COLOR_BUFFER_BIT);

  glUseProgramObjectARB(m_mergeOpPruneShader);

  glActiveTexture(GL_TEXTURE2);
  glEnable(GL_TEXTURE_RECTANGLE_ARB);
  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_lightTex[ct]);      
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      
  glActiveTexture(GL_TEXTURE1);
  glEnable(GL_TEXTURE_RECTANGLE_ARB);
  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_opacityBuffer->texture());
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      
  glUniform1iARB(m_mergeOpPruneParm[0], 2); // lightTex
  glUniform1iARB(m_mergeOpPruneParm[1], 1); // opTex
  
  StaticFunctions::pushOrthoView(0, 0, sX, sY);
  StaticFunctions::drawQuad(0, 0, sX, sY, 1.0);
  StaticFunctions::popOrthoView();
  
  glFinish();

  m_pruneBuffer->release();

  glUseProgramObjectARB(0);
  glDisable(GL_BLEND);

  glActiveTexture(GL_TEXTURE1);
  glDisable(GL_TEXTURE_RECTANGLE_ARB);

  glActiveTexture(GL_TEXTURE2);
  glDisable(GL_TEXTURE_RECTANGLE_ARB);
}


void
LightHandler::updatePruneBuffer()
{
  if (!m_available || !m_lightBuffer || !m_lightTex[0] ||
      !m_pruneBuffer || !m_mergeOpPruneShader)
    return;

  // clear both buffer attachments
  glBindFramebuffer(GL_FRAMEBUFFER_EXT, m_lightBuffer);
  for(int i=0; i<2; i++)
    {
      glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
			     GL_COLOR_ATTACHMENT0_EXT,
			     GL_TEXTURE_RECTANGLE_ARB,
			     m_lightTex[i],
			     0);
      glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
      glClearColor(1,1,1,1);
      glClear(GL_COLOR_BUFFER_BIT);
    }

  glClearColor(0,0,0,0);

  int ct = 0;
  if (m_applyClip)
    ct = applyClipping(ct);

  if (m_applyCrop)
    ct = applyCropping(ct);

  if (m_applyCrop && m_blendShader)
    applyBlending(ct);
  else
    mergeOpPruneBuffers(ct);

//  QImage bimg = m_pruneBuffer->toImage();
//  bimg.save("prunebuffer.png");

}


void
LightHandler::updateLightBuffers()
{
  m_lutChanged = false;
  m_onlyLightBuffers = false;

  if (basicLight() || !m_finalLightBuffer || !m_pruneBuffer ||
      !m_lightBuffer || !m_lightTex[0])
    return;

  MainWindowUI::mainWindowUI()->menubar->parentWidget()->\
    setWindowTitle("----------Updating light buffers----------");

  updatePruneBuffer();

  //---------------------------------------------
  // clear final light buffer
  m_finalLightBuffer->bind();
  glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
  glClearColor(0, 0, 0, 0);
  glClear(GL_COLOR_BUFFER_BIT);
  m_finalLightBuffer->release();
  //---------------------------------------------

  if (m_aoLightColor.squaredNorm() > 0.02)
    {
//    updateAmbientOcclusionLightBuffer(m_aoRad, m_aoFrac,
//				      m_aoDensity1, m_aoDensity2, m_aoTimes,
//				      m_aoLightColor, m_aoOpMod);
//    updateAmbientOcclusionLightBuffer(1.0, m_aoFrac,
//				      0.3, 0.75, m_aoTimes,
//				      m_aoLightColor, m_aoOpMod);
      QList<Vec> dpos;
      dpos << Vec(0,0,0);
      updatePointLightBuffer(dpos,
			     0,
			     qMin(m_aoDensity2,0.95f),
			     0,
			     m_aoLightColor,
			     1,
			     m_aoTimes, // m_aoTimes
			     true,
			     m_aoOpMod);
    }      

  if (m_onlyAOLight)
    {
      MainWindowUI::mainWindowUI()->menubar->parentWidget()->	\
	setWindowTitle(Global::DrishtiVersion());
      return;
    }

  if (m_emisTF >=0 && m_emisTF <= Global::lutSize())
    updateEmissiveBuffer(m_emisDecay);

  QList<GiLightGrabber*> lightsPtr = m_giLights->giLightsPtr();
  if (lightsPtr.count() > 0)
    {
      for(int i=0; i<lightsPtr.count(); i++)
	{
	  Vec color = lightsPtr[i]->color();
	  //color *= lightsPtr[i]->opacity();
	  if (color.squaredNorm() > 0.02)
	    {
	      float angle = cos(3.14159265*lightsPtr[i]->angle()/180.0);
	      QList<Vec> pts = lightsPtr[i]->pathPoints();
	      int clod = lightsPtr[i]->lod();
	      int smooth = lightsPtr[i]->smooth();
	      float opmod = lightsPtr[i]->opacity();
	      if (lightsPtr[i]->lightType() == 1) // direction light
		{
		  Vec lv = (pts[0] - pts[1]).unit();
		  updateDirectionalLightBuffer(lv, angle, color,
					       clod, smooth,
					       opmod);
		}
	      else // point light
		{
		  bool doshadows = lightsPtr[i]->doShadows();
		  float rad = lightsPtr[i]->rad();
		  float decay = lightsPtr[i]->decay();
		  updatePointLightBuffer(pts, rad,
					 decay, angle,
					 color, clod, smooth,
					 doshadows,
					 opmod);
		}
	    }
	}
    }
  
  if (m_lightDiffuse > 1)
    diffuseLightBuffer(m_lightDiffuse);

//  QImage bimg = m_finalLightBuffer->toImage();
//  bimg.save("lightbuffer.png");

  for(int i=0; i<lightsPtr.count(); i++)
    lightsPtr[i]->setLightChanged(false);

  MainWindowUI::mainWindowUI()->menubar->parentWidget()->\
    setWindowTitle(Global::DrishtiVersion());
}

void
LightHandler::updateAndLoadLightTexture(GLuint dataTex,
					int dtX, int dtY,
					Vec dragInfo, Vec dragVolSize,
					Vec dataMin, Vec dataMax,
					Vec subVolSize,
					uchar *vlut)
{
  m_doAll = false;
  updateOpacityTexture(dataTex,
		       dtX, dtY,
		       dragInfo, dragVolSize,
		       dataMin, dataMax,
		       subVolSize,
		       vlut);

  if (!m_available || basicLight())
    return;

  updateLightBuffers();
}

void
LightHandler::updateAmbientOcclusionLightBuffer(int orad, float ofrac,
						float den1, float den2,
						int ntimes, Vec lcol,
						float opmod)
{
  glActiveTexture(GL_TEXTURE2);
  glEnable(GL_TEXTURE_RECTANGLE_ARB);
  //glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_opacityBuffer->texture());
  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_pruneBuffer->texture());
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  int sX = m_opacityBuffer->width();
  int sY = m_opacityBuffer->height();

  glDisable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);

  //---------------------------------------------
  // initialize the AO light buffer
  glUseProgramObjectARB(m_aoLightShader);
  glUniform1iARB(m_aoLightParm[0], 2); // opTex
  glUniform1iARB(m_aoLightParm[1], m_gridx); // gridx
  glUniform1iARB(m_aoLightParm[2], m_gridy); // gridy
  glUniform1iARB(m_aoLightParm[3], m_gridz); // gridz
  glUniform1iARB(m_aoLightParm[5], m_ncols); // ncols
  glUniform1iARB(m_aoLightParm[6], orad); // radius for AO calculation
  glUniform1fARB(m_aoLightParm[7], ofrac); // fraction for AO calculation
  glUniform1fARB(m_aoLightParm[8], den1); // light density for AO calculation
  glUniform1fARB(m_aoLightParm[9], den2); // light density for AO calculation
  glUniform1fARB(m_aoLightParm[10], opmod); // opacity modulator
  glBindFramebuffer(GL_FRAMEBUFFER_EXT, m_lightBuffer);
  glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
			 GL_COLOR_ATTACHMENT0_EXT,
			 GL_TEXTURE_RECTANGLE_ARB,
			 m_lightTex[1],
			 0);
  glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
  StaticFunctions::pushOrthoView(0, 0, sX, sY);
  StaticFunctions::drawQuad(0, 0, sX, sY, 1.0);
  StaticFunctions::popOrthoView();
  glFinish();
  glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
  //---------------------------------------------


  glUseProgramObjectARB(m_diffuseLightShader);
  glUniform1iARB(m_diffuseLightParm[0], 2); // finalLightBuffer
  glUniform1iARB(m_diffuseLightParm[1], m_gridx); // gridx
  glUniform1iARB(m_diffuseLightParm[2], m_gridy); // gridy
  glUniform1iARB(m_diffuseLightParm[3], m_gridz); // gridz
  glUniform1iARB(m_diffuseLightParm[5], m_ncols); // ncols
  glUniform1fARB(m_diffuseLightParm[6], 1.0); // boost

  glBindFramebuffer(GL_FRAMEBUFFER_EXT, m_lightBuffer);
  int ct = lightBufferCalculations(ntimes-1);
  glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);

  glUseProgramObjectARB(0);

  glActiveTexture(GL_TEXTURE2);
  glDisable(GL_TEXTURE_RECTANGLE_ARB);

  updateFinalLightBuffer(ct, lcol);
}

void
LightHandler::updateDirectionalLightBuffer(Vec ldir, float cangle, Vec lcol,
					   int clod, int smooth,
					   float opmod)
{
  int sX = m_opacityBuffer->width();
  int sY = m_opacityBuffer->height();

  glDisable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);

  // take care of different light buffer size for direction lights
  int pointLightLod = qMax(clod, m_lightLod);
  float llod;
  int lgridx, lgridy, lgridz, lnrows, lncols, ltexX, ltexY;
  if (m_lightLod == pointLightLod)
    {
      llod = 1;
      lgridx = m_gridx;
      lgridy = m_gridy;
      lgridz = m_gridz;
      lnrows = m_nrows;
      lncols = m_ncols;
      ltexX = m_dtexX;
      ltexY = m_dtexY;
    }
  else
    {
      llod = (float)m_lightLod/(float)pointLightLod;
      lgridx = m_gridx*llod;
      lgridy = m_gridy*llod;
      lgridz = m_gridz*llod;
      lncols = m_dtexX/lgridx;
      lnrows = 1;
      if (lncols > lgridz)
	lncols = lgridz;
      else
	lnrows = lgridz/lncols + (lgridz%lncols > 0);  
      ltexX = lncols*lgridx;
      ltexY = lnrows*lgridy;
    }

  //---------------------------------------------
  // contract and initialize the directional light buffer
  glUseProgramObjectARB(m_initdLightShader);
  glUniform1iARB(m_initdLightParm[0], 2); // opTex
  glUniform1iARB(m_initdLightParm[1], lgridx); // gridx
  glUniform1iARB(m_initdLightParm[2], lgridy); // gridy
  glUniform1iARB(m_initdLightParm[3], lgridz); // gridz
  glUniform1iARB(m_initdLightParm[4], lncols); // ncols
  glUniform3fARB(m_initdLightParm[5], ldir.x, ldir.y, ldir.z); // light direction
  glUniform1fARB(m_initdLightParm[6], llod); // oplod
  glUniform1iARB(m_initdLightParm[7], m_gridx); // opgridx
  glUniform1iARB(m_initdLightParm[8], m_gridy); // opgridy
  glUniform1iARB(m_initdLightParm[9], m_ncols); // opncols
  glUniform1fARB(m_initdLightParm[10], opmod); // opacity modulator

  glActiveTexture(GL_TEXTURE2);
  glEnable(GL_TEXTURE_RECTANGLE_ARB);
  //glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_opacityBuffer->texture());
  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_pruneBuffer->texture());
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  glBindFramebuffer(GL_FRAMEBUFFER_EXT, m_lightBuffer);
  glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
			 GL_COLOR_ATTACHMENT0_EXT,
			 GL_TEXTURE_RECTANGLE_ARB,
			 m_lightTex[1],
			 0);
  glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
  StaticFunctions::pushOrthoView(0, 0, sX, sY);
  StaticFunctions::drawQuad(0, 0, sX, sY, 1.0);
  StaticFunctions::popOrthoView();
  glFinish();
  glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
  //---------------------------------------------


  //---------------------------------------------
  // now start propagating the light front
  glUseProgramObjectARB(m_dLightShader);
  glUniform1iARB(m_dLightParm[0], 2); // lightTex
  glUniform1iARB(m_dLightParm[1], m_gridx); // gridx
  glUniform1iARB(m_dLightParm[2], m_gridy); // gridy
  glUniform1iARB(m_dLightParm[3], m_gridz); // gridz
  glUniform1iARB(m_dLightParm[4], m_ncols); // ncols
  glUniform3fARB(m_dLightParm[5], ldir.x, ldir.y, ldir.z); // light direction
  glUniform1fARB(m_dLightParm[6], cangle); // collection angle
  
  glBindFramebuffer(GL_FRAMEBUFFER_EXT, m_lightBuffer);
  glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
			 GL_COLOR_ATTACHMENT0_EXT,
			 GL_TEXTURE_RECTANGLE_ARB,
			 m_lightTex[0],
			 0);
  glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
  glClearColor(0, 0, 0, 0);
  glClear(GL_COLOR_BUFFER_BIT);
  glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
  
  int ntimes = qMax(lgridx, qMax(lgridy, lgridz));

  glBindFramebuffer(GL_FRAMEBUFFER_EXT, m_lightBuffer);
  int ct = lightBufferCalculations(ntimes);
  glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
  //---------------------------------------------


  //---------------------------------------------
  // expand the light buffer back to original size
  glUseProgramObjectARB(m_expandLightShader);
  glUniform1iARB(m_expandLightParm[0], 2); // opTex
  glUniform1iARB(m_expandLightParm[1], m_gridx); // gridx
  glUniform1iARB(m_expandLightParm[2], m_gridy); // gridy
  glUniform1iARB(m_expandLightParm[3], m_gridz); // gridz
  glUniform1iARB(m_expandLightParm[4], m_ncols); // ncols
  glUniform1fARB(m_expandLightParm[5], llod); // oplod
  glUniform1iARB(m_expandLightParm[6], lgridx); // opgridx
  glUniform1iARB(m_expandLightParm[7], lgridy); // opgridy
  glUniform1iARB(m_expandLightParm[8], lncols); // opncols

  glBindFramebuffer(GL_FRAMEBUFFER_EXT, m_lightBuffer);
  ct = lightBufferCalculations(1, ct);
  glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
  //---------------------------------------------

  if (smooth > 0)
    {
      glUseProgramObjectARB(m_diffuseLightShader);
      glUniform1iARB(m_diffuseLightParm[0], 2); // finalLightBuffer
      glUniform1iARB(m_diffuseLightParm[1], m_gridx); // gridx
      glUniform1iARB(m_diffuseLightParm[2], m_gridy); // gridy
      glUniform1iARB(m_diffuseLightParm[3], m_gridz); // gridz
      glUniform1iARB(m_diffuseLightParm[5], m_ncols); // ncols
      glUniform1fARB(m_diffuseLightParm[6], 1.0); // boost
      
      glBindFramebuffer(GL_FRAMEBUFFER_EXT, m_lightBuffer);
      ct = lightBufferCalculations(smooth, ct);
      glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
    }

  glUseProgramObjectARB(0);

  glActiveTexture(GL_TEXTURE2);
  glDisable(GL_TEXTURE_RECTANGLE_ARB);

  updateFinalLightBuffer(ct, lcol);
}

void
LightHandler::updatePointLightBuffer(QList<Vec> olpos, float lradius,
				     float ldecay, float cangle,
				     Vec lcol, int clod, int smooth,
				     bool doshadows,
				     float opmod)
{
  int sX = m_opacityBuffer->width();
  int sY = m_opacityBuffer->height();

  glDisable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);

  int npts = olpos.count();

  // take care of different light buffer size for point lights
  int pointLightLod = qMax(clod, m_lightLod);

  Vec voxelScaling = Global::voxelScaling();
  float *lpos = new float[3*npts];
  for(int i=0; i<npts; i++)
    {
      Vec v = olpos[i];
      v = VECDIVIDE(v, voxelScaling);
      v = (v-m_dataMin)/m_dragInfo.z/pointLightLod;
      lpos[3*i+0] = v.x;
      lpos[3*i+1] = v.y;
      lpos[3*i+2] = v.z;
    }

  float llod;
  int lgridx, lgridy, lgridz, lnrows, lncols, ltexX, ltexY;
  if (m_lightLod == pointLightLod)
    {
      llod = 1;
      lgridx = m_gridx;
      lgridy = m_gridy;
      lgridz = m_gridz;
      lnrows = m_nrows;
      lncols = m_ncols;
      ltexX = m_dtexX;
      ltexY = m_dtexY;
    }
  else
    {
      llod = (float)m_lightLod/(float)pointLightLod;
      lgridx = m_gridx*llod;
      lgridy = m_gridy*llod;
      lgridz = m_gridz*llod;
      lncols = m_dtexX/lgridx;
      lnrows = 1;
      if (lncols > lgridz)
	lncols = lgridz;
      else
	lnrows = lgridz/lncols + (lgridz%lncols > 0);  
      ltexX = lncols*lgridx;
      ltexY = lnrows*lgridy;
    }

  //---------------------------------------------
  // contract and initialize the point light buffer
  glUseProgramObjectARB(m_initpLightShader);
  glUniform1iARB(m_initpLightParm[0], 2); // opTex
  glUniform1iARB(m_initpLightParm[1], lgridx); // lgridx
  glUniform1iARB(m_initpLightParm[2], lgridy); // lgridy
  glUniform1iARB(m_initpLightParm[3], lgridz); // lgridz
  glUniform1iARB(m_initpLightParm[4], lncols); // lncols
  glUniform1iARB(m_initpLightParm[5], npts); // light radius
  glUniform3fvARB(m_initpLightParm[6], npts, lpos); // light radius
  glUniform1fARB(m_initpLightParm[7], lradius); // light radius

  if (lradius < 1.0)
    glUniform1fARB(m_initpLightParm[8], cangle); // point light cone
  else
    {
      if (doshadows)
        glUniform1fARB(m_initpLightParm[8], qPow(ldecay, 0.5f)); // light decay
      else // change decay value
	glUniform1fARB(m_initpLightParm[8], qPow(ldecay, 0.1f)); // light decay
    }

  glUniform1fARB(m_initpLightParm[9], llod); // oplod
  glUniform1iARB(m_initpLightParm[10], m_gridx); // opgridx
  glUniform1iARB(m_initpLightParm[11], m_gridy); // opgridy
  glUniform1iARB(m_initpLightParm[12], m_ncols); // opncols
  glUniform1iARB(m_initpLightParm[13], doshadows); // doshadows
  glUniform1fARB(m_initpLightParm[14], opmod); // doshadows

  glActiveTexture(GL_TEXTURE2);
  glEnable(GL_TEXTURE_RECTANGLE_ARB);
  //glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_opacityBuffer->texture());
  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_pruneBuffer->texture());
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  glBindFramebuffer(GL_FRAMEBUFFER_EXT, m_lightBuffer);
  glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
			 GL_COLOR_ATTACHMENT0_EXT,
			 GL_TEXTURE_RECTANGLE_ARB,
			 m_lightTex[1],
			 0);
  glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
  StaticFunctions::pushOrthoView(0, 0, ltexX, ltexY);
  StaticFunctions::drawQuad(0, 0, ltexX, ltexY, 1.0);
  StaticFunctions::popOrthoView();
  glFinish();
  glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
  //---------------------------------------------


  if (doshadows)
    {
      //---------------------------------------------
      // now start propagating the point light front
      glUseProgramObjectARB(m_pLightShader);
      glUniform1iARB(m_pLightParm[0], 2); // lightTex
      glUniform1iARB(m_pLightParm[1], lgridx); // gridx
      glUniform1iARB(m_pLightParm[2], lgridy); // gridy
      glUniform1iARB(m_pLightParm[3], lgridz); // gridz
      glUniform1iARB(m_pLightParm[4], lncols); // ncols
      glUniform1iARB(m_pLightParm[5], npts); // light radius
      glUniform3fvARB(m_pLightParm[6], npts, lpos); // light radius
      glUniform1fARB(m_pLightParm[7], lradius); // light radius
      if (doshadows)
        glUniform1fARB(m_pLightParm[8], qPow(ldecay, 0.5f)); // light decay
      else // change decay value
	glUniform1fARB(m_pLightParm[8], qPow(ldecay, 0.1f)); // light decay
      glUniform1fARB(m_pLightParm[9], cangle); // collection angle
      
      glBindFramebuffer(GL_FRAMEBUFFER_EXT, m_lightBuffer);
      glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
			     GL_COLOR_ATTACHMENT0_EXT,
			     GL_TEXTURE_RECTANGLE_ARB,
			     m_lightTex[0],
			     0);
      glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
      glClearColor(0, 0, 0, 0);
      glClear(GL_COLOR_BUFFER_BIT);
      glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
    }

  delete [] lpos;

  int ct = 1;
  if (doshadows)
    {
      int maxtimes = qMax(lgridx, qMax(lgridy, lgridz));
      int ntimes = maxtimes;

      if (lradius < 1.0 && cangle < 1.0) // ambient occulusion
	ntimes*=0.5;
            
      glBindFramebuffer(GL_FRAMEBUFFER_EXT, m_lightBuffer);
      ct = lightBufferCalculations(ntimes, 1, ltexX, ltexY);
      glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
    }

      //---------------------------------------------
  // expand the light buffer back to original size
  glUseProgramObjectARB(m_expandLightShader);
  glUniform1iARB(m_expandLightParm[0], 2); // opTex
  glUniform1iARB(m_expandLightParm[1], m_gridx); // gridx
  glUniform1iARB(m_expandLightParm[2], m_gridy); // gridy
  glUniform1iARB(m_expandLightParm[3], m_gridz); // gridz
  glUniform1iARB(m_expandLightParm[4], m_ncols); // ncols
  glUniform1fARB(m_expandLightParm[5], llod); // oplod
  glUniform1iARB(m_expandLightParm[6], lgridx); // opgridx
  glUniform1iARB(m_expandLightParm[7], lgridy); // opgridy
  glUniform1iARB(m_expandLightParm[8], lncols); // opncols

  glBindFramebuffer(GL_FRAMEBUFFER_EXT, m_lightBuffer);
  ct = lightBufferCalculations(1, ct);
  glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
  //---------------------------------------------

  //if (lradius < 1 && cangle > 0.1) ct = invertLightBuffer(ct);

  if (smooth > 0)
    {
      glUseProgramObjectARB(m_diffuseLightShader);
      glUniform1iARB(m_diffuseLightParm[0], 2); // finalLightBuffer
      glUniform1iARB(m_diffuseLightParm[1], m_gridx); // gridx
      glUniform1iARB(m_diffuseLightParm[2], m_gridy); // gridy
      glUniform1iARB(m_diffuseLightParm[3], m_gridz); // gridz
      glUniform1iARB(m_diffuseLightParm[5], m_ncols); // ncols
      glUniform1fARB(m_diffuseLightParm[6], 1.0); // boost
      
      glBindFramebuffer(GL_FRAMEBUFFER_EXT, m_lightBuffer);
      ct = lightBufferCalculations(smooth, ct);
      glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
    }

  glUseProgramObjectARB(0);
  
  glActiveTexture(GL_TEXTURE2);
  glDisable(GL_TEXTURE_RECTANGLE_ARB);

  updateFinalLightBuffer(ct, lcol);
}

void
LightHandler::updateFinalLightBuffer(int ct, Vec lcol)
{
  int sX = m_finalLightBuffer->width();
  int sY = m_finalLightBuffer->height();

  m_finalLightBuffer->bind();
  glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);

  glDisable(GL_DEPTH_TEST);
  //glDisable(GL_BLEND);
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE);
  glUseProgramObjectARB(m_fLightShader);

  glActiveTexture(GL_TEXTURE2);
  glEnable(GL_TEXTURE_RECTANGLE_ARB);
  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_lightTex[ct]);      
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 

  glUniform1iARB(m_fLightParm[0], 2); // lightTex
  glUniform3fARB(m_fLightParm[1], lcol.x, lcol.y, lcol.z); // light color
  
  StaticFunctions::pushOrthoView(0, 0, sX, sY);
  StaticFunctions::drawQuad(0, 0, sX, sY, 1.0);
  StaticFunctions::popOrthoView();
  
  glFinish();

  m_finalLightBuffer->release();

  glUseProgramObjectARB(0);

  glActiveTexture(GL_TEXTURE2);
  glDisable(GL_TEXTURE_RECTANGLE_ARB);

  glDisable(GL_BLEND);

//  QImage oimg = m_opacityBuffer->toImage();
//  oimg.save("opacitybuffer.png");
//
//  QImage bimg = m_finalLightBuffer->toImage();
//  bimg.save("lightbuffer.png");
}

void
LightHandler::updateFinalLightBuffer(int ct)
{
  int sX = m_finalLightBuffer->width();
  int sY = m_finalLightBuffer->height();

  m_finalLightBuffer->bind();
  glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);

  glDisable(GL_DEPTH_TEST);
  //glDisable(GL_BLEND);
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE);
  glUseProgramObjectARB(m_efLightShader);

  glActiveTexture(GL_TEXTURE2);
  glEnable(GL_TEXTURE_RECTANGLE_ARB);
  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_lightTex[ct]);      
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
      
  glActiveTexture(GL_TEXTURE1);
  glEnable(GL_TEXTURE_RECTANGLE_ARB);
  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_emisTex[m_dilatedEmisTex]);
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
      
  glUniform1iARB(m_efLightParm[0], 2); // lightTex
  glUniform1iARB(m_efLightParm[1], 1); // eTex
  
  StaticFunctions::pushOrthoView(0, 0, sX, sY);
  StaticFunctions::drawQuad(0, 0, sX, sY, 1.0);
  StaticFunctions::popOrthoView();
  
  glFinish();

  m_finalLightBuffer->release();

  glUseProgramObjectARB(0);
  glDisable(GL_BLEND);

  glActiveTexture(GL_TEXTURE1);
  glDisable(GL_TEXTURE_RECTANGLE_ARB);

  glActiveTexture(GL_TEXTURE2);
  glDisable(GL_TEXTURE_RECTANGLE_ARB);

//  QImage oimg = m_opacityBuffer->toImage();
//  oimg.save("opacitybuffer.png");
//
//  QImage bimg = m_finalLightBuffer->toImage();
//  bimg.save("lightbuffer.png");
}

void
LightHandler::diffuseLightBuffer(int ntimes)
{
  int sX = m_finalLightBuffer->width();
  int sY = m_finalLightBuffer->height();

  glActiveTexture(GL_TEXTURE2);
  glEnable(GL_TEXTURE_RECTANGLE_ARB);
  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_finalLightBuffer->texture());
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 

  // diffuse to lightTex[1] from finalLightBuffer
  glUseProgramObjectARB(m_diffuseLightShader);
  glUniform1iARB(m_diffuseLightParm[0], 2); // finalLightBuffer
  glUniform1iARB(m_diffuseLightParm[1], m_gridx); // gridx
  glUniform1iARB(m_diffuseLightParm[2], m_gridy); // gridy
  glUniform1iARB(m_diffuseLightParm[3], m_gridz); // gridz
  glUniform1iARB(m_diffuseLightParm[5], m_ncols); // ncols
  glUniform1fARB(m_diffuseLightParm[6], 1.0); // boost
  glBindFramebuffer(GL_FRAMEBUFFER_EXT, m_lightBuffer);
  glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
			 GL_COLOR_ATTACHMENT0_EXT,
			 GL_TEXTURE_RECTANGLE_ARB,
			 m_lightTex[1],
			 0);
  glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
  StaticFunctions::pushOrthoView(0, 0, sX, sY);
  StaticFunctions::drawQuad(0, 0, sX, sY, 1.0);
  StaticFunctions::popOrthoView();
  glFinish();

  int ct = lightBufferCalculations(ntimes-1);
  glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);

  // now diffuse back to finalLightBuffer from lightTex[1]
  glActiveTexture(GL_TEXTURE2);
  glEnable(GL_TEXTURE_RECTANGLE_ARB);
  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_lightTex[ct]);
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 

  m_finalLightBuffer->bind();
  glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);
  StaticFunctions::pushOrthoView(0, 0, sX, sY);
  StaticFunctions::drawQuad(0, 0, sX, sY, 1.0);
  StaticFunctions::popOrthoView();
  glFinish();
  m_finalLightBuffer->release();

  glUseProgramObjectARB(0);
  glDisable(GL_BLEND);

  glActiveTexture(GL_TEXTURE2);
  glDisable(GL_TEXTURE_RECTANGLE_ARB);
}

bool
LightHandler::openPropertyEditor()
{
  PropertyEditor propertyEditor;
  QMap<QString, QVariantList> plist;
  
  QVariantList vlist;
  
  vlist.clear();
  vlist << QVariant("int");
  vlist << QVariant(m_lightLod);
  vlist << QVariant(1);
  vlist << QVariant(10);
  plist["light buffer size"] = vlist;

  vlist.clear();
  vlist << QVariant("int");
  vlist << QVariant(m_lightDiffuse);
  vlist << QVariant(1);
  vlist << QVariant(10);
  plist["light diffusion"] = vlist;

  vlist.clear();
  vlist << QVariant("int");
  vlist << QVariant(m_opacityTF);
  vlist << QVariant(0);
  vlist << QVariant(Global::lutSize());
  plist["opacity tfset"] = vlist;

  vlist.clear();
  vlist << QVariant("int");
  vlist << QVariant(m_emisTF);
  vlist << QVariant(-1);
  vlist << QVariant(Global::lutSize());
  plist["emis tfset"] = vlist;

  vlist.clear();
  vlist << QVariant("slider");
  vlist << QVariant(m_emisTimes);
  vlist << QVariant(1);
  vlist << QVariant(100);
  vlist << QVariant(10);
  plist["emis smoothing"] = vlist;

  vlist.clear();
  vlist << QVariant("slider");
  vlist << QVariant(m_emisBoost);
  vlist << QVariant(0);
  vlist << QVariant(50);
  vlist << QVariant(5);
  plist["emis boost"] = vlist;

  vlist.clear();
  vlist << QVariant("color");
  QColor dcolor = QColor::fromRgbF(m_aoLightColor.x,
				   m_aoLightColor.y,
				   m_aoLightColor.z);
  vlist << dcolor;
  plist["ao color"] = vlist;
  
  vlist.clear();
  vlist << QVariant("slider");
  vlist << QVariant(qMax(0, (int)(m_aoDensity2*100-50)));
  vlist << QVariant(0);
  vlist << QVariant(50);
  vlist << QVariant(5);
  plist["ao dark level"] = vlist;
  
  vlist.clear();
  vlist << QVariant("slider");
  if (m_aoOpMod <= 1.0)
    vlist << QVariant(qMax(0, (int)(m_aoOpMod*10-2)));
  else
    vlist << QVariant(8+(int)m_aoOpMod);
  vlist << QVariant(0);
  vlist << QVariant(12);
  vlist << QVariant(1);
  plist["ao opmod"] = vlist;

  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(m_onlyAOLight);
  plist["only ao light"] = vlist;
  
  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(m_basicLight);
  plist["only basic light"] = vlist;
  
  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(m_applyClip);
  plist["apply clip"] = vlist;
  
  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(m_applyCrop);
  plist["apply crop"] = vlist;
  
  vlist.clear();
  plist["command"] = vlist;
  
  
  vlist.clear();
  QFile helpFile(":/gilights.help");
  if (helpFile.open(QFile::ReadOnly))
    {
      QTextStream in(&helpFile);
      QString line = in.readLine();
      while (!line.isNull())
	{
	  if (line == "#begin")
	    {
	      QString keyword = in.readLine();
	      QString helptext;
	      line = in.readLine();
	      while (!line.isNull())
		{
		  helptext += line;
		  helptext += "\n";
		  line = in.readLine();
		  if (line == "#end") break;
		}
	      vlist << keyword << helptext;
	    }
	  line = in.readLine();
	}
    }	      
  plist["commandhelp"] = vlist;
  
  QStringList keys;
  keys << "only basic light";
  keys << "only ao light";
  keys << "apply clip";
  keys << "apply crop";
  keys << "gap";
  keys << "light buffer size";
  keys << "light diffusion";
  keys << "opacity tfset";
  keys << "gap";
  keys << "ao color";
  keys << "ao opmod";
  //keys << "ao size";
  //keys << "ao fraction";
  keys << "ao dark level";
  //keys << "ao bright level";
  //keys << "ao smoothing";
  keys << "gap";
  keys << "emis tfset";
  //keys << "emis opmod";
  keys << "emis smoothing";
  keys << "emis boost";
  //keys << "command";
  //keys << "commandhelp";
  
  
  propertyEditor.set("GI Light Parameters", plist, keys);
  propertyEditor.resize(300, 600);  
  
  QMap<QString, QPair<QVariant, bool> > vmap;
  
  if (propertyEditor.exec() == QDialog::Accepted)
    vmap = propertyEditor.get();
  else
    return false;
  
  keys = vmap.keys();

  bool basicLightChanged = false;
  bool emisChanged = false;
  bool dilateChanged = false;
  bool lightlodChanged = false;
  for(int ik=0; ik<keys.count(); ik++)
    {
      QPair<QVariant, bool> pair = vmap.value(keys[ik]);
      
      
      if (pair.second)
	{
	  if (keys[ik] == "only ao light")
	    m_onlyAOLight = pair.first.toBool();
	  else if (keys[ik] == "only basic light")
	    {
	      m_basicLight = pair.first.toBool();
	      basicLightChanged = true;
	    }
	  else if (keys[ik] == "apply clip")
	    {
	      m_applyClip = pair.first.toBool();
	    }
	  else if (keys[ik] == "apply crop")
	    {
	      m_applyCrop = pair.first.toBool();
	    }
	  else if (keys[ik] == "opacity tfset")
	    {
	      if (m_opacityTF != pair.first.toInt())
		{
		  m_opacityTF = pair.first.toInt();
		  generateOpacityTexture();
		}
	    }
	  else if (keys[ik] == "emis tfset")
	    {
	      m_emisTF = pair.first.toInt();
	      emisChanged = true;
	    }
	  else if (keys[ik] == "emis opmod")
	    {
	      m_emisDecay = pair.first.toFloat();
	      emisChanged = true;
	    }
	  else if (keys[ik] == "emis smoothing")
	    {
	      m_emisTimes = pair.first.toInt();
	      dilateChanged = true;
	    }
	  else if (keys[ik] == "emis boost")
	    {	      
	      m_emisBoost = pair.first.toInt();
	      dilateChanged = true;
	    }
	  else if (keys[ik] == "light buffer size")
	    {
	      m_lightLod = pair.first.toInt();
	      lightlodChanged = true;
	    }
	  else if (keys[ik] == "light diffusion")
	    m_lightDiffuse = pair.first.toInt();
	  else if (keys[ik] == "ao color")
	    {
	      QColor color = pair.first.value<QColor>();
	      float r = color.redF();
	      float g = color.greenF();
	      float b = color.blueF();
	      m_aoLightColor = Vec(r,g,b);
	    }
	  else if (keys[ik] == "ao dark level")
	    {
	      //m_aoDensity2 = pair.first.toFloat();
	      m_aoDensity2 = 0.5+0.5*pair.first.toInt()/50.0;
	    }
//	  else if (keys[ik] == "ao smoothing")
//	    m_aoTimes = pair.first.toInt();
	  else if (keys[ik] == "ao opmod")
	    {	      
	      //m_aoOpMod = pair.first.toFloat();
	      float om = pair.first.toInt();
	      if (om <= 8)
		m_aoOpMod = 0.2+0.1*om;
	      else
		m_aoOpMod = om-8;
	    }
	}
    }
  
  if (lightlodChanged)
    updateAndLoadLightTexture(m_dataTex,
			      m_dtexX, m_dtexY,
			      m_dragInfo, m_dragVolSize,
			      m_dataMin, m_dataMax,
			      m_subVolSize,
			      NULL);
  else
    {
      if (emisChanged)
	generateEmissiveTexture();
      else if (dilateChanged)
	dilateEmissiveTexture();

      updateLightBuffers();
    }

  return true;
}

int
LightHandler::lightBufferCalculations(int ntimes, int lct, int lX, int lY)
{
  int sX = m_finalLightBuffer->width();
  int sY = m_finalLightBuffer->height();

  if (lX > 0 && lY > 0)
    {
      sX = lX;
      sY = lY;
    }

  int ct = 1;
  if (lct > -1) ct = lct;

  for (int nt=0; nt<ntimes; nt++)
    {
      glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
			     GL_COLOR_ATTACHMENT0_EXT,
			     GL_TEXTURE_RECTANGLE_ARB,
			     m_lightTex[(ct+1)%2],
			     0);
      
      glActiveTexture(GL_TEXTURE2);
      glEnable(GL_TEXTURE_RECTANGLE_ARB);
      glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_lightTex[ct]);      
      glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      
      StaticFunctions::pushOrthoView(0, 0, sX, sY);
      StaticFunctions::drawQuad(0, 0, sX, sY, 1.0);
      StaticFunctions::popOrthoView();
      
      glFinish();
      
      ct = (ct+1)%2;
    }

  return ct;
}

int
LightHandler::invertLightBuffer(int lct, int lX, int lY)
{
  glUseProgramObjectARB(m_invertLightShader);
  glUniform1iARB(m_invertLightParm[0], 2); // finalLightBuffer
  
  glBindFramebuffer(GL_FRAMEBUFFER_EXT, m_lightBuffer);

  int sX = m_finalLightBuffer->width();
  int sY = m_finalLightBuffer->height();

  if (lX > 0 && lY > 0)
    {
      sX = lX;
      sY = lY;
    }

  int ct = 1;
  if (lct > -1) ct = lct;

  glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
			 GL_COLOR_ATTACHMENT0_EXT,
			 GL_TEXTURE_RECTANGLE_ARB,
			 m_lightTex[(ct+1)%2],
			 0);
      
  glActiveTexture(GL_TEXTURE2);
  glEnable(GL_TEXTURE_RECTANGLE_ARB);
  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_lightTex[ct]);      
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  
  StaticFunctions::pushOrthoView(0, 0, sX, sY);
  StaticFunctions::drawQuad(0, 0, sX, sY, 1.0);
  StaticFunctions::popOrthoView();
  
  glFinish();
      
  ct = (ct+1)%2;

  glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);

  return ct;
}

void LightHandler::setAMR(bool b)  
{
  m_amrData = b;
}
