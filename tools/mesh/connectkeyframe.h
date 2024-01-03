#ifndef CONNECTKEYFRAME_H
#define CONNECTKEYFRAME_H

  connect(m_keyFrame, SIGNAL(updateBrickInfo(QList<BrickInformation>)),
	  m_bricks, SLOT(setBricks(QList<BrickInformation>)));



  connect(m_keyFrame, SIGNAL(newKeyframeNumber(int, int)),
	  m_keyFrameEditor, SLOT(newKeyframeNumber(int, int)));

  connect(m_keyFrame, SIGNAL(addKeyFrameNumbers(QList<int>)),
	  m_keyFrameEditor, SLOT(addKeyFrameNumbers(QList<int>)));

  connect(m_keyFrame, SIGNAL(setImage(int, QImage)),
	  m_keyFrameEditor, SLOT(setImage(int, QImage)));
  connect(m_keyFrame, SIGNAL(loadKeyframes(QList<int>, QList<QImage>)),
	  m_keyFrameEditor, SLOT(loadKeyframes(QList<int>, QList<QImage>)));  


  connect(m_keyFrame, SIGNAL(updateLightInfo(LightingInformation)),
	  m_lightingWidget,
	  SLOT(setLightInfo(LightingInformation)));


  connect(m_keyFrame, SIGNAL(updateParameters(bool, bool, bool, Vec, QString)),
	  this, SLOT(updateParameters(bool, bool, bool, Vec, QString)));



  // connect updateGL to updateScaling
  connect(m_keyFrame, SIGNAL(updateGL()),
	  m_Viewer, SLOT(updateScaling()));

  connect(m_keyFrame, SIGNAL(replaceKeyFrameImage(int)),
	  m_Viewer, SLOT(captureKeyFrameImage(int)));

  connect(m_keyFrame, SIGNAL(currentFrameChanged(int)),
	  m_Viewer, SLOT(setCurrentFrame(int)));

  connect(m_keyFrame, SIGNAL(updateLookFrom(Vec, Quaternion)),
	  m_Viewer, SLOT(updateLookFrom(Vec, Quaternion)));


#endif
