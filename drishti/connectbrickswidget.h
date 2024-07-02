#ifndef CONNECTBRICKSWIDGETVIEWER_H
#define CONNECTBRICKSWIDGETVIEWER_H

  connect(m_bricksWidget, SIGNAL(updateGL()),
	  m_Viewer, SLOT(updateGL()));

  connect(m_bricksWidget, SIGNAL(brickAngleFromMouse(bool)),
	  m_Viewer, SLOT(brickAngleFromMouse(bool)));
#endif
