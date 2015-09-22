#include "keyframe.h"
#include "global.h"
#include "geometryobjects.h"
#include "propertyeditor.h"
#include "staticfunctions.h"
#include "prunehandler.h"
#include "enums.h"
#include "mainwindowui.h"
#include "lighthandler.h"

#include <QInputDialog>

int KeyFrame::numberOfKeyFrames() { return m_keyFrameInfo.count(); }

KeyFrame::KeyFrame()
{
  m_cameraList.clear();
  m_keyFrameInfo.clear();
  m_tgP.clear();
  m_tgQ.clear();
}

KeyFrame::~KeyFrame()
{
  for(int i=0; i<m_cameraList.size(); i++)
    delete m_cameraList[i];
  m_cameraList.clear();

  for(int i=0; i<m_keyFrameInfo.size(); i++)
    delete m_keyFrameInfo[i];
  m_keyFrameInfo.clear();

  m_tgP.clear();
  m_tgQ.clear();
}

void
KeyFrame::clear()
{
  for(int i=0; i<m_cameraList.size(); i++)
    delete m_cameraList[i];
  m_cameraList.clear();

  for(int i=0; i<m_keyFrameInfo.size(); i++)
    delete m_keyFrameInfo[i];
  m_keyFrameInfo.clear();

  m_tgP.clear();
  m_tgQ.clear();

  m_savedKeyFrame.clear();
  m_copyKeyFrame.clear();
}

void
KeyFrame::draw(float widgetSize)
{
  for (QList<CameraPathNode*>::const_iterator it=m_cameraList.begin(),
	 end=m_cameraList.end();
       it != end;
       ++it)
    {
      (*it)->draw(widgetSize);
    }
}

int
KeyFrame::searchCaption(QStringList str)
{
  for(int i=0; i<m_keyFrameInfo.size(); i++)
    {
      if (m_keyFrameInfo[i]->hasCaption(str))
	return m_keyFrameInfo[i]->frameNumber();
    }
  return -1;
}

KeyFrameInformation
KeyFrame::keyFrameInfo(int i)
{
  if (i < m_keyFrameInfo.size())
    return *m_keyFrameInfo[i];
  else
    return KeyFrameInformation();
}

void
KeyFrame::setKeyFrameNumbers(QList<int> fno)
{
  for(int i=0; i<fno.size(); i++)
    m_keyFrameInfo[i]->setFrameNumber(fno[i]);
}

void
KeyFrame::setKeyFrameNumber(int selected, int frameNumber)
{
  m_keyFrameInfo[selected]->setFrameNumber(frameNumber);
}

void
KeyFrame::reorder(QList<int> sorted)
{
  QList<CameraPathNode*> camList(m_cameraList);
  QList<KeyFrameInformation*> kfInfo(m_keyFrameInfo);

  for(int i=0; i<sorted.size(); i++)
    {
      m_cameraList[i] = camList[sorted[i]];
      m_keyFrameInfo[i] = kfInfo[sorted[i]];
    }

  camList.clear();
  kfInfo.clear();

  updateCameraPath();
}

void
KeyFrame::saveProject(Vec pos, Quaternion rot,
		      float focusDistance,
		      float eyeSeparation,
		      int volumeNumber,
		      int volumeNumber2,
		      int volumeNumber3,
		      int volumeNumber4,
		      unsigned char *lut,
		      LightingInformation lightInfo,
		      QList<BrickInformation> brickInfo,
		      Vec bmin, Vec bmax,
		      QImage image,
		      int sz, int st,
		      QString xl, QString yl, QString zl,
		      int mixvol, bool mixColor, bool mixOpacity, bool mixTag,
		      QByteArray pb,
		      float fop, float bop) 
{
  m_savedKeyFrame.clear();
  m_savedKeyFrame.setFrameNumber(-1);
  m_savedKeyFrame.setDrawBox(Global::drawBox());
  m_savedKeyFrame.setDrawAxis(Global::drawAxis());
  m_savedKeyFrame.setBackgroundColor(Global::backgroundColor());
  m_savedKeyFrame.setBackgroundImageFile(Global::backgroundImageFile());
  m_savedKeyFrame.setFocusDistance(focusDistance, eyeSeparation);
  m_savedKeyFrame.setVolumeNumber(volumeNumber);
  m_savedKeyFrame.setVolumeNumber2(volumeNumber2);
  m_savedKeyFrame.setVolumeNumber3(volumeNumber3);
  m_savedKeyFrame.setVolumeNumber4(volumeNumber4);
  m_savedKeyFrame.setPosition(pos);
  m_savedKeyFrame.setOrientation(rot);
  m_savedKeyFrame.setLut(lut);
  m_savedKeyFrame.setTagColors(Global::tagColors());
  m_savedKeyFrame.setLandmarkInfo(GeometryObjects::landmarks()->getLandmarkInfo());
  m_savedKeyFrame.setLightInfo(lightInfo);
  m_savedKeyFrame.setGiLightInfo(LightHandler::giLightInfo());
  m_savedKeyFrame.setClipInfo(GeometryObjects::clipplanes()->clipInfo());
  m_savedKeyFrame.setBrickInfo(brickInfo);
  m_savedKeyFrame.setTick(sz, st, xl, yl, zl);
  m_savedKeyFrame.setMix(mixvol, mixColor, mixOpacity, mixTag);
  m_savedKeyFrame.setVolumeBounds(bmin, bmax);
  m_savedKeyFrame.setImage(image);
  m_savedKeyFrame.setCaptions(GeometryObjects::captions()->captions());
  m_savedKeyFrame.setImageCaptions(GeometryObjects::imageCaptions()->imageCaptions());
  m_savedKeyFrame.setColorBars(GeometryObjects::colorbars()->colorbars());
  m_savedKeyFrame.setScaleBars(GeometryObjects::scalebars()->scalebars());
  m_savedKeyFrame.setPoints(GeometryObjects::hitpoints()->points(),
			    GeometryObjects::hitpoints()->barePoints(),
			    GeometryObjects::hitpoints()->pointSize(),
			    GeometryObjects::hitpoints()->pointColor());
  m_savedKeyFrame.setPaths(GeometryObjects::paths()->paths());
  m_savedKeyFrame.setGrids(GeometryObjects::grids()->grids());
  m_savedKeyFrame.setCrops(GeometryObjects::crops()->crops());
  m_savedKeyFrame.setPathGroups(GeometryObjects::pathgroups()->paths());
  m_savedKeyFrame.setTrisets(GeometryObjects::trisets()->get());
  m_savedKeyFrame.setNetworks(GeometryObjects::networks()->get());
  m_savedKeyFrame.setPruneBuffer(pb);
  m_savedKeyFrame.setPruneBlend(PruneHandler::blend());
  m_savedKeyFrame.setOpMod(fop, bop);

  // not saving splineInfo for savedKeyFrame
  //m_savedKeyFrame.setSplineInfo(splineInfo);
}

void
KeyFrame::setKeyFrame(Vec pos, Quaternion rot,
		      float focusDistance,
		      float eyeSeparation,
		      int frameNumber,
		      unsigned char *lut,
		      LightingInformation lightInfo,
		      QList<BrickInformation> brickInfo,
		      Vec bmin, Vec bmax,
		      QImage image,
		      QList<SplineInformation> splineInfo,
		      int sz, int st,
		      QString xl, QString yl, QString zl,
		      int mixvol, bool mixColor, bool mixOpacity, bool mixTag,
		      float fop, float bop)
{
  int volumeNumber=0, volumeNumber2=0, volumeNumber3=0, volumeNumber4=0;
  volumeNumber = Global::volumeNumber();
  if (Global::volumeType() < Global::RGBVolume)
    {
      if (Global::volumeType() >= Global::DoubleVolume)
	{
	  volumeNumber2 = Global::volumeNumber(1);
	  if (Global::volumeType() >= Global::TripleVolume)
	    {
	      volumeNumber3 = Global::volumeNumber(2);
	      if (Global::volumeType() == Global::QuadVolume)
		volumeNumber4 = Global::volumeNumber(3);
	    }
	}
    }

  QByteArray pb = PruneHandler::getPruneBuffer();

  // -- save keyframe first into a m_savedKeyFrame
  saveProject(pos, rot,
	      focusDistance,
	      eyeSeparation,
	      volumeNumber,
	      volumeNumber2,
	      volumeNumber3,
	      volumeNumber4,
	      lut,
	      lightInfo,
	      brickInfo,
	      bmin, bmax,
	      image,
	      sz, st, xl, yl, zl,
	      mixvol, mixColor, mixOpacity, mixTag,
	      pb,
	      fop, bop);

  bool found = false;
  int kfn = -1;
  for(int i=0; i<m_keyFrameInfo.size(); i++)
    {
      if (m_keyFrameInfo[i]->frameNumber() == frameNumber)
	{
	  kfn= i;
	  found = true;
	  break;
	}
    }	

  CameraPathNode *cam;
  KeyFrameInformation *kfi;
  QString title;
  title = QString("Keyframe %1").arg(frameNumber);
  if (found)
    title = m_keyFrameInfo[kfn]->title();

  if (!found)
    { // append a new node
      kfn = m_keyFrameInfo.count();

      cam = new CameraPathNode(pos, rot);
      m_cameraList.append(cam);
      connect(cam, SIGNAL(modified()),
	      this, SLOT(updateKeyFrameInterpolator()));

      kfi = new KeyFrameInformation();
      m_keyFrameInfo.append(kfi);
    }

  cam = m_cameraList[kfn];
  kfi = m_keyFrameInfo[kfn];
  
  cam->setPosition(pos);
  cam->setOrientation(rot);

  title = QInputDialog::getText(0, "Keyframe Title",
				QString("Title").arg(frameNumber),
				QLineEdit::Normal,
				title);

  kfi->setTitle(title);
  kfi->setFrameNumber(frameNumber);
  kfi->setDrawBox(Global::drawBox());
  kfi->setDrawAxis(Global::drawAxis());
  kfi->setBackgroundColor(Global::backgroundColor());
  kfi->setBackgroundImageFile(Global::backgroundImageFile());
  kfi->setFocusDistance(focusDistance, eyeSeparation);
  kfi->setVolumeNumber(volumeNumber);
  kfi->setVolumeNumber2(volumeNumber2);
  kfi->setVolumeNumber3(volumeNumber3);
  kfi->setVolumeNumber4(volumeNumber4);
  kfi->setPosition(pos);
  kfi->setOrientation(rot);
  kfi->setLut(lut);
  kfi->setTagColors(Global::tagColors());
  kfi->setLandmarkInfo(GeometryObjects::landmarks()->getLandmarkInfo());
  kfi->setLightInfo(lightInfo);
  kfi->setGiLightInfo(LightHandler::giLightInfo());
  kfi->setClipInfo(GeometryObjects::clipplanes()->clipInfo());
  kfi->setBrickInfo(brickInfo);
  kfi->setVolumeBounds(bmin, bmax);
  kfi->setImage(image);
  kfi->setTick(sz, st, xl, yl, zl);
  kfi->setMix(mixvol, mixColor, mixOpacity, mixTag);
  kfi->setSplineInfo(splineInfo);
  kfi->setMorphTF(Global::morphTF());
  kfi->setCaptions(GeometryObjects::captions()->captions());
  kfi->setImageCaptions(GeometryObjects::imageCaptions()->imageCaptions());
  kfi->setColorBars(GeometryObjects::colorbars()->colorbars());
  kfi->setScaleBars(GeometryObjects::scalebars()->scalebars());
  kfi->setPoints(GeometryObjects::hitpoints()->points(),
		 GeometryObjects::hitpoints()->barePoints(),
		 GeometryObjects::hitpoints()->pointSize(),
		 GeometryObjects::hitpoints()->pointColor());
  kfi->setPaths(GeometryObjects::paths()->paths());
  kfi->setGrids(GeometryObjects::grids()->grids());
  kfi->setCrops(GeometryObjects::crops()->crops());
  kfi->setPathGroups(GeometryObjects::pathgroups()->paths());
  kfi->setTrisets(GeometryObjects::trisets()->get());
  kfi->setNetworks(GeometryObjects::networks()->get());
  kfi->setPruneBuffer(pb);
  kfi->setPruneBlend(PruneHandler::blend());
  kfi->setOpMod(fop, bop);

  emit setImage(kfn, image);  
  
  updateKeyFrameInterpolator();
}


