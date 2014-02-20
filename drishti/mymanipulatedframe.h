#ifndef MYMANIPULATEDFRAME_H
#define MYMANIPULATEDFRAME_H

#include <QGLViewer/manipulatedFrame.h>
#include <QGLViewer/mouseGrabber.h>
#include <QGLViewer/qglviewer.h>
using namespace qglviewer;

#include "commonqtclasses.h"

class MyManipulatedFrame : public ManipulatedFrame {
  Q_OBJECT
  public :
    MyManipulatedFrame();

    void setThreshold(int);
    int threshold();

    void setOnlyRotate(bool);
    bool onlyRotate();

    void setOnlyTranslate(bool);
    bool onlyTranslate();

    void mousePosition(int&, int&);

    virtual void checkIfGrabsMouse(int, int, const Camera* const);
 protected :
    virtual void mousePressEvent(QMouseEvent* const, Camera* const);
    virtual void mouseMoveEvent(QMouseEvent* const, Camera* const);
    virtual void mouseReleaseEvent(QMouseEvent* const, Camera* const);
  private :
    int m_threshold;
    bool m_keepsGrabbingMouse;
    bool m_onlyRotate;
    bool m_onlyTranslate;
    int m_lastX, m_lastY;
};

#endif
