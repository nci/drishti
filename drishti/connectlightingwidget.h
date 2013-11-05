#ifndef CONNECTLIGHTINGWIDGET_H
#define CONNECTLIGHTINGWIDGET_H

  connect(m_lightingWidget,
	  SIGNAL(directionChanged(Vec)),
	  this, SLOT(lightDirectionChanged(Vec)));
  connect(m_lightingWidget,
	  SIGNAL(lightDistanceOffset(float)),
	  this, SLOT(lightDistanceOffset(float)));

  connect(m_lightingWidget,
	  SIGNAL(applyEmissive(bool)),
	  this, SLOT(applyEmissive(bool)));

  connect(m_lightingWidget,
	  SIGNAL(applyLighting(bool)),
	  this, SLOT(applyLighting(bool)));
  connect(m_lightingWidget,
	  SIGNAL(highlights(Highlights)),
	  this, SLOT(highlights(Highlights)));

  connect(m_lightingWidget,
	  SIGNAL(applyShadow(bool)),
	  this, SLOT(applyShadow(bool)));
  connect(m_lightingWidget,
	  SIGNAL(shadowBlur(float)),
	  this, SLOT(shadowBlur(float)));
  connect(m_lightingWidget,
	  SIGNAL(shadowScale(float)),
	  this, SLOT(shadowScale(float)));
  connect(m_lightingWidget,
	  SIGNAL(shadowFOV(float)),
	  this, SLOT(shadowFOV(float)));
  connect(m_lightingWidget,
	  SIGNAL(shadowIntensity(float)),
	  this, SLOT(shadowIntensity(float)));

  connect(m_lightingWidget,
	  SIGNAL(applyColoredShadow(bool)),
	  this, SLOT(applyColoredShadow(bool)));
  connect(m_lightingWidget,
	  SIGNAL(shadowColorAttenuation(float, float, float)),
	  this, SLOT(shadowColorAttenuation(float, float, float)));

  connect(m_lightingWidget,
	  SIGNAL(applyBackplane(bool)),
	  this, SLOT(applyBackplane(bool)));
  connect(m_lightingWidget,
	  SIGNAL(backplaneShadowScale(float)),
	  this, SLOT(backplaneShadowScale(float)));
  connect(m_lightingWidget,
	  SIGNAL(backplaneIntensity(float)),
	  this, SLOT(backplaneIntensity(float)));

  connect(m_lightingWidget,
	  SIGNAL(peel(bool)),
	  this, SLOT(peel(bool)));
  connect(m_lightingWidget,
	  SIGNAL(peelInfo(int, float, float, float)),
	  this, SLOT(peelInfo(int, float, float, float)));

#endif