void
KeyFrame::updateCameraPath()
{
  int kf = 0;
  for (QList<CameraPathNode*>::const_iterator it=m_cameraList.begin(), end=m_cameraList.end();
       it != end;
       ++it)
    {
      Vec pos = (*it)->position();
      Quaternion q = (*it)->orientation();

      m_keyFrameInfo[kf]->setPosition(pos);
      m_keyFrameInfo[kf]->setOrientation(q);
      kf ++;
    }

  computeTangents();
}

void
KeyFrame::updateKeyFrameInterpolator()
{
  updateCameraPath();

  Global::setPlayFrames(true);
  emit updateGL();
  qApp->processEvents();
}

void
KeyFrame::removeKeyFrame(int fno)
{
  delete m_cameraList[fno];
  delete m_keyFrameInfo[fno];

  m_cameraList.removeAt(fno);
  m_keyFrameInfo.removeAt(fno);

  updateKeyFrameInterpolator();
}

void
KeyFrame::removeKeyFrames(int f0, int f1)
{
  for (int i=f0; i<=f1; i++)
    {
      delete m_cameraList[f0];
      delete m_keyFrameInfo[f0];

      m_cameraList.removeAt(f0);
      m_keyFrameInfo.removeAt(f0);
    }

  updateKeyFrameInterpolator();
}


