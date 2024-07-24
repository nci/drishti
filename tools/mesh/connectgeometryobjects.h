#ifndef CONNECTGEOMETRYOBJECTS_H
#define CONNECTGEOMETRYOBJECTS_H

    connect(GeometryObjects::trisets(), SIGNAL(removeAllKeyFrames()),
	    m_keyFrameEditor, SLOT(removeAllKeyFrames()));

    connect(GeometryObjects::trisets(), SIGNAL(resetBoundingBox()),
	    m_Viewer, SLOT(showFullScene()));

    connect(GeometryObjects::trisets(), SIGNAL(updateGL()),
	    m_Viewer, SLOT(updateGL()));

    connect(GeometryObjects::trisets(), SIGNAL(updateScaling()),
	    m_Viewer, SLOT(updateScaling()));

    connect(GeometryObjects::trisets(), SIGNAL(clearScene()),
	    this, SLOT(clearScene()));

    connect(GeometryObjects::paths(), SIGNAL(showMessage(QString, bool)),
	    m_Viewer, SLOT(displayMessage(QString, bool)));


    connect(GeometryObjects::paths(),
	    SIGNAL(addToCameraPath(QList<Vec>,QList<Vec>,QList<Vec>,QList<Vec>)),
	    this, SLOT(addToCameraPath(QList<Vec>,QList<Vec>,QList<Vec>,QList<Vec>)));


    connect(GeometryObjects::paths(), SIGNAL(updateGL()),
	    m_Viewer, SLOT(updateGL()));


    connect(GeometryObjects::clipplanes(), SIGNAL(reorientCameraUsingClipPlane(int)),
	    this, SLOT(reorientCameraUsingClipPlane(int)));


#endif
