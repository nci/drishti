#ifndef CONNECTVIEWER_H
#define CONNECTVIEWER_H

  connect(m_Viewer, SIGNAL(resetFlipImage()),
	  this, SLOT(resetFlipImage()));

  connect(m_Viewer, SIGNAL(addRotationAnimation(int, float, int)),
	  this, SLOT(addRotationAnimation(int, float, int)));

  connect(m_Viewer, SIGNAL(quitDrishti()),
	  this, SLOT(quitDrishti()));

  connect(m_Viewer, SIGNAL(switchBB()),
	  this, SLOT(switchBB()));

  connect(m_Viewer, SIGNAL(switchAxis()),
	  this, SLOT(switchAxis()));

  connect(m_Viewer, SIGNAL(moveToKeyframe(int)),
	  this, SLOT(moveToKeyframe(int)));

  connect(m_Viewer, SIGNAL(searchCaption(QStringList)),
	  this, SLOT(searchCaption(QStringList)));

  connect(m_Viewer, SIGNAL(saveVolume()),
	  this, SLOT(saveVolume()));

  connect(m_Viewer, SIGNAL(maskRawVolume()),
	  this, SLOT(maskRawVolume()));

  connect(m_Viewer, SIGNAL(countIsolatedRegions()),
	  this, SLOT(countIsolatedRegions()));

  connect(m_Viewer, SIGNAL(getSurfaceArea()),
	  this, SLOT(getSurfaceArea()));

  connect(m_Viewer, SIGNAL(getSurfaceArea(unsigned char)),
	  this, SLOT(getSurfaceArea(unsigned char)));

  connect(m_Viewer, SIGNAL(setView(Vec, Quaternion,
				   QImage, float)),
	  this, SLOT(setView(Vec, Quaternion,
			     QImage, float)));

  connect(m_Viewer, SIGNAL(setKeyFrame(Vec, Quaternion,
				       int, float, float,
				       unsigned char*,
				       QImage)),
	  this, SLOT(setKeyFrame(Vec, Quaternion,
				 int, float, float,
				 unsigned char*,
				 QImage)));

  connect(m_Viewer, SIGNAL(setHiresMode(bool)),
	  m_keyFrameEditor, SLOT(setHiresMode(bool)));

  connect(m_Viewer, SIGNAL(replaceKeyFrameImage(int, QImage)),
	  m_keyFrameEditor, SLOT(setImage(int, QImage)));

  connect(m_Viewer, SIGNAL(replaceKeyFrameImage(int, QImage)),
	  m_keyFrame, SLOT(replaceKeyFrameImage(int, QImage)));

  connect(m_Viewer, SIGNAL(histogramUpdated(QImage, QImage)),
	  m_tfEditor, SLOT(setHistogramImage(QImage, QImage)));



#endif