void
KeyFrame::interpolateAt(int kf, float frc,
			Vec &pos, Quaternion &rot,
			KeyFrameInformation &kfi,
			float &volInterp)
{
  volInterp = 0.0f;

  float rfrc;

  if (kf < numberOfKeyFrames()-1)
    {
      rfrc = StaticFunctions::remapKeyframe(m_keyFrameInfo[kf]->interpCameraPosition(), frc);
      pos = interpolatePosition(kf, kf+1, rfrc);

      rfrc = StaticFunctions::remapKeyframe(m_keyFrameInfo[kf]->interpCameraOrientation(), frc);
      rot = interpolateOrientation(kf, kf+1, rfrc);
    }
  else
    {
      pos = m_keyFrameInfo[kf]->position();
      rot = m_keyFrameInfo[kf]->orientation();
    }


  //-------------------------------  
  kfi.setDrawBox(m_keyFrameInfo[kf]->drawBox());
  kfi.setDrawAxis(m_keyFrameInfo[kf]->drawAxis());
  //-------------------------------  


  //-------------------------------  
  if (m_keyFrameInfo[kf]->interpMop() != Enums::KFIT_None)
    {
      rfrc = StaticFunctions::remapKeyframe(m_keyFrameInfo[kf]->interpMop(), frc);
      QByteArray pb1 = m_keyFrameInfo[kf]->pruneBuffer();
      QByteArray pb2 = m_keyFrameInfo[kf+1]->pruneBuffer();
      QByteArray pb = PruneHandler::interpolate(pb1, pb2, rfrc);
      kfi.setPruneBuffer(pb);
    }
  else
    kfi.setPruneBuffer(m_keyFrameInfo[kf]->pruneBuffer());
  //-------------------------------  

  kfi.setPruneBlend(m_keyFrameInfo[kf]->pruneBlend());


  //-------------------------------  
  rfrc = StaticFunctions::remapKeyframe(m_keyFrameInfo[kf]->interpBGColor(), frc);
  Vec bg0 = m_keyFrameInfo[kf]->backgroundColor();
  Vec bg1 = m_keyFrameInfo[kf+1]->backgroundColor();
  Vec bg = bg0 + rfrc*(bg1-bg0);
  kfi.setBackgroundColor(bg);
  //-------------------------------  


  //-------------------------------  
  float fop, bop, fop0, bop0, fop1, bop1;
  m_keyFrameInfo[kf]->getOpMod(fop0, bop0);
  m_keyFrameInfo[kf+1]->getOpMod(fop1, bop1);
  fop = fop0 + frc*(fop1-fop0);
  bop = bop0 + frc*(bop1-bop0);
  kfi.setOpMod(fop, bop);
  //-------------------------------  


  //-------------------------------  
  if (frc < 0.5)
    kfi.setBackgroundImageFile(m_keyFrameInfo[kf]->backgroundImageFile());
  else
    kfi.setBackgroundImageFile(m_keyFrameInfo[kf+1]->backgroundImageFile());
  //-------------------------------  


  //-------------------------------  
  rfrc = StaticFunctions::remapKeyframe(m_keyFrameInfo[kf]->interpFocus(), frc);
  float focusDistance = m_keyFrameInfo[kf]->focusDistance() +
                        rfrc*(m_keyFrameInfo[kf+1]->focusDistance() -
			      m_keyFrameInfo[kf]->focusDistance());
  float es = m_keyFrameInfo[kf]->eyeSeparation() +
                        rfrc*(m_keyFrameInfo[kf+1]->eyeSeparation() -
			      m_keyFrameInfo[kf]->eyeSeparation());
  kfi.setFocusDistance(focusDistance, es);
  //-------------------------------  

  //-------------------------------  
  int volnum;
  volnum = m_keyFrameInfo[kf]->volumeNumber();
  if (volnum != m_keyFrameInfo[kf+1]->volumeNumber())
    {
      volInterp = volnum +
	          frc*(m_keyFrameInfo[kf+1]->volumeNumber() - volnum);

      if (volnum < m_keyFrameInfo[kf+1]->volumeNumber())
	{
	  volnum = volInterp;
	  volInterp -= volnum; // get fractional part
	  if (volInterp > 0.999)
	    {
	      volnum++;
	      volInterp = 0.0;
	    }
	}
      else
	{
	  volnum = ceil(volInterp);
	  volInterp = volnum - volInterp; // get fractional part
	}
    }
  kfi.setVolumeNumber(volnum);



  volnum = m_keyFrameInfo[kf]->volumeNumber2();
  if (volnum != m_keyFrameInfo[kf+1]->volumeNumber2())
    {
      float vi = volnum +
	          frc*(m_keyFrameInfo[kf+1]->volumeNumber2() - volnum);
      if (volnum < m_keyFrameInfo[kf+1]->volumeNumber2())
	{
	  volnum = vi;
	  if (vi-volnum > 0.999)
	    volnum++;
	}
      else
	volnum = ceil(vi);
    }
  kfi.setVolumeNumber2(volnum);

  volnum = m_keyFrameInfo[kf]->volumeNumber3();
  if (volnum != m_keyFrameInfo[kf+1]->volumeNumber3())
    {
      float vi = volnum +
	          frc*(m_keyFrameInfo[kf+1]->volumeNumber3() - volnum);
      if (volnum < m_keyFrameInfo[kf+1]->volumeNumber3())
	{
	  volnum = vi;
	  if (vi-volnum > 0.999)
	    volnum++;
	}
      else
	volnum = ceil(vi);
    }
  kfi.setVolumeNumber3(volnum);

  volnum = m_keyFrameInfo[kf]->volumeNumber4();
  if (volnum != m_keyFrameInfo[kf+1]->volumeNumber4())
    {
      float vi = volnum +
	          frc*(m_keyFrameInfo[kf+1]->volumeNumber4() - volnum);
      if (volnum < m_keyFrameInfo[kf+1]->volumeNumber4())
	{
	  volnum = vi;
	  if (vi-volnum > 0.999)
	    volnum++;
	}
      else
	volnum = ceil(vi);
    }
  kfi.setVolumeNumber4(volnum);
  //-------------------------------  

  //-------------------------------  
  if (m_keyFrameInfo[kf]->lut() &&
      m_keyFrameInfo[kf+1]->lut())
    {
      uchar *lut1 = m_keyFrameInfo[kf]->lut();
      uchar *lut2 = m_keyFrameInfo[kf+1]->lut();
      rfrc = StaticFunctions::remapKeyframe(m_keyFrameInfo[kf]->interpTF(), frc);
      unsigned char *lut = new unsigned char[Global::lutSize()*256*256*4];

//#ifdef MAC_OS_X_VERSION_10_8
      for(int j=0; j<Global::lutSize()*256*256*4; j++)
	lut[j] = (1.0-rfrc)*lut1[j] + rfrc*lut2[j];
//#else
//      // convert into HSV space and then interpolate
//      for (int j=0; j<Global::lutSize()*256*256; j++)
//	{      
//	  int r1,g1,b1,a1;
//	  int r2,g2,b2,a2;
//	  int r,g,b,a;
//
//	  r1 = lut1[4*j+0];
//	  g1 = lut1[4*j+1];
//	  b1 = lut1[4*j+2];
//	  a1 = lut1[4*j+3];	  
//
//	  r2 = lut2[4*j+0];
//	  g2 = lut2[4*j+1];
//	  b2 = lut2[4*j+2];
//	  a2 = lut2[4*j+3];	  
//
//	  QColor c1 = QColor(r1, g1, b1);
//	  QColor c2 = QColor(r2, g2, b2);
//	  qreal h1,s1,v1;
//	  qreal h2,s2,v2;
//	  qreal h,s,v;
//	  c1.getHsvF(&h1, &s1, &v1);
//	  c2.getHsvF(&h2, &s2, &v2);
//	  h1 = qBound(0.0, h1, 1.0);
//	  s1 = qBound(0.0, s1, 1.0);
//	  v1 = qBound(0.0, v1, 1.0);
//	  h2 = qBound(0.0, h2, 1.0);
//	  s2 = qBound(0.0, s2, 1.0);
//	  v2 = qBound(0.0, v2, 1.0);
//
//	  s = (1-rfrc)*s1 + rfrc*s2;
//	  v = (1-rfrc)*v1 + rfrc*v2;
//	  a = (1-rfrc)*a1 + rfrc*a2;
//
//	  QColor::fromHsvF(h1,1.0,1.0).getRgb(&r1, &g1, &b1);
//	  QColor::fromHsvF(h2,1.0,1.0).getRgb(&r2, &g2, &b2);
//	  r = (1-rfrc)*r1 + rfrc*r2;
//	  g = (1-rfrc)*g1 + rfrc*g2;
//	  b = (1-rfrc)*b1 + rfrc*b2;
//	  QColor::fromRgb(r,g,b).getHsvF(&h, &s1, &v1);
//	  h = qBound(0.0, h, 1.0);
//	  s = qBound(0.0, s, 1.0);
//	  v = qBound(0.0, v, 1.0);
//	  QColor::fromHsvF(h,s,v).getRgb(&r, &g, &b);
//
//	  if (v*a < 10)
//	    {
//	      float va = (float)(v*a)/10.0f;
//	      r = (1-va)*v + va*r;
//	      g = (1-va)*v + va*g;
//	      b = (1-va)*v + va*b;
//	    }
//
//	  lut[4*j+0] = r;
//	  lut[4*j+1] = g;
//	  lut[4*j+2] = b;
//	  lut[4*j+3] = a;
//	}
//#endif

      kfi.setLut(lut);
      delete [] lut;
    }
  //-------------------------------  

  //-------------------------------  
  rfrc = StaticFunctions::remapKeyframe(m_keyFrameInfo[kf]->interpTagColors(), frc);
  unsigned char *tc = new unsigned char[1024];
  unsigned char *tc1 = m_keyFrameInfo[kf]->tagColors();
  unsigned char *tc2 = m_keyFrameInfo[kf+1]->tagColors();
  for(int j=0; j<1024; j++)
    tc[j] = tc1[j] + rfrc*(tc2[j] - tc1[j]);
  kfi.setTagColors(tc);
  delete [] tc;
  //-------------------------------  

  //-------------------------------
  if (frc < 0.5)
    kfi.setLandmarkInfo(m_keyFrameInfo[kf]->landmarkInfo());
  else
    kfi.setLandmarkInfo(m_keyFrameInfo[kf+1]->landmarkInfo());
  //-------------------------------

  //-------------------------------  
  rfrc = StaticFunctions::remapKeyframe(m_keyFrameInfo[kf]->interpLightInfo(), frc);
  LightingInformation lightInfo;
  LightingInformation lightInfo1 = m_keyFrameInfo[kf]->lightInfo();
  LightingInformation lightInfo2 = m_keyFrameInfo[kf+1]->lightInfo();
  lightInfo = LightingInformation::interpolate(lightInfo1, lightInfo2, rfrc);
  kfi.setLightInfo(lightInfo);
  //-------------------------------  

  //-------------------------------  
  rfrc = StaticFunctions::remapKeyframe(m_keyFrameInfo[kf]->interpGiLightInfo(), frc);
  GiLightInfo giLightInfo;
  GiLightInfo giLightInfo1 = m_keyFrameInfo[kf]->giLightInfo();
  GiLightInfo giLightInfo2 = m_keyFrameInfo[kf+1]->giLightInfo();
  giLightInfo = GiLightInfo::interpolate(giLightInfo1, giLightInfo2, rfrc);
  kfi.setGiLightInfo(giLightInfo);
  //-------------------------------  

  //-------------------------------  
  rfrc = StaticFunctions::remapKeyframe(m_keyFrameInfo[kf]->interpClipInfo(), frc);
  ClipInformation clipInfo;
  ClipInformation clipInfo1 = m_keyFrameInfo[kf]->clipInfo();
  ClipInformation clipInfo2 = m_keyFrameInfo[kf+1]->clipInfo();
  clipInfo = ClipInformation::interpolate(clipInfo1, clipInfo2, rfrc);
  kfi.setClipInfo(clipInfo);
  //-------------------------------  

  //-------------------------------  
  rfrc = StaticFunctions::remapKeyframe(m_keyFrameInfo[kf]->interpBrickInfo(), frc);
  QList<BrickInformation> brickInfo;  
  QList<BrickInformation> brickInfo1 = m_keyFrameInfo[kf]->brickInfo();
  QList<BrickInformation> brickInfo2 = m_keyFrameInfo[kf+1]->brickInfo();
  brickInfo = BrickInformation::interpolate(brickInfo1, brickInfo2, rfrc);
  kfi.setBrickInfo(brickInfo);
  //-------------------------------  

  //-------------------------------  
  kfi.setMorphTF(m_keyFrameInfo[kf]->morphTF());
  if (m_keyFrameInfo[kf]->morphTF())
    {
      rfrc = StaticFunctions::remapKeyframe(m_keyFrameInfo[kf]->interpTF(), frc);
      //-------------------------------  
      QList<SplineInformation> splineInfo;
      QList<SplineInformation> splineInfo1(m_keyFrameInfo[kf]->splineInfo());
      QList<SplineInformation> splineInfo2(m_keyFrameInfo[kf+1]->splineInfo());
      splineInfo = SplineInformation::interpolate(splineInfo1, splineInfo2, rfrc);
      kfi.setSplineInfo(splineInfo);
      //-------------------------------  
    }
  //-------------------------------  


  //-------------------------------  
  rfrc = StaticFunctions::remapKeyframe(m_keyFrameInfo[kf]->interpTickInfo(), frc);
  int sz0, st0;
  QString xl0, yl0, zl0;
  int sz1, st1;
  QString xl1, yl1, zl1;
  int sz, st;
  QString xl, yl, zl;
  m_keyFrameInfo[kf]->getTick(sz0, st0, xl0, yl0, zl0);
  m_keyFrameInfo[kf+1]->getTick(sz1, st1, xl1, yl1, zl1);
  sz = sz0 + rfrc*(sz1-sz0);
  st = st0 + rfrc*(st1-st0);
  kfi.setTick(sz, st, xl0, yl0, zl0);
  //-------------------------------


  //-------------------------------  
  int mv;
  bool mc, mo, mt;  
  m_keyFrameInfo[kf]->getMix(mv, mc, mo, mt);
  kfi.setMix(mv, mc, mo, mt);
  //-------------------------------  

  // no interpolation for image captions
  kfi.setImageCaptions(m_keyFrameInfo[kf]->imageCaptions());

  //-------------------------------  
  rfrc = StaticFunctions::remapKeyframe(m_keyFrameInfo[kf]->interpCaptions(), frc);
  QList<CaptionObject> caption;  
  QList<CaptionObject> caption1 = m_keyFrameInfo[kf]->captions();
  QList<CaptionObject> caption2 = m_keyFrameInfo[kf+1]->captions();
  caption = CaptionObject::interpolate(caption1, caption2, rfrc);
  kfi.setCaptions(caption);
  //-------------------------------  

  kfi.setColorBars(m_keyFrameInfo[kf]->colorbars());

  //-------------------------------  
  QList<ScaleBarObject> scalebars;  
  QList<ScaleBarObject> scalebars1 = m_keyFrameInfo[kf]->scalebars();
  QList<ScaleBarObject> scalebars2 = m_keyFrameInfo[kf+1]->scalebars();
  scalebars = ScaleBarObject::interpolate(scalebars1, scalebars2, frc);
  kfi.setScaleBars(scalebars);
  //kfi.setScaleBars(m_keyFrameInfo[kf]->scalebars());
  //-------------------------------  
  
  //-------------------------------  
  rfrc = StaticFunctions::remapKeyframe(m_keyFrameInfo[kf]->interpCrop(), frc);
  QList<CropObject> crop;
  QList<CropObject> crop1 = m_keyFrameInfo[kf]->crops();
  QList<CropObject> crop2 = m_keyFrameInfo[kf+1]->crops();
  crop = CropObject::interpolate(crop1, crop2, rfrc);
  kfi.setCrops(crop);
  //-------------------------------  

  //-------------------------------  
  int ptsz0 = m_keyFrameInfo[kf]->pointSize();
  int ptsz1 = m_keyFrameInfo[kf+1]->pointSize();
  int ptsz = ptsz0 + frc*(ptsz1-ptsz0);
  Vec ptcol0 = m_keyFrameInfo[kf]->pointColor();
  Vec ptcol1 = m_keyFrameInfo[kf+1]->pointColor();
  Vec ptcol = ptcol0 + frc*(ptcol1-ptcol0);
  kfi.setPoints(m_keyFrameInfo[kf]->points(),
		m_keyFrameInfo[kf]->barepoints(),
		ptsz, ptcol);
  kfi.setGrids(m_keyFrameInfo[kf]->grids());

  //kfi.setPaths(m_keyFrameInfo[kf]->paths());
  QList<PathObject> po;
  QList<PathObject> po1 = m_keyFrameInfo[kf]->paths();
  QList<PathObject> po2 = m_keyFrameInfo[kf+1]->paths();
  po = PathObject::interpolate(po1, po2, frc);
  kfi.setPaths(po);
  //-------------------------------  
  
  //  kfi.setPathGroups(m_keyFrameInfo[kf]->pathgroups());
  QList<PathGroupObject> pgo;
  QList<PathGroupObject> pgo1 = m_keyFrameInfo[kf]->pathgroups();
  QList<PathGroupObject> pgo2 = m_keyFrameInfo[kf+1]->pathgroups();
  pgo = PathGroupObject::interpolate(pgo1, pgo2, frc);
  kfi.setPathGroups(pgo);
  //-------------------------------  

  //-------------------------------  
  QList<TrisetInformation> tinfo;  
  QList<TrisetInformation> tinfo1 = m_keyFrameInfo[kf]->trisets();
  QList<TrisetInformation> tinfo2 = m_keyFrameInfo[kf+1]->trisets();
  tinfo = TrisetInformation::interpolate(tinfo1, tinfo2, frc);
  kfi.setTrisets(tinfo);
  //-------------------------------  

  //-------------------------------  
  QList<NetworkInformation> ninfo;  
  QList<NetworkInformation> ninfo1 = m_keyFrameInfo[kf]->networks();
  QList<NetworkInformation> ninfo2 = m_keyFrameInfo[kf+1]->networks();
  ninfo = NetworkInformation::interpolate(ninfo1, ninfo2, frc);
  kfi.setNetworks(ninfo);
  //-------------------------------  


// --- do not interpolate the volume bounds ---
//  rfrc = StaticFunctions::remapKeyframe(m_keyFrameInfo[kf]->interpVolumeBounds(), frc);
  Vec bmin, bmax;
  Vec bmin1, bmax1;
  Vec bmin2, bmax2;
  m_keyFrameInfo[kf]->volumeBounds(bmin1, bmax1);
  m_keyFrameInfo[kf+1]->volumeBounds(bmin2, bmax2);
//  bmin = bmin1 + rfrc*(bmin2-bmin1);
//  bmax = bmax1 + rfrc*(bmax2-bmax1);

  if (frc <= 0.5)
    {
      bmin = bmin1;
      bmax = bmax1;
    }
  else
    {
      bmin = bmin2;
      bmax = bmax2;
    }

  kfi.setVolumeBounds(bmin, bmax);
}

