#ifndef CONNECTLIGHTINGWIDGET_H
#define CONNECTLIGHTINGWIDGET_H

  connect(m_lightingWidget, SIGNAL(updateGL()),
	  m_Viewer, SLOT(update()));

  connect(m_lightingWidget,
	  SIGNAL(highlights(Highlights)),
	  this, SLOT(highlights(Highlights)));

  connect(m_lightingWidget,
	  SIGNAL(softness(float)),
	  this, SLOT(softness(float)));
  connect(m_lightingWidget,
	  SIGNAL(edges(float)),
	  this, SLOT(edges(float)));
  connect(m_lightingWidget,
	  SIGNAL(shadowIntensity(float)),
	  this, SLOT(shadowIntensity(float)));
  connect(m_lightingWidget,
	  SIGNAL(valleyIntensity(float)),
	  this, SLOT(valleyIntensity(float)));
  connect(m_lightingWidget,
	  SIGNAL(peakIntensity(float)),
	  this, SLOT(peakIntensity(float)));

  connect(m_lightingWidget,
	  SIGNAL(lightDirectionChanged(Vec)),
	  this, SLOT(lightDirectionChanged(Vec)));


#endif
