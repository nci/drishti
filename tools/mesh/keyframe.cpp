#include "keyframe.h"
#include "global.h"
#include "geometryobjects.h"
#include "propertyeditor.h"
#include "staticfunctions.h"
#include "enums.h"
#include "mainwindowui.h"

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
KeyFrame::checkKeyFrameNumbers()
{
  // check overlapping keyframes
  QString mesg;
  for(int i=0; i<m_keyFrameInfo.count()-1; i++)
    for(int j=i+1; j<m_keyFrameInfo.count(); j++)
      {
	if (m_keyFrameInfo[i]->frameNumber() ==
	    m_keyFrameInfo[j]->frameNumber())
	  mesg += QString("Clash of keyframes at : %1").\
	          arg(m_keyFrameInfo[i]->frameNumber());
      }
  if (!mesg.isEmpty())
    QMessageBox::information(0, "Keyframe clash", mesg);
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
		      LightingInformation lightInfo,
		      QList<BrickInformation> brickInfo,
		      QImage image)
{
  m_savedKeyFrame.clear();
  m_savedKeyFrame.setFrameNumber(-1);
  m_savedKeyFrame.setDrawBox(Global::drawBox());
  m_savedKeyFrame.setDrawAxis(Global::drawAxis());
  m_savedKeyFrame.setShadowBox(Global::shadowBox());
  m_savedKeyFrame.setBackgroundColor(Global::backgroundColor());
  m_savedKeyFrame.setBackgroundImageFile(Global::backgroundImageFile());
  m_savedKeyFrame.setPosition(pos);
  m_savedKeyFrame.setOrientation(rot);
  m_savedKeyFrame.setLightInfo(lightInfo);
  m_savedKeyFrame.setClipInfo(GeometryObjects::clipplanes()->clipInfo());
  m_savedKeyFrame.setBrickInfo(brickInfo);
  m_savedKeyFrame.setImage(image);
  m_savedKeyFrame.setCaptions(GeometryObjects::captions()->captions());
  m_savedKeyFrame.setScaleBars(GeometryObjects::scalebars()->scalebars());
  m_savedKeyFrame.setPoints(GeometryObjects::hitpoints()->points(),
			    GeometryObjects::hitpoints()->barePoints(),
			    GeometryObjects::hitpoints()->pointSize(),
			    GeometryObjects::hitpoints()->pointColor());
  m_savedKeyFrame.setPaths(GeometryObjects::paths()->paths());
  m_savedKeyFrame.setTrisets(GeometryObjects::trisets()->get());
  m_savedKeyFrame.setGamma(Global::gamma());
}