void
KeyFrame::playSavedKeyFrame()
{
  Global::disableViewerUpdate();
  MainWindowUI::changeDrishtiIcon(false);

  Vec pos;
  Quaternion rot;
  pos = m_savedKeyFrame.position();
  rot = m_savedKeyFrame.orientation();

  Vec bmin, bmax;
  m_savedKeyFrame.volumeBounds(bmin, bmax);

  emit updateVolumeBounds(bmin, bmax);

  if (Global::volumeType() == Global::SingleVolume)
    {
      int volnum = m_savedKeyFrame.volumeNumber();
      emit updateVolInfo(volnum);
      emit updateVolumeBounds(volnum, bmin, bmax);
    }
  else if (Global::volumeType() == Global::DoubleVolume)
    {
      int volnum1 = m_savedKeyFrame.volumeNumber();
      int volnum2 = m_savedKeyFrame.volumeNumber2();
      emit updateVolInfo(volnum1);
      emit updateVolInfo(1, volnum2);
      emit updateVolumeBounds(volnum1, volnum2, bmin, bmax);
    }
  else if (Global::volumeType() == Global::TripleVolume)
    {
      int volnum1 = m_savedKeyFrame.volumeNumber();
      int volnum2 = m_savedKeyFrame.volumeNumber2();
      int volnum3 = m_savedKeyFrame.volumeNumber3();
      emit updateVolInfo(volnum1);
      emit updateVolInfo(1, volnum2);
      emit updateVolInfo(2, volnum3);
      emit updateVolumeBounds(volnum1, volnum2, volnum3, bmin, bmax);
    }
  else if (Global::volumeType() == Global::QuadVolume)
    {
      int volnum1 = m_savedKeyFrame.volumeNumber();
      int volnum2 = m_savedKeyFrame.volumeNumber2();
      int volnum3 = m_savedKeyFrame.volumeNumber3();
      int volnum4 = m_savedKeyFrame.volumeNumber4();
      emit updateVolInfo(volnum1);
      emit updateVolInfo(1, volnum2);
      emit updateVolInfo(2, volnum3);
      emit updateVolInfo(3, volnum4);
      emit updateVolumeBounds(volnum1, volnum2, volnum3, volnum4, bmin, bmax);
    }

  Global::setTagColors(m_savedKeyFrame.tagColors());
  emit updateTagColors();

  GeometryObjects::captions()->setCaptions(m_savedKeyFrame.captions());
  GeometryObjects::imageCaptions()->setImageCaptions(m_savedKeyFrame.imageCaptions());
  GeometryObjects::colorbars()->setColorBars(m_savedKeyFrame.colorbars());
  GeometryObjects::scalebars()->setScaleBars(m_savedKeyFrame.scalebars());
  GeometryObjects::hitpoints()->setPoints(m_savedKeyFrame.points());
  GeometryObjects::hitpoints()->setBarePoints(m_savedKeyFrame.barepoints());
  GeometryObjects::hitpoints()->setPointSize(m_savedKeyFrame.pointSize());
  GeometryObjects::hitpoints()->setPointColor(m_savedKeyFrame.pointColor());
  GeometryObjects::paths()->setPaths(m_savedKeyFrame.paths());
  GeometryObjects::grids()->setGrids(m_savedKeyFrame.grids());
  GeometryObjects::crops()->setCrops(m_savedKeyFrame.crops());
  GeometryObjects::pathgroups()->setPaths(m_savedKeyFrame.pathgroups());
  GeometryObjects::trisets()->set(m_savedKeyFrame.trisets());
  GeometryObjects::networks()->set(m_savedKeyFrame.networks());
  GeometryObjects::clipplanes()->set(m_savedKeyFrame.clipInfo());
  GeometryObjects::landmarks()->setLandmarkInfo(m_savedKeyFrame.landmarkInfo());

  float focusDistance = m_savedKeyFrame.focusDistance();
  float es = m_savedKeyFrame.eyeSeparation();
  emit updateFocus(focusDistance, es);
  emit updateLookFrom(pos, rot, focusDistance, es);

  LightHandler::setGiLightInfo(m_savedKeyFrame.giLightInfo());

  emit updateBrickInfo(m_savedKeyFrame.brickInfo());
  emit updateLightInfo(m_savedKeyFrame.lightInfo());

  if (m_savedKeyFrame.lut())
    emit updateLookupTable(m_savedKeyFrame.lut());

  bool drawBox = m_savedKeyFrame.drawBox();
  bool drawAxis = m_savedKeyFrame.drawAxis();
  Vec backgroundColor = m_savedKeyFrame.backgroundColor();
  QString backgroundImage = m_savedKeyFrame.backgroundImageFile();
  int sz, st;
  QString xl, yl, zl;
  m_savedKeyFrame.getTick(sz, st, xl, yl, zl);
  int mv;
  bool mc, mo, mt;
  m_savedKeyFrame.getMix(mv, mc, mo, mt);

  float fop, bop;
  m_savedKeyFrame.getOpMod(fop, bop);

  emit updateParameters(drawBox, drawAxis,
			backgroundColor,
			backgroundImage,
			sz, st, xl, yl, zl,
			mv, mc, mo, 0.0f, mt,
			m_savedKeyFrame.pruneBlend(),
			fop, bop);

  QByteArray pb = m_savedKeyFrame.pruneBuffer();
  if (! pb.isEmpty())
    PruneHandler::setPruneBuffer(pb);

  Global::enableViewerUpdate();
  MainWindowUI::changeDrishtiIcon(true);
  Global::setPlayFrames(true);

  emit updateLightBuffers();

  emit updateGL();
  qApp->processEvents();  

  return;
}

