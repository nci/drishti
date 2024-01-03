#ifndef CONNECTCLIPPLANES_H
#define CONNECTCLIPPLANES_H

   connect(GeometryObjects::clipplanes(), SIGNAL(addClipper()),
	   m_bricksWidget, SLOT(addClipper()));

   connect(GeometryObjects::clipplanes(), SIGNAL(removeClipper(int)),
	   m_bricksWidget, SLOT(removeClipper(int)));

#endif