void
KeyFrame::setKeyFrame(Vec pos, Quaternion rot,
		      int frameNumber,
		      LightingInformation lightInfo,
		      QList<BrickInformation> brickInfo,
		      QImage image)
{
  // -- save keyframe first into a m_savedKeyFrame
  saveProject(pos, rot,
	      lightInfo,
	      brickInfo,
	      image);

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

  kfi->setTitle(title);
  kfi->setFrameNumber(frameNumber);
  kfi->setDrawBox(Global::drawBox());
  kfi->setDrawAxis(Global::drawAxis());
  kfi->setShadowBox(Global::shadowBox());
  kfi->setBackgroundColor(Global::backgroundColor());
  kfi->setBackgroundImageFile(Global::backgroundImageFile());
  kfi->setPosition(pos);
  kfi->setOrientation(rot);
  kfi->setLightInfo(lightInfo);
  kfi->setClipInfo(GeometryObjects::clipplanes()->clipInfo());
  kfi->setBrickInfo(brickInfo);
  kfi->setImage(image);
  kfi->setCaptions(GeometryObjects::captions()->captions());
  kfi->setScaleBars(GeometryObjects::scalebars()->scalebars());
  kfi->setPoints(GeometryObjects::hitpoints()->points(),
		 GeometryObjects::hitpoints()->barePoints(),
		 GeometryObjects::hitpoints()->pointSize(),
		 GeometryObjects::hitpoints()->pointColor());
  kfi->setPaths(GeometryObjects::paths()->paths());
  kfi->setTrisets(GeometryObjects::trisets()->get());
  kfi->setGamma(Global::gamma());

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
  kfi.setShadowBox(m_keyFrameInfo[kf]->shadowBox());
  //-------------------------------  

  
  //-------------------------------  
  rfrc = StaticFunctions::remapKeyframe(m_keyFrameInfo[kf]->interpBGColor(), frc);
  Vec bg0 = m_keyFrameInfo[kf]->backgroundColor();
  Vec bg1 = m_keyFrameInfo[kf+1]->backgroundColor();
  Vec bg = bg0 + rfrc*(bg1-bg0);
  kfi.setBackgroundColor(bg);
  //-------------------------------  


  //-------------------------------  
  float bt0 = m_keyFrameInfo[kf]->gamma();
  float bt1 = m_keyFrameInfo[kf+1]->gamma();
  float bt = bt0 + frc*(bt1-bt0);
  kfi.setGamma(bt);
  //-------------------------------  

  
  //-------------------------------  
  if (frc < 0.5)
    kfi.setBackgroundImageFile(m_keyFrameInfo[kf]->backgroundImageFile());
  else
    kfi.setBackgroundImageFile(m_keyFrameInfo[kf+1]->backgroundImageFile());
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
  rfrc = StaticFunctions::remapKeyframe(m_keyFrameInfo[kf]->interpCaptions(), frc);
  QList<CaptionObject> caption;  
  QList<CaptionObject> caption1 = m_keyFrameInfo[kf]->captions();
  QList<CaptionObject> caption2 = m_keyFrameInfo[kf+1]->captions();
  caption = CaptionObject::interpolate(caption1, caption2, rfrc);
  kfi.setCaptions(caption);
  //-------------------------------  


  //-------------------------------  
  QList<ScaleBarObject> scalebars;  
  QList<ScaleBarObject> scalebars1 = m_keyFrameInfo[kf]->scalebars();
  QList<ScaleBarObject> scalebars2 = m_keyFrameInfo[kf+1]->scalebars();
  scalebars = ScaleBarObject::interpolate(scalebars1, scalebars2, frc);
  kfi.setScaleBars(scalebars);
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

  //kfi.setPaths(m_keyFrameInfo[kf]->paths());
  QList<PathObject> po;
  QList<PathObject> po1 = m_keyFrameInfo[kf]->paths();
  QList<PathObject> po2 = m_keyFrameInfo[kf+1]->paths();
  po = PathObject::interpolate(po1, po2, frc);
  kfi.setPaths(po);
  //-------------------------------  
  

  //-------------------------------  
  QList<TrisetInformation> tinfo;  
  QList<TrisetInformation> tinfo1 = m_keyFrameInfo[kf]->trisets();
  QList<TrisetInformation> tinfo2 = m_keyFrameInfo[kf+1]->trisets();
  tinfo = TrisetInformation::interpolate(tinfo1, tinfo2, frc);
  kfi.setTrisets(tinfo);
  //-------------------------------  
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

  Global::setGamma(m_savedKeyFrame.gamma());

  GeometryObjects::captions()->setCaptions(m_savedKeyFrame.captions());
  GeometryObjects::scalebars()->setScaleBars(m_savedKeyFrame.scalebars());
  GeometryObjects::hitpoints()->setPoints(m_savedKeyFrame.points());
  GeometryObjects::hitpoints()->setBarePoints(m_savedKeyFrame.barepoints());
  GeometryObjects::hitpoints()->setPointSize(m_savedKeyFrame.pointSize());
  GeometryObjects::hitpoints()->setPointColor(m_savedKeyFrame.pointColor());
  GeometryObjects::paths()->setPaths(m_savedKeyFrame.paths());
  GeometryObjects::trisets()->set(m_savedKeyFrame.trisets());
  GeometryObjects::clipplanes()->set(m_savedKeyFrame.clipInfo());

  emit updateLookFrom(pos, rot);
  emit updateBrickInfo(m_savedKeyFrame.brickInfo());
  emit updateLightInfo(m_savedKeyFrame.lightInfo());


  bool drawBox = m_savedKeyFrame.drawBox();
  bool drawAxis = m_savedKeyFrame.drawAxis();
  Vec backgroundColor = m_savedKeyFrame.backgroundColor();
  QString backgroundImage = m_savedKeyFrame.backgroundImageFile();
  bool shadowBox = m_savedKeyFrame.shadowBox();
  emit updateParameters(drawBox, drawAxis,
			shadowBox,
			backgroundColor,
			backgroundImage);
  
  Global::enableViewerUpdate();
  MainWindowUI::changeDrishtiIcon(true);
  Global::setPlayFrames(true);

  
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

	  if (!Global::prayogMode())
	    {
	      Vec pos;
	      Quaternion rot;
	      pos = m_keyFrameInfo[kf]->position();
	      rot = m_keyFrameInfo[kf]->orientation();
	      emit updateLookFrom(pos, rot);
	    }
	  
	  Global::setGamma(m_keyFrameInfo[kf]->gamma());
	  
	  GeometryObjects::captions()->setCaptions(m_keyFrameInfo[kf]->captions());
	  GeometryObjects::scalebars()->setScaleBars(m_keyFrameInfo[kf]->scalebars());
	  GeometryObjects::hitpoints()->setPoints(m_keyFrameInfo[kf]->points());
	  GeometryObjects::hitpoints()->setBarePoints(m_keyFrameInfo[kf]->barepoints());
	  GeometryObjects::hitpoints()->setPointSize(m_keyFrameInfo[kf]->pointSize());
	  GeometryObjects::hitpoints()->setPointColor(m_keyFrameInfo[kf]->pointColor());
	  GeometryObjects::paths()->setPaths(m_keyFrameInfo[kf]->paths());
	  GeometryObjects::trisets()->set(m_keyFrameInfo[kf]->trisets());
	  GeometryObjects::clipplanes()->set(m_keyFrameInfo[kf]->clipInfo());



	  emit updateBrickInfo(m_keyFrameInfo[kf]->brickInfo());
	  emit updateLightInfo(m_keyFrameInfo[kf]->lightInfo());
	  

	  bool drawBox = m_keyFrameInfo[kf]->drawBox();
	  bool drawAxis = m_keyFrameInfo[kf]->drawAxis();
	  Vec backgroundColor = m_keyFrameInfo[kf]->backgroundColor();
	  QString backgroundImage = m_keyFrameInfo[kf]->backgroundImageFile();
	  bool shadowBox = m_keyFrameInfo[kf]->shadowBox();
	  emit updateParameters(drawBox, drawAxis,
				shadowBox,
				backgroundColor,
				backgroundImage);


	  Global::enableViewerUpdate();
	  MainWindowUI::changeDrishtiIcon(true);
	  Global::setPlayFrames(true);

	  
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

  emit updateLookFrom(pos, rot);

  Global::setGamma(keyFrameInfo.gamma());

  GeometryObjects::captions()->setCaptions(keyFrameInfo.captions());
  GeometryObjects::scalebars()->setScaleBars(keyFrameInfo.scalebars());
  GeometryObjects::hitpoints()->setPoints(keyFrameInfo.points());
  GeometryObjects::hitpoints()->setBarePoints(keyFrameInfo.barepoints());
  GeometryObjects::hitpoints()->setPointSize(keyFrameInfo.pointSize());
  GeometryObjects::hitpoints()->setPointColor(keyFrameInfo.pointColor());
  GeometryObjects::paths()->setPaths(keyFrameInfo.paths());
  GeometryObjects::trisets()->set(keyFrameInfo.trisets());
  GeometryObjects::clipplanes()->set(keyFrameInfo.clipInfo());


  emit updateLightInfo(keyFrameInfo.lightInfo());
  emit updateBrickInfo(keyFrameInfo.brickInfo());


  bool drawBox = keyFrameInfo.drawBox();
  bool drawAxis = keyFrameInfo.drawAxis();
  Vec backgroundColor = keyFrameInfo.backgroundColor();
  QString backgroundImage = keyFrameInfo.backgroundImageFile();
  bool shadowBox = keyFrameInfo.shadowBox();
  emit updateParameters(drawBox, drawAxis,
			shadowBox,
			backgroundColor,
			backgroundImage);


  Global::enableViewerUpdate();
  MainWindowUI::changeDrishtiIcon(true);
  Global::setPlayFrames(true);

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
  //keys << "title";
  keys << "frame";
  keys << "background color";
  keys << "captions";
  //keys << "separator";
  keys << "camera position";
  keys << "camera orientation";
  //keys << "separator";
  keys << "model transform";
  //keys << "clip planes";
  keys << "shading";

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
	  else if (keys[ik] == "frame")
	    {
	      vlist << QVariant("int");
	      vlist << kfi->frameNumber();
	      vlist << 1;
	      vlist << 100000;
	    }
	  else
	    {
	      vlist << QVariant("combobox");

	      if (keys[ik] == "background color") vlist << kfi->interpBGColor();
	      else if (keys[ik] == "captions") vlist << kfi->interpCaptions();
	      else if (keys[ik] == "camera position") vlist << kfi->interpCameraPosition();
	      else if (keys[ik] == "camera orientation") vlist << kfi->interpCameraOrientation();
	      else if (keys[ik] == "model transform") vlist << kfi->interpBrickInfo();
	      else if (keys[ik] == "clip planes") vlist << kfi->interpClipInfo();
	      else if (keys[ik] == "shading") vlist << kfi->interpLightInfo();
	      
	      vlist << QVariant("linear");
	      vlist << QVariant("smoothstep");
	      vlist << QVariant("easein");
	      vlist << QVariant("easeout");
	      vlist << QVariant("none");
	    }

	  plist[keys[ik]] = vlist;
	}
    }


  propertyEditor.setFont(QFont("Helvetica", 12));
  propertyEditor.set("Keyframe Interpolation Parameters",
		     plist, keys);
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
	  else if (keys[ik] == "frame")
	    {
	      int fn = pair.first.toInt();	      
	      bool ok = true;
	      if (fn < 1) // make sure frame number is greater than 0
		{
		  QMessageBox::information(0, "Error",
					   "Cannot move keyframe to 0");
		  return;
		}
	      
	      for(int ic=0; ic<m_keyFrameInfo.count(); ic++)
		{
		  if (fn == m_keyFrameInfo[ic]->frameNumber())
		    {
		      ok = false;
		      break;
		    }			
		}
	      if (ok)
		{
		  kfi->setFrameNumber(fn);
		  emit newKeyframeNumber(kfn, fn);
		}
	      else
		{
		  QMessageBox::information(0, "Error",
					   QString("Cannot move keyframe onto existing one at %1").arg(fn));
		}
	    }
	  else
	    {
	      int flag = pair.first.toInt();
	      if (keys[ik] == "background color") kfi->setInterpBGColor(flag);
	      else if (keys[ik] == "captions") kfi->setInterpCaptions(flag);
	      else if (keys[ik] == "camera position") kfi->setInterpCameraPosition(flag);
	      else if (keys[ik] == "camera orientation") kfi->setInterpCameraOrientation(flag);
	      else if (keys[ik] == "model transform") kfi->setInterpBrickInfo(flag);
	      else if (keys[ik] == "clip planes") kfi->setInterpClipInfo(flag);
	      else if (keys[ik] == "shading") kfi->setInterpLightInfo(flag);
	    }
	}
    }
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
	  else if (keys[ik] == "shadow box")
	    kfi->setShadowBox(m_copyKeyFrame.shadowBox());  
	  else if (keys[ik] == "bounding box")
	    kfi->setDrawBox(m_copyKeyFrame.drawBox());  
	  else if (keys[ik] == "model transform")
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
	  else if (keys[ik] == "scalebars")
	    kfi->setScaleBars(m_copyKeyFrame.scalebars());  
	  else if (keys[ik] == "clip planes")
	    kfi->setClipInfo(m_copyKeyFrame.clipInfo());  
	  else if (keys[ik] == "shading")
	    {
	      kfi->setLightInfo(m_copyKeyFrame.lightInfo());
	      kfi->setGamma(m_copyKeyFrame.gamma());
	    }
	  else if (keys[ik] == "paths")
	    {
	      kfi->setPaths(m_copyKeyFrame.paths());  
	    }
	  else if (keys[ik] == "points")
	    {
	      kfi->setPoints(m_copyKeyFrame.points(),
			     m_copyKeyFrame.barepoints(),
			     m_copyKeyFrame.pointSize(),
			     m_copyKeyFrame.pointColor());
	    }
	  else if (keys[ik] == "surfaces")
	    kfi->setTrisets(m_copyKeyFrame.trisets());  
	  else if (keys[ik] == "surface colors only")
	    kfi->setTrisetsColors(m_copyKeyFrame.trisets());  
	  else if (keys[ik] == "surface labels only")
	    kfi->setTrisetsLabels(m_copyKeyFrame.trisets());  
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
	      else if (keys[ik] == "shadow box")
		kfi->setShadowBox(m_copyKeyFrame.shadowBox());  
	      else if (keys[ik] == "bounding box")
		kfi->setDrawBox(m_copyKeyFrame.drawBox());  
	      else if (keys[ik] == "model transform")
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
	      else if (keys[ik] == "scalebars")
		kfi->setScaleBars(m_copyKeyFrame.scalebars());  
	      else if (keys[ik] == "clip planes")
		kfi->setClipInfo(m_copyKeyFrame.clipInfo());  
	      else if (keys[ik] == "shading")
		{
		  kfi->setLightInfo(m_copyKeyFrame.lightInfo());  
		  kfi->setGamma(m_copyKeyFrame.gamma());
		}
	      else if (keys[ik] == "paths")
		{
		  kfi->setPaths(m_copyKeyFrame.paths());  
		}
	      else if (keys[ik] == "points")
		{
		  kfi->setPoints(m_copyKeyFrame.points(),
				 m_copyKeyFrame.barepoints(),
				 m_copyKeyFrame.pointSize(),
				 m_copyKeyFrame.pointColor());
		}
	      else if (keys[ik] == "surfaces")
		kfi->setTrisets(m_copyKeyFrame.trisets());  
	      else if (keys[ik] == "surface colors only")
		kfi->setTrisetsColors(m_copyKeyFrame.trisets());  
	      else if (keys[ik] == "surface labels only")
		kfi->setTrisetsLabels(m_copyKeyFrame.trisets());  
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