void
KeyFrame::playFrameNumber(int fno)
{
  if (numberOfKeyFrames() == 0)
    return;

  emit currentFrameChanged(fno);
  qApp->processEvents();

  int maxFrame = m_keyFrameInfo[numberOfKeyFrames()-1]->frameNumber();
  int minFrame = m_keyFrameInfo[0]->frameNumber();

  if (fno > maxFrame || fno < minFrame)
    return;

  
  for(int kf=0; kf<numberOfKeyFrames(); kf++)
    {
      if (fno == m_keyFrameInfo[kf]->frameNumber())
	{
	  Global::disableViewerUpdate();
	  MainWindowUI::changeDrishtiIcon(false);

	  Vec pos;
	  Quaternion rot;
	  pos = m_keyFrameInfo[kf]->position();
	  rot = m_keyFrameInfo[kf]->orientation();
	  float focusDistance = m_keyFrameInfo[kf]->focusDistance();
	  float es = m_keyFrameInfo[kf]->eyeSeparation();
	  emit updateFocus(focusDistance, es);
	  emit updateLookFrom(pos, rot, focusDistance, es);
	  
	  Vec bmin, bmax;
	  m_keyFrameInfo[kf]->volumeBounds(bmin, bmax);

	  emit updateVolumeBounds(bmin, bmax);

	  if (Global::volumeType() == Global::SingleVolume)
	    {
	      int volnum = m_keyFrameInfo[kf]->volumeNumber();
	      emit updateVolInfo(volnum);
	      emit updateVolumeBounds(volnum, bmin, bmax);
	    }
	  else if (Global::volumeType() == Global::DoubleVolume)
	    {
	      int volnum1 = m_keyFrameInfo[kf]->volumeNumber();
	      int volnum2 = m_keyFrameInfo[kf]->volumeNumber2();
	      emit updateVolInfo(volnum1);
	      emit updateVolInfo(1, volnum2);
	      emit updateVolumeBounds(volnum1, volnum2, bmin, bmax);
	    }
	  else if (Global::volumeType() == Global::TripleVolume)
	    {
	      int volnum1 = m_keyFrameInfo[kf]->volumeNumber();
	      int volnum2 = m_keyFrameInfo[kf]->volumeNumber2();
	      int volnum3 = m_keyFrameInfo[kf]->volumeNumber3();
	      emit updateVolInfo(volnum1);
	      emit updateVolInfo(1, volnum2);
	      emit updateVolInfo(2, volnum3);
	      emit updateVolumeBounds(volnum1, volnum2, volnum3, bmin, bmax);
	    }
	  else if (Global::volumeType() == Global::QuadVolume)
	    {
	      int volnum1 = m_keyFrameInfo[kf]->volumeNumber();
	      int volnum2 = m_keyFrameInfo[kf]->volumeNumber2();
	      int volnum3 = m_keyFrameInfo[kf]->volumeNumber3();
	      int volnum4 = m_keyFrameInfo[kf]->volumeNumber4();
	      emit updateVolInfo(volnum1);
	      emit updateVolInfo(1, volnum2);
	      emit updateVolInfo(2, volnum3);
	      emit updateVolInfo(3, volnum4);
	      emit updateVolumeBounds(volnum1, volnum2, volnum3, volnum4, bmin, bmax);
	    }
	  
	  GeometryObjects::captions()->setCaptions(m_keyFrameInfo[kf]->captions());
	  GeometryObjects::imageCaptions()->setImageCaptions(m_keyFrameInfo[kf]->imageCaptions());
	  GeometryObjects::colorbars()->setColorBars(m_keyFrameInfo[kf]->colorbars());
	  GeometryObjects::scalebars()->setScaleBars(m_keyFrameInfo[kf]->scalebars());
	  GeometryObjects::hitpoints()->setPoints(m_keyFrameInfo[kf]->points());
	  GeometryObjects::hitpoints()->setBarePoints(m_keyFrameInfo[kf]->barepoints());
	  GeometryObjects::hitpoints()->setPointSize(m_keyFrameInfo[kf]->pointSize());
	  GeometryObjects::hitpoints()->setPointColor(m_keyFrameInfo[kf]->pointColor());
	  GeometryObjects::paths()->setPaths(m_keyFrameInfo[kf]->paths());
	  GeometryObjects::grids()->setGrids(m_keyFrameInfo[kf]->grids());
	  GeometryObjects::crops()->setCrops(m_keyFrameInfo[kf]->crops());
	  GeometryObjects::pathgroups()->setPaths(m_keyFrameInfo[kf]->pathgroups());
	  GeometryObjects::trisets()->set(m_keyFrameInfo[kf]->trisets());
	  GeometryObjects::networks()->set(m_keyFrameInfo[kf]->networks());
	  GeometryObjects::clipplanes()->set(m_keyFrameInfo[kf]->clipInfo());
	  GeometryObjects::landmarks()->setLandmarkInfo(m_keyFrameInfo[kf]->landmarkInfo());

	  LightHandler::setGiLightInfo(m_keyFrameInfo[kf]->giLightInfo());

	  emit updateBrickInfo(m_keyFrameInfo[kf]->brickInfo());
	  emit updateLightInfo(m_keyFrameInfo[kf]->lightInfo());
	  
	  emit updateMorph(m_keyFrameInfo[kf]->morphTF());

	  if (m_keyFrameInfo[kf]->splineInfo().size() == 0 ||
	      Global::replaceTF() == false)
	    {
	      // update lookup tables only
	      if (m_keyFrameInfo[kf]->lut())
		emit updateLookupTable(m_keyFrameInfo[kf]->lut());
	    }
	  else
	    {
	      // update transfer functions which in turn will update lookup tables
	      emit updateTransferFunctionManager(m_keyFrameInfo[kf]->splineInfo());
	    }

	  Global::setTagColors(m_keyFrameInfo[kf]->tagColors());
	  emit updateTagColors();

	  bool drawBox = m_keyFrameInfo[kf]->drawBox();
	  bool drawAxis = m_keyFrameInfo[kf]->drawAxis();
	  Vec backgroundColor = m_keyFrameInfo[kf]->backgroundColor();
	  QString backgroundImage = m_keyFrameInfo[kf]->backgroundImageFile();
	  int sz, st;
	  QString xl, yl, zl;
	  m_keyFrameInfo[kf]->getTick(sz, st, xl, yl, zl);
	  int mv;
	  bool mc, mo, mt;
	  m_keyFrameInfo[kf]->getMix(mv, mc, mo, mt);

	  float fop, bop;
	  m_keyFrameInfo[kf]->getOpMod(fop, bop);

	  emit updateParameters(drawBox, drawAxis,
				backgroundColor,
				backgroundImage,
				sz, st, xl, yl, zl,
				mv, mc, mo, 0.0f, mt,
				m_keyFrameInfo[kf]->pruneBlend(),
				fop, bop);

	  QByteArray pb = m_keyFrameInfo[kf]->pruneBuffer();
	  if (! pb.isEmpty())
	    PruneHandler::setPruneBuffer(pb);

	  Global::enableViewerUpdate();
	  MainWindowUI::changeDrishtiIcon(true);
	  Global::setPlayFrames(true);

	  emit updateLightBuffers();

	  emit updateGL();
	  qApp->processEvents();
  
	  return;
	}
    }


  float frc = 0;
  int i = 0;
  for(int kf=1; kf<numberOfKeyFrames(); kf++)
    {
      if (fno <= m_keyFrameInfo[kf]->frameNumber())
	{
	  i = kf-1;

	  if ( m_keyFrameInfo[i+1]->frameNumber() >
	       m_keyFrameInfo[i]->frameNumber())
	    frc = ((float)(fno-m_keyFrameInfo[i]->frameNumber()) /
		   (float)(m_keyFrameInfo[i+1]->frameNumber() -
			   m_keyFrameInfo[i]->frameNumber()));
	  else
	    frc = 1;

	  break;
	}
    }

  Global::disableViewerUpdate();
  MainWindowUI::changeDrishtiIcon(false);

  Vec pos;
  Quaternion rot;
  KeyFrameInformation keyFrameInfo;
  float volInterp;
  interpolateAt(i, frc,
		pos, rot,
		keyFrameInfo,
		volInterp);

  float focusDistance = keyFrameInfo.focusDistance();
  float es = keyFrameInfo.eyeSeparation();
  emit updateFocus(focusDistance, es);
  emit updateLookFrom(pos, rot, focusDistance, es);

  Vec bmin, bmax;
  keyFrameInfo.volumeBounds(bmin, bmax);

  emit updateVolumeBounds(bmin, bmax);

  if (Global::volumeType() == Global::SingleVolume)
    {
      int volnum = keyFrameInfo.volumeNumber();
      emit updateVolInfo(volnum);
      emit updateVolumeBounds(volnum, bmin, bmax);
    }
  else if (Global::volumeType() == Global::DoubleVolume)
    {
      int volnum1 = keyFrameInfo.volumeNumber();
      int volnum2 = keyFrameInfo.volumeNumber2();
      emit updateVolInfo(volnum1);
      emit updateVolInfo(1, volnum2);
      emit updateVolumeBounds(volnum1, volnum2, bmin, bmax);
    }
  else if (Global::volumeType() == Global::TripleVolume)
    {
      int volnum1 = keyFrameInfo.volumeNumber();
      int volnum2 = keyFrameInfo.volumeNumber2();
      int volnum3 = keyFrameInfo.volumeNumber3();
      emit updateVolInfo(volnum1);
      emit updateVolInfo(1, volnum2);
      emit updateVolInfo(2, volnum3);
      emit updateVolumeBounds(volnum1, volnum2, volnum3, bmin, bmax);
    }
  else if (Global::volumeType() == Global::QuadVolume)
    {
      int volnum1 = keyFrameInfo.volumeNumber();
      int volnum2 = keyFrameInfo.volumeNumber2();
      int volnum3 = keyFrameInfo.volumeNumber3();
      int volnum4 = keyFrameInfo.volumeNumber4();
      emit updateVolInfo(volnum1);
      emit updateVolInfo(1, volnum2);
      emit updateVolInfo(2, volnum3);
      emit updateVolInfo(3, volnum4);
      emit updateVolumeBounds(volnum1, volnum2, volnum3, volnum4, bmin, bmax);
    }

  GeometryObjects::captions()->setCaptions(keyFrameInfo.captions());
  GeometryObjects::imageCaptions()->setImageCaptions(keyFrameInfo.imageCaptions());
  GeometryObjects::colorbars()->setColorBars(keyFrameInfo.colorbars());
  GeometryObjects::scalebars()->setScaleBars(keyFrameInfo.scalebars());
  GeometryObjects::hitpoints()->setPoints(keyFrameInfo.points());
  GeometryObjects::hitpoints()->setBarePoints(keyFrameInfo.barepoints());
  GeometryObjects::hitpoints()->setPointSize(keyFrameInfo.pointSize());
  GeometryObjects::hitpoints()->setPointColor(keyFrameInfo.pointColor());
  GeometryObjects::paths()->setPaths(keyFrameInfo.paths());
  GeometryObjects::grids()->setGrids(keyFrameInfo.grids());
  GeometryObjects::crops()->setCrops(keyFrameInfo.crops());
  GeometryObjects::pathgroups()->setPaths(keyFrameInfo.pathgroups());
  GeometryObjects::trisets()->set(keyFrameInfo.trisets());
  GeometryObjects::networks()->set(keyFrameInfo.networks());
  GeometryObjects::clipplanes()->set(keyFrameInfo.clipInfo());
  GeometryObjects::landmarks()->setLandmarkInfo(keyFrameInfo.landmarkInfo());

  LightHandler::setGiLightInfo(keyFrameInfo.giLightInfo());

  emit updateLightInfo(keyFrameInfo.lightInfo());
  emit updateBrickInfo(keyFrameInfo.brickInfo());

  emit updateMorph(keyFrameInfo.morphTF());

  if (keyFrameInfo.morphTF() == false ||
      keyFrameInfo.splineInfo().size()==0)
    {
      // update lookup tables only
      if (keyFrameInfo.lut())
	emit updateLookupTable(keyFrameInfo.lut());
    }
  else
    {
      // update transfer functions which in turn will update lookup tables
      emit updateTransferFunctionManager(keyFrameInfo.splineInfo());
    }

  Global::setTagColors(keyFrameInfo.tagColors());
  emit updateTagColors();

  bool drawBox = keyFrameInfo.drawBox();
  bool drawAxis = keyFrameInfo.drawAxis();
  Vec backgroundColor = keyFrameInfo.backgroundColor();
  QString backgroundImage = keyFrameInfo.backgroundImageFile();
  int sz, st;
  QString xl, yl, zl;
  keyFrameInfo.getTick(sz, st, xl, yl, zl);
  int mv;
  bool mc, mo, mt;
  keyFrameInfo.getMix(mv, mc, mo, mt);

  float fop, bop;
  keyFrameInfo.getOpMod(fop, bop);

  emit updateParameters(drawBox, drawAxis,
			backgroundColor,
			backgroundImage,
			sz, st, xl, yl, zl,
			mv, mc, mo, volInterp, mt,
			keyFrameInfo.pruneBlend(),
			fop, bop);

  QByteArray pb = keyFrameInfo.pruneBuffer();
  if (! pb.isEmpty())
    PruneHandler::setPruneBuffer(pb);

  Global::enableViewerUpdate();
  MainWindowUI::changeDrishtiIcon(true);
  Global::setPlayFrames(true);

  emit updateLightBuffers();

  emit updateGL();
  qApp->processEvents();
}

//--------------------------------
//---- load and save -------------
//--------------------------------
void
KeyFrame::load(fstream &fin)
{
  char keyword[100];

  clear();

  int n;
  fin.read((char*)&n, sizeof(int));

  bool savedFirst = false;
  while (!fin.eof())
    {
      fin.getline(keyword, 100, 0);

      if (strcmp(keyword, "done") == 0)
	break;

      if (strcmp(keyword, "keyframestart") == 0)
	{
	  // the zeroeth keyframe should be moved to m_savedKeyFrame
	  if (!savedFirst)
	    {
	      m_savedKeyFrame.load(fin);
	      savedFirst = true;
	    }
	  else
	    {
	      KeyFrameInformation *kfi = new KeyFrameInformation();
	      kfi->load(fin);
	      m_keyFrameInfo.append(kfi);
	    }
	}
    }

  // read all keyframes.  now generate the camerapath information
  for (int i=0; i<m_keyFrameInfo.size(); i++)
    {
      Vec pos = m_keyFrameInfo[i]->position();
      Quaternion rot = m_keyFrameInfo[i]->orientation();

      CameraPathNode *cam = new CameraPathNode(pos, rot);      
      m_cameraList.append(cam);
      connect(cam, SIGNAL(modified()),
	      this, SLOT(updateKeyFrameInterpolator()));
    }

  updateCameraPath();


  // --- build list for keyframe editor
  QList<int> framenumbers;
  QList<QImage> images;
  for (int i=0; i<m_keyFrameInfo.size(); i++)
    {
      framenumbers.append(m_keyFrameInfo[i]->frameNumber());
      images.append(m_keyFrameInfo[i]->image());
    }

  emit loadKeyframes(framenumbers, images);
  qApp->processEvents();


  //playSavedKeyFrame();
  // ----------------------------------
}

