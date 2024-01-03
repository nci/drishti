#ifndef CONNECTHIRES_H
#define CONNECTHIRES_H

  connect(m_keyFrame, SIGNAL(updateLightInfo(LightingInformation)),
	  m_Hires, SLOT(setLightInfo(LightingInformation)));

#endif
