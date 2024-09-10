#ifndef CONNECTCLIPPLANES_H
#define CONNECTCLIPPLANES_H

   connect(GeometryObjects::clipplanes(), SIGNAL(addClipper()),
	   m_bricksWidget, SLOT(addClipper()));

#endif