void
KeyFrame::save(fstream& fout)
{
  char keyword[100];

  memset(keyword, 0, 100);
  sprintf(keyword, "keyframes");
  fout.write((char*)keyword, strlen(keyword)+1);

  int n = numberOfKeyFrames();
  fout.write((char*)&n, sizeof(int));

  m_savedKeyFrame.save(fout);

  for(int kf=0; kf<numberOfKeyFrames(); kf++)
    m_keyFrameInfo[kf]->save(fout);

  sprintf(keyword, "done");
  fout.write((char*)keyword, strlen(keyword)+1);
}


void
KeyFrame::computeTangents()
{
  int nkf = numberOfKeyFrames();
  if (nkf <= 1)
    return;

  // -- flip orientation if necessary --
  Quaternion prevQ = m_keyFrameInfo[0]->orientation();
  for(int kf=1; kf<nkf; kf++)
    {
      Quaternion currQ = m_keyFrameInfo[kf]->orientation();
      if (Quaternion::dot(prevQ, currQ) < 0.0)
	{
	  currQ.negate();
	  m_keyFrameInfo[kf]->setOrientation(currQ);
	}
      prevQ = currQ;
    }
  // ------------------------------------

  m_tgP.clear();
  m_tgQ.clear();
  Vec prevP, nextP, currP;
  Quaternion nextQ, currQ;
  
  currP = m_keyFrameInfo[0]->position();
  currQ = m_keyFrameInfo[0]->orientation();
  prevP = currP;
  prevQ = currQ;
  for(int kf=0; kf<nkf; kf++)
    {
      if (kf < nkf-1)
	{
	  nextP = m_keyFrameInfo[kf+1]->position();
	  nextQ = m_keyFrameInfo[kf+1]->orientation();
	}
      Vec tgP = 0.5*(nextP - prevP);
      Quaternion tgQ = Quaternion::squadTangent(prevQ, currQ, nextQ);

      m_tgP.append(tgP);
      m_tgQ.append(tgQ);


      prevP = currP;
      prevQ = currQ;
      currP = nextP;
      currQ = nextQ;
    }
}

Vec
KeyFrame::interpolatePosition(int kf1, int kf2, float frc)
{
  Vec pos = m_keyFrameInfo[kf1]->position();

  Vec diff = m_keyFrameInfo[kf2]->position() -
	     m_keyFrameInfo[kf1]->position();

  float len = diff.squaredNorm();
  if (len > 0.1)
    {
      if (Global::interpolationType(Global::CameraPositionInterpolation))
	{ // spline interpolation of position
	  Vec v1 = 3*diff - 2*m_tgP[kf1] - m_tgP[kf2];
	  Vec v2 = -2*diff + m_tgP[kf1] + m_tgP[kf2];
	  
	  pos += frc*(m_tgP[kf1] + frc*(v1+frc*v2));
	}
      else // linear interpolation of position
	pos += frc*diff;
    }

  return pos;
}

Quaternion
KeyFrame::interpolateOrientation(int kf1, int kf2, float frc)
{
  Quaternion q1 = m_keyFrameInfo[kf1]->orientation();
  Quaternion q2 = m_keyFrameInfo[kf2]->orientation();

  double a0 = q1[0];
  double a1 = q1[1];
  double a2 = q1[2];
  double a3 = q1[3];

  double b0 = q2[0];
  double b1 = q2[1];
  double b2 = q2[2];
  double b3 = q2[3];

  if (fabs(a0-b0) > 0.0001 ||
      fabs(a1-b1) > 0.0001 ||
      fabs(a2-b2) > 0.0001 ||
      fabs(a3-b3) > 0.0001)
    {
      Quaternion q;
      q = Quaternion::squad(q1,
			    m_tgQ[kf1],
			    m_tgQ[kf2],
			    q2,
			    frc);
      return q;
    }
  else
    return q1;
}

void
KeyFrame::copyFrame(int kfn)
{
  if (kfn < m_keyFrameInfo.count())
    m_copyKeyFrame = *m_keyFrameInfo[kfn];
  else
    QMessageBox::information(0, "Error",
			     QString("KeyFrame %1 not present for copying").arg(kfn));
}
void
KeyFrame::pasteFrame(int frameNumber)
{
  CameraPathNode *cam;
  KeyFrameInformation *kfi;

  cam = new CameraPathNode(m_copyKeyFrame.position(),
			   m_copyKeyFrame.orientation());
  m_cameraList.append(cam);
  connect(cam, SIGNAL(modified()),
	  this, SLOT(updateKeyFrameInterpolator()));

  kfi = new KeyFrameInformation();
  *kfi = m_copyKeyFrame;
  kfi->setFrameNumber(frameNumber);
  m_keyFrameInfo.append(kfi);

  updateKeyFrameInterpolator();
}

void
KeyFrame::editFrameInterpolation(int kfn)
{
  KeyFrameInformation *kfi = m_keyFrameInfo[kfn];

  PropertyEditor propertyEditor;
  QMap<QString, QVariantList> plist;
  
  QStringList keys;
  keys << "title";
  keys << "background color";
  keys << "captions";
  keys << "focus";
  keys << "tag colors";
  keys << "tick information";
  keys << "volume bounds";
  keys << "separator";
  keys << "camera position";
  keys << "camera orientation";
  keys << "separator";
  keys << "brick information";
  keys << "clip planes";
  keys << "lighting";
  keys << "gi lighting";
  keys << "transfer functions";
  keys << "crop/dissect/blend/displace";
  keys << "mop";

  for(int ik=0; ik<keys.count(); ik++)
    {
      if (keys[ik] != "separator")
	{
	  QVariantList vlist;  
	  vlist.clear();
	  if (keys[ik] == "title")
	    {
	      vlist << QVariant("string");
	      vlist << kfi->title();
	    }
	  else
	    {
	      vlist << QVariant("combobox");

	      if (keys[ik] == "background color") vlist << kfi->interpBGColor();
	      else if (keys[ik] == "captions") vlist << kfi->interpCaptions();
	      else if (keys[ik] == "focus") vlist << kfi->interpFocus();
	      else if (keys[ik] == "tag colors") vlist << kfi->interpTagColors();
	      else if (keys[ik] == "tick information") vlist << kfi->interpTickInfo();
	      else if (keys[ik] == "volume bounds") vlist << kfi->interpVolumeBounds();
	      else if (keys[ik] == "camera position") vlist << kfi->interpCameraPosition();
	      else if (keys[ik] == "camera orientation") vlist << kfi->interpCameraOrientation();
	      else if (keys[ik] == "brick information") vlist << kfi->interpBrickInfo();
	      else if (keys[ik] == "clip planes") vlist << kfi->interpClipInfo();
	      else if (keys[ik] == "lighting") vlist << kfi->interpLightInfo();
	      else if (keys[ik] == "gi lighting") vlist << kfi->interpGiLightInfo();
	      else if (keys[ik] == "transfer functions") vlist << kfi->interpTF();
	      else if (keys[ik] == "crop/dissect/blend/displace") vlist << kfi->interpCrop();
	      else if (keys[ik] == "mop") vlist << kfi->interpMop();
	      
	      vlist << QVariant("linear");
	      vlist << QVariant("smoothstep");
	      vlist << QVariant("easein");
	      vlist << QVariant("easeout");
	      vlist << QVariant("none");
	    }

	  plist[keys[ik]] = vlist;
	}
    }

  propertyEditor.set("Keyframe Interpolation Parameters",
		     plist, keys,
		     false); // do not add reset buttons
  propertyEditor.resize(300, 500);
	      
  QMap<QString, QPair<QVariant, bool> > vmap;

  if (propertyEditor.exec() == QDialog::Accepted)
    vmap = propertyEditor.get();
  else
    {
//      QMessageBox::information(0,
//			       "Keyframe Interpolation Parameters",
//			       "No interpolation parameter changed");
      return;
    }

  keys = vmap.keys();
  
  for(int ik=0; ik<keys.count(); ik++)
    {
      QPair<QVariant, bool> pair = vmap.value(keys[ik]);
      
      
      if (pair.second)
	{
	  if (keys[ik] == "title")
	    {
	      QString title = pair.first.toString();
	      kfi->setTitle(title);
	    }
	  else
	    {
	      int flag = pair.first.toInt();
	      if (keys[ik] == "background color") kfi->setInterpBGColor(flag);
	      else if (keys[ik] == "captions") kfi->setInterpCaptions(flag);
	      else if (keys[ik] == "focus") kfi->setInterpFocus(flag);
	      else if (keys[ik] == "tag colors") kfi->setInterpTagColors(flag);
	      else if (keys[ik] == "tick information") kfi->setInterpTickInfo(flag);
	      else if (keys[ik] == "volume bounds") kfi->setInterpVolumeBounds(flag);
	      else if (keys[ik] == "camera position") kfi->setInterpCameraPosition(flag);
	      else if (keys[ik] == "camera orientation") kfi->setInterpCameraOrientation(flag);
	      else if (keys[ik] == "brick information") kfi->setInterpBrickInfo(flag);
	      else if (keys[ik] == "clip planes") kfi->setInterpClipInfo(flag);
	      else if (keys[ik] == "lighting") kfi->setInterpLightInfo(flag);
	      else if (keys[ik] == "gi lighting") kfi->setInterpGiLightInfo(flag);
	      else if (keys[ik] == "transfer functions") kfi->setInterpTF(flag);
	      else if (keys[ik] == "crop/dissect/blend/displace") kfi->setInterpCrop(flag);
	      else if (keys[ik] == "mop") kfi->setInterpMop(flag);
	    }
	}
    }

//  QMessageBox::information(0, "Parameters", "Changed");
}

