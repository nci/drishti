#ifndef CONNECTKEYFRAME_H
#define CONNECTKEYFRAME_H

  connect(m_keyFrame, SIGNAL(updateBrickInfo(QList<BrickInformation>)),
	  m_bricks, SLOT(setBricks(QList<BrickInformation>)));




  connect(m_keyFrame, SIGNAL(addKeyFrameNumbers(QList<int>)),
	  m_keyFrameEditor, SLOT(addKeyFrameNumbers(QList<int>)));

  connect(m_keyFrame, SIGNAL(setImage(int, QImage)),
	  m_keyFrameEditor, SLOT(setImage(int, QImage)));
  connect(m_keyFrame, SIGNAL(loadKeyframes(QList<int>, QList<QImage>)),
	  m_keyFrameEditor, SLOT(loadKeyframes(QList<int>, QList<QImage>)));  


  connect(m_keyFrame, SIGNAL(updateLightInfo(LightingInformation)),
	  m_lightingWidget,
	  SLOT(setLightInfo(LightingInformation)));


  connect(m_keyFrame, SIGNAL(updateVolInfo(int)),
	  this, SLOT(updateVolInfo(int)));

  connect(m_keyFrame, SIGNAL(updateVolInfo(int, int)),
	  this, SLOT(updateVolInfo(int, int)));

  connect(m_keyFrame, SIGNAL(updateTransferFunctionManager(QList<SplineInformation>)),
	  this, SLOT(updateTransferFunctionManager(QList<SplineInformation>)));

  connect(m_keyFrame, SIGNAL(updateMorph(bool)),
	  this, SLOT(updateMorph(bool)));

connect(m_keyFrame, SIGNAL(updateFocus(float, float)),
	this, SLOT(updateFocus(float, float)));

  connect(m_keyFrame, SIGNAL(updateParameters(bool, bool, Vec, QString,
					      int, int, QString, QString, QString,
					      int, bool, bool, float, bool, bool)),
	  this, SLOT(updateParameters(bool, bool, Vec, QString,
				      int, int, QString, QString, QString,
				      int, bool, bool, float, bool, bool)));



  // connect updateGL to updateScaling
  connect(m_keyFrame, SIGNAL(updateGL()),
	  m_Viewer, SLOT(updateScaling()));

  connect(m_keyFrame, SIGNAL(replaceKeyFrameImage(int)),
	  m_Viewer, SLOT(captureKeyFrameImage(int)));

  connect(m_keyFrame, SIGNAL(currentFrameChanged(int)),
	  m_Viewer, SLOT(setCurrentFrame(int)));

connect(m_keyFrame, SIGNAL(updateLookFrom(Vec, Quaternion, float, float)),
	m_Viewer, SLOT(updateLookFrom(Vec, Quaternion, float, float)));

  connect(m_keyFrame, SIGNAL(updateLookupTable(unsigned char*)),
	  m_Viewer, SLOT(updateLookupTable(unsigned char*)));

  connect(m_keyFrame, SIGNAL(updateTagColors()),
	  m_Viewer, SLOT(updateTagColors()));

  connect(m_keyFrame, SIGNAL(updateTagColors()),
	  m_preferencesWidget, SLOT(updateTagColors()));


#endif
