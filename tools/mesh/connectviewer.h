#ifndef CONNECTVIEWER_H
#define CONNECTVIEWER_H

  connect(m_Viewer, SIGNAL(changeSelectionMode(bool)),
	  this, SLOT(on_actionGrabMode_triggered(bool)));

  connect(m_Viewer, SIGNAL(loadSurfaceMesh(QString)),
	  this, SLOT(loadSurfaceMesh(QString)));

  connect(m_Viewer, SIGNAL(addRotationAnimation(int, float, int)),
	  this, SLOT(addRotationAnimation(int, float, int)));

  connect(m_Viewer, SIGNAL(quitDrishti()),
	  this, SLOT(quitDrishti()));

  connect(m_Viewer, SIGNAL(allGood(bool)),
	  this, SLOT(checkParvana(bool)));

  connect(m_Viewer, SIGNAL(switchBB()),
	  this, SLOT(switchBB()));

  connect(m_Viewer, SIGNAL(switchAxis()),
	  this, SLOT(switchAxis()));

  connect(m_Viewer, SIGNAL(moveToKeyframe(int)),
	  this, SLOT(moveToKeyframe(int)));

  connect(m_Viewer, SIGNAL(setKeyFrame(Vec, Quaternion, int, QImage)),
	  this, SLOT(setKeyFrame(Vec, Quaternion, int, QImage)));

  connect(m_Viewer, SIGNAL(replaceKeyFrameImage(int, QImage)),
	  m_keyFrameEditor, SLOT(setImage(int, QImage)));

  connect(m_Viewer, SIGNAL(nextVRKeyFrame()),
	  m_keyFrameEditor, SLOT(nextKeyFrame()));

  connect(m_Viewer, SIGNAL(prevVRKeyFrame()),
	  m_keyFrameEditor, SLOT(prevKeyFrame()));

  connect(m_Viewer, SIGNAL(replaceKeyFrameImage(int, QImage)),
	  m_keyFrame, SLOT(replaceKeyFrameImage(int, QImage)));


#endif