void
KeyFrame::pasteFrameOnTop(int keyFrameNumber)
{
  if (keyFrameNumber < 0 ||
      keyFrameNumber >= m_keyFrameInfo.count() ||
      keyFrameNumber >= m_cameraList.count())
    {
      QMessageBox::information(0, "",
	  QString("%1 keyframe does not exist").arg(keyFrameNumber));
      return;
    }
      
  QMap<QString, QPair<QVariant, bool> > vmap;
  vmap = copyProperties(QString("Paste Parameters to keyframe on %1").\
			arg(m_keyFrameInfo[keyFrameNumber]->frameNumber()));

  QStringList keys = vmap.keys();
  if (keys.count() == 0)
    return;
  
	      
  KeyFrameInformation *kfi = m_keyFrameInfo[keyFrameNumber];
  CameraPathNode *cam = m_cameraList[keyFrameNumber];


  for(int ik=0; ik<keys.count(); ik++)
    {
      QPair<QVariant, bool> pair = vmap.value(keys[ik]);      
      
      if (pair.second)
	{
	  if (keys[ik] == "axis")
	    kfi->setDrawAxis(m_copyKeyFrame.drawAxis());  
	  else if (keys[ik] == "background color")
	    kfi->setBackgroundColor(m_copyKeyFrame.backgroundColor());  
	  else if (keys[ik] == "bounding box")
	    kfi->setDrawBox(m_copyKeyFrame.drawBox());  
	  else if (keys[ik] == "brick information")
	    kfi->setBrickInfo(m_copyKeyFrame.brickInfo());  
	  else if (keys[ik] == "camera position")
	    {
	      kfi->setPosition(m_copyKeyFrame.position());
	      cam->setPosition(m_copyKeyFrame.position());
	    }
	  else if (keys[ik] == "camera orientation")
	    {
	      kfi->setOrientation(m_copyKeyFrame.orientation());  
	      cam->setOrientation(m_copyKeyFrame.orientation());  
	    }
	  else if (keys[ik] == "captions")
	    kfi->setCaptions(m_copyKeyFrame.captions());  
	  else if (keys[ik] == "image captions")
	    kfi->setImageCaptions(m_copyKeyFrame.imageCaptions());  
	  else if (keys[ik] == "colorbars")
	    kfi->setColorBars(m_copyKeyFrame.colorbars());  
	  else if (keys[ik] == "scalebars")
	    kfi->setScaleBars(m_copyKeyFrame.scalebars());  
	  else if (keys[ik] == "clip planes")
	    kfi->setClipInfo(m_copyKeyFrame.clipInfo());  
	  else if (keys[ik] == "focus")
	    {
	      kfi->setFocusDistance(m_copyKeyFrame.focusDistance(),
				    m_copyKeyFrame.eyeSeparation());  
	    }
	  else if (keys[ik] == "landmarks")
	    kfi->setLandmarkInfo(m_copyKeyFrame.landmarkInfo());  
	  else if (keys[ik] == "lighting")
	    kfi->setLightInfo(m_copyKeyFrame.lightInfo());  
	  else if (keys[ik] == "gi lighting")
	    kfi->setGiLightInfo(m_copyKeyFrame.giLightInfo());  
	  else if (keys[ik] == "paths")
	    {
	      kfi->setPaths(m_copyKeyFrame.paths());  
	      kfi->setPathGroups(m_copyKeyFrame.pathgroups());  
	    }
	  else if (keys[ik] == "grids")
	    kfi->setGrids(m_copyKeyFrame.grids());  
	  else if (keys[ik] == "crop/dissect/blend/displace")
	    kfi->setCrops(m_copyKeyFrame.crops());  
	  else if (keys[ik] == "mop")
	    {
	      kfi->setPruneBuffer(m_copyKeyFrame.pruneBuffer());  
	      kfi->setPruneBlend(m_copyKeyFrame.pruneBlend());  
	    }
	  else if (keys[ik] == "points")
	    {
	      kfi->setPoints(m_copyKeyFrame.points(),
			     m_copyKeyFrame.barepoints(),
			     m_copyKeyFrame.pointSize(),
			     m_copyKeyFrame.pointColor());
	    }
	  else if (keys[ik] == "tag colors")
	    kfi->setTagColors(m_copyKeyFrame.tagColors());  
	  else if (keys[ik] == "tick information")
	    {
	      int t0, t1;
	      QString s0, s1, s2;
	      m_copyKeyFrame.getTick(t0, t1, s0, s1, s2);
	      kfi->setTick(t0, t1, s0, s1, s2); 
	    }
	  else if (keys[ik] == "mix information")
	    {
	      int mv;
	      bool mc, mo, mt;
	      m_copyKeyFrame.getMix(mv, mc, mo, mt);
	      kfi->setMix(mv, mc, mo, mt); 
	    }
	  else if (keys[ik] == "transfer functions")
	    {
	      kfi->setLut(m_copyKeyFrame.lut());  
	      kfi->setSplineInfo(m_copyKeyFrame.splineInfo());  
	      kfi->setMorphTF(m_copyKeyFrame.morphTF());  
	    }
	  else if (keys[ik] == "trisets")
	    kfi->setTrisets(m_copyKeyFrame.trisets());  
	  else if (keys[ik] == "networks")
	    kfi->setNetworks(m_copyKeyFrame.networks());  
	  else if (keys[ik] == "volume bounds")
	    {
	      Vec bmin, bmax;
	      m_copyKeyFrame.volumeBounds(bmin, bmax);
	      kfi->setVolumeBounds(bmin, bmax);
	    }
	}
    }

  updateKeyFrameInterpolator();

  playFrameNumber(kfi->frameNumber());

  emit replaceKeyFrameImage(keyFrameNumber);
}

void
KeyFrame::pasteFrameOnTop(int startKF, int endKF)
{
  QMap<QString, QPair<QVariant, bool> > vmap;
  vmap = copyProperties(QString("Paste Parameters to keyframes between %1 - %2").\
			arg(m_keyFrameInfo[startKF]->frameNumber()).\
			arg(m_keyFrameInfo[endKF]->frameNumber()));

  QStringList keys = vmap.keys();
  if (keys.count() == 0)
    return;
  

  for(int keyFrameNumber=startKF; keyFrameNumber<=endKF; keyFrameNumber++)
    {	      
      KeyFrameInformation *kfi = m_keyFrameInfo[keyFrameNumber];
      CameraPathNode *cam = m_cameraList[keyFrameNumber];

      for(int ik=0; ik<keys.count(); ik++)
	{
	  QPair<QVariant, bool> pair = vmap.value(keys[ik]);
            
	  if (pair.second)
	    {
	      if (keys[ik] == "axis")
		kfi->setDrawAxis(m_copyKeyFrame.drawAxis());  
	      else if (keys[ik] == "background color")
		kfi->setBackgroundColor(m_copyKeyFrame.backgroundColor());  
	      else if (keys[ik] == "bounding box")
		kfi->setDrawBox(m_copyKeyFrame.drawBox());  
	      else if (keys[ik] == "brick information")
		kfi->setBrickInfo(m_copyKeyFrame.brickInfo());  
	      else if (keys[ik] == "camera position")
		{
		  kfi->setPosition(m_copyKeyFrame.position());
		  cam->setPosition(m_copyKeyFrame.position());
		}
	      else if (keys[ik] == "camera orientation")
		{
		  kfi->setOrientation(m_copyKeyFrame.orientation());  
		  cam->setOrientation(m_copyKeyFrame.orientation());  
		}
	      else if (keys[ik] == "captions")
		kfi->setCaptions(m_copyKeyFrame.captions());  
	      else if (keys[ik] == "image captions")
		kfi->setImageCaptions(m_copyKeyFrame.imageCaptions());  
	      else if (keys[ik] == "colorbars")
		kfi->setColorBars(m_copyKeyFrame.colorbars());  
	      else if (keys[ik] == "scalebars")
		kfi->setScaleBars(m_copyKeyFrame.scalebars());  
	      else if (keys[ik] == "clip planes")
		kfi->setClipInfo(m_copyKeyFrame.clipInfo());  
	      else if (keys[ik] == "focus")
		{
		  kfi->setFocusDistance(m_copyKeyFrame.focusDistance(),
					m_copyKeyFrame.eyeSeparation());  
		}
	      else if (keys[ik] == "landmarks")
		kfi->setLandmarkInfo(m_copyKeyFrame.landmarkInfo());  
	      else if (keys[ik] == "lighting")
		kfi->setLightInfo(m_copyKeyFrame.lightInfo());  
	      else if (keys[ik] == "gi lighting")
		kfi->setGiLightInfo(m_copyKeyFrame.giLightInfo());  
	      else if (keys[ik] == "paths")
		{
		  kfi->setPaths(m_copyKeyFrame.paths());  
		  kfi->setPathGroups(m_copyKeyFrame.pathgroups());  
		}
	      else if (keys[ik] == "grids")
		kfi->setGrids(m_copyKeyFrame.grids());  
	      else if (keys[ik] == "crop/dissect/blend/displace")
		kfi->setCrops(m_copyKeyFrame.crops());  
	      else if (keys[ik] == "mop")
		{
		  kfi->setPruneBuffer(m_copyKeyFrame.pruneBuffer());  
		  kfi->setPruneBlend(m_copyKeyFrame.pruneBlend());  
		}
	      else if (keys[ik] == "points")
		{
		  kfi->setPoints(m_copyKeyFrame.points(),
				 m_copyKeyFrame.barepoints(),
				 m_copyKeyFrame.pointSize(),
				 m_copyKeyFrame.pointColor());
		}
	      else if (keys[ik] == "tag colors")
		kfi->setTagColors(m_copyKeyFrame.tagColors());  
	      else if (keys[ik] == "tick information")
		{
		  int t0, t1;
		  QString s0, s1, s2;
		  m_copyKeyFrame.getTick(t0, t1, s0, s1, s2);
		  kfi->setTick(t0, t1, s0, s1, s2); 
		}
	      else if (keys[ik] == "mix information")
		{
		  int mv;
		  bool mc, mo, mt;
		  m_copyKeyFrame.getMix(mv, mc, mo, mt);
		  kfi->setMix(mv, mc, mo, mt); 
		}
	      else if (keys[ik] == "transfer functions")
		{
		  kfi->setLut(m_copyKeyFrame.lut());  
		  kfi->setSplineInfo(m_copyKeyFrame.splineInfo());  
		  kfi->setMorphTF(m_copyKeyFrame.morphTF());  
		}
	      else if (keys[ik] == "trisets")
		kfi->setTrisets(m_copyKeyFrame.trisets());  
	      else if (keys[ik] == "networks")
		kfi->setNetworks(m_copyKeyFrame.networks());  
	      else if (keys[ik] == "volume bounds")
		{
		  Vec bmin, bmax;
		  m_copyKeyFrame.volumeBounds(bmin, bmax);
		  kfi->setVolumeBounds(bmin, bmax);
		}
	    }
	}
      
      updateKeyFrameInterpolator();
      
      playFrameNumber(kfi->frameNumber());
      
      emit replaceKeyFrameImage(keyFrameNumber);
      qApp->processEvents();
    }
}

