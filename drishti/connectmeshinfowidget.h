#ifndef CONNECTMESHINFOWIDGET_H
#define CONNECTMESHINFOWIDGET_H

  connect(m_meshInfoWidget, SIGNAL(setVisible(int, bool)),
	  GeometryObjects::trisets(), SLOT(setShow(int, bool)));
  connect(m_meshInfoWidget, SIGNAL(setVisible(QList<bool>)),
	  GeometryObjects::trisets(), SLOT(setShow(QList<bool>)));
  connect(m_meshInfoWidget, SIGNAL(setClip(int, bool)),
	  GeometryObjects::trisets(), SLOT(setClip(int, bool)));
  connect(m_meshInfoWidget, SIGNAL(setClip(QList<bool>)),
	  GeometryObjects::trisets(), SLOT(setClip(QList<bool>)));
  connect(m_meshInfoWidget, SIGNAL(setActive(int, bool)),
	  GeometryObjects::trisets(), SLOT(setActive(int, bool)));
  connect(m_meshInfoWidget, SIGNAL(removeMesh(int)),
	  GeometryObjects::trisets(), SLOT(removeMesh(int)));
  connect(m_meshInfoWidget, SIGNAL(removeMesh(QList<int>)),
	  GeometryObjects::trisets(), SLOT(removeMesh(QList<int>)));
  connect(m_meshInfoWidget, SIGNAL(saveMesh(int)),
	  GeometryObjects::trisets(), SLOT(saveMesh(int)));
//  connect(m_meshInfoWidget, SIGNAL(duplicateMesh(int)),
//	  GeometryObjects::trisets(), SLOT(duplicateMesh(int)));
//  connect(m_meshInfoWidget, SIGNAL(reorder(QList<int>)),
//	  GeometryObjects::trisets(), SLOT(reorder(QList<int>)));

  connect(GeometryObjects::trisets(), SIGNAL(setParameters(QMap<QString, QVariantList>)),
	  m_meshInfoWidget, SLOT(setParameters(QMap<QString, QVariantList>)));

  connect(m_meshInfoWidget, SIGNAL(positionChanged(QVector3D)),
	  GeometryObjects::trisets(), SLOT(positionChanged(QVector3D)));
  connect(m_meshInfoWidget, SIGNAL(scaleChanged(QVector3D)),
	  GeometryObjects::trisets(), SLOT(scaleChanged(QVector3D)));
  connect(m_meshInfoWidget, SIGNAL(colorChanged(QColor)),
	  GeometryObjects::trisets(), SLOT(colorChanged(QColor)));
  connect(m_meshInfoWidget, SIGNAL(colorChanged(QList<int>, QColor)),
	  GeometryObjects::trisets(), SLOT(colorChanged(QList<int>, QColor)));
  connect(m_meshInfoWidget, SIGNAL(transparencyChanged(int)),
	  GeometryObjects::trisets(), SLOT(transparencyChanged(int)));
  connect(m_meshInfoWidget, SIGNAL(revealChanged(int)),
	  GeometryObjects::trisets(), SLOT(revealChanged(int)));
  connect(m_meshInfoWidget, SIGNAL(outlineChanged(int)),
	  GeometryObjects::trisets(), SLOT(outlineChanged(int)));
  connect(m_meshInfoWidget, SIGNAL(glowChanged(int)),
	  GeometryObjects::trisets(), SLOT(glowChanged(int)));
  connect(m_meshInfoWidget, SIGNAL(darkenChanged(int)),
	  GeometryObjects::trisets(), SLOT(darkenChanged(int)));

  connect(m_meshInfoWidget, SIGNAL(processCommand(QList<int>, QString)),
	  GeometryObjects::trisets(), SLOT(processCommand(QList<int>, QString)));

  connect(m_meshInfoWidget, SIGNAL(processCommand(int, QString)),
	  GeometryObjects::trisets(), SLOT(processCommand(int, QString)));

  connect(m_meshInfoWidget, SIGNAL(processCommand(QString)),
	  GeometryObjects::trisets(), SLOT(processCommand(QString)));

  connect(m_meshInfoWidget, SIGNAL(rotationMode(bool)),
	  GeometryObjects::trisets(), SLOT(setRotationMode(bool)));

  connect(m_meshInfoWidget, SIGNAL(grabMesh(bool)),
	  GeometryObjects::trisets(), SLOT(setGrab(bool)));

  connect(m_meshInfoWidget, SIGNAL(multiSelection(QList<int>)),
	  GeometryObjects::trisets(), SLOT(multiSelection(QList<int>)));

  connect(GeometryObjects::trisets(), SIGNAL(meshGrabbed(int)),
	  m_meshInfoWidget, SLOT(setActive(int)));

#endif