//  vlist.clear();
//  vlist << QVariant("checkbox");
//  vlist << QVariant(false);
//  plist["axis"] = vlist;

  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(false);
  plist["background color"] = vlist;

  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(false);
  plist["shadow box"] = vlist;

  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(false);
  plist["bounding box"] = vlist;
  
  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(false);
  plist["model transform"] = vlist;

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

//  vlist.clear();
//  vlist << QVariant("checkbox");
//  vlist << QVariant(false);
//  plist["scalebars"] = vlist;

  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(false);
  plist["clip planes"] = vlist;

  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(false);
  plist["shading"] = vlist;

//  vlist.clear();
//  vlist << QVariant("checkbox");
//  vlist << QVariant(false);
//  plist["paths"] = vlist;

  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(false);
  plist["points"] = vlist;

  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(false);
  plist["surfaces"] = vlist;

  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(false);
  plist["surface colors only"] = vlist;

  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(false);
  plist["surface labels only"] = vlist;


  QStringList keys;
  keys << "SELECT ALL";
  //keys << "axis";
  keys << "background color";
  keys << "shadow box";
  keys << "bounding box";
  keys << "captions";
  //keys << "scalebars";
  keys << "separator";
  keys << "camera position";
  keys << "camera orientation";
  keys << "separator";
  keys << "model transform";
  keys << "clip planes";
  keys << "shading";
  keys << "separator";
  //keys << "paths";
  keys << "points";
  keys << "surfaces";
  keys << "surface colors only";
  keys << "surface labels only";


  propertyEditor.setFont(QFont("Helvetica", 12));
  propertyEditor.set(title,
		     plist, keys);
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
	      else if (keys[ik] == "model transform")
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
	      else if (keys[ik] == "scalebars")
		{
		  kfi->setScaleBars(ckf->scalebars());  
		}
	      else if (keys[ik] == "clip planes")
		{
		  kfi->setClipInfo(ckf->clipInfo());  
		  kfi->setInterpClipInfo(ckf->interpClipInfo());
		}
	      else if (keys[ik] == "shading")
		{
		  kfi->setLightInfo(ckf->lightInfo());  
		  kfi->setInterpLightInfo(ckf->interpLightInfo());
		  kfi->setGamma(m_copyKeyFrame.gamma());
		}
	      else if (keys[ik] == "paths")
		{
		  kfi->setPaths(ckf->paths());  
		}
	      else if (keys[ik] == "points")
		{
		  kfi->setPoints(ckf->points(),
				 ckf->barepoints(),
				 ckf->pointSize(),
				 ckf->pointColor());
		}
	      else if (keys[ik] == "surfaces")
		kfi->setTrisets(ckf->trisets());  
	      else if (keys[ik] == "surface colors only")
		kfi->setTrisetsColors(ckf->trisets());  
	      else if (keys[ik] == "surface labels only")
		kfi->setTrisetsLabels(ckf->trisets());  
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
