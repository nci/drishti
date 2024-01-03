#ifndef CONNECTBRICKSWIDGETVIEWER_H
#define CONNECTBRICKSWIDGETVIEWER_H

  connect(m_bricksWidget, SIGNAL(updateGL()),
	  m_Viewer, SLOT(updateGL()));
#endif