QMap<QString, QPair<QVariant, bool> >
KeyFrame::copyProperties(QString title)
{
  PropertyEditor propertyEditor;
  QMap<QString, QVariantList> plist;
  
  QVariantList vlist;

  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(false);
  plist["SELECT ALL"] = vlist;

  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(false);
  plist["axis"] = vlist;

  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(false);
  plist["background color"] = vlist;

  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(false);
  plist["bounding box"] = vlist;
  
  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(false);
  plist["brick information"] = vlist;

  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(false);
  plist["camera position"] = vlist;

  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(false);
  plist["camera orientation"] = vlist;

  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(false);
  plist["captions"] = vlist;

  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(false);
  plist["image captions"] = vlist;

  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(false);
  plist["colorbars"] = vlist;

  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(false);
  plist["scalebars"] = vlist;

  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(false);
  plist["clip planes"] = vlist;

  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(false);
  plist["focus"] = vlist;

  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(false);
  plist["landmarks"] = vlist;

  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(false);
  plist["lighting"] = vlist;

  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(false);
  plist["gi lighting"] = vlist;

  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(false);
  plist["paths"] = vlist;

  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(false);
  plist["grids"] = vlist;

  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(false);
  plist["crop/dissect/blend/displace"] = vlist;

  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(false);
  plist["mop"] = vlist;

  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(false);
  plist["points"] = vlist;

  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(false);
  plist["tag colors"] = vlist;

  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(false);
  plist["tick information"] = vlist;

  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(false);
  plist["mix information"] = vlist;

  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(false);
  plist["transfer functions"] = vlist;

  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(false);
  plist["trisets"] = vlist;

  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(false);
  plist["networks"] = vlist;

  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(false);
  plist["volume bounds"] = vlist;



  QStringList keys;
  keys << "SELECT ALL";
  keys << "axis";
  keys << "background color";
  keys << "bounding box";
  keys << "captions";
  keys << "image captions";
  keys << "colorbars";
  keys << "scalebars";
  keys << "focus";
  keys << "tag colors";
  keys << "tick information";
  keys << "volume bounds";
  keys << "separator";
  keys << "camera position";
  keys << "camera orientation";
  keys << "separator";
  keys << "brick information";
  keys << "clip planes";
  keys << "landmarks";
  keys << "lighting";
  keys << "gi lighting";
  keys << "transfer functions";
  keys << "separator";
  keys << "paths";
  keys << "grids";
  keys << "points";
  keys << "trisets";
  keys << "networks";
  keys << "separator";
  keys << "crop/dissect/blend/displace";
  keys << "mop";
  keys << "mix information";


  propertyEditor.set(title,
		     plist, keys,
		     false); // do not add reset buttons
  propertyEditor.resize(300, 500);

	      
  QMap<QString, QPair<QVariant, bool> > vmap;

  if (propertyEditor.exec() == QDialog::Accepted)
    vmap = propertyEditor.get();
  else
    {
      QMessageBox::information(0,
			       title,
			       "No parameters pasted");
    }

  return vmap;
}



void
KeyFrame::replaceKeyFrameImage(int kfn, QImage img)
{
  m_keyFrameInfo[kfn]->setImage(img);
}

void
KeyFrame::import(QString flnm)
{
  //--------------------------------
  if (m_keyFrameInfo.count() == 0)
    {
      QMessageBox::information(0,
			       "Import KeyFrames",
			       "Need atleast one keyframe in the keyframe editor before import can take place");
      return;
    }
  //--------------------------------


  //--------------------------------
  fstream fin(flnm.toLatin1().data(), ios::binary|ios::in);

  char keyword[100];
  fin.getline(keyword, 100, 0);
  if (strcmp(keyword, "Drishti Keyframes") != 0)
    {
      QMessageBox::information(0, "Import Keyframes",
			       QString("Invalid .keyframes file : ")+flnm);
      return;
    }

  // search for keyframes
  bool flag = false;
  while (!fin.eof())
    {
      fin.getline(keyword, 100, 0);
      if (strcmp(keyword, "keyframes") == 0)
	{
	  flag = true;
	  break;
	}
    }
  if (!flag)
    {
      QMessageBox::information(0, "Import Keyframes", "No keyframes found in the file !");
      return;
    }
  //--------------------------------

  
  //----------------------------------------
  //-- read all the keyframe data
  QList<CameraPathNode*> cameraList;
  QList<KeyFrameInformation*> keyFrameInfo;

  int n;
  fin.read((char*)&n, sizeof(int));

  bool savedFirst = false;
  while (!fin.eof())
    {
      fin.getline(keyword, 100, 0);

      if (strcmp(keyword, "done") == 0)
	break;

      if (strcmp(keyword, "keyframestart") == 0)
	{
	  // the zeroeth keyframe should be moved to m_savedKeyFrame
	  if (!savedFirst)
	    {
	      KeyFrameInformation dummyKeyFrame;
	      dummyKeyFrame.load(fin);
	      savedFirst = true;
	    }
	  else
	    {
	      KeyFrameInformation *kfi = new KeyFrameInformation();
	      kfi->load(fin);
	      keyFrameInfo.append(kfi);
	    }
	}
    }
  fin.close();
  //----------------------------------------

  
  //----------------------------------------
  //-- start importing selected parameters
  QMap<QString, QPair<QVariant, bool> > vmap;
  vmap = copyProperties("Import Parameters");

  QStringList keys = vmap.keys();
  if (keys.count() == 0)
    return;
  
  int lastfno = 1;
  if (m_keyFrameInfo.count() > 0)
    {
      lastfno = m_keyFrameInfo[m_keyFrameInfo.count()-1]->frameNumber();      
      lastfno += 10;
    }

  QList<int> fnos;
  for (int i=0; i<keyFrameInfo.size(); i++)
    {
      if (i > 0)
	lastfno += (keyFrameInfo[i]->frameNumber()-keyFrameInfo[i-1]->frameNumber());

      // --- add a new KeyFrameInformation node
      KeyFrameInformation *kfi = new KeyFrameInformation(m_savedKeyFrame);
      kfi->setFrameNumber(lastfno);
      fnos.append(lastfno);
      m_keyFrameInfo.append(kfi);

      // --- add a new CameraPathNode
      Vec pos = kfi->position();
      Quaternion rot = kfi->orientation();

      CameraPathNode *cam = new CameraPathNode(pos, rot);      
      m_cameraList.append(cam);
      connect(cam, SIGNAL(modified()),
	      this, SLOT(updateKeyFrameInterpolator()));

      // -- now import relevant information
      KeyFrameInformation *ckf = keyFrameInfo[i];

      for(int ik=0; ik<keys.count(); ik++)
	{
	  QPair<QVariant, bool> pair = vmap.value(keys[ik]);
	  
	  if (pair.second)
	    {
	      if (keys[ik] == "axis")
		kfi->setDrawAxis(ckf->drawAxis());  
	      else if (keys[ik] == "background color")
		{
		  kfi->setBackgroundColor(ckf->backgroundColor());
		  kfi->setInterpBGColor(ckf->interpBGColor());
		}
	      else if (keys[ik] == "bounding box")
		kfi->setDrawBox(ckf->drawBox());  
	      else if (keys[ik] == "brick information")
		{
		  kfi->setBrickInfo(ckf->brickInfo());  
		  kfi->setInterpBrickInfo(ckf->interpBrickInfo());
		}
	      else if (keys[ik] == "camera position")
		{
		  kfi->setPosition(ckf->position());
		  cam->setPosition(ckf->position());
		  kfi->setInterpCameraPosition(ckf->interpCameraPosition());
		}
	      else if (keys[ik] == "camera orientation")
		{
		  kfi->setOrientation(ckf->orientation());  
		  cam->setOrientation(ckf->orientation());  
		  kfi->setInterpCameraOrientation(ckf->interpCameraOrientation());
		}
	      else if (keys[ik] == "captions")
		{
		  kfi->setCaptions(ckf->captions());  
		  kfi->setInterpCaptions(ckf->interpCaptions());
		}
	      else if (keys[ik] == "image captions")
		{
		  kfi->setImageCaptions(ckf->imageCaptions());  
		}
	      else if (keys[ik] == "colorbars")
		{
		  kfi->setColorBars(ckf->colorbars());  
		}
	      else if (keys[ik] == "scalebars")
		{
		  kfi->setScaleBars(ckf->scalebars());  
		}
	      else if (keys[ik] == "clip planes")
		{
		  kfi->setClipInfo(ckf->clipInfo());  
		  kfi->setInterpClipInfo(ckf->interpClipInfo());
		}
	      else if (keys[ik] == "focus")
		{
		  kfi->setFocusDistance(ckf->focusDistance(),
					ckf->eyeSeparation());  
		  kfi->setInterpFocus(ckf->interpFocus());
		}
	      else if (keys[ik] == "landmarks")
		{
		  kfi->setLandmarkInfo(ckf->landmarkInfo());  
		}
	      else if (keys[ik] == "lighting")
		{
		  kfi->setLightInfo(ckf->lightInfo());  
		  kfi->setInterpLightInfo(ckf->interpLightInfo());
		}
	      else if (keys[ik] == "gi lighting")
		{
		  kfi->setGiLightInfo(ckf->giLightInfo());  
		  kfi->setInterpGiLightInfo(ckf->interpGiLightInfo());
		}
	      else if (keys[ik] == "paths")
		{
		  kfi->setPaths(ckf->paths());  
		  kfi->setPathGroups(ckf->pathgroups());  
		}
	      else if (keys[ik] == "grids")
		kfi->setGrids(ckf->grids());  
	      else if (keys[ik] == "points")
		{
		  kfi->setPoints(ckf->points(),
				 ckf->barepoints(),
				 ckf->pointSize(),
				 ckf->pointColor());
		}
	      else if (keys[ik] == "tag colors")
		kfi->setTagColors(ckf->tagColors());  
	      else if (keys[ik] == "tick information")
		{
		  int t0, t1;
		  QString s0, s1, s2;
		  ckf->getTick(t0, t1, s0, s1, s2);
		  kfi->setTick(t0, t1, s0, s1, s2); 
		  kfi->setInterpTagColors(ckf->interpTagColors());
		}
	      else if (keys[ik] == "mix information")
		{
		  int mv;
		  bool mc, mo, mt;
		  ckf->getMix(mv, mc, mo, mt);
		  kfi->setMix(mv, mc, mo, mt); 
		}
	      else if (keys[ik] == "transfer functions")
		{
		  kfi->setLut(ckf->lut());  
		  kfi->setSplineInfo(ckf->splineInfo());  
		  kfi->setMorphTF(ckf->morphTF());  
		  kfi->setInterpTF(ckf->interpTF());
		}
	      else if (keys[ik] == "trisets")
		kfi->setTrisets(ckf->trisets());  
	      else if (keys[ik] == "networks")
		kfi->setNetworks(ckf->networks());  
	      else if (keys[ik] == "volume bounds")
		{
		  Vec bmin, bmax;
		  ckf->volumeBounds(bmin, bmax);
		  kfi->setVolumeBounds(bmin, bmax);
		  kfi->setInterpVolumeBounds(ckf->interpVolumeBounds());
		}
	      else if (keys[ik] == "crop/dissect/blend/displace")
		{
		  kfi->setCrops(ckf->crops());  
		  kfi->setInterpCrop(ckf->interpCrop());
		}
	      else if (keys[ik] == "mop")
		{
		  kfi->setPruneBuffer(ckf->pruneBuffer());  
		  kfi->setPruneBlend(ckf->pruneBlend());  
		  kfi->setInterpMop(ckf->interpMop());
		}
	    }
	}
    }

  for(int i=0; i<keyFrameInfo.size(); i++)
    delete keyFrameInfo[i];
  keyFrameInfo.clear();

  emit addKeyFrameNumbers(fnos);

  updateCameraPath();

  for(int i=0; i<m_keyFrameInfo.count(); i++)
    {
      playFrameNumber(m_keyFrameInfo[i]->frameNumber());

      emit replaceKeyFrameImage(i);

      qApp->processEvents();
    }
}
