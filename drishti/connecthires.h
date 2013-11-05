#ifndef CONNECTHIRES_H
#define CONNECTHIRES_H

  connect(m_Hires, SIGNAL(histogramUpdated(QImage, QImage)),
	  m_tfEditor, SLOT(setHistogramImage(QImage, QImage)));



  connect(m_keyFrame, SIGNAL(updateLightInfo(LightingInformation)),
	  m_Hires, SLOT(setLightInfo(LightingInformation)));

  connect(m_keyFrame, SIGNAL(updateVolumeBounds(int, Vec, Vec)),
	  m_Hires, SLOT(updateSubvolume(int, Vec, Vec)));

  connect(m_keyFrame, SIGNAL(updateVolumeBounds(int, int, Vec, Vec)),
	  m_Hires, SLOT(updateSubvolume(int, int, Vec, Vec)));

  connect(m_keyFrame, SIGNAL(updateVolumeBounds(int, int, int, Vec, Vec)),
	  m_Hires, SLOT(updateSubvolume(int, int, int, Vec, Vec)));

  connect(m_keyFrame, SIGNAL(updateVolumeBounds(int, int, int, int, Vec, Vec)),
	  m_Hires, SLOT(updateSubvolume(int, int, int, int, Vec, Vec)));



  connect(m_gallery, SIGNAL(updateLightInfo(LightingInformation)),
	  m_Hires, SLOT(setLightInfo(LightingInformation)));

  connect(m_gallery, SIGNAL(updateVolumeBounds(int, Vec, Vec)),
	  m_Hires, SLOT(updateSubvolume(int, Vec, Vec)));

  connect(m_gallery, SIGNAL(updateVolumeBounds(int, int, Vec, Vec)),
	  m_Hires, SLOT(updateSubvolume(int, int, Vec, Vec)));

  connect(m_gallery, SIGNAL(updateVolumeBounds(int, int, int, Vec, Vec)),
	  m_Hires, SLOT(updateSubvolume(int, int, int, Vec, Vec)));

  connect(m_gallery, SIGNAL(updateVolumeBounds(int, int, int, int, Vec, Vec)),
	  m_Hires, SLOT(updateSubvolume(int, int, int, int, Vec, Vec)));

#endif
