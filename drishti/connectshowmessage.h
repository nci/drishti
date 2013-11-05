#ifndef CONNECTSHOWMESSAGE_H
#define CONNECTSHOWMESSAGE_H

  connect(this, SIGNAL(showMessage(QString, bool)),
	  m_Viewer, SLOT(displayMessage(QString, bool)));

  connect(GeometryObjects::clipplanes(), SIGNAL(showMessage(QString, bool)),
	  m_Viewer, SLOT(displayMessage(QString, bool)));

  connect(m_bricksWidget, SIGNAL(showMessage(QString, bool)),
	  m_Viewer, SLOT(displayMessage(QString, bool)));

  connect(m_keyFrameEditor, SIGNAL(showMessage(QString, bool)),
	  m_Viewer, SLOT(displayMessage(QString, bool)));

  connect(m_gallery, SIGNAL(showMessage(QString, bool)),
	  m_Viewer, SLOT(displayMessage(QString, bool)));

  connect(GeometryObjects::paths(),
	  SIGNAL(showMessage(QString, bool)),
	  m_Viewer, SLOT(displayMessage(QString, bool)));

#endif
