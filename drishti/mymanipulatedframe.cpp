#include "mymanipulatedframe.h"

MyManipulatedFrame::MyManipulatedFrame()
{
  m_threshold = 20;
  m_keepsGrabbingMouse = false;
  m_onlyRotate = false;
  m_onlyTranslate = false;
}

int MyManipulatedFrame::threshold() { return m_threshold; }
bool MyManipulatedFrame::onlyRotate() { return m_onlyRotate; }
bool MyManipulatedFrame::onlyTranslate() { return m_onlyTranslate; }

void MyManipulatedFrame::setThreshold(int t)
{
  m_threshold = qMax(10, t);
}
void MyManipulatedFrame::setOnlyRotate(bool b)
{
  m_onlyRotate = b;
  if (m_onlyRotate)
    m_onlyTranslate = false;
}
void MyManipulatedFrame::setOnlyTranslate(bool b)
{
  m_onlyTranslate = b;
  if (m_onlyTranslate)
    m_onlyRotate = false;
}

void MyManipulatedFrame::mousePosition(int& x, int& y)
{
  x = m_lastX;
  y = m_lastY;
}

void MyManipulatedFrame::checkIfGrabsMouse(int x, int y,
					   const Camera* const camera)
{
  m_lastX = x;
  m_lastY = y;
  const Vec proj = camera->projectedCoordinatesOf(position());
  setGrabsMouse(m_keepsGrabbingMouse ||
		((fabs(x-proj.x) < m_threshold) &&
		 (fabs(y-proj.y) < m_threshold)) );
}
void
MyManipulatedFrame::mousePressEvent(QMouseEvent* const event,
				    Camera* const camera)
{
  ManipulatedFrame::mousePressEvent(event, camera);

  if (grabsMouse())
    m_keepsGrabbingMouse = true;
}

void
MyManipulatedFrame::mouseMoveEvent(QMouseEvent* const event,
				   Camera* const camera)
{
  if (m_onlyTranslate && action_ == QGLViewer::ROTATE)
    action_ = QGLViewer::TRANSLATE;
  else if (m_onlyRotate && action_ == QGLViewer::TRANSLATE)
    action_ = QGLViewer::ROTATE;

  ManipulatedFrame::mouseMoveEvent(event, camera);
}

void
MyManipulatedFrame::mouseReleaseEvent(QMouseEvent* const event,
				      Camera* const camera)
{
  ManipulatedFrame::mouseReleaseEvent(event, camera);
  m_keepsGrabbingMouse = false;
}
