#ifndef CONNECTGEOMETRYOBJECTS_H
#define CONNECTGEOMETRYOBJECTS_H

    connect(GeometryObjects::paths(), SIGNAL(showMessage(QString, bool)),
	    m_Viewer, SLOT(displayMessage(QString, bool)));

    connect(GeometryObjects::paths(), SIGNAL(showProfile(int, int, QList<Vec>)),
	    this, SLOT(viewProfile(int, int, QList<Vec>)));

    connect(GeometryObjects::paths(),
	    SIGNAL(showThicknessProfile(int, int,
					QList< QPair<Vec, Vec> >)),
	    this, SLOT(viewThicknessProfile(int, int,
					    QList< QPair<Vec, Vec> >)));

    connect(GeometryObjects::paths(),
	    SIGNAL(addToCameraPath(QList<Vec>,QList<Vec>,QList<Vec>,QList<Vec>)),
	    this, SLOT(addToCameraPath(QList<Vec>,QList<Vec>,QList<Vec>,QList<Vec>)));


    connect(GeometryObjects::paths(),
	    SIGNAL(sculpt(int, QList<Vec>, float, float, int)),
	    this,
	    SLOT(sculpt(int, QList<Vec>, float, float, int)));

    connect(GeometryObjects::paths(),
	    SIGNAL(fillPathPatch(QList<Vec>, int, int)),
	    this,
	    SLOT(fillPathPatch(QList<Vec>, int, int)));

    connect(GeometryObjects::paths(),
	    SIGNAL(paintPathPatch(QList<Vec>, int, int)),
	    this,
	    SLOT(paintPathPatch(QList<Vec>, int, int)));

    connect(GeometryObjects::hitpoints(),
	    SIGNAL(sculpt(int, QList<Vec>, float, float, int)),
	    this,
	    SLOT(sculpt(int, QList<Vec>, float, float, int)));

connect(GeometryObjects::paths(), SIGNAL(extractPath(int, bool, int, int)),
	    this, SLOT(extractPath(int, bool, int, int)));

    connect(GeometryObjects::paths(), SIGNAL(updateGL()),
	    m_Viewer, SLOT(updateGL()));


    connect(GeometryObjects::crops(), SIGNAL(showMessage(QString, bool)),
	    m_Viewer, SLOT(displayMessage(QString, bool)));

    connect(GeometryObjects::crops(), SIGNAL(updateGL()),
	    m_Viewer, SLOT(updateGL()));

    connect(GeometryObjects::crops(), SIGNAL(mopCrop(int)),
	    this, SLOT(mopCrop(int)));


    connect(GeometryObjects::clipplanes(), SIGNAL(saveSliceImage(int, int)),
	    this, SLOT(saveSliceImage(int, int)));

    connect(GeometryObjects::clipplanes(), SIGNAL(extractClip(int, int, int)),
	    this, SLOT(extractClip(int, int, int)));

    connect(GeometryObjects::clipplanes(), SIGNAL(reorientCameraUsingClipPlane(int)),
	    this, SLOT(reorientCameraUsingClipPlane(int)));

    connect(GeometryObjects::clipplanes(), SIGNAL(mopClip(Vec, Vec)),
	    this, SLOT(mopClip(Vec, Vec)));


    connect(GeometryObjects::grids(), SIGNAL(updateGL()),
	    m_Viewer, SLOT(updateGL()));

    connect(GeometryObjects::grids(),
	    SIGNAL(gridStickToSurface(int, int,
				      QList< QPair<Vec, Vec> >)),
	    this, SLOT(gridStickToSurface(int, int,
					  QList< QPair<Vec, Vec> >)));



#endif
