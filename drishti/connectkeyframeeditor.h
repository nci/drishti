#ifndef CONNECTKEYFRAMEEDITOR_H
#define CONNECTKEYFRAMEEDITOR_H

  connect(m_keyFrameEditor, SIGNAL(updateGL()),
	  m_Viewer, SLOT(updateGL()));

  connect(m_keyFrameEditor, SIGNAL(endPlay()),
	  m_Viewer, SLOT(endPlay()));

  connect(m_keyFrameEditor, SIGNAL(setKeyFrame(int)),
	  m_Viewer, SLOT(setKeyFrame(int)));



  connect(m_keyFrameEditor, SIGNAL(reorder(QList<int>)),
	  m_keyFrame, SLOT(reorder(QList<int>)));

  connect(m_keyFrameEditor, SIGNAL(setKeyFrameNumbers(QList<int>)),
	  m_keyFrame, SLOT(setKeyFrameNumbers(QList<int>)));

  connect(m_keyFrameEditor, SIGNAL(playFrameNumber(int)),
	  m_keyFrame, SLOT(playFrameNumber(int)));

  connect(m_keyFrameEditor, SIGNAL(setKeyFrameNumber(int, int)),
	  m_keyFrame, SLOT(setKeyFrameNumber(int, int)));

  connect(m_keyFrameEditor, SIGNAL(removeKeyFrame(int)),
	  m_keyFrame, SLOT(removeKeyFrame(int)));

connect(m_keyFrameEditor, SIGNAL(removeKeyFrames(int, int)),
	m_keyFrame, SLOT(removeKeyFrames(int, int)));

  connect(m_keyFrameEditor, SIGNAL(copyFrame(int)),
	  m_keyFrame, SLOT(copyFrame(int)));

  connect(m_keyFrameEditor, SIGNAL(pasteFrame(int)),
	  m_keyFrame, SLOT(pasteFrame(int)));

  connect(m_keyFrameEditor, SIGNAL(pasteFrameOnTop(int)),
	  m_keyFrame, SLOT(pasteFrameOnTop(int)));

connect(m_keyFrameEditor, SIGNAL(pasteFrameOnTop(int, int)),
	  m_keyFrame, SLOT(pasteFrameOnTop(int, int)));

  connect(m_keyFrameEditor, SIGNAL(editFrameInterpolation(int)),
	  m_keyFrame, SLOT(editFrameInterpolation(int)));


#endif
